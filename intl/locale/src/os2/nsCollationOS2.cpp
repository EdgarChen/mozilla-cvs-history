/*
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 * License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is the Mozilla OS/2 libraries.
 *
 * The Initial Developer of the Original Code is John Fairhurst,
 * <john_fairhurst@iname.com>.  Portions created by John Fairhurst are
 * Copyright (C) 1999 John Fairhurst. All Rights Reserved.
 *
 * Contributor(s): 
 * This Original Code has been modified by IBM Corporation.
 * Modifications made by IBM described herein are
 * Copyright (c) International Business Machines
 * Corporation, 2000
 *
 * Modifications to Mozilla code or documentation
 * identified per MPL Section 3.3
 *
 * Date             Modified by     Description of modification
 * 07/05/2000       IBM Corp.      Reworked file.
 */

#include "unidef.h"
#include "prmem.h"
#include "nsCollationOS2.h"
#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsLocaleCID.h"
#include "nsILocaleService.h"
#include "nsIPlatformCharset.h"
#include "nsIOS2Locale.h"
#include "nsCOMPtr.h"
#include "nsString.h"

NS_IMPL_ISUPPORTS1(nsCollationOS2, nsICollation)

nsCollationOS2::nsCollationOS2()
{
  mCollation = NULL;
}

nsCollationOS2::~nsCollationOS2()
{
   if (mCollation != NULL)
     delete mCollation;
}

/* Workaround for GCC problem */
#ifndef LOCI_iCodepage
#define LOCI_iCodepage ((LocaleItem)111)
#endif

nsresult nsCollationOS2::Initialize(nsILocale *locale)
{
  NS_ASSERTION(mCollation == NULL, "Should only be initialized once");

  nsresult res;

  mCollation = new nsCollation;
  if (mCollation == NULL) {
    NS_ASSERTION(0, "mCollation creation failed");
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return NS_OK;
}

 
nsresult nsCollationOS2::GetSortKeyLen(const nsCollationStrength strength, 
                                       const nsAString& stringIn, PRUint32* outLen)
{ 
  // this may not necessary because collation key length 
  // probably will not change by this normalization
  nsresult res = NS_OK;
  nsAutoString stringNormalized;
  if (strength != kCollationCaseSensitive) {
    res = mCollation->NormalizeString(stringIn, stringNormalized);
  } else {
    stringNormalized = stringIn;
  }

  LocaleObject locObj = NULL;
  int ret = UniCreateLocaleObject(UNI_UCS_STRING_POINTER, (UniChar *)L"", &locObj);
  if (ret != ULS_SUCCESS)
    UniCreateLocaleObject(UNI_UCS_STRING_POINTER, (UniChar *)L"en_US", &locObj);
  int uLen = UniStrxfrm(locObj, NULL, NS_REINTERPRET_CAST(const UniChar *, stringNormalized.get()), 0);
  if ( uLen > 0 ) {
    uLen += 5;      // Allow for the "extra" chars UniStrxfrm() will out put
                    // (overrunning the buffer if you let it...)
    uLen *= 2;      // And then adjust uLen to be the size in bytes,
                    // not unicode character count
  } else {
    uLen = 0;
  }  
  *outLen = uLen;
  UniFreeLocaleObject(locObj);
  return res;
}


nsresult nsCollationOS2::CreateRawSortKey(const nsCollationStrength strength, 
                                          const nsAString& stringIn, PRUint8* key, PRUint32* outLen)
{
  nsresult res = NS_OK;

  nsAutoString stringNormalized;
  if (strength != kCollationCaseSensitive) {
    res = mCollation->NormalizeString(stringIn, stringNormalized);
  } else {
    stringNormalized = stringIn;
  }

  LocaleObject locObj = NULL;
  int ret = UniCreateLocaleObject(UNI_UCS_STRING_POINTER, (UniChar *)L"", &locObj);
  if (ret != ULS_SUCCESS)
    UniCreateLocaleObject(UNI_UCS_STRING_POINTER, (UniChar *)L"en_US", &locObj);

  res = NS_ERROR_FAILURE;               // From here on out assume failure...
  int length = UniStrxfrm(locObj, NULL, NS_REINTERPRET_CAST(const UniChar *,stringNormalized.get()),0);
  if (length >= 0) {
    length += 5;                        // Allow for the "extra" chars UniStrxfrm()
                                        //  will out put (overrunning the buffer if
                                        // you let it...)
    // Magic, persistant buffer.  If it's not twice the size we need,
    // we grow/reallocate it 4X so it doesn't grow often.
    static UniChar* pLocalBuffer = NULL;
    static int iBufferLength = 100;
    if (iBufferLength < length*2) {
      if ( pLocalBuffer ) {
        free(pLocalBuffer);
        pLocalBuffer = nsnull;
      }
      iBufferLength = length*4;
    }
    if (!pLocalBuffer)
      pLocalBuffer = (UniChar*) malloc(sizeof(UniChar) * iBufferLength);
    if (pLocalBuffer) {
      // Do the Xfrm
      int uLen = UniStrxfrm(locObj, pLocalBuffer, NS_REINTERPRET_CAST(const UniChar *,stringNormalized.get()), iBufferLength);
      // See how big the result really is
      uLen = UniStrlen(pLocalBuffer);
      // make sure it will fit in the output buffer...
      if (uLen < length)
          // Success!
          // Give 'em the real size in bytes...
          *outLen = uLen * 2;
          // And copy the string, byte reversed, so that it can be
          // memcmp'ed
          char *srcLow, *srcHi, *dst;
          srcLow = (char*) pLocalBuffer;
          srcHi = srcLow+1;
          dst = (char*) key;
          while( uLen ) {
            *dst = *srcHi;
            dst++;
            *dst = *srcLow;
            dst++;
            srcLow += 2;
            srcHi += 2;
            uLen--;
          }
          *dst = 0;
          dst++;
          *dst = 0;
          res = NS_OK;
    }
  }
  UniFreeLocaleObject(locObj);

  return res;
}

