/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8; c-file-style: "stroustrup" -*-
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
  Implementation for a file system RDF data store.
 */

#include <ctype.h> // for toupper()
#include <stdio.h>
#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsIEnumerator.h"
#include "nsIRDFDataSource.h"
#include "nsIRDFNode.h"
#include "nsIRDFObserver.h"
#include "nsIServiceManager.h"
#include "nsString.h"
#include "nsVoidArray.h"  // XXX introduces dependency on raptorbase
#include "nsXPIDLString.h"
#include "nsRDFCID.h"
#include "rdfutil.h"
#include "nsIRDFService.h"
#include "xp_core.h"
#include "plhash.h"
#include "plstr.h"
#include "prmem.h"
#include "prprf.h"
#include "prio.h"
#include "rdf.h"
#include "nsFileSpec.h"
#include "nsFileStream.h"
#include "nsIRDFFileSystem.h"
#include "nsSpecialSystemDirectory.h"
#include "nsEnumeratorUtils.h"

#include "nsIURL.h"
#include "nsNetUtil.h"
#include "nsIChannel.h"
#include "nsIFile.h"

#ifdef	XP_WIN
#include "nsIUnicodeDecoder.h"
#include "nsIPlatformCharset.h"
#include "nsICharsetConverterManager.h"
#include "nsICharsetAlias.h"

#include "windef.h"
#include "winbase.h"
#endif

#ifdef	XP_BEOS
#include <File.h>
#include <NodeInfo.h>
#endif


static NS_DEFINE_CID(kRDFServiceCID,               NS_RDFSERVICE_CID);
static NS_DEFINE_IID(kISupportsIID,                NS_ISUPPORTS_IID);
#ifdef	XP_WIN
static NS_DEFINE_CID(kCharsetConverterManagerCID,  NS_ICHARSETCONVERTERMANAGER_CID);
#endif

static const char kURINC_FileSystemRoot[] = "NC:FilesRoot";



class FileSystemDataSource : public nsIRDFFileSystemDataSource
{
private:
    nsCOMPtr<nsISupportsArray> mObservers;

    static PRInt32 gRefCnt;

    // pseudo-constants
    static nsIRDFResource		*kNC_FileSystemRoot;
    static nsIRDFResource		*kNC_Child;
    static nsIRDFResource		*kNC_Name;
    static nsIRDFResource		*kNC_URL;
    static nsIRDFResource		*kNC_FileSystemObject;
    static nsIRDFResource		*kNC_pulse;
    static nsIRDFResource		*kRDF_InstanceOf;
    static nsIRDFResource		*kRDF_type;

#ifdef	XP_WIN
    static nsIRDFResource		*kNC_IEFavoriteObject;
    static nsIRDFResource		*kNC_IEFavoriteFolder;
    static char			*ieFavoritesDir;
    nsCOMPtr<nsIUnicodeDecoder>	mUnicodeDecoder;
#endif

#ifdef	XP_BEOS
    static nsIRDFResource		*kNC_NetPositiveObject;
    static char			*netPositiveDir;
#endif

public:

    NS_DECL_ISUPPORTS

    FileSystemDataSource(void);
    virtual		~FileSystemDataSource(void);

    // nsIRDFDataSource methods
    NS_DECL_NSIRDFDATASOURCE

    // helper methods
    static PRBool isFileURI(nsIRDFResource* aResource);
    static PRBool isDirURI(nsIRDFResource* aSource);

    static nsresult GetVolumeList(nsISimpleEnumerator **aResult);
    static nsresult GetFolderList(nsIRDFResource *source, PRBool allowHidden, PRBool onlyFirst, nsISimpleEnumerator **aResult);
    static nsresult GetName(nsIRDFResource *source, nsIRDFLiteral** aResult);
    static nsresult GetURL(nsIRDFResource *source, nsIRDFLiteral** aResult);

#ifdef	XP_WIN
    static PRBool   isValidFolder(nsIRDFResource *source);
    static nsresult getIEFavoriteURL(nsIRDFResource *source, nsString aFileURL, nsIRDFLiteral **urlLiteral);
#endif

#ifdef	XP_BEOS
    static nsresult getNetPositiveURL(nsIRDFResource *source, nsString aFileURL, nsIRDFLiteral **urlLiteral);
#endif

};



static	nsIRDFService		*gRDFService = nsnull;
static	FileSystemDataSource	*gFileSystemDataSource = nsnull;

PRInt32 FileSystemDataSource::gRefCnt;

nsIRDFResource		*FileSystemDataSource::kNC_FileSystemRoot;
nsIRDFResource		*FileSystemDataSource::kNC_Child;
nsIRDFResource		*FileSystemDataSource::kNC_Name;
nsIRDFResource		*FileSystemDataSource::kNC_URL;
nsIRDFResource		*FileSystemDataSource::kNC_FileSystemObject;
nsIRDFResource		*FileSystemDataSource::kNC_pulse;
nsIRDFResource		*FileSystemDataSource::kRDF_InstanceOf;
nsIRDFResource		*FileSystemDataSource::kRDF_type;

#ifdef	XP_WIN
nsIRDFResource		*FileSystemDataSource::kNC_IEFavoriteObject;
nsIRDFResource		*FileSystemDataSource::kNC_IEFavoriteFolder;
char			*FileSystemDataSource::ieFavoritesDir;
#endif

#ifdef	XP_BEOS
nsIRDFResource		*FileSystemDataSource::kNC_NetPositiveObject;
char			*FileSystemDataSource::netPositiveDir;
#endif



static const char	kFileProtocol[] = "file://";



PRBool
FileSystemDataSource::isFileURI(nsIRDFResource *r)
{
	PRBool		isFileURIFlag = PR_FALSE;
	const char	*uri = nsnull;
	
	r->GetValueConst(&uri);
	if ((uri) && (!strncmp(uri, kFileProtocol, sizeof(kFileProtocol) - 1)))
	{
		// XXX HACK HACK HACK
		if (!strchr(uri, '#'))
		{
			isFileURIFlag = PR_TRUE;
		}
	}
	return(isFileURIFlag);
}

PRBool
FileSystemDataSource::isDirURI(nsIRDFResource* source)
{
    nsresult    rv;
    const char  *uri = nsnull;

    rv = source->GetValueConst(&uri);
    if (NS_FAILED(rv)) return(PR_FALSE);

    nsCOMPtr<nsIURI> aIURI;
    if (NS_FAILED(rv = NS_NewURI(getter_AddRefs(aIURI), uri)))
	    return(rv);
    if (!aIURI) return(PR_FALSE);

    nsCOMPtr<nsIFileURL>    fileURL = do_QueryInterface(aIURI);
    if (!fileURL)   return(PR_FALSE);
    
    nsCOMPtr<nsIFile> aDir;
    rv = fileURL->GetFile(getter_AddRefs(aDir));
    if (NS_FAILED(rv))  return(PR_FALSE);

    PRBool isDirFlag = PR_FALSE;

    rv = aDir->IsDirectory(&isDirFlag);
    if (NS_FAILED(rv)) return(PR_FALSE);

    return(isDirFlag);
}


FileSystemDataSource::FileSystemDataSource(void)
{
	NS_INIT_REFCNT();

	if (gRefCnt++ == 0)
	{
		nsresult rv = nsServiceManager::GetService(kRDFServiceCID,
					   NS_GET_IID(nsIRDFService),
					   (nsISupports**) &gRDFService);

		PR_ASSERT(NS_SUCCEEDED(rv));

#ifdef	XP_WIN
		nsSpecialSystemDirectory	ieFavoritesFolder(nsSpecialSystemDirectory::Win_Favorites);
		nsFileURL			ieFavoritesURLSpec(ieFavoritesFolder);
		const char			*ieFavoritesURI = ieFavoritesURLSpec.GetAsString();
		if (ieFavoritesURI)
		{
			ieFavoritesDir = nsCRT::strdup(ieFavoritesURI);
		}
		gRDFService->GetResource(NC_NAMESPACE_URI "IEFavorite",       &kNC_IEFavoriteObject);
		gRDFService->GetResource(NC_NAMESPACE_URI "IEFavoriteFolder", &kNC_IEFavoriteFolder);
/*
		NS_WITH_SERVICE(nsIPlatformCharset, platformCharset, kPlatformCharsetCID, &rv);
		if (NS_SUCCEEDED(rv) && (platformCharset))
		{
			nsAutoString	defaultCharset;
			if (NS_SUCCEEDED(rv = platformCharset->GetCharset(kPlatformCharsetSel_4xBookmarkFile,
				defaultCharset)))
				{
					// found the default platform charset
					// now try and get a decoder from it to Unicode
					NS_WITH_SERVICE(nsICharsetConverterManager, charsetConv,
						kCharsetConverterManagerCID, &rv);
					if (NS_SUCCEEDED(rv) && (charsetConv))
					{
						rv = charsetConv->GetUnicodeDecoder(&defaultCharset,
							getter_AddRefs(mUnicodeDecoder));
					}
				}
		}
*/
#endif

		gRDFService->GetResource(kURINC_FileSystemRoot,               &kNC_FileSystemRoot);
		gRDFService->GetResource(NC_NAMESPACE_URI "child",            &kNC_Child);
		gRDFService->GetResource(NC_NAMESPACE_URI "Name",             &kNC_Name);
		gRDFService->GetResource(NC_NAMESPACE_URI "URL",              &kNC_URL);
		gRDFService->GetResource(NC_NAMESPACE_URI "FileSystemObject", &kNC_FileSystemObject);
		gRDFService->GetResource(NC_NAMESPACE_URI "pulse",            &kNC_pulse);

		gRDFService->GetResource(RDF_NAMESPACE_URI "instanceOf",      &kRDF_InstanceOf);
		gRDFService->GetResource(RDF_NAMESPACE_URI "type",            &kRDF_type);

		gFileSystemDataSource = this;
	}
}



FileSystemDataSource::~FileSystemDataSource (void)
{
#ifdef DEBUG_REFS
    --gInstanceCount;
    fprintf(stdout, "%d - RDF: FileSystemDataSource\n", gInstanceCount);
#endif

    if (--gRefCnt == 0) {
        NS_RELEASE(kNC_FileSystemRoot);
        NS_RELEASE(kNC_Child);
        NS_RELEASE(kNC_Name);
        NS_RELEASE(kNC_URL);
	NS_RELEASE(kNC_FileSystemObject);
	NS_RELEASE(kNC_pulse);
        NS_RELEASE(kRDF_InstanceOf);
        NS_RELEASE(kRDF_type);

#ifdef	XP_WIN
	NS_RELEASE(kNC_IEFavoriteObject);
	NS_RELEASE(kNC_IEFavoriteFolder);

        if (ieFavoritesDir)
        {
        	nsCRT::free(ieFavoritesDir);
        	ieFavoritesDir = nsnull;
        }
#endif

        gFileSystemDataSource = nsnull;
        nsServiceManager::ReleaseService(kRDFServiceCID, gRDFService);
        gRDFService = nsnull;
    }
}



NS_IMPL_ISUPPORTS(FileSystemDataSource, NS_GET_IID(nsIRDFDataSource));



NS_IMETHODIMP
FileSystemDataSource::GetURI(char **uri)
{
    NS_PRECONDITION(uri != nsnull, "null ptr");
    if (! uri)
        return NS_ERROR_NULL_POINTER;

    if ((*uri = nsXPIDLCString::Copy("rdf:files")) == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;

	return NS_OK;
}



NS_IMETHODIMP
FileSystemDataSource::GetSource(nsIRDFResource* property,
                                nsIRDFNode* target,
                                PRBool tv,
                                nsIRDFResource** source /* out */)
{
    NS_PRECONDITION(property != nsnull, "null ptr");
    if (! property)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(target != nsnull, "null ptr");
    if (! target)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(source != nsnull, "null ptr");
    if (! source)
        return NS_ERROR_NULL_POINTER;

    *source = nsnull;
	return NS_RDF_NO_VALUE;
}



NS_IMETHODIMP
FileSystemDataSource::GetSources(nsIRDFResource *property,
                                 nsIRDFNode *target,
                                 PRBool tv,
                                 nsISimpleEnumerator **sources /* out */)
{
//	NS_NOTYETIMPLEMENTED("write me");
	return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP
FileSystemDataSource::GetTarget(nsIRDFResource *source,
                                nsIRDFResource *property,
                                PRBool tv,
                                nsIRDFNode **target /* out */)
{
	NS_PRECONDITION(source != nsnull, "null ptr");
	if (! source)
		return NS_ERROR_NULL_POINTER;

	NS_PRECONDITION(property != nsnull, "null ptr");
	if (! property)
		return NS_ERROR_NULL_POINTER;

	NS_PRECONDITION(target != nsnull, "null ptr");
	if (! target)
		return NS_ERROR_NULL_POINTER;

	nsresult		rv = NS_RDF_NO_VALUE;

	// we only have positive assertions in the file system data source.
	if (! tv)
		return NS_RDF_NO_VALUE;

	if (source == kNC_FileSystemRoot)
	{
		if (property == kNC_pulse)
		{
			nsIRDFLiteral	*pulseLiteral;
			gRDFService->GetLiteral(NS_ConvertASCIItoUCS2("12").GetUnicode(), &pulseLiteral);
			*target = pulseLiteral;
			return NS_OK;
		}
	}
	else if (isFileURI(source))
	{
		if (property == kNC_Name)
		{
			nsCOMPtr<nsIRDFLiteral> name;
			rv = GetName(source, getter_AddRefs(name));
			if (NS_FAILED(rv)) return(rv);
			if (!name)	rv = NS_RDF_NO_VALUE;
			if (rv == NS_RDF_NO_VALUE)	return(rv);
			return name->QueryInterface(NS_GET_IID(nsIRDFNode), (void**) target);
		}
		else if (property == kNC_URL)
		{
			nsCOMPtr<nsIRDFLiteral> url;
			rv = GetURL(source, getter_AddRefs(url));
			if (NS_FAILED(rv)) return(rv);
			if (!url)	rv = NS_RDF_NO_VALUE;
			if (rv == NS_RDF_NO_VALUE)	return(rv);

			return url->QueryInterface(NS_GET_IID(nsIRDFNode), (void**) target);
		}
		else if (property == kRDF_type)
		{
			const char	*type;
			rv = kNC_FileSystemObject->GetValueConst(&type);
			if (NS_FAILED(rv)) return(rv);

#ifdef	XP_WIN
			// under Windows, if its an IE favorite, return that type
			if (ieFavoritesDir)
			{
				const char		*uri;
				rv = source->GetValueConst(&uri);
				if (NS_FAILED(rv)) return(rv);

				nsAutoString		theURI;theURI.AssignWithConversion(uri);
				if (theURI.Find(ieFavoritesDir) == 0)
				{
					if (theURI[theURI.Length() - 1] == '/')
					{
						rv = kNC_IEFavoriteFolder->GetValueConst(&type);
					}
					else
					{
						rv = kNC_IEFavoriteObject->GetValueConst(&type);
					}
					if (NS_FAILED(rv)) return rv;
				}
			}
#endif

			nsAutoString	url; url.AssignWithConversion(type);
			nsIRDFLiteral	*literal;
			gRDFService->GetLiteral(url.GetUnicode(), &literal);
			rv = literal->QueryInterface(NS_GET_IID(nsIRDFNode), (void**) target);
			NS_RELEASE(literal);
			return rv;
		}
		else if (property == kNC_pulse)
		{
			nsIRDFLiteral	*pulseLiteral;
			gRDFService->GetLiteral(NS_ConvertASCIItoUCS2("12").GetUnicode(), &pulseLiteral);
			rv = pulseLiteral->QueryInterface(NS_GET_IID(nsIRDFNode), (void**) target);
			NS_RELEASE(pulseLiteral);
			return rv;
		}
		else if (property == kNC_Child)
		{
			// Oh this is evil. Somebody kill me now.
			nsCOMPtr<nsISimpleEnumerator> children;
			rv = GetFolderList(source, PR_FALSE, PR_TRUE, getter_AddRefs(children));
			if (NS_FAILED(rv)) return rv;

			PRBool hasMore;
			rv = children->HasMoreElements(&hasMore);
			if (NS_FAILED(rv)) return rv;

			if (hasMore)
			{
				nsCOMPtr<nsISupports> isupports;
				rv = children->GetNext(getter_AddRefs(isupports));
				if (NS_FAILED(rv)) return rv;

				return isupports->QueryInterface(NS_GET_IID(nsIRDFNode), (void**) target);
			}
		}
	}

	*target = nsnull;
	return NS_RDF_NO_VALUE;
}



NS_IMETHODIMP
FileSystemDataSource::GetTargets(nsIRDFResource *source,
				nsIRDFResource *property,
				PRBool tv,
				nsISimpleEnumerator **targets /* out */)
{
	NS_PRECONDITION(source != nsnull, "null ptr");
	if (! source)
		return NS_ERROR_NULL_POINTER;

	NS_PRECONDITION(property != nsnull, "null ptr");
	if (! property)
		return NS_ERROR_NULL_POINTER;

	NS_PRECONDITION(targets != nsnull, "null ptr");
	if (! targets)
		return NS_ERROR_NULL_POINTER;

	// we only have positive assertions in the file system data source.
	if (! tv)
		return NS_RDF_NO_VALUE;

	nsresult rv;

	if (source == kNC_FileSystemRoot)
	{
		if (property == kNC_Child)
		{
			return GetVolumeList(targets);
		}
		else if (property == kNC_pulse)
		{
			nsIRDFLiteral	*pulseLiteral;
			gRDFService->GetLiteral(NS_ConvertASCIItoUCS2("12").GetUnicode(), &pulseLiteral);
			nsISimpleEnumerator* result = new nsSingletonEnumerator(pulseLiteral);
			NS_RELEASE(pulseLiteral);

			if (! result)
				return NS_ERROR_OUT_OF_MEMORY;

			NS_ADDREF(result);
			*targets = result;
			return NS_OK;
		}
	}
	else if (isFileURI(source))
	{
		if (property == kNC_Child)
		{
			return GetFolderList(source, PR_FALSE, PR_FALSE, targets);
		}
		else if (property == kNC_Name)
		{
			nsCOMPtr<nsIRDFLiteral> name;
			rv = GetName(source, getter_AddRefs(name));
			if (NS_FAILED(rv)) return rv;

			nsISimpleEnumerator* result = new nsSingletonEnumerator(name);
			if (! result)
				return NS_ERROR_OUT_OF_MEMORY;

			NS_ADDREF(result);
			*targets = result;
			return NS_OK;
		}
		else if (property == kNC_URL)
		{
			nsCOMPtr<nsIRDFLiteral> url;
			rv = GetURL(source, getter_AddRefs(url));
			if (NS_FAILED(rv)) return rv;

			nsISimpleEnumerator* result = new nsSingletonEnumerator(url);
			if (! result)
				return NS_ERROR_OUT_OF_MEMORY;

			NS_ADDREF(result);
			*targets = result;
			return NS_OK;
		}
		else if (property == kRDF_type)
		{
			const	char	*uri = nsnull;
			rv = kNC_FileSystemObject->GetValueConst( &uri );
			if (NS_FAILED(rv)) return rv;

			nsAutoString	url;
			url.AssignWithConversion(uri);

			nsCOMPtr<nsIRDFLiteral>	literal;
			rv = gRDFService->GetLiteral(url.GetUnicode(), getter_AddRefs(literal));
			if (NS_FAILED(rv)) return rv;

			nsISimpleEnumerator* result = new nsSingletonEnumerator(literal);

			if (! result)
				return NS_ERROR_OUT_OF_MEMORY;

			NS_ADDREF(result);
			*targets = result;
			return NS_OK;
		}
		else if (property == kNC_pulse)
		{
			nsCOMPtr<nsIRDFLiteral>	pulseLiteral;
			rv = gRDFService->GetLiteral(NS_ConvertASCIItoUCS2("12").GetUnicode(),
				getter_AddRefs(pulseLiteral));
			if (NS_FAILED(rv)) return rv;

			nsISimpleEnumerator* result = new nsSingletonEnumerator(pulseLiteral);

			if (! result)
				return NS_ERROR_OUT_OF_MEMORY;

			NS_ADDREF(result);
			*targets = result;
			return NS_OK;
		}
	}

	return NS_NewEmptyEnumerator(targets);
}



NS_IMETHODIMP
FileSystemDataSource::Assert(nsIRDFResource *source,
                       nsIRDFResource *property,
                       nsIRDFNode *target,
                       PRBool tv)
{
	return NS_RDF_ASSERTION_REJECTED;
}



NS_IMETHODIMP
FileSystemDataSource::Unassert(nsIRDFResource *source,
                         nsIRDFResource *property,
                         nsIRDFNode *target)
{
	return NS_RDF_ASSERTION_REJECTED;
}



NS_IMETHODIMP
FileSystemDataSource::Change(nsIRDFResource* aSource,
							 nsIRDFResource* aProperty,
							 nsIRDFNode* aOldTarget,
							 nsIRDFNode* aNewTarget)
{
	return NS_RDF_ASSERTION_REJECTED;
}



NS_IMETHODIMP
FileSystemDataSource::Move(nsIRDFResource* aOldSource,
						   nsIRDFResource* aNewSource,
						   nsIRDFResource* aProperty,
						   nsIRDFNode* aTarget)
{
	return NS_RDF_ASSERTION_REJECTED;
}



NS_IMETHODIMP
FileSystemDataSource::HasAssertion(nsIRDFResource *source,
                             nsIRDFResource *property,
                             nsIRDFNode *target,
                             PRBool tv,
                             PRBool *hasAssertion /* out */)
{
	NS_PRECONDITION(source != nsnull, "null ptr");
	if (! source)
		return NS_ERROR_NULL_POINTER;

	NS_PRECONDITION(property != nsnull, "null ptr");
	if (! property)
		return NS_ERROR_NULL_POINTER;

	NS_PRECONDITION(target != nsnull, "null ptr");
	if (! target)
		return NS_ERROR_NULL_POINTER;

	NS_PRECONDITION(hasAssertion != nsnull, "null ptr");
	if (! hasAssertion)
		return NS_ERROR_NULL_POINTER;

	// we only have positive assertions in the file system data source.
	if (! tv) {
		*hasAssertion = PR_FALSE;
		return NS_OK;
	}

	if ((source == kNC_FileSystemRoot) || isFileURI(source))
	{
		if (property == kRDF_type)
		{
			nsCOMPtr<nsIRDFResource> resource( do_QueryInterface(target) );
			if (resource.get() == kRDF_type) {
				*hasAssertion = PR_TRUE;
				return NS_OK;
			}
		}
	}

	*hasAssertion = PR_FALSE;
	return NS_OK;
}


NS_IMETHODIMP 
FileSystemDataSource::HasArcIn(nsIRDFNode *aNode, nsIRDFResource *aArc, PRBool *result)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
FileSystemDataSource::HasArcOut(nsIRDFResource *aSource, nsIRDFResource *aArc, PRBool *result)
{
    if (aSource == kNC_FileSystemRoot) {
	*result = (aArc == kNC_Child || aArc == kNC_pulse);
    }
    else if (isFileURI(aSource)) {
	if (aArc == kNC_pulse) {
	    *result = PR_TRUE;
	}
	else if (isDirURI(aSource)) {
#ifdef	XP_WIN
	    *result = isValidFolder(aSource);
#else
	    *result = PR_TRUE;
#endif
	}
    }
    else {
	*result = PR_FALSE;
    }
    return NS_OK;
}


NS_IMETHODIMP
FileSystemDataSource::ArcLabelsIn(nsIRDFNode *node,
                            nsISimpleEnumerator ** labels /* out */)
{
//	NS_NOTYETIMPLEMENTED("write me");
	return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP
FileSystemDataSource::ArcLabelsOut(nsIRDFResource *source,
				   nsISimpleEnumerator **labels /* out */)
{
    NS_PRECONDITION(source != nsnull, "null ptr");
    if (! source)
	return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(labels != nsnull, "null ptr");
    if (! labels)
	return NS_ERROR_NULL_POINTER;

    nsresult rv;

    if (source == kNC_FileSystemRoot) {
	nsCOMPtr<nsISupportsArray> array;
	rv = NS_NewISupportsArray(getter_AddRefs(array));
	if (NS_FAILED(rv)) return rv;

	array->AppendElement(kNC_Child);
	array->AppendElement(kNC_pulse);

	nsISimpleEnumerator* result = new nsArrayEnumerator(array);
	if (! result)
	    return NS_ERROR_OUT_OF_MEMORY;

	NS_ADDREF(result);
	*labels = result;
	return NS_OK;
    }
    else if (isFileURI(source)) {
	nsCOMPtr<nsISupportsArray> array;
	rv = NS_NewISupportsArray(getter_AddRefs(array));
	if (NS_FAILED(rv)) return rv;

	if (isDirURI(source)) {
#ifdef	XP_WIN
	    if (isValidFolder(source) == PR_TRUE)
	    {
		array->AppendElement(kNC_Child);
	    }
#else
	    array->AppendElement(kNC_Child);
#endif
	    array->AppendElement(kNC_pulse);
	}

	array->AppendElement(kRDF_type);

	nsISimpleEnumerator* result = new nsArrayEnumerator(array);
	if (! result)
	    return NS_ERROR_OUT_OF_MEMORY;

	NS_ADDREF(result);
	*labels = result;
	return NS_OK;
    }

    return NS_NewEmptyEnumerator(labels);
}



NS_IMETHODIMP
FileSystemDataSource::GetAllResources(nsISimpleEnumerator** aCursor)
{
	NS_NOTYETIMPLEMENTED("sorry!");
	return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP
FileSystemDataSource::AddObserver(nsIRDFObserver *n)
{
    NS_PRECONDITION(n != nsnull, "null ptr");
    if (! n)
        return NS_ERROR_NULL_POINTER;

	if (! mObservers)
	{
		nsresult rv;
		rv = NS_NewISupportsArray(getter_AddRefs(mObservers));
		if (NS_FAILED(rv)) return rv;
	}
	mObservers->AppendElement(n);
	return NS_OK;
}



NS_IMETHODIMP
FileSystemDataSource::RemoveObserver(nsIRDFObserver *n)
{
    NS_PRECONDITION(n != nsnull, "null ptr");
    if (! n)
        return NS_ERROR_NULL_POINTER;

	if (! mObservers)
		return NS_OK;

	mObservers->RemoveElement(n);
	return NS_OK;
}



NS_IMETHODIMP
FileSystemDataSource::GetAllCommands(nsIRDFResource* source,
                                     nsIEnumerator/*<nsIRDFResource>*/** commands)
{
	NS_NOTYETIMPLEMENTED("write me!");
	return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP
FileSystemDataSource::GetAllCmds(nsIRDFResource* source,
                                     nsISimpleEnumerator/*<nsIRDFResource>*/** commands)
{
	return(NS_NewEmptyEnumerator(commands));
}



NS_IMETHODIMP
FileSystemDataSource::IsCommandEnabled(nsISupportsArray/*<nsIRDFResource>*/* aSources,
                                       nsIRDFResource*   aCommand,
                                       nsISupportsArray/*<nsIRDFResource>*/* aArguments,
                                       PRBool* aResult)
{
	return(NS_ERROR_NOT_IMPLEMENTED);
}



NS_IMETHODIMP
FileSystemDataSource::DoCommand(nsISupportsArray/*<nsIRDFResource>*/* aSources,
                                nsIRDFResource*   aCommand,
                                nsISupportsArray/*<nsIRDFResource>*/* aArguments)
{
	return(NS_ERROR_NOT_IMPLEMENTED);
}



nsresult
NS_NewRDFFileSystemDataSource(nsIRDFDataSource **result)
{
	if (!result)
		return NS_ERROR_NULL_POINTER;

	// only one file system data source
	if (nsnull == gFileSystemDataSource)
	{
		if ((gFileSystemDataSource = new FileSystemDataSource()) == nsnull)
		{
			return NS_ERROR_OUT_OF_MEMORY;
		}
	}
	NS_ADDREF(gFileSystemDataSource);
	*result = gFileSystemDataSource;
	return NS_OK;
}



nsresult
FileSystemDataSource::GetVolumeList(nsISimpleEnumerator** aResult)
{
	nsresult rv;
	nsCOMPtr<nsISupportsArray> volumes;

	rv = NS_NewISupportsArray(getter_AddRefs(volumes));
	if (NS_FAILED(rv)) return rv;

	nsCOMPtr<nsIRDFResource> vol;

#ifdef	XP_MAC
	StrFileName     fname;
	HParamBlockRec	pb;
	for (int16 volNum = 1; ; volNum++)
	{
		pb.volumeParam.ioCompletion = NULL;
		pb.volumeParam.ioVolIndex = volNum;
		pb.volumeParam.ioNamePtr = (StringPtr)fname;
		if (PBHGetVInfo(&pb,FALSE) != noErr)
			break;
		nsFileSpec fss(pb.volumeParam.ioVRefNum, fsRtParID, fname);
		rv = gRDFService->GetResource(nsFileURL(fss).GetAsString(), getter_AddRefs(vol));
		if (NS_FAILED(rv)) return rv;

		volumes->AppendElement(vol);
	}
#endif

#ifdef	XP_WIN
	PRInt32			driveType;
	char			drive[32];
	PRInt32			volNum;
	char			*url;

	for (volNum = 0; volNum < 26; volNum++)
	{
		sprintf(drive, "%c:\\", volNum + 'A');
		driveType = GetDriveType(drive);
		if (driveType != DRIVE_UNKNOWN && driveType != DRIVE_NO_ROOT_DIR)
		{
			if (nsnull != (url = PR_smprintf("file:///%c|/", volNum + 'A')))
			{
				rv = gRDFService->GetResource(url, getter_AddRefs(vol));
				PR_Free(url);

				if (NS_FAILED(rv)) return rv;
				volumes->AppendElement(vol);
			}
		}
	}
#endif

#if defined(XP_UNIX) || defined(XP_BEOS)
	gRDFService->GetResource("file:///", getter_AddRefs(vol));
	volumes->AppendElement(vol);
#endif

#ifdef XP_OS2
	ULONG ulDriveNo = 0;
	ULONG ulDriveMap = 0;
	char *url;

	rv = DosQueryCurrentDisk(&ulDriveNo, &ulDriveMap);
	if (NS_FAILED(rv))
	    return rv;

	for (int volNum = 0; volNum < 26; volNum++)
	{
	    if (((ulDriveMap << (31 - volNum)) >> 31))
	    {
		if (nsnull != (url = PR_smprintf("file:///%c|/", volNum + 'A')))
		{
		    rv = gRDFService->GetResource(url, getter_AddRefs(vol));
		    PR_Free(url);

		    if (NS_FAILED(rv)) return rv;
		    volumes->AppendElement(vol);
		}
	    }

	}
#endif

	nsISimpleEnumerator* result = new nsArrayEnumerator(volumes);
	if (! result)
		return NS_ERROR_OUT_OF_MEMORY;

	NS_ADDREF(result);
	*aResult = result;

	return NS_OK;
}



#ifdef	XP_WIN
PRBool
FileSystemDataSource::isValidFolder(nsIRDFResource *source)
{
	PRBool	isValid = PR_TRUE;
	if (!ieFavoritesDir)	return(isValid);

	nsresult		rv;
	const char		*uri;
	rv = source->GetValueConst(&uri);
	if (NS_FAILED(rv)) return(isValid);

	PRBool			isIEFavorite = PR_FALSE;
	nsAutoString		theURI; theURI.AssignWithConversion(uri);
	if (theURI.Find(ieFavoritesDir) == 0)
	{
		isValid = PR_FALSE;

		nsCOMPtr<nsISimpleEnumerator>	folderEnum;
		if (NS_SUCCEEDED(rv = GetFolderList(source, PR_TRUE, PR_FALSE, getter_AddRefs(folderEnum))))
		{
			PRBool		hasAny = PR_FALSE, hasMore;
			while (NS_SUCCEEDED(folderEnum->HasMoreElements(&hasMore)) &&
					(hasMore == PR_TRUE))
			{
				hasAny = PR_TRUE;

				nsCOMPtr<nsISupports>		isupports;
				if (NS_FAILED(rv = folderEnum->GetNext(getter_AddRefs(isupports))))
					break;
				nsCOMPtr<nsIRDFResource>	res = do_QueryInterface(isupports);
				if (!res)	break;

				nsCOMPtr<nsIRDFLiteral>		nameLiteral;
				if (NS_FAILED(rv = GetName(res, getter_AddRefs(nameLiteral))))
					break;
				
				const PRUnichar			*uniName;
				if (NS_FAILED(rv = nameLiteral->GetValueConst(&uniName)))
					break;
				nsAutoString			name(uniName);

				// An empty folder, or a folder that contains just "desktop.ini",
				// is considered to be a IE Favorite; otherwise, its a folder
				if (!name.EqualsIgnoreCase("desktop.ini"))
				{
					isValid = PR_TRUE;
					break;
				}
			}
			if (hasAny == PR_FALSE)	isValid = PR_TRUE;
		}
	}
	return(isValid);
}
#endif



nsresult
FileSystemDataSource::GetFolderList(nsIRDFResource *source, PRBool allowHidden,
				PRBool onlyFirst, nsISimpleEnumerator** aResult)
{
	nsresult	                rv;
	nsCOMPtr<nsISupportsArray>  nameArray;

	rv = NS_NewISupportsArray(getter_AddRefs(nameArray));
	if (NS_FAILED(rv)) return rv;

	const char		*parentURI = nsnull;
	rv = source->GetValueConst(&parentURI);
	if (NS_FAILED(rv)) return(rv);
	if (!parentURI)	return(NS_ERROR_UNEXPECTED);

    nsCOMPtr<nsIURI>    aIURI;
    if (NS_FAILED(rv = NS_NewURI(getter_AddRefs(aIURI), parentURI)))
        return(rv);

    nsCOMPtr<nsIFileURL>    fileURL = do_QueryInterface(aIURI);
    if (!fileURL)   return(PR_FALSE);

	nsCOMPtr<nsIFile>	aDir;
    if (NS_FAILED(rv = fileURL->GetFile(getter_AddRefs(aDir))))
		return(rv);

	nsCOMPtr<nsISimpleEnumerator>	dirContents;
	if (NS_FAILED(rv = aDir->GetDirectoryEntries(getter_AddRefs(dirContents))))
		return(rv);
	if (!dirContents)	return(NS_ERROR_UNEXPECTED);

	PRBool			hasMore;
	while(NS_SUCCEEDED(rv = dirContents->HasMoreElements(&hasMore)) &&
		(hasMore == PR_TRUE))
	{
		nsCOMPtr<nsISupports>	isupports;
		if (NS_FAILED(rv = dirContents->GetNext(getter_AddRefs(isupports))))
			break;

		nsCOMPtr<nsIFile>	aFile = do_QueryInterface(isupports);
		if (!aFile)	break;

		if (allowHidden == PR_FALSE)
		{
			PRBool			hiddenFlag = PR_FALSE;
			if (NS_FAILED(rv = aFile->IsHidden(&hiddenFlag)))
				break;
			if (hiddenFlag == PR_TRUE)	continue;
		}

		// XXX We should use nsIFile::GetUnicodeLeafName().
		// But currently mozilla's xpcom/io is not unicode normalization.
		// And URI cannot use UTF-8 (On RFC2396, URI should use UTF-8)
		// So, we uses nsIFile::GetLeafName() for performance...
 
		char            *leafStr = nsnull;
		if (NS_FAILED(rv = aFile->GetLeafName(&leafStr)))
		    break;
		if (!leafStr)   break;
  
		nsCAutoString           fullURI;
		fullURI.Assign(parentURI);
		if (fullURI.Last() != '/')
                {
		    fullURI.Append('/');
                }
  
		nsCAutoString           leaf(leafStr);
		Recycle(leafStr);
		leafStr = nsnull;

		// we need to escape this leaf name;
		// for now, let's just hack it and encode spaces and slashes
		PRInt32			aOffset;
		while ((aOffset = leaf.FindChar(' ')) >= 0)
		{
			leaf.Cut((PRUint32)aOffset, 1);
			leaf.Insert("%20", (PRUint32)aOffset);
		}
		while ((aOffset = leaf.FindChar('/')) >= 0)
		{
			leaf.Cut((PRUint32)aOffset, 1);
			leaf.Insert("%2F", (PRUint32)aOffset);
		}

		// append the encoded name
		fullURI.Append(leaf);

		PRBool			dirFlag = PR_FALSE;
		if (NS_FAILED(rv = aFile->IsDirectory(&dirFlag)))
			break;
		if (dirFlag == PR_TRUE)
		{
			// XXX hmmm, causes problems getting name,
			// so comment it out for the short term
//			fullURI.Append(PRUnichar('/'));
		}

		nsCOMPtr<nsIRDFResource>	fileRes;
		gRDFService->GetResource(fullURI.GetBuffer(), getter_AddRefs(fileRes));

		nameArray->AppendElement(fileRes);

		if (onlyFirst == PR_TRUE)	break;
	}

	nsISimpleEnumerator* result = new nsArrayEnumerator(nameArray);
	if (! result)
		return NS_ERROR_OUT_OF_MEMORY;

	NS_ADDREF(result);
	*aResult = result;

	return NS_OK;
}



nsresult
FileSystemDataSource::GetName(nsIRDFResource *source, nsIRDFLiteral **aResult)
{
	nsresult		rv;
	const char		*uri = nsnull;

	rv = source->GetValueConst(&uri);
	if (NS_FAILED(rv)) return(rv);
	if (!uri)	return(NS_ERROR_UNEXPECTED);

    nsCOMPtr<nsIURI>    aIURI;
    if (NS_FAILED(rv = NS_NewURI(getter_AddRefs(aIURI), uri)))
        return(rv);

    nsCOMPtr<nsIFileURL>    fileURL = do_QueryInterface(aIURI);
    if (!fileURL)   return(PR_FALSE);

	nsCOMPtr<nsIFile>	aFile;
    if (NS_FAILED(rv = fileURL->GetFile(getter_AddRefs(aFile))))
		return(rv);
	if (!aFile)		return(NS_ERROR_UNEXPECTED);

	PRUnichar		*nameUni = nsnull;
	if (NS_FAILED(rv = aFile->GetUnicodeLeafName(&nameUni)))
		return(rv);
	if (!nameUni)		return(NS_ERROR_UNEXPECTED);

	nsAutoString		name(nameUni);
	Recycle(nameUni);
	nameUni = nsnull;

#ifdef	XP_WIN

	// special hack for IE favorites under Windows; strip off the
	// trailing ".url" or ".lnk" at the end of IE favorites names
	PRInt32			nameLen = name.Length();
	nsAutoString		theURI; theURI.AssignWithConversion(uri);
	if ((theURI.Find(ieFavoritesDir) == 0) && (nameLen > 4))
	{
		nsAutoString	extension;
		name.Right(extension, 4);
		if (extension.EqualsIgnoreCase(".url") ||
		    extension.EqualsIgnoreCase(".lnk"))
		{
			name.Truncate(nameLen - 4);
		}
	}
#endif

#ifdef	XP_BEOS
#if 0 	// XXX fix me
	// under BEOS, try and get the "META:title" attribute (if its a file)
	nsAutoString		theURI; theURI.AssignWithConversion(uri);
	if (theURI.Find(netPositiveDir) == 0)
	{
		nsFileSpec		spec(url);
		if (spec.IsFile() && (!spec.IsHidden()))
		{
			const char	*nativeURI = spec.GetNativePathCString();
			if (nativeURI)
			{
				BFile	bf(nativeURI, B_READ_ONLY);
				if (bf.InitCheck() == B_OK)
				{
					char		beNameAttr[4096];
					ssize_t		len;

					if ((len = bf.ReadAttr("META:title", B_STRING_TYPE,
						0, beNameAttr, sizeof(beNameAttr-1))) > 0)
					{
						beNameAttr[len] = '\0';
						name.AssignWithConversion(beNameAttr);
					}
				}
			}
		}
	}
#endif
#endif

	gRDFService->GetLiteral(name.GetUnicode(), aResult);

	return NS_OK;
}



#ifdef	XP_WIN
nsresult
FileSystemDataSource::getIEFavoriteURL(nsIRDFResource *source, nsString aFileURL, nsIRDFLiteral **urlLiteral)
{
	nsresult		rv = NS_OK;

	*urlLiteral = nsnull;

	nsFileURL		url(aFileURL);
	nsFileSpec		uri(url);
	if (uri.IsDirectory())
	{
		if (isValidFolder(source))
			return(NS_RDF_NO_VALUE);
		uri += "desktop.ini";
	}
	else if (aFileURL.Length() > 4)
	{
		nsAutoString	extension;
		aFileURL.Right(extension, 4);
		if (!extension.EqualsIgnoreCase(".url"))
		{
			return(NS_RDF_NO_VALUE);
		}
	}

	nsInputFileStream	stream(uri);

	if (!stream.is_open())
		return(NS_RDF_NO_VALUE);

	char		buffer[256];
	nsAutoString	line;
	while(NS_SUCCEEDED(rv) && (!stream.eof()) && (!stream.failed()))
	{
		PRBool	untruncated = stream.readline(buffer, sizeof(buffer));

		if (stream.failed())
		{
			rv = NS_ERROR_FAILURE;
			break;
		}

		line.AppendWithConversion(buffer);

		if (untruncated || stream.eof())
		{
			if (line.Find("URL=", PR_TRUE) == 0)
			{
				line.Cut(0, 4);
				rv = gRDFService->GetLiteral(line.GetUnicode(), urlLiteral);
				break;
			}
			else if (line.Find("CDFURL=", PR_TRUE) == 0)
			{
				line.Cut(0, 7);
				rv = gRDFService->GetLiteral(line.GetUnicode(), urlLiteral);
				break;
			}
			line.Truncate();
		}

	}

	return(rv);
}
#endif



nsresult
FileSystemDataSource::GetURL(nsIRDFResource *source, nsIRDFLiteral** aResult)
{
	nsresult		rv;
	const char		*uri;
	rv = source->GetValueConst(&uri);
	if (NS_FAILED(rv)) return(rv);
	nsAutoString		url;
	url.AssignWithConversion(uri);

#ifdef	XP_WIN
	// under Windows, if its an IE favorite, munge the URL
	if (ieFavoritesDir)
	{
		if (url.Find(ieFavoritesDir) == 0)
		{
			rv = getIEFavoriteURL(source, url, aResult);
			return(rv);
		}
	}
#endif

#ifdef	XP_BEOS
	// under BEOS, try and get the "META:url" attribute
	if (netPositiveDir)
	{
		if (url.Find(netPositiveDir) == 0)
		{
			rv = getNetPositiveURL(source, url, aResult);
			return(rv);
		}
	}
#endif

	// if we fall through to here, its not any type of bookmark
	// stored in the platform native file system, so just set the URL

	gRDFService->GetLiteral(url.GetUnicode(), aResult);

	return(NS_OK);
}



#ifdef	XP_BEOS

nsresult
FileSystemDataSource::getNetPositiveURL(nsIRDFResource *source, nsString aFileURL, nsIRDFLiteral **urlLiteral)
{
	nsresult		rv = NS_RDF_NO_VALUE;

	*urlLiteral = nsnull;

	nsFileURL		url(aFileURL);
	nsFileSpec		uri(url);
	if (uri.IsFile() && (!uri.IsHidden()))
	{
		const char	*nativeURI = uri.GetNativePathCString();
		if (nativeURI)
		{
			BFile	bf(nativeURI, B_READ_ONLY);
			if (bf.InitCheck() == B_OK)
			{
				char		beURLattr[4096];
				ssize_t		len;

				if ((len = bf.ReadAttr("META:url", B_STRING_TYPE,
					0, beURLattr, sizeof(beURLattr-1))) > 0)
				{
					beURLattr[len] = '\0';
					nsAutoString	bookmarkURL;
                                        bookmarkURL.AssignWithConversion(beURLattr);
					rv = gRDFService->GetLiteral(bookmarkURL.GetUnicode(),
						urlLiteral);
				}
			}
		}
	}
	return(rv);
}

#endif
