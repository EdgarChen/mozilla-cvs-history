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
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */

#include <locale.h>
#include "plstr.h"
#include "nsIServiceManager.h"
#include "nsICharsetConverterManager.h"
#include "nsDateTimeFormatUnix.h"
#include "nsIComponentManager.h"
#include "nsLocaleCID.h"
#include "nsILocaleService.h"
#include "nsIPlatformCharset.h"
#include "nsIPosixLocale.h"
#include "nsCOMPtr.h"
#include "nsCRT.h"

static NS_DEFINE_IID(kIDateTimeFormatIID, NS_IDATETIMEFORMAT_IID);
static NS_DEFINE_CID(kPosixLocaleFactoryCID, NS_POSIXLOCALEFACTORY_CID);
static NS_DEFINE_CID(kCharsetConverterManagerCID, NS_ICHARSETCONVERTERMANAGER_CID);
static NS_DEFINE_IID(kICharsetConverterManagerIID, NS_ICHARSETCONVERTERMANAGER_IID);
static NS_DEFINE_CID(kLocaleServiceCID, NS_LOCALESERVICE_CID); 
static NS_DEFINE_CID(kPlatformCharsetCID, NS_PLATFORMCHARSET_CID);

NS_IMPL_THREADSAFE_ISUPPORTS(nsDateTimeFormatUnix, kIDateTimeFormatIID);

// init this interface to a specified locale
nsresult nsDateTimeFormatUnix::Initialize(nsILocale* locale)
{
  PRUnichar *aLocaleUnichar = NULL;
  nsString aCategory;
  aCategory.AssignWithConversion("NSILOCALE_TIME");
  nsresult res = NS_OK;

  // use cached info if match with stored locale
  if (NULL == locale) {
    if (mLocale.Length() && mLocale.EqualsIgnoreCase(mAppLocale)) {
      return NS_OK;
    }
  }
  else {
    res = locale->GetCategory(aCategory.GetUnicode(), &aLocaleUnichar);
    if (NS_SUCCEEDED(res) && NULL != aLocaleUnichar) {
      if (mLocale.Length() && mLocale.EqualsIgnoreCase(aLocaleUnichar)) {
        nsMemory::Free(aLocaleUnichar);
        return NS_OK;
      }
      nsMemory::Free(aLocaleUnichar);
    }
  }

  mCharset.AssignWithConversion("ISO-8859-1");
  PL_strncpy(mPlatformLocale, "en_US", kPlatformLocaleLength+1);

  // get locale name string, use app default if no locale specified
  if (NULL == locale) {
    NS_WITH_SERVICE(nsILocaleService, localeService, kLocaleServiceCID, &res);
    if (NS_SUCCEEDED(res)) {
      nsILocale *appLocale;
      res = localeService->GetApplicationLocale(&appLocale);
      if (NS_SUCCEEDED(res)) {
        res = appLocale->GetCategory(aCategory.GetUnicode(), &aLocaleUnichar);
        if (NS_SUCCEEDED(res) && NULL != aLocaleUnichar) {
          mAppLocale = aLocaleUnichar; // cache app locale name
        }
        appLocale->Release();
      }
    }
  }
  else {
    res = locale->GetCategory(aCategory.GetUnicode(), &aLocaleUnichar);
  }

  if (NS_SUCCEEDED(res) && NULL != aLocaleUnichar) {
    mLocale = aLocaleUnichar; // cache locale name
    nsMemory::Free(aLocaleUnichar);

    nsCOMPtr <nsIPosixLocale> posixLocale = do_GetService(kPosixLocaleFactoryCID, &res);
    if (NS_SUCCEEDED(res)) {
      res = posixLocale->GetPlatformLocale(&mLocale, mPlatformLocale, kPlatformLocaleLength+1);
    }

    nsCOMPtr <nsIPlatformCharset> platformCharset = do_GetService(NS_PLATFORMCHARSET_PROGID, &res);
    if (NS_SUCCEEDED(res)) {
      PRUnichar* mappedCharset = NULL;
      res = platformCharset->GetDefaultCharsetForLocale(mLocale.GetUnicode(), &mappedCharset);
      if (NS_SUCCEEDED(res) && mappedCharset) {
        mCharset = mappedCharset;
        nsMemory::Free(mappedCharset);
      }
    }
  }

  mLocalePreferred24hour = LocalePreferred24hour();

  return res;
}

PRBool nsDateTimeFormatUnix::LocalePreferred24hour()
{
  char str[100];
  time_t tt;
  struct tm *tmc;
  int i;

  tt = time((time_t)NULL);
  tmc = localtime(&tt);

  tmc->tm_hour=22;    // put the test sample hour to 22:00 which is 10PM
  tmc->tm_min=0;      // set the min & sec other number than '2'
  tmc->tm_sec=0;

  char *temp = setlocale(LC_TIME, mPlatformLocale);
  strftime(str, (size_t)99, "%X", (struct tm *)tmc);
  (void) setlocale(LC_TIME, temp);

  for (i=0; str[i]; i++)
    if (str[i] == '2') {    // if there is any '2', that locale use 0-23 time format
        return PR_TRUE;
    }

  return PR_FALSE;
}

nsresult nsDateTimeFormatUnix::FormatTime(nsILocale* locale, 
                                      const nsDateFormatSelector  dateFormatSelector, 
                                      const nsTimeFormatSelector timeFormatSelector, 
                                      const time_t  timetTime, 
                                      nsString& stringOut) 
{
  return FormatTMTime(locale, dateFormatSelector, timeFormatSelector, localtime(&timetTime), stringOut);
}

// performs a locale sensitive date formatting operation on the struct tm parameter
nsresult nsDateTimeFormatUnix::FormatTMTime(nsILocale* locale, 
                                        const nsDateFormatSelector  dateFormatSelector, 
                                        const nsTimeFormatSelector timeFormatSelector, 
                                        const struct tm*  tmTime, 
                                        nsString& stringOut) 
{
#define NSDATETIME_FORMAT_BUFFER_LEN  80
  char strOut[NSDATETIME_FORMAT_BUFFER_LEN];
  char fmtD[NSDATETIME_FORMAT_BUFFER_LEN], fmtT[NSDATETIME_FORMAT_BUFFER_LEN];
  nsresult res;

  
  // set up locale data
  (void) Initialize(locale);

  // set date format
  switch (dateFormatSelector) {
    case kDateFormatNone:
      PL_strncpy(fmtD, "", NSDATETIME_FORMAT_BUFFER_LEN);
      break; 
    case kDateFormatLong:
      PL_strncpy(fmtD, "%c", NSDATETIME_FORMAT_BUFFER_LEN);
      break; 
    case kDateFormatShort:
      PL_strncpy(fmtD, "%x", NSDATETIME_FORMAT_BUFFER_LEN);
      break; 
    case kDateFormatYearMonth:
      PL_strncpy(fmtD, "%y/%m", NSDATETIME_FORMAT_BUFFER_LEN);
      break; 
    case kDateFormatWeekday:
      PL_strncpy(fmtD, "%a", NSDATETIME_FORMAT_BUFFER_LEN);
      break;
    default:
      PL_strncpy(fmtD, "", NSDATETIME_FORMAT_BUFFER_LEN); 
  }

  // set time format
  switch (timeFormatSelector) {
    case kTimeFormatNone:
      PL_strncpy(fmtT, "", NSDATETIME_FORMAT_BUFFER_LEN); 
      break;
    case kTimeFormatSeconds:
      PL_strncpy(fmtT, 
                 mLocalePreferred24hour ? "%H:%M:%S" : "%I:%M:%S %p", 
                 NSDATETIME_FORMAT_BUFFER_LEN);
      break;
    case kTimeFormatNoSeconds:
      PL_strncpy(fmtT, 
                 mLocalePreferred24hour ? "%H:%M" : "%I:%M %p", 
                 NSDATETIME_FORMAT_BUFFER_LEN);
      break;
    case kTimeFormatSecondsForce24Hour:
      PL_strncpy(fmtT, "%H:%M:%S", NSDATETIME_FORMAT_BUFFER_LEN);
      break;
    case kTimeFormatNoSecondsForce24Hour:
      PL_strncpy(fmtT, "%H:%M", NSDATETIME_FORMAT_BUFFER_LEN);
      break;
    default:
      PL_strncpy(fmtT, "", NSDATETIME_FORMAT_BUFFER_LEN); 
  }

  // generate data/time string
  char *old_locale = setlocale(LC_TIME, NULL);
  (void) setlocale(LC_TIME, mPlatformLocale);
  if (PL_strlen(fmtD) && PL_strlen(fmtT)) {
    PL_strncat(fmtD, " ", NSDATETIME_FORMAT_BUFFER_LEN);
    PL_strncat(fmtD, fmtT, NSDATETIME_FORMAT_BUFFER_LEN);
    strftime(strOut, NSDATETIME_FORMAT_BUFFER_LEN, fmtD, tmTime);
  }
  else if (PL_strlen(fmtD) && !PL_strlen(fmtT)) {
    strftime(strOut, NSDATETIME_FORMAT_BUFFER_LEN, fmtD, tmTime);
  }
  else if (!PL_strlen(fmtD) && PL_strlen(fmtT)) {
    strftime(strOut, NSDATETIME_FORMAT_BUFFER_LEN, fmtT, tmTime);
  }
  else {
    PL_strncpy(strOut, "", NSDATETIME_FORMAT_BUFFER_LEN);
  }
  (void) setlocale(LC_TIME, old_locale);

  // convert result to unicode
  NS_WITH_SERVICE(nsICharsetConverterManager, ccm, kCharsetConverterManagerCID, &res);
  if(NS_SUCCEEDED(res) && ccm) {
    nsCOMPtr<nsIUnicodeDecoder> decoder;
    res = ccm->GetUnicodeDecoder(&mCharset, getter_AddRefs(decoder));
    if (NS_SUCCEEDED(res) && decoder) {
      PRInt32 unicharLength = 0;
      PRInt32 srcLength = (PRInt32) PL_strlen(strOut);
      res = decoder->GetMaxLength(strOut, srcLength, &unicharLength);
      PRUnichar *unichars = new PRUnichar [ unicharLength ];
  
      if (nsnull != unichars) {
        res = decoder->Convert(strOut, &srcLength,
                               unichars, &unicharLength);
        if (NS_SUCCEEDED(res)) {
          stringOut.Assign(unichars, unicharLength);
        }
      }
      delete [] unichars;
    }    
  }
  
  return res;
}

// performs a locale sensitive date formatting operation on the PRTime parameter
nsresult nsDateTimeFormatUnix::FormatPRTime(nsILocale* locale, 
                                           const nsDateFormatSelector  dateFormatSelector, 
                                           const nsTimeFormatSelector timeFormatSelector, 
                                           const PRTime  prTime, 
                                           nsString& stringOut)
{
  PRExplodedTime explodedTime;
  PR_ExplodeTime(prTime, PR_LocalTimeParameters, &explodedTime);

  return FormatPRExplodedTime(locale, dateFormatSelector, timeFormatSelector, &explodedTime, stringOut);
}

// performs a locale sensitive date formatting operation on the PRExplodedTime parameter
nsresult nsDateTimeFormatUnix::FormatPRExplodedTime(nsILocale* locale, 
                                                   const nsDateFormatSelector  dateFormatSelector, 
                                                   const nsTimeFormatSelector timeFormatSelector, 
                                                   const PRExplodedTime*  explodedTime, 
                                                   nsString& stringOut)
{
  struct tm  tmTime;
  /* be safe and set all members of struct tm to zero
   *
   * there are other fields in the tm struct that we aren't setting
   * (tm_isdst, tm_gmtoff, tm_zone, should we set these?) and since
   * tmTime is on the stack, it may be filled with garbage, but
   * the garbage may vary.  (this may explain why some saw bug #10412, and
   * others did not.
   *
   * when tmTime is passed to strftime() with garbage bad things may happen. 
   * see bug #10412
   */
  nsCRT::memset( &tmTime, 0, sizeof(tmTime) );

  tmTime.tm_yday = explodedTime->tm_yday;
  tmTime.tm_wday = explodedTime->tm_wday;
  tmTime.tm_year = explodedTime->tm_year;
  tmTime.tm_year -= 1900;
  tmTime.tm_mon = explodedTime->tm_month;
  tmTime.tm_mday = explodedTime->tm_mday;
  tmTime.tm_hour = explodedTime->tm_hour;
  tmTime.tm_min = explodedTime->tm_min;
  tmTime.tm_sec = explodedTime->tm_sec;

  return FormatTMTime(locale, dateFormatSelector, timeFormatSelector, &tmTime, stringOut);
}

