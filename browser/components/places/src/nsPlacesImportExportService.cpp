/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is Mozilla History System
 *
 * The Initial Developer of the Original Code is
 * Google Inc.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Brett Wilson <brettw@gmail.com>
 *   Dietrich Ayala <dietrich@mozilla.com>
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

/**
 * Importer/exporter between the mozStorage-based bookmarks and the old-style
 * "bookmarks.html"
 *
 * Format:
 *
 * Primary heading := h1
 *   Old version used this to set attributes on the bookmarks RDF root, such
 *   as the last modified date. We only use H1 to check for the attribute
 *   PLACES_ROOT, which tells us that this hierarchy root is the places root.
 *   For backwards compatability, if we don't find this, we assume that the
 *   hierarchy is rooted at the bookmarks menu.
 * Heading := any heading other than h1
 *   Old version used this to set attributes on the current container. We only
 *   care about the content of the heading container, which contains the title
 *   of the bookmark container.
 * Bookmark := a
 *   HREF is the destination of the bookmark
 *   FEEDURL is the URI of the RSS feed if this is a livemark.
 *   LAST_CHARSET should be stored as an annotation (FIXME bug 334408) so that the
 *     next time we go to that page we remember the user's preference.
 *   WEB_PANEL is set to "true" if the bookmark should be loaded in the sidebar.
 *   ICON will be stored in the favicon service
 *   ICON_URI is new for places bookmarks.html, it refers to the original
 *     URI of the favicon so we don't have to make up favicon URLs.
 *   Text of the <a> container is the name of the bookmark
 *   Ignored: ADD_DATE, LAST_VISIT, LAST_MODIFIED, ID
 * Bookmark comment := dd
 *   This affects the previosly added bookmark
 * Separator := hr
 *   Insert a separator into the current container
 * The folder hierarchy is defined by <dl>/<ul>/<menu> (the old importing code
 *     handles all these cases, when we write, use <dl>).
 *
 * Overall design
 * --------------
 *
 * We need to emulate a recursive parser. A "Bookmark import frame" is created
 * corresponding to each folder we encounter. These are arranged in a stack,
 * and contain all the state we need to keep track of.
 *
 * A frame is created when we find a heading, which defines a new container.
 * The frame also keeps track of the nesting of <DL>s, (in well-formed
 * bookmarks files, these will have a 1-1 correspondence with frames, but we
 * try to be a little more flexible here). When the nesting count decreases
 * to 0, then we know a frame is complete and to pop back to the previous
 * frame.
 *
 * Note that a lot of things happen when tags are CLOSED because we need to
 * get the text from the content of the tag. For example, link and heading tags
 * both require the content (= title) before actually creating it.
 */

#include "nsPlacesImportExportService.h"
#include "nsNetUtil.h"
#include "nsParserCIID.h"
#include "nsStringAPI.h"
#include "nsUnicharUtils.h"
#include "plbase64.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsDirectoryServiceUtils.h"
#include "nsIPrefService.h"
#include "nsToolkitCompsCID.h"
#include "nsIHTMLContentSink.h"
#include "nsIParser.h"

static NS_DEFINE_CID(kParserCID, NS_PARSER_CID);

#define KEY_TOOLBARFOLDER_LOWER "personal_toolbar_folder"
#define KEY_BOOKMARKSSMENU_LOWER "bookmarks_menu"
#define KEY_PLACESROOT_LOWER "places_root"
#define KEY_HREF_LOWER "href"
#define KEY_FEEDURL_LOWER "feedurl"
#define KEY_WEB_PANEL_LOWER "web_panel"
#define KEY_LASTCHARSET_LOWER "last_charset"
#define KEY_ICON_LOWER "icon"
#define KEY_ICON_URI_LOWER "icon_uri"
#define KEY_SHORTCUTURL_LOWER "shortcuturl"
#define KEY_ID_LOWER "id"

#define LOAD_IN_SIDEBAR_ANNO NS_LITERAL_CSTRING("bookmarkProperties/loadInSidebar")

#define BOOKMARKSS_MENU_ICON_URI "chrome://browser/skin/places/bookmarksMenu.png"
#define BOOKMARKSS_TOOLBAR_ICON_URI "chrome://browser/skin/places/bookmarksToolbar.png"

// define to get debugging messages on console about import
//#define DEBUG_IMPORT

#if defined(XP_WIN) || defined(XP_OS2)
#define NS_LINEBREAK "\015\012"
#else
#define NS_LINEBREAK "\012"
#endif

class nsIOutputStream;
static const char kWhitespace[] = " \r\n\t\b";
static nsresult WriteEscapedUrl(const nsCString &aString, nsIOutputStream* aOutput);

class BookmarkImportFrame
{
public:
  BookmarkImportFrame(PRInt64 aID) :
      mContainerID(aID),
      mContainerNesting(0),
      mLastContainerType(Container_Normal),
      mInDescription(PR_FALSE)
  {
  }

  enum ContainerType { Container_Normal,
                       Container_Places,
                       Container_Menu,
                       Container_Toolbar };

  PRInt64 mContainerID;

  // How many <dl>s have been nested. Each frame/container should start
  // with a heading, and is then followed by a <dl>, <ul>, or <menu>. When
  // that list is complete, then it is the end of this container and we need
  // to pop back up one level for new items. If we never get an open tag for
  // one of these things, we should assume that the container is empty and
  // that things we find should be siblings of it. Normally, these <dl>s won't
  // be nested so this will be 0 or 1.
  PRInt32 mContainerNesting;

  // when we find a heading tag, it actually affects the title of the NEXT
  // container in the list. This stores that heading tag and whether it was
  // special. 'ConsumeHeading' resets this.
  ContainerType mLastContainerType;

  // Container Id, see above
  PRInt64 mLastContainerId;

  // this contains the text from the last begin tag until now. It is reset
  // at every begin tag. We can check it when we see a </a>, or </h3>
  // to see what the text content of that node should be.
  nsString mPreviousText;

  // true when we hit a <dd>, which contains the description for the preceeding
  // <a> tag. We can't just check for </dd> like we can for </a> or </h3>
  // because if there is a sub-folder, it is actually a child of the <dd>
  // because the tag is never explicitly closed. If this is true and we see a
  // new open tag, that means to commit the description to the previous
  // bookmark.
  //
  // Additional weirdness happens when the previous <dt> tag contains a <h3>:
  // this means there is a new folder with the given description, and whose
  // children are contained in the following <dl> list.
  //
  // This is handled in OpenContainer(), which commits previous text if
  // necessary.
  PRBool mInDescription;

  // contains the URL of the previous bookmark created. This is used so that
  // when we encounter a <dd>, we know what bookmark to associate the text with.
  // This is cleared whenever we hit a <h3>, so that we know NOT to save this
  // with a bookmark, but to keep it until 
  nsCOMPtr<nsIURI> mPreviousLink;

  // contains the URL of the previous livemark, so that when the link ends,
  // and the livemark title is known, we can create it.
  nsCOMPtr<nsIURI> mPreviousFeed;

  void ConsumeHeading(nsAString* aHeading, ContainerType* aContainerType, PRInt64* aContainerId)
  {
    *aHeading = mPreviousText;
    *aContainerType = mLastContainerType;
    *aContainerId = mLastContainerId;
    mPreviousText.Truncate();
  }

  // Contains the id of an imported, or newly created bookmark.
  PRInt64 mPreviousId;
};

/**
 * copied from nsEscape.cpp, which requires internal string API
 */
char *
nsEscapeHTML(const char * string)
{
	/* XXX Hardcoded max entity len. The +1 is for the trailing null. */
	char *rv = (char *) nsMemory::Alloc(strlen(string) * 6 + 1);
	char *ptr = rv;

	if(rv)
	  {
		for(; *string != '\0'; string++)
		  {
			if(*string == '<')
			  {
				*ptr++ = '&';
				*ptr++ = 'l';
				*ptr++ = 't';
				*ptr++ = ';';
			  }
			else if(*string == '>')
			  {
				*ptr++ = '&';
				*ptr++ = 'g';
				*ptr++ = 't';
				*ptr++ = ';';
			  }
			else if(*string == '&')
			  {
				*ptr++ = '&';
				*ptr++ = 'a';
				*ptr++ = 'm';
				*ptr++ = 'p';
				*ptr++ = ';';
			  }
			else if (*string == '"')
			  {
				*ptr++ = '&';
				*ptr++ = 'q';
				*ptr++ = 'u';
				*ptr++ = 'o';
				*ptr++ = 't';
				*ptr++ = ';';
			  }			
			else if (*string == '\'')
			  {
				*ptr++ = '&';
				*ptr++ = '#';
				*ptr++ = '3';
				*ptr++ = '9';
				*ptr++ = ';';
			  }
			else
			  {
				*ptr++ = *string;
			  }
		  }
		*ptr = '\0';
	  }

	return(rv);
}

NS_IMPL_ISUPPORTS1(nsPlacesImportExportService, nsIPlacesImportExportService)

nsPlacesImportExportService::nsPlacesImportExportService()
  : mPlacesRoot(0), mBookmarksRoot(0), mToolbarFolder(0)
{
  nsresult rv;
  mHistoryService = do_GetService(NS_NAVHISTORYSERVICE_CONTRACTID, &rv);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "could not get history service");
  mFaviconService = do_GetService(NS_FAVICONSERVICE_CONTRACTID, &rv);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "could not get favicon service");
  mAnnotationService = do_GetService(NS_ANNOTATIONSERVICE_CONTRACTID, &rv);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "could not get annotation service");
  mBookmarksService = do_GetService(NS_NAVBOOKMARKSSERVICE_CONTRACTID, &rv);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "could not get bookmarks service");
  mLivemarkService = do_GetService(NS_LIVEMARKSERVICE_CONTRACTID, &rv);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "could not get livemark service");

  mBookmarksService->GetPlacesRoot(&mPlacesRoot);
  mBookmarksService->GetBookmarksRoot(&mBookmarksRoot);
  mBookmarksService->GetToolbarFolder(&mToolbarFolder);
}

nsPlacesImportExportService::~nsPlacesImportExportService()
{
}

/**
 * The content sink stuff is based loosely on 
 */
class BookmarkContentSink : public nsIHTMLContentSink
{
public:
  nsresult Init(PRBool aAllowRootChanges,
                nsINavBookmarksService* bookmarkService,
                PRInt64 aFolder,
                PRBool aIsImportDefaults);

  NS_DECL_ISUPPORTS

  // nsIContentSink (superclass of nsIHTMLContentSink)
  NS_IMETHOD WillTokenize() { return NS_OK; }
  NS_IMETHOD WillBuildModel() { return NS_OK; }
  NS_IMETHOD DidBuildModel() { return NS_OK; }
  NS_IMETHOD WillInterrupt() { return NS_OK; }
  NS_IMETHOD WillResume() { return NS_OK; }
  NS_IMETHOD SetParser(nsIParser* aParser) { return NS_OK; }
  virtual void FlushPendingNotifications(mozFlushType aType) { }
  NS_IMETHOD SetDocumentCharset(nsACString& aCharset) { return NS_OK; }
  virtual nsISupports *GetTarget() { return nsnull; }

  // nsIHTMLContentSink
  NS_IMETHOD OpenHead() { return NS_OK; }
  NS_IMETHOD BeginContext(PRInt32 aPosition) { return NS_OK; }
  NS_IMETHOD EndContext(PRInt32 aPosition) { return NS_OK; }
  NS_IMETHOD IsEnabled(PRInt32 aTag, PRBool* aReturn)
    { *aReturn = PR_TRUE; return NS_OK; }
  NS_IMETHOD WillProcessTokens() { return NS_OK; }
  NS_IMETHOD DidProcessTokens() { return NS_OK; }
  NS_IMETHOD WillProcessAToken() { return NS_OK; }
  NS_IMETHOD DidProcessAToken() { return NS_OK; }
  NS_IMETHOD OpenContainer(const nsIParserNode& aNode);
  NS_IMETHOD CloseContainer(const nsHTMLTag aTag);
  NS_IMETHOD AddLeaf(const nsIParserNode& aNode);
  NS_IMETHOD AddComment(const nsIParserNode& aNode) { return NS_OK; }
  NS_IMETHOD AddProcessingInstruction(const nsIParserNode& aNode) { return NS_OK; }
  NS_IMETHOD AddDocTypeDecl(const nsIParserNode& aNode) { return NS_OK; }
  NS_IMETHOD NotifyTagObservers(nsIParserNode* aNode) { return NS_OK; }
  NS_IMETHOD_(PRBool) IsFormOnStack() { return PR_FALSE; }

protected:
  nsCOMPtr<nsINavBookmarksService> mBookmarksService;
  nsCOMPtr<nsINavHistoryService> mHistoryService;
  nsCOMPtr<nsIAnnotationService> mAnnotationService;
  nsCOMPtr<nsILivemarkService> mLivemarkService;

  // If set, we will move root items to from their existing position
  // in the hierarchy, to where we find them in the bookmarks file
  // being imported. This should be set when we are loading 
  // the default places html file, and should be unset when doing
  // normal imports so that root folders will not get moved  when
  // importing bookmarks.html files.
  PRBool mAllowRootChanges;

  // if set, this is an import of initial bookmarks.html content,
  // so we don't want to kick off HTTP traffic
  PRBool mIsImportDefaults;

  // If a folder was specified to import into, then ignore flags to put
  // bookmarks in the bookmarks menu or toolbar and keep them inside
  // the folder.
  PRBool mFolderSpecified;

  void HandleContainerBegin(const nsIParserNode& node);
  void HandleContainerEnd();
  void HandleHead1Begin(const nsIParserNode& node);
  void HandleHeadBegin(const nsIParserNode& node);
  void HandleHeadEnd();
  void HandleLinkBegin(const nsIParserNode& node);
  void HandleLinkEnd();
  void HandleSeparator();

  // This is a list of frames. We really want a recursive parser, but the HTML
  // parser gives us tags as a stream. This implements all the state on a stack
  // so we can get the recursive information we need. Use "CurFrame" to get the
  // top "stack frame" with the current state in it.
  nsTArray<BookmarkImportFrame> mFrames;
  BookmarkImportFrame& CurFrame()
  {
    NS_ASSERTION(mFrames.Length() > 0, "Asking for frame when there are none!");
    return mFrames[mFrames.Length() - 1];
  }
  nsresult NewFrame();
  nsresult PopFrame();

  nsresult SetFaviconForURI(nsIURI* aPageURI, nsIURI* aFaviconURI,
                            const nsCString& aData);
  nsresult SetFaviconForFolder(PRInt64 aFolder, const nsACString& aFavicon);

  PRInt64 ConvertImportedIdToInternalId(const nsCString& aId);

#ifdef DEBUG_IMPORT
  // prints spaces for indenting to the current frame depth
  void PrintNesting()
  {
    for (PRUint32 i = 0; i < mFrames.Length(); i ++)
      printf("  ");
  }
#endif
};

// BookmarkContentSink::Init
//
//    Note that the bookmark service pointer is passed in. We can not create
//    the bookmark service from here because this can be called from bookmark
//    service creation, making a weird reentrant loop.

nsresult
BookmarkContentSink::Init(PRBool aAllowRootChanges,
                          nsINavBookmarksService* bookmarkService,
                          PRInt64 aFolder,
                          PRBool aIsImportDefaults)
{
  nsresult rv;
  mBookmarksService = bookmarkService;
  mHistoryService = do_GetService(NS_NAVHISTORYSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  mAnnotationService = do_GetService(NS_ANNOTATIONSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  mLivemarkService = do_GetService(NS_LIVEMARKSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mAllowRootChanges = aAllowRootChanges;
  mIsImportDefaults = aIsImportDefaults;

  // initialize the root frame with the menu root
  PRInt64 menuRoot;
  if (aFolder == 0) {
    rv = mBookmarksService->GetBookmarksRoot(&menuRoot);
    NS_ENSURE_SUCCESS(rv, rv);
    mFolderSpecified = false;
  }
  else {
    menuRoot = aFolder;
    mFolderSpecified = true;
  }
  if (!mFrames.AppendElement(BookmarkImportFrame(menuRoot)))
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}


NS_IMPL_ISUPPORTS2(BookmarkContentSink,
                   nsIContentSink,
                   nsIHTMLContentSink)

// nsIContentSink **************************************************************

NS_IMETHODIMP
BookmarkContentSink::OpenContainer(const nsIParserNode& aNode)
{
  // see the comment for the definition of mInDescription. Basically, we commit
  // any text in mPreviousText to the description of the node/folder if there
  // is any.
  BookmarkImportFrame& frame = CurFrame();
  if (frame.mInDescription) {
    frame.mPreviousText.Trim(kWhitespace); // important!
    if (!frame.mPreviousText.IsEmpty()) {
      // FIXME bug 334758: This description should be stored as an annotation
      // on the URL or folder. We should probably not overwrite existing
      // annotations.
      frame.mPreviousText.Truncate();
    }
    frame.mInDescription = PR_FALSE;
  }

  switch(aNode.GetNodeType()) {
    case eHTMLTag_h1:
      HandleHead1Begin(aNode);
      break;
    case eHTMLTag_h2:
    case eHTMLTag_h3:
    case eHTMLTag_h4:
    case eHTMLTag_h5:
    case eHTMLTag_h6:
      HandleHeadBegin(aNode);
      break;
    case eHTMLTag_a:
      HandleLinkBegin(aNode);
      break;
    case eHTMLTag_dl:
    case eHTMLTag_ul:
    case eHTMLTag_menu:
      HandleContainerBegin(aNode);
      break;
  }
  return NS_OK;
}

NS_IMETHODIMP
BookmarkContentSink::CloseContainer(const nsHTMLTag aTag)
{
  switch (aTag) {
    case eHTMLTag_dl:
    case eHTMLTag_ul:
    case eHTMLTag_menu:
      HandleContainerEnd();
      break;
    case eHTMLTag_dt:
      break;
    case eHTMLTag_h1:
      // ignore
      break;
    case eHTMLTag_h2:
    case eHTMLTag_h3:
    case eHTMLTag_h4:
    case eHTMLTag_h5:
    case eHTMLTag_h6:
      HandleHeadEnd();
      break;
    case eHTMLTag_a:
      HandleLinkEnd();
      break;
    default:
      break;
  }
  return NS_OK;
}


// BookmarkContentSink::AddLeaf
//
//    XXX on the branch, we should be calling CollectSkippedContent as in
//    nsHTMLFragmentContentSink.cpp:AddLeaf when we encounter title, script,
//    style, or server tags. Apparently if we don't, we'll leak the next DOM
//    node. However, this requires that we keep a reference to the parser we'll
//    introduce a circular reference because it has a reference to us.
//
//    This is annoying to fix and these elements are not allowed in bookmarks
//    files anyway. So if somebody tries to import a crazy bookmarks file, it
//    will leak a little bit.

NS_IMETHODIMP
BookmarkContentSink::AddLeaf(const nsIParserNode& aNode)
{
  switch (aNode.GetNodeType()) {
  case eHTMLTag_text:
    // save any text we find
    CurFrame().mPreviousText += aNode.GetText();
    break;
  case eHTMLTag_entity: {
    nsAutoString tmp;
    PRInt32 unicode = aNode.TranslateToUnicodeStr(tmp);
    if (unicode < 0) {
      // invalid entity - just use the text of it
      CurFrame().mPreviousText += aNode.GetText();
    } else {
      CurFrame().mPreviousText.Append(unicode);
    }
    break;
  }
  case eHTMLTag_whitespace:
    CurFrame().mPreviousText.Append(PRUnichar(' '));
    break;
  case eHTMLTag_hr:
    HandleSeparator();
    break;
  }

  return NS_OK;
}

// BookmarkContentSink::HandleContainerBegin

void
BookmarkContentSink::HandleContainerBegin(const nsIParserNode& node)
{
  CurFrame().mContainerNesting ++;
}


// BookmarkContentSink::HandleContainerEnd
//
//    Our "indent" count has decreased, and when we hit 0 that means that this
//    container is complete and we need to pop back to the outer frame. Never
//    pop the toplevel frame

void
BookmarkContentSink::HandleContainerEnd()
{
  BookmarkImportFrame& frame = CurFrame();
  if (frame.mContainerNesting > 0)
    frame.mContainerNesting --;
  if (mFrames.Length() > 1 && frame.mContainerNesting == 0)
    PopFrame();
}


// BookmarkContentSink::HandleHead1Begin
//
//    Handles <H1>. We check for the attribute PLACES_ROOT and reset the
//    container id if it's found. Otherwise, the default bookmark menu
//    root is assumed and imported things will go into the bookmarks menu.

void
BookmarkContentSink::HandleHead1Begin(const nsIParserNode& node)
{
  PRInt32 attrCount = node.GetAttributeCount();
  for (PRInt32 i = 0; i < attrCount; i ++) {
    if (node.GetKeyAt(i).LowerCaseEqualsLiteral(KEY_PLACESROOT_LOWER)) {
      if (mFrames.Length() > 1) {
        NS_WARNING("Trying to set the places root from the middle of the hierarchy. "
                   "This can only be set at the beginning.");
        return;
      }
      PRInt64 mPlacesRoot;
      mBookmarksService->GetPlacesRoot(&mPlacesRoot);
      CurFrame().mContainerID = mPlacesRoot;
      break;
    }
  }
}


// BookmarkContentSink::HandleHeadBegin
//
//    Called for h2,h3,h4,h5,h6. This just stores the correct information in
//    the current frame; the actual new frame corresponding to the container
//    associated with the heading will be created when the tag has been closed
//    and we know the title (we don't know to create a new folder or to merge
//    with an existing one until we have the title).

void
BookmarkContentSink::HandleHeadBegin(const nsIParserNode& node)
{
  BookmarkImportFrame& frame = CurFrame();

  // after a heading, a previous bookmark is not applicable (for example, for
  // the descriptions contained in a <dd>). Neither is any previous head type
  frame.mPreviousLink = nsnull;
  frame.mLastContainerType = BookmarkImportFrame::Container_Normal;
  frame.mLastContainerId = 0;

  // It is syntactically possible for a heading to appear after another heading
  // but before the <dl> that encloses that folder's contents.  This should not
  // happen in practice, as the file will contain "<dl></dl>" sequence for
  // empty containers.
  //
  // Just to be on the safe side, if we encounter
  //   <h3>FOO</h3>
  //   <h3>BAR</h3>
  //   <dl>...content 1...</dl>
  //   <dl>...content 2...</dl>
  // we'll pop the stack when we find the h3 for BAR, treating that as an
  // implicit ending of the FOO container. The output will be FOO and BAR as
  // siblings. If there's another <dl> following (as in "content 2"), those
  // items will be treated as further siblings of FOO and BAR
  if (frame.mContainerNesting == 0)
    PopFrame();

  // We have to check for some attributes to see if this is a "special"
  // folder, which will have different creation rules when the end tag is
  // processed.
  PRInt32 attrCount = node.GetAttributeCount();
  frame.mLastContainerType = BookmarkImportFrame::Container_Normal;
  if (!mFolderSpecified) {
    for (PRInt32 i = 0; i < attrCount; i ++) {
      if (node.GetKeyAt(i).LowerCaseEqualsLiteral(KEY_TOOLBARFOLDER_LOWER)) {
        frame.mLastContainerType = BookmarkImportFrame::Container_Toolbar;
        break;
      } else if (node.GetKeyAt(i).LowerCaseEqualsLiteral(KEY_BOOKMARKSSMENU_LOWER)) {
        frame.mLastContainerType = BookmarkImportFrame::Container_Menu;
        break;
      } else if (node.GetKeyAt(i).LowerCaseEqualsLiteral(KEY_PLACESROOT_LOWER)) {
        frame.mLastContainerType = BookmarkImportFrame::Container_Places;
        break;
      } else if (node.GetKeyAt(i).LowerCaseEqualsLiteral(KEY_ID_LOWER)) {
        frame.mLastContainerId =
          ConvertImportedIdToInternalId(NS_ConvertUTF16toUTF8(node.GetValueAt(i)));
      }
    }
  }
  CurFrame().mPreviousText.Truncate();
}


// BookmarkContentSink::HandleHeadEnd
//
//    Creates the new frame for this heading now that we know the name of the
//    container (tokens since the heading open tag will have been placed in
//    mPreviousText).

void
BookmarkContentSink::HandleHeadEnd()
{
  NewFrame();
}


// BookmarkContentSink::HandleLinkBegin
//
//    Handles "<a" tags by creating a new bookmark. The title of the bookmark
//    will be the text content, which will be stuffed in mPreviousText for us
//    and which will be saved by HandleLinkEnd

void
BookmarkContentSink::HandleLinkBegin(const nsIParserNode& node)
{
  nsresult rv;

  BookmarkImportFrame& frame = CurFrame();

  // We need to make sure that the feed URIs from previous frames are emptied. 
  frame.mPreviousFeed = nsnull;

  // We need to make sure that the bookmark id from previous frames are emptied. 
  frame.mPreviousId = 0;

  // mPreviousText will hold our link text, clear it so that can be appended to
  frame.mPreviousText.Truncate();

  // get the attributes we care about
  nsAutoString href;
  nsAutoString feedUrl;
  nsAutoString icon;
  nsAutoString iconUri;
  nsAutoString lastCharset;
  nsAutoString keyword;
  nsAutoString webPanel;
  nsAutoString id;
  PRInt32 attrCount = node.GetAttributeCount();
  for (PRInt32 i = 0; i < attrCount; i ++) {
    const nsAString& key = node.GetKeyAt(i);
    if (key.LowerCaseEqualsLiteral(KEY_HREF_LOWER)) {
      href = node.GetValueAt(i);
    } else if (key.LowerCaseEqualsLiteral(KEY_FEEDURL_LOWER)) {
      feedUrl = node.GetValueAt(i);
    } else if (key.LowerCaseEqualsLiteral(KEY_ICON_LOWER)) {
      icon = node.GetValueAt(i);
    } else if (key.LowerCaseEqualsLiteral(KEY_ICON_URI_LOWER)) {
      iconUri = node.GetValueAt(i);
    } else if (key.LowerCaseEqualsLiteral(KEY_LASTCHARSET_LOWER)) {
      lastCharset = node.GetValueAt(i);
    } else if (key.LowerCaseEqualsLiteral(KEY_SHORTCUTURL_LOWER)) {
      keyword = node.GetValueAt(i);
    } else if (key.LowerCaseEqualsLiteral(KEY_WEB_PANEL_LOWER)) {
      webPanel = node.GetValueAt(i);
    } else if (key.LowerCaseEqualsLiteral(KEY_ID_LOWER)) {
      id = node.GetValueAt(i);
    }
  }
  href.Trim(kWhitespace);
  feedUrl.Trim(kWhitespace);
  icon.Trim(kWhitespace);
  iconUri.Trim(kWhitespace);
  lastCharset.Trim(kWhitespace);
  keyword.Trim(kWhitespace);
  webPanel.Trim(kWhitespace);
  id.Trim(kWhitespace);

  // For feeds, get the feed URL. If it is invalid, it will leave mPreviousFeed
  // NULL and we'll continue trying to create it as a normal bookmark.
  if (!feedUrl.IsEmpty()) {
    NS_NewURI(getter_AddRefs(frame.mPreviousFeed),
              NS_ConvertUTF16toUTF8(feedUrl), nsnull);
  }

  // Ignore <a> tags that have no href: we don't know what to do with them.
  if (href.IsEmpty()) {
    frame.mPreviousLink = nsnull;

    // The exception is for feeds, where the href is an optional component
    // indicating the source web site.
    if (!frame.mPreviousFeed)
      return;
  } else {
    // Save this so the link text and descriptions can be associated with it.
    // Note that we ignore errors if this is a feed: URLs aren't strictly
    // necessary in these cases.
    nsresult rv = NS_NewURI(getter_AddRefs(frame.mPreviousLink),
                   href, nsnull);
    if (NS_FAILED(rv) && !frame.mPreviousFeed) {
      frame.mPreviousLink = nsnull;
      return; // invalid link
    }
  }

  // if there's a pre-existing Places bookmark id, use it
  frame.mPreviousId = ConvertImportedIdToInternalId(NS_ConvertUTF16toUTF8(id));

  // if there is a feedURL, this is a livemark, which is a special case
  // that we handle in HandleLinkEnd(): don't create normal bookmarks
  if (frame.mPreviousFeed)
    return;

  // attempt to get a property for the supposedly pre-existing bookmark
  if (frame.mPreviousId > 0) {
    PRInt32 index;
    rv = mBookmarksService->GetItemIndex(frame.mPreviousId, &index);
    if (NS_FAILED(rv))
      frame.mPreviousId = 0;
  }

  // if no previous id (or a legacy id), create a new bookmark
  if (frame.mPreviousId == 0) {
    // create the bookmark
    rv = mBookmarksService->InsertItem(frame.mContainerID, frame.mPreviousLink,
                                       mBookmarksService->DEFAULT_INDEX, &frame.mPreviousId);
    NS_ASSERTION(NS_SUCCEEDED(rv), "InsertItem failed");
  }

  // save the favicon, ignore errors
  if (!icon.IsEmpty() || !iconUri.IsEmpty()) {
    nsCOMPtr<nsIURI> iconUriObject;
    NS_NewURI(getter_AddRefs(iconUriObject), iconUri);
    if (!icon.IsEmpty() || iconUriObject) {
      rv = SetFaviconForURI(frame.mPreviousLink, iconUriObject,
                            NS_ConvertUTF16toUTF8(icon));
    }
  }

  // save the keyword, ignore errors
  if (!keyword.IsEmpty())
    mBookmarksService->SetKeywordForBookmark(frame.mPreviousId, keyword);

  if (webPanel.LowerCaseEqualsLiteral("true")) {
    // set load-in-sidebar annotation for the bookmark

    mAnnotationService->SetItemAnnotationInt32(frame.mPreviousId, LOAD_IN_SIDEBAR_ANNO,
                                               1, 0,
                                               nsIAnnotationService::EXPIRE_NEVER);
  }
  // FIXME bug 334408: save the last charset
}


// BookmarkContentSink::HandleLinkEnd
//
//    Saves the title for the given bookmark. This only writes the user title.
//    Any previous title will be untouched. If this is a new entry, it will have
//    an empty "official" title until you visit it.

void
BookmarkContentSink::HandleLinkEnd()
{
  nsresult rv;
  BookmarkImportFrame& frame = CurFrame();
  frame.mPreviousText.Trim(kWhitespace);
  if (frame.mPreviousFeed) {
    // The bookmark is actually a livemark.  Create it here.
    // (It gets created here instead of in HandleLinkBegin()
    // because we need to know the title before creating it.)
    PRInt64 folderId;

    if (frame.mPreviousId > 0) {
      // It's a pre-existing livemark, so update its properties
      rv = mLivemarkService->SetSiteURI(frame.mPreviousId, frame.mPreviousLink);
      NS_ASSERTION(NS_SUCCEEDED(rv), "SetSiteURI failed!");
      rv = mLivemarkService->SetFeedURI(frame.mPreviousId, frame.mPreviousFeed);
      NS_ASSERTION(NS_SUCCEEDED(rv), "SetFeedURI failed!");
      rv = mBookmarksService->SetItemTitle(frame.mPreviousId, frame.mPreviousText);
      NS_ASSERTION(NS_SUCCEEDED(rv), "SetItemTitle failed!");
    } else {
      if (mIsImportDefaults) {
        rv = mLivemarkService->CreateLivemarkFolderOnly(mBookmarksService,
                                                   frame.mContainerID,
                                                   frame.mPreviousText,
                                                   frame.mPreviousLink,
                                                   frame.mPreviousFeed,
                                                   -1,
                                                   &folderId);
        NS_ASSERTION(NS_SUCCEEDED(rv), "CreateLivemarkFolderOnly failed!");
      } else {
        rv = mLivemarkService->CreateLivemark(frame.mContainerID,
                                         frame.mPreviousText,
                                         frame.mPreviousLink,
                                         frame.mPreviousFeed,
                                         -1,
                                         &folderId);
        NS_ASSERTION(NS_SUCCEEDED(rv), "CreateLivemark failed!");
      }
    }
#ifdef DEBUG_IMPORT
    PrintNesting();
    printf("Creating livemark '%s'\n",
           NS_ConvertUTF16toUTF8(frame.mPreviousText).get());
#endif
  }
  else if (frame.mPreviousLink) {
#ifdef DEBUG_IMPORT
    PrintNesting();
    printf("Creating bookmark '%s'\n",
           NS_ConvertUTF16toUTF8(frame.mPreviousText).get());
#endif
    mBookmarksService->SetItemTitle(frame.mPreviousId, frame.mPreviousText);
  }
  frame.mPreviousText.Truncate();
}


// BookmarkContentSink::HandleSeparator
//
//    Inserts a separator into the current container
void
BookmarkContentSink::HandleSeparator()
{
  BookmarkImportFrame& frame = CurFrame();

  // bookmarks.html contains a separator between the toolbar menu and the
  // rest of the items.  Since we pull the toolbar menu out into the top level,
  // we want to skip over this separator since it looks out of place.
  if (frame.mLastContainerType != BookmarkImportFrame::Container_Toolbar) {
    // create the separator
#ifdef DEBUG_IMPORT
    PrintNesting();
    printf("--------\n");
#endif
    PRInt64 itemId;
    mBookmarksService->InsertSeparator(frame.mContainerID,
                                       mBookmarksService->DEFAULT_INDEX,
                                       &itemId);
  }
}


// BookmarkContentSink::NewFrame
//
//    This is called when there is a new folder found. The folder takes the
//    name from the previous frame's heading.

nsresult
BookmarkContentSink::NewFrame()
{
  nsresult rv;

  PRInt64 ourID = 0;
  nsString containerName;
  BookmarkImportFrame::ContainerType containerType;
  CurFrame().ConsumeHeading(&containerName, &containerType, &ourID);

  PRBool updateFolder = PR_FALSE;
  if (ourID == 0) {
    switch (containerType) {
      case BookmarkImportFrame::Container_Normal:
        // regular folder: use an existing folder if that name already exists
        rv = mBookmarksService->GetChildFolder(CurFrame().mContainerID,
                                               containerName, &ourID);
        NS_ENSURE_SUCCESS(rv, rv);
        if (ourID == 0) {
          // need to append a new folder
          rv = mBookmarksService->CreateFolder(CurFrame().mContainerID,
                                              containerName,
                                              mBookmarksService->DEFAULT_INDEX, &ourID);
          NS_ENSURE_SUCCESS(rv, rv);
        }
        break;
      case BookmarkImportFrame::Container_Places:
        // places root, never reparent here, when we're building the initial
        // hierarchy, it will only be defined at the top level
        rv = mBookmarksService->GetPlacesRoot(&ourID);
        NS_ENSURE_SUCCESS(rv, rv);
        break;
      case BookmarkImportFrame::Container_Menu:
        // menu root
        rv = mBookmarksService->GetBookmarksRoot(&ourID);
        NS_ENSURE_SUCCESS(rv, rv);
        if (mAllowRootChanges) {
          updateFolder = PR_TRUE;
          rv = SetFaviconForFolder(ourID, NS_LITERAL_CSTRING(BOOKMARKSS_MENU_ICON_URI));
          NS_ENSURE_SUCCESS(rv, rv);
        }
        break;
      case BookmarkImportFrame::Container_Toolbar:
        // get toolbar folder
        PRInt64 bookmarkToolbarFolder;
        rv = mBookmarksService->GetToolbarFolder(&bookmarkToolbarFolder);
        NS_ENSURE_SUCCESS(rv, rv);
        if (!bookmarkToolbarFolder) {
          // create new folder
          rv = mBookmarksService->CreateFolder(CurFrame().mContainerID,
                                              containerName,
                                              mBookmarksService->DEFAULT_INDEX, &ourID);
          NS_ENSURE_SUCCESS(rv, rv);
          // there's no toolbar folder, so make us the toolbar folder
          rv = mBookmarksService->SetToolbarFolder(ourID);
          NS_ENSURE_SUCCESS(rv, rv);
          // set favicon
          rv = SetFaviconForFolder(ourID, NS_LITERAL_CSTRING(BOOKMARKSS_TOOLBAR_ICON_URI));
          NS_ENSURE_SUCCESS(rv, rv);
        }
        else {
          ourID = bookmarkToolbarFolder;
        }
        break;
      default:
        NS_NOTREACHED("Unknown container type");
    }
  }
#ifdef DEBUG_IMPORT
  PrintNesting();
  printf("Folder %lld \'%s\'", ourID, NS_ConvertUTF16toUTF8(containerName).get());
#endif

  if (updateFolder) {
    // move the menu folder to the current position
    mBookmarksService->MoveFolder(ourID, CurFrame().mContainerID, -1);
    mBookmarksService->SetItemTitle(ourID, containerName);
#ifdef DEBUG_IMPORT
    printf(" [reparenting]");
#endif
  }

#ifdef DEBUG_IMPORT
  printf("\n");
#endif

  if (!mFrames.AppendElement(BookmarkImportFrame(ourID)))
    return NS_ERROR_OUT_OF_MEMORY;
  return NS_OK;
}


// BookmarkContentSink::PopFrame
//

nsresult
BookmarkContentSink::PopFrame()
{
  // we must always have one frame
  if (mFrames.Length() <= 1) {
    NS_NOTREACHED("Trying to complete more bookmark folders than you started");
    return NS_ERROR_FAILURE;
  }
  mFrames.RemoveElementAt(mFrames.Length() - 1);
  return NS_OK;
}


// BookmarkContentSink::SetFaviconForURI
//
//    aData is a string that is a data URI for the favicon. Our job is to
//    decode it and store it in the favicon service.
//
//    When aIconURI is non-null, we will use that as the URI of the favicon
//    when storing in the favicon service.
//
//    When aIconURI is null, we have to make up a URI for this favicon so that
//    it can be stored in the service. The real one will be set the next time
//    the user visits the page. Our made up one should get expired when the
//    page no longer references it.

nsresult
BookmarkContentSink::SetFaviconForURI(nsIURI* aPageURI, nsIURI* aIconURI,
                                      const nsCString& aData)
{
  nsresult rv;
  static PRUint32 serialNumber = 0; // for made-up favicon URIs

  nsCOMPtr<nsIFaviconService> faviconService(do_GetService(NS_FAVICONSERVICE_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  // if the input favicon URI is a chrome: URI, then we just save it and don't
  // worry about data
  if (aIconURI) {
    nsCString faviconScheme;
    aIconURI->GetScheme(faviconScheme);
    if (faviconScheme.EqualsLiteral("chrome")) {
      return faviconService->SetFaviconUrlForPage(aPageURI, aIconURI);
    }
  }

  // some bookmarks have placeholder URIs that contain just "data:"
  // ignore these
  if (aData.Length() <= 5)
    return NS_OK;

  nsCOMPtr<nsIURI> faviconURI;
  if (aIconURI) {
    faviconURI = aIconURI;
  } else {
    // make up favicon URL
    nsCAutoString faviconSpec;
    faviconSpec.AssignLiteral("http://www.mozilla.org/2005/made-up-favicon/");
    faviconSpec.AppendInt(serialNumber);
    faviconSpec.AppendLiteral("-");
    faviconSpec.AppendInt(PR_Now());
    rv = NS_NewURI(getter_AddRefs(faviconURI), faviconSpec);
    NS_ENSURE_SUCCESS(rv, rv);
    serialNumber ++;
  }

  nsCOMPtr<nsIURI> dataURI;
  rv = NS_NewURI(getter_AddRefs(dataURI), aData);
  NS_ENSURE_SUCCESS(rv, rv);

  // use the data: protocol handler to convert the data
  nsCOMPtr<nsIIOService> ioService = do_GetIOService(&rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIProtocolHandler> protocolHandler;
  rv = ioService->GetProtocolHandler("data", getter_AddRefs(protocolHandler));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIChannel> channel;
  rv = protocolHandler->NewChannel(dataURI, getter_AddRefs(channel));
  NS_ENSURE_SUCCESS(rv, rv);

  // blocking stream is OK for data URIs
  nsCOMPtr<nsIInputStream> stream;
  rv = channel->Open(getter_AddRefs(stream));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 available;
  rv = stream->Available(&available);
  NS_ENSURE_SUCCESS(rv, rv);
  if (available == 0)
    return NS_ERROR_FAILURE;

  // read all the decoded data
  PRUint8* buffer = NS_STATIC_CAST(PRUint8*,
                                   nsMemory::Alloc(sizeof(PRUint8) * available));
  if (!buffer)
    return NS_ERROR_OUT_OF_MEMORY;
  PRUint32 numRead;
  rv = stream->Read(NS_REINTERPRET_CAST(char*, buffer), available, &numRead);
  if (NS_FAILED(rv) || numRead != available) {
    nsMemory::Free(buffer);
    return rv;
  }

  nsCAutoString mimeType;
  rv = channel->GetContentType(mimeType);
  NS_ENSURE_SUCCESS(rv, rv);

  // save in service
  rv = faviconService->SetFaviconData(faviconURI, buffer, available, mimeType, 0);
  nsMemory::Free(buffer);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = faviconService->SetFaviconUrlForPage(aPageURI, faviconURI);
  return NS_OK; 
}


// BookmarkContentSink::SetFaviconForFolder
//
//    This sets the given favicon URI for the given folder. It is used to
//    initialize the favicons for the bookmarks menu and toolbar. We don't
//    actually set any data here because we assume the URI is a chrome: URI.
//    These do not have to contain any data for them to work.

nsresult
BookmarkContentSink::SetFaviconForFolder(PRInt64 aFolder,
                                         const nsACString& aFavicon)
{
  nsresult rv;
  nsCOMPtr<nsIFaviconService> faviconService(do_GetService(NS_FAVICONSERVICE_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> folderURI;
  rv = mBookmarksService->GetFolderURI(aFolder,
                                                getter_AddRefs(folderURI));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> faviconURI;
  rv = NS_NewURI(getter_AddRefs(faviconURI), aFavicon);
  NS_ENSURE_SUCCESS(rv, rv);

  return faviconService->SetFaviconUrlForPage(folderURI, faviconURI);
}

// Converts a string id (legacy rdf or contemporary) into an int id
PRInt64
BookmarkContentSink::ConvertImportedIdToInternalId(const nsCString& aId) {
  PRInt64 intId = 0;
  if (aId.IsEmpty() || Substring(aId, 0, 4).Equals(NS_LITERAL_CSTRING("rdf:"), CaseInsensitiveCompare))
    return intId;
  nsresult rv;
  intId = aId.ToInteger(&rv);
  if (NS_FAILED(rv))
    intId = 0;
  return intId;
}


// SyncChannelStatus
//
//    If a function returns an error, we need to set the channel status to be
//    the same, but only if the channel doesn't have its own error. This returns
//    the error code that should be sent to OnStopRequest.

static nsresult
SyncChannelStatus(nsIChannel* channel, nsresult status)
{
  nsresult channelStatus;
  channel->GetStatus(&channelStatus);
  if (NS_FAILED(channelStatus))
    return channelStatus;

  if (NS_SUCCEEDED(status))
    return NS_OK; // caller and the channel are happy

  // channel was OK, but caller wasn't: set the channel state
  channel->Cancel(status);
  return status;
}


static char kFileIntro[] =
    "<!DOCTYPE NETSCAPE-Bookmark-file-1>" NS_LINEBREAK
    // Note: we write bookmarks in UTF-8
    "<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=UTF-8\">" NS_LINEBREAK
    "<TITLE>Bookmarks</TITLE>" NS_LINEBREAK;
static const char kRootIntro[] = "<H1";
static const char kCloseRootH1[] = "</H1>" NS_LINEBREAK NS_LINEBREAK;

static const char kBookmarkIntro[] = "<DL><p>" NS_LINEBREAK;
static const char kBookmarkClose[] = "</DL><p>" NS_LINEBREAK;
static const char kContainerIntro[] = "<DT><H3";
static const char kContainerClose[] = "</H3>" NS_LINEBREAK;
static const char kItemOpen[] = "<DT><A";
static const char kItemClose[] = "</A>" NS_LINEBREAK;
static const char kSeparator[] = "<HR>" NS_LINEBREAK;
static const char kQuoteStr[] = "\"";
static const char kCloseAngle[] = ">";
static const char kIndent[] = "    ";

static const char kPlacesRootAttribute[] = " PLACES_ROOT=\"true\"";
static const char kBookmarksRootAttribute[] = " BOOKMARKSS_MENU=\"true\"";
static const char kToolbarFolderAttribute[] = " PERSONAL_TOOLBAR_FOLDER=\"true\"";
static const char kIconAttribute[] = " ICON=\"";
static const char kIconURIAttribute[] = " ICON_URI=\"";
static const char kHrefAttribute[] = " HREF=\"";
static const char kFeedURIAttribute[] = " FEEDURL=\"";
static const char kWebPanelAttribute[] = " WEB_PANEL=\"true\"";
static const char kKeywordAttribute[] = " SHORTCUTURL=\"";
static const char kIdAttribute[] = " ID=\"";

// WriteContainerPrologue
//
//    <DL><p>
//
//    Goes after the container header (<H3...) but before the contents

static nsresult
WriteContainerPrologue(const nsACString& aIndent, nsIOutputStream* aOutput)
{
  PRUint32 dummy;
  nsresult rv = aOutput->Write(PromiseFlatCString(aIndent).get(), aIndent.Length(), &dummy);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aOutput->Write(kBookmarkIntro, sizeof(kBookmarkIntro)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}


// WriteContainerEpilogue
//
//    </DL><p>
//
//    Goes after the container contents to close the container

static nsresult
WriteContainerEpilogue(const nsACString& aIndent, nsIOutputStream* aOutput)
{
  PRUint32 dummy;
  nsresult rv = aOutput->Write(PromiseFlatCString(aIndent).get(), aIndent.Length(), &dummy);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aOutput->Write(kBookmarkClose, sizeof(kBookmarkClose)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}


// DataToDataURI

static nsresult
DataToDataURI(PRUint8* aData, PRUint32 aDataLen, const nsACString& aMimeType,
              nsACString& aDataURI)
{
  char* encoded = PL_Base64Encode(NS_REINTERPRET_CAST(const char*, aData),
                                  aDataLen, nsnull);
  if (!encoded)
    return NS_ERROR_OUT_OF_MEMORY;

  aDataURI.AssignLiteral("data:");
  aDataURI.Append(aMimeType);
  aDataURI.AppendLiteral(";base64,");
  aDataURI.Append(encoded);

  nsMemory::Free(encoded);
  return NS_OK;
}


// WriteFaviconAttribute
//
//    This writes the 'ICON="data:asdlfkjas;ldkfja;skdljfasdf"' attribute for
//    an item. We special-case chrome favicon URIs by just writing the chrome:
//    URI.

static nsresult
WriteFaviconAttribute(const nsACString& aURI, nsIOutputStream* aOutput)
{
  nsresult rv;
  PRUint32 dummy;

  nsCOMPtr<nsIURI> uri;
  rv = NS_NewURI(getter_AddRefs(uri), aURI);
  NS_ENSURE_SUCCESS(rv, rv);

  // get favicon
  nsCOMPtr<nsIFaviconService> faviconService = do_GetService(NS_FAVICONSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIURI> faviconURI;
  rv = faviconService->GetFaviconForPage(uri, getter_AddRefs(faviconURI));
  if (rv == NS_ERROR_NOT_AVAILABLE)
    return NS_OK; // no favicon
  NS_ENSURE_SUCCESS(rv, rv); // anything else is error

  nsCAutoString faviconScheme;
  nsCAutoString faviconSpec;
  rv = faviconURI->GetSpec(faviconSpec);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = faviconURI->GetScheme(faviconScheme);
  NS_ENSURE_SUCCESS(rv, rv);

  // write favicon URI: 'ICON_URI="..."'
  rv = aOutput->Write(kIconURIAttribute, sizeof(kIconURIAttribute)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = WriteEscapedUrl(faviconSpec, aOutput);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aOutput->Write(kQuoteStr, sizeof(kQuoteStr)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!faviconScheme.EqualsLiteral("chrome")) {
    // only store data for non-chrome URIs

    // get the data - BE SURE TO FREE
    nsCAutoString mimeType;
    PRUint32 dataLen;
    PRUint8* data;
    rv = faviconService->GetFaviconData(faviconURI, mimeType, &dataLen, &data);
    NS_ENSURE_SUCCESS(rv, rv);
    if (dataLen > 0) {
      // convert to URI
      nsCString faviconContents;
      rv = DataToDataURI(data, dataLen, mimeType, faviconContents);
      nsMemory::Free(data);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = aOutput->Write(kIconAttribute, sizeof(kIconAttribute)-1, &dummy);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = aOutput->Write(faviconContents.get(), faviconContents.Length(), &dummy);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = aOutput->Write(kQuoteStr, sizeof(kQuoteStr)-1, &dummy);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  return NS_OK;
}


// nsPlacesImportExportService::WriteContainer
//
//    Writes out all the necessary parts of a bookmarks folder.

nsresult
nsPlacesImportExportService::WriteContainer(PRInt64 aFolder, const nsACString& aIndent,
                               nsIOutputStream* aOutput)
{
  nsresult rv = WriteContainerHeader(aFolder, aIndent, aOutput);
  NS_ENSURE_SUCCESS(rv, rv);

  // FIXME bug 334758: write container description here as a <DD>

  rv = WriteContainerPrologue(aIndent, aOutput);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = WriteContainerContents(aFolder, aIndent, aOutput);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = WriteContainerEpilogue(aIndent, aOutput);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}


// nsPlacesImportExportService::WriteContainerHeader
//
//    This writes '<DL><H3>Title</H3>'
//    Remember folders can also have favicons, which we put in the H3 tag

nsresult
nsPlacesImportExportService::WriteContainerHeader(PRInt64 aFolder, const nsACString& aIndent,
                                     nsIOutputStream* aOutput)
{
  PRUint32 dummy;
  nsresult rv;

  // indent
  if (!aIndent.IsEmpty()) {
    rv = aOutput->Write(PromiseFlatCString(aIndent).get(), aIndent.Length(), &dummy);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // "<DL H3"
  rv = aOutput->Write(kContainerIntro, sizeof(kContainerIntro)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  // " PERSONAL_TOOLBAR_FOLDER="true"", etc.
  if (aFolder == mPlacesRoot) {
    rv = aOutput->Write(kPlacesRootAttribute, sizeof(kPlacesRootAttribute)-1, &dummy);
    NS_ENSURE_SUCCESS(rv, rv);
  } else if (aFolder == mBookmarksRoot) {
    rv = aOutput->Write(kBookmarksRootAttribute, sizeof(kBookmarksRootAttribute)-1, &dummy);
    NS_ENSURE_SUCCESS(rv, rv);
  } else if (aFolder == mToolbarFolder) {
    rv = aOutput->Write(kToolbarFolderAttribute, sizeof(kToolbarFolderAttribute)-1, &dummy);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // id
  rv = aOutput->Write(kIdAttribute, sizeof(kIdAttribute)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCAutoString id;
  id.AppendInt(aFolder);
  rv = aOutput->Write(id.get(), id.Length(), &dummy);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aOutput->Write(kQuoteStr, sizeof(kQuoteStr)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  // favicon (most folders won't have one)
  nsCOMPtr<nsIURI> folderURI;
  rv = mBookmarksService->GetFolderURI(aFolder, getter_AddRefs(folderURI));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCAutoString folderSpec;
  rv = folderURI->GetSpec(folderSpec);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = WriteFaviconAttribute(folderSpec, aOutput);
  NS_ENSURE_SUCCESS(rv, rv);

  // ">"
  rv = aOutput->Write(kCloseAngle, sizeof(kCloseAngle)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  // title
  rv = WriteContainerTitle(aFolder, aOutput);
  NS_ENSURE_SUCCESS(rv, rv);

  // "</H3>\n"
  rv = aOutput->Write(kContainerClose, sizeof(kContainerClose)-1, &dummy);
  return rv;
}


// nsPlacesImportExportService::WriteContainerTitle
//
//    Retrieves, escapes and writes the container title to the stream.

nsresult
nsPlacesImportExportService::WriteContainerTitle(PRInt64 aFolder, nsIOutputStream* aOutput)
{
  nsAutoString title;
  nsresult rv;
  
  rv = mBookmarksService->GetItemTitle(aFolder, title);
  NS_ENSURE_SUCCESS(rv, rv);

  char* escapedTitle = nsEscapeHTML(NS_ConvertUTF16toUTF8(title).get());
  if (escapedTitle) {
    PRUint32 dummy;
    rv = aOutput->Write(escapedTitle, strlen(escapedTitle), &dummy);
    nsMemory::Free(escapedTitle);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}


// nsBookmarks::WriteItem
//
//    "<DT><A HREF="..." ICON="...">Name</A>"

nsresult
nsPlacesImportExportService::WriteItem(nsINavHistoryResultNode* aItem,
                          const nsACString& aIndent,
                          nsIOutputStream* aOutput)
{
  PRUint32 dummy;
  nsresult rv;

  // indent
  if (!aIndent.IsEmpty()) {
    rv = aOutput->Write(PromiseFlatCString(aIndent).get(), aIndent.Length(), &dummy);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // '<DT><A'
  rv = aOutput->Write(kItemOpen, sizeof(kItemOpen)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  // ' HREF="http://..."' - note that we need to call GetURI on the result
  // node because some nodes (eg queries) generate this lazily.
  rv = aOutput->Write(kHrefAttribute, sizeof(kHrefAttribute)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCAutoString uri;
  rv = aItem->GetUri(uri);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = WriteEscapedUrl(uri, aOutput);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aOutput->Write(kQuoteStr, sizeof(kQuoteStr)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  // ' ICON="..."'
  rv = WriteFaviconAttribute(uri, aOutput);
  NS_ENSURE_SUCCESS(rv, rv);

  // get item id 
  PRInt64 itemId;
  rv = aItem->GetItemId(&itemId);
  NS_ENSURE_SUCCESS(rv, rv);

  // write id
  rv = aOutput->Write(kIdAttribute, sizeof(kIdAttribute)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCAutoString id;
  id.AppendInt(itemId);
  rv = aOutput->Write(id.get(), id.Length(), &dummy);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aOutput->Write(kQuoteStr, sizeof(kQuoteStr)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  // keyword (shortcuturl)
  nsAutoString keyword;
  rv = mBookmarksService->GetKeywordForBookmark(itemId, keyword);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!keyword.IsEmpty()) {
    rv = aOutput->Write(kKeywordAttribute, sizeof(kKeywordAttribute)-1, &dummy);
    NS_ENSURE_SUCCESS(rv, rv);
    char* escapedKeyword = nsEscapeHTML(NS_ConvertUTF16toUTF8(keyword).get());
    rv = aOutput->Write(escapedKeyword, strlen(escapedKeyword), &dummy);
    nsMemory::Free(escapedKeyword);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = aOutput->Write(kQuoteStr, sizeof(kQuoteStr)-1, &dummy);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Write WEB_PANEL="true" if the load-in-sidebar annotation is set for the
  // item
  PRBool loadInSidebar = PR_FALSE;
  rv = mAnnotationService->ItemHasAnnotation(itemId, LOAD_IN_SIDEBAR_ANNO,
                                             &loadInSidebar);
  NS_ENSURE_SUCCESS(rv, rv);
  if (loadInSidebar)
    aOutput->Write(kWebPanelAttribute, sizeof(kWebPanelAttribute)-1, &dummy);

  // FIXME bug 334408: write last character set here

  // '>'
  rv = aOutput->Write(kCloseAngle, sizeof(kCloseAngle)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  // title
  nsCAutoString title;
  rv = aItem->GetTitle(title);
  NS_ENSURE_SUCCESS(rv, rv);
  char* escapedTitle = nsEscapeHTML(title.get());
  if (escapedTitle) {
    rv = aOutput->Write(escapedTitle, strlen(escapedTitle), &dummy);
    nsMemory::Free(escapedTitle);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // '</A>\n'
  rv = aOutput->Write(kItemClose, sizeof(kItemClose)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  // FIXME bug 334758: write item description here as a <DD>

  return NS_OK;
}


// WriteLivemark
//
//    Similar to WriteItem, this has an additional FEEDURL attribute and
//    the HREF is optional and points to the source page.

nsresult
nsPlacesImportExportService::WriteLivemark(PRInt64 aFolderId, const nsACString& aIndent,
                              nsIOutputStream* aOutput)
{
  PRUint32 dummy;
  nsresult rv;

  // indent
  if (!aIndent.IsEmpty()) {
    rv = aOutput->Write(PromiseFlatCString(aIndent).get(), aIndent.Length(), &dummy);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // '<DT><A'
  rv = aOutput->Write(kItemOpen, sizeof(kItemOpen)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  // get feed URI
  nsCOMPtr<nsIURI> feedURI;
  rv = mLivemarkService->GetFeedURI(aFolderId, getter_AddRefs(feedURI));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCString feedSpec;
  rv = feedURI->GetSpec(feedSpec);
  NS_ENSURE_SUCCESS(rv, rv);

  // write feed URI
  rv = aOutput->Write(kFeedURIAttribute, sizeof(kFeedURIAttribute)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = WriteEscapedUrl(feedSpec, aOutput);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aOutput->Write(kQuoteStr, sizeof(kQuoteStr)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  // get the optional site URI
  nsCOMPtr<nsIURI> siteURI;
  rv = mLivemarkService->GetSiteURI(aFolderId, getter_AddRefs(siteURI));
  NS_ENSURE_SUCCESS(rv, rv);
  if (siteURI) {
    nsCString siteSpec;
    rv = siteURI->GetSpec(siteSpec);
    NS_ENSURE_SUCCESS(rv, rv);

    // write site URI
    rv = aOutput->Write(kHrefAttribute, sizeof(kHrefAttribute)-1, &dummy);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = WriteEscapedUrl(siteSpec, aOutput);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = aOutput->Write(kQuoteStr, sizeof(kQuoteStr)-1, &dummy);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // write id
  rv = aOutput->Write(kIdAttribute, sizeof(kIdAttribute)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCAutoString id;
  id.AppendInt(aFolderId);
  rv = aOutput->Write(id.get(), id.Length(), &dummy);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aOutput->Write(kQuoteStr, sizeof(kQuoteStr)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  // '>'
  rv = aOutput->Write(kCloseAngle, sizeof(kCloseAngle)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  // title
  rv = WriteContainerTitle(aFolderId, aOutput);
  NS_ENSURE_SUCCESS(rv, rv);

  // '</A>\n'
  rv = aOutput->Write(kItemClose, sizeof(kItemClose)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


// WriteSeparator
//
//    "<HR>"

nsresult
WriteSeparator(const nsCString& aIndent, nsIOutputStream* aOutput)
{
  PRUint32 dummy;
  nsresult rv;

  // indent
  if (!aIndent.IsEmpty()) {
    rv = aOutput->Write(aIndent.get(), aIndent.Length(), &dummy);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = aOutput->Write(kSeparator, sizeof(kSeparator)-1, &dummy);
  return rv;
}


// WriteEscapedUrl
//
//    Writes the given string to the stream escaped as necessary for URLs.
//
//    Unfortunately, the old bookmarks system uses a custom hardcoded and
//    braindead escaping scheme that we need to emulate. It just replaces
//    quotes with %22 and that's it.

nsresult
WriteEscapedUrl(const nsCString& aString, nsIOutputStream* aOutput)
{
  nsCAutoString escaped(aString);
  PRInt32 offset;
  while ((offset = escaped.FindChar('\"')) >= 0) {
    escaped.Cut(offset, 1);
    escaped.Insert(NS_LITERAL_CSTRING("%22"), offset);
  }
  PRUint32 dummy;
  return aOutput->Write(escaped.get(), escaped.Length(), &dummy);
}


// nsPlacesImportExportService::WriteContainerContents
//
//    The indent here is the indent of the parent. We will add an additional
//    indent before writing data.

nsresult
nsPlacesImportExportService::WriteContainerContents(PRInt64 aFolder, const nsACString& aIndent,
                                       nsIOutputStream* aOutput)
{
  nsCAutoString myIndent(aIndent);
  myIndent.Append(kIndent);

  // get empty options
  nsresult rv;
  nsCOMPtr<nsINavHistoryQueryOptions> optionsInterface;
  rv = mHistoryService->GetNewQueryOptions(getter_AddRefs(optionsInterface));
  NS_ENSURE_SUCCESS(rv, rv);

  // QueryFolderChildren requires a concrete options
  nsCOMPtr<nsINavHistoryQueryOptions> options = do_QueryInterface(optionsInterface);
  NS_ENSURE_TRUE(options, NS_ERROR_UNEXPECTED);

  // get the query object
  nsCOMPtr<nsINavHistoryQuery> query;
  rv = mHistoryService->GetNewQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  // query for just this folder
  rv = query->SetFolders(&aFolder, 1);
  NS_ENSURE_SUCCESS(rv, rv);
  // query for just bookmarks (necessary?)
  rv = query->SetOnlyBookmarked(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  // group by folder (necessary? doesn't SetFolders trigger this?)
  const PRUint16 groupMode = nsINavHistoryQueryOptions::GROUP_BY_FOLDER;
  rv = options->SetGroupingMode(&groupMode, 1);
  NS_ENSURE_SUCCESS(rv, rv);

  // execute query
  nsCOMPtr<nsINavHistoryResult> result;
  rv = mHistoryService->ExecuteQuery(query, options, getter_AddRefs(result));
  NS_ENSURE_SUCCESS(rv, rv);

  // get root (folder) node
  nsCOMPtr<nsINavHistoryQueryResultNode> rootNode;
  rv = result->GetRoot(getter_AddRefs(rootNode));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = rootNode->SetContainerOpen(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 childCount = 0;
  rootNode->GetChildCount(&childCount);
  for (PRUint32 i = 0; i < childCount; ++i) {
    nsCOMPtr<nsINavHistoryResultNode> child;
    rv = rootNode->GetChild(i, getter_AddRefs(child));
    NS_ENSURE_SUCCESS(rv, rv);
    PRUint32 type = 0;
    rv = child->GetType(&type);
    NS_ENSURE_SUCCESS(rv, rv);
    if (type == nsINavHistoryResultNode::RESULT_TYPE_FOLDER) {
      // bookmarks folder
      nsCOMPtr<nsINavHistoryFolderResultNode> folderNode = do_QueryInterface(child);
      PRInt64 folderId;
      rv = folderNode->GetItemId(&folderId);
      NS_ENSURE_SUCCESS(rv, rv);
      if (aFolder == mPlacesRoot && (folderId == mToolbarFolder ||
                               folderId == mBookmarksRoot)) {
        // don't write out the bookmarks menu folder from the
        // places root. When writing to bookmarks.html, it is reparented
        // to the menu, which is the root of the namespace. This provides
        // better backwards compatability.
        continue;
      }

      // it could be a regular folder or it could be a livemark
      PRBool isLivemark;
      rv = mLivemarkService->IsLivemark(folderId, &isLivemark);
      NS_ENSURE_SUCCESS(rv, rv);

      if (isLivemark)
        rv = WriteLivemark(folderId, myIndent, aOutput);
      else
        rv = WriteContainer(folderId, myIndent, aOutput);
    } else if (type == nsINavHistoryResultNode::RESULT_TYPE_SEPARATOR) {
      rv = WriteSeparator(myIndent, aOutput);
    } else {
      rv = WriteItem(child, myIndent, aOutput);
    }
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

// nsIPlacesImportExportService::ImportHTMLFromFile
//
NS_IMETHODIMP
nsPlacesImportExportService::ImportHTMLFromFile(nsILocalFile* aFile)
{
  // this version is exposed on the interface and disallows changing of roots
  return ImportHTMLFromFileInternal(aFile, PR_FALSE, 0, PR_FALSE);
}

// nsIPlacesImportExportService::ImportHTMLFromFileToFolder
//
NS_IMETHODIMP
nsPlacesImportExportService::ImportHTMLFromFileToFolder(nsILocalFile* aFile, PRInt64 aFolderId)
{
  // this version is exposed on the interface and disallows changing of roots
  return ImportHTMLFromFileInternal(aFile, PR_FALSE, aFolderId, PR_FALSE);
}

nsresult
nsPlacesImportExportService::ImportHTMLFromFileInternal(nsILocalFile* aFile,
                                       PRBool aAllowRootChanges,
                                       PRInt64 aFolder,
                                       PRBool aIsImportDefaults)
{
  nsCOMPtr<nsIFile> file(do_QueryInterface(aFile));
#ifdef DEBUG_IMPORT
  nsAutoString path;
  file->GetPath(path);
  printf("\nImporting %s\n", NS_ConvertUTF16toUTF8(path).get());
#endif

  // wrap the import in a transaction to make it faster
  nsresult rv;
  mBookmarksService->BeginUpdateBatch();

  nsCOMPtr<nsIParser> parser = do_CreateInstance(kParserCID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<BookmarkContentSink> sink = new BookmarkContentSink;
  NS_ENSURE_TRUE(sink, NS_ERROR_OUT_OF_MEMORY);
  rv = sink->Init(aAllowRootChanges, mBookmarksService, aFolder, aIsImportDefaults);
  NS_ENSURE_SUCCESS(rv, rv);
  parser->SetContentSink(sink);

  // channel: note we have to set the content type or the default "unknown" type
  // will confuse the parser
  nsCOMPtr<nsIIOService> ioservice = do_GetIOService(&rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIURI> fileURI;
  rv = ioservice->NewFileURI(file, getter_AddRefs(fileURI));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIChannel> channel;
  rv = ioservice->NewChannelFromURI(fileURI, getter_AddRefs(channel));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = channel->SetContentType(NS_LITERAL_CSTRING("text/html"));
  NS_ENSURE_SUCCESS(rv, rv);

  // streams
  nsCOMPtr<nsIInputStream> stream;
  rv = channel->Open(getter_AddRefs(stream));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIInputStream> bufferedstream;
  rv = NS_NewBufferedInputStream(getter_AddRefs(bufferedstream), stream, 4096);
  NS_ENSURE_SUCCESS(rv, rv);

  // init parser
  rv = parser->Parse(fileURI, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  // feed the parser the data
  // Note: on error, we always need to set the channel's status to be the
  // same, and to always call OnStopRequest with the channel error.
  nsCOMPtr<nsIStreamListener> listener = do_QueryInterface(parser, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = listener->OnStartRequest(channel, nsnull);
  rv = SyncChannelStatus(channel, rv);
  while(NS_SUCCEEDED(rv))
  {
    PRUint32 available;
    rv = bufferedstream->Available(&available);
    if (rv == NS_BASE_STREAM_CLOSED) {
      rv = NS_OK;
      available = 0;
    }
    if (NS_FAILED(rv)) {
      channel->Cancel(rv);
      break;
    }
    if (!available)
      break; // blocking input stream has none available when done

    rv = listener->OnDataAvailable(channel, nsnull, bufferedstream, 0, available);
    rv = SyncChannelStatus(channel, rv);
    if (NS_FAILED(rv))
      break;
  }
  listener->OnStopRequest(channel, nsnull, rv);
  // commit transaction
  mBookmarksService->EndUpdateBatch();
  return NS_OK;
}

// nsIPlacesImportExportService::ExportHMTLToFile
//
NS_IMETHODIMP
nsPlacesImportExportService::ExportHTMLToFile(nsILocalFile* aBookmarksFile)
{
  if (!aBookmarksFile)
    return NS_ERROR_NULL_POINTER;

  // get a safe output stream, so we don't clobber the bookmarks file unless
  // all the writes succeeded.
  nsCOMPtr<nsIOutputStream> out;
  nsresult rv = NS_NewSafeLocalFileOutputStream(getter_AddRefs(out),
                                                aBookmarksFile,
                                                PR_WRONLY | PR_CREATE_FILE,
                                                /*octal*/ 0600,
                                                0);
  NS_ENSURE_SUCCESS(rv, rv);

  // We need a buffered output stream for performance.
  // See bug 202477.
  nsCOMPtr<nsIOutputStream> strm;
  rv = NS_NewBufferedOutputStream(getter_AddRefs(strm), out, 4096);
  NS_ENSURE_SUCCESS(rv, rv);

  // file header
  PRUint32 dummy;
  rv = strm->Write(kFileIntro, sizeof(kFileIntro)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  // '<H1'
  rv = strm->Write(kRootIntro, sizeof(kRootIntro)-1, &dummy); // <H1
  NS_ENSURE_SUCCESS(rv, rv);

  // bookmarks menu favicon
  nsCOMPtr<nsIURI> folderURI;
  rv = mBookmarksService->GetFolderURI(mBookmarksRoot, getter_AddRefs(folderURI));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCAutoString folderSpec;
  rv = folderURI->GetSpec(folderSpec);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = WriteFaviconAttribute(folderSpec, strm);
  NS_ENSURE_SUCCESS(rv, rv);

  // '>Bookmarks</H1>
  rv = strm->Write(kCloseAngle, sizeof(kCloseAngle)-1, &dummy); // >
  NS_ENSURE_SUCCESS(rv, rv);
  rv = WriteContainerTitle(mBookmarksRoot, strm);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = strm->Write(kCloseRootH1, sizeof(kCloseRootH1)-1, &dummy); // </H1>
  NS_ENSURE_SUCCESS(rv, rv);

  // prologue
  rv = WriteContainerPrologue(EmptyCString(), strm);
  NS_ENSURE_SUCCESS(rv, rv);

  // indents
  nsCAutoString indent;
  indent.Assign(kIndent);

  // places root
  rv = WriteContainer(mPlacesRoot, indent, strm);
  NS_ENSURE_SUCCESS(rv, rv);

  // bookmarks menu contents
  rv = WriteContainerContents(mBookmarksRoot, EmptyCString(), strm);
  NS_ENSURE_SUCCESS(rv, rv);

  // epilogue
  rv = WriteContainerEpilogue(EmptyCString(), strm);
  NS_ENSURE_SUCCESS(rv, rv);

  // commit the write
  nsCOMPtr<nsISafeOutputStream> safeStream = do_QueryInterface(strm, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  return safeStream->Finish();
}

NS_IMETHODIMP
nsPlacesImportExportService::BackupBookmarksFile()
{
  // get bookmarks file
  nsCOMPtr<nsIFile> bookmarksFileDir;
  nsresult rv = NS_GetSpecialDirectory(NS_APP_BOOKMARKS_50_FILE,
                                       getter_AddRefs(bookmarksFileDir));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsILocalFile> bookmarksFile(do_QueryInterface(bookmarksFileDir));

  // create if it doesn't exist
  PRBool exists;
  rv = bookmarksFile->Exists(&exists);
  if (NS_FAILED(rv)) {
    rv = bookmarksFile->Create(nsIFile::NORMAL_FILE_TYPE, 0600);
    NS_ASSERTION(rv, "Unable to create bookmarks.html!");
    return rv;
  }

  // export bookmarks.html
  rv = ExportHTMLToFile(bookmarksFile);
  NS_ENSURE_SUCCESS(rv, rv);

  // archive if needed
  nsCOMPtr<nsIPrefService> prefServ(do_GetService(NS_PREFSERVICE_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIPrefBranch> bookmarksPrefs;
  rv = prefServ->GetBranch("browser.bookmarks.", getter_AddRefs(bookmarksPrefs));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 numberOfBackups;
  rv = bookmarksPrefs->GetIntPref("max_backups", &numberOfBackups);
  if (NS_FAILED(rv))
    numberOfBackups = 5;

  if (numberOfBackups > 0) {
    rv = ArchiveBookmarksFile(numberOfBackups, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

/**
 *  ArchiveBookmarksFile()
 *
 *  Creates a dated backup once a day in <profile>/bookmarkbackups
 *
 *  PRInt32 numberOfBackups - the maximum number of backups to keep
 *
 *  PRBool forceArchive - forces creating an archive even if one was 
 *                        already created that day (overwrites)
 */
nsresult
nsPlacesImportExportService::ArchiveBookmarksFile(PRInt32 numberOfBackups,
                                         PRBool forceArchive)
{
  nsCOMPtr<nsIFile> bookmarksBackupDir;
  nsresult rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                                       getter_AddRefs(bookmarksBackupDir));
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsDependentCString dirName("bookmarkbackups");
  rv = bookmarksBackupDir->AppendNative(dirName);
  NS_ENSURE_SUCCESS(rv, rv);
  
  PRBool exists;
  rv = bookmarksBackupDir->Exists(&exists);
  if (NS_FAILED(rv) || !exists) {
    rv = bookmarksBackupDir->Create(nsIFile::DIRECTORY_TYPE, 0700);
    
    // if there's no backup folder, there's no backup, fail
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // construct the new leafname
  PRTime          now64 = PR_Now();
  PRExplodedTime  nowInfo;
  PR_ExplodeTime(now64, PR_LocalTimeParameters, &nowInfo);
  PR_NormalizeTime(&nowInfo, PR_LocalTimeParameters);

  char timeString[128];
  
  PR_FormatTime(timeString, 128, "bookmarks-%Y-%m-%d.html", &nowInfo);

  //nsCAutoString backupFilenameCString(timeString);
  //nsAutoString backupFilenameString = NS_ConvertUTF8toUTF16(backupFilenameCString);
  nsAutoString backupFilenameString = NS_ConvertUTF8toUTF16((timeString));

  nsCOMPtr<nsIFile> backupFile;
  if (forceArchive) {
    // if we have a backup from today, nuke it
    nsCOMPtr<nsIFile> currentBackup;
    rv = bookmarksBackupDir->Clone(getter_AddRefs(currentBackup));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = currentBackup->Append(backupFilenameString);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = currentBackup->Exists(&exists);
    if (NS_SUCCEEDED(rv) && exists) {
      rv = currentBackup->Remove(PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  } else {
    nsCOMPtr<nsISimpleEnumerator> existingBackups;
    rv = bookmarksBackupDir->GetDirectoryEntries(getter_AddRefs(existingBackups));
    NS_ENSURE_SUCCESS(rv, rv);

    nsStringArray backupFileNames;

    PRBool hasMoreElements = PR_FALSE;
    PRBool hasCurrentBackup = PR_FALSE;
    
    while (NS_SUCCEEDED(existingBackups->HasMoreElements(&hasMoreElements)) &&
           hasMoreElements)
    {
      rv = existingBackups->GetNext(getter_AddRefs(backupFile));
      NS_ENSURE_SUCCESS(rv, rv);
      nsAutoString backupName;
      rv = backupFile->GetLeafName(backupName);
      NS_ENSURE_SUCCESS(rv, rv);
      
      // the backup for today exists, do not create later
      if (backupName == backupFilenameString) {
        hasCurrentBackup = PR_TRUE;
        continue;
      }

      // mark the rest for possible removal
      if (Substring(backupName, 0, 10) == NS_LITERAL_STRING("bookmarks-"))
        backupFileNames.AppendString(backupName);
    }

    if (numberOfBackups > 0 && backupFileNames.Count() >= numberOfBackups) {
      PRInt32 numberOfBackupsToDelete = backupFileNames.Count() - numberOfBackups + 1;
      backupFileNames.Sort();

      while (numberOfBackupsToDelete--) {
        (void)bookmarksBackupDir->Clone(getter_AddRefs(backupFile));
        (void)backupFile->Append(*backupFileNames[0]);
        (void)backupFile->Remove(PR_FALSE);
        backupFileNames.RemoveStringAt(0);
      }
    }

    if (hasCurrentBackup)
      return NS_OK;
  }

  nsCOMPtr<nsIFile> bookmarksFile;
  rv = NS_GetSpecialDirectory(NS_APP_BOOKMARKS_50_FILE,
                              getter_AddRefs(bookmarksFile));
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = bookmarksFile->CopyTo(bookmarksBackupDir, backupFilenameString);
  // at least dump something out in case this fails in a debug build
  NS_ENSURE_SUCCESS(rv, rv);

  return rv;
}
