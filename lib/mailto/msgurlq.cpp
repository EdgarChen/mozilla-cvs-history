/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
//

#include "msg.h"
#include "msgurlq.h"
#include "msgpane.h"


MSG_UrlQueueElement::MSG_UrlQueueElement (const char *url, MSG_UrlQueue *queue, Net_GetUrlExitFunc *exitFunction, MSG_Pane *pane, NET_ReloadMethod reloadMethod, FO_Present_Types outputFormat)
{
	m_queue = queue;
	m_pane = pane;
	m_urlString = XP_STRDUP(url);
	m_exitFunction = exitFunction;
	m_reloadMethod = reloadMethod;
	m_url = NULL;
	m_callGetURLDirectly = FALSE;
	m_outputFormat = outputFormat;
}

MSG_UrlQueueElement::MSG_UrlQueueElement (URL_Struct *url, MSG_UrlQueue *queue, Net_GetUrlExitFunc *exitFunction, MSG_Pane *pane, XP_Bool skipFE, FO_Present_Types outputFormat)
{
	m_queue = queue;
	m_pane = pane;
	m_urlString = XP_STRDUP(url->address);
	m_exitFunction = exitFunction;
	m_reloadMethod = url->force_reload;
	m_url = url;
	m_callGetURLDirectly = skipFE;
	m_outputFormat = outputFormat;
}

MSG_UrlQueueElement::MSG_UrlQueueElement (URL_Struct *urls, MSG_UrlQueue *q)
{
	m_queue = q;
	m_pane = urls->msg_pane;
	m_urlString = XP_STRDUP(urls->address);;
	m_url = urls;
	m_exitFunction = NULL;
	m_reloadMethod = NET_DONT_RELOAD;
	m_callGetURLDirectly = FALSE;
	m_outputFormat = FO_CACHE_AND_PRESENT;
}

MSG_UrlQueueElement::~MSG_UrlQueueElement()
{
	FREEIF(m_urlString);
//	if (m_url && m_callGetURLDirectly)
//		NET_FreeURLStruct(m_url);	// Tempting, but not our job 
}

void MSG_UrlQueueElement::PrepareToRun()
{
	// nothing we have to do right now for queue elements.
	return;
}

char* MSG_UrlQueueElement::GetURLString()
{
	if (m_urlString)
		return m_urlString;
	if (m_url)
		return(m_url->address);
	return(NULL);
}

URL_Struct* MSG_UrlQueueElement::GetURLStruct()
{
	if (!m_url)
		m_url = NET_CreateURLStruct( m_urlString, NET_DONT_RELOAD);
	return(m_url);
}


MSG_UrlLocalMsgCopyQueueElement::MSG_UrlLocalMsgCopyQueueElement(MessageCopyInfo * info, const char * url, MSG_UrlQueue * q, 
													  Net_GetUrlExitFunc * func, MSG_Pane * pane, NET_ReloadMethod reloadMethod)
  : MSG_UrlQueueElement (url, q, func, pane, reloadMethod)
{
	m_copyInfo = info;
}

MSG_UrlLocalMsgCopyQueueElement::MSG_UrlLocalMsgCopyQueueElement(MessageCopyInfo * info, URL_Struct * url, MSG_UrlQueue * q, 
													   Net_GetUrlExitFunc * func, MSG_Pane * pane, XP_Bool skipFE)
   : MSG_UrlQueueElement (url, q, func, pane, skipFE)
{
	m_copyInfo = info;
}

MSG_UrlLocalMsgCopyQueueElement::MSG_UrlLocalMsgCopyQueueElement (MessageCopyInfo * info, URL_Struct * urls, MSG_UrlQueue * q)
	: MSG_UrlQueueElement(urls, q)
{
	m_copyInfo = info;
}

MSG_UrlLocalMsgCopyQueueElement::~MSG_UrlLocalMsgCopyQueueElement()
{
	// should we delete the copy info? I don't think so!! 
	// when copy was finished, a call to MSG_FolderInfo::CleanUpCopy deletes
	// the copy info in the current context. Assuming our queue element isn't deleted before
	// it is executed upon......we don't need to delete it.
}

void MSG_UrlLocalMsgCopyQueueElement::PrepareToRun()
{
	MWContext * context = m_pane->GetContext();
	MessageCopyInfo * victim = NULL;
	if (context) 
	{
		victim = context->msgCopyInfo;
		XP_ASSERT(!victim);   // we actually should never have a victim....but if we do:
		// the last copy info is done.
		if (victim)
			XP_FREEIF(victim);  // local copies never have copy info chains, don't have to worry about nextCopyInfo

		context->msgCopyInfo = m_copyInfo;  // set ourselves up as the next copy info
	}
}

//*****************************************************************************
// MSG_UrlQueue -- Intended to be a general purpose way to chain several
//                 URLs together using the exit functions
//*****************************************************************************

const int MSG_UrlQueue::kNoSpecialIndex = -1;

MSG_UrlQueue::MSG_UrlQueue (MSG_Pane *pane)
{
#ifdef DEBUG
	MSG_UrlQueue *existingQueue = FindQueue(pane);
	if (existingQueue)
	{
		MSG_UrlQueueElement *elem = existingQueue->GetAt (existingQueue->m_runningUrl);
		if (elem)
			XP_Trace("trying to create queue while %s is running\n", elem->m_urlString);
	}
	XP_ASSERT(!existingQueue);
#endif

	m_pane = pane;
	m_runningUrl = -1;
	m_IndexOfNextUrl = kNoSpecialIndex;
	m_inExitFunc = FALSE;
	GetQueueArray()->Add (this);
}


MSG_UrlQueue::~MSG_UrlQueue ()
{
	GetQueueArray()->Remove(this);

	for (int i = 0; i < GetSize (); i++)
	{
		MSG_UrlQueueElement *e = GetAt(i);
		delete e;
	}
}

XPPtrArray *
MSG_UrlQueue::GetQueueArray()
{
	if (!m_queueArray)
		m_queueArray = new XPPtrArray();
	return m_queueArray;
}

XP_Bool MSG_UrlQueue::IsIMAPLoadFolderUrlQueue()
{
	return FALSE;
}

void MSG_UrlQueue::AddUrl (const char *url, Net_GetUrlExitFunc *exitFunction, MSG_Pane *pane, NET_ReloadMethod reloadMethod)
{
	MSG_UrlQueueElement *elem = NULL;
	
	if (!pane)
		pane = m_pane;
	elem = new MSG_UrlQueueElement (url, this, exitFunction, pane, reloadMethod);
	if (elem)
		Add(elem);
}

void MSG_UrlQueue::AddUrl (URL_Struct *url, Net_GetUrlExitFunc *exitFunction, MSG_Pane *pane, XP_Bool skipFE, FO_Present_Types outputFormat)
{
	MSG_UrlQueueElement * elem = NULL;
	if (!pane)
		pane = m_pane;
	elem = new MSG_UrlQueueElement(url, this, exitFunction, pane, skipFE, outputFormat);
	if (elem)
		Add(elem);
}

void MSG_UrlQueue::AddLocalMsgCopyUrl(MessageCopyInfo * info, const char *url, Net_GetUrlExitFunc *exitFunction, MSG_Pane * pane, NET_ReloadMethod reloadMethod)
{
	MSG_UrlLocalMsgCopyQueueElement *elem = NULL;

	if (!pane)
		pane = m_pane;
	elem = new MSG_UrlLocalMsgCopyQueueElement (info, url, this, exitFunction, pane, reloadMethod);
	if (elem)
		Add(elem);
}

/* static */ MSG_UrlQueue * MSG_UrlQueue::FindQueueWithSameContext(MSG_Pane *pane)
{
	MSG_UrlQueue * q = NULL;

	XP_ASSERT(pane);
	if (pane)
	{
		q = MSG_UrlQueue::FindQueue(pane);
		MSG_Pane *QPane = pane->GetFirstPaneForContext(pane->GetContext());
		while (!q && QPane)
		{
			q = MSG_UrlQueue::FindQueue(QPane);
			if (!q)
				QPane = pane->GetNextPaneForContext(QPane, pane->GetContext());
		}
#ifdef DEBUG_bienvenu
		if (q && QPane != pane)
			XP_Trace("found queue for different pane with same context!\n");
#endif
	}
	return q;
}

/* static */ MSG_UrlQueue * MSG_UrlQueue::GetOrCreateUrlQueue (MSG_Pane * pane, XP_Bool * newQueue)
// The following code appeared in just about every AddUrl method or variant thereof. I've generalized it in this single
// routine. 
// Returns: pointer to the queue for the pane. If we had to create the queue, then newQueue is set to TRUE;
{
	*newQueue = FALSE;
	MSG_UrlQueue *q = FindQueueWithSameContext(pane);
	if (!q) 
	{
		q = new MSG_UrlQueue(pane);
		*newQueue = TRUE;
	}
	// we seem to get in this state where we're not running a url but the queue
	// thinks we are.
	if (! (q->m_inExitFunc || q->m_runningUrl == -1 || XP_IsContextBusy(pane->GetContext())))
	{
#ifdef DEBUG
		MSG_UrlQueueElement *runningElement = q->GetAt(q->m_runningUrl);
		if (runningElement)
			XP_Trace("q was running url %s\n", runningElement->GetURLString());
		XP_ASSERT(FALSE);
#endif
	}
	return q;
}


/* static */ MSG_UrlQueue * MSG_UrlQueue::AddUrlToPane (const char *url, Net_GetUrlExitFunc *exitFunction, MSG_Pane *pane, NET_ReloadMethod reloadMethod)
{
	MSG_UrlQueue *q;
	MSG_UrlQueueElement *elem = NULL;
	XP_Bool newQ = FALSE;
	
	q = GetOrCreateUrlQueue(pane, &newQ);
	if (q)
	{
		elem = new MSG_UrlQueueElement (url, q, exitFunction, pane, reloadMethod);
		if (elem)
			q->Add(elem);
		if (newQ)
			q->GetNextUrl();
	}
	return q;
}

/* static*/ MSG_UrlQueue *MSG_UrlQueue::AddLocalMsgCopyUrlToPane (MessageCopyInfo * info, const char *url, Net_GetUrlExitFunc *exitFunction, MSG_Pane *pane, NET_ReloadMethod reloadMethod)
{
	MSG_UrlQueue *q;
	MSG_UrlLocalMsgCopyQueueElement *elem = NULL;
	XP_Bool newQ = FALSE;

	q = GetOrCreateUrlQueue(pane, &newQ);

	if (q)
	{
		elem = new MSG_UrlLocalMsgCopyQueueElement (info, url, q, exitFunction, pane, reloadMethod);
		if (elem)
			q->Add(elem);
		if (newQ)
			q->GetNextUrl();
	}
	return q;
}

/* static */ MSG_UrlQueue *MSG_UrlQueue::AddUrlToPane (URL_Struct *url, Net_GetUrlExitFunc *exitFunction, MSG_Pane *pane, XP_Bool skipFE, FO_Present_Types outputFormat)
{
	MSG_UrlQueue *q;
	MSG_UrlQueueElement *elem = NULL;
	XP_Bool newQ = FALSE;
	
	q = GetOrCreateUrlQueue(pane, &newQ);
	if (q)
	{
		elem = new MSG_UrlQueueElement (url, q, exitFunction, pane, skipFE, outputFormat);
		if (elem)
			q->Add(elem);
		if (newQ)
			q->GetNextUrl();
	}
	return q;
}

/* static */ MSG_UrlQueue *MSG_UrlQueue::AddLocalMsgCopyUrlToPane(MessageCopyInfo * info, URL_Struct * url, Net_GetUrlExitFunc * exitFunction, MSG_Pane * pane, XP_Bool skipFE)
{
	MSG_UrlQueue *q;
	MSG_UrlLocalMsgCopyQueueElement *elem = NULL;
	XP_Bool newQ = FALSE;

	q = GetOrCreateUrlQueue(pane, &newQ);
	if (q)
	{
		elem = new MSG_UrlLocalMsgCopyQueueElement (info, url, q, exitFunction, pane, skipFE);
		if (elem)
			q->Add(elem);
		if (newQ)
			q->GetNextUrl();
	}
	return q;
}


void MSG_UrlQueue::AddUrlAt (int where, const char *url, Net_GetUrlExitFunc *exitFunction, MSG_Pane *pane, NET_ReloadMethod reloadMethod)
{
	if (!pane)
		pane = m_pane;

	MSG_UrlQueueElement *elem = new MSG_UrlQueueElement (url, this, exitFunction, pane, reloadMethod);
	if (elem)
		InsertAt (where, elem);
}

void MSG_UrlQueue::AddLocalMsgCopyUrlAt (MessageCopyInfo * info, int where, const char * url, Net_GetUrlExitFunc * exitFunction, MSG_Pane * pane, NET_ReloadMethod reloadMethod)
{
	if (!pane)
		pane = m_pane;

	MSG_UrlLocalMsgCopyQueueElement *elem = new MSG_UrlLocalMsgCopyQueueElement (info, url, this, exitFunction, pane, reloadMethod);
	if (elem)
		InsertAt (where, elem);
}


void MSG_UrlQueue::GetNextUrl()
{
	int err = 0;
	
	MSG_UrlQueueElement *elem = GetAt(++m_runningUrl);
	XP_ASSERT(elem);
	if (elem)
	{
		// we need to make sure the queue element is ready to be run!!!
		elem->PrepareToRun();

		if (!elem->m_url)
		{
			elem->m_url = NET_CreateURLStruct (elem->m_urlString, elem->m_reloadMethod);
			if (!elem->m_url)
				return; //  (MsgERR) MK_OUT_OF_MEMORY;
		}
		if (elem->m_url && XP_STRLEN(elem->m_url->address) > 0)
		{
			elem->m_url->internal_url = TRUE;
			elem->m_url->pre_exit_fn = &MSG_UrlQueue::ExitFunction;
			if (elem->m_callGetURLDirectly)
			{
				elem->m_url->pre_exit_fn = NULL;
				err = NET_GetURL(elem->m_url, elem->m_outputFormat, elem->m_pane->GetContext(), MSG_UrlQueue::ExitFunction);
			} else
			{
#ifdef MOZ_MAIL_NEWS
				err = elem->m_pane->GetURL (elem->m_url, FALSE);
#else /* MOZ_MAIL_NEWS */
                XP_ASSERT(0);
#endif /* MOZ_MAIL_NEWS */
			}
		}
		else
		{
			elem->m_url->msg_pane = m_pane;
			CallExitAndChain(elem->m_url, 0, elem->m_pane->GetContext());
		}
	}
}

void MSG_UrlQueue::HandleUrlQueueInterrupt(URL_Struct * URL_s, int status, MWContext * window_id)
{
	for (int i = 0; i <  m_interruptCallbacks.GetSize(); i++)
	{
		MSG_UrlQueueInterruptFunc *exitFunc = (MSG_UrlQueueInterruptFunc *) m_interruptCallbacks.GetAt(i);
		if (exitFunc)
			(*exitFunc) (this, URL_s, status, window_id);
	}
	// default is to do nothing
}

/*static*/ XPPtrArray *MSG_UrlQueue::m_queueArray = NULL;


/*static*/ MSG_UrlQueue *MSG_UrlQueue::FindQueue (const char *url, MWContext *context)
{
	for (int i = 0; i < GetQueueArray()->GetSize(); i++)
	{
		MSG_UrlQueue *queue = (MSG_UrlQueue*)(GetQueueArray()->GetAt(i));
		for (int j = 0; j < queue->GetSize(); j++)
		{
			MSG_UrlQueueElement *elem = queue->GetAt(j);

			// pane has been deleted - remove element from queue
			if (!MSG_Pane::PaneInMasterList(elem->m_pane))
			{
#ifdef DEBUG_akkana
				printf("FindQueue: Removing deleted pane\n");
#endif /* DEBUG */
				queue->RemoveAt(j--);
				delete elem;
			}
			else if (elem->m_pane->GetContext() == context)
			{
				if (elem->m_urlString && !XP_STRCMP(elem->m_urlString, url))
					return queue;
			}
		}
	}
	return NULL;
}


/*static*/ MSG_UrlQueue *MSG_UrlQueue::FindQueue (MSG_Pane *pane)
{
	for (int i = 0; i < GetQueueArray()->GetSize (); i++)
	{
		MSG_UrlQueue *queue = (MSG_UrlQueue*)(GetQueueArray()->GetAt(i));
		if (queue->m_pane == pane)
			return queue;
	}
	return NULL;
}

/*static*/ void MSG_UrlQueue::ExitFunction (URL_Struct *URL_s, int status, MWContext *window_id)
{
	MSG_UrlQueue *queue = FindQueue (URL_s->address, window_id);
	XP_ASSERT(queue);
	if (queue)
		queue->CallExitAndChain(URL_s, status, window_id);
}

// Note that this can delete itself if at the end of its list.
void MSG_UrlQueue::CallExitAndChain(URL_Struct *URL_s, int status, MWContext *window_id)
{
	MSG_UrlQueueElement *elem = GetAt (m_runningUrl);
	if (elem->m_exitFunction)
	{
		m_inExitFunc = TRUE;
		elem->m_exitFunction (URL_s, status, window_id);
		m_inExitFunc = FALSE;
	}
	if ((m_runningUrl >= GetSize() - 1) || status == MK_INTERRUPTED)
	{
if (status == MK_INTERRUPTED)
			HandleUrlQueueInterrupt(URL_s, status, window_id);
		delete this;
	}
	else
		GetNextUrl();
}


extern "C" int MSG_GetUrlQueueSize (const char *url, MWContext *context)
{
	MSG_UrlQueue *queue = MSG_UrlQueue::FindQueue (url, context);
	if (queue)
		return queue->GetSize();
	return 0;
}

void MSG_UrlQueue::AddInterruptCallback(MSG_UrlQueueInterruptFunc *interruptFunc)
{
	m_interruptCallbacks.Add((void *) interruptFunc);
}


