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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

/*

  An aggregate object that implements the XUL Radio widget APIs

*/

#ifndef nsXULRadioElement_h__
#define nsXULRadioElement_h__

#include "nsXULElement.h"
#include "nsIDOMXULRadioElement.h"

class nsXULRadioElement : public nsXULAggregateElement,
                          public nsIDOMXULRadioElement
{
public:
    nsXULRadioElement(nsIDOMXULElement* aOuter);
    ~nsXULRadioElement();

    NS_DECL_ISUPPORTS_INHERITED

    // nsIDOMNode interface
    NS_FORWARD_IDOMNODE(mOuter->);

    // nsIDOMElement interface
    NS_FORWARD_IDOMELEMENT(mOuter->);

    // nsIDOMXULElement interface
    NS_FORWARD_IDOMXULELEMENT(mOuter->);

    // nsIDOMXULRadioElement interface
    NS_DECL_IDOMXULRADIOELEMENT
};


#endif // nsXULRadioElement_h__
