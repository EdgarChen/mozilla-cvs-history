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
/*
 * formerly listngst.h
 * This class should ultimately be part of a news group listing
 * state machine - either by inheritance or delegation.
 * Currently, a folder pane owns one and libnet news group listing
 * related messages get passed to this object.
 */
#ifndef nsNNTPNewsgroupListState_h___
#define nsNNTPNewsgroupListState_h___


/* The below is all stuff that we remember for netlib about which
   articles we've already seen in the current newsgroup. */

typedef struct MSG_NewsKnown {
	MSG_NewsHost* host;
	char* group_name;
	msg_NewsArtSet* set; /* Set of articles we've already gotten
								  from the newsserver (if it's marked
								  "read", then we've already gotten it).
								  If an article is marked "read", it
								  doesn't mean we're actually displaying
								  it; it may be an article that no longer
								  exists, or it may be one that we've
								  marked read and we're only viewing
								  unread messages. */

	int32 first_possible;	/* The oldest article in this group. */
	int32 last_possible;	/* The newest article in this group. */

	XP_Bool shouldGetOldest;
} MSG_NewsKnown;

extern NS_COM nsresult
NS_NewMsgXOVERParser(nsIMsgXOVERParser **aInstancePtrResult,
                     nsIMsgNewsHost *newsHost,
                     PRInt32 m_firstArticle,
                     PRInt32 m_lastArticle,
                     PRInt32 m_firstPossibleArticle,
                     PRInt32 m_lastPossibleArticle);

extern NS_COM nsresult
NS_NewMsgNewsArticleList(nsIMsgNewsArticleList **aInstancePtrResult,
                         nsIMsgNewsHost *newsHost,
                         nsIMsgNewsgroup *newsgroup);
    
#endif
