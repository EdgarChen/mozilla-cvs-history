/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is the Mozilla browser.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications, Inc.  Portions created by Netscape are
 * Copyright (C) 1999, Mozilla.  All Rights Reserved.
 * 
 * Author:
 *   Conrad Carlen <ccarlen@netscape.com>
 */

#ifndef nsWebBrowserFindImpl_h__
#define nsWebBrowserFindImpl_h__

#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsWeakReference.h"

#include "nsIWebBrowserFind.h"

#include "nsIFindAndReplace.h"


// {57cf9383-3405-11d5-be5b-aa20fa2cf37c}
#define NS_WEB_BROWSER_FIND_CID \
 {0x57cf9383, 0x3405, 0x11d5, {0xbe, 0x5b, 0xaa, 0x20, 0xfa, 0x2c, 0xf3, 0x7c}}

#define NS_WEB_BROWSER_FIND_CONTRACTID \
 "@mozilla.org/embedcomp/find;1"


class nsIDOMWindow;

class nsIDocShell;
class nsIDocShellTreeItem;

class nsITextServicesDocument;

//*****************************************************************************
// class nsWebBrowserFind
//*****************************************************************************   

class nsWebBrowserFind  : public nsIWebBrowserFind,
                          public nsIWebBrowserFindInFrames
{
public:
                nsWebBrowserFind();
    virtual     ~nsWebBrowserFind();
    
    // nsISupports
    NS_DECL_ISUPPORTS
    
    // nsIWebBrowserFind
    NS_DECL_NSIWEBBROWSERFIND
                
    // nsIWebBrowserFindInFrames
    NS_DECL_NSIWEBBROWSERFINDINFRAMES


protected:
     
    PRBool      CanFindNext()
                { return mSearchString.Length() != 0; }

    nsresult    SearchInFrame(nsIDOMWindow* aWindow, PRBool* didFind);

    nsresult    OnStartSearchFrame(nsIDOMWindow *aWindow);
    nsresult    OnEndSearchFrame(nsIDOMWindow *aWindow);

    nsresult    ClearFrameSelection(nsIDOMWindow *aWindow);
    
    nsresult    OnFind(nsIDOMWindow *aFoundWindow);
    
    nsresult    MakeTSDocument(nsIDOMWindow* aWindow, nsITextServicesDocument** aDoc);

    nsresult    GetDocShellFromWindow(nsIDOMWindow *inWindow, nsIDocShell** outDocShell);
    
protected:

    nsString        mSearchString;
    
    PRPackedBool    mFindBackwards;
    PRPackedBool    mWrapFind;
    PRPackedBool    mEntireWord;
    PRPackedBool    mMatchCase;
    
    PRPackedBool    mSearchSubFrames;
    PRPackedBool    mSearchParentFrames;
    
    nsWeakPtr       mCurrentSearchFrame;    // who knows if windows can go away during our lifetime, hence weak
    nsWeakPtr       mRootSearchFrame;       // who knows if windows can go away during our lifetime, hence weak
    
    nsCOMPtr<nsIFindAndReplace> mTSFind;
};

#endif
