/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */
#include "nsCOMPtr.h"
#include "nsMsgCopy.h"
#include "nsIPref.h"
#include "nsMsgCompPrefs.h"
#include "nsMsgSendLater.h"
#include "nsIEnumerator.h"
#include "nsIFileSpec.h"
#include "nsISmtpService.h"
#include "nsIMsgMailNewsUrl.h"
#include "nsMsgDeliveryListener.h"
#include "nsIMsgIncomingServer.h"
#include "nsICopyMessageListener.h"
#include "nsIMsgMessageService.h"
#include "nsIMsgMailSession.h"
#include "nsIMsgAccountManager.h"
#include "nsMsgBaseCID.h"
#include "nsMsgCompCID.h"
#include "nsMsgCompUtils.h"
#include "nsMsgUtils.h"
#include "nsMsgFolderFlags.h"
#include "nsIMessage.h"
#include "nsIRDFResource.h"
#include "nsISupportsArray.h"
#include "nsMailHeaders.h"
#include "nsMsgPrompts.h"
#include "nsIMsgSendListener.h"
#include "nsIMsgSendLaterListener.h"
#include "nsMsgCopy.h"
#include "nsMsgComposeStringBundle.h"

static NS_DEFINE_CID(kPrefCID, NS_PREF_CID);
static NS_DEFINE_CID(kCMsgMailSessionCID, NS_MSGMAILSESSION_CID);
static NS_DEFINE_CID(kSmtpServiceCID, NS_SMTPSERVICE_CID);
static NS_DEFINE_IID(kIMsgSendLater, NS_IMSGSENDLATER_IID);
static NS_DEFINE_CID(kMsgCompFieldsCID, NS_MSGCOMPFIELDS_CID); 
static NS_DEFINE_CID(kMsgSendCID, NS_MSGSEND_CID); 
static NS_DEFINE_CID(kISupportsArrayCID, NS_SUPPORTSARRAY_CID);

// 
// This function will be used by the factory to generate the 
// nsMsgComposeAndSend Object....
//
nsresult NS_NewMsgSendLater(const nsIID &aIID, void ** aInstancePtrResult)
{
	/* note this new macro for assertions...they can take a string describing the assertion */
	NS_PRECONDITION(nsnull != aInstancePtrResult, "nsnull ptr");
	if (nsnull != aInstancePtrResult)
	{
		nsMsgSendLater *pSendLater = new nsMsgSendLater();
		if (pSendLater)
			return pSendLater->QueryInterface(kIMsgSendLater, aInstancePtrResult);
		else
			return NS_ERROR_OUT_OF_MEMORY; /* we couldn't allocate the object */
	}
	else
		return NS_ERROR_NULL_POINTER; /* aInstancePtrResult was NULL....*/
}

NS_IMPL_ISUPPORTS(nsMsgSendLater, nsCOMTypeInfo<nsIMsgSendLater>::GetIID())

nsMsgSendLater::nsMsgSendLater()
{
  mIdentity = nsnull;  
  mTempIFileSpec = nsnull;
  mTempFileSpec = nsnull;
  mHackTempIFileSpec = nsnull;
  mHackTempFileSpec = nsnull;
  mOutFile = nsnull;
  mEnumerator = nsnull;
  mFirstTime = PR_TRUE;
  mTotalSentSuccessfully = 0;
  mTotalSendCount = 0;
  mMessageFolder = nsnull;
  mMessage = nsnull;
  mLeftoverBuffer = nsnull;

  mSendListener = nsnull;
  mListenerArray = nsnull;
  mListenerArrayCount = 0;

  m_to = nsnull;
  m_bcc = nsnull;
  m_fcc = nsnull;
  m_newsgroups = nsnull;
  m_newshost = nsnull;
  m_headers = nsnull;
  m_flags = 0;
  m_headersFP = 0;
  m_inhead = PR_TRUE;
  m_headersPosition = 0;

  m_bytesRead = 0;
  m_position = 0;
  m_flagsPosition = 0;
  m_headersSize = 0;

  mSaveListener = nsnull;
  mRequestReturnReceipt = PR_FALSE;
  NS_INIT_REFCNT();
}

nsMsgSendLater::~nsMsgSendLater()
{
  if (mEnumerator)
    NS_RELEASE(mEnumerator);
  if (mTempIFileSpec)
    NS_RELEASE(mTempIFileSpec);
  if (mSaveListener)
    NS_RELEASE(mSaveListener);
  PR_FREEIF(m_to);
  PR_FREEIF(m_fcc);
  PR_FREEIF(m_bcc);
  PR_FREEIF(m_newsgroups);
  PR_FREEIF(m_newshost);
  PR_FREEIF(m_headers);
  PR_FREEIF(mLeftoverBuffer);

  NS_IF_RELEASE(mSendListener);
  NS_IF_RELEASE(mIdentity);
}

// Stream is done...drive on!
nsresult
nsMsgSendLater::Close(void) 
{
  // First, this shouldn't happen, but if
  // it does, flush the buffer and move on.
  if (mLeftoverBuffer)
  {
    DeliverQueuedLine(mLeftoverBuffer, PL_strlen(mLeftoverBuffer));
  }

  if (mOutFile)
    mOutFile->close();

  // Message is done...send it!
  return CompleteMailFileSend();
}

char *
FindEOL(char *inBuf, char *buf_end)
{
  char *buf = inBuf;
  char *findLoc = nsnull;

  while (buf <= buf_end)
    if (*buf == 0) 
      return buf;
    else if ( (*buf == LF) || (*buf == CR) )
    {
      findLoc = buf;
      break;
    }
    else
      ++buf;

  if (!findLoc)
    return nsnull;
  else if ((findLoc + 1) > buf_end)
    return buf;

  if ( (*findLoc == LF && *(findLoc+1) == CR) || 
       (*findLoc == CR && *(findLoc+1) == LF))
    findLoc++; // possibly a pair.       
  return findLoc;
}

nsresult
nsMsgSendLater::RebufferLeftovers(char *startBuf, PRUint32 aLen)
{
  PR_FREEIF(mLeftoverBuffer);
  mLeftoverBuffer = (char *)PR_Malloc(aLen + 1);
  if (!mLeftoverBuffer)
    return NS_ERROR_OUT_OF_MEMORY;

  nsCRT::memcpy(mLeftoverBuffer, startBuf, aLen);
  mLeftoverBuffer[aLen] = '\0';
  return NS_OK;
}

nsresult
nsMsgSendLater::BuildNewBuffer(const char* aBuf, PRUint32 aCount, PRUint32 *totalBufSize)
{
  // Only build a buffer when there are leftovers...
  if (!mLeftoverBuffer)
    return NS_ERROR_FAILURE;

  PRInt32 leftoverSize = PL_strlen(mLeftoverBuffer);
  mLeftoverBuffer = (char *)PR_Realloc(mLeftoverBuffer, aCount + leftoverSize);
  if (!mLeftoverBuffer)
    return NS_ERROR_FAILURE;

  nsCRT::memcpy(mLeftoverBuffer + leftoverSize, aBuf, aCount);
  *totalBufSize = aCount + leftoverSize;
  return NS_OK;
}

// Got data?
nsresult
nsMsgSendLater::Write(const char* aBuf, PRUint32 aCount, PRUint32 *aWriteCount) 
{
  // This is a little bit tricky since we have to chop random 
  // buffers into lines and deliver the lines...plus keeping the
  // leftovers for next time...some fun, eh?
  //
  nsresult    rv = NS_OK;
  char        *startBuf; 
  char        *endBuf;
  char        *lineEnd;
  char        *newbuf = nsnull;
  PRUint32     size;

  // First, create a new work buffer that will 
  if (NS_FAILED(BuildNewBuffer(aBuf, aCount, &size))) // no leftovers...
  {
    startBuf = (char *)aBuf;
    endBuf = (char *)(aBuf + aCount - 1);
  }
  else  // yum, leftovers...new buffer created...sitting in mLeftoverBuffer
  {
    newbuf = mLeftoverBuffer;
    startBuf = newbuf; 
    endBuf = startBuf + size - 1;
    mLeftoverBuffer = nsnull; // null out this 
  }

  while (startBuf < endBuf)
  {
    lineEnd = FindEOL(startBuf, endBuf);
    if (!lineEnd)
    {
      rv = RebufferLeftovers(startBuf, (endBuf - startBuf) + 1);           
      break;
    }

    rv = DeliverQueuedLine(startBuf, (lineEnd - startBuf) + 1);
    if (NS_FAILED(rv))
      break;

    startBuf = lineEnd+1;
  }

  if (newbuf)
    PR_FREEIF(newbuf);

  *aWriteCount = aCount;
  return rv;
}

nsresult
nsMsgSendLater::Flush(void) 
{
  return NS_OK;
}

nsresult
SaveMessageCompleteCallback(nsIURI *aUrl, nsresult aExitCode, void *tagData)
{
  nsresult rv = NS_OK;

  if (tagData)
  {
    nsMsgSendLater *ptr = (nsMsgSendLater *) tagData;
    if (NS_SUCCEEDED(aExitCode))
    {
#ifdef NS_DEBUG
      printf("nsMsgSendLater: Success on the message save...\n");
#endif
      // RICHIE 
      // Drive the file IO to the stream..which is us :-)
      rv = ptr->DriveFakeStream(ptr);

      // If the send operation failed..try the next one...
      if (NS_FAILED(rv))
        rv = ptr->StartNextMailFileSend();

      NS_RELEASE(ptr);
    }
    else
    {
      // RICHIE - do we do the message loss here?
      nsMsgDisplayMessageByString("Failed to get message from unsent folder.");

      // Save failed, but we will still keep trying to send the rest...
      rv = ptr->StartNextMailFileSend();
      NS_RELEASE(ptr);
    }
  }

  return rv;
}

////////////////////////////////////////////////////////////////////////////////////
// This is the listener class for the send operation. We have to create this class 
// to listen for message send completion and eventually notify the caller
////////////////////////////////////////////////////////////////////////////////////
NS_IMPL_ISUPPORTS(SendOperationListener, nsCOMTypeInfo<nsIMsgSendListener>::GetIID());

SendOperationListener::SendOperationListener(void) 
{ 
  mSendLater = nsnull;
  NS_INIT_REFCNT(); 
}

SendOperationListener::~SendOperationListener(void) 
{
}

nsresult
SendOperationListener::SetSendLaterObject(nsMsgSendLater *obj)
{
  mSendLater = obj;
  return NS_OK;
}

nsresult
SendOperationListener::OnStartSending(const char *aMsgID, PRUint32 aMsgSize)
{
#ifdef NS_DEBUG
  printf("SendOperationListener::OnStartSending()\n");
#endif
  return NS_OK;
}
  
nsresult
SendOperationListener::OnProgress(const char *aMsgID, PRUint32 aProgress, PRUint32 aProgressMax)
{
#ifdef NS_DEBUG
  printf("SendOperationListener::OnProgress()\n");
#endif
  return NS_OK;
}

nsresult
SendOperationListener::OnStatus(const char *aMsgID, const PRUnichar *aMsg)
{
#ifdef NS_DEBUG
  printf("SendOperationListener::OnStatus()\n");
#endif

  return NS_OK;
}
  
nsresult
SendOperationListener::OnStopSending(const char *aMsgID, nsresult aStatus, const PRUnichar *aMsg, 
                                     nsIFileSpec *returnFileSpec)
{
  nsresult                    rv = NS_OK;

  if (mSendLater)
  {
    if (NS_SUCCEEDED(aStatus))
    {
#ifdef NS_DEBUG
      printf("nsMsgSendLater: Success on the message send operation!\n");
#endif

      PRBool    deleteMsgs = PR_TRUE;

      //
      // Now delete the message from the outbox folder.
      //
      NS_WITH_SERVICE(nsIPref, prefs, kPrefCID, &rv); 
      if (NS_SUCCEEDED(rv) && prefs)
    		prefs->GetBoolPref("mail.really_delete_unsent_messages", &deleteMsgs);

      if (deleteMsgs)
        mSendLater->DeleteCurrentMessage();

      ++(mSendLater->mTotalSentSuccessfully);
    }
    else
    {
      // RICHIE - do we do the message loss here?
      nsMsgDisplayMessageByString("Sending of message failed.");
    }

    // Regardless, we will still keep trying to send the rest...
    rv = mSendLater->StartNextMailFileSend();
    NS_RELEASE(mSendLater);
  }

  return rv;
}

nsIMsgSendListener **
CreateListenerArray(nsIMsgSendListener *listener)
{
  if (!listener)
    return nsnull;

  nsIMsgSendListener **tArray = (nsIMsgSendListener **)PR_Malloc(sizeof(nsIMsgSendListener *) * 2);
  if (!tArray)
    return nsnull;
  nsCRT::memset(tArray, 0, sizeof(nsIMsgSendListener *) * 2);
  tArray[0] = listener;
  return tArray;
}

nsresult
nsMsgSendLater::CompleteMailFileSend()
{
nsresult                    rv;
nsString                    recips;
nsString                    ccList;
PRBool                      created;
nsCOMPtr<nsIMsgCompFields>  compFields = nsnull;
nsCOMPtr<nsIMsgSend>        pMsgSend = nsnull;

  // If for some reason the tmp file didn't get created, we've failed here
  mTempIFileSpec->exists(&created);
  if (!created)
    return NS_ERROR_FAILURE;

  // Get the recipients...
  if (NS_FAILED(mMessage->GetRecipients(recips)))
    return NS_ERROR_UNEXPECTED;
  else
  	mMessage->GetCCList(ccList);

  // Get the composition fields interface
  nsresult res = nsComponentManager::CreateInstance(kMsgCompFieldsCID, NULL, nsCOMTypeInfo<nsIMsgCompFields>::GetIID(), 
                                                    (void **) getter_AddRefs(compFields)); 
  if (NS_FAILED(res) || !compFields)
  {
    return NS_ERROR_FACTORY_NOT_LOADED;
  }

  // Get the message send interface
  rv = nsComponentManager::CreateInstance(kMsgSendCID, NULL, nsCOMTypeInfo<nsIMsgSend>::GetIID(), 
                                          (void **) getter_AddRefs(pMsgSend)); 
  if (NS_FAILED(res) || !pMsgSend)
  {
    return NS_ERROR_FACTORY_NOT_LOADED;
  }

  // Since we have already parsed all of the headers, we are simply going to
  // set the composition fields and move on.
  //
  nsMsgCompFields * fields = (nsMsgCompFields *)compFields.get();
  if (m_to)
  	fields->SetTo(m_to);

  if (m_bcc)
  	fields->SetBcc(m_bcc);

  if (m_fcc)
  	fields->SetFcc(m_fcc);

  if (m_newsgroups)
    fields->SetNewsgroups(m_newsgroups);

  // If we have this, we found a HEADER_X_MOZILLA_NEWSHOST which means
  // that we saved what the user typed into the "Newsgroup" line in this
  // header
  if (m_newshost)
    fields->SetNewsgroups(m_newshost);

  if (mRequestReturnReceipt)
    fields->SetReturnReceipt(PR_TRUE);

  // Create the listener for the send operation...
  mSendListener = new SendOperationListener();
  if (!mSendListener)
  {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  NS_ADDREF(mSendListener);
  // set this object for use on completion...
  mSendListener->SetSendLaterObject(this);
  nsIMsgSendListener **tArray = CreateListenerArray(mSendListener);
  if (!tArray)
  {
    NS_RELEASE(mSendListener);
    mSendListener = nsnull;
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(this);  
  rv = pMsgSend->SendMessageFile(mIdentity,
                            compFields, // nsIMsgCompFields                  *fields,
                            mTempIFileSpec,   // nsIFileSpec                        *sendFileSpec,
                            PR_TRUE,         // PRBool                            deleteSendFileOnCompletion,
                            PR_FALSE,        // PRBool                            digest_p,
                            nsMsgDeliverNow, // nsMsgDeliverMode                  mode,
                            nsnull,          // nsIMessage *msgToReplace, 
                            tArray); 
  NS_RELEASE(mSendListener);
  mSendListener = nsnull;
  if (NS_FAILED(rv))
    return rv;

  return NS_OK;
}

nsresult
nsMsgSendLater::StartNextMailFileSend()
{
  nsFileSpec    fileSpec;
  nsresult      rv;
  char          *aMessageURI = nsnull;

  //
  // First, go to the next entry and check where we are in the 
  // enumerator!
  //
  if (mFirstTime)
  {
    mFirstTime = PR_FALSE;
    mEnumerator->First();
  }
  else
    mEnumerator->Next();

  if (mEnumerator->IsDone() == NS_OK)
  {
    // Call any listeners on this operation and then exit cleanly
#ifdef NS_DEBUG
    printf("nsMsgSendLater: Finished \"Send Later\" operation.\n");
#endif
    NotifyListenersOnStopSending(NS_OK, nsnull, mTotalSendCount, mTotalSentSuccessfully);
    return NS_OK;
  }

  nsCOMPtr<nsISupports>   currentItem;

  rv = mEnumerator->CurrentItem(getter_AddRefs(currentItem));
  if (NS_FAILED(rv))
  {
    return rv;
  }

  mMessage = do_QueryInterface(currentItem); 
  if(!mMessage)
  {
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsCOMPtr<nsIRDFResource>  myRDFNode ;
  myRDFNode = do_QueryInterface(mMessage, &rv);
  if(NS_FAILED(rv) || (!myRDFNode))
  {
    return NS_ERROR_NOT_AVAILABLE;
  }

  myRDFNode->GetValue(&aMessageURI);

  char *tString = nsnull;
  nsString      subject;
  mMessage->GetSubject(subject);
  tString = subject.ToNewCString();
  printf("Sending message: [%s]\n", tString);
  PR_FREEIF(tString);

  mTempFileSpec = nsMsgCreateTempFileSpec("nsqmail.tmp"); 
	if (!mTempFileSpec)
    return NS_ERROR_FAILURE;

  NS_NewFileSpecWithSpec(*mTempFileSpec, &mTempIFileSpec);
	if (!mTempIFileSpec)
    return NS_ERROR_FAILURE;

  mHackTempFileSpec = nsMsgCreateTempFileSpec("hack.tmp"); 
	if (!mHackTempFileSpec)
    return NS_ERROR_FAILURE;

  NS_NewFileSpecWithSpec(*mHackTempFileSpec, &mHackTempIFileSpec);
	if (!mHackTempIFileSpec)
    return NS_ERROR_FAILURE;

  nsIMsgMessageService * messageService = nsnull;
	rv = GetMessageServiceFromURI(aMessageURI, &messageService);
  if (NS_FAILED(rv) && !messageService)
    return NS_ERROR_FACTORY_NOT_LOADED;

  ++mTotalSendCount;

  // Setup what we need to parse the data stream correctly
  m_inhead = PR_TRUE;
  m_headersFP = 0;
  m_headersPosition = 0;
  m_bytesRead = 0;
  m_position = 0;
  m_flagsPosition = 0;
  m_headersSize = 0;
  PR_FREEIF(mLeftoverBuffer);

  //
  // RICHIE
  // For now, we save as a file, but in the future, we will get a 
  // stream of data from netlib and make ourselves the consumer of this
  // stream
  //
  NS_ADDREF(this);

  // cleanup the save listener...
  if (mSaveListener)
    NS_RELEASE(mSaveListener);

  mSaveListener = new nsMsgDeliveryListener(SaveMessageCompleteCallback, nsFileSaveDelivery, this);
  if (!mSaveListener)
  {
    ReleaseMessageServiceFromURI(aMessageURI, messageService);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(mSaveListener);
  rv = messageService->SaveMessageToDisk(aMessageURI, mHackTempIFileSpec, PR_FALSE, mSaveListener, nsnull);
  ReleaseMessageServiceFromURI(aMessageURI, messageService);

  // RICHIE 
  // Problem. If I release the SaveListener, then there is a refcount problem.
  // NS_RELEASE(mSaveListener); 

	if (NS_FAILED(rv))
    return rv;    

  return NS_OK;
}

nsIMsgFolder *
nsMsgSendLater::GetUnsentMessagesFolder(nsIMsgIdentity *userIdentity)
{
  return LocateMessageFolder(userIdentity, nsMsgQueueForLater, nsnull);
}

//
// To really finalize this capability, we need to have the ability to get
// the message from the mail store in a stream for processing. The flow 
// would be something like this:
//
//      foreach (message in Outbox folder)
//         get stream of Nth message
//         if (not done with headers)
//            Tack on to current buffer of headers
//         when done with headers
//            BuildHeaders()
//            Write Headers to Temp File
//         after done with headers
//            write rest of message body to temp file
//
//          when done with the message
//            do send operation
//
//          when send is complete
//            Copy from Outbox to FCC folder
//            Delete from Outbox folder
//
//
nsresult 
nsMsgSendLater::SendUnsentMessages(nsIMsgIdentity                   *identity,
                                   nsIMsgSendLaterListener          **listenerArray)
{
  if (!identity)
    return NS_ERROR_INVALID_ARG;

  mIdentity = identity;
  NS_ADDREF(mIdentity);

  // Set the listener array 
  if (listenerArray)
    SetListenerArray(listenerArray);

  mMessageFolder = GetUnsentMessagesFolder(mIdentity);
  if (!mMessageFolder)
  {
    NS_RELEASE(mIdentity);
    mIdentity = nsnull;
    return NS_ERROR_FAILURE;
  }

  nsresult ret = mMessageFolder->GetMessages(&mEnumerator);
	if (NS_FAILED(ret) || (!mEnumerator))
  {
    NS_RELEASE(mIdentity);
    mIdentity = nsnull;
    return NS_ERROR_FAILURE;
  }

  mFirstTime = PR_TRUE;
	return StartNextMailFileSend();
}

nsresult
nsMsgSendLater::DeleteCurrentMessage()
{
  nsCOMPtr<nsISupportsArray>  msgArray;

  // Get the composition fields interface
  nsresult res = nsComponentManager::CreateInstance(kISupportsArrayCID, NULL, nsCOMTypeInfo<nsISupportsArray>::GetIID(), 
                                                    (void **) getter_AddRefs(msgArray)); 
  if (NS_FAILED(res) || !msgArray)
  {
    return NS_ERROR_FACTORY_NOT_LOADED;
  }

  nsCOMPtr<nsISupports> msgSupport = do_QueryInterface(mMessage, &res);
  msgArray->InsertElementAt(msgSupport, 0);
  res = mMessageFolder->DeleteMessages(msgArray, nsnull, PR_TRUE);
  if (NS_FAILED(res))
    return NS_ERROR_FAILURE;

  return NS_OK;
}

//
// This function parses the headers, and also deletes from the header block
// any headers which should not be delivered in mail, regardless of whether
// they were present in the queue file.  Such headers include: BCC, FCC,
// Sender, X-Mozilla-Status, X-Mozilla-News-Host, and Content-Length.
// (Content-Length is for the disk file only, and must not be allowed to
// escape onto the network, since it depends on the local linebreak
// representation.  Arguably, we could allow Lines to escape, but it's not
// required by NNTP.)
//
#define UNHEX(C) \
  ((C >= '0' && C <= '9') ? C - '0' : \
  ((C >= 'A' && C <= 'F') ? C - 'A' + 10 : \
        ((C >= 'a' && C <= 'f') ? C - 'a' + 10 : 0)))
nsresult
nsMsgSendLater::BuildHeaders()
{
	char *buf = m_headers;
	char *buf_end = buf + m_headersFP;

	PR_FREEIF(m_to);
	PR_FREEIF(m_bcc);
	PR_FREEIF(m_newsgroups);
	PR_FREEIF(m_newshost);
	m_flags = 0;

	while (buf < buf_end)
	{
		PRBool prune_p = PR_FALSE;
		PRBool  do_flags_p = PR_FALSE;
		PRBool  do_return_receipt_p = PR_FALSE;
		char *colon = PL_strchr(buf, ':');
		char *end;
		char *value = 0;
		char **header = 0;
		char *header_start = buf;

		if (! colon)
			break;

		end = colon;
		while (end > buf && (*end == ' ' || *end == '\t'))
			end--;

		switch (buf [0])
		{
		case 'B': case 'b':
		  if (!PL_strncasecmp ("BCC", buf, end - buf))
			{
			  header = &m_bcc;
			  prune_p = PR_TRUE;
			}
		  break;
		case 'C': case 'c':
		  if (!PL_strncasecmp ("CC", buf, end - buf))
			header = &m_to;
		  else if (!PL_strncasecmp (HEADER_CONTENT_LENGTH, buf, end - buf))
			prune_p = PR_TRUE;
		  break;
		case 'F': case 'f':
		  if (!PL_strncasecmp ("FCC", buf, end - buf))
			{
			  header = &m_fcc;
			  prune_p = PR_TRUE;
			}
		  break;
		case 'L': case 'l':
		  if (!PL_strncasecmp ("Lines", buf, end - buf))
			prune_p = PR_TRUE;
		  break;
		case 'N': case 'n':
		  if (!PL_strncasecmp ("Newsgroups", buf, end - buf))
  			header = &m_newsgroups;
		  break;
		case 'S': case 's':
		  if (!PL_strncasecmp ("Sender", buf, end - buf))
			prune_p = PR_TRUE;
		  break;
		case 'T': case 't':
		  if (!PL_strncasecmp ("To", buf, end - buf))
			header = &m_to;
		  break;
		case 'X': case 'x':
      {
        PRInt32 headLen = PL_strlen(HEADER_X_MOZILLA_STATUS2);
        if (headLen == end - buf &&
          !PL_strncasecmp(HEADER_X_MOZILLA_STATUS2, buf, end - buf))
          prune_p = PR_TRUE;
        else if (headLen == end - buf &&
          !PL_strncasecmp(HEADER_X_MOZILLA_STATUS, buf, end - buf))
          prune_p = do_flags_p = PR_TRUE;
        else if (!PL_strncasecmp(HEADER_X_MOZILLA_DRAFT_INFO, buf, end - buf))
          prune_p = do_return_receipt_p = PR_TRUE;
        else if (!PL_strncasecmp(HEADER_X_MOZILLA_NEWSHOST, buf, end - buf))
        {
          prune_p = PR_TRUE;
          header = &m_newshost;
        }
        break;
      }
		}

	  buf = colon + 1;
	  while (*buf == ' ' || *buf == '\t')
		buf++;

	  value = buf;

SEARCH_NEWLINE:
	  while (*buf != 0 && *buf != CR && *buf != LF)
		  buf++;

	  if (buf+1 >= buf_end)
		  ;
	  // If "\r\n " or "\r\n\t" is next, that doesn't terminate the header.
	  else if (buf+2 < buf_end &&
			   (buf[0] == CR  && buf[1] == LF) &&
			   (buf[2] == ' ' || buf[2] == '\t'))
		{
		  buf += 3;
		  goto SEARCH_NEWLINE;
		}
	  // If "\r " or "\r\t" or "\n " or "\n\t" is next, that doesn't terminate
		// the header either. 
	  else if ((buf[0] == CR  || buf[0] == LF) &&
			   (buf[1] == ' ' || buf[1] == '\t'))
		{
		  buf += 2;
		  goto SEARCH_NEWLINE;
		}

	  if (header)
		{
		  int L = buf - value;
		  if (*header)
			{
			  char *newh = (char*) PR_Realloc ((*header),
											   PL_strlen(*header) + L + 10);
			  if (!newh) return NS_ERROR_OUT_OF_MEMORY;
			  *header = newh;
			  newh = (*header) + PL_strlen (*header);
			  *newh++ = ',';
			  *newh++ = ' ';
        nsCRT::memcpy(newh, value, L);
			  newh [L] = 0;
			}
		  else
			{
			  *header = (char *) PR_Malloc(L+1);
			  if (!*header) return NS_ERROR_OUT_OF_MEMORY;
        nsCRT::memcpy((*header), value, L);
			  (*header)[L] = 0;
			}
		}
	  else if (do_flags_p)
		{
		  int i;
		  char *s = value;
		  PR_ASSERT(*s != ' ' && *s != '\t');
		  m_flags = 0;
		  for (i=0 ; i<4 ; i++) {
			m_flags = (m_flags << 4) | UNHEX(*s);
			s++;
		  }
		}
	  else if (do_return_receipt_p)
		{
		  int L = buf - value;
		  char *draftInfo = (char*) PR_Malloc(L+1);
		  char *receipt = NULL;
		  if (!draftInfo) return NS_ERROR_OUT_OF_MEMORY;
      nsCRT::memcpy(draftInfo, value, L);
		  *(draftInfo+L)=0;
		  receipt = PL_strstr(draftInfo, "receipt=");
		  if (receipt) 
			{
printf("RICHIE - FIX THIS......jjust add a member var and set the comp fields later!!!!!\n\7");

			  char *s = receipt+8;
			  int requestForReturnReceipt = 0;
			  sscanf(s, "%d", &requestForReturnReceipt);
          // RICHIE - return recipients are an issue! We should probably
          // add these to the CompFields for tracking instead of tying this
          // stuff to the MessagePane like in the old days.
			    // if ((requestForReturnReceipt == 2 || requestForReturnReceipt == 3))
  			  //   m_pane->SetRequestForReturnReceipt(TRUE);
			}
		  PR_FREEIF(draftInfo);
		}

	  if (*buf == CR || *buf == LF)
		{
		  if (*buf == CR && buf[1] == LF)
			buf++;
		  buf++;
		}

	  if (prune_p)
		{
		  char *to = header_start;
		  char *from = buf;
		  while (from < buf_end)
			*to++ = *from++;
		  buf = header_start;
		  buf_end = to;
		  m_headersFP = buf_end - m_headers;
		}
	}

  m_headers[m_headersFP++] = CR;
  m_headers[m_headersFP++] = LF;

  // Now we have parsed out all of the headers we need and we 
  // can proceed.
  return NS_OK;
}

// 
// This is a temporary method...in the future, we will build this
// buffer as we receive the data from the mail store.
//
nsresult
nsMsgSendLater::DriveFakeStream(nsIOutputStream *stream)
{
  nsIOFileStream  *inFile = nsnull;
  PRInt32         totSize = 0;
  char            buf[8192];
  PRUint32        aWriteCount;
  
  inFile = new nsIOFileStream(*mHackTempFileSpec);
  if (!inFile)
    return NS_ERROR_OUT_OF_MEMORY;

  m_headersFP = 0;
  inFile->seek(0);
  while (!inFile->eof())
  {
    buf[0] = '\0';
    totSize = inFile->read(buf, sizeof(buf));
    if (totSize > 0)
      stream->Write(buf, totSize, &aWriteCount);
  }

  inFile->close();  
  delete inFile;
  mHackTempFileSpec->Delete(PR_FALSE);
  return stream->Close();
}

int
DoGrowBuffer(PRInt32 desired_size, PRInt32 element_size, PRInt32 quantum,
    				char **buffer, PRInt32 *size)
{
  if (*size <= desired_size)
  {
    char *new_buf;
    PRInt32 increment = desired_size - *size;
    if (increment < quantum) // always grow by a minimum of N bytes 
      increment = quantum;
    
    new_buf = (*buffer
                ? (char *) PR_Realloc (*buffer, (*size + increment)
                * (element_size / sizeof(char)))
                : (char *) PR_Malloc ((*size + increment)
                * (element_size / sizeof(char))));
    if (! new_buf)
      return NS_ERROR_OUT_OF_MEMORY;
    *buffer = new_buf;
    *size += increment;
  }
  return 0;
}

#define do_grow_headers(desired_size) \
  (((desired_size) >= m_headersSize) ? \
   DoGrowBuffer ((desired_size), sizeof(char), 1024, \
				   &m_headers, &m_headersSize) \
   : 0)

nsresult
nsMsgSendLater::DeliverQueuedLine(char *line, PRInt32 length)
{
  PRInt32 flength = length;
  
  m_bytesRead += length;
  
// convert existing newline to CRLF 
// Don't need this because the calling routine is taking care of it.
//  if (length > 0 && (line[length-1] == CR || 
//     (line[length-1] == LF && (length < 2 || line[length-2] != CR))))
//  {
//    line[length-1] = CR;
//    line[length++] = LF;
//  }
//
  if (m_inhead)
  {
    if (m_headersPosition == 0)
    {
		  // This line is the first line in a header block.
      // Remember its position.
      m_headersPosition = m_position;
      
      // Also, since we're now processing the headers, clear out the
      // slots which we will parse data into, so that the values that
      // were used the last time around do not persist.
      
      // We must do that here, and not in the previous clause of this
      // `else' (the "I've just seen a `From ' line clause") because
      // that clause happens before delivery of the previous message is
      // complete, whereas this clause happens after the previous msg
      // has been delivered.  If we did this up there, then only the
      // last message in the folder would ever be able to be both
      // mailed and posted (or fcc'ed.)
      PR_FREEIF(m_to);
      PR_FREEIF(m_bcc);
      PR_FREEIF(m_newsgroups);
      PR_FREEIF(m_newshost);
      PR_FREEIF(m_fcc);
    }
    
    if (line[0] == CR || line[0] == LF || line[0] == 0)
    {
		  // End of headers.  Now parse them; open the temp file;
      // and write the appropriate subset of the headers out. 
      m_inhead = PR_FALSE;

			mOutFile = new nsOutputFileStream(*mTempFileSpec, PR_WRONLY | PR_CREATE_FILE);
      if ( (!mOutFile) || (!mOutFile->is_open()) )
        return NS_MSG_ERROR_WRITING_FILE;

      nsresult status = BuildHeaders();
      if (NS_FAILED(status))
        return status;

      if (mOutFile->write(m_headers, m_headersFP) != m_headersFP)
        return NS_MSG_ERROR_WRITING_FILE;
    }
    else
    {
		  // Otherwise, this line belongs to a header.  So append it to the
      // header data.
      
      if (!PL_strncasecmp (line, HEADER_X_MOZILLA_STATUS, PL_strlen(HEADER_X_MOZILLA_STATUS)))
        // Notice the position of the flags.
        m_flagsPosition = m_position;
      else if (m_headersFP == 0)
        m_flagsPosition = 0;
      
      nsresult status = do_grow_headers (length + m_headersFP + 10);
      if (NS_FAILED(status)) 
        return status;
      
      nsCRT::memcpy(m_headers + m_headersFP, line, length);
      m_headersFP += length;
    }
  }
  else
  {
    // This is a body line.  Write it to the file.
    PR_ASSERT(mOutFile);
    if (mOutFile)
    {
      PRInt32 wrote = mOutFile->write(line, length);
      if (wrote < (PRInt32) length) 
        return NS_MSG_ERROR_WRITING_FILE;
    }
  }
  
  m_position += flength;
  return NS_OK;
}

nsresult
nsMsgSendLater::SetListenerArray(nsIMsgSendLaterListener **aListenerArray)
{
  nsIMsgSendLaterListener **ptr = aListenerArray;
  if ( (!aListenerArray) || (!*aListenerArray) )
    return NS_OK;

  // First, count the listeners passed in...
  mListenerArrayCount = 0;
  while (*ptr != nsnull)
  {
    mListenerArrayCount++;
    ++ptr;
  }

  // now allocate an array to hold the number of entries.
  mListenerArray = (nsIMsgSendLaterListener **) PR_Malloc(sizeof(nsIMsgSendLaterListener *) * mListenerArrayCount);
  if (!mListenerArray)
    return NS_ERROR_OUT_OF_MEMORY;

  nsCRT::memset(mListenerArray, 0, (sizeof(nsIMsgSendLaterListener *) * mListenerArrayCount));
  
  // Now assign the listeners...
  PRInt32 i;
  for (i=0; i<mListenerArrayCount; i++)
  {
    mListenerArray[i] = aListenerArray[i];
    NS_ADDREF(mListenerArray[i]);
  }

  return NS_OK;
}

nsresult
nsMsgSendLater::AddListener(nsIMsgSendLaterListener *aListener)
{
  if ( (mListenerArrayCount > 0) || mListenerArray )
  {
    mListenerArrayCount = 1;
    mListenerArray = (nsIMsgSendLaterListener **) 
                  PR_Realloc(*mListenerArray, sizeof(nsIMsgSendLaterListener *) * mListenerArrayCount);
    if (!mListenerArray)
      return NS_ERROR_OUT_OF_MEMORY;
    else
      return NS_OK;
  }
  else
  {
    mListenerArrayCount = 1;
    mListenerArray = (nsIMsgSendLaterListener **) PR_Malloc(sizeof(nsIMsgSendLaterListener *) * mListenerArrayCount);
    if (!mListenerArray)
      return NS_ERROR_OUT_OF_MEMORY;

    nsCRT::memset(mListenerArray, 0, (sizeof(nsIMsgSendLaterListener *) * mListenerArrayCount));
  
    mListenerArray[0] = aListener;
    NS_ADDREF(mListenerArray[0]);
    return NS_OK;
  }
}

nsresult
nsMsgSendLater::RemoveListener(nsIMsgSendLaterListener *aListener)
{
  PRInt32 i;
  for (i=0; i<mListenerArrayCount; i++)
    if (mListenerArray[i] == aListener)
    {
      NS_RELEASE(mListenerArray[i]);
      mListenerArray[i] = nsnull;
      return NS_OK;
    }

  return NS_ERROR_INVALID_ARG;
}

nsresult
nsMsgSendLater::DeleteListeners()
{
  if ( (mListenerArray) && (*mListenerArray) )
  {
    PRInt32 i;
    for (i=0; i<mListenerArrayCount; i++)
    {
      NS_RELEASE(mListenerArray[i]);
    }
    
    PR_FREEIF(mListenerArray);
  }

  mListenerArrayCount = 0;
  return NS_OK;
}

nsresult
nsMsgSendLater::NotifyListenersOnStartSending(PRUint32 aTotalMessageCount)
{
  PRInt32 i;
  for (i=0; i<mListenerArrayCount; i++)
    if (mListenerArray[i] != nsnull)
      mListenerArray[i]->OnStartSending(aTotalMessageCount);

  return NS_OK;
}

nsresult
nsMsgSendLater::NotifyListenersOnProgress(PRUint32 aCurrentMessage, PRUint32 aTotalMessage)
{
  PRInt32 i;
  for (i=0; i<mListenerArrayCount; i++)
    if (mListenerArray[i] != nsnull)
      mListenerArray[i]->OnProgress(aCurrentMessage, aTotalMessage);

  return NS_OK;
}

nsresult
nsMsgSendLater::NotifyListenersOnStatus(const PRUnichar *aMsg)
{
  PRInt32 i;
  for (i=0; i<mListenerArrayCount; i++)
    if (mListenerArray[i] != nsnull)
      mListenerArray[i]->OnStatus(aMsg);

  return NS_OK;
}

nsresult
nsMsgSendLater::NotifyListenersOnStopSending(nsresult aStatus, const PRUnichar *aMsg, 
                                             PRUint32 aTotalTried, PRUint32 aSuccessful)
{
  PRInt32 i;
  for (i=0; i<mListenerArrayCount; i++)
    if (mListenerArray[i] != nsnull)
      mListenerArray[i]->OnStopSending(aStatus, aMsg, aTotalTried, aSuccessful);

  return NS_OK;
}

