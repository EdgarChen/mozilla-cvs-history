/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 */

#include "prlog.h"
#include "nsCacheBkgThd.h"
#include "nsCacheManager.h"
#include "nsCacheModule.h"
#include "nsCachePref.h"

nsCacheBkgThd::nsCacheBkgThd(PRIntervalTime iSleepTime):
    nsBkgThread(iSleepTime)
{
}

nsCacheBkgThd::~nsCacheBkgThd()
{
}

/*
nsrefcnt nsCacheBkgThd::AddRef(void)
{
    return ++m_RefCnt;
}
nsrefcnt nsCacheBkgThd::Release(void)
{
    if (--m_RefCnt == 0)
    {
        delete this;
        return 0;
    }
    return m_RefCnt;
}

nsresult nsCacheBkgThd::QueryInterface(const nsIID& aIID,
                                        void** aInstancePtrResult)
{
    return NS_OK;
}
*/

void nsCacheBkgThd::Run(void)
{
    /* Step thru all the modules and call cleanup on each */
    nsCacheManager* pCM = nsCacheManager::GetInstance();
    PR_ASSERT(pCM);
    if (pCM->IsOffline())
        return; /* Dont update entries if offline */
    PRInt16 i = pCM->Entries();
    while (i>0)
    {
        nsCacheModule* pModule = pCM->GetModule(--i);
        PR_ASSERT(pModule);
        /* TODO change this based on if it supports garbage cleaning */
        if (!pModule->IsReadOnly())
        {
            pModule->GarbageCollect();
            if (nsCachePref::GetInstance()->RevalidateInBkg())
            {
                pModule->Revalidate();
            }
        }
    }
}