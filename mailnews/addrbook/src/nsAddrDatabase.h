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

#ifndef _nsAddrDatabase_H_
#define _nsAddrDatabase_H_

#include "nsIAddrDatabase.h"
#include "mdb.h"
#include "nsVoidArray.h"
#include "nsString.h"
#include "nsFileSpec.h"
#include "nsIAddrDBListener.h"
#include "nsISupportsArray.h"

class nsAddrDatabase : public nsIAddrDatabase 
{
public:
	NS_DECL_ISUPPORTS

	//////////////////////////////////////////////////////////////////////////////
	// nsIAddrDBAnnouncer methods:

	NS_IMETHOD AddListener(nsIAddrDBListener *listener);
	NS_IMETHOD RemoveListener(nsIAddrDBListener *listener);
	NS_IMETHOD NotifyCardAttribChange(PRUint32 abCode, nsIAddrDBListener *instigator);
	NS_IMETHOD NotifyCardEntryChange(PRUint32 abCode, PRUint32 entryID, nsIAddrDBListener *instigator);
	NS_IMETHOD NotifyAnnouncerGoingAway();

	//////////////////////////////////////////////////////////////////////////////
	// nsIAddrDatabase methods:

	NS_IMETHOD Open(nsFileSpec & folderName, PRBool create, nsIAddrDatabase **pAddrDB, PRBool upgrading);
	NS_IMETHOD Close(PRBool forceCommit);
	NS_IMETHOD OpenMDB(const char *dbName, PRBool create);
	NS_IMETHOD CloseMDB(PRBool commit);
	NS_IMETHOD Commit(PRUint32 commitType);
	NS_IMETHOD ForceClosed();

	NS_IMETHOD CreateNewCardAndAddToDB(nsIAbCard *newCard, PRBool benotify);
	NS_IMETHOD EnumerateCards(nsIEnumerator **result);

	//////////////////////////////////////////////////////////////////////////////
	// nsAddrDatabase methods:

	nsAddrDatabase();
	~nsAddrDatabase();

	nsIMdbFactory	*GetMDBFactory();
	nsIMdbEnv		*GetEnv() {return m_mdbEnv;}
	nsIMdbStore		*GetStore() {return m_mdbStore;}
	PRUint32		GetCurVersion();
	nsIMdbTableRowCursor *GetTableRowCursor();
	nsIMdbTable		*GetPabTable() {return m_mdbPabTable;}

	static nsAddrDatabase*	FindInCache(nsFileSpec &dbName);

	//helper function to fill in nsStrings from hdr row cell contents.
	nsresult				RowCellColumnTonsString(nsIMdbRow *row, mdb_token columnToken, nsString &resultStr);
	nsresult				RowCellColumnToUInt32(nsIMdbRow *row, mdb_token columnToken, PRUint32 *uint32Result);
	nsresult				RowCellColumnToUInt32(nsIMdbRow *row, mdb_token columnToken, PRUint32 &uint32Result);
	nsresult				RowCellColumnToMime2EncodedString(nsIMdbRow *row, mdb_token columnToken, nsString &resultStr);
	nsresult				RowCellColumnToCollationKey(nsIMdbRow *row, mdb_token columnToken, nsString &resultStr);

	// helper functions to put values in cells for the passed-in row
	nsresult				UInt32ToRowCellColumn(nsIMdbRow *row, mdb_token columnToken, PRUint32 value);
	// helper functions to copy an nsString to a yarn, int32 to yarn, and vice versa.
	static	void			YarnTonsString(struct mdbYarn *yarn, nsString *str);
	static	void			YarnToUInt32(struct mdbYarn *yarn, PRUint32 *i);

	static void		CleanupCache();

	nsresult CreateABCard(nsIMdbRow* cardRow, nsIAbCard **result);

protected:

	// open db cache
    static void		AddToCache(nsAddrDatabase* pAddrDB) 
						{GetDBCache()->AppendElement(pAddrDB);}
	static void		RemoveFromCache(nsAddrDatabase* pAddrDB);
	static PRInt32	FindInCache(nsAddrDatabase* pAddrDB);
	PRBool			MatchDbName(nsFileSpec &dbName);	// returns TRUE if they match

#if defined(XP_PC) || defined(XP_MAC)	// this should go away when we can provide our own file stream to MDB/Mork
	static void		UnixToNative(char*& ioPath);
#endif
#if defined(XP_MAC)
	static void		NativeToUnix(char*& ioPath);
#endif


	mdb_err AddCardColumn(nsIMdbRow* cardRow, mdb_column inColumn, char* str);


	static nsVoidArray/*<nsAddrDatabase>*/* GetDBCache();
	static nsVoidArray/*<nsAddrDatabase>*/* m_dbCache;

	// mdb bookkeeping stuff
	nsresult			InitExistingDB();
	nsresult			InitNewDB();
	nsresult			InitMDBInfo();

	nsIMdbEnv		    *m_mdbEnv;	// to be used in all the db calls.
	nsIMdbStore	 	    *m_mdbStore;
	nsIMdbTable		    *m_mdbPabTable;
	nsIMdbRow			*m_mdbRow;	// singleton row in table;
	nsFileSpec		    m_dbName;
	PRBool				m_mdbTokensInitialized;
    nsVoidArray/*<nsIAddrDBListener>*/ *m_ChangeListeners;

	mdb_kind			m_pabTableKind;
	mdb_kind			m_buddyTableKind;
	mdb_kind			m_historyTableKind;
	mdb_kind			m_mailListTableKind;
	mdb_kind			m_categoryTableKind;

	mdb_scope			m_cardRowScopeToken;
	mdb_token			m_firstNameColumnToken;
	mdb_token			m_lastNameColumnToken;
	mdb_token			m_displayNameColumnToken;
	mdb_token			m_priEmailColumnToken;
	mdb_token			m_2ndEmailColumnToken;
	mdb_token			m_workPhoneColumnToken;
	mdb_token			m_homePhoneColumnToken;
	mdb_token			m_faxColumnToken;
	mdb_token			m_pagerColumnToken;
	mdb_token			m_cellularColumnToken;
	mdb_token			m_workCityColumnToken;
	mdb_token			m_organizationColumnToken;
	mdb_token			m_nicknameColumnToken;
	mdb_token			m_addressCharSetColumnToken;
};

#endif
