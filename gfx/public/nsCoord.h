/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#ifndef NSCOORD_H
#define NSCOORD_H

#include "nscore.h"

/*
 * Basic type used for the geometry classes. By making it a typedef we can
 * easily change it in the future.
 *
 * All coordinates are maintained as signed 32-bit integers in the twips
 * coordinate space. A twip is 1/20th of a point, and there are 72 points per
 * inch.
 *
 * Twips are used because they are a device-independent unit of measure. See
 * header file nsUnitConversion.h for many useful macros to convert between
 * different units of measure.
 */
typedef PRInt32 nscoord;

#endif /* NSCOORD_H */
