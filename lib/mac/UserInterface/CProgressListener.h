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


// CProgressListener.h

#pragma once

#include <LListener.h>

class LCaption;
class LBroadcaster;
class CProgressCaption;

//======================================
class CProgressListener : public LListener
//======================================
{
public:
	// We have three kinds of information displayed by the Progress Bar:
	// - graphical status level showing a percent value (msg_NSCProgressPercentChanged)
	// - text message showing a percent value (msg_NSCProgressUpdate)
	// - general purpose text message (msg_NSCProgressMessageChanged)
	//
	// The first two messages (those about the percent value) are always filtered
	// in order to avoid flickers generated by frequent updates.
	//
	// The last one is not filtered by default: it allows for instance
	// to keep track of the mouse cursor over an HTML page and display as fast
	// as possible the URLs corresponding to the links pointed by the cursor.
	//
	// The first level of laziness (lazy_JustABit) filters frequent updates of
	// the text messages and allows to go much faster when loading a mail.
	//
	// The second level of laziness (lazy_VeryButForThisCommandOnly) filters
	// the normal text messages and completely ignores the text messages showing
	// a percent value. It allows to go much faster when downloading news articles.
	// This mode is automatically reset to the previous one when the command has completed.
	//
	enum ProgressBarLaziness
	{
		lazy_NotAtAll = 1,
			// display all messages, still filter frequent refresh of the progress bar
		lazy_JustABit,
			// filter 
		lazy_VeryButForThisCommandOnly
	};

	CProgressListener(CProgressCaption* progressCaption)
	: mProgressCaption(progressCaption) {}
	CProgressListener(LView* superview, LBroadcaster* broadcaster);
		// The broadcaster is the one to listen to.
	~CProgressListener();
	void ListenToMessage(MessageT inMessage, void *ioParam);
	void SetLaziness(ProgressBarLaziness inLaziness);

protected:
	CProgressCaption*		mProgressCaption;
	unsigned long			mProgressLastTicks;
	unsigned long			mMessageLastTicks;
	unsigned long			mPercentLastTicks;
	ProgressBarLaziness		mLaziness;
	ProgressBarLaziness		mPreviousLaziness;

}; // class ProgressListener
