/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
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
 *     Jerry.Kirk@Nexwarecorp.com
 *     Michael.Kedl@Nexwarecorp.com
 *	   Dale.Stansberry@Nexwarecorop.com
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
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */


#include "nsWidget.h"

#include "nsIAppShell.h"
#include "nsIComponentManager.h"
#include "nsIDeviceContext.h"
#include "nsIFontMetrics.h"
#include "nsILookAndFeel.h"
#include "nsToolkit.h"
#include "nsWidgetsCID.h"
#include "nsGfxCIID.h"
#include <Pt.h>
#include "PtRawDrawContainer.h"
#include "nsIRollupListener.h"
#include "nsIServiceManager.h"
#include "nsWindow.h"
#include "nsDragService.h"
#include "nsReadableUtils.h"

#include "nsIPref.h"
#include "nsPhWidgetLog.h"

#include <errno.h>
#include <photon/PtServer.h>

static NS_DEFINE_CID(kLookAndFeelCID, NS_LOOKANDFEEL_CID);

// BGR, not RGB - REVISIT
#define NSCOLOR_TO_PHCOLOR(g,n) \
  g.red=NS_GET_B(n); \
  g.green=NS_GET_G(n); \
  g.blue=NS_GET_R(n);

////////////////////////////////////////////////////////////////////
//
// Define and Initialize global variables
//
////////////////////////////////////////////////////////////////////


//
// Keep track of the last widget being "dragged"
//
nsILookAndFeel     *nsWidget::sLookAndFeel = nsnull;
nsIDragService     *nsWidget::sDragService = nsnull;
PRUint32            nsWidget::sWidgetCount = 0;
PRBool              nsWidget::sJustGotActivated = PR_FALSE;
PRBool              nsWidget::sJustGotDeactivated = PR_FALSE;


nsWidget::nsWidget()
{
  // XXX Shouldn't this be done in nsBaseWidget?
	// NS_INIT_ISUPPORTS();

  if (!sLookAndFeel) {
    if (NS_OK != nsComponentManager::CreateInstance(kLookAndFeelCID,
                                                    nsnull,
                                                    NS_GET_IID(nsILookAndFeel),
                                                    (void**)&sLookAndFeel))
		sLookAndFeel = nsnull;
  	}

  if( sLookAndFeel )
    sLookAndFeel->GetColor( nsILookAndFeel::eColor_WindowBackground, mBackground );

	if( !sDragService ) {
		nsresult rv;
		nsCOMPtr<nsIDragService> s;
		s = do_GetService( "@mozilla.org/widget/dragservice;1", &rv );
		sDragService = ( nsIDragService * ) s;
		if( NS_FAILED( rv ) ) sDragService = 0;
		}

  mWidget = nsnull;
  mParent = nsnull;
  mPreferredWidth  = 0;
  mPreferredHeight = 0;
  mShown = PR_FALSE;
  mBounds.x = 0;
  mBounds.y = 0;
  mBounds.width = 0;
  mBounds.height = 0;
  mIsDestroying = PR_FALSE;
  mOnDestroyCalled = PR_FALSE;
  mIsToplevel = PR_FALSE;
  mListenForResizes = PR_FALSE;
  sWidgetCount++;
}


nsWidget::~nsWidget( ) {
  // it's safe to always call Destroy() because it will only allow itself to be called once
  Destroy();

  if( !sWidgetCount-- ) {
    NS_IF_RELEASE( sLookAndFeel );
  	}
	}

//-------------------------------------------------------------------------
//
// nsISupport stuff
//
//-------------------------------------------------------------------------
NS_IMPL_ISUPPORTS_INHERITED1(nsWidget, nsBaseWidget, nsIKBStateControl)

NS_METHOD nsWidget::WidgetToScreen( const nsRect& aOldRect, nsRect& aNewRect ) {
  if( mWidget ) {
    /* This is NOT correct */
    aNewRect.x = aOldRect.x;
    aNewRect.y = aOldRect.y;
  	}
  return NS_OK;
	}

// nsWidget::ScreenToWidget - Not Implemented
NS_METHOD nsWidget::ScreenToWidget( const nsRect& aOldRect, nsRect& aNewRect ) {
  return NS_OK;
	}

//-------------------------------------------------------------------------
//
// Close this nsWidget
//
//-------------------------------------------------------------------------

NS_IMETHODIMP nsWidget::Destroy( void ) {

 // make sure we don't call this more than once.
  if( mIsDestroying ) return NS_OK;

  // ok, set our state
  mIsDestroying = PR_TRUE;

  // call in and clean up any of our base widget resources
  // are released
  nsBaseWidget::Destroy();

  // destroy our native windows
  DestroyNative();

  // make sure to call the OnDestroy if it hasn't been called yet
  if( mOnDestroyCalled == PR_FALSE ) OnDestroy();

  // make sure no callbacks happen
  mEventCallback = nsnull;

  return NS_OK;
	}

// this is the function that will destroy the native windows for this widget.

/* virtual */
void nsWidget::DestroyNative( void ) {
  if( mWidget ) {
    // prevent the widget from causing additional events
    mEventCallback = nsnull;
	  EnableDamage( mWidget, PR_FALSE );
	  PtDestroyWidget( mWidget );
	  EnableDamage( mWidget, PR_TRUE );
    mWidget = nsnull;
  	}
	}

// make sure that we clean up here

void nsWidget::OnDestroy( ) {
  mOnDestroyCalled = PR_TRUE;
  // release references to children, device context, toolkit + app shell
  nsBaseWidget::OnDestroy();
  DispatchStandardEvent(NS_DESTROY);
	}

//////////////////////////////////////////////////////////////////////
//
// nsIKBStateControl Mehthods
//
//////////////////////////////////////////////////////////////////////

NS_IMETHODIMP nsWidget::ResetInputState( ) {
  return NS_OK;
	}

// to be implemented
NS_IMETHODIMP nsWidget::PasswordFieldInit( ) {
  return NS_OK;
	}

//-------------------------------------------------------------------------
//
// Hide or show this component
//
//-------------------------------------------------------------------------

NS_METHOD nsWidget::Show( PRBool bState ) {

  if( !mWidget ) return NS_OK; // Will be null durring printing

  PtArg_t   arg;

  if( bState ) {

		if( mWindowType != eWindowType_child ) {

		  if (PtWidgetIsRealized(mWidget)) {
			  mShown = PR_TRUE; 
			  return NS_OK;
		  	}

		  EnableDamage( mWidget, PR_FALSE );
		  PtRealizeWidget(mWidget);

		  if( mWidget->rid == -1 ) {
			  EnableDamage( mWidget, PR_TRUE );
			  NS_ASSERTION(0,"nsWidget::Show mWidget's rid == -1\n");
			  mShown = PR_FALSE; 
			  return NS_ERROR_FAILURE;
		  	}

		  PtSetArg(&arg, Pt_ARG_FLAGS, 0, Pt_DELAY_REALIZE);
		  PtSetResources(mWidget, 1, &arg);
		  EnableDamage( mWidget, PR_TRUE );
		  PtDamageWidget(mWidget);
#ifdef Ph_REGION_NOTIFY			
		  PhRegion_t region;
		  PtWidget_t *mWgt;
		  mWgt = (PtWidget_t*) GetNativeData( NS_NATIVE_WIDGET );
		  region.flags = Ph_REGION_NOTIFY | Ph_FORCE_BOUNDARY;
		  region.rid = PtWidgetRid(mWgt);
		  PhRegionChange(Ph_REGION_FLAGS, 0, &region, NULL, NULL);
#endif
		}
		else {
			PtWidgetToFront( mWidget );
			if( !mShown || !( mWidget->flags & Pt_REALIZED ) ) PtRealizeWidget( mWidget );
			}
  	}
  else {
		if( mWindowType != eWindowType_child ) {
      EnableDamage( mWidget, PR_FALSE );
      PtUnrealizeWidget(mWidget);

      EnableDamage( mWidget, PR_TRUE );

      PtSetArg(&arg, Pt_ARG_FLAGS, Pt_DELAY_REALIZE, Pt_DELAY_REALIZE);
      PtSetResources(mWidget, 1, &arg);
			}
		else {
			EnableDamage( mWidget, PR_FALSE );
			PtWidgetToBack( mWidget );
			if( mShown ) PtUnrealizeWidget( mWidget );
			EnableDamage( mWidget, PR_TRUE );
			}
  	}

  mShown = bState;
  return NS_OK;
	}


// nsWidget::SetModal - Not Implemented
NS_IMETHODIMP nsWidget::SetModal( PRBool aModal ) {
  return NS_ERROR_FAILURE;
	}

//-------------------------------------------------------------------------
//
// Move this component
//
//-------------------------------------------------------------------------
NS_METHOD nsWidget::Move( PRInt32 aX, PRInt32 aY ) {

	if( ( mBounds.x == aX ) && ( mBounds.y == aY ) ) return NS_OK; 

	mBounds.x = aX;
	mBounds.y = aY;
	
	if( mWidget ) {
		if(( mWidget->area.pos.x != aX ) || ( mWidget->area.pos.y != aY )) {
			PhPoint_t pos = { aX, aY };
			PtSetResource( mWidget, Pt_ARG_POS, &pos, 0 );
		}
	}
  return NS_OK;
}


NS_METHOD nsWidget::Resize( PRInt32 aWidth, PRInt32 aHeight, PRBool aRepaint ) {

	if( ( mBounds.width == aWidth ) && ( mBounds.height == aHeight ) ) 
	   return NS_OK;

  mBounds.width  = aWidth;
  mBounds.height = aHeight;

  if( mWidget ) {
		PhDim_t dim = { aWidth, aHeight };
		EnableDamage( mWidget, PR_FALSE );
		PtSetResource( mWidget, Pt_ARG_DIM, &dim, 0 );
		EnableDamage( mWidget, PR_TRUE );
		}

	return NS_OK;
}


//-------------------------------------------------------------------------
//
// Send a resize message to the listener
//
//-------------------------------------------------------------------------
PRBool nsWidget::OnResize( nsRect &aRect ) {

  PRBool result = PR_FALSE;

  // call the event callback
  if( mEventCallback ) {
		nsSizeEvent event;

	  InitEvent(event, NS_SIZE);

		nsRect *foo = new nsRect(0, 0, aRect.width, aRect.height);
		event.windowSize = foo;

		event.point.x = 0;
		event.point.y = 0;
		event.mWinWidth = aRect.width;
		event.mWinHeight = aRect.height;
  
		NS_ADDREF_THIS();
		result = DispatchWindowEvent(&event);
		NS_RELEASE_THIS();
		delete foo;
		}

	return result;
	}

//------
// Move
//------
PRBool nsWidget::OnMove( PRInt32 aX, PRInt32 aY ) {
  nsGUIEvent event;
  InitEvent(event, NS_MOVE);
  event.point.x = aX;
  event.point.y = aY;
  return DispatchWindowEvent(&event);
	}


//-------------------------------------------------------------------------
//
// Set this component font
//
//-------------------------------------------------------------------------
NS_METHOD nsWidget::SetFont( const nsFont &aFont ) {

  nsIFontMetrics* mFontMetrics;
  mContext->GetMetricsFor(aFont, mFontMetrics);

	if( mFontMetrics ) {
		PtArg_t arg;

		nsFontHandle aFontHandle;
		mFontMetrics->GetFontHandle(aFontHandle);
		nsString *aString;
		aString = (nsString *) aFontHandle;
		char *str = ToNewCString(*aString);

		PtSetArg( &arg, Pt_ARG_TEXT_FONT, str, 0 );
		PtSetResources( mWidget, 1, &arg );

		delete [] str;
		NS_RELEASE(mFontMetrics);
		}
	return NS_OK;
	}

//-------------------------------------------------------------------------
//
// Set this component cursor
//
//-------------------------------------------------------------------------
NS_METHOD nsWidget::SetCursor( nsCursor aCursor ) {

  // Only change cursor if it's changing
  if( aCursor != mCursor ) {

  	unsigned short curs = Ph_CURSOR_POINTER;
  	PgColor_t color = Ph_CURSOR_DEFAULT_COLOR;

    switch( aCursor ) {
			case eCursor_sizeNW:
				curs = Ph_CURSOR_DRAG_TL;
				break;
			case eCursor_sizeSE:
				curs = Ph_CURSOR_DRAG_BR;
				break;
			case eCursor_sizeNE:
				curs = Ph_CURSOR_DRAG_TL;
				break;
			case eCursor_sizeSW:
				curs = Ph_CURSOR_DRAG_BL;
				break;

			case eCursor_crosshair:
				curs = Ph_CURSOR_CROSSHAIR;
				break;

			case eCursor_copy:
			case eCursor_alias:
			case eCursor_context_menu:
				break;

			case eCursor_cell:
				break;

			case eCursor_spinning:
				break;

			case eCursor_count_up:
			case eCursor_count_down:
			case eCursor_count_up_down:
				break;

  		case eCursor_move:
  		  curs = Ph_CURSOR_MOVE;
  		  break;
      
  		case eCursor_help:
  		  curs = Ph_CURSOR_QUESTION_POINT;
  		  break;
      
  		case eCursor_grab:
  		case eCursor_grabbing:
  		  curs = Ph_CURSOR_FINGER;
  		  break;
      
  		case eCursor_select:
  		  curs = Ph_CURSOR_INSERT;
				color = Pg_BLACK;
  		  break;
      
  		case eCursor_wait:
  		  curs = Ph_CURSOR_LONG_WAIT;
  		  break;

  		case eCursor_hyperlink:
  		  curs = Ph_CURSOR_FINGER;
  		  break;

  		case eCursor_standard:
  		  curs = Ph_CURSOR_POINTER;
  		  break;

  		case eCursor_sizeWE:
  		  curs = Ph_CURSOR_DRAG_HORIZONTAL;
  		  break;

  		case eCursor_sizeNS:
  		  curs = Ph_CURSOR_DRAG_VERTICAL;
  		  break;

  		// REVISIT - Photon does not have the following cursor types...
  		case eCursor_arrow_north:
  		case eCursor_arrow_north_plus:
  		  curs = Ph_CURSOR_DRAG_TOP;
  		  break;

  		case eCursor_arrow_south:
  		case eCursor_arrow_south_plus:
  		  curs = Ph_CURSOR_DRAG_BOTTOM;
  		  break;

  		case eCursor_arrow_east:
  		case eCursor_arrow_east_plus:
  		  curs = Ph_CURSOR_DRAG_RIGHT;
  		  break;

  		case eCursor_arrow_west:
  		case eCursor_arrow_west_plus:
  		  curs = Ph_CURSOR_DRAG_LEFT;
  		  break;

  		case eCursor_zoom_in:
  		case eCursor_zoom_out:
  		  break;

  		default:
  		  NS_ASSERTION(0, "Invalid cursor type");
  		  break;
  		}

  	if( mWidget ) {
  	  PtArg_t args[2];

			PtSetArg( &args[0], Pt_ARG_CURSOR_TYPE, curs, 0 );
			PtSetArg( &args[1], Pt_ARG_CURSOR_COLOR, color, 0 );
			PtSetResources( mWidget, 2, args );
  		}
		mCursor = aCursor;
		}

	return NS_OK;
	}


NS_METHOD nsWidget::Invalidate( PRBool aIsSynchronous ) {

	// mWidget will be null during printing
	if( !mWidget || !PtWidgetIsRealized( mWidget ) ) return NS_OK;

	PtWidget_t *aWidget = (PtWidget_t *)GetNativeData(NS_NATIVE_WIDGET);
	PtDamageWidget( aWidget );
	if( aIsSynchronous ) PtFlush();
	return NS_OK;
	}

NS_METHOD nsWidget::Invalidate( const nsRect & aRect, PRBool aIsSynchronous ) {

  if( !mWidget ) return NS_OK; // mWidget will be null during printing
  if( !PtWidgetIsRealized( mWidget ) ) return NS_OK;

	PhRect_t prect;
	prect.ul.x = aRect.x;
	prect.ul.y = aRect.y;
	prect.lr.x = prect.ul.x + aRect.width - 1;
	prect.lr.y = prect.ul.y + aRect.height - 1;
	if( ! ( mWidget->class_rec->flags & Pt_DISJOINT ) )
		PhTranslateRect( &prect, &mWidget->area.pos );
	PtDamageExtent( mWidget, &prect );
	if( aIsSynchronous ) PtFlush( );
	return NS_OK;
	}

NS_IMETHODIMP nsWidget::InvalidateRegion( const nsIRegion *aRegion, PRBool aIsSynchronous ) {
	PhTile_t *tiles = NULL;
	aRegion->GetNativeRegion( ( void*& ) tiles );
	if( tiles ) {
		PhTranslateTiles( tiles, &mWidget->area.pos );
		PtDamageTiles( mWidget, tiles );
		if( aIsSynchronous ) PtFlush( );
		}
  return NS_OK;
	}

nsresult nsWidget::CreateWidget(nsIWidget *aParent,
                                const nsRect &aRect,
                                EVENT_CALLBACK aHandleEventFunction,
                                nsIDeviceContext *aContext,
                                nsIAppShell *aAppShell,
                                nsIToolkit *aToolkit,
                                nsWidgetInitData *aInitData,
                                nsNativeWidget aNativeParent)
{

  PtWidget_t *parentWidget = nsnull;

  nsIWidget *baseParent = aInitData &&
                            (aInitData->mWindowType == eWindowType_dialog ||
                             aInitData->mWindowType == eWindowType_toplevel ||
                             aInitData->mWindowType == eWindowType_invisible) ?
                          nsnull : aParent;

  BaseCreate( baseParent, aRect, aHandleEventFunction, aContext, aAppShell, aToolkit, aInitData );

  mParent = aParent;

  if( aNativeParent ) {
    parentWidget = (PtWidget_t*)aNativeParent;
    // we've got a native parent so listen for resizes
    mListenForResizes = PR_TRUE;
  	}
  else if( aParent ) {
    parentWidget = (PtWidget_t*) (aParent->GetNativeData(NS_NATIVE_WIDGET));
  	}

  mBounds = aRect;

  CreateNative (parentWidget);

	if( aRect.width > 1 && aRect.height > 1 ) Resize(aRect.width, aRect.height, PR_FALSE);

  if( mWidget ) {
    SetInstance(mWidget, this);
    PtAddCallback( mWidget, Pt_CB_GOT_FOCUS, GotFocusCallback, this );
    PtAddCallback( mWidget, Pt_CB_LOST_FOCUS, LostFocusCallback, this );
    PtAddCallback( mWidget, Pt_CB_IS_DESTROYED, DestroyedCallback, this );
//    PtAddCallback( mWidget, Pt_CB_DND, DndCallback, this );
  	}

  DispatchStandardEvent(NS_CREATE);

  return NS_OK;
	}


//-------------------------------------------------------------------------
//
// Invokes callback and ProcessEvent method on Event Listener object
//
//-------------------------------------------------------------------------

NS_IMETHODIMP nsWidget::DispatchEvent( nsGUIEvent *aEvent, nsEventStatus &aStatus ) {

  NS_ADDREF(aEvent->widget);

  if( nsnull != mMenuListener ) {
    if( NS_MENU_EVENT == aEvent->eventStructType )
      aStatus = mMenuListener->MenuSelected(NS_STATIC_CAST(nsMenuEvent&, *aEvent));
  	}

  aStatus = nsEventStatus_eIgnore;

///* ATENTIE */ printf( "mEventCallback call (%d %d) this=%p\n", aEvent->point.x, aEvent->point.y, this );

  if( nsnull != mEventCallback ) aStatus = (*mEventCallback)(aEvent);

  // Dispatch to event listener if event was not consumed
  if( (aStatus != nsEventStatus_eIgnore) && (nsnull != mEventListener) )
    aStatus = mEventListener->ProcessEvent(*aEvent);

   NS_IF_RELEASE(aEvent->widget);

  return NS_OK;
	}

//==============================================================
void nsWidget::InitMouseEvent(PhPointerEvent_t *aPhButtonEvent,
                              nsWidget * aWidget,
                              nsMouseEvent &anEvent,
                              PRUint32   aEventType)
{
  anEvent.message = aEventType;
  anEvent.widget  = aWidget;

  if (aPhButtonEvent != nsnull) {
    anEvent.time =      PR_IntervalNow();
    anEvent.isShift =   ( aPhButtonEvent->key_mods & Pk_KM_Shift ) ? PR_TRUE : PR_FALSE;
    anEvent.isControl = ( aPhButtonEvent->key_mods & Pk_KM_Ctrl )  ? PR_TRUE : PR_FALSE;
    anEvent.isAlt =     ( aPhButtonEvent->key_mods & Pk_KM_Alt )   ? PR_TRUE : PR_FALSE;
		anEvent.isMeta =		PR_FALSE;
    anEvent.point.x =   aPhButtonEvent->pos.x; 
    anEvent.point.y =   aPhButtonEvent->pos.y;
    anEvent.clickCount = aPhButtonEvent->click_count;
  	}
	}

//-------------------------------------------------------------------------
//
// Deal with all sort of mouse event
//
//-------------------------------------------------------------------------
PRBool nsWidget::DispatchMouseEvent( nsMouseEvent& aEvent ) {

  PRBool result = PR_FALSE;
  if (nsnull == mEventCallback && nsnull == mMouseListener) return result;

  // call the event callback
  if (nsnull != mEventCallback) {
    result = DispatchWindowEvent(&aEvent);
    return result;
  	}

  if (nsnull != mMouseListener) {

    switch (aEvent.message) {
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

    	case NS_DRAGDROP_DROP:
    	  break;

			case NS_MOUSE_MOVE:
    	  	break;

    	default:
    	  break;

    	} // switch
  	}
  return result;
	}

//-------------------------------------------------------------------------
// Old icky code I am trying to replace!
//-------------------------------------------------------------------------
PRBool nsWidget::DispatchMouseEvent( PhPoint_t &aPos, PRUint32 aEvent ) {

  PRBool result = PR_FALSE;

  if( nsnull == mEventCallback && nsnull == mMouseListener ) return result;

  nsMouseEvent event;

  InitEvent( event, aEvent );
  event.point.x = aPos.x;
  event.point.y = aPos.y;
  event.isShift = PR_FALSE;
  event.isControl = PR_FALSE;
  event.isAlt = PR_FALSE;
  event.isMeta = PR_FALSE;
  event.clickCount = 0;       /* hack  makes the mouse not work */
  
  // call the event callback
  if( nsnull != mEventCallback ) {
    result = DispatchWindowEvent( &event );
    NS_IF_RELEASE(event.widget);
    return result;
  	}

  if( nsnull != mMouseListener ) { 
    switch( aEvent ) {
      case NS_MOUSE_MOVE:
        result = ConvertStatus(mMouseListener->MouseMoved(event));
        break;

      case NS_MOUSE_LEFT_BUTTON_DOWN:
      case NS_MOUSE_MIDDLE_BUTTON_DOWN:
      case NS_MOUSE_RIGHT_BUTTON_DOWN:
        result = ConvertStatus(mMouseListener->MousePressed(event));
        break;

      case NS_MOUSE_LEFT_BUTTON_UP:
      case NS_MOUSE_MIDDLE_BUTTON_UP:
      case NS_MOUSE_RIGHT_BUTTON_UP:
        result = ConvertStatus(mMouseListener->MouseReleased(event));
        result = ConvertStatus(mMouseListener->MouseClicked(event));
        break;
    	} // switch
  	}
  return result;
	}

struct nsKeyConverter {
  PRUint32       vkCode; // Platform independent key code
  unsigned long  keysym; // Photon key_sym key code
  PRBool         isChar;
};

static struct nsKeyConverter nsKeycodes[] = {
  { NS_VK_CANCEL,     Pk_Cancel, PR_FALSE },
  { NS_VK_BACK,       Pk_BackSpace, PR_FALSE },
  { NS_VK_TAB,        Pk_Tab, PR_FALSE },
  { NS_VK_TAB,        Pk_KP_Tab, PR_FALSE },
  { NS_VK_CLEAR,      Pk_Clear, PR_FALSE },
  { NS_VK_RETURN,     Pk_Return, PR_FALSE },
  { NS_VK_SHIFT,      Pk_Shift_L, PR_FALSE },
  { NS_VK_SHIFT,      Pk_Shift_R, PR_FALSE },
  { NS_VK_SHIFT,      Pk_Shift_L, PR_FALSE },
  { NS_VK_SHIFT,      Pk_Shift_R, PR_FALSE },
  { NS_VK_CONTROL,    Pk_Control_L, PR_FALSE },
  { NS_VK_CONTROL,    Pk_Control_R, PR_FALSE },
  { NS_VK_ALT,        Pk_Alt_L, PR_FALSE },
  { NS_VK_ALT,        Pk_Alt_R, PR_FALSE },
  { NS_VK_PAUSE,      Pk_Pause, PR_FALSE },
  { NS_VK_CAPS_LOCK,  Pk_Caps_Lock, PR_FALSE },
  { NS_VK_ESCAPE,     Pk_Escape, PR_FALSE },
  { NS_VK_SPACE,      Pk_space, PR_TRUE },
  { NS_VK_PAGE_UP,    Pk_Pg_Up, PR_FALSE },
  { NS_VK_PAGE_DOWN,  Pk_Pg_Down, PR_FALSE },
  { NS_VK_END,        Pk_End, PR_FALSE },
  { NS_VK_HOME,       Pk_Home, PR_FALSE },
  { NS_VK_LEFT,       Pk_Left, PR_FALSE },
  { NS_VK_UP,         Pk_Up, PR_FALSE },
  { NS_VK_RIGHT,      Pk_Right, PR_FALSE },
  { NS_VK_DOWN,       Pk_Down, PR_FALSE },
  { NS_VK_PRINTSCREEN, Pk_Print, PR_FALSE },
  { NS_VK_INSERT,     Pk_Insert, PR_FALSE },
  { NS_VK_DELETE,     Pk_Delete, PR_FALSE },
  { NS_VK_COMMA,      Pk_comma, PR_TRUE },
  { NS_VK_PERIOD,     Pk_period, PR_TRUE },
  { NS_VK_SLASH,      Pk_slash, PR_TRUE },
  { NS_VK_OPEN_BRACKET,  Pk_bracketleft, PR_TRUE },
  { NS_VK_CLOSE_BRACKET, Pk_bracketright, PR_TRUE },
  { NS_VK_QUOTE,         Pk_quotedbl, PR_TRUE },
  { NS_VK_MULTIPLY,      Pk_KP_Multiply, PR_TRUE },
  { NS_VK_ADD,           Pk_KP_Add, PR_TRUE },
  { NS_VK_COMMA,         Pk_KP_Separator, PR_FALSE },
  { NS_VK_SUBTRACT,      Pk_KP_Subtract, PR_TRUE },
  { NS_VK_PERIOD,        Pk_KP_Decimal, PR_TRUE },
  { NS_VK_DIVIDE,        Pk_KP_Divide, PR_TRUE },
  { NS_VK_RETURN,        Pk_KP_Enter, PR_FALSE },
  { NS_VK_INSERT,        Pk_KP_0, PR_FALSE },
  { NS_VK_END,           Pk_KP_1, PR_FALSE },
  { NS_VK_DOWN,          Pk_KP_2, PR_FALSE },
  { NS_VK_PAGE_DOWN,     Pk_KP_3, PR_FALSE },
  { NS_VK_LEFT,          Pk_KP_4, PR_FALSE },
  { NS_VK_NUMPAD5,       Pk_KP_5, PR_FALSE },
  { NS_VK_RIGHT,         Pk_KP_6, PR_FALSE },
  { NS_VK_HOME,          Pk_KP_7, PR_FALSE },
  { NS_VK_UP,            Pk_KP_8, PR_FALSE },
  { NS_VK_PAGE_UP,       Pk_KP_9, PR_FALSE }
  };


// Input keysym is in gtk format; output is in NS_VK format
PRUint32 nsWidget::nsConvertKey(unsigned long keysym, PRBool *aIsChar ) {

  const int length = sizeof(nsKeycodes) / sizeof(struct nsKeyConverter);

	/* Default this to TRUE */
	*aIsChar = PR_TRUE;
	
  // First, try to handle alphanumeric input, not listed in nsKeycodes:
  if (keysym >= Pk_a && keysym <= Pk_z)
     return keysym - Pk_a + NS_VK_A;

  if (keysym >= Pk_A && keysym <= Pk_Z)
     return keysym - Pk_A + NS_VK_A;

  if (keysym >= Pk_0 && keysym <= Pk_9)
     return keysym - Pk_0 + NS_VK_0;
		  
  if (keysym >= Pk_F1 && keysym <= Pk_F24) {
     *aIsChar = PR_FALSE;
     return keysym - Pk_F1 + NS_VK_F1;
  	}

  for (int i = 0; i < length; i++) {
    if( nsKeycodes[i].keysym == keysym ) {
      *aIsChar = (nsKeycodes[i].isChar);
      return (nsKeycodes[i].vkCode);
    	}
  	}

  return((int) 0);
	}

//==============================================================
inline void nsWidget::InitKeyEvent(PhKeyEvent_t *aPhKeyEvent,
                            nsWidget * aWidget,
                            nsKeyEvent &anEvent,
                            PRUint32   aEventType) {

  if( aPhKeyEvent != nsnull ) {
  
    anEvent.message = aEventType;
    anEvent.widget  = aWidget;
    anEvent.nativeMsg = (void *)aPhKeyEvent;
    anEvent.time =      PR_IntervalNow();
    anEvent.point.x = 0; 
    anEvent.point.y = 0;

    PRBool IsChar;
    unsigned long vkey;
		if (Pk_KF_Cap_Valid & aPhKeyEvent->key_flags)
			vkey = nsConvertKey(aPhKeyEvent->key_cap, &IsChar);
		else vkey = nsConvertKey(aPhKeyEvent->key_sym, &IsChar);

    anEvent.isShift =   ( aPhKeyEvent->key_mods & Pk_KM_Shift ) ? PR_TRUE : PR_FALSE;
    anEvent.isControl = ( aPhKeyEvent->key_mods & Pk_KM_Ctrl )  ? PR_TRUE : PR_FALSE;
    anEvent.isAlt =     ( aPhKeyEvent->key_mods & Pk_KM_Alt )   ? PR_TRUE : PR_FALSE;
    anEvent.isMeta =    PR_FALSE;

    if ((aEventType == NS_KEY_PRESS) && (IsChar == PR_TRUE)) {
      anEvent.charCode = aPhKeyEvent->key_sym;
      anEvent.keyCode =  0; /* I think the spec says this should be 0 */

      if ((anEvent.isControl) || (anEvent.isAlt))
        anEvent.charCode = aPhKeyEvent->key_cap;
			else anEvent.isShift = anEvent.isControl = anEvent.isAlt = anEvent.isMeta = PR_FALSE;
    	}
		else {
 	    anEvent.charCode = 0; 
 	    anEvent.keyCode = vkey;
  	  }
  	}

	}


PRBool  nsWidget::DispatchKeyEvent( PhKeyEvent_t *aPhKeyEvent ) {
  NS_ASSERTION(aPhKeyEvent, "nsWidget::DispatchKeyEvent a NULL PhKeyEvent was passed in");

  nsKeyEvent keyEvent;
  PRBool result = PR_FALSE;

  if ( (aPhKeyEvent->key_flags & Pk_KF_Cap_Valid) == 0) {
		//printf("nsWidget::DispatchKeyEvent throwing away invalid key: Modifiers Valid=<%d,%d,%d> this=<%p>\n",
		//(aPhKeyEvent->key_flags & Pk_KF_Scan_Valid), (aPhKeyEvent->key_flags & Pk_KF_Sym_Valid), (aPhKeyEvent->key_flags & Pk_KF_Cap_Valid), this );
		return PR_FALSE; //PR_TRUE;
		}

  if ( PtIsFocused(mWidget) != 2) {
     //printf("nsWidget::DispatchKeyEvent Not on focus leaf! PtIsFocused(mWidget)=<%d>\n", PtIsFocused(mWidget));
     return PR_FALSE;
  	}
  
  if ( ( aPhKeyEvent->key_cap == Pk_Shift_L )
       || ( aPhKeyEvent->key_cap == Pk_Shift_R )
       || ( aPhKeyEvent->key_cap == Pk_Control_L )
       || ( aPhKeyEvent->key_cap ==  Pk_Control_R )
       || ( aPhKeyEvent->key_cap ==  Pk_Num_Lock )
       || ( aPhKeyEvent->key_cap ==  Pk_Scroll_Lock )
     )
    return PR_FALSE; //PR_TRUE;

  nsWindow *w = (nsWindow *) this;

  w->AddRef();
 
  if (aPhKeyEvent->key_flags & Pk_KF_Key_Down) {
//    InitKeyEvent(aPhKeyEvent, this, keyEvent, NS_KEY_DOWN);
//    result = w->OnKey(keyEvent); 

    InitKeyEvent(aPhKeyEvent, this, keyEvent, NS_KEY_PRESS);
    result = w->OnKey(keyEvent); 
  	}
  else if (aPhKeyEvent->key_flags & Pk_KF_Key_Repeat) {
    InitKeyEvent(aPhKeyEvent, this, keyEvent, NS_KEY_PRESS);
    result = w->OnKey(keyEvent);   
  	}
  else if (PkIsKeyDown(aPhKeyEvent->key_flags) == 0) {
    InitKeyEvent(aPhKeyEvent, this, keyEvent, NS_KEY_UP);
    result = w->OnKey(keyEvent); 
  	}

  w->Release();

  return result;
	}



// used only once
inline PRBool nsWidget::HandleEvent( PtWidget_t *widget, PtCallbackInfo_t* aCbInfo ) {
  PRBool  result = PR_TRUE; // call the default nsWindow proc
  PhEvent_t* event = aCbInfo->event;

	if (event->processing_flags & Ph_CONSUMED) return PR_TRUE;

	switch ( event->type ) {

      case Ph_EV_PTR_MOTION_NOBUTTON:
       	{
	    	PhPointerEvent_t* ptrev = (PhPointerEvent_t*) PhGetData( event );
	    	nsMouseEvent   theMouseEvent;

        if( ptrev ) {

					if( ptrev->flags & Ph_PTR_FLAG_Z_ONLY ) break; // sometimes z presses come out of nowhere */

///* ATENTIE */ printf( "Ph_EV_PTR_MOTION_NOBUTTON (%d %d)\n", ptrev->pos.x, ptrev->pos.y );

        	ScreenToWidgetPos( ptrev->pos );
 	      	InitMouseEvent(ptrev, this, theMouseEvent, NS_MOUSE_MOVE );
        	result = DispatchMouseEvent(theMouseEvent);
        	}
       	}
	    	break;

      case Ph_EV_BUT_PRESS:
       {

	    	PhPointerEvent_t* ptrev = (PhPointerEvent_t*) PhGetData( event );
   		  nsMouseEvent   theMouseEvent;

				/* there should be no reason to do this - mozilla should figure out how to call SetFocus */
				/* this though fixes the problem with the plugins capturing the focus */
				PtWidget_t *disjoint = PtFindDisjoint( widget );
 				if( PtWidgetIsClassMember( disjoint, PtServer ) )
					PtContainerGiveFocus( widget, aCbInfo->event );
				else {
					if( sJustGotActivated ) {
						sJustGotActivated = PR_FALSE;
						DispatchStandardEvent(NS_ACTIVATE);
						}
					}

        if( ptrev ) {
          ScreenToWidgetPos( ptrev->pos );

          if( ptrev->buttons & Ph_BUTTON_SELECT ) // Normally the left mouse button
						InitMouseEvent(ptrev, this, theMouseEvent, NS_MOUSE_LEFT_BUTTON_DOWN );
          else if( ptrev->buttons & Ph_BUTTON_MENU ) // the right button
						InitMouseEvent(ptrev, this, theMouseEvent, NS_MOUSE_RIGHT_BUTTON_DOWN );
          else // middle button
						InitMouseEvent(ptrev, this, theMouseEvent, NS_MOUSE_MIDDLE_BUTTON_DOWN );

		  		result = DispatchMouseEvent(theMouseEvent);

					// if we're a right-button-up we're trying to popup a context menu. send that event to gecko also
					if( ptrev->buttons & Ph_BUTTON_MENU ) {
						nsMouseEvent contextMenuEvent;
						InitMouseEvent( ptrev, this, contextMenuEvent, NS_CONTEXTMENU );
						result = DispatchMouseEvent( contextMenuEvent );
						}
      	  }

      	 }
		break;		
		
		case Ph_EV_BUT_RELEASE:
		  {
			  PhPointerEvent_t* ptrev = (PhPointerEvent_t*) PhGetData( event );
			  nsMouseEvent      theMouseEvent;
			  
			  if (event->subtype==Ph_EV_RELEASE_REAL || event->subtype==Ph_EV_RELEASE_PHANTOM) {
				  if (ptrev) {
					  ScreenToWidgetPos( ptrev->pos );
					  if ( ptrev->buttons & Ph_BUTTON_SELECT ) // Normally the left mouse button
						 InitMouseEvent(ptrev, this, theMouseEvent, NS_MOUSE_LEFT_BUTTON_UP );
					  else if( ptrev->buttons & Ph_BUTTON_MENU ) // the right button
						 InitMouseEvent(ptrev, this, theMouseEvent, NS_MOUSE_RIGHT_BUTTON_UP );
					  else // middle button
						 InitMouseEvent(ptrev, this, theMouseEvent, NS_MOUSE_MIDDLE_BUTTON_UP );
					  
					  result = DispatchMouseEvent(theMouseEvent);
				  }
			  }
			  else if (event->subtype==Ph_EV_RELEASE_OUTBOUND) {
				  PhRect_t rect = {{0,0},{0,0}};
				  PhRect_t boundary = {{-10000,-10000},{10000,10000}};
				  PhInitDrag( PtWidgetRid(mWidget), ( Ph_DRAG_KEY_MOTION | Ph_DRAG_TRACK | Ph_TRACK_DRAG),&rect, &boundary, aCbInfo->event->input_group , NULL, NULL, NULL, NULL, NULL);
			  }
		  }
	    break;

		case Ph_EV_PTR_MOTION_BUTTON:
      	{
        PhPointerEvent_t* ptrev = (PhPointerEvent_t*) PhGetData( event );
	    	nsMouseEvent   theMouseEvent;

        if( ptrev ) {

					if( ptrev->flags & Ph_PTR_FLAG_Z_ONLY ) break; // sometimes z presses come out of nowhere */

					if( sDragService ) {
						nsDragService *d;
						nsIDragService *s = sDragService;
						d = ( nsDragService * )s;
						d->SetNativeDndData( widget, event );
						}

          ScreenToWidgetPos( ptrev->pos );
 	      	InitMouseEvent(ptrev, this, theMouseEvent, NS_MOUSE_MOVE );
          result = DispatchMouseEvent(theMouseEvent);
        	}
      	}
      	break;

      case Ph_EV_KEY:
				result = DispatchKeyEvent( (PhKeyEvent_t*) PhGetData( event ) );
        break;

      case Ph_EV_DRAG:
	    	{
          nsMouseEvent   theMouseEvent;

          switch(event->subtype) {

		  			case Ph_EV_DRAG_COMPLETE: 
            	{  
 		      		nsMouseEvent      theMouseEvent;
              PhPointerEvent_t* ptrev2 = (PhPointerEvent_t*) PhGetData( event );
              ScreenToWidgetPos( ptrev2->pos );
              InitMouseEvent(ptrev2, this, theMouseEvent, NS_MOUSE_LEFT_BUTTON_UP );
              result = DispatchMouseEvent(theMouseEvent);
            	}
							break;
		  			case Ph_EV_DRAG_MOTION_EVENT: {
      		    PhPointerEvent_t* ptrev2 = (PhPointerEvent_t*) PhGetData( event );
      		    ScreenToWidgetPos( ptrev2->pos );

							if( sDragService ) {
								nsDragService *d;
								nsIDragService *s = sDragService;
								d = ( nsDragService * )s;
								d->SetNativeDndData( widget, event );
								}

  	  		    InitMouseEvent(ptrev2, this, theMouseEvent, NS_MOUSE_MOVE );
      		    result = DispatchMouseEvent(theMouseEvent);
							}
							break;
		  			}
					}
        break;

      case Ph_EV_BOUNDARY:
        switch( event->subtype ) {
          case Ph_EV_PTR_ENTER:
						result = DispatchStandardEvent( NS_MOUSE_ENTER );
            break;
					case Ph_EV_PTR_LEAVE_TO_CHILD:
          case Ph_EV_PTR_LEAVE:
						result = DispatchStandardEvent( NS_MOUSE_EXIT );
            break;
          default:
            break;
        	}
        break;
    	}

  return result;
	}



//-------------------------------------------------------------------------
//
// the nsWidget raw event callback for all nsWidgets in this toolkit
//
//-------------------------------------------------------------------------
int nsWidget::RawEventHandler( PtWidget_t *widget, void *data, PtCallbackInfo_t *cbinfo ) {

  // Get the window which caused the event and ask it to process the message
  nsWidget *someWidget = (nsWidget*) data;

  if( someWidget && someWidget->mIsDestroying == PR_FALSE && someWidget->HandleEvent( widget, cbinfo ) )
		return Pt_END; // Event was consumed

  return Pt_CONTINUE;
	}


int nsWidget::GotFocusCallback( PtWidget_t *widget, void *data, PtCallbackInfo_t *cbinfo ) 
{
  nsWidget *pWidget = (nsWidget *) data;

	if( widget->class_rec->description && PtWidgetIsClass( widget, PtWindow ) ) {
		if( pWidget->mEventCallback ) {

			/* the WM_ACTIVATE code */

			sJustGotActivated = PR_TRUE;
		
			nsMouseEvent event;
			pWidget->InitEvent(event, NS_MOUSE_ACTIVATE);
			event.acceptActivation = PR_TRUE;

			pWidget->DispatchWindowEvent(&event);
			}
		}

	pWidget->DispatchStandardEvent(NS_GOTFOCUS);

 	if( sJustGotActivated ) {
		sJustGotActivated = PR_FALSE;
		pWidget->DispatchStandardEvent(NS_ACTIVATE);
 		}

  return Pt_CONTINUE;
}

int nsWidget::LostFocusCallback( PtWidget_t *widget, void *data, PtCallbackInfo_t *cbinfo ) 
{
  nsWidget *pWidget = (nsWidget *) data;
 	if( sJustGotDeactivated ) {
		sJustGotDeactivated = PR_FALSE;
		pWidget->DispatchStandardEvent(NS_DEACTIVATE);
		}
 	pWidget->DispatchStandardEvent(NS_LOSTFOCUS);
  return Pt_CONTINUE;
}

int nsWidget::DestroyedCallback( PtWidget_t *widget, void *data, PtCallbackInfo_t *cbinfo ) {
  nsWidget *pWidget = (nsWidget *) data;
  if( !pWidget->mIsDestroying ) pWidget->OnDestroy();
  return Pt_CONTINUE;
	}

void nsWidget::ProcessDrag( PhEvent_t *event, PRUint32 aEventType, PhPoint_t *pos ) {
	nsCOMPtr<nsIDragSession> currSession;
	sDragService->GetCurrentSession ( getter_AddRefs(currSession) );
	if( !currSession ) return;

	currSession->SetDragAction( 2 );

	DispatchDragDropEvent( aEventType, pos );

	event->subtype = Ph_EV_DND_ENTER;

	PRBool canDrop;
	currSession->GetCanDrop(&canDrop);
	if(!canDrop) {
		static PhCharacterCursorDescription_t nodrop_cursor = {  { Ph_CURSOR_NOINPUT, sizeof(PhCharacterCursorDescription_t) }, PgRGB( 255, 255, 224 ) };
		PhAckDnd( event, Ph_EV_DND_MOTION, ( PhCursorDescription_t * ) &nodrop_cursor );
		}
	else {
		static PhCharacterCursorDescription_t drop_cursor = { { Ph_CURSOR_PASTE, sizeof(PhCharacterCursorDescription_t) }, PgRGB( 255, 255, 224 ) };
		PhAckDnd( event, Ph_EV_DND_MOTION, ( PhCursorDescription_t * ) &drop_cursor );
		}

	// Clear the cached value
	currSession->SetCanDrop(PR_FALSE);
	}

void nsWidget::DispatchDragDropEvent( PRUint32 aEventType, PhPoint_t *pos ) {
  nsEventStatus status;
  nsMouseEvent event;

  InitEvent( event, aEventType );

  event.point.x = pos->x;
  event.point.y = pos->y;

  event.isShift   = 0;
  event.isControl = 0;
  event.isMeta    = PR_FALSE;
  event.isAlt     = 0;

	event.widget = this;

///* ATENTIE */ printf("DispatchDragDropEvent pos=%d %d widget=%p\n", event.point.x, event.point.y, this );

  DispatchEvent( &event, status );
	}


int nsWidget::DndCallback( PtWidget_t *widget, void *data, PtCallbackInfo_t *cbinfo ) {
	nsWidget *pWidget = (nsWidget *) data;
	 PtDndCallbackInfo_t *cbdnd = (  PtDndCallbackInfo_t * ) cbinfo->cbdata;

	static PtDndFetch_t dnd_data_template = { "Mozilla", "dnddata", Ph_TRANSPORT_INLINE, Pt_DND_SELECT_MOTION,
                        NULL, NULL, NULL, NULL, NULL };

///* ATENTIE */ printf( "In nsWidget::DndCallback subtype=%d\n", cbinfo->reason_subtype );

			PhPointerEvent_t* ptrev = (PhPointerEvent_t*) PhGetData( cbinfo->event );
//printf("Enter pos=%d %d\n", ptrev->pos.x, ptrev->pos.y );
			pWidget->ScreenToWidgetPos( ptrev->pos );
//printf("After trans pos=%d %d pWidget=%p\n", ptrev->pos.x, ptrev->pos.y, pWidget );


	switch( cbinfo->reason_subtype ) {
		case Ph_EV_DND_ENTER: {
			sDragService->StartDragSession();
			pWidget->ProcessDrag( cbinfo->event, NS_DRAGDROP_ENTER, &ptrev->pos );

			PtDndSelect( widget, &dnd_data_template, 1, NULL, NULL, cbinfo );
			}
			break;

		case Ph_EV_DND_MOTION: {
			pWidget->ProcessDrag( cbinfo->event, NS_DRAGDROP_OVER, &ptrev->pos );
			}
			break;
		case Ph_EV_DND_DROP:
			nsDragService *d;
			d = ( nsDragService * )sDragService;
			d->SetDropData( (char*)cbdnd->data+4+100, (PRUint32) *(int*)cbdnd->data, (char*) cbdnd->data + 4 );
///* ATENTIE */ printf("SetDropData with data=%s\n", (char*) cbdnd->data );

			pWidget->ProcessDrag( cbinfo->event, NS_DRAGDROP_DROP, &ptrev->pos );
			sDragService->EndDragSession();
			break;
		case Ph_EV_DND_LEAVE:
		case Ph_EV_DND_CANCEL:
			pWidget->ProcessDrag( cbinfo->event, NS_DRAGDROP_EXIT, &ptrev->pos );
			sDragService->EndDragSession();
			break;
		}

	return Pt_CONTINUE;
	}
