/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation.  All Rights Reserved.
 */

#ifndef nsWebShellWindow_h__
#define nsWebShellWindow_h__

#include "nsISupports.h"
#include "nsIWebShellWindow.h"
#include "nsIBrowserWindow.h"
#include "nsGUIEvent.h"
#include "nsIWebShell.h"  
#include "nsIDocumentLoaderObserver.h"
#include "nsIDocumentObserver.h"
#include "nsVoidArray.h"
#include "nsIMenu.h"

// can't use forward class decl's because of template bugs on Solaris 
#include "nsIDOMDocument.h"
#include "nsIDOMNode.h"

#include "nsCOMPtr.h"

/* Forward declarations.... */
struct PLEvent;

class nsIURL;
class nsIAppShell;
class nsIContent;
class nsIDocument;
class nsIDOMCharacterData;
class nsIDOMElement;
class nsIDOMWindow;
class nsIDOMHTMLImageElement;
class nsIDOMHTMLInputElement;
class nsIStreamObserver;
class nsIWidget;
class nsIXULWindowCallbacks;
class nsVoidArray;

class nsWebShellWindow : public nsIWebShellWindow,
                         public nsIWebShellContainer,
												 public nsIBrowserWindow,
                         public nsIDocumentLoaderObserver,
                         public nsIDocumentObserver
{
public:
  nsWebShellWindow();

  // nsISupports interface...
  NS_DECL_ISUPPORTS


  // nsIWebShellContainer interface...
  NS_IMETHOD WillLoadURL(nsIWebShell* aShell,
                         const PRUnichar* aURL,
                         nsLoadType aReason);

  NS_IMETHOD BeginLoadURL(nsIWebShell* aShell,
                          const PRUnichar* aURL);

  NS_IMETHOD ProgressLoadURL(nsIWebShell* aShell,
                             const PRUnichar* aURL,
                             PRInt32 aProgress,
                             PRInt32 aProgressMax);

  NS_IMETHOD EndLoadURL(nsIWebShell* aShell,
                        const PRUnichar* aURL,
                        PRInt32 aStatus);


  NS_IMETHOD CreatePopup(nsIDOMElement* aElement, nsIDOMElement* aPopupContent, 
                         PRInt32 aXPos, PRInt32 aYPos, 
                         const nsString& aPopupType, const nsString& aPopupAlignment,
                         nsIDOMWindow* aWindow);

  NS_IMETHOD ContentShellAdded(nsIWebShell* aChildShell, nsIContent* frameNode);

  NS_IMETHOD NewWebShell(PRUint32 aChromeMask,
                         PRBool aVisible,
                         nsIWebShell *&aNewWebShell);

  NS_IMETHOD AddWebShellInfo(const nsString& aID,
                             nsIWebShell* aChildShell);

	NS_IMETHOD GetContentShellById(const nsString& anID, nsIWebShell** aResult);
	NS_IMETHOD LockUntilChromeLoad() { mLockedUntilChromeLoad = PR_TRUE; return NS_OK; }
  NS_IMETHOD GetLockedState(PRBool& aResult) { aResult = mLockedUntilChromeLoad; return NS_OK; }

  NS_IMETHOD FindWebShellWithName(const PRUnichar* aName,
                                  nsIWebShell*& aResult);

  NS_IMETHOD FocusAvailable(nsIWebShell* aFocusedWebShell, PRBool& aFocusTaken);


  // nsIWebShellWindow methods...
  NS_IMETHOD Show(PRBool aShow);
  NS_IMETHOD ShowModal();
  NS_IMETHOD Close();
  NS_IMETHOD GetWebShell(nsIWebShell *& aWebShell);
  NS_IMETHOD GetWidget(nsIWidget *& aWidget);

  // nsWebShellWindow methods...
  nsresult Initialize(nsIWebShellWindow * aParent, nsIAppShell* aShell, nsIURL* aUrl,
                      nsIStreamObserver* anObserver,
                      nsIXULWindowCallbacks *aCallbacks,
                      PRInt32 aInitialWidth, PRInt32 aInitialHeight);
  nsIWidget* GetWidget(void) { return mWindow; }

  void DoContextMenu(
	  nsMenuEvent * aMenuEvent,
	  nsIDOMNode  * aMenuNode, 
	  nsIWidget   * aParentWindow,
	  PRInt32       aX,
	  PRInt32       aY);
  
  // nsIDocumentLoaderObserver
  NS_IMETHOD OnStartDocumentLoad(nsIDocumentLoader* loader, 
                                 nsIURL* aURL, const char* aCommand);
  NS_IMETHOD OnEndDocumentLoad(nsIDocumentLoader* loader, 
                               nsIURL *aUrl, PRInt32 aStatus);
  NS_IMETHOD OnStartURLLoad(nsIDocumentLoader* loader, 
                            nsIURL* aURL, const char* aContentType, 
                            nsIContentViewer* aViewer);
  NS_IMETHOD OnProgressURLLoad(nsIDocumentLoader* loader, 
                               nsIURL* aURL, PRUint32 aProgress, 
                               PRUint32 aProgressMax);
  NS_IMETHOD OnStatusURLLoad(nsIDocumentLoader* loader, 
                             nsIURL* aURL, nsString& aMsg);
  NS_IMETHOD OnEndURLLoad(nsIDocumentLoader* loader, 
                          nsIURL* aURL, PRInt32 aStatus);
  NS_IMETHOD HandleUnknownContentType(nsIDocumentLoader* loader, 
                                      nsIURL* aURL,
                                      const char *aContentType,
                                      const char *aCommand );

  
  // nsIDocumentObserver
  NS_IMETHOD BeginUpdate(nsIDocument *aDocument);
  NS_IMETHOD EndUpdate(nsIDocument *aDocument);
  NS_IMETHOD BeginLoad(nsIDocument *aDocument);
  NS_IMETHOD EndLoad(nsIDocument *aDocument);
  NS_IMETHOD BeginReflow(nsIDocument *aDocument, nsIPresShell* aShell);
  NS_IMETHOD EndReflow(nsIDocument *aDocument, nsIPresShell* aShell);
  NS_IMETHOD ContentChanged(nsIDocument *aDocument,
                            nsIContent* aContent,
                            nsISupports* aSubContent);
  NS_IMETHOD ContentStatesChanged(nsIDocument *aDocument,
                                  nsIContent* aContent1,
                                  nsIContent* aContent2);
  NS_IMETHOD AttributeChanged(nsIDocument *aDocument,
                              nsIContent*  aContent,
                              nsIAtom*     aAttribute,
                              PRInt32      aHint);
  NS_IMETHOD ContentAppended(nsIDocument *aDocument,
                             nsIContent* aContainer,
                             PRInt32     aNewIndexInContainer);
  NS_IMETHOD ContentInserted(nsIDocument *aDocument,
                             nsIContent* aContainer,
                             nsIContent* aChild,
                             PRInt32 aIndexInContainer);
  NS_IMETHOD ContentReplaced(nsIDocument *aDocument,
                             nsIContent* aContainer,
                             nsIContent* aOldChild,
                             nsIContent* aNewChild,
                             PRInt32 aIndexInContainer);
  NS_IMETHOD ContentRemoved(nsIDocument *aDocument,
                            nsIContent* aContainer,
                            nsIContent* aChild,
                            PRInt32 aIndexInContainer);
  NS_IMETHOD StyleSheetAdded(nsIDocument *aDocument,
                             nsIStyleSheet* aStyleSheet);
  NS_IMETHOD StyleSheetRemoved(nsIDocument *aDocument,
                               nsIStyleSheet* aStyleSheet);
  NS_IMETHOD StyleSheetDisabledStateChanged(nsIDocument *aDocument,
                                            nsIStyleSheet* aStyleSheet,
                                            PRBool aDisabled);
  NS_IMETHOD StyleRuleChanged(nsIDocument *aDocument,
                              nsIStyleSheet* aStyleSheet,
                              nsIStyleRule* aStyleRule,
                              PRInt32 aHint);
  NS_IMETHOD StyleRuleAdded(nsIDocument *aDocument,
                            nsIStyleSheet* aStyleSheet,
                            nsIStyleRule* aStyleRule);
  NS_IMETHOD StyleRuleRemoved(nsIDocument *aDocument,
                              nsIStyleSheet* aStyleSheet,
                              nsIStyleRule* aStyleRule);
  NS_IMETHOD DocumentWillBeDestroyed(nsIDocument *aDocument);

	// nsIBrowserWindow methods not already covered elsewhere
  NS_IMETHOD Init(nsIAppShell* aAppShell,
                  nsIPref* aPrefs,
                  const nsRect& aBounds,
                  PRUint32 aChromeMask,
                  PRBool aAllowPlugins = PR_TRUE);
  NS_IMETHOD MoveTo(PRInt32 aX, PRInt32 aY);
  NS_IMETHOD SizeTo(PRInt32 aWidth, PRInt32 aHeight);
  NS_IMETHOD GetBounds(nsRect& aResult);
  NS_IMETHOD GetWindowBounds(nsRect& aResult);
  NS_IMETHOD Show() { Show(PR_TRUE); return NS_OK; }
  NS_IMETHOD Hide() { Show(PR_FALSE); return NS_OK; }
  NS_IMETHOD SetChrome(PRUint32 aNewChromeMask);
  NS_IMETHOD GetChrome(PRUint32& aChromeMaskResult);
  NS_IMETHOD SetTitle(const PRUnichar* aTitle);
  NS_IMETHOD GetTitle(const PRUnichar** aResult);
  NS_IMETHOD SetStatus(const PRUnichar* aStatus);
  NS_IMETHOD GetStatus(const PRUnichar** aResult);
  NS_IMETHOD SetProgress(PRInt32 aProgress, PRInt32 aProgressMax);

protected:
  void ExecuteJavaScriptString(nsString& aJavaScript);

  NS_IMETHOD ConvertWebShellToDOMWindow(nsIWebShell* aShell, nsIDOMWindow** aDOMWindow);

  PRInt32 GetDocHeight(nsIDocument * aDoc);
 
  void LoadMenus(nsIDOMDocument * aDOMDoc, nsIWidget * aParentWindow);
  void DynamicLoadMenus(nsIDOMDocument * aDOMDoc, nsIWidget * aParentWindow);
  nsCOMPtr<nsIDOMNode>     FindNamedParentFromDoc(nsIDOMDocument * aDomDoc, const nsString &aName);
  nsCOMPtr<nsIDOMNode>     FindNamedDOMNode(const nsString &aName, nsIDOMNode * aParent, PRInt32 & aCount, PRInt32 aEndCount);
  nsCOMPtr<nsIDOMDocument> GetNamedDOMDoc(const nsString & aWebShellName);
  nsCOMPtr<nsIDOMNode>     GetParentNodeFromDOMDoc(nsIDOMDocument * aDOMDoc);
  NS_IMETHOD               CreateMenu(nsIMenuBar * aMenuBar, nsIDOMNode * aMenuNode, nsString & aMenuName);
  void LoadSubMenu(nsIMenu * pParentMenu, nsIDOMElement * menuElement,nsIDOMNode * menuNode);
  NS_IMETHOD LoadMenuItem(nsIMenu * pParentMenu, nsIDOMElement * menuitemElement, nsIDOMNode * menuitemNode);

  nsCOMPtr<nsIDOMNode>     GetDOMNodeFromWebShell(nsIWebShell *aShell);
  void                     ExecuteStartupCode();
  void                     SetSizeFromXUL();
  void                     SetTitleFromXUL();
  

  virtual ~nsWebShellWindow();

  static nsEventStatus PR_CALLBACK HandleEvent(nsGUIEvent *aEvent);
  NS_IMETHODIMP           ShowModalInternal();
  void                    ExitModalLoop() { mContinueModalLoop = PR_FALSE; }

  nsIWidget*              mWindow;
  nsIWebShell*            mWebShell;
  nsIXULWindowCallbacks*  mCallbacks;
  PRBool                  mContinueModalLoop;
	PRBool									mLockedUntilChromeLoad;
  PRBool                  mChromeInitialized;

  nsVoidArray mMenuDelegates;

  nsVoidArray* mContentShells; // Tracks an array of information about new shells that will be
                               // created as the XUL file for this window loads.

  nsIDOMNode * contextMenuTest;
private:

  static void * HandleModalDialogEvent(PLEvent *aEvent);
  static void DestroyModalDialogEvent(PLEvent *aEvent);
};


#endif /* nsWebShellWindow_h__ */
