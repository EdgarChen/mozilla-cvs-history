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

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include <LStream.h>
#include <UTextTraits.h>
#include <UDrawingState.h>
#include <UDrawingUtils.h>

#include "CClusterView.h"
#include "UStdBevels.h"

// some private constants
const Int16 kClusterIndentSize = 13;
const Int16 kClusterTitleFudgeSize = 4;

// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ
//	₯	
// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ

CClusterView::CClusterView(LStream *inStream)
	:	LView(inStream)
{
	inStream->ReadPString(mTitle);
	inStream->ReadData(&mTraitsID, sizeof(ResIDT));
}

// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ
//	₯	
// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ

CClusterView::~CClusterView()
{
}

// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ
//	₯	
// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ

void CClusterView::FinishCreateSelf(void)
{
	LView::FinishCreateSelf();

	CalcTitleFrame();	
}

// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ
//	₯	
// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ

StringPtr CClusterView::GetDescriptor(Str255 outDescriptor) const
{
	return LString::CopyPStr(mTitle, outDescriptor);
}

// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ
//	₯	
// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ

void CClusterView::SetDescriptor(ConstStr255Param inDescriptor)
{
	mTitle = inDescriptor;
	CalcTitleFrame();
}

// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ
//	₯	
// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ

void CClusterView::DrawSelf(void)
{
	StColorPenState theStateSaver;
	theStateSaver.Normalize();
	
	Rect theFrame;
	CalcLocalFrameRect(theFrame);

	Rect theOutlineFrame = theFrame;
	theOutlineFrame.top += (mTitleFrame.bottom - mTitleFrame.top) / 2;

	UTextTraits::SetPortTextTraits(mTraitsID);

	Int16 theDepth;
	StDeviceLoop theLoop(theFrame);
	while (theLoop.NextDepth(theDepth))
		{
		if (theDepth > 4)
			{
			// Draw the frame in color
			::PmForeColor(eStdGray40);
			::PmBackColor(eStdGray53);
			}

		::PenPat(&qd.gray);			
		::FrameRect(&theOutlineFrame);
		
		if (mTitle.Length() > 0)
			{
			StColorState::Normalize();
			ApplyForeAndBackColors();
			::EraseRect(&mTitleFrame);
			::MoveTo(mTitleFrame.left + kClusterTitleFudgeSize, mTitleBaseline);
			::DrawText(mTitle, 1, mTitle.Length());
			}
		}
}

// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ
//	₯	
// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ

void CClusterView::CalcTitleFrame(void)
{
	if (mTitle.Length() == 0)
		return;
		
	Rect theFrame;
	CalcLocalFrameRect(theFrame);
	mTitleFrame = theFrame;

	Int16 theJust = UTextTraits::SetPortTextTraits(mTraitsID);
	Int16 theTextWidth = ::StringWidth(mTitle) + (2 * kClusterTitleFudgeSize);
	
	FontInfo theInfo;
	::GetFontInfo(&theInfo);
	Int16 theTextHeight = theInfo.ascent + theInfo.descent + theInfo.leading;
	
	mTitleBaseline = theFrame.top + theInfo.ascent;
	mTitleFrame.bottom = mTitleFrame.top + theTextHeight;
	
	switch (theJust)
		{
		case teCenter:
			mTitleFrame.left += ((theFrame.right - theFrame.left - theTextWidth) / 2);
			mTitleFrame.right = mTitleFrame.left + theTextWidth;
			break;
				
		case teFlushRight:
			mTitleFrame.right = theFrame.right - kClusterIndentSize;
			mTitleFrame.left = mTitleFrame.right - theTextWidth;
			break;

		case teFlushDefault:
		case teFlushLeft:
		default:
			mTitleFrame.left += kClusterIndentSize;
			mTitleFrame.right = mTitleFrame.left + theTextWidth;
			break;
		}
}
