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

/*
	Created 3/20/96 - Tim Craycroft

*/

#pragma once

#include "LTableHeader.h"

class CSpecialTableView;
class LTableViewHeader : public LTableHeader
{
public:
	enum { class_ID = 'tbvH' };
	
	LTableViewHeader(LStream *inStream);

	virtual void	FinishCreateSelf();		
	virtual void	ResizeImageBy(Int32 inWidthDelta, Int32 inHeightDelta, Boolean inRefresh);
									
	inline	CSpecialTableView*	GetTableView() const { return mTableView; }	

	virtual void	ReadColumnState(LStream * inStream, Boolean inMoveHeaders = true);
			void	ChangeIconOfColumn(PaneIDT	inColumn, ResIDT	inIconID);
			
protected:

			SInt16	GetGlobalBottomOfTable() const;

	virtual void	ComputeResizeDragRect(UInt16 inLeftColumn, Rect	&outDragRect);
	virtual void	ComputeColumnDragRect(UInt16 inLeftColumn, Rect &outDragRect);
	
	virtual void	RedrawColumns(UInt16 inFrom, UInt16 inTo);								
	virtual void	ShowHideRightmostColumn(Boolean inShow);


	CSpecialTableView*	mTableView;
	PaneIDT			mTableViewID;
		
};
