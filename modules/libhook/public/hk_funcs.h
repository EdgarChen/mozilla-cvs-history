/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */
#include "xp_core.h"
#include "hk_types.h"

XP_BEGIN_PROTOS

extern const char *HK_GetFunctionName(int32 hook_id, void *extra);
extern intn HK_Init(void);
extern intn HK_IsHook(int32 hook_id, void *extra);
extern intn HK_CallHook(int32 hook_id, void *extra, int32 window_id,
				char *hook_str, char **hook_ret);
extern void HK_ReadHookFile(char *filename);
XP_END_PROTOS

