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
 * The Original Code is the Mozilla SVG project.
 *
 * The Initial Developer of the Original Code is IBM Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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

#include "nsSVGDefsFrame.h"
#include "nsLayoutAtoms.h"

#define NS_SVGCLIPPATHFRAME_CID \
{0xb497bbe2, 0x4434, 0x4d96, {0x9c, 0xe8, 0xf2, 0xad, 0xd1, 0x1f, 0x1d, 0x26}}

typedef nsSVGDefsFrame nsSVGClipPathFrameBase;

class nsSVGClipPathFrame : public nsSVGClipPathFrameBase
{
  friend nsresult
  NS_NewSVGClipPathFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsIFrame** aNewFrame);

  virtual ~nsSVGClipPathFrame();
  NS_IMETHOD InitSVG();

 public:
  NS_DECL_ISUPPORTS

  NS_DEFINE_STATIC_CID_ACCESSOR(NS_SVGCLIPPATHFRAME_CID)
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_SVGCLIPPATHFRAME_CID)

  NS_IMETHOD ClipPaint(nsISVGRendererCanvas* canvas,
                       nsISVGChildFrame* aParent,
                       nsCOMPtr<nsIDOMSVGMatrix> aMatrix);

  NS_IMETHOD ClipHitTest(nsISVGChildFrame* aParent,
                         nsCOMPtr<nsIDOMSVGMatrix> aMatrix,
                         float aX, float aY, PRBool *aHit);
  /**
   * Get the "type" of the frame
   *
   * @see nsLayoutAtoms::svgClipPathFrame
   */
  virtual nsIAtom* GetType() const;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGClipPath"), aResult);
  }
#endif

 private:
  nsISVGChildFrame *mClipParent;
  nsCOMPtr<nsIDOMSVGMatrix> mClipParentMatrix;

  // nsISVGContainerFrame interface:
  already_AddRefed<nsIDOMSVGMatrix> GetCanvasTM();
};

nsresult
NS_GetSVGClipPathFrame(nsSVGClipPathFrame **aResult, nsIURI *aURI, nsIContent *aContent);
