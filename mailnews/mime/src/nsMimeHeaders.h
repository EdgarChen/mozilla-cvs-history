/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
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
 * Copyright (C) 1999 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#ifndef nsMimeHeaders_h_
#define nsMimeHeaders_h_

#include "msgCore.h"    // precompiled header...
#include "mimehdrs.h"
#include "nsISupports.h"
#include "nsIMimeHeaders.h"

class nsMimeHeaders : public nsIMimeHeaders
{
 public: 
 	nsMimeHeaders();
 	virtual ~nsMimeHeaders();
 
	 /* this macro defines QueryInterface, AddRef and Release for this class */
	 NS_DECL_ISUPPORTS
	 
	 NS_DECL_NSIMIMEHEADERS

private:
	MimeHeaders	*	mHeaders;
};

#endif
