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
 * Alec Flett <alecf@netscape.com>
 */

#include "nsMsgFilterDelegateFactory.h"

#include "nsCOMPtr.h"
#include "nsXPIDLString.h"

#include "nsIRDFService.h"
#include "nsIRDFResource.h"
#include "nsRDFCID.h"

#include "nsIMsgFolder.h"
#include "nsIMsgFilter.h"
#include "nsIMsgFilterList.h"

static NS_DEFINE_CID(kRDFServiceCID, NS_RDFSERVICE_CID);

NS_IMPL_ISUPPORTS1(nsMsgFilterDelegateFactory, nsIRDFDelegateFactory)

nsMsgFilterDelegateFactory::nsMsgFilterDelegateFactory()
{
  NS_INIT_ISUPPORTS();
  /* member initializers and constructor code */
}

nsMsgFilterDelegateFactory::~nsMsgFilterDelegateFactory()
{
  /* destructor code */
}

/* void CreateDelegate (in nsIRDFResource aOuter, in string aKey, in nsIIDRef aIID, [iid_is (aIID), retval] out nsQIResult aResult); */
NS_IMETHODIMP nsMsgFilterDelegateFactory::CreateDelegate(nsIRDFResource *aOuter, const char *aKey, const nsIID & aIID, void * *aResult)
{
    // if it's a folder, then we return a filter list..
    // otherwise make sure it's in the form
    // mailbox://userid@server/foldername#filter4

#ifdef DEBUG_alecf
    nsXPIDLCString outerValue;
    aOuter->GetValue(getter_Copies(outerValue));
    printf("nsMsgFilterDelegateFactory::CreateDelegate(%s, %s, ..)\n",
           outerValue, aKey);
#endif
    
    *aResult = nsnull;
    
    nsresult rv;
    if (aIID.Equals(NS_GET_IID(nsIMsgFilterList))) {
        nsIMsgFilterList *filterList;
        rv = getFilterListDelegate(aOuter, &filterList);
#ifdef DEBUG_alecf
        if (NS_SUCCEEDED(rv))
            printf("  creating nsIMsgFilterList delegate\n");
#endif
        if (NS_SUCCEEDED(rv))
            *aResult = (void *)filterList;

    } else if (aIID.Equals(NS_GET_IID(nsIMsgFilter))) {
        nsIMsgFilter *filter;
        rv = getFilterDelegate(aOuter, &filter);
#ifdef DEBUG_alecf
        if (NS_SUCCEEDED(rv))
            printf("  creating nsIMsgFilter delegate\n");
#endif
        if (NS_SUCCEEDED(rv))
            *aResult = (void *)filter;
    }

    if (!*aResult)
        return NS_ERROR_FAILURE;
    
    return NS_OK;
}

nsresult
nsMsgFilterDelegateFactory::getFilterListDelegate(nsIRDFResource *aOuter,
                                                  nsIMsgFilterList **aResult)
{
    nsresult rv;
    
    nsCOMPtr<nsIMsgFolder> folder = do_QueryInterface(aOuter, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsCOMPtr<nsIMsgFilterList> filterList;
    folder->GetFilterList(getter_AddRefs(filterList));
    
    *aResult = filterList;
    NS_ADDREF(*aResult);
    return NS_OK;
}

nsresult
nsMsgFilterDelegateFactory::getFilterDelegate(nsIRDFResource *aOuter,
                                              nsIMsgFilter **aResult)
{
    nsresult rv;
    // now try to find "#filter"
    nsXPIDLCString uri;
    rv = aOuter->GetValue(getter_Copies(uri));
    if (NS_FAILED(rv)) return rv;
    
    PRInt32 seperatorPosition = 0;
    const char *filterTag = uri;
    while (filterTag && *filterTag != '#') {
        seperatorPosition++;
        filterTag++;
    }
    
    // if no #, and it's not a folder,
    // I don't know what the heck it is.
    if (!filterTag)
        return NS_ERROR_FAILURE;

#ifdef DEBUG_alecf
    printf("nsMsgFilterDelegateFactory: found #: %s\n", filterTag);
#endif

    PRInt32 filterNumber = getFilterNumber(filterTag);

    nsCOMPtr<nsIMsgFilterList> filterList;
    rv = getFilterList(uri, seperatorPosition, getter_AddRefs(filterList));

        // now that we have the filter list and index, retrieve the filter.

#ifdef DEBUG_alecf
    printf("Retrieving filter #%d\n" , filterNumber);
#endif
        
    nsCOMPtr<nsIMsgFilter> filter;
    rv = filterList->GetFilterAt(filterNumber, getter_AddRefs(filter));
    if (NS_FAILED(rv)) return rv;

    *aResult = filter;
    NS_ADDREF(*aResult);
    
    return NS_OK;
}
    

PRInt32
nsMsgFilterDelegateFactory::getFilterNumber(const char *filterTag)
{
    
    if (nsCRT::strncmp(filterTag, MSGFILTER_TAG, MSGFILTER_TAG_LENGTH) != 0)
        return -1;
    
    const char *filterNumberStr = filterTag + MSGFILTER_TAG_LENGTH;
    
    return atoi(filterNumberStr);
}

nsresult
nsMsgFilterDelegateFactory::getFilterList(const char *aUri,
                                          PRInt32 aTagPosition,
                                          nsIMsgFilterList** aResult)
{
    nsresult rv;
    // now we actually need the filter list, so we truncate the string
    // and use getdelegate to get the filter delegate
    nsCAutoString folderUri((const char*)aUri);
    folderUri.Truncate(aTagPosition);
    
    // convert URI to resource
    nsCOMPtr<nsIRDFService> rdf(do_GetService(kRDFServiceCID, &rv));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIRDFResource> filterListResource;
    rdf->GetResource(folderUri.GetBuffer(), getter_AddRefs(filterListResource));
    NS_ENSURE_SUCCESS(rv, rv);

    return filterListResource->GetDelegate("filter",
                                           NS_GET_IID(nsIMsgFilterList),
                                           (void **)aResult);
}

