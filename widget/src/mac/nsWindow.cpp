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

#include "nsWindow.h"
#include "nsIFontMetrics.h"
#include "nsIDeviceContext.h"
#include "nsFontMetricsMac.h"

NS_IMPL_ADDREF(ChildWindow)
NS_IMPL_RELEASE(ChildWindow)

ChildWindow::ChildWindow() : nsWindow()
{
	NS_INIT_REFCNT();
  strcpy(gInstanceClassName, "ChildWindow");
}


//-------------------------------------------------------------------------
//
// nsWindow constructor
//
//-------------------------------------------------------------------------
nsWindow::nsWindow() : nsBaseWidget()
{
	strcpy(gInstanceClassName, "nsWindow");

  mParent = nsnull;
  mBounds.SetRect(0,0,0,0);
  mVisible = PR_TRUE;
  mEnabled = PR_TRUE;
	SetPreferredSize(0,0);

	mFontMetrics = nsnull;
	mMenuBar = nsnull;
	mTempRenderingContext = nsnull;

  mWindowRegion = nsnull;
  mWindowPtr = nsnull;
  mPainting = PR_FALSE;
	mDestroyCalled = PR_FALSE;

	SetBackgroundColor(NS_RGB(255, 255, 255));
	SetForegroundColor(NS_RGB(0, 0, 0));
}


//-------------------------------------------------------------------------
//
// nsWindow destructor
//
//-------------------------------------------------------------------------
nsWindow::~nsWindow()
{
	//Destroy();
}


//-------------------------------------------------------------------------
//
// Utility method for implementing both Create(nsIWidget ...) and
// Create(nsNativeWidget...)
//-------------------------------------------------------------------------

nsresult nsWindow::StandardCreate(nsIWidget *aParent,
                      const nsRect &aRect,
                      EVENT_CALLBACK aHandleEventFunction,
                      nsIDeviceContext *aContext,
                      nsIAppShell *aAppShell,
                      nsIToolkit *aToolkit,
                      nsWidgetInitData *aInitData,
                      nsNativeWidget aNativeParent)	// should always be nil here
{
	mParent = aParent;

	mBounds = aRect;
	mWindowRegion = ::NewRgn();
	::SetRectRgn(mWindowRegion, aRect.x, aRect.y, aRect.x + aRect.width, aRect.y + aRect.height);		 

	BaseCreate(aParent, aRect, aHandleEventFunction, 
							aContext, aAppShell, aToolkit, aInitData);


	if (mParent)
	{
		SetBackgroundColor(mParent->GetBackgroundColor());
		SetForegroundColor(mParent->GetForegroundColor());
	}

	if (mWindowPtr == nsnull) {
		if (aParent)
			mWindowPtr = (WindowPtr)aParent->GetNativeData(NS_NATIVE_DISPLAY);
		else if (aAppShell)
			mWindowPtr = (WindowPtr)aAppShell->GetNativeData(NS_NATIVE_SHELL);
	}
	return NS_OK;
}

//-------------------------------------------------------------------------
//
// create a nswindow
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::Create(nsIWidget *aParent,
                      const nsRect &aRect,
                      EVENT_CALLBACK aHandleEventFunction,
                      nsIDeviceContext *aContext,
                      nsIAppShell *aAppShell,
                      nsIToolkit *aToolkit,
                      nsWidgetInitData *aInitData)
{	 
	return(StandardCreate(aParent, aRect, aHandleEventFunction,
													aContext, aAppShell, aToolkit, aInitData,
														nsnull));
}

//-------------------------------------------------------------------------
//
// Creates a main nsWindow using a native widget
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::Create(nsNativeWidget aNativeParent,		// this is a nsWindow*
                      const nsRect &aRect,
                      EVENT_CALLBACK aHandleEventFunction,
                      nsIDeviceContext *aContext,
                      nsIAppShell *aAppShell,
                      nsIToolkit *aToolkit,
                      nsWidgetInitData *aInitData)
{
	// On Mac, a native widget is a nsWindow* because 
	// nsWindow::GetNativeData(NS_NATIVE_WIDGET) returns 'this'
	nsIWidget* aParent = (nsIWidget*)aNativeParent;
	
	return(Create(aParent, aRect, aHandleEventFunction,
									aContext, aAppShell, aToolkit, aInitData));
}

//-------------------------------------------------------------------------
//
// Close this nsWindow
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::Destroy()
{
	if (mDestroyCalled)
		return NS_OK;
	mDestroyCalled = PR_TRUE;

	nsBaseWidget::OnDestroy();
	nsBaseWidget::Destroy();

	ReportDestroyEvent();
  mEventCallback = nsnull;	 // prevent the widget from causing additional events

	if (mWindowRegion != nsnull)
	{
		::DisposeRgn(mWindowRegion);
		mWindowRegion = nsnull;	
	}

	NS_IF_RELEASE(mTempRenderingContext);

	return NS_OK;
}

#pragma mark -
//-------------------------------------------------------------------------
//
// Get this nsWindow parent
//
//-------------------------------------------------------------------------
nsIWidget* nsWindow::GetParent(void)
{
  NS_IF_ADDREF(mParent);
  return  mParent;
}

//-------------------------------------------------------------------------
//
// Return some native data according to aDataType
//
//-------------------------------------------------------------------------
void* nsWindow::GetNativeData(PRUint32 aDataType)
{
		nsPoint		point;
		void*			retVal = nsnull;

  switch (aDataType) 
	{
		case NS_NATIVE_WIDGET:
    case NS_NATIVE_WINDOW:
    	retVal = (void*)this;
    	break;

    case NS_NATIVE_GRAPHIC:
    case NS_NATIVE_DISPLAY:
      retVal = (void*)mWindowPtr;
    	break;

    case NS_NATIVE_REGION:
    	retVal = (void*)mWindowRegion;
    	break;

    case NS_NATIVE_COLORMAP:
    	//�TODO
    	break;

    case NS_NATIVE_OFFSETX:
    	point.MoveTo(mBounds.x, mBounds.y);
    	LocalToWindowCoordinate(point);
    	retVal = (void*)point.x;
    	break;

    case NS_NATIVE_OFFSETY:
    	point.MoveTo(mBounds.x, mBounds.y);
    	LocalToWindowCoordinate(point);
    	retVal = (void*)point.y;
    	break;
	}

  return retVal;
}

#pragma mark -
//-------------------------------------------------------------------------
//
// Return PR_TRUE if the whether the component is visible, PR_FALSE otherwise
//
//-------------------------------------------------------------------------
NS_METHOD nsWindow::IsVisible(PRBool & bState)
{
  bState = mVisible;
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Hide or show this component
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::Show(PRBool bState)
{
  mVisible = bState;
  return NS_OK;
}

    
//-------------------------------------------------------------------------
//
// Enable/disable this component
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::Enable(PRBool bState)
{
	mEnabled = bState;
	return NS_OK;
}

    
//-------------------------------------------------------------------------
//
// Set the focus on this component
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::SetFocus(void)
{
	if (mToolkit)
		((nsToolkit*)mToolkit)->SetFocus(this);
	return NS_OK;
}

//-------------------------------------------------------------------------
//
// Get this component font
//
//-------------------------------------------------------------------------
nsIFontMetrics* nsWindow::GetFont(void)
{
	return mFontMetrics;
}

    
//-------------------------------------------------------------------------
//
// Set this component font
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::SetFont(const nsFont &aFont)
{
	NS_IF_RELEASE(mFontMetrics);
	if (mContext)
		mContext->GetMetricsFor(aFont, mFontMetrics);
 	return NS_OK;
}


//-------------------------------------------------------------------------
//
// Set the colormap of the window
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::SetColorMap(nsColorMap *aColorMap)
{
	//�TODO
	// We may need to move this to nsMacWindow:
	// I'm not sure all the individual widgets
	// can have each their own colorMap on Mac.
	return NS_OK;
}

//-------------------------------------------------------------------------
//
// Set the widget's MenuBar.
// Must be called after Create.
//
// @param aTitle string displayed as the title of the widget
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::SetMenuBar(nsIMenuBar * aMenuBar)
{
	mMenuBar = aMenuBar;
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Get the widget's MenuBar.
//
//-------------------------------------------------------------------------
nsIMenuBar* nsWindow::GetMenuBar()
{
  return mMenuBar;
}

#pragma mark -
//-------------------------------------------------------------------------
//
// Get this component dimension
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::GetBounds(nsRect &aRect)
{
  aRect = mBounds;
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Move this component
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::Move(PRUint32 aX, PRUint32 aY)
{
	if ((mBounds.x != aX) || (mBounds.y != aY))
	{
		mBounds.x = aX;
		mBounds.y = aY;
		ReportMoveEvent();
	}
	return NS_OK;
}

//-------------------------------------------------------------------------
//
// Resize this component
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::Resize(PRUint32 aWidth, PRUint32 aHeight, PRBool aRepaint)
{
	if ((mBounds.width != aWidth) || (mBounds.height != aHeight))
	{
	  mBounds.width  = aWidth;
	  mBounds.height = aHeight;
	 
		if (mWindowRegion)
			::SetRectRgn(mWindowRegion, mBounds.x, mBounds.y, mBounds.x + aWidth, mBounds.y + aHeight);		 
	 
	  if (aRepaint)
	  	Invalidate(true);

		ReportSizeEvent();
	}
	return NS_OK;
}

//-------------------------------------------------------------------------
//
// Resize this component
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::Resize(PRUint32 aX, PRUint32 aY, PRUint32 aWidth, PRUint32 aHeight, PRBool aRepaint)
{
	Move(aX, aY);
	Resize(aWidth, aHeight, aRepaint);
	return NS_OK;
}


NS_METHOD nsWindow::GetPreferredSize(PRInt32& aWidth, PRInt32& aHeight)
{
  aWidth  = mPreferredWidth;
  aHeight = mPreferredHeight;
  return NS_ERROR_FAILURE;
}

NS_METHOD nsWindow::SetPreferredSize(PRInt32 aWidth, PRInt32 aHeight)
{
  mPreferredWidth  = aWidth;
  mPreferredHeight = aHeight;
  return NS_OK;
}

//-------------------------------------------------------------------------
// 
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::BeginResizingChildren(void)
{
	return NS_OK;
}

//-------------------------------------------------------------------------
// 
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::EndResizingChildren(void)
{
	return NS_OK;
}

#pragma mark -
 
//-------------------------------------------------------------------------
//
// Invalidate this component visible area
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::Invalidate(const nsRect &aRect, PRBool aIsSynchronous)
{
	if (!mWindowPtr)
		return NS_OK;

	if (aIsSynchronous && !mPainting)
	{
		mPainting = PR_TRUE;	// no reentrance please
		Update();
		mPainting = PR_FALSE;
	}
	else
	{
		nsRect wRect = aRect;
		Rect macRect;
		LocalToWindowCoordinate(wRect);
		nsRectToMacRect(wRect, macRect);

		::SetPort(mWindowPtr);
		::InvalRect(&macRect);
	}
	return NS_OK;
}
   
//-------------------------------------------------------------------------
//
// Invalidate this component visible area
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::Invalidate(PRBool aIsSynchronous)
{
	nsRect wRect = mBounds;
	if (mParent == nsnull)		// the topLeft corner of the window is in global coordinates
		wRect.x = wRect.y = 0;
	Invalidate(wRect, aIsSynchronous);
	return NS_OK;
}


//-------------------------------------------------------------------------
//	StartDraw
//		Initialize graphic attributes.
//		When they receive a Paint event, XP Widgets rely
//		on these attributes to be set.
//
//-------------------------------------------------------------------------
//�TODO: some of that work is done by the rendering context -> do some cleanup
void nsWindow::StartDraw(nsIRenderingContext* aRenderingContext)
{
	if (aRenderingContext == nsnull)
		mTempRenderingContext = GetRenderingContext();
	else
	{
		NS_IF_ADDREF(aRenderingContext);
		mTempRenderingContext = aRenderingContext;
	}
	if (mTempRenderingContext)
		mTempRenderingContext->PushState();
		
	// set the font
	if (mFontMetrics)
	{
		nsFont* font;
		mFontMetrics->GetFont(font);
		nsFontMetricsMac::SetFont(*font, mContext);
	}

	// set the background & foreground colors
	#define COLOR8TOCOLOR16(color8)	 (color8 == 0xFF ? 0xFFFF : (color8 << 8))
	nscolor color = GetBackgroundColor();
	RGBColor macColor;
	macColor.red   = COLOR8TOCOLOR16(NS_GET_R(color));
	macColor.green = COLOR8TOCOLOR16(NS_GET_G(color));
	macColor.blue  = COLOR8TOCOLOR16(NS_GET_B(color));
	::RGBBackColor(&macColor);

	color = GetForegroundColor();
	macColor.red   = COLOR8TOCOLOR16(NS_GET_R(color));
	macColor.green = COLOR8TOCOLOR16(NS_GET_G(color));
	macColor.blue  = COLOR8TOCOLOR16(NS_GET_B(color));
	::RGBForeColor(&macColor);
}


//-------------------------------------------------------------------------
//	EndDraw
//
//-------------------------------------------------------------------------
void nsWindow::EndDraw()
{
	PRBool clipEmpty;
	mTempRenderingContext->PopState(clipEmpty);
	NS_RELEASE(mTempRenderingContext);
}

//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
PRBool nsWindow::OnPaint(nsPaintEvent &event)
{
	// override this
  return PR_TRUE;
}


//-------------------------------------------------------------------------
//	Update
//		Called by the event handler to redraw the widgets.
//		The window visRgn is expected to be set to whatever needs to be drawn
//		(ie. if we are not between BeginUpdate/EndUpdate, we redraw the whole widget)
//-------------------------------------------------------------------------
NS_IMETHODIMP	nsWindow::Update()
{
	RgnHandle updateRgn = ::NewRgn();
	::SectRgn(mWindowPtr->visRgn, mWindowRegion, updateRgn);
	if (!::EmptyRgn(updateRgn))
	{
		nsIRenderingContext* renderingContext = GetRenderingContext();	// this sets the origin
		if (renderingContext)
		{
			// initialize the paint event for that widget
			nsRect rect;
			Rect macRect;
			GetBounds(rect);					//�TODO�: for complex objects, maybe we should clip to the widgetRgn
			rect.x = rect.y = 0;	// the origin is set on the topLeft corner of the widget
			nsRectToMacRect(rect, macRect);
			::ClipRect(&macRect);			//�TODO? shouldn't this be done in the rendering context?

			// 			nsEvent
			nsPaintEvent paintEvent;
			paintEvent.eventStructType = NS_PAINT_EVENT;
			paintEvent.message		= NS_PAINT;
			paintEvent.point.x		= 0;
			paintEvent.point.y		= 0;
			paintEvent.time				= PR_IntervalNow();

			// 			nsGUIEvent
			paintEvent.widget			= this;
			paintEvent.nativeMsg	= nsnull;

			// 			nsPaintEvent
			paintEvent.renderingContext	= renderingContext;
			paintEvent.rect							= &rect;

			// draw the widget
			StartDraw(renderingContext);
			OnPaint(paintEvent);
			DispatchWindowEvent(paintEvent);
			EndDraw();

			// recursively scan through its children to draw them too
			nsIEnumerator* children = GetChildren();
			if (children)
			{
				children->Reset();
				nsWindow* child = (nsWindow*)children->Next();
				while (child)
				{
					child->Update();
					child = (nsWindow*)children->Next();
				}
				delete children;
			}
			NS_RELEASE(renderingContext);		// this restores the origin to (0, 0)
		}
	}
	::DisposeRgn(updateRgn);
	return NS_OK;
}


//-------------------------------------------------------------------------
//
// Scroll the bits of a window
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::Scroll(PRInt32 aDx, PRInt32 aDy, nsRect *aClipRect)
{
	return NS_OK;
}

//-------------------------------------------------------------------------
//
// Scroll the bits of a window
//
//-------------------------------------------------------------------------

PRBool nsWindow::ConvertStatus(nsEventStatus aStatus)
{
  switch (aStatus)
  {
    case nsEventStatus_eIgnore:							return(PR_FALSE);
    case nsEventStatus_eConsumeNoDefault:		return(PR_TRUE);	// don't do default processing
    case nsEventStatus_eConsumeDoDefault:		return(PR_FALSE);
    default:
      NS_ASSERTION(0, "Illegal nsEventStatus enumeration value");
      break;
  }
  return(PR_FALSE);
}

//-------------------------------------------------------------------------
//
// Invokes callback and  ProcessEvent method on Event Listener object
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::DispatchEvent(nsGUIEvent* event, nsEventStatus& aStatus)
{
	nsIWidget* aWidget = event->widget;
	NS_IF_ADDREF(aWidget);

  aStatus = nsEventStatus_eIgnore;
  if (mEventCallback)
    aStatus = (*mEventCallback)(event);

	// Dispatch to event listener if event was not consumed
  if ((aStatus != nsEventStatus_eIgnore) && (mEventListener != nsnull))
    aStatus = mEventListener->ProcessEvent(*event);

	NS_IF_RELEASE(aWidget);

  return NS_OK;
}

//-------------------------------------------------------------------------
PRBool nsWindow::DispatchWindowEvent(nsGUIEvent &event)
{
  nsEventStatus status;
  DispatchEvent(&event, status);
  return ConvertStatus(status);
}

//-------------------------------------------------------------------------
//
// Deal with all sort of mouse event
//
//-------------------------------------------------------------------------
PRBool nsWindow::DispatchMouseEvent(nsMouseEvent &aEvent)
{

  PRBool result = PR_FALSE;
  if (nsnull == mEventCallback && nsnull == mMouseListener) {
    return result;
  }

  // call the event callback 
  if (nsnull != mEventCallback) 
  	{
    result = (DispatchWindowEvent(aEvent));
    return result;
  	}

  if (nsnull != mMouseListener) {
    switch (aEvent.message) {
      case NS_MOUSE_MOVE: {
        result = ConvertStatus(mMouseListener->MouseMoved(aEvent));
        nsRect rect;
        GetBounds(rect);
        if (rect.Contains(aEvent.point.x, aEvent.point.y)) 
        	{
          //if (mWindowPtr == NULL || mWindowPtr != this) 
          	//{
            printf("Mouse enter");
            //mCurrentWindow = this;
          	//}
        	} 
        else 
        	{
          printf("Mouse exit");
        	}

      } break;

      case NS_MOUSE_LEFT_BUTTON_DOWN:
      case NS_MOUSE_MIDDLE_BUTTON_DOWN:
      case NS_MOUSE_RIGHT_BUTTON_DOWN:
        result = ConvertStatus(mMouseListener->MousePressed(aEvent));
        break;

      case NS_MOUSE_LEFT_BUTTON_UP:
      case NS_MOUSE_MIDDLE_BUTTON_UP:
      case NS_MOUSE_RIGHT_BUTTON_UP:
        result = ConvertStatus(mMouseListener->MouseReleased(aEvent));
        result = ConvertStatus(mMouseListener->MouseClicked(aEvent));
        break;
    } // switch
  } 
  return result;
}

#pragma mark -

//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
PRBool nsWindow::ReportDestroyEvent()
{
	// nsEvent
	nsGUIEvent moveEvent;
	moveEvent.eventStructType = NS_GUI_EVENT;
	moveEvent.message			= NS_DESTROY;
	moveEvent.point.x			= 0;
	moveEvent.point.y			= 0;
	moveEvent.time				= PR_IntervalNow();

	// nsGUIEvent
	moveEvent.widget			= this;
	moveEvent.nativeMsg		= nsnull;

	// dispatch event
	return (DispatchWindowEvent(moveEvent));
}

//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
PRBool nsWindow::ReportMoveEvent()
{
	// nsEvent
	nsGUIEvent moveEvent;
	moveEvent.eventStructType = NS_GUI_EVENT;
	moveEvent.message			= NS_MOVE;
	moveEvent.point.x			= mBounds.x;
	moveEvent.point.y			= mBounds.y;
	moveEvent.time				= PR_IntervalNow();

	// nsGUIEvent
	moveEvent.widget			= this;
	moveEvent.nativeMsg		= nsnull;

	// dispatch event
	return (DispatchWindowEvent(moveEvent));
}

//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
PRBool nsWindow::ReportSizeEvent()
{
	// nsEvent
	nsSizeEvent sizeEvent;
	sizeEvent.eventStructType = NS_SIZE_EVENT;
	sizeEvent.message			= NS_SIZE;
	sizeEvent.point.x			= 0;
	sizeEvent.point.y			= 0;
	sizeEvent.time				= PR_IntervalNow();

	// nsGUIEvent
	sizeEvent.widget			= this;
	sizeEvent.nativeMsg		= nsnull;

	// nsSizeEvent
	sizeEvent.windowSize	= &mBounds;
  
	// dispatch event
	return(DispatchWindowEvent(sizeEvent));
}



#pragma mark -
//-------------------------------------------------------------------------
/*
 *  @update  dc 08/28/98
 *  @param   aTheRegion -- The region to intersect with for this widget
 *  @return  PR_TRUE if the these regions intersect
 */

PRBool
nsWindow::RgnIntersects(RgnHandle aTheRegion,RgnHandle	aIntersectRgn)
{
PRBool			result = PR_FALSE;


	::SectRgn(aTheRegion,this->mWindowRegion,aIntersectRgn);
	if (!::EmptyRgn(aIntersectRgn))
		result = TRUE;
	return(result);
}



//-------------------------------------------------------------------------
/*  Calculate the x and y offsets for this particular widget
 *  @update  ps 09/22/98
 *  @param   aX -- x offset amount
 *  @param   aY -- y offset amount 
 *  @return  NOTHING
 */
NS_IMETHODIMP nsWindow::CalcOffset(PRInt32 &aX,PRInt32 &aY)
{
nsIWidget	*theparent,*child;
nsRect		therect;

	aX = 0;
	aY = 0;
	theparent = this->GetParent();
	while (theparent)
	{
		theparent->GetBounds(therect);
		child = theparent->GetParent();
		if (child)
		{
			aX += therect.x;
			aY += therect.y;
		}
		NS_IF_RELEASE(theparent);
		theparent = child;
	}
	return NS_OK;
}

//-------------------------------------------------------------------------
/*  Calculate the x and y offsets for this particular widget
 *  @update  ps 09/22/98
 *  @param   aX -- x offset amount
 *  @param   aY -- y offset amount 
 *  @return  NOTHING
 */
NS_IMETHODIMP nsWindow::CalcTotalOffset(PRInt32 &aX,PRInt32 &aY)
{
nsIWidget	*theparent,*child;
nsRect		therect;

	aX = mBounds.x;
	aY = mBounds.y;
	theparent = this->GetParent();
	while (theparent)
	{
		theparent->GetBounds(therect);
		child = theparent->GetParent();
		if (child)
		{
			aX += therect.x;
			aY += therect.y;
		}
		NS_IF_RELEASE(theparent);
		theparent = child;
	}

	return NS_OK;
}


//-------------------------------------------------------------------------
// PointInWidget
//		Find if a point in local coordinates is inside this object
//-------------------------------------------------------------------------
PRBool nsWindow::PointInWidget(Point aThePoint)
{
	// get the origin in local coordinates
	nsPoint widgetOrigin(0, 0);
	ConvertToDeviceCoordinates(widgetOrigin.x, widgetOrigin.y);

	// get rectangle relatively to the parent
	nsRect widgetRect;
	GetBounds(widgetRect);

	// convert the topLeft corner to local coordinates
	widgetRect.MoveBy(widgetOrigin.x, widgetOrigin.y);

	// finally tell whether it's a hit
	return(widgetRect.Contains(aThePoint.h, aThePoint.v));
}


//-------------------------------------------------------------------------
// FindWidgetHit
//		Recursively look for the widget hit
//		@param aParent   -- parent widget. 
//		@param aThePoint -- a point in local coordinates to test for the hit. 
//-------------------------------------------------------------------------
nsWindow*  nsWindow::FindWidgetHit(Point aThePoint)
{
		nsWindow*	widgetHit;

	if (PointInWidget(aThePoint))
	{
		widgetHit = this;
		nsIEnumerator* children = GetChildren();
		if (children)
		{
			children->Reset();
			nsWindow* child = (nsWindow*)children->Next();
			while (child)
			{
				nsWindow* deeperHit = child->FindWidgetHit(aThePoint);
				if (deeperHit)
				{
					widgetHit = deeperHit;
					break;
				}
				else
					child = (nsWindow*)children->Next();
			}
			delete children;
		}
	}
	else
		widgetHit = nsnull;

	return widgetHit;
}

#pragma mark -
#pragma mark - will be sorted -
//-------------------------------------------------------------------------
//
// 
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::WidgetToScreen(const nsRect& aOldRect, nsRect& aNewRect)
{
	return NS_OK;
}

//-------------------------------------------------------------------------
//
// 
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::ScreenToWidget(const nsRect& aOldRect, nsRect& aNewRect)
{
	return NS_OK;
} 


/*
 *  Set a Mac Rect to the value of an nsRect 
 *  The source rect is assumed to be in pixels not TWIPS
 *  @update  gpk 08/27/98
 *  @param   aRect -- The nsRect that is the source
 *  @param   aMacRect -- The Mac Rect destination
 */
void nsWindow::nsRectToMacRect(const nsRect& aRect, Rect& aMacRect) const
{
		aMacRect.left = aRect.x;
		aMacRect.top = aRect.y;
		aMacRect.right = aRect.x + aRect.width;
		aMacRect.bottom = aRect.y + aRect.height;
}


//=================================================================
/*  Convert the coordinates to some device coordinates so GFX can draw.
 *  @update  dc 09/16/98
 *  @param   nscoord -- X coordinate to convert
 *  @param   nscoord -- Y coordinate to convert
 *  @return  NONE
 */
void  nsWindow::ConvertToDeviceCoordinates(nscoord &aX,nscoord &aY)
{
PRInt32	offX,offY;
        
  this->CalcOffset(offX,offY);
	
	aX -=offX;
	aY -=offY;
	
}

/*
 * Convert an nsPoint into mac local coordinates.
 * The tree hierarchy is navigated upwards, changing
 * the x,y offset by the parent's coordinates
 *
 */
void nsWindow::LocalToWindowCoordinate(nsPoint& aPoint)
{
	nsIWidget* 	parent = GetParent();
	nsIWidget* 	grandParent;
  nsRect 			bounds;
  
	while (parent)
	{
		parent->GetBounds(bounds);
		grandParent = parent->GetParent();
		NS_IF_RELEASE(parent);
		parent = grandParent;
		if (parent)		// don't add the topLeft corner of the window: it's in global coordinates
		{
			aPoint.x += bounds.x;
			aPoint.y += bounds.y;
		}
	}
}

/* 
 * Convert widget local coordinates into mac local coordinates
 */
void nsWindow::LocalToWindowCoordinate(nscoord& aX, nscoord& aY)
{
	nsIWidget* 	parent = GetParent();
	nsIWidget* 	grandParent;
  nsRect 			bounds;
  
	while (parent)
	{
		parent->GetBounds(bounds);
		grandParent = parent->GetParent();
		NS_IF_RELEASE(parent);
		parent = grandParent;
		if (parent)		// don't add the topLeft corner of the window: it's in global coordinates
		{
			aX += bounds.x;
			aY += bounds.y;
		}
	}
}

/* 
 * Convert an nsRect into mac local coordinates
 */
void nsWindow::LocalToWindowCoordinate(nsRect& aRect)
{
	nsIWidget* 	parent = GetParent();
	nsIWidget* 	grandParent;
  nsRect 			bounds;
  
	while (parent)
	{
		parent->GetBounds(bounds);
		grandParent = parent->GetParent();
		NS_IF_RELEASE(parent);
		parent = grandParent;
		if (parent)		// don't add the topLeft corner of the window: it's in global coordinates
		{
			aRect.x += bounds.x;
			aRect.y += bounds.y;
		}
	}
}


#pragma mark -
#pragma mark - will be gone -
/* 
 * Convert a nsString to a PascalStr255
 */
void nsWindow::StringToStr255(const nsString& aText, Str255& aStr255)
{
  char buffer[256];
	
	aText.ToCString(buffer,255);
		
	PRInt32 len = strlen(buffer);
	memcpy(&aStr255[1],buffer,len);
	aStr255[0] = len;
	
		
}


/* 
 * Convert a nsString to a PascalStr255
 */
void nsWindow::Str255ToString(const Str255& aStr255, nsString& aText)
{
  char 		buffer[256];
	PRInt32 len = aStr255[0];
  
	memcpy(buffer,&aStr255[1],len);
	buffer[len] = 0;

	aText = buffer;		
}
