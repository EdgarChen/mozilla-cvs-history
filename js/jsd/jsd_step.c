/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * JavaScript Debugging support - Stepping support
 */

#include "jsd.h"

/*
* #define JSD_TRACE 1
*/

#ifdef JSD_TRACE

static char*
_indentSpaces(int i)
{
#define MAX_INDENT 63
    static char* p = NULL;
    if(!p)
    {
        p = calloc(1, MAX_INDENT+1);
        if(!p) return "";
        memset(p, ' ', MAX_INDENT);
    }
    if(i > MAX_INDENT) return p;
    return p + MAX_INDENT-i;
}

static void
_interpreterTrace(JSDContext* jsdc, JSContext *cx, JSStackFrame *fp,
                  JSBool before, JSBool *ok)
{
    JSDScript* jsdscript = NULL;
    JSScript * script;
    static indent = 0;
    char* buf;
    const char* funName = NULL;

    script = JS_GetFrameScript(cx, fp);
    if(script)
    {
        JSD_LOCK_SCRIPTS(jsdc);
        jsdscript = jsd_FindJSDScript(jsdc, script);
        JSD_UNLOCK_SCRIPTS(jsdc);
        if(jsdscript)
            funName = JSD_GetScriptFunctionName(jsdc, jsdscript);
    }
    if(!funName)
        funName = "TOP_LEVEL";

    if(before)
    {
        buf = JS_smprintf("%sentering %s %s this: %0x\n",
                _indentSpaces(indent++),
                funName,
                JS_IsContructorFrame(cx, fp) ? "constructing":"",
                (int)JS_GetFrameThis(cx, fp));
    }
    else
    {
        buf = JS_smprintf("%sleaving %s\n",
                _indentSpaces(--indent),
                funName);
    }
    JS_ASSERT(indent >= 0);

    if(!buf)
        return;

    printf(buf);
    free(buf);
}
#endif

void * JS_DLL_CALLBACK
jsd_InterpreterHook(JSContext *cx, JSStackFrame *fp, JSBool before,
                    JSBool *ok, void *closure)
{
    JSDContext*     jsdc = (JSDContext*) closure;

    if( ! jsdc || ! jsdc->inited )
        return NULL;

    if(before && JS_IsContructorFrame(cx, fp))
        jsd_Constructing(jsdc, cx, JS_GetFrameThis(cx, fp), fp);

#ifdef JSD_TRACE
    _interpreterTrace(jsdc, cx, fp, before, ok);
    return closure;
#else
    return NULL;
#endif
/*
*   use this before calling any hook...
*     if( JSD_IS_DANGEROUS_THREAD(jsdc) )
*         return NULL;
*/

}
