/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
#ifndef nsCSSProps_h___
#define nsCSSProps_h___

#include "nslayout.h"
#include "nsCSSPropIDs.h"

class NS_LAYOUT nsCSSProps {
public:
  // Given a null terminated string of 7 bit characters, return the
  // tag id (see nsCSSPropIDs.h) for the tag or -1 if the tag is not
  // known. The lookup function uses a perfect hash.
  static PRInt32 LookupName(const char* str);

    
  // Given a CSS Property ID and an Property Value Index
  // Return back a const char* representation of the 
  // value. Return back nsnull if no value is found
  static const char* LookupProperty(PRInt32 aProp, PRInt32 aIndex);

  struct NameTableEntry {
    const char* name;
    PRInt32 id;
  };

   // A table whose index is the tag id (from nsCSSPropIDs)
  static const NameTableEntry kNameTable[];

  static const PRInt32  kHintTable[];
};

#endif /* nsCSSProps_h___ */
