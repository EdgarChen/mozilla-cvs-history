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

/* Platform specific code to invoke XPCOM methods on native objects */

#include "xptcprivate.h"

#if (_MIPS_SIM != _ABIN32)
#error "This code is for IRIX N32 only"
#endif

extern "C" uint32
invoke_count_words(PRUint32 paramCount, nsXPTCVariant* s)
{
    return(paramCount*2);
}

extern "C" void
invoke_copy_to_stack(PRUint64* d, PRUint32 paramCount,
                     nsXPTCVariant* s, PRUint64 *regs)
{
#define N_ARG_REGS       7       /* 8 regs minus 1 for "this" ptr */

    for (PRUint32 i = 0; i < paramCount; i++, s++)
    {
        if (s->IsPtrData()) {
            if (i < N_ARG_REGS)
               regs[i] = (PRUint64)s->ptr;
            else
               *d++ = (PRUint64)s->ptr; 
            continue;
        }
        switch (s->type) {
        //
        // signed types first
        //
        case nsXPTType::T_I8:
           if (i < N_ARG_REGS)
              ((PRInt64*)regs)[i] = s->val.i8;
           else
              *d++ = s->val.i8;
           break;
        case nsXPTType::T_I16:
           if (i < N_ARG_REGS)
              ((PRInt64*)regs)[i] = s->val.i16;
           else
              *d++ = s->val.i16;
           break;
        case nsXPTType::T_I32:
           if (i < N_ARG_REGS)
              ((PRInt64*)regs)[i] = s->val.i32;
           else
              *d++ = s->val.i32;
           break;
        case nsXPTType::T_I64:
           if (i < N_ARG_REGS)
              ((PRInt64*)regs)[i] = s->val.i64;
           else
              *d++ = s->val.i64;
           break;
        //
        // unsigned types next
        //
        case nsXPTType::T_U8:
           if (i < N_ARG_REGS)
              regs[i] = s->val.u8;
           else
              *d++ = s->val.u8;
           break;
        case nsXPTType::T_U16:
           if (i < N_ARG_REGS)
              regs[i] = s->val.u16;
           else
              *d++ = s->val.u16;
           break;
        case nsXPTType::T_U32:
           if (i < N_ARG_REGS)
              regs[i] = s->val.u32;
           else
              *d++ = s->val.u32;
           break;
        case nsXPTType::T_U64:
           if (i < N_ARG_REGS)
              regs[i] = s->val.u64;
           else
              *d++ = s->val.u64;
           break;
        case nsXPTType::T_FLOAT:
           if (i < N_ARG_REGS)
              // Place a float in least significant bytes.
              *(float*)(((char*)&regs[i+1]) - sizeof(float)) = s->val.f;
           else
              *(float*)d++ = s->val.f;
           break;
        case nsXPTType::T_DOUBLE:
           if (i < N_ARG_REGS)
              *(double*)&regs[i] = s->val.d;
           else
              *(double*)d++ = s->val.d;
           break;
        case nsXPTType::T_BOOL:
           if (i < N_ARG_REGS)
              regs[i] = s->val.b;
           else
              *d++ = s->val.b;
           break;
        case nsXPTType::T_CHAR:
           if (i < N_ARG_REGS)
              regs[i] = s->val.c;
           else
              *d++ = s->val.c;
           break;
        case nsXPTType::T_WCHAR:
           if (i < N_ARG_REGS)
              regs[i] = s->val.wc;
           else
              *d++ = s->val.wc;
           break;
        default:
           // all the others are plain pointer types
           if (i < N_ARG_REGS)
              regs[i] = (PRUint64)s->val.p;
           else
              *d++ = (PRUint64)s->val.p;
           break;
        }
    }
}

extern "C" nsresult _XPTC_InvokeByIndex(nsISupports* that, PRUint32 methodIndex,
                   PRUint32 paramCount, nsXPTCVariant* params);

extern "C"
XPTC_PUBLIC_API(nsresult)
XPTC_InvokeByIndex(nsISupports* that, PRUint32 methodIndex,
                   PRUint32 paramCount, nsXPTCVariant* params)
{
    return _XPTC_InvokeByIndex(that, methodIndex, paramCount, params);
}    
