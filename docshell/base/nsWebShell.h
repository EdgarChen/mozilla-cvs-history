/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * ***** BEGIN LICENSE BLOCK *****
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
 * The Original Code is the Mozilla browser.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications, Inc.
 * Portions created by the Initial Developer are Copyright (C) 1999
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
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
#ifndef webshell____h
#define webshell____h

#include "nsError.h"
#include "nsIWebShellServices.h"
#include "nsIWebShell.h"
#include "nsILinkHandler.h"
#include "nsIClipboardCommands.h"
#include "nsDocShell.h"
#include "nsICommandManager.h"
#include "nsIIOService.h"
#include "nsCRT.h"
#include "nsIDocumentLoader.h"

class nsIEventQueue;
class nsIController;
struct PRThread;
struct OnLinkClickEvent;

typedef enum {
    eCharsetReloadInit,
    eCharsetReloadRequested,
    eCharsetReloadStopOrigional
} eCharsetReloadState;

#define NS_ERROR_WEBSHELL_REQUEST_REJECTED  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GENERAL,1001)

class nsWebShell : public nsDocShell,
                   public nsIWebShell,
                   public nsIWebShellContainer,
                   public nsIWebShellServices,
                   public nsILinkHandler,
                   public nsIClipboardCommands
{
public:
    nsWebShell();
    virtual ~nsWebShell();

    NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIINTERFACEREQUESTOR
    NS_DECL_NSICLIPBOARDCOMMANDS
    NS_DECL_NSIWEBSHELLSERVICES

    // nsIWebShell
    NS_IMETHOD SetContainer(nsIWebShellContainer* aContainer);
    NS_IMETHOD GetContainer(nsIWebShellContainer*& aResult);

    // nsIWebShellContainer::GetContainer overrides an
    // nsIDocumentLoader method, so redeclare and forward
    NS_IMETHOD GetContainer(nsISupports** aContainer) {
        return nsDocLoader::GetContainer(aContainer);
    }
    
    // nsILinkHandler
    NS_IMETHOD OnLinkClick(nsIContent* aContent,
        nsLinkVerb aVerb,
        nsIURI* aURI,
        const PRUnichar* aTargetSpec,
        nsIInputStream* aPostDataStream = 0,
        nsIInputStream* aHeadersDataStream = 0);
    NS_IMETHOD OnLinkClickSync(nsIContent* aContent,
        nsLinkVerb aVerb,
        nsIURI* aURI,
        const PRUnichar* aTargetSpec,
        nsIInputStream* aPostDataStream = 0,
        nsIInputStream* aHeadersDataStream = 0,
        nsIDocShell** aDocShell = 0,
        nsIRequest** aRequest = 0);
    NS_IMETHOD OnOverLink(nsIContent* aContent,
        nsIURI* aURI,
        const PRUnichar* aTargetSpec);
    NS_IMETHOD OnLeaveLink();
    NS_IMETHOD GetLinkState(nsIURI* aLinkURI, nsLinkState& aState);

    NS_IMETHOD Create();
    NS_IMETHOD Destroy();

  // nsWebShell
    nsresult GetEventQueue(nsIEventQueue **aQueue);

    static nsEventStatus PR_CALLBACK HandleEvent(nsGUIEvent *aEvent);

    // NS_IMETHOD SetURL(const PRUnichar* aURL);

    friend struct OnLinkClickEvent;

protected:
    // void GetRootWebShellEvenIfChrome(nsIWebShell** aResult);
    void InitFrameData();

    // helpers for executing commands
    virtual nsresult GetControllerForCommand ( const char *inCommand, nsIController** outController );
    virtual nsresult IsCommandEnabled ( const char * inCommand, PRBool* outEnabled );
    virtual nsresult DoCommand ( const char * inCommand );
    nsresult EnsureCommandHandler();

    //
    // Helper method that is called when a new document (including any
    // sub-documents - ie. frames) has been completely loaded.
    //
    virtual nsresult EndPageLoad(nsIWebProgress *aProgress,
        nsIChannel* channel,
        nsresult aStatus);

    PRThread *mThread;

    nsIWebShellContainer* mContainer;

    eCharsetReloadState mCharsetReloadState;

    nsISupports* mHistoryState; // Weak reference.  Session history owns this.

    nsresult CreateViewer(nsIRequest* request,
        const char* aContentType,
        const char* aCommand,
        nsIStreamListener** aResult);

    nsCOMPtr<nsICommandManager> mCommandManager;
    
#ifdef DEBUG
private:
    // We're counting the number of |nsWebShells| to help find leaks
    static unsigned long gNumberOfWebShells;
#endif /* DEBUG */
};

#endif /* webshell____h */

