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
  
/*
	Text import module
*/

#include "nsCOMPtr.h"
#include "nsIModule.h"
#include "nsIGenericFactory.h"
#include "nsIServiceManager.h"
#include "nsIRegistry.h"
#include "nsIImportService.h"
#include "nsTextImport.h"

#include "TextDebugLog.h"

static NS_DEFINE_CID(kTextImportCID,    	NS_TEXTIMPORT_CID);
static NS_DEFINE_CID(kImportServiceCID,		NS_IMPORTSERVICE_CID);
static NS_DEFINE_CID(kRegistryCID,			NS_REGISTRY_CID);

/*
	This doesn't work for me because I need to do some
	additional work in RegisterSelf.
	However, I should be able to delegate to the generic implementation
	and just do the extra work I need?
*/


nsresult GetImportModulesRegKey( nsIRegistry *reg, nsRegistryKey *pKey)
{
	nsRegistryKey	nScapeKey;

	nsresult rv = reg->GetSubtree( nsIRegistry::Common, "Netscape", &nScapeKey);
	if (NS_FAILED(rv)) {
		rv = reg->AddSubtree( nsIRegistry::Common, "Netscape", &nScapeKey);
	}
	if (NS_FAILED( rv))
		return( rv);

	nsRegistryKey	iKey;
	rv = reg->GetSubtree( nScapeKey, "Import", &iKey);
	if (NS_FAILED( rv)) {
		rv = reg->AddSubtree( nScapeKey, "Import", &iKey);
	}
	
	if (NS_FAILED( rv))
		return( rv);

	rv = reg->GetSubtree( iKey, "Modules", pKey);
	if (NS_FAILED( rv)) {
		rv = reg->AddSubtree( iKey, "Modules", pKey);
	}

	return( rv);
}

NS_METHOD TextRegister(nsIComponentManager *aCompMgr,
                                            nsIFile *aPath,
                                            const char *registryLocation,
                                            const char *componentType)
{	
	nsresult rv;

    NS_WITH_SERVICE( nsIRegistry, reg, kRegistryCID, &rv);
    if (NS_FAILED(rv)) {
	    IMPORT_LOG0( "*** Import Text, ERROR GETTING THE Registry\n");
    	return rv;
    }
    
    rv = reg->OpenDefault();
    if (NS_FAILED(rv)) {
	    IMPORT_LOG0( "*** Import Text, ERROR OPENING THE REGISTRY\n");
    	return( rv);
    }
    
	nsRegistryKey	importKey;
	
	rv = GetImportModulesRegKey( reg, &importKey);
	if (NS_FAILED( rv)) {
		IMPORT_LOG0( "*** Import Text, ERROR getting Netscape/Import registry key\n");
		return( rv);
	}		
		    
	nsRegistryKey	key;
    rv = reg->AddSubtree( importKey, "Text", &key);
    if (NS_FAILED(rv)) return( rv);
    
    rv = reg->SetString( key, "Supports", kTextSupportsString);
    if (NS_FAILED(rv)) return( rv);
    char *myCID = kTextImportCID.ToString();
    rv = reg->SetString( key, "CLSID", myCID);
    delete [] myCID;
    if (NS_FAILED(rv)) return( rv);  	

	return( rv);
}


NS_GENERIC_FACTORY_CONSTRUCTOR(nsTextImport)

static nsModuleComponentInfo components[] = {
    {	"Text Import Component", 
		NS_TEXTIMPORT_CID,
		"component://mozilla/import/import-text", 
		nsTextImportConstructor,
		&TextRegister,
		nsnull
	}
};

NS_IMPL_NSGETMODULE("nsTextImportModule", components)


