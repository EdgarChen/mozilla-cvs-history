/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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


/*
 *  Prototypes for functions exported by OJI based libplugin and called by the FEs or other XP libs.
 */

#ifndef _NP2_H
#define _NP2_H

#include "jni.h"
#include "lo_ele.h"

PR_EXTERN(const char *) NPL_GetText(LO_CommonPluginStruct* embed);
PR_EXTERN(jobject) NPL_GetJavaObject(LO_CommonPluginStruct* embed);

#endif /* _NP2_H */

