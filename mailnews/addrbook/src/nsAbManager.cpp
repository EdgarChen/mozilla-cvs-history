/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Seth Spitzer <sspitzer@netscape.com>
 *   Pierre Phaneuf <pp@ludusdesign.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nscore.h"
#include "nsIAddressBook.h"
#include "nsAddressBook.h"
#include "nsAbBaseCID.h"
#include "nsDirPrefs.h"
#include "nsIAddrBookSession.h"
#include "nsIAddrDatabase.h"
#include "nsIOutputStream.h"
#include "nsIFileStreams.h"
#include "msgCore.h"
#include "nsIImportService.h"
#include "nsIStringBundle.h"

#include "plstr.h"
#include "prmem.h"
#include "prprf.h"	 

#include "nsCOMPtr.h"
#include "nsIDOMXULElement.h"
#include "nsIDOMNodeList.h"
#include "nsIRDFCompositeDataSource.h"
#include "nsIRDFResource.h"
#include "nsRDFResource.h"
#include "nsIRDFService.h"
#include "nsIServiceManager.h"
#include "nsAppShellCIDs.h"
#include "nsIAppShellService.h"
#include "nsIDOMWindowInternal.h"
#include "nsIContentViewer.h"
#include "nsIContentViewerFile.h"
#include "nsIDocShell.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsICategoryManager.h"
#include "nsIAbUpgrader.h"
#include "nsSpecialSystemDirectory.h"
#include "nsIFilePicker.h"
#include "nsIPref.h"
#include "nsVoidArray.h"
#include "nsIAbCard.h"
#include "nsIAbMDBCard.h"

// for now, these should be in the same order as they are in the import code
// 36 = IMPORT_FIELD_DESC_END - IMPORT_FIELD_DESC_START + 1
// see importMsgProperties and nsImportStringBundle.h

#define   AB_COLUMN_COUNT      36

// XXX todo, merge with what's in nsAbLDAPProperties.cpp, so we can
// use this for LDAP and LDIF export
// XXX todo what about _AimScreenName, or other generic columns?
static const char *AB_COLUMN_ARRAY[] = { 
      kFirstNameColumn,
      kLastNameColumn,
      kDisplayNameColumn,  
      kNicknameColumn,   
      kPriEmailColumn,
      k2ndEmailColumn, 
      kWorkPhoneColumn,   
      kHomePhoneColumn,  
      kFaxColumn,       
      kPagerColumn,       
      kCellularColumn,     
      kHomeAddressColumn, 
      kHomeAddress2Column, 
      kHomeCityColumn, 
      kHomeStateColumn, 
      kHomeZipCodeColumn,
      kHomeCountryColumn, 
      kWorkAddressColumn,   
      kWorkAddress2Column, 
      kWorkCityColumn,     
      kWorkStateColumn,   
      kWorkZipCodeColumn,  
      kWorkCountryColumn, 
      kJobTitleColumn,      
      kDepartmentColumn,    
      kCompanyColumn,
      kWebPage1Column,
      kWebPage2Column, 
      kBirthYearColumn,
      kBirthMonthColumn, 
      kBirthDayColumn, 
      kCustom1Column, 
      kCustom2Column, 
      kCustom3Column, 
      kCustom4Column, 
      kNotesColumn   
};

static nsresult ConvertDOMListToResourceArray(nsIDOMNodeList *nodeList, nsISupportsArray **resourceArray)
{
	nsresult rv = NS_OK;
	PRUint32 listLength;
	nsIDOMNode *node;
	nsIDOMXULElement *xulElement;
	nsIRDFResource *resource;

	if(!resourceArray)
		return NS_ERROR_NULL_POINTER;

	if(NS_FAILED(rv = nodeList->GetLength(&listLength)))
		return rv;

	if(NS_FAILED(NS_NewISupportsArray(resourceArray)))
	{
		return NS_ERROR_OUT_OF_MEMORY;
	}

	for(PRUint32 i = 0; i < listLength; i++)
	{
		if(NS_FAILED(nodeList->Item(i, &node)))
			return rv;

		if(NS_SUCCEEDED(rv = node->QueryInterface(NS_GET_IID(nsIDOMXULElement), (void**)&xulElement)))
		{
			if(NS_SUCCEEDED(rv = xulElement->GetResource(&resource)))
			{
				(*resourceArray)->AppendElement(resource);
				NS_RELEASE(resource);
			}
			NS_RELEASE(xulElement);
		}
		NS_RELEASE(node);
		
	}

	return rv;
}

//
// nsAddressBook
//
nsAddressBook::nsAddressBook()
{
	NS_INIT_REFCNT();
}

nsAddressBook::~nsAddressBook()
{
}

NS_IMPL_THREADSAFE_ADDREF(nsAddressBook);
NS_IMPL_THREADSAFE_RELEASE(nsAddressBook);

NS_IMPL_QUERY_INTERFACE2(nsAddressBook, nsIAddressBook, nsICmdLineHandler);

//
// nsIAddressBook
//

NS_IMETHODIMP nsAddressBook::NewAddressBook
(nsIRDFCompositeDataSource* db, PRUint32 prefCount, const char **prefName, const PRUnichar **prefValue)
{
	if(!db || !*prefName || !*prefValue)
		return NS_ERROR_NULL_POINTER;

	nsresult rv = NS_OK;
	nsCOMPtr<nsIRDFService> rdfService = do_GetService (NS_RDF_CONTRACTID "/rdf-service;1", &rv);
	NS_ENSURE_SUCCESS(rv, rv);

	nsCOMPtr<nsIRDFResource> parentResource;
	rv = rdfService->GetResource(kAllDirectoryRoot, getter_AddRefs(parentResource));
	NS_ENSURE_SUCCESS(rv, rv);

	nsCOMPtr<nsIAbDirectory> parentDir = do_QueryInterface(parentResource, &rv);
	NS_ENSURE_SUCCESS(rv, rv);
		
	rv = parentDir->CreateNewDirectory (prefCount, prefName, prefValue);
	return rv;
}

NS_IMETHODIMP nsAddressBook::DeleteAddressBooks
(nsIRDFCompositeDataSource* db, nsISupportsArray *parentDir, nsIDOMNodeList *nodeList)
{
	if(!db || !parentDir || !nodeList)
		return NS_ERROR_NULL_POINTER;

	nsresult rv = NS_OK;

	nsCOMPtr<nsISupportsArray> resourceArray;
	rv = ConvertDOMListToResourceArray(nodeList, getter_AddRefs(resourceArray));
	NS_ENSURE_SUCCESS(rv, rv);

	DoCommand(db, NC_RDF_DELETE, parentDir, resourceArray);
	return rv;
}

nsresult nsAddressBook::DoCommand(nsIRDFCompositeDataSource* db,
                                  const char *command,
                                  nsISupportsArray *srcArray,
                                  nsISupportsArray *argumentArray)
{

	nsresult rv = NS_OK;

	nsCOMPtr<nsIRDFService> rdfService = do_GetService (NS_RDF_CONTRACTID "/rdf-service;1", &rv);
	NS_ENSURE_SUCCESS(rv, rv);

	nsCOMPtr<nsIRDFResource> commandResource;
	rv = rdfService->GetResource(command, getter_AddRefs(commandResource));
	if(NS_SUCCEEDED(rv))
	{
		rv = db->DoCommand(srcArray, commandResource, argumentArray);
	}

	return rv;

}

NS_IMETHODIMP nsAddressBook::PrintCard()
{
  nsresult rv = NS_ERROR_FAILURE;
  nsCOMPtr<nsIContentViewer> viewer;

  if (!mDocShell) {
    return NS_ERROR_FAILURE;
  }

  mDocShell->GetContentViewer(getter_AddRefs(viewer));

  if (viewer) 
  {
    nsCOMPtr<nsIContentViewerFile> viewerFile = do_QueryInterface(viewer);
    if (viewerFile) {
      rv = viewerFile->Print(PR_FALSE, nsnull, (nsIWebProgressListener*)nsnull);
    }
  }

  return rv;  
}

NS_IMETHODIMP nsAddressBook::PrintAddressbook()
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsAddressBook::SetDocShellWindow(nsIDOMWindowInternal *aWin)
{
   NS_PRECONDITION(aWin != nsnull, "null ptr");
   if (!aWin)
       return NS_ERROR_NULL_POINTER;
 
   nsCOMPtr<nsIScriptGlobalObject> globalObj( do_QueryInterface(aWin) );
   if (!globalObj) {
     return NS_ERROR_FAILURE;
   }
 
   globalObj->GetDocShell(&mDocShell);
   if (!mDocShell)
     return NS_ERROR_NOT_INITIALIZED;

   // Make reference weak by releasing
   mDocShell->Release();
 
   return NS_OK;
}

NS_IMETHODIMP nsAddressBook::GetAbDatabaseFromURI(const char *uri, nsIAddrDatabase **db)
{
  nsCOMPtr<nsIAddrDatabase> database;  
  
  nsresult rv = NS_ERROR_NULL_POINTER;
  if (uri)
  {
    nsFileSpec* dbPath = nsnull;
    
    nsCOMPtr<nsIAddrBookSession> abSession = 
      do_GetService(NS_ADDRBOOKSESSION_CONTRACTID, &rv); 
    if(NS_SUCCEEDED(rv))
      abSession->GetUserProfileDirectory(&dbPath);
    
    if (NS_SUCCEEDED(rv) && dbPath)
    {
      nsCAutoString file(&(uri[PL_strlen(kMDBDirectoryRoot)]));
      PRInt32 pos = file.Find("/");
      if (pos != kNotFound)
        file.Truncate(pos);
      (*dbPath) += file.get();
      
      nsCOMPtr<nsIAddrDatabase> addrDBFactory = 
        do_GetService(NS_ADDRDATABASE_CONTRACTID, &rv);
      
      if (NS_SUCCEEDED(rv) && addrDBFactory)
        rv = addrDBFactory->Open(dbPath, PR_TRUE, getter_AddRefs(database), PR_TRUE);
      
      delete dbPath;
      
      if (NS_SUCCEEDED(rv) && database)
      {
        NS_IF_ADDREF(*db = database);
      }
      else
        rv = NS_ERROR_NULL_POINTER;
    }
  }
  return rv;
}

nsresult nsAddressBook::GetAbDatabaseFromFile(char* pDbFile, nsIAddrDatabase **db)
{
	nsresult rv = NS_OK;
	nsCOMPtr<nsIAddrDatabase> database;
	if (pDbFile)
	{
		nsFileSpec* dbPath = nsnull;

		nsCOMPtr<nsIAddrBookSession> abSession = 
		         do_GetService(NS_ADDRBOOKSESSION_CONTRACTID, &rv); 
		if(NS_SUCCEEDED(rv))
			abSession->GetUserProfileDirectory(&dbPath);
		
		nsCAutoString file(pDbFile);
		(*dbPath) += file.get();

		nsCOMPtr<nsIAddrDatabase> addrDBFactory = 
		         do_GetService(NS_ADDRDATABASE_CONTRACTID, &rv);
		if (NS_SUCCEEDED(rv) && addrDBFactory)
			rv = addrDBFactory->Open(dbPath, PR_TRUE, getter_AddRefs(database), PR_TRUE);

      delete dbPath;

		if (NS_SUCCEEDED(rv) && database)
		{
			NS_IF_ADDREF(*db = database);
		}
		else
			rv = NS_ERROR_NULL_POINTER;

	}
	return NS_OK;
}

NS_IMETHODIMP nsAddressBook::MailListNameExistsInDB(const PRUnichar *name, const char *URI, PRBool *exist)
{
	*exist = PR_FALSE;

	nsCOMPtr<nsIAddrDatabase> database;
	nsresult rv = GetAbDatabaseFromURI(URI, getter_AddRefs(database));				
	if (NS_SUCCEEDED(rv) && database)
		database->FindMailListbyUnicodeName(name, exist);
    return NS_OK;
}

//check for all address book
NS_IMETHODIMP nsAddressBook::MailListNameExists(const PRUnichar *name, PRBool *exist)
{
	*exist = PR_FALSE;
	nsVoidArray* pDirectories = DIR_GetDirectories();
	if (pDirectories)
	{
		PRInt32 count = pDirectories->Count();
		/* check: only show personal address book for now */
		/* not showing 4.x address book unitl we have the converting done */
		PRInt32 i;
		for (i = 0; i < count; i++)
		{
			DIR_Server *server = (DIR_Server *)pDirectories->ElementAt(i);
			if (server->dirType == PABDirectory)
			{
				nsAutoString dbfile; dbfile.AssignWithConversion(server->fileName);
				PRInt32 pos = dbfile.Find("na2");
				if (pos >= 0) /* check: this is a 4.x file, remove when conversion is done */
					continue;

				nsCOMPtr<nsIAddrDatabase> database;
				nsresult rv = GetAbDatabaseFromFile(server->fileName, getter_AddRefs(database));				
				if (NS_SUCCEEDED(rv) && database)
				{
					database->FindMailListbyUnicodeName(name, exist);
					if (*exist == PR_TRUE)
						return NS_OK;
				}
			}
		}
	}
    return NS_OK;
}

class AddressBookParser 
{
protected:

    nsCAutoString	mLine;
    nsCOMPtr <nsIFileSpec> mFileSpec;
	char*			mDbUri;
	nsCOMPtr<nsIAddrDatabase> mDatabase;  
	PRBool mMigrating;
	PRBool mStoreLocAsHome;
	PRBool mDeleteDB;
	PRBool mImportingComm4x;

    nsresult ParseLDIFFile();
	void AddLdifRowToDatabase(PRBool bIsList);
	void AddLdifColToDatabase(nsIMdbRow* newRow, char* typeSlot, char* valueSlot, PRBool bIsList);

	nsresult GetLdifStringRecord(char* buf, PRInt32 len, PRInt32* stopPos);
	nsresult str_parse_line(char *line, char	**type, char **value, int *vlen);
	char * str_getline( char **next );

public:
    AddressBookParser(nsIFileSpec *fileSpec, PRBool migrating, nsIAddrDatabase *db, PRBool bStoreLocAsHome, PRBool bImportingComm4x);
    ~AddressBookParser();

    nsresult ParseFile();
};

AddressBookParser::AddressBookParser(nsIFileSpec * fileSpec, PRBool migrating, nsIAddrDatabase *db, PRBool bStoreLocAsHome, PRBool bImportingComm4x)
{
	mFileSpec = fileSpec;
	mDbUri = nsnull;
	mMigrating = migrating;
  mDatabase = db;
  if (mDatabase)
    mDeleteDB = PR_FALSE;
  else
    mDeleteDB = PR_TRUE;
  mStoreLocAsHome = bStoreLocAsHome;
  mImportingComm4x = bImportingComm4x;
}

AddressBookParser::~AddressBookParser(void)
{
	if(mDbUri)
		PR_smprintf_free(mDbUri);
	if (mDatabase && mDeleteDB)
	{
		mDatabase->Close(PR_TRUE);
		mDatabase = nsnull;
	}
}

nsresult AddressBookParser::ParseFile()
{
  // Initialize the parser for a run...
  mLine.Truncate();

  // this is converting 4.x to 6.0
  if (mImportingComm4x && mDatabase) 
  {
    return ParseLDIFFile();
  }
 
	/* Get database file name */
	char *leafName = nsnull;
	nsAutoString fileString;
	if (mFileSpec) {
		mFileSpec->GetLeafName(&leafName);
		fileString.AssignWithConversion(leafName);

		PRInt32 i = 0;
		while (leafName[i] != '\0')
		{
			if (leafName[i] == '.')
			{
				leafName[i] = '\0';
				break;
			}
			else
				i++;
		}
		if (leafName)
			mDbUri = PR_smprintf("%s%s.mab", kMDBDirectoryRoot, leafName);
	}

	// to do: we should use only one "return rv;" at the very end, instead of this
	//  multi return structure
	nsresult rv = NS_OK;
	nsFileSpec* dbPath = nsnull;
	char* fileName = PR_smprintf("%s.mab", leafName);

	nsCOMPtr<nsIAddrBookSession> abSession = 
	         do_GetService(NS_ADDRBOOKSESSION_CONTRACTID, &rv); 
	if(NS_SUCCEEDED(rv))
		abSession->GetUserProfileDirectory(&dbPath);
	
	/* create address book database  */
	if (dbPath)
	{
		(*dbPath) += fileName;
		nsCOMPtr<nsIAddrDatabase> addrDBFactory = 
		         do_GetService(NS_ADDRDATABASE_CONTRACTID, &rv);
		if (NS_SUCCEEDED(rv) && addrDBFactory)
			rv = addrDBFactory->Open(dbPath, PR_TRUE, getter_AddRefs(mDatabase), PR_TRUE);
	}
	NS_ENSURE_SUCCESS(rv, rv);

    delete dbPath;

    nsCOMPtr<nsIRDFService> rdfService = do_GetService (NS_RDF_CONTRACTID "/rdf-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
	nsCOMPtr<nsIRDFResource> parentResource;
	char *parentUri = PR_smprintf("%s", kAllDirectoryRoot);
	rv = rdfService->GetResource(parentUri, getter_AddRefs(parentResource));
	nsCOMPtr<nsIAbDirectory> parentDir = do_QueryInterface(parentResource);
	if (!parentDir)
		return NS_ERROR_NULL_POINTER;
	if (parentUri)
		PR_smprintf_free(parentUri);

	if (PL_strcmp(fileName, kPersonalAddressbook) == 0)
	{
		// This is the personal address book, get name from prefs
		nsCOMPtr<nsIPref> pPref(do_GetService(NS_PREF_CONTRACTID, &rv)); 
		if (NS_FAILED(rv) || !pPref) 
			return nsnull;
		
	        nsXPIDLString dirName;
		rv = pPref->GetLocalizedUnicharPref("ldap_2.servers.pab.description", getter_Copies(dirName));
		parentDir->CreateDirectoryByURI(dirName, mDbUri, mMigrating);
	}
	else
		parentDir->CreateDirectoryByURI(fileString.get(), mDbUri, mMigrating);
		
	rv = ParseLDIFFile();

	if (leafName)
		nsCRT::free(leafName);
	if (fileName)
		PR_smprintf_free(fileName);

	return rv;
}

#define RIGHT2			0x03
#define RIGHT4			0x0f
#define CONTINUED_LINE_MARKER	'\001'

static unsigned char b642nib[0x80] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0x3e, 0xff, 0xff, 0xff, 0x3f,
	0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b,
	0x3c, 0x3d, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
	0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
	0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
	0x17, 0x18, 0x19, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
	0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
	0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30,
	0x31, 0x32, 0x33, 0xff, 0xff, 0xff, 0xff, 0xff
};

/*
 * str_parse_line - takes a line of the form "type:[:] value" and splits it
 * into components "type" and "value".  if a double colon separates type from
 * value, then value is encoded in base 64, and parse_line un-decodes it
 * (in place) before returning.
 */

nsresult AddressBookParser::str_parse_line(
    char	*line,
    char	**type,
    char	**value,
    int		*vlen
)
{
	char	*p, *s, *d, *byte, *stop;
	char	nib;
	int	i, b64;

	/* skip any leading space */
	while ( IS_SPACE( *line ) ) {
		line++;
	}
	*type = line;

	for ( s = line; *s && *s != ':'; s++ )
		;	/* NULL */
	if ( *s == '\0' ) {
		return NS_ERROR_FAILURE;
	}

	/* trim any space between type and : */
	for ( p = s - 1; p > line && nsString::IsSpace( *p ); p-- ) {
		*p = '\0';
	}
	*s++ = '\0';

	/* check for double : - indicates base 64 encoded value */
	if ( *s == ':' ) {
		s++;
		b64 = 1;

	/* single : - normally encoded value */
	} else {
		b64 = 0;
	}

	/* skip space between : and value */
	while ( IS_SPACE( *s ) ) {
		s++;
	}

	/* if no value is present, error out */
	if ( *s == '\0' ) {
		return NS_ERROR_FAILURE;
	}

	/* check for continued line markers that should be deleted */
	for ( p = s, d = s; *p; p++ ) {
		if ( *p != CONTINUED_LINE_MARKER )
			*d++ = *p;
	}
	*d = '\0';

	*value = s;
	if ( b64 ) {
		stop = PL_strchr( s, '\0' );
		byte = s;
		for ( p = s, *vlen = 0; p < stop; p += 4, *vlen += 3 ) {
			for ( i = 0; i < 3; i++ ) {
				if ( p[i] != '=' && (p[i] & 0x80 ||
				    b642nib[ p[i] & 0x7f ] > 0x3f) ) {
					return NS_ERROR_FAILURE;
				}
			}

			/* first digit */
			nib = b642nib[ p[0] & 0x7f ];
			byte[0] = nib << 2;
			/* second digit */
			nib = b642nib[ p[1] & 0x7f ];
			byte[0] |= nib >> 4;
			byte[1] = (nib & RIGHT4) << 4;
			/* third digit */
			if ( p[2] == '=' ) {
				*vlen += 1;
				break;
			}
			nib = b642nib[ p[2] & 0x7f ];
			byte[1] |= nib >> 2;
			byte[2] = (nib & RIGHT2) << 6;
			/* fourth digit */
			if ( p[3] == '=' ) {
				*vlen += 2;
				break;
			}
			nib = b642nib[ p[3] & 0x7f ];
			byte[2] |= nib;

			byte += 3;
		}
		s[ *vlen ] = '\0';
	} else {
		*vlen = (int) (d - s);
	}

	return NS_OK;
}

/*
 * str_getline - return the next "line" (minus newline) of input from a
 * string buffer of lines separated by newlines, terminated by \n\n
 * or \0.  this routine handles continued lines, bundling them into
 * a single big line before returning.  if a line begins with a white
 * space character, it is a continuation of the previous line. the white
 * space character (nb: only one char), and preceeding newline are changed
 * into CONTINUED_LINE_MARKER chars, to be deleted later by the
 * str_parse_line() routine above.
 *
 * it takes a pointer to a pointer to the buffer on the first call,
 * which it updates and must be supplied on subsequent calls.
 */

char * AddressBookParser::str_getline( char **next )
{
	char	*lineStr;
	char	c;

	if ( *next == NULL || **next == '\n' || **next == '\0' ) {
		return( NULL );
	}

	lineStr = *next;
	while ( (*next = PL_strchr( *next, '\n' )) != NULL ) {
		c = *(*next + 1);
		if ( IS_SPACE ( c ) && c != '\n' ) {
			**next = CONTINUED_LINE_MARKER;
			*(*next+1) = CONTINUED_LINE_MARKER;
		} else {
			*(*next)++ = '\0';
			break;
		}
	}

	return( lineStr );
}

/*
 * get one ldif record
 * 
 */
nsresult AddressBookParser::GetLdifStringRecord(char* buf, PRInt32 len, PRInt32* stopPos)
{
	PRInt32 LFCount = 0;
	PRInt32 CRCount = 0;

	for (; *stopPos < len; (*stopPos)++) 
	{
		char c = buf[*stopPos];

		if (c == 0xA)
		{
			LFCount++;
		}
		else if (c == 0xD)
		{
			CRCount++;
		}
		else if ( c != 0xA && c != 0xD)
		{
			if (LFCount == 0 && CRCount == 0)
         		mLine.Append(c);
			else if (( LFCount > 1) || ( CRCount > 2 && LFCount ) ||
				( !LFCount && CRCount > 1 ))
			{
				return NS_OK;
			}
			else if ((LFCount == 1 || CRCount == 1))
			{
         		mLine.Append('\n');
         		mLine.Append(c);
				LFCount = 0;
				CRCount = 0;
			}
		}
	}
	if (*stopPos ==len && (LFCount > 1) || (CRCount > 2 && LFCount) ||
		(!LFCount && CRCount > 1))
		return NS_OK;
	else
		return NS_ERROR_FAILURE;
}

nsresult AddressBookParser::ParseLDIFFile()
{
    char buf[1024];
	char* pBuf = &buf[0];
	PRInt32 startPos = 0;
    PRInt32 len = 0;
	PRBool bEof = PR_FALSE;
	nsVoidArray listPosArray;
	PRInt32 savedStartPos = 0;
	PRInt32 filePos = 0;

	while (NS_SUCCEEDED(mFileSpec->Eof(&bEof)) && !bEof)
	{
		if (NS_SUCCEEDED(mFileSpec->Read(&pBuf, (PRInt32)sizeof(buf), &len)) && len > 0)
		{
			startPos = 0;

			while (NS_SUCCEEDED(GetLdifStringRecord(buf, len, &startPos)))
			{
				if (mLine.Find("groupOfNames") == kNotFound)
					AddLdifRowToDatabase(PR_FALSE);
				else
				{
					//keep file position for mailing list
					listPosArray.AppendElement((void*)savedStartPos);
					if (mLine.Length() > 0)
						mLine.Truncate();
				}
				savedStartPos = filePos + startPos;
			}
			filePos += len;
		}
	}
	//last row
	if (mLine.Length() > 0 && mLine.Find("groupOfNames") == kNotFound)
		AddLdifRowToDatabase(PR_FALSE); 

	// mail Lists
    PRInt32 i;
	PRInt32 listTotal = listPosArray.Count();
	for (i = 0; i < listTotal; i++)
	{
		PRInt32 pos = NS_PTR_TO_INT32(listPosArray.ElementAt(i));
		if (NS_SUCCEEDED(mFileSpec->Seek(pos)))
		{
			if (NS_SUCCEEDED(mFileSpec->Read(&pBuf, (PRInt32)sizeof(buf), &len)) && len > 0)
			{
				startPos = 0;

				while (NS_SUCCEEDED(GetLdifStringRecord(buf, len, &startPos)))
				{
					if (mLine.Find("groupOfNames") != kNotFound)
					{
						AddLdifRowToDatabase(PR_TRUE);
						if (NS_SUCCEEDED(mFileSpec->Seek(0)))
							break;
					}
				}
			}
		}
	}
	return NS_OK;
}

void AddressBookParser::AddLdifRowToDatabase(PRBool bIsList)
{
	nsCOMPtr <nsIMdbRow> newRow;
	if (mDatabase)
	{
		if (bIsList)
			mDatabase->GetNewListRow(getter_AddRefs(newRow)); 
		else
			mDatabase->GetNewRow(getter_AddRefs(newRow)); 

		if (!newRow)
			return;
	}
	else
		return;

	char* cursor = ToNewCString(mLine); 
	char* saveCursor = cursor;  /* keep for deleting */ 
	char* line = 0; 
	char* typeSlot = 0; 
	char* valueSlot = 0; 
	int length = 0;  // the length  of an ldif attribute
	while ( (line = str_getline(&cursor)) != NULL)
	{
		if ( str_parse_line(line, &typeSlot, &valueSlot, &length) == 0 )
		{
			AddLdifColToDatabase(newRow, typeSlot, valueSlot, bIsList);
		}
		else
			continue; // parse error: continue with next loop iteration
	}
	delete [] saveCursor;
	mDatabase->AddCardRowToDB(newRow);	

	if (bIsList)
		mDatabase->AddListDirNode(newRow);

	if (mLine.Length() > 0)
		mLine.Truncate();
}

// We have two copies of this function in the code, one here for import and 
// the other one in addrbook/src/nsAddressBook.cpp for migrating.  If ths 
// function need modification, make sure change in both places until we resolve
// this problem.
void AddressBookParser::AddLdifColToDatabase(nsIMdbRow* newRow, char* typeSlot, char* valueSlot, PRBool bIsList)
{
    nsCAutoString colType(typeSlot);
    nsCAutoString column(valueSlot);

	mdb_u1 firstByte = (mdb_u1)(colType.get())[0];
	switch ( firstByte )
	{
	case 'b':
	  if ( kNotFound != colType.Find("birthyear") )
		mDatabase->AddBirthYear(newRow, column.get());
	  break; // 'b'

	case 'c':
	  if ( kNotFound != colType.Find("cn") || kNotFound != colType.Find("commonname") )
	  {
		if (bIsList)
		  mDatabase->AddListName(newRow, column.get());
		else
		  mDatabase->AddDisplayName(newRow, column.get());
	  }
	  else if ( kNotFound != colType.Find("countryname") )
 	  {
      if (mStoreLocAsHome )
        mDatabase->AddHomeCountry(newRow, column.get());
      else
	    mDatabase->AddWorkCountry(newRow, column.get());
      }

	  // else if ( kNotFound != colType.Find("charset") )
	  //   ioRow->AddColumn(ev, this->ColCharset(), yarn);

	  else if ( kNotFound != colType.Find("cellphone") )
		mDatabase->AddCellularNumber(newRow, column.get());

//		  else if ( kNotFound != colType.Find("calendar") )
//			ioRow->AddColumn(ev, this->ColCalendar(), yarn);

//		  else if ( kNotFound != colType.Find("car") )
//			ioRow->AddColumn(ev, this->ColCar(), yarn);

	  else if ( kNotFound != colType.Find("carphone") )
		mDatabase->AddCellularNumber(newRow, column.get());
//			ioRow->AddColumn(ev, this->ColCarPhone(), yarn);

//		  else if ( kNotFound != colType.Find("carlicense") )
//			ioRow->AddColumn(ev, this->ColCarLicense(), yarn);
        
	  else if ( kNotFound != colType.Find("custom1") )
		mDatabase->AddCustom1(newRow, column.get());
        
	  else if ( kNotFound != colType.Find("custom2") )
		mDatabase->AddCustom2(newRow, column.get());
        
	  else if ( kNotFound != colType.Find("custom3") )
		mDatabase->AddCustom3(newRow, column.get());
        
	  else if ( kNotFound != colType.Find("custom4") )
		mDatabase->AddCustom4(newRow, column.get());
        
	  else if ( kNotFound != colType.Find("company") )
		mDatabase->AddCompany(newRow, column.get());
	  break; // 'c'

	case 'd':
	  if ( kNotFound != colType.Find("description") )
	  {
		if (bIsList)
		  mDatabase->AddListDescription(newRow, column.get());
		else
		  mDatabase->AddNotes(newRow, column.get());
	  }
//		  else if ( kNotFound != colType.Find("dn") ) // distinuished name
//			ioRow->AddColumn(ev, this->ColDistName(), yarn);

	  else if ( kNotFound != colType.Find("department") )
		mDatabase->AddDepartment(newRow, column.get());

//		  else if ( kNotFound != colType.Find("departmentnumber") )
//			ioRow->AddColumn(ev, this->ColDepartmentNumber(), yarn);

//		  else if ( kNotFound != colType.Find("date") )
//			ioRow->AddColumn(ev, this->ColDate(), yarn);
	  break; // 'd'

	case 'e':

//		  if ( kNotFound != colType.Find("employeeid") )
//			ioRow->AddColumn(ev, this->ColEmployeeId(), yarn);

//		  else if ( kNotFound != colType.Find("employeetype") )
//			ioRow->AddColumn(ev, this->ColEmployeeType(), yarn);
	  break; // 'e'

	case 'f':

	  if ( kNotFound != colType.Find("fax") ||
		kNotFound != colType.Find("facsimiletelephonenumber") )
		mDatabase->AddFaxNumber(newRow, column.get());
	  break; // 'f'

	case 'g':
	  if ( kNotFound != colType.Find("givenname") )
		mDatabase->AddFirstName(newRow, column.get());

//		  else if ( kNotFound != colType.Find("gif") )
//			ioRow->AddColumn(ev, this->ColGif(), yarn);

//		  else if ( kNotFound != colType.Find("geo") )
//			ioRow->AddColumn(ev, this->ColGeo(), yarn);

	  break; // 'g'

	case 'h':
	  if ( kNotFound != colType.Find("homephone") )
		mDatabase->AddHomePhone(newRow, column.get());

	  else if ( kNotFound != colType.Find("homeurl") )
		mDatabase->AddWebPage1(newRow, column.get());
	  break; // 'h'

	case 'i':
//		  if ( kNotFound != colType.Find("imapurl") )
//			ioRow->AddColumn(ev, this->ColImapUrl(), yarn);
	  break; // 'i'

	case 'j':
//		  if ( kNotFound != colType.Find("jpeg") || kNotFound != colType.Find("jpegfile") )
//			ioRow->AddColumn(ev, this->ColJpegFile(), yarn);

	  break; // 'j'

	case 'k':
//		  if ( kNotFound != colType.Find("key") )
//			ioRow->AddColumn(ev, this->ColKey(), yarn);

//		  else if ( kNotFound != colType.Find("keywords") )
//			ioRow->AddColumn(ev, this->ColKeywords(), yarn);

	  break; // 'k'

	case 'l':
	  if ( kNotFound != colType.Find("l") || kNotFound != colType.Find("locality") )
 	  {
      if (mStoreLocAsHome )
          mDatabase->AddHomeCity(newRow, column.get());
      else
	      mDatabase->AddWorkCity(newRow, column.get());
      }

//		  else if ( kNotFound != colType.Find("language") )
//			ioRow->AddColumn(ev, this->ColLanguage(), yarn);

//		  else if ( kNotFound != colType.Find("logo") )
//			ioRow->AddColumn(ev, this->ColLogo(), yarn);

//		  else if ( kNotFound != colType.Find("location") )
//			ioRow->AddColumn(ev, this->ColLocation(), yarn);

	  break; // 'l'

	case 'm':
	  if ( kNotFound != colType.Find("mail") )
		mDatabase->AddPrimaryEmail(newRow, column.get());

	  else if ( kNotFound != colType.Find("member") && bIsList )
		mDatabase->AddLdifListMember(newRow, column.get());

//		  else if ( kNotFound != colType.Find("manager") )
//			ioRow->AddColumn(ev, this->ColManager(), yarn);

//		  else if ( kNotFound != colType.Find("modem") )
//			ioRow->AddColumn(ev, this->ColModem(), yarn);

//		  else if ( kNotFound != colType.Find("msgphone") )
//			ioRow->AddColumn(ev, this->ColMessagePhone(), yarn);

	  break; // 'm'

	case 'n':
//		  if ( kNotFound != colType.Find("note") )
//			ioRow->AddColumn(ev, this->ColNote(), yarn);

	  if ( kNotFound != colType.Find("notes") )
		mDatabase->AddNotes(newRow, column.get());

//		  else if ( kNotFound != colType.Find("n") )
//			ioRow->AddColumn(ev, this->ColN(), yarn);

//		  else if ( kNotFound != colType.Find("notifyurl") )
//			ioRow->AddColumn(ev, this->ColNotifyUrl(), yarn);

	  break; // 'n'

	case 'o':
	  if ( kNotFound != colType.Find("objectclass"))
		break;

	  else if ( kNotFound != colType.Find("ou") || kNotFound != colType.Find("orgunit") )
		mDatabase->AddDepartment(newRow, column.get());

	  else if ( kNotFound != colType.Find("o") ) // organization
		mDatabase->AddCompany(newRow, column.get());

	  break; // 'o'

	case 'p':
	  if ( kNotFound != colType.Find("postalcode") )
	  {
      if (mStoreLocAsHome )
          mDatabase->AddHomeZipCode(newRow, column.get());
      else
	      mDatabase->AddWorkZipCode(newRow, column.get());
      }

	  else if ( kNotFound != colType.Find("postOfficeBox") )
		mDatabase->AddWorkAddress(newRow, column.get());

	  else if ( kNotFound != colType.Find("pager") ||
		kNotFound != colType.Find("pagerphone") )
		mDatabase->AddPagerNumber(newRow, column.get());
                    
//		  else if ( kNotFound != colType.Find("photo") )
//			ioRow->AddColumn(ev, this->ColPhoto(), yarn);

//		  else if ( kNotFound != colType.Find("parentphone") )
//			ioRow->AddColumn(ev, this->ColParentPhone(), yarn);

//		  else if ( kNotFound != colType.Find("pageremail") )
//			ioRow->AddColumn(ev, this->ColPagerEmail(), yarn);

//		  else if ( kNotFound != colType.Find("prefurl") )
//			ioRow->AddColumn(ev, this->ColPrefUrl(), yarn);

//		  else if ( kNotFound != colType.Find("priority") )
//			ioRow->AddColumn(ev, this->ColPriority(), yarn);

	  break; // 'p'

	case 'r':
	  if ( kNotFound != colType.Find("region") )
 	  {
      if (mStoreLocAsHome )
        mDatabase->AddHomeZipCode(newRow, column.get());
      else
	    mDatabase->AddWorkState(newRow, column.get());
      }

//		  else if ( kNotFound != colType.Find("rfc822mailbox") )
//			ioRow->AddColumn(ev, this->ColPrimaryEmail(), yarn);

//		  else if ( kNotFound != colType.Find("rev") )
//			ioRow->AddColumn(ev, this->ColRev(), yarn);

//		  else if ( kNotFound != colType.Find("role") )
//			ioRow->AddColumn(ev, this->ColRole(), yarn);
	  break; // 'r'

	case 's':
	  if ( kNotFound != colType.Find("sn") || kNotFound != colType.Find("surname") )
		mDatabase->AddLastName(newRow, column.get());

	  else if ( kNotFound != colType.Find("streetaddress") )
 	  {
      if (mStoreLocAsHome )
          mDatabase->AddHomeAddress(newRow, column.get());
      else
	      mDatabase->AddWorkAddress(newRow, column.get());
      }

	  else if ( kNotFound != colType.Find("st") )
	  {
      if (mStoreLocAsHome )
        mDatabase->AddHomeState(newRow, column.get());
      else
	    mDatabase->AddWorkState(newRow, column.get());
      }

//		  else if ( kNotFound != colType.Find("secretary") )
//			ioRow->AddColumn(ev, this->ColSecretary(), yarn);

//		  else if ( kNotFound != colType.Find("sound") )
//			ioRow->AddColumn(ev, this->ColSound(), yarn);

//		  else if ( kNotFound != colType.Find("sortstring") )
//			ioRow->AddColumn(ev, this->ColSortString(), yarn);
        
	  break; // 's'

	case 't':
	  if ( kNotFound != colType.Find("title") )
		mDatabase->AddJobTitle(newRow, column.get());

	  else if ( kNotFound != colType.Find("telephonenumber") )
      {
		mDatabase->AddWorkPhone(newRow, column.get());
      }

//		  else if ( kNotFound != colType.Find("tiff") )
//			ioRow->AddColumn(ev, this->ColTiff(), yarn);

//		  else if ( kNotFound != colType.Find("tz") )
//			ioRow->AddColumn(ev, this->ColTz(), yarn);
	  break; // 't'

	case 'u':

		if ( kNotFound != colType.Find("uniquemember") && bIsList )
			mDatabase->AddLdifListMember(newRow, column.get());

//		  else if ( kNotFound != colType.Find("uid") )
//			ioRow->AddColumn(ev, this->ColUid(), yarn);

	  break; // 'u'

	case 'v':
//		  if ( kNotFound != colType.Find("version") )
//			ioRow->AddColumn(ev, this->ColVersion(), yarn);

//		  else if ( kNotFound != colType.Find("voice") )
//			ioRow->AddColumn(ev, this->ColVoice(), yarn);

	  break; // 'v'

	case 'w':
	  if ( kNotFound != colType.Find("workurl") )
		mDatabase->AddWebPage2(newRow, column.get());

	  break; // 'w'

	case 'x':
	  if ( kNotFound != colType.Find("xmozillanickname") )
	  {
		if (bIsList)
		  mDatabase->AddListNickName(newRow, column.get());
		else
		  mDatabase->AddNickName(newRow, column.get());
	  }

	  else if ( kNotFound != colType.Find("xmozillausehtmlmail") )
	  {
		ToLowerCase(column);
		if (kNotFound != column.Find("true"))
			mDatabase->AddPreferMailFormat(newRow, nsIAbPreferMailFormat::html);
		else
			mDatabase->AddPreferMailFormat(newRow, nsIAbPreferMailFormat::unknown);
	  }

	  break; // 'x'

	case 'z':
	  if ( kNotFound != colType.Find("zip") ) // alias for postalcode
 	  {
      if (mStoreLocAsHome )
        mDatabase->AddHomeZipCode(newRow, column.get());
      else
	    mDatabase->AddWorkZipCode(newRow, column.get());
      }

	  break; // 'z'

	default:
	  break; // default
	}
}

NS_IMETHODIMP nsAddressBook::ConvertNA2toLDIF(nsIFileSpec *srcFileSpec, nsIFileSpec *dstFileSpec)
{
  nsresult rv = NS_OK;
  if (!srcFileSpec || !dstFileSpec) return NS_ERROR_NULL_POINTER;
  
  nsCOMPtr <nsIAbUpgrader> abUpgrader = do_GetService(NS_AB4xUPGRADER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!abUpgrader) return NS_ERROR_FAILURE;

  rv = abUpgrader->StartUpgrade4xAddrBook(srcFileSpec, dstFileSpec);
  if (NS_SUCCEEDED(rv)) {
    PRBool done = PR_FALSE;
    
    do {
      rv = abUpgrader->ContinueExport(&done);
      // XXX todo 
      // put this in the msg status
      printf("converting na2 to ldif...\n");
    } while (NS_SUCCEEDED(rv) && !done);
  }
  return rv;  
}

NS_IMETHODIMP nsAddressBook::ConvertLDIFtoMAB(nsIFileSpec *fileSpec, PRBool migrating, nsIAddrDatabase *db, PRBool bStoreLocAsHome, PRBool bImportingComm4x)
{
    nsresult rv;
    if (!fileSpec) return NS_ERROR_FAILURE;

    rv = fileSpec->OpenStreamForReading();
    NS_ENSURE_SUCCESS(rv, rv);

    AddressBookParser abParser(fileSpec, migrating, db, bStoreLocAsHome, bImportingComm4x);

    rv = abParser.ParseFile();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = fileSpec->CloseStream();
    return rv;
}

#define CSV_DELIM ","
#define CSV_DELIM_LEN 1
#define TAB_DELIM "\t"
#define TAB_DELIM_LEN 1
#define CSV_FILE_EXTENSION ".csv"

NS_IMETHODIMP nsAddressBook::ExportAddressBook(nsIAbDirectory *aDirectory)
{
  nsresult rv;

  nsCOMPtr<nsIFilePicker> filePicker = do_CreateInstance("@mozilla.org/filepicker;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIStringBundleService> bundleService = do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIStringBundle> bundle;
  rv = bundleService->CreateBundle("chrome://messenger/locale/addressbook/addressBook.properties", getter_AddRefs(bundle));
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsXPIDLString title;
  rv = bundle->GetStringFromName(NS_LITERAL_STRING("ExportAddressBookTitle").get(), getter_Copies(title));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = filePicker->Init(nsnull, title.get(), nsIFilePicker::modeSave);
  NS_ENSURE_SUCCESS(rv, rv);
 
  nsXPIDLString filterString;
  rv = bundle->GetStringFromName(NS_LITERAL_STRING("CSVFiles").get(), getter_Copies(filterString));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = filePicker->AppendFilter(filterString.get(), NS_LITERAL_STRING("*.csv").get());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = bundle->GetStringFromName(NS_LITERAL_STRING("TABFiles").get(), getter_Copies(filterString));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = filePicker->AppendFilter(filterString.get(), NS_LITERAL_STRING("*.tab;*.txt").get());
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt16 dialogResult;
  filePicker->Show(&dialogResult);

  if (dialogResult == nsIFilePicker::returnCancel)
    return rv;

  nsCOMPtr<nsILocalFile> localFile;
  rv = filePicker->GetFile(getter_AddRefs(localFile));
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (dialogResult == nsIFilePicker::returnReplace) {
    // be extra safe and only delete when the file is really a file
    PRBool isFile;
    rv = localFile->IsFile(&isFile);
    if (NS_SUCCEEDED(rv) && isFile) {
      rv = localFile->Remove(PR_FALSE /* recursive delete */);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  nsXPIDLCString leafName;
  rv = localFile->GetLeafName(getter_Copies(leafName));
  NS_ENSURE_SUCCESS(rv,rv);

  // XXX todo, check if the file suffix is .csv, not just if the file has ".csv" in it
  if (PL_strstr(leafName,CSV_FILE_EXTENSION))
    rv = ExportDirectory(aDirectory, CSV_DELIM, CSV_DELIM_LEN, localFile);
  else 
    rv = ExportDirectory(aDirectory, TAB_DELIM, TAB_DELIM_LEN, localFile);

  return rv;
}

nsresult
nsAddressBook::ExportDirectory(nsIAbDirectory *aDirectory, const char *aDelim, PRUint32 aDelimLen, nsILocalFile *aLocalFile)
{
  nsCOMPtr <nsIEnumerator> cardsEnumerator;
  nsCOMPtr <nsIAbCard> card;

  nsresult rv;

  nsCOMPtr <nsIImportService> importService = do_GetService(NS_IMPORTSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv,rv);

  nsCOMPtr <nsIOutputStream> outputStream;
  rv = NS_NewLocalFileOutputStream(getter_AddRefs(outputStream),
                                   aLocalFile,
                                   PR_CREATE_FILE | PR_WRONLY | PR_TRUNCATE,
                                   0664);

  // the desired file may be read only
  if (NS_FAILED(rv))
    return rv;

  PRUint32 i;
  PRUint32 writeCount;
  PRUint32 length;
   
  // XXX TODO
  // when Outlook Express exports to .csv, the first line of the file is the column descriptions.
  // and I think we should too.  (otherwise, this data is just data, who knows what is what.)
  // but we have to fix the import code to skip the first card before we can do that.
  // the problem is Netscape 4.x didn't this, so if we always skip the first card
  // we'll lose data when importing from 4.x
  // for now, do it how 4.x does it and how the current import code expect it.
  // note, these are more fields that 4.x expects, so Mozilla -> 4.x will not work right.
#ifdef EXPORT_LIKE_OE_DOES
  for (i = 0; i < AB_COLUMN_COUNT; i++) {
    // XXX localize this?
    length = PL_strlen(AB_COLUMN_ARRAY[i]);
    rv = outputStream->Write(AB_COLUMN_ARRAY[i], length, &writeCount);
    NS_ENSURE_SUCCESS(rv,rv);
    if (length != writeCount)
      return NS_ERROR_FAILURE;

    if (i + 1 < AB_COLUMN_COUNT) {
      rv = outputStream->Write(aDelim, aDelimLen, &writeCount);
      NS_ENSURE_SUCCESS(rv,rv);
      if (aDelimLen != writeCount)
        return NS_ERROR_FAILURE;
    }
  }
  rv = outputStream->Write(MSG_LINEBREAK, MSG_LINEBREAK_LEN, &writeCount);
  NS_ENSURE_SUCCESS(rv,rv);
  if (MSG_LINEBREAK_LEN != writeCount)
    return NS_ERROR_FAILURE;
#endif

  rv = aDirectory->GetChildCards(getter_AddRefs(cardsEnumerator));
  if (NS_SUCCEEDED(rv) && cardsEnumerator) {
    nsCOMPtr<nsISupports> item;
    for (rv = cardsEnumerator->First(); NS_SUCCEEDED(rv); rv = cardsEnumerator->Next()) {
      rv = cardsEnumerator->CurrentItem(getter_AddRefs(item));
      if (NS_SUCCEEDED(rv)) {
        nsCOMPtr <nsIAbCard> card = do_QueryInterface(item, &rv);
        NS_ENSURE_SUCCESS(rv,rv);
         
        PRBool isMailList;
        rv = card->GetIsMailList(&isMailList);
        NS_ENSURE_SUCCESS(rv,rv);
       
        if (isMailList) {
          // XXX TODO
          // it's a mailing list, skip for now.
          // 4.x doesn't handle this right, either.
          // we export mailing lists as cards, so when you import back, you get
          // a card, not a mailing list.
        }
        else {
          nsXPIDLString value;
          nsCAutoString valueCStr;

          for (i = 0; i < AB_COLUMN_COUNT; i++) {
            if (AB_COLUMN_ARRAY[i]) {
              rv = card->GetCardValue(AB_COLUMN_ARRAY[i], getter_Copies(value));
              NS_ENSURE_SUCCESS(rv,rv);

              // XXX i18n TODO
              // is this right?  
              // do we want escaped utf8?  base64 encoded data?
              // the import code appears to expect it in the system charset
              // so we'll do that for now.  
              // 
              // one reason against doing the system char set:
              // my machine is set to US-ASCII, but I can have japanese cards
              // in my addressbook.  but if I go to export to a .csv or .tab file
              // the conversion will fail.
              rv = importService->SystemStringFromUnicode(value.get(), valueCStr);
              if (NS_FAILED(rv)) {
                NS_ASSERTION(0, "failed to convert string to system charset");
                valueCStr = "";
              }
              
              length = valueCStr.Length();
              rv = outputStream->Write(valueCStr.get(), length, &writeCount);
              NS_ENSURE_SUCCESS(rv,rv);
              if (length != writeCount)
                return NS_ERROR_FAILURE;
            }
            else {
              // something we don't support, like PO Box.
              // this will matter when we share a table of columns with the LDAP code
            }

            if (i + 1 < AB_COLUMN_COUNT) {
              rv = outputStream->Write(aDelim, aDelimLen, &writeCount);
              NS_ENSURE_SUCCESS(rv,rv);
              if (aDelimLen != writeCount)
                return NS_ERROR_FAILURE;
            }
          }
          rv = outputStream->Write(MSG_LINEBREAK, MSG_LINEBREAK_LEN, &writeCount);
          NS_ENSURE_SUCCESS(rv,rv);
          if (MSG_LINEBREAK_LEN != writeCount)
            return NS_ERROR_FAILURE;
        }
      }
    }
  }

  rv = outputStream->Flush();
  NS_ENSURE_SUCCESS(rv,rv);

  rv = outputStream->Close();
  NS_ENSURE_SUCCESS(rv,rv);
  return NS_OK;
}

CMDLINEHANDLER_IMPL(nsAddressBook,"-addressbook","general.startup.addressbook","chrome://messenger/content/addressbook/addressbook.xul","Start with the addressbook.",NS_ADDRESSBOOKSTARTUPHANDLER_CONTRACTID,"Addressbook Startup Handler",PR_FALSE,"", PR_TRUE)
