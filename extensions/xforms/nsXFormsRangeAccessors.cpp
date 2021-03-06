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
 * The Original Code is Mozilla XForms support.
 *
 * The Initial Developer of the Original Code is
 * Novell, Inc.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Allan Beaufour <abeaufour@novell.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
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

#include "nsXFormsRangeAccessors.h"
#include "nsIDOMElement.h"
#include "nsStringAPI.h"
#include "nsXFormsUtils.h"

NS_IMPL_ISUPPORTS_INHERITED2(nsXFormsRangeAccessors,
                             nsXFormsRangeConditionAccessors,
                             nsIXFormsRangeAccessors, 
                             nsIClassInfo)

// nsXFormsRangeAccessors
nsresult
nsXFormsRangeAccessors::AttributeGetter(const nsAString &aAttr, nsAString &aVal)
{
  nsAutoString val;
  if (mElement) {
    mElement->GetAttribute(aAttr, val);
  }
  if (val.IsEmpty()) {
    aVal.SetIsVoid(PR_TRUE);
  } else {
    aVal = val;
  }

  return NS_OK;
}


// nsIXFormsRangeElement

// XXX this should do a max(type.minumum, @start)
NS_IMETHODIMP
nsXFormsRangeAccessors::GetRangeStart(nsAString &aMin)
{
  return AttributeGetter(NS_LITERAL_STRING("start"), aMin);
}

// XXX this should do min(type.maximu, @end)
NS_IMETHODIMP
nsXFormsRangeAccessors::GetRangeEnd(nsAString &aMax)
{
  return AttributeGetter(NS_LITERAL_STRING("end"), aMax);
}

// XXX if step is not set, it should be set to something "smart" and also
// needs to be something that is valid for the given type. This could be
// pushed to the widget though.
NS_IMETHODIMP
nsXFormsRangeAccessors::GetRangeStep(nsAString &aStep)
{
  return AttributeGetter(NS_LITERAL_STRING("step"), aStep);
}


// nsIClassInfo implementation

static const nsIID sScriptingIIDs[] = {
  NS_IXFORMSACCESSORS_IID,
  NS_IXFORMSRANGECONDITIONACCESSORS_IID,
  NS_IXFORMSRANGEACCESSORS_IID
};

NS_IMETHODIMP
nsXFormsRangeAccessors::GetInterfaces(PRUint32 *aCount, nsIID * **aArray)
{
  return nsXFormsUtils::CloneScriptingInterfaces(sScriptingIIDs,
                                                 NS_ARRAY_LENGTH(sScriptingIIDs),
                                                 aCount, aArray);
}
