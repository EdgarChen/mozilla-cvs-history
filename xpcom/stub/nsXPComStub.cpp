/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla.
 *
 * The Initial Developer of the Original Code is IBM Corporation.
 * Portions created by IBM Corporation are Copyright (C) 2004
 * IBM Corporation. All Rights Reserved.
 *
 * Contributor(s):
 *   Darin Fisher <darin@meer.net>
 *   Benjamin Smedberg <bsmedberg@covad.net>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsXPCOM.h"
#include "nsXPCOMPrivate.h"
#include "nsStringAPI.h"

#include <string.h>

static const XPCOMFunctions kFrozenFunctions = {
    XPCOM_GLUE_VERSION,
    sizeof(XPCOMFunctions),
    &NS_InitXPCOM2_P,
    &NS_ShutdownXPCOM_P,
    &NS_GetServiceManager_P,
    &NS_GetComponentManager_P,
    &NS_GetComponentRegistrar_P,
    &NS_GetMemoryManager_P,
    &NS_NewLocalFile_P,
    &NS_NewNativeLocalFile_P,
    &NS_RegisterXPCOMExitRoutine_P,
    &NS_UnregisterXPCOMExitRoutine_P,

    // these functions were added post 1.4
    &NS_GetDebug_P,
    &NS_GetTraceRefcnt_P,

    // these functions were added post 1.6
    &NS_StringContainerInit_P,
    &NS_StringContainerFinish_P,
    &NS_StringGetData_P,
    &NS_StringSetData_P,
    &NS_StringSetDataRange_P,
    &NS_StringCopy_P,
    &NS_CStringContainerInit_P,
    &NS_CStringContainerFinish_P,
    &NS_CStringGetData_P,
    &NS_CStringSetData_P,
    &NS_CStringSetDataRange_P,
    &NS_CStringCopy_P,
    &NS_CStringToUTF16_P,
    &NS_UTF16ToCString_P,
    &NS_StringCloneData_P,
    &NS_CStringCloneData_P,

    // these functions were added post 1.7 (post Firefox 1.0)
    &NS_Alloc_P,
    &NS_Realloc_P,
    &NS_Free_P,
    &NS_StringContainerInit2_P,
    &NS_CStringContainerInit2_P,
    &NS_StringGetMutableData_P,
    &NS_CStringGetMutableData_P,
    &NS_InitXPCOM3_P
};  

EXPORT_XPCOM_API(nsresult)
NS_GetFrozenFunctions(XPCOMFunctions *functions, const char* /* libraryPath */)
{
    if (!functions)
        return NS_ERROR_OUT_OF_MEMORY;

    if (functions->version != XPCOM_GLUE_VERSION)
        return NS_ERROR_FAILURE;

    PRUint32 size = functions->size;
    if (size > sizeof(XPCOMFunctions))
        size = sizeof(XPCOMFunctions);

    size -= offsetof(XPCOMFunctions, init);

    memcpy(&functions->init, &kFrozenFunctions.init, size);

    return NS_OK;
}

/*
 * Stubs for nsXPCOM.h
 */

#undef NS_InitXPCOM2
EXPORT_XPCOM_API(nsresult)
NS_InitXPCOM2(nsIServiceManager **result,
              nsIFile *binDirectory,
              nsIDirectoryServiceProvider *dirProvider)
{
  return NS_InitXPCOM2_P(result, binDirectory, dirProvider);
}

#undef NS_InitXPCOM3
EXPORT_XPCOM_API(nsresult)
NS_InitXPCOM3(nsIServiceManager **result,
	      nsIFile *binDirectory,
	      nsIDirectoryServiceProvider *dirProvider,
	      nsStaticModuleInfo const *staticComponents,
	      PRUint32 componentCount)
{
  return NS_InitXPCOM3_P(result, binDirectory, dirProvider,
                         staticComponents, componentCount);
}

#undef NS_ShutdownXPCOM
EXPORT_XPCOM_API(nsresult)
NS_ShutdownXPCOM(nsIServiceManager *svcMgr)
{
  return NS_ShutdownXPCOM_P(svcMgr);
}

#undef NS_GetServiceManager
EXPORT_XPCOM_API(nsresult)
NS_GetServiceManager(nsIServiceManager* *result)
{
  return NS_GetServiceManager_P(result);
}

#undef NS_GetComponentManager
EXPORT_XPCOM_API(nsresult)
NS_GetComponentManager(nsIComponentManager* *result)
{
  return NS_GetComponentManager_P(result);
}

#undef NS_GetComponentRegistrar
EXPORT_XPCOM_API(nsresult)
NS_GetComponentRegistrar(nsIComponentRegistrar* *result)
{
  return NS_GetComponentRegistrar_P(result);
}

#undef NS_GetMemoryManager
EXPORT_XPCOM_API(nsresult)
NS_GetMemoryManager(nsIMemory* *result)
{
  return NS_GetMemoryManager_P(result);
}

#undef NS_NewLocalFile
EXPORT_XPCOM_API(nsresult)
NS_NewLocalFile(const nsAString &path,
                PRBool followLinks,
                nsILocalFile **result)
{
  return NS_NewLocalFile_P(path, followLinks, result);
}

#undef NS_NewNativeLocalFile
EXPORT_XPCOM_API(nsresult)
NS_NewNativeLocalFile(const nsACString &path,
                      PRBool followLinks,
                      nsILocalFile **result)
{
  return NS_NewNativeLocalFile_P(path, followLinks, result);
}

#undef NS_GetDebug
EXPORT_XPCOM_API(nsresult)
NS_GetDebug(nsIDebug **result)
{
  return NS_GetDebug_P(result);
}

#undef NS_GetTraceRefcnt
EXPORT_XPCOM_API(nsresult)
NS_GetTraceRefcnt(nsITraceRefcnt **result)
{
  return NS_GetTraceRefcnt_P(result);
}

#undef NS_Alloc
EXPORT_XPCOM_API(void*)
NS_Alloc(PRSize size)
{
  return NS_Alloc_P(size);
}

#undef NS_Realloc
EXPORT_XPCOM_API(void*)
NS_Realloc(void* ptr, PRSize size)
{
  return NS_Realloc_P(ptr, size);
}

#undef NS_Free
EXPORT_XPCOM_API(void)
NS_Free(void* ptr)
{
  NS_Free_P(ptr);
}

/*
 * Stubs for nsXPCOMPrivate.h
 */

#undef NS_RegisterXPCOMExitRoutine
EXPORT_XPCOM_API(nsresult)
NS_RegisterXPCOMExitRoutine(XPCOMExitRoutine exitRoutine, PRUint32 priority)
{
  return NS_RegisterXPCOMExitRoutine_P(exitRoutine, priority);
}

#undef NS_UnregisterXPCOMExitRoutine
EXPORT_XPCOM_API(nsresult)
NS_UnregisterXPCOMExitRoutine(XPCOMExitRoutine exitRoutine)
{
  return NS_UnregisterXPCOMExitRoutine_P(exitRoutine);
}

/*
 * Stubs for nsStringAPI.h
 */

#undef NS_StringContainerInit
EXPORT_XPCOM_API(nsresult)
NS_StringContainerInit(nsStringContainer &aStr)
{
  return NS_StringContainerInit_P(aStr);
}

#undef NS_StringContainerInit2
EXPORT_XPCOM_API(nsresult)
NS_StringContainerInit2(nsStringContainer &aStr,
                        const PRUnichar   *aData,
                        PRUint32           aDataLength,
                        PRUint32           aFlags)
{   
  return NS_StringContainerInit2_P(aStr, aData, aDataLength, aFlags);
}

#undef NS_StringContainerFinish
EXPORT_XPCOM_API(void)
NS_StringContainerFinish(nsStringContainer &aStr)
{
  NS_StringContainerFinish_P(aStr);
}

#undef NS_StringGetData
EXPORT_XPCOM_API(PRUint32)
NS_StringGetData(const nsAString &aStr, const PRUnichar **aBuf, PRBool *aTerm)
{
  return NS_StringGetData_P(aStr, aBuf, aTerm);
}

#undef NS_StringGetMutableData
EXPORT_XPCOM_API(PRUint32)
NS_StringGetMutableData(nsAString &aStr, PRUint32 aLen, PRUnichar **aBuf)
{
  return NS_StringGetMutableData_P(aStr, aLen, aBuf);
}

#undef NS_StringCloneData
EXPORT_XPCOM_API(PRUnichar *)
NS_StringCloneData(const nsAString &aStr)
{
  return NS_StringCloneData_P(aStr);
}

#undef NS_StringSetData
EXPORT_XPCOM_API(nsresult)
NS_StringSetData(nsAString &aStr, const PRUnichar *aBuf, PRUint32 aCount)
{
  return NS_StringSetData_P(aStr, aBuf, aCount);
}

#undef NS_StringSetDataRange
EXPORT_XPCOM_API(nsresult)
NS_StringSetDataRange(nsAString &aStr, PRUint32 aCutStart, PRUint32 aCutLength,
                      const PRUnichar *aBuf, PRUint32 aCount)
{
  return NS_StringSetDataRange_P(aStr, aCutStart, aCutLength, aBuf, aCount);
}

#undef NS_StringCopy
EXPORT_XPCOM_API(nsresult)
NS_StringCopy(nsAString &aDest, const nsAString &aSrc)
{
  return NS_StringCopy_P(aDest, aSrc);
}

#undef NS_CStringContainerInit
EXPORT_XPCOM_API(nsresult)
NS_CStringContainerInit(nsCStringContainer &aStr)
{
  return NS_CStringContainerInit_P(aStr);
}

#undef NS_CStringContainerInit2
EXPORT_XPCOM_API(nsresult)
NS_CStringContainerInit2(nsCStringContainer &aStr,
                         const char         *aData,
                         PRUint32            aDataLength,
                         PRUint32            aFlags)
{   
  return NS_CStringContainerInit2_P(aStr, aData, aDataLength, aFlags);
}

#undef NS_CStringContainerFinish
EXPORT_XPCOM_API(void)
NS_CStringContainerFinish(nsCStringContainer &aStr)
{
  NS_CStringContainerFinish_P(aStr);
}

#undef NS_CStringGetData
EXPORT_XPCOM_API(PRUint32)
NS_CStringGetData(const nsACString &aStr, const char **aBuf, PRBool *aTerm)
{
  return NS_CStringGetData_P(aStr, aBuf, aTerm);
}

#undef NS_CStringGetMutableData
EXPORT_XPCOM_API(PRUint32)
NS_CStringGetMutableData(nsACString &aStr, PRUint32 aLen, char **aBuf)
{
  return NS_CStringGetMutableData_P(aStr, aLen, aBuf);
}

#undef NS_CStringCloneData
EXPORT_XPCOM_API(char *)
NS_CStringCloneData(const nsACString &aStr)
{
  return NS_CStringCloneData_P(aStr);
}

#undef NS_CStringSetData
EXPORT_XPCOM_API(nsresult)
NS_CStringSetData(nsACString &aStr, const char *aBuf, PRUint32 aCount)
{
  return NS_CStringSetData_P(aStr, aBuf, aCount);
}

#undef NS_CStringSetDataRange
EXPORT_XPCOM_API(nsresult)
NS_CStringSetDataRange(nsACString &aStr, PRUint32 aCutStart, PRUint32 aCutLength,
                       const char *aBuf, PRUint32 aCount)
{
  return NS_CStringSetDataRange_P(aStr, aCutStart, aCutLength, aBuf, aCount);
}

#undef NS_CStringCopy
EXPORT_XPCOM_API(nsresult)
NS_CStringCopy(nsACString &aDest, const nsACString &aSrc)
{
  return NS_CStringCopy_P(aDest, aSrc);
}

#undef NS_CStringToUTF16
EXPORT_XPCOM_API(nsresult)
NS_CStringToUTF16(const nsACString &aSrc, nsCStringEncoding aSrcEncoding, nsAString &aDest)
{
  return NS_CStringToUTF16_P(aSrc, aSrcEncoding, aDest);
}

#undef NS_UTF16ToCString
EXPORT_XPCOM_API(nsresult)
NS_UTF16ToCString(const nsAString &aSrc, nsCStringEncoding aDestEncoding, nsACString &aDest)
{
  return NS_UTF16ToCString_P(aSrc, aDestEncoding, aDest);
}
