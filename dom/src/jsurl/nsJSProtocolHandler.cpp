/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
#include "nsCOMPtr.h"
#include "jsapi.h"
#include "prmem.h"
#include "nsCRT.h"
#include "nsIComponentManager.h"
#include "nsIGenericFactory.h"
#include "nsIServiceManager.h"
#include "nsNetUtil.h"
#include "nsIInputStream.h"
#include "nsIStringStream.h"
#include "nsIURI.h"
#include "nsIScriptContext.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptGlobalObjectOwner.h"
#include "nsJSProtocolHandler.h"
#include "nsIPrincipal.h"
#include "nsIScriptSecurityManager.h"
#include "nsProxyObjectManager.h"
#include "nsIDocShell.h"
#include "nsDOMError.h"
#include "nsIInterfaceRequestor.h"
#include "nsIEvaluateStringProxy.h"
#include "nsXPIDLString.h"
#include "nsIByteArrayInputStream.h"

static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);
static NS_DEFINE_CID(kSimpleURICID, NS_SIMPLEURI_CID);
static NS_DEFINE_CID(kJSProtocolHandlerCID, NS_JSPROTOCOLHANDLER_CID);

class nsJSThunk;

/***************************************************************************/
/* nsEvaluateStringProxy                                                   */
/* This private class will allow us to evaluate js on another thread       */
/***************************************************************************/
class nsEvaluateStringProxy : public nsIEvaluateStringProxy {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIEVALUATESTRINGPROXY

    nsEvaluateStringProxy();
    virtual ~nsEvaluateStringProxy();
protected:
    nsCOMPtr<nsIChannel>        mChannel;
};


nsEvaluateStringProxy::nsEvaluateStringProxy()
{
    NS_INIT_REFCNT();
}

nsEvaluateStringProxy::~nsEvaluateStringProxy()
{
}

NS_IMPL_THREADSAFE_ISUPPORTS1(nsEvaluateStringProxy, nsIEvaluateStringProxy)

NS_IMETHODIMP
nsEvaluateStringProxy::Init(nsIChannel* channel)
{
    mChannel = channel;
    return NS_OK;
}

NS_IMETHODIMP
nsEvaluateStringProxy::EvaluateString(char **aRetValue, PRBool *aIsUndefined)
{
    NS_ENSURE_ARG_POINTER(mChannel);
    NS_ENSURE_ARG_POINTER(aRetValue);
    NS_ENSURE_ARG_POINTER(aIsUndefined);

    nsresult rv;
    
    nsCOMPtr<nsIInterfaceRequestor> callbacks;
    rv = mChannel->GetNotificationCallbacks(getter_AddRefs(callbacks));
    if (NS_FAILED(rv)) return rv;
    NS_ENSURE_TRUE(callbacks, NS_ERROR_FAILURE);

    // The event sink must be a script global Object Owner or we fail.
    nsCOMPtr<nsIScriptGlobalObjectOwner> globalOwner;
    callbacks->GetInterface(NS_GET_IID(nsIScriptGlobalObjectOwner),
                            getter_AddRefs(globalOwner));
    NS_ENSURE_TRUE(globalOwner, NS_ERROR_FAILURE);

    // So far so good: get the script context from its owner.
    nsCOMPtr<nsIScriptGlobalObject> global;
    globalOwner->GetScriptGlobalObject(getter_AddRefs(global));
    NS_ENSURE_TRUE(global, NS_ERROR_FAILURE);

    nsCOMPtr<nsIScriptContext> scriptContext;
    rv = global->GetContext(getter_AddRefs(scriptContext));
    if (NS_FAILED(rv)) return rv;

    // Get principal of code for execution
    NS_WITH_SERVICE(nsIScriptSecurityManager, securityManager,
                    NS_SCRIPTSECURITYMANAGER_PROGID, &rv);
    if (NS_FAILED(rv)) return rv;

    // Get principal
    nsCOMPtr<nsIPrincipal> principal;
    nsCOMPtr<nsIURI> referringUri;
    // XXX this is wrong: see bugs 31818 and 29831. Norris is looking at it.
    rv = mChannel->GetOriginalURI(getter_AddRefs(referringUri));
    if (NS_FAILED(rv)) {
        // No referrer available. Use the current javascript: URI, which will mean 
        // that this script will be in another trust domain than any other script
        // since SameOrigin should be false for anything other than the same
        // javascript: URI.
#if 0
        nsCOMPtr<nsIDocShell> docShell;
        docShell = do_QueryInterface(globalOwner, &rv);
        if (NS_FAILED(rv)) return rv;
        rv = docShell->GetCurrentURI(getter_AddRefs(referringUri));
#else
        rv = mChannel->GetURI(getter_AddRefs(referringUri));
#endif
        if (NS_FAILED(rv)) return rv;
    }
    rv = securityManager->GetCodebasePrincipal(referringUri, 
                                               getter_AddRefs(principal));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIURI> jsURI;
    rv = mChannel->GetURI(getter_AddRefs(jsURI));
    if (NS_FAILED(rv)) return rv;

    nsXPIDLCString script;
    rv = jsURI->GetPath(getter_Copies(script));
    if (NS_FAILED(rv)) return rv;

    // Finally, we have everything needed to evaluate the expression.

    nsString result;
    {
      nsAutoString scriptString;
      scriptString.AssignWithConversion(script);
      rv = scriptContext->EvaluateString(scriptString,
                                         nsnull,  // obj
                                         principal,
                                         nsnull,  // url
                                         0,       // line no
                                         nsnull,
                                         result,
                                         aIsUndefined);
    }
    // XXXbe this should not decimate! pass back UCS-2 to necko
    *aRetValue = result.ToNewCString();
    return rv;
}

////////////////////////////////////////////////////////////////////////////////

class nsJSThunk : public nsIStreamIO
{
public:
    NS_DECL_ISUPPORTS

    nsJSThunk()
        : mChannel(nsnull), mResult(nsnull), mLength(0), mReadCursor(0) {
        NS_INIT_REFCNT();
    }

    virtual ~nsJSThunk() {
        (void)Close(NS_BASE_STREAM_CLOSED);
    }

    nsresult Init(nsIURI* uri, nsIChannel* channel) {
        NS_ENSURE_ARG_POINTER(uri);
        NS_ENSURE_ARG_POINTER(channel);
        mURI = uri;
        mChannel = channel;
        return NS_OK;
    }

    NS_IMETHOD Open(char* *contentType, PRInt32 *contentLength) {
        // IMPORTANT CHANGE: We used to just implement nsIInputStream and use an
        // input stream channel in the js protocol, but that had the nasty side effect
        // of doing the evaluation after the window content was already torn down (in
        // the webshell's OnStartRequest callback). By using an nsIStreamIO instead, 
        // we now get more control over when the evaluation occurs. By evaluating in the
        // Open method, we can can get the status of the channel in the OnStartRequest
        // callback and detect NS_ERROR_DOM_RETVAL_UNDEFINED, thereby avoiding tearing
        // down the current document.

        nsresult rv;

        // We do this by proxying back to the main thread.
        NS_WITH_SERVICE(nsIProxyObjectManager, proxyObjectManager,
                        nsIProxyObjectManager::GetCID(), &rv);
        if (NS_FAILED(rv)) return rv;

        nsEvaluateStringProxy* eval = new nsEvaluateStringProxy();
        if (eval == nsnull)
            return NS_ERROR_OUT_OF_MEMORY;
        NS_ADDREF(eval);
        rv = eval->Init(mChannel);
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsIEvaluateStringProxy> evalProxy;
        rv = proxyObjectManager->GetProxyObject(NS_UI_THREAD_EVENTQ,
                                                NS_GET_IID(nsIEvaluateStringProxy),
                                                NS_STATIC_CAST(nsISupports*, eval),
                                                PROXY_SYNC | PROXY_ALWAYS,
                                                getter_AddRefs(evalProxy));

        if (NS_FAILED(rv)) {        
            NS_RELEASE(eval);
            return rv;
        }

        char* retString;
        PRBool isUndefined;
        rv = evalProxy->EvaluateString(&retString, &isUndefined);

        NS_RELEASE(eval);

        if (NS_FAILED(rv)) {
            rv = NS_ERROR_MALFORMED_URI;
            return rv;
        }

        if (isUndefined) {
            if (retString) 
                Recycle(retString);
            rv = NS_ERROR_DOM_RETVAL_UNDEFINED;
            return rv;
        }
#if 0
        else {
            // This is from the old code which need to be hack on a bit more:

            // plaintext is apparently busted
            if (ret[0] != PRUnichar('<'))
                ret.Insert("<plaintext>\n", 0);
            mLength = ret.Length();
            PRUint32 resultSize = mLength + 1;
            mResult = (char *)PR_Malloc(resultSize);
            ret.ToCString(mResult, resultSize);
        }
#endif

        mResult = retString;
        mLength = nsCRT::strlen(retString);

        *contentType = nsCRT::strdup("text/html");
        *contentLength = mLength;
        return rv;
    }

    NS_IMETHOD Close(nsresult status) {
        if (mResult) {
            nsCRT::free(mResult);
            mResult = nsnull;
        }
        mLength = 0;
        mReadCursor = 0;
        mChannel = nsnull;
        return NS_OK;
    }

    NS_IMETHOD GetInputStream(nsIInputStream* *aInputStream) {
        nsresult rv;
        nsIByteArrayInputStream* str;
        rv = NS_NewByteArrayInputStream(&str, mResult, mLength);
        *aInputStream = str;
        return rv;
    }

    NS_IMETHOD GetOutputStream(nsIOutputStream* *aOutputStream) {
        // should never be called
        NS_NOTREACHED("nsJSThunk::GetOutputStream");
        return NS_ERROR_FAILURE;
    }

    NS_IMETHOD GetName(char* *aName) {
        return mURI->GetSpec(aName);
    }

protected:
    nsCOMPtr<nsIURI>            mURI;
    nsCOMPtr<nsIChannel>        mChannel;
    char*                       mResult;
    PRUint32                    mLength;
    PRUint32                    mReadCursor;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(nsJSThunk, nsIStreamIO);

////////////////////////////////////////////////////////////////////////////////

nsJSProtocolHandler::nsJSProtocolHandler()
{
    NS_INIT_REFCNT();
}

nsresult
nsJSProtocolHandler::Init()
{
    return NS_OK;
}

nsJSProtocolHandler::~nsJSProtocolHandler()
{
}

NS_IMPL_ISUPPORTS(nsJSProtocolHandler, NS_GET_IID(nsIProtocolHandler));

NS_METHOD
nsJSProtocolHandler::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    if (aOuter)
        return NS_ERROR_NO_AGGREGATION;

    nsJSProtocolHandler* ph = new nsJSProtocolHandler();
    if (!ph)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(ph);
    nsresult rv = ph->Init();
    if (NS_SUCCEEDED(rv)) {
        rv = ph->QueryInterface(aIID, aResult);
    }
    NS_RELEASE(ph);
    return rv;
}

////////////////////////////////////////////////////////////////////////////////
// nsIProtocolHandler methods:

NS_IMETHODIMP
nsJSProtocolHandler::GetScheme(char* *result)
{
    *result = nsCRT::strdup("javascript");
    if (!*result)
        return NS_ERROR_OUT_OF_MEMORY;
    return NS_OK;
}

NS_IMETHODIMP
nsJSProtocolHandler::GetDefaultPort(PRInt32 *result)
{
    *result = -1;        // no port for javascript: URLs
    return NS_OK;
}

NS_IMETHODIMP
nsJSProtocolHandler::NewURI(const char *aSpec, nsIURI *aBaseURI,
                            nsIURI **result)
{
    nsresult rv;

    // javascript: URLs (currently) have no additional structure beyond that
    // provided by standard URLs, so there is no "outer" object given to
    // CreateInstance.

    nsIURI* url;
    if (aBaseURI) {
        rv = aBaseURI->Clone(&url);
    } else {
        rv = nsComponentManager::CreateInstance(kSimpleURICID, nsnull,
                                                NS_GET_IID(nsIURI),
                                                (void**)&url);
    }
    if (NS_FAILED(rv))
        return rv;

    rv = url->SetSpec((char*)aSpec);
    if (NS_FAILED(rv)) {
        NS_RELEASE(url);
        return rv;
    }

    *result = url;
    return rv;
}

NS_IMETHODIMP
nsJSProtocolHandler::NewChannel(nsIURI* uri, nsIChannel* *result)
{
    nsresult rv;
    NS_ENSURE_ARG_POINTER(uri);

    nsJSThunk* thunk = new nsJSThunk();
    if (thunk == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(thunk);

    nsCOMPtr<nsIStreamIOChannel> channel;
    rv = NS_NewStreamIOChannel(getter_AddRefs(channel), uri, thunk);
    if (NS_SUCCEEDED(rv)) {
        rv = thunk->Init(uri, channel);
    }
    NS_RELEASE(thunk);
    if (NS_FAILED(rv)) return rv;

    *result = channel;
    NS_ADDREF(*result);
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////

static nsModuleComponentInfo gJSModuleInfo[] = {
    { "JavaScript Protocol Handler", 
      NS_JSPROTOCOLHANDLER_CID,
      NS_NETWORK_PROTOCOL_PROGID_PREFIX "javascript",
      nsJSProtocolHandler::Create }
};

NS_IMPL_NSGETMODULE("javascript: protocol", gJSModuleInfo)

////////////////////////////////////////////////////////////////////////////////
