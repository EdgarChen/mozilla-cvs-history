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

#include "pratom.h"
#include "nsISupports.h"
#include "nsRepository.h"
#include "nsIFactory.h"
#include "nsLWBrkCIID.h"
#include "nsILineBreakerFactory.h"
#include "nsIWordBreakerFactory.h"
#include "nsLWBreakerFImp.h"


NS_DEFINE_CID(kLWBrkCID, NS_LWBRK_CID);

NS_DEFINE_IID(kFactoryIID, NS_IFACTORY_IID);

extern "C" PRInt32 g_InstanceCount = 0;
extern "C" PRInt32 g_LockCount = 0;

class nsLWBrkFactory : public nsIFactory {
  NS_DECL_ISUPPORTS
  nsLWBrkFactory() {
    NS_INIT_REFCNT();
    PR_AtomicIncrement(&g_InstanceCount);
  };
  virtual ~nsLWBrkFactory() {
    PR_AtomicDecrement(&g_InstanceCount);
  };

  NS_IMETHOD CreateInstance(nsISupports *aDelegate,
                            const nsIID &aIID,
                            void **aResult);

  NS_IMETHOD LockFactory(PRBool aLock) {
    if (aLock) {
      PR_AtomicIncrement(&g_LockCount);
    } else {
      PR_AtomicDecrement(&g_LockCount);
    }
    return NS_OK;
  };
};

NS_IMPL_ISUPPORTS(nsLWBrkFactory, kFactoryIID);

nsresult nsLWBrkFactory::CreateInstance(nsISupports *aDelegate,
                                            const nsIID &aIID,
                                            void **aResult)
{
  if(NULL == aResult ) {
    return NS_ERROR_NULL_POINTER;
  }
  
  *aResult = NULL;
   
  nsLWBreakerFImp *imp = new nsLWBreakerFImp() ;
  
  nsresult res = imp->QueryInterface(aIID, aResult);
  
  if(NS_FAILED(res)) {
    delete imp;
  }
  return res;
}

extern "C" NS_EXPORT nsresult NSGetFactory(nsISupports* serviceMgr,
                                           const nsCID &aClass,
                                           const char *aClassName,
                                           const char *aProgID,
                                           nsIFactory **aFactory)
{
  if (aFactory == NULL) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aClass.Equals(kLWBrkCID)) {
    nsLWBrkFactory *factory = new nsLWBrkFactory();
    nsresult res = factory->QueryInterface(kFactoryIID, (void **) aFactory);
    if (NS_FAILED(res)) {
      *aFactory = NULL;
      delete factory;
    }
    return res;
  }
  return NS_NOINTERFACE;
}

extern "C" NS_EXPORT PRBool NSCanUnload(nsISupports* serviceMgr) {
  return PRBool(g_InstanceCount == 0 && g_LockCount == 0);
}

extern "C" NS_EXPORT nsresult NSRegisterSelf(nsISupports* serviceMgr, const char *path)
{
  return nsRepository::RegisterComponent(kLWBrkCID, NULL, NULL, path,
                                       PR_TRUE, PR_TRUE);
}

extern "C" NS_EXPORT nsresult NSUnregisterSelf(nsISupports* serviceMgr, const char *path)
{
  return nsRepository::UnregisterFactory(kLWBrkCID, path);
}
