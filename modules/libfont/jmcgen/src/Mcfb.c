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
 * Source date: 16 Apr 1997 08:37:58 GMT
 * netscape/fonts/cfb module C stub file
 * Generated by jmc version 1.8 -- DO NOT EDIT
 ******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "xp_mem.h"

/* Include the implementation-specific header: */
#include "Pcfb.h"

/* Include other interface headers: */
#include "Mnffbp.h"
#include "Mnffbu.h"

/*******************************************************************************
 * cfb Methods
 ******************************************************************************/

#ifndef OVERRIDE_cfb_getInterface
JMC_PUBLIC_API(void*)
_cfb_getInterface(struct cfb* self, jint op, const JMCInterfaceID* iid, JMCException* *exc)
{
	if (memcmp(iid, &cfb_ID, sizeof(JMCInterfaceID)) == 0)
		return cfbImpl2cfb(cfb2cfbImpl(self));
	if (memcmp(iid, &nffbp_ID, sizeof(JMCInterfaceID)) == 0)
		return cfbImpl2nffbp(cfb2cfbImpl(self));
	if (memcmp(iid, &nffbu_ID, sizeof(JMCInterfaceID)) == 0)
		return cfbImpl2nffbu(cfb2cfbImpl(self));
	return _cfb_getBackwardCompatibleInterface(self, iid, exc);
}
#endif

#ifndef OVERRIDE_cfb_addRef
JMC_PUBLIC_API(void)
_cfb_addRef(struct cfb* self, jint op, JMCException* *exc)
{
	cfbImplHeader* impl = (cfbImplHeader*)cfb2cfbImpl(self);
	impl->refcount++;
}
#endif

#ifndef OVERRIDE_cfb_release
JMC_PUBLIC_API(void)
_cfb_release(struct cfb* self, jint op, JMCException* *exc)
{
	cfbImplHeader* impl = (cfbImplHeader*)cfb2cfbImpl(self);
	if (--impl->refcount == 0) {
		cfb_finalize(self, exc);
	}
}
#endif

#ifndef OVERRIDE_cfb_hashCode
JMC_PUBLIC_API(jint)
_cfb_hashCode(struct cfb* self, jint op, JMCException* *exc)
{
	return (jint)self;
}
#endif

#ifndef OVERRIDE_cfb_equals
JMC_PUBLIC_API(jbool)
_cfb_equals(struct cfb* self, jint op, void* obj, JMCException* *exc)
{
	return self == obj;
}
#endif

#ifndef OVERRIDE_cfb_clone
JMC_PUBLIC_API(void*)
_cfb_clone(struct cfb* self, jint op, JMCException* *exc)
{
	cfbImpl* impl = cfb2cfbImpl(self);
	cfbImpl* newImpl = (cfbImpl*)malloc(sizeof(cfbImpl));
	if (newImpl == NULL) return NULL;
	memcpy(newImpl, impl, sizeof(cfbImpl));
	((cfbImplHeader*)newImpl)->refcount = 1;
	return newImpl;
}
#endif

#ifndef OVERRIDE_cfb_toString
JMC_PUBLIC_API(const char*)
_cfb_toString(struct cfb* self, jint op, JMCException* *exc)
{
	return NULL;
}
#endif

#ifndef OVERRIDE_cfb_finalize
JMC_PUBLIC_API(void)
_cfb_finalize(struct cfb* self, jint op, JMCException* *exc)
{
	/* Override this method and add your own finalization here. */
	XP_FREEIF(self);
}
#endif

/*******************************************************************************
 * nffbp Methods
 ******************************************************************************/

JMC_PUBLIC_API(void*)
_cfb_nffbp_getInterface(struct nffbp* self, jint op, const JMCInterfaceID* iid, JMCException* *exc)
{
	return _cfb_getInterface(cfbImpl2cfb(nffbp2cfbImpl(self)), op, iid, exc);
}

JMC_PUBLIC_API(void)
_cfb_nffbp_addRef(struct nffbp* self, jint op, JMCException* *exc)
{
	_cfb_addRef(cfbImpl2cfb(nffbp2cfbImpl(self)), op, exc);
}

JMC_PUBLIC_API(void)
_cfb_nffbp_release(struct nffbp* self, jint op, JMCException* *exc)
{
	_cfb_release(cfbImpl2cfb(nffbp2cfbImpl(self)), op, exc);
}

JMC_PUBLIC_API(jint)
_cfb_nffbp_hashCode(struct nffbp* self, jint op, JMCException* *exc)
{
	return _cfb_hashCode(cfbImpl2cfb(nffbp2cfbImpl(self)), op, exc);
}

JMC_PUBLIC_API(jbool)
_cfb_nffbp_equals(struct nffbp* self, jint op, void* other, JMCException* *exc)
{
	return _cfb_equals(cfbImpl2cfb(nffbp2cfbImpl(self)), op, other, exc);
}

JMC_PUBLIC_API(void*)
_cfb_nffbp_clone(struct nffbp* self, jint op, JMCException* *exc)
{
	return _cfb_clone(cfbImpl2cfb(nffbp2cfbImpl(self)), op, exc);
}

JMC_PUBLIC_API(const char*)
_cfb_nffbp_toString(struct nffbp* self, jint op, JMCException* *exc)
{
	return _cfb_toString(cfbImpl2cfb(nffbp2cfbImpl(self)), op, exc);
}

JMC_PUBLIC_API(void)
_cfb_nffbp_finalize(struct nffbp* self, jint op, JMCException* *exc)
{
	_cfb_finalize(cfbImpl2cfb(nffbp2cfbImpl(self)), op, exc);
}

/*******************************************************************************
 * nffbu Methods
 ******************************************************************************/

JMC_PUBLIC_API(void*)
_cfb_nffbu_getInterface(struct nffbu* self, jint op, const JMCInterfaceID* iid, JMCException* *exc)
{
	return _cfb_getInterface(cfbImpl2cfb(nffbu2cfbImpl(self)), op, iid, exc);
}

JMC_PUBLIC_API(void)
_cfb_nffbu_addRef(struct nffbu* self, jint op, JMCException* *exc)
{
	_cfb_addRef(cfbImpl2cfb(nffbu2cfbImpl(self)), op, exc);
}

JMC_PUBLIC_API(void)
_cfb_nffbu_release(struct nffbu* self, jint op, JMCException* *exc)
{
	_cfb_release(cfbImpl2cfb(nffbu2cfbImpl(self)), op, exc);
}

JMC_PUBLIC_API(jint)
_cfb_nffbu_hashCode(struct nffbu* self, jint op, JMCException* *exc)
{
	return _cfb_hashCode(cfbImpl2cfb(nffbu2cfbImpl(self)), op, exc);
}

JMC_PUBLIC_API(jbool)
_cfb_nffbu_equals(struct nffbu* self, jint op, void* other, JMCException* *exc)
{
	return _cfb_equals(cfbImpl2cfb(nffbu2cfbImpl(self)), op, other, exc);
}

JMC_PUBLIC_API(void*)
_cfb_nffbu_clone(struct nffbu* self, jint op, JMCException* *exc)
{
	return _cfb_clone(cfbImpl2cfb(nffbu2cfbImpl(self)), op, exc);
}

JMC_PUBLIC_API(const char*)
_cfb_nffbu_toString(struct nffbu* self, jint op, JMCException* *exc)
{
	return _cfb_toString(cfbImpl2cfb(nffbu2cfbImpl(self)), op, exc);
}

JMC_PUBLIC_API(void)
_cfb_nffbu_finalize(struct nffbu* self, jint op, JMCException* *exc)
{
	_cfb_finalize(cfbImpl2cfb(nffbu2cfbImpl(self)), op, exc);
}

/*******************************************************************************
 * Jump Tables
 ******************************************************************************/

const struct cfbInterface cfbVtable = {
	_cfb_getInterface,
	_cfb_addRef,
	_cfb_release,
	_cfb_hashCode,
	_cfb_equals,
	_cfb_clone,
	_cfb_toString,
	_cfb_finalize,
	_cfb_LookupFont,
	_cfb_CreateFontFromUrl,
	_cfb_CreateFontFromFile,
	_cfb_ListFonts,
	_cfb_ListSizes,
	_cfb_GetBaseFont
};

const struct nffbpInterface nffbpVtable = {
	_cfb_nffbp_getInterface,
	_cfb_nffbp_addRef,
	_cfb_nffbp_release,
	_cfb_nffbp_hashCode,
	_cfb_nffbp_equals,
	_cfb_nffbp_clone,
	_cfb_nffbp_toString,
	_cfb_nffbp_finalize,
	_cfb_nffbp_RegisterFontDisplayer,
	_cfb_nffbp_CreateFontDisplayerFromDLM,
	_cfb_nffbp_ScanForFontDisplayers,
	_cfb_nffbp_RfDone
};

const struct nffbuInterface nffbuVtable = {
	_cfb_nffbu_getInterface,
	_cfb_nffbu_addRef,
	_cfb_nffbu_release,
	_cfb_nffbu_hashCode,
	_cfb_nffbu_equals,
	_cfb_nffbu_clone,
	_cfb_nffbu_toString,
	_cfb_nffbu_finalize,
	_cfb_nffbu_CreateFontMatchInfo,
	_cfb_nffbu_CreateRenderingContext,
	_cfb_nffbu_CreateFontObserver,
	_cfb_nffbu_malloc,
	_cfb_nffbu_free,
	_cfb_nffbu_realloc,
	_cfb_nffbu_IsWebfontsEnabled,
	_cfb_nffbu_EnableWebfonts,
	_cfb_nffbu_DisableWebfonts,
	_cfb_nffbu_ListFontDisplayers,
	_cfb_nffbu_IsFontDisplayerEnabled,
	_cfb_nffbu_ListFontDisplayersForMimetype,
	_cfb_nffbu_FontDisplayerForMimetype,
	_cfb_nffbu_EnableFontDisplayer,
	_cfb_nffbu_DisableFontDisplayer,
	_cfb_nffbu_EnableMimetype,
	_cfb_nffbu_DisableMimetype,
	_cfb_nffbu_LoadCatalog,
	_cfb_nffbu_SaveCatalog,
	_cfb_nffbu_LoadWebfont,
	_cfb_nffbu_ReleaseWebfonts,
	_cfb_nffbu_WebfontsNeedReload,
	_cfb_nffbu_LookupFailed,
	_cfb_nffbu_ToUnicode,
	_cfb_nffbu_UnicodeLen
};

/*******************************************************************************
 * Factory Operations
 ******************************************************************************/

JMC_PUBLIC_API(cfb*)
cfbFactory_Create(JMCException* *exception)
{
	cfbImplHeader* impl = (cfbImplHeader*)XP_NEW_ZAP(cfbImpl);
	cfb* self;
	if (impl == NULL) {
		JMC_EXCEPTION(exception, JMCEXCEPTION_OUT_OF_MEMORY);
		return NULL;
	}
	self = cfbImpl2cfb(impl);
	impl->vtablecfb = &cfbVtable;
	impl->vtablenffbp = &nffbpVtable;
	impl->vtablenffbu = &nffbuVtable;
	impl->refcount = 1;
	_cfb_init(self, exception);
	if (JMC_EXCEPTION_RETURNED(exception)) {
		XP_FREE(impl);
		return NULL;
	}
	return self;
}

