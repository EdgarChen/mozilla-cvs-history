/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#include "nsVixenShell.h"

#include "nsTransactionManagerCID.h"
#include "nsITransactionManager.h"

// Supported Transactions
/*
#include "VxSetAttributeTxn.h"
#include "VxRemoveAttributeTxn.h"
#include "VxInsertElementTxn.h"
#include "VxRemoveElementTxn.h"
*/

nsVixenShell::nsVixenShell()
{
    NS_INIT_REFCNT();
}

nsVixenShell::~nsVixenShell()
{
}

NS_IMPL_ISUPPORTS1(nsVixenShell, nsIVixenShell);

NS_IMETHODIMP 
nsVixenShell::SetAttribute(nsIDOMElement *aElement, 
                           const PRUnichar *aAttribute, 
                           const PRUnichar *aValue)
{
    return NS_OK;
}

NS_IMETHODIMP 
nsVixenShell::RemoveAttribute(nsIDOMElement *aElement, 
                              const PRUnichar *aAttribute)
{
    return NS_OK;
}

NS_IMETHODIMP 
nsVixenShell::InsertElement(nsIDOMElement *aElement, 
                            nsIDOMElement *aParent, 
                            PRInt32 aPos)
{
    return NS_OK;
}

NS_IMETHODIMP 
nsVixenShell::RemoveElement(nsIDOMElement *aElement)
{
    return NS_OK;
}

// Execute Transactions
NS_IMETHODIMP 
nsVixenShell::Do(nsITransaction* aTxn)
{
    return NS_OK;
}

// Create Transformations for the Transaction Manager
NS_IMETHODIMP
nsVixenShell::CreateTxnForSetAttribute(nsIDOMElement* aElement, 
                                       const nsString& aAttribute, 
                                       const nsString& aValue, 
                                       VxChangeAttributeTxn** aTxn)
{
    return NS_OK;
}

NS_IMETHODIMP
nsVixenShell::CreateTxnForRemoveAttribute(nsIDOMElement* aElement,
                                          const nsString& aAttribute,
                                          VxRemoveAttributeTxn** aTxn)
{
    return NS_OK;
}

NS_IMETHODIMP
nsVixenShell::CreateTxnForInsertElement(nsIDOMElement* aElement,
                                        nsIDOMElement* aParent,
                                        PRInt32 aPos,
                                        VxInsertElementTxn** aTxn)
{
    return NS_OK;
}

NS_IMETHODIMP 
nsVixenShell::CreateTxnForRemoveElement(nsIDOMElement* aElement,
                                        VxRemoveElementTxn** aTxn)
{
    return NS_OK;
}


