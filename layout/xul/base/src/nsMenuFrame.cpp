/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#include "nsXULAtoms.h"
#include "nsHTMLAtoms.h"
#include "nsMenuFrame.h"
#include "nsBoxFrame.h"
#include "nsIContent.h"
#include "prtypes.h"
#include "nsIAtom.h"
#include "nsIPresContext.h"
#include "nsIStyleContext.h"
#include "nsIReflowCommand.h"
#include "nsCSSRendering.h"
#include "nsINameSpaceManager.h"
#include "nsLayoutAtoms.h"
#include "nsMenuPopupFrame.h"
#include "nsMenuBarFrame.h"
#include "nsIView.h"
#include "nsIWidget.h"
#include "nsIDocument.h"
#include "nsIDOMNSDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMXULDocument.h"
#include "nsIDOMElement.h"
#include "nsISupportsArray.h"
#include "nsIDOMText.h"

#define NS_MENU_POPUP_LIST_INDEX   (NS_AREA_FRAME_ABSOLUTE_LIST_INDEX + 1)

static PRInt32 gEatMouseMove = PR_FALSE;

nsMenuDismissalListener* nsMenuFrame::mDismissalListener = nsnull;

//
// NS_NewMenuFrame
//
// Wrapper for creating a new menu popup container
//
nsresult
NS_NewMenuFrame(nsIFrame** aNewFrame, PRUint32 aFlags)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsMenuFrame* it = new nsMenuFrame;
  if ( !it )
    return NS_ERROR_OUT_OF_MEMORY;
  *aNewFrame = it;
  if (aFlags)
    it->SetIsMenu(PR_TRUE);
  return NS_OK;
}

NS_IMETHODIMP_(nsrefcnt) 
nsMenuFrame::AddRef(void)
{
  return NS_OK;
}

NS_IMETHODIMP_(nsrefcnt)
nsMenuFrame::Release(void)
{
    return NS_OK;
}

NS_IMETHODIMP nsMenuFrame::QueryInterface(REFNSIID aIID, void** aInstancePtr)      
{           
  if (NULL == aInstancePtr) {                                            
    return NS_ERROR_NULL_POINTER;                                        
  }                                                  
  *aInstancePtr = NULL;
  if (aIID.Equals(nsIMenuFrame::GetIID())) {
    *aInstancePtr = (void*)(nsIMenuFrame*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  else if (aIID.Equals(nsIAnonymousContentCreator::GetIID())) {                           
    *aInstancePtr = (void*)(nsIAnonymousContentCreator*) this;                                        
    NS_ADDREF_THIS();                                                    
    return NS_OK;                                                        
  }
  else if (aIID.Equals(nsITimerCallback::GetIID())) {                           
    *aInstancePtr = (void*)(nsITimerCallback*) this;                                        
    NS_ADDREF_THIS();                                                    
    return NS_OK;                                                        
  }
  return nsBoxFrame::QueryInterface(aIID, aInstancePtr);                                     
}

//
// nsMenuFrame cntr
//
nsMenuFrame::nsMenuFrame()
  : mIsMenu(PR_FALSE),
    mMenuOpen(PR_FALSE),
    mHasAnonymousContent(PR_FALSE),
    mMenuParent(nsnull),
    mPresContext(nsnull)
{

} // cntr

NS_IMETHODIMP
nsMenuFrame::Init(nsIPresContext&  aPresContext,
                     nsIContent*      aContent,
                     nsIFrame*        aParent,
                     nsIStyleContext* aContext,
                     nsIFrame*        aPrevInFlow)
{
  mPresContext = &aPresContext; // Don't addref it.  Our lifetime is shorter.

  nsresult  rv = nsBoxFrame::Init(aPresContext, aContent, aParent, aContext, aPrevInFlow);

  // Set our menu parent.
  nsCOMPtr<nsIMenuParent> menuparent = do_QueryInterface(aParent);
  mMenuParent = menuparent.get();
  return rv;
}

// The following methods are all overridden to ensure that the menupopup frame
// is placed in the appropriate list.
NS_IMETHODIMP
nsMenuFrame::FirstChild(nsIAtom*   aListName,
                        nsIFrame** aFirstChild) const
{
  if (nsLayoutAtoms::popupList == aListName) {
    *aFirstChild = mPopupFrames.FirstChild();
  } else {
    nsBoxFrame::FirstChild(aListName, aFirstChild);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsMenuFrame::SetInitialChildList(nsIPresContext& aPresContext,
                                               nsIAtom*        aListName,
                                               nsIFrame*       aChildList)
{
  nsresult rv = NS_OK;
  if (nsLayoutAtoms::popupList == aListName) {
    mPopupFrames.SetFrames(aChildList);
  } else {

    nsFrameList frames(aChildList);

    // We may have a menupopup in here. Get it out, and move it into
    // the popup frame list.
    nsIFrame* frame = frames.FirstChild();
    while (frame) {
      nsCOMPtr<nsIContent> content;
      frame->GetContent(getter_AddRefs(content));
      nsCOMPtr<nsIAtom> tag;
      content->GetTag(*getter_AddRefs(tag));
      if (tag.get() == nsXULAtoms::menupopup) {
        // Remove this frame from the list and place it in the other list.
        frames.RemoveFrame(frame);
        mPopupFrames.AppendFrame(this, frame);
        nsIFrame* first = frames.FirstChild();
        rv = nsBoxFrame::SetInitialChildList(aPresContext, aListName, first);
        return rv;
      }
      frame->GetNextSibling(&frame);
    }

    // Didn't find it.
    rv = nsBoxFrame::SetInitialChildList(aPresContext, aListName, aChildList);
  }
  return rv;
}

NS_IMETHODIMP
nsMenuFrame::GetAdditionalChildListName(PRInt32   aIndex,
                                        nsIAtom** aListName) const
{
  return nsBoxFrame::GetAdditionalChildListName(aIndex, aListName);
}

NS_IMETHODIMP
nsMenuFrame::Destroy(nsIPresContext& aPresContext)
{
   // Cleanup frames in popup child list
  mPopupFrames.DestroyFrames(aPresContext);
  return nsBoxFrame::Destroy(aPresContext);
}

// Called to prevent events from going to anything inside the menu.
NS_IMETHODIMP
nsMenuFrame::GetFrameForPoint(const nsPoint& aPoint, 
                              nsIFrame**     aFrame)
{
  *aFrame = this; // Capture all events so that we can perform selection
  return NS_OK;
}

NS_IMETHODIMP 
nsMenuFrame::HandleEvent(nsIPresContext& aPresContext, 
                             nsGUIEvent*     aEvent,
                             nsEventStatus&  aEventStatus)
{
  aEventStatus = nsEventStatus_eConsumeDoDefault;
  
  if (IsDisabled()) // Disabled menus process no events.
    return NS_OK;

  if (aEvent->message == NS_MOUSE_LEFT_BUTTON_DOWN) {
    PRBool isMenuBar = PR_FALSE;
    if (mMenuParent)
      mMenuParent->IsMenuBar(isMenuBar);

    // The menu item was selected. Bring up the menu.
    // We have children.
    if (mIsMenu)
      ToggleMenuState();

    if (isMenuBar && mIsMenu) {

      if (!IsOpen()) {
        // We closed up. The menu bar should always be
        // deactivated when this happens.
        mMenuParent->SetActive(PR_FALSE);
      }
    }
  }
  else if (aEvent->message == NS_MOUSE_LEFT_BUTTON_UP && !IsMenu() &&
           mMenuParent) {
    // Execute the execute event handler.
    Execute();
  }
  else if (aEvent->message == NS_MOUSE_EXIT) {
    // Kill our timer if one is active.
    if (mOpenTimer) {
      mOpenTimer->Cancel();
      mOpenTimer = nsnull;
    }

    // Deactivate the menu.
    PRBool isActive = PR_FALSE;
    PRBool isMenuBar = PR_FALSE;
    if (mMenuParent) {
      mMenuParent->IsMenuBar(isMenuBar);
      PRBool cancel = PR_TRUE;
      if (isMenuBar) {
        mMenuParent->GetIsActive(isActive);
        if (isActive) cancel = PR_FALSE;
      }
      
      if (cancel) {
        if (IsMenu() && !isMenuBar && mMenuOpen) {
          // Submenus don't get closed up.
        }
        else mMenuParent->SetCurrentMenuItem(nsnull);
      }
    }
  }
  else if (aEvent->message == NS_MOUSE_MOVE && mMenuParent) {
    if (gEatMouseMove) {
      gEatMouseMove = PR_FALSE;
      return NS_OK;
    }

    // we checked for mMenuParent right above

    PRBool isMenuBar = PR_FALSE;
    mMenuParent->IsMenuBar(isMenuBar);

    // Let the menu parent know we're the new item.
    mMenuParent->SetCurrentMenuItem(this);
    
    // If we're a menu (and not a menu item),
    // kick off the timer.
    if (!isMenuBar && IsMenu() && !mMenuOpen && !mOpenTimer) {
        // We're a menu, we're built, we're closed, and no timer has been kicked off.
        NS_NewTimer(getter_AddRefs(mOpenTimer));
        mOpenTimer->Init(this, 250);   // 250 ms delay 
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsMenuFrame::ToggleMenuState()
{  
  if (mMenuOpen) {
    OpenMenu(PR_FALSE);
  }
  else {
    OpenMenu(PR_TRUE);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMenuFrame::SelectMenu(PRBool aActivateFlag)
{
  if (aActivateFlag) {
    // Highlight the menu.
    mContent->SetAttribute(kNameSpaceID_None, nsXULAtoms::menuactive, "true", PR_TRUE);
  }
  else {
    // Unhighlight the menu.
    mContent->UnsetAttribute(kNameSpaceID_None, nsXULAtoms::menuactive, PR_TRUE);
  }

  return NS_OK;
}

PRBool nsMenuFrame::IsGenerated()
{
  nsCOMPtr<nsIContent> child;
  GetMenuChildrenElement(getter_AddRefs(child));
  
  // Generate the menu if it hasn't been generated already.  This
  // takes it from display: none to display: block and gives us
  // a menu forevermore.
  if (child) {
    nsString genVal;
    child->GetAttribute(kNameSpaceID_None, nsXULAtoms::menugenerated, genVal);
    if (genVal == "")
      return PR_FALSE;
  }

  return PR_TRUE;
}

NS_IMETHODIMP
nsMenuFrame::MarkAsGenerated()
{
  nsCOMPtr<nsIContent> child;
  GetMenuChildrenElement(getter_AddRefs(child));
  
  // Generate the menu if it hasn't been generated already.  This
  // takes it from display: none to display: block and gives us
  // a menu forevermore.
  if (child) {
    nsAutoString genVal;
    child->GetAttribute(kNameSpaceID_None, nsXULAtoms::menugenerated, genVal);
    if (genVal == "")
      child->SetAttribute(kNameSpaceID_None, nsXULAtoms::menugenerated, "true", PR_TRUE);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMenuFrame::ActivateMenu(PRBool aActivateFlag)
{
  // Activate the menu without opening it.
  nsCOMPtr<nsIContent> child;
  GetMenuChildrenElement(getter_AddRefs(child));

  // We've got some children for real.
  if (child) {
    if (aActivateFlag)
      child->SetAttribute(kNameSpaceID_None, nsXULAtoms::menuactive, "true", PR_TRUE);
    else child->UnsetAttribute(kNameSpaceID_None, nsXULAtoms::menuactive, PR_TRUE);
  }

  return NS_OK;
}  

NS_IMETHODIMP
nsMenuFrame::AttributeChanged(nsIPresContext* aPresContext,
                              nsIContent* aChild,
                              nsIAtom* aAttribute,
                              PRInt32 aHint)
{
  nsAutoString value;

  if (aAttribute == nsXULAtoms::open) {
    aChild->GetAttribute(kNameSpaceID_None, aAttribute, value);
    if (value == "true")
      OpenMenuInternal(PR_TRUE);
    else
      OpenMenuInternal(PR_FALSE);
  }

  if (mHasAnonymousContent) {
    if (aAttribute == nsXULAtoms::accesskey ||
        aAttribute == nsHTMLAtoms::value) {
      /* update accesskey or value on menu-left */
      aChild->GetAttribute(kNameSpaceID_None, aAttribute, value);
      mMenuText->SetAttribute(kNameSpaceID_None, aAttribute, value, PR_TRUE);
    } else if (aAttribute == nsXULAtoms::acceltext) {
      /* update content in accel-text */
      aChild->GetAttribute(kNameSpaceID_None, aAttribute, value);
      mAccelText->SetData(value);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMenuFrame::OpenMenu(PRBool aActivateFlag)
{
  nsCOMPtr<nsIDOMElement> domElement = do_QueryInterface(mContent);
  if (aActivateFlag) {
    // Now that the menu is opened, we should have a menupopup child built.
    // Mark it as generated, which ensures a frame gets built.
    MarkAsGenerated();

    domElement->SetAttribute("open", "true");
  }
  else domElement->RemoveAttribute("open");

  return NS_OK;
}

void 
nsMenuFrame::OpenMenuInternal(PRBool aActivateFlag) 
{
  gEatMouseMove = PR_TRUE;

  if (!mIsMenu)
    return;

  if (aActivateFlag) {
    // Execute the oncreate handler
    if (!OnCreate())
      return;

    // XXX Only have this here because of RDF-generated content.
    MarkAsGenerated();

    nsIFrame* frame = mPopupFrames.FirstChild();
    nsMenuPopupFrame* menuPopup = (nsMenuPopupFrame*)frame;
  
    nsCOMPtr<nsIMenuParent> childPopup = do_QueryInterface(frame);
    UpdateDismissalListener(childPopup);

    ActivateMenu(PR_TRUE);
    if (menuPopup) {
      // Tell the menu bar we're active.
      if (mMenuParent)
        mMenuParent->SetActive(PR_TRUE);

      // Sync up the view.
      PRBool onMenuBar = PR_FALSE;
      if (mMenuParent)
        mMenuParent->IsMenuBar(onMenuBar);

      menuPopup->SyncViewWithFrame(*mPresContext, onMenuBar, this, -1, -1);
    }

    mMenuOpen = PR_TRUE;
  }
  else {
    // Close the menu. 
    // Execute the ondestroy handler
    if (!OnDestroy())
      return;

    UpdateDismissalListener(mMenuParent);

    nsIFrame* frame = mPopupFrames.FirstChild();
    nsMenuPopupFrame* menuPopup = (nsMenuPopupFrame*)frame;
  
    // Make sure we clear out our own items.
    if (menuPopup)
      menuPopup->SetCurrentMenuItem(nsnull);

    ActivateMenu(PR_FALSE);

    mMenuOpen = PR_FALSE;

    // Set the focus back to our view's widget.
    nsIView*  view;
    GetView(&view);
    if (!view) {
      nsPoint offset;
      GetOffsetFromView(offset, &view);
    }
    nsCOMPtr<nsIWidget> widget;
    view->GetWidget(*getter_AddRefs(widget));
    widget->SetFocus();
  }
}

void
nsMenuFrame::GetMenuChildrenElement(nsIContent** aResult)
{
  PRInt32 count;
  mContent->ChildCount(count);

  for (PRInt32 i = 0; i < count; i++) {
    nsCOMPtr<nsIContent> child;
    mContent->ChildAt(i, *getter_AddRefs(child));
    nsCOMPtr<nsIAtom> tag;
    child->GetTag(*getter_AddRefs(tag));
    if (tag && tag.get() == nsXULAtoms::menupopup) {
      *aResult = child.get();
      NS_ADDREF(*aResult);
      return;
    }
  }
}

NS_IMETHODIMP
nsMenuFrame::Reflow(nsIPresContext&   aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus)
{
  nsresult rv = nsBoxFrame::Reflow(aPresContext, aDesiredSize, aReflowState, aStatus);
  nsIFrame* frame = mPopupFrames.FirstChild();
    
  if (!frame || (rv != NS_OK))
    return rv;

  // Constrain the child's width and height to aAvailableWidth and aAvailableHeight
  nsSize availSize(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);
  nsHTMLReflowState kidReflowState(aPresContext, aReflowState, frame,
                                   availSize);
  kidReflowState.mComputedWidth = NS_UNCONSTRAINEDSIZE;
  kidReflowState.mComputedHeight = NS_UNCONSTRAINEDSIZE;
    
   // Reflow child
  nscoord w = aDesiredSize.width;
  nscoord h = aDesiredSize.height;
  
  rv = ReflowChild(frame, aPresContext, aDesiredSize, kidReflowState, aStatus);

   // Set the child's width and height to its desired size
  nsRect rect;
  frame->GetRect(rect);
  rect.width = aDesiredSize.width;
  rect.height = aDesiredSize.height;
  frame->SetRect(rect);

  // Don't let it affect our size.
  aDesiredSize.width = w;
  aDesiredSize.height = h;
  
  return rv;
}


NS_IMETHODIMP
nsMenuFrame::DidReflow(nsIPresContext& aPresContext,
                            nsDidReflowStatus aStatus)
{
  // Sync up the view.
  PRBool onMenuBar = PR_FALSE;
  if (mMenuParent)
    mMenuParent->IsMenuBar(onMenuBar);

  nsIFrame* frame = mPopupFrames.FirstChild();
  nsMenuPopupFrame* menuPopup = (nsMenuPopupFrame*)frame;
  if (menuPopup && mMenuOpen)
    menuPopup->SyncViewWithFrame(aPresContext, onMenuBar, this, -1, -1);

  return nsBoxFrame::DidReflow(aPresContext, aStatus);
}

// Overridden Box method.
NS_IMETHODIMP
nsMenuFrame::Dirty(const nsHTMLReflowState& aReflowState, nsIFrame*& incrementalChild)
{
  incrementalChild = nsnull;
  nsresult rv = NS_OK;

  // Dirty any children that need it.
  nsIFrame* frame;
  aReflowState.reflowCommand->GetNext(frame, PR_FALSE);
  if (frame == nsnull) {
    incrementalChild = this;
    return rv;
  }

  // Now call our original box frame method
  rv = nsBoxFrame::Dirty(aReflowState, incrementalChild);
  if (rv != NS_OK || incrementalChild)
    return rv;

  nsIFrame* popup = mPopupFrames.FirstChild();
  if (popup && (frame == popup)) {
    // An incremental reflow command is targeting something inside our
    // hidden popup view.  We can't actually return the child, since it
    // won't ever be found by box.  Instead return ourselves, so that box
    // will later send us an incremental reflow command.
    incrementalChild = this;

    // In order for the child box to know what it needs to reflow, we need
    // to call its Dirty method...
    nsIFrame* ignore;
    nsIBox* ibox;
    if (NS_SUCCEEDED(popup->QueryInterface(nsIBox::GetIID(), (void**)&ibox)) && ibox)
      ibox->Dirty(aReflowState, ignore);
  }

  return rv;
}

NS_IMETHODIMP
nsMenuFrame::ShortcutNavigation(PRUint32 aLetter, PRBool& aHandledFlag)
{
  nsIFrame* frame = mPopupFrames.FirstChild();
  if (frame) {
    nsMenuPopupFrame* popup = (nsMenuPopupFrame*)frame;
    popup->ShortcutNavigation(aLetter, aHandledFlag);
  } 

  return NS_OK;
}

NS_IMETHODIMP
nsMenuFrame::KeyboardNavigation(PRUint32 aDirection, PRBool& aHandledFlag)
{
  nsIFrame* frame = mPopupFrames.FirstChild();
  if (frame) {
    nsMenuPopupFrame* popup = (nsMenuPopupFrame*)frame;
    popup->KeyboardNavigation(aDirection, aHandledFlag);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMenuFrame::Escape(PRBool& aHandledFlag)
{
  nsIFrame* frame = mPopupFrames.FirstChild();
  if (frame) {
    nsMenuPopupFrame* popup = (nsMenuPopupFrame*)frame;
    popup->Escape(aHandledFlag);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMenuFrame::Enter()
{
  if (!mMenuOpen) {
    // The enter key press applies to us.
    if (!IsMenu() && mMenuParent) {
      // Execute our event handler
      Execute();
    }
    else {
      OpenMenu(PR_TRUE);
      SelectFirstItem();
    }

    return NS_OK;
  }

  nsIFrame* frame = mPopupFrames.FirstChild();
  if (frame) {
    nsMenuPopupFrame* popup = (nsMenuPopupFrame*)frame;
    popup->Enter();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMenuFrame::SelectFirstItem()
{
  nsIFrame* frame = mPopupFrames.FirstChild();
  if (frame) {
    nsMenuPopupFrame* popup = (nsMenuPopupFrame*)frame;
    nsIMenuFrame* result;
    popup->GetNextMenuItem(nsnull, &result);
    popup->SetCurrentMenuItem(result);
  }

  return NS_OK;
}

PRBool
nsMenuFrame::IsMenu()
{
  nsCOMPtr<nsIAtom> tag;
  mContent->GetTag(*getter_AddRefs(tag));
  if (tag.get() == nsXULAtoms::menu)
    return PR_TRUE;
  return PR_FALSE;
}

void
nsMenuFrame::Notify(nsITimer* aTimer)
{
  // Our timer has fired.
  if (aTimer == mOpenTimer.get()) {
    if (!mMenuOpen && mMenuParent) {
      nsAutoString active = "";
      mContent->GetAttribute(kNameSpaceID_None, nsXULAtoms::menuactive, active);
      if (active == "true") {
        // We're still the active menu.
        OpenMenu(PR_TRUE);
      }
    }
    mOpenTimer->Cancel();
    mOpenTimer = nsnull;
  }
  
  mOpenTimer = nsnull;
}

PRBool 
nsMenuFrame::IsDisabled()
{
  nsAutoString disabled;
  mContent->GetAttribute(kNameSpaceID_None, nsHTMLAtoms::disabled, disabled);
  if (disabled == "true")
    return PR_TRUE;
  return PR_FALSE;
}

NS_IMETHODIMP
nsMenuFrame::CreateAnonymousContent(nsISupportsArray& aAnonymousChildren)
{
  // Create anonymous children only if the menu has no children or
  // only has a menuchildren as its child.
  nsCOMPtr<nsIDOMNode> dummyResult;
  
  PRInt32 childCount;
  mContent->ChildCount(childCount);
  mHasAnonymousContent = PR_TRUE;
  for (PRInt32 i = 0; i < childCount; i++) {
    // XXX Should optimize this to look for a display type of none.
    // Not sure how to do this.  For now screen out some known tags.
    nsCOMPtr<nsIContent> childContent;
    mContent->ChildAt(i, *getter_AddRefs(childContent));
    nsCOMPtr<nsIAtom> tag;
    childContent->GetTag(*getter_AddRefs(tag));
    if (tag.get() != nsXULAtoms::menupopup &&
      tag.get() != nsXULAtoms::templateAtom &&
      tag.get() != nsXULAtoms::observes) {
      mHasAnonymousContent = PR_FALSE;
      break;
    }
  }

  if (!mHasAnonymousContent)
    return NS_OK;

  nsCOMPtr<nsIDocument> idocument;
  mContent->GetDocument(*getter_AddRefs(idocument));
  nsCOMPtr<nsIDOMNSDocument> nsDocument(do_QueryInterface(idocument));
  nsCOMPtr<nsIDOMDocument> document(do_QueryInterface(idocument));

  nsAutoString xulNamespace("http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul");
  nsAutoString htmlNamespace("http://www.w3.org/TR/REC-html40");
  nsCOMPtr<nsIAtom> classAtom = dont_AddRef(NS_NewAtom("class"));

  nsCOMPtr<nsIDOMElement> node;
  nsCOMPtr<nsIContent> content;

  PRBool onMenuBar = PR_FALSE;
  if (mMenuParent)
    mMenuParent->IsMenuBar(onMenuBar);
  
  /* Create .menu-left (.menubar-left) titledbutton for icon. */
  nsDocument->CreateElementWithNameSpace("titledbutton", xulNamespace,
                                         getter_AddRefs(node));
  content = do_QueryInterface(node);
  content->SetAttribute(kNameSpaceID_None, classAtom,
                        onMenuBar ? "menubar-left" : "menu-left" , PR_FALSE);
  aAnonymousChildren.AppendElement(content);
  
  nsAutoString beforeString;
  nsAutoString accessString;
  nsAutoString afterString;
  SplitOnShortcut(beforeString, accessString, afterString);

  /*
   * Create the .menu-text titledbutton, and propagate crop, accesskey and
   * value attributes.  If we're a menubar, make the class menubar-text
   * instead.
   */
  nsDocument->CreateElementWithNameSpace("titledbutton", xulNamespace,
                                         getter_AddRefs(node));
  content = do_QueryInterface(node);
  content->SetAttribute(kNameSpaceID_None, classAtom, 
                        onMenuBar ? "menubar-text" : "menu-text", PR_FALSE);
  nsAutoString accessKey, value, crop;

  mMenuText = content;
  mContent->GetAttribute(kNameSpaceID_None, nsHTMLAtoms::value, value);
  content->SetAttribute(kNameSpaceID_None, nsHTMLAtoms::value, value, PR_FALSE);
  mContent->GetAttribute(kNameSpaceID_None, nsXULAtoms::accesskey, accessKey);
  content->SetAttribute(kNameSpaceID_None, nsXULAtoms::accesskey, accessKey,
                        PR_FALSE);
  
  mContent->GetAttribute(kNameSpaceID_None, nsXULAtoms::crop, crop);
  if (crop == "")
    crop = "right";
  content->SetAttribute(kNameSpaceID_None, nsXULAtoms::crop, crop, PR_FALSE);

  // append now, after we've set all the attributes
  aAnonymousChildren.AppendElement(content);

  // Create a spring that serves as padding between the text and the
  // accelerator.
  if (!onMenuBar) {
    nsDocument->CreateElementWithNameSpace("spring", xulNamespace, getter_AddRefs(node));
    content = do_QueryInterface(node);
    content->SetAttribute(kNameSpaceID_None, nsXULAtoms::flex, "100", PR_FALSE);
    aAnonymousChildren.AppendElement(content);
  
    // Build the accelerator out of the corresponding key node.
    nsAutoString accelString;
    BuildAcceleratorText(accelString);
    if (accelString != "") {
      // Create the accelerator (it's a div)
      nsDocument->CreateElementWithNameSpace("div", htmlNamespace, getter_AddRefs(node));
      content = do_QueryInterface(node);
      aAnonymousChildren.AppendElement(content);

      nsCOMPtr<nsIDOMText> accelNode;
      document->CreateTextNode(accelString, getter_AddRefs(accelNode));
      mAccelText = accelNode;
      node->AppendChild(accelNode, getter_AddRefs(dummyResult));
    }

    // Create the "menu-right" object.  It's a titledbutton.
    // XXX Maybe we should make one for a .menubar-right class so that the option exists
    nsDocument->CreateElementWithNameSpace("titledbutton", xulNamespace, getter_AddRefs(node));
    content = do_QueryInterface(node);
    content->SetAttribute(kNameSpaceID_None, classAtom, "menu-right", PR_FALSE);
    aAnonymousChildren.AppendElement(content);
  }

  return NS_OK;
}

void 
nsMenuFrame::SplitOnShortcut(nsString& aBeforeString, nsString& aAccessString, nsString& aAfterString)
{
  nsString value;
  nsString accessKey;
  aBeforeString = value;
  aAccessString = "";
  aAfterString = "";

  if (accessKey == "") // Nothing to do. 
    return;

  // Find the index of the first occurrence of the accessKey
  PRInt32 indx = value.Find(accessKey, PR_TRUE);

  if (indx == -1) // Wasn't in there. Just return.
    return;

  // It was in the value string. Split based on the indx.
  value.Left(aBeforeString, indx);
  value.Mid(aAccessString, indx, 1);
  value.Right(aAfterString, value.Length()-indx-1);
}

void 
nsMenuFrame::BuildAcceleratorText(nsString& aAccelString)
{
  nsString accelText;
  mContent->GetAttribute(kNameSpaceID_None, nsXULAtoms::acceltext, accelText);
  if (accelText != "") {
    // Just use this.
    aAccelString = accelText;
    return;
  }

  // See if we have a key node and use that instead.
  nsString keyValue;
  mContent->GetAttribute(kNameSpaceID_None, nsXULAtoms::key, keyValue);

  nsCOMPtr<nsIDocument> document;
  mContent->GetDocument(*getter_AddRefs(document));

  // Turn the document into a XUL document so we can use getElementById
  nsCOMPtr<nsIDOMXULDocument> xulDocument = do_QueryInterface(document);
  if (!xulDocument)
    return;

  nsCOMPtr<nsIDOMElement> keyElement;
  xulDocument->GetElementById(keyValue, getter_AddRefs(keyElement));
  
  if (!keyElement)
    return;
    
  nsAutoString keyAtom("key");
  nsAutoString shiftAtom("shift");
	nsAutoString altAtom("alt");
	nsAutoString commandAtom("command");
  nsAutoString controlAtom("control");

	nsString shiftValue;
	nsString altValue;
	nsString commandValue;
  nsString controlValue;
	nsString keyChar = " ";
	
	keyElement->GetAttribute(keyAtom, keyChar);
	keyElement->GetAttribute(shiftAtom, shiftValue);
	keyElement->GetAttribute(altAtom, altValue);
	keyElement->GetAttribute(commandAtom, commandValue);
  keyElement->GetAttribute(controlAtom, controlValue);
	  
  PRBool prependPlus = PR_FALSE;

  if(commandValue != "" && commandValue != "false") {
    prependPlus = PR_TRUE;
	  aAccelString += "Ctrl"; // Hmmm. Kinda defeats the point of having an abstraction.
  }

  if(controlValue != "" && controlValue != "false") {
    prependPlus = PR_TRUE;
	  aAccelString += "Ctrl";
  }

  if(shiftValue != "" && shiftValue != "false") {
    if (prependPlus)
      aAccelString += "+";
    prependPlus = PR_TRUE;
    aAccelString += "Shift";
  }

  if (altValue != "" && altValue != "false") {
	  if (prependPlus)
      aAccelString += "+";
    prependPlus = PR_TRUE;
    aAccelString += "Alt";
  }

  keyChar.ToUpperCase();
  if (keyChar != "") {
    if (prependPlus)
      aAccelString += "+";
    prependPlus = PR_TRUE;
    aAccelString += keyChar;
  }
}

void
nsMenuFrame::Execute()
{
  // Get our own content node and hold on to it to keep it from going away.
  nsCOMPtr<nsIContent> content = dont_QueryInterface(mContent);
  
  // First hide all of the open menus.
  if (mMenuParent)
    mMenuParent->HideChain();

  nsEventStatus status = nsEventStatus_eIgnore;
  nsMouseEvent event;
  event.eventStructType = NS_EVENT;
  event.message = NS_MENU_ACTION;
  mContent->HandleDOMEvent(*mPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, status);

  // XXX HACK. Just gracefully exit if the node has been removed, e.g., window.close()
  // was executed.
  nsCOMPtr<nsIDocument> doc;
  content->GetDocument(*getter_AddRefs(doc));

  // Now properly close them all up.
  if (doc && mMenuParent)
    mMenuParent->DismissChain();
}

PRBool
nsMenuFrame::OnCreate()
{
  nsEventStatus status = nsEventStatus_eIgnore;
  nsMouseEvent event;
  event.eventStructType = NS_EVENT;
  event.message = NS_MENU_CREATE;
  
  nsCOMPtr<nsIContent> child;
  GetMenuChildrenElement(getter_AddRefs(child));
  
  nsresult rv;
  if (child) 
    rv = child->HandleDOMEvent(*mPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, status);
  else rv = mContent->HandleDOMEvent(*mPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, status);

  if ( NS_FAILED(rv) || status == nsEventStatus_eConsumeNoDefault )
    return PR_FALSE;
  return PR_TRUE;
}

PRBool
nsMenuFrame::OnDestroy()
{
  nsEventStatus status = nsEventStatus_eIgnore;
  nsMouseEvent event;
  event.eventStructType = NS_EVENT;
  event.message = NS_MENU_DESTROY;
  
  nsCOMPtr<nsIContent> child;
  GetMenuChildrenElement(getter_AddRefs(child));
  
  nsresult rv;
  if (child) 
    rv = child->HandleDOMEvent(*mPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, status);
  else rv = mContent->HandleDOMEvent(*mPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, status);

  if ( NS_FAILED(rv) || status == nsEventStatus_eConsumeNoDefault )
    return PR_FALSE;
  return PR_TRUE;
}

NS_IMETHODIMP
nsMenuFrame::RemoveFrame(nsIPresContext& aPresContext,
                           nsIPresShell& aPresShell,
                           nsIAtom* aListName,
                           nsIFrame* aOldFrame)
{
  // need to rebuild all the springs.
  for (int i=0; i < mSpringCount; i++) 
    mSprings[i].clear();

  nsresult  rv;

  if (mPopupFrames.ContainsFrame(aOldFrame)) {
    // Go ahead and remove this frame.
    mPopupFrames.DestroyFrame(aPresContext, aOldFrame);
    rv = GenerateDirtyReflowCommand(aPresContext, aPresShell);
  } else {
    rv = nsBoxFrame::RemoveFrame(aPresContext, aPresShell, aListName, aOldFrame);
  }

  return rv;
}

NS_IMETHODIMP
nsMenuFrame::InsertFrames(nsIPresContext& aPresContext,
                            nsIPresShell& aPresShell,
                            nsIAtom* aListName,
                            nsIFrame* aPrevFrame,
                            nsIFrame* aFrameList)
{
  // need to rebuild all the springs.
  for (int i=0; i < mSpringCount; i++) 
    mSprings[i].clear();

  nsCOMPtr<nsIContent> frameChild;
  aFrameList->GetContent(getter_AddRefs(frameChild));

  nsCOMPtr<nsIAtom> tag;
  nsresult          rv;

  frameChild->GetTag(*getter_AddRefs(tag));
  if (tag && tag.get() == nsXULAtoms::menupopup) {
    mPopupFrames.InsertFrames(nsnull, nsnull, aFrameList);
    rv = GenerateDirtyReflowCommand(aPresContext, aPresShell);
  } else {
    rv = nsBoxFrame::InsertFrames(aPresContext, aPresShell, aListName, aPrevFrame, aFrameList);  
  }

  return rv;
}

NS_IMETHODIMP
nsMenuFrame::AppendFrames(nsIPresContext& aPresContext,
                           nsIPresShell&   aPresShell,
                           nsIAtom*        aListName,
                           nsIFrame*       aFrameList)
{
  if (!aFrameList)
    return NS_OK;

  // need to rebuild all the springs.
  for (int i=0; i < mSpringCount; i++) 
    mSprings[i].clear();

  nsCOMPtr<nsIContent> frameChild;
  aFrameList->GetContent(getter_AddRefs(frameChild));

  nsCOMPtr<nsIAtom> tag;
  nsresult          rv;

  frameChild->GetTag(*getter_AddRefs(tag));
  if (tag && tag.get() == nsXULAtoms::menupopup) {
    mPopupFrames.AppendFrames(nsnull, aFrameList);
    rv = GenerateDirtyReflowCommand(aPresContext, aPresShell);
  } else {
    rv = nsBoxFrame::AppendFrames(aPresContext, aPresShell, aListName, aFrameList); 
  }

  return rv;
}

void
nsMenuFrame::UpdateDismissalListener(nsIMenuParent* aMenuParent)
{
#if 0
  if (!nsMenuFrame::mDismissalListener) {
    // Create the listener and attach it to the outermost window.
    aMenuParent->CreateDismissalListener();
  }
  
  // Make sure the menu dismissal listener knows what the current
  // innermost menu popup frame is.
  nsMenuFrame::mDismissalListener->SetCurrentMenuParent(aMenuParent);
#endif
}
