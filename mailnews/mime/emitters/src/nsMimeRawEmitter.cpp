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
#include "nsCOMPtr.h"
#include "stdio.h"
#include "nsMimeRebuffer.h"
#include "nsMimeRawEmitter.h"
#include "plstr.h"
#include "nsIMimeEmitter.h"
#include "nsMailHeaders.h"
#include "nscore.h"
#include "nsEscape.h"
#include "prmem.h"
#include "nsEmitterUtils.h"

nsresult NS_NewMimeRawEmitter(const nsIID& iid, void **result)
{
	nsMimeRawEmitter *obj = new nsMimeRawEmitter();
	if (obj)
		return obj->QueryInterface(iid, result);
	else
		return NS_ERROR_OUT_OF_MEMORY;
}

/*
 * nsMimeRawEmitter definitions....
 */
nsMimeRawEmitter::nsMimeRawEmitter()
{
}


nsMimeRawEmitter::~nsMimeRawEmitter(void)
{
}

NS_IMETHODIMP
nsMimeRawEmitter::WriteBody(const char *buf, PRUint32 size, PRUint32 *amountWritten)
{
  Write(buf, size, amountWritten);
  return NS_OK;
}



