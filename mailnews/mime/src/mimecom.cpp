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
#include "mimei.h"
#include "modmime.h"
#include "mimeobj.h"	/*  MimeObject (abstract)							*/
#include "mimecont.h"	/*   |--- MimeContainer (abstract)					*/
#include "mimemult.h"	/*   |     |--- MimeMultipart (abstract)			*/
#include "mimemsig.h"	/*   |     |     |--- MimeMultipartSigned (abstract)*/
#include "mimetext.h"	/*   |     |--- MimeInlineText (abstract)			*/
#include "mimecth.h"

/*
 * These calls are necessary to expose the object class heirarchy 
 * to externally developed content type handlers.
 */
extern "C" void *
XPCOM_GetmimeInlineTextClass(void)
{
  return (void *) &mimeInlineTextClass;
}

extern "C" void *
XPCOM_GetmimeLeafClass(void)
{
  return (void *) &mimeLeafClass;
}

extern "C" void *
XPCOM_GetmimeObjectClass(void)
{
  return (void *) &mimeObjectClass;
}

extern "C" void *
XPCOM_GetmimeContainerClass(void)
{
  return (void *) &mimeContainerClass;
}

extern "C" void *
XPCOM_GetmimeMultipartClass(void)
{
  return (void *) &mimeMultipartClass;
}

extern "C" void *
XPCOM_GetmimeMultipartSignedClass(void)
{
  return (void *) &mimeMultipartSignedClass;
}

extern "C" int  
XPCOM_MimeObject_write(void *mimeObject, 
                       char *data, 
                       PRInt32 length, 
                       PRBool user_visible_p)
{
  return MIME_MimeObject_write((MimeObject *)mimeObject, data, 
                                length, user_visible_p);
}
