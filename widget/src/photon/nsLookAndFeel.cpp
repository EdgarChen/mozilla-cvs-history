/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * Contributor(s): 
 */

#include "nsLookAndFeel.h"
#include <Pt.h>
#include "nsFont.h"
#include "nsPhWidgetLog.h"

#include "nsXPLookAndFeel.h"
 
NS_IMPL_ISUPPORTS1(nsLookAndFeel, nsILookAndFeel)

#define PH_TO_NS_RGB(ns) (ns & 0xff) << 16 | (ns & 0xff00) | ((ns >> 16) & 0xff) 

nsLookAndFeel::nsLookAndFeel() : nsILookAndFeel()
{
  NS_INIT_REFCNT();

  (void)NS_NewXPLookAndFeel(getter_AddRefs(mXPLookAndFeel));
}

nsLookAndFeel::~nsLookAndFeel()
{
}

NS_IMETHODIMP nsLookAndFeel::GetColor(const nsColorID aID, nscolor &aColor)
{
  nsresult res = NS_OK;

  if (mXPLookAndFeel)
  {
    res = mXPLookAndFeel->GetColor(aID, aColor);
    if (NS_SUCCEEDED(res))
      return res;
    res = NS_OK;
  }

  switch (aID) 
  {
	  case eColor_WindowBackground:
		aColor = PH_TO_NS_RGB(Pg_LGREY);
		break;
	  case eColor_WindowForeground:
		aColor = PH_TO_NS_RGB(Pg_DGREY);
		break;
	  case eColor_WidgetBackground:
		aColor = PH_TO_NS_RGB(Pg_LGREY);
		break;
	  case eColor_WidgetForeground:
		aColor = PH_TO_NS_RGB(Pg_DGREY);
		break;
	  case eColor_WidgetSelectBackground:
		aColor = PH_TO_NS_RGB(Pg_DGREY);
		break;
	  case eColor_WidgetSelectForeground:
		aColor = PH_TO_NS_RGB(Pg_DGREY);
		break;
	  case eColor_Widget3DHighlight:
		aColor = PH_TO_NS_RGB(Pg_WHITE);
		break;
	  case eColor_Widget3DShadow:
		aColor = PH_TO_NS_RGB(Pg_DGREY);
		break;
	  case eColor_TextBackground:
		aColor = PH_TO_NS_RGB(Pg_WHITE);
		break;
	  case eColor_TextForeground: 
		aColor = PH_TO_NS_RGB(Pg_BLACK);
		break;
	  case eColor_TextSelectBackground:
		aColor = PH_TO_NS_RGB(Pg_BLACK);
		break;
	  case eColor_TextSelectForeground:
		aColor = PH_TO_NS_RGB(Pg_WHITE);
		break;

		// css2  http://www.w3.org/TR/REC-CSS2/ui.html#system-colors
	  case eColor_activeborder:
		aColor = PH_TO_NS_RGB(Pg_BLACK);
		break;
	  case eColor_activecaption:
		aColor = PH_TO_NS_RGB(Pg_YELLOW);
		break;
	  case eColor_appworkspace:
		aColor = PH_TO_NS_RGB(Pg_LGREY);
		break;
	  case eColor_background:
		aColor = PH_TO_NS_RGB(Pg_LGREY);
		break;
	  case eColor_captiontext:
		aColor = PH_TO_NS_RGB(Pg_BLACK);
		break;
	  case eColor_graytext:
		aColor = PH_TO_NS_RGB(Pg_DGREY);
		break;
	  case eColor_highlight:
		aColor = PH_TO_NS_RGB(0x9ba9c9); // bill blue
		break;
	  case eColor_highlighttext:
		aColor = PH_TO_NS_RGB(Pg_BLACK);
		break;
	  case eColor_inactiveborder:
		aColor = PH_TO_NS_RGB(Pg_DGREY);
		break;
	  case eColor_inactivecaption:
		aColor = PH_TO_NS_RGB(Pg_GREY);
		break;
	  case eColor_inactivecaptiontext:
		aColor = PH_TO_NS_RGB(Pg_DGREY);
		break;
	  case eColor_infobackground:
		aColor = PH_TO_NS_RGB(Pg_BALLOONCOLOR); // popup yellow
		break;
	  case eColor_infotext:
		aColor = PH_TO_NS_RGB(Pg_BLACK);
		break;
	  case eColor_menu:
		aColor = PH_TO_NS_RGB(Pg_LGREY);
		break;
	  case eColor_menutext:
		aColor = PH_TO_NS_RGB(Pg_BLACK);
		break;
	  case eColor_scrollbar:
		aColor = PH_TO_NS_RGB(Pg_LGREY);
		break;
	  case eColor_threedface:
	  case eColor_buttonface:
		aColor = PH_TO_NS_RGB(Pg_LGREY);
		break;

	  case eColor_buttonhighlight:
	  case eColor_threedhighlight:
		aColor = PH_TO_NS_RGB(Pg_WHITE);
		break;

	  case eColor_buttontext:
		aColor = PH_TO_NS_RGB(Pg_BLACK);
		break;

	  case eColor_buttonshadow:
	  case eColor_threedshadow: // i think these should be the same
		aColor = PH_TO_NS_RGB(Pg_DGREY);
		break;

	  case eColor_threeddarkshadow:
		aColor = PH_TO_NS_RGB(Pg_DGREY);
		break;

	  case eColor_threedlightshadow:
		aColor = PH_TO_NS_RGB(Pg_LGREY);
		break;

	  case eColor_window:
		aColor = PH_TO_NS_RGB(Pg_WHITE);
		break;

	  case eColor_windowframe:
		aColor = PH_TO_NS_RGB(Pg_LGREY);
		break;

	  case eColor_windowtext:
		aColor = PH_TO_NS_RGB(Pg_BLACK);
		break;

	  // from the CSS3 working draft (not yet finalized)
	  // http://www.w3.org/tr/2000/wd-css3-userint-20000216.html#color

	  case eColor__moz_field:
		aColor = PH_TO_NS_RGB(Pg_WHITE);
		break;

	case eColor__moz_fieldtext:
	  aColor = PH_TO_NS_RGB(Pg_BLACK);
	  break;

	case eColor__moz_dialog:
	  aColor = PH_TO_NS_RGB(Pg_LGREY);
	  break;

	case eColor__moz_dialogtext:
	  aColor = PH_TO_NS_RGB(Pg_BLACK);
	  break;

	case eColor__moz_dragtargetzone:
	  aColor = PH_TO_NS_RGB(Pg_LGREY);
	  break;

  	default:
    aColor = PH_TO_NS_RGB(Pg_WHITE);
    break;
  }

  return res;
}
  
NS_IMETHODIMP nsLookAndFeel::GetMetric(const nsMetricID aID, PRInt32 & aMetric)
{
  nsresult res = NS_OK;

  if (mXPLookAndFeel)
  {
    res = mXPLookAndFeel->GetMetric(aID, aMetric);
    if (NS_SUCCEEDED(res))
      return res;
    res = NS_OK;
  }

  switch (aID) 
  {
  case eMetric_WindowTitleHeight:
    aMetric = 0;
    break;
  case eMetric_WindowBorderWidth:
    //    aMetric = mStyle->klass->xthickness;
    aMetric = 1;
    break;
  case eMetric_WindowBorderHeight:
    //    aMetric = mStyle->klass->ythickness;
    aMetric = 1;
    break;
  case eMetric_Widget3DBorder:
    //    aMetric = 4;
    break;
  case eMetric_TextFieldHeight:
  	aMetric = 20;
    break;
  case eMetric_TextFieldBorder:
    aMetric = 1;
    break;
  case eMetric_TextVerticalInsidePadding:
    aMetric = 0;
    break;
  case eMetric_TextShouldUseVerticalInsidePadding:
    aMetric = 0;
    break;
  case eMetric_TextHorizontalInsideMinimumPadding:
    aMetric = 0;
    break;
  case eMetric_TextShouldUseHorizontalInsideMinimumPadding:
    aMetric = 0;
    break;
  case eMetric_ButtonHorizontalInsidePaddingNavQuirks:
  	aMetric = 10;
    break;
  case eMetric_ButtonHorizontalInsidePaddingOffsetNavQuirks:
  	aMetric = 8;
    break;
  case eMetric_CheckboxSize:
    aMetric = 10;
    break;
  case eMetric_RadioboxSize:
    aMetric = 10;
    break;
  case eMetric_ListShouldUseHorizontalInsideMinimumPadding:
    aMetric = 0;
    break;
  case eMetric_ListHorizontalInsideMinimumPadding:
    aMetric = 0;
    break;
  case eMetric_ListShouldUseVerticalInsidePadding:
    aMetric = 0;
    break;
  case eMetric_ListVerticalInsidePadding:
    aMetric = 0;
    break;
  case eMetric_CaretBlinkTime:
    aMetric = 500;
    break;
  case eMetric_SingleLineCaretWidth:
  case eMetric_MultiLineCaretWidth:
    aMetric = 1;
    break;
  case eMetric_SubmenuDelay:
    aMetric = 200;
    break;
	case eMetric_DragFullWindow:
		aMetric = 0;
		break;
  default:
    aMetric = -1;
    res     = NS_ERROR_FAILURE;
  }

  return res;
}

NS_IMETHODIMP nsLookAndFeel::GetMetric(const nsMetricFloatID aID, float & aMetric)
{
  nsresult res = NS_OK;

  if (mXPLookAndFeel)
  {
    res = mXPLookAndFeel->GetMetric(aID, aMetric);
    if (NS_SUCCEEDED(res))
      return res;
    res = NS_OK;
  }

  switch (aID) {
  case eMetricFloat_TextFieldVerticalInsidePadding:
    aMetric = 0.25f;
    break;
  case eMetricFloat_TextFieldHorizontalInsidePadding:
    aMetric = 0.95f; // large number on purpose so minimum padding is used
    break;
  case eMetricFloat_TextAreaVerticalInsidePadding:
    aMetric = 0.40f;    
    break;
  case eMetricFloat_TextAreaHorizontalInsidePadding:
    aMetric = 0.40f; // large number on purpose so minimum padding is used
    break;
  case eMetricFloat_ListVerticalInsidePadding:
    aMetric = 0.10f;
    break;
  case eMetricFloat_ListHorizontalInsidePadding:
    aMetric = 0.40f;
    break;
  case eMetricFloat_ButtonVerticalInsidePadding:
    aMetric = 0.25f;
    break;
  case eMetricFloat_ButtonHorizontalInsidePadding:
    aMetric = 0.25f;
    break;
  default:
    aMetric = -1.0;
    res = NS_ERROR_FAILURE;
  }
 
  return res;
}

#ifdef NS_DEBUG
NS_IMETHODIMP nsLookAndFeel::GetNavSize(const nsMetricNavWidgetID aWidgetID,
                                        const nsMetricNavFontID   aFontID, 
                                        const PRInt32             aFontSize, 
                                        nsSize &aSize)
{
  if (mXPLookAndFeel)
  {
    nsresult rv = mXPLookAndFeel->GetNavSize(aWidgetID, aFontID, aFontSize, aSize);
    if (NS_SUCCEEDED(rv))
      return rv;
  }

  aSize.width  = 0;
  aSize.height = 0;
  return NS_ERROR_NOT_IMPLEMENTED;
}
#endif
