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

#ifndef MKHTTP_H
#define MKHTTP_H

#ifndef MKGETURL_H
#include "mkgeturl.h"
#endif /* MKGETURL_H */

#define DEF_HTTP_PORT 80
#define DEF_HTTPS_PORT 443

typedef enum {
    POINT_NINE,
    ONE_POINT_O,
    ONE_POINT_ONE
} HTTP_Version;

extern void
NET_SetSendRefererHeader(Bool b);

extern void
NET_InitHTTPProtocol(void);

extern void
NET_getInternetKeyword(const URL_Struct *inURL, char *outKeyword,
	int16 inMaxLength);

#endif /* MKHTTP_H */
