/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 */

#ifndef nsImapUrl_h___
#define nsImapUrl_h___

#include "nsIImapUrl.h"
#include "nsIImapMockChannel.h"
#include "nsCOMPtr.h"
#include "nsMsgMailNewsUrl.h"
#include "nsIMsgIncomingServer.h"
#include "nsIImapMailFolderSink.h"
#include "nsIImapServerSink.h"
#include "nsIImapMessageSink.h"
#include "nsIImapExtensionSink.h"
#include "nsIImapMiscellaneousSink.h"

#include "nsWeakPtr.h"
#include "nsXPIDLString.h"

class nsImapUrl : public nsIImapUrl, public nsMsgMailNewsUrl, public nsIMsgMessageUrl, public nsIMsgI18NUrl
{
public:

	NS_DECL_ISUPPORTS_INHERITED

	// nsIURI override
	NS_IMETHOD SetSpec(const char * aSpec);

	/////////////////////////////////////////////////////////////////////////////// 
	// we support the nsIImapUrl interface
	///////////////////////////////////////////////////////////////////////////////
  NS_DECL_NSIIMAPURL

  // nsIMsgMailNewsUrl overrides
	NS_IMETHOD IsUrlType(PRUint32 type, PRBool *isType);
  NS_IMETHOD SetMsgWindow(nsIMsgWindow *aMsgWindow);

  // nsIMsgMessageUrl
  NS_DECL_NSIMSGMESSAGEURL
  NS_DECL_NSIMSGI18NURL

	// nsImapUrl
	nsImapUrl();
	virtual ~nsImapUrl();

  static nsresult ConvertToCanonicalFormat(const char *folderName, char onlineDelimiter, char **resultingCanonicalPath);
  static nsresult EscapeSlashes(const char *sourcePath, char **resultPath);
  static nsresult UnescapeSlashes(char *path);

protected:
	virtual nsresult ParseUrl();
	virtual const char * GetUserName() { return m_userName;}

	char		*m_listOfMessageIds;

	// handle the imap specific parsing
	void		ParseImapPart(char *imapPartOfUrl);

	static char *		ReplaceCharsInCopiedString(const char *stringToCopy, char oldChar, char newChar);
	void		ParseFolderPath(char **resultingCanonicalPath);
	void		ParseSearchCriteriaString();
	void		ParseChildDiscoveryDepth();
	void		ParseUidChoice();
	void		ParseMsgFlags();
	void		ParseListOfMessageIds();

  nsresult GetMsgFolder(nsIMsgFolder **msgFolder);

  char        *m_sourceCanonicalFolderPathSubString;
  char        *m_destinationCanonicalFolderPathSubString;
  char		*m_tokenPlaceHolder;
	char		*m_urlidSubString;
  char		m_onlineSubDirSeparator;
	char		*m_searchCriteriaString;	// should we use m_search, or is this special?

	PRBool					m_validUrl;
	PRBool					m_runningUrl;
	PRBool					m_idsAreUids;
	PRBool					m_mimePartSelectorDetected;
	PRBool					m_allowContentChange;	// if PR_FALSE, we can't use Mime parts on demand
  PRBool          m_fetchPartsOnDemand; // if PR_TRUE, we should fetch leave parts on server.
  PRBool          m_msgLoadingFromCache; // if PR_TRUE, we might need to mark read on server
	nsImapContentModifiedType	m_contentModified;
	PRInt32					m_discoveryDepth;

	nsXPIDLCString  m_userName;

	// event sinks
	imapMessageFlagsType	m_flags;
	nsImapAction			m_imapAction;

  nsWeakPtr m_imapFolder;
  nsWeakPtr m_imapMailFolderSink;
  nsWeakPtr	m_imapMessageSink;
  nsWeakPtr	m_imapExtensionSink;
  nsWeakPtr m_imapMiscellaneousSink;

  nsWeakPtr m_imapServerSink;

  // online message copy support; i don't have a better solution yet
  nsCOMPtr <nsISupports> m_copyState;   // now, refcounted.
  nsIFileSpec* m_fileSpec;
  nsCOMPtr<nsIImapMockChannel> m_mockChannel;

    // used by save message to disk
	nsCOMPtr<nsIFileSpec> m_messageFileSpec;
  PRBool                m_addDummyEnvelope;
  PRBool                m_canonicalLineEnding; // CRLF
  
  nsCString mURI; // the RDF URI associated with this url.
  nsString mCharsetOverride; // used by nsIMsgI18NUrl...
};

#endif /* nsImapUrl_h___ */
