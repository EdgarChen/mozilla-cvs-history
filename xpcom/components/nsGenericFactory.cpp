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
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */


// DO NOT COPY THIS CODE INTO YOUR SOURCE!  USE NS_IMPL_NSGETMODULE()

#include "nsGenericFactory.h"
#include "nsCRT.h"
#include "nsCOMPtr.h"
#include "nsIComponentManager.h"

nsGenericFactory::nsGenericFactory(nsModuleComponentInfo *info)
    : mInfo(info)
{
    NS_INIT_ISUPPORTS();
    if (mInfo && mInfo->mClassInfoGlobal)
        *mInfo->mClassInfoGlobal = NS_STATIC_CAST(nsIClassInfo *, this);
}

nsGenericFactory::~nsGenericFactory()
{
    if (mInfo) {
        if (mInfo->mFactoryDestructor)
            mInfo->mFactoryDestructor();
        if (mInfo->mClassInfoGlobal)
            *mInfo->mClassInfoGlobal = 0;
    }
}

NS_IMPL_THREADSAFE_ISUPPORTS3(nsGenericFactory,
                              nsIGenericFactory, 
                              nsIFactory,
                              nsIClassInfo)

NS_IMETHODIMP nsGenericFactory::CreateInstance(nsISupports *aOuter,
                                               REFNSIID aIID, void **aResult)
{
    return mInfo->mConstructor(aOuter, aIID, aResult);
}

NS_IMETHODIMP nsGenericFactory::LockFactory(PRBool aLock)
{
    // XXX do we care if (mInfo->mFlags & THREADSAFE)?
    return NS_OK;
}

NS_IMETHODIMP nsGenericFactory::GetInterfaces(PRUint32 *countp,
                                              nsIID* **array)
{
    if (!mInfo->mGetInterfacesProc) {
        *countp = 0;
        *array = nsnull;
        return NS_OK;
    }
    return mInfo->mGetInterfacesProc(countp, array);
}

NS_IMETHODIMP nsGenericFactory::GetHelperForLanguage(PRUint32 language,
                                                     nsISupports **helper)
{
    if (mInfo->mGetLanguageHelperProc)
        return mInfo->mGetLanguageHelperProc(language, helper);
    *helper = nsnull;
    return NS_OK;
}

NS_IMETHODIMP nsGenericFactory::GetContractID(char **aContractID)
{
    if (mInfo->mContractID) {
        *aContractID = (char *)nsMemory::Alloc(strlen(mInfo->mContractID) + 1);
        if (!*aContractID)
            return NS_ERROR_OUT_OF_MEMORY;
        strcpy(*aContractID, mInfo->mContractID);
    } else {
        *aContractID = nsnull;
    }
    return NS_OK;
}

NS_IMETHODIMP nsGenericFactory::GetClassDescription(char * *aClassDescription)
{
    if (mInfo->mDescription) {
        *aClassDescription = (char *)
            nsMemory::Alloc(strlen(mInfo->mDescription) + 1);
        if (!*aClassDescription)
            return NS_ERROR_OUT_OF_MEMORY;
        strcpy(*aClassDescription, mInfo->mDescription);
    } else {
        *aClassDescription = nsnull;
    }
    return NS_OK;
}

NS_IMETHODIMP nsGenericFactory::GetClassID(nsCID *aClassID)
{
    *aClassID = mInfo->mCID;
    return NS_OK;
}

NS_IMETHODIMP nsGenericFactory::GetImplementationLanguage(PRUint32 *langp)
{
    *langp = nsIProgrammingLanguage::CPLUSPLUS;
    return NS_OK;
}

NS_IMETHODIMP nsGenericFactory::GetFlags(PRUint32 *flagsp)
{
    *flagsp = mInfo->mFlags;
    return NS_OK;
}

// nsIGenericFactory: component-info accessors
NS_IMETHODIMP nsGenericFactory::SetComponentInfo(nsModuleComponentInfo *info)
{
    if (mInfo && mInfo->mClassInfoGlobal)
        *mInfo->mClassInfoGlobal = 0;
    mInfo = info;
    if (mInfo && mInfo->mClassInfoGlobal)
        *mInfo->mClassInfoGlobal = NS_STATIC_CAST(nsIClassInfo *, this);
    return NS_OK;
}

NS_IMETHODIMP nsGenericFactory::GetComponentInfo(nsModuleComponentInfo **infop)
{
    *infop = mInfo;
    return NS_OK;
}

NS_METHOD nsGenericFactory::Create(nsISupports* outer, const nsIID& aIID, void* *aInstancePtr)
{
    // sorry, aggregation not spoken here.
    nsresult res = NS_ERROR_NO_AGGREGATION;
    if (outer == NULL) {
        nsGenericFactory* factory = new nsGenericFactory;
        if (factory != NULL) {
            res = factory->QueryInterface(aIID, aInstancePtr);
            if (res != NS_OK)
                delete factory;
        } else {
            res = NS_ERROR_OUT_OF_MEMORY;
        }
    }
    return res;
}

NS_COM nsresult
NS_NewGenericFactory(nsIGenericFactory* *result,
                     nsModuleComponentInfo *info)
{
    nsresult rv;
    nsIGenericFactory* fact;
    rv = nsGenericFactory::Create(NULL, NS_GET_IID(nsIGenericFactory), (void**)&fact);
    if (NS_FAILED(rv)) return rv;
    rv = fact->SetComponentInfo(info);
    if (NS_FAILED(rv)) goto error;
    *result = fact;
    return rv;

  error:
    NS_RELEASE(fact);
    return rv;
}

////////////////////////////////////////////////////////////////////////////////

nsGenericModule::nsGenericModule(const char* moduleName, PRUint32 componentCount,
                                 nsModuleComponentInfo* components,
                                 nsModuleDestructorProc dtor)
    : mInitialized(PR_FALSE), 
      mModuleName(moduleName),
      mComponentCount(componentCount),
      mComponents(components),
      mFactories(32, PR_FALSE),
      mDtor(dtor)
{
    NS_INIT_ISUPPORTS();
}

nsGenericModule::~nsGenericModule()
{
    Shutdown();
}

NS_IMPL_THREADSAFE_ISUPPORTS1(nsGenericModule, nsIModule)

// Perform our one-time intialization for this module
nsresult
nsGenericModule::Initialize()
{
    if (mInitialized) {
        return NS_OK;
    }
    mInitialized = PR_TRUE;
    return NS_OK;
}

// Shutdown this module, releasing all of the module resources
void
nsGenericModule::Shutdown()
{
    if (mDtor)
        mDtor(this);
    // Release the factory objects
    mFactories.Reset();
}

// Create a factory object for creating instances of aClass.
NS_IMETHODIMP
nsGenericModule::GetClassObject(nsIComponentManager *aCompMgr,
                                const nsCID& aClass,
                                const nsIID& aIID,
                                void** r_classObj)
{
    nsresult rv;

    // Defensive programming: Initialize *r_classObj in case of error below
    if (!r_classObj) {
        return NS_ERROR_INVALID_POINTER;
    }
    *r_classObj = NULL;

    // Do one-time-only initialization if necessary
    if (!mInitialized) {
        rv = Initialize();
        if (NS_FAILED(rv)) {
            // Initialization failed! yikes!
            return rv;
        }
    }

    // Choose the appropriate factory, based on the desired instance
    // class type (aClass).
    nsIDKey key(aClass);
    nsCOMPtr<nsIGenericFactory> fact = getter_AddRefs(NS_REINTERPRET_CAST(nsIGenericFactory *, mFactories.Get(&key)));
    if (fact == nsnull) {
        nsModuleComponentInfo* desc = mComponents;
        for (PRUint32 i = 0; i < mComponentCount; i++) {
            if (desc->mCID.Equals(aClass)) {
                rv = NS_NewGenericFactory(getter_AddRefs(fact), desc);
                if (NS_FAILED(rv)) return rv;

                (void)mFactories.Put(&key, fact);
                goto found;
            }
            desc++;
        }
        // not found in descriptions
#ifdef DEBUG
        char* cs = aClass.ToString();
        printf("+++ nsGenericModule %s: unable to create factory for %s\n", mModuleName, cs);
        nsCRT::free(cs);
#endif
        // XXX put in stop-gap so that we don't search for this one again
        return NS_ERROR_FACTORY_NOT_REGISTERED;
    }
  found:    
    rv = fact->QueryInterface(aIID, r_classObj);
    return rv;
}

NS_IMETHODIMP
nsGenericModule::RegisterSelf(nsIComponentManager *aCompMgr,
                              nsIFile* aPath,
                              const char* registryLocation,
                              const char* componentType)
{
    nsresult rv = NS_OK;

#ifdef DEBUG
    printf("*** Registering %s components (all right -- a generic module!)\n", mModuleName);
#endif

    nsModuleComponentInfo* cp = mComponents;
    for (PRUint32 i = 0; i < mComponentCount; i++) {
        rv = aCompMgr->RegisterComponentWithType(cp->mCID, cp->mDescription,
                                                 cp->mContractID, aPath,
                                                 registryLocation, PR_TRUE,
                                                 PR_TRUE, componentType);
        if (NS_FAILED(rv)) {
#ifdef DEBUG
            printf("nsGenericModule %s: unable to register %s component => %x\n",
                   mModuleName?mModuleName:"(null)", cp->mDescription?cp->mDescription:"(null)", rv);
#endif
            break;
        }
        // Call the registration hook of the component, if any
        if (cp->mRegisterSelfProc)
        {
            rv = cp->mRegisterSelfProc(aCompMgr, aPath, registryLocation,
                                       componentType, cp);
            if (NS_FAILED(rv)) {
#ifdef DEBUG
                printf("nsGenericModule %s: Register hook for %s component returned error => %x\n",
                       mModuleName?mModuleName:"(null)", cp->mDescription?cp->mDescription:"(null)", rv);
#endif
                break;
            }
        }
        cp++;
    }

    return rv;
}

NS_IMETHODIMP
nsGenericModule::UnregisterSelf(nsIComponentManager* aCompMgr,
                            nsIFile* aPath,
                            const char* registryLocation)
{
#ifdef DEBUG
    printf("*** Unregistering %s components (all right -- a generic module!)\n", mModuleName);
#endif
    nsModuleComponentInfo* cp = mComponents;
    for (PRUint32 i = 0; i < mComponentCount; i++) {
        // Call the unregistration hook of the component, if any
        if (cp->mUnregisterSelfProc)
        {
            cp->mUnregisterSelfProc(aCompMgr, aPath, registryLocation, cp);
        }
        // Unregister the component
        nsresult rv = aCompMgr->UnregisterComponentSpec(cp->mCID, aPath);
        if (NS_FAILED(rv)) {
#ifdef DEBUG
            printf("nsGenericModule %s: unable to unregister %s component => %x\n",
                   mModuleName, cp->mDescription, rv);
#endif
        }
        cp++;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsGenericModule::CanUnload(nsIComponentManager *aCompMgr, PRBool *okToUnload)
{
    if (!okToUnload) {
        return NS_ERROR_INVALID_POINTER;
    }
    *okToUnload = PR_FALSE;
    return NS_ERROR_FAILURE;
}

NS_COM nsresult
NS_NewGenericModule(const char* moduleName,
                    PRUint32 componentCount,
                    nsModuleComponentInfo* components,
                    nsModuleDestructorProc dtor,
                    nsIModule* *result)
{
    nsresult rv = NS_OK;

    NS_ASSERTION(result, "Null argument");

    // Create and initialize the module instance
    nsGenericModule *m = 
        new nsGenericModule(moduleName, componentCount, components, dtor);
    if (!m) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    // Increase refcnt and store away nsIModule interface to m in return_cobj
    rv = m->QueryInterface(NS_GET_IID(nsIModule), (void**)result);
    if (NS_FAILED(rv)) {
        delete m;
        m = nsnull;
    }
    return rv;
}

////////////////////////////////////////////////////////////////////////////////
