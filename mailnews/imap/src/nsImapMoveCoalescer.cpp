/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#include "msgCore.h"
#include "nsMsgImapCID.h"
#include "nsImapMailFolder.h"
#include "nsImapMoveCoalescer.h"
#include "nsMsgKeyArray.h"
#include "nsImapService.h"

static NS_DEFINE_CID(kCImapService, NS_IMAPSERVICE_CID);

nsImapMoveCoalescer::nsImapMoveCoalescer(nsImapMailFolder *sourceFolder)
{
	m_sourceFolder = sourceFolder;
	if (sourceFolder)
		NS_ADDREF(sourceFolder);
}

nsImapMoveCoalescer::~nsImapMoveCoalescer()
{
	NS_IF_RELEASE(m_sourceFolder);
	for (PRInt32 i = 0; i < m_sourceKeyArrays.Count(); i++)
	{
		nsMsgKeyArray *keys = (nsMsgKeyArray *) m_sourceKeyArrays.ElementAt(i);
		delete keys;
	}
}

nsresult nsImapMoveCoalescer::AddMove(nsIMsgFolder *folder, nsMsgKey key)
{
	if (!m_destFolders)
        NS_NewISupportsArray(getter_AddRefs(m_destFolders));
	if (m_destFolders)
	{
		PRInt32 folderIndex = m_destFolders->IndexOf(folder);
		nsMsgKeyArray *keysToAdd=nsnull;
		if (folderIndex >= 0)
		{
			keysToAdd = (nsMsgKeyArray *) m_sourceKeyArrays.ElementAt(folderIndex);
		}
		else
		{
			nsCOMPtr <nsISupports> supports = do_QueryInterface(folder);
			if (supports)
			{
				m_destFolders->AppendElement(supports);
				keysToAdd = new nsMsgKeyArray;
				if (!keysToAdd)
					return NS_ERROR_OUT_OF_MEMORY;

				m_sourceKeyArrays.AppendElement(keysToAdd);
			}
		}
		if (keysToAdd)
			keysToAdd->Add(key);
		return NS_OK;
	}
	else
		return NS_ERROR_OUT_OF_MEMORY;

}

nsresult nsImapMoveCoalescer::PlaybackMoves(nsIEventQueue *eventQueue)
{
	PRUint32 numFolders;
	nsresult rv = NS_OK;

	if (!m_destFolders)
		return NS_OK;	// nothing to do.

	m_destFolders->Count(&numFolders);
	for (PRUint32 i = 0; i < numFolders; i++)
	{
		nsCOMPtr <nsISupports> destSupports = getter_AddRefs(m_destFolders->ElementAt(i));
		nsCOMPtr <nsIMsgFolder> destFolder(do_QueryInterface(destSupports));
        NS_WITH_SERVICE(nsIImapService, imapService, kCImapService, &rv);
        if (NS_SUCCEEDED(rv) && imapService)
		{
			nsMsgKeyArray *keysToAdd = (nsMsgKeyArray *) m_sourceKeyArrays.ElementAt(i);
			if (keysToAdd)
			{
				nsCString messageIds;

				nsImapMailFolder::AllocateUidStringFromKeyArray(*keysToAdd, messageIds);

				destFolder->SetNumNewMessages(keysToAdd->GetSize());
				destFolder->SetBiffState(nsMsgBiffState_NewMail);

				nsCOMPtr <nsISupports> sourceSupports = do_QueryInterface((nsIMsgImapMailFolder *) m_sourceFolder, &rv);
				nsCOMPtr <nsIUrlListener> urlListener(do_QueryInterface(sourceSupports));

				nsCOMPtr<nsISupportsArray> messages;
				NS_NewISupportsArray(getter_AddRefs(messages));
				for (PRUint32 keyIndex = 0; i < keysToAdd->GetSize(); i++)
				{
					nsCOMPtr<nsIMessage> message;
					nsCOMPtr<nsIMsgDBHdr> mailHdr = nsnull;
					rv = m_sourceFolder->GetMessageHeader(keysToAdd->ElementAt(keyIndex), getter_AddRefs(mailHdr));
					if (NS_SUCCEEDED(rv) && mailHdr)
					{
						nsCOMPtr<nsISupports> iSupports;
	  					m_sourceFolder->CreateMessageFromMsgDBHdr(mailHdr, getter_AddRefs(message)) ;
						iSupports = do_QueryInterface(message);
						messages->AppendElement(iSupports);
					}
				}
				rv = destFolder->CopyMessages(m_sourceFolder,
                               messages, PR_TRUE, nsnull,
                               /*nsIMsgCopyServiceListener* listener*/ nsnull);
//			   rv = imapService->OnlineMessageCopy(eventQueue,
//						m_sourceFolder, messageIds.GetBuffer(),
//						destFolder, PR_TRUE, PR_TRUE,
//						urlListener, nsnull, nsnull);
			}
		}
	}
	return rv;
}

