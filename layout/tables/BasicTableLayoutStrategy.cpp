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

#include "BasicTableLayoutStrategy.h"
#include "nsTableFrame.h"
#include "nsTableColFrame.h"
#include "nsColLayoutData.h"
#include "nsCellLayoutData.h"
#include "nsTableCol.h"
#include "nsTableCellFrame.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIPtr.h"
#include "nsHTMLIIDs.h"

NS_DEF_PTR(nsTableCol);
NS_DEF_PTR(nsIStyleContext);


#ifdef NS_DEBUG
static PRBool gsDebug = PR_FALSE;
static PRBool gsDebugCLD = PR_FALSE;
#else
static const PRBool gsDebug = PR_FALSE;
static const PRBool gsDebugCLD = PR_FALSE;
#endif

/* ---------- ProportionalColumnLayoutStruct ---------- */  
// TODO: make public so other subclasses can use it

/** useful info about a column for layout */
struct ProportionalColumnLayoutStruct
{
  ProportionalColumnLayoutStruct(PRInt32 aColIndex, 
                                 nscoord aMinColWidth, 
                                 nscoord aMaxColWidth, 
                                 PRInt32 aProportion)
  {
    mColIndex = aColIndex;
    mMinColWidth = aMinColWidth;
    mMaxColWidth = aMaxColWidth;
    mProportion = aProportion;
  };

  PRInt32 mColIndex;
  nscoord mMinColWidth; 
  nscoord mMaxColWidth; 
  PRInt32 mProportion;
};



/* ---------- BasicTableLayoutStrategy ---------- */

/* return true if the style indicates that the width is proportional 
 * for the purposes of column width determination
 */
PRBool BasicTableLayoutStrategy::IsFixedWidth(const nsStylePosition* aStylePosition)
{
  PRBool result = PR_FALSE; // assume that it is not fixed width
  PRInt32 unitType;
  if (nsnull == aStylePosition) {
    unitType = eStyleUnit_Auto;
  }
  else {
    unitType = aStylePosition->mWidth.GetUnit();
  }

  switch (unitType) {
    case eStyleUnit_Coord:
      result = PR_TRUE;

    case eStyleUnit_Auto:
    case eStyleUnit_Proportional:
    case eStyleUnit_Percent:
      break;

    // TODO
    case eStyleUnit_Inherit:
      break;

    default:
      NS_ASSERTION(PR_FALSE, "illegal style type in IsFixedWidth");
      break;
  }
  return result;
}

PRBool BasicTableLayoutStrategy::IsAutoWidth(const nsStylePosition* aStylePosition)
{
  PRBool result = PR_TRUE; // assume that it is
  if (nsnull!=aStylePosition)
  {
    result = (PRBool)(eStyleUnit_Auto==aStylePosition->mWidth.GetUnit());
  }
  return result;
}


BasicTableLayoutStrategy::BasicTableLayoutStrategy(nsTableFrame *aFrame, PRInt32 aNumCols)
{
  NS_ASSERTION(nsnull!=aFrame, "bad frame arg");

  mTableFrame = aFrame;
  mNumCols = aNumCols;
  //cache the value of the cols attribute
  nsIFrame * tableFrame = mTableFrame;
// begin REMOVE_ME_WHEN_TABLE_STYLE_IS_RESOLVED!   XXX
  tableFrame->GetGeometricParent(tableFrame);
// end REMOVE_ME_WHEN_TABLE_STYLE_IS_RESOLVED!     XXX
  nsStyleTable *tableStyle;
  tableFrame->GetStyleData(eStyleStruct_Table, (nsStyleStruct *&)tableStyle);
  mCols = tableStyle->mCols;
}

BasicTableLayoutStrategy::~BasicTableLayoutStrategy()
{
}

PRBool BasicTableLayoutStrategy::BalanceColumnWidths(nsIPresContext* aPresContext,
                                                     nsIStyleContext *aTableStyle,
                                                     const nsReflowState& aReflowState,
                                                     nscoord aMaxWidth, 
                                                     nscoord &aTotalFixedWidth,
                                                     nscoord &aMinTableWidth,
                                                     nscoord &aMaxTableWidth,
                                                     nsSize* aMaxElementSize)
{
  PRBool result = PR_TRUE;
  // initialize out parameters
  aTotalFixedWidth=aMinTableWidth=aMaxTableWidth=0;

  // Step 1 - assign the width of all fixed-width columns
  AssignFixedColumnWidths(aPresContext, aMaxWidth, aTotalFixedWidth, aMinTableWidth, aMaxTableWidth);

  if (nsnull!=aMaxElementSize)
  { // this is where we initialize maxElementSize if it is non-null
    aMaxElementSize->height = 0;
    aMaxElementSize->width = aMinTableWidth;
    if (gsDebug) printf("  setting aMaxElementSize->width = %d\n", aMaxElementSize->width);
  }
  else
  {
    if (gsDebug) printf("  nsnull aMaxElementSize\n");
  }

  // Step 2 - determine how much space is really available
  PRInt32 availWidth = aMaxWidth - aTotalFixedWidth;
  nscoord tableWidth = 0;
  PRBool tableIsAutoWidth = nsTableFrame::TableIsAutoWidth(mTableFrame, aTableStyle, aReflowState, tableWidth);
  if (PR_FALSE==tableIsAutoWidth)
    availWidth = tableWidth - aTotalFixedWidth;
  
  // Step 3 - assign the width of all proportional-width columns in the remaining space
  if (gsDebug==PR_TRUE) printf ("Step 2...\n  availWidth = %d\n", availWidth);
  result = BalanceProportionalColumns(aPresContext, aReflowState,
                                      availWidth, aMaxWidth,
                                      aMinTableWidth, aMaxTableWidth,
                                      tableWidth, tableIsAutoWidth);
  return result;
}

/*
STEP 1
for every col
  if the col has a fixed width
    set the width to max(fixed width, maxElementSize)
    if the cell spans columns, divide the cell width between the columns
  else skip it for now

take borders and padding into account

STEP 2
determine the min and max size for the table width
if col is proportionately specified
  if (col width specified to 0)
    col width = minColWidth
  else if (minTableWidth >= aMaxSize.width)
    set col widths to min, install a hor. scroll bar
  else if (maxTableWidth <= aMaxSize.width)
    set each col to its max size
  else
    W = aMaxSize.width -  minTableWidth
    D = maxTableWidth - minTableWidth
    for each col
      d = maxColWidth - minColWidth
      col width = minColWidth + ((d*W)/D)

STEP 3
if there is space left over
  for every col
    if col is proportionately specified
      add space to col width until it is that proportion of the table width
      do this non-destructively in case there isn't enough space
  if there isn't enough space as determined in the prior step,
    add space in proportion to the proportionate width attribute
  */


// Step 1 - assign the width of all fixed-width columns, 
//          and calculate min/max table width
PRBool BasicTableLayoutStrategy::AssignFixedColumnWidths(nsIPresContext* aPresContext,
                                                         nscoord aMaxWidth, 
                                                         nscoord &aTotalFixedWidth,
                                                         nscoord &aMinTableWidth,
                                                         nscoord &aMaxTableWidth)
{
#ifdef DEBUG
  nsIFrame *tablePIF=nsnull;
  mTableFrame->GetPrevInFlow(tablePIF);
  NS_ASSERTION(nsnull==tablePIF, "never ever call me on a continuing frame!");
#endif

  if (gsDebug==PR_TRUE) printf ("  AssignFixedColumnWidths\n");
  nsVoidArray *spanList=nsnull;


  PRBool hasColsAttribute = (PRBool)(NS_STYLE_TABLE_COLS_NONE!=mCols);
  
  PRInt32 *minColWidthArray = nsnull;
  PRInt32 *maxColWidthArray = nsnull;
  if (PR_TRUE==hasColsAttribute)
  {
    minColWidthArray = new PRInt32[mNumCols];
    maxColWidthArray = new PRInt32[mNumCols];
  }

  // for every column, determine it's min and max width, and keep track of the table width
  for (PRInt32 colIndex = 0; colIndex<mNumCols; colIndex++)
  { 
    nsVoidArray *columnLayoutData = mTableFrame->GetColumnLayoutData();
    nsColLayoutData * colData = (nsColLayoutData *)(columnLayoutData->ElementAt(colIndex));
    NS_ASSERTION(nsnull != colData, "bad column data");
    nsTableColFrame *colFrame = colData->GetColFrame();
    NS_ASSERTION(nsnull!=colFrame, "bad col frame");

    // need to track min/max column width for setting min/max table widths
    PRInt32 minColWidth = 0;
    PRInt32 maxColWidth = 0;
    nsVoidArray *cells = colData->GetCells();
    PRInt32 numCells = cells->Count();
    if (gsDebug==PR_TRUE) printf ("  for column %d  numCells = %d\n", colIndex, numCells);

    // Get the columns's style
    nsIStyleContextPtr colSC;
    colFrame->GetStyleContext(aPresContext, colSC.AssignRef());
    const nsStylePosition* colPosition = (const nsStylePosition*) colSC->GetStyleData(eStyleStruct_Position);

    // Get column width if it has one
    PRBool haveColWidth = PR_FALSE;
    nscoord specifiedFixedColWidth=0;
    switch (colPosition->mWidth.GetUnit()) {
      case eStyleUnit_Coord:
        haveColWidth = PR_TRUE;
        specifiedFixedColWidth = colPosition->mWidth.GetCoordValue();
        mTableFrame->SetColumnWidth(colIndex, specifiedFixedColWidth);  //QQQ add in margins?
        break;

      default:
        break;
    }

    // TODO - use specifiedFixedColWidth to influence the width when both col span and fixed col width are given
    /* Scan the column, simulatneously assigning fixed column widths
     * and computing the min/max column widths
     */
    // first, deal with any cells that span into this column from a pervious column
    if (nsnull!=spanList)
    {
      PRInt32 spanCount = spanList->Count();
      // go through the list backwards so we can delete easily
      for (PRInt32 spanIndex=spanCount-1; 0<=spanIndex; spanIndex--)
      {
        SpanInfo *spanInfo = (SpanInfo *)(spanList->ElementAt(spanIndex));
        if (minColWidth < spanInfo->cellMinWidth)
          minColWidth = spanInfo->cellMinWidth;
        if (maxColWidth < spanInfo->cellDesiredWidth)
          maxColWidth = spanInfo->cellDesiredWidth;
        spanInfo->span--;
        if (gsDebug==PR_TRUE) 
          printf ("    for spanning cell %d with remaining span=%d, min = %d and  des = %d\n", 
                  spanIndex, spanInfo->span, spanInfo->cellMinWidth, spanInfo->cellDesiredWidth);
        if (0==spanInfo->span)
        {
          spanList->RemoveElementAt(spanIndex);
          delete spanInfo;
        }
      }
    }

    for (PRInt32 cellIndex = 0; cellIndex<numCells; cellIndex++)
    {
      nsCellLayoutData * data = (nsCellLayoutData *)(cells->ElementAt(cellIndex));
      if (nsnull == data) {
        // For cells that span rows there's only cell layout data for the first row
        continue;
      }
      
      nsMargin  margin;
      nsresult  result = data->GetMargin(margin);

      nsSize * cellMinSize = data->GetMaxElementSize();
      nsReflowMetrics * cellDesiredSize = data->GetDesiredSize();
      NS_ASSERTION(nsnull != cellDesiredSize, "bad cellDesiredSize");
      PRInt32 colSpan = data->GetCellFrame()->GetColSpan();
      if (gsDebug==PR_TRUE) 
        printf ("    for cell %d with colspan=%d, min = %d,%d  and  des = %d,%d, margins %d %d\n", 
                cellIndex, colSpan, cellMinSize->width, cellMinSize->height,
                cellDesiredSize->width, cellDesiredSize->height, 
                margin.left, margin.right);

      switch (colPosition->mWidth.GetUnit()) {
        case eStyleUnit_Coord:
          {
            // This col has a fixed width, so set the cell's width to the
            // larger of (specified width, largest max_element_size of the
            // cells in the column)
            PRInt32 widthForThisCell = PR_MAX(cellMinSize->width, colPosition->mWidth.GetCoordValue());
            if (mTableFrame->GetColumnWidth(colIndex) < widthForThisCell)
            {
              if (gsDebug) printf ("    setting fixed width to %d\n",widthForThisCell);
              mTableFrame->SetColumnWidth(colIndex, widthForThisCell); //QQQ add in margins?
              maxColWidth = widthForThisCell;
            }
          }
          break;

        default:
          break;

      }

      // regardless of the width specification, keep track of the
      // min/max column widths
      nscoord cellMinWidth = cellMinSize->width/colSpan;
      nscoord cellDesiredWidth = cellDesiredSize->width/colSpan;

      if (aMaxWidth!=cellDesiredSize->width)
      {
        if (NS_UNCONSTRAINEDSIZE!=cellMinWidth)
          cellMinWidth += margin.left + margin.right;
        if (NS_UNCONSTRAINEDSIZE!=cellDesiredWidth)
          cellDesiredWidth += margin.left + margin.right;
      }

      if (minColWidth < cellMinWidth)
        minColWidth = cellMinWidth;
      if (maxColWidth < cellDesiredWidth)
        maxColWidth = cellDesiredWidth;
      if (1<colSpan)
      {
        // add the cell to our list of spanners
        SpanInfo *spanInfo = new SpanInfo(colSpan-1, cellMinWidth, cellDesiredWidth);
        if (nsnull==spanList)
          spanList = new nsVoidArray();
        spanList->AppendElement(spanInfo);
      }
      if (gsDebug) {
        printf ("    after cell %d, minColWidth = %d and maxColWidth = %d\n",
                cellIndex, minColWidth, maxColWidth);
      }
    }

    // keep a running total of the amount of space taken up by all fixed-width columns
    if (PR_TRUE==haveColWidth)
      aTotalFixedWidth += mTableFrame->GetColumnWidth(colIndex);

    if (gsDebug) {
      printf ("    after col %d, aTotalFixedWidth = %d\n",
              colIndex, aTotalFixedWidth);
    }

    // add col[i] metrics to the running totals for the table min/max width
    if (NS_UNCONSTRAINEDSIZE!=aMinTableWidth)
      aMinTableWidth += minColWidth;  // SEC: insets!
    if (aMinTableWidth<=0) 
      aMinTableWidth = NS_UNCONSTRAINEDSIZE; // handle overflow
    if (NS_UNCONSTRAINEDSIZE!=aMaxTableWidth)
      aMaxTableWidth += maxColWidth;  // SEC: insets!
    if (aMaxTableWidth<=0) 
      aMaxTableWidth = NS_UNCONSTRAINEDSIZE; // handle overflow
    if (PR_TRUE==hasColsAttribute)
    {
      minColWidthArray[colIndex] = minColWidth;
      maxColWidthArray[colIndex] = maxColWidth;
    }
    if (gsDebug==PR_TRUE) 
      printf ("  after col %d, minTableWidth = %d and maxTableWidth = %d\n", 
              colIndex, aMinTableWidth, aMaxTableWidth);
  
  } // end Step 1 for fixed-width columns

  // now, post-process the computed values based on the table attributes
  // if there is a COLS attribute, fix up aMinTableWidth and aMaxTableWidth
  if (PR_TRUE==hasColsAttribute)
  {
    // for every effected column, subtract out its prior contribution and add back in the new value
    PRInt32 numColsEffected = mNumCols;
    if (NS_STYLE_TABLE_COLS_ALL!=mCols)
      numColsEffected = mCols;
    PRInt32 maxOfMinColWidths=0;
    PRInt32 maxOfMaxColWidths=0;
    PRInt32 effectedColIndex;
    for (effectedColIndex=0; effectedColIndex<numColsEffected; effectedColIndex++)
    {
      if (maxOfMinColWidths < minColWidthArray[effectedColIndex])
        maxOfMinColWidths = minColWidthArray[effectedColIndex];
      if (maxOfMaxColWidths < maxColWidthArray[effectedColIndex])
        maxOfMaxColWidths = maxColWidthArray[effectedColIndex];
    }
    for (effectedColIndex=0; effectedColIndex<numColsEffected; effectedColIndex++)
    {
      // subtract out the prior contributions of this column
      // and add back in the adjusted value
      if (NS_UNCONSTRAINEDSIZE!=aMinTableWidth)
      {
        aMinTableWidth -= minColWidthArray[effectedColIndex];
        aMinTableWidth += maxOfMinColWidths;
      }
      if (NS_UNCONSTRAINEDSIZE!=aMaxTableWidth)
      {
        aMaxTableWidth -= maxColWidthArray[effectedColIndex];
        aMaxTableWidth += maxOfMaxColWidths;
      }
    }
    delete [] minColWidthArray;
    delete [] maxColWidthArray;
  }

  if (PR_TRUE==gsDebug)
    printf ("%p: aMinTW=%d, aMaxTW=%d\n", mTableFrame, aMinTableWidth, aMaxTableWidth);
  
  if (nsnull!=spanList)
    delete spanList;
  return PR_TRUE;
}

PRBool BasicTableLayoutStrategy::BalanceProportionalColumns(nsIPresContext* aPresContext,
                                                            const nsReflowState& aReflowState,
                                                            nscoord aAvailWidth,
                                                            nscoord aMaxWidth,
                                                            nscoord aMinTableWidth, 
                                                            nscoord aMaxTableWidth,
                                                            nscoord aTableFixedWidth,
                                                            PRBool  aTableIsAutoWidth)
{
  PRBool result = PR_TRUE;

  if (0==aTableFixedWidth)
  {
    if (NS_UNCONSTRAINEDSIZE==aMaxWidth  ||  NS_UNCONSTRAINEDSIZE==aMinTableWidth)
    { // the max width of the table fits comfortably in the available space
      if (gsDebug) printf ("  * table laying out in NS_UNCONSTRAINEDSIZE, calling BalanceColumnsTableFits\n");
      result = BalanceColumnsTableFits(aPresContext, aReflowState, aAvailWidth, 
                                       aMaxWidth, aTableFixedWidth, aTableIsAutoWidth);
    }
    else if (aMinTableWidth > aMaxWidth)
    { // the table doesn't fit in the available space
      if (gsDebug) printf ("  * min table does not fit, calling SetColumnsToMinWidth\n");
      result = SetColumnsToMinWidth(aPresContext);
    }
    else if (aMaxTableWidth <= aMaxWidth)
    { // the max width of the table fits comfortably in the available space
      if (gsDebug) printf ("  * table desired size fits, calling BalanceColumnsTableFits\n");
      result = BalanceColumnsTableFits(aPresContext, aReflowState, aAvailWidth, 
                                       aMaxWidth, aTableFixedWidth, aTableIsAutoWidth);
    }
    else
    { // the table fits somewhere between its min and desired size
      if (gsDebug) printf ("  * table desired size does not fit, calling BalanceColumnsConstrained\n");
      result = BalanceColumnsConstrained(aPresContext, aReflowState, aAvailWidth,
                                         aMaxWidth, aMinTableWidth, aMaxTableWidth);
    }
  }
  else
  { // tables with fixed-width have their own rules
    if (NS_UNCONSTRAINEDSIZE==aMinTableWidth)
    { // the table has empty content, and needs to be streched to the specified width
      if (gsDebug) printf ("  * specified width table > maxTableWidth, calling BalanceColumnsTableFits\n");
      result = BalanceColumnsTableFits(aPresContext, aReflowState, aAvailWidth, 
                                       aMaxWidth, aTableFixedWidth, aTableIsAutoWidth);
    }
    else if ((aTableFixedWidth<aMinTableWidth) /*|| (aAvailWidth<aMinTableWidth)*/)
    { // the table's specified width doesn't fit in the available space
      if (gsDebug) printf ("  * specified width table with width<minTableWidth, calling SetColumnsToMinWidth\n");
      result = SetColumnsToMinWidth(aPresContext);
    }
    else if (aTableFixedWidth<aMaxTableWidth)
    { // the table's specified width is between it's min and max, so fit the columns in proportionately
      if (gsDebug) printf ("  * specified width table < maxTableWidth, calling BalanceColumnsConstrained\n");
      result = BalanceColumnsConstrained(aPresContext, aReflowState, aAvailWidth,
                                         aAvailWidth, aMinTableWidth, aMaxTableWidth);
      // should 3rd param above be aAvailWidth?
    }
    else
    { // the table's specified width is >= its max width, so give each column its max requested size
      if (gsDebug) printf ("  * specified width table > maxTableWidth, calling BalanceColumnsTableFits\n");
      result = BalanceColumnsTableFits(aPresContext, aReflowState, aAvailWidth, 
                                       aMaxWidth, aTableFixedWidth, aTableIsAutoWidth);
    }
  }
  return result;
}

PRBool BasicTableLayoutStrategy::SetColumnsToMinWidth(nsIPresContext* aPresContext)
{
  PRBool result = PR_TRUE;

  PRBool hasColsAttribute = (PRBool)(NS_STYLE_TABLE_COLS_NONE!=mCols);
  
  PRInt32 *minColWidthArray = nsnull;
  if (PR_TRUE==hasColsAttribute)
  {
    minColWidthArray = new PRInt32[mNumCols];
  }

  nsVoidArray *columnLayoutData = mTableFrame->GetColumnLayoutData();
  PRInt32 numCols = columnLayoutData->Count();
  for (PRInt32 colIndex = 0; colIndex<numCols; colIndex++)
  { 
    nsColLayoutData * colData = (nsColLayoutData *)(columnLayoutData->ElementAt(colIndex));
    nsVoidArray *cells = colData->GetCells();
    PRInt32 minColWidth = 0;
    PRInt32 maxColWidth = 0;
    PRInt32 numCells = cells->Count();
    if (gsDebug==PR_TRUE) printf ("  for col %d\n", colIndex);

    // XXX need column frame to ask this question
    const nsStylePosition* colPosition = nsnull;

    if (PR_FALSE==IsFixedWidth(colPosition))
    {
      for (PRInt32 cellIndex = 0; cellIndex<numCells; cellIndex++)
      { // this col has proportional width, so determine its width requirements
        nsCellLayoutData * data = (nsCellLayoutData *)(cells->ElementAt(cellIndex));
        if (nsnull==data)
        { // For cells that span rows there's only cell layout data for the first row
          continue;
        }
        nsMargin  margin;
        data->GetMargin(margin);
        nsSize * cellMinSize = data->GetMaxElementSize();
        NS_ASSERTION(nsnull != cellMinSize, "bad cellMinSize");
        nscoord cellMinWidth = cellMinSize->width;    // do we need to take into account colSpan here?
        cellMinWidth += margin.left + margin.right;
        if (minColWidth < cellMinWidth)
          minColWidth = cellMinWidth;
      }
      if (PR_TRUE==hasColsAttribute)
      {
        minColWidthArray[colIndex] = minColWidth;
      }

      if (gsDebug==PR_TRUE) 
      {
        printf ("  for determining width of col %d %s:\n",
                 colIndex, !IsFixedWidth(colPosition)? "(P)":"(A)");
        printf ("    minColWidth = %d\n", minColWidth);
      }

      mTableFrame->SetColumnWidth(colIndex, minColWidth);
      if (gsDebug==PR_TRUE) 
        printf ("  2: col %d, set to width = %d\n", colIndex, mTableFrame->GetColumnWidth(colIndex));
    }
  }

  // now, post-process the computed values based on the table attributes
  // if there is a COLS attribute, fix up aMinTableWidth and aMaxTableWidth
  if (PR_TRUE==hasColsAttribute)
  {
    // for every effected column, subtract out its prior contribution and add back in the new value
    PRInt32 numColsEffected = mNumCols;
    if (NS_STYLE_TABLE_COLS_ALL!=mCols)
      numColsEffected = mCols;
    PRInt32 maxOfEffectedColWidths=0;
    PRInt32 effectedColIndex;
    // XXX need to fix this and all similar code if any fixed-width columns intersect COLS
    for (effectedColIndex=0; effectedColIndex<numColsEffected; effectedColIndex++)
    {
      if (maxOfEffectedColWidths < minColWidthArray[effectedColIndex])
        maxOfEffectedColWidths = minColWidthArray[effectedColIndex];
    }
    for (effectedColIndex=0; effectedColIndex<numColsEffected; effectedColIndex++)
    {
      // set each effected column to the size of the largest column in the group
      mTableFrame->SetColumnWidth(effectedColIndex, maxOfEffectedColWidths);
      if (PR_TRUE==gsDebug)
        printf("  2 (cols): setting %d to %d\n", effectedColIndex, maxOfEffectedColWidths); 
    }
    delete [] minColWidthArray;
  }


  return result;
}

PRBool BasicTableLayoutStrategy::BalanceColumnsTableFits(nsIPresContext* aPresContext,
                                                         const nsReflowState& aReflowState,
                                                         nscoord aAvailWidth,
                                                         nscoord aMaxWidth,
                                                         nscoord aTableFixedWidth,
                                                         PRBool  aTableIsAutoWidth)
{
#ifdef DEBUG
  nsIFrame *tablePIF=nsnull;
  mTableFrame->GetPrevInFlow(tablePIF);
  NS_ASSERTION(nsnull==tablePIF, "never ever call me on a continuing frame!");
#endif

  PRBool  result = PR_TRUE;
  nscoord tableWidth=0;               // the width of the table as a result of setting column widths
  PRInt32 totalSlices=0;              // the total number of slices the proportional-width columns request
  nsVoidArray *proportionalColumnsList=nsnull; // a list of the columns that are proportional-width
  nsVoidArray *spanList=nsnull;       // a list of the cells that span columns
  nsVoidArray *columnLayoutData = mTableFrame->GetColumnLayoutData();
  PRInt32 numCols = columnLayoutData->Count();
  nscoord * effectiveColumnWidths = new nscoord[numCols]; // used for Nav4 compatible table fluffing
  nsCRT::memset (effectiveColumnWidths, 0, numCols*sizeof(PRInt32));
  for (PRInt32 colIndex = 0; colIndex<numCols; colIndex++)
  { 
    if (gsDebug==PR_TRUE) printf ("  for col %d\n", colIndex);
    PRInt32 minColWidth = 0;
    PRInt32 maxColWidth = 0;
    // Get column information
    nsColLayoutData * colData = (nsColLayoutData *)(columnLayoutData->ElementAt(colIndex));
    nsVoidArray *cells = colData->GetCells();
    PRInt32 numCells = cells->Count();

    // Get the columns's style
    nsTableColFrame *colFrame = colData->GetColFrame();
    NS_ASSERTION(nsnull!=colFrame, "bad col frame");
    nsIStyleContextPtr colSC;
    colFrame->GetStyleContext(aPresContext, colSC.AssignRef());
    const nsStylePosition* colPosition = (const nsStylePosition*) colSC->GetStyleData(eStyleStruct_Position);
    if (gsDebug) printf("col %d has frame %p with style %p and pos %p\n", 
                         colIndex, colFrame, (nsIStyleContext *)colSC, colPosition);

    if (PR_FALSE==IsFixedWidth(colPosition))
    {
      // first, deal with any cells that span into this column from a pervious column
      if (nsnull!=spanList)
      {
        PRInt32 spanCount = spanList->Count();
        // go through the list backwards so we can delete easily
        for (PRInt32 spanIndex=spanCount-1; 0<=spanIndex; spanIndex--)
        {
          SpanInfo *spanInfo = (SpanInfo *)(spanList->ElementAt(spanIndex));
          if (minColWidth < spanInfo->cellMinWidth)
            minColWidth = spanInfo->cellMinWidth;
          if (maxColWidth < spanInfo->cellDesiredWidth)
            maxColWidth = spanInfo->cellDesiredWidth;
          spanInfo->span--;
          if (gsDebug==PR_TRUE) 
            printf ("    for spanning cell %d with remaining span=%d, min = %d and  des = %d\n", 
                    spanIndex, spanInfo->span, spanInfo->cellMinWidth, spanInfo->cellDesiredWidth);
          if (0==spanInfo->span)
          {
            spanList->RemoveElementAt(spanIndex);
            delete spanInfo;
          }
        }
      }

      for (PRInt32 cellIndex = 0; cellIndex<numCells; cellIndex++)
      { // this col has proportional width, so determine its width requirements
        nsCellLayoutData * data = (nsCellLayoutData *)(cells->ElementAt(cellIndex));
        if (nsnull == data) {
          // For cells that span columns there's only cell layout data for the first column
          continue;
        }
 
        PRInt32 colSpan = data->GetCellFrame()->GetColSpan();
        // distribute a portion of the spanning cell's min and max width to this column
        nsSize * cellMinSize = data->GetMaxElementSize();
        NS_ASSERTION(nsnull != cellMinSize, "bad cellMinSize");
        nsReflowMetrics * cellDesiredSize = data->GetDesiredSize();
        NS_ASSERTION(nsnull != cellDesiredSize, "bad cellDesiredSize");
        
        nsMargin  margin;
        nsresult  result = data->GetMargin(margin);        
        nscoord marginWidth = margin.left + margin.right;
        PRInt32 cellMinWidth = cellMinSize->width/colSpan + marginWidth;
        // first get the desired size info from reflow pass 1
        PRInt32 cellDesiredWidth = cellDesiredSize->width/colSpan + marginWidth;  
        // then get the desired size info factoring in the cell style attributes
        nscoord specifiedCellWidth=-1;
        nsIStyleContextPtr cellSC;
        data->GetCellFrame()->GetStyleContext(aPresContext, cellSC.AssignRef());
        const nsStylePosition* cellPosition = (const nsStylePosition*)
          cellSC->GetStyleData(eStyleStruct_Position);
        switch (cellPosition->mWidth.GetUnit()) {
          case eStyleUnit_Coord:
            specifiedCellWidth = cellPosition->mWidth.GetCoordValue();
            if (gsDebug) printf("cell has specified coord width = %d\n", specifiedCellWidth);
            break;

          case eStyleUnit_Percent: 
          {
            nscoord tableWidth=0;
            nsIStyleContextPtr tableSC;
            mTableFrame->GetStyleContext(aPresContext, tableSC.AssignRef());
            PRBool tableIsAutoWidth = nsTableFrame::TableIsAutoWidth(mTableFrame, tableSC, aReflowState, tableWidth);
            float percent = cellPosition->mWidth.GetPercentValue();
            specifiedCellWidth = (PRInt32)(tableWidth*percent);
            if (gsDebug) printf("specified percent width %f of %d = %d\n", 
                                percent, tableWidth, specifiedCellWidth);
            break;
          }

          case eStyleUnit_Inherit:
            // XXX for now, do nothing
          default:
          case eStyleUnit_Auto:
            break;
        }
        if (-1!=specifiedCellWidth)
        {
          if (specifiedCellWidth>cellMinWidth)
          {
            if (gsDebug) printf("setting cellDesiredWidth from %d to %d\n", 
                                 cellDesiredWidth, specifiedCellWidth);
            cellDesiredWidth = specifiedCellWidth;    // TODO:  some math needed here for colspans
          }
        }

        if (PR_TRUE==gsDebug)
          printf("factoring in cell %d with colSpan=%d\n  factoring in min=%d and desired=%d\n", 
                 cellIndex, colSpan, cellMinWidth, cellDesiredWidth);
        if (minColWidth < cellMinWidth)
          minColWidth = cellMinWidth;
        if (maxColWidth < cellDesiredWidth)
          maxColWidth = cellDesiredWidth;
        // effectiveColumnWidth is the width as if no cells with colspans existed
        if ((1==colSpan) && (effectiveColumnWidths[colIndex] < maxColWidth))
          effectiveColumnWidths[colIndex] = cellDesiredWidth;
        if (gsDebug==PR_TRUE) 
          printf ("    after cell %d, minColWidth=%d  maxColWidth=%d  effColWidth[%d]=%d\n", 
                  cellIndex, minColWidth, maxColWidth, 
                  colIndex, effectiveColumnWidths[colIndex]);
        if (1<colSpan)
        { // add the cell to our list of spanners
          SpanInfo *spanInfo = new SpanInfo(colSpan-1, cellMinWidth, cellDesiredWidth);
          if (nsnull==spanList)
            spanList = new nsVoidArray();
          spanList->AppendElement(spanInfo);
        }
      }

      if (gsDebug==PR_TRUE) 
      {
        printf ("  for determining width of col %d %s:\n",
                colIndex, !IsFixedWidth(colPosition)? "(P)":"(A)");
        printf ("    minColWidth = %d and maxColWidth = %d\n", minColWidth, maxColWidth);
        printf ("    aAvailWidth = %d\n", aAvailWidth);
      }

      // Get column width if it has one
      nscoord specifiedProportionColumnWidth = -1;
      float specifiedPercentageColWidth = -1.0f;
      nscoord specifiedFixedColumnWidth = -1;
      PRBool isAutoWidth = PR_FALSE;
      switch (colPosition->mWidth.GetUnit()) {
        case eStyleUnit_Coord:
          specifiedFixedColumnWidth = colPosition->mWidth.GetCoordValue();
          if (gsDebug) printf("column %d has specified coord width = %d\n", colIndex, specifiedFixedColumnWidth);
          break;
        case eStyleUnit_Percent:
          specifiedPercentageColWidth = colPosition->mWidth.GetPercentValue();
          if (gsDebug) printf("column %d has specified percent width = %f\n", colIndex, specifiedPercentageColWidth);
          break;
        case eStyleUnit_Proportional:
          specifiedProportionColumnWidth = colPosition->mWidth.GetIntValue();
          if (gsDebug) printf("column %d has specified percent width = %d\n", colIndex, specifiedProportionColumnWidth);
          break;
        case eStyleUnit_Auto:
          isAutoWidth = PR_TRUE;
          break;
        default:
          break;
      }

      /* set the column width, knowing that the table fits in the available space */
      if (0==specifiedProportionColumnWidth || 0.0==specifiedPercentageColWidth)
      { // col width is specified to be the minimum
        mTableFrame->SetColumnWidth(colIndex, minColWidth);
        if (gsDebug==PR_TRUE) 
          printf ("  3 min: col %d set to min width = %d because style set proportionalWidth=0\n", 
                  colIndex, mTableFrame->GetColumnWidth(colIndex));
      }
      else if (PR_TRUE==isAutoWidth)
      {  // col width is determined by the cells' content,
         // so give each remaining column it's desired width (because we know we fit.)
         // if there is width left over, we'll factor that in after this loop is complete
        mTableFrame->SetColumnWidth(colIndex, maxColWidth);
        if (gsDebug==PR_TRUE) 
          printf ("  3 auto: col %d with availWidth %d, set to width = %d\n", 
                  colIndex, aAvailWidth, mTableFrame->GetColumnWidth(colIndex));
      }
      else if (-1!=specifiedProportionColumnWidth)
      { // we need to save these and do them after all other columns have been calculated
        /* the calculation will be: 
              sum up n, the total number of slices for the columns with proportional width
              compute the table "required" width, fixed-width + percentage-width +
                the sum of the proportional column's max widths (especially because in this routine I know the table fits)
              compute the remaining width: the required width - the used width (fixed + percentage)
              compute the width per slice
              set the width of each proportional-width column to it's number of slices * width per slice
        */
        mTableFrame->SetColumnWidth(colIndex, 0);   // set the column width to 0, since it isn't computed yet
        if (nsnull==proportionalColumnsList)
          proportionalColumnsList = new nsVoidArray();
        ProportionalColumnLayoutStruct * info = 
          new ProportionalColumnLayoutStruct(colIndex, minColWidth, 
                                             maxColWidth, specifiedProportionColumnWidth);
        proportionalColumnsList->AppendElement(info);
        totalSlices += specifiedProportionColumnWidth;  // keep track of the total number of proportions
        if (gsDebug==PR_TRUE) 
          printf ("  3 proportional: col %d with availWidth %d, gets %d slices with %d slices so far.\n", 
                  colIndex, aAvailWidth, specifiedProportionColumnWidth, totalSlices);
      }
      else if (-1!=specifiedFixedColumnWidth)
      { // the column has a specified pixel width
        // if the column was computed to be too small, enlarge the column
        nscoord resolvedWidth = specifiedFixedColumnWidth;
        if (specifiedFixedColumnWidth <= minColWidth)
          resolvedWidth = minColWidth;
        mTableFrame->SetColumnWidth(colIndex, resolvedWidth);
        if (gsDebug==PR_TRUE) 
          printf ("  3 fixed: col %d given %d\n", colIndex, resolvedWidth);
      }
      else
      {  // give the column a percentage of the remaining space
        PRInt32 percentage = -1;
        if (NS_UNCONSTRAINEDSIZE==aAvailWidth)
        { // since the "remaining space" is infinite, give the column it's max requested size
          mTableFrame->SetColumnWidth(colIndex, maxColWidth);
        }
        else
        {
          if (-1.0f != specifiedPercentageColWidth)
          {
            percentage = (PRInt32)(specifiedPercentageColWidth*100.0f); // TODO: rounding errors?
            // base the % on the total specified fixed width of the table
            mTableFrame->SetColumnWidth(colIndex, (percentage*aTableFixedWidth)/100);
            if (gsDebug==PR_TRUE) 
              printf ("  3 percent specified: col %d given %d percent of aTableFixedWidth %d, set to width = %d\n", 
                      colIndex, percentage, aTableFixedWidth, mTableFrame->GetColumnWidth(colIndex));
          }
          if (-1==percentage)
          {
            percentage = 100/numCols;
            // base the % on the remaining available width 
            mTableFrame->SetColumnWidth(colIndex, (percentage*aAvailWidth)/100);
            if (gsDebug==PR_TRUE) 
              printf ("  3 percent default: col %d given %d percent of aAvailWidth %d, set to width = %d\n", 
                      colIndex, percentage, aAvailWidth, mTableFrame->GetColumnWidth(colIndex));
          }
          // if the column was computed to be too small, enlarge the column
          if (mTableFrame->GetColumnWidth(colIndex) <= minColWidth)
          {
            mTableFrame->SetColumnWidth(colIndex, minColWidth);
            if (PR_TRUE==gsDebug) printf("  enlarging column to it's minimum = %d\n", minColWidth);
          }
        }
      }
    }
    tableWidth += mTableFrame->GetColumnWidth(colIndex);
  }

  /* --- post-process if necessary --- */
  // first, assign a width to proportional-width columns
  if (nsnull!=proportionalColumnsList)
  {
    // first, figure out the amount of space per slice
    nscoord maxWidthForTable = (0!=aTableFixedWidth) ? aTableFixedWidth : aMaxWidth;
    if (NS_UNCONSTRAINEDSIZE!=maxWidthForTable)
    {
      nscoord widthRemaining = maxWidthForTable - tableWidth;
      nscoord widthPerSlice = widthRemaining/totalSlices;
      PRInt32 numSpecifiedProportionalColumns = proportionalColumnsList->Count();
      for (PRInt32 i=0; i<numSpecifiedProportionalColumns; i++)
      { // for every proportionally-sized column, set the col width to the computed width
        ProportionalColumnLayoutStruct * info = 
          (ProportionalColumnLayoutStruct *)(proportionalColumnsList->ElementAt(i));
        nscoord computedColWidth = info->mProportion*widthPerSlice;
        mTableFrame->SetColumnWidth(info->mColIndex, computedColWidth);
        if (gsDebug==PR_TRUE) 
          printf ("  3 proportional step 2: col %d given %d proportion of remaining space %d, set to width = %d\n", 
                  info->mColIndex, info->mProportion, widthRemaining, computedColWidth);
        tableWidth += computedColWidth;
        effectiveColumnWidths[info->mColIndex] = computedColWidth;
        delete info;
      }
    }
    else
    { // we're in an unconstrained situation, so give each column the max of the max column widths
      PRInt32 numSpecifiedProportionalColumns = proportionalColumnsList->Count();
      PRInt32 maxOfMaxColWidths = 0;
      PRInt32 i;
      for (i=0; i<numSpecifiedProportionalColumns; i++)
      {
        ProportionalColumnLayoutStruct * info = 
          (ProportionalColumnLayoutStruct *)(proportionalColumnsList->ElementAt(i));
        if (maxOfMaxColWidths < info->mMaxColWidth)
          maxOfMaxColWidths = info->mMaxColWidth;
      }
      for (i=0; i<numSpecifiedProportionalColumns; i++)
      {
        ProportionalColumnLayoutStruct * info = 
          (ProportionalColumnLayoutStruct *)(proportionalColumnsList->ElementAt(i));
        mTableFrame->SetColumnWidth(info->mColIndex, maxOfMaxColWidths);
        if (gsDebug==PR_TRUE) 
          printf ("  3 proportional step 2 (unconstrained!): col %d set to width = %d\n", 
                  info->mColIndex, maxOfMaxColWidths);
        tableWidth += maxOfMaxColWidths;
        effectiveColumnWidths[info->mColIndex] = maxOfMaxColWidths;
        delete info;
      }
    }
    delete proportionalColumnsList;
  }

  // next, if the specified width of the table is greater than the table's computed width, expand the
  // table's computed width to match the specified width, giving the extra space to proportionately-sized
  // columns if possible.
  if ((PR_FALSE==aTableIsAutoWidth) && (aAvailWidth > tableWidth))
  {
    DistributeExcessSpace(aAvailWidth, tableWidth, effectiveColumnWidths);
  }
  // today we always allocate effectiveColumnWidths, but tomorrow we might be smarter, so leave the check in
  if (nsnull!=effectiveColumnWidths)
    delete [] effectiveColumnWidths;
  if (nsnull!=spanList)
    delete spanList;
  return result;
}

// take the extra space in the table and distribute it proportionately (based on desired width)
// give extra space to auto-width cells first, or if there are none to all cells
void BasicTableLayoutStrategy::DistributeExcessSpace(nscoord  aTableFixedWidth,
                                                     nscoord  aComputedTableWidth, 
                                                     nscoord *aColWidths)
{
  nscoord excess = 0;
  if (PR_TRUE==gsDebug) printf ("DistributeExcessSpace: fixed width %d > computed table width %d\n",
                                aTableFixedWidth, aComputedTableWidth);
  // if there are auto-sized columns, give them the extra space
  PRInt32 numAutoColumns=0;
  PRInt32 *autoColumns=nsnull;
  mTableFrame->GetColumnsByType(eStyleUnit_Auto, numAutoColumns, autoColumns);
  if (0!=numAutoColumns)
  {
    // there's at least one auto-width column, so give it (them) the extra space
    // proportionately distributed extra space, based on the column's desired size
    nscoord totalEffectiveWidthOfAutoColumns = 0;
    if (gsDebug==PR_TRUE) 
      printf("  aTableFixedWidth specified as %d, expanding columns by excess = %d\n", aTableFixedWidth, excess);
    // 1. first, get the total width of the auto columns
    PRInt32 i;
    for (i = 0; i<numAutoColumns; i++)
    {
      if (0!=aColWidths[autoColumns[i]])
        totalEffectiveWidthOfAutoColumns += aColWidths[autoColumns[i]];
      else
        totalEffectiveWidthOfAutoColumns += mTableFrame->GetColumnWidth(autoColumns[i]);
    }
    excess = aTableFixedWidth - aComputedTableWidth;
    // 2. next, compute the proportion to be added to each column, and add it
    for (i = 0; i<numAutoColumns; i++)
    {
      PRInt32 colIndex = autoColumns[i];
      nscoord oldColWidth = aColWidths[colIndex];
      float percent;
      if (0!=totalEffectiveWidthOfAutoColumns)
        percent = ((float)(aColWidths[colIndex]))/((float)totalEffectiveWidthOfAutoColumns);
      else
        percent = ((float)1)/((float)numAutoColumns);
      nscoord excessForThisColumn = (nscoord)(excess*percent);
      nscoord colWidth = excessForThisColumn+oldColWidth;
      if (gsDebug==PR_TRUE) 
        printf("  distribute excess to auto columns:  column %d was %d, now set to %d\n", 
               colIndex, aColWidths[colIndex], colWidth);
      mTableFrame->SetColumnWidth(colIndex, colWidth);
    }
  }
  // otherwise, distribute the space between all the columns 
  // (they must be all fixed and percentage-width columns, or we would have gone into the block above)
  else
  {
    excess = aTableFixedWidth - aComputedTableWidth;
    nscoord excessPerColumn = excess/mNumCols;
    if (gsDebug==PR_TRUE) 
      printf("  aTableFixedWidth specified as %d, expanding columns by excess = %d\n", aTableFixedWidth, excess);
    for (PRInt32 colIndex = 0; colIndex<mNumCols; colIndex++)
    {
      nscoord oldColWidth = aColWidths[colIndex];
      if (0==oldColWidth)
        oldColWidth = mTableFrame->GetColumnWidth(colIndex);
      float percent;
      if (0!=aComputedTableWidth)
        percent = (float)oldColWidth/(float)aComputedTableWidth;
      else
        percent = (float)1/(float)mNumCols;
      nscoord excessForThisColumn = (nscoord)(excess*percent);
      nscoord colWidth = excessForThisColumn+oldColWidth;
      if (gsDebug==PR_TRUE) 
        printf("  distribute excess to all columns:  column %d was %d, now set to %d\n", 
               colIndex, aColWidths[colIndex], colWidth);
      mTableFrame->SetColumnWidth(colIndex, colWidth);
    }
  }
}

/* this method is very similar to BalanaceColumnsTableFits, but there are enough
 * subtle differences that code reuse is impractical.  it's a shame.
 */
PRBool BasicTableLayoutStrategy::BalanceColumnsConstrained( nsIPresContext* aPresContext,
                                                            const nsReflowState& aReflowState,
                                                            nscoord aAvailWidth,
                                                            nscoord aMaxWidth,
                                                            nscoord aMinTableWidth, 
                                                            nscoord aMaxTableWidth)
{
#ifdef DEBUG
  nsIFrame *tablePIF=nsnull;
  mTableFrame->GetPrevInFlow(tablePIF);
  NS_ASSERTION(nsnull==tablePIF, "never ever call me on a continuing frame!");
#endif

  PRBool result = PR_TRUE;
  PRInt32 maxOfAllMinColWidths = 0;   // for the case where we have equal column widths, this is the smallest a column can be
  nscoord tableWidth=0;               // the width of the table as a result of setting column widths
  PRInt32 totalSlices=0;              // the total number of slices the proportional-width columns request
  nsVoidArray *proportionalColumnsList=nsnull; // a list of the columns that are proportional-width
  PRBool equalWidthColumns = PR_TRUE; // remember if we're in the special case where all
                                      // proportional-width columns are equal (if any are anything other than 1)
  PRBool atLeastOneAutoWidthColumn = PR_FALSE;  // true if at least one column is auto-width, requiring us to post-process
  nsVoidArray *spanList=nsnull;       // a list of the cells that span columns
  nsVoidArray *columnLayoutData = mTableFrame->GetColumnLayoutData();
  PRInt32 numCols = columnLayoutData->Count();
  nscoord * effectiveMaxColumnWidths = new nscoord[numCols]; // used for Nav4 compatible table fluffing
  nsCRT::memset (effectiveMaxColumnWidths, 0, numCols*sizeof(PRInt32));
  nscoord * effectiveMinColumnWidths = new nscoord[numCols]; // used for Nav4 compatible table fluffing
  nsCRT::memset (effectiveMinColumnWidths, 0, numCols*sizeof(PRInt32));
  for (PRInt32 colIndex = 0; colIndex<numCols; colIndex++)
  { 
    if (gsDebug==PR_TRUE) printf ("  for col %d\n", colIndex);
    nscoord minColWidth = 0;
    nscoord maxColWidth = 0;
    
    // Get column information
    nsColLayoutData * colData = (nsColLayoutData *)(columnLayoutData->ElementAt(colIndex));
    nsVoidArray *cells = colData->GetCells();
    PRInt32 numCells = cells->Count();

    // Get the columns's style
    nsTableColFrame *colFrame = colData->GetColFrame();
    NS_ASSERTION(nsnull!=colFrame, "bad col frame");
    nsIStyleContextPtr colSC;
    colFrame->GetStyleContext(aPresContext, colSC.AssignRef());
    const nsStylePosition* colPosition = (const nsStylePosition*) colSC->GetStyleData(eStyleStruct_Position);

    if (PR_FALSE==IsFixedWidth(colPosition))
    {
      // first, deal with any cells that span into this column from a pervious column
      if (nsnull!=spanList)
      {
        PRInt32 spanCount = spanList->Count();
        // go through the list backwards so we can delete easily
        for (PRInt32 spanIndex=spanCount-1; 0<=spanIndex; spanIndex--)
        {
          SpanInfo *spanInfo = (SpanInfo *)(spanList->ElementAt(spanIndex));
          if (minColWidth < spanInfo->cellMinWidth)
            minColWidth = spanInfo->cellMinWidth;
          if (maxColWidth < spanInfo->cellDesiredWidth)
            maxColWidth = spanInfo->cellDesiredWidth;
          spanInfo->span--;
          if (gsDebug==PR_TRUE) 
            printf ("    for spanning cell %d with remaining span=%d, min = %d and  des = %d\n", 
                    spanIndex, spanInfo->span, spanInfo->cellMinWidth, spanInfo->cellDesiredWidth);
          if (0==spanInfo->span)
          {
            spanList->RemoveElementAt(spanIndex);
            delete spanInfo;
          }
        }
      }
      for (PRInt32 cellIndex = 0; cellIndex<numCells; cellIndex++)
      { // this col has proportional width, so determine its width requirements
        nsCellLayoutData * data = (nsCellLayoutData *)(cells->ElementAt(cellIndex));
        if (nsnull == data) {
          // For cells that span rows there's only cell layout data for the first row
          continue;
        }

        PRInt32 colSpan = data->GetCellFrame()->GetColSpan();
        nsSize * cellMinSize = data->GetMaxElementSize();
        NS_ASSERTION(nsnull != cellMinSize, "bad cellMinSize");
        nsReflowMetrics * cellDesiredSize = data->GetDesiredSize();
        NS_ASSERTION(nsnull != cellDesiredSize, "bad cellDesiredSize");

        nsMargin  margin;
        nsresult  result = data->GetMargin(margin);        
        nscoord marginWidth = margin.left + margin.right;

        PRInt32 cellMinWidth = cellMinSize->width/colSpan + marginWidth;
        // first get the desired size info from reflow pass 1
        PRInt32 cellDesiredWidth = cellDesiredSize->width/colSpan + marginWidth;
        // then get the desired size info factoring in the cell style attributes
        nscoord specifiedCellWidth=-1;
        nsIStyleContextPtr cellSC;
        data->GetCellFrame()->GetStyleContext(aPresContext, cellSC.AssignRef());
        const nsStylePosition* cellPosition = (const nsStylePosition*)
          cellSC->GetStyleData(eStyleStruct_Position);
        switch (cellPosition->mWidth.GetUnit()) {
          case eStyleUnit_Coord:
            specifiedCellWidth = cellPosition->mWidth.GetCoordValue();
            if (gsDebug) printf("cell has specified coord width = %d\n", specifiedCellWidth);
            break;

          case eStyleUnit_Percent: 
          {
            nscoord knownTableWidth=0;
            nsIStyleContextPtr tableSC;
            mTableFrame->GetStyleContext(aPresContext, tableSC.AssignRef());
            PRBool tableIsAutoWidth = nsTableFrame::TableIsAutoWidth(mTableFrame, tableSC, aReflowState, knownTableWidth);
            float percent = cellPosition->mWidth.GetPercentValue();
            specifiedCellWidth = (PRInt32)(knownTableWidth*percent);
            if (gsDebug) printf("specified percent width %f of %d = %d\n", 
                                percent, knownTableWidth, specifiedCellWidth);
            break;
          }

          case eStyleUnit_Inherit:
            // XXX for now, do nothing
          default:
          case eStyleUnit_Auto:
            break;
        }
        if (-1!=specifiedCellWidth)
        {
          if (specifiedCellWidth>cellMinWidth)
          {
            if (gsDebug) printf("setting cellDesiredWidth from %d to %d\n", 
                                 cellDesiredWidth, specifiedCellWidth);
            cellDesiredWidth = specifiedCellWidth;    // TODO:  some math needed here for colspans
          }
        }

        if (PR_TRUE==gsDebug)
          printf("factoring in cell %d with colSpan=%d\n  factoring in min=%d and desired=%d\n", 
                 cellIndex, colSpan, cellMinWidth, cellDesiredWidth);
        // remember the widest min cell width
        if (minColWidth < cellMinWidth)
          minColWidth = cellMinWidth;
        // effectiveMinColumnWidth is the width as if no cells with colspans existed
        if ((1==colSpan) && (effectiveMinColumnWidths[colIndex] < minColWidth))
          effectiveMinColumnWidths[colIndex] = cellMinWidth;
        // remember the max desired cell width
        if (maxColWidth < cellDesiredWidth)
          maxColWidth = cellDesiredWidth;
        // effectiveMaxColumnWidth is the width as if no cells with colspans existed
        if ((1==colSpan) && (effectiveMaxColumnWidths[colIndex] < maxColWidth))
          effectiveMaxColumnWidths[colIndex] = cellDesiredWidth;
        if (gsDebug==PR_TRUE) 
          printf ("    after cell %d, minColWidth=%d  maxColWidth=%d  effColWidth[%d]=%d,%d\n", 
                  cellIndex, minColWidth, maxColWidth, 
                  colIndex, effectiveMaxColumnWidths[colIndex], effectiveMaxColumnWidths[colIndex]);
        if (1<colSpan)
        {
          // add the cell to our list of spanners
          SpanInfo *spanInfo = new SpanInfo(colSpan-1, cellMinWidth, cellDesiredWidth);
          if (nsnull==spanList)
            spanList = new nsVoidArray();
          spanList->AppendElement(spanInfo);
        }
      }

      if (gsDebug==PR_TRUE) 
      {
        printf ("  for determining width of col %d %s:\n",
                colIndex, !IsFixedWidth(colPosition)? "(P)":"(A)");
        printf ("    minTableWidth = %d and maxTableWidth = %d\n", 
                aMinTableWidth, aMaxTableWidth);
        printf ("    minColWidth = %d, maxColWidth = %d, eff=%d,%d\n", 
                minColWidth, maxColWidth, effectiveMinColumnWidths[colIndex], 
                effectiveMaxColumnWidths[colIndex]);
        printf ("    aAvailWidth = %d\n", aAvailWidth);
      }

      // Get column width if it has one
      nscoord specifiedProportionColumnWidth = -1;
      float specifiedPercentageColWidth = -1.0f;
      nscoord specifiedFixedColumnWidth = -1;
      PRBool isAutoWidth = PR_FALSE;
      switch (colPosition->mWidth.GetUnit()) {
        case eStyleUnit_Coord:
          specifiedFixedColumnWidth = colPosition->mWidth.GetCoordValue();
          if (gsDebug) printf("column %d has specified coord width = %d\n", colIndex, specifiedFixedColumnWidth);
          break;
        case eStyleUnit_Percent:
          specifiedPercentageColWidth = colPosition->mWidth.GetPercentValue();
          if (gsDebug) printf("column %d has specified percent width = %f\n", colIndex, specifiedPercentageColWidth);
          break;
        case eStyleUnit_Proportional:
          specifiedProportionColumnWidth = colPosition->mWidth.GetIntValue();
          if (gsDebug) printf("column %d has specified percent width = %d\n", colIndex, specifiedProportionColumnWidth);
          break;
        case eStyleUnit_Auto:
          isAutoWidth = PR_TRUE;
          break;
        default:
          break;
      }

      /* set the column width, knowing that the table fits in the available space */
      if (0==specifiedProportionColumnWidth || 0.0==specifiedPercentageColWidth)
      { // col width is specified to be the minimum
        mTableFrame->SetColumnWidth(colIndex, minColWidth);
        if (gsDebug==PR_TRUE) 
          printf ("  4 (0): col %d set to min width = %d because style set proportionalWidth=0\n", 
                  colIndex, mTableFrame->GetColumnWidth(colIndex));
      }
      else if (1==numCols)
      { // there is only one column, and we know that it's desired width doesn't fit
        // so the column should be as wide as the available space allows it to be
        if (gsDebug==PR_TRUE) printf ("  4 one-column:  col %d  set to width = %d\n", colIndex, aAvailWidth);
        mTableFrame->SetColumnWidth(colIndex, aAvailWidth);
        if (gsDebug==PR_TRUE) 
          printf ("  4 one-col:  col %d set to width = %d from available width %d\n", 
                  colIndex, mTableFrame->GetColumnWidth(colIndex), aAvailWidth);
      }
      else if (PR_TRUE==isAutoWidth)
      { // column's width is determined by its content, done in post-processing
        mTableFrame->SetColumnWidth(colIndex, 0); // set to 0 width for now
        atLeastOneAutoWidthColumn = PR_TRUE;
      }
      else if (-1!=specifiedProportionColumnWidth)
      { // we need to save these and do them after all other columns have been calculated
        /* the calculation will be: 
              sum up n, the total number of slices for the columns with proportional width
              compute the table "required" width, fixed-width + percentage-width +
                the sum of the proportional column's max widths (especially because in this routine I know the table fits)
              compute the remaining width: the required width - the used width (fixed + percentage)
              compute the width per slice
              set the width of each proportional-width column to it's number of slices * width per slice
        */
        mTableFrame->SetColumnWidth(colIndex, 0);   // set the column width to 0, since it isn't computed yet
        if (nsnull==proportionalColumnsList)
          proportionalColumnsList = new nsVoidArray();
        ProportionalColumnLayoutStruct * info = 
          new ProportionalColumnLayoutStruct(colIndex, minColWidth, maxColWidth, specifiedProportionColumnWidth);
        proportionalColumnsList->AppendElement(info);
        totalSlices += specifiedProportionColumnWidth;
        if (1!=specifiedProportionColumnWidth)
          equalWidthColumns = PR_FALSE;
      }
      else if (-1!=specifiedFixedColumnWidth)
      { // the column has a specified pixel width
        // if the column was computed to be too small, enlarge the column
        nscoord resolvedWidth = specifiedFixedColumnWidth;
        if (specifiedFixedColumnWidth <= minColWidth)
          resolvedWidth = minColWidth;
        mTableFrame->SetColumnWidth(colIndex, resolvedWidth);
        if (gsDebug==PR_TRUE) 
          printf ("  4 fixed: col %d given %d\n", colIndex, resolvedWidth);
      }
      else
      {  // give the column a percentage of the remaining space
        PRInt32 percentage = -1;
        if (NS_UNCONSTRAINEDSIZE==aAvailWidth)
        { // since the "remaining space" is infinite, give the column it's max requested size
          mTableFrame->SetColumnWidth(colIndex, maxColWidth);
        }
        else
        {
          if (-1.0f != specifiedPercentageColWidth)
          {
            percentage = (PRInt32)(specifiedPercentageColWidth*100.0f); // TODO: rounding errors?
            // base the % on the total specified fixed width of the table
            mTableFrame->SetColumnWidth(colIndex, (percentage*aMaxWidth)/100);
            if (gsDebug==PR_TRUE) 
              printf ("  4 percent specified: col %d given %d percent of aMaxWidth %d, set to width = %d\n", 
                      colIndex, percentage, aMaxWidth, mTableFrame->GetColumnWidth(colIndex));
          }
          if (-1==percentage)
          {
            percentage = 100/numCols;
            // base the % on the remaining available width 
            mTableFrame->SetColumnWidth(colIndex, (percentage*aAvailWidth)/100);
            if (gsDebug==PR_TRUE) 
              printf ("  4 percent default: col %d given %d percent of aAvailWidth %d, set to width = %d\n", 
                      colIndex, percentage, aAvailWidth, mTableFrame->GetColumnWidth(colIndex));
          }
          // if the column was computed to be too small, enlarge the column
          if (mTableFrame->GetColumnWidth(colIndex) <= minColWidth)
          {
            mTableFrame->SetColumnWidth(colIndex, minColWidth);
            if (PR_TRUE==gsDebug) printf("  enlarging column to it's minimum = %d\n", minColWidth);
            if (maxOfAllMinColWidths < minColWidth)
            {
              maxOfAllMinColWidths = minColWidth;
              if (PR_TRUE==gsDebug)
                printf("   and setting maxOfAllMins to %d\n", maxOfAllMinColWidths);
            }
          }
        }
      }
    }
  }
  /* --- post-process if necessary --- */
  if (PR_TRUE==atLeastOneAutoWidthColumn)
  { // proportionately distribute the remaining space to autowidth columns
    DistributeRemainingSpace(aAvailWidth, tableWidth, effectiveMinColumnWidths, effectiveMaxColumnWidths);
  }
  // second, assign a width to proportional-width columns
  if (nsnull!=proportionalColumnsList)
  {
    // first, figure out the amount of space per slice
    nscoord widthRemaining = aMaxWidth - tableWidth;
    nscoord widthPerSlice = widthRemaining/totalSlices;
    PRInt32 numSpecifiedProportionalColumns = proportionalColumnsList->Count();
    for (PRInt32 i=0; i<numSpecifiedProportionalColumns; i++)
    {
      ProportionalColumnLayoutStruct * info = 
        (ProportionalColumnLayoutStruct *)(proportionalColumnsList->ElementAt(i));
      if (PR_TRUE==equalWidthColumns && 0!=maxOfAllMinColWidths)
      {
        if (gsDebug==PR_TRUE) 
          printf("  EqualColWidths specified and some column couldn't fit, so setting col %d width to %d\n", 
                 info->mColIndex, maxOfAllMinColWidths);
        mTableFrame->SetColumnWidth(info->mColIndex, maxOfAllMinColWidths);
      }
      else
      {
        nscoord computedColWidth = info->mProportion*widthPerSlice;
        mTableFrame->SetColumnWidth(info->mColIndex, computedColWidth);
        if (gsDebug==PR_TRUE) 
          printf ("  4 proportion: col %d given %d proportion of remaining space %d, set to width = %d\n", 
                  info->mColIndex, info->mProportion, widthRemaining, computedColWidth);
      }
      delete info;
    }
    delete proportionalColumnsList;
  }

  // clean up
  // today we always allocate effectiveColumnWidths, but tomorrow we might be smarter, so leave the check in
  if (nsnull!=effectiveMinColumnWidths)
    delete [] effectiveMinColumnWidths;  
  if (nsnull!=effectiveMaxColumnWidths)
    delete [] effectiveMaxColumnWidths;
  if (nsnull!=spanList)
    delete spanList;

  return result;
}

//XXX: make sure aTFW takes into account fixed width cells already assigned
// take the remaining space in the table and distribute it proportionately 
// to the auto-width cells in the table (based on desired width)
void BasicTableLayoutStrategy::DistributeRemainingSpace(nscoord  aTableFixedWidth,
                                                        nscoord  aComputedTableWidth, 
                                                        nscoord *aMinColWidths,
                                                        nscoord *aMaxColWidths)
{
  if (PR_TRUE==gsDebug) 
    printf ("DistributeRemainingSpace: fixed width %d > computed table width %d\n",
            aTableFixedWidth, aComputedTableWidth);
  // if there are auto-sized columns, give them the extra space
  PRInt32 numAutoColumns=0;
  PRInt32 *autoColumns=nsnull;
  // availWidth is the difference between the total available width and the 
  // amount of space already assigned, assuming auto col widths were assigned 0.
  nscoord availWidth = aTableFixedWidth - aComputedTableWidth;
  mTableFrame->GetColumnsByType(eStyleUnit_Auto, numAutoColumns, autoColumns);
  if (0!=numAutoColumns)
  {
    // there's at least one auto-width column, so give it (them) the extra space
    // proportionately distributed extra space, based on the column's desired size
    nscoord totalEffectiveWidthOfAutoColumns = 0;
    // 1. first, get the total width of the auto columns
    PRInt32 i;
    for (i = 0; i<numAutoColumns; i++)
    {
      if (0!=aMaxColWidths[autoColumns[i]])
        totalEffectiveWidthOfAutoColumns += aMaxColWidths[autoColumns[i]];
      else
        totalEffectiveWidthOfAutoColumns += mTableFrame->GetColumnWidth(autoColumns[i]);
    }
    if (gsDebug==PR_TRUE) 
      printf("  aTableFixedWidth specified as %d, availWidth is = %d\n", 
             aTableFixedWidth, availWidth);
    
    // 2. next, compute the proportion to be added to each column, and add it
    for (i = 0; i<numAutoColumns; i++)
    {
      PRInt32 colIndex = autoColumns[i];
      float percent;
      if (0!=aTableFixedWidth)
        percent = ((float)(aMaxColWidths[colIndex]))/((float)totalEffectiveWidthOfAutoColumns);
      else
        percent = ((float)1)/((float)numAutoColumns);
      nscoord colWidth = (nscoord)(availWidth*percent);
      if (colWidth<aMinColWidths[colIndex])
      {
        availWidth -= (aMinColWidths[colIndex]-colWidth);
        colWidth=aMinColWidths[colIndex];
      }
      if (gsDebug==PR_TRUE) 
        printf("  distribute width to auto columns:  column %d was %d, now set to %d\n", 
               colIndex, aMaxColWidths[colIndex], colWidth);
      mTableFrame->SetColumnWidth(colIndex, colWidth);
    }
  }
}


