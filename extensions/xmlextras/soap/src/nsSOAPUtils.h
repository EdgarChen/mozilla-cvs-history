/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
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
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef nsSOAPUtils_h__
#define nsSOAPUtils_h__

#include "nsIDOMElement.h"
#include "nsString.h"
#include "jsapi.h"

class nsSOAPUtils {
public:
  static void GetFirstChildElement(nsIDOMElement* aParent, 
                                   nsIDOMElement** aElement);
  static void GetNextSiblingElement(nsIDOMElement* aStart, 
                                    nsIDOMElement** aElement);
  static void GetElementTextContent(nsIDOMElement* aElement, 
                                    nsString& aText);
  static PRBool HasChildElements(nsIDOMElement* aElement);
  static void GetInheritedEncodingStyle(nsIDOMElement* aEntry, 
                                        char** aEncodingStyle);
  static JSContext* GetSafeContext();
  static JSContext* GetCurrentContext();
  static nsresult ConvertValueToJSVal(JSContext* aContext, 
                                      nsISupports* aValue, 
                                      JSObject* aJSValue, 
                                      PRInt32 aType,
                                      jsval* vp);
  static nsresult ConvertJSValToValue(JSContext* aContext,
                                      jsval val, 
                                      nsISupports** aValue,
                                      JSObject** aJSValue,
                                      PRInt32* aType);

  static const char* kSOAPEnvURI;
  static const char* kSOAPEncodingURI;
  static const char* kSOAPEnvPrefix;
  static const char* kSOAPEncodingPrefix;
  static const char* kXSIURI;
  static const char* kXSDURI;
  static const char* kXSIPrefix;
  static const char* kXSDPrefix;
  static const char* kEncodingStyleAttribute;
  static const char* kEnvelopeTagName;
  static const char* kHeaderTagName;
  static const char* kBodyTagName;
  static const char* kFaultTagName;
  static const char* kFaultCodeTagName;
  static const char* kFaultStringTagName;
  static const char* kFaultActorTagName;
  static const char* kFaultDetailTagName;
};

#endif
