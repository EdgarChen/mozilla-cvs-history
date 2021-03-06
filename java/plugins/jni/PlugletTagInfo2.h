/* 
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Sun Microsystems,
 * Inc. Portions created by Sun are
 * Copyright (C) 1999 Sun Microsystems, Inc. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */
#ifndef __PlugletTagInfo2_h__
#define __PlugletTagInfo2_h__
#include "nsIPluginTagInfo2.h"
#include "jni.h"

class PlugletTagInfo2 {
 public:
    static jobject GetJObject(JNIEnv* env,const nsIPluginTagInfo2 *info);
 private:
    static void Initialize(JNIEnv* env);
    static void Destroy(JNIEnv *env);
    static jclass    clazz;
    static jmethodID initMID;

};
#endif /*  __PlugletTagInfo2__h__ */

