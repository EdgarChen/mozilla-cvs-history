/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the Private language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */
 
#include "nsRepository.h"
#include "nsShellInstance.h"
#include "nsApplicationManager.h"
#include "nsGfxCIID.h"
#include "nsxpfcCIID.h"
#include "nsIAppShell.h"
#include "nspr.h"

#include "Xm/Xm.h"
#include "Xm/MainW.h"
#include "Xm/Frame.h"
#include "Xm/XmStrDefs.h"
#include "Xm/DrawingA.h"

#define XPFC_DLL "libxpfc10.so"

extern nsIID kIXPCOMApplicationShellCID ;

static NS_DEFINE_IID(kIApplicationShellIID, NS_IAPPLICATIONSHELL_IID);
static NS_DEFINE_IID(kCApplicationShellIID, NS_IAPPLICATIONSHELL_CID);

static NS_DEFINE_IID(kIShellInstanceIID, NS_IXPFC_SHELL_INSTANCE_IID);
static NS_DEFINE_IID(kCShellInstanceCID, NS_XPFC_SHELL_INSTANCE_CID);
static NS_DEFINE_IID(kCMenuBarCID, NS_MENUBAR_CID);
static NS_DEFINE_IID(kCMenuContainerCID, NS_MENUCONTAINER_CID);
static NS_DEFINE_IID(kCMenuItemCID, NS_MENUITEM_CID);
static NS_DEFINE_IID(kCMenuManagerCID, NS_MENU_MANAGER_CID);
static NS_DEFINE_IID(kIAppShellIID, NS_IAPPSHELL_IID);
static NS_DEFINE_IID(kCXPFCToolbarCID, NS_XPFC_TOOLBAR_CID);
static NS_DEFINE_IID(kCXPFCDialogCID, NS_XPFC_DIALOG_CID);
static NS_DEFINE_IID(kCXPFCButtonCID, NS_XPFC_BUTTON_CID);
static NS_DEFINE_IID(kCXPFCTextWidgetCID, NS_XPFC_TEXTWIDGET_CID);
static NS_DEFINE_IID(kCXPFCTabWidgetCID, NS_XPFC_TABWIDGET_CID);
static NS_DEFINE_IID(kCToolbarManagerCID, NS_TOOLBAR_MANAGER_CID);
static NS_DEFINE_IID(kCVectorCID, NS_VECTOR_CID);
static NS_DEFINE_IID(kCVectorIteratorCID, NS_VECTOR_ITERATOR_CID);
static NS_DEFINE_IID(kCstackCID, NS_STACK_CID);
static NS_DEFINE_IID(kCStreamManagerCID, NS_STREAM_MANAGER_CID);

XtAppContext app_context ;
Widget topLevel;
extern XtAppContext gAppContext ;

void main(int argc, char **argv)
{
  nsresult result = NS_OK ;

  nsIShellInstance * pShellInstance ;
  nsIApplicationShell * pApplicationShell ;

  XtSetLanguageProc(NULL, NULL, NULL);

  topLevel = XtVaAppInitialize(&app_context, "Shell", NULL, 0, &argc, argv, NULL, NULL);

  gAppContext = app_context;

    PR_Init(PR_USER_THREAD, PR_PRIORITY_NORMAL, 0);
    PR_STDIO_INIT();

    // Let get a ShellInstance for this Application instance
    nsRepository::RegisterFactory(kCShellInstanceCID, XPFC_DLL, PR_FALSE, PR_FALSE);
    nsRepository::RegisterFactory(kCMenuBarCID, XPFC_DLL, PR_FALSE, PR_FALSE);
    nsRepository::RegisterFactory(kCMenuContainerCID, XPFC_DLL, PR_FALSE, PR_FALSE);
    nsRepository::RegisterFactory(kCMenuItemCID, XPFC_DLL, PR_FALSE, PR_FALSE);
    nsRepository::RegisterFactory(kCMenuManagerCID, XPFC_DLL, PR_FALSE, PR_FALSE);
    nsRepository::RegisterFactory(kCXPFCToolbarCID, XPFC_DLL, PR_FALSE, PR_FALSE);
    nsRepository::RegisterFactory(kCXPFCDialogCID, XPFC_DLL, PR_FALSE, PR_FALSE);
    nsRepository::RegisterFactory(kCXPFCButtonCID, XPFC_DLL, PR_FALSE, PR_FALSE);
    nsRepository::RegisterFactory(kCXPFCTextWidgetCID, XPFC_DLL, PR_FALSE, PR_FALSE);
    nsRepository::RegisterFactory(kCXPFCTabWidgetCID, XPFC_DLL, PR_FALSE, PR_FALSE);
    nsRepository::RegisterFactory(kCToolbarManagerCID, XPFC_DLL, PR_FALSE, PR_FALSE);
    nsRepository::RegisterFactory(kCStreamManagerCID, XPFC_DLL, PR_FALSE, PR_FALSE);
    nsRepository::RegisterFactory(kCVectorCID, XPFC_DLL, PR_FALSE, PR_FALSE);
    nsRepository::RegisterFactory(kCVectorIteratorCID, XPFC_DLL, PR_FALSE, PR_FALSE);
    nsRepository::RegisterFactory(kCstackCID, XPFC_DLL, PR_FALSE, PR_FALSE);

	result = nsRepository::CreateInstance(kCShellInstanceCID,
										  NULL,
										  kIShellInstanceIID,
										  (void **) &pShellInstance) ;

	if (result != NS_OK)
		return result ;

    // Let's instantiate the Application's Shell
    NS_RegisterApplicationShellFactory() ;

	result = nsRepository::CreateInstance(kIXPCOMApplicationShellCID,
										  NULL,
										  kIXPCOMApplicationShellCID,
										  (void **) &pApplicationShell) ;
		
	if (result != NS_OK)
		return result ;


  // Let the the State know who it's Application Instance is
    pShellInstance->SetNativeInstance((nsNativeApplicationInstance) topLevel);
    pShellInstance->SetApplicationShell(pApplicationShell);

    // Tell the application manager to store away the association so the
    // Application can look up its State
    nsApplicationManager::SetShellAssociation(pApplicationShell, pShellInstance);

    //  Initialize the system

    pShellInstance->Init();
    pApplicationShell->Init();

    // Now, let actually start dispatching events.
    nsIAppShell * app_shell = nsnull;

    result = pApplicationShell->QueryInterface(kIAppShellIID,(void**)&app_shell);

    if (result == NS_OK)
    {
	  result = app_shell->Run();
      NS_RELEASE(app_shell);
    }


    // We're done, clean up
  nsApplicationManager::DeleteShellAssociation(pApplicationShell, pShellInstance);
  PR_Cleanup();

  // book out of here
  return ;
}















