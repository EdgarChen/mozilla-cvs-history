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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */

#define NS_IMPL_IDS

#include "nsString.h"
#include "nsICharsetAlias.h"
#include "nsIRegistry.h"
#include "nsIServiceManager.h"
#include "nsICharsetConverterManager.h"
#include "nsICharsetConverterManager2.h"
#include "nsIStringBundle.h"
#include "nsICharsetDetector.h"
#include "nsILocaleService.h"
#include "nsUConvDll.h"
#include "nsObjectArray.h"
#include "prmem.h"

// just for CIDs
#include "nsIUnicodeDecodeHelper.h"
#include "nsIUnicodeEncodeHelper.h"

static NS_DEFINE_IID(kRegistryNodeIID, NS_IREGISTRYNODE_IID);
static NS_DEFINE_CID(kRegistryCID, NS_REGISTRY_CID);
static NS_DEFINE_CID(kStringBundleServiceCID, NS_STRINGBUNDLESERVICE_CID);
static NS_DEFINE_CID(kLocaleServiceCID, NS_LOCALESERVICE_CID); 
static NS_DEFINE_CID(kSupportsArrayCID, NS_SUPPORTSARRAY_CID); 

//----------------------------------------------------------------------------
// Class nsConverterInfo [declaration]

// XXX give up these internal data structs and create&return the lists on the fly
class nsConverterInfo : public nsObject
{
public: 
  nsString *    mName;
  nsCID         mCID;

  nsConverterInfo();
  ~nsConverterInfo();
};

//----------------------------------------------------------------------------
// Class nsCharsetConverterManager [declaration]

/**
 * The actual implementation of the nsICharsetConverterManager interface.
 *
 * @created         15/Nov/1999
 * @author  Catalin Rotaru [CATA]
 */
class nsCharsetConverterManager : public nsICharsetConverterManager, 
nsICharsetConverterManager2
{
  NS_DECL_ISUPPORTS

private:

  nsObjectArray mDecoderArray;
  nsObjectArray mEncoderArray;
  nsIStringBundle * mDataBundle;
  nsIStringBundle * mTitleBundle;

  /**
   * Takes charset information from Registry and puts it into those arrays.
   */
  void FillInfoArrays();

  nsresult GetConverterList(nsObjectArray * aArray, nsString *** aResult, 
      PRInt32 * aCount);

  nsresult LoadExtensibleBundle(const char * aRegistryKey, 
      nsIStringBundle ** aResult);

  static nsresult RegisterConverterTitles(nsIRegistry * aRegistry, 
      char * aRegistryPath);

  static nsresult RegisterConverterData(nsIRegistry * aRegistry, 
      char * aRegistryPath);

  nsresult GetBundleValue(nsIStringBundle * aBundle, nsString * aName, 
      nsString * aProp, nsString ** aResult);

  nsresult GetBundleValue(nsIStringBundle * aBundle, nsString * aName, 
      nsString * aProp, nsIAtom ** aResult);

  nsresult GetBundleValue(nsIStringBundle * aBundle, const nsIAtom * aName, 
    nsString * aProp, PRUnichar ** aResult);

  nsresult GetBundleValue(nsIStringBundle * aBundle, const nsIAtom * aName, 
    nsString * aProp, nsIAtom ** aResult);

  nsresult GetRegistryEnumeration(char * aRegistryKey, nsStringArray * aArray);

  nsresult GetRegistryEnumeration(char * aRegistryKey, char * aAddPrefix,
    nsISupportsArray ** aArray);

  nsresult GetRegistryEnumeration2(char * aRegistryKey, PRBool aDecoder,
    nsISupportsArray ** aArray);

public:

  nsCharsetConverterManager();
  virtual ~nsCharsetConverterManager();

  static nsresult RegisterConverterManagerData();

  //--------------------------------------------------------------------------
  // Interface nsICharsetConverterManager [declaration]

  NS_IMETHOD GetUnicodeEncoder(const nsString * aDest, 
      nsIUnicodeEncoder ** aResult);
  NS_IMETHOD GetUnicodeDecoder(const nsString * aSrc, 
      nsIUnicodeDecoder ** aResult);

  NS_IMETHOD GetDecoderList(nsString *** aResult, PRInt32 * aCount);
  NS_IMETHOD GetEncoderList(nsString *** aResult, PRInt32 * aCount);
  NS_IMETHOD GetCharsetDetectorList(nsStringArray * aArray);

  NS_IMETHOD GetCharsetData(nsString * aCharset, nsString * aProp, 
      nsString ** aResult);
  NS_IMETHOD GetCharsetTitle(nsString * aCharset, nsString ** aResult);
  NS_IMETHOD GetCharsetLangGroup(nsString * aCharset, nsIAtom ** aResult);

  NS_IMETHOD GetMIMEMailCharset(nsString * aCharset, nsString ** aResult);
  NS_IMETHOD GetMIMEHeaderEncodingMethod(nsString * aCharset, nsString ** 
      aResult);

  //--------------------------------------------------------------------------
  // Interface nsICharsetConverterManager2 [declaration]

  NS_IMETHOD GetUnicodeDecoder(const nsIAtom * aCharset,
    nsIUnicodeDecoder ** aResult);
  NS_IMETHOD GetUnicodeEncoder(const nsIAtom * aCharset,
    nsIUnicodeEncoder ** aResult);

  NS_IMETHOD GetDecoderList(nsISupportsArray ** aResult);
  NS_IMETHOD GetEncoderList(nsISupportsArray ** aResult);
  NS_IMETHOD GetCharsetDetectorList(nsISupportsArray ** aResult);

  NS_IMETHOD GetCharsetAtom(const PRUnichar * aCharset, nsIAtom ** aResult);
  NS_IMETHOD GetCharsetTitle(const nsIAtom * aCharset, PRUnichar ** aResult);
  NS_IMETHOD GetCharsetData(const nsIAtom * aCharset, const PRUnichar * aProp, 
    PRUnichar ** aResult);
  NS_IMETHOD GetCharsetData2(const nsIAtom * aCharset, const PRUnichar * aProp, 
    nsString * aResult);
  NS_IMETHOD GetCharsetLangGroup(const nsIAtom * aCharset, nsIAtom ** aResult);
};

//----------------------------------------------------------------------------
// Global functions and data [implementation]

NS_IMETHODIMP NS_NewCharsetConverterManager(nsISupports* aOuter, 
                                            const nsIID& aIID,
                                            void** aResult)
{
  if (!aResult) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aOuter) {
    *aResult = nsnull;
    return NS_ERROR_NO_AGGREGATION;
  }
  nsCharsetConverterManager * inst = new nsCharsetConverterManager();
  if (!inst) {
    *aResult = nsnull;
    return NS_ERROR_OUT_OF_MEMORY;
  }
  nsresult res = inst->QueryInterface(aIID, aResult);
  if (NS_FAILED(res)) {
    *aResult = nsnull;
    delete inst;
  }
  return res;
}

NS_IMETHODIMP NS_RegisterConverterManagerData()
{
  return nsCharsetConverterManager::RegisterConverterManagerData();
}

//----------------------------------------------------------------------------
// Class nsConverterInfo [implementation]

nsConverterInfo::nsConverterInfo()
{
  mName = NULL;
}

nsConverterInfo::~nsConverterInfo()
{
  if (mName != NULL) delete mName;
}

//----------------------------------------------------------------------------
// Class nsCharsetConverterManager [implementation]

NS_IMPL_THREADSAFE_ISUPPORTS2(nsCharsetConverterManager,
                              nsICharsetConverterManager, 
                              nsICharsetConverterManager2);

nsCharsetConverterManager::nsCharsetConverterManager() 
:mDataBundle(NULL), mTitleBundle(NULL)
{
  NS_INIT_REFCNT();
  PR_AtomicIncrement(&g_InstanceCount);

  FillInfoArrays();
}

nsCharsetConverterManager::~nsCharsetConverterManager() 
{
  NS_IF_RELEASE(mDataBundle);
  NS_IF_RELEASE(mTitleBundle);
  PR_AtomicDecrement(&g_InstanceCount);
}

nsresult nsCharsetConverterManager::RegisterConverterManagerData()
{
  nsresult res = NS_OK;

  NS_WITH_SERVICE(nsIRegistry, registry, kRegistryCID, &res);
  if (NS_FAILED(res)) return res;

  // open registry if necessary
  PRBool regOpen = PR_FALSE;
  registry->IsOpen(&regOpen);
  if (!regOpen) {
    res = registry->OpenWellKnownRegistry(nsIRegistry::ApplicationComponentRegistry);
    if (NS_FAILED(res)) return res;
  }

  RegisterConverterTitles(registry, NS_TITLE_BUNDLE_REGISTRY_KEY);
  RegisterConverterData(registry, NS_DATA_BUNDLE_REGISTRY_KEY);

  return NS_OK;
}

nsresult nsCharsetConverterManager::RegisterConverterTitles(
                                    nsIRegistry * aRegistry,
                                    char * aRegistryPath)
{
  nsresult res;
  nsRegistryKey key;

  nsAutoString str; str.AssignWithConversion(aRegistryPath);
  str.AppendWithConversion("defaultFile");

  char * p = str.ToNewCString();
  res = aRegistry->AddSubtree(nsIRegistry::Common, p, &key);
  nsAllocator::Free(p);
  if (NS_FAILED(res)) return res;
  res = aRegistry->SetStringUTF8(key, "name", "resource:/res/charsetTitles.properties");
  if (NS_FAILED(res)) return res;

  return NS_OK;
}

nsresult nsCharsetConverterManager::RegisterConverterData(
                                    nsIRegistry * aRegistry,
                                    char * aRegistryPath)
{
  nsresult res;
  nsRegistryKey key;

  nsAutoString str; str.AssignWithConversion(aRegistryPath);
  str.AppendWithConversion("defaultFile");

  char * p = str.ToNewCString();
  res = aRegistry->AddSubtree(nsIRegistry::Common, p, &key);
  nsAllocator::Free(p);
  if (NS_FAILED(res)) return res;
  res = aRegistry->SetStringUTF8(key, "name", "resource:/res/charsetData.properties");
  if (NS_FAILED(res)) return res;

  return NS_OK;
}

// XXX rethink the registry structure(tree) for these converters
// The idea is to have two trees:
// .../uconv/decoder/(CID/name)
// .../uconv/encoder/(CID/name)
// XXX take the registry strings out and make them macros
void nsCharsetConverterManager::FillInfoArrays() 
{
  nsresult res = NS_OK;
  nsIEnumerator * components = NULL;
  nsIRegistry * registry = NULL;
  nsRegistryKey uconvKey, key;
  PRBool regOpen = PR_FALSE;

  // get the registry
  res = nsServiceManager::GetService(NS_REGISTRY_PROGID, 
    NS_GET_IID(nsIRegistry), (nsISupports**)&registry);
  if (NS_FAILED(res)) goto done;

  // open registry if necessary
  registry->IsOpen(&regOpen);
  if (!regOpen) {
    res = registry->OpenWellKnownRegistry(nsIRegistry::ApplicationComponentRegistry);
    if (NS_FAILED(res)) goto done;
  }

  // get subtree
  res = registry->GetSubtree(nsIRegistry::Common,  
    "software/netscape/intl/uconv", &uconvKey);
  if (NS_FAILED(res)) goto done;

  // enumerate subtrees
  res = registry->EnumerateSubtrees(uconvKey, &components);
  if (NS_FAILED(res)) goto done;
  res = components->First();
  if (NS_FAILED(res)) goto done;

  while (NS_OK != components->IsDone()) {
    nsISupports * base = NULL;
    nsIRegistryNode * node = NULL;
    char * name = NULL;
    char * src = NULL;
    char * dest = NULL;
    nsConverterInfo * ci = NULL;

    res = components->CurrentItem(&base);
    if (NS_FAILED(res)) goto done1;

    res = base->QueryInterface(kRegistryNodeIID, (void**)&node);
    if (NS_FAILED(res)) goto done1;

    res = node->GetNameUTF8(&name);
    if (NS_FAILED(res)) goto done1;

    ci = new nsConverterInfo();
    if (ci == NULL) goto done1;
    if (!(ci->mCID.Parse(name))) goto done1;

    res = node->GetKey(&key);
    if (NS_FAILED(res)) goto done1;

    res = registry->GetStringUTF8(key, "source", &src);
    if (NS_FAILED(res)) goto done1;

    res = registry->GetStringUTF8(key, "destination", &dest);
    if (NS_FAILED(res)) goto done1;

    // XXX do an alias resolution here instead
    if (!strcmp(src, "Unicode")) {
      ci->mName = new nsString;
      ci->mName->AssignWithConversion(dest);
      ci->mName->ToLowerCase();
      mEncoderArray.AddObject(ci);
    } else if (!strcmp(dest, "Unicode")) {
      ci->mName = new nsString;
      ci->mName->AssignWithConversion(src);
      ci->mName->ToLowerCase();
      mDecoderArray.AddObject(ci);
    } else goto done1;

    ci = NULL;

done1:
    NS_IF_RELEASE(base);
    NS_IF_RELEASE(node);

    if (name != NULL) nsCRT::free(name);
    if (src != NULL) nsCRT::free(src);
    if (dest != NULL) nsCRT::free(dest);
    if (ci != NULL) delete ci;

    res = components->Next();
    if (NS_FAILED(res)) break; // this is NOT supposed to fail!
  }

  // finish and clean up
done:
  if (registry != NULL) {
    nsServiceManager::ReleaseService(NS_REGISTRY_PROGID, registry);
  }

  NS_IF_RELEASE(components);
}

nsresult nsCharsetConverterManager::GetConverterList(
                                    nsObjectArray * aArray,
                                    nsString *** aResult,
                                    PRInt32 * aCount)
{
  *aResult = NULL;
  *aCount = 0;

  PRInt32 size = aArray->GetUsage();
  nsConverterInfo ** array = (nsConverterInfo **)aArray->GetArray();
  if ((size == 0) || (array == NULL)) return NS_OK;

  *aResult = new nsString * [size];
  if (*aResult == NULL) return NS_ERROR_OUT_OF_MEMORY;

  *aCount = size;
  for (PRInt32 i=0;i<size;i++) (*aResult)[i] = array[i]->mName;

  return NS_OK;

  // XXX also create new Strings here, as opposed to just providing pointers 
  // to the existing ones
}

nsresult nsCharsetConverterManager::LoadExtensibleBundle(
                                    const char* aRegistryKey, 
                                    nsIStringBundle ** aResult)
{
  nsresult res = NS_OK;
  nsCOMPtr<nsILocale> locale = nsnull;

  NS_WITH_SERVICE(nsIStringBundleService, sbServ, kStringBundleServiceCID, &res);
  if (NS_FAILED(res)) return res;

  NS_WITH_SERVICE(nsILocaleService, localeServ, kLocaleServiceCID, &res);
  if (NS_FAILED(res)) return res;

  res = localeServ->GetApplicationLocale(getter_AddRefs(locale));
  if (NS_FAILED(res)) return res;

  res = sbServ->CreateExtensibleBundle(aRegistryKey, locale, aResult);
  if (NS_FAILED(res)) return res;

  return res;
}

nsresult nsCharsetConverterManager::GetBundleValue(nsIStringBundle * aBundle, 
                                                   nsString * aName, 
                                                   nsString * aProp, 
                                                   nsString ** aResult)
{
  nsresult res = NS_OK;

  nsAutoString key(*aName);
  if (aProp != NULL) key.Append(*aProp); // yes, this parameter may be NULL

  PRUnichar * value = NULL;
  res = aBundle->GetStringFromName(key.GetUnicode(), &value);
  if (NS_FAILED(res)) return res;

  *aResult = new nsString(value);
  PR_Free(value);
  return res;
}

nsresult nsCharsetConverterManager::GetBundleValue(nsIStringBundle * aBundle, 
                                                   nsString * aName, 
                                                   nsString * aProp, 
                                                   nsIAtom ** aResult)
{
  nsresult res = NS_OK;

  nsAutoString key(*aName);
  if (aProp != NULL) key.Append(*aProp); // yes, this parameter may be NULL

  PRUnichar * value = NULL;
  res = aBundle->GetStringFromName(key.GetUnicode(), &value);
  if (NS_FAILED(res)) return res;

  *aResult =  NS_NewAtom(value);
  PR_Free(value);
  return res;
}

nsresult nsCharsetConverterManager::GetBundleValue(nsIStringBundle * aBundle, 
                                                   const nsIAtom * aName, 
                                                   nsString * aProp, 
                                                   PRUnichar ** aResult)
{
  nsresult res = NS_OK;

  nsAutoString key;
  res = ((nsIAtom *) aName)->ToString(key);
  if (NS_FAILED(res)) return res;

  key.ToLowerCase(); // we lowercase the main comparison key
  if (aProp != NULL) key.Append(*aProp); // yes, this param may be NULL

  res = aBundle->GetStringFromName(key.GetUnicode(), aResult);
  return res;
}

nsresult nsCharsetConverterManager::GetBundleValue(nsIStringBundle * aBundle, 
                                                   const nsIAtom * aName, 
                                                   nsString * aProp, 
                                                   nsIAtom ** aResult)
{
  nsresult res = NS_OK;

  PRUnichar * value;
  res = GetBundleValue(aBundle, aName, aProp, &value);
  if (NS_FAILED(res)) return res;

  *aResult =  NS_NewAtom(value);
  PR_Free(value);

  return NS_OK;
}

nsresult nsCharsetConverterManager::GetRegistryEnumeration(
                                    char * aRegistryKey, 
                                    nsStringArray * aArray)
{
  nsresult res = NS_OK;
  nsIEnumerator * components = NULL;
  nsIRegistry * registry = NULL;
  nsRegistryKey key;
  PRBool regOpen = PR_FALSE;

  // get the registry
  res = nsServiceManager::GetService(NS_REGISTRY_PROGID, 
    NS_GET_IID(nsIRegistry), (nsISupports**)&registry);
  if (NS_FAILED(res)) goto done;

  // open registry if necessary
  registry->IsOpen(&regOpen);
  if (!regOpen) {
    res = registry->OpenWellKnownRegistry(nsIRegistry::ApplicationComponentRegistry);
    if (NS_FAILED(res)) goto done;
  }

  // get subtree
  res = registry->GetSubtree(nsIRegistry::Common, aRegistryKey, &key);
  if (NS_FAILED(res)) goto done;

  // enumerate subtrees
  res = registry->EnumerateSubtrees(key, &components);
  if (NS_FAILED(res)) goto done;
  res = components->First();
  if (NS_FAILED(res)) goto done;

  while (NS_OK != components->IsDone()) {
    nsISupports * base = NULL;
    nsIRegistryNode * node = NULL;
    char * name = NULL;
    nsString * s = NULL;

    res = components->CurrentItem(&base);
    if (NS_FAILED(res)) goto done1;

    res = base->QueryInterface(kRegistryNodeIID, (void**)&node);
    if (NS_FAILED(res)) goto done1;

    res = node->GetNameUTF8(&name);
    if (NS_FAILED(res)) goto done1;

      // STRING USE WARNING: this use should be looked at.  Why does |s| have to be allocated?
      //  Couldn't it just be an |nsAutoString| on the stack?
    s = new nsString;
    s->AssignWithConversion("charsetDetector.");
    if (s == NULL) {
      res = NS_ERROR_OUT_OF_MEMORY;
      goto done1;
    }
    s->AppendWithConversion(name);
    aArray->AppendString(*s);

done1:
    NS_IF_RELEASE(base);
    NS_IF_RELEASE(node);

    if (name != NULL) nsCRT::free(name);
    if (s != NULL) delete s;

    res = components->Next();
    if (NS_FAILED(res)) break; // this is NOT supposed to fail!
  }

  // finish and clean up
done:
  if (registry != NULL) {
    nsServiceManager::ReleaseService(NS_REGISTRY_PROGID, registry);
  }

  NS_IF_RELEASE(components);
  return res;
}

nsresult nsCharsetConverterManager::GetRegistryEnumeration(
                                    char * aRegistryKey, 
                                    char * aAddPrefix,
                                    nsISupportsArray ** aArray)
{
  nsresult res = NS_OK;
  nsCOMPtr<nsISupportsArray> array = NULL;
  nsRegistryKey key;

  res = nsComponentManager::CreateInstance(kSupportsArrayCID, NULL, 
      NS_GET_IID(nsISupportsArray), getter_AddRefs(array));
  if (NS_FAILED(res)) return res;

  // get the registry
  NS_WITH_SERVICE(nsIRegistry, registry, kRegistryCID, &res);
  if (NS_FAILED(res)) return res;

  // open registry if necessary
  PRBool regOpen = PR_FALSE;
  registry->IsOpen(&regOpen);
  if (!regOpen) {
    res = registry->OpenWellKnownRegistry(nsIRegistry::ApplicationComponentRegistry);
    if (NS_FAILED(res)) return res;
  }

  // get subtree
  res = registry->GetSubtree(nsIRegistry::Common, aRegistryKey, &key);
  if (NS_FAILED(res)) return res;

  // enumerate subtrees
  nsCOMPtr<nsIEnumerator> components;
  res = registry->EnumerateSubtrees(key, getter_AddRefs(components));
  if (NS_FAILED(res)) return res;
  res = components->First();
  if (NS_FAILED(res)) return res;

  while (NS_OK != components->IsDone()) {
    nsCOMPtr<nsISupports> base;
    nsCOMPtr<nsIRegistryNode> node;
    char * name = NULL;
    nsAutoString fullName(aAddPrefix);
    nsCOMPtr<nsIAtom> atom;

    res = components->CurrentItem(getter_AddRefs(base));
    if (NS_FAILED(res)) goto done1;

    node = do_QueryInterface(base, &res);
    if (NS_FAILED(res)) goto done1;

    res = node->GetNameUTF8(&name);
    if (NS_FAILED(res)) goto done1;

    fullName.Append(name);
    res = GetCharsetAtom(fullName.GetUnicode(), getter_AddRefs(atom));
    if (NS_FAILED(res)) goto done1;

    res = array->AppendElement(atom);
    if (NS_FAILED(res)) goto done1;

done1:
    if (name != NULL) nsCRT::free(name);

    res = components->Next();
    if (NS_FAILED(res)) break; // this is NOT supposed to fail!
  }

  // everything was fine, set the result
  *aArray = array;
  NS_ADDREF(*aArray);

  return res;
}

// XXX deprecate this method and switch to GetRegistryEnumeration() when 
// changing the registration style for converters.
// The idea is to have two trees:
// .../uconv/decoder/name
// .../uconv/encoder/name
nsresult nsCharsetConverterManager::GetRegistryEnumeration2(
                                    char * aRegistryKey, 
                                    PRBool aDecoder,
                                    nsISupportsArray ** aArray)
{
  nsresult res = NS_OK;
  nsCOMPtr<nsISupportsArray> array = NULL;
  nsRegistryKey key;

  res = nsComponentManager::CreateInstance(kSupportsArrayCID, NULL, 
      NS_GET_IID(nsISupportsArray), getter_AddRefs(array));
  if (NS_FAILED(res)) return res;

  // get the registry
  NS_WITH_SERVICE(nsIRegistry, registry, kRegistryCID, &res);
  if (NS_FAILED(res)) return res;

  // open registry if necessary
  PRBool regOpen = PR_FALSE;
  registry->IsOpen(&regOpen);
  if (!regOpen) {
    res = registry->OpenWellKnownRegistry(nsIRegistry::ApplicationComponentRegistry);
    if (NS_FAILED(res)) return res;
  }

  // get subtree
  res = registry->GetSubtree(nsIRegistry::Common, aRegistryKey, &key);
  if (NS_FAILED(res)) return res;

  // enumerate subtrees
  nsCOMPtr<nsIEnumerator> components;
  res = registry->EnumerateSubtrees(key, getter_AddRefs(components));
  if (NS_FAILED(res)) return res;
  res = components->First();
  if (NS_FAILED(res)) return res;

  while (NS_OK != components->IsDone()) {
    nsCOMPtr<nsISupports> base;
    nsCOMPtr<nsIRegistryNode> node;
    char * src = NULL;
    char * dest = NULL;
    nsAutoString fullName;
    nsCOMPtr<nsIAtom> atom;

    res = components->CurrentItem(getter_AddRefs(base));
    if (NS_FAILED(res)) goto done1;

    node = do_QueryInterface(base, &res);
    if (NS_FAILED(res)) goto done1;

    res = node->GetKey(&key);
    if (NS_FAILED(res)) goto done1;

    res = registry->GetStringUTF8(key, "source", &src);
    if (NS_FAILED(res)) goto done1;

    res = registry->GetStringUTF8(key, "destination", &dest);
    if (NS_FAILED(res)) goto done1;

    if (aDecoder) {
      if (!strcmp(dest, "Unicode")) {
        fullName.Assign(src);
        res = GetCharsetAtom(fullName.GetUnicode(), getter_AddRefs(atom));
        if (NS_FAILED(res)) goto done1;

        res = array->AppendElement(atom);
        if (NS_FAILED(res)) goto done1;
      }
    } else {
      if (!strcmp(src, "Unicode")) {
        fullName.Assign(dest);
        res = GetCharsetAtom(fullName.GetUnicode(), getter_AddRefs(atom));
        if (NS_FAILED(res)) goto done1;

        res = array->AppendElement(atom);
        if (NS_FAILED(res)) goto done1;
      }
    }

done1:
    if (src != NULL) nsCRT::free(src);
    if (dest != NULL) nsCRT::free(dest);

    res = components->Next();
    if (NS_FAILED(res)) break; // this is NOT supposed to fail!
  }

  // everything was fine, set the result
  *aArray = array;
  NS_ADDREF(*aArray);

  return res;
}

//----------------------------------------------------------------------------
// Interface nsICharsetConverterManager [implementation]

NS_IMETHODIMP nsCharsetConverterManager::GetUnicodeEncoder(
                                         const nsString * aDest, 
                                         nsIUnicodeEncoder ** aResult)
{
  *aResult= nsnull;
  nsIComponentManager* comMgr;
  nsresult res;
  res = NS_GetGlobalComponentManager(&comMgr);
  if(NS_FAILED(res))
     return res;
  PRInt32 baselen = nsCRT::strlen(NS_UNICODEENCODER_PROGID_BASE);
  char progid[256];
  PL_strncpy(progid, NS_UNICODEENCODER_PROGID_BASE, 256);
  aDest->ToCString(progid + baselen, 256 - baselen);
  res = comMgr->CreateInstanceByProgID(progid,NULL,
                 NS_GET_IID(nsIUnicodeEncoder),(void**)aResult);
  if(NS_FAILED(res))
    res = NS_ERROR_UCONV_NOCONV;
  return res;
}

NS_IMETHODIMP nsCharsetConverterManager::GetUnicodeDecoder(
                                         const nsString * aSrc, 
                                         nsIUnicodeDecoder ** aResult)
{
  *aResult= nsnull;
  nsIComponentManager* comMgr;
  nsresult res;
  res = NS_GetGlobalComponentManager(&comMgr);
  if(NS_FAILED(res))
     return res;
  PRInt32 baselen = nsCRT::strlen(NS_UNICODEDECODER_PROGID_BASE);
  char progid[256];
  PL_strncpy(progid, NS_UNICODEDECODER_PROGID_BASE, 256);
  aSrc->ToCString(progid + baselen, 256 - baselen);
  res = comMgr->CreateInstanceByProgID(progid,NULL,
                 NS_GET_IID(nsIUnicodeDecoder),(void**)aResult);
  if(NS_FAILED(res))
    res = NS_ERROR_UCONV_NOCONV;
  return res;
}

NS_IMETHODIMP nsCharsetConverterManager::GetDecoderList(nsString *** aResult, 
                                                        PRInt32 * aCount)
{
  return GetConverterList(&mDecoderArray, aResult, aCount);
}

NS_IMETHODIMP nsCharsetConverterManager::GetEncoderList(nsString *** aResult, 
                                                        PRInt32 * aCount)
{
  return GetConverterList(&mEncoderArray, aResult, aCount);
}

NS_IMETHODIMP nsCharsetConverterManager::GetCharsetDetectorList(
                                         nsStringArray * aArray)
{
  return GetRegistryEnumeration(NS_CHARSET_DETECTOR_REG_BASE, aArray);
}

NS_IMETHODIMP nsCharsetConverterManager::GetCharsetData(nsString * aCharset, 
                                                        nsString * aProp, 
                                                        nsString ** aResult)
{
  if (aCharset == NULL) return NS_ERROR_NULL_POINTER;
  if (aResult == NULL) return NS_ERROR_NULL_POINTER;
  *aResult = NULL;

  nsresult res = NS_OK;

  if (mDataBundle == NULL) {
    res = LoadExtensibleBundle(NS_DATA_BUNDLE_REGISTRY_KEY, &mDataBundle);
    if (NS_FAILED(res)) return res;
  }

  res = GetBundleValue(mDataBundle, aCharset, aProp, aResult);
  return res;
}

NS_IMETHODIMP nsCharsetConverterManager::GetCharsetTitle(nsString * aCharset, 
                                                         nsString ** aResult)
{
  if (aCharset == NULL) return NS_ERROR_NULL_POINTER;
  if (aResult == NULL) return NS_ERROR_NULL_POINTER;
  *aResult = NULL;

  nsresult res = NS_OK;
  nsAutoString prop; prop.AssignWithConversion(".title");

  if (mTitleBundle == NULL) {
    res = LoadExtensibleBundle(NS_TITLE_BUNDLE_REGISTRY_KEY, &mTitleBundle);
    if (NS_FAILED(res)) return res;
  }

  res = GetBundleValue(mTitleBundle, aCharset, &prop, aResult);
  return res;
}

NS_IMETHODIMP nsCharsetConverterManager::GetCharsetLangGroup(
                                         nsString * aCharset, 
                                         nsIAtom ** aResult)
{
  if (aCharset == NULL) return NS_ERROR_NULL_POINTER;
  if (aResult == NULL) return NS_ERROR_NULL_POINTER;
  *aResult = NULL;

  nsresult res = NS_OK;;
  nsAutoString prop; prop.AssignWithConversion(".LangGroup");

  if (mDataBundle == NULL) {
    res = LoadExtensibleBundle(NS_DATA_BUNDLE_REGISTRY_KEY, &mDataBundle);
    if (NS_FAILED(res)) return res;
  }

  res = GetBundleValue(mDataBundle, aCharset, &prop, aResult);
  return res;
}

NS_IMETHODIMP nsCharsetConverterManager::GetMIMEMailCharset(
                                         nsString * aCharset, 
                                         nsString ** aResult)
{
  nsAutoString prop; prop.AssignWithConversion(".MIMEMailCharset");
  return GetCharsetData(aCharset, &prop, aResult);
}

NS_IMETHODIMP nsCharsetConverterManager::GetMIMEHeaderEncodingMethod(
                                         nsString * aCharset, 
                                         nsString ** aResult)
{
  nsAutoString prop; prop.AssignWithConversion(".MIMEHeaderEncodingMethod");
  return GetCharsetData(aCharset, &prop, aResult);
}

//----------------------------------------------------------------------------
// Interface nsICharsetConverterManager2 [implementation]

NS_IMETHODIMP nsCharsetConverterManager::GetUnicodeDecoder(
                                         const nsIAtom * aCharset, 
                                         nsIUnicodeDecoder ** aResult)
{
  if (aCharset == NULL) return NS_ERROR_NULL_POINTER;
  if (aResult == NULL) return NS_ERROR_NULL_POINTER;
  *aResult = NULL;

  nsresult res = NS_OK;

  const PRUnichar * name;
  res = ((nsIAtom *) aCharset)->GetUnicode(&name);
  if (NS_FAILED(res)) return res;

  nsAutoString progID(NS_UNICODEDECODER_PROGID_BASE);
  progID.Append(name);
  char buff[256];
  progID.ToCString(buff, 256);

  res = nsComponentManager::CreateInstance(buff, NULL, 
    NS_GET_IID(nsIUnicodeDecoder), (void **) aResult);

  return res;
}

NS_IMETHODIMP nsCharsetConverterManager::GetUnicodeEncoder(
                                         const nsIAtom * aCharset, 
                                         nsIUnicodeEncoder ** aResult)
{
  if (aCharset == NULL) return NS_ERROR_NULL_POINTER;
  if (aResult == NULL) return NS_ERROR_NULL_POINTER;
  *aResult = NULL;

  nsresult res = NS_OK;

  const PRUnichar * name;
  res = ((nsIAtom *) aCharset)->GetUnicode(&name);
  if (NS_FAILED(res)) return res;

  nsAutoString progID(NS_UNICODEENCODER_PROGID_BASE);
  progID.Append(name);
  char buff[256];
  progID.ToCString(buff, 256);

  res = nsComponentManager::CreateInstance(buff, NULL, 
    NS_GET_IID(nsIUnicodeEncoder), (void **) aResult);

  return res;
}

// XXX move this macro into the UnicodeDecoder/Encoder interface
#define NS_CHARSET_CONVERTER_REG_BASE   "software/netscape/intl/uconv"

NS_IMETHODIMP nsCharsetConverterManager::GetDecoderList(
                                         nsISupportsArray ** aResult)
{
  if (aResult == NULL) return NS_ERROR_NULL_POINTER;
  *aResult = NULL;

  return GetRegistryEnumeration2(NS_CHARSET_CONVERTER_REG_BASE, PR_TRUE, 
    aResult);
}

NS_IMETHODIMP nsCharsetConverterManager::GetEncoderList(
                                         nsISupportsArray ** aResult)
{
  if (aResult == NULL) return NS_ERROR_NULL_POINTER;
  *aResult = NULL;

  return GetRegistryEnumeration2(NS_CHARSET_CONVERTER_REG_BASE, PR_FALSE, 
    aResult);
}

NS_IMETHODIMP nsCharsetConverterManager::GetCharsetDetectorList(
                                         nsISupportsArray ** aResult)
{
  if (aResult == NULL) return NS_ERROR_NULL_POINTER;
  *aResult = NULL;

  return GetRegistryEnumeration(NS_CHARSET_DETECTOR_REG_BASE, "chardet.", 
    aResult);
}

// XXX Improve the implementation of this method. Right now, it is build on 
// top of two things: the nsCharsetAlias service and the Atom engine. We can 
// improve on both. First, make the nsCharsetAlias better, with its own hash 
// table (not the StringBundle anymore) and a nicer file format. Second, 
// reimplement the Atom engine for the specific Charset case - more optimal.
// Finally, unify the two for even better performance.
NS_IMETHODIMP nsCharsetConverterManager::GetCharsetAtom(
                                         const PRUnichar * aCharset, 
                                         nsIAtom ** aResult)
{
  if (aCharset == NULL) return NS_ERROR_NULL_POINTER;
  if (aResult == NULL) return NS_ERROR_NULL_POINTER;
  *aResult = NULL;

  nsresult res;

  // We try to obtain the preferred name for this charset from the charset 
  // aliases. If we don't get it from there, we just use the original string
  nsAutoString charset(aCharset);
  NS_WITH_SERVICE(nsICharsetAlias, csAlias, kCharsetAliasCID, &res);
  NS_ASSERTION(NS_SUCCEEDED(res), "failed to get the CharsetAlias service");
  if (NS_SUCCEEDED(res)) {
    nsAutoString pref;
    res = csAlias->GetPreferred(charset, pref);
    if (NS_SUCCEEDED(res)) {
      charset.Assign(pref);
    }
  }

  // turn that cannonical name into an Atom
  nsCOMPtr<nsIAtom> atom =  getter_AddRefs(NS_NewAtom(charset));
  if (atom.get() == NULL) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  // everything was fine, set the result
  *aResult = atom;
  NS_ADDREF(*aResult);

  return NS_OK;
}

NS_IMETHODIMP nsCharsetConverterManager::GetCharsetTitle(
                                         const nsIAtom * aCharset, 
                                         PRUnichar ** aResult)
{
  if (aCharset == NULL) return NS_ERROR_NULL_POINTER;
  if (aResult == NULL) return NS_ERROR_NULL_POINTER;
  *aResult = NULL;

  nsresult res = NS_OK;
  nsAutoString prop(".title");

  if (mTitleBundle == NULL) {
    res = LoadExtensibleBundle(NS_TITLE_BUNDLE_REGISTRY_KEY, &mTitleBundle);
    if (NS_FAILED(res)) return res;
  }

  res = GetBundleValue(mTitleBundle, aCharset, &prop, aResult);
  return res;
}

NS_IMETHODIMP nsCharsetConverterManager::GetCharsetData(
                                         const nsIAtom * aCharset, 
                                         const PRUnichar * aProp,
                                         PRUnichar ** aResult)
{
  if (aCharset == NULL) return NS_ERROR_NULL_POINTER;
  // aProp can be NULL
  if (aResult == NULL) return NS_ERROR_NULL_POINTER;
  *aResult = NULL;

  nsresult res = NS_OK;
  nsAutoString prop(aProp);

  if (mDataBundle == NULL) {
    res = LoadExtensibleBundle(NS_DATA_BUNDLE_REGISTRY_KEY, &mDataBundle);
    if (NS_FAILED(res)) return res;
  }

  res = GetBundleValue(mDataBundle, aCharset, &prop, aResult);
  return res;
}

NS_IMETHODIMP nsCharsetConverterManager::GetCharsetData2(
                                         const nsIAtom * aCharset, 
                                         const PRUnichar * aProp,
                                         nsString * aResult)
{
  if (aResult == NULL) return NS_ERROR_NULL_POINTER;

  nsresult res = NS_OK;

  PRUnichar * data;
  res = GetCharsetData(aCharset, aProp, &data);
  if (NS_FAILED(res)) return res;

  aResult->Assign(data);
  PR_Free(data);
  return res;
}

NS_IMETHODIMP nsCharsetConverterManager::GetCharsetLangGroup(
                                         const nsIAtom * aCharset, 
                                         nsIAtom ** aResult)
{
  if (aCharset == NULL) return NS_ERROR_NULL_POINTER;
  if (aResult == NULL) return NS_ERROR_NULL_POINTER;
  *aResult = NULL;

  nsresult res = NS_OK;
  nsAutoString prop(".LangGroup");

  if (mDataBundle == NULL) {
    res = LoadExtensibleBundle(NS_DATA_BUNDLE_REGISTRY_KEY, &mDataBundle);
    if (NS_FAILED(res)) return res;
  }

  res = GetBundleValue(mDataBundle, aCharset, &prop, aResult);
  return res;
}
