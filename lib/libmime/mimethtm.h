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

/* mimethtm.h --- definition of the MimeInlineTextHTML class (see mimei.h)
   Created: Jamie Zawinski <jwz@netscape.com>, 15-May-96.
 */

#ifndef _MIMETHTM_H_
#define _MIMETHTM_H_

#include "mimetext.h"

/* The MimeInlineTextHTML class implements the text/html MIME content type.
 */

typedef struct MimeInlineTextHTMLClass MimeInlineTextHTMLClass;
typedef struct MimeInlineTextHTML      MimeInlineTextHTML;

struct MimeInlineTextHTMLClass {
  MimeInlineTextClass text;
};

extern MimeInlineTextHTMLClass mimeInlineTextHTMLClass;

struct MimeInlineTextHTML {
  MimeInlineText text;
};

#endif /* _MIMETHTM_H_ */
