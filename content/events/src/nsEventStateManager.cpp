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
#include "nsCOMPtr.h"
#include "nsIEventStateManager.h"
#include "nsEventStateManager.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIFrame.h"
#include "nsIWidget.h"
#include "nsIStyleContext.h"
#include "nsIPresContext.h"
#include "nsIPresShell.h"
#include "nsDOMEvent.h"
#include "nsHTMLAtoms.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMHTMLSelectElement.h"
#include "nsIDOMHTMLTextAreaElement.h"
#include "nsIDOMHTMLAreaElement.h"
#include "nsIDOMHTMLButtonElement.h"
#include "nsIDOMHTMLObjectElement.h"
#include "nsINameSpaceManager.h"  // for kNameSpaceID_HTML
#include "nsIWebShell.h"
#include "nsIFocusableContent.h"
#include "nsIScrollableView.h"
#include "nsIDOMSelection.h"
#include "nsIFrameSelection.h"
#include "nsIDeviceContext.h"


static NS_DEFINE_IID(kIEventStateManagerIID, NS_IEVENTSTATEMANAGER_IID);
static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIDOMHTMLAnchorElementIID, NS_IDOMHTMLANCHORELEMENT_IID);
static NS_DEFINE_IID(kIDOMHTMLInputElementIID, NS_IDOMHTMLINPUTELEMENT_IID);
static NS_DEFINE_IID(kIDOMHTMLSelectElementIID, NS_IDOMHTMLSELECTELEMENT_IID);
static NS_DEFINE_IID(kIDOMHTMLTextAreaElementIID, NS_IDOMHTMLTEXTAREAELEMENT_IID);
static NS_DEFINE_IID(kIDOMHTMLAreaElementIID, NS_IDOMHTMLAREAELEMENT_IID);
static NS_DEFINE_IID(kIDOMHTMLObjectElementIID, NS_IDOMHTMLOBJECTELEMENT_IID);
static NS_DEFINE_IID(kIDOMHTMLButtonElementIID, NS_IDOMHTMLBUTTONELEMENT_IID);
static NS_DEFINE_IID(kIWebShellIID, NS_IWEB_SHELL_IID);
static NS_DEFINE_IID(kIFocusableContentIID, NS_IFOCUSABLECONTENT_IID);
static NS_DEFINE_IID(kIScrollableViewIID, NS_ISCROLLABLEVIEW_IID);

nsEventStateManager::nsEventStateManager()
  : mGestureDownPoint(0,0)
{
  mLastMouseOverFrame = nsnull;
  mLastDragOverFrame = nsnull;
  mCurrentTarget = nsnull;
  mCurrentTargetContent = nsnull;
  mLastLeftMouseDownContent = nsnull;
  mLastMiddleMouseDownContent = nsnull;
  mLastRightMouseDownContent = nsnull;

  // init d&d gesture state machine variables
  mIsTrackingDragGesture = PR_FALSE;
  mGestureDownFrame = nsnull;

  mActiveContent = nsnull;
  mHoverContent = nsnull;
  mDragOverContent = nsnull;
  mCurrentFocus = nsnull;
  mDocument = nsnull;
  mPresContext = nsnull;
  mCurrentTabIndex = 0;
  NS_INIT_REFCNT();
}

nsEventStateManager::~nsEventStateManager() {
  NS_IF_RELEASE(mCurrentTargetContent);
  NS_IF_RELEASE(mActiveContent);
  NS_IF_RELEASE(mHoverContent);
  NS_IF_RELEASE(mDragOverContent);
  NS_IF_RELEASE(mCurrentFocus);
  NS_IF_RELEASE(mDocument);
  NS_IF_RELEASE(mLastLeftMouseDownContent);
  NS_IF_RELEASE(mLastMiddleMouseDownContent);
  NS_IF_RELEASE(mLastRightMouseDownContent);
}

NS_IMPL_ADDREF(nsEventStateManager)
NS_IMPL_RELEASE(nsEventStateManager)
NS_IMPL_QUERY_INTERFACE(nsEventStateManager, kIEventStateManagerIID);

NS_IMETHODIMP
nsEventStateManager::PreHandleEvent(nsIPresContext& aPresContext, 
                                 nsGUIEvent *aEvent,
                                 nsIFrame* aTargetFrame,
                                 nsEventStatus& aStatus,
                                 nsIView* aView)
{
  mCurrentTarget = aTargetFrame;
  NS_IF_RELEASE(mCurrentTargetContent);

  nsFrameState state;
  mCurrentTarget->GetFrameState(&state);
  state |= NS_FRAME_EXTERNAL_REFERENCE;
  mCurrentTarget->SetFrameState(state);

  aStatus = nsEventStatus_eIgnore;
  
  switch (aEvent->message) {
  case NS_MOUSE_LEFT_BUTTON_DOWN:
    BeginTrackingDragGesture ( aEvent, aTargetFrame );
    break;
  case NS_MOUSE_LEFT_BUTTON_UP:
    StopTrackingDragGesture();
    break;
  case NS_MOUSE_MOVE:
    GenerateDragGesture(aPresContext, aEvent);
    UpdateCursor(aPresContext, aEvent->point, aTargetFrame, aStatus);
    GenerateMouseEnterExit(aPresContext, aEvent);
    break;
  case NS_MOUSE_EXIT:
    GenerateMouseEnterExit(aPresContext, aEvent);
    break;
  case NS_DRAGDROP_OVER:
    GenerateDragDropEnterExit(aPresContext, aEvent);
    break;
  case NS_DRAGDROP_DROP:
  case NS_DRAGDROP_EXIT:
    GenerateDragDropEnterExit(aPresContext, aEvent);
    break;
  case NS_GOTFOCUS:
    {
      nsIContent* newFocus;
      mCurrentTarget->GetContent(&newFocus);
      if (newFocus) {
        nsIFocusableContent *focusChange;
        if (NS_SUCCEEDED(newFocus->QueryInterface(kIFocusableContentIID,
                                                  (void **)&focusChange))) {
          NS_RELEASE(focusChange);
          NS_RELEASE(newFocus);
          break;
        }
        NS_RELEASE(newFocus);
      }

      //fire focus
      nsEventStatus status = nsEventStatus_eIgnore;
      nsEvent event;
      event.eventStructType = NS_EVENT;
      event.message = NS_FOCUS_CONTENT;

      if (!mDocument) {
        nsCOMPtr<nsIPresShell> presShell;
        aPresContext.GetShell(getter_AddRefs(presShell));
        if (presShell) {
          presShell->GetDocument(&mDocument);
        }
      }

      if (mDocument) {
        mCurrentTarget = nsnull;
        mDocument->HandleDOMEvent(aPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, status); 
      }
    }
    break;
  case NS_LOSTFOCUS:
    {
      nsIContent* oldFocus;
      mCurrentTarget->GetContent(&oldFocus);
      if (oldFocus) {
        nsIFocusableContent *focusChange;
        if (NS_SUCCEEDED(oldFocus->QueryInterface(kIFocusableContentIID,
                                                  (void **)&focusChange))) {
          NS_RELEASE(focusChange);
          NS_RELEASE(oldFocus);
          break;
        }
        NS_RELEASE(oldFocus);
      }

      //fire blur
      nsEventStatus status = nsEventStatus_eIgnore;
      nsEvent event;
      event.eventStructType = NS_EVENT;
      event.message = NS_BLUR_CONTENT;

      if (!mDocument) {
        nsCOMPtr<nsIPresShell> presShell;
        aPresContext.GetShell(getter_AddRefs(presShell));
        if (presShell) {
          presShell->GetDocument(&mDocument);
        }
      }

      if (mDocument) {
        mCurrentTarget = nsnull;
        mDocument->HandleDOMEvent(aPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, status); 
      }
    }
    break;
  case NS_KEY_PRESS:
  case NS_KEY_DOWN:
  case NS_KEY_UP:
    {
      if (mCurrentFocus) {
        mCurrentTargetContent = mCurrentFocus;
        NS_ADDREF(mCurrentTargetContent);
      }

      if (aEvent->message == NS_KEY_PRESS) {
        nsKeyEvent * keyEvent = (nsKeyEvent *)aEvent;
        if (keyEvent->isControl) {
          keyEvent->charCode += 64;
        }
      }
    }
    break;
  }
  return NS_OK;
}


// 
// BeginTrackingDragGesture
//
// Record that the mouse has gone down and that we should move to TRACKING state
// of d&d gesture tracker.
//
void
nsEventStateManager :: BeginTrackingDragGesture ( nsGUIEvent* inDownEvent, nsIFrame* inDownFrame )
{
  mIsTrackingDragGesture = PR_TRUE;
  mGestureDownPoint = inDownEvent->point;
  mGestureDownFrame = inDownFrame;
}


//
// StopTrackingDragGesture
//
// Record that the mouse has gone back up so that we should leave the TRACKING 
// state of d&d gesture tracker and return to the START state.
//
void
nsEventStateManager :: StopTrackingDragGesture ( )
{
  mIsTrackingDragGesture = PR_FALSE;
  mGestureDownPoint = nsPoint(0,0);
  mGestureDownFrame = nsnull;
}


//
// GenerateDragGesture
//
// If we're in the TRACKING state of the d&d gesture tracker, check the current position
// of the mouse in relation to the old one. If we've moved a sufficient amount from
// the mouse down, then fire off a drag gesture event.
//
// Note that when the mouse enters a new child window with its own view, the event's
// coordinates will be in relation to the origin of the inner child window, which could
// either be very different from that of the mouse coords of the mouse down and trigger
// a drag too early, or very similiar which might not trigger a drag. 
//
// Do we need to do anything about this? Let's wait and see.
//
void
nsEventStateManager :: GenerateDragGesture ( nsIPresContext& aPresContext, nsGUIEvent *aEvent )
{
  if ( IsTrackingDragGesture() ) {
    // figure out the delta in twips, since that is how it is in the event.
    // Do we need to do this conversion every time? Will the pres context really change on
    // us or can we cache it?
    long twipDeltaToStartDrag = 0;
    const long pixelDeltaToStartDrag = 5;
    nsCOMPtr<nsIDeviceContext> devContext;
    aPresContext.GetDeviceContext ( getter_AddRefs(devContext) );
    if ( devContext ) {
      float pixelsToTwips = 0.0;
      devContext->GetDevUnitsToTwips(pixelsToTwips);
      twipDeltaToStartDrag =  pixelDeltaToStartDrag * pixelsToTwips;
    }
 
    // fire drag gesture if mouse has moved enough
    if ( abs(aEvent->point.x - mGestureDownPoint.x) > twipDeltaToStartDrag ||
          abs(aEvent->point.y - mGestureDownPoint.y) > twipDeltaToStartDrag ) {
      nsEventStatus status = nsEventStatus_eIgnore;
      nsMouseEvent event;
      event.eventStructType = NS_DRAGDROP_EVENT;
      event.message = NS_DRAGDROP_GESTURE;
      event.widget = aEvent->widget;
      event.clickCount = 0;
      event.point = aEvent->point;
      event.refPoint = aEvent->refPoint;

      // dispatch to the DOM
      nsCOMPtr<nsIContent> lastContent;
      mGestureDownFrame->GetContent(getter_AddRefs(lastContent));
      if ( lastContent )
        lastContent->HandleDOMEvent(aPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, status); 
 
      // dispatch to the frame
      mGestureDownFrame->HandleEvent(aPresContext, &event, status);   

      StopTrackingDragGesture();              
    }
  }
} // GenerateDragGesture


NS_IMETHODIMP
nsEventStateManager::PostHandleEvent(nsIPresContext& aPresContext, 
                                 nsGUIEvent *aEvent,
                                 nsIFrame* aTargetFrame,
                                 nsEventStatus& aStatus,
                                 nsIView* aView)
{
  mCurrentTarget = aTargetFrame;
  NS_IF_RELEASE(mCurrentTargetContent);  
  nsresult ret = NS_OK;

  nsFrameState state;
  mCurrentTarget->GetFrameState(&state);
  state |= NS_FRAME_EXTERNAL_REFERENCE;
  mCurrentTarget->SetFrameState(state);

  switch (aEvent->message) {
  case NS_MOUSE_LEFT_BUTTON_DOWN:
  case NS_MOUSE_MIDDLE_BUTTON_DOWN:
  case NS_MOUSE_RIGHT_BUTTON_DOWN: 
    {
      ret = CheckForAndDispatchClick(aPresContext, (nsMouseEvent*)aEvent, aStatus);

      if (nsEventStatus_eConsumeNoDefault != aStatus) {
        nsIContent* newFocus;
        mCurrentTarget->GetContent(&newFocus);
        if (newFocus) {
          if (!ChangeFocus(newFocus, PR_TRUE)) {
            if (nsnull != aEvent->widget) {
              aEvent->widget->SetFocus();
            }
            SetContentState(nsnull, NS_EVENT_STATE_FOCUS);
          }
        }

        SetContentState(newFocus, NS_EVENT_STATE_ACTIVE);

        NS_IF_RELEASE(newFocus);
      }

    }
    break;
  case NS_MOUSE_LEFT_BUTTON_UP:
  case NS_MOUSE_MIDDLE_BUTTON_UP:
  case NS_MOUSE_RIGHT_BUTTON_UP:
    {
      ret = CheckForAndDispatchClick(aPresContext, (nsMouseEvent*)aEvent, aStatus);
      SetContentState(nsnull, NS_EVENT_STATE_ACTIVE);
      nsCOMPtr<nsIPresShell> shell;
      nsresult rv = aPresContext.GetShell(getter_AddRefs(shell));
      if (NS_SUCCEEDED(rv) && shell){
        nsCOMPtr<nsIFrameSelection> frameSel;
        rv = shell->GetFrameSelection(getter_AddRefs(frameSel));
        if (NS_SUCCEEDED(rv) && frameSel){
            frameSel->SetMouseDownState(PR_FALSE);
        }
      }
    }
    break;
  case NS_KEY_DOWN:
    if (nsEventStatus_eConsumeNoDefault != aStatus) {
      switch(((nsKeyEvent*)aEvent)->keyCode) {
        case NS_VK_TAB:
          //Shift focus forward or back depending on shift key
          ShiftFocus(!((nsInputEvent*)aEvent)->isShift);
          aStatus = nsEventStatus_eConsumeNoDefault;
          break;
        case NS_VK_PAGE_DOWN: 
        case NS_VK_PAGE_UP:
          if (!mCurrentFocus) {
            nsIScrollableView* sv = GetNearestScrollingView(aView);
            if (sv) {
              nsKeyEvent * keyEvent = (nsKeyEvent *)aEvent;
              sv->ScrollByPages((keyEvent->keyCode != NS_VK_PAGE_UP) ? 1 : -1);
            }
          }
          break;
        case NS_VK_DOWN: 
        case NS_VK_UP:
          if (!mCurrentFocus) {
            nsIScrollableView* sv = GetNearestScrollingView(aView);
            if (sv) {
              nsKeyEvent * keyEvent = (nsKeyEvent *)aEvent;
              sv->ScrollByLines((keyEvent->keyCode == NS_VK_DOWN) ? 1 : -1);
              
              // force the update to happen now, otherwise multiple scrolls can
              // occur before the update is processed. (bug #7354)
              nsIViewManager* vm = nsnull;
             	if (NS_OK == aView->GetViewManager(vm) && nsnull != vm) {
             	  // I'd use Composite here, but it doesn't always work.
                // vm->Composite();
                nsIView* rootView = nsnull;
                if (NS_OK == vm->GetRootView(rootView) && nsnull != rootView) {
              	  nsIWidget* rootWidget = nsnull;
              		if (NS_OK == rootView->GetWidget(rootWidget) && nsnull != rootWidget) {
                    rootWidget->Update();
                    NS_RELEASE(rootWidget);
                  }
              	}
                NS_RELEASE(vm);
              }
            }
          }
          break;
      }
    }
    break;
  case NS_KEY_PRESS:
    if (nsEventStatus_eConsumeNoDefault != aStatus) {
      //Handle key commands from keys with char representation here, not on KeyDown
      nsKeyEvent * keyEvent = (nsKeyEvent *)aEvent;

      //Spacebar
      if (keyEvent->charCode == 0x20) {
        if (!mCurrentFocus) {
          nsIScrollableView* sv = GetNearestScrollingView(aView);
          if (sv) {
            sv->ScrollByPages(1);
          }
        }
      }
    }
  }
  return ret;
}

NS_IMETHODIMP
nsEventStateManager::SetPresContext(nsIPresContext* aPresContext)
{
  mPresContext = aPresContext;
  return NS_OK;
}

NS_IMETHODIMP
nsEventStateManager::ClearFrameRefs(nsIFrame* aFrame)
{
  if (aFrame == mLastMouseOverFrame) {
    mLastMouseOverFrame = nsnull;
  }
  if (aFrame == mCurrentTarget) {
    if (aFrame) {
      aFrame->GetContent(&mCurrentTargetContent);
    }
    mCurrentTarget = nsnull;
  }
  return NS_OK;
}

nsIScrollableView*
nsEventStateManager::GetNearestScrollingView(nsIView* aView)
{
  nsIScrollableView* sv;
  if (NS_OK == aView->QueryInterface(kIScrollableViewIID, (void**)&sv)) {
    return sv;
  }

  nsIView* parent;
  aView->GetParent(parent);

  if (nsnull != parent) {
    return GetNearestScrollingView(parent);
  }

  return nsnull;
}

void
nsEventStateManager::UpdateCursor(nsIPresContext& aPresContext, nsPoint& aPoint, nsIFrame* aTargetFrame, 
                                  nsEventStatus& aStatus)
{
  PRInt32 cursor;
  nsCursor c;

  aTargetFrame->GetCursor(aPresContext, aPoint, cursor);

  switch (cursor) {
  default:
  case NS_STYLE_CURSOR_AUTO:
  case NS_STYLE_CURSOR_DEFAULT:
    c = eCursor_standard;
    break;
  case NS_STYLE_CURSOR_POINTER:
    c = eCursor_hyperlink;
    break;
  case NS_STYLE_CURSOR_CROSSHAIR:
    c = eCursor_crosshair;
    break;
  case NS_STYLE_CURSOR_MOVE:
    c = eCursor_move;
    break;
  case NS_STYLE_CURSOR_TEXT:
    c = eCursor_select;
    break;
  case NS_STYLE_CURSOR_WAIT:
    c = eCursor_wait;
    break;
  case NS_STYLE_CURSOR_HELP:
    c = eCursor_help;
    break;
  case NS_STYLE_CURSOR_N_RESIZE:
  case NS_STYLE_CURSOR_S_RESIZE:
    c = eCursor_sizeNS;
    break;
  case NS_STYLE_CURSOR_W_RESIZE:
  case NS_STYLE_CURSOR_E_RESIZE:
    c = eCursor_sizeWE;
    break;
  //These aren't in the CSS2 spec. Don't know what to do with them.
  case NS_STYLE_CURSOR_NE_RESIZE:
  case NS_STYLE_CURSOR_NW_RESIZE:
  case NS_STYLE_CURSOR_SE_RESIZE:
  case NS_STYLE_CURSOR_SW_RESIZE:
    c = eCursor_select;
    break;
  }

  if (NS_STYLE_CURSOR_AUTO != cursor) {
    aStatus = nsEventStatus_eConsumeDoDefault;
  }

  nsIWidget* window;
  aTargetFrame->GetWindow(&window);
  window->SetCursor(c);
  NS_RELEASE(window);
}

void
nsEventStateManager::GenerateMouseEnterExit(nsIPresContext& aPresContext, nsGUIEvent* aEvent)
{
  switch(aEvent->message) {
  case NS_MOUSE_MOVE:
    {
      if (mLastMouseOverFrame != mCurrentTarget) {
        //We'll need the content, too, to check if it changed separately from the frames.
        nsIContent *lastContent = nsnull;
        nsIContent *targetContent;

        mCurrentTarget->GetContent(&targetContent);

        if (nsnull != mLastMouseOverFrame) {
          //fire mouseout
          nsEventStatus status = nsEventStatus_eIgnore;
          nsMouseEvent event;
          event.eventStructType = NS_MOUSE_EVENT;
          event.message = NS_MOUSE_EXIT;
          event.widget = aEvent->widget;
          event.clickCount = 0;
          event.point = aEvent->point;
          event.refPoint = aEvent->refPoint;

          //The frame has change but the content may not have.  Check before dispatching to content
          mLastMouseOverFrame->GetContent(&lastContent);

          if (lastContent != targetContent) {
            //XXX This event should still go somewhere!!
            if (nsnull != lastContent) {
              lastContent->HandleDOMEvent(aPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, status); 
            }
          }

          //Now dispatch to the frame
          if (nsnull != mLastMouseOverFrame) {
            //XXX Get the new frame
            mLastMouseOverFrame->HandleEvent(aPresContext, &event, status);   
          }
        }

        //fire mouseover
        nsEventStatus status = nsEventStatus_eIgnore;
        nsMouseEvent event;
        event.eventStructType = NS_MOUSE_EVENT;
        event.message = NS_MOUSE_ENTER;
        event.widget = aEvent->widget;
        event.clickCount = 0;
        event.point = aEvent->point;
        event.refPoint = aEvent->refPoint;

        //The frame has change but the content may not have.  Check before dispatching to content
        if (lastContent != targetContent) {
          //XXX This event should still go somewhere!!
          if (nsnull != targetContent) {
            targetContent->HandleDOMEvent(aPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, status); 
          }

          if (nsEventStatus_eConsumeNoDefault != status) {
            SetContentState(targetContent, NS_EVENT_STATE_HOVER);
          }
        }

        //Now dispatch to the frame
        if (nsnull != mCurrentTarget) {
          //XXX Get the new frame
          mCurrentTarget->HandleEvent(aPresContext, &event, status);
        }

        NS_IF_RELEASE(lastContent);
        NS_IF_RELEASE(targetContent);

        mLastMouseOverFrame = mCurrentTarget;
      }
    }
    break;
  case NS_MOUSE_EXIT:
    {
      //This is actually the window mouse exit event.  Such a think does not
      // yet exist but it will need to eventually.
      if (nsnull != mLastMouseOverFrame) {
        //fire mouseout
        nsEventStatus status = nsEventStatus_eIgnore;
        nsMouseEvent event;
        event.eventStructType = NS_MOUSE_EVENT;
        event.message = NS_MOUSE_EXIT;
        event.widget = aEvent->widget;
        event.clickCount = 0;
        event.point = aEvent->point;
        event.refPoint = aEvent->refPoint;

        nsIContent *lastContent;
        mLastMouseOverFrame->GetContent(&lastContent);

        if (nsnull != lastContent) {
          lastContent->HandleDOMEvent(aPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, status); 

          if (nsEventStatus_eConsumeNoDefault != status) {
            SetContentState(nsnull, NS_EVENT_STATE_HOVER);
          }
          NS_RELEASE(lastContent);
        }


        //Now dispatch to the frame
        if (nsnull != mLastMouseOverFrame) {
          //XXX Get the new frame
          mLastMouseOverFrame->HandleEvent(aPresContext, &event, status);   
          mLastMouseOverFrame = nsnull;
        }
      }
    }
    break;
  }
  
}

void
nsEventStateManager::GenerateDragDropEnterExit(nsIPresContext& aPresContext, nsGUIEvent* aEvent)
{
  switch(aEvent->message) {
  case NS_DRAGDROP_OVER:
    {
      if (mLastDragOverFrame != mCurrentTarget) {
        //We'll need the content, too, to check if it changed separately from the frames.
        nsIContent *lastContent = nsnull;
        nsIContent *targetContent;

        mCurrentTarget->GetContent(&targetContent);

        if (nsnull != mLastDragOverFrame) {
          //fire mouseout
          nsEventStatus status = nsEventStatus_eIgnore;
          nsMouseEvent event;
          event.eventStructType = NS_DRAGDROP_EVENT;
          event.message = NS_DRAGDROP_EXIT;
          event.widget = aEvent->widget;
          event.clickCount = 0;
          event.point = aEvent->point;
          event.refPoint = aEvent->refPoint;

          //The frame has change but the content may not have.  Check before dispatching to content
          mLastDragOverFrame->GetContent(&lastContent);

          if (lastContent != targetContent) {
            //XXX This event should still go somewhere!!
            if (nsnull != lastContent) {
              lastContent->HandleDOMEvent(aPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, status); 
            }
          }

          if (nsEventStatus_eConsumeNoDefault != status) {
            SetContentState(nsnull, NS_EVENT_STATE_DRAGOVER);
          }
          //Now dispatch to the frame
          if (nsnull != mLastDragOverFrame) {
            //XXX Get the new frame
            mLastDragOverFrame->HandleEvent(aPresContext, &event, status);   
          }
        }

        //fire dragover
        nsEventStatus status = nsEventStatus_eIgnore;
        nsMouseEvent event;
        event.eventStructType = NS_DRAGDROP_EVENT;
        event.message = NS_DRAGDROP_ENTER;
        event.widget = aEvent->widget;
        event.clickCount = 0;
        event.point = aEvent->point;
        event.refPoint = aEvent->refPoint;

        //The frame has change but the content may not have.  Check before dispatching to content
        if (lastContent != targetContent) {
          //XXX This event should still go somewhere!!
          if (nsnull != targetContent) {
            targetContent->HandleDOMEvent(aPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, status); 
          }

          if (nsEventStatus_eConsumeNoDefault != status) {
            SetContentState(targetContent, NS_EVENT_STATE_DRAGOVER);
          }
        }

       //Now dispatch to the frame
        if (nsnull != mCurrentTarget) {
          //XXX Get the new frame
          mCurrentTarget->HandleEvent(aPresContext, &event, status);
        }

        NS_IF_RELEASE(lastContent);
        NS_IF_RELEASE(targetContent);

        mLastDragOverFrame = mCurrentTarget;
      }
    }
    break;
  case NS_DRAGDROP_DROP:
  case NS_DRAGDROP_EXIT:
    {
      //This is actually the window mouse exit event.  Such a think does not
      // yet exist but it will need to eventually.
      if (nsnull != mLastDragOverFrame) {
        //fire mouseout
        nsEventStatus status = nsEventStatus_eIgnore;
        nsMouseEvent event;
        event.eventStructType = NS_DRAGDROP_EVENT;
        event.message = NS_DRAGDROP_EXIT;
        event.widget = aEvent->widget;
        event.clickCount = 0;
        event.point = aEvent->point;
        event.refPoint = aEvent->refPoint;

        nsIContent *lastContent;
        mLastDragOverFrame->GetContent(&lastContent);

        if (nsnull != lastContent) {
          lastContent->HandleDOMEvent(aPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, status); 
          if (nsEventStatus_eConsumeNoDefault != status) {
            SetContentState(nsnull, NS_EVENT_STATE_DRAGOVER);
          }
          NS_RELEASE(lastContent);
        }

        //Now dispatch to the frame
        if (nsnull != mLastDragOverFrame) {
          //XXX Get the new frame
          mLastDragOverFrame->HandleEvent(aPresContext, &event, status);   
          mLastDragOverFrame = nsnull;
        }
      }
    }
    break;
  }
  
}

NS_IMETHODIMP
nsEventStateManager::CheckForAndDispatchClick(nsIPresContext& aPresContext, 
                                              nsMouseEvent *aEvent,
                                              nsEventStatus& aStatus)
{
  nsresult ret = NS_OK;
  nsMouseEvent event;
  nsIContent* mouseContent;
  PRBool fireClick = PR_FALSE;

  mCurrentTarget->GetContent(&mouseContent);

  switch (aEvent->message) {
  case NS_MOUSE_LEFT_BUTTON_DOWN:
    mLastLeftMouseDownContent = mouseContent;
    NS_IF_ADDREF(mLastLeftMouseDownContent);
    break;
  case NS_MOUSE_LEFT_BUTTON_UP:
    if (mLastLeftMouseDownContent == mouseContent) {
      fireClick = PR_TRUE;
      event.message = NS_MOUSE_LEFT_CLICK;
    }
    NS_IF_RELEASE(mLastLeftMouseDownContent);
    break;
  case NS_MOUSE_MIDDLE_BUTTON_DOWN:
    mLastMiddleMouseDownContent = mouseContent;
    NS_IF_ADDREF(mLastMiddleMouseDownContent);
    break;
  case NS_MOUSE_MIDDLE_BUTTON_UP:
    if (mLastMiddleMouseDownContent == mouseContent) {
      fireClick = PR_TRUE;
      event.message = NS_MOUSE_MIDDLE_CLICK;
    }
    NS_IF_RELEASE(mLastMiddleMouseDownContent);
    break;
  case NS_MOUSE_RIGHT_BUTTON_DOWN:
    mLastRightMouseDownContent = mouseContent;
    NS_IF_ADDREF(mLastRightMouseDownContent);
    break;
  case NS_MOUSE_RIGHT_BUTTON_UP:
    if (mLastRightMouseDownContent == mouseContent) {
      fireClick = PR_TRUE;
      event.message = NS_MOUSE_RIGHT_CLICK;
    }
    NS_IF_RELEASE(mLastRightMouseDownContent);
    break;
  }

  if (fireClick) {
    //fire click
    event.eventStructType = NS_MOUSE_EVENT;
    event.widget = aEvent->widget;
    event.point.x = aEvent->point.x;
    event.point.y = aEvent->point.y;
    event.refPoint.x = aEvent->refPoint.x;
    event.refPoint.y = aEvent->refPoint.y;

    if (nsnull != mouseContent) {
      ret = mouseContent->HandleDOMEvent(aPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, aStatus); 
      NS_RELEASE(mouseContent);
    }

    if (nsnull != mCurrentTarget) {
      ret = mCurrentTarget->HandleEvent(aPresContext, &event, aStatus);
    }
  }
  return ret;
}

NS_IMETHODIMP
nsEventStateManager::DispatchKeyPressEvent(nsIPresContext& aPresContext, 
                                           nsKeyEvent *aEvent,
                                           nsEventStatus& aStatus)
{
  nsresult ret = NS_OK;

  //fire keypress
  nsKeyEvent event;
  event.eventStructType = NS_KEY_EVENT;
  event.message = NS_KEY_PRESS;
  event.widget = nsnull;
  event.keyCode = aEvent->keyCode;

  nsIContent *content;
  mCurrentTarget->GetContent(&content);

  if (nsnull != content) {
    ret = content->HandleDOMEvent(aPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, aStatus); 
    NS_RELEASE(content);
  }

  //Now dispatch to the frame
  if (nsnull != mCurrentTarget) {
    mCurrentTarget->HandleEvent(aPresContext, &event, aStatus);   
  }

  return ret;
}

PRBool
nsEventStateManager::ChangeFocus(nsIContent* aFocus, PRBool aSetFocus)
{
  nsIFocusableContent *focusChange;

  if (NS_OK == aFocus->QueryInterface(kIFocusableContentIID,(void **)&focusChange)) {
    if (aSetFocus) {
      focusChange->SetFocus(mPresContext);
    }
    else {
      focusChange->RemoveFocus(mPresContext);
    }
    NS_RELEASE(focusChange);
    return PR_TRUE;
  }
  //XXX Need to deal with Object tag

  return PR_FALSE;
} 

void
nsEventStateManager::ShiftFocus(PRBool forward)
{
  PRBool topOfDoc = PR_FALSE;

  if (nsnull == mPresContext) {
    return;
  }

  if (nsnull == mCurrentFocus) {
    if (nsnull == mDocument) {
      nsCOMPtr<nsIPresShell> presShell;
      mPresContext->GetShell(getter_AddRefs(presShell));
      if (presShell) {
        presShell->GetDocument(&mDocument);
        if (nsnull == mDocument) {
          return;
        }
      }
    }
    mCurrentFocus = mDocument->GetRootContent();
    if (nsnull == mCurrentFocus) {  
      return;
    }
    mCurrentTabIndex = forward ? 1 : 0;
    topOfDoc = PR_TRUE;
  }

  nsIContent* next = GetNextTabbableContent(mCurrentFocus, nsnull, mCurrentFocus, forward);

  if (nsnull == next) {
    PRBool focusTaken = PR_FALSE;

    SetContentState(nsnull, NS_EVENT_STATE_FOCUS);

    //Pass focus up to nsIWebShellContainer FocusAvailable
    nsISupports* container;
    mPresContext->GetContainer(&container);
    if (nsnull != container) {
      nsIWebShell* webShell;
      if (NS_OK == container->QueryInterface(kIWebShellIID, (void**)&webShell)) {
        nsIWebShellContainer* webShellContainer;
        webShell->GetContainer(webShellContainer);
        if (nsnull != webShellContainer) {
          webShellContainer->FocusAvailable(webShell, focusTaken);
          NS_RELEASE(webShellContainer);
        }
        NS_RELEASE(webShell);
      }
      NS_RELEASE(container);
    }
    if (!focusTaken && !topOfDoc) {
      ShiftFocus(forward);
    }
    return;
  }

  ChangeFocus(next, PR_TRUE);

  NS_IF_RELEASE(mCurrentFocus);
  mCurrentFocus = next;
}

/*
 * At some point this will need to be linked into HTML 4.0 tabindex
 */
nsIContent*
nsEventStateManager::GetNextTabbableContent(nsIContent* aParent, nsIContent* aChild, nsIContent* aTop, PRBool forward)
{
  PRInt32 count, index;
  aParent->ChildCount(count);

  if (nsnull != aChild) {
    aParent->IndexOf(aChild, index);
    index += forward ? 1 : -1;
  }
  else {
    index = forward ? 0 : count-1;
  }

  for (;index < count && index >= 0;index += forward ? 1 : -1) {
    nsIContent* child;

    aParent->ChildAt(index, child);
    nsIContent* content = GetNextTabbableContent(child, nsnull, aTop, forward);
    if (content != nsnull) {
      NS_IF_RELEASE(child);
      return content;
    }
    if (nsnull != child) {
      nsIAtom* tag;
      PRInt32 tabIndex = -1;
      PRBool disabled = PR_TRUE;
      PRBool hidden = PR_FALSE;

      child->GetTag(tag);
      if (nsHTMLAtoms::input==tag) {
        nsIDOMHTMLInputElement *nextInput;
        if (NS_OK == child->QueryInterface(kIDOMHTMLInputElementIID,(void **)&nextInput)) {
          nextInput->GetDisabled(&disabled);
          nextInput->GetTabIndex(&tabIndex);

          nsAutoString type;
          nextInput->GetType(type);
          if (type.EqualsIgnoreCase("hidden")) {
            hidden = PR_TRUE;
          }
          NS_RELEASE(nextInput);
        }
      }
      else if (nsHTMLAtoms::select==tag) {
        nsIDOMHTMLSelectElement *nextSelect;
        if (NS_OK == child->QueryInterface(kIDOMHTMLSelectElementIID,(void **)&nextSelect)) {
          nextSelect->GetDisabled(&disabled);
          nextSelect->GetTabIndex(&tabIndex);
          NS_RELEASE(nextSelect);
        }
      }
      else if (nsHTMLAtoms::textarea==tag) {
        nsIDOMHTMLTextAreaElement *nextTextArea;
        if (NS_OK == child->QueryInterface(kIDOMHTMLTextAreaElementIID,(void **)&nextTextArea)) {
          nextTextArea->GetDisabled(&disabled);
          nextTextArea->GetTabIndex(&tabIndex);
          NS_RELEASE(nextTextArea);
        }

      }
      else if(nsHTMLAtoms::a==tag) {
        nsIDOMHTMLAnchorElement *nextAnchor;
        if (NS_OK == child->QueryInterface(kIDOMHTMLAnchorElementIID,(void **)&nextAnchor)) {
          nextAnchor->GetTabIndex(&tabIndex);
          NS_RELEASE(nextAnchor);
        }
        disabled = PR_FALSE;
      }
      else if(nsHTMLAtoms::button==tag) {
        nsIDOMHTMLButtonElement *nextButton;
        if (NS_OK == child->QueryInterface(kIDOMHTMLButtonElementIID,(void **)&nextButton)) {
          nextButton->GetTabIndex(&tabIndex);
          nextButton->GetDisabled(&disabled);
          NS_RELEASE(nextButton);
        }
      }
      else if(nsHTMLAtoms::area==tag) {
        nsIDOMHTMLAreaElement *nextArea;
        if (NS_OK == child->QueryInterface(kIDOMHTMLAreaElementIID,(void **)&nextArea)) {
          nextArea->GetTabIndex(&tabIndex);
          NS_RELEASE(nextArea);
        }
        disabled = PR_FALSE;
      }
      else if(nsHTMLAtoms::object==tag) {
        nsIDOMHTMLObjectElement *nextObject;
        if (NS_OK == child->QueryInterface(kIDOMHTMLObjectElementIID,(void **)&nextObject)) {
          nextObject->GetTabIndex(&tabIndex);
          NS_RELEASE(nextObject);
        }
        disabled = PR_FALSE;
      }

      //TabIndex not set (-1) treated at same level as set to 0
      tabIndex = tabIndex < 0 ? 0 : tabIndex;

      if (!disabled && !hidden && mCurrentTabIndex == tabIndex) {
        return child;
      }
      NS_RELEASE(child);
    }
  }

  if (aParent == aTop) {
    nsIContent* nextParent;
    aParent->GetParent(nextParent);
    if (nsnull != nextParent) {
      nsIContent* content = GetNextTabbableContent(nextParent, aParent, nextParent, forward);
      NS_RELEASE(nextParent);
      return content;
    }
    //Reached end of document
    else {
      //If already at lowest priority tab (0), end
      if (((forward) && (0 == mCurrentTabIndex)) ||
          ((!forward) && (1 == mCurrentTabIndex))) {
        return nsnull;
      }
      //else continue looking for next highest priority tab
      mCurrentTabIndex = GetNextTabIndex(aParent, forward);
      nsIContent* content = GetNextTabbableContent(aParent, nsnull, aParent, forward);
      return content;
    }
  }

  return nsnull;
}

PRInt32
nsEventStateManager::GetNextTabIndex(nsIContent* aParent, PRBool forward)
{
  PRInt32 count, tabIndex, childTabIndex;
  nsIContent* child;
  
  aParent->ChildCount(count);
 
  if (forward) {
    tabIndex = 0;
    for (PRInt32 index = 0; index < count; index++) {
      aParent->ChildAt(index, child);
      childTabIndex = GetNextTabIndex(child, forward);
      if (childTabIndex > mCurrentTabIndex && childTabIndex != tabIndex) {
        tabIndex = (tabIndex == 0 || childTabIndex < tabIndex) ? childTabIndex : tabIndex; 
      }
      
      nsAutoString tabIndexStr;
      child->GetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::tabindex, tabIndexStr);
      PRInt32 ec, val = tabIndexStr.ToInteger(&ec);
      if (NS_OK == ec && val > mCurrentTabIndex && val != tabIndex) {
        tabIndex = (tabIndex == 0 || val < tabIndex) ? val : tabIndex; 
      }
      NS_RELEASE(child);
    }
  } 
  else { /* !forward */
    tabIndex = 1;
    for (PRInt32 index = 0; index < count; index++) {
      aParent->ChildAt(index, child);
      childTabIndex = GetNextTabIndex(child, forward);
      if ((mCurrentTabIndex==0 && childTabIndex > tabIndex) ||
          (childTabIndex < mCurrentTabIndex && childTabIndex > tabIndex)) {
        tabIndex = childTabIndex;
      }
      
      nsAutoString tabIndexStr;
      child->GetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::tabindex, tabIndexStr);
      PRInt32 ec, val = tabIndexStr.ToInteger(&ec);
      if (NS_OK == ec) {
        if ((mCurrentTabIndex==0 && val > tabIndex) ||
            (val < mCurrentTabIndex && val > tabIndex) ) {
          tabIndex = val;
        }
      }
      NS_RELEASE(child);
    }
  }
  return tabIndex;
}


NS_IMETHODIMP
nsEventStateManager::GetEventTarget(nsIFrame **aFrame)
{
  if (!mCurrentTarget && mCurrentTargetContent) {
    nsCOMPtr<nsIPresShell> shell;
    if (mPresContext) {
      nsresult rv = mPresContext->GetShell(getter_AddRefs(shell));
      if (NS_SUCCEEDED(rv) && shell){
        shell->GetPrimaryFrameFor(mCurrentTargetContent, &mCurrentTarget);
      }
    }
  }

  *aFrame = mCurrentTarget;
  return NS_OK;
}

NS_IMETHODIMP
nsEventStateManager::GetEventTargetContent(nsIContent** aContent)
{
  if (mCurrentTargetContent) {
    *aContent = mCurrentTargetContent;
    NS_IF_ADDREF(*aContent);
    return NS_OK;      
  }
  
  if (mCurrentTarget) {
    mCurrentTarget->GetContent(aContent);
    return NS_OK;
  }

  *aContent = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsEventStateManager::GetContentState(nsIContent *aContent, PRInt32& aState)
{
  aState = NS_EVENT_STATE_UNSPECIFIED;

  if (aContent == mActiveContent) {
    aState |= NS_EVENT_STATE_ACTIVE;
  }
  if (aContent == mHoverContent) {
    aState |= NS_EVENT_STATE_HOVER;
  }
  if (aContent == mCurrentFocus) {
    aState |= NS_EVENT_STATE_FOCUS;
  }
  if (aContent == mDragOverContent) {
    aState |= NS_EVENT_STATE_DRAGOVER;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsEventStateManager::SetContentState(nsIContent *aContent, PRInt32 aState)
{
  const PRInt32 maxNotify = 5;
  nsIContent  *notifyContent[maxNotify] = {nsnull, nsnull, nsnull, nsnull, nsnull};

  if ((aState & NS_EVENT_STATE_DRAGOVER) && (aContent != mDragOverContent)) {
    //transferring ref to notifyContent from mDragOverContent
    notifyContent[4] = mDragOverContent; // notify dragover first, since more common case
    mDragOverContent = aContent;
    NS_IF_ADDREF(mDragOverContent);
  }

  if ((aState & NS_EVENT_STATE_ACTIVE) && (aContent != mActiveContent)) {
    //transferring ref to notifyContent from mActiveContent
    notifyContent[2] = mActiveContent;
    mActiveContent = aContent;
    NS_IF_ADDREF(mActiveContent);
  }

  if ((aState & NS_EVENT_STATE_HOVER) && (aContent != mHoverContent)) {
    //transferring ref to notifyContent from mHoverContent
    notifyContent[1] = mHoverContent; // notify hover first, since more common case
    mHoverContent = aContent;
    NS_IF_ADDREF(mHoverContent);
  }

  if ((aState & NS_EVENT_STATE_FOCUS) && (aContent != mCurrentFocus)) {
    SendFocusBlur(aContent);

    //transferring ref to notifyContent from mCurrentFocus
    notifyContent[3] = mCurrentFocus;
    mCurrentFocus = aContent;
    NS_IF_ADDREF(mCurrentFocus);
  }

  if (aContent) { // notify about new content too
    notifyContent[0] = aContent;
    NS_ADDREF(aContent);  // everything in notify array has a ref
  }

  // remove duplicates
  if ((notifyContent[4] == notifyContent[3]) || (notifyContent[4] == notifyContent[2]) || (notifyContent[4] == notifyContent[1])) {
    NS_IF_RELEASE(notifyContent[4]);
  }
  // remove duplicates
  if ((notifyContent[3] == notifyContent[2]) || (notifyContent[3] == notifyContent[1])) {
    NS_IF_RELEASE(notifyContent[3]);
  }
  if (notifyContent[2] == notifyContent[1]) {
    NS_IF_RELEASE(notifyContent[2]);
  }

  // remove notifications for content not in document.
  // we may decide this is possible later but right now it has problems.
  nsIDocument* doc = nsnull;
  for  (int i = 0; i < maxNotify; i++) {
    if (notifyContent[i] && NS_SUCCEEDED(notifyContent[i]->GetDocument(doc)) && !doc) {
      NS_RELEASE(notifyContent[i]);
    }
    NS_IF_RELEASE(doc);
  }

  // compress the notify array to group notifications tighter
  nsIContent** from = &(notifyContent[0]);
  nsIContent** to   = &(notifyContent[0]);
  nsIContent** end  = &(notifyContent[maxNotify]);

  while (from < end) {
    if (! *from) {
      while (++from < end) {
        if (*from) {
          *to++ = *from;
          *from = nsnull;
          break;
        }
      }
    }
    else {
      if (from == to) {
        to++;
        from++;
      }
      else {
        *to++ = *from;
        *from++ = nsnull;
      }
    }
  }

  if (notifyContent[0]) { // have at least one to notify about
    nsIDocument *document;  // this presumes content can't get/lose state if not connected to doc
    notifyContent[0]->GetDocument(document);
    if (document) {
      document->ContentStatesChanged(notifyContent[0], notifyContent[1]);
      if (notifyContent[2]) {  // more that two notifications are needed (should be rare)
        // XXX a further optimization here would be to group the notification pairs
        // together by parent/child, only needed if more than two content changed
        // (ie: if [0] and [2] are parent/child, then notify (0,2) (1,3))
        document->ContentStatesChanged(notifyContent[2], notifyContent[3]);
        if (notifyContent[4]) {  // more that two notifications are needed (should be rare)
          document->ContentStatesChanged(notifyContent[4], nsnull);
        }
      }
      NS_RELEASE(document);
    }

    from = &(notifyContent[0]);
    while (from < to) {  // release old refs now that we are through
      nsIContent* notify = *from++;
      NS_RELEASE(notify);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsEventStateManager::SendFocusBlur(nsIContent *aContent)
{
  if (mCurrentFocus == aContent) {
    return NS_OK;
  }

  nsIContent* targetBeforeEvent = mCurrentTargetContent;

  if (mCurrentFocus) {
    ChangeFocus(mCurrentFocus, PR_FALSE);

    //fire blur
    nsEventStatus status = nsEventStatus_eIgnore;
    nsEvent event;
    event.eventStructType = NS_EVENT;
    event.message = NS_BLUR_CONTENT;

    mCurrentTargetContent = mCurrentFocus;
    NS_ADDREF(mCurrentTargetContent);

    if (nsnull != mPresContext) {
      mCurrentFocus->HandleDOMEvent(*mPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, status); 
    }

    NS_RELEASE(mCurrentTargetContent);
  }

  if (nsnull != aContent) {
    //fire focus
    nsEventStatus status = nsEventStatus_eIgnore;
    nsEvent event;
    event.eventStructType = NS_EVENT;
    event.message = NS_FOCUS_CONTENT;

    mCurrentTargetContent = aContent;
    NS_ADDREF(mCurrentTargetContent);

    if (nsnull != mPresContext) {
      aContent->HandleDOMEvent(*mPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, status);
    }

    NS_RELEASE(mCurrentTargetContent);

    //reset mCurretTargetContent to what it was
    mCurrentTargetContent = targetBeforeEvent;

    nsAutoString tabIndex;
    aContent->GetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::tabindex, tabIndex);
    PRInt32 ec, val = tabIndex.ToInteger(&ec);
    if (NS_OK == ec) {
      mCurrentTabIndex = val;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsEventStateManager::GetFocusedContent(nsIContent** aContent)
{
  *aContent = mCurrentFocus;
  NS_IF_ADDREF(*aContent);
  return NS_OK;
}

nsresult NS_NewEventStateManager(nsIEventStateManager** aInstancePtrResult)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "nsnull ptr");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }
  nsIEventStateManager* manager = new nsEventStateManager();
  if (nsnull == manager) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return manager->QueryInterface(kIEventStateManagerIID, (void **) aInstancePtrResult);
}

