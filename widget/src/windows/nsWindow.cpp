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
#include "nsIAppShell.h"
#include "nsIFontMetrics.h"
#include "nsIFontCache.h"
#include "nsGUIEvent.h"
#include "nsIRenderingContext.h"
#include "nsIDeviceContext.h"
#include "nsRect.h"
#include "nsTransform2D.h"
#include "nsStringUtil.h"
#include <windows.h>
#include "nsGfxCIID.h"
#include "resource.h"
#include <commctrl.h>
#include "prtime.h"

BOOL nsWindow::sIsRegistered = FALSE;

nsWindow* nsWindow::gCurrentWindow = nsnull;

// Global variable 
//     g_hinst - handle of the application instance 
extern HINSTANCE g_hinst; 

static NS_DEFINE_IID(kIWidgetIID, NS_IWIDGET_IID);

//-------------------------------------------------------------------------
//
// Default for height modification is to do nothing
//
//-------------------------------------------------------------------------

PRInt32 nsWindow::GetHeight(PRInt32 aProposedHeight)
{
  return(aProposedHeight);
}

//-------------------------------------------------------------------------
//
// Deferred Window positioning
//
//-------------------------------------------------------------------------

void nsWindow::BeginResizingChildren(void)
{
  if (NULL == mDeferredPositioner)
    mDeferredPositioner = ::BeginDeferWindowPos(1);
}

void nsWindow::EndResizingChildren(void)
{
  if (NULL != mDeferredPositioner) {
    ::EndDeferWindowPos(mDeferredPositioner);
    mDeferredPositioner = NULL;
  }
}

// DoCreateTooltip - creates a tooltip control and adds some tools  
//     to it. 
// Returns the handle of the tooltip control if successful or NULL
//     otherwise. 
// hwndOwner - handle of the owner window 
// 

void nsWindow::AddTooltip(HWND hwndOwner,nsRect* aRect, int aId) 
{ 
    TOOLINFO ti;    // tool information
    memset(&ti, 0, sizeof(TOOLINFO));
  
    // Make sure the common control DLL is loaded
    InitCommonControls(); 
 
    // Create a tooltip control for the window if needed
    if (mTooltip == (HWND) NULL) {
        mTooltip = CreateWindow(TOOLTIPS_CLASS, (LPSTR) NULL, TTS_ALWAYSTIP, 
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 
        NULL, (HMENU) NULL, 
        nsToolkit::mDllInstance,
        NULL);
    }
 
    if (mTooltip == (HWND) NULL) 
        return;

    ti.cbSize = sizeof(TOOLINFO); 
    ti.uFlags = TTF_SUBCLASS;
    ti.hwnd = hwndOwner; 
    ti.hinst = nsToolkit::mDllInstance; 
    ti.uId = aId;
    ti.lpszText = (LPSTR)" "; // must set text to 
                              // something for tooltip to give events; 
    ti.rect.left = aRect->x; 
    ti.rect.top = aRect->y; 
    ti.rect.right = aRect->x + aRect->width; 
    ti.rect.bottom = aRect->y + aRect->height; 

    if (!SendMessage(mTooltip, TTM_ADDTOOL, 0, 
            (LPARAM) (LPTOOLINFO) &ti)) 
        return; 
}

void nsWindow::WidgetToScreen(const nsRect& aOldRect, nsRect& aNewRect)
{
  POINT point;
  point.x = aOldRect.x;
  point.y = aOldRect.y;
  ::ClientToScreen((HWND)GetNativeData(NS_NATIVE_WINDOW), &point);
  aNewRect.x = point.x;
  aNewRect.y = point.y;
  aNewRect.width = aOldRect.width;
  aNewRect.height = aOldRect.height;
}

void nsWindow::ScreenToWidget(const nsRect& aOldRect, nsRect& aNewRect)
{
  POINT point;
  point.x = aOldRect.x;
  point.y = aOldRect.y;
  ::ScreenToClient((HWND)GetNativeData(NS_NATIVE_WINDOW), &point);
  aNewRect.x = point.x;
  aNewRect.y = point.y;
  aNewRect.width = aOldRect.width;
  aNewRect.height = aOldRect.height;
} 

//-------------------------------------------------------------------------
//
// Setup initial tooltip rectangles
//
//-------------------------------------------------------------------------

void nsWindow::SetTooltips(PRUint32 aNumberOfTips,nsRect* aTooltipAreas[])
{
  RemoveTooltips();
  for (int i = 0; i < (int)aNumberOfTips; i++) {
    AddTooltip(mWnd, aTooltipAreas[i], i);
  }
}

//-------------------------------------------------------------------------
//
// Update all tooltip rectangles
//
//-------------------------------------------------------------------------

void nsWindow::UpdateTooltips(nsRect* aNewTips[])
{
  TOOLINFO ti;
  memset(&ti, 0, sizeof(TOOLINFO));
  ti.cbSize = sizeof(TOOLINFO);
  ti.hwnd = mWnd;
  // Get the number of tooltips
  UINT count = ::SendMessage(mTooltip, TTM_GETTOOLCOUNT, 0, 0); 
  NS_ASSERTION(count > 0, "Called UpdateTooltips before calling SetTooltips");

  for (UINT i = 0; i < count; i++) {
    ti.uId = i;
    int result =::SendMessage(mTooltip, TTM_ENUMTOOLS, i, (LPARAM) (LPTOOLINFO)&ti);

    nsRect* newTip = aNewTips[i];
    ti.rect.left    = newTip->x; 
    ti.rect.top     = newTip->y; 
    ti.rect.right   = newTip->x + newTip->width; 
    ti.rect.bottom  = newTip->y + newTip->height; 
    ::SendMessage(mTooltip, TTM_NEWTOOLRECT, 0, (LPARAM) (LPTOOLINFO)&ti);

  }

}

//-------------------------------------------------------------------------
//
// Remove all tooltip rectangles
//
//-------------------------------------------------------------------------

void nsWindow::RemoveTooltips()
{
  TOOLINFO ti;
  memset(&ti, 0, sizeof(TOOLINFO));
  ti.cbSize = sizeof(TOOLINFO);
  long val;

  if (mTooltip == NULL)
    return;

  // Get the number of tooltips
  UINT count = ::SendMessage(mTooltip, TTM_GETTOOLCOUNT, 0, (LPARAM)&val); 
  for (UINT i = 0; i < count; i++) {
    ti.uId = i;
    ti.hwnd = mWnd;
    ::SendMessage(mTooltip, TTM_DELTOOL, 0, (LPARAM) (LPTOOLINFO)&ti);
  }
}


//-------------------------------------------------------------------------
//
// Convert nsEventStatus value to a windows boolean
//
//-------------------------------------------------------------------------

PRBool nsWindow::ConvertStatus(nsEventStatus aStatus)
{
  switch(aStatus) {
    case nsEventStatus_eIgnore:
      return(PR_FALSE);
    break;
    case nsEventStatus_eConsumeNoDefault:
      return(PR_TRUE);
    break;
    case nsEventStatus_eConsumeDoDefault:
      return(PR_FALSE);
    break;
    default:
      NS_ASSERTION(0, "Illegal nsEventStatus enumeration value");
      return(PR_FALSE);
    break;
  }
}

//-------------------------------------------------------------------------
//
// Initialize an event to dispatch
//
//-------------------------------------------------------------------------

void nsWindow::InitEvent(nsGUIEvent& event, PRUint32 aEventType, nsPoint* aPoint)
{
    event.widget = this;
    event.widgetSupports = mOuter;

    if (nsnull == aPoint) {     // use the point from the event
      // get the message position in client coordinates and in twips
      DWORD pos = ::GetMessagePos();
      POINT cpos;

      cpos.x = LOWORD(pos);
      cpos.y = HIWORD(pos);

      if (mWnd != NULL) {
        ::ScreenToClient(mWnd, &cpos);
        event.point.x = cpos.x;
        event.point.y = cpos.y;
      } else {
        event.point.x = 0;
        event.point.y = 0;
      }
    }
    else {                      // use the point override if provided
      event.point.x = aPoint->x;
      event.point.y = aPoint->y;
    }

    event.time = ::GetMessageTime();
    event.message = aEventType;

    mLastPoint.x = event.point.x;
    mLastPoint.y = event.point.y;
}

//-------------------------------------------------------------------------
//
// Invokes callback and  ProcessEvent method on Event Listener object
//
//-------------------------------------------------------------------------

PRBool nsWindow::DispatchEvent(nsGUIEvent* event)
{
  PRBool result = PR_FALSE;
 
  if (nsnull != mEventCallback) {
    result = ConvertStatus((*mEventCallback)(event));
  }

    // Dispatch to event listener if event was not consumed
  if ((result != PR_TRUE) && (nsnull != mEventListener)) {
    return ConvertStatus(mEventListener->ProcessEvent(*event));
  }
  else {
    return(result); 
  }
}

//-------------------------------------------------------------------------
//
// Dispatch standard event
//
//-------------------------------------------------------------------------

PRBool nsWindow::DispatchStandardEvent(PRUint32 aMsg)
{
  nsGUIEvent event;
  event.eventStructType = NS_GUI_EVENT;
  InitEvent(event, aMsg);
  return(DispatchEvent(&event));
}

//-------------------------------------------------------------------------
//
// the nsWindow procedure for all nsWindows in this toolkit
//
//-------------------------------------------------------------------------
LRESULT CALLBACK nsWindow::WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
      // Get the window which caused the event and ask it to process the message
    nsWindow *someWindow = (nsWindow*)::GetWindowLong(hWnd, GWL_USERDATA);


      // Re-direct a tab change message destined for it's parent window to the
      // the actual window which generated the event.
    if (msg == WM_NOTIFY) {
      LPNMHDR pnmh = (LPNMHDR) lParam;
      if (pnmh->code == TCN_SELCHANGE) {             
        someWindow = (nsWindow*)::GetWindowLong(pnmh->hwndFrom, GWL_USERDATA); 
      }
    }


    if (nsnull != someWindow) {
        LRESULT retValue;
        if (PR_TRUE == someWindow->ProcessMessage(msg, wParam, lParam, &retValue)) {
            return retValue;
        }
    }

#if defined(STRICT)
    return ::CallWindowProc((WNDPROC)someWindow->GetPrevWindowProc(), hWnd, 
                            msg, wParam, lParam);
#else
    return ::CallWindowProc((FARPROC)someWindow->GetPrevWindowProc(), hWnd, 
                            msg, wParam, lParam);
#endif
}


//-------------------------------------------------------------------------
//
// nsWindow constructor
//
//-------------------------------------------------------------------------
nsWindow::nsWindow(nsISupports *aOuter) : nsObject(aOuter)
{
    mWnd           = 0;
    mPrevWndProc   = NULL;
    mChildren      = NULL;
    mEventCallback = NULL;
    mToolkit       = NULL;
    mAppShell      = NULL;
    mMouseListener = NULL;
    mEventListener = NULL;
    mBackground    = ::GetSysColor(COLOR_BTNFACE);
    mBrush         = ::CreateSolidBrush(NSRGB_2_COLOREF(mBackground));
    mForeground    = ::GetSysColor(COLOR_WINDOWTEXT);
    mPalette       = NULL;
    mCursor        = eCursor_standard;
    mBorderStyle   = eBorderStyle_none;
    mIsShiftDown   = PR_FALSE;
    mIsControlDown = PR_FALSE;
    mIsAltDown     = PR_FALSE;
    mIsDestroying = PR_FALSE;
    mTooltip       = NULL;
    mDeferredPositioner = NULL;
    mLastPoint.x   = 0;
    mLastPoint.y   = 0;
    mWidth = mHeight = 0;
    mClientData = NULL;
    mContext = NULL;
}


//-------------------------------------------------------------------------
//
// nsWindow destructor
//
//-------------------------------------------------------------------------
nsWindow::~nsWindow()
{
  mIsDestroying = PR_TRUE;
  if (gCurrentWindow == this) {
    gCurrentWindow = nsnull;
  }

  // If the widget was released without calling Destroy() then the native
  // window still exists, and we need to destroy it
  if (NULL != mWnd) {
    Destroy();
  }
}


//-------------------------------------------------------------------------
//
// Query interface implementation
//
//-------------------------------------------------------------------------
nsresult nsWindow::QueryObject(const nsIID& aIID, void** aInstancePtr)
{
    nsresult result = nsObject::QueryObject(aIID, aInstancePtr);

    static NS_DEFINE_IID(kIWidgetIID, NS_IWIDGET_IID);
    if (result == NS_NOINTERFACE && aIID.Equals(kIWidgetIID)) {
        *aInstancePtr = (void*) ((nsIWidget*)this);
        AddRef();
        result = NS_OK;
    }

    return result;
}


//-------------------------------------------------------------------------
//
// Create the proper widget
//
//-------------------------------------------------------------------------
void nsWindow::Create(nsIWidget *aParent,
                      const nsRect &aRect,
                      EVENT_CALLBACK aHandleEventFunction,
                      nsIDeviceContext *aContext,
                      nsIAppShell *aAppShell,
                      nsIToolkit *aToolkit,
                      nsWidgetInitData *aInitData)
{
    if (NULL == mToolkit) {
        if (NULL != aToolkit) {
            mToolkit = (nsToolkit*)aToolkit;
            NS_ADDREF(mToolkit);
        }
        else {
            if (NULL != aParent) {
                mToolkit = (nsToolkit*)(aParent->GetToolkit()); // the call AddRef's, we don't have to
            }
            // it's some top level window with no toolkit passed in.
            // Create a default toolkit with the current thread
            else {
                mToolkit = new nsToolkit();
                NS_ADDREF(mToolkit);
                mToolkit->Init(PR_GetCurrentThread());
            }
        }

    }

    mAppShell = aAppShell;
    NS_IF_ADDREF(mAppShell);

    //
    // Switch to the "main gui thread" if necessary... This method must
    // be executed on the "gui thread"...
    //
    if (!mToolkit->IsGuiThread()) {
        DWORD args[5];
        args[0] = (DWORD)aParent;
        args[1] = (DWORD)&aRect;
        args[2] = (DWORD)aHandleEventFunction;
        args[3] = (DWORD)aContext;
        args[4] = (DWORD)aToolkit;
        args[5] = (DWORD)aInitData;
        MethodInfo info(this, nsWindow::CREATE, 6, args);
        mToolkit->CallMethod(&info);
        return;
    }

    // save the event callback function
    mEventCallback = aHandleEventFunction;

    // keep a reference to the device context
    if (aContext) {
        mContext = aContext;
        NS_ADDREF(mContext);
    }
    else {
      nsresult  res;

      static NS_DEFINE_IID(kDeviceContextCID, NS_DEVICE_CONTEXT_CID);
      static NS_DEFINE_IID(kDeviceContextIID, NS_IDEVICE_CONTEXT_IID);

      res = NSRepository::CreateInstance(kDeviceContextCID, nsnull, kDeviceContextIID, (void **)&mContext);

      if (NS_OK == res)
        mContext->Init(nsnull);
    }

    if (nsnull != aInitData) {
      PreCreateWidget(aInitData);
    }

    // See if the caller wants to explictly set clip children
    DWORD style = WindowStyle();
    if (nsnull != aInitData) {
      if (aInitData->clipChildren) {
        style |= WS_CLIPCHILDREN;
      } else {
        style &= ~WS_CLIPCHILDREN;
      }
    }

    mWnd = ::CreateWindowEx(WindowExStyle(),
                            WindowClass(),
                            "",
                            style,
                            aRect.x,
                            aRect.y,
                            aRect.width,
                            GetHeight(aRect.height),
                            (aParent) ? (HWND)aParent->GetNativeData(NS_NATIVE_WINDOW):
                                        (HWND)NULL,
                            NULL,
                            nsToolkit::mDllInstance,
                            NULL);
    
    VERIFY(mWnd);
    if (aParent) {
        aParent->AddChild(this);
    }

    // Force cursor to default setting
    mCursor = eCursor_select;
    SetCursor(eCursor_standard);

    // call the event callback to notify about creation

    DispatchStandardEvent(NS_CREATE);
    SubclassWindow(TRUE);
}


//-------------------------------------------------------------------------
//
// create with a native parent
//
//-------------------------------------------------------------------------
void nsWindow::Create(nsNativeWidget aParent,
                         const nsRect &aRect,
                         EVENT_CALLBACK aHandleEventFunction,
                         nsIDeviceContext *aContext,
                         nsIAppShell *aAppShell,
                         nsIToolkit *aToolkit,
                         nsWidgetInitData *aInitData)
{

    if (NULL == mToolkit) {
        if (NULL != aToolkit) {
            mToolkit = (nsToolkit*)aToolkit;
            NS_ADDREF(mToolkit);
        }
        else {
            mToolkit = new nsToolkit();
            NS_ADDREF(mToolkit);
            mToolkit->Init(PR_GetCurrentThread());
        }

    }

    mAppShell = aAppShell;
    NS_IF_ADDREF(aAppShell);

    //
    // Switch to the "main gui thread" if necessary... This method must
    // be executed on the "gui thread"...
    //
    if (!mToolkit->IsGuiThread()) {
        DWORD args[5];
        args[0] = (DWORD)aParent;
        args[1] = (DWORD)&aRect;
        args[2] = (DWORD)aHandleEventFunction;
        args[3] = (DWORD)aContext;
        args[4] = (DWORD)aToolkit;
        args[5] = (DWORD)aInitData;
        MethodInfo info(this, nsWindow::CREATE_NATIVE, 5, args);
        mToolkit->CallMethod(&info);
        return;
    }

    // save the event callback function
    mEventCallback = aHandleEventFunction;

    // keep a reference to the device context
    if (aContext) {
        mContext = aContext;
        NS_ADDREF(mContext);
    }
    else {
      nsresult  res;

      static NS_DEFINE_IID(kDeviceContextCID, NS_DEVICE_CONTEXT_CID);
      static NS_DEFINE_IID(kDeviceContextIID, NS_IDEVICE_CONTEXT_IID);

      res = NSRepository::CreateInstance(kDeviceContextCID, nsnull, kDeviceContextIID, (void **)&mContext);

      if (NS_OK == res)
        mContext->Init(nsnull);
    }

    if (nsnull != aInitData) {
      PreCreateWidget(aInitData);
    }

    // See if the caller wants to explictly set clip children
    DWORD style = WindowStyle();
    if (nsnull != aInitData) {
      if (aInitData->clipChildren) {
        style |= WS_CLIPCHILDREN;
      } else {
        style &= ~WS_CLIPCHILDREN;
      }
    }

    mWnd = ::CreateWindowEx(WindowExStyle(),
                            WindowClass(),
                            "",
                            style,
                            aRect.x,
                            aRect.y,
                            aRect.width,
                            GetHeight(aRect.height),
                            (HWND)aParent,
                            NULL,
                            nsToolkit::mDllInstance,
                            NULL);
    
    VERIFY(mWnd);

    // call the event callback to notify about creation
    DispatchStandardEvent(NS_CREATE);
    SubclassWindow(TRUE);
}

//-------------------------------------------------------------------------
//
// Accessor functions to get/set the client data
//
//-------------------------------------------------------------------------

NS_IMETHODIMP nsWindow::GetClientData(void*& aClientData)
{
  aClientData = mClientData;
  return NS_OK;
}

NS_IMETHODIMP nsWindow::SetClientData(void* aClientData)
{
  mClientData = aClientData;
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Close this nsWindow
//
//-------------------------------------------------------------------------
void nsWindow::Destroy()
{
  // Switch to the "main gui thread" if necessary... This method must
  // be executed on the "gui thread"...
  if (mToolkit != nsnull && !mToolkit->IsGuiThread()) {
    MethodInfo info(this, nsWindow::DESTROY);
    mToolkit->CallMethod(&info);
    return;
  }

  // disconnect from the parent
  if (!mIsDestroying) {
    nsIWidget *parent = GetParent();
    if (parent) {
      parent->RemoveChild(this);
      NS_RELEASE(parent);
    }
  }

  // destroy the HWND
  if (mWnd) {
    // prevent the widget from causing additional events
    mEventCallback = nsnull;
    VERIFY(::DestroyWindow(mWnd));
  }
}


//-------------------------------------------------------------------------
//
// Get this nsWindow parent
//
//-------------------------------------------------------------------------
nsIWidget* nsWindow::GetParent(void)
{
    nsWindow* widget = NULL;
    if (mWnd) {
        HWND parent = ::GetParent(mWnd);
        if (parent) {
            widget = (nsWindow *)::GetWindowLong(parent, GWL_USERDATA);
            if (widget) {
              // If the widget is in the process of being destroyed then
              // do NOT return it
              if (widget->mIsDestroying) {
                widget = NULL;
              } else {
                NS_ADDREF(widget);
              }
            }
        }
    }

    return (nsIWidget*)widget;
}


//-------------------------------------------------------------------------
//
// Get this nsWindow's list of children
//
//-------------------------------------------------------------------------
nsIEnumerator* nsWindow::GetChildren()
{
  if (mChildren) {
    // Reset the current position to 0
    mChildren->Reset();

    // XXX Does this copy of our enumerator work? It looks like only
    // the first widget in the list is added...
    Enumerator * children = new Enumerator;
    NS_ADDREF(children);
    nsISupports   * next = mChildren->Next();
    if (next) {
      nsIWidget *widget;
      if (NS_OK == next->QueryInterface(kIWidgetIID, (void**)&widget)) {
        children->Append(widget);
      }
    }

    return (nsIEnumerator*)children;
  }

  return NULL;
}


//-------------------------------------------------------------------------
//
// Add a child to the list of children
//
//-------------------------------------------------------------------------
void nsWindow::AddChild(nsIWidget* aChild)
{
  if (!mChildren) {
    mChildren = new Enumerator;
  }

  mChildren->Append(aChild);
}


//-------------------------------------------------------------------------
//
// Remove a child from the list of children
//
//-------------------------------------------------------------------------
void nsWindow::RemoveChild(nsIWidget* aChild)
{
  if (mChildren) {
    mChildren->Remove(aChild);
  }
}


//-------------------------------------------------------------------------
//
// Hide or show this component
//
//-------------------------------------------------------------------------
void nsWindow::Show(PRBool bState)
{
    if (mWnd) {
        if (bState) {
            ::SetWindowPos(mWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW);
        }
        else
            ::ShowWindow(mWnd, SW_HIDE);
    }
}

//-------------------------------------------------------------------------
//
// Move this component
//
//-------------------------------------------------------------------------
void nsWindow::Move(PRUint32 aX, PRUint32 aY)
{
    if (mWnd) {
        nsIWidget *par = GetParent();
        HDWP      deferrer = NULL;

        if (nsnull != par) {
          deferrer = ((nsWindow *)par)->mDeferredPositioner;
        }

        if (NULL != deferrer) {
            VERIFY(((nsWindow *)par)->mDeferredPositioner = ::DeferWindowPos(deferrer,
                                  mWnd, NULL, aX, aY, 0, 0,
                                  SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE));
        }
        else {
            VERIFY(::SetWindowPos(mWnd, NULL, aX, aY, 0, 0, 
                                  SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE));
        }

        NS_IF_RELEASE(par);
    }
}

//-------------------------------------------------------------------------
//
// Resize this component
//
//-------------------------------------------------------------------------
void nsWindow::Resize(PRUint32 aWidth, PRUint32 aHeight, PRBool aRepaint)
{
    if (mWnd) {
        nsIWidget *par = GetParent();
        HDWP      deferrer = NULL;

        if (nsnull != par) {
          deferrer = ((nsWindow *)par)->mDeferredPositioner;
        }

        UINT  flags = SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE;
        if (!aRepaint) {
          flags |= SWP_NOREDRAW;
        }

        if (NULL != deferrer) {
            VERIFY(((nsWindow *)par)->mDeferredPositioner = ::DeferWindowPos(deferrer,
                                    mWnd, NULL, 0, 0, aWidth, GetHeight(aHeight), flags));
        }
        else {
            VERIFY(::SetWindowPos(mWnd, NULL, 0, 0, aWidth, GetHeight(aHeight), 
                                  flags));
        }

        NS_IF_RELEASE(par);
    }
}

    
//-------------------------------------------------------------------------
//
// Resize this component
//
//-------------------------------------------------------------------------
void nsWindow::Resize(PRUint32 aX,
                      PRUint32 aY,
                      PRUint32 aWidth,
                      PRUint32 aHeight,
                      PRBool   aRepaint)
{
    if (mWnd) {
        nsIWidget *par = GetParent();
        HDWP      deferrer = NULL;

        if (nsnull != par) {
          deferrer = ((nsWindow *)par)->mDeferredPositioner;
        }

        UINT  flags = SWP_NOZORDER | SWP_NOACTIVATE;
        if (!aRepaint) {
          flags |= SWP_NOREDRAW;
        }

        if (NULL != deferrer) {
            VERIFY(((nsWindow *)par)->mDeferredPositioner = ::DeferWindowPos(deferrer,
                                    mWnd, NULL, aX, aY, aWidth, GetHeight(aHeight), 
                                    flags));
        }
        else {
            VERIFY(::SetWindowPos(mWnd, NULL, aX, aY, aWidth, GetHeight(aHeight), 
                                  flags));
        }

        NS_IF_RELEASE(par);
    }
}

    
//-------------------------------------------------------------------------
//
// Enable/disable this component
//
//-------------------------------------------------------------------------
void nsWindow::Enable(PRBool bState)
{
    if (mWnd) {
        ::EnableWindow(mWnd, bState);
    }
}

    
//-------------------------------------------------------------------------
//
// Give the focus to this component
//
//-------------------------------------------------------------------------
void nsWindow::SetFocus(void)
{
    //
    // Switch to the "main gui thread" if necessary... This method must
    // be executed on the "gui thread"...
    //
    if (!mToolkit->IsGuiThread()) {
        MethodInfo info(this, nsWindow::SET_FOCUS);
        mToolkit->CallMethod(&info);
        return;
    }

    if (mWnd) {
        ::SetFocus(mWnd);
    }
}

    
//-------------------------------------------------------------------------
//
// Get this component dimension
//
//-------------------------------------------------------------------------
void nsWindow::GetBounds(nsRect &aRect)
{
    if (mWnd) {
        RECT r;
        VERIFY(::GetClientRect(mWnd, &r));

        // assign size
        aRect.width = r.right - r.left;
        aRect.height = r.bottom - r.top;

        // convert coordinates if parent exists
        HWND parent = ::GetParent(mWnd);
        if (parent) {
          RECT pr;
          VERIFY(::GetClientRect(parent, &pr));
          VERIFY(::ClientToScreen(mWnd, (LPPOINT)&r));
          VERIFY(::ClientToScreen(parent, (LPPOINT)&pr));
          r.left -= pr.left;
          r.top -= pr.top;
        }
        aRect.x = r.left;
        aRect.y = r.top;
    }
}

//get the bounds, but don't take into account the client size

void nsWindow::GetNonClientBounds(nsRect &aRect)
{
    if (mWnd) {
        RECT r;
        VERIFY(::GetWindowRect(mWnd, &r));

        // assign size
        aRect.width = r.right - r.left;
        aRect.height = r.bottom - r.top;

        // convert coordinates if parent exists
        HWND parent = ::GetParent(mWnd);
        if (parent) {
          RECT pr;
          VERIFY(::GetWindowRect(parent, &pr));
          r.left -= pr.left;
          r.top -= pr.top;
        }
        aRect.x = r.left;
        aRect.y = r.top;
    }
}

    
//-------------------------------------------------------------------------
//
// Get the foreground color
//
//-------------------------------------------------------------------------
nscolor nsWindow::GetForegroundColor(void)
{
    return mForeground;
}

    
//-------------------------------------------------------------------------
//
// Set the foreground color
//
//-------------------------------------------------------------------------
void nsWindow::SetForegroundColor(const nscolor &aColor)
{
    mForeground = aColor;
}

    
//-------------------------------------------------------------------------
//
// Get the background color
//
//-------------------------------------------------------------------------
nscolor nsWindow::GetBackgroundColor(void)
{
    return mBackground;
}

    
//-------------------------------------------------------------------------
//
// Set the background color
//
//-------------------------------------------------------------------------
void nsWindow::SetBackgroundColor(const nscolor &aColor)
{
    mBackground = aColor;

    if (mBrush)
      ::DeleteObject(mBrush);

    mBrush = ::CreateSolidBrush(NSRGB_2_COLOREF(mBackground));
    if (mWnd != NULL) {
      SetClassLong(mWnd, GCL_HBRBACKGROUND, (LONG)mBrush);
    }
}

    
//-------------------------------------------------------------------------
//
// Get this component font
//
//-------------------------------------------------------------------------
nsIFontMetrics* nsWindow::GetFont(void)
{
    NS_NOTYETIMPLEMENTED("GetFont not yet implemented"); // to be implemented
    return NULL;
}

    
//-------------------------------------------------------------------------
//
// Set this component font
//
//-------------------------------------------------------------------------
void nsWindow::SetFont(const nsFont &aFont)
{
    nsIFontCache* fontCache;
    mContext->GetFontCache(fontCache);
    nsIFontMetrics* metrics;
    fontCache->GetMetricsFor(aFont, metrics);
    nsFontHandle  fontHandle;
    metrics->GetFontHandle(fontHandle);
    HFONT hfont = (HFONT)fontHandle;

      // Draw in the new font
    ::SendMessage(mWnd, WM_SETFONT, (WPARAM)hfont, (LPARAM)0); 
    NS_RELEASE(metrics);
    NS_RELEASE(fontCache);
}

    
//-------------------------------------------------------------------------
//
// Get this component cursor
//
//-------------------------------------------------------------------------
nsCursor nsWindow::GetCursor()
{
    return mCursor;
}

    
//-------------------------------------------------------------------------
//
// Set this component cursor
//
//-------------------------------------------------------------------------

#define DLLQUOTE(x) #x
#define DLLNAME(x) DLLQUOTE(x)

void nsWindow::SetCursor(nsCursor aCursor)
{
  // Only change cursor if it's changing
  if (aCursor != mCursor) {
    HCURSOR newCursor = NULL;
    HMODULE hm ;

    switch(aCursor) {
    case eCursor_select:
      newCursor = ::LoadCursor(NULL, IDC_IBEAM);
      break;
      
    case eCursor_wait:
      newCursor = ::LoadCursor(NULL, IDC_WAIT);
      break;

    case eCursor_hyperlink: {
      hm = ::GetModuleHandle(DLLNAME(NS_DLLNAME));
      newCursor = ::LoadCursor(hm, MAKEINTRESOURCE(IDC_SELECTANCHOR));
      break;
    }

    case eCursor_standard:
      newCursor = ::LoadCursor(NULL, IDC_ARROW);
      break;

    case eCursor_sizeWE:
      newCursor = ::LoadCursor(NULL, IDC_SIZEWE);
      break;

    case eCursor_sizeNS:
      newCursor = ::LoadCursor(NULL, IDC_SIZENS);
      break;

    case eCursor_arrow_north:
      hm = ::GetModuleHandle(DLLNAME(NS_DLLNAME));
      newCursor = ::LoadCursor(hm, MAKEINTRESOURCE(IDC_ARROWNORTH));
      break;

    case eCursor_arrow_north_plus:
      hm = ::GetModuleHandle(DLLNAME(NS_DLLNAME));
      newCursor = ::LoadCursor(hm, MAKEINTRESOURCE(IDC_ARROWNORTHPLUS));
      break;

    case eCursor_arrow_south:
      hm = ::GetModuleHandle(DLLNAME(NS_DLLNAME));
      newCursor = ::LoadCursor(hm, MAKEINTRESOURCE(IDC_ARROWSOUTH));
      break;

    case eCursor_arrow_south_plus:
      hm = ::GetModuleHandle(DLLNAME(NS_DLLNAME));
      newCursor = ::LoadCursor(hm, MAKEINTRESOURCE(IDC_ARROWSOUTHPLUS));
      break;

    case eCursor_arrow_east:
      hm = ::GetModuleHandle(DLLNAME(NS_DLLNAME));
      newCursor = ::LoadCursor(hm, MAKEINTRESOURCE(IDC_ARROWEAST));
      break;

    case eCursor_arrow_east_plus:
      hm = ::GetModuleHandle(DLLNAME(NS_DLLNAME));
      newCursor = ::LoadCursor(hm, MAKEINTRESOURCE(IDC_ARROWEASTPLUS));
      break;

    case eCursor_arrow_west:
      hm = ::GetModuleHandle(DLLNAME(NS_DLLNAME));
      newCursor = ::LoadCursor(hm, MAKEINTRESOURCE(IDC_ARROWWEST));
      break;

    case eCursor_arrow_west_plus:
      hm = ::GetModuleHandle(DLLNAME(NS_DLLNAME));
      newCursor = ::LoadCursor(hm, MAKEINTRESOURCE(IDC_ARROWWESTPLUS));
      break;

    default:
      NS_ASSERTION(0, "Invalid cursor type");
      break;
    }

    if (NULL != newCursor) {
      mCursor = aCursor;
      HCURSOR oldCursor = ::SetCursor(newCursor);
    }
  }
}
    
//-------------------------------------------------------------------------
//
// Invalidate this component visible area
//
//-------------------------------------------------------------------------
void nsWindow::Invalidate(PRBool aIsSynchronous)
{
    if (mWnd) {
        VERIFY(::InvalidateRect(mWnd, NULL, TRUE));
        if (aIsSynchronous) {
          VERIFY(::UpdateWindow(mWnd));
        }
    }
}


//-------------------------------------------------------------------------
//
// Return some native data according to aDataType
//
//-------------------------------------------------------------------------
void* nsWindow::GetNativeData(PRUint32 aDataType)
{
    switch(aDataType) {
        case NS_NATIVE_WIDGET:
        case NS_NATIVE_WINDOW:
            return (void*)mWnd;
        case NS_NATIVE_GRAPHIC:
            // XXX:  This is sleezy!!  Remember to Release the DC after using it!
            return (void*)::GetDC(mWnd);
        case NS_NATIVE_COLORMAP:
        default:
            break;
    }

    return NULL;
}


//-------------------------------------------------------------------------
//
// Create a rendering context from this nsWindow
//
//-------------------------------------------------------------------------
nsIRenderingContext* nsWindow::GetRenderingContext()
{
nsRect  bounds;

    nsIRenderingContext *renderingCtx = NULL;
    if (mWnd) {
        nsresult  res;

        static NS_DEFINE_IID(kRenderingContextCID, NS_RENDERING_CONTEXT_CID);
        static NS_DEFINE_IID(kRenderingContextIID, NS_IRENDERING_CONTEXT_IID);

        res = NSRepository::CreateInstance(kRenderingContextCID, nsnull, kRenderingContextIID, (void **)&renderingCtx);

        if (NS_OK == res)
          renderingCtx->Init(mContext, this);

        NS_ASSERTION(NULL != renderingCtx, "Null rendering context");
    }

    return renderingCtx;
}

//-------------------------------------------------------------------------
//
// Return the toolkit this widget was created on
//
//-------------------------------------------------------------------------
nsIToolkit* nsWindow::GetToolkit()
{
  NS_IF_ADDREF(mToolkit);
  return mToolkit;
}


//-------------------------------------------------------------------------
//
// Set the colormap of the window
//
//-------------------------------------------------------------------------
void nsWindow::SetColorMap(nsColorMap *aColorMap)
{
    if (mPalette != NULL) {
        ::DeleteObject(mPalette);
    }

    PRUint8 *map = aColorMap->Index;
    LPLOGPALETTE pLogPal = (LPLOGPALETTE) new char[2 * sizeof(WORD) +
                                              aColorMap->NumColors * sizeof(PALETTEENTRY)];
	pLogPal->palVersion = 0x300;
	pLogPal->palNumEntries = aColorMap->NumColors;
	for(int i = 0; i < aColorMap->NumColors; i++) 
    {
		pLogPal->palPalEntry[i].peRed = *map++;
		pLogPal->palPalEntry[i].peGreen = *map++;
		pLogPal->palPalEntry[i].peBlue = *map++;
		pLogPal->palPalEntry[i].peFlags = 0;
	}
	mPalette = ::CreatePalette(pLogPal);
	delete pLogPal;

    NS_ASSERTION(mPalette != NULL, "Null palette");
    if (mPalette != NULL) {
        HDC hDC = ::GetDC(mWnd);
        HPALETTE hOldPalette = ::SelectPalette(hDC, mPalette, TRUE);
        ::RealizePalette(hDC);
        ::SelectPalette(hDC, hOldPalette, TRUE);
        ::ReleaseDC(mWnd, hDC);
    }
}

//-------------------------------------------------------------------------
//
// Return the used device context
//
//-------------------------------------------------------------------------
nsIDeviceContext* nsWindow::GetDeviceContext() 
{
  NS_IF_ADDREF(mContext);
  return mContext; 
}

//-------------------------------------------------------------------------
//
// Return the App Shell
//
//-------------------------------------------------------------------------

nsIAppShell *nsWindow::GetAppShell()
{
    NS_IF_ADDREF(mAppShell);
    return mAppShell;
}

//-------------------------------------------------------------------------
//
// Scroll the bits of a window
//
//-------------------------------------------------------------------------
void nsWindow::Scroll(PRInt32 aDx, PRInt32 aDy, nsRect *aClipRect)
{
  RECT  trect;

  if (nsnull != aClipRect)
  {
    trect.left = aClipRect->x;
    trect.top = aClipRect->y;
    trect.right = aClipRect->XMost();
    trect.bottom = aClipRect->YMost();
  }

  ::ScrollWindowEx(mWnd, aDx, aDy, (nsnull != aClipRect) ? &trect : NULL, NULL,
                   NULL, NULL, SW_INVALIDATE | SW_SCROLLCHILDREN);
  ::UpdateWindow(mWnd);
}


//-------------------------------------------------------------------------
//
// Every function that needs a thread switch goes through this function
// by calling SendMessage (..WM_CALLMETHOD..) in nsToolkit::CallMethod.
//
//-------------------------------------------------------------------------
BOOL nsWindow::CallMethod(MethodInfo *info)
{
    BOOL bRet = TRUE;

    switch (info->methodId) {
        case nsWindow::CREATE:
            NS_ASSERTION(info->nArgs == 7, "Wrong number of arguments to CallMethod");
            Create((nsIWidget*)(info->args[0]), 
                        (nsRect&)*(nsRect*)(info->args[1]), 
                        (EVENT_CALLBACK)(info->args[2]), 
                        (nsIDeviceContext*)(info->args[3]),
                        (nsIAppShell *)(info->args[4]),
                        (nsIToolkit*)(info->args[5]),
                        (nsWidgetInitData*)(info->args[6]));
            break;

        case nsWindow::CREATE_NATIVE:
            NS_ASSERTION(info->nArgs == 7, "Wrong number of arguments to CallMethod");
            Create((nsNativeWidget)(info->args[0]), 
                        (nsRect&)*(nsRect*)(info->args[1]), 
                        (EVENT_CALLBACK)(info->args[2]), 
                        (nsIDeviceContext*)(info->args[3]),
                        (nsIAppShell *)(info->args[4]),
                        (nsIToolkit*)(info->args[5]),
                        (nsWidgetInitData*)(info->args[6]));
            return TRUE;

        case nsWindow::DESTROY:
            NS_ASSERTION(info->nArgs == 0, "Wrong number of arguments to CallMethod");
            Destroy();
            break;

        case nsWindow::SET_FOCUS:
            NS_ASSERTION(info->nArgs == 0, "Wrong number of arguments to CallMethod");
            SetFocus();
            break;

        default:
            bRet = FALSE;
            break;
    }

    return bRet;
}

//-------------------------------------------------------------------------
//
// OnKey
//
//-------------------------------------------------------------------------
PRBool nsWindow::OnKey(PRUint32 aEventType, PRUint32 aKeyCode)
{
    nsKeyEvent event;
    InitEvent(event, aEventType);
    event.keyCode   = aKeyCode;
    event.isShift   = mIsShiftDown;
    event.isControl = mIsControlDown;
    event.isAlt     = mIsAltDown;
    event.eventStructType = NS_KEY_EVENT;
    return(DispatchEvent(&event));
}

//-------------------------------------------------------------------------
void nsWindow::SetUpForPaint(HDC aHDC) 
{
  ::SetBkColor (aHDC, NSRGB_2_COLOREF(mBackground));
  ::SetTextColor(aHDC, NSRGB_2_COLOREF(mForeground));
  ::SetBkMode (aHDC, TRANSPARENT);
}

//-------------------------------------------------------------------------
//
// Process all nsWindows messages
//
//-------------------------------------------------------------------------
PRBool nsWindow::ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *aRetValue)
{
    PRBool        result = PR_FALSE; // call the default nsWindow proc
    nsPaletteInfo palInfo;
            
    *aRetValue = 0;

    switch (msg) {

        case WM_COMMAND: {
          WORD wNotifyCode = HIWORD(wParam); // notification code 
          if (wNotifyCode == 0) { // Menu selection
            nsMenuEvent event;
            event.menuItem = LOWORD(wParam);
            event.eventStructType = NS_MENU_EVENT;
            InitEvent(event, NS_MENU_SELECTED);
            result = DispatchEvent(&event);
          }
        }
        break;

        
        case WM_NOTIFY:
            // TAB change
          {
            LPNMHDR pnmh = (LPNMHDR) lParam;

            switch (pnmh->code) {
              case TCN_SELCHANGE: {
                DispatchStandardEvent(NS_TABCHANGE);
                result = PR_TRUE;
              }
              break;

              case TTN_SHOW: {
                  nsTooltipEvent event;
                  InitEvent(event, NS_SHOW_TOOLTIP);
                  event.tipIndex = (PRUint32)wParam;
                  event.eventStructType = NS_TOOLTIP_EVENT;
                  result = DispatchEvent(&event);
              }
              break;

              case TTN_POP:
                result = DispatchStandardEvent(NS_HIDE_TOOLTIP);
                break;
            }
          }
          break;

        case WM_MOVE: // Window moved 
          {
            PRInt32 x = (PRInt32)LOWORD(lParam); // horizontal position in screen coordinates 
            PRInt32 y = (PRInt32)HIWORD(lParam); // vertical position in screen coordinates 
            result = OnMove(x, y); 
          }
          break;

        case WM_DESTROY:
            // clean up.
            OnDestroy();
            result = PR_TRUE;
            break;

        case WM_PAINT:
            result = OnPaint();
            break;

        case WM_KEYUP: 
            if (wParam == NS_VK_SHIFT) {
              mIsShiftDown = PR_FALSE;
            }
            if (wParam == NS_VK_CONTROL) {
              mIsControlDown = PR_FALSE;
            }
            if (wParam == NS_VK_ALT) {
              mIsAltDown = PR_FALSE;
            }
            result = OnKey(NS_KEY_UP, wParam);
            break;

        case WM_KEYDOWN:
            if (wParam == NS_VK_SHIFT) {
              mIsShiftDown = PR_TRUE;
            }
            if (wParam == NS_VK_CONTROL) {
              mIsControlDown = PR_TRUE;
            }
            if (wParam == NS_VK_ALT) {
              mIsAltDown = PR_TRUE;
            }
            result = OnKey(NS_KEY_DOWN, wParam);
            break;

        // say we've dealt with erase background if widget does
        // not need auto-erasing
        case WM_ERASEBKGND: 
            if (! AutoErase()) {
              *aRetValue = 1;
              result = PR_TRUE;
            } 
            break;

        case WM_MOUSEMOVE:
            //RelayMouseEvent(msg,wParam, lParam); 
            result = DispatchMouseEvent(NS_MOUSE_MOVE);
            break;

        case WM_LBUTTONDOWN:
            //RelayMouseEvent(msg,wParam, lParam); 
            result = DispatchMouseEvent(NS_MOUSE_LEFT_BUTTON_DOWN);
            break;

        case WM_LBUTTONUP:
            //RelayMouseEvent(msg,wParam, lParam); 
            result = DispatchMouseEvent(NS_MOUSE_LEFT_BUTTON_UP);
            break;

        case WM_LBUTTONDBLCLK:
            result = DispatchMouseEvent(NS_MOUSE_LEFT_BUTTON_DOWN);
            if (result == PR_FALSE)
              result = DispatchMouseEvent(NS_MOUSE_LEFT_DOUBLECLICK);
            break;

        case WM_MBUTTONDOWN:
            result = DispatchMouseEvent(NS_MOUSE_MIDDLE_BUTTON_DOWN);
            break;

        case WM_MBUTTONUP:
            result = DispatchMouseEvent(NS_MOUSE_MIDDLE_BUTTON_UP);
            break;

        case WM_MBUTTONDBLCLK:
            result = DispatchMouseEvent(NS_MOUSE_MIDDLE_BUTTON_DOWN);           
            break;

        case WM_RBUTTONDOWN:
            result = DispatchMouseEvent(NS_MOUSE_RIGHT_BUTTON_DOWN);            
            break;

        case WM_RBUTTONUP:
            result = DispatchMouseEvent(NS_MOUSE_RIGHT_BUTTON_UP);
            break;

        case WM_RBUTTONDBLCLK:
            result = DispatchMouseEvent(NS_MOUSE_RIGHT_BUTTON_DOWN);
            if (result == PR_FALSE)
              result = DispatchMouseEvent(NS_MOUSE_RIGHT_DOUBLECLICK);                      
            break;

        case WM_HSCROLL:
        case WM_VSCROLL:
	          // check for the incoming nsWindow handle to be null in which case
	          // we assume the message is coming from a horizontal scrollbar inside
	          // a listbox and we don't bother processing it (well, we don't have to)
	          if (lParam) {
                nsWindow* scrollbar = (nsWindow*)::GetWindowLong((HWND)lParam, GWL_USERDATA);

		            if (scrollbar) {
		                result = scrollbar->OnScroll(LOWORD(wParam), (short)HIWORD(wParam));
		            }
	          }
            break;

        case WM_CTLCOLORLISTBOX:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLORSCROLLBAR:
        case WM_CTLCOLORSTATIC:
	          if (lParam) {
              nsWindow* control = (nsWindow*)::GetWindowLong((HWND)lParam, GWL_USERDATA);
		          if (control) {
                control->SetUpForPaint((HDC)wParam);
		            *aRetValue = (LPARAM)control->OnControlColor();
              }
	          }
    
            result = PR_TRUE;
            break;

        case WM_SETFOCUS:
            result = DispatchFocus(NS_GOTFOCUS);
            break;

        case WM_KILLFOCUS:
            result = DispatchFocus(NS_LOSTFOCUS);
            break;

        case WM_WINDOWPOSCHANGED: 
        {
            WINDOWPOS *wp = (LPWINDOWPOS)lParam;

            // We only care about a resize, so filter out things like z-order
            // changes. Note: there's a WM_MOVE handler above which is why we're
            // not handling them here...
            if (0 == (wp->flags & SWP_NOSIZE)) {
              // XXX Why are we using the client size area? If the size notification
              // is for the client area then the origin should be (0,0) and not
              // the window origin in screen coordinates...
              RECT r;
              ::GetClientRect(mWnd, &r);
              PRInt32 newWidth, newHeight;
              newWidth = PRInt32(r.right - r.left);
              newHeight = PRInt32(r.bottom - r.top);
              nsRect rect(wp->x, wp->y, newWidth, newHeight);
              if (newWidth > mWidth)
              {
                RECT drect;

                //getting wider

                drect.left = wp->x + mWidth;
                drect.top = wp->y;
                drect.right = drect.left + (newWidth - mWidth);
                drect.bottom = drect.top + newHeight;

//                ::InvalidateRect(mWnd, &drect, FALSE);
                ::RedrawWindow(mWnd, &drect, NULL,
                               RDW_INVALIDATE | RDW_NOERASE | RDW_NOINTERNALPAINT | RDW_ERASENOW | RDW_ALLCHILDREN);
              }
              if (newHeight > mHeight)
              {
                RECT drect;

                //getting taller

                drect.left = wp->x;
                drect.top = wp->y + mHeight;
                drect.right = drect.left + newWidth;
                drect.bottom = drect.top + (newHeight - mHeight);

//                ::InvalidateRect(mWnd, &drect, FALSE);
                ::RedrawWindow(mWnd, &drect, NULL,
                               RDW_INVALIDATE | RDW_NOERASE | RDW_NOINTERNALPAINT | RDW_ERASENOW | RDW_ALLCHILDREN);
              }
              mWidth = newWidth;
              mHeight = newHeight;
              ///nsRect rect(wp->x, wp->y, wp->cx, wp->cy);
              result = OnResize(rect);
            }
            break;
        }

        case WM_PALETTECHANGED:
            if ((HWND)wParam == mWnd) {
                // We caused the WM_PALETTECHANGED message so avoid realizing
                // another foreground palette
                result = PR_TRUE;
                break;
            }
            // fall thru...

        case WM_QUERYNEWPALETTE:
            mContext->GetPaletteInfo(palInfo);
            if (palInfo.isPaletteDevice && palInfo.palette) {
                HDC hDC = ::GetDC(mWnd);
                HPALETTE hOldPal = ::SelectPalette(hDC, (HPALETTE)palInfo.palette, FALSE);
                
                // Realize the drawing palette
                int i = ::RealizePalette(hDC);

                // Did any of our colors change?
                if (i > 0) {
                  // Yes, so repaint
                  ::InvalidateRect(mWnd, (LPRECT)NULL, TRUE);
                }

                ::SelectPalette(hDC, hOldPal, TRUE);
                ::RealizePalette(hDC);
                ::ReleaseDC(mWnd, hDC);
                *aRetValue = TRUE;
            }
            result = PR_TRUE;
            break;
    }

    return result;
}



//-------------------------------------------------------------------------
//
// return the window class name and initialize the class if needed
//
//-------------------------------------------------------------------------
LPCTSTR nsWindow::WindowClass()
{
    const LPCTSTR className = "NetscapeWindowClass";

    if (!nsWindow::sIsRegistered) {
        WNDCLASS wc;

//        wc.style            = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wc.style            = CS_DBLCLKS;
        wc.lpfnWndProc      = ::DefWindowProc;
        wc.cbClsExtra       = 0;
        wc.cbWndExtra       = 0;
        wc.hInstance        = nsToolkit::mDllInstance;
        wc.hIcon            = ::LoadIcon(NULL, IDI_APPLICATION);
        wc.hCursor          = NULL;
        wc.hbrBackground    = mBrush;
        wc.lpszMenuName     = NULL;
        wc.lpszClassName    = className;
    
        nsWindow::sIsRegistered = ::RegisterClass(&wc);
    }

    return className;
}


//-------------------------------------------------------------------------
//
// return nsWindow styles
//
//-------------------------------------------------------------------------
DWORD nsWindow::WindowStyle()
{
    return WS_OVERLAPPEDWINDOW;
}


//-------------------------------------------------------------------------
//
// return nsWindow extended styles
//
//-------------------------------------------------------------------------
DWORD nsWindow::WindowExStyle()
{
    return WS_EX_CLIENTEDGE;
}


// -----------------------------------------------------------------------
//
// Subclass (or remove the subclass from) this component's nsWindow
//
// -----------------------------------------------------------------------
void nsWindow::SubclassWindow(BOOL bState)
{
    NS_PRECONDITION(::IsWindow(mWnd), "Invalid window handle");
    
    if (bState) {
        // change the nsWindow proc
        mPrevWndProc = (WNDPROC)::SetWindowLong(mWnd, GWL_WNDPROC, 
                                                 (LONG)nsWindow::WindowProc);
        NS_ASSERTION(mPrevWndProc, "Null standard window procedure");
        // connect the this pointer to the nsWindow handle
        ::SetWindowLong(mWnd, GWL_USERDATA, (LONG)this);
    } 
    else {
        ::SetWindowLong(mWnd, GWL_WNDPROC, (LONG)mPrevWndProc);
        ::SetWindowLong(mWnd, GWL_USERDATA, (LONG)NULL);
        mPrevWndProc = NULL;
    }
}


//-------------------------------------------------------------------------
//
// WM_DESTROY has been called
//
//-------------------------------------------------------------------------
void nsWindow::OnDestroy()
{
    SubclassWindow(FALSE);
    mWnd = NULL;

    // free GDI objects
    if (mBrush) {
      VERIFY(::DeleteObject(mBrush));
      mBrush = NULL;
    }
    if (mPalette) {
      VERIFY(::DeleteObject(mPalette));
      mPalette = NULL;
    }

    // free tooltip window
    if (mTooltip) {
      VERIFY(::DestroyWindow(mTooltip));
      mTooltip = NULL;
    }

    // if we were in the middle of deferred window positioning then
    // free the memory for the multiple-window position structure
    if (mDeferredPositioner) {
      VERIFY(::EndDeferWindowPos(mDeferredPositioner));
      mDeferredPositioner = NULL;
    }

    // release references to children, device context, toolkit, and app shell
    NS_IF_RELEASE(mChildren);
    NS_IF_RELEASE(mContext);
    NS_IF_RELEASE(mToolkit);
    NS_IF_RELEASE(mAppShell);

    // dispatch the event
    if (!mIsDestroying) {
      // dispatching of the event may cause the reference count to drop to 0
      // and result in this object being destroyed. To avoid that, add a reference
      // and then release it after dispatching the event
      AddRef();
      DispatchStandardEvent(NS_DESTROY);
      Release();
    }
}

//-------------------------------------------------------------------------
//
// Move
//
//-------------------------------------------------------------------------
PRBool nsWindow::OnMove(PRInt32 aX, PRInt32 aY)
{            
  nsGUIEvent event;
  InitEvent(event, NS_MOVE);
  event.point.x = aX;
  event.point.y = aY;
  event.eventStructType = NS_GUI_EVENT;
  return DispatchEvent(&event);
}

//-------------------------------------------------------------------------
//
// Paint
//
//-------------------------------------------------------------------------
PRBool nsWindow::OnPaint()
{
    nsRect    bounds;
    PRBool result = PR_TRUE;
    PAINTSTRUCT ps;
    HDC hDC = ::BeginPaint(mWnd, &ps);

    // XXX What is this check doing? If it's trying to check for an empty
    // paint rect then use the IsRectEmpty() function...
    if (ps.rcPaint.left || ps.rcPaint.right || ps.rcPaint.top || ps.rcPaint.bottom) {
        // call the event callback 
        if (mEventCallback) {
            nsPaintEvent event;

            InitEvent(event, NS_PAINT);

            nsRect rect(ps.rcPaint.left, 
                        ps.rcPaint.top, 
                        ps.rcPaint.right - ps.rcPaint.left, 
                        ps.rcPaint.bottom - ps.rcPaint.top);
            event.rect = &rect;
            event.eventStructType = NS_PAINT_EVENT;

            ::EndPaint(mWnd, &ps);

            static NS_DEFINE_IID(kRenderingContextCID, NS_RENDERING_CONTEXT_CID);
            static NS_DEFINE_IID(kRenderingContextIID, NS_IRENDERING_CONTEXT_IID);

            if (NS_OK == NSRepository::CreateInstance(kRenderingContextCID, nsnull, kRenderingContextIID, (void **)&event.renderingContext))
            {
              event.renderingContext->Init(mContext, this);
              result = DispatchEvent(&event);
              NS_RELEASE(event.renderingContext);
            }
            else
              result = PR_FALSE;
        }
        else
            ::EndPaint(mWnd, &ps);
    }
    else
        ::EndPaint(mWnd, &ps);

//    ::EndPaint(mWnd, &ps);

    return result;
}


//-------------------------------------------------------------------------
//
// Send a resize message to the listener
//
//-------------------------------------------------------------------------
PRBool nsWindow::OnResize(nsRect &aWindowRect)
{
    // call the event callback 
    if (mEventCallback) {
        nsSizeEvent event;
        InitEvent(event, NS_SIZE);
        event.windowSize = &aWindowRect;
        event.eventStructType = NS_SIZE_EVENT;
        return(DispatchEvent(&event));
    }

    return PR_FALSE;
}



//-------------------------------------------------------------------------
//
// Deal with all sort of mouse event
//
//-------------------------------------------------------------------------
PRBool nsWindow::DispatchMouseEvent(PRUint32 aEventType, nsPoint* aPoint)
{
  PRBool result = PR_FALSE;

  if (nsnull == mEventCallback && nsnull == mMouseListener) {
    return result;
  }

  nsMouseEvent event;
  InitEvent(event, aEventType, aPoint);

  event.isShift   = GetKeyState(VK_LSHIFT) < 0   || GetKeyState(VK_RSHIFT) < 0;
  event.isControl = GetKeyState(VK_LCONTROL) < 0 || GetKeyState(VK_RCONTROL) < 0;
  event.isAlt     = GetKeyState(VK_LMENU) < 0    || GetKeyState(VK_RMENU) < 0;
  event.clickCount = (aEventType == NS_MOUSE_LEFT_DOUBLECLICK ||
                      aEventType == NS_MOUSE_LEFT_DOUBLECLICK)? 2:1;
  event.eventStructType = NS_MOUSE_EVENT;

  // call the event callback 
  if (nsnull != mEventCallback) {
//printf(" %d %d %d (%d,%d) \n", event.widget, event.widgetSupports, 
//       event.message, event.point.x, event.point.y);

    result = DispatchEvent(&event);

    //printf("**result=%d%\n",result);
    if (aEventType == NS_MOUSE_MOVE) {

      MouseTrailer * mouseTrailer = MouseTrailer::GetMouseTrailer(0);
      MouseTrailer::SetMouseTrailerWindow(this);
      mouseTrailer->CreateTimer();

      nsRect rect;
      GetBounds(rect);
      rect.x = 0;
      rect.y = 0;
      //printf("Rect[%d, %d, %d, %d]  Point[%d,%d]\n", rect.x, rect.y, rect.width, rect.height, event.position.x, event.position.y);
      //printf("gCurrentWindow 0x%X\n", gCurrentWindow);

      if (rect.Contains(event.point.x, event.point.y)) {
        if (gCurrentWindow == NULL || gCurrentWindow != this) {
          if ((nsnull != gCurrentWindow) && (!gCurrentWindow->mIsDestroying)) {
            MouseTrailer::IgnoreNextCycle();
            gCurrentWindow->DispatchMouseEvent(NS_MOUSE_EXIT, gCurrentWindow->GetLastPoint());
          }
          gCurrentWindow = this;
          if (!mIsDestroying) {
            gCurrentWindow->DispatchMouseEvent(NS_MOUSE_ENTER);
          }
        }
      } 
    } else if (aEventType == NS_MOUSE_EXIT) {
      if (gCurrentWindow == this) {
        gCurrentWindow = nsnull;
      }
    }

    return result;
  }

  if (nsnull != mMouseListener) {
    switch (aEventType) {
      case NS_MOUSE_MOVE: {
        result = ConvertStatus(mMouseListener->MouseMoved(event));
        nsRect rect;
        GetBounds(rect);
        if (rect.Contains(event.point.x, event.point.y)) {
          if (gCurrentWindow == NULL || gCurrentWindow != this) {
            //printf("Mouse enter");
            gCurrentWindow = this;
          }
        } else {
          //printf("Mouse exit");
        }

      } break;

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

//-------------------------------------------------------------------------
//
// Deal with focus messages
//
//-------------------------------------------------------------------------
PRBool nsWindow::DispatchFocus(PRUint32 aEventType)
{
    // call the event callback 
    if (mEventCallback) {
      if ((nsnull != gCurrentWindow) && (!gCurrentWindow->mIsDestroying)) {
        return(DispatchStandardEvent(aEventType));
      }
    }

    return PR_FALSE;
}


//-------------------------------------------------------------------------
//
// Deal with scrollbar messages (actually implemented only in nsScrollbar)
//
//-------------------------------------------------------------------------
PRBool nsWindow::OnScroll(UINT scrollCode, int cPos)
{
    return PR_FALSE;
}


//-------------------------------------------------------------------------
//
// Return the brush used to paint the background of this control 
//
//-------------------------------------------------------------------------
HBRUSH nsWindow::OnControlColor()
{
    return mBrush;
}


//-------------------------------------------------------------------------
//
// Constructor
//
//-------------------------------------------------------------------------

nsWindow::Enumerator::Enumerator()
{
  mRefCnt = 1;
  mCurrentPosition = 0;
}


//-------------------------------------------------------------------------
//
// Destructor
//
//-------------------------------------------------------------------------
nsWindow::Enumerator::~Enumerator()
{   
  // We add ref'd when adding the child widgets, so we need to release
  // the references now
  for (PRInt32 i = 0; i < mChildren.Count(); i++) {
    nsIWidget*  widget = (nsIWidget*)mChildren.ElementAt(i);

    NS_ASSERTION(nsnull != widget, "null widget pointer");
    NS_IF_RELEASE(widget);
  }
}

//
// nsISupports implementation macro
//
NS_IMPL_ISUPPORTS(nsWindow::Enumerator, NS_IENUMERATOR_IID);

//-------------------------------------------------------------------------
//
// Get enumeration next element. Return null at the end
//
//-------------------------------------------------------------------------
nsISupports* nsWindow::Enumerator::Next()
{
  if (mCurrentPosition < mChildren.Count()) {
    nsIWidget*  widget = (nsIWidget*)mChildren.ElementAt(mCurrentPosition);

    mCurrentPosition++;
    NS_IF_ADDREF(widget);
    return widget;
  }

  return NULL;
}


//-------------------------------------------------------------------------
//
// Reset enumerator internal pointer to the beginning
//
//-------------------------------------------------------------------------
void nsWindow::Enumerator::Reset()
{
  mCurrentPosition = 0;
}


//-------------------------------------------------------------------------
//
// Append an element 
//
//-------------------------------------------------------------------------
void nsWindow::Enumerator::Append(nsIWidget* aWidget)
{
  NS_PRECONDITION(aWidget, "Null widget");
  if (nsnull != aWidget) {
    mChildren.AppendElement(aWidget);
    NS_ADDREF(aWidget);
  }
}


//-------------------------------------------------------------------------
//
// Remove an element 
//
//-------------------------------------------------------------------------
void nsWindow::Enumerator::Remove(nsIWidget* aWidget)
{
  if (mChildren.RemoveElement(aWidget)) {
    NS_RELEASE(aWidget);
  }
}

//-------------------------------------------------------------------------
//
// return the style for a child nsWindow
//
//-------------------------------------------------------------------------
DWORD ChildWindow::WindowStyle()
{
    return WS_CHILD | WS_CLIPCHILDREN | GetBorderStyle(mBorderStyle);
}

//-------------------------------------------------------------------------
//
// Deal with all sort of mouse event
//
//-------------------------------------------------------------------------
PRBool ChildWindow::DispatchMouseEvent(PRUint32 aEventType, nsPoint* aPoint)
{
  PRBool result = PR_FALSE;

  if (nsnull == mEventCallback && nsnull == mMouseListener) {
    return result;
  }

  switch (aEventType) {
    case NS_MOUSE_LEFT_BUTTON_DOWN:
    case NS_MOUSE_MIDDLE_BUTTON_DOWN:
    case NS_MOUSE_RIGHT_BUTTON_DOWN:
      SetCapture(mWnd);
      break;

    case NS_MOUSE_LEFT_BUTTON_UP:
    case NS_MOUSE_MIDDLE_BUTTON_UP:
    case NS_MOUSE_RIGHT_BUTTON_UP:
      ReleaseCapture();
      break;

    default:
      break;

  } // switch

  return nsWindow::DispatchMouseEvent(aEventType, aPoint);
}

DWORD nsWindow::GetBorderStyle(nsBorderStyle aBorderStyle)
{
  switch(aBorderStyle)
  {
    case eBorderStyle_none:
      return(0);
    break;

    case eBorderStyle_dialog:
     return(WS_DLGFRAME | DS_3DLOOK);
    break;

    default:
      NS_ASSERTION(0, "unknown border style");
      return(WS_OVERLAPPEDWINDOW);
  }
}


void nsWindow::SetBorderStyle(nsBorderStyle aBorderStyle) 
{
  mBorderStyle = aBorderStyle; 
} 

void nsWindow::SetTitle(const nsString& aTitle) 
{
  NS_ALLOC_STR_BUF(buf, aTitle, 256);
  ::SendMessage(mWnd, WM_SETTEXT, (WPARAM)0, (LPARAM)(LPCTSTR)buf);
  NS_FREE_STR_BUF(buf);
} 


/**
 * Processes a mouse pressed event
 *
 **/
void nsWindow::AddMouseListener(nsIMouseListener * aListener)
{
  NS_PRECONDITION(mMouseListener == nsnull, "Null mouse listener");
  mMouseListener = aListener;
}

/**
 * Processes a mouse pressed event
 *
 **/
void nsWindow::AddEventListener(nsIEventListener * aListener)
{
  NS_PRECONDITION(mEventListener == nsnull, "Null mouse listener");
  mEventListener = aListener;
}

PRBool nsWindow::AutoErase()
{
  return(PR_FALSE);
}



