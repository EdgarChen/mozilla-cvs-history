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

#include "nsNeckoUtil.h"
#include "nsIIOService.h"
#include "nsIServiceManager.h"
#include "nsIChannel.h"
#include "nsIAllocator.h"

static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);

PR_PUBLIC_API(nsresult)
NS_NewURI(nsIURI* *result, const char* spec, nsIURI* baseURI)
{
    nsresult rv;
    NS_WITH_SERVICE(nsIIOService, serv, kIOServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;
    
    rv = serv->NewURI(spec, baseURI, result);
    return rv;
}

PR_PUBLIC_API(nsresult)
NS_NewURI(nsIURI* *result, const nsString& spec, nsIURI* baseURI)
{
    // XXX if the string is unicode, GetBuffer() returns null. 
    // XXX we need a strategy to deal w/ unicode specs (if there should
    // XXX even be such a thing)
    char* specStr = spec.ToNewCString(); // this forces a single byte char*
    if (specStr == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    nsresult rv = NS_NewURI(result, specStr, baseURI);
    nsAllocator::Free(specStr);
    return rv;
}

PR_PUBLIC_API(nsresult)
NS_OpenURI(nsIChannel* *result, nsIURI* uri, nsILoadGroup* group)
{
    nsresult rv;
    NS_WITH_SERVICE(nsIIOService, serv, kIOServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    nsIChannel* channel;
    rv = serv->NewChannelFromURI("load", uri, nsnull, &channel);
    if (NS_FAILED(rv)) return rv;
#if 0
    if (group) {
        rv = group->AddChannel(channel);
        if (NS_FAILED(rv)) {
            NS_RELEASE(channel);
            return rv;
        }
    }
#endif
    *result = channel;
    return rv;
}

PR_PUBLIC_API(nsresult)
NS_OpenURI(nsIInputStream* *result, nsIURI* uri, nsILoadGroup* group)
{
    nsresult rv;
    nsIChannel* channel;

    rv = NS_OpenURI(&channel, uri, group);
    if (NS_FAILED(rv)) return rv;

    nsIInputStream* inStr;
    rv = channel->OpenInputStream(0, -1, &inStr);
    NS_RELEASE(channel);
    if (NS_FAILED(rv)) return rv;

    *result = inStr;
    return rv;
}

PR_PUBLIC_API(nsresult)
NS_OpenURI(nsIStreamListener* aConsumer, nsISupports* context, 
           nsIURI* uri, nsILoadGroup* group)
{
    nsresult rv;
    nsIChannel* channel;

    rv = NS_OpenURI(&channel, uri, group);
    if (NS_FAILED(rv)) return rv;

    rv = channel->AsyncRead(0, -1, context, aConsumer);
    NS_RELEASE(channel);
    return rv;
}

PR_PUBLIC_API(nsresult)
NS_MakeAbsoluteURI(const char* spec, nsIURI* baseURI, char* *result)
{
    nsresult rv;
    NS_WITH_SERVICE(nsIIOService, serv, kIOServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;
    
    return serv->MakeAbsolute(spec, baseURI, result);
}

PR_PUBLIC_API(nsresult)
NS_MakeAbsoluteURI(const nsString& spec, nsIURI* baseURI, nsString& result)
{
    char* resultStr;
    char* specStr = spec.ToNewCString();
    if (!specStr) {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    nsresult rv = NS_MakeAbsoluteURI(specStr, baseURI, &resultStr);
    delete [] specStr;
    if (NS_FAILED(rv)) return rv;

    result = resultStr;
    return rv;
}

PR_PUBLIC_API(nsresult)
NS_NewLoadGroup(nsILoadGroup* parent, nsISupports* outer, nsILoadGroup* *result)
{
    nsresult rv;
    NS_WITH_SERVICE(nsIIOService, serv, kIOServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;
    
    return serv->NewLoadGroup(parent, outer, result);
}

////////////////////////////////////////////////////////////////////////////////
