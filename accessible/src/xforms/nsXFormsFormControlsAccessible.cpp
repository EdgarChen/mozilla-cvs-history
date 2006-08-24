/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Mozilla Foundation.
 * Portions created by the Initial Developer are Copyright (C) 2006
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Alexander Surkov <surkov.alexander@gmail.com> (original author)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsXFormsFormControlsAccessible.h"

// nsXFormsLabelAccessible

nsXFormsLabelAccessible::
  nsXFormsLabelAccessible(nsIDOMNode *aNode, nsIWeakReference *aShell):
  nsXFormsAccessible(aNode, aShell)
{
}

NS_IMETHODIMP
nsXFormsLabelAccessible::GetRole(PRUint32 *aRole)
{
  NS_ENSURE_ARG_POINTER(aRole);

  *aRole = ROLE_STATICTEXT;
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsLabelAccessible::GetName(nsAString& aName)
{
  nsAutoString name;
  nsresult rv = GetTextFromRelationID(nsAccessibilityAtoms::labelledby, name);
  aName = name;
  return rv;
}

NS_IMETHODIMP
nsXFormsLabelAccessible::GetDescription(nsAString& aDescription)
{
  nsAutoString description;
  nsresult rv = GetTextFromRelationID(nsAccessibilityAtoms::describedby,
                                      description);
  aDescription = description;
  return rv;
}

// nsXFormsOutputAccessible

nsXFormsOutputAccessible::
  nsXFormsOutputAccessible(nsIDOMNode *aNode, nsIWeakReference *aShell):
  nsXFormsAccessible(aNode, aShell)
{
}

NS_IMETHODIMP
nsXFormsOutputAccessible::GetRole(PRUint32 *aRole)
{
  NS_ENSURE_ARG_POINTER(aRole);

  *aRole = ROLE_STATICTEXT;
  return NS_OK;
}

// nsXFormsTriggerAccessible

nsXFormsTriggerAccessible::
  nsXFormsTriggerAccessible(nsIDOMNode *aNode, nsIWeakReference *aShell):
  nsXFormsAccessible(aNode, aShell)
{
}

NS_IMETHODIMP
nsXFormsTriggerAccessible::GetRole(PRUint32 *aRole)
{
  NS_ENSURE_ARG_POINTER(aRole);

  *aRole = ROLE_PUSHBUTTON;
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsTriggerAccessible::GetValue(nsAString& aValue)
{
  aValue.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsTriggerAccessible::GetNumActions(PRUint8 *aCount)
{
  NS_ENSURE_ARG_POINTER(aCount);

  *aCount = 1;
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsTriggerAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  if (aIndex == eAction_Click) {
    nsAccessible::GetTranslatedString(NS_LITERAL_STRING("press"), aName);
    return NS_OK;
  }
  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
nsXFormsTriggerAccessible::DoAction(PRUint8 aIndex)
{
  if (aIndex == eAction_Click)
    return DoCommand();

  return NS_ERROR_INVALID_ARG;
}

