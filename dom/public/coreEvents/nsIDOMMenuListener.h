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


#ifndef nsIDOMMenuListener_h__
#define nsIDOMMenuListener_h__

#include "nsIDOMEvent.h"
#include "nsIDOMEventListener.h"

// {0730C841-42F3-11d3-97FA-00400553EEF0}
#define NS_IDOMMENULISTENER_IID \
{ 0x730c841, 0x42f3, 0x11d3, { 0x97, 0xfa, 0x0, 0x40, 0x5, 0x53, 0xee, 0xf0 } }

class nsIDOMMenuListener : public nsIDOMEventListener {

public:
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_IDOMMENULISTENER_IID)

  NS_IMETHOD Create(nsIDOMEvent* aEvent) = 0;
  NS_IMETHOD Close(nsIDOMEvent* aEvent) = 0;
  NS_IMETHOD Destroy(nsIDOMEvent* aEvent) = 0;
  NS_IMETHOD Action(nsIDOMEvent* aEvent) = 0;
  NS_IMETHOD Broadcast(nsIDOMEvent* aEvent) = 0;
  NS_IMETHOD CommandUpdate(nsIDOMEvent* aEvent) = 0;
  NS_IMETHOD Overflow(nsIDOMEvent* aEvent) = 0;
  NS_IMETHOD Underflow(nsIDOMEvent* aEvent) = 0;
};

#endif // nsIDOMMenuListener_h__
