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


/* We need this because Solaris' version of qsort is broken and
 * causes array bounds reads.
 */

#ifndef xp_qsort_h___
#define xp_qsort_h___

/* Had to pull the following define out of xp_core.h
 * to avoid including xp_core.h.
 * That brought in too many header file dependencies.
 */
#if defined(__cplusplus)
extern "C" {
#endif

#if defined(BROKEN_QSORT) || !defined(HAVE_QSORT)
extern void XP_QSORT(void *, size_t, size_t,
                     int (*)(const void *, const void *));
#elif defined(XP_OS2)
#define XP_QSORT(base, nel, width, compar) qsort((base),(nel),(width),(int(_Optlink*)(const void*,const void*))(compar))
#else
#define XP_QSORT(base, nel, width, compar) qsort((base),(nel),(width),(compar))
#endif

#if defined(__cplusplus)
}
#endif

#endif /* xp_qsort_h___ */
