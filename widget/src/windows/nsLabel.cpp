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

#include "nsLabel.h"
#include "nsILabel.h"
#include "nsToolkit.h"
#include "nsColor.h"
#include "nsGUIEvent.h"
#include "nsString.h"
#include "nsStringUtil.h"
#include "nsIFontMetrics.h"
#include "nsIDeviceContext.h"
#include <windows.h>

NS_IMPL_ADDREF(nsLabel)
NS_IMPL_RELEASE(nsLabel)

//-------------------------------------------------------------------------
//
// nsLabel constructor
//
//-------------------------------------------------------------------------
nsLabel::nsLabel() : nsWindow(), nsILabel()
{
  NS_INIT_REFCNT();
  mAlignment = eAlign_Left;
}

//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
NS_METHOD nsLabel::PreCreateWidget(nsWidgetInitData *aInitData)
{
  if (nsnull != aInitData) {
    nsLabelInitData* data = (nsLabelInitData *) aInitData;
    mAlignment = data->mAlignment;
  }
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// nsLabel destructor
//
//-------------------------------------------------------------------------
nsLabel::~nsLabel()
{
}

//-------------------------------------------------------------------------
//
// Query interface implementation
//
//-------------------------------------------------------------------------
nsresult nsLabel::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  nsresult result = nsWindow::QueryInterface(aIID, aInstancePtr);

  static NS_DEFINE_IID(kILabelIID, NS_ILABEL_IID);
  if (result == NS_NOINTERFACE && aIID.Equals(kILabelIID)) {
      *aInstancePtr = (void*) ((nsILabel*)this);
      NS_ADDREF_THIS();
      result = NS_OK;
  }

  return result;
}

//-------------------------------------------------------------------------
//
// Set this button label
//
//-------------------------------------------------------------------------
NS_METHOD nsLabel::SetAlignment(nsLabelAlignment aAlignment)
{
  mAlignment = aAlignment;
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Set this button label
//
//-------------------------------------------------------------------------
NS_METHOD nsLabel::SetLabel(const nsString& aText)
{
  NS_ALLOC_STR_BUF(label, aText, 256);
  VERIFY(::SetWindowText(mWnd, label));
  NS_FREE_STR_BUF(label);
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Get this button label
//
//-------------------------------------------------------------------------
NS_METHOD nsLabel::GetLabel(nsString& aBuffer)
{
  int actualSize = ::GetWindowTextLength(mWnd)+1;
  NS_ALLOC_CHAR_BUF(label, 256, actualSize);
  ::GetWindowText(mWnd, label, actualSize);
  aBuffer.SetLength(0);
  aBuffer.Append(label);
  NS_FREE_CHAR_BUF(label);
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// move, paint, resizes message - ignore
//
//-------------------------------------------------------------------------
PRBool nsLabel::OnMove(PRInt32, PRInt32)
{
  return PR_FALSE;
}

PRBool nsLabel::OnPaint()
{
  //printf("** nsLabel::OnPaint **\n");
  return PR_FALSE;
}

PRBool nsLabel::OnResize(nsRect &aWindowRect)
{
    return PR_FALSE;
}

//-------------------------------------------------------------------------
//
// return the window class name and initialize the class if needed
//
//-------------------------------------------------------------------------
LPCTSTR nsLabel::WindowClass()
{
  return "STATIC";
}

//-------------------------------------------------------------------------
//
// return window styles
//
//-------------------------------------------------------------------------
DWORD nsLabel::WindowStyle()
{ 
  DWORD style =  WS_CHILD | WS_CLIPSIBLINGS; 
  switch (mAlignment) {
    case eAlign_Right : style |= SS_RIGHT; break;
    case eAlign_Left  : style |= SS_LEFT;  break;
    case eAlign_Center: style |= SS_CENTER;break;
    default :
      style |= SS_LEFT; 
  }
  return style;
}

//-------------------------------------------------------------------------
//
// return window extended styles
//
//-------------------------------------------------------------------------
DWORD nsLabel::WindowExStyle()
{
  return 0;
}


//-------------------------------------------------------------------------
//
// get position/dimensions
//
//-------------------------------------------------------------------------

NS_METHOD nsLabel::GetBounds(nsRect &aRect)
{
  return nsWindow::GetBounds(aRect);
}

//-------------------------------------------------------------------------
NS_METHOD nsLabel::GetPreferredSize(PRInt32& aWidth, PRInt32& aHeight)
{
  if (nsnull == mContext) {
    return NS_ERROR_FAILURE;
  }
  //nsIFontMetrics * fm = GetFont();;
 // mDeviceContext->GetMetricsFor(mFont, &fm);

  nsIFontMetrics* metrics;
  mContext->GetMetricsFor(*mFont, metrics);

  nsString text;
  GetLabel(text);

  nsIRenderingContext *cx;
  mContext->CreateRenderingContext(this, cx);
  cx->SetFont(metrics);
  nscoord string_height, string_width;
  metrics->GetHeight(string_height);
  cx->GetWidth(text, string_width);
  NS_RELEASE(cx);
  NS_RELEASE(metrics);

  if (mPreferredWidth != 0) {
    aWidth = mPreferredWidth;
  } else {
    aWidth = string_width+8;
  }

  if (mPreferredHeight != 0) {
    aHeight = mPreferredHeight;
  } else {
    aHeight = string_height+8;
  }

  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsLabel::SetPreferredSize(PRInt32 aWidth, PRInt32 aHeight)
{
  mPreferredWidth  = aWidth;
  mPreferredHeight = aHeight;
  return NS_OK;
}

