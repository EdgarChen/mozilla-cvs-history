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

#pragma once

#include <UAttachments.h>
#include "CStandardFlexTable.h"

class CSpecialTableView;

class CTableKeyAttachment : public LKeyScrollAttachment
{
	public:
							CTableKeyAttachment(CSpecialTableView* inView);
		virtual				~CTableKeyAttachment();
		
	protected:
		virtual void		SelectCell( const STableCell& inCell, Boolean multiple = false);
		
		virtual void		ExecuteSelf(
								MessageT		inMessage,
								void			*ioParam);

		CSpecialTableView*		mTableView; // safe cast of the view in the base class.
		
		Boolean					mGettingKeyUps;		// true if the app is receiving keyups
};

// a key attachment class that is optimized for row-selection tables
class CTableRowKeyAttachment : public CTableKeyAttachment
{
	public:
							CTableRowKeyAttachment(CSpecialTableView* inView);
		virtual				~CTableRowKeyAttachment();
		
	protected:

		virtual void		ExecuteSelf(
								MessageT		inMessage,
								void			*ioParam);
};