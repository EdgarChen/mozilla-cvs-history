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

#include "nsCocoaBrowserService.h"

#include "nsIWindowWatcher.h"
#include "nsIWebBrowserChrome.h"
#include "nsIEmbeddingSiteWindow.h"
#include "nsCRT.h"

nsAlertController* nsCocoaBrowserService::sController = nsnull;
nsCocoaBrowserService* nsCocoaBrowserService::sSingleton = nsnull;

// nsCocoaBrowserService implementation
nsCocoaBrowserService::nsCocoaBrowserService()
{
  NS_INIT_ISUPPORTS();
}

nsCocoaBrowserService::~nsCocoaBrowserService()
{
  TermEmbedding();
}

NS_IMPL_ISUPPORTS3(nsCocoaBrowserService,
                   nsIWindowCreator,
                   nsIPromptService,
                   nsIFactory)

nsresult
nsCocoaBrowserService::InitEmbedding()
{
  if (sSingleton) {
    return NS_OK;
  }

  // XXX Temporary hack to make sure we find the 
  // right executable directory
  NSBundle* mainBundle = [NSBundle mainBundle];
  NSString* path = [mainBundle bundlePath];
  NSMutableString* mutablePath = [NSMutableString stringWithString:path];
  [mutablePath appendString:@"/Contents/MacOS/"];
  const char* cstr = [mutablePath cString];
  setenv("MOZILLA_FIVE_HOME", cstr, 1);

  sSingleton = new nsCocoaBrowserService();
  if (!sSingleton) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  NS_ADDREF(sSingleton);

  nsresult rv = NS_InitEmbedding(nsnull, nsnull);
  if (NS_FAILED(rv)) {
    return rv;
  }

  // Register as the prompt service
#define NS_PROMPTSERVICE_CID \
     {0xa2112d6a, 0x0e28, 0x421f, {0xb4, 0x6a, 0x25, 0xc0, 0xb3, 0x8, 0xcb, 0xd0}}
  static NS_DEFINE_CID(kPromptServiceCID, NS_PROMPTSERVICE_CID);

  rv = nsComponentManager::RegisterFactory(kPromptServiceCID,
                                           "Prompt Service",
                                           "@mozilla.org/embedcomp/prompt-service;1",
                                           sSingleton,
                                           PR_TRUE); // replace existing
  if (NS_FAILED(rv)) {
    return rv;
  }

  // Register as the window creator
  nsCOMPtr<nsIWindowWatcher> watcher(do_GetService("@mozilla.org/embedcomp/window-watcher;1"));
  if (!watcher) {
    return NS_ERROR_FAILURE;
  }

  watcher->SetWindowCreator(sSingleton);

  return NS_OK;
}

void
nsCocoaBrowserService::TermEmbedding()
{
  NS_TermEmbedding();
}

#define NS_ALERT_NIB_NAME "alert"

nsAlertController* 
nsCocoaBrowserService::GetAlertController()
{
  if (!sController) {
    [NSBundle loadNibNamed:@NS_ALERT_NIB_NAME owner:nsnull];
  }
  return sController;
}

void
nsCocoaBrowserService::SetAlertController(nsAlertController* aController)
{
  // XXX When should the controller be released?
  sController = aController;
  [sController retain];
}

// nsIFactory implementation
NS_IMETHODIMP 
nsCocoaBrowserService::CreateInstance(nsISupports *aOuter, 
                                      const nsIID & aIID, 
                                      void **aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);

  return sSingleton->QueryInterface(aIID, aResult);
}

NS_IMETHODIMP 
nsCocoaBrowserService::LockFactory(PRBool lock)
{
  return NS_OK;
}

// nsIPromptService implementation
static NSWindow*
GetNSWindowForDOMWindow(nsIDOMWindow* window)
{
  nsCOMPtr<nsIWindowWatcher> watcher(do_GetService("@mozilla.org/embedcomp/window-watcher;1"));
  if (!watcher) {
    return nsnull;
  }
  
  nsCOMPtr<nsIWebBrowserChrome> chrome;
  watcher->GetChromeForWindow(window, getter_AddRefs(chrome));
  if (!chrome) {
    return nsnull;
  }

  nsCOMPtr<nsIEmbeddingSiteWindow> siteWindow(do_QueryInterface(chrome));
  if (!siteWindow) {
    return nsnull;
  }

  NSWindow* nswin;
  siteWindow->GetSiteWindow((void**)&nswin);

  return nswin;
}

// Implementation of nsIPromptService
NS_IMETHODIMP 
nsCocoaBrowserService::Alert(nsIDOMWindow *parent, 
                             const PRUnichar *dialogTitle, 
                             const PRUnichar *text)
{
  nsAlertController* controller = GetAlertController();
  if (!controller) {
    return NS_ERROR_FAILURE;
  }

  NSString* titleStr = [NSString stringWithCharacters:dialogTitle length:nsCRT::strlen(dialogTitle)];
  NSString* textStr = [NSString stringWithCharacters:text length:nsCRT::strlen(text)];
  NSWindow* window = GetNSWindowForDOMWindow(parent);

  [controller alert:window title:titleStr text:textStr];

  return NS_OK;
}

/* void alertCheck (in nsIDOMWindow parent, in wstring dialogTitle, in wstring text, in wstring checkMsg, inout boolean checkValue); */
NS_IMETHODIMP 
nsCocoaBrowserService::AlertCheck(nsIDOMWindow *parent, 
                                  const PRUnichar *dialogTitle, 
                                  const PRUnichar *text, 
                                  const PRUnichar *checkMsg, 
                                  PRBool *checkValue)
{
  nsAlertController* controller = GetAlertController();
  if (!controller) {
    return NS_ERROR_FAILURE;
  }

  NSString* titleStr = [NSString stringWithCharacters:dialogTitle length:nsCRT::strlen(dialogTitle)];
  NSString* textStr = [NSString stringWithCharacters:text length:nsCRT::strlen(text)];
  NSString* msgStr = [NSString stringWithCharacters:checkMsg length:nsCRT::strlen(checkMsg)];
  BOOL valueBool = *checkValue ? YES : NO;
  NSWindow* window = GetNSWindowForDOMWindow(parent);

  [controller alertCheck:window title:titleStr text:textStr checkMsg:msgStr checkValue:&valueBool];

  *checkValue = (valueBool == YES) ? PR_TRUE : PR_FALSE;
  
  return NS_OK;
}

/* boolean confirm (in nsIDOMWindow parent, in wstring dialogTitle, in wstring text); */
NS_IMETHODIMP 
nsCocoaBrowserService::Confirm(nsIDOMWindow *parent, 
                               const PRUnichar *dialogTitle, 
                               const PRUnichar *text, 
                               PRBool *_retval)
{
  nsAlertController* controller = GetAlertController();
  if (!controller) {
    return NS_ERROR_FAILURE;
  }

  NSString* titleStr = [NSString stringWithCharacters:dialogTitle length:nsCRT::strlen(dialogTitle)];
  NSString* textStr = [NSString stringWithCharacters:text length:nsCRT::strlen(text)];
  NSWindow* window = GetNSWindowForDOMWindow(parent);

  *_retval = (PRBool)[controller confirm:window title:titleStr text:textStr];
  
  return NS_OK;
}

/* boolean confirmCheck (in nsIDOMWindow parent, in wstring dialogTitle, in wstring text, in wstring checkMsg, inout boolean checkValue); */
NS_IMETHODIMP 
nsCocoaBrowserService::ConfirmCheck(nsIDOMWindow *parent, 
                                    const PRUnichar *dialogTitle, 
                                    const PRUnichar *text, 
                                    const PRUnichar *checkMsg, 
                                    PRBool *checkValue, PRBool *_retval)
{
  nsAlertController* controller = GetAlertController();
  if (!controller) {
    return NS_ERROR_FAILURE;
  }

  NSString* titleStr = [NSString stringWithCharacters:dialogTitle length:nsCRT::strlen(dialogTitle)];
  NSString* textStr = [NSString stringWithCharacters:text length:nsCRT::strlen(text)];
  NSString* msgStr = [NSString stringWithCharacters:checkMsg length:nsCRT::strlen(checkMsg)];
  BOOL valueBool = *checkValue ? YES : NO;
  NSWindow* window = GetNSWindowForDOMWindow(parent);

  *_retval = (PRBool)[controller confirmCheck:window title:titleStr text:textStr checkMsg:msgStr checkValue:&valueBool];

  *checkValue = (valueBool == YES) ? PR_TRUE : PR_FALSE;
  
  return NS_OK;
}

/* void confirmEx (in nsIDOMWindow parent, in wstring dialogTitle, in wstring text, in unsigned long buttonFlags, in wstring button0Title, in wstring button1Title, in wstring button2Title, in wstring checkMsg, inout boolean checkValue, out PRInt32 buttonPressed); */
NS_IMETHODIMP 
nsCocoaBrowserService::ConfirmEx(nsIDOMWindow *parent, 
                                 const PRUnichar *dialogTitle, 
                                 const PRUnichar *text, 
                                 PRUint32 buttonFlags, 
                                 const PRUnichar *button0Title, 
                                 const PRUnichar *button1Title, 
                                 const PRUnichar *button2Title, 
                                 const PRUnichar *checkMsg, 
                                 PRBool *checkValue, PRInt32 *buttonPressed)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* boolean prompt (in nsIDOMWindow parent, in wstring dialogTitle, in wstring text, inout wstring value, in wstring checkMsg, inout boolean checkValue); */
NS_IMETHODIMP 
nsCocoaBrowserService::Prompt(nsIDOMWindow *parent, 
                              const PRUnichar *dialogTitle, 
                              const PRUnichar *text, 
                              PRUnichar **value, 
                              const PRUnichar *checkMsg, 
                              PRBool *checkValue, 
                              PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* boolean promptUsernameAndPassword (in nsIDOMWindow parent, in wstring dialogTitle, in wstring text, inout wstring username, inout wstring password, in wstring checkMsg, inout boolean checkValue); */
NS_IMETHODIMP 
nsCocoaBrowserService::PromptUsernameAndPassword(nsIDOMWindow *parent, 
                                                 const PRUnichar *dialogTitle, 
                                                 const PRUnichar *text, 
                                                 PRUnichar **username, 
                                                 PRUnichar **password, 
                                                 const PRUnichar *checkMsg, 
                                                 PRBool *checkValue, 
                                                 PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* boolean promptPassword (in nsIDOMWindow parent, in wstring dialogTitle, in wstring text, inout wstring password, in wstring checkMsg, inout boolean checkValue); */
NS_IMETHODIMP 
nsCocoaBrowserService::PromptPassword(nsIDOMWindow *parent, 
                                      const PRUnichar *dialogTitle, 
                                      const PRUnichar *text, 
                                      PRUnichar **password, 
                                      const PRUnichar *checkMsg, 
                                      PRBool *checkValue, 
                                      PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* boolean select (in nsIDOMWindow parent, in wstring dialogTitle, in wstring text, in PRUint32 count, [array, size_is (count)] in wstring selectList, out long outSelection); */
NS_IMETHODIMP 
nsCocoaBrowserService::Select(nsIDOMWindow *parent, 
                              const PRUnichar *dialogTitle, 
                              const PRUnichar *text, 
                              PRUint32 count, 
                              const PRUnichar **selectList, 
                              PRInt32 *outSelection, 
                              PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

// Implementation of nsIWindowCreator
/* nsIWebBrowserChrome createChromeWindow (in nsIWebBrowserChrome parent, in PRUint32 chromeFlags); */
NS_IMETHODIMP 
nsCocoaBrowserService::CreateChromeWindow(nsIWebBrowserChrome *parent, 
                                          PRUint32 chromeFlags, 
                                          nsIWebBrowserChrome **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

