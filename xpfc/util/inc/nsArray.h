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

#ifndef nsArray_h___
#define nsArray_h___

#include "nsIArray.h"

#include "nsVoidArray.h"

class nsArray : public nsIArray
{

public:
  nsArray();

  NS_DECL_ISUPPORTS

  NS_IMETHOD                  Init() ;

  NS_IMETHOD_(PRUint32)       Count() ;
  NS_IMETHOD_(PRBool)         Empty() ;
  NS_IMETHOD_(PRBool)         Contains(nsComponent aComponent) ;
  NS_IMETHOD_(PRUint32)       IndexOf(nsComponent aComponent) ;
  NS_IMETHOD_(nsComponent)    ElementAt(PRUint32 aIndex) ;
  NS_IMETHOD_(PRInt32)        InsertBinary(nsComponent aComponent, nsArrayCompareProc aCompFn, PRBool bAllowDups);

  NS_IMETHOD                  Insert(PRUint32 aIndex, nsComponent aComponent) ;
  NS_IMETHOD                  Append(nsComponent aComponent) ;
  NS_IMETHOD                  Remove(nsComponent aComponent) ;
  NS_IMETHOD                  RemoveAll() ;
  NS_IMETHOD                  RemoveAt(PRUint32 aIndex) ;

  NS_IMETHOD                  CreateIterator(nsIIterator ** aIterator) ;

protected:
  ~nsArray();

private:
  nsVoidArray * mVoidArray ;

};

#endif /* nsArray_h___ */
