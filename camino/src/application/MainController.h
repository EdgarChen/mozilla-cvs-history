/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Josh Aas - josh@mozilla.com
 *   Nate Weaver (Wevah) - wevah@derailer.org
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

#import <Cocoa/Cocoa.h>

@class BookmarkMenu;
@class BookmarkItem;
@class BookmarkManager;
@class KeychainService;
@class BrowserWindowController;
@class GrowlController;
@class PreferenceManager;
@class MVPreferencesController;
@class SUUpdater;


typedef enum EBookmarkOpenBehavior
{
  eBookmarkOpenBehavior_Preferred,     // Reuse current window/tab, unless there isn't one, then open a new one
  eBookmarkOpenBehavior_NewPreferred,  // Open in new window or tab based on prefs
  eBookmarkOpenBehavior_ForceReuse,
  eBookmarkOpenBehavior_NewWindow,
  eBookmarkOpenBehavior_NewTab
};

typedef enum ETabAndWindowCount
{
  eNoWindows,                     // so we have something to fall back on
  eOneWindowWithoutTabs,
  eMultipleWindowsWithoutTabs,
  eMultipleTabsInOneWindow,
  eMultipleTabsInMultipleWindows
};

@interface MainController : NSObject 
{
    IBOutlet NSApplication* mApplication;

    // The following item is added to NSSavePanels as an accessory view
    IBOutlet NSView*        mFilterView;
    IBOutlet NSView*        mExportPanelView;

    IBOutlet NSMenuItem*    mOfflineMenuItem;
    IBOutlet NSMenuItem*    mCloseWindowMenuItem;
    IBOutlet NSMenuItem*    mCloseTabMenuItem;

    IBOutlet BookmarkMenu*  mBookmarksMenu;
    IBOutlet BookmarkMenu*  mDockMenu;
    IBOutlet NSMenu*        mServersSubmenu;

    IBOutlet NSMenu*        mViewMenu;
    IBOutlet NSMenuItem*    mTextZoomOnlyMenuItem;

    IBOutlet NSMenu*        mTextEncodingsMenu;

    IBOutlet NSMenu*        mBookmarksHelperMenu; // not shown, used to get enable state
	
    IBOutlet NSMenuItem*    mBookmarksToolbarMenuItem;
    IBOutlet NSMenuItem*    mAddBookmarkMenuItem;
    IBOutlet NSMenuItem*    mAddBookmarkWithoutPromptMenuItem;
    IBOutlet NSMenuItem*    mAddTabGroupMenuItem;
    IBOutlet NSMenuItem*    mAddTabGroupWithoutPromptMenuItem;
    IBOutlet NSMenuItem*    mCreateBookmarksFolderMenuItem;
    IBOutlet NSMenuItem*    mCreateBookmarksSeparatorMenuItem;  // unused
    IBOutlet NSMenuItem*    mShowAllBookmarksMenuItem;

    IBOutlet SUUpdater*     mAutoUpdater;

    BOOL                    mInitialized;
    BOOL                    mOffline;
    BOOL                    mGeckoInitted;
    BOOL                    mFileMenuUpdatePending;
    BOOL                    mPageInfoUpdatePending;

    BookmarkMenu*           mMenuBookmarks;
    BookmarkMenu*           mDockBookmarks;

    KeychainService*        mKeychainService;

    NSString*               mStartURL;

    GrowlController*        mGrowlController;

    NSMutableDictionary*    mCharsets;
}

- (BOOL)isInitialized;

// Application menu actions
- (IBAction)aboutWindow:(id)sender;
- (IBAction)feedbackLink:(id)aSender;
- (IBAction)checkForUpdates:(id)sender;
- (IBAction)displayPreferencesWindow:(id)sender;
- (IBAction)resetBrowser:(id)sender;
- (IBAction)emptyCache:(id)sender;
- (IBAction)toggleOfflineMode:(id)aSender;

// File menu actions
- (IBAction)newWindow:(id)aSender;
- (IBAction)newTab:(id)aSender;
- (IBAction)openFile:(id)aSender;
- (IBAction)openLocation:(id)aSender;
- (IBAction)doSearch:(id)aSender;
- (IBAction)closeAllWindows:(id)aSender;
- (IBAction)closeCurrentTab:(id)aSender;
- (IBAction)savePage:(id)aSender;
- (IBAction)sendURL:(id)aSender;
- (IBAction)importBookmarks:(id)aSender;
- (IBAction)exportBookmarks:(id)aSender;
- (IBAction)pageSetup:(id)aSender;
- (IBAction)printDocument:(id)aSender;

// Edit menu actions
// (none)

// View menu actions.
- (IBAction)toggleBookmarksToolbar:(id)aSender;
- (IBAction)stop:(id)aSender;
- (IBAction)reload:(id)aSender;
- (IBAction)reloadAllTabs:(id)aSender;
- (IBAction)makeTextBigger:(id)aSender;
- (IBAction)makeTextDefaultSize:(id)aSender;
- (IBAction)makeTextSmaller:(id)aSender;
- (IBAction)makePageBigger:(id)aSender;
- (IBAction)makePageDefaultSize:(id)aSender;
- (IBAction)makePageSmaller:(id)aSender;
- (IBAction)toggleTextZoom:(id)aSender;
- (IBAction)viewPageSource:(id)aSender;
- (IBAction)reloadWithCharset:(id)aSender;
- (IBAction)toggleAutoCharsetDetection:(id)aSender;

// History menu actions
- (IBAction)goHome:(id)aSender;
- (IBAction)goBack:(id)aSender;
- (IBAction)goForward:(id)aSender;
- (IBAction)showHistory:(id)aSender;
- (IBAction)clearHistory:(id)aSender;

// Bookmarks menu actions
- (IBAction)manageBookmarks: (id)aSender;
- (IBAction)openMenuBookmark:(id)aSender;

// Bonjour submenu
- (IBAction)aboutServers:(id)aSender;
- (IBAction)connectToServer:(id)aSender;

// Window menu actions
- (IBAction)zoomAll:(id)aSender;
- (IBAction)previousTab:(id)aSender;
- (IBAction)nextTab:(id)aSender;
- (IBAction)toggleTabThumbnailView:(id)aSender;
- (IBAction)downloadsWindow:(id)aSender;

// Help menu actions
- (IBAction)supportLink:(id)aSender;
- (IBAction)keyboardShortcutsLink:(id)aSender;
- (IBAction)infoLink:(id)aSender;
- (IBAction)reportPhishingPage:(id)aSender;
- (IBAction)aboutPlugins:(id)aSender;

// used by export bookmarks popup to set file extension for the resulting bookmarks file
- (IBAction)setFileExtension:(id)aSender;
// used by page info panel to show certificate information
- (IBAction)showCertificates:(id)aSender;

// if the main/key window is a browser window, return its controller, otherwise nil
- (BrowserWindowController*)mainWindowBrowserController;
- (BrowserWindowController*)keyWindowBrowserController;
- (NSWindow*)frontmostBrowserWindow;
- (NSArray*)browserWindows;

- (BrowserWindowController*)openBrowserWindowWithURL:(NSString*)aURL andReferrer:(NSString*)aReferrer behind:(NSWindow*)window allowPopups:(BOOL)inAllowPopups;
- (BrowserWindowController*)openBrowserWindowWithURLs:(NSArray*)urlArray behind:(NSWindow*)window allowPopups:(BOOL)inAllowPopups;
- (void)showURL:(NSString*)aURL;
- (void)loadBookmark:(BookmarkItem*)item withBWC:(BrowserWindowController*)browserWindowController openBehavior:(EBookmarkOpenBehavior)behavior reverseBgToggle:(BOOL)reverseBackgroundPref;

- (void)adjustCloseWindowMenuItemKeyEquivalent:(BOOL)inHaveTabs;
- (void)adjustCloseTabMenuItemKeyEquivalent:(BOOL)inHaveTabs;
- (void)delayedFixCloseMenuItemKeyEquivalents;
- (void)delayedUpdatePageInfo;

- (NSView*)savePanelView;

@end
