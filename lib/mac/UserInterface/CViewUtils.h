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

// ===========================================================================
//	by Mark G. Young
// ===========================================================================

#ifndef CViewUtils_H
#define CViewUtils_H
#pragma once

#include <LPane.h>
#include <LView.h>


class CViewUtils
{
public:
	static LView*	GetTopMostSuperView(
								LPane*			inPane);

	static void		SetRect32(
								SRect32*		outRect,
								Int32			inLeft,
								Int32			inTop,
								Int32			inRight,
								Int32			inBottom);
								
	static void		ImageToLocalRect(
								LView*			inView,
								const SRect32&	inRect,
								Rect*			outRect);
								
	static void		LocalToImageRect(	
								LView*			inView,
								const Rect&		inRect,
								SRect32*		outRect);
};

#endif