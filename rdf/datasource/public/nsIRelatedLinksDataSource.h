/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#ifndef	nsIRDFRelatedLinks_h__
#define	nsIRDFRelatedLinks_h__

#include "nsIRDFDataSource.h"
#include "nsIStreamListener.h"

#define NS_IRDFRELATEDLINKSDATAOURCE_IID \
{ 0xdc2fb181, 0xf7ff, 0x11d2, { 0x98, 0x20, 0xf6, 0x5e, 0xa6, 0x52, 0xae, 0x3c } }

class nsIRDFRelatedLinksDataSource : public nsIRDFDataSource
{
public:
	NS_IMETHOD	SetRelatedLinksURL(const char *url) = 0;
};


#define NS_IRDFRELATEDLINKSDATASOURCECALLBACK_IID \
{ 0xdc2fb183, 0xf7ff, 0x11d2, { 0x98, 0x20, 0xf6, 0x5e, 0xa6, 0x52, 0xae, 0x3c } }

class nsIRDFRelatedLinksDataSourceCallback : public nsIStreamListener
{
public:
};


#endif // nsIRDFRelatedLinks_h__
