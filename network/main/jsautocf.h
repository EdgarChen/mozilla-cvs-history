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
/*
 * jsautocf.h: JavaScript auto-config parser and evaluator
 */

#ifndef MK_JAVASCRIPT_AUTO_CONFIG
#define MK_JAVASCRIPT_AUTO_CONFIG


#include "xp_mcom.h"
#include "xp.h"

/*
 * A stream constructor function for application/x-ns-javascript-autoconfig.
 *
 *
 *
 */
MODULE_PRIVATE NET_StreamClass *
NET_JavaScriptAutoConfig(int fmt, void *data_obj, URL_Struct *URL_s, MWContext *w);

MODULE_PRIVATE int NET_LoadJavaScriptConfig(char *autoconf_url,MWContext *window_id);

void jsacf_exit_routine(URL_Struct *URL_s, int status, MWContext *window_id);
PUBLIC char * NET_GetJavaScriptConfigSource(void);
PUBLIC XP_Bool NET_InitPacfContext(void);


#endif /* ! MK_JAVASCRIPT_AUTO_CONFIG */

