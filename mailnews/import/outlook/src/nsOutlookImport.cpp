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
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */


/*

  Outlook Express (Win32) import mail and addressbook interfaces

*/
#include "nscore.h"
#include "nsCRT.h"
#include "nsString.h"
#include "nsIServiceManager.h"
#include "nsIImportService.h"
#include "nsIComponentManager.h"
#include "nsOutlookImport.h"
#include "nsIMemory.h"
#include "nsIImportService.h"
#include "nsIImportMail.h"
#include "nsIImportMailboxDescriptor.h"
#include "nsIImportGeneric.h"
#include "nsIImportAddressBooks.h"
#include "nsIImportABDescriptor.h"
#include "nsIImportFieldMap.h"
#include "nsIOutputStream.h"
#include "nsIAddrDatabase.h"
#include "nsOutlookSettings.h"
#include "nsTextFormatter.h"
#include "nsOutlookStringBundle.h"
#include "nsIStringBundle.h"
#include "OutlookDebugLog.h"

#include "nsOutlookMail.h"

#include "MapiApi.h"

static NS_DEFINE_CID(kImportServiceCID,		NS_IMPORTSERVICE_CID);
static NS_DEFINE_IID(kISupportsIID,			NS_ISUPPORTS_IID);



class ImportMailImpl : public nsIImportMail
{
public:
    ImportMailImpl();
    virtual ~ImportMailImpl();

	static nsresult Create(nsIImportMail** aImport);

    // nsISupports interface
    NS_DECL_ISUPPORTS

    // nsIImportmail interface
  
	/* void GetDefaultLocation (out nsIFileSpec location, out boolean found, out boolean userVerify); */
	NS_IMETHOD GetDefaultLocation(nsIFileSpec **location, PRBool *found, PRBool *userVerify);
	
	/* nsISupportsArray FindMailboxes (in nsIFileSpec location); */
	NS_IMETHOD FindMailboxes(nsIFileSpec *location, nsISupportsArray **_retval);
	
	/* void ImportMailbox (in nsIImportMailboxDescriptor source, in nsIFileSpec destination, out boolean fatalError); */
	NS_IMETHOD ImportMailbox(nsIImportMailboxDescriptor *source, nsIFileSpec *destination, 
								PRUnichar **pErrorLog, PRUnichar **pSuccessLog, PRBool *fatalError);
	
	/* unsigned long GetImportProgress (); */
	NS_IMETHOD GetImportProgress(PRUint32 *_retval);
	
public:
	static void	ReportSuccess( nsString& name, PRInt32 count, nsString *pStream);
	static void ReportError( PRInt32 errorNum, nsString& name, nsString *pStream);
	static void	AddLinebreak( nsString *pStream);
	static void	SetLogs( nsString& success, nsString& error, PRUnichar **pError, PRUnichar **pSuccess);

private:
	nsOutlookMail	m_mail;
	PRUint32		m_bytesDone;
};


class ImportAddressImpl : public nsIImportAddressBooks
{
public:
    ImportAddressImpl();
    virtual ~ImportAddressImpl();

	static nsresult Create(nsIImportAddressBooks** aImport);

    // nsISupports interface
    NS_DECL_ISUPPORTS

    // nsIImportAddressBooks interface
    
	/* PRBool GetSupportsMultiple (); */
	NS_IMETHOD GetSupportsMultiple(PRBool *_retval) { *_retval = PR_TRUE; return( NS_OK);}
	
	/* PRBool GetAutoFind (out wstring description); */
	NS_IMETHOD GetAutoFind(PRUnichar **description, PRBool *_retval);
	
	/* PRBool GetNeedsFieldMap ( nsIFileSpec location); */
	NS_IMETHOD GetNeedsFieldMap(nsIFileSpec *location, PRBool *_retval) { *_retval = PR_FALSE; return( NS_OK);}
	
	/* void GetDefaultLocation (out nsIFileSpec location, out boolean found, out boolean userVerify); */
	NS_IMETHOD GetDefaultLocation(nsIFileSpec **location, PRBool *found, PRBool *userVerify)
		{ return( NS_ERROR_FAILURE);}
	
	/* nsISupportsArray FindAddressBooks (in nsIFileSpec location); */
	NS_IMETHOD FindAddressBooks(nsIFileSpec *location, nsISupportsArray **_retval);
	
	/* nsISupports GetFieldMap (in nsIImportABDescriptor source); */
	NS_IMETHOD InitFieldMap(nsIFileSpec *location, nsIImportFieldMap *fieldMap)
		{ return( NS_ERROR_FAILURE); }
	
	/* void ImportAddressBook (in nsIImportABDescriptor source, in nsISupports destination, in nsISupports fieldMap, out boolean fatalError); */
	NS_IMETHOD ImportAddressBook(	nsIImportABDescriptor *source, 
									nsIAddrDatabase *	destination, 
									nsIImportFieldMap *	fieldMap, 
									PRUnichar **		errorLog,
									PRUnichar **		successLog,
									PRBool *			fatalError);
	
	/* unsigned long GetImportProgress (); */
	NS_IMETHOD GetImportProgress(PRUint32 *_retval);
 
	NS_IMETHOD GetSampleData( PRInt32 index, PRBool *pFound, PRUnichar **pStr)
		{ return( NS_ERROR_FAILURE);}

	NS_IMETHOD SetSampleLocation( nsIFileSpec *) { return( NS_OK); }

private:
	void	ReportSuccess( nsString& name, nsString *pStream);

private:
	PRUint32		m_msgCount;
	PRUint32		m_msgTotal;
	nsOutlookMail	m_address;
};
////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////


nsOutlookImport::nsOutlookImport()
{
    NS_INIT_REFCNT();

	IMPORT_LOG0( "nsOutlookImport Module Created\n");

	nsOutlookStringBundle::GetStringBundle();
}


nsOutlookImport::~nsOutlookImport()
{

	IMPORT_LOG0( "nsOutlookImport Module Deleted\n");

}



NS_IMPL_ISUPPORTS(nsOutlookImport, NS_GET_IID(nsIImportModule));


NS_IMETHODIMP nsOutlookImport::GetName( PRUnichar **name)
{
    NS_PRECONDITION(name != nsnull, "null ptr");
    if (! name)
        return NS_ERROR_NULL_POINTER;

	// nsString	title = "Outlook Express";
	// *name = title.ToNewUnicode();
	*name = nsOutlookStringBundle::GetStringByID( OUTLOOKIMPORT_NAME);
		
    return NS_OK;
}

NS_IMETHODIMP nsOutlookImport::GetDescription( PRUnichar **name)
{
    NS_PRECONDITION(name != nsnull, "null ptr");
    if (! name)
        return NS_ERROR_NULL_POINTER;

	// nsString	desc = "Outlook Express mail and address books";
	// *name = desc.ToNewUnicode();
	*name = nsOutlookStringBundle::GetStringByID( OUTLOOKIMPORT_DESCRIPTION);
		
    return NS_OK;
}

NS_IMETHODIMP nsOutlookImport::GetSupports( char **supports)
{
    NS_PRECONDITION(supports != nsnull, "null ptr");
    if (! supports)
        return NS_ERROR_NULL_POINTER;
       
	*supports = nsCRT::strdup( kOutlookSupportsString);
	return( NS_OK);
}

NS_IMETHODIMP nsOutlookImport::GetSupportsUpgrade( PRBool *pUpgrade)
{
    NS_PRECONDITION(pUpgrade != nsnull, "null ptr");
    if (! pUpgrade)
        return NS_ERROR_NULL_POINTER;
       
	*pUpgrade = PR_TRUE;
	return( NS_OK);
}

NS_IMETHODIMP nsOutlookImport::GetImportInterface( const char *pImportType, nsISupports **ppInterface)
{
    NS_PRECONDITION(pImportType != nsnull, "null ptr");
    if (! pImportType)
        return NS_ERROR_NULL_POINTER;
    NS_PRECONDITION(ppInterface != nsnull, "null ptr");
    if (! ppInterface)
        return NS_ERROR_NULL_POINTER;

	*ppInterface = nsnull;
	nsresult	rv;
	if (!nsCRT::strcmp( pImportType, "mail")) {
		// create the nsIImportMail interface and return it!
		nsIImportMail *	pMail = nsnull;
		nsIImportGeneric *pGeneric = nsnull;
		rv = ImportMailImpl::Create( &pMail);
		if (NS_SUCCEEDED( rv)) {
			NS_WITH_SERVICE( nsIImportService, impSvc, kImportServiceCID, &rv);
			if (NS_SUCCEEDED( rv)) {
				rv = impSvc->CreateNewGenericMail( &pGeneric);
				if (NS_SUCCEEDED( rv)) {
					pGeneric->SetData( "mailInterface", pMail);
					nsString name;
					nsOutlookStringBundle::GetStringByID( OUTLOOKIMPORT_NAME, name);
					pGeneric->SetData( "name", (nsISupports *) name.GetUnicode());
					rv = pGeneric->QueryInterface( kISupportsIID, (void **)ppInterface);
				}
			}
		}
		NS_IF_RELEASE( pMail);
		NS_IF_RELEASE( pGeneric);
		return( rv);
	}
	
	if (!nsCRT::strcmp( pImportType, "addressbook")) {
		// create the nsIImportMail interface and return it!
		nsIImportAddressBooks *	pAddress = nsnull;
		nsIImportGeneric *		pGeneric = nsnull;
		rv = ImportAddressImpl::Create( &pAddress);
		if (NS_SUCCEEDED( rv)) {
			NS_WITH_SERVICE( nsIImportService, impSvc, kImportServiceCID, &rv);
			if (NS_SUCCEEDED( rv)) {
				rv = impSvc->CreateNewGenericAddressBooks( &pGeneric);
				if (NS_SUCCEEDED( rv)) {
					pGeneric->SetData( "addressInterface", pAddress);
					rv = pGeneric->QueryInterface( kISupportsIID, (void **)ppInterface);
				}
			}
		}
		NS_IF_RELEASE( pAddress);
		NS_IF_RELEASE( pGeneric);
		return( rv);
	}
	
	if (!nsCRT::strcmp( pImportType, "settings")) {
		nsIImportSettings *pSettings = nsnull;
		rv = nsOutlookSettings::Create( &pSettings);
		if (NS_SUCCEEDED( rv)) {
			pSettings->QueryInterface( kISupportsIID, (void **)ppInterface);
		}
		NS_IF_RELEASE( pSettings);
		return( rv);
	}
		
	return( NS_ERROR_NOT_AVAILABLE);
}

/////////////////////////////////////////////////////////////////////////////////
nsresult ImportMailImpl::Create(nsIImportMail** aImport)
{
    NS_PRECONDITION(aImport != nsnull, "null ptr");
    if (! aImport)
        return NS_ERROR_NULL_POINTER;

    *aImport = new ImportMailImpl();
    if (! *aImport)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*aImport);
    return NS_OK;
}

ImportMailImpl::ImportMailImpl()
{
    NS_INIT_REFCNT();
}


ImportMailImpl::~ImportMailImpl()
{
}



NS_IMPL_THREADSAFE_ISUPPORTS(ImportMailImpl, NS_GET_IID(nsIImportMail));

NS_IMETHODIMP ImportMailImpl::GetDefaultLocation( nsIFileSpec **ppLoc, PRBool *found, PRBool *userVerify)
{
    NS_PRECONDITION(ppLoc != nsnull, "null ptr");
    NS_PRECONDITION(found != nsnull, "null ptr");
    NS_PRECONDITION(userVerify != nsnull, "null ptr");
    if (!ppLoc || !found || !userVerify)
        return NS_ERROR_NULL_POINTER;
	

	*found = PR_FALSE;
	*ppLoc = nsnull;
	*userVerify = PR_FALSE;
	// We need to verify here that we can get the mail, if true then
	// return a dummy location, otherwise return no location
	CMapiApi	mapi;
	if (!mapi.Initialize())
		return( NS_OK);
	if (!mapi.LogOn())
		return( NS_OK);

	CMapiFolderList	store;
	if (!mapi.IterateStores( store))
		return( NS_OK);

	if (store.GetSize() == 0)
		return( NS_OK);


	nsresult	rv;
	nsIFileSpec *	spec;
	if (NS_FAILED( rv = NS_NewFileSpec( &spec)))
		return( rv);
	
	*found = PR_TRUE;
	*ppLoc = spec;
	*userVerify = PR_FALSE;

	return( NS_OK);
}


NS_IMETHODIMP ImportMailImpl::FindMailboxes( nsIFileSpec *pLoc, nsISupportsArray **ppArray)
{
    NS_PRECONDITION(pLoc != nsnull, "null ptr");
    NS_PRECONDITION(ppArray != nsnull, "null ptr");
    if (!pLoc || !ppArray)
        return NS_ERROR_NULL_POINTER;
	
	return( m_mail.GetMailFolders( ppArray));
}

void ImportMailImpl::AddLinebreak( nsString *pStream)
{
	if (pStream)
		pStream->AppendWithConversion(NS_LINEBREAK);
}

void ImportMailImpl::ReportSuccess( nsString& name, PRInt32 count, nsString *pStream)
{
	if (!pStream)
		return;
	// load the success string
	nsIStringBundle *pBundle = nsOutlookStringBundle::GetStringBundleProxy();
	PRUnichar *pFmt = nsOutlookStringBundle::GetStringByID( OUTLOOKIMPORT_MAILBOX_SUCCESS, pBundle);
	PRUnichar *pText = nsTextFormatter::smprintf( pFmt, name.GetUnicode(), count);
	pStream->Append( pText);
	nsTextFormatter::smprintf_free( pText);
	nsOutlookStringBundle::FreeString( pFmt);
	AddLinebreak( pStream);
	NS_IF_RELEASE( pBundle);
}

void ImportMailImpl::ReportError( PRInt32 errorNum, nsString& name, nsString *pStream)
{
	if (!pStream)
		return;
	// load the error string
	nsIStringBundle *pBundle = nsOutlookStringBundle::GetStringBundleProxy();
	PRUnichar *pFmt = nsOutlookStringBundle::GetStringByID( errorNum);
	PRUnichar *pText = nsTextFormatter::smprintf( pFmt, name.GetUnicode());
	pStream->Append( pText);
	nsTextFormatter::smprintf_free( pText);
	nsOutlookStringBundle::FreeString( pFmt);
	AddLinebreak( pStream);
	NS_IF_RELEASE( pBundle);
}


void ImportMailImpl::SetLogs( nsString& success, nsString& error, PRUnichar **pError, PRUnichar **pSuccess)
{
	if (pError)
		*pError = error.ToNewUnicode();
	if (pSuccess)
		*pSuccess = success.ToNewUnicode();
}

NS_IMETHODIMP ImportMailImpl::ImportMailbox(	nsIImportMailboxDescriptor *pSource, 
												nsIFileSpec *pDestination, 
												PRUnichar **pErrorLog,
												PRUnichar **pSuccessLog,
												PRBool *fatalError)
{
    NS_PRECONDITION(pSource != nsnull, "null ptr");
    NS_PRECONDITION(pDestination != nsnull, "null ptr");
    NS_PRECONDITION(fatalError != nsnull, "null ptr");
	
	nsCOMPtr<nsIStringBundle>	bundle( dont_AddRef( nsOutlookStringBundle::GetStringBundleProxy()));

	nsString	success;
	nsString	error;
    if (!pSource || !pDestination || !fatalError) {
		nsOutlookStringBundle::GetStringByID( OUTLOOKIMPORT_MAILBOX_BADPARAM, error, bundle);
		if (fatalError)
			*fatalError = PR_TRUE;
		SetLogs( success, error, pErrorLog, pSuccessLog);
	    return NS_ERROR_NULL_POINTER;
	}
      
    PRBool		abort = PR_FALSE;
    nsString	name;
    PRUnichar *	pName;
    if (NS_SUCCEEDED( pSource->GetDisplayName( &pName))) {
    	name = pName;
    	nsCRT::free( pName);
    }
    
	PRUint32 mailSize = 0;
	pSource->GetSize( &mailSize);
	if (mailSize == 0) {
		ReportSuccess( name, 0, &success);
		SetLogs( success, error, pErrorLog, pSuccessLog);
		return( NS_OK);
	}
	
	PRUint32 index = 0;
	pSource->GetIdentifier( &index);
	PRInt32	msgCount = 0;
    nsresult rv = NS_OK;

	m_bytesDone = 0;

	rv = m_mail.ImportMailbox( &m_bytesDone, &abort, (PRInt32)index, name.GetUnicode(), pDestination, &msgCount);
    
	if (NS_SUCCEEDED( rv)) {
		ReportSuccess( name, msgCount, &success);
	}
	else {
		ReportError( OUTLOOKIMPORT_MAILBOX_CONVERTERROR, name, &error);
	}

	SetLogs( success, error, pErrorLog, pSuccessLog);

    return( rv);
}


NS_IMETHODIMP ImportMailImpl::GetImportProgress( PRUint32 *pDoneSoFar) 
{ 
    NS_PRECONDITION(pDoneSoFar != nsnull, "null ptr");
    if (! pDoneSoFar)
        return NS_ERROR_NULL_POINTER;
	
	*pDoneSoFar = m_bytesDone;
	return( NS_OK);
}



nsresult ImportAddressImpl::Create(nsIImportAddressBooks** aImport)
{
    NS_PRECONDITION(aImport != nsnull, "null ptr");
    if (! aImport)
        return NS_ERROR_NULL_POINTER;

    *aImport = new ImportAddressImpl();
    if (! *aImport)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*aImport);
    return NS_OK;
}

ImportAddressImpl::ImportAddressImpl()
{
    NS_INIT_REFCNT();
	m_msgCount = 0;
	m_msgTotal = 0;
}


ImportAddressImpl::~ImportAddressImpl()
{
}



NS_IMPL_THREADSAFE_ISUPPORTS(ImportAddressImpl, NS_GET_IID(nsIImportAddressBooks));

	
NS_IMETHODIMP ImportAddressImpl::GetAutoFind(PRUnichar **description, PRBool *_retval)
{
    NS_PRECONDITION(description != nsnull, "null ptr");
    NS_PRECONDITION(_retval != nsnull, "null ptr");
    if (! description || !_retval)
        return NS_ERROR_NULL_POINTER;
    
    *_retval = PR_TRUE;
    nsString str;
 	nsOutlookStringBundle::GetStringByID( OUTLOOKIMPORT_ADDRNAME, str);
	*description = str.ToNewUnicode();
    
    return( NS_OK);
}

	
	
NS_IMETHODIMP ImportAddressImpl::FindAddressBooks(nsIFileSpec *location, nsISupportsArray **_retval)
{
    NS_PRECONDITION(_retval != nsnull, "null ptr");
    if (!_retval)
        return NS_ERROR_NULL_POINTER;
	
	return( m_address.GetAddressBooks( _retval));
}

	
	
NS_IMETHODIMP ImportAddressImpl::ImportAddressBook(	nsIImportABDescriptor *source, 
													nsIAddrDatabase *	destination, 
													nsIImportFieldMap *	fieldMap, 
													PRUnichar **		pErrorLog,
													PRUnichar **		pSuccessLog,
													PRBool *			fatalError)
{
	m_msgCount = 0;
	m_msgTotal = 0;
    NS_PRECONDITION(source != nsnull, "null ptr");
    NS_PRECONDITION(destination != nsnull, "null ptr");
	NS_PRECONDITION(fatalError != nsnull, "null ptr");
	
	nsCOMPtr<nsIStringBundle> bundle( dont_AddRef( nsOutlookStringBundle::GetStringBundleProxy()));

	nsString	success;
	nsString	error;
    if (!source || !destination || !fatalError) {
		IMPORT_LOG0( "*** Bad param passed to outlook address import\n");
		nsOutlookStringBundle::GetStringByID( OUTLOOKIMPORT_ADDRESS_BADPARAM, error, bundle);
		if (fatalError)
			*fatalError = PR_TRUE;
		ImportMailImpl::SetLogs( success, error, pErrorLog, pSuccessLog);
	    return NS_ERROR_NULL_POINTER;
	}
      
    nsString	name;
    PRUnichar *	pName;
    if (NS_SUCCEEDED( source->GetPreferredName( &pName))) {
    	name = pName;
    	nsCRT::free( pName);
    }
    
    
	PRUint32	id;
    if (NS_FAILED( source->GetIdentifier( &id))) {
		ImportMailImpl::ReportError( OUTLOOKIMPORT_ADDRESS_BADSOURCEFILE, name, &error);
		ImportMailImpl::SetLogs( success, error, pErrorLog, pSuccessLog);		
    	return( NS_ERROR_FAILURE);
    }
	
	    
    nsresult rv = NS_OK;
	
	rv = m_address.ImportAddresses( &m_msgCount, &m_msgTotal, name.GetUnicode(), id, destination, error);
    
	if (NS_SUCCEEDED( rv) && error.IsEmpty()) {
		ReportSuccess( name, &success);
	}
	else {
		ImportMailImpl::ReportError( OUTLOOKIMPORT_ADDRESS_CONVERTERROR, name, &error);
	}

	ImportMailImpl::SetLogs( success, error, pErrorLog, pSuccessLog);

	IMPORT_LOG0( "*** Returning from outlook address import\n");

    return( rv);
	
}

	
NS_IMETHODIMP ImportAddressImpl::GetImportProgress(PRUint32 *_retval)
{
    NS_PRECONDITION(_retval != nsnull, "null ptr");
    if (!_retval)
        return NS_ERROR_NULL_POINTER;
	
	PRUint32 result = m_msgCount;
	if (m_msgTotal) {
		result *= 100;
		result /= m_msgTotal;
	}
	else
		result = 0;
	
	if (result > 100)
		result = 100;

	*_retval = result;
	
	return( NS_OK);
}

void ImportAddressImpl::ReportSuccess( nsString& name, nsString *pStream)
{
	if (!pStream)
		return;
	// load the success string
	nsIStringBundle *pBundle = nsOutlookStringBundle::GetStringBundleProxy();
	PRUnichar *pFmt = nsOutlookStringBundle::GetStringByID( OUTLOOKIMPORT_ADDRESS_SUCCESS, pBundle);
	PRUnichar *pText = nsTextFormatter::smprintf( pFmt, name.GetUnicode());
	pStream->Append( pText);
	nsTextFormatter::smprintf_free( pText);
	nsOutlookStringBundle::FreeString( pFmt);
	ImportMailImpl::AddLinebreak( pStream);
	NS_IF_RELEASE( pBundle);
}

