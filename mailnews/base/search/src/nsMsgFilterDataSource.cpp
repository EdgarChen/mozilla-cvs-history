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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 * Alec Flett <alecf@netscape.com>
 */

#include "nsMsgFilterDataSource.h"
#include "nsMsgRDFUtils.h"
#include "nsEnumeratorUtils.h"

#include "nsIMsgFilter.h"
#include "nsIMsgFilterList.h"

#include "nsXPIDLString.h"

NS_IMPL_ISUPPORTS1(nsMsgFilterDataSource, nsIRDFDataSource)

    nsrefcnt nsMsgFilterDataSource::mGlobalRefCount = 0;
nsCOMPtr<nsIRDFResource> nsMsgFilterDataSource::kNC_Child;
nsCOMPtr<nsIRDFResource> nsMsgFilterDataSource::kNC_Name;

nsCOMPtr<nsISupportsArray> nsMsgFilterDataSource::mFilterListArcsOut;
nsCOMPtr<nsISupportsArray> nsMsgFilterDataSource::mFilterArcsOut;
  
nsMsgFilterDataSource::nsMsgFilterDataSource()
{
    NS_INIT_ISUPPORTS();
    if (mGlobalRefCount == 0)
        initGlobalObjects(getRDFService());
  
    mGlobalRefCount++;
    /* member initializers and constructor code */
}

nsMsgFilterDataSource::~nsMsgFilterDataSource()
{
    mGlobalRefCount--;
    if (mGlobalRefCount == 0)
        cleanupGlobalObjects();
}

nsresult
nsMsgFilterDataSource::cleanupGlobalObjects()
{
    mFilterListArcsOut = nsnull;
    mFilterArcsOut = nsnull;
    kNC_Child = nsnull;
    kNC_Name = nsnull;
    return NS_OK;
}

nsresult
nsMsgFilterDataSource::initGlobalObjects(nsIRDFService *rdf)
{
    rdf->GetResource(NC_RDF_CHILD, getter_AddRefs(kNC_Child));
    rdf->GetResource(NC_RDF_NAME, getter_AddRefs(kNC_Name));

    NS_NewISupportsArray(getter_AddRefs(mFilterListArcsOut));
    mFilterListArcsOut->AppendElement(kNC_Child);
  
    NS_NewISupportsArray(getter_AddRefs(mFilterArcsOut));
    mFilterArcsOut->AppendElement(kNC_Name);
  
    return NS_OK;
}

NS_IMETHODIMP
nsMsgFilterDataSource::GetTargets(nsIRDFResource *aSource,
                                  nsIRDFResource *aProperty,
                                  PRBool aTruthValue,
                                  nsISimpleEnumerator **aResult)
{
    nsresult rv;
    nsCOMPtr<nsIMsgFilterList> filterList;
    
    nsCOMPtr<nsISupportsArray> resourceList;
    NS_NewISupportsArray(getter_AddRefs(resourceList));

    // first see if it's a filter list
    rv = aSource->GetDelegate("filter", NS_GET_IID(nsIMsgFilterList),
                             (void **)getter_AddRefs(filterList));
    if (NS_SUCCEEDED(rv))
        rv = getFilterListTargets(filterList, aSource, aProperty,
                                  aTruthValue, resourceList);
    else {
        // maybe it's just a filter
        nsCOMPtr<nsIMsgFilter> filter;
        rv = aSource->GetDelegate("filter", NS_GET_IID(nsIMsgFilter),
                                  (void **)getter_AddRefs(filter));
        if (NS_SUCCEEDED(rv))
            rv = getFilterTargets(filter, aProperty, aTruthValue, resourceList);
    }

    nsArrayEnumerator *cursor = new nsArrayEnumerator(resourceList);
    NS_ENSURE_TRUE(cursor, NS_ERROR_OUT_OF_MEMORY);
    
    *aResult = cursor;
    NS_ADDREF(*aResult);
    
    return NS_OK;
}


NS_IMETHODIMP
nsMsgFilterDataSource::ArcLabelsOut(nsIRDFResource *aSource,
                                    nsISimpleEnumerator **aResult)
{
    nsresult rv;
    nsCOMPtr<nsISupportsArray> arcs;

    nsCOMPtr<nsIMsgFilterList> filterList;
    rv = aSource->GetDelegate("filter", NS_GET_IID(nsIMsgFilterList),
                              (void **)getter_AddRefs(filterList));
    if (NS_SUCCEEDED(rv)) {
        arcs = mFilterListArcsOut;
    
    } else {

        nsCOMPtr<nsIMsgFilter> filter;
        rv = aSource->GetDelegate("filter", NS_GET_IID(nsIMsgFilter),
                                 (void **)getter_AddRefs(filter));
    
        if (NS_SUCCEEDED(rv))
            arcs = mFilterArcsOut;
    }

    if (!arcs) {
        *aResult = nsnull;
        return NS_RDF_NO_VALUE;
    }

    nsArrayEnumerator* enumerator =
        new nsArrayEnumerator(arcs);
    NS_ENSURE_TRUE(enumerator, NS_ERROR_OUT_OF_MEMORY);
  
    *aResult = enumerator;
    NS_ADDREF(*aResult);

    return NS_OK;
}

// takes a base resource, like mailbox://username@host/folder and returns
// the array that corresponds to all the filters.
// it does this quickly because it knows the sub-filters will be in the form
//
// mailbox://username@host/folder#filter0
// mailbox://username@host/folder#filter1
// mailbox://username@host/folder#filter2
// mailbox://username@host/folder#filter3
//
// and so forth
nsresult
nsMsgFilterDataSource::getFilterListTargets(nsIMsgFilterList *aFilterList,
                                            nsIRDFResource *aSource,
                                            nsIRDFResource *aProperty,
                                            PRBool aTruthValue,
                                            nsISupportsArray *aResult)
{
    nsresult rv;

    // get the URI of the source resource, say mailbox://username@host/folder
    // from there we'll append "#filter", and then enumerate the
    // filters to get all the resources like
    // mailbox://username@host/folder#filter4
    nsXPIDLCString filterListUri;
    aSource->GetValue(getter_Copies(filterListUri));

    nsCAutoString filterUri((const char *)filterListUri);
    filterUri.Append("#filter");

    // we'll use the length of this base string to truncate the string later
    PRInt32 baseFilterUriLength = filterUri.Length();

    // now start looping through the filters
    // we're not actually touching any filters here, just creating
    // resources that correspond to them.
    PRUint32 filterCount;
    rv = aFilterList->GetFilterCount(&filterCount);
    if (NS_FAILED(rv)) return rv;

    PRUint32 i;
    for (i=0; i<filterCount; i++) {
        filterUri.Append(PRInt32(i));
        
        nsCOMPtr<nsIRDFResource> filterResource;
        rv = getRDFService()->GetResource(filterUri.GetBuffer(),
                              getter_AddRefs(filterResource));
        if (NS_SUCCEEDED(rv))
            aResult->AppendElement(filterResource);

        // now reduce the URI back to the base uri
        filterUri.Truncate(baseFilterUriLength);
    }
    
    return NS_OK;
}
