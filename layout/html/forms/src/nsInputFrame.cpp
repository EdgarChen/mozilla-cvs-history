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

#include "nsInput.h"
#include "nsInputFrame.h"
#include "nsHTMLParts.h"
#include "nsHTMLContainer.h"
#include "nsIRenderingContext.h"
#include "nsIPresShell.h"
#include "nsIPresContext.h"
#include "nsIStyleContext.h"
#include "nsLeafFrame.h"
#include "nsCSSRendering.h"
#include "nsHTMLIIDs.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsCoord.h"
#include "nsWidgetsCID.h"
#include "nsViewsCID.h"
#include "nsRepository.h"
#include "nsGUIEvent.h"
#include "nsIFontCache.h"
#include "nsIFontMetrics.h"
#include "nsIFormManager.h"
#include "nsIDeviceContext.h"
#include "nsHTMLAtoms.h"
#include "nsIButton.h"  // remove this when GetCID is pure virtual
#include "nsICheckButton.h"  //remove this
#include "nsITextWidget.h"  //remove this
#include "nsISupports.h"
#include "nsHTMLForms.h"
#include "nsStyleConsts.h"
#include "nsUnitConversion.h"

static NS_DEFINE_IID(kStyleFontSID, NS_STYLEFONT_SID);
static NS_DEFINE_IID(kStylePositionSID, NS_STYLEPOSITION_SID);
static NS_DEFINE_IID(kStyleSpacingSID, NS_STYLESPACING_SID);

struct nsInputCallbackData
{
  nsIPresContext* mPresContext;
  nsInputFrame*   mFrame;
  nsInputCallbackData(nsIPresContext* aPresContext, nsInputFrame* aFrame)
    :mPresContext(aPresContext), mFrame(aFrame)
  {
  }
};

nsInputFrame::nsInputFrame(nsIContent* aContent, nsIFrame* aParentFrame)
  : nsLeafFrame(aContent, aParentFrame)
{
  mLastMouseState = eMouseNone;
}

nsInputFrame::~nsInputFrame()
{
}

NS_METHOD nsInputFrame::SetRect(const nsRect& aRect)
{
  return nsInputFrameSuper::SetRect(aRect);
}

NS_METHOD
nsInputFrame::MoveTo(nscoord aX, nscoord aY)
{
  if ((aX != mRect.x) || (aY != mRect.y)) {
    mRect.x = aX;
    mRect.y = aY;

    // Let the view know
    nsIView* view = nsnull;
    GetView(view);
    if (nsnull != view) {
      // Position view relative to it's parent, not relative to our
      // parent frame (our parent frame may not have a view). Also,
      // inset the view by the border+padding if present
      nsIView* parentWithView;
      nsPoint origin;
      GetOffsetFromView(origin, parentWithView);
      view->SetPosition(origin.x + mViewBounds.x, origin.y + mViewBounds.y);
      NS_IF_RELEASE(parentWithView);
      NS_RELEASE(view);
    }
  }

  return NS_OK;
}

NS_METHOD
nsInputFrame::SizeTo(nscoord aWidth, nscoord aHeight)
{
  mRect.width = aWidth;
  mRect.height = aHeight;

  // Let the view know the correct size
  nsIView* view = nsnull;
  GetView(view);
  if (nsnull != view) {
    view->SetDimensions(mViewBounds.width, mViewBounds.height);
    NS_RELEASE(view);
  }
  return NS_OK;
}


// XXX it would be cool if form element used our rendering sw, then
// they could be blended, and bordered, and so on...
NS_METHOD
nsInputFrame::Paint(nsIPresContext& aPresContext,
                    nsIRenderingContext& aRenderingContext,
                    const nsRect& aDirtyRect)
{
  // Make sure the widget is visible if it isn't currently visible
  nsIView* view = nsnull;
  GetView(view);
  if (nsnull != view) {
    SetViewVisiblity(&aPresContext, PR_TRUE);
    NS_RELEASE(view);
  }

  // Point borders/padding if any
  return nsInputFrameSuper::Paint(aPresContext, aRenderingContext, aDirtyRect);
}

void
nsInputFrame::SetViewVisiblity(nsIPresContext* aPresContext, PRBool aShow)
{
  nsIView* view = nsnull;
  GetView(view);
  if (nsnull != view) {
    nsIWidget* widget;
    nsresult result = GetWidget(view, &widget);
    if (NS_OK == result) {
      // initially the widget was created as hidden
      nsViewVisibility newVisibility =
        aShow ? nsViewVisibility_kShow : nsViewVisibility_kHide;
      if (newVisibility != view->GetVisibility()) {
        // this only inits the 1st time
        // XXX kipp says: this is yucky; init on first visibility seems lame
        ((nsInput*)mContent)->GetFormManager()->Init(PR_FALSE);
        view->SetVisibility(newVisibility);
        PostCreateWidget(aPresContext, view);
      }
      NS_IF_RELEASE(widget);
    }
    NS_RELEASE(view);
  }
}

PRBool 
nsInputFrame::BoundsAreSet()
{
  if ((0 != mRect.width) || (0 != mRect.height)) {
    return PR_TRUE;
  } else {
    return PR_FALSE;
  }
}

PRBool
nsInputFrame::IsHidden()
{
  nsInput* content = (nsInput *)mContent; 
  return content->IsHidden();
}

void 
nsInputFrame::GetDesiredSize(nsIPresContext* aPresContext,
                             const nsSize& aMaxSize,
                             nsReflowMetrics& aDesiredLayoutSize,
                             nsSize& aDesiredWidgetSize)
{
  // get the css size and let the frame use or override it
  nsSize styleSize;
  GetStyleSize(*aPresContext, aMaxSize, styleSize);

  // subclasses should always override this method, but if not and no css, make it small
  aDesiredLayoutSize.width  = (styleSize.width  > CSS_NOTSET) ? styleSize.width  : 144;
  aDesiredLayoutSize.height = (styleSize.height > CSS_NOTSET) ? styleSize.height : 144;
  aDesiredLayoutSize.ascent = aDesiredLayoutSize.height;
  aDesiredLayoutSize.descent = 0;
  aDesiredWidgetSize.width  = aDesiredLayoutSize.width;
  aDesiredWidgetSize.height = aDesiredLayoutSize.height;
}

void 
nsInputFrame::GetDesiredSize(nsIPresContext* aPresContext,
                             nsReflowMetrics& aDesiredSize,
                             const nsSize& aMaxSize)
{
  nsSize ignore;
  GetDesiredSize(aPresContext, aMaxSize, aDesiredSize, ignore);
}

NS_METHOD
nsInputFrame::ResizeReflow(nsIPresContext* aPresContext,
                          nsReflowMetrics& aDesiredSize,
                          const nsSize& aMaxSize,
                          nsSize* aMaxElementSize,
                          ReflowStatus& aStatus)
{
  nsIView* view = nsnull;
  GetView(view);
  if (nsnull == view) {
    static NS_DEFINE_IID(kViewCID, NS_VIEW_CID);
    static NS_DEFINE_IID(kIViewIID, NS_IVIEW_IID);

    // make sure the style context is set
    if (nsnull == mStyleContext) {
      GetStyleContext(aPresContext, mStyleContext);
    }
    nsresult result = 
	    NSRepository::CreateInstance(kViewCID, nsnull, kIViewIID,
                                   (void **)&view);
	  if (NS_OK != result) {
	    NS_ASSERTION(0, "Could not create view for button"); 
      aStatus = frNotComplete;
      return result;
	  }
	  nsIPresShell   *presShell = aPresContext->GetShell();     // need to release
	  nsIViewManager *viewMan   = presShell->GetViewManager();  // need to release

    nsSize widgetSize;
    GetDesiredSize(aPresContext, aMaxSize, aDesiredSize, widgetSize);

    mViewBounds.width = widgetSize.width;
    mViewBounds.height = widgetSize.height;
    nsRect boundBox(0, 0, widgetSize.width, widgetSize.height); 

    nsIFrame* parWithView;
	  nsIView *parView;

    GetParentWithView(parWithView);
	  parWithView->GetView(parView);

	  const nsIID& id = GetCID();
    nsInputWidgetData* initData = GetWidgetInitData(); // needs to be deleted
	  // initialize the view as hidden since we don't know the (x,y) until Paint
    result = view->Init(viewMan, boundBox, parView, &id, initData,
                        nsnull, 0, nsnull,
                        1.0f, nsViewVisibility_kHide);
    if (nsnull != initData) {
      delete(initData);
    }
    if (NS_OK != result) {
	    NS_ASSERTION(0, "widget initialization failed"); 
      aStatus = frNotComplete;
      return NS_OK;
	  }

	  // set the content's widget, so it can get content modified by the widget
	  nsIWidget *widget;
	  result = GetWidget(view, &widget);
	  if (NS_OK == result) {
      nsInput* content = (nsInput *)mContent; // change this cast to QueryInterface 
      content->SetWidget(widget);
	    NS_IF_RELEASE(widget);
	  } else {
	    NS_ASSERTION(0, "could not get widget");
	  }

    viewMan->InsertChild(parView, view, 0);

    SetView(view);
    NS_RELEASE(view);
	  
    NS_IF_RELEASE(parView);
	  NS_IF_RELEASE(viewMan);  
	  NS_IF_RELEASE(presShell); 
  }
  else {
    nsSize widgetSize;
    GetDesiredSize(aPresContext, aMaxSize, aDesiredSize, widgetSize);

    // If we are being reflowed and have a view, hide the view until
    // we are told to paint (which is when our location will have
    // stabilized).
    SetViewVisiblity(aPresContext, PR_FALSE);
  }

  // Add in borders and padding
  nsStyleSpacing* space =
    (nsStyleSpacing*)mStyleContext->GetData(kStyleSpacingSID);
  aDesiredSize.width += space->mBorderPadding.left +
    space->mBorderPadding.right;
  aDesiredSize.height += space->mBorderPadding.top +
    space->mBorderPadding.bottom;
  aDesiredSize.ascent = aDesiredSize.height;
  aDesiredSize.descent = 0;

  // Remember the view's offset coordinates
  mViewBounds.x = space->mBorderPadding.left;
  mViewBounds.y = space->mBorderPadding.top;

  if (nsnull != aMaxElementSize) {
    aMaxElementSize->width = aDesiredSize.width;
	  aMaxElementSize->height = aDesiredSize.height;
  }
    
  aStatus = frComplete;
  return NS_OK;
}

nsInputWidgetData* 
nsInputFrame::GetWidgetInitData()
{
  return nsnull;
}

nscoord nsInputFrame::GetDefaultPadding() const 
{
  return NS_POINTS_TO_TWIPS_INT(2); // XXX should be pixels, need pres context
} 

nscoord nsInputFrame::GetPadding() const
{
  return 0;
}

void 
nsInputFrame::PostCreateWidget(nsIPresContext* aPresContext, nsIView *aView)
{
}

nsresult
nsInputFrame::GetWidget(nsIView* aView, nsIWidget** aWidget)
{
  const nsIID id = GetIID();
  if (NS_OK == aView->QueryInterface(id, (void**)aWidget)) {
    return NS_OK;
  } else {
	NS_ASSERTION(0, "The widget interface is invalid");  // need to print out more details of the widget
	return NS_NOINTERFACE;
  }
}

const nsIID&
nsInputFrame::GetIID()
{
  static NS_DEFINE_IID(kButtonIID, NS_IBUTTON_IID);
  return kButtonIID;
}
  
const nsIID&
nsInputFrame::GetCID()
{
  static NS_DEFINE_IID(kButtonCID, NS_BUTTON_CID);
  return kButtonCID;
}

NS_METHOD nsInputFrame::HandleEvent(nsIPresContext& aPresContext, 
                                    nsGUIEvent* aEvent,
                                    nsEventStatus& aEventStatus)
{
	// make sure that the widget in the event is this
  static NS_DEFINE_IID(kSupportsIID, NS_ISUPPORTS_IID);
  nsIWidget* thisWidget;

  nsIView* view;
  GetView(view);
  if (view != nsnull) {
    nsresult result = GetWidget(view, &thisWidget);
    nsISupports* thisWidgetSup;
    result = thisWidget->QueryInterface(kSupportsIID, (void **)&thisWidgetSup);
    if (thisWidget == nsnull) {
      return nsEventStatus_eIgnore;
    }
    nsISupports* eventWidgetSup;
    result = aEvent->widget->QueryInterface(kSupportsIID, (void **)&eventWidgetSup);

    PRBool isOurEvent = (thisWidgetSup == eventWidgetSup) ? PR_TRUE : PR_FALSE;
    
    NS_RELEASE(eventWidgetSup);
    NS_RELEASE(thisWidgetSup);
    NS_IF_RELEASE(view);
    NS_IF_RELEASE(thisWidget);

	  if (!isOurEvent) {
		  aEventStatus = nsEventStatus_eIgnore;
      return NS_OK;
	  }
  }

  switch (aEvent->message) {
    case NS_MOUSE_ENTER:
	    mLastMouseState = eMouseEnter;
	    break;
    case NS_MOUSE_LEFT_BUTTON_DOWN:
	    mLastMouseState = 
	      (eMouseEnter == mLastMouseState) ? eMouseDown : eMouseNone;
	    break;
    case NS_MOUSE_LEFT_BUTTON_UP:
	    if (eMouseDown == mLastMouseState) {
	      /*nsIView* view = GetView();
	      nsIWidget *widget = view->GetWindow();
		    widget->SetFocus();
		    NS_RELEASE(widget);
	      NS_RELEASE(view); */
        float conv = aPresContext.GetTwipsToPixels();
        ((nsInput*)mContent)->SetClickPoint(NS_TO_INT_ROUND(conv * aEvent->point.x),
                                            NS_TO_INT_ROUND(conv * aEvent->point.y));   
 		    MouseClicked(&aPresContext);
		    //return PR_FALSE;
	    } 
	    mLastMouseState = eMouseEnter;
	    break;
    case NS_MOUSE_EXIT:
	    mLastMouseState = eMouseNone;
	    break;
  }
  return nsEventStatus_eConsumeDoDefault;
}

void nsInputFrame::GetStyleSize(nsIPresContext& aPresContext,
                                const nsSize& aMaxSize, nsSize& aSize)
{
  nsInput* input;
  GetContent((nsIContent *&) input); // this must be an nsInput
  nsStylePosition* pos = (nsStylePosition*) 
    mStyleContext->GetData(kStylePositionSID);

  aSize.width  = GetStyleDim(aPresContext, aMaxSize.width, aMaxSize.width, pos->mWidth);
  aSize.height = GetStyleDim(aPresContext, aMaxSize.height, aMaxSize.width, pos->mHeight);

  NS_RELEASE(input);
}

nscoord 
nsInputFrame::GetStyleDim(nsIPresContext& aPresContext, nscoord aMaxDim, 
                          nscoord aMaxWidth, const nsStyleCoord& aCoord)
{
  nscoord result = 0;
  switch (aCoord.GetUnit()) {
    case eStyleUnit_Coord:
      result = aCoord.GetCoordValue();
      break;
    case eStyleUnit_Inherit:
      result = aMaxDim;  // XXX correct?? this needs to be the inherited value
      break;
    case eStyleUnit_Percent:
      result = (nscoord)((float)aMaxWidth * aCoord.GetPercentValue());
      break;
    default:
    case eStyleUnit_Auto:
      result = CSS_NOTSET;
      break;
  }

  if (result <= 0) {
    result = CSS_NOTSET;
  }

  return result;
}

nscoord 
nsInputFrame::GetTextSize(nsIPresContext& aPresContext, nsIFrame* aFrame,
                          const nsString& aString, nsSize& aSize)
{
  //printf("\n GetTextSize %s", aString.ToNewCString());
  nsIStyleContext* styleContext;
  aFrame->GetStyleContext(&aPresContext, styleContext);
  nsStyleFont* styleFont = (nsStyleFont*)styleContext->GetData(kStyleFontSID);
  NS_RELEASE(styleContext);
  nsIDeviceContext* deviceContext = aPresContext.GetDeviceContext();
  nsIFontCache* fontCache = deviceContext->GetFontCache();

  nsIFontMetrics* fontMet = fontCache->GetMetricsFor(styleFont->mFont);
  aSize.width  = fontMet->GetWidth(aString);
  aSize.height = fontMet->GetHeight() + fontMet->GetLeading();

  nscoord charWidth  = fontMet->GetWidth("W");

  NS_RELEASE(fontMet);
  NS_RELEASE(fontCache);
  NS_RELEASE(deviceContext);

  return charWidth;
}
  
nscoord
nsInputFrame::GetTextSize(nsIPresContext& aPresContext, nsIFrame* aFrame,
                          PRInt32 aNumChars, nsSize& aSize)
{
  nsAutoString val;
  for (int i = 0; i < aNumChars; i++) {
    val += 'e';  // use a typical char, what is the avg width character?
  }
  return GetTextSize(aPresContext, aFrame, val, aSize);
}
  
PRInt32
nsInputFrame::CalculateSize (nsIPresContext* aPresContext, nsInputFrame* aFrame,
                             const nsSize& aCSSSize, nsInputDimensionSpec& aSpec, 
                             nsSize& aBounds, PRBool& aWidthExplicit, 
                             PRBool& aHeightExplicit, nscoord& aRowHeight) 
{
  nscoord charWidth   = 0;
  PRInt32 numRows     = 1;
  aWidthExplicit      = PR_FALSE;
  aHeightExplicit     = PR_FALSE;

  aBounds.width  = CSS_NOTSET;
  aBounds.height = CSS_NOTSET;
  nsSize textSize(0,0);

  nsInput* content;
  aFrame->GetContent((nsIContent*&) content);

  nsAutoString valAttr;
  nsContentAttr valStatus = eContentAttr_NotThere;
  if (nsnull != aSpec.mColValueAttr) {
    valStatus = content->GetAttribute(aSpec.mColValueAttr, valAttr);
  }
  nsHTMLValue colAttr;
  nsContentAttr colStatus = eContentAttr_NotThere;
  if (nsnull != aSpec.mColSizeAttr) {
    colStatus = content->GetAttribute(aSpec.mColSizeAttr, colAttr);
  }

  if (eContentAttr_HasValue == colStatus) {  // col attr will provide width
    if (aSpec.mColSizeAttrInPixels) {
      float p2t = aPresContext->GetPixelsToTwips();
      aBounds.width = (int) (((float)colAttr.GetPixelValue()) * p2t);
    }
    else {
      PRInt32 col = ((colAttr.GetUnit() == eHTMLUnit_Pixel) ? colAttr.GetPixelValue() : colAttr.GetIntValue());
      charWidth = GetTextSize(*aPresContext, aFrame, col, aBounds);
      aRowHeight = aBounds.height;
    }
	  if (aSpec.mColSizeAttrInPixels) {
	    aWidthExplicit = PR_TRUE;
	  }
  }
  else {
    if (CSS_NOTSET != aCSSSize.width) {  // css provides width
      aBounds.width = aCSSSize.width;
	    aWidthExplicit = PR_TRUE;
    } 
    else {                       
      if (eContentAttr_HasValue == valStatus) { // use width of initial value if specified
        charWidth = GetTextSize(*aPresContext, aFrame, valAttr, aBounds);
      }
      else if (aSpec.mColDefaultSizeInPixels) {    // use default width in pixels
        charWidth = GetTextSize(*aPresContext, aFrame, 1, aBounds);
        aBounds.width = aSpec.mColDefaultSize;
      }
      else  {                                    // use default width in num characters
        charWidth = GetTextSize(*aPresContext, aFrame, aSpec.mColDefaultSize, aBounds); 
      }
      aRowHeight = aBounds.height;
    }
  }

  nsHTMLValue rowAttr;
  nsContentAttr rowStatus = eContentAttr_NotThere;
  if (nsnull != aSpec.mRowSizeAttr) {
    rowStatus = content->GetAttribute(aSpec.mRowSizeAttr, rowAttr);
  }

  if (eContentAttr_HasValue == rowStatus) { // row attr will provide height
    PRInt32 rowAttrInt = ((rowAttr.GetUnit() == eHTMLUnit_Pixel) ? rowAttr.GetPixelValue() : rowAttr.GetIntValue());
    if (0 == charWidth) {
      charWidth = GetTextSize(*aPresContext, aFrame, 1, textSize);
      aBounds.height = textSize.height * rowAttrInt;
      aRowHeight = textSize.height;
      numRows = rowAttrInt;
    }
    else {
      aBounds.height = aBounds.height * rowAttrInt;
    }
  }
  else if (CSS_NOTSET != aCSSSize.height) {  // css provides height
    aBounds.height = aCSSSize.height;
	  aHeightExplicit = PR_TRUE;
  } 
  else {         // use default height in num lines
    if (0 == charWidth) {  
      charWidth = GetTextSize(*aPresContext, aFrame, 1, textSize);
      aBounds.height = textSize.height * aSpec.mRowDefaultSize;
      aRowHeight = textSize.height;
    } 
    else {
      aBounds.height = aBounds.height * aSpec.mRowDefaultSize;
    }
  }

  if (0 == charWidth) {
    charWidth = GetTextSize(*aPresContext, aFrame, 1, textSize);
    aRowHeight = textSize.height;
  }

  // add padding to width if width wasn't specified either from css or
  // size attr
  if (!aWidthExplicit) {
    aBounds.width += charWidth;
  }

  NS_RELEASE(content);
  return numRows;
}


nsFont& 
nsInputFrame::GetFont(nsIPresContext* aPresContext)
{
  nsStyleFont* styleFont = (nsStyleFont*)mStyleContext->GetData(kStyleFontSID);

  return styleFont->mFont;
}





