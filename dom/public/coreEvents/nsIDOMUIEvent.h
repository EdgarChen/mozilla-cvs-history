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
/* AUTO-GENERATED. DO NOT EDIT!!! */

#ifndef nsIDOMUIEvent_h__
#define nsIDOMUIEvent_h__

#include "nsISupports.h"
#include "nsString.h"
#include "nsIScriptContext.h"
#include "nsIDOMEvent.h"


#define NS_IDOMUIEVENT_IID \
 { 0xa6cf90c3, 0x15b3, 0x11d2, \
  { 0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32 } } 

class nsIDOMUIEvent : public nsIDOMEvent {
public:
  static const nsIID& GetIID() { static nsIID iid = NS_IDOMUIEVENT_IID; return iid; }
  enum {
    DOM_VK_CANCEL = 3,
    DOM_VK_BACK = 8,
    DOM_VK_TAB = 9,
    DOM_VK_CLEAR = 12,
    DOM_VK_RETURN = 13,
    DOM_VK_ENTER = 14,
    DOM_VK_SHIFT = 16,
    DOM_VK_CONTROL = 17,
    DOM_VK_ALT = 18,
    DOM_VK_PAUSE = 19,
    DOM_VK_CAPS_LOCK = 20,
    DOM_VK_ESCAPE = 27,
    DOM_VK_SPACE = 32,
    DOM_VK_PAGE_UP = 33,
    DOM_VK_PAGE_DOWN = 34,
    DOM_VK_END = 35,
    DOM_VK_HOME = 36,
    DOM_VK_LEFT = 37,
    DOM_VK_UP = 38,
    DOM_VK_RIGHT = 39,
    DOM_VK_DOWN = 40,
    DOM_VK_PRINTSCREEN = 44,
    DOM_VK_INSERT = 45,
    DOM_VK_DELETE = 46,
    DOM_VK_0 = 48,
    DOM_VK_1 = 49,
    DOM_VK_2 = 50,
    DOM_VK_3 = 51,
    DOM_VK_4 = 52,
    DOM_VK_5 = 53,
    DOM_VK_6 = 54,
    DOM_VK_7 = 55,
    DOM_VK_8 = 56,
    DOM_VK_9 = 57,
    DOM_VK_SEMICOLON = 59,
    DOM_VK_EQUALS = 61,
    DOM_VK_A = 65,
    DOM_VK_B = 66,
    DOM_VK_C = 67,
    DOM_VK_D = 68,
    DOM_VK_E = 69,
    DOM_VK_F = 70,
    DOM_VK_G = 71,
    DOM_VK_H = 72,
    DOM_VK_I = 73,
    DOM_VK_J = 74,
    DOM_VK_K = 75,
    DOM_VK_L = 76,
    DOM_VK_M = 77,
    DOM_VK_N = 78,
    DOM_VK_O = 79,
    DOM_VK_P = 80,
    DOM_VK_Q = 81,
    DOM_VK_R = 82,
    DOM_VK_S = 83,
    DOM_VK_T = 84,
    DOM_VK_U = 85,
    DOM_VK_V = 86,
    DOM_VK_W = 87,
    DOM_VK_X = 88,
    DOM_VK_Y = 89,
    DOM_VK_Z = 90,
    DOM_VK_NUMPAD0 = 96,
    DOM_VK_NUMPAD1 = 97,
    DOM_VK_NUMPAD2 = 98,
    DOM_VK_NUMPAD3 = 99,
    DOM_VK_NUMPAD4 = 100,
    DOM_VK_NUMPAD5 = 101,
    DOM_VK_NUMPAD6 = 102,
    DOM_VK_NUMPAD7 = 103,
    DOM_VK_NUMPAD8 = 104,
    DOM_VK_NUMPAD9 = 105,
    DOM_VK_MULTIPLY = 106,
    DOM_VK_ADD = 107,
    DOM_VK_SEPARATOR = 108,
    DOM_VK_SUBTRACT = 109,
    DOM_VK_DECIMAL = 110,
    DOM_VK_DIVIDE = 111,
    DOM_VK_F1 = 112,
    DOM_VK_F2 = 113,
    DOM_VK_F3 = 114,
    DOM_VK_F4 = 115,
    DOM_VK_F5 = 116,
    DOM_VK_F6 = 117,
    DOM_VK_F7 = 118,
    DOM_VK_F8 = 119,
    DOM_VK_F9 = 120,
    DOM_VK_F10 = 121,
    DOM_VK_F11 = 122,
    DOM_VK_F12 = 123,
    DOM_VK_F13 = 124,
    DOM_VK_F14 = 125,
    DOM_VK_F15 = 126,
    DOM_VK_F16 = 127,
    DOM_VK_F17 = 128,
    DOM_VK_F18 = 129,
    DOM_VK_F19 = 130,
    DOM_VK_F20 = 131,
    DOM_VK_F21 = 132,
    DOM_VK_F22 = 133,
    DOM_VK_F23 = 134,
    DOM_VK_F24 = 135,
    DOM_VK_NUM_LOCK = 144,
    DOM_VK_SCROLL_LOCK = 145,
    DOM_VK_COMMA = 188,
    DOM_VK_PERIOD = 190,
    DOM_VK_SLASH = 191,
    DOM_VK_BACK_QUOTE = 192,
    DOM_VK_OPEN_BRACKET = 219,
    DOM_VK_BACK_SLASH = 220,
    DOM_VK_CLOSE_BRACKET = 221,
    DOM_VK_QUOTE = 222
  };

  NS_IMETHOD    GetScreenX(PRInt32* aScreenX)=0;

  NS_IMETHOD    GetScreenY(PRInt32* aScreenY)=0;

  NS_IMETHOD    GetClientX(PRInt32* aClientX)=0;

  NS_IMETHOD    GetClientY(PRInt32* aClientY)=0;

  NS_IMETHOD    GetAltKey(PRBool* aAltKey)=0;

  NS_IMETHOD    GetCtrlKey(PRBool* aCtrlKey)=0;

  NS_IMETHOD    GetShiftKey(PRBool* aShiftKey)=0;

  NS_IMETHOD    GetMetaKey(PRBool* aMetaKey)=0;

  NS_IMETHOD    GetCharCode(PRUint32* aCharCode)=0;

  NS_IMETHOD    GetKeyCode(PRUint32* aKeyCode)=0;

  NS_IMETHOD    GetButton(PRUint16* aButton)=0;

  NS_IMETHOD    GetClickcount(PRUint16* aClickcount)=0;
};


#define NS_DECL_IDOMUIEVENT   \
  NS_IMETHOD    GetScreenX(PRInt32* aScreenX);  \
  NS_IMETHOD    GetScreenY(PRInt32* aScreenY);  \
  NS_IMETHOD    GetClientX(PRInt32* aClientX);  \
  NS_IMETHOD    GetClientY(PRInt32* aClientY);  \
  NS_IMETHOD    GetAltKey(PRBool* aAltKey);  \
  NS_IMETHOD    GetCtrlKey(PRBool* aCtrlKey);  \
  NS_IMETHOD    GetShiftKey(PRBool* aShiftKey);  \
  NS_IMETHOD    GetMetaKey(PRBool* aMetaKey);  \
  NS_IMETHOD    GetCharCode(PRUint32* aCharCode);  \
  NS_IMETHOD    GetKeyCode(PRUint32* aKeyCode);  \
  NS_IMETHOD    GetButton(PRUint16* aButton);  \
  NS_IMETHOD    GetClickcount(PRUint16* aClickcount);  \



#define NS_FORWARD_IDOMUIEVENT(_to)  \
  NS_IMETHOD    GetScreenX(PRInt32* aScreenX) { return _to GetScreenX(aScreenX); } \
  NS_IMETHOD    GetScreenY(PRInt32* aScreenY) { return _to GetScreenY(aScreenY); } \
  NS_IMETHOD    GetClientX(PRInt32* aClientX) { return _to GetClientX(aClientX); } \
  NS_IMETHOD    GetClientY(PRInt32* aClientY) { return _to GetClientY(aClientY); } \
  NS_IMETHOD    GetAltKey(PRBool* aAltKey) { return _to GetAltKey(aAltKey); } \
  NS_IMETHOD    GetCtrlKey(PRBool* aCtrlKey) { return _to GetCtrlKey(aCtrlKey); } \
  NS_IMETHOD    GetShiftKey(PRBool* aShiftKey) { return _to GetShiftKey(aShiftKey); } \
  NS_IMETHOD    GetMetaKey(PRBool* aMetaKey) { return _to GetMetaKey(aMetaKey); } \
  NS_IMETHOD    GetCharCode(PRUint32* aCharCode) { return _to GetCharCode(aCharCode); } \
  NS_IMETHOD    GetKeyCode(PRUint32* aKeyCode) { return _to GetKeyCode(aKeyCode); } \
  NS_IMETHOD    GetButton(PRUint16* aButton) { return _to GetButton(aButton); } \
  NS_IMETHOD    GetClickcount(PRUint16* aClickcount) { return _to GetClickcount(aClickcount); } \


extern "C" NS_DOM nsresult NS_InitUIEventClass(nsIScriptContext *aContext, void **aPrototype);

extern "C" NS_DOM nsresult NS_NewScriptUIEvent(nsIScriptContext *aContext, nsISupports *aSupports, nsISupports *aParent, void **aReturn);

#endif // nsIDOMUIEvent_h__
