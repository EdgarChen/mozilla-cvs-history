/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#ifndef __nsNNTPNewsgroupPost_h
#define __nsNNTPNewsgroupPost_h

#include "msgCore.h"
#include "nsINNTPNewsgroupPost.h"
#include "nsCOMPtr.h"
#include "nsIFileSpec.h"


#define HEADER_FROM				0
#define HEADER_NEWSGROUPS		1
#define HEADER_SUBJECT			2

// set this to the last required header
#define HEADER_LAST_REQUIRED	HEADER_SUBJECT

#define HEADER_PATH				3
#define HEADER_DATE				4

#define HEADER_REPLYTO			5
#define HEADER_SENDER			6
#define HEADER_FOLLOWUPTO		7
#define HEADER_DATERECEIVED		8
#define HEADER_EXPIRES			9
#define HEADER_CONTROL			10
#define HEADER_DISTRIBUTION		11
#define HEADER_ORGANIZATION		12
#define HEADER_REFERENCES		13

// stuff that's required to be in the message,
// but probably generated on the server
#define HEADER_RELAYVERSION		14
#define HEADER_POSTINGVERSION	15
#define HEADER_MESSAGEID	    16

// keep this in sync with the above
#define HEADER_LAST             HEADER_MESSAGEID

class nsNNTPNewsgroupPost : public nsINNTPNewsgroupPost {
    
public:
    nsNNTPNewsgroupPost();
    virtual ~nsNNTPNewsgroupPost();
    
    NS_DECL_ISUPPORTS
    
    // Required headers
    NS_IMPL_CLASS_GETSET_STR(RelayVersion, m_header[HEADER_RELAYVERSION]);
    NS_IMPL_CLASS_GETSET_STR(PostingVersion, m_header[HEADER_POSTINGVERSION]);
    NS_IMPL_CLASS_GETSET_STR(From, m_header[HEADER_FROM]);
    NS_IMPL_CLASS_GETSET_STR(Date, m_header[HEADER_DATE]);
    NS_IMPL_CLASS_GETSET_STR(Subject, m_header[HEADER_SUBJECT]);
    
    NS_IMETHOD AddNewsgroup(const char *newsgroupName);
    NS_IMPL_CLASS_GETTER_STR(GetNewsgroups, m_header[HEADER_NEWSGROUPS]);
    
    NS_IMETHOD GetMessageID(char * *aMessageID);
    
    NS_IMPL_CLASS_GETSET_STR(Path, m_header[HEADER_PATH]);

    // Optional Headers
    NS_IMPL_CLASS_GETSET_STR(ReplyTo, m_header[HEADER_REPLYTO]);
    NS_IMPL_CLASS_GETSET_STR(Sender, m_header[HEADER_SENDER]);
    NS_IMPL_CLASS_GETSET_STR(FollowupTo, m_header[HEADER_FOLLOWUPTO]);
    NS_IMPL_CLASS_GETSET_STR(DateRecieved, m_header[HEADER_DATERECEIVED]);
    NS_IMPL_CLASS_GETSET_STR(Expires, m_header[HEADER_EXPIRES]);
    NS_IMPL_CLASS_GETSET_STR(Control, m_header[HEADER_CONTROL]);
    NS_IMPL_CLASS_GETSET_STR(Distribution, m_header[HEADER_DISTRIBUTION]);
    NS_IMPL_CLASS_GETSET_STR(Organization, m_header[HEADER_ORGANIZATION]);
    NS_IMPL_CLASS_GETSET_STR(Body, m_body);    

    NS_IMETHOD AddReference(const char *referenceID);
    NS_IMPL_CLASS_GETTER_STR(GetReferences, m_header[HEADER_REFERENCES]);


    NS_IMETHOD IsValid(PRBool *_retval);
    
    NS_IMETHOD MakeControlCancel(const char *messageID);

    NS_IMPL_CLASS_GETTER(GetIsControl, PRBool, m_isControl);

    NS_IMETHOD GetFullMessage(char **message);

    // the message can be stored in a file....allow accessors for getting and setting
	// the file name to post...
	NS_IMETHOD SetPostMessageFile(nsIFileSpec * aFileName);
	NS_IMETHOD GetPostMessageFile(nsIFileSpec ** aFileName);
    
    // helper routines
    static char *AppendAndAlloc(char *string, const char *newSubstring,
                                PRBool withComma);
private:
    nsCOMPtr<nsIFileSpec>  m_postMessageFile;
    char *m_header[HEADER_LAST+1];
    static const char *m_headerName[HEADER_LAST+1];
    
    char *m_body;
    char *m_messageBuffer;
    PRBool m_isControl;
};

#endif /* __nsNNTPNewsgroupPost_h */
