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
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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
#include "nsCOMPtr.h"
#include "nsLeafFrame.h"
#include "nsHTMLContainerFrame.h"
#include "nsHTMLParts.h"
#include "nsHTMLAtoms.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"

nsLeafFrame::~nsLeafFrame()
{
}

NS_IMETHODIMP
nsLeafFrame::Paint(nsPresContext*      aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect&        aDirtyRect,
                   nsFramePaintLayer    aWhichLayer,
                   PRUint32             aFlags)
{
  if (NS_FRAME_PAINT_LAYER_BACKGROUND == aWhichLayer) {
    PaintSelf(aPresContext, aRenderingContext, aDirtyRect);
  }
  DO_GLOBAL_REFLOW_COUNT_DSP("nsLeafFrame", &aRenderingContext);
  return NS_OK;
}

NS_IMETHODIMP
nsLeafFrame::Reflow(nsPresContext* aPresContext,
                    nsHTMLReflowMetrics& aMetrics,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus& aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsLeafFrame", aReflowState.reason);
  NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
                 ("enter nsLeafFrame::Reflow: aMaxSize=%d,%d",
                  aReflowState.availableWidth, aReflowState.availableHeight));

  NS_PRECONDITION(mState & NS_FRAME_IN_REFLOW, "frame is not in reflow");

  // XXX add in code to check for width/height being set via css
  // and if set use them instead of calling GetDesiredSize.


  GetDesiredSize(aPresContext, aReflowState, aMetrics);
  nsMargin borderPadding;
  AddBordersAndPadding(aPresContext, aReflowState, aMetrics, borderPadding);
  if (aMetrics.mComputeMEW) {
    aMetrics.SetMEWToActualWidth(aReflowState.mStylePosition->mWidth.GetUnit());
  }
  aStatus = NS_FRAME_COMPLETE;

  NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
                 ("exit nsLeafFrame::Reflow: size=%d,%d",
                  aMetrics.width, aMetrics.height));
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aMetrics);
  return NS_OK;
}

// XXX how should border&padding effect baseline alignment?
// => descent = borderPadding.bottom for example
void
nsLeafFrame::AddBordersAndPadding(nsPresContext* aPresContext,
                                  const nsHTMLReflowState& aReflowState,
                                  nsHTMLReflowMetrics& aMetrics,
                                  nsMargin& aBorderPadding)
{
  aBorderPadding = aReflowState.mComputedBorderPadding;
  aMetrics.width += aBorderPadding.left + aBorderPadding.right;
  aMetrics.height += aBorderPadding.top + aBorderPadding.bottom;
  aMetrics.ascent = aMetrics.height;
  aMetrics.descent = 0;
}
