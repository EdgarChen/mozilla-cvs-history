/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#ifndef nsIXPFCObserver_h___
#define nsIXPFCObserver_h___

#include "nsISupports.h"
#include "nsGUIEvent.h"

class nsIXPFCSubject;
class nsIXPFCCommand;

//1ed118a0-1f53-11d2-bed9-00805f8a8dbd
#define NS_IXPFC_OBSERVER_IID   \
{ 0x1ed118a0, 0x1f53, 0x11d2,    \
{ 0xbe, 0xd9, 0x00, 0x80, 0x5f, 0x8a, 0x8d, 0xbd } }

class nsIXPFCObserver : public nsISupports
{

public:

  NS_IMETHOD Init() = 0;
  NS_IMETHOD_(nsEventStatus) Update(nsIXPFCSubject * aSubject, nsIXPFCCommand * aCommand) = 0;

};

#endif /* nsIXPFCObserver_h___ */
