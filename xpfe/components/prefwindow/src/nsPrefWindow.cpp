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
#include "nsIPrefWindow.h"

#include "nsIAppShellComponentImpl.h"

#if 0
#include "nsCOMPtr.h"
#include "pratom.h"
#include "nsIFactory.h"
#include "nsIServiceManager.h"
#include "nsIAppShellService.h"
#include "prprf.h"

#include "nsIXULWindowCallbacks.h"
#include "nsIDocumentObserver.h"
#include "nsString.h"
#include "nsIURL.h"
#include "nsIWebShellWindow.h"
#include "nsIContent.h"
#include "nsINameSpaceManager.h"
#include "nsIContentViewer.h"
#include "nsIDocumentViewer.h"
#include "nsIWebShell.h"
#include "nsIDocument.h"
#include "nsIDOMXULDocument.h"
#include "nsIDOMElement.h"
#include "nsIStreamTransfer.h"
#endif

// You'll have to generate your own CID for your component...
// {CFC599F0-04CA-11d3-8068-00600811A9C3}
#define NS_PREFWINDOW_CID \
    { 0xcfc599f0, 0x4ca, 0x11d3, { 0x80, 0x68, 0x0, 0x60, 0x8, 0x11, 0xa9, 0xc3 } }

// Implementation of the sample app shell component interface.
class nsPrefWindow : public nsIPrefWindow,
                                  public nsAppShellComponentImpl {
public:
    NS_DEFINE_STATIC_CID_ACCESSOR( NS_PREFWINDOW_CID );

    // ctor/dtor
    nsPrefWindow() {
        NS_INIT_REFCNT();
    }
    virtual ~nsPrefWindow() {
    }

    // This class implements the nsISupports interface functions.
    NS_DECL_ISUPPORTS

    // This class implements the nsIAppShellComponent interface functions.
    NS_DECL_IAPPSHELLCOMPONENT

    // This class implements the nsIPrefWindow interface functions.
    NS_DECL_IPREFWINDOW

private:
    // Data members and implemention functions go here.

    // Objects of this class are counted to manage library unloading...
    nsInstanceCounter instanceCounter;
}; // nsPrefWindow


NS_IMETHODIMP
nsPrefWindow::DoDialogTests() {
    nsresult rv = NS_OK;
    DEBUG_PRINTF( PR_STDOUT, "nsPrefWindow::DoDialogTests called\n" );
    return rv;
}

// Generate base nsIAppShellComponent implementation.
NS_IMPL_IAPPSHELLCOMPONENT( nsPrefWindow,
                            nsIPrefWindow,
                            NS_IPREFWINDOW_PROGID )
