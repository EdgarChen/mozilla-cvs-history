/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
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

#ifndef nsIXPFCICalContentSink_h___
#define nsIXPFCICalContentSink_h___

#include "nsISupports.h"
#include "nsIArray.h"

//ca581fe0-743f-11d2-943f-006008268c31
#define NS_IXPFC_ICAL_CONTENT_SINK_IID   \
{ 0xca581fe0, 0x743f, 0x11d2,    \
{ 0x94, 0x3f, 0x00, 0x60, 0x08, 0x26, 0x8c, 0x31 } }

class nsIXPFCICalContentSink : public nsISupports
{

public:

  NS_IMETHOD                 Init() = 0 ;
  NS_IMETHOD                 SetCalendarList(nsIArray * aCalendarList) = 0;

};

#endif /* nsIXPFCICalContentSink_h___ */




