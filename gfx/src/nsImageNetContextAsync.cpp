/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include "libimg.h"
#include "nsImageNet.h"
#include "ilINetContext.h"
#include "ilIURL.h"
#include "ilINetReader.h"
#include "nsIStreamListener.h"
#include "nsIInputStream.h"
#include "nsIURL.h"
#include "nsILoadGroup.h"
#include "nsIChannel.h"
#include "nsCOMPtr.h"
#include "nsWeakPtr.h"
#include "prprf.h"

#include "nsITimer.h"
#include "nsVoidArray.h"
#include "nsString.h"
#include "prmem.h"
#include "plstr.h"
//#include "il_strm.h"
#include "merrors.h"

#include "nsNeckoUtil.h"

static NS_DEFINE_IID(kIImageNetContextIID, IL_INETCONTEXT_IID);
static NS_DEFINE_IID(kIURLIID, NS_IURL_IID);

#define IMAGE_BUF_SIZE 4096

class ImageConsumer;

class ImageNetContextImpl : public ilINetContext {
public:
  ImageNetContextImpl(NET_ReloadMethod aReloadPolicy,
                      nsILoadGroup* aLoadGroup,
                      nsReconnectCB aReconnectCallback,
                      void* aReconnectArg);
  virtual ~ImageNetContextImpl();

  NS_DECL_ISUPPORTS

  virtual ilINetContext* Clone();

  virtual NET_ReloadMethod GetReloadPolicy();
  virtual NET_ReloadMethod SetReloadPolicy(NET_ReloadMethod ReloadPolicy);


  virtual void AddReferer(ilIURL *aUrl);

  virtual void Interrupt();

  virtual ilIURL* CreateURL(const char *aUrl, 
			    NET_ReloadMethod aReloadMethod);

  virtual PRBool IsLocalFileURL(char *aAddress);
#ifdef NU_CACHE
  virtual PRBool IsURLInCache(ilIURL *aUrl);
#else /* NU_CACHE */
  virtual PRBool IsURLInMemCache(ilIURL *aUrl);

  virtual PRBool IsURLInDiskCache(ilIURL *aUrl);
#endif

  virtual int GetURL (ilIURL * aUrl, NET_ReloadMethod aLoadMethod,
		      ilINetReader *aReader);

  nsresult RequestDone(ImageConsumer *aConsumer, nsIChannel* channel,
                       nsISupports* ctxt, nsresult status, const PRUnichar* aMsg);

  nsVoidArray *mRequests;
  NET_ReloadMethod mReloadPolicy;
  nsWeakPtr mLoadGroup;
  nsReconnectCB mReconnectCallback;
  void* mReconnectArg;
};

class ImageConsumer : public nsIStreamListener
{
public:
  NS_DECL_ISUPPORTS
  
  ImageConsumer(ilIURL *aURL, ImageNetContextImpl *aContext);
  
  // nsIStreamObserver methods:
  NS_DECL_NSISTREAMOBSERVER
  NS_DECL_NSISTREAMLISTENER

  void SetKeepPumpingData(nsIChannel* channel, nsISupports* context) {
    NS_ADDREF(channel);
    NS_IF_RELEASE(mChannel);
    mChannel = channel;

    NS_IF_ADDREF(context);
    NS_IF_RELEASE(mUserContext);
    mUserContext = context;
  }
  
  void Interrupt();

  static void KeepPumpingStream(nsITimer *aTimer, void *aClosure);

protected:
  virtual ~ImageConsumer();
  ilIURL *mURL;
  PRBool mInterrupted;
  ImageNetContextImpl *mContext;
  nsIInputStream *mStream;
  nsITimer *mTimer;
  PRBool mFirstRead;
  char *mBuffer;
  PRInt32 mStatus;
  nsIChannel* mChannel;
  nsISupports* mUserContext;
};

ImageConsumer::ImageConsumer(ilIURL *aURL, ImageNetContextImpl *aContext)
{
  NS_INIT_REFCNT();
  mURL = aURL;
  NS_ADDREF(mURL);
  mContext = aContext;
  NS_ADDREF(mContext);
  mInterrupted = PR_FALSE;
  mFirstRead = PR_TRUE;
  mStream = nsnull;
  mTimer = nsnull;
  mBuffer = nsnull;
  mStatus = 0;
  mChannel = nsnull;
  mUserContext = nsnull;
}

NS_DEFINE_IID(kIStreamNotificationIID, NS_ISTREAMLISTENER_IID);
NS_IMPL_ISUPPORTS(ImageConsumer,kIStreamNotificationIID);

NS_IMETHODIMP
ImageConsumer::OnStartRequest(nsIChannel* channel, nsISupports* aContext)
{
  if (mInterrupted) {
    mStatus = MK_INTERRUPTED;
    return NS_ERROR_ABORT;
  }

  mBuffer = (char *)PR_MALLOC(IMAGE_BUF_SIZE);
  if (mBuffer == nsnull) {
    mStatus = MK_IMAGE_LOSSAGE;
    return NS_ERROR_ABORT;
  }

  ilINetReader *reader = mURL->GetReader(); //ptn test: nsCOMPtr??

  nsresult rv = NS_OK;
  char* aContentType = NULL;
  rv = channel->GetContentType(&aContentType); //nsCRT alloc's str
  if (NS_FAILED(rv)) {
      if(aContentType){
          nsCRT::free(aContentType);
      }
      aContentType = nsCRT::strdup("unknown");        
  }
  if(nsCRT::strlen(aContentType) > 50){
      //somethings wrong. mimetype string shouldn't be this big.
      //protect us from the user.
      nsCRT::free(aContentType);
      aContentType = nsCRT::strdup("unknown"); 
  }

  if (reader->StreamCreated(mURL, aContentType) != PR_TRUE) {
    mStatus = MK_IMAGE_LOSSAGE;
    reader->StreamAbort(mStatus);
    NS_RELEASE(reader);
    nsCRT::free(aContentType);
    return NS_ERROR_ABORT;
  }
  nsCRT::free(aContentType);
  NS_RELEASE(reader);
    
  return NS_OK;
}

#define IMAGE_BUF_SIZE 4096


NS_IMETHODIMP
ImageConsumer::OnDataAvailable(nsIChannel* channel, nsISupports* aContext, nsIInputStream *pIStream,
                               PRUint32 offset, PRUint32 length)
{
  PRUint32 max_read=0;
  PRUint32 bytes_read = 0;
  ilINetReader *reader = mURL->GetReader();

  if (mInterrupted || mStatus != 0) {
    mStatus = MK_INTERRUPTED;
    reader->StreamAbort(mStatus);
    NS_RELEASE(reader);
    return NS_ERROR_ABORT;
  }

  nsresult err = 0;
  PRUint32 nb;

  do {
    err = reader->WriteReady(&max_read); //max read is most decoder can handle
    if(NS_FAILED(err))   //note length tells how much we already have.
        break;

    if(max_read < 0){
            max_read = 128;
    }

    if (max_read > (length-bytes_read)) {
      max_read = length-bytes_read;
    }

    if (max_read > IMAGE_BUF_SIZE) {
      max_read = IMAGE_BUF_SIZE;
    }
    
    // make sure there's enough data available to decode the image.
    // put test into WriteReady
    if (mFirstRead && length < 4)
      break;

    err = pIStream->Read(mBuffer,
                         max_read, &nb);

    if (err == NS_BASE_STREAM_WOULD_BLOCK) {
      NS_ASSERTION(nb == 0, "Data will be lost.");
      err = NS_OK;
      break;
    }
    if (NS_FAILED(err) || nb == 0) {
      NS_ASSERTION(nb == 0, "Data will be lost.");
      break;
    }

    bytes_read += nb;
    if (mFirstRead == PR_TRUE) {
            
      err = reader->FirstWrite((const unsigned char *)mBuffer, nb );
      mFirstRead = PR_FALSE; //? move after err chk?
      /* 
       * If FirstWrite(...) fails then the image type
       * cannot be determined and the il_container 
       * stream functions have not been initialized!
       */
      if (NS_FAILED(err)) {
        mStatus = MK_IMAGE_LOSSAGE;
        mInterrupted = PR_TRUE;
	    NS_RELEASE(reader);
        return NS_ERROR_ABORT;
      }
    }

    err = reader->Write((const unsigned char *)mBuffer, (int32)nb);
    if(NS_FAILED(err)){
      mStatus = MK_IMAGE_LOSSAGE;
      mInterrupted = PR_TRUE;
	  NS_RELEASE(reader);
      return NS_ERROR_ABORT;
    }
  } while(bytes_read < length);

  if (NS_FAILED(err)) {
    mStatus = MK_IMAGE_LOSSAGE;
    mInterrupted = PR_TRUE;
  }

  if (bytes_read < length) {
    // If we haven't emptied the stream, hold onto it, because
    // we will need to read from it subsequently and we don't
    // know if we'll get a OnDataAvailable call again.
    //
    // Addref the new stream before releasing the old one,
    // in case it is the same stream!
    NS_ADDREF(pIStream);
    NS_IF_RELEASE(mStream);
    mStream = pIStream;
  } else {
    NS_IF_RELEASE(mStream);
  }
  NS_RELEASE(reader);
  return NS_OK;
}

void
ImageConsumer::KeepPumpingStream(nsITimer *aTimer, void *aClosure)
{
  ImageConsumer *consumer = (ImageConsumer *)aClosure;
  nsAutoString status;

  consumer->OnStopRequest(consumer->mChannel, consumer->mUserContext,
                          NS_BINDING_SUCCEEDED, status.GetUnicode());
}

NS_IMETHODIMP
ImageConsumer::OnStopRequest(nsIChannel* channel, nsISupports* aContext, nsresult status, const PRUnichar* aMsg)
{
  if (mTimer != nsnull) {
    NS_RELEASE(mTimer);
  }

  if (NS_BINDING_SUCCEEDED != status) {
    mStatus = MK_INTERRUPTED;
  }

  // Since we're still holding on to the stream, there's still data
  // that needs to be read. So, pump the stream ourselves.
  if((mStream != nsnull) && (status == NS_BINDING_SUCCEEDED)) {
    PRUint32 str_length;
    nsresult err = mStream->Available(&str_length);
    if (NS_SUCCEEDED(err)) {
      NS_ASSERTION((str_length > 0), "No data left in the stream!");
      err = OnDataAvailable(channel, aContext, mStream, 0, str_length);  // XXX fix offset
      if (NS_SUCCEEDED(err)) {
        // If we still have the stream, there's still data to be 
        // pumped, so we set a timer to call us back again.
        if (mStream) {
          SetKeepPumpingData(channel, aContext);

          if ((NS_OK != NS_NewTimer(&mTimer)) ||
              (NS_OK != mTimer->Init(ImageConsumer::KeepPumpingStream, this, 0))) {
            mStatus = MK_IMAGE_LOSSAGE;
            NS_RELEASE(mStream);
          }
          else {
            return NS_OK;
          }
        }
      }
      else {
        mStatus = MK_IMAGE_LOSSAGE;
        NS_IF_RELEASE(mStream);
      }
    }
    else {
      mStatus = MK_IMAGE_LOSSAGE;
      NS_IF_RELEASE(mStream);
    }
  }

  ilINetReader *reader = mURL->GetReader();
  if (0 != mStatus) {
    reader->StreamAbort(mStatus);
  }
  else {
    reader->StreamComplete(PR_FALSE);
  }
  reader->NetRequestDone(mURL, mStatus);
  NS_RELEASE(reader);
  
  return mContext->RequestDone(this, channel, aContext, status, aMsg);
}

void
ImageConsumer::Interrupt()
{
  mInterrupted = PR_TRUE;
}

ImageConsumer::~ImageConsumer()
{
  NS_IF_RELEASE(mURL);
  NS_IF_RELEASE(mContext);
  NS_IF_RELEASE(mStream);
  NS_IF_RELEASE(mTimer);
  if (mBuffer != nsnull) {
    PR_DELETE(mBuffer);
  }
  NS_IF_RELEASE(mChannel);
  NS_IF_RELEASE(mUserContext);
}

ImageNetContextImpl::ImageNetContextImpl(NET_ReloadMethod aReloadPolicy,
                                         nsILoadGroup* aLoadGroup,
                                         nsReconnectCB aReconnectCallback,
                                         void* aReconnectArg)
{
  NS_INIT_REFCNT();
  mRequests = nsnull;
  mLoadGroup = getter_AddRefs(NS_GetWeakReference(aLoadGroup));
  mReloadPolicy = aReloadPolicy;
  mReconnectCallback = aReconnectCallback;
  mReconnectArg = aReconnectArg;
}

ImageNetContextImpl::~ImageNetContextImpl()
{
  if (mRequests != nsnull) {
    int i, count = mRequests->Count();
    for (i=0; i < count; i++) {
      ImageConsumer *ic = (ImageConsumer *)mRequests->ElementAt(i);
          
      NS_RELEASE(ic);
    }
    delete mRequests;
  }
///  NS_IF_RELEASE(mLoadGroup);
}

NS_IMPL_ISUPPORTS(ImageNetContextImpl, kIImageNetContextIID)

ilINetContext* 
ImageNetContextImpl::Clone()
{
  ilINetContext *cx;
  nsCOMPtr<nsILoadGroup> group = do_QueryReferent(mLoadGroup);

  if (NS_NewImageNetContext(&cx, group, mReconnectCallback, mReconnectArg) == NS_OK)
  {
    return cx;
  }
  else {
    return nsnull;
  }
}

NET_ReloadMethod 
ImageNetContextImpl::GetReloadPolicy()
{
  return mReloadPolicy;
}

NET_ReloadMethod 
ImageNetContextImpl::SetReloadPolicy(NET_ReloadMethod reloadpolicy)
{
  mReloadPolicy=reloadpolicy;
  return mReloadPolicy;
}

void 
ImageNetContextImpl::AddReferer(ilIURL *aUrl)
{
}

void 
ImageNetContextImpl::Interrupt()
{
  if (mRequests != nsnull) {
    int i, count = mRequests->Count();
    for (i=0; i < count; i++) {
      ImageConsumer *ic = (ImageConsumer *)mRequests->ElementAt(i);
      ic->Interrupt();
    }
  }
}

ilIURL* 
ImageNetContextImpl::CreateURL(const char *aURL, 
                               NET_ReloadMethod aReloadMethod)
{
  ilIURL *url;
  nsCOMPtr<nsILoadGroup> group = do_QueryReferent(mLoadGroup);

  if (NS_NewImageURL(&url, aURL, group) == NS_OK)
  {
    return url;
  }
  else {
    return nsnull;
  }
}

PRBool 
ImageNetContextImpl::IsLocalFileURL(char *aAddress)
{
  if (PL_strncasecmp(aAddress, "file:", 5) == 0) {
    return PR_TRUE;
  }
  else {
    return PR_FALSE;
  }
}

#ifdef NU_CACHE
PRBool 
ImageNetContextImpl::IsURLInCache(ilIURL *aUrl)
{
  return PR_TRUE;
}
#else /* NU_CACHE */
PRBool 
ImageNetContextImpl::IsURLInMemCache(ilIURL *aUrl)
{
  return PR_FALSE;
}

PRBool 
ImageNetContextImpl::IsURLInDiskCache(ilIURL *aUrl)
{
  return PR_TRUE;
}
#endif /* NU_CACHE */

int 
ImageNetContextImpl::GetURL (ilIURL * aURL, 
                             NET_ReloadMethod aLoadMethod,
                             ilINetReader *aReader)
{
  NS_PRECONDITION(nsnull != aURL, "null URL");
  NS_PRECONDITION(nsnull != aReader, "null reader");
  if (aURL == nsnull || aReader == nsnull) {
    return -1;
  }

  if (mRequests == nsnull) {
    mRequests = new nsVoidArray();
    if (mRequests == nsnull) {
      // XXX Should still call exit function
      return -1;
    }
  }

  nsresult rv;
  nsCOMPtr<nsIURI> nsurl = do_QueryInterface(aURL, &rv);
  if (NS_FAILED(rv)) return 0;

  aURL->SetReader(aReader);
  SetReloadPolicy(aLoadMethod);    
  
  // Find previously created ImageConsumer if possible
  ImageConsumer *ic = new ImageConsumer(aURL, this);
  if (ic == nsnull)
    return -1;
  NS_ADDREF(ic);
        
  // See if a reconnect is being done...(XXX: hack!)
  if (mReconnectCallback == nsnull
      || !(*mReconnectCallback)(mReconnectArg, ic)) {
    nsCOMPtr<nsIChannel> channel;
    nsCOMPtr<nsILoadGroup> group = do_QueryReferent(mLoadGroup);
    rv = NS_OpenURI(getter_AddRefs(channel), nsurl, group);
    if (NS_FAILED(rv)) goto error;

    PRBool bIsBackground = aURL->GetBackgroundLoad();
    if (bIsBackground) {
      (void)channel->SetLoadAttributes(nsIChannel::LOAD_BACKGROUND);
    }
    rv = channel->AsyncRead(0, -1, nsnull, ic);
    if (NS_FAILED(rv)) goto error;
  }
  return mRequests->AppendElement((void *)ic) ? 0 : -1;

error:
  NS_RELEASE(ic);
  return -1;
}

nsresult
ImageNetContextImpl::RequestDone(ImageConsumer *aConsumer, nsIChannel* channel,
                                 nsISupports* ctxt, nsresult status, const PRUnichar* aMsg)
{
  if (mRequests != nsnull) {
    if (mRequests->RemoveElement((void *)aConsumer) == PR_TRUE) {
      NS_RELEASE(aConsumer);
    }
  }
///  if (mLoadGroup)
///    return mLoadGroup->RemoveChannel(channel, ctxt, status, aMsg);
///  else
    return NS_OK;
}

extern "C" NS_GFX_(nsresult)
NS_NewImageNetContext(ilINetContext **aInstancePtrResult,
                      nsILoadGroup* aLoadGroup,
                      nsReconnectCB aReconnectCallback,
                      void* aReconnectArg)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null ptr");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }
  
  ilINetContext *cx = new ImageNetContextImpl(NET_NORMAL_RELOAD,
                                              aLoadGroup,
                                              aReconnectCallback,
                                              aReconnectArg);
  if (cx == nsnull) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return cx->QueryInterface(kIImageNetContextIID, (void **) aInstancePtrResult);
}
