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
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#include "nsMessengerBootstrap.h"
#include "nsCOMPtr.h"

#include "nsDOMCID.h"
#include "nsMsgBaseCID.h"
#include "nsIMsgMailSession.h"

static NS_DEFINE_CID(kCMsgMailSessionCID, NS_MSGMAILSESSION_CID); 


NS_IMPL_ISUPPORTS(nsMessengerBootstrap, nsCOMTypeInfo<nsIAppShellComponent>::GetIID())

nsMessengerBootstrap::nsMessengerBootstrap()
{
  NS_INIT_REFCNT();
  
}

nsMessengerBootstrap::~nsMessengerBootstrap()
{
}

nsresult
nsMessengerBootstrap::Initialize(nsIAppShellService*,
                                 nsICmdLineService*)
{
	nsresult rv;
    rv = nsServiceManager::RegisterService( "component://netscape/appshell/component/messenger", this );

	return rv;
}

nsresult
nsMessengerBootstrap::Shutdown()
{
	nsresult finalrv = NS_OK;
	nsresult rv;

    NS_WITH_SERVICE(nsIMsgMailSession, mailSession, kCMsgMailSessionCID, &rv);
    if (NS_SUCCEEDED(rv))
	{
		mailSession->Shutdown();
	}
	rv = nsServiceManager::UnregisterService("component://netscape/appshell/component/messenger");
	if(NS_FAILED(rv)) finalrv = rv;

	rv = nsServiceManager::UnregisterService("component://netscape/messenger/services/session");
	if(NS_FAILED(rv)) finalrv = rv;

	rv = nsServiceManager::UnregisterService("component://netscape/messenger/biffManager");
	if(NS_FAILED(rv)) finalrv = rv;

	return finalrv;
}


