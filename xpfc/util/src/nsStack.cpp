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

#include "nsStack.h"
#include "nsxpfcCIID.h"
#include "nsxpfcutil.h"

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kCStackCID, NS_STACK_CID);

nsStack :: nsStack()
{
  NS_INIT_REFCNT();

  mVoidArray = nsnull ;
}

nsStack :: ~nsStack()
{
  DeleteIfObject(mVoidArray) ;
}

NS_IMPL_QUERY_INTERFACE(nsStack, kCStackCID)
NS_IMPL_ADDREF(nsStack)
NS_IMPL_RELEASE(nsStack)

nsresult nsStack :: Init()
{
  mVoidArray = NewObject(nsVoidArray) ;

  return NS_OK ;
}

PRBool nsStack :: Empty()
{
  return (mVoidArray->Count() ? PR_TRUE : PR_FALSE) ;
}

nsresult nsStack :: Push(nsComponent aComponent)
{
  nsresult res = NS_OK;

  if (PR_TRUE == mVoidArray->AppendElement(aComponent))
    return NS_OK ;

  return res ;
}

nsComponent nsStack :: Pop()
{
  nsComponent component = mVoidArray->ElementAt(mVoidArray->Count()-1);

  if (PR_TRUE == mVoidArray->RemoveElementAt(mVoidArray->Count()-1))
    return component ;

  return nsnull ;
}

nsComponent nsStack :: Top()
{
  return (mVoidArray->ElementAt(mVoidArray->Count()-1)) ;
}

