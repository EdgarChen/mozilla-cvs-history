/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
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

#include "nsDOMEvent.h"
#include "nsIDOMNode.h"

static NS_DEFINE_IID(kIDOMNodeIID, NS_IDOMNODE_IID);
static NS_DEFINE_IID(kIDOMEventIID, NS_IDOMEVENT_IID);
static NS_DEFINE_IID(kIDOMNSEventIID, NS_IDOMNSEVENT_IID);


enum mEventEnum {
  onmousedown=0, onmouseup=1, onclick=2, ondblclick=3, onmouseover=4, onmouseout=5,
  onmousemove=6, onkeydown=7, onkeyup=8, onkeypress=9, onfocus=10, onblur=11,
  onload=12, onabort=13, onerror=14
};

static char* mEventNames[] = {
  "onmousedown", "onmouseup", "onclick", "ondblclick", "onmouseover", "onmouseout",
  "onmousemove", "onkeydown", "onkeyup", "onkeypress", "onfocus", "onblur", 
  "onload", "onabort", "onerror"
};

nsDOMEvent::nsDOMEvent(nsIPresContext* aPresContext) {
  kPresContext = aPresContext;
  NS_ADDREF(kPresContext);
}

nsDOMEvent::~nsDOMEvent() {
  NS_RELEASE(kPresContext);
}

NS_IMPL_ADDREF(nsDOMEvent)
NS_IMPL_RELEASE(nsDOMEvent)

nsresult nsDOMEvent::QueryInterface(const nsIID& aIID,
                                       void** aInstancePtrResult)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null pointer");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(kIDOMEventIID)) {
    *aInstancePtrResult = (void*) ((nsIDOMEvent*)this);
    AddRef();
    return NS_OK;
  }
  if (aIID.Equals(kIDOMNSEventIID)) {
    *aInstancePtrResult = (void*) ((nsIDOMNSEvent*)this);
    AddRef();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

// nsIDOMEventInterface
NS_METHOD nsDOMEvent::GetType(nsString& aType)
{
  const char* mName = GetEventName(kEvent->message);

  if (nsnull != mName) {
    aType = nsString(mName);
    return NS_OK;
  }
  
  return NS_ERROR_FAILURE;
}

NS_METHOD nsDOMEvent::SetType(const nsString& aType)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_METHOD nsDOMEvent::GetTarget(nsIDOMNode** aTarget)
{
  return kTarget->QueryInterface(kIDOMNodeIID, (void**)aTarget);
}

NS_METHOD nsDOMEvent::SetTarget(nsIDOMNode* aTarget)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_METHOD nsDOMEvent::GetScreenX(PRInt32* aScreenX)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_METHOD nsDOMEvent::SetScreenX(PRInt32 aScreenX)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_METHOD nsDOMEvent::GetScreenY(PRInt32* aScreenY)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_METHOD nsDOMEvent::SetScreenY(PRInt32 aScreenY)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_METHOD nsDOMEvent::GetClientX(PRInt32* aClientX)
{
  *aClientX = NS_TO_INT_ROUND(kEvent->point.x * kPresContext->GetTwipsToPixels());
  return NS_OK;
}

NS_METHOD nsDOMEvent::SetClientX(PRInt32 aClientX)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_METHOD nsDOMEvent::GetClientY(PRInt32* aClientY)
{
  *aClientY = NS_TO_INT_ROUND(kEvent->point.y * kPresContext->GetTwipsToPixels());
  return NS_OK;
}

NS_METHOD nsDOMEvent::SetClientY(PRInt32 aClientY)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_METHOD nsDOMEvent::GetAltKey(PRBool* aIsDown)
{
  *aIsDown = ((nsInputEvent*)kEvent)->isAlt;
  return NS_OK;
}

NS_METHOD nsDOMEvent::SetAltKey(PRBool aAltKey)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_METHOD nsDOMEvent::GetCtrlKey(PRBool* aIsDown)
{
  *aIsDown = ((nsInputEvent*)kEvent)->isControl;
  return NS_OK;
}

NS_METHOD nsDOMEvent::SetCtrlKey(PRBool aCtrlKey)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_METHOD nsDOMEvent::GetShiftKey(PRBool* aIsDown)
{
  *aIsDown = ((nsInputEvent*)kEvent)->isShift;
  return NS_OK;
}

NS_METHOD nsDOMEvent::SetShiftKey(PRBool aShiftKey)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_METHOD nsDOMEvent::GetMetaKey(PRBool* aIsDown)
{
  return NS_OK;
}

NS_METHOD nsDOMEvent::SetMetaKey(PRBool aMetaKey)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_METHOD nsDOMEvent::GetCharCode(PRUint32* aCharCode)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_METHOD nsDOMEvent::SetCharCode(PRUint32 aCharCode)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_METHOD nsDOMEvent::GetKeyCode(PRUint32* aKeyCode)
{
  switch (kEvent->message) {
  case NS_KEY_UP:
  case NS_KEY_DOWN:
    *aKeyCode = ((nsKeyEvent*)kEvent)->keyCode;
    break;
  default:
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

NS_METHOD nsDOMEvent::SetKeyCode(PRUint32 aKeyCode)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_METHOD nsDOMEvent::GetButton(PRUint32* aButton)
{
  switch (kEvent->message) {
  case NS_MOUSE_LEFT_BUTTON_UP:
  case NS_MOUSE_LEFT_BUTTON_DOWN:
  case NS_MOUSE_LEFT_DOUBLECLICK:
    *aButton = 1;
    break;
  case NS_MOUSE_MIDDLE_BUTTON_UP:
  case NS_MOUSE_MIDDLE_BUTTON_DOWN:
    *aButton = 2;
    break;
  case NS_MOUSE_RIGHT_BUTTON_UP:
  case NS_MOUSE_RIGHT_BUTTON_DOWN:
  case NS_MOUSE_RIGHT_DOUBLECLICK:
    *aButton = 3;
    break;
  default:
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

NS_METHOD nsDOMEvent::SetButton(PRUint32 aButton)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

// nsINSEventInterface
NS_METHOD nsDOMEvent::GetLayerX(PRInt32* aLayerX)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_METHOD nsDOMEvent::SetLayerX(PRInt32 aLayerX)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_METHOD nsDOMEvent::GetLayerY(PRInt32* aLayerY)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_METHOD nsDOMEvent::SetLayerY(PRInt32 aLayerY)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_METHOD nsDOMEvent::SetGUIEvent(nsGUIEvent *aEvent)
{
  kEvent = aEvent;
  return NS_OK;
}

NS_METHOD nsDOMEvent::SetEventTarget(nsISupports *aTarget)
{
  kTarget = aTarget;
  return NS_OK;
}

const char* nsDOMEvent::GetEventName(PRUint32 aEventType)
{
  switch(aEventType) {
  case NS_MOUSE_LEFT_BUTTON_DOWN:
  case NS_MOUSE_MIDDLE_BUTTON_DOWN:
  case NS_MOUSE_RIGHT_BUTTON_DOWN:
    return mEventNames[onmousedown];
    break;
  case NS_MOUSE_LEFT_BUTTON_UP:
  case NS_MOUSE_MIDDLE_BUTTON_UP:
  case NS_MOUSE_RIGHT_BUTTON_UP:
    return mEventNames[onmouseup];
    break;
  case NS_MOUSE_LEFT_DOUBLECLICK:
  case NS_MOUSE_RIGHT_DOUBLECLICK:
    return mEventNames[ondblclick];
    break;
  /*case NS_MOUSE_ENTER:
    return mEventNames[onmouseover];
    break;
  case NS_MOUSE_EXIT:
    return mEventNames[onmouseout];
    break;*/
  case NS_MOUSE_MOVE:
    return mEventNames[onmousemove];
    break;
  case NS_KEY_UP:
    return mEventNames[onkeyup];
    break;
  case NS_KEY_DOWN:
    return mEventNames[onkeydown];
    break;
  default:
    break;
  }
  return nsnull;
}
