/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#include "nsIStreamListener.h"
#include "nsHTTPResponseListener.h"
#include "nsIChannel.h"
#include "nsIBufferInputStream.h"
#include "nsHTTPChannel.h"
#include "nsHTTPResponse.h"
#include "nsIHttpEventSink.h"
#include "nsCRT.h"
#include "stdio.h" //sscanf

#include "nsIHttpNotify.h"
#include "nsINetModRegEntry.h"
#include "nsProxyObjectManager.h"
#include "nsIServiceManager.h"
#include "nsINetModuleMgr.h"
#include "nsIEventQueueService.h"
#include "nsIBuffer.h"

static const int kMAX_FIRST_LINE_SIZE= 256;
static const int kMAX_BUFFER_SIZE = 1024;

static const char* kCRLF = "\r\n";
static const char kSP = ' ';
static const char kHT = '\t';

#define Skipln(p,max)   \
    PR_BEGIN_MACRO      \
    while ((*p != LF) && (max > p)) \
        ++p;    \
    PR_END_MACRO

nsHTTPResponseListener::nsHTTPResponseListener(): 
    m_pConnection(nsnull),
    m_bFirstLineParsed(PR_FALSE),
    m_pResponse(nsnull),
    m_pConsumer(nsnull),
    m_ReadLength(0),
    m_PartHeader(nsnull),
    m_PartHeaderLen(0),
    m_bHeadersDone(PR_FALSE)
{
    NS_INIT_REFCNT();
}

nsHTTPResponseListener::~nsHTTPResponseListener()
{
    NS_IF_RELEASE(m_pConnection);
    NS_IF_RELEASE(m_pResponse);
    NS_IF_RELEASE(m_pConsumer);
    if (m_PartHeader)
    {
        delete m_PartHeader;
        m_PartHeader = 0;
    }
}

NS_IMPL_ISUPPORTS(nsHTTPResponseListener,nsIStreamListener::GetIID());

static NS_DEFINE_IID(kProxyObjectManagerIID, NS_IPROXYEVENT_MANAGER_IID);
static NS_DEFINE_CID(kEventQueueService, NS_EVENTQUEUESERVICE_CID);
static NS_DEFINE_CID(kNetModuleMgrCID, NS_NETMODULEMGR_CID);

NS_IMETHODIMP
nsHTTPResponseListener::OnDataAvailable(nsISupports* context,
                                        nsIBufferInputStream *i_pStream, 
                                        PRUint32 i_SourceOffset,
                                        PRUint32 i_Length)
{
    nsresult rv = NS_OK;
    NS_ASSERTION(i_pStream, "No stream supplied by the transport!");

    if (!m_pResponse)
    {
        // why do I need the connection in the constructor... get rid.. TODO
        m_pResponse = new nsHTTPResponse (m_pConnection, i_pStream);
        if (!m_pResponse)
        {
            NS_ERROR("Failed to create the response object!");
            return NS_ERROR_OUT_OF_MEMORY;
        }
        NS_ADDREF(m_pResponse);
        nsHTTPChannel* pTestCon = NS_STATIC_CAST(nsHTTPChannel*, m_pConnection);
        pTestCon->SetResponse(m_pResponse);
    }
    
    //Rick- can the i_Length ever be zero? 
    if (0==i_Length)
        return NS_OK; // Wait for next cycle. 

    if (m_bHeadersDone)
    {
        // Pass the notification out to the consumer...
        NS_ASSERTION(m_pConsumer, "No Stream Listener!");
        if (m_pConsumer) {
            rv = m_pConsumer->OnDataAvailable(m_pConnection, i_pStream, 0, i_Length);
        }
        return rv;
    } 

    else if (!m_bFirstLineParsed) {
      rv = ParseStatusLine(i_pStream);
    }

    //Search for the end of the headers mark
    //Rick- Is this max correct? or should it be the same as transport's max? 
    char buffer[kMAX_BUFFER_SIZE];

    nsCOMPtr<nsIBuffer> pBuffer;
    rv = i_pStream->GetBuffer(getter_AddRefs(pBuffer));
    if (NS_FAILED(rv)) return rv;

    const char* headerTerminationStr = "\r\n\r\n";
    PRBool bFoundEnd = PR_FALSE;
    PRUint32 offsetSearchedTo = i_Length;

    rv = pBuffer->Search(headerTerminationStr, PR_FALSE, &bFoundEnd, &offsetSearchedTo);
    if (NS_FAILED(rv)) return rv;

    if (!bFoundEnd)
    {
        headerTerminationStr = "\n\n";
        rv = pBuffer->Search(headerTerminationStr, PR_FALSE, &bFoundEnd, &offsetSearchedTo);
        if (NS_FAILED(rv)) return rv;
    }

    // If the end of the headers was found then adjust the offset to include
    // the termination characters...
    if (bFoundEnd)
    {
        offsetSearchedTo += PL_strlen(headerTerminationStr);
    }

    // Now read the buffer upto the offsetSearchedTo
    PRUint32 lengthRead = 0;
    rv = pBuffer->Read(buffer, offsetSearchedTo, &lengthRead);
    if (NS_FAILED(rv)) return rv;
    buffer[lengthRead] = '\0';
    char* p = buffer;
    //parse this buffer-
    while (buffer+lengthRead > p)
    {
        char* lineStart = p;
        if (*lineStart == '\0' || *lineStart == CR || *lineStart == LF)
        {
            m_bHeadersDone = PR_TRUE;
            // TODO process headers here. 
            // Based on the process headers we may or may not want to 
            // fire the headers available
            FireOnHeadersAvailable();
            // Fire the partial length onDataAvailable
            if (i_Length > lengthRead)
            {
                NS_ASSERTION(m_pConsumer, "No Stream Listner!");
                if (m_pConsumer)
                    rv = m_pConsumer->OnDataAvailable(m_pConnection, i_pStream, 0, i_Length-lengthRead);
            }
            break; // break off this buffer while
        }
        // Skip to end of line;
        Skipln(p, buffer+lengthRead);
/*
        if (!m_bFirstLineParsed)
        {
            char server_version[9]; // HTTP/1.1 
            PRUint32 stat = 0;
            char stat_str[kMAX_FIRST_LINE_SIZE];
            sscanf(lineStart, "%8s %d %s", server_version, &stat, stat_str);
            m_pResponse->SetServerVersion(server_version);
            m_pResponse->SetStatus(stat);
            m_pResponse->SetStatusString(stat_str);
            m_bFirstLineParsed = PR_TRUE;
        }
        else 
*/
        {
            char* header = lineStart;
            char* value = PL_strchr(lineStart, ':');
            if(value)
            {
                //mark the end of header
                *value = '\0';
                value++;
                //mark the end of value
                *p = '\0';
                if (m_PartHeaderLen == 0)
                    m_pResponse->SetHeaderInternal(header, value);
                else
                {
                    //append the header to the partheader
                    header = PL_strcat(m_PartHeader, header);
                    m_pResponse->SetHeaderInternal(header, value);
                    //Reset partHeader now
                    delete m_PartHeader;
                    m_PartHeader = 0;
                    m_PartHeaderLen = 0;
                }
            }
            else // this is just a part of the header so save it for later use...
            {
                NS_ASSERTION(m_PartHeaderLen == 0, "Overwriting partial header!");
                m_PartHeaderLen = p-header;
                m_PartHeader = new char(m_PartHeaderLen+1);
                PL_strncpy(m_PartHeader, lineStart, m_PartHeaderLen);
                m_PartHeader[m_PartHeaderLen] = '\0';
            }
        }
        p++;
    }
    return rv;
}

NS_IMETHODIMP
nsHTTPResponseListener::OnStartBinding(nsISupports* i_pContext)
{
    nsresult rv;

    //TODO globally replace printf with trace calls. 
    //printf("nsHTTPResponseListener::OnStartBinding...\n");

    // Initialize header varaibles...  
    m_bHeadersDone     = PR_FALSE;
    m_bFirstLineParsed = PR_FALSE;

    // Cache the nsIHTTPChannel...
    if (i_pContext) {
        rv = i_pContext->QueryInterface(nsIHTTPChannel::GetIID(), 
                                        (void**)&m_pConnection);
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }

    // Cache the nsIStreamListener of the consumer...
    if (NS_SUCCEEDED(rv)) {
        rv = m_pConnection->GetResponseDataListener(&m_pConsumer);
    }

    NS_ASSERTION(m_pConsumer, "No Stream Listener!");

    // Pass the notification out to the consumer...
    if (m_pConsumer) {
        // XXX: This is the wrong context being passed out to the consumer
        rv = m_pConsumer->OnStartBinding(i_pContext);
    }

    return rv;
}

NS_IMETHODIMP
nsHTTPResponseListener::OnStopBinding(nsISupports* i_pContext,
                                 nsresult i_Status,
                                 const PRUnichar* i_pMsg)
{
    nsresult rv;
    //printf("nsHTTPResponseListener::OnStopBinding...\n");
    //NS_ASSERTION(m_pResponse, "Response object not created yet or died?!");
    // Should I save this as a member variable? yes... todo

    NS_ASSERTION(m_pConsumer, "No Stream Listener!");
    // Pass the notification out to the consumer...
    if (m_pConsumer) {
        // XXX: This is the wrong context being passed out to the consumer
        rv = m_pConsumer->OnStopBinding(i_pContext, i_Status, i_pMsg);
    } 

    return rv;
}

NS_IMETHODIMP
nsHTTPResponseListener::OnStartRequest(nsISupports* i_pContext)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHTTPResponseListener::OnStopRequest(nsISupports* i_pContext,
                                      nsresult iStatus,
                                      const PRUnichar* i_pMsg)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


nsresult nsHTTPResponseListener::FireOnHeadersAvailable()
{
    nsresult rv;
    NS_ASSERTION(m_bHeadersDone, "Headers have not been received!");

    if (m_bHeadersDone) {

        // Notify the event sink that response headers are available...
        nsIHTTPEventSink* pSink= nsnull;
        m_pConnection->GetEventSink(&pSink);
        if (pSink) {
            pSink->OnHeadersAvailable(m_pConnection);
        }

        // Check for any modules that want to receive headers once they've arrived.
        NS_WITH_SERVICE(nsINetModuleMgr, pNetModuleMgr, kNetModuleMgrCID, &rv);
        if (NS_FAILED(rv)) return rv;

        nsISimpleEnumerator* pModules = nsnull;
        rv = pNetModuleMgr->EnumerateModules("http-response", &pModules);
        if (NS_FAILED(rv)) return rv;

        nsIProxyObjectManager*  proxyObjectManager = nsnull; 
        rv = nsServiceManager::GetService( NS_XPCOMPROXY_PROGID, 
                                            kProxyObjectManagerIID,
                                            (nsISupports **)&proxyObjectManager);
        if (NS_FAILED(rv)) {
            NS_RELEASE(pModules);
            return rv;
        }

        nsISupports *supEntry = nsnull;

        // Go through the external modules and notify each one.
        rv = pModules->GetNext(&supEntry);
        while (NS_SUCCEEDED(rv)) {
            nsINetModRegEntry *entry = nsnull;
            rv = supEntry->QueryInterface(nsINetModRegEntry::GetIID(), (void**)&entry);
            NS_RELEASE(supEntry);
            if (NS_FAILED(rv)) {
                NS_RELEASE(pModules);
                NS_RELEASE(proxyObjectManager);
                return rv;
            }

            nsCID *lCID;
            nsIEventQueue* lEventQ = nsnull;

            rv = entry->GetMCID(&lCID);
            if (NS_FAILED(rv)) {
                NS_RELEASE(pModules);
                NS_RELEASE(proxyObjectManager);
                return rv;
            }

            rv = entry->GetMEventQ(&lEventQ);
            if (NS_FAILED(rv)) {
                NS_RELEASE(pModules);
                NS_RELEASE(proxyObjectManager);            
                return rv;
            }

            nsIHTTPNotify *pNotify = nsnull;
            // if this call fails one of the following happened.
            // a) someone registered an object for this topic but didn't
            //    implement the nsIHTTPNotify interface on that object.
            // b) someone registered an object for this topic bud didn't
            //    put the .xpt lib for that object in the components dir
            rv = proxyObjectManager->GetProxyObject(lEventQ, 
                                               *lCID,
                                               nsnull,
                                               nsIHTTPNotify::GetIID(),
                                               PROXY_ASYNC,
                                               (void**)&pNotify);
            NS_RELEASE(proxyObjectManager);
        
            NS_RELEASE(lEventQ);

            if (NS_SUCCEEDED(rv)) {
                // send off the notification, and block.
                nsIHTTPNotify* externMod = nsnull;
                rv = pNotify->QueryInterface(nsIHTTPNotify::GetIID(), (void**)&externMod);
                NS_RELEASE(pNotify);
            
                if (NS_FAILED(rv)) {
                    NS_ASSERTION(0, "proxy object manager found an interface we can not QI for");
                    NS_RELEASE(pModules);
                    return rv;
                }

                // make the nsIHTTPNotify api call
                externMod->AsyncExamineResponse(m_pConnection);
                NS_RELEASE(externMod);
                // we could do something with the return code from the external
                // module, but what????            
            }

            NS_RELEASE(entry);
            rv = pModules->GetNext(&supEntry); // go around again
        }
        NS_RELEASE(pModules);
        NS_IF_RELEASE(proxyObjectManager);

    } else {
        rv = NS_ERROR_FAILURE;
    }

    return rv;
}

char *nsHTTPResponseListener::EatWhiteSpace(char *aBuffer)
{
  while ((*aBuffer == ' ') || (*aBuffer == '\t')) aBuffer++;
  return aBuffer;
}

nsresult nsHTTPResponseListener::ParseStatusLine(nsIBufferInputStream* aStream)
{
  nsresult rv = NS_OK;

  nsCOMPtr<nsIBuffer> pBuffer;
  char statusLineBuffer[255];
  char *start, *end;

  PRBool bFoundString = PR_FALSE;
  PRUint32 offset, length, bytesRead;
  PRInt32 CRLFlength, statusCode;

  rv = aStream->GetBuffer(getter_AddRefs(pBuffer));
  if (NS_FAILED(rv)) return rv;

  // Look for the CRLF which ends the Status-Line.
  // If found, then offset will mark the beginning of the CRLF...
  rv = pBuffer->Search(kCRLF, PR_FALSE, &bFoundString, &offset);
  if (NS_FAILED(rv)) return rv;

  //
  // The Status Line has the following: format:
  //    HTTP-Version SP Status-Code SP Reason-Phrase CRLF
  //
  if (bFoundString) {
    // Include the CRLF as part of the line to read...
    CRLFlength = PL_strlen(kCRLF);
    length = offset + CRLFlength;

    //
    // Make sure the statusLineBuffer does not overflow.  This would only
    // happen if the Reason-Phrase returned from the server was large...
    //
    // If the Status-Line is too large, then limit it to the size of the 
    // buffer.  This will truncate the Reason-Phrase, but who really cares?
    //
    NS_ASSERTION(length < sizeof(statusLineBuffer), "Status Line is too long!");
    if (length >= sizeof(statusLineBuffer)) {
      length = sizeof(statusLineBuffer);
    }

    // Read the Status-Line out of the stream...
    rv = aStream->Read(statusLineBuffer, length, &bytesRead);
    if (NS_FAILED(rv)) return rv;

    // Null terminate the buffer right before the CRLF...
    // If the Status-Line was truncated then NULL terminate at the end...
    if (bytesRead > offset) {
      statusLineBuffer[bytesRead-CRLFlength] = '\0';
    } else {
      statusLineBuffer[bytesRead] = '\0';
    }

    //
    // Parse the HTTP-Version -> "HTTP" "/" 1*DIGIT "." 1*DIGIT
    //
    start = EatWhiteSpace(statusLineBuffer); // Consume any leading whitespace
    // Find the next space character...
    for (end=start; (*end && (*end != kSP) && (*end != kHT)); end++) {};
    if (! *end) {
      // The status line is bogus...
      return NS_ERROR_FAILURE;
    }
    *end = '\0';  // Replace the first space character with NULL...
    m_pResponse->SetServerVersion(start);

    //
    // Parse the Status-Code -> 3DIGIT
    //
    start = EatWhiteSpace(end+1); // Consume any leading whitespace
    end   = start+3;

    // Verify that the character after the 3 digits is whitespace...
    if ((bytesRead < (PRUint32)(end - statusLineBuffer)) || 
        ((*end != kSP) && (*end != kHT))) {
      // The status line is bogus...
      return NS_ERROR_FAILURE;
    }
    *end = '\0';  // Replace the first space character with NULL...
    statusCode = atoi(start);
    m_pResponse->SetStatus(statusCode);

    //
    // Parse the Reason-Phrase -> *<TEXT excluding CR,LF>
    //
    start = EatWhiteSpace(end+1); // Consume any leading whitespace
    m_pResponse->SetStatusString(start);

    m_bFirstLineParsed = PR_TRUE;
  }
  else {
    // XXX: What do we do if only a partial status line is received?
    rv = NS_ERROR_FAILURE;
  }
  return rv;
}
