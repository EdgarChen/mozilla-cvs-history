/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#include "nsLoadGroup.h"
#include "nsIIOService.h"
#include "nsIChannel.h"
#include "nsIServiceManager.h"
#include "nsISupportsArray.h"
#include "nsEnumeratorUtils.h"

static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);
static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);

////////////////////////////////////////////////////////////////////////////////

nsLoadGroup::nsLoadGroup(nsISupports* outer)
    : mChannels(nsnull), mSubGroups(nsnull), 
      mDefaultLoadAttributes(nsIChannel::LOAD_NORMAL)
{
    NS_INIT_AGGREGATED(outer);
}

nsLoadGroup::~nsLoadGroup()
{
    nsresult rv = Cancel();
    NS_ASSERTION(NS_SUCCEEDED(rv), "Cancel failed");
    NS_IF_RELEASE(mChannels);
    NS_IF_RELEASE(mSubGroups);
}
    
NS_METHOD
nsLoadGroup::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    nsLoadGroup* group = new nsLoadGroup(aOuter);
    if (group == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(group);
    nsresult rv = group->QueryInterface(aIID, aResult);
    NS_RELEASE(group);
    return rv;
}

NS_IMPL_AGGREGATED(nsLoadGroup);
    
NS_IMETHODIMP
nsLoadGroup::AggregatedQueryInterface(const nsIID& aIID, void** aInstancePtr)
{
    NS_ASSERTION(aInstancePtr, "no instance pointer");
    if (aIID.Equals(nsCOMTypeInfo<nsILoadGroup>::GetIID()) ||
        aIID.Equals(nsCOMTypeInfo<nsIRequest>::GetIID()) ||
        aIID.Equals(nsCOMTypeInfo<nsISupports>::GetIID())) {
        *aInstancePtr = NS_STATIC_CAST(nsILoadGroup*, this);
        NS_ADDREF_THIS();
        return NS_OK;
    }
    return NS_NOINTERFACE; 
}

////////////////////////////////////////////////////////////////////////////////
// nsIRequest methods:

nsresult
nsLoadGroup::PropagateDown(PropagateDownFun fun)
{
    nsresult rv;
    PRUint32 i;
    PRUint32 count;
    rv = mChannels->Count(&count);
    if (NS_FAILED(rv)) return rv;
    for (i = 0; i < count; i++) {
        nsIRequest* req = NS_STATIC_CAST(nsIRequest*, mChannels->ElementAt(i));
        if (req == nsnull)
            return NS_ERROR_FAILURE;
        rv = fun(req);
        NS_RELEASE(req);
        if (NS_FAILED(rv)) return rv;
    }

    rv = mSubGroups->Count(&count);
    if (NS_FAILED(rv)) return rv;
    for (i = 0; i < count; i++) {
        nsIRequest* req = NS_STATIC_CAST(nsIRequest*, mSubGroups->ElementAt(i));
        if (req == nsnull)
            return NS_ERROR_FAILURE;
        rv = fun(req);
        NS_RELEASE(req);
        if (NS_FAILED(rv)) return rv;
    }
    return NS_OK;
}

NS_IMETHODIMP
nsLoadGroup::IsPending(PRBool *result)
{
    nsresult rv;
    PRUint32 count;
    rv = mChannels->Count(&count);
    if (count > 0) {
        *result = PR_FALSE;
        return NS_OK;
    }

    PRUint32 i;
    rv = mSubGroups->Count(&count);
    if (NS_FAILED(rv)) return rv;
    for (i = 0; i < count; i++) {
        nsIRequest* req = NS_STATIC_CAST(nsIRequest*, mSubGroups->ElementAt(i));
        if (req == nsnull)
            return NS_ERROR_FAILURE;
        PRBool pending;
        rv = req->IsPending(&pending);
        NS_RELEASE(req);
        if (NS_FAILED(rv)) return rv;
        if (pending) {
            *result = PR_FALSE;
            return NS_OK;
        }
    }

    *result = PR_TRUE;
    return NS_OK;
}

static nsresult
CancelFun(nsIRequest* req)
{
    return req->Cancel();
}

NS_IMETHODIMP
nsLoadGroup::Cancel()
{
    return PropagateDown(CancelFun);
}

static nsresult
SuspendFun(nsIRequest* req)
{
    return req->Suspend();
}

NS_IMETHODIMP
nsLoadGroup::Suspend()
{
    return PropagateDown(SuspendFun);
}

static nsresult
ResumeFun(nsIRequest* req)
{
    return req->Resume();
}

NS_IMETHODIMP
nsLoadGroup::Resume()
{
    return PropagateDown(ResumeFun);
}

////////////////////////////////////////////////////////////////////////////////
// nsILoadGroup methods:

NS_IMETHODIMP
nsLoadGroup::Init(nsILoadGroup *parent)
{
//    mParent = parent;   // weak ref
    if (!parent)
        return NS_ERROR_NULL_POINTER;
    parent->AddSubGroup(this);  // XXX
    return NS_OK;
}
    
NS_IMETHODIMP
nsLoadGroup::GetDefaultLoadAttributes(PRUint32 *aDefaultLoadAttributes)
{
    *aDefaultLoadAttributes = mDefaultLoadAttributes;
    return NS_OK;
}

NS_IMETHODIMP
nsLoadGroup::SetDefaultLoadAttributes(PRUint32 aDefaultLoadAttributes)
{
    mDefaultLoadAttributes = aDefaultLoadAttributes;
    return NS_OK;
}

NS_IMETHODIMP
nsLoadGroup::AddChannel(nsIChannel *channel)
{
    nsresult rv = mChannels->AppendElement(channel);
    if (NS_FAILED(rv)) return rv;

    rv = channel->SetLoadAttributes(mDefaultLoadAttributes);
    return NS_OK;
}

NS_IMETHODIMP
nsLoadGroup::RemoveChannel(nsIChannel *channel)
{
    return mChannels->RemoveElement(channel);
}

NS_IMETHODIMP
nsLoadGroup::GetChannels(nsISimpleEnumerator * *aChannels)
{
    return NS_NewArrayEnumerator(aChannels, mChannels);
}

NS_IMETHODIMP
nsLoadGroup::AddObserver(nsIStreamObserver *observer)
{
    return mObservers->AppendElement(observer);
}

NS_IMETHODIMP
nsLoadGroup::RemoveObserver(nsIStreamObserver *observer)
{
    return mObservers->RemoveElement(observer);
}

NS_IMETHODIMP
nsLoadGroup::GetObservers(nsISimpleEnumerator * *aObservers)
{
    return NS_NewArrayEnumerator(aObservers, mObservers);
}

NS_IMETHODIMP
nsLoadGroup::AddSubGroup(nsILoadGroup *group)
{
//    group->mParent = this;    // XXX
    return mSubGroups->AppendElement(group);
}

NS_IMETHODIMP
nsLoadGroup::RemoveSubGroup(nsILoadGroup *group)
{
//    group->mParent = nsnull;  // XXX
    return mSubGroups->RemoveElement(group);
}

NS_IMETHODIMP
nsLoadGroup::GetSubGroups(nsISimpleEnumerator * *aSubGroups)
{
    return NS_NewArrayEnumerator(aSubGroups, mSubGroups);
}

////////////////////////////////////////////////////////////////////////////////
// nsIStreamObserver methods:

nsresult
nsLoadGroup::PropagateUp(PropagateUpFun fun, void* closure)
{
    nsresult rv;
    PRUint32 i;
    PRUint32 count;

    for (nsLoadGroup* group = this; group != nsnull; group = group->mParent) {
        rv = group->mObservers->Count(&count);
        if (NS_FAILED(rv)) return rv;
        for (i = 0; i < count; i++) {
            nsIStreamObserver* obs =
                NS_STATIC_CAST(nsIStreamObserver*, group->mObservers->ElementAt(i));
            if (obs == nsnull)
                return NS_ERROR_FAILURE;
            rv = fun(obs, closure);
            NS_RELEASE(obs);
            if (NS_FAILED(rv)) return rv;
        }
    }
    return NS_OK;
}

static nsresult
OnStartRequestFun(nsIStreamObserver* obs, void* closure)
{
    nsISupports* ctxt = (nsISupports*)closure;
    return obs->OnStartRequest(ctxt);
}

NS_IMETHODIMP
nsLoadGroup::OnStartRequest(nsISupports *ctxt)
{
    return PropagateUp(OnStartRequestFun, ctxt);
}

struct OnStopRequestArgs {
    nsISupports* ctxt;
    nsresult status;
    const PRUnichar* errorMsg;
};

static nsresult
OnStopRequestFun(nsIStreamObserver* obs, void* closure)
{
    OnStopRequestArgs* args = (OnStopRequestArgs*)closure;
    return obs->OnStopRequest(args->ctxt, args->status, args->errorMsg);
}

NS_IMETHODIMP
nsLoadGroup::OnStopRequest(nsISupports *ctxt, nsresult status, 
                           const PRUnichar *errorMsg)
{
    OnStopRequestArgs args;
    args.ctxt = ctxt;
    args.status = status;
    args.errorMsg = errorMsg;
    return PropagateUp(OnStartRequestFun, &args);
}

////////////////////////////////////////////////////////////////////////////////
// nsIStreamListener methods:

struct OnDataAvailableArgs {
    nsISupports* ctxt;
    nsIInputStream *inStr;
    PRUint32 sourceOffset;
    PRUint32 count;
};

static nsresult
OnDataAvailableFun(nsIStreamObserver* obs, void* closure)
{
    OnDataAvailableArgs* args = (OnDataAvailableArgs*)closure;
    nsIStreamListener* l = NS_STATIC_CAST(nsIStreamListener*, obs);
    return l->OnDataAvailable(args->ctxt, args->inStr,
                              args->sourceOffset, args->count);
}

NS_IMETHODIMP
nsLoadGroup::OnDataAvailable(nsISupports *ctxt, 
                             nsIInputStream *inStr, 
                             PRUint32 sourceOffset, 
                             PRUint32 count)
{
    OnDataAvailableArgs args;
    args.ctxt = ctxt;
    args.inStr = inStr;
    args.sourceOffset = sourceOffset;
    args.count = count;
    return PropagateUp(OnDataAvailableFun, &args);
}

////////////////////////////////////////////////////////////////////////////////
