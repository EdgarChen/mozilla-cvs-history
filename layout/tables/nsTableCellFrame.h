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
#ifndef nsTableCellFrame_h__
#define nsTableCellFrame_h__

#include "nscore.h"
#include "nsContainerFrame.h"
#include "nsTableFrame.h"

struct nsStyleSpacing;

/**
 * nsTableCellFrame
 * data structure to maintain information about a single table cell's frame
 *
 * @author  sclark
 */
class nsTableCellFrame : public nsContainerFrame
{
public:

  void Init(PRInt32 aRowSpan, PRInt32 aColSpan, PRInt32 aColIndex);

  static nsresult NewFrame(nsIFrame** aInstancePtrResult,
                           nsIContent* aContent,
                           nsIFrame*   aParent);

  NS_IMETHOD Paint(nsIPresContext& aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect& aDirtyRect);

  NS_IMETHOD Reflow(nsIPresContext*      aPresContext,
                    nsReflowMetrics&     aDesiredSize,
                    const nsReflowState& aReflowState,
                    nsReflowStatus&      aStatus);

  /**
   * @see nsContainerFrame
   */
  NS_IMETHOD CreateContinuingFrame(nsIPresContext*  aPresContext,
                                   nsIFrame*        aParent,
                                   nsIStyleContext* aStyleContext,
                                   nsIFrame*&       aContinuingFrame);

  void          VerticallyAlignChild(nsIPresContext* aPresContext);

  /** return the mapped cell's row span.  Always >= 1. */
  virtual PRInt32 GetRowSpan();

  /** return the mapped cell's col span.  Always >= 1. */
  virtual PRInt32 GetColSpan();

  /** return the mapped cell's column index (starting at 0 for the first column) */
  virtual PRInt32 GetColIndex();

  virtual ~nsTableCellFrame();

  // Get the TableFrame that contains this cell frame
  virtual nsTableFrame* GetTableFrame();

  // For DEBUGGING Purposes Only
  NS_IMETHOD  MoveTo(nscoord aX, nscoord aY);
  NS_IMETHOD  SizeTo(nscoord aWidth, nscoord aHeight);

protected:

  /** protected constructor.
    * @see NewFrame
    */
  nsTableCellFrame(nsIContent* aContent,
					         nsIFrame* aParentFrame);

  /** Create a psuedo-frame for this cell.  Handles continuing frames as needed.
    */
  virtual void CreatePsuedoFrame(nsIPresContext* aPresContext);

  // Subclass hook for style post processing
  NS_IMETHOD DidSetStyleContext(nsIPresContext* aPresContext);
  void      MapTextAttributes(nsIPresContext* aPresContext);
  void      MapBorderMarginPadding(nsIPresContext* aPresContext);
  void      MapHTMLBorderStyle(nsIPresContext* aPresContext,nsStyleSpacing& aSpacingStyle, nscoord aBorderWidth);
  PRBool    ConvertToPixelValue(nsHTMLValue& aValue, PRInt32 aDefault, PRInt32& aResult);

protected:
  /** the number of rows spanned by this cell */
  int          mRowSpan;
    
  /** the number of columns spanned by this cell */
  int          mColSpan;

  /** the starting column for this cell */
  int          mColIndex;

};

inline void nsTableCellFrame::Init(PRInt32 aRowSpan, PRInt32 aColSpan, PRInt32 aColIndex)
{
  NS_PRECONDITION(0<aRowSpan,  "bad row span arg");
  NS_PRECONDITION(0<aColSpan,  "bad col span arg");
  NS_PRECONDITION(0<=aColIndex, "bad col index arg");
  mRowSpan = aRowSpan;
  mColSpan = aColSpan;
  mColIndex = aColIndex;
}

inline PRInt32 nsTableCellFrame::GetRowSpan()
{  return mRowSpan;}

inline PRInt32 nsTableCellFrame::GetColSpan()
{  return mColSpan;}

inline PRInt32 nsTableCellFrame::GetColIndex()
{  return mColIndex;}


#endif
