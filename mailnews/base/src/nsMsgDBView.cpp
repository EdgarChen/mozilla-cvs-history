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
 * Copyright (C) 2001 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 */

#include "msgCore.h"
#include "nsMsgDBView.h"
#include "nsISupports.h"
#include "nsIMsgHdr.h"

/* Implementation file */

NS_IMPL_ISUPPORTS1(nsMsgDBView, nsIMsgDBView)

nsMsgDBView::nsMsgDBView()
{
  NS_INIT_ISUPPORTS();
  m_viewType = nsMsgDBViewType::anyView;
  /* member initializers and constructor code */
  m_sortValid = PR_TRUE;
  m_sortOrder = nsMsgViewSortOrder::none;
  //m_viewFlags = (ViewFlags) 0;
}

nsMsgDBView::~nsMsgDBView()
{
  /* destructor code */
}

NS_IMETHODIMP nsMsgDBView::Open(nsIMsgDatabase *msgDB, nsMsgViewSortTypeValue viewType, PRInt32 *pCount)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsMsgDBView::Close()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsMsgDBView::Init(PRInt32 *pCount)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsMsgDBView::AddKeys(nsMsgKey *pKeys, PRInt32 *pFlags, const char *pLevels, nsMsgViewSortTypeValue sortType, PRInt32 numKeysToAdd)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult nsMsgDBView::ReverseThreads()
{
    printf("XXX same sort type, just different sort order.  just reverse threads\n");
    return NS_OK;
}

nsresult nsMsgDBView::ReverseSort()
{
    PRUint32 num = GetSize();

    // go up half the array swapping values
    for (PRUint32 i = 0; i < (num / 2); i++) {

        // swap flags
        PRUint32 end = num - i - 1;
        PRUint32 tempFlags = m_flags.GetAt(i);
        m_flags.SetAt(i, m_flags.GetAt(i));
        m_flags.SetAt(end, tempFlags);

        // swap keys
        nsMsgKey tempKey = m_keys.GetAt(i);
        m_keys.SetAt(i, m_keys.GetAt(end));
        m_keys.SetAt(end, tempKey);

        // no need to swap elements in m_levels, 
        // since we won't call ReverseSort() if we
        // are in threaded mode, so m_levels are all the same.
    }

    return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::PopulateView()
{
    PRUint32 i;
    for (i=0;i<10;i++) {
        m_keys.InsertAt(i,i*100+1);
        m_flags.InsertAt(i,i);
        //m_levels.InsertAt(i,i%2);
    }
    return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::DumpView()
{
    PRUint32 i;
    PRUint32 num = GetSize();
    printf("#:  (key,flag)\n");
    for (i = 0; i < num; i++) {
        printf("%d:  (%d,%d)\n",i,m_keys.GetAt(i),m_flags.GetAt(i));
    }
    printf("\n");
    return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::Sort(nsMsgViewSortTypeValue sortType, nsMsgViewSortOrderValue sortOrder)
{
    nsresult rv;

    printf("XXX nsMsgDBView::Sort(%d,%d)\n",(int)sortType,(int)sortOrder);
    if (m_sortType == sortType && m_sortValid) {
        if (m_sortOrder == sortOrder) {
            printf("XXX same as it ever was.  do nothing\n");
            return NS_OK;
        }   
        else {
            if (m_sortType != nsMsgViewSortType::byThread) {
                rv = ReverseSort();
                NS_ENSURE_SUCCESS(rv,rv);
            }
            else {
                rv = ReverseThreads();
                NS_ENSURE_SUCCESS(rv,rv);
            }

            m_sortType = sortType;
            m_sortOrder = sortOrder;
            return NS_OK;
        }
    }

    printf("XXX ok, sort the beast.\n");

    m_sortType = sortType;
    m_sortOrder = sortOrder;

    return NS_OK;
}

nsresult nsMsgDBView::ExpandAll()
{
	for (PRInt32 i = GetSize() - 1; i >= 0; i--) 
	{
		PRUint32 numExpanded;
		PRUint32 flags = m_flags[i];
		if (flags & MSG_FLAG_ELIDED)
			ExpandByIndex(i, &numExpanded);
	}
	return NS_OK;
}

nsresult nsMsgDBView::ExpandByIndex(nsMsgViewIndex index, PRUint32 *pNumExpanded)
{
	int				numListed = 0;
	char			flags = m_flags[index];
	nsMsgKey		firstIdInThread, startMsg = nsMsgKey_None;
	nsresult		rv = NS_OK;
	nsMsgViewIndex	firstInsertIndex = index + 1;
	nsMsgViewIndex	insertIndex = firstInsertIndex;
	uint32			numExpanded = 0;
	nsMsgKeyArray			tempIDArray;
	nsByteArray		tempFlagArray;
	nsByteArray		tempLevelArray;
	nsByteArray		unreadLevelArray;

	NS_ASSERTION(flags & MSG_FLAG_ELIDED, "can't expand an already expanded thread");
	flags &= ~MSG_FLAG_ELIDED;

	if ((PRUint32) index > m_keys.GetSize())
		return NS_MSG_MESSAGE_NOT_FOUND;

	firstIdInThread = m_keys[index];
	nsCOMPtr <nsIMsgDBHdr> msgHdr;
  m_db->GetMsgHdrForKey(firstIdInThread, getter_AddRefs(msgHdr));
	if (msgHdr == nsnull)
	{
		NS_ASSERTION(PR_FALSE, "couldn't find message to expand");
		return NS_MSG_MESSAGE_NOT_FOUND;
	}
	m_flags[index] = flags;
  NoteChange(index, 1, nsMsgViewNotificationCode::changed);
	do
	{
		const int listChunk = 200;
#ifdef ON_BRANCH_YET
		nsMsgKey	listIDs[listChunk];
		char		listFlags[listChunk];
		char		listLevels[listChunk];

		if (m_viewFlags & kUnreadOnly)
		{
			if (flags & MSG_FLAG_READ)
				unreadLevelArray.Add(0);	// keep top level hdr in thread, even though read.
      nsMsgKey threadId;
      msgHdr->GetThreadId(&threadId);
			rv = m_db->ListUnreadIdsInThread(threadId,  &startMsg, unreadLevelArray, 
											listChunk, listIDs, listFlags, listLevels, &numListed);
		}
		else
			rv = m_db->ListIdsInThread(msgHdr,  &startMsg, listChunk, 
											listIDs, listFlags, listLevels, &numListed);
		// Don't add thread to view, it's already in.
		for (int i = 0; i < numListed; i++)
		{
			if (listIDs[i] != firstIdInThread)
			{
				tempIDArray.Add(listIDs[i]);
				tempFlagArray.Add(listFlags[i]);
				tempLevelArray.Add(listLevels[i]);
				insertIndex++;
			}
		}
#endif
		if (numListed < listChunk || startMsg == nsMsgKey_None)
			break;
	}
	while (NS_SUCCEEDED(rv));
	numExpanded = (insertIndex - firstInsertIndex);

	NoteStartChange(firstInsertIndex, numExpanded, nsMsgViewNotificationCode::insertOrDelete);

	m_keys.InsertAt(firstInsertIndex, &tempIDArray);
#ifdef ON_BRANCH_YET
	m_flags.InsertAt(firstInsertIndex, &tempFlagArray);
  m_levels.InsertAt(firstInsertIndex, &tempLevelArray);
#endif
	NoteEndChange(firstInsertIndex, numExpanded, nsMsgViewNotificationCode::insertOrDelete);
	if (pNumExpanded != nsnull)
		*pNumExpanded = numExpanded;
	return rv;
}

void	nsMsgDBView::EnableChangeUpdates()
{
}
void	nsMsgDBView::DisableChangeUpdates()
{
}
void	nsMsgDBView::NoteChange(nsMsgViewIndex firstlineChanged, PRInt32 numChanged, 
							 nsMsgViewNotificationCodeValue changeType)
{
}

void	nsMsgDBView::NoteStartChange(nsMsgViewIndex firstlineChanged, PRInt32 numChanged, 
							 nsMsgViewNotificationCodeValue changeType)
{
}
void	nsMsgDBView::NoteEndChange(nsMsgViewIndex firstlineChanged, PRInt32 numChanged, 
							 nsMsgViewNotificationCodeValue changeType)
{
}
