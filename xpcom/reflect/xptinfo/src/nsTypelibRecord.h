/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
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
 * Copyright (C) 1999 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

/* For keeping track of typelibs and their interface directory tables. */

#ifndef nsTypelibRecord_h___
#define nsTypelibRecord_h___

/*  class nsInterfaceInfoManager; */


struct XPTHeader;
class nsIAllocator;
class nsInterfaceInfoManager;
class nsInterfaceRecord;

class nsTypelibRecord {
    // XXX accessors?  Could conceal 1-based offset here.
public:
    friend class nsInterfaceInfoManager;

    nsTypelibRecord(int size, nsTypelibRecord *in_next, XPTHeader *in_header,
                    nsIAllocator *allocator);

    static void DestroyList(nsTypelibRecord* aList, nsIAllocator* aAllocator);

    // array of pointers to (potentially shared) interface records,
    // NULL terminated.
    nsInterfaceRecord **interfaceRecords;
    nsTypelibRecord *next;
    XPTHeader *header;

protected:
    void Destroy(nsIAllocator* aAllocator);
};    

#endif /* nsTypelibRecord_h___ */

