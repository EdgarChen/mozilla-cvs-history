/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
/*******************************************************************************
 * Source date: 9 Apr 1997 21:45:12 GMT
 * netscape/fonts/nfdoer public interface
 * Generated by jmc version 1.8 -- DO NOT EDIT
 ******************************************************************************/

#ifndef _Mnfdoer_H_
#define _Mnfdoer_H_

#include "jmc.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*******************************************************************************
 * nfdoer
 ******************************************************************************/

/* The type of the nfdoer interface. */
struct nfdoerInterface;

/* The public type of a nfdoer instance. */
typedef struct nfdoer {
	const struct nfdoerInterface*	vtable;
} nfdoer;

/* The inteface ID of the nfdoer interface. */
#ifndef JMC_INIT_nfdoer_ID
extern EXTERN_C_WITHOUT_EXTERN const JMCInterfaceID nfdoer_ID;
#else
EXTERN_C const JMCInterfaceID nfdoer_ID = { 0x31133d0d, 0x67392f05, 0x1367271c, 0x60794020 };
#endif /* JMC_INIT_nfdoer_ID */
/*******************************************************************************
 * nfdoer Operations
 ******************************************************************************/

#define nfdoer_getInterface(self, a, exception)	\
	(((self)->vtable->getInterface)(self, nfdoer_getInterface_op, a, exception))

#define nfdoer_addRef(self, exception)	\
	(((self)->vtable->addRef)(self, nfdoer_addRef_op, exception))

#define nfdoer_release(self, exception)	\
	(((self)->vtable->release)(self, nfdoer_release_op, exception))

#define nfdoer_hashCode(self, exception)	\
	(((self)->vtable->hashCode)(self, nfdoer_hashCode_op, exception))

#define nfdoer_equals(self, a, exception)	\
	(((self)->vtable->equals)(self, nfdoer_equals_op, a, exception))

#define nfdoer_clone(self, exception)	\
	(((self)->vtable->clone)(self, nfdoer_clone_op, exception))

#define nfdoer_toString(self, exception)	\
	(((self)->vtable->toString)(self, nfdoer_toString_op, exception))

#define nfdoer_finalize(self, exception)	\
	(((self)->vtable->finalize)(self, nfdoer_finalize_op, exception))

#define nfdoer_Update(self, a, exception)	\
	(((self)->vtable->Update)(self, nfdoer_Update_op, a, exception))

/*******************************************************************************
 * nfdoer Interface
 ******************************************************************************/

struct netscape_jmc_JMCInterfaceID;
struct java_lang_Object;
struct java_lang_String;
struct netscape_fonts_nff;

struct nfdoerInterface {
	void*	(*getInterface)(struct nfdoer* self, jint op, const JMCInterfaceID* a, JMCException* *exception);
	void	(*addRef)(struct nfdoer* self, jint op, JMCException* *exception);
	void	(*release)(struct nfdoer* self, jint op, JMCException* *exception);
	jint	(*hashCode)(struct nfdoer* self, jint op, JMCException* *exception);
	jbool	(*equals)(struct nfdoer* self, jint op, void* a, JMCException* *exception);
	void*	(*clone)(struct nfdoer* self, jint op, JMCException* *exception);
	const char*	(*toString)(struct nfdoer* self, jint op, JMCException* *exception);
	void	(*finalize)(struct nfdoer* self, jint op, JMCException* *exception);
	void	(*Update)(struct nfdoer* self, jint op, struct nff* a, JMCException* *exception);
};

/*******************************************************************************
 * nfdoer Operation IDs
 ******************************************************************************/

typedef enum nfdoerOperations {
	nfdoer_getInterface_op,
	nfdoer_addRef_op,
	nfdoer_release_op,
	nfdoer_hashCode_op,
	nfdoer_equals_op,
	nfdoer_clone_op,
	nfdoer_toString_op,
	nfdoer_finalize_op,
	nfdoer_Update_op
} nfdoerOperations;

/******************************************************************************/

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* _Mnfdoer_H_ */
