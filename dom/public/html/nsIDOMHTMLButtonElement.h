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

#ifndef nsIDOMHTMLButtonElement_h__
#define nsIDOMHTMLButtonElement_h__

#include "nsISupports.h"
#include "nsString.h"
#include "nsIScriptContext.h"
#include "nsIDOMHTMLElement.h"

class nsIDOMHTMLFormElement;

#define NS_IDOMHTMLBUTTONELEMENT_IID \
{ 0x6f7652f5,  0xee43, 0x11d1, \
 { 0x9b, 0xc3, 0x00, 0x60, 0x08, 0x8c, 0xa6, 0xb3 } } 

class nsIDOMHTMLButtonElement : public nsIDOMHTMLElement {
public:

  NS_IMETHOD    GetForm(nsIDOMHTMLFormElement** aForm)=0;
  NS_IMETHOD    SetForm(nsIDOMHTMLFormElement* aForm)=0;

  NS_IMETHOD    GetAccessKey(nsString& aAccessKey)=0;
  NS_IMETHOD    SetAccessKey(const nsString& aAccessKey)=0;

  NS_IMETHOD    GetDisabled(PRBool* aDisabled)=0;
  NS_IMETHOD    SetDisabled(PRBool aDisabled)=0;

  NS_IMETHOD    GetName(nsString& aName)=0;
  NS_IMETHOD    SetName(const nsString& aName)=0;

  NS_IMETHOD    GetTabIndex(PRInt32* aTabIndex)=0;
  NS_IMETHOD    SetTabIndex(PRInt32 aTabIndex)=0;

  NS_IMETHOD    GetType(nsString& aType)=0;
  NS_IMETHOD    SetType(const nsString& aType)=0;

  NS_IMETHOD    GetValue(nsString& aValue)=0;
  NS_IMETHOD    SetValue(const nsString& aValue)=0;
};


#define NS_DECL_IDOMHTMLBUTTONELEMENT   \
  NS_IMETHOD    GetForm(nsIDOMHTMLFormElement** aForm);  \
  NS_IMETHOD    SetForm(nsIDOMHTMLFormElement* aForm);  \
  NS_IMETHOD    GetAccessKey(nsString& aAccessKey);  \
  NS_IMETHOD    SetAccessKey(const nsString& aAccessKey);  \
  NS_IMETHOD    GetDisabled(PRBool* aDisabled);  \
  NS_IMETHOD    SetDisabled(PRBool aDisabled);  \
  NS_IMETHOD    GetName(nsString& aName);  \
  NS_IMETHOD    SetName(const nsString& aName);  \
  NS_IMETHOD    GetTabIndex(PRInt32* aTabIndex);  \
  NS_IMETHOD    SetTabIndex(PRInt32 aTabIndex);  \
  NS_IMETHOD    GetType(nsString& aType);  \
  NS_IMETHOD    SetType(const nsString& aType);  \
  NS_IMETHOD    GetValue(nsString& aValue);  \
  NS_IMETHOD    SetValue(const nsString& aValue);  \



#define NS_FORWARD_IDOMHTMLBUTTONELEMENT(superClass)  \
  NS_IMETHOD    GetForm(nsIDOMHTMLFormElement** aForm) { return superClass::GetForm(aForm); } \
  NS_IMETHOD    SetForm(nsIDOMHTMLFormElement* aForm) { return superClass::SetForm(aForm); } \
  NS_IMETHOD    GetAccessKey(nsString& aAccessKey) { return superClass::GetAccessKey(aAccessKey); } \
  NS_IMETHOD    SetAccessKey(const nsString& aAccessKey) { return superClass::SetAccessKey(aAccessKey); } \
  NS_IMETHOD    GetDisabled(PRBool* aDisabled) { return superClass::GetDisabled(aDisabled); } \
  NS_IMETHOD    SetDisabled(PRBool aDisabled) { return superClass::SetDisabled(aDisabled); } \
  NS_IMETHOD    GetName(nsString& aName) { return superClass::GetName(aName); } \
  NS_IMETHOD    SetName(const nsString& aName) { return superClass::SetName(aName); } \
  NS_IMETHOD    GetTabIndex(PRInt32* aTabIndex) { return superClass::GetTabIndex(aTabIndex); } \
  NS_IMETHOD    SetTabIndex(PRInt32 aTabIndex) { return superClass::SetTabIndex(aTabIndex); } \
  NS_IMETHOD    GetType(nsString& aType) { return superClass::GetType(aType); } \
  NS_IMETHOD    SetType(const nsString& aType) { return superClass::SetType(aType); } \
  NS_IMETHOD    GetValue(nsString& aValue) { return superClass::GetValue(aValue); } \
  NS_IMETHOD    SetValue(const nsString& aValue) { return superClass::SetValue(aValue); } \


extern nsresult NS_InitHTMLButtonElementClass(nsIScriptContext *aContext, void **aPrototype);

extern "C" NS_DOM nsresult NS_NewScriptHTMLButtonElement(nsIScriptContext *aContext, nsIDOMHTMLButtonElement *aSupports, nsISupports *aParent, void **aReturn);

#endif // nsIDOMHTMLButtonElement_h__
