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

#include <LView.h>
#include <LCommander.h>
#include <LListener.h>
#include <LArray.h>

class CTabSwitcher :
		public LView,
		public LCommander,
		public LListener
{
	public:
		enum { class_ID = 'TbSw' };
							CTabSwitcher(LStream* inStream);
		virtual				~CTabSwitcher();
	
		virtual void		ListenToMessage(
									MessageT		inMessage,
									void*			ioParam);

		virtual void		SwitchToPage(ResIDT inPageResID);
		virtual LView*		FindPageByID(ResIDT inPageResID);
		virtual LView*		GetCurrentPage(void) const;
		
		virtual	void		DoPreDispose(
									LView* 			inLeavingPage,
									Boolean			inWillCache);
									
		virtual	void		DoPostLoad(
									LView* 			inLoadedPage,
									Boolean			inFromCache);
	
	protected:
		virtual	void		FinishCreateSelf(void);

		virtual LView*		FetchPageFromCache(ResIDT inPageResID);
		virtual void		RemovePageFromCache(LView* inPage);

		virtual	void		FlushCachedPages(void);

		PaneIDT				mTabControlID;
		PaneIDT				mContainerID;
		Boolean				mIsCachingPages;
		
		LArray				mCachedPages;
		Int32				mSavedValue;
		LView*				mCurrentPage;
};

inline LView* CTabSwitcher::GetCurrentPage(void) const
	{	return mCurrentPage;		}
