/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * Gagan Saksena  02/02/98
 * 
 */

#include <prtypes.h>
#include <prinrval.h>
#include "nsCacheManager.h"
#include "nsCacheTrace.h"
#include "nsCacheModule.h"

#include <assert.h> // TODO change
#include <time.h>   //TODO change to PR_

static nsCacheManager TheManager;

nsCacheManager::nsCacheManager(): m_pFirstModule(0)
{
}

nsCacheManager::~nsCacheManager()
{
#if 0
    // this is crashing!?!
	if (m_pFirstModule)
		delete m_pFirstModule;
#endif
}

nsCacheManager* nsCacheManager::GetInstance()
{
	return &TheManager;
}

const char* nsCacheManager::Trace() const
{

	char linebuffer[128];
	char* total = 0;

	sprintf(linebuffer, "nsCacheManager: Modules = %d\n", Entries());
	APPEND(linebuffer);

	return total;
}

PRInt32	nsCacheManager::AddModule(nsCacheModule* pModule)
{
	if (pModule) 
	{
		if (m_pFirstModule)
			LastModule()->Next(pModule);
		else
			m_pFirstModule = pModule;
		
		return Entries()-1;
	}
	else
		return -1;
}

PRBool nsCacheManager::Contains(const char* i_url) const
{
    if (m_pFirstModule)
    {
        nsCacheModule* pModule = m_pFirstModule;
        while (pModule)
        {
            if (pModule->Contains(i_url))
            {
                return PR_TRUE;
            }
            pModule = pModule->Next();
        }
    }
    return PR_FALSE;
}

nsCacheObject* nsCacheManager::GetObject(const char* i_url) const
{
    if (m_pFirstModule) 
    {
        nsCacheModule* pModule = m_pFirstModule;
        nsCacheObject* obj = 0;
        while (pModule)
        {
            obj = pModule->GetObject(i_url);
            if (obj)
                return obj;
            pModule = pModule->Next();
        }
    }
    return 0;
}

PRInt32 nsCacheManager::Entries() const
{
	if (m_pFirstModule) 
	{
		PRInt32 count=1;
		nsCacheModule* pModule = m_pFirstModule;
		while (pModule = pModule->Next()) 
		{
			count++;
		}
		return count;
	}
	return 0;
}

nsCacheModule* nsCacheManager::GetModule(PRInt32 i_index) const
{
	if ((i_index < 0) || (i_index >= Entries()))
		return 0;
	nsCacheModule* pModule = m_pFirstModule;
	for (PRInt32 i=0; i<=i_index; pModule = pModule->Next())
		i++;
#ifdef DEBUG
	assert(pModule);
#endif
	return pModule;
}

nsCacheModule* nsCacheManager::LastModule() const 
{
	if (m_pFirstModule) 
	{
		nsCacheModule* pModule = m_pFirstModule;
		while(pModule->Next()) {
			pModule = pModule->Next();
		}
		return pModule;
	}
	return 0;
}

PRUint32 nsCacheManager::WorstCaseTime() const 
{
    PRIntervalTime start = PR_IntervalNow();
    while (this->Contains("a vague string that should not be in any of the modules"));
    return PR_IntervalToMicroseconds(PR_IntervalNow() - start);
}