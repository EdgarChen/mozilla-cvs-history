/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * Copyright (C) 1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include "nsURILoader.h"
#include "nsIURIContentListener.h"
#include "nsIContentHandler.h"
#include "nsILoadGroup.h"
#include "nsIIOService.h"
#include "nsIServiceManager.h"
#include "nsIStreamListener.h"
#include "nsIChannel.h"
#include "nsIEventSinkGetter.h"
#include "nsIProgressEventSink.h"


#include "nsVoidArray.h"
#include "nsXPIDLString.h"
#include "nsString.h"

static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);
static NS_DEFINE_CID(kURILoaderCID, NS_URI_LOADER_CID);

// mscott - i ripped this event sink getter class off of the one in the old
// webshell doc loader...

class nsUriLoaderEventSinkGetter : public nsIEventSinkGetter {
public:
  NS_DECL_ISUPPORTS

  nsUriLoaderEventSinkGetter(nsIProgressEventSink *aProgressEventsink) {
    NS_INIT_REFCNT();
    mProgressEventSink = aProgressEventsink;
  }

  virtual ~nsUriLoaderEventSinkGetter() {};

  NS_IMETHOD GetEventSink(const char* aVerb, const nsIID& anIID, nsISupports** aSink) {
    if (mProgressEventSink) {
        return mProgressEventSink->QueryInterface(anIID, (void**)aSink);
    }
    return NS_ERROR_FAILURE;
  }
private:
  nsCOMPtr<nsIProgressEventSink> mProgressEventSink;
};

NS_IMPL_ISUPPORTS1(nsUriLoaderEventSinkGetter, nsIEventSinkGetter);

/* 
 * The nsDocumentOpenInfo contains the state required when a single document
 * is being opened in order to discover the content type...  Each instance remains alive until its target URL has 
 * been loaded (or aborted).
 *
 */
class nsDocumentOpenInfo : public nsIStreamListener
{
public:
    nsDocumentOpenInfo();

    nsresult Init(nsIURIContentListener * aContentListener);

    NS_DECL_ISUPPORTS

    nsresult Open(nsIURI *aURL, 
                  const char * aWindowTarget,
                  nsIProgressEventSink * aProgressEventSink,
                  nsIURI * aReferringURI,
                  nsISupports * aOpenContext,
                  nsISupports ** aCurrentOpenContext);

    // nsIStreamObserver methods:
    NS_DECL_NSISTREAMOBSERVER

	  // nsIStreamListener methods:
    NS_DECL_NSISTREAMLISTENER

protected:
    virtual ~nsDocumentOpenInfo();

protected:
    nsCOMPtr<nsIURIContentListener> m_contentListener;
    nsCOMPtr<nsIChannel> m_channel;
    nsCOMPtr<nsIStreamListener> m_targetStreamListener;
    nsCString m_windowTarget;
};

NS_IMPL_ISUPPORTS2(nsDocumentOpenInfo, nsIStreamObserver, nsIStreamListener);

nsDocumentOpenInfo::nsDocumentOpenInfo()
{
  NS_INIT_ISUPPORTS();
}

nsDocumentOpenInfo::~nsDocumentOpenInfo()
{
}

nsresult nsDocumentOpenInfo::Init(nsIURIContentListener * aContentListener)
{
  m_contentListener = aContentListener;
  return NS_OK;
}

nsresult nsDocumentOpenInfo::Open(nsIURI *aURI, 
                                  const char * aWindowTarget,
                                  nsIProgressEventSink * aProgressEventSink,
                                  nsIURI * aReferringURI,
                                  nsISupports * aOpenContext,
                                  nsISupports ** aCurrentOpenContext)
{
   // this method is not complete!!! Eventually, we should first go
  // to the content listener and ask them for a protocol handler...
  // if they don't give us one, we need to go to the registry and get
  // the preferred protocol handler. 

  // But for now, I'm going to let necko do the work for us....

  // store any local state
  m_windowTarget = aWindowTarget;

  // turn the arguments we received into something we can pass into NewChannelFromURI:
  // that means turning the progress event sink into an event sink getter...
  nsCOMPtr<nsIEventSinkGetter> aEventSinkGetter = new nsUriLoaderEventSinkGetter(aProgressEventSink);
  // and get the load group out of the open context
  nsCOMPtr<nsILoadGroup> aLoadGroup = do_QueryInterface(aOpenContext);
  if (!aLoadGroup)
  { 
    // i haven't implemented this yet...it's going to be hard
    // because it requires a persistant stream observer (the uri loader maybe???)
    // that we don't have in this architecture in order to create a new load group
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  if (aCurrentOpenContext)
    aLoadGroup->QueryInterface(NS_GET_IID(nsISupports), (void **) aCurrentOpenContext);

  // now we have all we need, so go get the necko channel service so we can 
  // open the url. 

  nsresult rv = NS_OK;
  NS_WITH_SERVICE(nsIIOService, pNetService, kIOServiceCID, &rv);
  if (NS_SUCCEEDED(rv))
  {
    nsCOMPtr<nsIChannel> m_channel;
    rv = pNetService->NewChannelFromURI("", aURI, aLoadGroup, aEventSinkGetter, aReferringURI, getter_AddRefs(m_channel));
    if (NS_FAILED(rv)) return rv; // uhoh we were unable to get a channel to handle the url!!!
    rv =  m_channel->AsyncRead(0, -1, nsnull, this);
  }

  return rv;
}

NS_IMETHODIMP nsDocumentOpenInfo::OnStartRequest(nsIChannel * aChannel, nsISupports * aCtxt)
{
  nsresult rv = NS_OK;

  nsXPIDLCString aContentType;
  rv = aChannel->GetContentType(getter_Copies(aContentType));
  if (NS_FAILED(rv)) return rv;

  // go to the uri dispatcher and give them our stuff...
  NS_WITH_SERVICE(nsIURILoader, pURILoader, kURILoaderCID, &rv);
  if (NS_SUCCEEDED(rv))
  {
    rv = pURILoader->DispatchContent(aContentType, "view", m_windowTarget, 
                                     aChannel, aCtxt, m_contentListener, getter_AddRefs(m_targetStreamListener));
    if (m_targetStreamListener)
      m_targetStreamListener->OnStartRequest(aChannel, aCtxt);
  }
  return rv;
}

NS_IMETHODIMP nsDocumentOpenInfo::OnDataAvailable(nsIChannel * aChannel, nsISupports * aCtxt,
                                                  nsIInputStream * inStr, PRUint32 sourceOffset, PRUint32 count)
{
  // if we have retarged to the end stream listener, then forward the call....
  // otherwise, don't do anything

  nsresult rv = NS_OK;
  if (m_targetStreamListener)
    rv = m_targetStreamListener->OnDataAvailable(aChannel, aCtxt, inStr, sourceOffset, count);
  return rv;
}

NS_IMETHODIMP nsDocumentOpenInfo::OnStopRequest(nsIChannel * aChannel, nsISupports *aCtxt, 
                                                nsresult aStatus, const PRUnichar * errorMsg)
{
  if (m_targetStreamListener)
    m_targetStreamListener->OnStopRequest(aChannel, aCtxt, aStatus, errorMsg);

  return NS_OK;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Implementation of nsURILoader
///////////////////////////////////////////////////////////////////////////////////////////////

nsURILoader::nsURILoader()
{
  NS_INIT_ISUPPORTS();
  m_listeners = new nsVoidArray();
}

nsURILoader::~nsURILoader()
{
  if (m_listeners)
    delete m_listeners;
}

NS_IMPL_ISUPPORTS1(nsURILoader, nsIURILoader)

NS_IMETHODIMP nsURILoader::RegisterContentListener(nsIURIContentListener * aContentListener)
{
  nsresult rv = NS_OK;
  if (m_listeners)
    m_listeners->AppendElement(aContentListener);
  else
    rv = NS_ERROR_FAILURE;

  return rv;
} 

NS_IMETHODIMP nsURILoader::UnRegisterContentListener(nsIURIContentListener * aContentListener)
{
  if (m_listeners)
    m_listeners->RemoveElement(aContentListener);
  return NS_OK;
  
}

NS_IMETHODIMP nsURILoader::OpenURI(nsIURI *aURI, 
                                   const char * aWindowTarget,
                                   nsIProgressEventSink *aProgressEventSink, 
                                   nsIURIContentListener *aContentListener,
                                   nsIURI *aReferringURI,
                                   nsISupports *aOpenContext, 
                                   nsISupports **aCurrentOpenContext)
{
  return OpenURIVia(aURI, aWindowTarget, aProgressEventSink,
                    aContentListener, aReferringURI, aOpenContext, aCurrentOpenContext,
                    0 /* ip address */); 
}

NS_IMETHODIMP nsURILoader::OpenURIVia(nsIURI *aURI, 
                                      const char * aWindowTarget,
                                      nsIProgressEventSink *aProgressEventSink, 
                                      nsIURIContentListener *aContentListener,
                                      nsIURI *aReferringURI,
                                      nsISupports *aOpenContext, 
                                      nsISupports **aCurrentOpenContext,
                                      const PRUint32 aLocalIP)
{
  // we need to create a DocumentOpenInfo object which will go ahead and open the url
  // and discover the content type....

  nsresult rv = NS_OK;
  nsDocumentOpenInfo* loader = nsnull;

  if (!aURI) return NS_ERROR_NULL_POINTER;

  NS_NEWXPCOM(loader, nsDocumentOpenInfo);
  if (!loader) return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(loader);
  loader->Init(aContentListener);    // Extra Info

  // now instruct the loader to go ahead and open the url
  rv = loader->Open(aURI, aWindowTarget, aProgressEventSink,  
                    aReferringURI, aOpenContext, aCurrentOpenContext);
  NS_RELEASE(loader);

  return NS_OK;
}

nsresult nsURILoader::DispatchContent(const char * aContentType,
                                      const char * aCommand,
                                      const char * aWindowTarget,
                                      nsIChannel * aChannel, 
                                      nsISupports * aCtxt, 
                                      nsIURIContentListener * aContentListener,
                                      nsIStreamListener ** aTargetListener)
{
  // okay, now we've discovered the content type. We need to do the following:
  // (1) Give our uri content listener first crack at handling this content type.  
  nsresult rv = NS_OK;

  nsCOMPtr<nsIURIContentListener> listenerToUse = aContentListener;
  // find a content handler that can and will handle the content
  PRBool canHandleContent = PR_FALSE;
  if (listenerToUse)
    listenerToUse->CanHandleContent(aContentType, aCommand, aWindowTarget, &canHandleContent);
  if (!canHandleContent) // if it can't handle the content, scan through the list of registered listeners
  {
     PRInt32 i = 0;
     // keep looping until we are told to abort or we get a content listener back
     for(i = 0; i < m_listeners->Count() && !canHandleContent; i++)
	   {
	      //nsIURIContentListener's aren't refcounted.
		    nsIURIContentListener * listener =(nsIURIContentListener*)m_listeners->ElementAt(i);
        if (listener)
         {
            rv = listener->CanHandleContent(aContentType, aCommand, aWindowTarget, &canHandleContent);
            if (canHandleContent)
              listenerToUse = listener;
         }
    } // for loop
  } // if we can't handle the content


  if (canHandleContent && listenerToUse)
  {
      nsCOMPtr<nsIStreamListener> aContentStreamListener;
      PRBool aAbortProcess = PR_FALSE;
      rv = listenerToUse->DoContent(aContentType, aCommand, aWindowTarget, 
                                    aChannel, getter_AddRefs(aContentStreamListener),
                                    &aAbortProcess);

      // the listener is doing all the work from here...we are done!!!
      if (aAbortProcess) return rv;


      // okay, all registered listeners have had a chance to handle this content...
      // did one of them give us a stream listener back? if so, let's start reading data
      // into it...
      if (aContentStreamListener)
      {
        *aTargetListener = aContentStreamListener;
        NS_IF_ADDREF(*aTargetListener);
        return rv;
      }
  }

  // no registered content listeners to handle this type!!! so go to the register 
  // and get a registered nsIContentHandler for our content type. Hand it off 
  // to them...
  // eventually we want to hit up the category manager so we can allow people to
  // over ride the default content type handlers....for now...i'm skipping that part.

  nsCAutoString handlerProgID (NS_CONTENT_HANDLER_PROGID_PREFIX);
  handlerProgID += aContentType;
  
  nsCOMPtr<nsIContentHandler> aContentHandler;
  rv = nsComponentManager::CreateInstance(handlerProgID, nsnull, NS_GET_IID(nsIContentHandler), getter_AddRefs(aContentHandler));
  if (NS_SUCCEEDED(rv)) // we did indeed have a content handler for this type!! yippee...
    rv = aContentHandler->HandleContent(aContentType, aCommand, aWindowTarget, aChannel);
  return rv;
}

