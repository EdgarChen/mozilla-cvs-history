/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */
#include "nsInlineFrame.h"
#include "nsBlockFrame.h"
#include "nsSize.h"
#include "nsIContent.h"
#include "nsIContentDelegate.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIPresContext.h"
#include "nsIPresShell.h"
#include "nsCSSLayout.h"
#include "nsPlaceholderFrame.h"
#include "nsReflowCommand.h"
#include "nsHTMLAtoms.h"
#include "nsAbsoluteFrame.h"
#include "nsLeafFrame.h"
#include "nsIPtr.h"

// XXX To Do:
// 2. horizontal child margins
// 3. borders/padding and splitting
// 4. child relative positioning
// 5. absolutely positioned children
// 6. direction support
// 7. CSS line-height property

/* XXX */
#include "nsHTMLIIDs.h"
#include "nsBlockFrame.h"

#define DEFAULT_ASCENT_LEN  10

NS_DEF_PTR(nsIStyleContext);
NS_DEF_PTR(nsIContent);
NS_DEF_PTR(nsIContentDelegate);

class nsInlineState
{
public:
  nsStyleFont*         font;           // style font
  nsMargin             borderPadding;  // inner margin
  nsSize               mStyleSize;
  PRIntn               mStyleSizeFlags;
  nsSize               availSize;      // available space in which to reflow (starts as max size minus insets)
  nsSize*              maxElementSize; // maximum element size (may be null)
  nscoord              x;              // running x-offset (starts at left inner edge)
  const nscoord        y;              // y-offset (top inner edge)
  nscoord              maxAscent;      // max child ascent
  nscoord              maxDescent;     // max child descent
  nscoord*             ascents;        // ascent information for each child
  PRBool               unconstrainedWidth;
  PRBool               unconstrainedHeight;
  nsLineLayout*        lineLayout;
  PRBool               atBreakPoint;
  const nsReflowState& reflowState;

  // Constructor
  nsInlineState(nsIPresContext*      aPresContext,
                nsIFrame*            aInlineFrame,
                nsStyleFont*         aStyleFont,
                const nsMargin&      aBorderPadding,
                const nsReflowState& aReflowState,
                nsSize*              aMaxElementSize)
    : x(aBorderPadding.left),  // determined by inner edge
      y(aBorderPadding.top),   // determined by inner edge
      reflowState(aReflowState)
  {
    nsBlockReflowState* brs =
      nsBlockFrame::FindBlockReflowState(aPresContext, aInlineFrame);
    lineLayout = brs->mCurrentLine;
    atBreakPoint = PR_TRUE;

    font = aStyleFont;
    borderPadding = aBorderPadding;

    unconstrainedWidth = PRBool(reflowState.maxSize.width == NS_UNCONSTRAINEDSIZE);
    unconstrainedHeight = PRBool(reflowState.maxSize.height == NS_UNCONSTRAINEDSIZE);

    // If we're constrained adjust the available size so it excludes space
    // needed for border/padding
    availSize.width = reflowState.maxSize.width;
    if (PR_FALSE == unconstrainedWidth) {
      availSize.width -= aBorderPadding.left + aBorderPadding.right;
    }
    availSize.height = reflowState.maxSize.height;
    if (PR_FALSE == unconstrainedHeight) {
      availSize.height -= aBorderPadding.top + aBorderPadding.bottom;
    }

    // Initialize max element size
    maxElementSize = aMaxElementSize;
    if (nsnull != maxElementSize) {
      maxElementSize->SizeTo(0, 0);
    }

    ascents = ascentBuf;
    maxAscent = 0;
    maxDescent = 0;
  }

  void SetNumAscents(PRIntn aNumAscents) {
    // We keep around ascent information so that we can vertically align
    // children after we figure out how many children fit.
    if (aNumAscents > DEFAULT_ASCENT_LEN) {
      ascents = new nscoord[aNumAscents];
    }
  }

  // Destructor
 ~nsInlineState() {
    if (ascents != ascentBuf) {
      delete ascents;
    }
  }

private:
  nscoord   ascentBuf[DEFAULT_ASCENT_LEN];
};

/////////////////////////////////////////////////////////////////////////////
//

nsresult nsInlineFrame::NewFrame(nsIFrame**  aInstancePtrResult,
                                 nsIContent* aContent,
                                 nsIFrame*   aParent)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null ptr");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }
  nsIFrame* it = new nsInlineFrame(aContent, aParent);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  *aInstancePtrResult = it;
  return NS_OK;
}

nsInlineFrame::nsInlineFrame(nsIContent* aContent, nsIFrame* aParent)
  : nsHTMLContainerFrame(aContent, aParent)
{
  NS_PRECONDITION(!IsPseudoFrame(), "can not be a pseudo frame");
}

nsInlineFrame::~nsInlineFrame()
{
}

void nsInlineFrame::PlaceChild(nsIFrame*              aChild,
                               PRInt32                aIndex,
                               nsInlineState&         aState,
                               const nsReflowMetrics& aChildSize,
                               const nsSize*          aChildMaxElementSize)
{
  // Set the child's rect
  aChild->SetRect(nsRect(aState.x, aState.y, aChildSize.width, aChildSize.height));

  // Adjust the running x-offset
  aState.x += aChildSize.width;

  // Update the array of ascents and the max ascent and descent
  aState.ascents[aIndex] = aChildSize.ascent;
  if (aChildSize.ascent > aState.maxAscent) {
    aState.maxAscent = aChildSize.ascent;
  }
  if (aChildSize.descent > aState.maxDescent) {
    aState.maxDescent = aChildSize.descent;
  }

  // If we're constrained then update the available width
  if (!aState.unconstrainedWidth) {
    aState.availSize.width -= aChildSize.width;
  }

  // Update the maximum element size
  if (nsnull != aChildMaxElementSize) {
    if (aChildMaxElementSize->width > aState.maxElementSize->width) {
      aState.maxElementSize->width = aChildMaxElementSize->width;
    }
    if (aChildMaxElementSize->height > aState.maxElementSize->height) {
      aState.maxElementSize->height = aChildMaxElementSize->height;
    }
  }
}

// XXX an image shouldn't be allowed to continue a word but text
// should.  hmmm.

PRBool
nsInlineFrame::CanFitChild(nsIPresContext* aPresContext,
                           nsInlineState& aState,
                           nsIFrame* aChildFrame)
{
  PRBool result = PR_TRUE;
  aState.atBreakPoint = nsnull == aState.lineLayout->mBreakFrame;
  if ((aState.availSize.width <= 0) &&
      (aChildFrame != mFirstChild) && (nsnull != mFirstChild)) {
    // If we are at a breakable position then it's ok to be out of
    // room. Otherwise the child is allowed to fit.
    if (aState.atBreakPoint) {
      result = PR_FALSE;
    }
  }

  NS_FRAME_LOG(NS_FRAME_TRACE_CHILD_REFLOW,
               ("nsInlineFrame::CanFitChild: availSize=%d result=%s",
                aState.availSize.width, result ? "true" : "false"));
  return result;
}

// XXX dup first-child check because of calling code not being nice! ick!
PRBool
nsInlineFrame::DidFitChild(nsIPresContext* aPresContext,
                           nsInlineState& aState,
                           nsIFrame* aChildFrame,
                           nsReflowMetrics& aChildMetrics)
{
  if ((aChildMetrics.width > aState.availSize.width) &&
      (aChildFrame != mFirstChild) && (nsnull != mFirstChild)) {
    // If we *were* at a breakable position then it's ok to be out of
    // room. Otherwise the child is allowed to fit.
    if (aState.atBreakPoint) {
      return PR_FALSE;
    }
  }
  return PR_TRUE;
}

/**
 * Reflow the frames we've already created
 *
 * @param   aPresContext presentation context to use
 * @param   aState current inline state
 * @param   aChildFrame the first child frame to reflow
 * @param   aChildIndex the first child frame's index in the frame list
 * @return  true if we successfully reflowed all the mapped children and false
 *            otherwise, e.g. we pushed children to the next in flow
 */
PRBool nsInlineFrame::ReflowMappedChildrenFrom(nsIPresContext* aPresContext,
                                               nsInlineState&  aState,
                                               nsIFrame*       aChildFrame,
                                               PRInt32         aChildIndex)
{
#ifdef NS_DEBUG
  if (GetVerifyTreeEnable()) {
    VerifyLastIsComplete();
  }
#endif
  NS_PRECONDITION(nsnull != aChildFrame, "no children");

  PRInt32   childCount = aChildIndex;
  nsIFrame* prevKidFrame = nsnull;

  // Remember our original mLastContentIsComplete so that if we end up
  // having to push children, we have the correct value to hand to
  // PushChildren.
  PRBool    originalLastContentIsComplete = mLastContentIsComplete;

  nsSize    kidMaxElementSize;
  nsSize*   pKidMaxElementSize = (nsnull != aState.maxElementSize) ? &kidMaxElementSize : nsnull;
  PRBool    result = PR_TRUE;

  for (nsIFrame* kidFrame = aChildFrame; nsnull != kidFrame; ) {
    nsReflowMetrics kidSize(pKidMaxElementSize);
    nsReflowState   kidReflowState(kidFrame, aState.reflowState, aState.availSize,
                                   eReflowReason_Resize);
    nsReflowStatus  status;

    // Reflow the child into the available space
    PRBool room = CanFitChild(aPresContext, aState, kidFrame);
    if (room) {
      kidFrame->WillReflow(*aPresContext);
      status = ReflowChild(kidFrame, aPresContext, kidSize, kidReflowState);
    }

    // Did the child fit?
    if (!room || !DidFitChild(aPresContext, aState, kidFrame, kidSize)) {
      // The child is too wide to fit in the available space, and it's
      // not our first child

      // Since we are giving the next-in-flow our last child, we
      // give it our original mLastContentIsComplete too (in case we
      // are pushing into an empty next-in-flow)
      PushChildren(kidFrame, prevKidFrame, originalLastContentIsComplete);
      SetLastContentOffset(prevKidFrame);

      result = PR_FALSE;
      break;
    }

    // Place and size the child. We'll deal with vertical alignment when
    // we're all done
    PlaceChild(kidFrame, childCount, aState, kidSize, pKidMaxElementSize);
    childCount++;

    // Remember where we just were in case we end up pushing children
    prevKidFrame = kidFrame;

    // Is the child complete?
    mLastContentIsComplete = NS_FRAME_IS_COMPLETE(status);
    if (NS_FRAME_IS_NOT_COMPLETE(status)) {
      // No, the child isn't complete
      nsIFrame* kidNextInFlow;
       
      kidFrame->GetNextInFlow(kidNextInFlow);
      PRBool lastContentIsComplete = mLastContentIsComplete;
      if (nsnull == kidNextInFlow) {
        // The child doesn't have a next-in-flow so create a continuing
        // frame. This hooks the child into the flow
        nsIFrame* continuingFrame;
         
        nsIStyleContextPtr kidSC;
        kidFrame->GetStyleContext(aPresContext, kidSC.AssignRef());
        kidFrame->CreateContinuingFrame(aPresContext, this, kidSC,
                                        continuingFrame);
        NS_ASSERTION(nsnull != continuingFrame, "frame creation failed");

        // Add the continuing frame to the sibling list
        nsIFrame* nextSib;
         
        kidFrame->GetNextSibling(nextSib);
        continuingFrame->SetNextSibling(nextSib);
        kidFrame->SetNextSibling(continuingFrame);
        if (nsnull == nextSib) {
          // Assume that the continuation frame we just created is
          // complete, for now. It will get reflowed by our
          // next-in-flow (we are going to push it now)
          lastContentIsComplete = PR_TRUE;
        }
      }

      // We've used up all of our available space so push the remaining
      // children to the next-in-flow
      nsIFrame* nextSibling;
       
      kidFrame->GetNextSibling(nextSibling);
      if (nsnull != nextSibling) {
        PushChildren(nextSibling, kidFrame, lastContentIsComplete);
        SetLastContentOffset(prevKidFrame);
      }
      result = PR_FALSE;
      break;
    }

    // Get the next child frame
    kidFrame->GetNextSibling(kidFrame);
  }

  // Update the child count member data
  mChildCount = childCount;
#ifdef NS_DEBUG
  if (GetVerifyTreeEnable()) {
    NS_POSTCONDITION(LengthOf(mFirstChild) == mChildCount, "bad child count");

    nsIFrame* lastChild;
    PRInt32   lastIndexInParent;

    LastChild(lastChild);
    lastChild->GetContentIndex(lastIndexInParent);
    NS_POSTCONDITION(lastIndexInParent == mLastContentOffset, "bad last content offset");
    VerifyLastIsComplete();
  }
#endif
  return result;
}

/**
 * Try and pull-up frames from our next-in-flow
 *
 * @param   aPresContext presentation context to use
 * @param   aState current inline state
 * @return  true if we successfully pulled-up all the children and false
 *            otherwise, e.g. child didn't fit
 */
PRBool nsInlineFrame::PullUpChildren(nsIPresContext* aPresContext,
                                     nsInlineState&  aState)
{
  NS_FRAME_LOG(NS_FRAME_TRACE_PUSH_PULL,
               ("nsInlineFrame::PullUpChildren: [%d,%d,%c] childCount=%d",
                mFirstContentOffset,
                mLastContentOffset,
                mLastContentIsComplete ? 'T' : 'F',
                mChildCount));
#ifdef NS_DEBUG
  if (GetVerifyTreeEnable()) {
    VerifyLastIsComplete();
  }
#endif

  nsInlineFrame* nextInFlow = (nsInlineFrame*)mNextInFlow;
  nsSize         kidMaxElementSize;
  nsSize*        pKidMaxElementSize = (nsnull != aState.maxElementSize) ? &kidMaxElementSize : nsnull;
#ifdef NS_DEBUG
  PRInt32        kidIndex = NextChildOffset();
#endif
  nsIFrame*      prevKidFrame;
   
  LastChild(prevKidFrame);

  PRBool        result = PR_TRUE;

  while (nsnull != nextInFlow) {
    nsReflowMetrics kidSize(pKidMaxElementSize);
    nsReflowStatus  status;

    // Get the next child
    nsIFrame* kidFrame = nextInFlow->mFirstChild;

    // Any more child frames?
    if (nsnull == kidFrame) {
      // No. Any frames on its overflow list?
      if (nsnull != nextInFlow->mOverflowList) {
        // Move the overflow list to become the child list
        NS_ABORT();
        nextInFlow->AppendChildren(nextInFlow->mOverflowList);
        nextInFlow->mOverflowList = nsnull;
        kidFrame = nextInFlow->mFirstChild;
      } else {
        // We've pulled up all the children, so move to the next-in-flow.
        nextInFlow = (nsInlineFrame*)nextInFlow->mNextInFlow;
        continue;
      }
    }

    // XXX if the frame being pulled up is not appropriate (e.g. a block
    // frame) then we should stop! If we have an inline BR tag we should
    // stop too!

    // If there is no room, stop pulling up
    if (!CanFitChild(aPresContext, aState, kidFrame)) {
      result = PR_FALSE;
      break;
    }

// XXX inline reflow avoidance will be done by nsLineLayout
#if XXX
    // See if the child fits in the available space. If it fits or
    // it's splittable then reflow it. The reason we can't just move
    // it is that we still need ascent/descent information
    nsSize            kidFrameSize;
    nsSplittableType  kidIsSplittable;

    kidFrame->GetSize(kidFrameSize);
    kidFrame->IsSplittable(kidIsSplittable);
    if ((kidFrameSize.width > aState.availSize.width) &&
        NS_FRAME_IS_NOT_SPLITTABLE(kidIsSplittable)) {
      result = PR_FALSE;
      break;
    }
#endif

    nsReflowState kidReflowState(kidFrame, aState.reflowState, aState.availSize,
                                 eReflowReason_Resize);
    kidFrame->WillReflow(*aPresContext);
    status = ReflowChild(kidFrame, aPresContext, kidSize, kidReflowState);

    // Did the child fit?
    if (!DidFitChild(aPresContext, aState, kidFrame, kidSize)) {
      // The child is too wide to fit in the available space, and it's
      // not our first child
      result = PR_FALSE;
      break;
    }

    NS_FRAME_LOG(NS_FRAME_TRACE_PUSH_PULL,
                 ("nsInlineFrame::PullUpChildren: pulled frame=%p [%c][%x]",
                  kidFrame,
                  NS_FRAME_IS_COMPLETE(status) ? 'T' : 'F',
                  status));

    // Place and size the child. We'll deal with vertical alignment when
    // we're all done
    PlaceChild(kidFrame, mChildCount, aState, kidSize, pKidMaxElementSize);

    // Remove the frame from its current parent
    kidFrame->GetNextSibling(nextInFlow->mFirstChild);
    nextInFlow->mChildCount--;
    // Update the next-in-flows first content offset
    if (nsnull != nextInFlow->mFirstChild) {
      nextInFlow->SetFirstContentOffset(nextInFlow->mFirstChild);
    }

    // Link the frame into our list of children
    kidFrame->SetGeometricParent(this);
    nsIFrame* kidContentParent;

    kidFrame->GetContentParent(kidContentParent);
    if (nextInFlow == kidContentParent) {
      kidFrame->SetContentParent(this);
    }
    if (nsnull == prevKidFrame) {
      mFirstChild = kidFrame;
      SetFirstContentOffset(kidFrame);
    } else {
      prevKidFrame->SetNextSibling(kidFrame);
    }
    kidFrame->SetNextSibling(nsnull);
    mChildCount++;
    prevKidFrame = kidFrame;

    // Is the child we just pulled up complete?
    mLastContentIsComplete = NS_FRAME_IS_COMPLETE(status);
    if (NS_FRAME_IS_NOT_COMPLETE(status)) {
      // No the child isn't complete
      nsIFrame* kidNextInFlow;

      kidFrame->GetNextInFlow(kidNextInFlow);
      if (nsnull == kidNextInFlow) {
        // The child doesn't have a next-in-flow so create a
        // continuing frame. The creation appends it to the flow and
        // prepares it for reflow.
        nsIFrame* continuingFrame;

        nsIStyleContextPtr kidSC;
        kidFrame->GetStyleContext(aPresContext, kidSC.AssignRef());
        kidFrame->CreateContinuingFrame(aPresContext, this, kidSC,
                                        continuingFrame);
        NS_ASSERTION(nsnull != continuingFrame, "frame creation failed");

        // Add the continuing frame to our sibling list and then push
        // it to the next-in-flow. This ensures the next-in-flow's
        // content offsets and child count are set properly. Note that
        // we can safely assume that the continuation is complete so
        // we pass PR_TRUE into PushChidren in case our next-in-flow
        // was just drained and now needs to know it's
        // mLastContentIsComplete state.
        kidFrame->SetNextSibling(continuingFrame);

        NS_FRAME_LOG(NS_FRAME_TRACE_PUSH_PULL,
                     ("nsInlineFrame::PullUpChildren: pushing frame=%p",
                      continuingFrame));
        PushChildren(continuingFrame, kidFrame, PR_TRUE);

        // After we push the continuation frame we don't need to fuss
        // with mLastContentIsComplete beause the continuation frame
        // is no longer on *our* list.
      }

      // If the child isn't complete then it means that we've used up
      // all of our available space.
      result = PR_FALSE;
      break;
    }
  }

  // Update our last content offset
  if (nsnull != prevKidFrame) {
    NS_ASSERTION(IsLastChild(prevKidFrame), "bad last child");
    SetLastContentOffset(prevKidFrame);
  }

  // We need to make sure the first content offset is correct for any empty
  // next-in-flow frames (frames where we pulled up all the child frames)
  nextInFlow = (nsInlineFrame*)mNextInFlow;
  if ((nsnull != nextInFlow) && (nsnull == nextInFlow->mFirstChild)) {
    // We have at least one empty frame. Did we succesfully pull up all the
    // child frames?
    if (PR_FALSE == result) {
      // No, so we need to adjust the first content offset of all the empty
      // frames
      AdjustOffsetOfEmptyNextInFlows();
#ifdef NS_DEBUG
    } else {
      // Yes, we successfully pulled up all the child frames which means all
      // the next-in-flows must be empty. Do a sanity check
      if (GetVerifyTreeEnable()) {
        while (nsnull != nextInFlow) {
          NS_ASSERTION(nsnull == nextInFlow->mFirstChild,
                       "non-empty next-in-flow");
          nextInFlow = (nsInlineFrame*)nextInFlow->mNextInFlow;
        }
      }
#endif
    }
  }

#ifdef NS_DEBUG
  if (GetVerifyTreeEnable()) {
    VerifyLastIsComplete();
  }
#endif
  NS_FRAME_LOG(NS_FRAME_TRACE_PUSH_PULL,
               ("nsInlineFrame::PullUpChildren: [%d,%d,%c] childCount=%d",
                mFirstContentOffset,
                mLastContentOffset,
                mLastContentIsComplete ? 'T' : 'F',
                mChildCount));
  return result;
}

/**
 * Create new frames for content we haven't yet mapped
 *
 * @param   aPresContext presentation context to use
 * @param   aState current inline state
 * @return  NS_FRAME_COMPLETE if all content has been mapped and 0 if we
 *            should be continued
 */
nsReflowStatus
nsInlineFrame::ReflowUnmappedChildren(nsIPresContext* aPresContext,
                                      nsInlineState&  aState)
{
#ifdef NS_DEBUG
  if (GetVerifyTreeEnable()) {
    VerifyLastIsComplete();
  }
#endif
  nsIFrame*      kidPrevInFlow = nsnull;
  nsReflowStatus result = NS_FRAME_NOT_COMPLETE;

  // If we have no children and we have a prev-in-flow then we need to pick
  // up where it left off. If we have children, e.g. we're being resized, then
  // our content offset should already be set correctly...
  if ((nsnull == mFirstChild) && (nsnull != mPrevInFlow)) {
    nsInlineFrame* prev = (nsInlineFrame*)mPrevInFlow;
    NS_ASSERTION(prev->mLastContentOffset >= prev->mFirstContentOffset,
                 "bad prevInFlow");

    mFirstContentOffset = prev->NextChildOffset();
    if (!prev->mLastContentIsComplete) {
      // Our prev-in-flow's last child is not complete
      prev->LastChild(kidPrevInFlow);
    }
  }
  mLastContentIsComplete = PR_TRUE;

  // Place our children, one at a time until we are out of children
  nsSize    kidMaxElementSize;
  nsSize*   pKidMaxElementSize =
    (nsnull != aState.maxElementSize) ? &kidMaxElementSize : nsnull;
  PRInt32   kidIndex = NextChildOffset();
  nsIFrame* prevKidFrame;

  PRBool breakAfter = PR_FALSE;
  LastChild(prevKidFrame);
  for (;;) {
    // Get the next content object
    nsIContentPtr kid = mContent->ChildAt(kidIndex);
    if (kid.IsNull()) {
      result = NS_FRAME_COMPLETE;
      break;
    }

    // Make sure we still have room left
    if (!CanFitChild(aPresContext, aState, nsnull)) {
      // Note: return status was set to 0 above...
      break;
    }

    // Resolve style for the child
    nsIStyleContextPtr kidStyleContext =
      aPresContext->ResolveStyleContextFor(kid, this);

    // Figure out how we should treat the child
    nsIFrame*        kidFrame;
    nsStyleDisplay*  kidDisplay =
      (nsStyleDisplay*)kidStyleContext->GetData(eStyleStruct_Display);
    nsStylePosition* kidPosition = (nsStylePosition*)
      kidStyleContext->GetData(eStyleStruct_Position);

    // Check whether it wants to floated or absolutely positioned
    nsresult rv;
    if (NS_STYLE_POSITION_ABSOLUTE == kidPosition->mPosition) {
      rv = nsAbsoluteFrame::NewFrame(&kidFrame, kid, this);
      if (NS_OK == rv) {
        kidFrame->SetStyleContext(aPresContext, kidStyleContext);
      }
    } else if (kidDisplay->mFloats != NS_STYLE_FLOAT_NONE) {
      rv = nsPlaceholderFrame::NewFrame(&kidFrame, kid, this);
      if (NS_OK == rv) {
        kidFrame->SetStyleContext(aPresContext, kidStyleContext);
      }
    } else if (nsnull == kidPrevInFlow) {
      nsIContentDelegatePtr kidDel;
      switch (kidDisplay->mDisplay) {
      case NS_STYLE_DISPLAY_BLOCK:
      case NS_STYLE_DISPLAY_LIST_ITEM:
        if (kidIndex != mFirstContentOffset) {
          // We don't allow block elements to be placed in us anywhere
          // other than at our left margin.
          goto done;
        }
        // FALLTHROUGH

      case NS_STYLE_DISPLAY_INLINE:
        kidDel = kid->GetDelegate(aPresContext);
        rv = kidDel->CreateFrame(aPresContext, kid, this,
                                 kidStyleContext, kidFrame);
        break;

      default:
        NS_ASSERTION(nsnull == kidPrevInFlow, "bad prev in flow");
        rv = nsFrame::NewFrame(&kidFrame, kid, this);
        if (NS_OK == rv) {
          kidFrame->SetStyleContext(aPresContext, kidStyleContext);
        }
        break;
      }
    } else {
      rv = kidPrevInFlow->CreateContinuingFrame(aPresContext, this,
                                                kidStyleContext, kidFrame);
    }

    // Try to reflow the child into the available space. It might not
    // fit or might need continuing.
    nsReflowMetrics kidSize(pKidMaxElementSize);
    nsReflowState   kidReflowState(kidFrame, aState.reflowState, aState.availSize,
                                   eReflowReason_Resize);
    kidFrame->WillReflow(*aPresContext);
    nsReflowStatus  status = ReflowChild(kidFrame, aPresContext, kidSize,
                                         kidReflowState);

    // Did the child fit?
    if (!DidFitChild(aPresContext, aState, kidFrame, kidSize)) {
      // The child is too wide to fit in the available space, and it's
      // not our first child. Add the frame to our overflow list
      NS_ASSERTION(nsnull == mOverflowList, "bad overflow list");
      mOverflowList = kidFrame;
      prevKidFrame->SetNextSibling(nsnull);
      break;
    }

    // Place and size the child. We'll deal with vertical alignment when
    // we're all done
    PlaceChild(kidFrame, mChildCount, aState, kidSize, pKidMaxElementSize);

    // Link child frame into the list of children
    if (nsnull != prevKidFrame) {
      prevKidFrame->SetNextSibling(kidFrame);
    } else {
      mFirstChild = kidFrame;  // our first child
      SetFirstContentOffset(kidFrame);
    }
    prevKidFrame = kidFrame;
    mChildCount++;
    kidIndex++;

    // Did the child complete?
    if (NS_FRAME_IS_NOT_COMPLETE(status)) {
      // If the child isn't complete then it means that we've used up
      // all of our available space
      mLastContentIsComplete = PR_FALSE;
      break;
    }
    kidPrevInFlow = nsnull;

    // If we need to break after the kidFrame, then do so now
    if (breakAfter) {
      break;
    }
  }

done:;
  // Update the content mapping
  NS_ASSERTION(IsLastChild(prevKidFrame), "bad last child");
  SetLastContentOffset(prevKidFrame);
#ifdef NS_DEBUG
  if (GetVerifyTreeEnable()) {
    PRInt32 len = LengthOf(mFirstChild);
    NS_ASSERTION(len == mChildCount, "bad child count");
    VerifyLastIsComplete();
  }
#endif
  return result;
}

void
nsInlineFrame::InitializeState(nsIPresContext* aPresContext,
                               nsInlineState& aState)
{
  PRIntn ss = aState.mStyleSizeFlags =
    nsCSSLayout::GetStyleSize(aPresContext, this, aState.mStyleSize);
  if (0 != (ss & NS_SIZE_HAS_WIDTH)) {
    // When we are given a width, change the reflow behavior to
    // unconstrained.
    aState.unconstrainedWidth = PR_TRUE;
    aState.availSize.width = NS_UNCONSTRAINEDSIZE;
  }
}

NS_METHOD nsInlineFrame::Reflow(nsIPresContext*      aPresContext,
                                nsReflowMetrics&     aDesiredSize,
                                const nsReflowState& aReflowState,
                                nsReflowStatus&      aStatus)
{
  NS_FRAME_TRACE_REFLOW_IN("nsInlineFrame::Reflow"); 
#ifdef NS_DEBUG
  if (GetVerifyTreeEnable()) {
    PreReflowCheck();
  }
#endif

  if (eReflowReason_Initial == aReflowState.reason) {
    nsresult rv = ProcessInitialReflow(aPresContext);
    if (NS_OK != rv) {
      return rv;
    }
  }

  PRBool        reflowMappedOK = PR_TRUE;

  aStatus = NS_FRAME_COMPLETE;  // initialize out parameter

  // Get the style molecule
  nsStyleFont* styleFont = (nsStyleFont*)
    mStyleContext->GetData(eStyleStruct_Font);
  nsStyleSpacing* styleSpacing = (nsStyleSpacing*)
    mStyleContext->GetData(eStyleStruct_Spacing);

  // Check for an overflow list
  if (eReflowReason_Incremental != aReflowState.reason) {
    MoveOverflowToChildList();
  }

  // Initialize our reflow state. We must wait until after we've processed
  // the overflow list, because our first content offset might change
  nsMargin borderPadding;
  styleSpacing->CalcBorderPaddingFor(this, borderPadding);
  nsInlineState state(aPresContext, this, styleFont, borderPadding,
                      aReflowState, aDesiredSize.maxElementSize);
  InitializeState(aPresContext, state);
  state.SetNumAscents(mContent->ChildCount() - mFirstContentOffset);

  if (eReflowReason_Incremental == aReflowState.reason) {
    NS_ASSERTION(nsnull != aReflowState.reflowCommand, "null reflow command");
    if (aReflowState.reflowCommand->GetTarget() == this) {
      switch (aReflowState.reflowCommand->GetType()) {
      case nsReflowCommand::FrameAppended:
        // Recover our state
        RecoverState(aPresContext, state, nsnull);
        aStatus = ReflowUnmappedChildren(aPresContext, state);
        break;
      
      default:
        NS_NOTYETIMPLEMENTED("unexpected reflow command");
        break;
      }
  
    } else {
      // The command is passing through us. Get the next frame in the reflow chain
      nsIFrame*       kidFrame = aReflowState.reflowCommand->GetNext();
      nsReflowMetrics kidSize(aDesiredSize.maxElementSize);
      nsReflowState   kidReflowState(kidFrame, aReflowState, state.availSize);
  
      // Restore our state as if nextFrame is the next frame to reflow
      PRInt32 kidIndex = RecoverState(aPresContext, state, kidFrame);
  
      // Reflow the child into the available space
      aStatus = ReflowChild(kidFrame, aPresContext, kidSize, kidReflowState);
  
      // Did the child fit?
      if (!DidFitChild(aPresContext, state, kidFrame, kidSize)) {
        nsIFrame* prevFrame;
  
        // The child is too wide to fit in the available space, and
        // it's not our first child
        PrevChild(kidFrame, prevFrame);
        PushChildren(kidFrame, prevFrame, mLastContentIsComplete);
        SetLastContentOffset(prevFrame);
        mChildCount = kidIndex - 1;
        aStatus = NS_FRAME_NOT_COMPLETE;
  
      } else {
        // Place and size the child
        PlaceChild(kidFrame, kidIndex, state, kidSize, nsnull);
  
        nsIFrame* kidNextInFlow;
        kidFrame->GetNextInFlow(kidNextInFlow);
  
        // Is the child complete?
        if (NS_FRAME_IS_COMPLETE(aStatus)) {
          // Check whether the frame has next-in-flow(s) that are no longer needed
          if (nsnull != kidNextInFlow) {
            // Remove the next-in-flow(s)
            DeleteChildsNextInFlow(kidFrame);
          }
  
          // Adjust the frames that follow
          aStatus = IncrementalReflowAfter(aPresContext, state, kidFrame, kidIndex);
  
        } else {
          nsIFrame* nextSibling;
  
          // No, the child isn't complete
          if (nsnull == kidNextInFlow) {
            // The child doesn't have a next-in-flow so create a continuing
            // frame. 
            nsIFrame* continuingFrame;
  
            nsIStyleContextPtr kidSC;
            kidFrame->GetStyleContext(aPresContext, kidSC.AssignRef());
            kidFrame->CreateContinuingFrame(aPresContext, this, kidSC, continuingFrame);
  
            // Link the child into the sibling list
            kidFrame->GetNextSibling(nextSibling);
            continuingFrame->SetNextSibling(nextSibling);
            kidFrame->SetNextSibling(continuingFrame);
          }
  
          // We've used up all of our available space, so push the remaining
          // children to the next-in-flow
          kidFrame->GetNextSibling(nextSibling);
          if (nsnull != nextSibling) {
            PushChildren(nextSibling, kidFrame, mLastContentIsComplete);
          }
  
          SetLastContentOffset(kidFrame);
          mChildCount = kidIndex;
        }
      }
    }

  } else {
    // Reflow any existing frames
    if (nsnull != mFirstChild) {
      reflowMappedOK = ReflowMappedChildrenFrom(aPresContext, state, mFirstChild, 0);
  
      if (PR_FALSE == reflowMappedOK) {
        // We didn't successfully reflow our mapped frames; therefore, we're not complete
        aStatus = NS_FRAME_NOT_COMPLETE;
      }
    }
  
    // Did we successfully relow our mapped children?
    if (PR_TRUE == reflowMappedOK) {
#if XXX
      // Any space left?
      if (state.availSize.width <= 0) {
        // No space left. Don't try to pull-up children or reflow unmapped
        if (NextChildOffset() < mContent->ChildCount()) {
          // No room left to map the remaining content; therefore, we're not complete
          aStatus = NS_FRAME_NOT_COMPLETE;
        }
      } else
#endif
      if (NextChildOffset() < mContent->ChildCount()) {
        // Try and pull-up some children from a next-in-flow
        if (PullUpChildren(aPresContext, state)) {
          // If we still have unmapped children then create some new frames
          if (NextChildOffset() < mContent->ChildCount()) {
            aStatus = ReflowUnmappedChildren(aPresContext, state);
          }
        } else {
          // We were unable to pull-up all the existing frames from the next in flow;
          // therefore, we're not complete
          aStatus = NS_FRAME_NOT_COMPLETE;
        }
      }
    }
  
  #if 0
    // XXX I don't think our return size properly accounts for the lineHeight
    // (which may not == state.maxAscent + state.maxDescent)
    // Return our size and our status
  #endif
  }

  // Vertically align the children
  nscoord lineHeight =
    nsCSSLayout::VerticallyAlignChildren(aPresContext, this, styleFont,
                                         borderPadding.top,
                                         mFirstChild, mChildCount,
                                         state.ascents, state.maxAscent);

  ComputeFinalSize(aPresContext, state, aDesiredSize);

#ifdef NS_DEBUG
  if (GetVerifyTreeEnable()) {
    PostReflowCheck(aStatus);
  }
#endif
  NS_FRAME_TRACE_REFLOW_OUT("nsInlineFrame::ResizeReflow", aStatus); 
  return NS_OK;
}

void
nsInlineFrame::ComputeFinalSize(nsIPresContext* aPresContext,
                                nsInlineState& aState,
                                nsReflowMetrics& aDesiredSize)
{
  // Compute default size
  aDesiredSize.width = aState.x + aState.borderPadding.right;
  aDesiredSize.ascent = aState.borderPadding.top + aState.maxAscent;
  aDesiredSize.descent = aState.maxDescent + aState.borderPadding.bottom;
  aDesiredSize.height = aDesiredSize.ascent + aDesiredSize.descent;

  // Apply width/height style properties
  PRIntn ss = aState.mStyleSizeFlags;
  if (0 != (ss & NS_SIZE_HAS_WIDTH)) {
    aDesiredSize.width = aState.borderPadding.left + aState.mStyleSize.width +
      aState.borderPadding.right;
  }
  if (0 != (ss & NS_SIZE_HAS_HEIGHT)) {
    aDesiredSize.height = aState.borderPadding.top + aState.mStyleSize.height +
      aState.borderPadding.bottom;
  }
}

/////////////////////////////////////////////////////////////////////////////

PRIntn nsInlineFrame::GetSkipSides() const
{
  PRIntn skip = 0;
  if (nsnull != mPrevInFlow) {
    skip |= 1 << NS_SIDE_LEFT;
  }
  if (nsnull != mNextInFlow) {
    skip |= 1 << NS_SIDE_RIGHT;
  }
  return skip;
}

/////////////////////////////////////////////////////////////////////////////

// Incremental reflow support

NS_METHOD nsInlineFrame::GetReflowMetrics(nsIPresContext* aPresContext,
                                          nsReflowMetrics& aMetrics)
{
  nscoord maxAscent = 0;
  nscoord maxDescent = 0;
  nsIFrame* kid = mFirstChild;
  while (nsnull != kid) {
    // XXX Properly handle max element size...
    nsReflowMetrics kidMetrics(aMetrics.maxElementSize);
    kid->GetReflowMetrics(aPresContext, kidMetrics);
    if (kidMetrics.ascent > maxAscent) maxAscent = kidMetrics.ascent;
    if (kidMetrics.descent > maxDescent) maxDescent = kidMetrics.descent;
    kid->GetNextSibling(kid);
  }

  // XXX what about border & padding
  aMetrics.width = mRect.width;
  aMetrics.height = mRect.height;
  aMetrics.ascent = maxAscent;
  aMetrics.descent = maxDescent;

  return NS_OK;
}

/**
 * Setup aState to the state it would have had we just reflowed our
 * children up to, but not including, aSkipChild. Return the index
 * of aSkipChild in our list of children.
 * If aSkipChild is nsnull then resets the state for appended content.
 */
PRInt32 nsInlineFrame::RecoverState(nsIPresContext* aPresContext,
                                    nsInlineState& aState,
                                    nsIFrame* aSkipChild)
{
  // Get ascent & descent info for all the children up to but not
  // including aSkipChild. Also compute the x coordinate for where
  // aSkipChild will be place after it is reflowed.
  PRInt32 i = 0;
  nsIFrame* kid = mFirstChild;
  nscoord x = aState.x;
  nscoord maxAscent = 0;
  nscoord maxDescent = 0;
  while ((nsnull != kid) && (kid != aSkipChild)) {
    nsReflowMetrics kidMetrics(nsnull);
    kid->GetReflowMetrics(aPresContext, kidMetrics);
    aState.ascents[i] = kidMetrics.ascent;
    if (kidMetrics.ascent > maxAscent) maxAscent = kidMetrics.ascent;
    if (kidMetrics.descent > maxDescent) maxDescent = kidMetrics.descent;

    // XXX Factor in left and right margins
    x += kidMetrics.width;

    kid->GetNextSibling(kid);
    i++;
  }
  aState.maxAscent = maxAscent;
  aState.maxDescent = maxDescent;
  aState.x = x;
  return i;
}

// XXX We need to return information about whether our next-in-flow is
// dirty...
nsReflowStatus
nsInlineFrame::IncrementalReflowFrom(nsIPresContext* aPresContext,
                                     nsInlineState&  aState,
                                     nsIFrame*       aChildFrame,
                                     PRInt32         aChildIndex)
{
  nsReflowStatus  status = NS_FRAME_COMPLETE;

  // Just reflow all the mapped children starting with childFrame.
  // XXX This isn't the optimal thing to do...
  if (ReflowMappedChildrenFrom(aPresContext, aState, aChildFrame, aChildIndex)) {
    if (NextChildOffset() < mContent->ChildCount()) {
#if XXX
      // Any space left?
      if (aState.availSize.width <= 0) {
        // No space left. Don't try to pull-up children
        status = NS_FRAME_NOT_COMPLETE;
      } else
#endif
      {
        // Try and pull-up some children from a next-in-flow
        if (!PullUpChildren(aPresContext, aState)) {
          // We were not able to pull-up all the child frames from our
          // next-in-flow
          status = NS_FRAME_NOT_COMPLETE;
        }
      }
    }
  } else {
    // We were unable to reflow all our mapped frames
    status = NS_FRAME_NOT_COMPLETE;
  }

  return status;
}

nsReflowStatus
nsInlineFrame::IncrementalReflowAfter(nsIPresContext* aPresContext,
                                      nsInlineState&  aState,
                                      nsIFrame*       aChildFrame,
                                      PRInt32         aChildIndex)
{
  nsReflowStatus  status = NS_FRAME_COMPLETE;
  nsIFrame*       nextFrame;

  aChildFrame->GetNextSibling(nextFrame);

  // Just reflow all the remaining mapped children
  // XXX This isn't the optimal thing to do...
  if ((nsnull == nextFrame) || 
      ReflowMappedChildrenFrom(aPresContext, aState, nextFrame, aChildIndex + 1)) {
    if (NextChildOffset() < mContent->ChildCount()) {
#if XXX
      // Any space left?
      if (aState.availSize.width <= 0) {
        // No space left. Don't try to pull-up children
        status = NS_FRAME_NOT_COMPLETE;
      } else
#endif
      {
        // Try and pull-up some children from a next-in-flow
        if (!PullUpChildren(aPresContext, aState)) {
          // We were not able to pull-up all the child frames from our
          // next-in-flow
          status = NS_FRAME_NOT_COMPLETE;
        }
      }
    }
  } else {
    // We were unable to reflow all our mapped frames
    status = NS_FRAME_NOT_COMPLETE;
  }

  return status;
}

// In order to execute the vertical alignment code after incremental
// reflow of the inline frame, we need to reposition any child frames
// that were relatively positioned back to their computed x origin.
// This should probably be done as a pre-alignment computation (and it
// can be avoided if there are no relatively positioned children).
