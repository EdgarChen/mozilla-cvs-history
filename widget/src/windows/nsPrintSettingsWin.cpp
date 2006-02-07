/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
#include "nsPrintSettingsWin.h"
#include "nsCRT.h"

NS_IMPL_ISUPPORTS_INHERITED1(nsPrintSettingsWin, 
                             nsPrintSettings, 
                             nsIPrintSettingsWin)

/** ---------------------------------------------------
 *  See documentation in nsPrintSettingsWin.h
 *	@update 
 */
nsPrintSettingsWin::nsPrintSettingsWin() :
  nsPrintSettings(),
  mDeviceName(nsnull),
  mDriverName(nsnull),
  mDevMode(nsnull)
{

}

/** ---------------------------------------------------
 *  See documentation in nsPrintSettingsWin.h
 *	@update 
 */
nsPrintSettingsWin::nsPrintSettingsWin(const nsPrintSettingsWin& aPS) :
  mDeviceName(nsnull),
  mDriverName(nsnull),
  mDevMode(nsnull)
{
  *this = aPS;
}

/** ---------------------------------------------------
 *  See documentation in nsPrintSettingsWin.h
 *	@update 
 */
nsPrintSettingsWin::~nsPrintSettingsWin()
{
  if (mDeviceName) nsMemory::Free(mDeviceName);
  if (mDriverName) nsMemory::Free(mDriverName);
  if (mDevMode) free(mDevMode);
}

/* [noscript] attribute charPtr deviceName; */
NS_IMETHODIMP nsPrintSettingsWin::SetDeviceName(char * aDeviceName)
{
  if (mDeviceName) {
    nsMemory::Free(mDeviceName);
  }
  mDeviceName = aDeviceName?nsCRT::strdup(aDeviceName):nsnull;
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettingsWin::GetDeviceName(char * *aDeviceName)
{
  NS_ENSURE_ARG_POINTER(aDeviceName);
  *aDeviceName = mDeviceName?nsCRT::strdup(mDeviceName):nsnull;
  return NS_OK;
}

/* [noscript] attribute charPtr driverName; */
NS_IMETHODIMP nsPrintSettingsWin::SetDriverName(char * aDriverName)
{
  if (mDriverName) {
    nsMemory::Free(mDriverName);
  }
  mDriverName = aDriverName?nsCRT::strdup(aDriverName):nsnull;
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettingsWin::GetDriverName(char * *aDriverName)
{
  NS_ENSURE_ARG_POINTER(aDriverName);
  *aDriverName = mDriverName?nsCRT::strdup(mDriverName):nsnull;
  return NS_OK;
}

/* [noscript] attribute nsDevMode devMode; */
NS_IMETHODIMP nsPrintSettingsWin::GetDevMode(DEVMODE * *aDevMode)
{
  NS_ENSURE_ARG_POINTER(aDevMode);

  if (mDevMode) {
    size_t size = sizeof(*mDevMode);
    *aDevMode = (LPDEVMODE)malloc(size);
    memcpy(*aDevMode, mDevMode, size);
  } else {
    *aDevMode = nsnull;
  }
  return NS_OK;
}

NS_IMETHODIMP nsPrintSettingsWin::SetDevMode(DEVMODE * aDevMode)
{
  if (mDevMode) {
    free(mDevMode);
    mDevMode = NULL;
  }

  if (aDevMode) {
    size_t size = sizeof(*aDevMode);
    mDevMode = (LPDEVMODE)malloc(size);
    memcpy(mDevMode, aDevMode, size);
  }
  return NS_OK;
}

//-------------------------------------------
nsresult 
nsPrintSettingsWin::_Clone(nsIPrintSettings **_retval)
{
  nsPrintSettingsWin* printSettings = new nsPrintSettingsWin(*this);
  return printSettings->QueryInterface(NS_GET_IID(nsIPrintSettings), (void**)_retval); // ref counts
}

//-------------------------------------------
nsPrintSettingsWin& nsPrintSettingsWin::operator=(const nsPrintSettingsWin& rhs)
{
  if (this == &rhs) {
    return *this;
  }

  ((nsPrintSettings&) *this) = rhs;

  if (mDeviceName) {
    nsCRT::free(mDeviceName);
  }

  if (mDriverName) {
    nsCRT::free(mDriverName);
  }

  // Use free because we used the native malloc to create the memory
  if (mDevMode) {
    free(mDevMode);
  }

  mDeviceName = rhs.mDeviceName?nsCRT::strdup(rhs.mDeviceName):nsnull;
  mDriverName = rhs.mDriverName?nsCRT::strdup(rhs.mDriverName):nsnull;

  if (rhs.mDevMode) {
    size_t size = sizeof(*rhs.mDevMode);
    mDevMode = (LPDEVMODE)malloc(size);
    memcpy(mDevMode, rhs.mDevMode, size);
  } else {
    mDevMode = nsnull;
  }

  return *this;
}

//-------------------------------------------
/* void assign (in nsIPrintSettings aPS); */
nsresult 
nsPrintSettingsWin::_Assign(nsIPrintSettings *aPS)
{
  nsPrintSettingsWin *psWin = NS_STATIC_CAST(nsPrintSettingsWin*, aPS);
  *this = *psWin;
  return NS_OK;
}

//----------------------------------------------------------------------
// Testing of assign and clone
// This define turns on the testing module below
// so at start up it writes and reads the prefs.
#ifdef DEBUG_rodsX
#include "nsIPrintOptions.h"
#include "nsIServiceManager.h"
class Tester {
public:
  Tester();
};
Tester::Tester()
{
  nsCOMPtr<nsIPrintSettings> ps;
  nsresult rv;
  nsCOMPtr<nsIPrintOptions> printService = do_GetService("@mozilla.org/gfx/printsettings-service;1", &rv);
  if (NS_SUCCEEDED(rv)) {
    rv = printService->CreatePrintSettings(getter_AddRefs(ps));
  }

  if (ps) {
    ps->SetPrintOptions(nsIPrintSettings::kPrintOddPages,  PR_TRUE);
    ps->SetPrintOptions(nsIPrintSettings::kPrintEvenPages,  PR_FALSE);
    ps->SetMarginTop(1.0);
    ps->SetMarginLeft(1.0);
    ps->SetMarginBottom(1.0);
    ps->SetMarginRight(1.0);
    ps->SetScaling(0.5);
    ps->SetPrintBGColors(PR_TRUE);
    ps->SetPrintBGImages(PR_TRUE);
    ps->SetPrintRange(15);
    ps->SetHeaderStrLeft(NS_ConvertUTF8toUCS2("Left").get());
    ps->SetHeaderStrCenter(NS_ConvertUTF8toUCS2("Center").get());
    ps->SetHeaderStrRight(NS_ConvertUTF8toUCS2("Right").get());
    ps->SetFooterStrLeft(NS_ConvertUTF8toUCS2("Left").get());
    ps->SetFooterStrCenter(NS_ConvertUTF8toUCS2("Center").get());
    ps->SetFooterStrRight(NS_ConvertUTF8toUCS2("Right").get());
    ps->SetPaperName(NS_ConvertUTF8toUCS2("Paper Name").get());
    ps->SetPaperSizeType(10);
    ps->SetPaperData(1);
    ps->SetPaperWidth(100.0);
    ps->SetPaperHeight(50.0);
    ps->SetPaperSizeUnit(nsIPrintSettings::kPaperSizeMillimeters);
    ps->SetPrintReversed(PR_TRUE);
    ps->SetPrintInColor(PR_TRUE);
    ps->SetPaperSize(5);
    ps->SetOrientation(nsIPrintSettings::kLandscapeOrientation);
    ps->SetPrintCommand(NS_ConvertUTF8toUCS2("Command").get());
    ps->SetNumCopies(2);
    ps->SetPrinterName(NS_ConvertUTF8toUCS2("Printer Name").get());
    ps->SetPrintToFile(PR_TRUE);
    ps->SetToFileName(NS_ConvertUTF8toUCS2("File Name").get());
    ps->SetPrintPageDelay(1000);

    nsCOMPtr<nsIPrintSettings> ps2;
    if (NS_SUCCEEDED(rv)) {
      rv = printService->CreatePrintSettings(getter_AddRefs(ps2));
    }

    ps2->Assign(ps);

    nsCOMPtr<nsIPrintSettings> psClone;
    ps2->Clone(getter_AddRefs(psClone));

  }

}
Tester gTester;
#endif

