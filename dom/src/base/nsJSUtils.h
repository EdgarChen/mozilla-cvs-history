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

/**
 * This is not a generated file. It contains common utility functions 
 * invoked from the JavaScript code generated from IDL interfaces.
 * The goal of the utility functions is to cut down on the size of
 * the generated code itself.
 */

#include "nsISupports.h"
#include "jsapi.h"
#include "nsString.h"

PRBool nsCallJSScriptObjectGetProperty(nsISupports* aSupports,
				       JSContext* aContext,
				       jsval aId,
				       jsval* aReturn);

PRBool nsCallJSScriptObjectSetProperty(nsISupports* aSupports,
				       JSContext* aContext,
				       jsval aId,
				       jsval* aReturn);

void nsConvertObjectToJSVal(nsISupports* aSupports,
			    JSContext* aContext,
			    jsval* aReturn);

void nsConvertStringToJSVal(const nsString& aProp,
			    JSContext* aContext,
			    jsval* aReturn);

PRBool nsConvertJSValToObject(nsISupports** aSupports,
			      REFNSIID aIID,
			      const nsString& aTypeName,
			      JSContext* aContext,
			      jsval aValue);

void nsConvertJSValToString(nsString& aString,
			    JSContext* aContext,
			    jsval aValue);

PRBool nsConvertJSValToBool(PRBool* aProp,
			    JSContext* aContext,
			    jsval aValue);

void nsGenericFinalize(JSContext* aContext,
		       JSObject* aObj);
		     
JSBool nsGenericEnumerate(JSContext* aContext,
			  JSObject* aObj);

JSBool nsGenericResolve(JSContext* aContext,
			JSObject* aObj, 
			jsval aId);
