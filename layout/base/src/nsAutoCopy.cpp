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
 * Original Author: Michael F. Judge (mjudge@netscape.com)
 *
 * Contributor(s): 
 */

#include "nsCOMPtr.h"
#include "nsIAutoCopyService.h"
#include "nsIDOMSelection.h"

class nsAutoCopyService : public nsIAutoCopyService , public nsIDOMSelectionListener
{
public:
  nsAutoCopyService(){}
  ~nsAutoCopyService(){}//someday maybe we have it able to shutdown during run

  //nsIAutoCopyService interfaces
  NS_IMETHOD Listen(nsIDOMSelection *aDomSelection);
  //end nsIAutoCopyService

  //nsIDOMSelectionListener interfaces
  NS_IMETHOD NotifySelectionChanged();
  //end nsIDOMSelectionListener 
};


NS_IMETHODIMP
nsAutoCopyService::Listen(nsIFrameSelection *aDomSelection)
{
  aDomSelection->AddSelectionListener(this);
}


NS_IMETHODIMP
nsAutoCopyService::NotifySelectionChanged()
{
}


