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
 * Copyright (C) 1999 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#ifndef _nsMailDatabase_H_
#define _nsMailDatabase_H_

#include "nsMsgDatabase.h"
#include "nsMsgMessageFlags.h"

// This is the subclass of nsMsgDatabase that handles local mail messages.
class nsOfflineImapOperation;
class nsMsgKeyArray;
class MSG_Master;
class MSG_FolderInfo;
class nsIOFileStream;
class nsFilePath;

// this is the version number for the mail db. If the file format changes, we 
// just reparse the mail folder. 
const int kMailDBVersion = 1;

class nsMailDatabase : public nsMsgDatabase
{
public:
			nsMailDatabase(nsFilePath& folder);
	virtual ~nsMailDatabase();
	static nsresult			Open(nsFilePath &folderName, PRBool create, nsMailDatabase** pMessageDB,
									PRBool upgrading = PR_FALSE);

	static  nsresult		CloneInvalidDBInfoIntoNewDB(nsFilePath &pathName, nsMailDatabase** pMailDB);

	virtual nsresult		OnNewPath (nsFilePath &newPath);

	virtual nsresult		DeleteMessages(nsMsgKeyArray &nsMsgKeys, nsIDBChangeListener *instigator);

	static  nsresult		SetFolderInfoValid(nsFilePath &pathname, int num, int numunread);
	nsresult				GetFolderName(nsString &folderName);
	virtual nsMailDatabase	*GetMailDB() {return this;}
	MSG_Master		*GetMaster() {return m_master;}
	void			SetMaster(MSG_Master *master) {m_master = master;}

	virtual PRUint32		GetCurVersion() {return kMailDBVersion;}
	virtual MSG_FolderInfo *GetFolderInfo();
	
	// for offline imap queued operations
	// these are in the base mail class (presumably) because offline moves between online and offline
	// folders can cause these operations to be stored in local mail folders.
	nsresult				ListAllOfflineOpIds(nsMsgKeyArray &outputIds);
	int						ListAllOfflineDeletes(nsMsgKeyArray &outputIds);
	nsresult				GetOfflineOpForKey(nsMsgKey opKey, PRBool create, nsOfflineImapOperation **);
	nsresult				AddOfflineOp(nsOfflineImapOperation *op);
	nsresult				DeleteOfflineOp(nsMsgKey opKey);
	nsresult				SetSourceMailbox(nsOfflineImapOperation *op, const char *mailbox, nsMsgKey key);
	
	virtual nsresult		SetSummaryValid(PRBool valid = PR_TRUE);
	
	nsresult 				GetIdsWithNoBodies (nsMsgKeyArray &bodylessIds);
#ifdef DEBUG	// strictly for testing purposes
	virtual	nsresult		PrePopulate();
#endif
protected:
	virtual PRBool			SetHdrFlag(nsMsgHdr *, PRBool bSet, MsgFlags flag);
	virtual void			UpdateFolderFlag(nsMsgHdr *msgHdr, PRBool bSet, 
									 MsgFlags flag, nsIOFileStream **ppFileStream);
	virtual void			SetReparse(PRBool reparse);

	MSG_Master				*m_master;
	PRBool					m_reparse;
	nsFilePath				m_folderName;
	nsIOFileStream			*m_folderStream; 	/* this is a cache for loops which want file left open */
};

#endif
