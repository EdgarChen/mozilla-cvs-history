/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
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

#include <iostream.h>
#include "TestFactory.h"
#include "nsISupports.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"

NS_DEFINE_IID(kFactoryIID, NS_IFACTORY_IID);
NS_DEFINE_CID(kTestFactoryCID, NS_TESTFACTORY_CID);
NS_DEFINE_CID(kTestLoadedFactoryCID, NS_TESTLOADEDFACTORY_CID);
NS_DEFINE_IID(kTestClassIID, NS_ITESTCLASS_IID);

int main(int argc, char **argv) {
  nsresult rv;
  nsIServiceManager* servMgr;

  rv = NS_InitXPCOM(&servMgr);
  if (NS_FAILED(rv)) return rv;

  // XXX why do I have to do this?!
  rv = nsComponentManager::AutoRegister(nsIComponentManager::NS_Startup,
                                        "components");
  if (NS_FAILED(rv)) return rv;

  RegisterTestFactories();

  ITestClass *t = NULL;
  nsComponentManager::CreateInstance(kTestFactoryCID,
                               NULL,
                               kTestClassIID,
                               (void **) &t);

  if (t != NULL) {
    t->Test();
    t->Release();
  } else {
    cout << "CreateInstance failed\n";
  }

  t = NULL;

  nsComponentManager::CreateInstance(kTestLoadedFactoryCID,
                               NULL,
                               kTestClassIID,
                               (void **) &t);

  if (t != NULL) {
    t->Test();
    t->Release();
  } else {
    cout << "Dynamic CreateInstance failed\n";
  }

  nsComponentManager::FreeLibraries();

  return 0;
}

/**
 * ITestClass implementation
 */

class TestClassImpl: public ITestClass {
  NS_DECL_ISUPPORTS
public:
  TestClassImpl() {
    NS_INIT_REFCNT();
  }

  void Test();
};

NS_IMPL_ISUPPORTS(TestClassImpl, kTestClassIID);

void TestClassImpl::Test() {
  cout << "hello, world!\n";
}

/**
 * TestFactory implementation
 */

class TestFactory: public nsIFactory {
  NS_DECL_ISUPPORTS
  
public:
  TestFactory() {
    NS_INIT_REFCNT();
  }

  NS_IMETHOD CreateInstance(nsISupports *aDelegate,
                            const nsIID &aIID,
                            void **aResult);

  NS_IMETHOD LockFactory(PRBool aLock) { return NS_OK; }
};

NS_IMPL_ISUPPORTS(TestFactory, kFactoryIID);

nsresult TestFactory::CreateInstance(nsISupports *aDelegate,
                                     const nsIID &aIID,
                                     void **aResult) {
  if (aDelegate != NULL) {
    return NS_ERROR_NO_AGGREGATION;
  }

  TestClassImpl *t = new TestClassImpl();
  
  if (t == NULL) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  nsresult res = t->QueryInterface(aIID, aResult);

  if (NS_FAILED(res)) {
    *aResult = NULL;
    delete t;
  }

  return res;
}

/**
 * TestFactory registration function
 */

extern "C" void RegisterTestFactories() {
  nsComponentManager::RegisterFactory(kTestFactoryCID, 0, 0,
                                new TestFactory(), PR_FALSE);

  // Windows can use persistant registry  
#ifndef USE_NSREG
  nsComponentManager::RegisterComponent(kTestLoadedFactoryCID, NULL, NULL,
                                "libtestdynamic.so",
                                PR_FALSE,
                                PR_TRUE);
#endif
}
