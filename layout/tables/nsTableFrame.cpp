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
#include "nsTableFrame.h"
#include "nsIRenderingContext.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIContent.h"
#include "nsCellMap.h"
#include "nsTableCellFrame.h"
#include "nsHTMLParts.h"
#include "nsTableColFrame.h"
#include "nsTableColGroupFrame.h"
#include "nsTableRowFrame.h"
#include "nsTableRowGroupFrame.h"
#include "nsTableOuterFrame.h"
#include "nsIHTMLContent.h"

#include "BasicTableLayoutStrategy.h"
#include "FixedTableLayoutStrategy.h"

#include "nsIPresContext.h"
#include "nsCSSRendering.h"
#include "nsStyleConsts.h"
#include "nsVoidArray.h"
#include "nsIPtr.h"
#include "nsIView.h"
#include "nsHTMLAtoms.h"
#include "nsHTMLIIDs.h"
#include "nsIReflowCommand.h"
#include "nsLayoutAtoms.h"
#include "nsIDeviceContext.h"
#include "nsIStyleSet.h"
#include "nsIPresShell.h"
#include "nsIDOMElement.h"
#include "nsIDOMHTMLElement.h"
#include "nsIDOMHTMLBodyElement.h"
#include "nsISizeOfHandler.h"

NS_DEF_PTR(nsIStyleContext);
NS_DEF_PTR(nsIContent);

static NS_DEFINE_IID(kIHTMLElementIID, NS_IDOMHTMLELEMENT_IID);
static NS_DEFINE_IID(kIBodyElementIID, NS_IDOMHTMLBODYELEMENT_IID);
static NS_DEFINE_IID(kITableRowGroupFrameIID, NS_ITABLEROWGROUPFRAME_IID);

static const PRInt32 kColumnWidthIncrement=10;

#if 1
PRBool nsDebugTable::gRflTableOuter = PR_FALSE;
PRBool nsDebugTable::gRflTable      = PR_FALSE;
PRBool nsDebugTable::gRflRowGrp     = PR_FALSE;
PRBool nsDebugTable::gRflRow        = PR_FALSE;
PRBool nsDebugTable::gRflCell       = PR_FALSE;
PRBool nsDebugTable::gRflArea       = PR_FALSE;
#else
PRBool nsDebugTable::gRflTableOuter = PR_TRUE;
PRBool nsDebugTable::gRflTable      = PR_TRUE;
PRBool nsDebugTable::gRflRowGrp     = PR_TRUE;
PRBool nsDebugTable::gRflRow        = PR_TRUE;
PRBool nsDebugTable::gRflCell       = PR_TRUE;
PRBool nsDebugTable::gRflArea       = PR_TRUE;
#endif
/* ----------- InnerTableReflowState ---------- */

struct InnerTableReflowState {

  // Our reflow state
  const nsHTMLReflowState& reflowState;

  // The body's available size (computed from the body's parent)
  nsSize availSize;

  // Flags for whether the max size is unconstrained
  PRBool  unconstrainedWidth;
  PRBool  unconstrainedHeight;

  // Running y-offset
  nscoord y;

  // Pointer to the footer in the table
  nsIFrame* footerFrame;

  // The first body section row group frame, i.e. not a header or footer
  nsIFrame* firstBodySection;

  // border/padding
  nsMargin  mBorderPadding;

  InnerTableReflowState(nsIPresContext&          aPresContext,
                        const nsHTMLReflowState& aReflowState,
                        const nsMargin&          aBorderPadding)
    : reflowState(aReflowState), mBorderPadding(aBorderPadding)
  {
    y=0;  // border/padding???

    unconstrainedWidth = PRBool(aReflowState.availableWidth == NS_UNCONSTRAINEDSIZE);
    availSize.width = aReflowState.availableWidth;
    if (!unconstrainedWidth) {
      availSize.width -= aBorderPadding.left + aBorderPadding.right;
    }

    unconstrainedHeight = PRBool(aReflowState.availableHeight == NS_UNCONSTRAINEDSIZE);
    availSize.height = aReflowState.availableHeight;
    if (!unconstrainedHeight) {
      availSize.height -= aBorderPadding.top + aBorderPadding.bottom;
    }

    footerFrame = nsnull;
    firstBodySection = nsnull;
  }
};

/* ----------- ColumnInfoCache ---------- */

static const PRInt32 NUM_COL_WIDTH_TYPES=4;

class ColumnInfoCache
{
public:
  ColumnInfoCache(PRInt32 aNumColumns);
  ~ColumnInfoCache();

  void AddColumnInfo(const nsStyleUnit aType, 
                     PRInt32 aColumnIndex);

  void GetColumnsByType(const nsStyleUnit aType, 
                        PRInt32& aOutNumColumns,
                        PRInt32 *& aOutColumnIndexes);
  enum ColWidthType {
    eColWidthType_Auto         = 0,      // width based on contents
    eColWidthType_Percent      = 1,      // (float) 1.0 == 100%
    eColWidthType_Coord        = 2,      // (nscoord) value is twips
    eColWidthType_Proportional = 3       // (int) value has proportional meaning
  };

#ifdef DEBUG
  void SizeOf(nsISizeOfHandler* aHandler, PRUint32* aResult) const;
#endif

private:
  PRInt32  mColCounts [4];
  PRInt32 *mColIndexes[4];
  PRInt32  mNumColumns;
};

ColumnInfoCache::ColumnInfoCache(PRInt32 aNumColumns)
{
  mNumColumns = aNumColumns;
  for (PRInt32 i=0; i<NUM_COL_WIDTH_TYPES; i++)
  {
    mColCounts[i] = 0;
    mColIndexes[i] = nsnull;
  }
}

ColumnInfoCache::~ColumnInfoCache()
{
  for (PRInt32 i=0; i<NUM_COL_WIDTH_TYPES; i++)
  {
    if (nsnull!=mColIndexes[i])
    {
      delete [] mColIndexes[i];
    }
  }
}

void ColumnInfoCache::AddColumnInfo(const nsStyleUnit aType, 
                                    PRInt32 aColumnIndex)
{
  // a table may have more COLs than actual columns, so we guard against that here
  if (aColumnIndex<mNumColumns)
  {
    switch (aType)
    {
      case eStyleUnit_Auto:
        if (nsnull==mColIndexes[eColWidthType_Auto])
          mColIndexes[eColWidthType_Auto] = new PRInt32[mNumColumns];     // TODO : be much more efficient
        mColIndexes[eColWidthType_Auto][mColCounts[eColWidthType_Auto]] = aColumnIndex;
        mColCounts[eColWidthType_Auto]++;
        break;

      case eStyleUnit_Percent:
        if (nsnull==mColIndexes[eColWidthType_Percent])
          mColIndexes[eColWidthType_Percent] = new PRInt32[mNumColumns];     // TODO : be much more efficient
        mColIndexes[eColWidthType_Percent][mColCounts[eColWidthType_Percent]] = aColumnIndex;
        mColCounts[eColWidthType_Percent]++;
        break;

      case eStyleUnit_Coord:
        if (nsnull==mColIndexes[eColWidthType_Coord])
          mColIndexes[eColWidthType_Coord] = new PRInt32[mNumColumns];     // TODO : be much more efficient
        mColIndexes[eColWidthType_Coord][mColCounts[eColWidthType_Coord]] = aColumnIndex;
        mColCounts[eColWidthType_Coord]++;
        break;

      case eStyleUnit_Proportional:
        if (nsnull==mColIndexes[eColWidthType_Proportional])
          mColIndexes[eColWidthType_Proportional] = new PRInt32[mNumColumns];     // TODO : be much more efficient
        mColIndexes[eColWidthType_Proportional][mColCounts[eColWidthType_Proportional]] = aColumnIndex;
        mColCounts[eColWidthType_Proportional]++;
        break;

      default:
        break;
    }
  }
}


void ColumnInfoCache::GetColumnsByType(const nsStyleUnit aType, 
                                       PRInt32& aOutNumColumns,
                                       PRInt32 *& aOutColumnIndexes)
{
  // initialize out-params
  aOutNumColumns=0;
  aOutColumnIndexes=nsnull;
  
  // fill out params with column info based on aType
  switch (aType)
  {
    case eStyleUnit_Auto:
      aOutNumColumns = mColCounts[eColWidthType_Auto];
      aOutColumnIndexes = mColIndexes[eColWidthType_Auto];
      break;
    case eStyleUnit_Percent:
      aOutNumColumns = mColCounts[eColWidthType_Percent];
      aOutColumnIndexes = mColIndexes[eColWidthType_Percent];
      break;
    case eStyleUnit_Coord:
      aOutNumColumns = mColCounts[eColWidthType_Coord];
      aOutColumnIndexes = mColIndexes[eColWidthType_Coord];
      break;
    case eStyleUnit_Proportional:
      aOutNumColumns = mColCounts[eColWidthType_Proportional];
      aOutColumnIndexes = mColIndexes[eColWidthType_Proportional];
      break;

    default:
      break;
  }
}

#ifdef DEBUG
void ColumnInfoCache::SizeOf(nsISizeOfHandler* aHandler, PRUint32* aResult) const
{
  NS_PRECONDITION(aResult, "null OUT parameter pointer");
  PRUint32 sum = sizeof(*this);

  // Now add in the space talen up by the arrays
  for (int i = 0; i < 4; i++) {
    if (mColIndexes[i]) {
      sum += mNumColumns * sizeof(PRInt32);
    }
  }

  *aResult = sum;
}
#endif

NS_IMETHODIMP
nsTableFrame::GetFrameType(nsIAtom** aType) const
{
  NS_PRECONDITION(nsnull != aType, "null OUT parameter pointer");
  *aType = nsLayoutAtoms::tableFrame; 
  NS_ADDREF(*aType);
  return NS_OK;
}


/* --------------------- nsTableFrame -------------------- */

nsTableFrame::nsTableFrame()
  : nsHTMLContainerFrame(),
    mCellMap(nsnull),
    mColCache(nsnull),
    mTableLayoutStrategy(nsnull),
    mPercentBasisForRows(0)
{
  mBits.mColumnWidthsSet = PR_FALSE;
  mBits.mColumnWidthsValid = PR_FALSE;
  mBits.mFirstPassValid = PR_FALSE;
  mBits.mColumnCacheValid = PR_FALSE;
  mBits.mCellMapValid = PR_TRUE;
  mBits.mIsInvariantWidth = PR_FALSE;
  mBits.mHasScrollableRowGroup = PR_FALSE;
  // XXX We really shouldn't do this, but if we don't then we'll have a
  // problem with the tree control...
#if 0
  mColumnWidthsLength = 0;  
  mColumnWidths = nsnull;
#else
  mColumnWidthsLength = 10;  
  mColumnWidths = new PRInt32[mColumnWidthsLength];
  nsCRT::memset (mColumnWidths, 0, mColumnWidthsLength*sizeof(PRInt32));
#endif
  mCellMap = new nsCellMap(0, 0);
}

NS_IMPL_ADDREF_INHERITED(nsTableFrame, nsHTMLContainerFrame)
NS_IMPL_RELEASE_INHERITED(nsTableFrame, nsHTMLContainerFrame)

nsresult nsTableFrame::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  if (NULL == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(nsITableLayout::GetIID())) 
  { // note there is no addref here, frames are not addref'd
    *aInstancePtr = (void*)(nsITableLayout*)this;
    return NS_OK;
  } else {
    return nsHTMLContainerFrame::QueryInterface(aIID, aInstancePtr);
  }
}

NS_IMETHODIMP
nsTableFrame::Init(nsIPresContext&  aPresContext,
                   nsIContent*      aContent,
                   nsIFrame*        aParent,
                   nsIStyleContext* aContext,
                   nsIFrame*        aPrevInFlow)
{
  nsresult  rv;

  // Let the base class do its processing
  rv = nsHTMLContainerFrame::Init(aPresContext, aContent, aParent, aContext,
                                  aPrevInFlow);

  if (aPrevInFlow) {
    // set my width, because all frames in a table flow are the same width and
    // code in nsTableOuterFrame depends on this being set
    nsSize  size;
    aPrevInFlow->GetSize(size);
    mRect.width = size.width;
  }

  return rv;
}


nsTableFrame::~nsTableFrame()
{
  if ((NS_STYLE_BORDER_COLLAPSE==GetBorderCollapseStyle()) && mBorderEdges)
  {
		PRInt32 i=0;
		for ( ; i<4; i++)
		{
			nsBorderEdge *border = (nsBorderEdge *)(mBorderEdges->mEdges[i].ElementAt(0));
			while (border) 
			{
				delete border;
				mBorderEdges->mEdges[i].RemoveElementAt(0);
				border = (nsBorderEdge *)(mBorderEdges->mEdges[i].ElementAt(0));
			}
		}
    delete mBorderEdges;
  }

  if (nsnull!=mCellMap) {
    delete mCellMap; 
    mCellMap = nsnull;
  } 

  if (nsnull!=mColumnWidths) {
    delete [] mColumnWidths;
    mColumnWidths = nsnull;
  }

  if (nsnull!=mTableLayoutStrategy) {
    delete mTableLayoutStrategy;
    mTableLayoutStrategy = nsnull;
  }

  if (nsnull!=mColCache) {
    delete mColCache;
    mColCache = nsnull;
  }

}

NS_IMETHODIMP
nsTableFrame::Destroy(nsIPresContext& aPresContext)
{
  mColGroups.DestroyFrames(aPresContext);
  return nsHTMLContainerFrame::Destroy(aPresContext);
}

NS_IMETHODIMP
nsTableFrame::SetInitialChildList(nsIPresContext& aPresContext,
                                  nsIAtom*        aListName,
                                  nsIFrame*       aChildList)
{
  nsresult rv=NS_OK;

  // I know now that I have all my children, so build the cell map
  nsIFrame *childFrame = aChildList;
  nsIFrame *prevMainChild = nsnull;
  nsIFrame *prevColGroupChild = nsnull;
  for ( ; nsnull!=childFrame; )
  {
    const nsStyleDisplay *childDisplay;
    childFrame->GetStyleData(eStyleStruct_Display, (const nsStyleStruct *&)childDisplay);
    if (PR_TRUE==IsRowGroup(childDisplay->mDisplay))
    {
      if (mFrames.IsEmpty())
        mFrames.SetFrames(childFrame);
      else
        prevMainChild->SetNextSibling(childFrame);
      // If we have a prev-in-flow, then we're a table that has been split and
      // so don't treat this like an append
      if (!mPrevInFlow) {
        rv = DidAppendRowGroup(GetRowGroupFrameFor(childFrame, childDisplay));
      }
      prevMainChild = childFrame;
    }
    else if (NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP == childDisplay->mDisplay)
    {
      if (mColGroups.IsEmpty())
        mColGroups.SetFrames(childFrame);
      else
        prevColGroupChild->SetNextSibling(childFrame);
      prevColGroupChild = childFrame;
    }
    else
    { // unknown frames go on the main list for now
      if (mFrames.IsEmpty())
        mFrames.SetFrames(childFrame);
      else
        prevMainChild->SetNextSibling(childFrame);
      prevMainChild = childFrame;
    }
    nsIFrame *prevChild = childFrame;
    childFrame->GetNextSibling(&childFrame);
    prevChild->SetNextSibling(nsnull);
  }
  if (nsnull!=prevMainChild)
    prevMainChild->SetNextSibling(nsnull);
  if (nsnull!=prevColGroupChild)
    prevColGroupChild->SetNextSibling(nsnull);

  if (NS_SUCCEEDED(rv)) {
    PRBool  createdColFrames;
    EnsureColumns(aPresContext, createdColFrames);
  }

  if (HasGroupRules()) {
    ProcessGroupRules(aPresContext);
  }

  return rv;
}

NS_IMETHODIMP nsTableFrame::DidAppendRowGroup(nsTableRowGroupFrame *aRowGroupFrame)
{
  nsresult rv=NS_OK;
  nsIFrame *nextRow=nsnull;
  aRowGroupFrame->FirstChild(nsnull, &nextRow);
  for ( ; nsnull!=nextRow; nextRow->GetNextSibling(&nextRow))
  {
    const nsStyleDisplay *rowDisplay;
    nextRow->GetStyleData(eStyleStruct_Display, (const nsStyleStruct *&)rowDisplay);
    if (NS_STYLE_DISPLAY_TABLE_ROW==rowDisplay->mDisplay) {
      rv = ((nsTableRowFrame *)nextRow)->InitChildren();
      if (NS_FAILED(rv)) {
        return rv;
      }
    }
    else if (NS_STYLE_DISPLAY_TABLE_ROW_GROUP==rowDisplay->mDisplay) {
      rv = DidAppendRowGroup((nsTableRowGroupFrame*)nextRow);
      if (NS_FAILED(rv)) {
        return rv;
      }
    }
  }

  return rv;
}


/* ****** CellMap methods ******* */

/* counts columns in column groups */
PRInt32 nsTableFrame::GetSpecifiedColumnCount ()
{
  PRInt32 colCount = 0;
  nsIFrame * childFrame = mColGroups.FirstChild();
  while (nsnull!=childFrame) {
    colCount += ((nsTableColGroupFrame *)childFrame)->GetColumnCount();
    childFrame->GetNextSibling(&childFrame);
  }    
  return colCount;
}

PRInt32 nsTableFrame::GetRowCount () const
{
  PRInt32 rowCount = 0;
  nsCellMap *cellMap = GetCellMap();
  NS_ASSERTION(nsnull!=cellMap, "GetRowCount null cellmap");
  if (nsnull!=cellMap)
    rowCount = cellMap->GetRowCount();
  return rowCount;
}

/* return the effective col count */
PRInt32 nsTableFrame::GetColCount ()
{
  PRInt32 colCount = 0;
  nsCellMap* cellMap = GetCellMap();
  NS_ASSERTION(nsnull != cellMap, "GetColCount null cellmap");
  if (nsnull != cellMap)
    colCount = cellMap->GetColCount();
  return colCount;
}


nsTableColFrame * nsTableFrame::GetColFrame(PRInt32 aColIndex)
{
  nsTableColFrame *result = nsnull;
  nsCellMap *cellMap = GetCellMap();
  if (nsnull!=cellMap)
  {
    result = cellMap->GetColumnFrame(aColIndex);
  }
  return result;
}

// can return nsnull
nsTableCellFrame* nsTableFrame::GetCellFrameAt(PRInt32 aRowIndex, PRInt32 aColIndex)
{
  nsCellMap* cellMap = GetCellMap();
  if (cellMap) 
    return cellMap->GetCellInfoAt(aRowIndex, aColIndex);
  return nsnull;
}

// return the number of rows spanned by aCell starting at aRowIndex
// note that this is different from just the rowspan of aCell
// (that would be GetEffectiveRowSpan (indexOfRowThatContains_aCell, aCell)
//
// XXX This code should be in the table row group frame instead, and it
// should clip rows spans so they don't extend past a row group rather than
// clip to the table itself. Before that can happen the code that builds the
// cell map needs to take row groups into account
PRInt32 nsTableFrame::GetEffectiveRowSpan (PRInt32 aRowIndex, nsTableCellFrame *aCell)
{
  NS_PRECONDITION (nsnull!=aCell, "bad cell arg");
  NS_PRECONDITION (0<=aRowIndex && aRowIndex<GetRowCount(), "bad row index arg");

  if (!(0<=aRowIndex && aRowIndex<GetRowCount()))
    return 1;

  // XXX I don't think this is correct...
#if 0
  PRInt32 rowSpan = aCell->GetRowSpan();
  PRInt32 rowCount = GetRowCount();
  if (rowCount < (aRowIndex + rowSpan))
    return (rowCount - aRowIndex);
  return rowSpan;
#else
  PRInt32 rowSpan = aCell->GetRowSpan();
  PRInt32 rowCount = GetRowCount();
  PRInt32 startRow;
  aCell->GetRowIndex(startRow);

  // Clip the row span so it doesn't extend past the bottom of the table
  if ((startRow + rowSpan) > rowCount) {
    rowSpan = rowCount - startRow;
  }

  // Check that aRowIndex is in the range startRow..startRow+rowSpan-1
  PRInt32 lastRow = startRow + rowSpan - 1;
  if ((aRowIndex < startRow) || (aRowIndex > lastRow)) {
    return 0;  // cell doesn't span any rows starting at aRowIndex
  } else {
    return lastRow - aRowIndex + 1;
  }
#endif
}

PRInt32 nsTableFrame::GetEffectiveRowSpan(nsTableCellFrame *aCell)
{
  PRInt32 startRow;
  aCell->GetRowIndex(startRow);
  return GetEffectiveRowSpan(startRow, aCell);
}


// Return the number of cols spanned by aCell starting at aColIndex
// This is different from the colspan of aCell. If the cell spans no
// dead cells then the colSpan of the cell would be
// GetEffectiveColSpan (indexOfColThatContains_aCell, aCell)
//
// XXX Should be moved to colgroup, as GetEffectiveRowSpan should be moved to rowgroup?
PRInt32 nsTableFrame::GetEffectiveColSpan(PRInt32 aColIndex, const nsTableCellFrame* aCell) const
{
  NS_PRECONDITION (nsnull != aCell, "bad cell arg");
  nsCellMap* cellMap = GetCellMap();
  NS_PRECONDITION (nsnull != cellMap, "bad call, cellMap not yet allocated.");

  return cellMap->GetEffectiveColSpan(aColIndex, aCell);
}

PRInt32 nsTableFrame::GetEffectiveColSpan(const nsTableCellFrame* aCell) const
{
  NS_PRECONDITION (nsnull != aCell, "bad cell arg");
  nsCellMap* cellMap = GetCellMap();
  NS_PRECONDITION (nsnull != cellMap, "bad call, cellMap not yet allocated.");

  PRInt32 initialColIndex;
  aCell->GetColIndex(initialColIndex);
  return cellMap->GetEffectiveColSpan(initialColIndex, aCell);
}

PRInt32 nsTableFrame::GetEffectiveCOLSAttribute()
{
  nsCellMap *cellMap = GetCellMap();
  NS_PRECONDITION (nsnull!=cellMap, "null cellMap.");
  PRInt32 result;
  const nsStyleTable *tableStyle=nsnull;
  GetStyleData(eStyleStruct_Table, (const nsStyleStruct *&)tableStyle);
  result = tableStyle->mCols;
  PRInt32 numCols = GetColCount();
  if (result>numCols)
    result = numCols;
  return result;
}

PRBool nsTableFrame::HasGroupRules() const
{
  const nsStyleTable* tableStyle = nsnull;
  GetStyleData(eStyleStruct_Table, (const nsStyleStruct *&)tableStyle);
  if (NS_STYLE_TABLE_RULES_GROUPS == tableStyle->mRules) { 
    return PR_TRUE;
  }
  return PR_FALSE;
}

// this won't work until bug 12948 is resolved and col groups are considered 
void nsTableFrame::ProcessGroupRules(nsIPresContext& aPresContext)
{
  PRInt32 numCols = GetColCount();

  // process row groups
  nsIFrame* iFrame;
  for (iFrame = mFrames.FirstChild(); iFrame; iFrame->GetNextSibling(&iFrame)) {
    nsIAtom* frameType;
    iFrame->GetFrameType(&frameType);
    if (nsLayoutAtoms::tableRowGroupFrame == frameType) {
      nsTableRowGroupFrame* rgFrame = (nsTableRowGroupFrame *)iFrame;
      PRInt32 startRow = rgFrame->GetStartRowIndex();
      PRInt32 numGroupRows;
      rgFrame->GetRowCount(numGroupRows, PR_FALSE);
      PRInt32 endRow = startRow + numGroupRows - 1;
      if (startRow == endRow) {
        continue;
      }
      for (PRInt32 rowX = startRow; rowX <= endRow; rowX++) {
        for (PRInt32 colX = 0; colX < numCols; colX++) {
          PRBool originates;
          nsTableCellFrame* cell = GetCellInfoAt(rowX, colX, &originates);
          if (originates) {
            nsCOMPtr<nsIStyleContext> styleContext;
            cell->GetStyleContext(getter_AddRefs(styleContext));
            nsStyleSpacing* spacing = (nsStyleSpacing*)styleContext->GetMutableStyleData(eStyleStruct_Spacing);
            if (rowX == startRow) { 
              spacing->SetBorderStyle(NS_SIDE_BOTTOM, NS_STYLE_BORDER_STYLE_NONE);
            }
            else if (rowX == endRow) { 
              spacing->SetBorderStyle(NS_SIDE_TOP, NS_STYLE_BORDER_STYLE_NONE);
            }
            else {
              spacing->SetBorderStyle(NS_SIDE_TOP, NS_STYLE_BORDER_STYLE_NONE);
              spacing->SetBorderStyle(NS_SIDE_BOTTOM, NS_STYLE_BORDER_STYLE_NONE);
            }
            styleContext->RecalcAutomaticData(&aPresContext);
          }
        }
      }
    }
    NS_IF_RELEASE(frameType);
  }
}


NS_IMETHODIMP nsTableFrame::AdjustRowIndices(nsIFrame* aRowGroup, 
                                             PRInt32 aRowIndex,
                                             PRInt32 anAdjustment)
{
  nsresult rv = NS_OK;
  nsIFrame *rowFrame;
  aRowGroup->FirstChild(nsnull, &rowFrame);
  for ( ; nsnull!=rowFrame; rowFrame->GetNextSibling(&rowFrame))
  {
    const nsStyleDisplay *rowDisplay;
    rowFrame->GetStyleData(eStyleStruct_Display, (const nsStyleStruct *&)rowDisplay);
    if (NS_STYLE_DISPLAY_TABLE_ROW==rowDisplay->mDisplay)
    {
      PRInt32 index = ((nsTableRowFrame*)rowFrame)->GetRowIndex();
      if (index >= aRowIndex)
        ((nsTableRowFrame *)rowFrame)->SetRowIndex(index+anAdjustment);
    }
    else if (NS_STYLE_DISPLAY_TABLE_ROW_GROUP==rowDisplay->mDisplay)
    {
      AdjustRowIndices(rowFrame, aRowIndex, anAdjustment);
    }
  }
  return rv;
}

NS_IMETHODIMP nsTableFrame::RemoveRowFromMap(nsTableRowFrame* aRow, PRInt32 aRowIndex)
{
  nsresult rv=NS_OK;

  // Create a new row in the cell map at the specified index.
  mCellMap->RemoveRowFromMap(aRowIndex);

  // Iterate over our row groups and increment the row indices of all rows whose index
  // is >= aRowIndex.
  nsIFrame *rowGroupFrame=mFrames.FirstChild();
  for ( ; nsnull!=rowGroupFrame; rowGroupFrame->GetNextSibling(&rowGroupFrame))
  {
    const nsStyleDisplay *rowGroupDisplay;
    rowGroupFrame->GetStyleData(eStyleStruct_Display, (const nsStyleStruct *&)rowGroupDisplay);
    if (PR_TRUE==IsRowGroup(rowGroupDisplay->mDisplay))
    {
      AdjustRowIndices(rowGroupFrame, aRowIndex, -1);
    }
  }

  return rv;
}

NS_IMETHODIMP nsTableFrame::InsertRowIntoMap(nsTableRowFrame* aRow, PRInt32 aRowIndex)
{
  nsresult rv=NS_OK;

  // Create a new row in the cell map at the specified index.
  mCellMap->InsertRowIntoMap(aRowIndex);

  // Iterate over our row groups and increment the row indices of all rows whose index
  // is >= aRowIndex.
  nsIFrame *rowGroupFrame=mFrames.FirstChild();
  for ( ; nsnull!=rowGroupFrame; rowGroupFrame->GetNextSibling(&rowGroupFrame))
  {
    const nsStyleDisplay *rowGroupDisplay;
    rowGroupFrame->GetStyleData(eStyleStruct_Display, (const nsStyleStruct *&)rowGroupDisplay);
    if (PR_TRUE==IsRowGroup(rowGroupDisplay->mDisplay))
    {
      AdjustRowIndices(rowGroupFrame, aRowIndex, 1);
    }
  }

  // Init the row's index and add its cells to the cell map.
  aRow->InitChildrenWithIndex(aRowIndex);

  return rv;
}

/** sum the columns represented by all nsTableColGroup objects
  * if the cell map says there are more columns than this, 
  * add extra implicit columns to the content tree.
  */
void nsTableFrame::EnsureColumns(nsIPresContext& aPresContext,
                                 PRBool&         aCreatedColFrames)
{
  NS_PRECONDITION(nsnull!=mCellMap, "bad state:  null cellmap");
  
  aCreatedColFrames = PR_FALSE;  // initialize OUT parameter
  if (nsnull == mCellMap)
    return; // no info yet, so nothing useful to do

  // make sure we've accounted for the COLS attribute
  AdjustColumnsForCOLSAttribute();

  // count the number of column frames we already have
  PRInt32 actualColumns = 0;
  nsTableColGroupFrame *lastColGroupFrame = nsnull;
  nsIFrame* childFrame = mColGroups.FirstChild();
  while (nsnull!=childFrame) {
    ((nsTableColGroupFrame*)childFrame)->SetStartColumnIndex(actualColumns);
    PRInt32 numCols = ((nsTableColGroupFrame*)childFrame)->GetColumnCount();
    actualColumns += numCols;
    lastColGroupFrame = (nsTableColGroupFrame *)childFrame;
    childFrame->GetNextSibling(&childFrame);
  }

  // if we have fewer column frames than we need, create some implicit column frames
  PRInt32 colCount = mCellMap->GetColCount();
  if (actualColumns < colCount) {
    nsIContent *lastColGroupElement = nsnull;
    if (nsnull==lastColGroupFrame) { // there are no col groups, so create an implicit colgroup frame 
      // Resolve style for the colgroup frame
      // first, need to get the nearest containing content object
      GetContent(&lastColGroupElement);                                          // ADDREF a: lastColGroupElement++  (either here or in the loop below)
      nsIFrame *parentFrame;
      GetParent(&parentFrame);
      while (nsnull==lastColGroupElement) {
        parentFrame->GetContent(&lastColGroupElement);
        if (nsnull==lastColGroupElement)
          parentFrame->GetParent(&parentFrame);
      }
      // now we have a ref-counted "lastColGroupElement" content object
      nsIStyleContext* colGroupStyleContext;
      aPresContext.ResolvePseudoStyleContextFor (lastColGroupElement, 
                                                 nsHTMLAtoms::tableColGroupPseudo,
                                                 mStyleContext,
                                                 PR_FALSE,
                                                 &colGroupStyleContext);        // colGroupStyleContext: REFCNT++
      // Create a col group frame
      nsIFrame* newFrame;
      NS_NewTableColGroupFrame(&newFrame);
      newFrame->Init(aPresContext, lastColGroupElement, this, colGroupStyleContext,
                     nsnull);
      lastColGroupFrame = (nsTableColGroupFrame*)newFrame;
      NS_RELEASE(colGroupStyleContext);                                         // kidStyleContenxt: REFCNT--

      // hook lastColGroupFrame into child list
      mColGroups.SetFrames(lastColGroupFrame);
    }
    else {
      lastColGroupFrame->GetContent(&lastColGroupElement);  // ADDREF b: lastColGroupElement++
    }

    // XXX It would be better to do this in the style code while constructing
    // the table's frames.  But we don't know how many columns we have at that point.
    nsAutoString colTag;
    nsHTMLAtoms::col->ToString(colTag);
    PRInt32 excessColumns = colCount - actualColumns;
    nsIFrame* firstNewColFrame = nsnull;
    nsIFrame* lastNewColFrame = nsnull;
    nsIStyleContextPtr  lastColGroupStyle;
    lastColGroupFrame->GetStyleContext(lastColGroupStyle.AssignPtr());
    for ( ; excessColumns > 0; excessColumns--) {
      // Create a new col frame
      nsIFrame* colFrame;
      // note we pass in PR_TRUE here to force unique style contexts.
      nsIStyleContext* colStyleContext;
      aPresContext.ResolvePseudoStyleContextFor (lastColGroupElement, 
                                                 nsHTMLAtoms::tableColPseudo,
                                                 lastColGroupStyle,
                                                 PR_TRUE,
                                                 &colStyleContext);             // colStyleContext: REFCNT++
      aCreatedColFrames = PR_TRUE;  // remember that we're creating implicit col frames
      NS_NewTableColFrame(&colFrame);
      colFrame->Init(aPresContext, lastColGroupElement, lastColGroupFrame,
                     colStyleContext, nsnull);
      NS_RELEASE(colStyleContext);
      colFrame->SetInitialChildList(aPresContext, nsnull, nsnull);
      ((nsTableColFrame *)colFrame)->SetIsAnonymous(PR_TRUE);

      // Add it to our list
      if (nsnull == lastNewColFrame) {
        firstNewColFrame = colFrame;
      } else {
        lastNewColFrame->SetNextSibling(colFrame);
      }
      lastNewColFrame = colFrame;
    }
    lastColGroupFrame->SetInitialChildList(aPresContext, nsnull, firstNewColFrame);
    NS_RELEASE(lastColGroupElement);                       // ADDREF: lastColGroupElement--
  }
  else if (actualColumns > colCount) { // the cell map needs to grow to accomodate extra cols
    nsCellMap* cellMap = GetCellMap();
    if (cellMap) {
      cellMap->AddColsAtEnd(actualColumns - colCount);
    }
  }

}


void nsTableFrame::AddColumnFrame (nsTableColFrame *aColFrame)
{
  nsCellMap *cellMap = GetCellMap();
  NS_PRECONDITION (nsnull!=cellMap, "null cellMap.");
  if (nsnull!=cellMap)
  {
    cellMap->AppendColumnFrame(aColFrame);
  }
}

/** return the index of the next row that is not yet assigned */
PRInt32 nsTableFrame::GetNextAvailRowIndex() const
{
  PRInt32 result=0;
  nsCellMap *cellMap = GetCellMap();
  NS_PRECONDITION (nsnull!=cellMap, "null cellMap.");
  if (nsnull!=cellMap) {
    result = cellMap->GetNextAvailRowIndex(); 
  }
  return result;
}

/** Get the cell map for this table frame.  It is not always mCellMap.
  * Only the firstInFlow has a legit cell map
  */
nsCellMap * nsTableFrame::GetCellMap() const
{
  nsTableFrame * firstInFlow = (nsTableFrame *)GetFirstInFlow();
  if (this!=firstInFlow)
  {
    return firstInFlow->GetCellMap();
  }
  return mCellMap;
}

PRInt32 nsTableFrame::AddCellToTable(nsTableCellFrame* aCellFrame,
                                     PRInt32           aRowIndex)
{
  NS_ASSERTION(nsnull != aCellFrame, "bad aCellFrame arg");
  NS_ASSERTION(nsnull != mCellMap,   "bad cellMap");

  // XXX: must be called only on first-in-flow!
  return mCellMap->AppendCell(aCellFrame, aRowIndex);
}

void nsTableFrame::RemoveCellFromTable(nsTableCellFrame* aCellFrame,
                                       PRInt32           aRowIndex)
{
  nsCellMap* cellMap = GetCellMap();
  PRInt32 numCols = cellMap->GetColCount();
  cellMap->RemoveCell(aCellFrame, aRowIndex);
  if (numCols != cellMap->GetColCount()) {
    // XXX need to remove the anonymous col frames at the end
    //nsTableColFrame* colFrame = nsnull;
    //GetColumnFrame(colIndex, colFrame);
    //if (colFrame && colFrame->IsAnonymous()) {
    //  nsTableColGroupFrame* colGroupFrame = nsnull;
    //colFrame->GetParent((nsIFrame **)&colGroupFrame);
    //if (colGroupFrame) {
    //  colGroupFrame->DeleteColFrame(aPresContext, colFrame);
  }
}

static nsresult BuildCellMapForRowGroup(nsIFrame* rowGroupFrame)
{
  nsresult rv = NS_OK;
  nsIFrame *rowFrame;
  rowGroupFrame->FirstChild(nsnull, &rowFrame);
  for ( ; nsnull!=rowFrame; rowFrame->GetNextSibling(&rowFrame))
  {
    const nsStyleDisplay *rowDisplay;
    rowFrame->GetStyleData(eStyleStruct_Display, (const nsStyleStruct *&)rowDisplay);
    if (NS_STYLE_DISPLAY_TABLE_ROW==rowDisplay->mDisplay)
    {
      rv = ((nsTableRowFrame *)rowFrame)->InitChildren();
      if (NS_FAILED(rv))
        return rv;
    }
    else if (NS_STYLE_DISPLAY_TABLE_ROW_GROUP==rowDisplay->mDisplay)
    {
      BuildCellMapForRowGroup(rowFrame);
    }
  }
  return rv;
}

NS_METHOD nsTableFrame::ReBuildCellMap()
{
  nsresult rv=NS_OK;
  nsIFrame *rowGroupFrame=mFrames.FirstChild();
  for ( ; nsnull!=rowGroupFrame; rowGroupFrame->GetNextSibling(&rowGroupFrame))
  {
    const nsStyleDisplay *rowGroupDisplay;
    rowGroupFrame->GetStyleData(eStyleStruct_Display, (const nsStyleStruct *&)rowGroupDisplay);
    if (PR_TRUE==IsRowGroup(rowGroupDisplay->mDisplay))
    {
      BuildCellMapForRowGroup(rowGroupFrame);
    }
  }
  mBits.mCellMapValid=PR_TRUE;
  return rv;
}

/* ***** Column Layout Data methods ***** */

/*
 * Lists the column layout data which turns
 * around and lists the cell layout data.
 * This is for debugging purposes only.
 */
#ifdef NS_DEBUG
void nsTableFrame::ListColumnLayoutData(FILE* out, PRInt32 aIndent) 
{
  nsTableFrame * firstInFlow = (nsTableFrame *)GetFirstInFlow();
  if (this!=firstInFlow)
  {
    firstInFlow->ListColumnLayoutData(out, aIndent);
    return;
  }

  nsCellMap *cellMap = GetCellMap();
  if (nsnull!=cellMap)
  {
    fprintf(out,"Column Layout Data \n");
    
    PRInt32 numCols = cellMap->GetColCount();
    PRInt32 numRows = cellMap->GetRowCount();
    for (PRInt32 colIndex = 0; colIndex<numCols; colIndex++)
    {
      for (PRInt32 indent = aIndent; --indent >= 0; ) 
        fputs("  ", out);
      fprintf(out,"Column Data [%d] \n",colIndex);
      for (PRInt32 rowIndex = 0; rowIndex < numRows; rowIndex++)
      {
        nsTableCellFrame* cellFrame = cellMap->GetCellInfoAt(rowIndex, colIndex);
        PRInt32 rowIndent;
        for (rowIndent = aIndent+2; --rowIndent >= 0; ) fputs("  ", out);
        fprintf(out,"Cell Data [%d] \n",rowIndex);
        for (rowIndent = aIndent+2; --rowIndent >= 0; ) fputs("  ", out);
        nsMargin margin;
        cellFrame->GetMargin(margin);
        fprintf(out,"Margin -- Top: %d Left: %d Bottom: %d Right: %d \n",  
                NSTwipsToIntPoints(margin.top),
                NSTwipsToIntPoints(margin.left),
                NSTwipsToIntPoints(margin.bottom),
                NSTwipsToIntPoints(margin.right));
      
        for (rowIndent = aIndent+2; --rowIndent >= 0; ) fputs("  ", out);
      }
    }
  }
}
#endif

void nsTableFrame::SetBorderEdgeLength(PRUint8 aSide, PRInt32 aIndex, nscoord aLength)
{
  if (!mBorderEdges) {
    mBorderEdges = new nsBorderEdges;
  }
  nsBorderEdge *border = (nsBorderEdge *)(mBorderEdges->mEdges[aSide].ElementAt(aIndex));
  if (border)
  {
    border->mLength = aLength;
  }
}

void nsTableFrame::DidComputeHorizontalCollapsingBorders(nsIPresContext& aPresContext,
                                                         PRInt32 aStartRowIndex,
                                                         PRInt32 aEndRowIndex)
{
  NS_PRECONDITION(mBorderEdges, "haven't allocated border edges struct");
  // XXX: for now, this only does table edges.  May need to do interior edges also?  Probably not.
  nsCellMap *cellMap = GetCellMap();
  PRInt32 lastRowIndex = cellMap->GetRowCount()-1;
  PRInt32 lastColIndex = cellMap->GetColCount()-1;
  if (0==aStartRowIndex)
  {
    nsTableCellFrame* cellFrame = mCellMap->GetCellInfoAt(0, 0);
    nsRect rowRect(0,0,0,0);
    if (nsnull!=cellFrame)
    {
      nsIFrame *rowFrame;
      cellFrame->GetParent(&rowFrame);
      rowFrame->GetRect(rowRect);
      nsBorderEdge *leftBorder = (nsBorderEdge *)(mBorderEdges->mEdges[NS_SIDE_LEFT].ElementAt(0));
      nsBorderEdge *rightBorder = (nsBorderEdge *)(mBorderEdges->mEdges[NS_SIDE_RIGHT].ElementAt(0));
      nsBorderEdge *topBorder = (nsBorderEdge *)(mBorderEdges->mEdges[NS_SIDE_TOP].ElementAt(0));
      if (leftBorder)
        leftBorder->mLength = rowRect.height + topBorder->mWidth;
      if (topBorder)
        topBorder = (nsBorderEdge *)(mBorderEdges->mEdges[NS_SIDE_TOP].ElementAt(lastColIndex));
      if (rightBorder)
        rightBorder->mLength = rowRect.height + topBorder->mWidth;
    }
  }

  if (lastRowIndex<=aEndRowIndex)
  {
    nsTableCellFrame* cellFrame = mCellMap->GetCellInfoAt(lastRowIndex, 0);
    nsRect rowRect(0,0,0,0);
    if (nsnull!=cellFrame)
    {
      nsIFrame *rowFrame;
      cellFrame->GetParent(&rowFrame);
      rowFrame->GetRect(rowRect);
      nsBorderEdge *leftBorder = (nsBorderEdge *)(mBorderEdges->mEdges[NS_SIDE_LEFT].ElementAt(lastRowIndex));
      nsBorderEdge *rightBorder = (nsBorderEdge *)(mBorderEdges->mEdges[NS_SIDE_RIGHT].ElementAt(lastRowIndex));
      nsBorderEdge *bottomBorder = (nsBorderEdge *)(mBorderEdges->mEdges[NS_SIDE_BOTTOM].ElementAt(0));
      if (leftBorder)
        leftBorder->mLength = rowRect.height + bottomBorder->mWidth;
      if (bottomBorder)
        bottomBorder = (nsBorderEdge *)(mBorderEdges->mEdges[NS_SIDE_BOTTOM].ElementAt(lastColIndex));
      if (rightBorder)
        rightBorder->mLength = rowRect.height + bottomBorder->mWidth;
    }
  }
  //XXX this won't work if the constants are redefined, too bad
  for (PRInt32 borderX = NS_SIDE_TOP; borderX <= NS_SIDE_LEFT; borderX++) {
    if (!mBorderEdges->mEdges[borderX].ElementAt(0)) {
      mBorderEdges->mEdges[borderX].AppendElement(new nsBorderEdge());
    }
  }
}


  // For every row between aStartRowIndex and aEndRowIndex (-1 == the end of the table),
  // walk across every edge and compute the border at that edge.
  // We compute each edge only once, arbitrarily choosing to compute right and bottom edges, 
  // except for exterior cells that share a left or top edge with the table itself.
  // Distribute half the computed border to the appropriate adjacent objects
  // (always a cell frame or the table frame.)  In the case of odd width, 
  // the object on the right/bottom gets the extra portion

/* compute the top and bottom collapsed borders between aStartRowIndex and aEndRowIndex, inclusive */
void nsTableFrame::ComputeHorizontalCollapsingBorders(nsIPresContext& aPresContext,
                                                      PRInt32 aStartRowIndex,
                                                      PRInt32 aEndRowIndex)
{
  // this method just uses mCellMap, because it can't get called unless nCellMap!=nsnull
  PRInt32 colCount = mCellMap->GetColCount();
  PRInt32 rowCount = mCellMap->GetRowCount();
  if (aStartRowIndex>=rowCount)
  {
    return; // we don't have the requested row yet
  }

  PRInt32 rowIndex = aStartRowIndex;
  for ( ; rowIndex<rowCount && rowIndex <=aEndRowIndex; rowIndex++)
  {
    PRInt32 colIndex=0;
    for ( ; colIndex<colCount; colIndex++)
    {
      if (0==rowIndex)
      { // table is top neighbor
        ComputeTopBorderForEdgeAt(aPresContext, rowIndex, colIndex);
      }
      ComputeBottomBorderForEdgeAt(aPresContext, rowIndex, colIndex);
    }
  }
}

/* compute the left and right collapsed borders between aStartRowIndex and aEndRowIndex, inclusive */
void nsTableFrame::ComputeVerticalCollapsingBorders(nsIPresContext& aPresContext,
                                                    PRInt32 aStartRowIndex,
                                                    PRInt32 aEndRowIndex)
{
  nsCellMap *cellMap = GetCellMap();
  if (nsnull==cellMap)
    return; // no info yet, so nothing useful to do
  
  CacheColFramesInCellMap();

  // compute all the collapsing border values for the entire table
  // XXX: we have to make this more incremental!
  if (NS_STYLE_BORDER_COLLAPSE != GetBorderCollapseStyle())
    return;
  
  if (!mBorderEdges) {
    mBorderEdges = new nsBorderEdges;
  }
  
  PRInt32 colCount = mCellMap->GetColCount();
  PRInt32 rowCount = mCellMap->GetRowCount();
  PRInt32 endRowIndex = aEndRowIndex;
  if (-1==endRowIndex)
    endRowIndex=rowCount-1;
  if (aStartRowIndex>=rowCount)
  {
    return; // we don't have the requested row yet
  }

  PRInt32 rowIndex = aStartRowIndex;
  for ( ; rowIndex<rowCount && rowIndex <=endRowIndex; rowIndex++)
  {
    PRInt32 colIndex=0;
    for ( ; colIndex<colCount; colIndex++)
    {
      if (0==colIndex)
      { // table is left neighbor
        ComputeLeftBorderForEdgeAt(aPresContext, rowIndex, colIndex);
      }
      ComputeRightBorderForEdgeAt(aPresContext, rowIndex, colIndex);
    }
  }
}

void nsTableFrame::ComputeLeftBorderForEdgeAt(nsIPresContext& aPresContext,
                                              PRInt32 aRowIndex, 
                                              PRInt32 aColIndex)
{
  NS_PRECONDITION(mBorderEdges, "haven't allocated border edges struct");
  // this method just uses mCellMap, because it can't get called unless nCellMap!=nsnull
  PRInt32 numSegments = mBorderEdges->mEdges[NS_SIDE_LEFT].Count();
  while (numSegments<=aRowIndex)
  {
    nsBorderEdge *borderToAdd = new nsBorderEdge();
    mBorderEdges->mEdges[NS_SIDE_LEFT].AppendElement(borderToAdd);
    numSegments++;
  }
  // "border" is the border segment we are going to set
  nsBorderEdge *border = (nsBorderEdge *)(mBorderEdges->mEdges[NS_SIDE_LEFT].ElementAt(aRowIndex));
  if (!border) 
    return;

  // collect all the incident frames and compute the dominant border 
  nsVoidArray styles;
  // styles are added to the array in the order least dominant -> most dominant
  //    1. table
  const nsStyleSpacing *spacing;
  GetStyleData(eStyleStruct_Spacing, ((const nsStyleStruct *&)spacing));
  styles.AppendElement((void*)spacing);
  //    2. colgroup
  nsTableColFrame *colFrame = mCellMap->GetColumnFrame(aColIndex);
  nsIFrame *colGroupFrame;
  colFrame->GetParent(&colGroupFrame);
  colGroupFrame->GetStyleData(eStyleStruct_Spacing, ((const nsStyleStruct *&)spacing));
  styles.AppendElement((void*)spacing);
  //    3. col
  colFrame->GetStyleData(eStyleStruct_Spacing, ((const nsStyleStruct *&)spacing));
  styles.AppendElement((void*)spacing);
  //    4. rowgroup
  nsTableCellFrame* cellFrame = mCellMap->GetCellInfoAt(aRowIndex, aColIndex);
  nsRect rowRect(0,0,0,0);
  if (nsnull!=cellFrame)
  {
    nsIFrame *rowFrame;
    cellFrame->GetParent(&rowFrame);
    rowFrame->GetRect(rowRect);
    nsIFrame *rowGroupFrame;
    rowFrame->GetParent(&rowGroupFrame);
    rowGroupFrame->GetStyleData(eStyleStruct_Spacing, ((const nsStyleStruct *&)spacing));
    styles.AppendElement((void*)spacing);
    //    5. row
    rowFrame->GetStyleData(eStyleStruct_Spacing, ((const nsStyleStruct *&)spacing));
    styles.AppendElement((void*)spacing);
    //    6. cell (need to do something smart for rowspanner with row frame)
    cellFrame->GetStyleData(eStyleStruct_Spacing, ((const nsStyleStruct *&)spacing));
    styles.AppendElement((void*)spacing);
  }
  ComputeCollapsedBorderSegment(NS_SIDE_LEFT, &styles, *border, PR_FALSE);
  // now give half the computed border to the table segment, and half to the cell
  // to avoid rounding errors, we convert up to pixels, divide by 2, and 
  // we give the odd pixel to the table border
  float t2p;
  aPresContext.GetTwipsToPixels(&t2p);
  float p2t;
  aPresContext.GetPixelsToTwips(&p2t);
  nscoord widthAsPixels = NSToCoordRound((float)(border->mWidth)*t2p);
  nscoord widthToAdd = 0;
  border->mWidth = widthAsPixels/2;
  if ((border->mWidth*2)!=widthAsPixels)
    widthToAdd = NSToCoordCeil(p2t);
  border->mWidth *= NSToCoordCeil(p2t);
  border->mLength = rowRect.height;
  border->mInsideNeighbor = &cellFrame->mBorderEdges;
  // we need to factor in the table's horizontal borders.
  // but we can't compute that length here because we don't know how thick top and bottom borders are
  // see DidComputeHorizontalCollapsingBorders
  if (nsnull!=cellFrame)
  {
    cellFrame->SetBorderEdge(NS_SIDE_LEFT, aRowIndex, aColIndex, border, 0);  // set the left edge of the cell frame
  }
  border->mWidth += widthToAdd;
  mBorderEdges->mMaxBorderWidth.left = PR_MAX(border->mWidth, mBorderEdges->mMaxBorderWidth.left);
}

void nsTableFrame::ComputeRightBorderForEdgeAt(nsIPresContext& aPresContext,
                                               PRInt32 aRowIndex, 
                                               PRInt32 aColIndex)
{
  NS_PRECONDITION(mBorderEdges, "haven't allocated border edges struct");
  // this method just uses mCellMap, because it can't get called unless nCellMap!=nsnull
  PRInt32 colCount = mCellMap->GetColCount();
  PRInt32 numSegments = mBorderEdges->mEdges[NS_SIDE_RIGHT].Count();
  while (numSegments<=aRowIndex)
  {
    nsBorderEdge *borderToAdd = new nsBorderEdge();
    mBorderEdges->mEdges[NS_SIDE_RIGHT].AppendElement(borderToAdd);
    numSegments++;
  }
  // "border" is the border segment we are going to set
  nsBorderEdge border;

  // collect all the incident frames and compute the dominant border 
  nsVoidArray styles;
  // styles are added to the array in the order least dominant -> most dominant
  //    1. table, only if this cell is in the right-most column and no rowspanning cell is
  //       to it's right.  Otherwise, we remember what cell is the right neighbor
  nsTableCellFrame *rightNeighborFrame=nsnull;  
  if ((colCount-1)!=aColIndex)
  {
    PRInt32 colIndex = aColIndex+1;
    for ( ; colIndex<colCount; colIndex++)
    {
		  CellData *cd = mCellMap->GetCellAt(aRowIndex, colIndex);
		  if (cd != nsnull)
		  { // there's really a cell at (aRowIndex, colIndex)
			  if (nsnull==cd->mOrigCell)
			  { // the cell at (aRowIndex, colIndex) is the result of a span
				  nsTableCellFrame* cell = nsnull;
          if (cd->mRowSpanData)
            cell = cd->mRowSpanData->mOrigCell;
          else if (cd->mColSpanData)
            cell = cd->mColSpanData->mOrigCell;
				  NS_ASSERTION(nsnull!=cell, "bad cell map state, missing real cell");
				  PRInt32 realRowIndex;
          cell->GetRowIndex (realRowIndex);
				  if (realRowIndex!=aRowIndex)
				  { // the span is caused by a rowspan
					  rightNeighborFrame = cd->mRowSpanData->mOrigCell;
					  break;
				  }
			  }
        else
        {
          rightNeighborFrame = cd->mOrigCell;
          break;
        }
		  }
    }
  }
  const nsStyleSpacing *spacing;
  if (nsnull==rightNeighborFrame)
  { // if rightNeighborFrame is null, our right neighbor is the table 
    GetStyleData(eStyleStruct_Spacing, ((const nsStyleStruct *&)spacing));
    styles.AppendElement((void*)spacing);
  }
  //    2. colgroup //XXX: need to test if we're really on a colgroup border
  nsTableColFrame *colFrame = mCellMap->GetColumnFrame(aColIndex);
  nsIFrame *colGroupFrame;
  colFrame->GetParent(&colGroupFrame);
  colGroupFrame->GetStyleData(eStyleStruct_Spacing, ((const nsStyleStruct *&)spacing));
  styles.AppendElement((void*)spacing);
  //    3. col
  colFrame->GetStyleData(eStyleStruct_Spacing, ((const nsStyleStruct *&)spacing));
  styles.AppendElement((void*)spacing);
  //    4. rowgroup
  nsTableCellFrame* cellFrame = mCellMap->GetCellInfoAt(aRowIndex, aColIndex);
  nsRect rowRect(0,0,0,0);
  if (nsnull!=cellFrame)
  {
    nsIFrame *rowFrame;
    cellFrame->GetParent(&rowFrame);
    rowFrame->GetRect(rowRect);
    nsIFrame *rowGroupFrame;
    rowFrame->GetParent(&rowGroupFrame);
    if (nsnull==rightNeighborFrame)
    { // if rightNeighborFrame is null, our right neighbor is the table so we include the rowgroup and row
      rowGroupFrame->GetStyleData(eStyleStruct_Spacing, ((const nsStyleStruct *&)spacing));
      styles.AppendElement((void*)spacing);
      //    5. row
      rowFrame->GetStyleData(eStyleStruct_Spacing, ((const nsStyleStruct *&)spacing));
      styles.AppendElement((void*)spacing);
    }
    //    6. cell (need to do something smart for rowspanner with row frame)
    cellFrame->GetStyleData(eStyleStruct_Spacing, ((const nsStyleStruct *&)spacing));
    styles.AppendElement((void*)spacing);
  }
  //    7. left edge of rightNeighborCell, if there is one
  if (nsnull!=rightNeighborFrame)
  {
    rightNeighborFrame->GetStyleData(eStyleStruct_Spacing, ((const nsStyleStruct *&)spacing));
    styles.AppendElement((void*)spacing);
  }
  ComputeCollapsedBorderSegment(NS_SIDE_RIGHT, &styles, border, PRBool(nsnull!=rightNeighborFrame));
  // now give half the computed border to each of the two neighbors 
  // (the 2 cells, or the cell and the table)
  // to avoid rounding errors, we convert up to pixels, divide by 2, and 
  // we give the odd pixel to the right cell border
  float t2p;
  aPresContext.GetTwipsToPixels(&t2p);
  float p2t;
  aPresContext.GetPixelsToTwips(&p2t);
  nscoord widthAsPixels = NSToCoordRound((float)(border.mWidth)*t2p);
  nscoord widthToAdd = 0;
  border.mWidth = widthAsPixels/2;
  if ((border.mWidth*2)!=widthAsPixels)
    widthToAdd = NSToCoordCeil(p2t);
  border.mWidth *= NSToCoordCeil(p2t);
  border.mLength = rowRect.height;
  if (nsnull!=cellFrame)
  {
    cellFrame->SetBorderEdge(NS_SIDE_RIGHT, aRowIndex, aColIndex, &border, widthToAdd);
  }
  if (nsnull==rightNeighborFrame)
  {
    nsBorderEdge * tableBorder = (nsBorderEdge *)(mBorderEdges->mEdges[NS_SIDE_RIGHT].ElementAt(aRowIndex));
    *tableBorder = border;
    tableBorder->mInsideNeighbor = &cellFrame->mBorderEdges;
    mBorderEdges->mMaxBorderWidth.right = PR_MAX(border.mWidth, mBorderEdges->mMaxBorderWidth.right);
    // since the table is our right neightbor, we need to factor in the table's horizontal borders.
    // can't compute that length here because we don't know how thick top and bottom borders are
    // see DidComputeHorizontalCollapsingBorders
  }
  else
  {
    rightNeighborFrame->SetBorderEdge(NS_SIDE_LEFT, aRowIndex, aColIndex, &border, 0);
  }
}

void nsTableFrame::ComputeTopBorderForEdgeAt(nsIPresContext& aPresContext,
                                             PRInt32 aRowIndex, 
                                             PRInt32 aColIndex)
{
  NS_PRECONDITION(mBorderEdges, "haven't allocated border edges struct");
  // this method just uses mCellMap, because it can't get called unless nCellMap!=nsnull
  PRInt32 numSegments = mBorderEdges->mEdges[NS_SIDE_TOP].Count();
  while (numSegments<=aColIndex)
  {
    nsBorderEdge *borderToAdd = new nsBorderEdge();
    mBorderEdges->mEdges[NS_SIDE_TOP].AppendElement(borderToAdd);
    numSegments++;
  }
  // "border" is the border segment we are going to set
  nsBorderEdge *border = (nsBorderEdge *)(mBorderEdges->mEdges[NS_SIDE_TOP].ElementAt(aColIndex));
  if (!border) 
    return;

  // collect all the incident frames and compute the dominant border 
  nsVoidArray styles;
  // styles are added to the array in the order least dominant -> most dominant
  //    1. table
  const nsStyleSpacing *spacing;
  GetStyleData(eStyleStruct_Spacing, ((const nsStyleStruct *&)spacing));
  styles.AppendElement((void*)spacing);
  //    2. colgroup
  nsTableColFrame *colFrame = mCellMap->GetColumnFrame(aColIndex);
  nsIFrame *colGroupFrame;
  colFrame->GetParent(&colGroupFrame);
  colGroupFrame->GetStyleData(eStyleStruct_Spacing, ((const nsStyleStruct *&)spacing));
  styles.AppendElement((void*)spacing);
  //    3. col
  colFrame->GetStyleData(eStyleStruct_Spacing, ((const nsStyleStruct *&)spacing));
  styles.AppendElement((void*)spacing);
  //    4. rowgroup
  nsTableCellFrame* cellFrame = mCellMap->GetCellInfoAt(aRowIndex, aColIndex);
  if (nsnull!=cellFrame)
  {
    nsIFrame *rowFrame;
    cellFrame->GetParent(&rowFrame);
    nsIFrame *rowGroupFrame;
    rowFrame->GetParent(&rowGroupFrame);
    rowGroupFrame->GetStyleData(eStyleStruct_Spacing, ((const nsStyleStruct *&)spacing));
    styles.AppendElement((void*)spacing);
    //    5. row
    rowFrame->GetStyleData(eStyleStruct_Spacing, ((const nsStyleStruct *&)spacing));
    styles.AppendElement((void*)spacing);
    //    6. cell (need to do something smart for rowspanner with row frame)
    cellFrame->GetStyleData(eStyleStruct_Spacing, ((const nsStyleStruct *&)spacing));
    styles.AppendElement((void*)spacing);
  }
  ComputeCollapsedBorderSegment(NS_SIDE_TOP, &styles, *border, PR_FALSE);
  // now give half the computed border to the table segment, and half to the cell
  // to avoid rounding errors, we convert up to pixels, divide by 2, and 
  // we give the odd pixel to the right border
  float t2p;
  aPresContext.GetTwipsToPixels(&t2p);
  float p2t;
  aPresContext.GetPixelsToTwips(&p2t);
  nscoord widthAsPixels = NSToCoordRound((float)(border->mWidth)*t2p);
  nscoord widthToAdd = 0;
  border->mWidth = widthAsPixels/2;
  if ((border->mWidth*2)!=widthAsPixels)
    widthToAdd = NSToCoordCeil(p2t);
  border->mWidth *= NSToCoordCeil(p2t);
  border->mLength = GetColumnWidth(aColIndex);
  border->mInsideNeighbor = &cellFrame->mBorderEdges;
  if (0==aColIndex)
  { // if we're the first column, factor in the thickness of the left table border
    nsBorderEdge *leftBorder = (nsBorderEdge *)(mBorderEdges->mEdges[NS_SIDE_LEFT].ElementAt(0));
    if (leftBorder) 
      border->mLength += leftBorder->mWidth;
  }
  if ((mCellMap->GetColCount()-1)==aColIndex)
  { // if we're the last column, factor in the thickness of the right table border
    nsBorderEdge *rightBorder = (nsBorderEdge *)(mBorderEdges->mEdges[NS_SIDE_RIGHT].ElementAt(0));
    if (rightBorder) 
      border->mLength += rightBorder->mWidth;
  }
  if (nsnull!=cellFrame)
  {
    cellFrame->SetBorderEdge(NS_SIDE_TOP, aRowIndex, aColIndex, border, 0);  // set the top edge of the cell frame
  }
  border->mWidth += widthToAdd;
  mBorderEdges->mMaxBorderWidth.top = PR_MAX(border->mWidth, mBorderEdges->mMaxBorderWidth.top);
}

void nsTableFrame::ComputeBottomBorderForEdgeAt(nsIPresContext& aPresContext,
                                                PRInt32 aRowIndex, 
                                                PRInt32 aColIndex)
{
  NS_PRECONDITION(mBorderEdges, "haven't allocated border edges struct");
  // this method just uses mCellMap, because it can't get called unless nCellMap!=nsnull
  PRInt32 rowCount = mCellMap->GetRowCount();
  PRInt32 numSegments = mBorderEdges->mEdges[NS_SIDE_BOTTOM].Count();
  while (numSegments<=aColIndex)
  {
    nsBorderEdge *borderToAdd = new nsBorderEdge();
    mBorderEdges->mEdges[NS_SIDE_BOTTOM].AppendElement(borderToAdd);
    numSegments++;
  }
  // "border" is the border segment we are going to set
  nsBorderEdge border;

  // collect all the incident frames and compute the dominant border 
  nsVoidArray styles;
  // styles are added to the array in the order least dominant -> most dominant
  //    1. table, only if this cell is in the bottom-most row and no colspanning cell is
  //       beneath it.  Otherwise, we remember what cell is the bottom neighbor
  nsTableCellFrame *bottomNeighborFrame=nsnull;  
  if ((rowCount-1)!=aRowIndex)
  {
    PRInt32 rowIndex = aRowIndex+1;
    for ( ; rowIndex<rowCount; rowIndex++)
    {
		  CellData *cd = mCellMap->GetCellAt(rowIndex, aColIndex);
		  if (cd != nsnull)
		  { // there's really a cell at (rowIndex, aColIndex)
			  if (nsnull==cd->mOrigCell)
			  { // the cell at (rowIndex, aColIndex) is the result of a span
				  nsTableCellFrame* cell = nsnull;
          if (cd->mRowSpanData)  // XXX should we check for a row span
            cell = cd->mRowSpanData->mOrigCell;
          else if (cd->mColSpanData)
            cell = cd->mColSpanData->mOrigCell;
				  NS_ASSERTION(nsnull!=cell, "bad cell map state, missing real cell");
				  PRInt32 realColIndex;
          cell->GetColIndex (realColIndex);
				  if (realColIndex!=aColIndex)
				  { // the span is caused by a colspan
					  bottomNeighborFrame = cd->mColSpanData->mOrigCell;
					  break;
				  }
			  }
        else
        {
          bottomNeighborFrame = cd->mOrigCell;
          break;
        }
		  }
    }
  }
  const nsStyleSpacing *spacing;
  if (nsnull==bottomNeighborFrame)
  { // if bottomNeighborFrame is null, our bottom neighbor is the table 
    GetStyleData(eStyleStruct_Spacing, ((const nsStyleStruct *&)spacing));
    styles.AppendElement((void*)spacing);

    //    2. colgroup   // XXX: need to deterine if we're on a colgroup boundary
    nsTableColFrame *colFrame = mCellMap->GetColumnFrame(aColIndex);
    nsIFrame *colGroupFrame;
    colFrame->GetParent(&colGroupFrame);
    colGroupFrame->GetStyleData(eStyleStruct_Spacing, ((const nsStyleStruct *&)spacing));
    styles.AppendElement((void*)spacing);
    //    3. col
    colFrame->GetStyleData(eStyleStruct_Spacing, ((const nsStyleStruct *&)spacing));
    styles.AppendElement((void*)spacing);
  }
  //    4. rowgroup // XXX: use rowgroup only if we're on a table edge
  nsTableCellFrame* cellFrame = mCellMap->GetCellInfoAt(aRowIndex, aColIndex);
  nsRect rowRect(0,0,0,0);
  if (nsnull!=cellFrame)
  {
    nsIFrame *rowFrame;
    cellFrame->GetParent(&rowFrame);
    rowFrame->GetRect(rowRect);
    nsIFrame *rowGroupFrame;
    rowFrame->GetParent(&rowGroupFrame);
    rowGroupFrame->GetStyleData(eStyleStruct_Spacing, ((const nsStyleStruct *&)spacing));
    styles.AppendElement((void*)spacing);
    //    5. row
    rowFrame->GetStyleData(eStyleStruct_Spacing, ((const nsStyleStruct *&)spacing));
    styles.AppendElement((void*)spacing);
    //    6. cell (need to do something smart for rowspanner with row frame)
    cellFrame->GetStyleData(eStyleStruct_Spacing, ((const nsStyleStruct *&)spacing));
    styles.AppendElement((void*)spacing);
  }
  //    7. top edge of bottomNeighborCell, if there is one
  if (nsnull!=bottomNeighborFrame)
  {
    bottomNeighborFrame->GetStyleData(eStyleStruct_Spacing, ((const nsStyleStruct *&)spacing));
    styles.AppendElement((void*)spacing);
  }
  ComputeCollapsedBorderSegment(NS_SIDE_BOTTOM, &styles, border, PRBool(nsnull!=bottomNeighborFrame));
  // now give half the computed border to each of the two neighbors 
  // (the 2 cells, or the cell and the table)
  // to avoid rounding errors, we convert up to pixels, divide by 2, and 
  // we give the odd pixel to the right cell border
  float t2p;
  aPresContext.GetTwipsToPixels(&t2p);
  float p2t;
  aPresContext.GetPixelsToTwips(&p2t);
  nscoord widthAsPixels = NSToCoordRound((float)(border.mWidth)*t2p);
  nscoord widthToAdd = 0;
  border.mWidth = widthAsPixels/2;
  if ((border.mWidth*2)!=widthAsPixels)
    widthToAdd = NSToCoordCeil(p2t);
  border.mWidth *= NSToCoordCeil(p2t);
  border.mLength = GetColumnWidth(aColIndex);
  if (nsnull!=cellFrame)
  {
    cellFrame->SetBorderEdge(NS_SIDE_BOTTOM, aRowIndex, aColIndex, &border, widthToAdd);
  }
  if (nsnull==bottomNeighborFrame)
  {
    nsBorderEdge * tableBorder = (nsBorderEdge *)(mBorderEdges->mEdges[NS_SIDE_BOTTOM].ElementAt(aColIndex));
    *tableBorder = border;
    tableBorder->mInsideNeighbor = &cellFrame->mBorderEdges;
    mBorderEdges->mMaxBorderWidth.bottom = PR_MAX(border.mWidth, mBorderEdges->mMaxBorderWidth.bottom);
    // since the table is our bottom neightbor, we need to factor in the table's vertical borders.
    PRInt32 lastColIndex = mCellMap->GetColCount()-1;
    if (0==aColIndex)
    { // if we're the first column, factor in the thickness of the left table border
      nsBorderEdge *leftBorder = (nsBorderEdge *)(mBorderEdges->mEdges[NS_SIDE_LEFT].ElementAt(rowCount-1));
      if (leftBorder) 
        tableBorder->mLength += leftBorder->mWidth;
    }
    if (lastColIndex==aColIndex)
    { // if we're the last column, factor in the thickness of the right table border
      nsBorderEdge *rightBorder = (nsBorderEdge *)(mBorderEdges->mEdges[NS_SIDE_RIGHT].ElementAt(rowCount-1));
      if (rightBorder) 
        tableBorder->mLength += rightBorder->mWidth;
    }
  }
  else
  {
    bottomNeighborFrame->SetBorderEdge(NS_SIDE_TOP, aRowIndex, aColIndex, &border, 0);
  }
}

nscoord nsTableFrame::GetWidthForSide(const nsMargin &aBorder, PRUint8 aSide)
{
  if (NS_SIDE_LEFT == aSide) return aBorder.left;
  else if (NS_SIDE_RIGHT == aSide) return aBorder.right;
  else if (NS_SIDE_TOP == aSide) return aBorder.top;
  else return aBorder.bottom;
}

/* Given an Edge, find the opposing edge (top<-->bottom, left<-->right) */
PRUint8 nsTableFrame::GetOpposingEdge(PRUint8 aEdge)
{
   PRUint8 result;
   switch (aEdge)
   {
   case NS_SIDE_LEFT:
        result = NS_SIDE_RIGHT;  break;
   case NS_SIDE_RIGHT:
        result = NS_SIDE_LEFT;   break;
   case NS_SIDE_TOP:
        result = NS_SIDE_BOTTOM; break;
   case NS_SIDE_BOTTOM:
        result = NS_SIDE_TOP;    break;
   default:
        result = NS_SIDE_TOP;
   }
  return result;
}

/* returns BORDER_PRECEDENT_LOWER if aStyle1 is lower precedent that aStyle2
 *         BORDER_PRECEDENT_HIGHER if aStyle1 is higher precedent that aStyle2
 *         BORDER_PRECEDENT_EQUAL if aStyle1 and aStyle2 have the same precedence
 *         (note, this is not necessarily the same as saying aStyle1==aStyle2)
 * this is a method on nsTableFrame because other objects might define their
 * own border precedence rules.
 */
PRUint8 nsTableFrame::CompareBorderStyles(PRUint8 aStyle1, PRUint8 aStyle2)
{
  PRUint8 result=BORDER_PRECEDENT_HIGHER; // if we get illegal types for table borders, HIGHER is the default
  if (aStyle1==aStyle2)
    result = BORDER_PRECEDENT_EQUAL;
  else if (NS_STYLE_BORDER_STYLE_HIDDEN==aStyle1)
    result = BORDER_PRECEDENT_HIGHER;
  else if (NS_STYLE_BORDER_STYLE_NONE==aStyle1)
    result = BORDER_PRECEDENT_LOWER;
  else if (NS_STYLE_BORDER_STYLE_NONE==aStyle2)
    result = BORDER_PRECEDENT_HIGHER;
  else if (NS_STYLE_BORDER_STYLE_HIDDEN==aStyle2)
    result = BORDER_PRECEDENT_LOWER;
  else
  {
    switch (aStyle1)
    {
    case NS_STYLE_BORDER_STYLE_BG_INSET:
      result = BORDER_PRECEDENT_LOWER;
      break;

    case NS_STYLE_BORDER_STYLE_GROOVE:
      if (NS_STYLE_BORDER_STYLE_BG_INSET==aStyle2)
        result = BORDER_PRECEDENT_HIGHER;
      else
        result = BORDER_PRECEDENT_LOWER;
      break;      

    case NS_STYLE_BORDER_STYLE_BG_OUTSET:
      if (NS_STYLE_BORDER_STYLE_BG_INSET==aStyle2 || 
          NS_STYLE_BORDER_STYLE_GROOVE==aStyle2)
        result = BORDER_PRECEDENT_HIGHER;
      else
        result = BORDER_PRECEDENT_LOWER;
      break;      

    case NS_STYLE_BORDER_STYLE_RIDGE:
      if (NS_STYLE_BORDER_STYLE_BG_INSET==aStyle2  || 
          NS_STYLE_BORDER_STYLE_GROOVE==aStyle2 ||
          NS_STYLE_BORDER_STYLE_BG_OUTSET==aStyle2)
        result = BORDER_PRECEDENT_HIGHER;
      else
        result = BORDER_PRECEDENT_LOWER;
      break;

    case NS_STYLE_BORDER_STYLE_DOTTED:
      if (NS_STYLE_BORDER_STYLE_BG_INSET==aStyle2  || 
          NS_STYLE_BORDER_STYLE_GROOVE==aStyle2 ||
          NS_STYLE_BORDER_STYLE_BG_OUTSET==aStyle2 ||
          NS_STYLE_BORDER_STYLE_RIDGE==aStyle2)
        result = BORDER_PRECEDENT_HIGHER;
      else
        result = BORDER_PRECEDENT_LOWER;
      break;

    case NS_STYLE_BORDER_STYLE_DASHED:
      if (NS_STYLE_BORDER_STYLE_BG_INSET==aStyle2  || 
          NS_STYLE_BORDER_STYLE_GROOVE==aStyle2 ||
          NS_STYLE_BORDER_STYLE_BG_OUTSET==aStyle2 ||
          NS_STYLE_BORDER_STYLE_RIDGE==aStyle2  ||
          NS_STYLE_BORDER_STYLE_DOTTED==aStyle2)
        result = BORDER_PRECEDENT_HIGHER;
      else
        result = BORDER_PRECEDENT_LOWER;
      break;

    case NS_STYLE_BORDER_STYLE_SOLID:
      if (NS_STYLE_BORDER_STYLE_BG_INSET==aStyle2  || 
          NS_STYLE_BORDER_STYLE_GROOVE==aStyle2 ||
          NS_STYLE_BORDER_STYLE_BG_OUTSET==aStyle2 ||
          NS_STYLE_BORDER_STYLE_RIDGE==aStyle2  ||
          NS_STYLE_BORDER_STYLE_DOTTED==aStyle2 ||
          NS_STYLE_BORDER_STYLE_DASHED==aStyle2)
        result = BORDER_PRECEDENT_HIGHER;
      else
        result = BORDER_PRECEDENT_LOWER;
      break;

    case NS_STYLE_BORDER_STYLE_DOUBLE:
        result = BORDER_PRECEDENT_LOWER;
      break;
    }
  }
  return result;
}

/*
  This method is the CSS2 border conflict resolution algorithm
  The spec says to resolve conflicts in this order:
  1. any border with the style HIDDEN wins
  2. the widest border with a style that is not NONE wins
  3. the border styles are ranked in this order, highest to lowest precedence: 
        double, solid, dashed, dotted, ridge, outset, groove, inset
  4. borders that are of equal width and style (differ only in color) have this precedence:
        cell, row, rowgroup, col, colgroup, table
  5. if all border styles are NONE, then that's the computed border style.
  This method assumes that the styles were added to aStyles in the reverse precedence order
  of their frame type, so that styles that come later in the list win over style 
  earlier in the list if the tie-breaker gets down to #4.
  This method sets the out-param aBorder with the resolved border attributes
*/
void nsTableFrame::ComputeCollapsedBorderSegment(PRUint8       aSide, 
                                                 nsVoidArray  *aStyles, 
                                                 nsBorderEdge &aBorder,
                                                 PRBool        aFlipLastSide)
{
  if (nsnull!=aStyles)
  {
    PRInt32 styleCount=aStyles->Count();
    if (0!=styleCount)
    {
      nsVoidArray sameWidthBorders;
      nsStyleSpacing * spacing;
      nsStyleSpacing * lastSpacing=nsnull;
      nsMargin border;
      PRInt32 maxWidth=0;
      PRInt32 i;
      PRUint8 side = aSide;
      for (i=0; i<styleCount; i++)
      {
        spacing = (nsStyleSpacing *)(aStyles->ElementAt(i));
        if ((PR_TRUE==aFlipLastSide) && (i==styleCount-1))
        {
          side = GetOpposingEdge(aSide);
          lastSpacing = spacing;
        }
        if (spacing->GetBorderStyle(side)==NS_STYLE_BORDER_STYLE_HIDDEN)
        {
          aBorder.mStyle=NS_STYLE_BORDER_STYLE_HIDDEN;
          aBorder.mWidth=0;
          return;
        }
        else if (spacing->GetBorderStyle(side)!=NS_STYLE_BORDER_STYLE_NONE)
        {
          spacing->GetBorder(border);
          nscoord borderWidth = GetWidthForSide(border, side);
          if (borderWidth==maxWidth)
            sameWidthBorders.AppendElement(spacing);
          else if (borderWidth>maxWidth)
          {
            maxWidth=borderWidth;
            sameWidthBorders.Clear();
            sameWidthBorders.AppendElement(spacing);
          }
        }
      }
      aBorder.mWidth=maxWidth;
      // now we've gone through each overlapping border once, and we have a list
      // of all styles with the same width.  If there's more than one, resolve the
      // conflict based on border style
      styleCount = sameWidthBorders.Count();
      if (0==styleCount)
      {  // all borders were of style NONE
        aBorder.mWidth=0;
        aBorder.mStyle=NS_STYLE_BORDER_STYLE_NONE;
        return;
      }
      else if (1==styleCount)
      { // there was just one border of the largest width
        spacing = (nsStyleSpacing *)(sameWidthBorders.ElementAt(0));
        side=aSide;
        if (spacing==lastSpacing)
          side=GetOpposingEdge(aSide);
        if (! spacing->GetBorderColor(side, aBorder.mColor)) {
          // XXX EEEK handle transparent border color somehow...
        }
        aBorder.mStyle=spacing->GetBorderStyle(side);
        return;
      }
      else
      {
        nsStyleSpacing *winningStyleBorder;
        PRUint8 winningStyle=NS_STYLE_BORDER_STYLE_NONE;
        for (i=0; i<styleCount; i++)
        {
          spacing = (nsStyleSpacing *)(sameWidthBorders.ElementAt(i));
          side=aSide;
          if (spacing==lastSpacing)
            side=GetOpposingEdge(aSide);
          PRUint8 thisStyle = spacing->GetBorderStyle(side);
          PRUint8 borderCompare = CompareBorderStyles(thisStyle, winningStyle);
          if (BORDER_PRECEDENT_HIGHER==borderCompare)
          {
            winningStyle=thisStyle;
            winningStyleBorder = spacing;
          }
          else if (BORDER_PRECEDENT_EQUAL==borderCompare)
          { // we're in lowest-to-highest precedence order, so later border styles win
            winningStyleBorder=spacing;
          }          
        }
        aBorder.mStyle = winningStyle;
        side=aSide;
        if (winningStyleBorder==lastSpacing)
          side=GetOpposingEdge(aSide);
        if (! winningStyleBorder->GetBorderColor(side, aBorder.mColor)) {
          // XXX handle transparent border colors somehow
        }
      }
    }
  }

}

void nsTableFrame::RecalcLayoutData(nsIPresContext& aPresContext)
{
  nsCellMap* cellMap = GetCellMap();
  if (!cellMap)
    return; // no info yet, so nothing useful to do

  PRInt32 numRows = cellMap->GetRowCount();
  PRInt32 numCols = cellMap->GetColCount();
  PRInt32 cellSpacingX = GetCellSpacingX();
  PRInt32 cellSpacingY = GetCellSpacingY();
  PRInt32 highestBottom = numRows - 1;

  for (PRInt32 rowX = 0; rowX < numRows; rowX++) {
    for (PRInt32 colX = 0; colX < numCols; colX++) {
      PRBool originates;
      nsTableCellFrame* cell = GetCellInfoAt(rowX, colX, &originates);
      if (originates) {
        nscoord left = (0 == colX) ? cellSpacingX : 0;
        nscoord top  = (rowX > highestBottom) ? 0 : cellSpacingY;
        nsMargin margin(left, top, cellSpacingX, cellSpacingY);
        cell->RecalcLayoutData(margin);
        if (highestBottom > rowX) {
          highestBottom = PR_MIN(highestBottom, rowX + cell->GetRowSpan() - 1);
        }
      }
    }
  }
}


/////////////////////////////////////////////////////////////////////////////
// Child frame enumeration

NS_IMETHODIMP
nsTableFrame::FirstChild(nsIAtom* aListName, nsIFrame** aFirstChild) const
{
  if (nsnull == aListName) {
    *aFirstChild = mFrames.FirstChild();
    return NS_OK;
  }
  else if (aListName == nsLayoutAtoms::colGroupList) {
    *aFirstChild = mColGroups.FirstChild();
    return NS_OK;
  }
  *aFirstChild = nsnull;
  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
nsTableFrame::GetAdditionalChildListName(PRInt32   aIndex,
                                         nsIAtom** aListName) const
{
  NS_PRECONDITION(nsnull != aListName, "null OUT parameter pointer");
  if (aIndex < 0) {
    return NS_ERROR_INVALID_ARG;
  }
  *aListName = nsnull;
  switch (aIndex) {
  case NS_TABLE_FRAME_COLGROUP_LIST_INDEX:
    *aListName = nsLayoutAtoms::colGroupList;
    NS_ADDREF(*aListName);
    break;
  }
  return NS_OK;
}

/* SEC: TODO: adjust the rect for captions */
NS_METHOD nsTableFrame::Paint(nsIPresContext& aPresContext,
                              nsIRenderingContext& aRenderingContext,
                              const nsRect& aDirtyRect,
                              nsFramePaintLayer aWhichLayer)
{
  // table paint code is concerned primarily with borders and bg color
  if (NS_FRAME_PAINT_LAYER_BACKGROUND == aWhichLayer) {
    const nsStyleDisplay* disp =
      (const nsStyleDisplay*)mStyleContext->GetStyleData(eStyleStruct_Display);
    if (disp->mVisible) {
      const nsStyleSpacing* spacing =
        (const nsStyleSpacing*)mStyleContext->GetStyleData(eStyleStruct_Spacing);
      const nsStyleColor* color =
        (const nsStyleColor*)mStyleContext->GetStyleData(eStyleStruct_Color);

      nsRect  rect(0, 0, mRect.width, mRect.height);
        
      nsCompatibility mode;
      aPresContext.GetCompatibilityMode(&mode);
      if (eCompatibility_Standard == mode) {
        nsCSSRendering::PaintBackground(aPresContext, aRenderingContext, this,
                                        aDirtyRect, rect, *color, *spacing, 0, 0);
        // paint the column groups and columns
        nsIFrame* colGroupFrame = mColGroups.FirstChild();
        while (nsnull != colGroupFrame) {
          PaintChild(aPresContext, aRenderingContext, aDirtyRect, colGroupFrame, aWhichLayer);
          colGroupFrame->GetNextSibling(&colGroupFrame);
        }
      }
      PRIntn skipSides = GetSkipSides();
      if (NS_STYLE_BORDER_SEPARATE == GetBorderCollapseStyle())
      {
        nsCSSRendering::PaintBorder(aPresContext, aRenderingContext, this,
                                    aDirtyRect, rect, *spacing, mStyleContext, skipSides);
      }
      else
      {
        //printf("paint table frame\n");
        nsCSSRendering::PaintBorderEdges(aPresContext, aRenderingContext, this,
                                         aDirtyRect, rect,  mBorderEdges, mStyleContext, skipSides);
      }
    }
  }
  // for debug...
  if ((NS_FRAME_PAINT_LAYER_DEBUG == aWhichLayer) && GetShowFrameBorders()) {
    aRenderingContext.SetColor(NS_RGB(0,255,0));
    aRenderingContext.DrawRect(0, 0, mRect.width, mRect.height);
  }

  PaintChildren(aPresContext, aRenderingContext, aDirtyRect, aWhichLayer);
  return NS_OK;
  /*nsFrame::Paint(aPresContext,
                        aRenderingContext,
                        aDirtyRect,
                        aWhichLayer);*/
}


//null range means the whole thing
NS_IMETHODIMP
nsTableFrame::SetSelected(nsIDOMRange *aRange,PRBool aSelected, nsSpread aSpread)
{
#if 0
  //traverse through children unselect tables
  if ((aSpread == eSpreadDown)){
    nsIFrame* kid;
    nsresult rv = FirstChild(nsnull, &kid);
    while (NS_SUCCEEDED(rv) && nsnull != kid) {
      kid->SetSelected(nsnull,aSelected,eSpreadDown);

      rv = kid->GetNextSibling(&kid);
    }
  }
#endif
  return NS_OK;//return nsFrame::SetSelected(aRange,aSelected,eSpreadNone);
  
}

PRBool nsTableFrame::ParentDisablesSelection() const //override default behavior
{
  PRBool returnval;
  if (NS_FAILED(GetSelected(&returnval)))
    return PR_FALSE;
  if (returnval)
    return PR_TRUE;
  return nsFrame::ParentDisablesSelection();
}

PRIntn
nsTableFrame::GetSkipSides() const
{
  PRIntn skip = 0;
  // frame attribute was accounted for in nsHTMLTableElement::MapTableBorderInto
  // account for pagination
  if (nsnull != mPrevInFlow) {
    skip |= 1 << NS_SIDE_TOP;
  }
  if (nsnull != mNextInFlow) {
    skip |= 1 << NS_SIDE_BOTTOM;
  }
  return skip;
}

PRBool nsTableFrame::NeedsReflow(const nsHTMLReflowState& aReflowState)
{
  PRBool result = PR_TRUE;

  if (mBits.mIsInvariantWidth) {
    result = PR_FALSE;

  } else if ((eReflowReason_Incremental == aReflowState.reason) &&
             (NS_UNCONSTRAINEDSIZE == aReflowState.availableHeight)) {
    // If it's an incremental reflow and we're in galley mode, then only
    // do a full reflow if we need to. We need to if the cell map is invalid,
    // or the column info is invalid, or if we need to do a pass-1 reflow
    result = !(IsCellMapValid() && IsFirstPassValid() &&
               IsColumnCacheValid() && IsColumnWidthsValid());
  }
  return result;
}

// Called by IR_TargetIsChild() after an incremental reflow of
// aKidFrame. Only called if we don't need a full reflow, e.g., the
// column widths haven't changed. Not used for paginated mode, so
// we don't need to worry about split row group frames
//
// Slides all the row groups following aKidFrame by the specified
// amount
//
// XXX This is kind of klunky because the InnerTableReflowState::y
// data member does not include the table's border/padding...
nsresult nsTableFrame::AdjustSiblingsAfterReflow(nsIPresContext&        aPresContext,
                                                 InnerTableReflowState& aReflowState,
                                                 nsIFrame*              aKidFrame,
                                                 nsSize*                aMaxElementSize,
                                                 nscoord                aDeltaY)
{
  NS_PRECONDITION(NS_UNCONSTRAINEDSIZE == aReflowState.reflowState.availableHeight,
                  "we're not in galley mode");

  // If it's the footer that was reflowed, then we don't need to adjust any of
  // the frames, because the footer is the bottom most frame
  if (aKidFrame != aReflowState.footerFrame) {
    nsIFrame* kidFrame;
    nsRect    kidRect;
    PRBool    movedFooter = PR_FALSE;
  
    // Move the frames that follow aKidFrame by aDeltaY, and update the running
    // y-offset
    for (aKidFrame->GetNextSibling(&kidFrame); kidFrame; kidFrame->GetNextSibling(&kidFrame)) {
      // See if it's the footer we're moving
      if (kidFrame == aReflowState.footerFrame) {
        movedFooter = PR_TRUE;
      }
  
      // Get the frame's bounding rect
      kidFrame->GetRect(kidRect);
  
      // Adjust the running y-offset
      aReflowState.y += kidRect.height;

      // Update the max element size
      //XXX: this should call into layout strategy to get the width field
      if (aMaxElementSize) 
      {
        const nsStyleSpacing* tableSpacing;
        GetStyleData(eStyleStruct_Spacing , ((const nsStyleStruct *&)tableSpacing));
        nsMargin borderPadding;
        GetTableBorder (borderPadding); // gets the max border thickness for each edge
        nsMargin padding;
        tableSpacing->GetPadding(padding);
        borderPadding += padding;
        nscoord cellSpacing = GetCellSpacingX();
        nsSize  kidMaxElementSize;
        ((nsTableRowGroupFrame*)kidFrame)->GetMaxElementSize(kidMaxElementSize);
        nscoord kidWidth = kidMaxElementSize.width + borderPadding.left + borderPadding.right + cellSpacing*2;
        aMaxElementSize->width = PR_MAX(aMaxElementSize->width, kidWidth); 
        aMaxElementSize->height += kidMaxElementSize.height;
      }
  
      // Adjust the y-origin if its position actually changed
      if (aDeltaY != 0) {
        kidRect.y += aDeltaY;
        kidFrame->MoveTo(kidRect.x, kidRect.y);
      }
    }
  
    // We also need to move the footer if there is one and we haven't already
    // moved it
    if (aReflowState.footerFrame && !movedFooter) {
      aReflowState.footerFrame->GetRect(kidRect);
      
      // Adjust the running y-offset
      aReflowState.y += kidRect.height;
  
      if (aDeltaY != 0) {
        kidRect.y += aDeltaY;
        aReflowState.footerFrame->MoveTo(kidRect.x, kidRect.y);
      }
    }

    // Invalidate the area we offset. Note that we only repaint within
    // our existing frame bounds.
    // XXX It would be better to bitblt the row frames and not repaint,
    // but we don't have such a view manager function yet...
    aKidFrame->GetRect(kidRect);
    if (kidRect.YMost() < mRect.height) {
      nsRect  dirtyRect(0, kidRect.YMost(),
                        mRect.width, mRect.height - kidRect.YMost());
      Invalidate(dirtyRect);
    }
  }

  return NS_OK;
}

void nsTableFrame::SetColumnDimensions(nscoord aHeight)
{
  const nsStyleSpacing* spacing =
    (const nsStyleSpacing*)mStyleContext->GetStyleData(eStyleStruct_Spacing);
  nsMargin borderPadding;
  spacing->CalcBorderPaddingFor(this, borderPadding);
  nscoord colHeight = aHeight -= borderPadding.top + borderPadding.bottom;
  nscoord cellSpacingX = GetCellSpacingX();
  nscoord halfCellSpacingX = NSToCoordRound(((float)cellSpacingX) / (float)2);

  nsIFrame* colGroupFrame = mColGroups.FirstChild();
  PRInt32 colX = 0;
  nsPoint colGroupOrigin(borderPadding.left, borderPadding.top);
  PRInt32 numCols = GetColCount();
  while (nsnull != colGroupFrame) {
    nscoord colGroupWidth = 0;
    nsIFrame* colFrame = nsnull;
    colGroupFrame->FirstChild(nsnull, &colFrame);
    nsPoint colOrigin(0, 0);
    while (nsnull != colFrame) {
      const nsStyleDisplay* colDisplay;
      colFrame->GetStyleData(eStyleStruct_Display, ((const nsStyleStruct *&)colDisplay));
      if (NS_STYLE_DISPLAY_TABLE_COLUMN == colDisplay->mDisplay) {
        NS_ASSERTION(colX < numCols, "invalid number of columns");
        nscoord colWidth = mColumnWidths[colX];
        if (numCols == 1) {
          colWidth += cellSpacingX + cellSpacingX;
        }
        else if ((0 == colX) || (numCols - 1 == colX)) {
          colWidth += cellSpacingX + halfCellSpacingX;
        }
        else if (GetNumCellsOriginatingInCol(colX) > 0) {
          colWidth += cellSpacingX;
        }

        colGroupWidth += colWidth;
        nsRect colRect(colOrigin.x, colOrigin.y, colWidth, colHeight);
        colFrame->SetRect(colRect);
        colOrigin.x += colWidth;
        colX++;
      }
      colFrame->GetNextSibling(&colFrame);
    }
    nsRect colGroupRect(colGroupOrigin.x, colGroupOrigin.y, colGroupWidth, colHeight);
    colGroupFrame->SetRect(colGroupRect);
    colGroupFrame->GetNextSibling(&colGroupFrame);
    colGroupOrigin.x += colGroupWidth;
  }
}

// SEC: TODO need to worry about continuing frames prev/next in flow for splitting across pages.

/* overview:
  if mFirstPassValid is false, this is our first time through since content was last changed
    do pass 1
      get min/max info for all cells in an infinite space
  do column balancing
  do pass 2
  use column widths to size table and ResizeReflow rowgroups (and therefore rows and cells)
*/

/* Layout the entire inner table. */
NS_METHOD nsTableFrame::Reflow(nsIPresContext& aPresContext,
                               nsHTMLReflowMetrics& aDesiredSize,
                               const nsHTMLReflowState& aReflowState,
                               nsReflowStatus& aStatus)
{
  if (nsDebugTable::gRflTable) nsTableFrame::DebugReflow("T::Rfl en", this, &aReflowState, nsnull);

  // Initialize out parameter
  if (nsnull != aDesiredSize.maxElementSize) {
    aDesiredSize.maxElementSize->width = 0;
    aDesiredSize.maxElementSize->height = 0;
  }
  aStatus = NS_FRAME_COMPLETE;
  if ((NS_UNCONSTRAINEDSIZE == aReflowState.availableWidth) &&
      (this == (nsTableFrame *)GetFirstInFlow())) {
    InvalidateFirstPassCache();
  }

  nsresult rv = NS_OK;

  if (eReflowReason_Incremental == aReflowState.reason) {
    rv = IncrementalReflow(aPresContext, aDesiredSize, aReflowState, aStatus);
  }

  // NeedsReflow and IsFirstPassValid take into account reflow type = Initial_Reflow
  if (PR_TRUE==NeedsReflow(aReflowState))
  {
    PRBool needsRecalc=PR_FALSE;
    if (eReflowReason_Initial!=aReflowState.reason && PR_FALSE==IsCellMapValid())
    {
      if (nsnull!=mCellMap)
        delete mCellMap;
      mCellMap = new nsCellMap(0,0);
      ReBuildCellMap();
      needsRecalc=PR_TRUE;
    }
    if ((NS_UNCONSTRAINEDSIZE == aReflowState.availableWidth) || 
        (PR_FALSE==IsFirstPassValid()))
    {
      nsReflowReason reason = aReflowState.reason;
      if (eReflowReason_Initial!=reason)
        reason = eReflowReason_Resize;
      ComputeVerticalCollapsingBorders(aPresContext, 0, -1);
      rv = ResizeReflowPass1(aPresContext, aDesiredSize, aReflowState, aStatus, nsnull, reason, PR_TRUE);
      if (NS_FAILED(rv))
        return rv;
      needsRecalc=PR_TRUE;
    }
    if (PR_FALSE==IsColumnCacheValid())
    {
      needsRecalc=PR_TRUE;
    }
    if (PR_TRUE==needsRecalc)
    {
      BuildColumnCache(aPresContext, aDesiredSize, aReflowState, aStatus);
      RecalcLayoutData(aPresContext);  // Recalculate Layout Dependencies
      // if we needed to rebuild the column cache, the data stored in the layout strategy is invalid
      if (nsnull!=mTableLayoutStrategy)
      {
        mTableLayoutStrategy->Initialize(aDesiredSize.maxElementSize, GetColCount(), aReflowState.mComputedWidth);
        mBits.mColumnWidthsValid=PR_TRUE; //so we don't do this a second time below
      }
    }
    if (PR_FALSE==IsColumnWidthsValid())
    {
      if (nsnull!=mTableLayoutStrategy)
      {
        mTableLayoutStrategy->Initialize(aDesiredSize.maxElementSize, GetColCount(), aReflowState.mComputedWidth);
        mBits.mColumnWidthsValid=PR_TRUE;
      }
    }

    if (nsnull==mPrevInFlow)
    { // only do this for a first-in-flow table frame
      // assign column widths, and assign aMaxElementSize->width
      BalanceColumnWidths(aPresContext, aReflowState, nsSize(aReflowState.availableWidth, aReflowState.availableHeight),
                          aDesiredSize.maxElementSize);

      // assign table width
      SetTableWidth(aPresContext);
    }

    // Constrain our reflow width to the computed table width. Note: this is based
    // on the width of the first-in-flow
    nsHTMLReflowState reflowState(aReflowState);
    PRInt32 pass1Width = mRect.width;
    if (mPrevInFlow) {
      nsTableFrame* table = (nsTableFrame*)GetFirstInFlow();
      pass1Width = table->mRect.width;
    }
    reflowState.availableWidth = pass1Width;
    rv = ResizeReflowPass2(aPresContext, aDesiredSize, reflowState, aStatus);
    if (NS_FAILED(rv)) {
      return rv;
    }
    aDesiredSize.width = PR_MIN(aDesiredSize.width, pass1Width);

    // If this is an incremental reflow and we're here that means we had to
    // reflow all the rows, e.g., the column widths changed. We need to make
    // sure that any damaged areas are repainted
    if (eReflowReason_Incremental == aReflowState.reason) {
      nsRect  damageRect;

      damageRect.x = 0;
      damageRect.y = 0;
      damageRect.width = mRect.width;
      damageRect.height = mRect.height;
      Invalidate(damageRect);
    }
  }
  else
  {
    // set aDesiredSize and aMaxElementSize
  }

  SetColumnDimensions(aDesiredSize.height);

  if (nsDebugTable::gRflTable) nsTableFrame::DebugReflow("T::Rfl ex", this, nsnull, &aDesiredSize);
  return rv;
}

/** the first of 2 reflow passes
  * lay out the captions and row groups in an infinite space (NS_UNCONSTRAINEDSIZE)
  * cache the results for each caption and cell.
  * if successful, set mFirstPassValid=PR_TRUE, so we know we can skip this step 
  * next time.  mFirstPassValid is set to PR_FALSE when content is changed.
  * NOTE: should never get called on a continuing frame!  All cached pass1 state
  *       is stored in the inner table first-in-flow.
  */
NS_METHOD nsTableFrame::ResizeReflowPass1(nsIPresContext&          aPresContext,
                                          nsHTMLReflowMetrics&     aDesiredSize,
                                          const nsHTMLReflowState& aReflowState,
                                          nsReflowStatus&          aStatus,
                                          nsTableRowGroupFrame *   aStartingFrame,
                                          nsReflowReason           aReason,
                                          PRBool                   aDoSiblingFrames)
{
  NS_PRECONDITION(aReflowState.frame == this, "bad reflow state");
  NS_PRECONDITION(aReflowState.parentReflowState->frame == mParent,
                  "bad parent reflow state");
  NS_ASSERTION(nsnull==mPrevInFlow, "illegal call, cannot call pass 1 on a continuing frame.");
  NS_ASSERTION(nsnull != mContent, "null content");

  nsresult rv=NS_OK;
  // set out params
  aStatus = NS_FRAME_COMPLETE;

  nsSize availSize(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE); // availSize is the space available at any given time in the process
  nsSize maxSize(0, 0);       // maxSize is the size of the largest child so far in the process
  nsSize kidMaxSize(0,0);
  nsHTMLReflowMetrics kidSize(&kidMaxSize);
  nscoord y = 0;

  // Compute the insets (sum of border and padding)
  // XXX: since this is pass1 reflow and where we place the rowgroup frames is irrelevant, insets are probably a waste

  if (PR_TRUE==RequiresPass1Layout())
  {
    nsIFrame* kidFrame = aStartingFrame;
    if (nsnull==kidFrame)
      kidFrame=mFrames.FirstChild();   
    for ( ; nsnull != kidFrame; kidFrame->GetNextSibling(&kidFrame)) 
    {
      const nsStyleDisplay *childDisplay;
      kidFrame->GetStyleData(eStyleStruct_Display, ((const nsStyleStruct *&)childDisplay));
      if ((NS_STYLE_DISPLAY_TABLE_HEADER_GROUP != childDisplay->mDisplay) &&
          (NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP != childDisplay->mDisplay) &&
          (NS_STYLE_DISPLAY_TABLE_ROW_GROUP    != childDisplay->mDisplay) )
      { // it's an unknown frame type, give it a generic reflow and ignore the results
        nsHTMLReflowState kidReflowState(aPresContext, aReflowState, kidFrame, 
                                         availSize, aReason);
        // rv intentionally not set here
        ReflowChild(kidFrame, aPresContext, kidSize, kidReflowState, aStatus);
        continue;
      }
      nsHTMLReflowState kidReflowState(aPresContext, aReflowState, kidFrame,
                                       availSize, aReason);
      // Note: we don't bother checking here for whether we should clear the
      // isTopOfPage reflow state flag, because we're dealing with an unconstrained
      // height and it isn't an issue...
      ReflowChild(kidFrame, aPresContext, kidSize, kidReflowState, aStatus);

      // Place the child since some of its content fit in us.
      kidFrame->SetRect(nsRect(0, 0, kidSize.width, kidSize.height));
      if (NS_UNCONSTRAINEDSIZE==kidSize.height)
        y = NS_UNCONSTRAINEDSIZE;
      else
        y += kidSize.height;
      if (kidMaxSize.width > maxSize.width) {
        maxSize.width = kidMaxSize.width;
      }
      if (kidMaxSize.height > maxSize.height) {
        maxSize.height = kidMaxSize.height;
      }

      if (NS_FRAME_IS_NOT_COMPLETE(aStatus)) {
        // If the child didn't finish layout then it means that it used
        // up all of our available space (or needs us to split).
        break;
      }
      if (PR_FALSE==aDoSiblingFrames)
        break;
    }

    // if required, give the colgroups their initial reflows
    if (PR_TRUE==aDoSiblingFrames)
    {
      kidFrame=mColGroups.FirstChild();   
      for ( ; nsnull != kidFrame; kidFrame->GetNextSibling(&kidFrame)) 
      {
        nsHTMLReflowState kidReflowState(aPresContext, aReflowState, kidFrame,
                                         availSize, aReason);
        ReflowChild(kidFrame, aPresContext, kidSize, kidReflowState, aStatus);
        kidFrame->SetRect(nsRect(0, 0, 0, 0));
      }
    }
  }

  aDesiredSize.width = kidSize.width;
  mBits.mFirstPassValid = PR_TRUE;

  return rv;
}
  
nsIFrame*
nsTableFrame::GetFirstBodyRowGroupFrame()
{
  nsIFrame* headerFrame = nsnull;
  nsIFrame* footerFrame = nsnull;

  for (nsIFrame* kidFrame = mFrames.FirstChild(); nsnull != kidFrame; ) {
    const nsStyleDisplay *childDisplay;
    kidFrame->GetStyleData(eStyleStruct_Display, ((const nsStyleStruct *&)childDisplay));

    // We expect the header and footer row group frames to be first, and we only
    // allow one header and one footer
    if (NS_STYLE_DISPLAY_TABLE_HEADER_GROUP == childDisplay->mDisplay) {
      if (headerFrame) {
        // We already have a header frame and so this header frame is treated
        // like an ordinary body row group frame
        return kidFrame;
      }
      headerFrame = kidFrame;
    
    } else if (NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP == childDisplay->mDisplay) {
      if (footerFrame) {
        // We already have a footer frame and so this footer frame is treated
        // like an ordinary body row group frame
        return kidFrame;
      }
      footerFrame = kidFrame;

    } else if (NS_STYLE_DISPLAY_TABLE_ROW_GROUP == childDisplay->mDisplay) {
      return kidFrame;
    }

    // Get the next child
    kidFrame->GetNextSibling(&kidFrame);
  }

  return nsnull;
}

// Table specific version that takes into account repeated header and footer
// frames when continuing table frames
void
nsTableFrame::PushChildren(nsIFrame* aFromChild, nsIFrame* aPrevSibling)
{
  NS_PRECONDITION(nsnull != aFromChild, "null pointer");
  NS_PRECONDITION(nsnull != aPrevSibling, "pushing first child");
#ifdef NS_DEBUG
  nsIFrame* prevNextSibling;
  aPrevSibling->GetNextSibling(&prevNextSibling);
  NS_PRECONDITION(prevNextSibling == aFromChild, "bad prev sibling");
#endif

  // Disconnect aFromChild from its previous sibling
  aPrevSibling->SetNextSibling(nsnull);

  if (nsnull != mNextInFlow) {
    nsTableFrame* nextInFlow = (nsTableFrame*)mNextInFlow;

    // Insert the frames after any repeated header and footer frames
    nsIFrame* firstBodyFrame = nextInFlow->GetFirstBodyRowGroupFrame();
    nsIFrame* prevSibling = nsnull;
    if (firstBodyFrame) {
      prevSibling = nextInFlow->mFrames.GetPrevSiblingFor(firstBodyFrame);
    }
    // When pushing and pulling frames we need to check for whether any
    // views need to be reparented.
    for (nsIFrame* f = aFromChild; f; f->GetNextSibling(&f)) {
      nsHTMLContainerFrame::ReparentFrameView(f, this, nextInFlow);
    }
    nextInFlow->mFrames.InsertFrames(mNextInFlow, prevSibling, aFromChild);
  }
  else {
    // Add the frames to our overflow list
    NS_ASSERTION(mOverflowFrames.IsEmpty(), "bad overflow list");
    mOverflowFrames.SetFrames(aFromChild);
  }
}

// Table specific version that takes into account header and footer row group
// frames that are repeated for continuing table frames
//
// Appends the overflow frames to the end of the child list, just like the
// nsContainerFrame version does, except that there are no assertions that
// the child list is empty (it may not be empty, because there may be repeated
// header/footer frames)
PRBool
nsTableFrame::MoveOverflowToChildList()
{
  PRBool result = PR_FALSE;

  // Check for an overflow list with our prev-in-flow
  nsTableFrame* prevInFlow = (nsTableFrame*)mPrevInFlow;
  if (nsnull != prevInFlow) {
    if (prevInFlow->mOverflowFrames.NotEmpty()) {
      // When pushing and pulling frames we need to check for whether any
      // views need to be reparented.
      for (nsIFrame* f = prevInFlow->mOverflowFrames.FirstChild(); f; f->GetNextSibling(&f)) {
        nsHTMLContainerFrame::ReparentFrameView(f, prevInFlow, this);
      }
      mFrames.InsertFrames(this, nsnull, prevInFlow->mOverflowFrames);
      result = PR_TRUE;
    }
  }

  // It's also possible that we have an overflow list for ourselves
  if (mOverflowFrames.NotEmpty()) {
    mFrames.AppendFrames(nsnull, mOverflowFrames);
    result = PR_TRUE;
  }
  return result;
}

/** the second of 2 reflow passes
  */
NS_METHOD nsTableFrame::ResizeReflowPass2(nsIPresContext&          aPresContext,
                                          nsHTMLReflowMetrics&     aDesiredSize,
                                          const nsHTMLReflowState& aReflowState,
                                          nsReflowStatus&          aStatus)
{
  NS_PRECONDITION(aReflowState.frame == this, "bad reflow state");
  NS_PRECONDITION(aReflowState.parentReflowState->frame == mParent,
                  "bad parent reflow state");
  nsresult rv = NS_OK;
  // set out param
  aStatus = NS_FRAME_COMPLETE;

  const nsStyleSpacing* mySpacing = (const nsStyleSpacing*)
    mStyleContext->GetStyleData(eStyleStruct_Spacing);
  nsMargin borderPadding;
  GetTableBorder (borderPadding);   // this gets the max border thickness at each edge
  nsMargin padding;
  mySpacing->GetPadding(padding);
  borderPadding += padding;

  InnerTableReflowState state(aPresContext, aReflowState, borderPadding);

  // now that we've computed the column  width information, reflow all children

#ifdef NS_DEBUG
  //PreReflowCheck();
#endif

  // Check for an overflow list, and append any row group frames being
  // pushed
  MoveOverflowToChildList();

  // Reflow the existing frames
  if (mFrames.NotEmpty()) {
    ComputePercentBasisForRows(aReflowState);
    rv = ReflowMappedChildren(aPresContext, aDesiredSize, state, aStatus);
  }

  // Did we successfully reflow our mapped children?
  if (NS_FRAME_COMPLETE == aStatus) {
    // Any space left?
    if (state.availSize.height > 0) {
      // Try and pull-up some children from a next-in-flow
      rv = PullUpChildren(aPresContext, aDesiredSize, state, aStatus);
    }
  }

  // Return our size and our status
  aDesiredSize.width = ComputeDesiredWidth(aReflowState);
  nscoord defaultHeight = state.y + borderPadding.top + borderPadding.bottom;
  aDesiredSize.height = ComputeDesiredHeight(aPresContext, aReflowState, defaultHeight);
 
  AdjustForCollapsingRows(aPresContext, aDesiredSize.height);
  AdjustForCollapsingCols(aPresContext, aDesiredSize.width);

  // once horizontal borders are computed and all row heights are set, 
  // we need to fix up length of vertical edges
  // XXX need to figure start row and end row correctly
  if (NS_STYLE_BORDER_COLLAPSE==GetBorderCollapseStyle())
    DidComputeHorizontalCollapsingBorders(aPresContext, 0, 10000);

#ifdef NS_DEBUG
  //PostReflowCheck(aStatus);
#endif

  return rv;

}

void nsTableFrame::ComputePercentBasisForRows(const nsHTMLReflowState& aReflowState)
{
  nscoord height;
  GetTableSpecifiedHeight(height, aReflowState);
  if ((height > 0) && (height < NS_UNCONSTRAINEDSIZE)) {
    // exclude our border and padding
    const nsStyleSpacing* spacing =
      (const nsStyleSpacing*)mStyleContext->GetStyleData(eStyleStruct_Spacing);
	  nsMargin borderPadding(0,0,0,0);
    // XXX handle percentages
    if (spacing->GetBorderPadding(borderPadding)) {
     height -= borderPadding.top + borderPadding.bottom;
    }
    // exclude cell spacing for all rows
    height -= (1 + GetRowCount()) * GetCellSpacingY();
    height = PR_MAX(0, height);
  }
  else {
    height = 0;
  }
  mPercentBasisForRows = height;
}

// collapsing row groups, rows, col groups and cols are accounted for after both passes of
// reflow so that it has no effect on the calculations of reflow.
NS_METHOD nsTableFrame::AdjustForCollapsingRowGroup(nsIFrame* aRowGroupFrame, 
                                                    PRInt32& aRowX)
{
  nsCellMap* cellMap = GetCellMap(); // XXX is this right
  const nsStyleDisplay* groupDisplay;
  aRowGroupFrame->GetStyleData(eStyleStruct_Display, ((const nsStyleStruct *&)groupDisplay));
  PRBool groupIsCollapsed = (NS_STYLE_VISIBILITY_COLLAPSE == groupDisplay->mVisible);

  nsTableRowFrame* rowFrame = nsnull;
  aRowGroupFrame->FirstChild(nsnull, (nsIFrame**)&rowFrame);
  while (nsnull != rowFrame) { 
    const nsStyleDisplay *rowDisplay;
    rowFrame->GetStyleData(eStyleStruct_Display, ((const nsStyleStruct *&)rowDisplay));
    if (NS_STYLE_DISPLAY_TABLE_ROW == rowDisplay->mDisplay) {
      if (groupIsCollapsed || (NS_STYLE_VISIBILITY_COLLAPSE == rowDisplay->mVisible)) {
        cellMap->SetRowCollapsedAt(aRowX, PR_TRUE);
      }
      aRowX++;
    }
    else if (NS_STYLE_DISPLAY_TABLE_ROW_GROUP == rowDisplay->mDisplay) {
      AdjustForCollapsingRowGroup(rowFrame, aRowX);
    }
    
    rowFrame->GetNextSibling((nsIFrame**)&rowFrame);
  }

  return NS_OK;
}

NS_METHOD nsTableFrame::CollapseRowGroup(nsIFrame* aRowGroupFrame,
                                         const nscoord& aYTotalOffset,
                                         nscoord& aYGroupOffset, PRInt32& aRowX)
{
  const nsStyleDisplay* groupDisplay;
  aRowGroupFrame->GetStyleData(eStyleStruct_Display, ((const nsStyleStruct *&)groupDisplay));
  
  PRBool collapseGroup = (NS_STYLE_VISIBILITY_COLLAPSE == groupDisplay->mVisible);
  nsIFrame* rowFrame;
  aRowGroupFrame->FirstChild(nsnull, &rowFrame);

  while (nsnull != rowFrame) {
    const nsStyleDisplay* rowDisplay;
    rowFrame->GetStyleData(eStyleStruct_Display, ((const nsStyleStruct *&)rowDisplay));
    if (NS_STYLE_DISPLAY_TABLE_ROW_GROUP == rowDisplay->mDisplay) {
      CollapseRowGroup(rowFrame, aYTotalOffset, aYGroupOffset, aRowX);
    }
    else if (NS_STYLE_DISPLAY_TABLE_ROW == rowDisplay->mDisplay) {
      nsRect rowRect;
      rowFrame->GetRect(rowRect);
      if (collapseGroup || (NS_STYLE_VISIBILITY_COLLAPSE == rowDisplay->mVisible)) {
        aYGroupOffset += rowRect.height;
        rowRect.height = 0;
        rowFrame->SetRect(rowRect);
        nsIFrame* cellFrame;
        rowFrame->FirstChild(nsnull, &cellFrame);
        while (nsnull != cellFrame) {
          const nsStyleDisplay* cellDisplay;
          cellFrame->GetStyleData(eStyleStruct_Display, ((const nsStyleStruct *&)cellDisplay));
          if (NS_STYLE_DISPLAY_TABLE_CELL == cellDisplay->mDisplay) {
            nsTableCellFrame* cFrame = (nsTableCellFrame*)cellFrame;
            nsRect cRect;
            cFrame->GetRect(cRect);
            cRect.height -= rowRect.height;
            cFrame->SetCollapseOffsetY(-aYGroupOffset);
            cFrame->SetRect(cRect);
          }
          cellFrame->GetNextSibling(&cellFrame);
        }
        // check if a cell above spans into here
         //if (!collapseGroup) {
          PRInt32 numCols = mCellMap->GetColCount();
          nsTableCellFrame* lastCell = nsnull;
          for (int colX = 0; colX < numCols; colX++) {
            CellData* cellData = mCellMap->GetCellAt(aRowX, colX);
            if (cellData && !cellData->mOrigCell) { // a cell above is spanning into here
              // adjust the real cell's rect only once
              nsTableCellFrame* realCell = nsnull;
              if (cellData->mRowSpanData)
                realCell = cellData->mRowSpanData->mOrigCell;
              if (realCell != lastCell) {
                nsRect realRect;
                realCell->GetRect(realRect);
                realRect.height -= rowRect.height;
                realCell->SetRect(realRect);
              }
              lastCell = realCell;
            }
          }
        //}
      } else { // row is not collapsed but needs to be adjusted by those that are
        rowRect.y -= aYGroupOffset;
        rowFrame->SetRect(rowRect);
      }
      aRowX++;
    }
    rowFrame->GetNextSibling(&rowFrame);
  } // end row frame while

  nsRect groupRect;
  aRowGroupFrame->GetRect(groupRect);
  groupRect.height -= aYGroupOffset;
  groupRect.y -= aYTotalOffset;
  aRowGroupFrame->SetRect(groupRect);

  return NS_OK;
}

NS_METHOD nsTableFrame::AdjustForCollapsingRows(nsIPresContext& aPresContext, 
                                                nscoord&        aHeight)
{
  // determine which row groups and rows are collapsed
  PRInt32 rowX = 0;
  nsIFrame* childFrame;
  FirstChild(nsnull, &childFrame);
  while (nsnull != childFrame) { 
    const nsStyleDisplay* groupDisplay;
    childFrame->GetStyleData(eStyleStruct_Display, ((const nsStyleStruct *&)groupDisplay));
    if (IsRowGroup(groupDisplay->mDisplay)) {
      AdjustForCollapsingRowGroup(childFrame, rowX);
    }
    childFrame->GetNextSibling(&childFrame);
  }

  if (mCellMap->GetNumCollapsedRows() <= 0) { 
    return NS_OK; // no collapsed rows, we're done
  }

  // collapse the rows and/or row groups
  nsIFrame* groupFrame = mFrames.FirstChild(); 
  nscoord yGroupOffset = 0; // total offset among rows within a single row group
  nscoord yTotalOffset = 0; // total offset among all rows in all row groups
  rowX = 0;
  
  while (nsnull != groupFrame) {
    const nsStyleDisplay* groupDisplay;
    groupFrame->GetStyleData(eStyleStruct_Display, ((const nsStyleStruct *&)groupDisplay));
    if (IsRowGroup(groupDisplay->mDisplay)) {
      CollapseRowGroup(groupFrame, yTotalOffset, yGroupOffset, rowX);
    }
    yTotalOffset += yGroupOffset;
    yGroupOffset = 0;
    groupFrame->GetNextSibling(&groupFrame);
  } // end group frame while

  aHeight -= yTotalOffset;
 
  return NS_OK;
}

NS_METHOD nsTableFrame::AdjustForCollapsingCols(nsIPresContext& aPresContext, 
                                                nscoord&        aWidth)
{
  // determine which col groups and cols are collapsed
  nsIFrame* childFrame = mColGroups.FirstChild();
  PRInt32 numCols = 0;
  while (nsnull != childFrame) { 
    const nsStyleDisplay* groupDisplay;
    GetStyleData(eStyleStruct_Display, ((const nsStyleStruct *&)groupDisplay));
    PRBool groupIsCollapsed = (NS_STYLE_VISIBILITY_COLLAPSE == groupDisplay->mVisible);

    nsTableColFrame* colFrame = nsnull;
    childFrame->FirstChild(nsnull, (nsIFrame**)&colFrame);
    while (nsnull != colFrame) { 
      const nsStyleDisplay *colDisplay;
      colFrame->GetStyleData(eStyleStruct_Display, ((const nsStyleStruct *&)colDisplay));
      if (NS_STYLE_DISPLAY_TABLE_COLUMN == colDisplay->mDisplay) {
        if (groupIsCollapsed || (NS_STYLE_VISIBILITY_COLLAPSE == colDisplay->mVisible)) {
          mCellMap->SetColCollapsedAt(numCols, PR_TRUE);
        }
        numCols++;
      }
      colFrame->GetNextSibling((nsIFrame**)&colFrame);
    }
    childFrame->GetNextSibling(&childFrame);
  }

  if (mCellMap->GetNumCollapsedCols() <= 0) { 
    return NS_OK; // no collapsed cols, we're done
  }

  // collapse the cols and/or col groups
  PRInt32 numRows = mCellMap->GetRowCount();
  nsTableIterator groupIter(mColGroups, eTableDIR);
  nsIFrame* groupFrame = groupIter.First(); 
  nscoord cellSpacingX = GetCellSpacingX();
  nscoord xOffset = 0;
  PRInt32 colX = (groupIter.IsLeftToRight()) ? 0 : numCols - 1; 
  PRInt32 direction = (groupIter.IsLeftToRight()) ? 1 : -1; 
  while (nsnull != groupFrame) {
    const nsStyleDisplay* groupDisplay;
    groupFrame->GetStyleData(eStyleStruct_Display, ((const nsStyleStruct *&)groupDisplay));
    PRBool collapseGroup = (NS_STYLE_VISIBILITY_COLLAPSE == groupDisplay->mVisible);
    nsTableIterator colIter(*groupFrame, eTableDIR);
    nsIFrame* colFrame = colIter.First();
    while (nsnull != colFrame) {
      const nsStyleDisplay* colDisplay;
      colFrame->GetStyleData(eStyleStruct_Display, ((const nsStyleStruct *&)colDisplay));
      if (NS_STYLE_DISPLAY_TABLE_COLUMN == colDisplay->mDisplay) {
        PRBool collapseCol = (NS_STYLE_VISIBILITY_COLLAPSE == colDisplay->mVisible);
        PRInt32 colSpan = ((nsTableColFrame*)colFrame)->GetSpan();
        for (PRInt32 spanX = 0; spanX < colSpan; spanX++) {
          PRInt32 col2X = colX + (direction * spanX);
          PRInt32 colWidth = GetColumnWidth(col2X);
          if (collapseGroup || collapseCol) {
            xOffset += colWidth + cellSpacingX;
          }
          nsTableCellFrame* lastCell  = nsnull;
          nsTableCellFrame* cellFrame = nsnull;
          for (PRInt32 rowX = 0; rowX < numRows; rowX++) {
            CellData* cellData = mCellMap->GetCellAt(rowX, col2X);
            nsRect cellRect;
            if (cellData) {
              cellFrame = cellData->mOrigCell;
              if (cellFrame) { // the cell originates at (rowX, colX)
                cellFrame->GetRect(cellRect);
                if (collapseGroup || collapseCol) {
                  if (lastCell != cellFrame) { // do it only once if there is a row span
                    cellRect.width -= colWidth;
                    cellFrame->SetCollapseOffsetX(-xOffset);
                  }
                } else { // the cell is not in a collapsed col but needs to move
                  cellRect.x -= xOffset;
                }
                cellFrame->SetRect(cellRect);
              // if the cell does not originate at (rowX, colX), adjust the real cells width
              } else if (collapseGroup || collapseCol) { 
                if (cellData->mColSpanData)
                  cellFrame = cellData->mColSpanData->mOrigCell;
                if ((cellFrame) && (lastCell != cellFrame)) {
                  cellFrame->GetRect(cellRect);
                  cellRect.width -= colWidth + cellSpacingX;
                  cellFrame->SetRect(cellRect);
                }
              }
            }
            lastCell = cellFrame;
          }
        }
        colX += direction * colSpan;
      }
      colFrame = colIter.Next();
    } // inner while
    groupFrame = groupIter.Next();
  } // outer while

  aWidth -= xOffset;
 
  return NS_OK;
}

NS_METHOD nsTableFrame::GetBorderPlusMarginPadding(nsMargin& aResult)
{
  const nsStyleSpacing* mySpacing = (const nsStyleSpacing*)
    mStyleContext->GetStyleData(eStyleStruct_Spacing);
  nsMargin borderPadding;
  GetTableBorder (borderPadding);
  nsMargin padding;
  mySpacing->GetPadding(padding);
  borderPadding += padding;
  aResult = borderPadding;
  return NS_OK;
}

// Sets the starting column index for aColGroupFrame and the siblings frames that
// follow
void
nsTableFrame::SetStartingColumnIndexFor(nsTableColGroupFrame* aColGroupFrame,
                                        PRInt32 aIndex)
{
  while (aColGroupFrame) {
    aIndex += aColGroupFrame->SetStartColumnIndex(aIndex);
    aColGroupFrame->GetNextSibling((nsIFrame**)&aColGroupFrame);
  }
}

// Calculate the starting column index to use for the specified col group frame
PRInt32
nsTableFrame::CalculateStartingColumnIndexFor(nsTableColGroupFrame* aColGroupFrame)
{
  PRInt32 index = 0;
  for (nsTableColGroupFrame* colGroupFrame = (nsTableColGroupFrame*)mColGroups.FirstChild();
       colGroupFrame && (colGroupFrame != aColGroupFrame);
       colGroupFrame->GetNextSibling((nsIFrame**)&colGroupFrame))
  {
    index += colGroupFrame->GetColumnCount();
  }

  return index;
}

NS_IMETHODIMP
nsTableFrame::AppendFrames(nsIPresContext& aPresContext,
                           nsIPresShell&   aPresShell,
                           nsIAtom*        aListName,
                           nsIFrame*       aFrameList)
{
  PRInt32 startingColIndex = -1;

  // Because we actually have two child lists, one for col group frames and one
  // for everything else, we need to look at each frame individually
  nsIFrame* f = aFrameList;
  while (f) {
    nsIFrame* next;

    // Get the next frame and disconnect this frame from its sibling
    f->GetNextSibling(&next);
    f->SetNextSibling(nsnull);

    // See what kind of frame we have
    const nsStyleDisplay *display;
    f->GetStyleData(eStyleStruct_Display, ((const nsStyleStruct *&)display));

    if (NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP == display->mDisplay) {
      // Append the new col group frame
      mColGroups.AppendFrame(nsnull, f);

      // Set its starting column index
      if (-1 == startingColIndex) {
        startingColIndex = CalculateStartingColumnIndexFor((nsTableColGroupFrame*)f);
      }
      ((nsTableColGroupFrame*)f)->SetStartColumnIndex(startingColIndex);
      startingColIndex += ((nsTableColGroupFrame *)f)->GetColumnCount();
    
    } else if (IsRowGroup(display->mDisplay)) {
      // Append the new row group frame
      mFrames.AppendFrame(nsnull, f);
      
      // Add the content of the row group to the cell map
      DidAppendRowGroup((nsTableRowGroupFrame*)f);

    } else {
      // Nothing special to do, just add the frame to our child list
      mFrames.AppendFrame(nsnull, f);
    }

    // Move to the next frame
    f = next;
  }

  // We'll need to do a pass-1 layout of all cells in all the rows of the
  // rowgroup
  InvalidateFirstPassCache();
  
  // If we've added any columns, we need to rebuild the column cache.
  InvalidateColumnCache();
  
  // Because the number of columns may have changed invalidate the column
  // cache. Note that this has the side effect of recomputing the column
  // widths, so we don't need to call InvalidateColumnWidths()
  InvalidateColumnWidths();  

  // Mark the table as dirty and generate a reflow command targeted at the
  // outer table frame
  nsIReflowCommand* reflowCmd;
  nsresult          rv;
  
  mState |= NS_FRAME_IS_DIRTY;
  rv = NS_NewHTMLReflowCommand(&reflowCmd, mParent, nsIReflowCommand::ReflowDirty);
  if (NS_SUCCEEDED(rv)) {
    // Add the reflow command
    rv = aPresShell.AppendReflowCommand(reflowCmd);
    NS_RELEASE(reflowCmd);
  }

  return rv;
}

NS_IMETHODIMP
nsTableFrame::InsertFrames(nsIPresContext& aPresContext,
                           nsIPresShell&   aPresShell,
                           nsIAtom*        aListName,
                           nsIFrame*       aPrevFrame,
                           nsIFrame*       aFrameList)
{
  // Asssume there's only one frame being inserted. The problem is that
  // row group frames and col group frames go in separate child lists and
  // so if there's more than one this gets messy...
  // XXX The frame construction code should be separating out child frames
  // based on the type...
  nsIFrame* nextSibling;
  aFrameList->GetNextSibling(&nextSibling);
  NS_PRECONDITION(!nextSibling, "expected only one child frame");

  // See what kind of frame we have
  const nsStyleDisplay *display=nsnull;
  aFrameList->GetStyleData(eStyleStruct_Display, ((const nsStyleStruct *&)display));

  if (NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP == display->mDisplay) {
    // Insert the column group frame
    mColGroups.InsertFrame(nsnull, aPrevFrame, aFrameList);

    // Set its starting column index and adjust the index of the
    // col group frames that follow
    SetStartingColumnIndexFor((nsTableColGroupFrame*)aFrameList,
                              CalculateStartingColumnIndexFor((nsTableColGroupFrame*)aFrameList));
  
  } else if (IsRowGroup(display->mDisplay)) {
    // Insert the frame
    mFrames.InsertFrame(nsnull, aPrevFrame, aFrameList);

    // We'll need to do a pass-1 layout of all cells in all the rows of the
    // rowgroup
    InvalidateFirstPassCache();

    // We need to rebuild the cell map, because currently we can't insert
    // new frames except at the end (append)
    InvalidateCellMap();

  } else {
    // Just insert the frame and don't worry about reflowing it
    mFrames.InsertFrame(nsnull, aPrevFrame, aFrameList);
    return NS_OK;
  }

  // If we've added any columns, we need to rebuild the column cache.
  InvalidateColumnCache();
  
  // Because the number of columns may have changed invalidate the column
  // cache. Note that this has the side effect of recomputing the column
  // widths, so we don't need to call InvalidateColumnWidths()
  InvalidateColumnWidths();  
  
  // Mark the table as dirty and generate a reflow command targeted at the
  // outer table frame
  nsIReflowCommand* reflowCmd;
  nsresult          rv;
  
  mState |= NS_FRAME_IS_DIRTY;
  rv = NS_NewHTMLReflowCommand(&reflowCmd, mParent, nsIReflowCommand::ReflowDirty);
  if (NS_SUCCEEDED(rv)) {
    // Add the reflow command
    rv = aPresShell.AppendReflowCommand(reflowCmd);
    NS_RELEASE(reflowCmd);
  }
  return rv;
}

NS_IMETHODIMP
nsTableFrame::RemoveFrame(nsIPresContext& aPresContext,
                          nsIPresShell&   aPresShell,
                          nsIAtom*        aListName,
                          nsIFrame*       aOldFrame)
{
  // See what kind of frame we have
  const nsStyleDisplay *display=nsnull;
  aOldFrame->GetStyleData(eStyleStruct_Display, ((const nsStyleStruct *&)display));

  if (NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP == display->mDisplay) {
    nsIFrame* nextColGroupFrame;
    aOldFrame->GetNextSibling(&nextColGroupFrame);

    // Remove the col group frame
    mColGroups.DestroyFrame(aPresContext, aOldFrame);

    // Adjust the starting column index of the frames that follow
    SetStartingColumnIndexFor((nsTableColGroupFrame*)nextColGroupFrame,
                              CalculateStartingColumnIndexFor((nsTableColGroupFrame*)nextColGroupFrame));

  } else if (IsRowGroup(display->mDisplay)) {
    mFrames.DestroyFrame(aPresContext, aOldFrame);

    // We need to rebuild the cell map, because currently we can't incrementally
    // remove rows
    InvalidateCellMap();

  } else {
    // Just remove the frame
    mFrames.DestroyFrame(aPresContext, aOldFrame);
    return NS_OK;
  }

  // Because the number of columns may have changed invalidate the column
  // cache. Note that this has the side effect of recomputing the column
  // widths, so we don't need to call InvalidateColumnWidths()
  InvalidateColumnCache();
  
  // Because the number of columns may have changed invalidate the column
  // cache. Note that this has the side effect of recomputing the column
  // widths, so we don't need to call InvalidateColumnWidths()
  InvalidateColumnWidths();  
  
  // Mark the table as dirty and generate a reflow command targeted at the
  // outer table frame
  nsIReflowCommand* reflowCmd;
  nsresult          rv;
  
  mState |= NS_FRAME_IS_DIRTY;
  rv = NS_NewHTMLReflowCommand(&reflowCmd, mParent, nsIReflowCommand::ReflowDirty);
  if (NS_SUCCEEDED(rv)) {
    // Add the reflow command
    rv = aPresShell.AppendReflowCommand(reflowCmd);
    NS_RELEASE(reflowCmd);
  }

  return rv;
}

NS_METHOD nsTableFrame::IncrementalReflow(nsIPresContext& aPresContext,
                                          nsHTMLReflowMetrics& aDesiredSize,
                                          const nsHTMLReflowState& aReflowState,
                                          nsReflowStatus& aStatus)
{
  nsresult  rv = NS_OK;

  // Constrain our reflow width to the computed table width. Note: this is
  // based on the width of the first-in-flow
  nsHTMLReflowState reflowState(aReflowState);
  PRInt32 pass1Width = mRect.width;
  if (mPrevInFlow) {
    nsTableFrame* table = (nsTableFrame*)GetFirstInFlow();
    pass1Width = table->mRect.width;
  }
  reflowState.availableWidth = pass1Width;

  nsMargin borderPadding;
  GetBorderPlusMarginPadding(borderPadding);
  InnerTableReflowState state(aPresContext, reflowState, borderPadding);

  // determine if this frame is the target or not
  nsIFrame *target=nsnull;
  rv = reflowState.reflowCommand->GetTarget(target);
  if ((PR_TRUE==NS_SUCCEEDED(rv)) && (nsnull!=target))
  {
    // this is the target if target is either this or the outer table frame containing this inner frame
    nsIFrame *outerTableFrame=nsnull;
    GetParent(&outerTableFrame);
    if ((this==target) || (outerTableFrame==target))
      rv = IR_TargetIsMe(aPresContext, aDesiredSize, state, aStatus);
    else
    {
      // Get the next frame in the reflow chain
      nsIFrame* nextFrame;
      reflowState.reflowCommand->GetNext(nextFrame);

      // Recover our reflow state
      rv = IR_TargetIsChild(aPresContext, aDesiredSize, state, aStatus, nextFrame);
    }
  }
  return rv;
}

NS_METHOD nsTableFrame::IR_TargetIsMe(nsIPresContext&        aPresContext,
                                      nsHTMLReflowMetrics&   aDesiredSize,
                                      InnerTableReflowState& aReflowState,
                                      nsReflowStatus&        aStatus)
{
  nsresult rv = NS_OK;
  aStatus = NS_FRAME_COMPLETE;
  nsIReflowCommand::ReflowType type;
  aReflowState.reflowState.reflowCommand->GetType(type);
  nsIFrame *objectFrame;
  aReflowState.reflowState.reflowCommand->GetChildFrame(objectFrame); 
  const nsStyleDisplay *childDisplay=nsnull;
  if (nsnull!=objectFrame)
    objectFrame->GetStyleData(eStyleStruct_Display, ((const nsStyleStruct *&)childDisplay));
  switch (type)
  {
  case nsIReflowCommand::StyleChanged :
    rv = IR_StyleChanged(aPresContext, aDesiredSize, aReflowState, aStatus);
    break;

  case nsIReflowCommand::ContentChanged :
    NS_ASSERTION(PR_FALSE, "illegal reflow type: ContentChanged");
    rv = NS_ERROR_ILLEGAL_VALUE;
    break;
  
  case nsIReflowCommand::ReflowDirty:
    // Problem is we don't know has changed, so assume the worst
    InvalidateCellMap();
    InvalidateFirstPassCache();
    InvalidateColumnCache();
    InvalidateColumnWidths();
    rv = NS_OK;
    break;
  
  default:
    NS_NOTYETIMPLEMENTED("unexpected reflow command type");
    rv = NS_ERROR_NOT_IMPLEMENTED;
    break;
  }

  return rv;
}

NS_METHOD nsTableFrame::IR_StyleChanged(nsIPresContext&        aPresContext,
                                        nsHTMLReflowMetrics&   aDesiredSize,
                                        InnerTableReflowState& aReflowState,
                                        nsReflowStatus&        aStatus)
{
  nsresult rv = NS_OK;
  // we presume that all the easy optimizations were done in the nsHTMLStyleSheet before we were called here
  // XXX: we can optimize this when we know which style attribute changed
  //      if something like border changes, we need to do pass1 again
  //      but if something like width changes from 100 to 200, we just need to do pass2
  InvalidateFirstPassCache();
  return rv;
}

// Recovers the reflow state to what it should be if aKidFrame is about
// to be reflowed. Restores the following:
// - availSize
// - y
// - footerFrame
// - firstBodySection
//
// In the case of the footer frame the y-offset is set to its current
// y-offset. Note that this is different from resize reflow when the
// footer is positioned higher up and then moves down as each row
// group frame is relowed
//
// XXX This is kind of klunky because the InnerTableReflowState::y
// data member does not include the table's border/padding...
nsresult
nsTableFrame::RecoverState(InnerTableReflowState& aReflowState,
                           nsIFrame*              aKidFrame,
                           nsSize*                aMaxElementSize)
{
  // Walk the list of children looking for aKidFrame
  for (nsIFrame* frame = mFrames.FirstChild(); frame; frame->GetNextSibling(&frame)) {
    // If this is a footer row group, remember it
    const nsStyleDisplay *display;
    frame->GetStyleData(eStyleStruct_Display, ((const nsStyleStruct *&)display));

    // We only allow a single footer frame, and the footer frame must occur before
    // any body section row groups
    if ((NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP == display->mDisplay) &&
        !aReflowState.footerFrame && !aReflowState.firstBodySection) {
      aReflowState.footerFrame = frame;
    
    } else if ((NS_STYLE_DISPLAY_TABLE_ROW_GROUP == display->mDisplay) &&
               !aReflowState.firstBodySection) {
      aReflowState.firstBodySection = frame;
    }
    
    // See if this is the frame we're looking for
    if (frame == aKidFrame) {
      // If it's the footer, then keep going because the footer is at the
      // very bottom
      if (frame != aReflowState.footerFrame) {
        break;
      }
    }

    // Get the frame's height
    nsSize  kidSize;
    frame->GetSize(kidSize);
    
    // If our height is constrained then update the available height. Do
    // this for all frames including the footer frame
    if (PR_FALSE == aReflowState.unconstrainedHeight) {
      aReflowState.availSize.height -= kidSize.height;
    }

    // Update the running y-offset. Don't do this for the footer frame
    if (frame != aReflowState.footerFrame) {
      aReflowState.y += kidSize.height;
    }

    // Update the max element size
    //XXX: this should call into layout strategy to get the width field
    if (nsnull != aMaxElementSize) 
    {
      const nsStyleSpacing* tableSpacing;
      GetStyleData(eStyleStruct_Spacing , ((const nsStyleStruct *&)tableSpacing));
      nsMargin borderPadding;
      GetTableBorder (borderPadding); // gets the max border thickness for each edge
      nsMargin padding;
      tableSpacing->GetPadding(padding);
      borderPadding += padding;
      nscoord cellSpacing = GetCellSpacingX();
      nsSize  kidMaxElementSize;
      ((nsTableRowGroupFrame*)frame)->GetMaxElementSize(kidMaxElementSize);
      nscoord kidWidth = kidMaxElementSize.width + borderPadding.left + borderPadding.right + cellSpacing*2;
      aMaxElementSize->width = PR_MAX(aMaxElementSize->width, kidWidth); 
      aMaxElementSize->height += kidMaxElementSize.height;
    }
  }

  return NS_OK;
}

NS_METHOD nsTableFrame::IR_TargetIsChild(nsIPresContext&        aPresContext,
                                         nsHTMLReflowMetrics&   aDesiredSize,
                                         InnerTableReflowState& aReflowState,
                                         nsReflowStatus&        aStatus,
                                         nsIFrame *             aNextFrame)

{
  nsresult rv;
  // Recover the state as if aNextFrame is about to be reflowed
  RecoverState(aReflowState, aNextFrame, aDesiredSize.maxElementSize);

  // Remember the old rect
  nsRect  oldKidRect;
  aNextFrame->GetRect(oldKidRect);

  // Pass along the reflow command
  nsSize               kidMaxElementSize;
  nsHTMLReflowMetrics desiredSize(aDesiredSize.maxElementSize ? &kidMaxElementSize
                                                              : nsnull);
  nsHTMLReflowState kidReflowState(aPresContext, aReflowState.reflowState,
                                   aNextFrame, aReflowState.availSize);

  rv = ReflowChild(aNextFrame, aPresContext, desiredSize, kidReflowState, aStatus);

  // Place the row group frame. Don't use PlaceChild(), because it moves
  // the footer frame as well. We'll adjust the footer frame later on in
  // AdjustSiblingsAfterReflow()
  nscoord x = aReflowState.mBorderPadding.left;
  nscoord y = aReflowState.mBorderPadding.top + aReflowState.y;
  nsRect  kidRect(x, y, desiredSize.width, desiredSize.height);
  aNextFrame->SetRect(kidRect);

  // Adjust the running y-offset
  aReflowState.y += kidRect.height;

  // If our height is constrained, then update the available height
  if (PR_FALSE == aReflowState.unconstrainedHeight) {
    aReflowState.availSize.height -= kidRect.height;
  }

  // Update the max element size
  //XXX: this should call into layout strategy to get the width field
  if (nsnull != aDesiredSize.maxElementSize) 
  {
    const nsStyleSpacing* tableSpacing;
    GetStyleData(eStyleStruct_Spacing , ((const nsStyleStruct *&)tableSpacing));
    nsMargin borderPadding;
    GetTableBorder (borderPadding); // gets the max border thickness for each edge
    nsMargin padding;
    tableSpacing->GetPadding(padding);
    borderPadding += padding;
    nscoord cellSpacing = GetCellSpacingX();
    nscoord kidWidth = kidMaxElementSize.width + borderPadding.left + borderPadding.right + cellSpacing*2;
    aDesiredSize.maxElementSize->width = PR_MAX(aDesiredSize.maxElementSize->width, kidWidth); 
    aDesiredSize.maxElementSize->height += kidMaxElementSize.height;
  }

  // If the column width info is valid, then adjust the row group frames
  // that follow. Otherwise, return and we'll recompute the column widths
  // and reflow all the row group frames
  if (!NeedsReflow(aReflowState.reflowState)) {
    // If the row group frame changed height, then damage the horizontal strip
    // that was either added or went away
    if (desiredSize.height != oldKidRect.height) {
      nsRect  dirtyRect;

      dirtyRect.x = 0;
      dirtyRect.y = PR_MIN(oldKidRect.YMost(), kidRect.YMost());
      dirtyRect.width = mRect.width;
      dirtyRect.height = PR_MAX(oldKidRect.YMost(), kidRect.YMost()) -
                         dirtyRect.y;
      Invalidate(dirtyRect);
    }

    // Adjust the row groups that follow
    AdjustSiblingsAfterReflow(aPresContext, aReflowState, aNextFrame,
                              aDesiredSize.maxElementSize, desiredSize.height -
                              oldKidRect.height);

    // Return our size and our status
    aDesiredSize.width = ComputeDesiredWidth(aReflowState.reflowState);
    nscoord defaultHeight = aReflowState.y + aReflowState.mBorderPadding.top +
                            aReflowState.mBorderPadding.bottom;
    aDesiredSize.height = ComputeDesiredHeight(aPresContext, aReflowState.reflowState,
                                               defaultHeight);

    // XXX Is this needed?
#if 0
    AdjustForCollapsingRows(aPresContext, aDesiredSize.height);
    AdjustForCollapsingCols(aPresContext, aDesiredSize.width);

    // once horizontal borders are computed and all row heights are set, 
    // we need to fix up length of vertical edges
    // XXX need to figure start row and end row correctly
    if (NS_STYLE_BORDER_COLLAPSE==GetBorderCollapseStyle())
      DidComputeHorizontalCollapsingBorders(aPresContext, 0, 10000);
#endif
  }

  return rv;
}

nscoord nsTableFrame::ComputeDesiredWidth(const nsHTMLReflowState& aReflowState) const
{
  nscoord desiredWidth=aReflowState.availableWidth;
  // this is the biggest hack in the world.  But there's no other rational way to handle nested percent tables
  const nsStylePosition* position;
  PRBool isNested=IsNested(aReflowState, position);
  if((eReflowReason_Initial==aReflowState.reason) && 
     (PR_TRUE==isNested) && (eStyleUnit_Percent==position->mWidth.GetUnit()))
  {
    nsITableLayoutStrategy* tableLayoutStrategy = mTableLayoutStrategy;
    if (mPrevInFlow) {
      // Get the table layout strategy from the first-in-flow
      nsTableFrame* table = (nsTableFrame*)GetFirstInFlow();
      tableLayoutStrategy = table->mTableLayoutStrategy;
    }
    desiredWidth = tableLayoutStrategy->GetTableMaxContentWidth();
  }
  return desiredWidth;
}

// Position and size aKidFrame and update our reflow state. The origin of
// aKidRect is relative to the upper-left origin of our frame
void nsTableFrame::PlaceChild(nsIPresContext&    aPresContext,
                              InnerTableReflowState& aReflowState,
                              nsIFrame*          aKidFrame,
                              const nsRect&      aKidRect,
                              nsSize*            aMaxElementSize,
                              nsSize&            aKidMaxElementSize)
{
  // Place and size the child
  aKidFrame->SetRect(aKidRect);

  // Adjust the running y-offset
  aReflowState.y += aKidRect.height;

  // If our height is constrained, then update the available height
  if (PR_FALSE == aReflowState.unconstrainedHeight) {
    aReflowState.availSize.height -= aKidRect.height;
  }

  // If this is a footer row group, remember it
  const nsStyleDisplay *childDisplay;
  aKidFrame->GetStyleData(eStyleStruct_Display, ((const nsStyleStruct *&)childDisplay));

  // We only allow a single footer frame, and the footer frame must occur before
  // any body section row groups
  if ((NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP == childDisplay->mDisplay) &&
      !aReflowState.footerFrame && !aReflowState.firstBodySection)
  {
    aReflowState.footerFrame = aKidFrame;
  }
  else if (aReflowState.footerFrame)
  {
    // Place the row group frame
    nsSize  footerSize;
    nsPoint origin;
    aKidFrame->GetOrigin(origin);
    aReflowState.footerFrame->GetSize(footerSize);
    origin.y -= footerSize.height;
    aKidFrame->MoveTo(origin.x, origin.y);
    
    // Move the footer below the body row group frame
    aReflowState.footerFrame->GetOrigin(origin);
    origin.y += aKidRect.height;
    aReflowState.footerFrame->MoveTo(origin.x, origin.y);
  }

  //XXX: this should call into layout strategy to get the width field
  if (nsnull != aMaxElementSize) 
  {
    const nsStyleSpacing* tableSpacing;
    GetStyleData(eStyleStruct_Spacing , ((const nsStyleStruct *&)tableSpacing));
    nsMargin borderPadding;
    GetTableBorder (borderPadding); // gets the max border thickness for each edge
    nsMargin padding;
    tableSpacing->GetPadding(padding);
    borderPadding += padding;
    nscoord cellSpacing = GetCellSpacingX();
    nscoord kidWidth = aKidMaxElementSize.width + borderPadding.left + borderPadding.right + cellSpacing*2;
    aMaxElementSize->width = PR_MAX(aMaxElementSize->width, kidWidth); 
    aMaxElementSize->height += aKidMaxElementSize.height;
  }
}

/**
 * Reflow the frames we've already created
 *
 * @param   aPresContext presentation context to use
 * @param   aReflowState current inline state
 * @return  true if we successfully reflowed all the mapped children and false
 *            otherwise, e.g. we pushed children to the next in flow
 */
NS_METHOD nsTableFrame::ReflowMappedChildren(nsIPresContext& aPresContext,
                                             nsHTMLReflowMetrics& aDesiredSize,
                                             InnerTableReflowState& aReflowState,
                                             nsReflowStatus& aStatus)
{
  NS_PRECONDITION(mFrames.NotEmpty(), "no children");

  PRInt32   childCount = 0;
  nsIFrame* prevKidFrame = nsnull;
  nsSize    kidMaxElementSize(0,0);
  nsSize*   pKidMaxElementSize = (nsnull != aDesiredSize.maxElementSize) ? &kidMaxElementSize : nsnull;
  nsresult  rv = NS_OK;

  nsReflowReason reason;
  if (PR_FALSE==RequiresPass1Layout())
  {
    reason = aReflowState.reflowState.reason;
    if (eReflowReason_Incremental==reason) {
      reason = eReflowReason_Resize;
      if (aDesiredSize.maxElementSize) {
        aDesiredSize.maxElementSize->width = 0;
        aDesiredSize.maxElementSize->height = 0;
      }
    }
  }
  else
    reason = eReflowReason_Resize;

  // this never passes reflows down to colgroups
  for (nsIFrame* kidFrame = mFrames.FirstChild(); nsnull != kidFrame; ) 
  {
    nsSize              kidAvailSize(aReflowState.availSize);
    nsHTMLReflowMetrics desiredSize(pKidMaxElementSize);
    desiredSize.width=desiredSize.height=desiredSize.ascent=desiredSize.descent=0;

    const nsStyleDisplay *childDisplay;
    kidFrame->GetStyleData(eStyleStruct_Display, ((const nsStyleStruct *&)childDisplay));
    if (PR_TRUE==IsRowGroup(childDisplay->mDisplay))
    {
      // Keep track of the first body section row group
      if (nsnull == aReflowState.firstBodySection) {
        if (NS_STYLE_DISPLAY_TABLE_ROW_GROUP == childDisplay->mDisplay) {
          aReflowState.firstBodySection = kidFrame;
        }
      }

      nsMargin borderPadding;
      GetTableBorderForRowGroup(GetRowGroupFrameFor(kidFrame, childDisplay), borderPadding);
      const nsStyleSpacing* tableSpacing;
      GetStyleData(eStyleStruct_Spacing, ((const nsStyleStruct *&)tableSpacing));
      nsMargin padding;
      tableSpacing->GetPadding(padding);
      borderPadding += padding;

      // Reflow the child into the available space
      nsHTMLReflowState  kidReflowState(aPresContext, aReflowState.reflowState,
                                        kidFrame, kidAvailSize, reason);
      if (aReflowState.firstBodySection && (kidFrame != aReflowState.firstBodySection)) {
        // If this isn't the first row group frame or the header or footer, then
        // we can't be at the top of the page anymore...
        kidReflowState.isTopOfPage = PR_FALSE;
      }

      nscoord x = borderPadding.left;
      nscoord y = borderPadding.top + aReflowState.y;
      
      if (RowGroupsShouldBeConstrained()) {
        // Only applies to the tree widget.
        nscoord tableSpecifiedHeight;
        GetTableSpecifiedHeight(tableSpecifiedHeight, aReflowState.reflowState);
        if (tableSpecifiedHeight != -1) {
          kidReflowState.availableHeight = tableSpecifiedHeight - y;
          if (kidReflowState.availableHeight < 0)
            kidReflowState.availableHeight = 0;
        }
      }

      rv = ReflowChild(kidFrame, aPresContext, desiredSize, kidReflowState, aStatus);
      // Did the child fit?
      if (desiredSize.height > kidAvailSize.height) {
        if (aReflowState.firstBodySection && (kidFrame != aReflowState.firstBodySection)) {
          // The child is too tall to fit at all in the available space, and it's
          // not a header/footer or our first row group frame
          PushChildren(kidFrame, prevKidFrame);
          aStatus = NS_FRAME_NOT_COMPLETE;
          break;
        }
      }

      // Place the child
      nsRect kidRect (x, y, desiredSize.width, desiredSize.height);
      if (PR_TRUE==IsRowGroup(childDisplay->mDisplay))
      {
        // we don't want to adjust the maxElementSize if this is an initial reflow
        // it was set by the TableLayoutStrategy and shouldn't be changed.
        nsSize *requestedMaxElementSize = nsnull;
        if (eReflowReason_Initial != aReflowState.reflowState.reason)
          requestedMaxElementSize = aDesiredSize.maxElementSize;
        PlaceChild(aPresContext, aReflowState, kidFrame, kidRect,
                   requestedMaxElementSize, kidMaxElementSize);
      }
      childCount++;

      // Remember where we just were in case we end up pushing children
      prevKidFrame = kidFrame;

      // Special handling for incomplete children
      if (NS_FRAME_IS_NOT_COMPLETE(aStatus)) {
        nsIFrame* kidNextInFlow;
         
        kidFrame->GetNextInFlow(&kidNextInFlow);
        if (nsnull == kidNextInFlow) {
          // The child doesn't have a next-in-flow so create a continuing
          // frame. This hooks the child into the flow
          nsIFrame*     continuingFrame;
          nsIPresShell* presShell;
          nsIStyleSet*  styleSet;

          aPresContext.GetShell(&presShell);
          presShell->GetStyleSet(&styleSet);
          NS_RELEASE(presShell);
          styleSet->CreateContinuingFrame(&aPresContext, kidFrame, this, &continuingFrame);
          NS_RELEASE(styleSet);

          // Add the continuing frame to the sibling list
          nsIFrame* nextSib;
           
          kidFrame->GetNextSibling(&nextSib);
          continuingFrame->SetNextSibling(nextSib);
          kidFrame->SetNextSibling(continuingFrame);
        }
        // We've used up all of our available space so push the remaining
        // children to the next-in-flow
        nsIFrame* nextSibling;
         
        kidFrame->GetNextSibling(&nextSibling);
        if (nsnull != nextSibling) {
          PushChildren(nextSibling, kidFrame);
        }
        break;
      }
    }
    else
    {// it's an unknown frame type, give it a generic reflow and ignore the results
        nsHTMLReflowState kidReflowState(aPresContext,
                                         aReflowState.reflowState, kidFrame,
                                         nsSize(0,0), eReflowReason_Resize);
        nsHTMLReflowMetrics unusedDesiredSize(nsnull);
        ReflowChild(kidFrame, aPresContext, unusedDesiredSize, kidReflowState, aStatus);
    }

    // Get the next child
    kidFrame->GetNextSibling(&kidFrame);
  }

  // Update the child count
  return rv;
}

/**
 * Try and pull-up frames from our next-in-flow
 *
 * @param   aPresContext presentation context to use
 * @param   aReflowState current inline state
 * @return  true if we successfully pulled-up all the children and false
 *            otherwise, e.g. child didn't fit
 */
NS_METHOD nsTableFrame::PullUpChildren(nsIPresContext& aPresContext,
                                       nsHTMLReflowMetrics& aDesiredSize,
                                       InnerTableReflowState& aReflowState,
                                       nsReflowStatus& aStatus)
{
  nsTableFrame*  nextInFlow = (nsTableFrame*)mNextInFlow;
  nsSize         kidMaxElementSize(0,0);
  nsSize*        pKidMaxElementSize = (nsnull != aDesiredSize.maxElementSize) ? &kidMaxElementSize : nsnull;
  nsIFrame*      prevKidFrame = mFrames.LastChild();
  nsresult       rv = NS_OK;

  while (nsnull != nextInFlow) {
    nsHTMLReflowMetrics kidSize(pKidMaxElementSize);
    kidSize.width=kidSize.height=kidSize.ascent=kidSize.descent=0;

    // XXX change to use nsFrameList::PullFrame

    // Get the next child
    nsIFrame* kidFrame = nextInFlow->mFrames.FirstChild();

    // Any more child frames?
    if (nsnull == kidFrame) {
      // No. Any frames on its overflow list?
      if (nextInFlow->mOverflowFrames.NotEmpty()) {
        // Move the overflow list to become the child list
        nextInFlow->mFrames.AppendFrames(nsnull, nextInFlow->mOverflowFrames);
        kidFrame = nextInFlow->mFrames.FirstChild();
      } else {
        // We've pulled up all the children, so move to the next-in-flow.
        nextInFlow->GetNextInFlow((nsIFrame**)&nextInFlow);
        continue;
      }
    }

    // See if the child fits in the available space. If it fits or
    // it's splittable then reflow it. The reason we can't just move
    // it is that we still need ascent/descent information
    nsSize            kidFrameSize(0,0);
    nsSplittableType  kidIsSplittable;

    kidFrame->GetSize(kidFrameSize);
    kidFrame->IsSplittable(kidIsSplittable);
    if ((kidFrameSize.height > aReflowState.availSize.height) &&
        NS_FRAME_IS_NOT_SPLITTABLE(kidIsSplittable)) {
      //XXX: Troy
      aStatus = NS_FRAME_NOT_COMPLETE;
      break;
    }
    nsHTMLReflowState  kidReflowState(aPresContext, aReflowState.reflowState,
                                      kidFrame, aReflowState.availSize,
                                      eReflowReason_Resize);

    rv = ReflowChild(kidFrame, aPresContext, kidSize, kidReflowState, aStatus);

    // Did the child fit?
    if ((kidSize.height > aReflowState.availSize.height) && mFrames.NotEmpty()) {
      // The child is too wide to fit in the available space, and it's
      // not our first child
      //XXX: Troy
      aStatus = NS_FRAME_NOT_COMPLETE;
      break;
    }

    nsRect kidRect (0, 0, kidSize.width, kidSize.height);
    kidRect.y += aReflowState.y;
    const nsStyleDisplay *childDisplay;
    kidFrame->GetStyleData(eStyleStruct_Display, ((const nsStyleStruct *&)childDisplay));
    if (PR_TRUE==IsRowGroup(childDisplay->mDisplay))
    {
      PlaceChild(aPresContext, aReflowState, kidFrame, kidRect, aDesiredSize.maxElementSize, *pKidMaxElementSize);
    }

    // Remove the frame from its current parent
    nextInFlow->mFrames.RemoveFirstChild();

    // Link the frame into our list of children
    kidFrame->SetParent(this);

    if (nsnull == prevKidFrame) {
      mFrames.SetFrames(kidFrame);
    } else {
      prevKidFrame->SetNextSibling(kidFrame);
    }
    kidFrame->SetNextSibling(nsnull);

    // Remember where we just were in case we end up pushing children
    prevKidFrame = kidFrame;
    if (NS_FRAME_IS_NOT_COMPLETE(aStatus)) {
      // No the child isn't complete
      nsIFrame* kidNextInFlow;
       
      kidFrame->GetNextInFlow(&kidNextInFlow);
      if (nsnull == kidNextInFlow) {
        // The child doesn't have a next-in-flow so create a
        // continuing frame. The creation appends it to the flow and
        // prepares it for reflow.
        nsIFrame*     continuingFrame;
        nsIPresShell* presShell;
        nsIStyleSet*  styleSet;
        aPresContext.GetShell(&presShell);
        presShell->GetStyleSet(&styleSet);
        NS_RELEASE(presShell);
        styleSet->CreateContinuingFrame(&aPresContext, kidFrame, this, &continuingFrame);
        NS_RELEASE(styleSet);

        // Add the continuing frame to our sibling list and then push
        // it to the next-in-flow. This ensures the next-in-flow's
        // content offsets and child count are set properly. Note that
        // we can safely assume that the continuation is complete so
        // we pass PR_TRUE into PushChidren
        kidFrame->SetNextSibling(continuingFrame);

        PushChildren(continuingFrame, kidFrame);
      }
      break;
    }
  }

  return rv;
}

/**
  Now I've got all the cells laid out in an infinite space.
  For each column, use the min size for each cell in that column
  along with the attributes of the table, column group, and column
  to assign widths to each column.
  */
// use the cell map to determine which cell is in which column.
void nsTableFrame::BalanceColumnWidths(nsIPresContext& aPresContext, 
                                       const nsHTMLReflowState& aReflowState,
                                       const nsSize& aMaxSize, 
                                       nsSize* aMaxElementSize)
{
  NS_ASSERTION(nsnull==mPrevInFlow, "never ever call me on a continuing frame!");
  NS_ASSERTION(nsnull!=mCellMap, "never ever call me until the cell map is built!");

  PRInt32 numCols = mCellMap->GetColCount();
  if (numCols>mColumnWidthsLength)
  {
    PRInt32 priorColumnWidthsLength=mColumnWidthsLength;
    if (0 == priorColumnWidthsLength) {
      mColumnWidthsLength = numCols;
    } else {
      while (numCols>mColumnWidthsLength)
        mColumnWidthsLength += kColumnWidthIncrement;
    }
    PRInt32 * newColumnWidthsArray = new PRInt32[mColumnWidthsLength];
    nsCRT::memset (newColumnWidthsArray, 0, mColumnWidthsLength*sizeof(PRInt32));
    if (mColumnWidths) {
      nsCRT::memcpy (newColumnWidthsArray, mColumnWidths, priorColumnWidthsLength*sizeof(PRInt32));
      delete [] mColumnWidths;
    }
    mColumnWidths = newColumnWidthsArray;
   }

  // need to figure out the overall table width constraint
  // default case, get 100% of available space

  PRInt32 maxWidth = aMaxSize.width;
  const nsStylePosition* position =
    (const nsStylePosition*)mStyleContext->GetStyleData(eStyleStruct_Position);
  if (eStyleUnit_Coord==position->mWidth.GetUnit()) 
  {
    nscoord coordWidth=0;
    coordWidth = position->mWidth.GetCoordValue();
    // NAV4 compatibility:  0-coord-width == auto-width
    if (0!=coordWidth)
      maxWidth = coordWidth;
  }

  if (0>maxWidth)  // nonsense style specification
    maxWidth = 0;

  // based on the compatibility mode, create a table layout strategy
  if (nsnull == mTableLayoutStrategy) {
    nsCompatibility mode;
    aPresContext.GetCompatibilityMode(&mode);
    if (PR_FALSE==RequiresPass1Layout())
      mTableLayoutStrategy = new FixedTableLayoutStrategy(this);
    else
      mTableLayoutStrategy = new BasicTableLayoutStrategy(this, eCompatibility_NavQuirks == mode);
    mTableLayoutStrategy->Initialize(aMaxElementSize, GetColCount(), aReflowState.mComputedWidth);
    mBits.mColumnWidthsValid=PR_TRUE;
  }
  // fixed-layout tables need to reinitialize the layout strategy. When there are scroll bars
  // reflow gets called twice and the 2nd time has the correct space available.
  else if (!RequiresPass1Layout()) {
    mTableLayoutStrategy->Initialize(aMaxElementSize, GetColCount(), aReflowState.mComputedWidth);
  }

  mTableLayoutStrategy->BalanceColumnWidths(mStyleContext, aReflowState, maxWidth);
  //Dump(PR_TRUE, PR_TRUE);
  mBits.mColumnWidthsSet=PR_TRUE;

  // if collapsing borders, compute the top and bottom edges now that we have column widths
  if (NS_STYLE_BORDER_COLLAPSE == GetBorderCollapseStyle())
  {
    ComputeHorizontalCollapsingBorders(aPresContext, 0, mCellMap->GetRowCount()-1);
  }
}

/**
  sum the width of each column
  add in table insets
  set rect
  */
void nsTableFrame::SetTableWidth(nsIPresContext& aPresContext)
{
  NS_ASSERTION(nsnull==mPrevInFlow, "never ever call me on a continuing frame!");
  NS_ASSERTION(nsnull!=mCellMap, "never ever call me until the cell map is built!");

  nscoord cellSpacing = GetCellSpacingX();
  PRInt32 tableWidth = 0;

  PRInt32 numCols = GetColCount();
  for (PRInt32 colIndex = 0; colIndex < numCols; colIndex++) {
    nscoord totalColWidth = mColumnWidths[colIndex];
    if (GetNumCellsOriginatingInCol(colIndex) > 0) { // skip degenerate cols
      totalColWidth += cellSpacing;           // add cell spacing to left of col
    }
    tableWidth += totalColWidth;
  }

  if (numCols > 0) {
    tableWidth += cellSpacing; // add last cellspacing
  } 
  else if (0 == tableWidth)  {
    nsRect tableRect = mRect;
    tableRect.width = 0;
    SetRect(tableRect);
    return;
  }

  // Compute the insets (sum of border and padding)
  const nsStyleSpacing* spacing =
    (const nsStyleSpacing*)mStyleContext->GetStyleData(eStyleStruct_Spacing);
  nsMargin borderPadding;
  GetTableBorder (borderPadding); // this gets the max border value at every edge
  nsMargin padding;
  spacing->GetPadding(padding);
  borderPadding += padding;

  nscoord rightInset = borderPadding.right;
  nscoord leftInset = borderPadding.left;
  tableWidth += (leftInset + rightInset);
  nsRect tableSize = mRect;
  tableSize.width = tableWidth;
    
  // account for scroll bars. XXX needs optimization/caching
  if (mBits.mHasScrollableRowGroup) {
    float sbWidth, sbHeight;
    nsCOMPtr<nsIDeviceContext> dc;
    aPresContext.GetDeviceContext(getter_AddRefs(dc));
    dc->GetScrollBarDimensions(sbWidth, sbHeight);
    tableSize.width += NSToCoordRound(sbWidth);
  }
  SetRect(tableSize);
}

// XXX percentage based margin/border/padding
nscoord GetVerticalMarginBorderPadding(nsIFrame* aFrame, const nsIID& aIID)
{
  nscoord result = 0;
  if (!aFrame) {
    return result;
  }
  nsCOMPtr<nsIContent> iContent;
  nsresult rv = aFrame->GetContent(getter_AddRefs(iContent));
  if (NS_SUCCEEDED(rv)) {
    nsIHTMLContent* htmlContent = nsnull;
    rv = iContent->QueryInterface(aIID, (void **)&htmlContent);  
    if (htmlContent && NS_SUCCEEDED(rv)) { 
      nsIStyleContext* styleContext;
      aFrame->GetStyleContext(&styleContext);
      const nsStyleSpacing* spacing =
        (const nsStyleSpacing*)styleContext->GetStyleData(eStyleStruct_Spacing);
	    nsMargin margin(0,0,0,0);
      if (spacing->GetMargin(margin)) {
        result += margin.top + margin.bottom;
      }
      if (spacing->GetBorderPadding(margin)) {
        result += margin.top + margin.bottom;
      }
      NS_RELEASE(htmlContent);
    }
  }
  return result;
}

/* Get the height of the nearest ancestor of this table which has a height other than
 * auto, except when there is an ancestor which is a table and that table does not have
 * a coord height. It can be the case that the nearest such ancestor is a scroll frame
 * or viewport frame; this provides backwards compatibility with Nav4.X and IE.
 */
nscoord nsTableFrame::GetEffectiveContainerHeight(const nsHTMLReflowState& aReflowState)
{
  nsIFrame* lastArea  = nsnull;
  nsIFrame* lastBlock = nsnull;
  nsIAtom*  frameType = nsnull;
  nscoord result = -1;
  const nsHTMLReflowState* rs = &aReflowState;

  while (rs) {
    const nsStyleDisplay* display;
    rs->frame->GetStyleData(eStyleStruct_Display, (const nsStyleStruct *&)display);
    if (NS_STYLE_DISPLAY_TABLE == display->mDisplay) {
      const nsStylePosition* position;
      rs->frame->GetStyleData(eStyleStruct_Position, (const nsStyleStruct *&)position);
      nsStyleUnit unit = position->mHeight.GetUnit();
      if ((eStyleUnit_Null == unit) || (eStyleUnit_Auto == unit)) {
        result = 0;
        break;
      }
    }
    if (NS_AUTOHEIGHT != rs->mComputedHeight) {
      result = rs->mComputedHeight;
      // if we get to the scroll frame or viewport frame, then subtract out 
      // margin/border/padding for the HTML and BODY elements
      rs->frame->GetFrameType(&frameType);
      if ((nsLayoutAtoms::viewportFrame == frameType) || 
          (nsLayoutAtoms::scrollFrame   == frameType)) {
        result -= GetVerticalMarginBorderPadding(lastArea,  kIHTMLElementIID); 
        result -= GetVerticalMarginBorderPadding(lastBlock, kIBodyElementIID);
      }
      NS_IF_RELEASE(frameType);
      break;
    }
    // keep track of the area and block frame on the way up because they could
    // be the HTML and BODY elements
    rs->frame->GetFrameType(&frameType);
    if (nsLayoutAtoms::areaFrame == frameType) {
      lastArea = rs->frame;
    } 
    else if (nsLayoutAtoms::blockFrame == frameType) {
      lastBlock = rs->frame;
    }
    NS_IF_RELEASE(frameType);
    
    // XXX: evil cast!
    rs = (nsHTMLReflowState *)(rs->parentReflowState);
  }
  NS_ASSERTION(-1 != result, "bad state:  no constrained height in reflow chain");
  return result;
}

/**
  get the table height attribute
  if it is auto, table height = SUM(height of rowgroups)
  else if (resolved table height attribute > SUM(height of rowgroups))
    proportionately distribute extra height to each row
  we assume we are passed in the default table height==the sum of the heights of the table's rowgroups
  in aDesiredSize.height.
  */
void nsTableFrame::DistributeSpaceToCells(nsIPresContext& aPresContext, 
                                   const nsHTMLReflowState& aReflowState,
                                   nsIFrame* aRowGroupFrame)
{
  // now that all of the rows have been resized, resize the cells       
  nsTableRowGroupFrame* rowGroupFrame = (nsTableRowGroupFrame*)aRowGroupFrame;
  nsIFrame * rowFrame = rowGroupFrame->GetFirstFrame();
  while (nsnull!=rowFrame) {
    const nsStyleDisplay *rowDisplay;
    rowFrame->GetStyleData(eStyleStruct_Display, ((const nsStyleStruct *&)rowDisplay));
    if (NS_STYLE_DISPLAY_TABLE_ROW == rowDisplay->mDisplay) { 
      ((nsTableRowFrame *)rowFrame)->DidResize(aPresContext, aReflowState);
    }
    // The calling function, DistributeSpaceToRows, takes care of the recursive row
    // group descent, which is why there's no NS_STYLE_DISPLAY_TABLE_ROW_GROUP case
    // here.
    rowGroupFrame->GetNextFrame(rowFrame, &rowFrame);
  }
}

void nsTableFrame::DistributeSpaceToRows(nsIPresContext& aPresContext,
                                  const nsHTMLReflowState& aReflowState,
                                  nsIFrame* aRowGroupFrame, const nscoord& aSumOfRowHeights,
                                  const nscoord& aExcess, const nsStyleTable* aTableStyle, 
                                  nscoord& aExcessForRowGroup, 
                                  nscoord& aRowGroupYPos)
{
  // the rows in rowGroupFrame need to be expanded by rowHeightDelta[i]
  // and the rowgroup itself needs to be expanded by SUM(row height deltas)
  nsTableRowGroupFrame* rowGroupFrame = (nsTableRowGroupFrame*)aRowGroupFrame;
  nsIFrame * rowFrame = rowGroupFrame->GetFirstFrame();
  nscoord y = 0;
  while (nsnull!=rowFrame)
  {
    const nsStyleDisplay *rowDisplay;
    rowFrame->GetStyleData(eStyleStruct_Display, ((const nsStyleStruct *&)rowDisplay));
    if (NS_STYLE_DISPLAY_TABLE_ROW_GROUP == rowDisplay->mDisplay) {
      DistributeSpaceToRows(aPresContext, aReflowState, rowFrame, aSumOfRowHeights, 
                            aExcess, aTableStyle, aExcessForRowGroup, y);
    }
    else if (NS_STYLE_DISPLAY_TABLE_ROW == rowDisplay->mDisplay)
    { // the row needs to be expanded by the proportion this row contributed to the original height
      nsRect rowRect;
      rowFrame->GetRect(rowRect);
      float percent = ((float)(rowRect.height)) / ((float)(aSumOfRowHeights));
      nscoord excessForRow = NSToCoordRound((float)aExcess*percent);

      if (rowGroupFrame->RowsDesireExcessSpace()) {
        nsRect newRowRect(rowRect.x, y, rowRect.width, excessForRow+rowRect.height);
        rowFrame->SetRect(newRowRect);
        if (NS_STYLE_BORDER_COLLAPSE == GetBorderCollapseStyle())
        {
          NS_PRECONDITION(mBorderEdges, "haven't allocated border edges struct");
          nsBorderEdge *border = (nsBorderEdge *)
            (mBorderEdges->mEdges[NS_SIDE_LEFT].ElementAt(((nsTableRowFrame*)rowFrame)->GetRowIndex()));
          if (border)
            border->mLength=newRowRect.height;
          border = (nsBorderEdge *)
            (mBorderEdges->mEdges[NS_SIDE_RIGHT].ElementAt(((nsTableRowFrame*)rowFrame)->GetRowIndex()));
          if (border) 
            border->mLength=newRowRect.height;
        }
        // better if this were part of an overloaded row::SetRect
        y += excessForRow+rowRect.height;
      }

      aExcessForRowGroup += excessForRow;
    }
    else
    {
      nsRect rowRect;
      rowFrame->GetRect(rowRect);
      y += rowRect.height;
    }

    rowGroupFrame->GetNextFrame(rowFrame, &rowFrame);
  }

  nsRect rowGroupRect;
  aRowGroupFrame->GetRect(rowGroupRect);  
  if (rowGroupFrame->RowGroupDesiresExcessSpace()) {
    nsRect newRowGroupRect(rowGroupRect.x, aRowGroupYPos, rowGroupRect.width, aExcessForRowGroup+rowGroupRect.height);
    aRowGroupFrame->SetRect(newRowGroupRect);
    aRowGroupYPos += aExcessForRowGroup + rowGroupRect.height;
  }
  else aRowGroupYPos += rowGroupRect.height;

  DistributeSpaceToCells(aPresContext, aReflowState, aRowGroupFrame);
}

NS_IMETHODIMP nsTableFrame::GetTableSpecifiedHeight(nscoord&                 aResult, 
                                                    const nsHTMLReflowState& aReflowState)
{
  const nsStylePosition* tablePosition;
  GetStyleData(eStyleStruct_Position, (const nsStyleStruct *&)tablePosition);
  
  const nsStyleTable* tableStyle;
  GetStyleData(eStyleStruct_Table, (const nsStyleStruct *&)tableStyle);
  nscoord tableSpecifiedHeight = -1;
  if (aReflowState.mComputedHeight != NS_UNCONSTRAINEDSIZE &&
      aReflowState.mComputedHeight > 0)
    tableSpecifiedHeight = aReflowState.mComputedHeight;
  else if (eStyleUnit_Coord == tablePosition->mHeight.GetUnit())
    tableSpecifiedHeight = tablePosition->mHeight.GetCoordValue();
  else if (eStyleUnit_Percent == tablePosition->mHeight.GetUnit()) {
    float percent = tablePosition->mHeight.GetPercentValue();
    nscoord parentHeight = GetEffectiveContainerHeight(aReflowState);
    if ((NS_UNCONSTRAINEDSIZE != parentHeight) && (0 != parentHeight))
      tableSpecifiedHeight = NSToCoordRound((float)parentHeight * percent);
  }
  aResult = tableSpecifiedHeight;
  return NS_OK;
}

nscoord nsTableFrame::ComputeDesiredHeight(nsIPresContext&          aPresContext,
                                           const nsHTMLReflowState& aReflowState, 
                                           nscoord                  aDefaultHeight) 
{
  NS_ASSERTION(mCellMap, "never ever call me until the cell map is built!");
  nscoord result = aDefaultHeight;

  nscoord tableSpecifiedHeight;
  GetTableSpecifiedHeight(tableSpecifiedHeight, aReflowState);
  if (-1 != tableSpecifiedHeight) {
    if (tableSpecifiedHeight > aDefaultHeight) { 
      // proportionately distribute the excess height to each row
      result = tableSpecifiedHeight;
      nscoord excess = tableSpecifiedHeight - aDefaultHeight;
      nscoord sumOfRowHeights = 0;
      nsIFrame* rowGroupFrame=mFrames.FirstChild();
      while (nsnull != rowGroupFrame) {
        const nsStyleDisplay *rowGroupDisplay;
        rowGroupFrame->GetStyleData(eStyleStruct_Display, ((const nsStyleStruct *&)rowGroupDisplay));
        if (PR_TRUE==IsRowGroup(rowGroupDisplay->mDisplay) &&
            ((nsTableRowGroupFrame*)rowGroupFrame)->RowGroupReceivesExcessSpace()) { 
          ((nsTableRowGroupFrame*)rowGroupFrame)->GetHeightOfRows(sumOfRowHeights);
        }
        rowGroupFrame->GetNextSibling(&rowGroupFrame);
      }
      rowGroupFrame=mFrames.FirstChild();
      // the first row group's y position starts inside our padding
      nscoord rowGroupYPos = 0;
      if (rowGroupFrame) {
        const nsStyleSpacing* spacing =
          (const nsStyleSpacing*)mStyleContext->GetStyleData(eStyleStruct_Spacing);
	      nsMargin margin(0,0,0,0);
        if (spacing->GetBorder(margin)) { // XXX see bug 10636 and handle percentages
          rowGroupYPos = margin.top;
        }
        if (spacing->GetPadding(margin)) { // XXX see bug 10636 and handle percentages
          rowGroupYPos += margin.top;
        }
      }
      while (nsnull!=rowGroupFrame) {
        const nsStyleDisplay *rowGroupDisplay;
        rowGroupFrame->GetStyleData(eStyleStruct_Display, ((const nsStyleStruct *&)rowGroupDisplay));
        if (PR_TRUE==IsRowGroup(rowGroupDisplay->mDisplay)) {
          if (((nsTableRowGroupFrame*)rowGroupFrame)->RowGroupReceivesExcessSpace()) {
            nscoord excessForGroup = 0;
            const nsStyleTable* tableStyle;
            GetStyleData(eStyleStruct_Table, (const nsStyleStruct *&)tableStyle);
            DistributeSpaceToRows(aPresContext, aReflowState, rowGroupFrame, sumOfRowHeights, 
                                  excess, tableStyle, excessForGroup, rowGroupYPos);
          }
          else {
            nsRect rowGroupRect;
            rowGroupFrame->GetRect(rowGroupRect);
            rowGroupYPos += rowGroupRect.height;
          }
        }
        rowGroupFrame->GetNextSibling(&rowGroupFrame);
      }
    }
  }
  return result;
}

void nsTableFrame::AdjustColumnsForCOLSAttribute()
{
// XXX this is not right, 
#if 0
  nsCellMap *cellMap = GetCellMap();
  NS_ASSERTION(nsnull!=cellMap, "bad cell map");
  
  // any specified-width column turns off COLS attribute
  nsStyleTable* tableStyle = (nsStyleTable *)mStyleContext->GetMutableStyleData(eStyleStruct_Table);
  if (tableStyle->mCols != NS_STYLE_TABLE_COLS_NONE)
  {
    PRInt32 numCols = cellMap->GetColCount();
    PRInt32 numRows = cellMap->GetRowCount();
    for (PRInt32 rowIndex=0; rowIndex<numRows; rowIndex++)
    {
      for (PRInt32 colIndex=0; colIndex<numCols; colIndex++)
      {
        nsTableCellFrame *cellFrame = cellMap->GetCellFrameAt(rowIndex, colIndex);
        // get the cell style info
        const nsStylePosition* cellPosition;
        if (nsnull!=cellFrame)
        {
          cellFrame->GetStyleData(eStyleStruct_Position, (const nsStyleStruct *&)cellPosition);
          if ((eStyleUnit_Coord == cellPosition->mWidth.GetUnit()) ||
               (eStyleUnit_Percent==cellPosition->mWidth.GetUnit())) 
          {
            tableStyle->mCols = NS_STYLE_TABLE_COLS_NONE;
            break;
          }
        }
      }
    }
  }
#endif 
}

/*
  The rule is:  use whatever width is greatest among those specified widths
  that span a column.  It doesn't matter what comes first, just what is biggest.
  Specified widths (when colspan==1) and span widths need to be stored separately, 
  because specified widths tell us what proportion of the span width to give to each column 
  (in their absence, we use the desired width of the cell.)                                                                  
*/

// XXX this is being phased out. cells with colspans will potentially overwrite
// col info stored by cells that don't span just because they are a bigger value. 
// The fixed width column info should be gotten from nsTableColFrame::GetFixedWidth
NS_METHOD
nsTableFrame::SetColumnStyleFromCell(nsIPresContext &  aPresContext,
                                     nsTableCellFrame* aCellFrame,
                                     nsTableRowFrame * aRowFrame)
{
#if 0
  // if the cell has a colspan, the width is used provisionally, divided equally among 
  // the spanned columns until the table layout strategy computes the real column width.
  if ((nsnull!=aCellFrame) && (nsnull!=aRowFrame))
  {
    // get the cell style info
    const nsStylePosition* cellPosition;
    aCellFrame->GetStyleData(eStyleStruct_Position, (const nsStyleStruct *&)cellPosition);
    if ((eStyleUnit_Coord == cellPosition->mWidth.GetUnit()) ||
         (eStyleUnit_Percent==cellPosition->mWidth.GetUnit())) {
      // compute the width per column spanned
      PRInt32 baseColIndex;
      aCellFrame->GetColIndex(baseColIndex);
      PRInt32 colSpan = GetEffectiveColSpan(baseColIndex, aCellFrame);
      for (PRInt32 i=0; i<colSpan; i++)
      {
        // get the appropriate column frame
        nsTableColFrame *colFrame;
        GetColumnFrame(i+baseColIndex, colFrame);
        // if the colspan is 1 and we already have a cell that set this column's width
        // then ignore this width attribute
        if ((1==colSpan) && (nsTableColFrame::eWIDTH_SOURCE_CELL == colFrame->GetWidthSource()))
        {
          break;
        }
        // get the column style
        nsIStyleContext *colSC;
        colFrame->GetStyleContext(&colSC);
        nsStylePosition* colPosition = (nsStylePosition*) colSC->GetMutableStyleData(eStyleStruct_Position);
        // if colSpan==1, then we can just set the column width
        if (1==colSpan)
        { // set the column width attribute
          if (eStyleUnit_Coord == cellPosition->mWidth.GetUnit())
          {
            nscoord width = cellPosition->mWidth.GetCoordValue();
            colPosition->mWidth.SetCoordValue(width);
          }
          else
          {
            float width = cellPosition->mWidth.GetPercentValue();
            colPosition->mWidth.SetPercentValue(width);
          }
          colFrame->SetWidthSource(nsTableColFrame::eWIDTH_SOURCE_CELL);
        }
        else  // we have a colspan > 1. so we need to set the column table style spanWidth
        { // if the cell is a coord width...
          nsStyleTable* colTableStyle = (nsStyleTable*) colSC->GetMutableStyleData(eStyleStruct_Table);
          if (eStyleUnit_Coord == cellPosition->mWidth.GetUnit())
          {
            // set the column width attribute iff this span's contribution to this column
            // is greater than any previous information
            nscoord cellWidth = cellPosition->mWidth.GetCoordValue();
            nscoord widthPerColumn = cellWidth/colSpan;
            nscoord widthForThisColumn = widthPerColumn;
            if (eStyleUnit_Coord == colTableStyle->mSpanWidth.GetUnit())
              widthForThisColumn = PR_MAX(widthForThisColumn, colTableStyle->mSpanWidth.GetCoordValue());
            colTableStyle->mSpanWidth.SetCoordValue(widthForThisColumn);
          }
          // else if the cell has a percent width...
          else if (eStyleUnit_Percent == cellPosition->mWidth.GetUnit())
          {
            // set the column width attribute iff this span's contribution to this column
            // is greater than any previous information
            float cellWidth = cellPosition->mWidth.GetPercentValue();
            float percentPerColumn = cellWidth/(float)colSpan;
            float percentForThisColumn = percentPerColumn;
            if (eStyleUnit_Percent == colTableStyle->mSpanWidth.GetUnit())
              percentForThisColumn = PR_MAX(percentForThisColumn, colTableStyle->mSpanWidth.GetPercentValue());
            colTableStyle->mSpanWidth.SetPercentValue(percentForThisColumn);
          }
          colFrame->SetWidthSource(nsTableColFrame::eWIDTH_SOURCE_CELL_WITH_SPAN);
        }
        NS_RELEASE(colSC);
      }
    }
  }
#endif
  return NS_OK;
}

/* there's an easy way and a hard way.  The easy way is to look in our
 * cache and pull the frame from there.
 * If the cache isn't built yet, then we have to go hunting.
 */
NS_METHOD nsTableFrame::GetColumnFrame(PRInt32 aColIndex, nsTableColFrame *&aColFrame)
{
  aColFrame = nsnull; // initialize out parameter
  nsCellMap *cellMap = GetCellMap();
  if (nsnull!=cellMap)
  { // hooray, we get to do this the easy way because the info is cached
    aColFrame = cellMap->GetColumnFrame(aColIndex);
  }
  else
  { // ah shucks, we have to go hunt for the column frame brute-force style
    nsIFrame *childFrame = mColGroups.FirstChild();
    for (;;)
    {
      if (nsnull==childFrame)
      {
        NS_ASSERTION (PR_FALSE, "scanned the frame hierarchy and no column frame could be found.");
        break;
      }
      PRInt32 colGroupStartingIndex = ((nsTableColGroupFrame *)childFrame)->GetStartColumnIndex();
      if (aColIndex >= colGroupStartingIndex)
      { // the cell's col might be in this col group
        PRInt32 colCount = ((nsTableColGroupFrame *)childFrame)->GetColumnCount();
        if (aColIndex < colGroupStartingIndex + colCount)
        { // yep, we've found it.  GetColumnAt gives us the column at the offset colCount, not the absolute colIndex for the whole table
          aColFrame = ((nsTableColGroupFrame *)childFrame)->GetColumnAt(colCount);
          break;
        }
      }
      childFrame->GetNextSibling(&childFrame);
    }
  }
  return NS_OK;
}

PRBool nsTableFrame::IsColumnWidthsSet()
{ 
  nsTableFrame * firstInFlow = (nsTableFrame *)GetFirstInFlow();
  NS_ASSERTION(nsnull!=firstInFlow, "illegal state -- no first in flow");
  return (PRBool)firstInFlow->mBits.mColumnWidthsSet; 
}

/* We have to go through our child list twice.
 * The first time, we scan until we find the first row.  
 * We set column style from the cells in the first row.
 * Then we terminate that loop and start a second pass.
 * In the second pass, we build column and cell cache info.
 */
void nsTableFrame::SetColumnStylesFromCells(nsIPresContext& aPresContext, nsIFrame* aRowGroupFrame)
{
  nsIFrame *rowFrame;
  aRowGroupFrame->FirstChild(nsnull, &rowFrame);
  while (nsnull!=rowFrame)
  {
    const nsStyleDisplay *rowDisplay;
    rowFrame->GetStyleData(eStyleStruct_Display, ((const nsStyleStruct *&)rowDisplay));
    if (NS_STYLE_DISPLAY_TABLE_ROW_GROUP == rowDisplay->mDisplay) {
      SetColumnStylesFromCells(aPresContext, rowFrame);
    }
    else if (NS_STYLE_DISPLAY_TABLE_ROW == rowDisplay->mDisplay)
    {
      nsIFrame *cellFrame;
      rowFrame->FirstChild(nsnull, &cellFrame);
      while (nsnull!=cellFrame)
      {
        /* this is the first time we are guaranteed to have both the cell frames
         * and the column frames, so it's a good time to 
         * set the column style from the cell's width attribute (if this is the first row)
         */
        const nsStyleDisplay *cellDisplay;
        cellFrame->GetStyleData(eStyleStruct_Display, ((const nsStyleStruct *&)cellDisplay));
        if (NS_STYLE_DISPLAY_TABLE_CELL == cellDisplay->mDisplay)
          SetColumnStyleFromCell(aPresContext, (nsTableCellFrame *)cellFrame, (nsTableRowFrame *)rowFrame);
        cellFrame->GetNextSibling(&cellFrame);
      }
    }
    rowFrame->GetNextSibling(&rowFrame);
  }
}

void nsTableFrame::BuildColumnCache( nsIPresContext&          aPresContext,
                                     nsHTMLReflowMetrics&     aDesiredSize,
                                     const nsHTMLReflowState& aReflowState,
                                     nsReflowStatus&          aStatus)
{
  NS_ASSERTION(nsnull==mPrevInFlow, "never ever call me on a continuing frame!");
  NS_ASSERTION(nsnull!=mCellMap, "never ever call me until the cell map is built!");
  PRInt32 colIndex=0;
  const nsStyleTable* tableStyle;
  PRBool createdColFrames;
  GetStyleData(eStyleStruct_Table, (const nsStyleStruct *&)tableStyle);
  EnsureColumns(aPresContext, createdColFrames);
  if (nsnull!=mColCache)
  {
    mCellMap->ClearColumnCache();
    delete mColCache;
    mColCache = nsnull;
  }

  mColCache = new ColumnInfoCache(GetColCount());
  CacheColFramesInCellMap();

  // handle rowgroups
  PRBool requiresPass1Layout = RequiresPass1Layout();

  nsIFrame * childFrame = mFrames.FirstChild();
  while (nsnull!=childFrame)
  { // in this loop, set column style info from cells
    const nsStyleDisplay *childDisplay;
    childFrame->GetStyleData(eStyleStruct_Display, ((const nsStyleStruct *&)childDisplay));
    if (PR_TRUE==IsRowGroup(childDisplay->mDisplay) && requiresPass1Layout)
    { // if it's a row group, get the cells and set the column style if appropriate
      SetColumnStylesFromCells(aPresContext, childFrame);
    }
    childFrame->GetNextSibling(&childFrame);
  }

  // second time through, set column cache info for each column
  // we can't do this until the loop above has set the column style info from the cells
  childFrame = mColGroups.FirstChild();
  while (nsnull!=childFrame)
  { // for every child, if it's a col group then get the columns
    nsTableColFrame *colFrame=nsnull;
    childFrame->FirstChild(nsnull, (nsIFrame **)&colFrame);
    while (nsnull!=colFrame)
    { // for every column, create an entry in the column cache
      // assumes that the col style has been twiddled to account for first cell width attribute
      const nsStyleDisplay *colDisplay;
      colFrame->GetStyleData(eStyleStruct_Display, ((const nsStyleStruct *&)colDisplay));
      if (NS_STYLE_DISPLAY_TABLE_COLUMN == colDisplay->mDisplay)
      {
        const nsStylePosition* colPosition;
        colFrame->GetStyleData(eStyleStruct_Position, ((const nsStyleStruct *&)colPosition));
        PRInt32 repeat = colFrame->GetSpan();
        colIndex = colFrame->GetColumnIndex();
        for (PRInt32 i=0; i<repeat; i++)
        {
          mColCache->AddColumnInfo(colPosition->mWidth.GetUnit(), colIndex+i);
        }
      }
      colFrame->GetNextSibling((nsIFrame **)&colFrame);
    }
    childFrame->GetNextSibling(&childFrame);
  }
  mBits.mColumnCacheValid=PR_TRUE;
}

void nsTableFrame::CacheColFramesInCellMap()
{
  nsIFrame * childFrame = mColGroups.FirstChild();
  while (nsnull != childFrame) { // in this loop, we cache column info 
    nsTableColFrame* colFrame = nsnull;
    childFrame->FirstChild(nsnull, (nsIFrame **)&colFrame);
    while (nsnull != colFrame) {
      const nsStyleDisplay* colDisplay;
      colFrame->GetStyleData(eStyleStruct_Display, ((const nsStyleStruct *&)colDisplay));
      if (NS_STYLE_DISPLAY_TABLE_COLUMN == colDisplay->mDisplay) {
        PRInt32 colIndex = colFrame->GetColumnIndex();
        PRInt32 repeat   = colFrame->GetSpan();
        for (PRInt32 i=0; i<repeat; i++) {
          nsTableColFrame* cachedColFrame = mCellMap->GetColumnFrame(colIndex+i);
          if (nsnull==cachedColFrame) {
            mCellMap->AppendColumnFrame(colFrame);
          }
          colIndex++;
        }
      }
      colFrame->GetNextSibling((nsIFrame **)&colFrame);
    }
    childFrame->GetNextSibling(&childFrame);
  }
}

PRBool nsTableFrame::ColumnsCanBeInvalidatedBy(nsStyleCoord*           aPrevStyleWidth,
                                               const nsTableCellFrame& aCellFrame) const
{
  if (mTableLayoutStrategy) {
    return mTableLayoutStrategy->ColumnsCanBeInvalidatedBy(aPrevStyleWidth, aCellFrame);
  }
  return PR_FALSE;
}

PRBool nsTableFrame::ColumnsCanBeInvalidatedBy(const nsTableCellFrame& aCellFrame,
                                               PRBool                  aConsiderMinWidth) const

{
  if (mTableLayoutStrategy) {
    return mTableLayoutStrategy->ColumnsCanBeInvalidatedBy(aCellFrame, aConsiderMinWidth);
  }
  return PR_FALSE;
}

PRBool nsTableFrame::ColumnsAreValidFor(const nsTableCellFrame& aCellFrame,
                                        nscoord                 aPrevCellMin,
                                        nscoord                 aPrevCellDes) const
{
  if (mTableLayoutStrategy) {
    return mTableLayoutStrategy->ColumnsAreValidFor(aCellFrame, aPrevCellMin, aPrevCellDes);
  }
  return PR_FALSE;
}
  
void nsTableFrame::InvalidateColumnWidths()
{
  nsTableFrame * firstInFlow = (nsTableFrame *)GetFirstInFlow();
  NS_ASSERTION(nsnull!=firstInFlow, "illegal state -- no first in flow");
  firstInFlow->mBits.mColumnWidthsValid=PR_FALSE;
}

PRBool nsTableFrame::IsColumnWidthsValid() const
{
  nsTableFrame * firstInFlow = (nsTableFrame *)GetFirstInFlow();
  NS_ASSERTION(nsnull!=firstInFlow, "illegal state -- no first in flow");
  return (PRBool)firstInFlow->mBits.mColumnWidthsValid;
}

PRBool nsTableFrame::IsFirstPassValid() const
{
  nsTableFrame * firstInFlow = (nsTableFrame *)GetFirstInFlow();
  NS_ASSERTION(nsnull!=firstInFlow, "illegal state -- no first in flow");
  return (PRBool)firstInFlow->mBits.mFirstPassValid;
}

void nsTableFrame::InvalidateFirstPassCache()
{
  nsTableFrame * firstInFlow = (nsTableFrame *)GetFirstInFlow();
  NS_ASSERTION(nsnull!=firstInFlow, "illegal state -- no first in flow");
  firstInFlow->mBits.mFirstPassValid=PR_FALSE;
}

PRBool nsTableFrame::IsColumnCacheValid() const
{
  nsTableFrame * firstInFlow = (nsTableFrame *)GetFirstInFlow();
  NS_ASSERTION(nsnull!=firstInFlow, "illegal state -- no first in flow");
  return (PRBool)firstInFlow->mBits.mColumnCacheValid;
}

void nsTableFrame::InvalidateColumnCache()
{
  nsTableFrame * firstInFlow = (nsTableFrame *)GetFirstInFlow();
  NS_ASSERTION(nsnull!=firstInFlow, "illegal state -- no first in flow");
  firstInFlow->mBits.mColumnCacheValid=PR_FALSE;
}

PRBool nsTableFrame::IsCellMapValid() const
{
  nsTableFrame * firstInFlow = (nsTableFrame *)GetFirstInFlow();
  NS_ASSERTION(nsnull!=firstInFlow, "illegal state -- no first in flow");
  return (PRBool)firstInFlow->mBits.mCellMapValid;
}

static void InvalidateCellMapForRowGroup(nsIFrame* aRowGroupFrame)
{
  nsIFrame *rowFrame;
  aRowGroupFrame->FirstChild(nsnull, &rowFrame);
  for ( ; nsnull!=rowFrame; rowFrame->GetNextSibling(&rowFrame))
  {
    const nsStyleDisplay *rowDisplay;
    rowFrame->GetStyleData(eStyleStruct_Display, (const nsStyleStruct *&)rowDisplay);
    if (NS_STYLE_DISPLAY_TABLE_ROW==rowDisplay->mDisplay)
    {
      ((nsTableRowFrame *)rowFrame)->ResetInitChildren();
    }
    else if (NS_STYLE_DISPLAY_TABLE_ROW_GROUP==rowDisplay->mDisplay)
    {
      InvalidateCellMapForRowGroup(rowFrame);
    }
  }

}

void nsTableFrame::InvalidateCellMap()
{
  nsTableFrame * firstInFlow = (nsTableFrame *)GetFirstInFlow();
  NS_ASSERTION(nsnull!=firstInFlow, "illegal state -- no first in flow");
  firstInFlow->mBits.mCellMapValid=PR_FALSE;
  // reset the state in each row
  nsIFrame *rowGroupFrame=mFrames.FirstChild();
  for ( ; nsnull!=rowGroupFrame; rowGroupFrame->GetNextSibling(&rowGroupFrame))
  {
    const nsStyleDisplay *rowGroupDisplay;
    rowGroupFrame->GetStyleData(eStyleStruct_Display, (const nsStyleStruct *&)rowGroupDisplay);
    if (PR_TRUE==IsRowGroup(rowGroupDisplay->mDisplay))
    {
      InvalidateCellMapForRowGroup(rowGroupFrame);
    }
  }
}

PRInt32 nsTableFrame::GetColumnWidth(PRInt32 aColIndex)
{
  nsTableFrame * firstInFlow = (nsTableFrame *)GetFirstInFlow();
  NS_ASSERTION(nsnull!=firstInFlow, "illegal state -- no first in flow");
  PRInt32 result = 0;
  if (this!=firstInFlow)
    result = firstInFlow->GetColumnWidth(aColIndex);
  else
  {
    NS_ASSERTION(nsnull!=mColumnWidths, "illegal state");
    // can't assert on IsColumnWidthsSet because we might want to call this
    // while we're in the process of setting column widths, and we don't
    // want to complicate IsColumnWidthsSet by making it a multiple state return value
    // (like eNotSet, eSetting, eIsSet)
#ifdef NS_DEBUG
    NS_ASSERTION(nsnull!=mCellMap, "no cell map");
    PRInt32 numCols = mCellMap->GetColCount();
    NS_ASSERTION (numCols > aColIndex, "bad arg, col index out of bounds");
#endif
    if (nsnull!=mColumnWidths)
     result = mColumnWidths[aColIndex];
  }

  return result;
}

void  nsTableFrame::SetColumnWidth(PRInt32 aColIndex, nscoord aWidth)
{
  nsTableFrame * firstInFlow = (nsTableFrame *)GetFirstInFlow();
  NS_ASSERTION(nsnull!=firstInFlow, "illegal state -- no first in flow");

  if (this!=firstInFlow)
    firstInFlow->SetColumnWidth(aColIndex, aWidth);
  else
  {
    // Note: in the case of incremental reflow sometimes the table layout
    // strategy will call to set a column width before we've allocated the
    // column width array
    if (!mColumnWidths) {
      mColumnWidthsLength = mCellMap->GetColCount();
      mColumnWidths = new PRInt32[mColumnWidthsLength];
      nsCRT::memset (mColumnWidths, 0, mColumnWidthsLength*sizeof(PRInt32));
    }
    
    if (nsnull!=mColumnWidths && aColIndex<mColumnWidthsLength) {
      mColumnWidths[aColIndex] = aWidth;
    }
  }
}

/**
  *
  * Update the border style to map to the HTML border style
  *
  */
void nsTableFrame::MapHTMLBorderStyle(nsStyleSpacing& aSpacingStyle, nscoord aBorderWidth)
{
  nsStyleCoord  width;
  width.SetCoordValue(aBorderWidth);
  aSpacingStyle.mBorder.SetTop(width);
  aSpacingStyle.mBorder.SetLeft(width);
  aSpacingStyle.mBorder.SetBottom(width);
  aSpacingStyle.mBorder.SetRight(width);

  aSpacingStyle.SetBorderStyle(NS_SIDE_TOP, NS_STYLE_BORDER_STYLE_BG_OUTSET);
  aSpacingStyle.SetBorderStyle(NS_SIDE_LEFT, NS_STYLE_BORDER_STYLE_BG_OUTSET);
  aSpacingStyle.SetBorderStyle(NS_SIDE_BOTTOM, NS_STYLE_BORDER_STYLE_BG_OUTSET);
  aSpacingStyle.SetBorderStyle(NS_SIDE_RIGHT, NS_STYLE_BORDER_STYLE_BG_OUTSET);

  nsIStyleContext* styleContext = mStyleContext; 
  const nsStyleColor* colorData = (const nsStyleColor*)
    styleContext->GetStyleData(eStyleStruct_Color);

  // Look until we find a style context with a NON-transparent background color
  while (styleContext)
  {
    if ((colorData->mBackgroundFlags & NS_STYLE_BG_COLOR_TRANSPARENT)!=0)
    {
      nsIStyleContext* temp = styleContext;
      styleContext = styleContext->GetParent();
      if (temp != mStyleContext)
        NS_RELEASE(temp);
      colorData = (const nsStyleColor*)styleContext->GetStyleData(eStyleStruct_Color);
    }
    else
    {
      break;
    }
  }

  // Yaahoo, we found a style context which has a background color 
  
  nscolor borderColor = 0xFFC0C0C0;

  if (styleContext != nsnull)
  {
    borderColor = colorData->mBackgroundColor;
    if (styleContext != mStyleContext)
      NS_RELEASE(styleContext);
  }

  // if the border color is white, then shift to grey
  if (borderColor == 0xFFFFFFFF)
    borderColor = 0xFFC0C0C0;

  aSpacingStyle.SetBorderColor(NS_SIDE_TOP, borderColor);
  aSpacingStyle.SetBorderColor(NS_SIDE_LEFT, borderColor);
  aSpacingStyle.SetBorderColor(NS_SIDE_BOTTOM, borderColor);
  aSpacingStyle.SetBorderColor(NS_SIDE_RIGHT, borderColor);

}



PRBool nsTableFrame::ConvertToPixelValue(nsHTMLValue& aValue, PRInt32 aDefault, PRInt32& aResult)
{
  if (aValue.GetUnit() == eHTMLUnit_Pixel)
    aResult = aValue.GetPixelValue();
  else if (aValue.GetUnit() == eHTMLUnit_Empty)
    aResult = aDefault;
  else
  {
    NS_ERROR("Unit must be pixel or empty");
    return PR_FALSE;
  }
  return PR_TRUE;
}

void nsTableFrame::MapBorderMarginPadding(nsIPresContext& aPresContext)
{
#if 0
  // Check to see if the table has either cell padding or 
  // Cell spacing defined for the table. If true, then
  // this setting overrides any specific border, margin or 
  // padding information in the cell. If these attributes
  // are not defined, the the cells attributes are used
  
  nsHTMLValue padding_value;
  nsHTMLValue spacing_value;
  nsHTMLValue border_value;


  nsresult border_result;

  nscoord   padding = 0;
  nscoord   spacing = 0;
  nscoord   border  = 1;

  float     p2t = aPresContext.GetPixelsToTwips();

  nsIHTMLContent*  table = (nsIHTMLContent*)mContent;

  NS_ASSERTION(table,"Table Must not be null");
  if (!table)
    return;

  nsStyleSpacing* spacingData = (nsStyleSpacing*)mStyleContext->GetMutableStyleData(eStyleStruct_Spacing);

  border_result = table->GetAttribute(nsHTMLAtoms::border,border_value);
  if (border_result == NS_CONTENT_ATTR_HAS_VALUE)
  {
    PRInt32 intValue = 0;

    if (ConvertToPixelValue(border_value,1,intValue)) //XXX this is busted if this code is ever used again. MMP
      border = NSIntPixelsToTwips(intValue, p2t); 
  }
  MapHTMLBorderStyle(*spacingData,border);
#endif
}


NS_METHOD nsTableFrame::GetCellMarginData(nsTableCellFrame* aKidFrame, nsMargin& aMargin)
{
  nsresult result = NS_ERROR_NOT_INITIALIZED;

  if (nsnull != aKidFrame)
  {
    result = aKidFrame->GetMargin(aMargin);
  }

  return result;
}

//XXX: ok, this looks dumb now.  but in a very short time this will get filled in
void nsTableFrame::GetTableBorder(nsMargin &aBorder)
{
  if (NS_STYLE_BORDER_COLLAPSE==GetBorderCollapseStyle())
  {
    aBorder = mBorderEdges->mMaxBorderWidth;
  }
  else
  {
    const nsStyleSpacing* spacing =
      (const nsStyleSpacing*)mStyleContext->GetStyleData(eStyleStruct_Spacing);
    spacing->GetBorder(aBorder);
  }
}

/*
now I need to actually use gettableborderat, instead of assuming that the table border is homogenous
across rows and columns.  in tableFrame, LayoutStrategy, and cellFrame, maybe rowFrame
need something similar for cell (for those with spans?)
*/

void nsTableFrame::GetTableBorderAt(nsMargin &aBorder, PRInt32 aRowIndex, PRInt32 aColIndex)
{
  if (NS_STYLE_BORDER_COLLAPSE==GetBorderCollapseStyle())
  {
    nsBorderEdge *border = (nsBorderEdge *)(mBorderEdges->mEdges[NS_SIDE_LEFT].ElementAt(aRowIndex));
		if (border) {
			aBorder.left = border->mWidth;
			border = (nsBorderEdge *)(mBorderEdges->mEdges[NS_SIDE_RIGHT].ElementAt(aRowIndex));
      if (border)
			  aBorder.right = border->mWidth;
			border = (nsBorderEdge *)(mBorderEdges->mEdges[NS_SIDE_TOP].ElementAt(aColIndex));
      if (border)
			  aBorder.top = border->mWidth;
			border = (nsBorderEdge *)(mBorderEdges->mEdges[NS_SIDE_TOP].ElementAt(aColIndex));
      if (border)
			  aBorder.bottom = border->mWidth;
		}
  }
  else
  {
    const nsStyleSpacing* spacing =
      (const nsStyleSpacing*)mStyleContext->GetStyleData(eStyleStruct_Spacing);
    spacing->GetBorder(aBorder);
  }
}

void nsTableFrame::GetTableBorderForRowGroup(nsTableRowGroupFrame * aRowGroupFrame, nsMargin &aBorder)
{
  aBorder.SizeTo(0,0,0,0);
  if (nsnull!=aRowGroupFrame)
  {
    if (NS_STYLE_BORDER_COLLAPSE==GetBorderCollapseStyle())
    {
      PRInt32 rowIndex = aRowGroupFrame->GetStartRowIndex();
      PRInt32 rowCount;
      aRowGroupFrame->GetRowCount(rowCount);
      for ( ; rowIndex<rowCount; rowIndex++)
      {
        PRInt32 colIndex = 0;
        nsCellMap *cellMap = GetCellMap();
        PRInt32 colCount = cellMap->GetColCount();
        for ( ; colIndex<colCount; colIndex++)
        {
          nsMargin border;
          GetTableBorderAt (border, rowIndex, colIndex);
          aBorder.top = PR_MAX(aBorder.top, border.top);
          aBorder.right = PR_MAX(aBorder.right, border.right);
          aBorder.bottom = PR_MAX(aBorder.bottom, border.bottom);
          aBorder.left = PR_MAX(aBorder.left, border.left);
        }
      }
    }
    else
    {
      GetTableBorder (aBorder);
    }
  }
}

PRUint8 nsTableFrame::GetBorderCollapseStyle()
{
  const nsStyleTable* tableStyle;
  GetStyleData(eStyleStruct_Table, (const nsStyleStruct *&)tableStyle);
  return tableStyle->mBorderCollapse;
}


// XXX: could cache this.  But be sure to check style changes if you do!
nscoord nsTableFrame::GetCellSpacingX()
{
  const nsStyleTable* tableStyle;
  GetStyleData(eStyleStruct_Table, (const nsStyleStruct *&)tableStyle);
  nscoord cellSpacing = 0;
  PRUint8 borderCollapseStyle = GetBorderCollapseStyle();
  if (NS_STYLE_BORDER_COLLAPSE!=borderCollapseStyle) {
    if (tableStyle->mBorderSpacingX.GetUnit() == eStyleUnit_Coord) {
      cellSpacing = tableStyle->mBorderSpacingX.GetCoordValue();
    }
  }
  return cellSpacing;
}

// XXX: could cache this. But be sure to check style changes if you do!
nscoord nsTableFrame::GetCellSpacingY()
{
  const nsStyleTable* tableStyle;
  GetStyleData(eStyleStruct_Table, (const nsStyleStruct *&)tableStyle);
  nscoord cellSpacing = 0;
  PRUint8 borderCollapseStyle = GetBorderCollapseStyle();
  if (NS_STYLE_BORDER_COLLAPSE!=borderCollapseStyle) {
    if (tableStyle->mBorderSpacingY.GetUnit() == eStyleUnit_Coord) {
      cellSpacing = tableStyle->mBorderSpacingY.GetCoordValue();
    }
  }
  return cellSpacing;
}

// Get the cellpadding defined on the table. Each cell can override this with style
nscoord nsTableFrame::GetCellPadding()
{
  const nsStyleTable* tableStyle;
  GetStyleData(eStyleStruct_Table, (const nsStyleStruct *&)tableStyle);
  nscoord cellPadding = -1;
  if (tableStyle->mCellPadding.GetUnit() == eStyleUnit_Coord) {
    cellPadding = tableStyle->mCellPadding.GetCoordValue();
  }
  return cellPadding;
}


void nsTableFrame::GetColumnsByType(const nsStyleUnit aType, 
                                    PRInt32& aOutNumColumns,
                                    PRInt32 *& aOutColumnIndexes)
{
  mColCache->GetColumnsByType(aType, aOutNumColumns, aOutColumnIndexes);
}



/* ----- global methods ----- */

nsresult 
NS_NewTableFrame(nsIFrame** aNewFrame)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsTableFrame* it = new nsTableFrame;
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  *aNewFrame = it;
  return NS_OK;
}

NS_METHOD nsTableFrame::GetTableFrame(nsIFrame *aSourceFrame, nsTableFrame *& aTableFrame)
{
  nsresult rv = NS_ERROR_UNEXPECTED;  // the value returned
  aTableFrame = nsnull;               // initialize out-param
  nsIFrame *parentFrame=nsnull;
  if (nsnull!=aSourceFrame)
  {
    // "result" is the result of intermediate calls, not the result we return from this method
    nsresult result = aSourceFrame->GetParent((nsIFrame **)&parentFrame); 
    while ((NS_OK==result) && (nsnull!=parentFrame))
    {
      const nsStyleDisplay *display;
      parentFrame->GetStyleData(eStyleStruct_Display, (const nsStyleStruct *&)display);
      if (NS_STYLE_DISPLAY_TABLE == display->mDisplay)
      {
        aTableFrame = (nsTableFrame *)parentFrame;
        rv = NS_OK; // only set if we found the table frame
        break;
      }
      result = parentFrame->GetParent((nsIFrame **)&parentFrame);
    }
  }
  NS_POSTCONDITION(nsnull!=aTableFrame, "unable to find table parent. aTableFrame null.");
  NS_POSTCONDITION(NS_OK==rv, "unable to find table parent. result!=NS_OK");
  return rv;
}

/* helper method for determining if this is a nested table or not */
// aReflowState must be the reflow state for this inner table frame, should have an assertion here for that
PRBool nsTableFrame::IsNested(const nsHTMLReflowState& aReflowState, const nsStylePosition *& aPosition) const
{
  PRBool result = PR_FALSE;
  // Walk up the reflow state chain until we find a cell or the root
  const nsReflowState* rs = aReflowState.parentReflowState; // this is for the outer frame
  if (rs)
    rs = rs->parentReflowState;  // and this is the parent of the outer frame
  while (nsnull != rs) 
  {
    const nsStyleDisplay *display;
    rs->frame->GetStyleData(eStyleStruct_Display, (const nsStyleStruct *&)display);
    if (NS_STYLE_DISPLAY_TABLE==display->mDisplay)
    {
      result = PR_TRUE;
      rs->frame->GetStyleData(eStyleStruct_Position, ((const nsStyleStruct *&)aPosition));
      break;
    }
    rs = rs->parentReflowState;
  }
  return result;
}


// aSpecifiedTableWidth is filled if the table witdth is not auto
PRBool nsTableFrame::IsAutoWidth(const nsHTMLReflowState& aReflowState,
                                 nscoord&                 aSpecifiedTableWidth)
{
  PRBool isAuto = PR_TRUE;  // the default

  nsStylePosition* tablePosition = (nsStylePosition*)mStyleContext->GetStyleData(eStyleStruct_Position);

  switch (tablePosition->mWidth.GetUnit()) {

  case eStyleUnit_Auto:         // specified auto width
  case eStyleUnit_Proportional: // illegal for table, so ignored
    break;
  case eStyleUnit_Inherit:
    // get width of parent and see if it is a specified value or not
    // XXX for now, just return true
    break;
  case eStyleUnit_Coord:
  case eStyleUnit_Percent:
    if ((aReflowState.mComputedWidth > 0) &&
        (aReflowState.mComputedWidth != NS_UNCONSTRAINEDSIZE)) {
      aSpecifiedTableWidth = aReflowState.mComputedWidth;
    }
    isAuto = PR_FALSE;
    break;
  default:
    break;
  }

  return isAuto; 
}


nscoord nsTableFrame::GetMinCaptionWidth()
{
  nsIFrame *outerTableFrame=nsnull;
  GetParent(&outerTableFrame);
  return (((nsTableOuterFrame *)outerTableFrame)->GetMinCaptionWidth());
}

/** return the minimum width of the table.  Return 0 if the min width is unknown. */
nscoord nsTableFrame::GetMinTableContentWidth()
{
  nscoord result = 0;
  if (nsnull!=mTableLayoutStrategy)
    result = mTableLayoutStrategy->GetTableMinContentWidth();
  return result;
}

/** return the maximum width of the table.  Return 0 if the max width is unknown. */
nscoord nsTableFrame::GetMaxTableContentWidth()
{
  nscoord result = 0;
  if (nsnull!=mTableLayoutStrategy)
    result = mTableLayoutStrategy->GetTableMaxContentWidth();
  return result;
}

void nsTableFrame::SetMaxElementSize(nsSize* aMaxElementSize)
{
  if (nsnull!=mTableLayoutStrategy)
    mTableLayoutStrategy->SetMaxElementSize(aMaxElementSize);
}


PRBool nsTableFrame::RequiresPass1Layout()
{
  const nsStyleTable* tableStyle;
  GetStyleData(eStyleStruct_Table, (const nsStyleStruct *&)tableStyle);
  return (PRBool)(NS_STYLE_TABLE_LAYOUT_FIXED!=tableStyle->mLayoutStrategy);
}

NS_IMETHODIMP
nsTableFrame::GetFrameName(nsString& aResult) const
{
  return MakeFrameName("Table", aResult);
}

// This assumes that aFrame is a scroll frame if  
// XXX make this a macro if it becomes an issue
// XXX it has the side effect of setting mHasScrollableRowGroup
nsTableRowGroupFrame*
nsTableFrame::GetRowGroupFrameFor(nsIFrame* aFrame, const nsStyleDisplay* aDisplay) 
{
  nsIFrame* result = nsnull;
  if (IsRowGroup(aDisplay->mDisplay)) {
    nsresult rv = aFrame->QueryInterface(kITableRowGroupFrameIID, (void **)&result);
    if (NS_SUCCEEDED(rv) && (nsnull != result)) {
      ;
    } else { // it is a scroll frame that contains the row group frame
      aFrame->FirstChild(nsnull, &result);
      mBits.mHasScrollableRowGroup = PR_TRUE;
    }
  }

  return (nsTableRowGroupFrame*)result;
}

PRBool
nsTableFrame::IsFinalPass(const nsReflowState& aState) 
{
  return (NS_UNCONSTRAINEDSIZE != aState.availableWidth) ||
         (NS_UNCONSTRAINEDSIZE != aState.availableHeight);
}

void nsTableFrame::Dump(PRBool aDumpCols, PRBool aDumpCellMap)
{
  printf("***START TABLE DUMP***, \n colWidths=");
  PRInt32 colX;
  PRInt32 numCols = GetColCount();
  for (colX = 0; colX < numCols; colX++) {
    printf("%d ", mColumnWidths[colX]);
  }
  if (aDumpCols) {
    for (colX = 0; colX < numCols; colX++) {
      printf("\n");
      nsTableColFrame* colFrame = GetColFrame(colX);
      colFrame->Dump(1);
    }
  }
  if (aDumpCellMap) {
    printf("\n");
    nsCellMap* cellMap = GetCellMap();
#ifdef NS_DEBUG
    cellMap->Dump();
#endif
  }
  printf(" ***END TABLE DUMP*** \n");
}

// nsTableIterator
nsTableIterator::nsTableIterator(nsIFrame&        aSource,
                                 nsTableIteration aType)
{
  nsIFrame* firstChild;
  aSource.FirstChild(nsnull, &firstChild);
  Init(firstChild, aType);
}

nsTableIterator::nsTableIterator(nsFrameList&     aSource,
                                 nsTableIteration aType)
{
  nsIFrame* firstChild = aSource.FirstChild();
  Init(firstChild, aType);
}

void nsTableIterator::Init(nsIFrame*        aFirstChild,
                           nsTableIteration aType)
{
  mFirstListChild = aFirstChild;
  mFirstChild     = aFirstChild;
  mCurrentChild   = nsnull;
  mLeftToRight    = (eTableRTL == aType) ? PR_FALSE : PR_TRUE; 
  mCount          = -1;

  if (!mFirstChild) {
    return;
  }
  if (eTableDIR == aType) {
    nsTableFrame* table = nsnull;
    nsresult rv = nsTableFrame::GetTableFrame(mFirstChild, table);
    if (NS_SUCCEEDED(rv) && (table != nsnull)) {
      const nsStyleDisplay* display;
      table->GetStyleData(eStyleStruct_Display, (const nsStyleStruct*&)display);
      mLeftToRight = (NS_STYLE_DIRECTION_LTR == display->mDirection);
    }
    else {
      NS_ASSERTION(PR_FALSE, "source of table iterator is not part of a table");
      return;
    }
  }
  if (!mLeftToRight) {
    mCount = 0;
    nsIFrame* nextChild;
    mFirstChild->GetNextSibling(&nextChild);
    while (nsnull != nextChild) {
      mCount++;
      mFirstChild = nextChild;
      nextChild->GetNextSibling(&nextChild);
    }
  } 
}

nsIFrame* nsTableIterator::First()
{
  mCurrentChild = mFirstChild;
  return mCurrentChild;
}
      
nsIFrame* nsTableIterator::Next()
{
  if (!mCurrentChild) {
    return nsnull;
  }

  if (mLeftToRight) {
    mCurrentChild->GetNextSibling(&mCurrentChild);
    return mCurrentChild;
  }
  else {
    nsIFrame* targetChild = mCurrentChild;
    mCurrentChild = nsnull;
    nsIFrame* child = mFirstListChild;
    while (child && (child != targetChild)) {
      mCurrentChild = child;
      child->GetNextSibling(&child);
    }
    return mCurrentChild;
  }
}

PRBool nsTableIterator::IsLeftToRight()
{
  return mLeftToRight;
}

PRInt32 nsTableIterator::Count()
{
  if (-1 == mCount) {
    mCount = 0;
    nsIFrame* child = mFirstListChild;
    while (nsnull != child) {
      mCount++;
      child->GetNextSibling(&child);
    }
  }
  return mCount;
}

nsTableCellFrame* nsTableFrame::GetCellInfoAt(PRInt32            aRowX, 
                                              PRInt32            aColX, 
                                              PRBool*            aOriginates, 
                                              PRInt32*           aColSpan)
{
  nsCellMap* cellMap = GetCellMap();
  return cellMap->GetCellInfoAt(aRowX, aColX, aOriginates, aColSpan);
}

/*------------------ nsITableLayout methods ------------------------------*/
NS_IMETHODIMP 
nsTableFrame::GetCellDataAt(PRInt32 aRowIndex, PRInt32 aColIndex,
                            nsIDOMElement* &aCell,   //out params
                            PRInt32& aStartRowIndex, PRInt32& aStartColIndex, 
                            PRInt32& aRowSpan, PRInt32& aColSpan,
                            PRBool& aIsSelected)
{
  nsresult result;
  nsCellMap* cellMap = GetCellMap();
  // Initialize out params
  aCell = nsnull;
  aStartRowIndex = 0;
  aStartColIndex = 0;
  aRowSpan = 0;
  aColSpan = 0;
  aIsSelected = PR_FALSE;

  if (!cellMap) { return NS_ERROR_NOT_INITIALIZED;}

  // Return a special error value if an index is out of bounds
  // This will pass the NS_SUCCEEDED() test
  // Thus we can iterate indexes to get all cells in a row or col
  //   and stop when aCell is returned null.
  PRInt32 rowCount = cellMap->GetRowCount();
  PRInt32 colCount = cellMap->GetColCount();

  if (aRowIndex >= rowCount || aColIndex >= colCount)
  {
    return NS_TABLELAYOUT_CELL_NOT_FOUND;
  }
  nsTableCellFrame *cellFrame = cellMap->GetCellFrameOriginatingAt(aRowIndex, aColIndex);
  if (!cellFrame)
  { 
    PRInt32 rowSpan, colSpan;
    PRInt32 row = aStartRowIndex;
    PRInt32 col = aStartColIndex;

    // We didn't find a cell at requested location,
    //  most probably because of ROWSPAN and/or COLSPAN > 1
    // Find the cell that extends into the location we requested,
    //  starting at the most likely indexes supplied by the caller
    //  in aStartRowIndex and aStartColIndex;
    cellFrame = cellMap->GetCellFrameOriginatingAt(row, col);
    if (cellFrame)
    {
      rowSpan = cellFrame->GetRowSpan();
      colSpan = cellFrame->GetColSpan();

      // Check if this extends into the location we want
      if( aRowIndex >= row && aRowIndex < row+rowSpan && 
          aColIndex >= col && aColIndex < col+colSpan) 
      {
CELL_FOUND:
        aStartRowIndex = row;
        aStartColIndex = col;
        aRowSpan = rowSpan;
        aColSpan = colSpan;
        // I know jumps aren't cool, but it's efficient!
        goto TEST_IF_SELECTED;
      } else {
        // Suggested indexes didn't work,
        // Scan through entire table to find the spanned cell
        for (row = 0; row < rowCount; row++ )
        {
          for (col = 0; col < colCount; col++)
          {
            cellFrame = cellMap->GetCellFrameOriginatingAt(row, col);
            if (cellFrame)
            {
              rowSpan = cellFrame->GetRowSpan();
              colSpan = cellFrame->GetColSpan();
              if( aRowIndex >= row && aRowIndex < row+rowSpan && 
                  aColIndex >= col && aColIndex < col+colSpan) 
              {
                goto CELL_FOUND;
              }
            }
          }
          col = 0;
        }
      }
      return NS_ERROR_FAILURE; // Some other error 
    }
  }

  result = cellFrame->GetRowIndex(aStartRowIndex);
  if (NS_FAILED(result)) return result;
  result = cellFrame->GetColIndex(aStartColIndex);
  if (NS_FAILED(result)) return result;
  aRowSpan = cellFrame->GetRowSpan();
  aColSpan = cellFrame->GetColSpan();
  result = cellFrame->GetSelected(&aIsSelected);
  if (NS_FAILED(result)) return result;

TEST_IF_SELECTED:
  // do this last, because it addrefs, 
  // and we don't want the caller leaking it on error
  nsCOMPtr<nsIContent>content;
  result = cellFrame->GetContent(getter_AddRefs(content));  
  if (NS_SUCCEEDED(result) && content)
  {
    content->QueryInterface(nsIDOMElement::GetIID(), (void**)(&aCell));
  }   
                                         
  return result;
}

NS_IMETHODIMP nsTableFrame::GetTableSize(PRInt32& aRowCount, PRInt32& aColCount)
{
  nsCellMap* cellMap = GetCellMap();
  // Initialize out params
  aRowCount = 0;
  aColCount = 0;
  if (!cellMap) { return NS_ERROR_NOT_INITIALIZED;}

  aRowCount = cellMap->GetRowCount();
  aColCount = cellMap->GetColCount();
  return NS_OK;
}

/*---------------- end of nsITableLayout implementation ------------------*/

PRInt32 nsTableFrame::GetNumCellsOriginatingInRow(PRInt32 aRowIndex) const
{
  nsCellMap* cellMap = GetCellMap();
  return cellMap->GetNumCellsOriginatingInRow(aRowIndex);
}

PRInt32 nsTableFrame::GetNumCellsOriginatingInCol(PRInt32 aColIndex) const
{
  nsCellMap* cellMap = GetCellMap();
  return cellMap->GetNumCellsOriginatingInCol(aColIndex);
}

#define INDENT_PER_LEVEL 2

void nsTableFrame::DebugGetIndent(const nsIFrame* aFrame, 
                                  char*           aBuf)
{
  PRInt32 numLevels = 0;
  nsIFrame* parent = nsnull;
  aFrame->GetParent(&parent);
  while (parent) {
    nsIAtom* frameType = nsnull;
    parent->GetFrameType(&frameType);
    if ((nsDebugTable::gRflTableOuter && (nsLayoutAtoms::tableOuterFrame    == frameType)) ||
        (nsDebugTable::gRflTable      && (nsLayoutAtoms::tableFrame         == frameType)) ||
        (nsDebugTable::gRflRowGrp     && (nsLayoutAtoms::tableRowGroupFrame == frameType)) ||
        (nsDebugTable::gRflRow        && (nsLayoutAtoms::tableRowFrame      == frameType)) ||
        (nsDebugTable::gRflCell       && (nsLayoutAtoms::tableCellFrame     == frameType)) ||
        (nsDebugTable::gRflArea       && (nsLayoutAtoms::areaFrame          == frameType))) {
      numLevels++;
    }
    NS_IF_RELEASE(frameType);
    parent->GetParent(&parent);
  }
  PRInt32 indent = INDENT_PER_LEVEL * numLevels;
  nsCRT::memset (aBuf, ' ', indent);
  aBuf[indent] = 0;
}

void PrettyUC(nscoord aSize,
              char*   aBuf)
{
  if (NS_UNCONSTRAINEDSIZE == aSize) {
    strcpy(aBuf, "UC");
  }
  else {
    sprintf(aBuf, "%d", aSize);
  }
}

void nsTableFrame::DebugReflow(char*                      aMessage,
                               const nsIFrame*            aFrame,
                               const nsHTMLReflowState*   aState, 
                               const nsHTMLReflowMetrics* aMetrics)
{
  char indent[256];
  nsTableFrame::DebugGetIndent(aFrame, indent);
  printf("%s%s %p ", indent, aMessage, aFrame);
  char width[32];
  char height[32];
  if (aState) {
    PrettyUC(aState->availableWidth, width);
    PrettyUC(aState->availableHeight, height);
    printf("rea=%d av=(%s,%s) ", aState->reason, width, height); 
    PrettyUC(aState->mComputedWidth, width);
    PrettyUC(aState->mComputedHeight, height);
    printf("comp=(%s,%s) \n", width, height);
  }
  if (aMetrics) {
    if (aState) {
      printf("%s", indent);
    }
    PrettyUC(aMetrics->width, width);
    PrettyUC(aMetrics->height, height);
    printf("des=(%s,%s) ", width, height);
    if (aMetrics->maxElementSize) {
      PrettyUC(aMetrics->maxElementSize->width, width);
      PrettyUC(aMetrics->maxElementSize->height, height);
      printf("maxElem=(%s,%s)", width, height);
    }
    printf("\n");
  }
}

PRBool nsTableFrame::RowHasSpanningCells(PRInt32 aRowIndex)
{
  PRBool result = PR_FALSE;
  nsCellMap* cellMap = GetCellMap();
  NS_PRECONDITION (cellMap, "bad call, cellMap not yet allocated.");
  if (cellMap) {
		result = cellMap->RowHasSpanningCells(aRowIndex);
  }
  return result;
}

PRBool nsTableFrame::RowIsSpannedInto(PRInt32 aRowIndex)
{
  PRBool result = PR_FALSE;
  nsCellMap* cellMap = GetCellMap();
  NS_PRECONDITION (cellMap, "bad call, cellMap not yet allocated.");
  if (cellMap) {
		result = cellMap->RowIsSpannedInto(aRowIndex);
  }
  return result;
}

PRBool nsTableFrame::ColIsSpannedInto(PRInt32 aColIndex)
{
  PRBool result = PR_FALSE;
  nsCellMap * cellMap = GetCellMap();
  NS_PRECONDITION (cellMap, "bad call, cellMap not yet allocated.");
  if (cellMap) {
		result = cellMap->ColIsSpannedInto(aColIndex);
  }
  return result;
}


#ifdef DEBUG
NS_IMETHODIMP
nsTableFrame::SizeOf(nsISizeOfHandler* aHandler, PRUint32* aResult) const
{
  if (!aResult) {
    return NS_ERROR_NULL_POINTER;
  }
  PRUint32 sum = sizeof(*this);

  // Add in the amount of space for the column width array
  sum += mColumnWidthsLength * sizeof(PRInt32);

  // And in size of column info cache
  if (mColCache) {
    PRUint32 colCacheSize;
    mColCache->SizeOf(aHandler, &colCacheSize);
    aHandler->AddSize(nsLayoutAtoms::tableColCache, colCacheSize);
  }

  // Add in size of cell map
  PRUint32 cellMapSize;
  mCellMap->SizeOf(aHandler, &cellMapSize);
  aHandler->AddSize(nsLayoutAtoms::cellMap, cellMapSize);

  // Add in size of table layout strategy
  PRUint32 strategySize;
  mTableLayoutStrategy->SizeOf(aHandler, &strategySize);
  aHandler->AddSize(nsLayoutAtoms::tableStrategy, strategySize);

  *aResult = sum;
  return NS_OK;
}
#endif
