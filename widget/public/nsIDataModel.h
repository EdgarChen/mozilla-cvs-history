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
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#ifndef nsIDataModel_h___
#define nsIDataModel_h___

#include "nsISupports.h"
#include "prthread.h"
#include "nsString.h"

// {65FCD501-789E-11d2-96ED-00104B7B7DEB}
#define NS_IDATAMODEL_IID \
{ 0x65fcd501, 0x789e, 0x11d2, { 0x96, 0xed, 0x0, 0x10, 0x4b, 0x7b, 0x7d, 0xeb } }

class nsIDMItem;
class nsIDMWidget;

class nsIDataModel : public nsISupports
{

public:
	// Initializers
	NS_IMETHOD InitFromURL(const nsString& url) = 0;
	NS_IMETHOD InitFromResource(nsIDMItem* pResource) = 0;

	// Inspectors
	NS_IMETHOD GetDMWidget(nsIDMWidget*& pWidget) const = 0;
	
	NS_IMETHOD GetFirstVisibleItemIndex(PRUint32& index) const = 0;
	NS_IMETHOD SetFirstVisibleItemIndex(PRUint32 index) = 0;

	NS_IMETHOD GetItemCount(PRUint32 count) const = 0;
	NS_IMETHOD GetNthItem(nsIDMItem*& pItem, PRUint32 n) const = 0;
	
	// Setters
	NS_IMETHOD SetDMWidget(nsIDMWidget* pWidget) = 0;

	// Methods to query the data model for property values for an entire widget.
	NS_IMETHOD GetStringPropertyValue(nsString& value, const nsString& property) const = 0;
	NS_IMETHOD GetIntPropertyValue(PRInt32& value, const nsString& property) const = 0;
};

#endif /* nsIDataModel_h___ */

