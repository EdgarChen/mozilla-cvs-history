/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef nsJSUtils_h__
#define nsJSUtils_h__

/**
 * This is not a generated file. It contains common utility functions 
 * invoked from the JavaScript code generated from IDL interfaces.
 * The goal of the utility functions is to cut down on the size of
 * the generated code itself.
 */

#include "nsISupports.h"
#include "jsapi.h"
#include "nsString.h"
#include "nsCOMPtr.h"

class nsIDOMEventListener;
class nsIScriptContext;
class nsIScriptGlobalObject;
class nsIDOMGCParticipant;
class nsIXPConnectJSObjectHolder;

class nsJSUtils
{
public:
  static JSBool GetCallingLocation(JSContext* aContext, const char* *aFilename,
                                   PRUint32 *aLineno);

  static jsval ConvertStringToJSVal(const nsString& aProp,
                                    JSContext* aContext);

  static void ConvertJSValToString(nsAString& aString,
                                   JSContext* aContext, jsval aValue);

  static PRBool ConvertJSValToUint32(PRUint32* aProp, JSContext* aContext,
                                     jsval aValue);

  static nsIScriptGlobalObject *GetStaticScriptGlobal(JSContext* aContext,
                                                      JSObject* aObj);

  static nsIScriptContext *GetStaticScriptContext(JSContext* aContext,
                                                  JSObject* aObj);

  static nsIScriptGlobalObject *GetDynamicScriptGlobal(JSContext *aContext);

  static nsIScriptContext *GetDynamicScriptContext(JSContext *aContext);
};


class nsDependentJSString : public nsDependentString
{
public:
  explicit nsDependentJSString(jsval v)
    : nsDependentString((PRUnichar *)::JS_GetStringChars(JSVAL_TO_STRING(v)),
                        ::JS_GetStringLength(JSVAL_TO_STRING(v)))
  {
  }

  explicit nsDependentJSString(JSString *str)
    : nsDependentString((PRUnichar *)::JS_GetStringChars(str), ::JS_GetStringLength(str))
  {
  }

  ~nsDependentJSString()
  {
  }
};

/**
 * nsMarkedJSFunctionHolder<T> is used to store objects of XPCOM
 * interface T.
 *
 * If the object stored is an XPConnect wrapped JS object and the
 * wrapper can be preserved through nsDOMClassInfo, the holder will hold
 * a weak reference and preserve the object from garbage collection as
 * long as the garbage collector can reach |aParticipant|; once both
 * |aParticipant| and the object are unreachable it will be garbage
 * collected and the holder will hold null.
 *
 * Otherwise, it holds a strong reference.
 */

class nsMarkedJSFunctionHolder_base
{
public:
  void Set(nsISupports *aPotentialFunction, nsIDOMGCParticipant *aParticipant);
  already_AddRefed<nsISupports> Get(REFNSIID aIID);

  nsMarkedJSFunctionHolder_base() : mObject(nsnull) {}
  ~nsMarkedJSFunctionHolder_base();

  PRBool TryMarkedSet(nsISupports *aPotentialFunction, nsIDOMGCParticipant *aParticipant);

  nsISupports *mObject;
};

template <class T>
class nsMarkedJSFunctionHolder : protected nsMarkedJSFunctionHolder_base
{
public:
  void Set(T *aPotentialFunction, nsIDOMGCParticipant *aParticipant) {
    nsMarkedJSFunctionHolder_base::Set(aPotentialFunction, aParticipant);
  }
  already_AddRefed<T> Get() {
    return already_AddRefed<T>(NS_STATIC_CAST(T*, nsMarkedJSFunctionHolder_base::Get(NS_GET_TEMPLATE_IID(T)).get()));
  }
  // An overloaded version that's more useful for XPCOM getters
  void Get(T** aResult) {
    *aResult = Get().get();
  }
};

#endif /* nsJSUtils_h__ */
