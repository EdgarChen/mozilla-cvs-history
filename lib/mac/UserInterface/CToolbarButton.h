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

// This is a subclass of CButton that can draw itself in three states:
// text and icon, text only, or icon only.

// ******** NOTE! ********
// So that we don't disturb any PPob's, this class assumes that the size of
// the button in the PPob is the size to use for the text and icon mode.
// For the other modes, there are "magic" constants (in CToolbarButton.cp)
// that define the frame dimensions of the button.

#pragma once

#include "CButton.h"

class CToolbarButton
	:	public CButton
{
	public:

		// button types enum is in CToolbarModeManager.h
		
		enum {	class_ID = 'TbBt' };
		
							CToolbarButton(LStream* inStream);
		virtual				~CToolbarButton();

		virtual Boolean		ChangeMode(Int8 newMode, SDimension16& outDimensionDeltas);

	protected:
		virtual	void		DrawSelf();
		virtual void		FinishCreateSelf();

		virtual	void		DrawButtonTitle(void);

		virtual	void		CalcTitleFrame(void);

		Int8		mCurrentMode;
		Int16		mOriginalWidth;
		Int16		mOriginalHeight;
};