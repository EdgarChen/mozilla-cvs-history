/* 
 The contents of this file are subject to the Mozilla Public
 License Version 1.1 (the "License"); you may not use this file
 except in compliance with the License. You may obtain a copy of
 the License at http://www.mozilla.org/MPL/

 Software distributed under the License is distributed on an "AS
 IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 implied. See the License for the specific language governing
 rights and limitations under the License.

 The Original Code is mozilla.org code.

 The Initial Developer of the Original Code is Sun Microsystems,
 Inc. Portions created by Sun are
 Copyright (C) 1999 Sun Microsystems, Inc. All
 Rights Reserved.

 Contributor(s): 
*/

#ifndef __JavaDOMEventsGlobals_h__
#define __JavaDOMEventsGlobals_h__

#include"javaDOMGlobals.h"
#include"nsIDOMEvent.h"

class JavaDOMEventsGlobals {

 public:
  static jclass eventClass;
  static jclass eventListenerClass;
  static jclass eventTargetClass;
  static jclass uiEventClass;
  static jclass mutationEventClass;
  static jclass mouseEventClass;
  
  static jfieldID eventPtrFID;
  static jfieldID eventTargetPtrFID;

  static jfieldID eventPhaseBubblingFID;
  static jfieldID eventPhaseCapturingFID;
  static jfieldID eventPhaseAtTargetFID;   

  static jmethodID eventListenerHandleEventMID;

  static void Initialize(JNIEnv *env);
  static void Destroy(JNIEnv *env);
  static jobject CreateEventSubtype(JNIEnv *env, 
				   nsIDOMEvent *event);

  static jlong RegisterNativeEventListener();
  static jlong UnregisterNativeEventListener();
};

#endif /* __JavaDOMEventsGlobals_h__ */
