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

#ifndef nsIDOMHTMLImageElement_h__
#define nsIDOMHTMLImageElement_h__

#include "nsISupports.h"
#include "nsString.h"
#include "nsIScriptContext.h"
#include "nsIDOMHTMLElement.h"

class nsIDOMHTMLImageElement;

#define NS_IDOMHTMLIMAGEELEMENT_IID \
{ 0x6f765306,  0xee43, 0x11d1, \
 { 0x9b, 0xc3, 0x00, 0x60, 0x08, 0x8c, 0xa6, 0xb3 } } 

class nsIDOMHTMLImageElement : public nsIDOMHTMLElement {
public:

  NS_IMETHOD    GetLowSrc(nsString& aLowSrc)=0;
  NS_IMETHOD    SetLowSrc(const nsString& aLowSrc)=0;

  NS_IMETHOD    GetName(nsString& aName)=0;
  NS_IMETHOD    SetName(const nsString& aName)=0;

  NS_IMETHOD    GetAlign(nsString& aAlign)=0;
  NS_IMETHOD    SetAlign(const nsString& aAlign)=0;

  NS_IMETHOD    GetAlt(nsString& aAlt)=0;
  NS_IMETHOD    SetAlt(const nsString& aAlt)=0;

  NS_IMETHOD    GetBorder(nsString& aBorder)=0;
  NS_IMETHOD    SetBorder(const nsString& aBorder)=0;

  NS_IMETHOD    GetHeight(nsString& aHeight)=0;
  NS_IMETHOD    SetHeight(const nsString& aHeight)=0;

  NS_IMETHOD    GetHspace(nsString& aHspace)=0;
  NS_IMETHOD    SetHspace(const nsString& aHspace)=0;

  NS_IMETHOD    GetIsMap(PRBool* aIsMap)=0;
  NS_IMETHOD    SetIsMap(PRBool aIsMap)=0;

  NS_IMETHOD    GetLongDesc(nsString& aLongDesc)=0;
  NS_IMETHOD    SetLongDesc(const nsString& aLongDesc)=0;

  NS_IMETHOD    GetSrc(nsString& aSrc)=0;
  NS_IMETHOD    SetSrc(const nsString& aSrc)=0;

  NS_IMETHOD    GetUseMap(nsString& aUseMap)=0;
  NS_IMETHOD    SetUseMap(const nsString& aUseMap)=0;

  NS_IMETHOD    GetVspace(nsString& aVspace)=0;
  NS_IMETHOD    SetVspace(const nsString& aVspace)=0;

  NS_IMETHOD    GetWidth(nsString& aWidth)=0;
  NS_IMETHOD    SetWidth(const nsString& aWidth)=0;
};

extern nsresult NS_InitHTMLImageElementClass(nsIScriptContext *aContext, void **aPrototype);

extern "C" NS_DOM nsresult NS_NewScriptHTMLImageElement(nsIScriptContext *aContext, nsIDOMHTMLImageElement *aSupports, nsISupports *aParent, void **aReturn);

#endif // nsIDOMHTMLImageElement_h__
