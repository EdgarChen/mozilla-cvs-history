/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ----- BEGIN LICENSE BLOCK -----
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Mozilla SVG project.
 *
 * The Initial Developer of the Original Code is 
 * Crocodile Clips Ltd..
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *    Alex Fritze <alex.fritze@crocodile-clips.com> (original author)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ----- END LICENSE BLOCK ----- */

#ifndef __NS_SVGMATRIX_H__
#define __NS_SVGMATRIX_H__

#include "nsIDOMSVGMatrix.h"
#include "nsSVGValue.h"

class nsSVGMatrix : public nsIDOMSVGMatrix,
                    public nsSVGValue
{
public:
  // XXX the first parameter 'a' is renamed to 'xxx_a' to work around
  // what seems to be a VC++ compiler problem; see bug#122363, comment#2
  static nsresult Create(nsIDOMSVGMatrix** aResult,
                         float xxx_a=1.0f, float b=0.0f, float c=0.0f,
                         float d=1.0f, float e=0.0f, float f=0.0f);
  
protected:
  nsSVGMatrix(float a=1.0f,
              float b=0.0f,
              float c=0.0f,
              float d=1.0f,
              float e=0.0f,
              float f=0.0f);
  
public:
  // nsISupports interface:
  NS_DECL_ISUPPORTS

  // nsIDOMSVGMatrix interface:
  NS_DECL_NSIDOMSVGMATRIX

  // nsISVGValue interface:
  NS_IMETHOD SetValueString(const nsAString& aValue);
  NS_IMETHOD GetValueString(nsAString& aValue);
  
  
protected:
  float mA, mB, mC, mD, mE, mF;
};


#endif //__NS_SVGMATRIX_H__
