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

#ifndef nsIDOMHTMLTableSectionElement_h__
#define nsIDOMHTMLTableSectionElement_h__

#include "nsISupports.h"
#include "nsString.h"
#include "nsIScriptContext.h"
#include "nsIDOMHTMLElement.h"

class nsIDOMHTMLElement;
class nsIDOMHTMLCollection;

#define NS_IDOMHTMLTABLESECTIONELEMENT_IID \
{ 0x6f765322,  0xee43, 0x11d1, \
 { 0x9b, 0xc3, 0x00, 0x60, 0x08, 0x8c, 0xa6, 0xb3 } } 

class nsIDOMHTMLTableSectionElement : public nsIDOMHTMLElement {
public:

  NS_IMETHOD    GetAlign(nsString& aAlign)=0;
  NS_IMETHOD    SetAlign(const nsString& aAlign)=0;

  NS_IMETHOD    GetVAlign(nsString& aVAlign)=0;
  NS_IMETHOD    SetVAlign(const nsString& aVAlign)=0;

  NS_IMETHOD    GetRows(nsIDOMHTMLCollection** aRows)=0;
  NS_IMETHOD    SetRows(nsIDOMHTMLCollection* aRows)=0;

  NS_IMETHOD    InsertRow(PRInt32 aIndex, nsIDOMHTMLElement** aReturn)=0;

  NS_IMETHOD    DeleteRow(PRInt32 aIndex)=0;
};


#define NS_DECL_IDOMHTMLTABLESECTIONELEMENT   \
  NS_IMETHOD    GetAlign(nsString& aAlign);  \
  NS_IMETHOD    SetAlign(const nsString& aAlign);  \
  NS_IMETHOD    GetVAlign(nsString& aVAlign);  \
  NS_IMETHOD    SetVAlign(const nsString& aVAlign);  \
  NS_IMETHOD    GetRows(nsIDOMHTMLCollection** aRows);  \
  NS_IMETHOD    SetRows(nsIDOMHTMLCollection* aRows);  \
  NS_IMETHOD    InsertRow(PRInt32 aIndex, nsIDOMHTMLElement** aReturn);  \
  NS_IMETHOD    DeleteRow(PRInt32 aIndex);  \



#define NS_FORWARD_IDOMHTMLTABLESECTIONELEMENT(superClass)  \
  NS_IMETHOD    GetAlign(nsString& aAlign) { return superClass::GetAlign(aAlign); } \
  NS_IMETHOD    SetAlign(const nsString& aAlign) { return superClass::SetAlign(aAlign); } \
  NS_IMETHOD    GetVAlign(nsString& aVAlign) { return superClass::GetVAlign(aVAlign); } \
  NS_IMETHOD    SetVAlign(const nsString& aVAlign) { return superClass::SetVAlign(aVAlign); } \
  NS_IMETHOD    GetRows(nsIDOMHTMLCollection** aRows) { return superClass::GetRows(aRows); } \
  NS_IMETHOD    SetRows(nsIDOMHTMLCollection* aRows) { return superClass::SetRows(aRows); } \
  NS_IMETHOD    InsertRow(PRInt32 aIndex, nsIDOMHTMLElement** aReturn) { return superClass::InsertRow(aIndex, aReturn); }  \
  NS_IMETHOD    DeleteRow(PRInt32 aIndex) { return superClass::DeleteRow(aIndex); }  \


extern nsresult NS_InitHTMLTableSectionElementClass(nsIScriptContext *aContext, void **aPrototype);

extern "C" NS_DOM nsresult NS_NewScriptHTMLTableSectionElement(nsIScriptContext *aContext, nsIDOMHTMLTableSectionElement *aSupports, nsISupports *aParent, void **aReturn);

#endif // nsIDOMHTMLTableSectionElement_h__
