/* -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; -*- */
/*
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

#ifndef nsMacMessageSink_h__
#define nsMacMessageSink_h__

#include "prtypes.h"
class nsMacWindow;


/*================================================
•••IMPORTANT
Hacks to make Raptor coexist with other windows.
================================================*/

// This bit is set in the Raptor windows 'windowKind'
#define kRaptorWindowKindBit		0x4000


// An nsRefData pointer is stored in the Raptor windows 'refCon'
class nsRefData
{
protected:
	nsMacWindow*	mNSMacWindow;			// The nsMacWindow, ie. the top level widget
	long					mUserData;				// Embedding applications data, guaranteed not to be used by widget code

public:
	nsMacWindow*	GetNSMacWindow()	{return(mNSMacWindow);}
	long					GetUserData()			{return(mUserData);}

	void					SetNSMacWindow(nsMacWindow* aNSMacWindow)	{mNSMacWindow = aNSMacWindow;}
	void					SetUserData(long aUserData)								{mUserData = aUserData;}
};



/*================================================
•••IMPORTANT
This class should be COM'd and made XP somehow
Right now it is just a normal class as the first step of breaking
it out from the "client" code. It's goal is to be the point of contact between
client code (say, the viewer) and the embedded webshell for event dispatching. From
here, it goes to the appropriate window, however that may be done (and that still
needs to be figured out).
(pinkerton)
================================================*/

// Macintosh Message Sink Class
class nsMacMessageSink
{		    	    
public:
	void			DispatchOSEvent(EventRecord &anEvent, WindowPtr aWindow);
	void			DispatchMenuCommand(EventRecord &anEvent, long menuResult);

};


#endif // nsMacMessageSink_h__

