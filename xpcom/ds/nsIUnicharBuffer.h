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
#ifndef nsIUnicharBuffer_h___
#define nsIUnicharBuffer_h___

#include "nscore.h"
#include "nsISupports.h"
class nsIUnicharInputStream;

#define NS_IUNICHARBUFFER_IID \
{ 0x14cf6970, 0x93b5, 0x11d1, \
  {0x89, 0x5b, 0x00, 0x60, 0x08, 0x91, 0x1b, 0x81} }

/// Interface to a buffer that holds unicode characters
class nsIUnicharBuffer : public nsISupports {
public:
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_IUNICHARBUFFER_IID);

  NS_IMETHOD Init(PRUint32 aBufferSize) = 0;
  NS_IMETHOD_(PRInt32) GetLength() const = 0;
  NS_IMETHOD_(PRInt32) GetBufferSize() const = 0;
  NS_IMETHOD_(PRUnichar*) GetBuffer() const = 0;
  NS_IMETHOD_(PRBool) Grow(PRInt32 aNewSize) = 0;
  NS_IMETHOD_(PRInt32) Fill(nsresult* aErrorCode, nsIUnicharInputStream* aStream,
                            PRInt32 aKeep) = 0;
};

/// Factory method for nsIUnicharBuffer.
extern NS_COM nsresult
NS_NewUnicharBuffer(nsIUnicharBuffer** aInstancePtrResult,
                    nsISupports* aOuter,
                    PRUint32 aBufferSize = 0);

#define NS_UNICHARBUFFER_CID                         \
{ /* c81fd8f0-0d6b-11d3-9331-00104ba0fd40 */         \
    0xc81fd8f0,                                      \
    0x0d6b,                                          \
    0x11d3,                                          \
    {0x93, 0x31, 0x00, 0x10, 0x4b, 0xa0, 0xfd, 0x40} \
}

#endif /* nsIUnicharBuffer_h___ */
