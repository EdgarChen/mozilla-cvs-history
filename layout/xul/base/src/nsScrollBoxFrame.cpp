/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * Author: Eric D Vaughan (evaughan@netscape.com)
 *
 * Contributor(s): 
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */

#include "nsCOMPtr.h"
#include "nsHTMLParts.h"
#include "nsIPresContext.h"
#include "nsIStyleContext.h"
#include "nsIReflowCommand.h"
#include "nsIDeviceContext.h"
#include "nsPageFrame.h"
#include "nsViewsCID.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsHTMLContainerFrame.h"
#include "nsHTMLIIDs.h"
#include "nsCSSRendering.h"
#include "nsIScrollableView.h"
#include "nsWidgetsCID.h"
#include "nsScrollBoxFrame.h"
#include "nsLayoutAtoms.h"
#include "nsIBox.h"
#include "nsBoxLayoutState.h"
#include "nsIBoxToBlockAdaptor.h"
#include "nsISupportsPrimitives.h"
#include "nsIPresState.h"
#include "nsButtonBoxFrame.h"
#include "nsITimerCallback.h"
#include "nsRepeatService.h"

static NS_DEFINE_IID(kWidgetCID, NS_CHILD_CID);
static NS_DEFINE_IID(kScrollBoxViewCID, NS_SCROLL_PORT_VIEW_CID);
static NS_DEFINE_IID(kViewCID, NS_VIEW_CID);

static NS_DEFINE_IID(kIViewIID, NS_IVIEW_IID);
static NS_DEFINE_IID(kScrollViewIID, NS_ISCROLLABLEVIEW_IID);

//----------------------------------------------------------------------

nsresult
NS_NewScrollBoxFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsScrollBoxFrame* it = new (aPresShell) nsScrollBoxFrame (aPresShell);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  *aNewFrame = it;
  return NS_OK;
}

nsScrollBoxFrame::nsScrollBoxFrame(nsIPresShell* aShell):nsBoxFrame(aShell), mVerticalOverflow(PR_FALSE), mHorizontalOverflow(PR_FALSE)
{
}


PRBool
nsScrollBoxFrame::NeedsClipWidget()
{
  return PR_TRUE;
}

NS_IMETHODIMP
nsScrollBoxFrame::Init(nsIPresContext*  aPresContext,
                    nsIContent*      aContent,
                    nsIFrame*        aParent,
                    nsIStyleContext* aStyleContext,
                    nsIFrame*        aPrevInFlow)
{
  nsresult  rv = nsBoxFrame::Init(aPresContext, aContent,
                                            aParent, aStyleContext,
                                            aPrevInFlow);

  // Create the scrolling view
  CreateScrollingView(aPresContext);
  return rv;
}
  
NS_IMETHODIMP
nsScrollBoxFrame::SetInitialChildList(nsIPresContext* aPresContext,
                                   nsIAtom*        aListName,
                                   nsIFrame*       aChildList)
{
  nsresult  rv = nsBoxFrame::SetInitialChildList(aPresContext, aListName,
                                                           aChildList);

  SetUpScrolledFrame(aPresContext);

  return rv;
}

void
nsScrollBoxFrame::SetUpScrolledFrame(nsIPresContext* aPresContext)
{
  NS_ASSERTION(mFrames.GetLength() <= 1, "ScrollBoxes can only have 1 child!");


  nsIFrame* frame = mFrames.FirstChild();

  if (!frame)
     return;

  // create a view if we don't already have one.
  nsCOMPtr<nsIStyleContext> context;
  frame->GetStyleContext(getter_AddRefs(context));
  nsHTMLContainerFrame::CreateViewForFrame(aPresContext, frame,
                                           context, PR_TRUE);

  // We need to allow the view's position to be different than the
  // frame's position
  nsFrameState  state;
  frame->GetFrameState(&state);
  state &= ~NS_FRAME_SYNC_FRAME_AND_VIEW;
  frame->SetFrameState(state);
}

NS_IMETHODIMP
nsScrollBoxFrame::AppendFrames(nsIPresContext* aPresContext,
                            nsIPresShell&   aPresShell,
                            nsIAtom*        aListName,
                            nsIFrame*       aFrameList)
{
  nsresult rv = nsBoxFrame::AppendFrames(aPresContext, aPresShell, aListName, aFrameList);

#ifdef DEBUG
  // make sure we only have 1 child.
  nsIFrame* frame = mFrames.FirstChild();
  frame->GetNextSibling(&frame);
  NS_ASSERTION(!frame, "Error ScrollBoxes can only have 1 child");
#endif

  SetUpScrolledFrame(aPresContext);

  return rv;
}

NS_IMETHODIMP
nsScrollBoxFrame::InsertFrames(nsIPresContext* aPresContext,
                            nsIPresShell&   aPresShell,
                            nsIAtom*        aListName,
                            nsIFrame*       aPrevFrame,
                            nsIFrame*       aFrameList)
{
  nsresult rv = nsBoxFrame::InsertFrames(aPresContext, aPresShell, aListName, aPrevFrame, aFrameList);


#ifdef DEBUG
  // make sure we only have 1 child.
  nsIFrame* frame = mFrames.FirstChild();
  frame->GetNextSibling(&frame);
  NS_ASSERTION(!frame, "Error ScrollBoxes can only have 1 child");
#endif

  SetUpScrolledFrame(aPresContext);

  return rv;
}

NS_IMETHODIMP
nsScrollBoxFrame::RemoveFrame(nsIPresContext* aPresContext,
                           nsIPresShell&   aPresShell,
                           nsIAtom*        aListName,
                           nsIFrame*       aOldFrame)
{
  nsresult rv = nsBoxFrame::RemoveFrame(aPresContext, aPresShell, aListName, aOldFrame);

  SetUpScrolledFrame(aPresContext);

  return rv;
}

nsresult
nsScrollBoxFrame::CreateScrollingViewWidget(nsIView* aView, const nsStylePosition* aPosition)
{
  nsresult rv = NS_OK;
   // If it's fixed positioned, then create a widget 
  if (NS_STYLE_POSITION_FIXED == aPosition->mPosition) {
    rv = aView->CreateWidget(kWidgetCID);
  }

  return(rv);
}

nsresult
nsScrollBoxFrame::GetScrollingParentView(nsIPresContext* aPresContext,
                                          nsIFrame* aParent,
                                          nsIView** aParentView)
{
  nsresult rv = aParent->GetView(aPresContext, aParentView);
  NS_ASSERTION(aParentView, "GetParentWithView failed");
  return(rv);
}

nsresult
nsScrollBoxFrame::CreateScrollingView(nsIPresContext* aPresContext)
{
  nsIView*  view;

   //Get parent frame
  nsIFrame* parent;
  GetParentWithView(aPresContext, &parent);
  NS_ASSERTION(parent, "GetParentWithView failed");

  // Get parent view
  nsIView* parentView = nsnull;
  GetScrollingParentView(aPresContext, parent, &parentView);
 
  // Get the view manager
  nsIViewManager* viewManager;
  parentView->GetViewManager(viewManager);

 
  // Create the scrolling view
  nsresult rv = nsComponentManager::CreateInstance(kScrollBoxViewCID, 
                                             nsnull, 
                                             kIViewIID, 
                                             (void **)&view);


 
  
  

  if (NS_OK == rv) {
    const nsStylePosition* position = (const nsStylePosition*)
      mStyleContext->GetStyleData(eStyleStruct_Position);
    const nsStyleColor*    color = (const nsStyleColor*)
      mStyleContext->GetStyleData(eStyleStruct_Color);
    const nsStyleSpacing*  spacing = (const nsStyleSpacing*)
      mStyleContext->GetStyleData(eStyleStruct_Spacing);
    const nsStyleDisplay*  display = (const nsStyleDisplay*)
      mStyleContext->GetStyleData(eStyleStruct_Display);

    // Get the z-index
    PRInt32 zIndex = 0;

    if (eStyleUnit_Integer == position->mZIndex.GetUnit()) {
      zIndex = position->mZIndex.GetIntValue();
    }

    // Initialize the scrolling view
    view->Init(viewManager, mRect, parentView, display->IsVisibleOrCollapsed() ?
               nsViewVisibility_kShow : nsViewVisibility_kHide);

    // Insert the view into the view hierarchy
    viewManager->InsertChild(parentView, view, zIndex);

    // Set the view's opacity
    viewManager->SetViewOpacity(view, color->mOpacity);

    // Because we only paint the border and we don't paint a background,
    // inform the view manager that we have transparent content
    viewManager->SetViewContentTransparency(view, PR_TRUE);

    // If it's fixed positioned, then create a widget too
    CreateScrollingViewWidget(view, position);

    // Get the nsIScrollableView interface
    nsIScrollableView* scrollingView;
    view->QueryInterface(kScrollViewIID, (void**)&scrollingView);

    scrollingView->SetScrollPreference(nsScrollPreference_kNeverScroll);

    // Have the scrolling view create its internal widgets
    scrollingView->CreateScrollControls(); 

    // Set the scrolling view's insets to whatever our border is
    nsMargin border;
    if (!spacing->GetBorder(border)) {
      NS_NOTYETIMPLEMENTED("percentage border");
      border.SizeTo(0, 0, 0, 0);
    }
    scrollingView->SetControlInsets(border);

    // Remember our view
    SetView(aPresContext, view);
  }

  NS_RELEASE(viewManager);
  return rv;
}

NS_IMETHODIMP
nsScrollBoxFrame::GetMargin(nsMargin& aMargin)
{
   aMargin.SizeTo(0,0,0,0);
   return NS_OK;
}

NS_IMETHODIMP
nsScrollBoxFrame::GetPadding(nsMargin& aMargin)
{
   aMargin.SizeTo(0,0,0,0);
   return NS_OK;
}

NS_IMETHODIMP
nsScrollBoxFrame::GetBorder(nsMargin& aMargin)
{
   aMargin.SizeTo(0,0,0,0);
   return NS_OK;
}

NS_IMETHODIMP
nsScrollBoxFrame::Layout(nsBoxLayoutState& aState)
{
  PRUint32 flags = 0;
  aState.GetLayoutFlags(flags);

  nsRect clientRect(0,0,0,0);
  GetClientRect(clientRect);
  nsIBox* kid = nsnull;
  GetChildBox(&kid);
  nsRect childRect(clientRect);
  nsMargin margin(0,0,0,0);
  kid->GetMargin(margin);
  childRect.Deflate(margin);
  nsSize min(0,0);
  kid->GetMinSize(aState, min);

  /*
  // if our child is not html then get is min size
  // and make sure we don't squeeze it smaller than that.
 nsIBoxToBlockAdaptor* adaptor = nsnull;
 if (NS_FAILED(kid->QueryInterface(NS_GET_IID(nsIBoxToBlockAdaptor), (void**)&adaptor))) {
    if (min.height > childRect.height)
       childRect.height = min.height;
 }
 */

 if (min.height > childRect.height)
     childRect.height = min.height;

 if (min.width > childRect.width)
    childRect.width = min.width;

  aState.SetLayoutFlags(NS_FRAME_NO_MOVE_VIEW);
  kid->SetBounds(aState, childRect);
  kid->Layout(aState);

  kid->GetBounds(childRect);

  clientRect.Inflate(margin);

  if (childRect.width < clientRect.width || childRect.height < childRect.height)
  {
    if (childRect.width < clientRect.width)
       childRect.width = clientRect.width;

    if (childRect.height < clientRect.height)
       childRect.height = clientRect.height;

    clientRect.Deflate(margin);

    kid->SetBounds(aState, childRect);
  }

  aState.SetLayoutFlags(flags);

  SyncLayout(aState);

  nsIPresContext* presContext = aState.GetPresContext();
  nsIScrollableView* scrollingView;
  nsIView*           view;
  GetView(presContext, &view);
  if (NS_SUCCEEDED(view->QueryInterface(kScrollViewIID, (void**)&scrollingView))) {
    scrollingView->ComputeScrollOffsets(PR_TRUE);
  }

  nsRect scrollPort;
  GetBounds(scrollPort);

  kid->GetBounds(childRect);

  // first see what changed
  PRBool vertChanged = PR_FALSE;
  PRBool horizChanged = PR_FALSE;

  if (mVerticalOverflow && childRect.height <= scrollPort.height) {
    mVerticalOverflow = PR_FALSE;
    vertChanged = PR_TRUE;
  } else if (!mVerticalOverflow && childRect.height > scrollPort.height) {
    mVerticalOverflow = PR_TRUE;
    vertChanged = PR_TRUE;
  }

  if (mHorizontalOverflow && childRect.width <= scrollPort.width) {
    mHorizontalOverflow = PR_FALSE;
    horizChanged = PR_TRUE;
  } else if (!mHorizontalOverflow && childRect.width > scrollPort.width) {
    mHorizontalOverflow = PR_TRUE;
    horizChanged = PR_TRUE;
  }

  nsCOMPtr<nsIPresShell> shell;
  aState.GetPresShell(getter_AddRefs(shell));

  // if either changed
  if (vertChanged || horizChanged) 
  {
    // are there 2 events or 1?
    if (vertChanged && horizChanged) {
      if (mVerticalOverflow == mHorizontalOverflow)
      {
        // both either overflowed or underflowed. 1 event
        PostScrollPortEvent(shell, mVerticalOverflow, nsScrollPortEvent::both);
      } else {
        // one overflowed and one underflowed
        PostScrollPortEvent(shell, mVerticalOverflow, nsScrollPortEvent::vertical);
        PostScrollPortEvent(shell, mHorizontalOverflow, nsScrollPortEvent::horizontal);
      }
    } else if (vertChanged) // only one changed either vert or horiz
       PostScrollPortEvent(shell, mVerticalOverflow, nsScrollPortEvent::vertical);
    else
       PostScrollPortEvent(shell, mHorizontalOverflow, nsScrollPortEvent::horizontal);
  }

  return NS_OK;
}

void
nsScrollBoxFrame::PostScrollPortEvent(nsIPresShell* aShell, PRBool aOverflow, nsScrollPortEvent::orientType aType)
{
  if (!mContent)
    return;

  nsScrollPortEvent* event = new nsScrollPortEvent();
  event->eventStructType = NS_SCROLLPORT_EVENT;  
  event->widget = nsnull;
  event->orient = aType;
  event->nativeMsg = nsnull;
  event->message = aOverflow ? NS_SCROLLPORT_OVERFLOW : NS_SCROLLPORT_UNDERFLOW;

  aShell->PostDOMEvent(mContent, event);
}


NS_IMETHODIMP
nsScrollBoxFrame::GetAscent(nsBoxLayoutState& aState, nscoord& aAscent)
{
  aAscent = 0;
  nsIBox* child = nsnull;
  GetChildBox(&child);
  nsresult rv = child->GetAscent(aState, aAscent);
  nsMargin m(0,0,0,0);
  GetBorderAndPadding(m);
  aAscent += m.top;
  GetMargin(m);
  aAscent += m.top;
  GetInset(m);
  aAscent += m.top;

  return rv;
}

NS_IMETHODIMP
nsScrollBoxFrame::GetPrefSize(nsBoxLayoutState& aBoxLayoutState, nsSize& aSize)
{
  nsIBox* child = nsnull;
  GetChildBox(&child);
  nsresult rv = child->GetPrefSize(aBoxLayoutState, aSize);
  AddMargin(child, aSize);
  AddBorderAndPadding(aSize);
  AddInset(aSize);
  nsIBox::AddCSSPrefSize(aBoxLayoutState, this, aSize);
  return rv;
}

NS_IMETHODIMP
nsScrollBoxFrame::GetMinSize(nsBoxLayoutState& aBoxLayoutState, nsSize& aSize)
{
  aSize.width = 0;
  aSize.height = 0;
  AddBorderAndPadding(aSize);
  AddInset(aSize);
  nsIBox::AddCSSMinSize(aBoxLayoutState, this, aSize);
  return NS_OK;
}

NS_IMETHODIMP
nsScrollBoxFrame::GetMaxSize(nsBoxLayoutState& aBoxLayoutState, nsSize& aSize)
{
  nsIBox* child = nsnull;
  GetChildBox(&child);
  nsresult rv = child->GetMaxSize(aBoxLayoutState, aSize);
  AddMargin(child, aSize);
  AddBorderAndPadding(aSize);
  AddInset(aSize);
  nsIBox::AddCSSMaxSize(aBoxLayoutState, this, aSize);
  return rv;
}

NS_IMETHODIMP
nsScrollBoxFrame::Paint(nsIPresContext*      aPresContext,
                     nsIRenderingContext& aRenderingContext,
                     const nsRect&        aDirtyRect,
                     nsFramePaintLayer    aWhichLayer)
{
    if (NS_FRAME_PAINT_LAYER_BACKGROUND == aWhichLayer) {
    // Only paint the border and background if we're visible
    const nsStyleDisplay* display = (const nsStyleDisplay*)
      mStyleContext->GetStyleData(eStyleStruct_Display);

    if (display->IsVisibleOrCollapsed()) {
      // Paint our border only (no background)
      const nsStyleSpacing* spacing = (const nsStyleSpacing*)
        mStyleContext->GetStyleData(eStyleStruct_Spacing);

      nsRect  rect(0, 0, mRect.width, mRect.height);
      nsCSSRendering::PaintBorder(aPresContext, aRenderingContext, this,
                                  aDirtyRect, rect, *spacing, mStyleContext, 0);
    }
  }

  // Paint our children
  nsresult rv = nsBoxFrame::Paint(aPresContext, aRenderingContext, aDirtyRect,
                                 aWhichLayer);

  return rv;
}

PRIntn
nsScrollBoxFrame::GetSkipSides() const
{
  return 0;
}

nsresult 
nsScrollBoxFrame::GetContentOf(nsIContent** aContent)
{
    return GetContent(aContent);
}

NS_IMETHODIMP
nsScrollBoxFrame::GetFrameType(nsIAtom** aType) const
{
  NS_PRECONDITION(nsnull != aType, "null OUT parameter pointer");
  *aType = nsLayoutAtoms::scrollFrame; 
  NS_ADDREF(*aType);
  return NS_OK;
}

#ifdef NS_DEBUG
NS_IMETHODIMP
nsScrollBoxFrame::GetFrameName(nsString& aResult) const
{
  return MakeFrameName("ScrollBox", aResult);
}
#endif

NS_IMETHODIMP_(nsrefcnt) 
nsScrollBoxFrame::AddRef(void)
{
  return NS_OK;
}

NS_IMETHODIMP_(nsrefcnt)
nsScrollBoxFrame::Release(void)
{
    return NS_OK;
}

//----------------------------------------------------------------------
// nsIStatefulFrame
//----------------------------------------------------------------------
NS_IMETHODIMP
nsScrollBoxFrame::GetStateType(nsIPresContext* aPresContext,
                                 nsIStatefulFrame::StateType* aStateType)
{
  *aStateType = nsIStatefulFrame::eScrollType;
  return NS_OK;
}

//----------------------------------------------------------------------
NS_IMETHODIMP
nsScrollBoxFrame::SaveState(nsIPresContext* aPresContext,
                              nsIPresState** aState)
{
  nsresult res = NS_OK;
  PRInt32 x,y;
  nsIScrollableView* scrollingView;
  nsIView*           view;
  GetView(aPresContext, &view);
  if (NS_SUCCEEDED(view->QueryInterface(kScrollViewIID, (void**)&scrollingView))) {
    scrollingView->GetScrollPosition(x,y);
  }
  nsIView* child = nsnull;
  nsRect childRect(0,0,0,0);
  if (NS_SUCCEEDED(scrollingView->GetScrolledView(child)) && child) {
    child->GetBounds(childRect);
  }

  res = NS_NewPresState(aState);
  nsCOMPtr<nsISupportsPRInt32> xoffset;
  if (NS_SUCCEEDED(res)) {
    res = nsComponentManager::CreateInstance(NS_SUPPORTS_PRINT32_PROGID,
            nsnull, NS_GET_IID(nsISupportsPRInt32), (void**)getter_AddRefs(xoffset));
    if (NS_SUCCEEDED(res) && xoffset) {
      res = xoffset->SetData(x);
      if (NS_SUCCEEDED(res)) {
        (*aState)->SetStatePropertyAsSupports(NS_ConvertASCIItoUCS2("x-offset"), xoffset);
      }
    }
  }
  nsCOMPtr<nsISupportsPRInt32> yoffset;
  if (NS_SUCCEEDED(res)) {
    res = nsComponentManager::CreateInstance(NS_SUPPORTS_PRINT32_PROGID,
            nsnull, NS_GET_IID(nsISupportsPRInt32), (void**)getter_AddRefs(yoffset));
    if (NS_SUCCEEDED(res) && yoffset) {
      res = yoffset->SetData(y);
      if (NS_SUCCEEDED(res)) {
        (*aState)->SetStatePropertyAsSupports(NS_ConvertASCIItoUCS2("y-offset"), yoffset);
      }
    }
  }
  nsCOMPtr<nsISupportsPRInt32> width;
  if (NS_SUCCEEDED(res)) {
    res = nsComponentManager::CreateInstance(NS_SUPPORTS_PRINT32_PROGID,
            nsnull, NS_GET_IID(nsISupportsPRInt32), (void**)getter_AddRefs(width));
    if (NS_SUCCEEDED(res) && width) {
      res = width->SetData(childRect.width);
      if (NS_SUCCEEDED(res)) {
        (*aState)->SetStatePropertyAsSupports(NS_ConvertASCIItoUCS2("width"), width);
      }
    }
  }
  nsCOMPtr<nsISupportsPRInt32> height;
  if (NS_SUCCEEDED(res)) {
    res = nsComponentManager::CreateInstance(NS_SUPPORTS_PRINT32_PROGID,
            nsnull, NS_GET_IID(nsISupportsPRInt32), (void**)getter_AddRefs(height));
    if (NS_SUCCEEDED(res) && height) {
      res = height->SetData(childRect.height);
      if (NS_SUCCEEDED(res)) {
        (*aState)->SetStatePropertyAsSupports(NS_ConvertASCIItoUCS2("height"), height);
      }
    }
  }
  return res;
}

//-----------------------------------------------------------
NS_IMETHODIMP
nsScrollBoxFrame::RestoreState(nsIPresContext* aPresContext,
                                 nsIPresState* aState)
{
  nsCOMPtr<nsISupportsPRInt32> xoffset;
  nsCOMPtr<nsISupportsPRInt32> yoffset;
  nsCOMPtr<nsISupportsPRInt32> width;
  nsCOMPtr<nsISupportsPRInt32> height;
  aState->GetStatePropertyAsSupports(NS_ConvertASCIItoUCS2("x-offset"), getter_AddRefs(xoffset));
  aState->GetStatePropertyAsSupports(NS_ConvertASCIItoUCS2("y-offset"), getter_AddRefs(yoffset));
  aState->GetStatePropertyAsSupports(NS_ConvertASCIItoUCS2("width"), getter_AddRefs(width));
  aState->GetStatePropertyAsSupports(NS_ConvertASCIItoUCS2("height"), getter_AddRefs(height));

  nsresult res = NS_ERROR_NULL_POINTER;
  if (xoffset && yoffset) {
    PRInt32 x,y,w,h;
    res = xoffset->GetData(&x);
    if (NS_SUCCEEDED(res))
      res = yoffset->GetData(&y);
    if (NS_SUCCEEDED(res))
      res = width->GetData(&w);
    if (NS_SUCCEEDED(res))
      res = height->GetData(&h);

    if (NS_SUCCEEDED(res)) {

      nsIScrollableView* scrollingView;
      nsIView*           view;
      GetView(aPresContext, &view);
      if (NS_SUCCEEDED(view->QueryInterface(kScrollViewIID, (void**)&scrollingView))) {

        nsIView* child = nsnull;
        nsRect childRect(0,0,0,0);
        if (NS_SUCCEEDED(scrollingView->GetScrolledView(child)) && child) {
          child->GetBounds(childRect);
        }
        x = (int)(((float)childRect.width / w) * x);
        y = (int)(((float)childRect.height / h) * y);

        scrollingView->ScrollTo(x,y,0);
      }
    }
  }

  return res;
}


NS_INTERFACE_MAP_BEGIN(nsScrollBoxFrame)
  NS_INTERFACE_MAP_ENTRY(nsIBox)
  NS_INTERFACE_MAP_ENTRY(nsIStatefulFrame)
#ifdef NS_DEBUG
  NS_INTERFACE_MAP_ENTRY(nsIFrameDebug)
#endif
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIBox)
NS_INTERFACE_MAP_END_INHERITING(nsBoxFrame)


class nsAutoRepeatBoxFrame : public nsButtonBoxFrame, 
                             public nsITimerCallback
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  nsAutoRepeatBoxFrame(nsIPresShell* aPresShell);
  friend nsresult NS_NewAutoRepeatBoxFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame);

  NS_IMETHOD Destroy(nsIPresContext* aPresContext);

  NS_IMETHOD HandleEvent(nsIPresContext* aPresContext, 
                         nsGUIEvent* aEvent,
                         nsEventStatus* aEventStatus);

  NS_IMETHOD Init(nsIPresContext*  aPresContext,
              nsIContent*      aContent,
              nsIFrame*        aParent,
              nsIStyleContext* aContext,
              nsIFrame*        aPrevInFlow);

  NS_IMETHOD_(void) Notify(nsITimer *timer);

  nsIPresContext* mPresContext;
  
};

nsresult
NS_NewAutoRepeatBoxFrame ( nsIPresShell* aPresShell, nsIFrame** aNewFrame )
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsAutoRepeatBoxFrame* it = new (aPresShell) nsAutoRepeatBoxFrame (aPresShell);
  if (nsnull == it)
    return NS_ERROR_OUT_OF_MEMORY;

  *aNewFrame = it;
  return NS_OK;
  
} // NS_NewScrollBarButtonFrame


nsAutoRepeatBoxFrame::nsAutoRepeatBoxFrame(nsIPresShell* aPresShell)
:nsButtonBoxFrame(aPresShell)
{
}

NS_IMETHODIMP
nsAutoRepeatBoxFrame::Init(nsIPresContext*  aPresContext,
              nsIContent*      aContent,
              nsIFrame*        aParent,
              nsIStyleContext* aContext,
              nsIFrame*        aPrevInFlow)
{
  mPresContext = aPresContext;
  return nsButtonBoxFrame::Init(aPresContext, aContent, aParent, aContext, aPrevInFlow);
}

NS_INTERFACE_MAP_BEGIN(nsAutoRepeatBoxFrame)
  NS_INTERFACE_MAP_ENTRY(nsITimerCallback)
NS_INTERFACE_MAP_END_INHERITING(nsButtonBoxFrame)

NS_IMPL_ADDREF_INHERITED(nsAutoRepeatBoxFrame, nsButtonBoxFrame)
NS_IMPL_RELEASE_INHERITED(nsAutoRepeatBoxFrame, nsButtonBoxFrame)

NS_IMETHODIMP
nsAutoRepeatBoxFrame::HandleEvent(nsIPresContext* aPresContext, 
                                      nsGUIEvent* aEvent,
                                      nsEventStatus* aEventStatus)
{  
  switch(aEvent->message)
  {
    case NS_MOUSE_ENTER:
    case NS_MOUSE_ENTER_SYNTH:
       nsRepeatService::GetInstance()->Start(this);
    break;

    case NS_MOUSE_EXIT:
    case NS_MOUSE_EXIT_SYNTH:
       nsRepeatService::GetInstance()->Stop();
    break;
  }
     
  return nsButtonBoxFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
}

NS_IMETHODIMP_(void) 
nsAutoRepeatBoxFrame::Notify(nsITimer *timer)
{
  MouseClicked(mPresContext);
}

NS_IMETHODIMP
nsAutoRepeatBoxFrame::Destroy(nsIPresContext* aPresContext)
{
  // Ensure our repeat service isn't going... it's possible that a scrollbar can disappear out
  // from under you while you're in the process of scrolling.
  nsRepeatService::GetInstance()->Stop();
  return nsButtonBoxFrame::Destroy(aPresContext);
}
