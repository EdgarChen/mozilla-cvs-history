/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is Mozilla MathML Project.
 * 
 * The Initial Developer of the Original Code is The University Of 
 * Queensland.  Portions created by The University Of Queensland are
 * Copyright (C) 1999 The University Of Queensland.  All Rights Reserved.
 * 
 * Contributor(s): 
 *   Roger B. Sidje <rbs@maths.uq.edu.au>
 *   David J. Fiddes <D.J.Fiddes@hw.ac.uk>
 */


#include "nsCOMPtr.h"
#include "nsHTMLParts.h"
#include "nsIHTMLContent.h"
#include "nsFrame.h"
#include "nsLineLayout.h"
#include "nsHTMLIIDs.h"
#include "nsIPresContext.h"
#include "nsHTMLAtoms.h"
#include "nsUnitConversion.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"
#include "nsINameSpaceManager.h"
#include "nsIRenderingContext.h"
#include "nsIFontMetrics.h"
#include "nsStyleUtil.h"

#include "nsMathMLmunderoverFrame.h"

//
// <munderover> -- attach an underscript-overscript pair to a base - implementation
//

nsresult
NS_NewMathMLmunderoverFrame(nsIFrame** aNewFrame)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsMathMLmunderoverFrame* it = new nsMathMLmunderoverFrame;
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  *aNewFrame = it;
  return NS_OK;
}

nsMathMLmunderoverFrame::nsMathMLmunderoverFrame()
{
}

nsMathMLmunderoverFrame::~nsMathMLmunderoverFrame()
{
}

NS_IMETHODIMP
nsMathMLmunderoverFrame::Reflow(nsIPresContext&          aPresContext,
                                nsHTMLReflowMetrics&     aDesiredSize,
                                const nsHTMLReflowState& aReflowState,
                                nsReflowStatus&          aStatus)
{
  nsresult rv = NS_OK;
  nsReflowStatus childStatus;
  nsHTMLReflowMetrics childDesiredSize(aDesiredSize.maxElementSize);
  nsSize availSize(aReflowState.mComputedWidth, aReflowState.mComputedHeight);

  aDesiredSize.width = 0;

  //////////////////
  // Reflow Children
   
  nscoord count = 0;
  nsRect rect[3];
  nsIFrame* child[3];
  nsIFrame* childFrame = mFrames.FirstChild(); 
  while (nsnull != childFrame) 
  {
    //////////////
    // WHITESPACE: don't forget that whitespace doesn't count in MathML!
    if (IsOnlyWhitespace(childFrame)) {
      ReflowEmptyChild(aPresContext, childFrame);
    }
    else if (3 > count) {
      nsHTMLReflowState childReflowState(aPresContext, aReflowState, 
                                         childFrame, availSize);
      rv = ReflowChild(childFrame, aPresContext, childDesiredSize, 
                       childReflowState, childStatus);
      NS_ASSERTION(NS_FRAME_IS_COMPLETE(childStatus), "bad status");

      if (NS_FAILED(rv)) {
        return rv;
      }

      child[count] = childFrame;     
      rect[count].width = childDesiredSize.width;
      rect[count].height = childDesiredSize.height;
      
      if (aDesiredSize.width < childDesiredSize.width) {
        aDesiredSize.width = childDesiredSize.width;
      }
      if (0 == count) {
        aDesiredSize.ascent = childDesiredSize.ascent;
        aDesiredSize.descent = childDesiredSize.descent;  
      }
      count++;  
    }
//  else { invalid markup... }
  
    rv = childFrame->GetNextSibling(&childFrame);
    NS_ASSERTION(NS_SUCCEEDED(rv),"failed to get next child");
  }

  /////////////
  // Ask stretchy children to stretch themselves
  
  nsStretchDirection stretchDir = NS_STRETCH_DIRECTION_HORIZONTAL;
  nsCharMetrics parentSize(aDesiredSize);
  for (PRInt32 i = 0; i < count; i++) {
    //////////
    // Stretch ...
    // Only directed at frames that implement the nsIMathMLFrame interface
    nsIMathMLFrame* aMathMLFrame = nsnull;
    rv = child[i]->QueryInterface(nsIMathMLFrame::GetIID(), (void**)&aMathMLFrame);
    if (NS_SUCCEEDED(rv) && aMathMLFrame) {
      nsCharMetrics childSize(rect[i].x, rect[i].y, rect[i].width, rect[i].height);
      nsIRenderingContext& renderingContext = *aReflowState.rendContext;
      aMathMLFrame->Stretch(aPresContext, renderingContext, 
                            stretchDir, parentSize, childSize);
      // store the updated metrics
      rect[i] = nsRect(childSize.descent, childSize.ascent,
                       childSize.width, childSize.height);
    }
  }

  //////////////////
  // Place Children 
 
  aDesiredSize.ascent += rect[1].height;
  aDesiredSize.descent += rect[2].height;
  aDesiredSize.height = aDesiredSize.ascent + aDesiredSize.descent;

  rect[0].x = (aDesiredSize.width - rect[0].width) / 2; // center w.r.t largest width
  rect[1].x = (aDesiredSize.width - rect[1].width) / 2;
  rect[2].x = (aDesiredSize.width - rect[2].width) / 2;
  rect[0].y = rect[1].height;
  rect[1].y = aDesiredSize.height - rect[1].height;
  rect[2].y = 0;
  
//ignore the leading (line spacing) that is attached to the text
  nscoord leading;
  nsCOMPtr<nsIFontMetrics> fm;
  const nsStyleFont* aFont =
    (const nsStyleFont*)mStyleContext->GetStyleData(eStyleStruct_Font);
  aPresContext.GetMetricsFor(aFont->mFont, getter_AddRefs(fm));
  fm->GetLeading(leading);
  
  aDesiredSize.height -= 2*leading;
  aDesiredSize.ascent -= leading;
  aDesiredSize.descent -= leading;  
  rect[0].y -= leading;
  rect[1].y -= 2*leading;
//     
  child[0]->SetRect(&aPresContext, rect[0]);
  child[1]->SetRect(&aPresContext, rect[1]); 
  child[2]->SetRect(&aPresContext, rect[2]);

  if (nsnull != aDesiredSize.maxElementSize) {
    aDesiredSize.maxElementSize->width = aDesiredSize.width;
    aDesiredSize.maxElementSize->height = aDesiredSize.height;
  }
  aStatus = NS_FRAME_COMPLETE;
  return NS_OK;
}
