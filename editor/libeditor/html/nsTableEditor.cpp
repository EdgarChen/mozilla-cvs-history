/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */


#include "nsIDOMDocument.h"
#include "nsEditor.h"
#include "nsIDOMText.h"
#include "nsIDOMElement.h"
#include "nsIDOMAttr.h"
#include "nsIDOMNode.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMRange.h"
#include "nsIDOMSelection.h"
#include "nsLayoutCID.h"
#include "nsIContent.h"
#include "nsIContentIterator.h"
#include "nsIAtom.h"
#include "nsIDOMHTMLTableElement.h"
#include "nsIDOMHTMLTableCellElement.h"
#include "nsITableCellLayout.h" // For efficient access to table cell
#include "nsITableLayout.h"     //  data owned by the table and cell frames
#include "nsHTMLEditor.h"

#include "nsEditorUtils.h"

static NS_DEFINE_CID(kCContentIteratorCID, NS_CONTENTITERATOR_CID);

/***************************************************************************
 * stack based helper class for restoring selection after table edit
 */
class nsSetCaretAfterTableEdit
{
  private:
    nsCOMPtr<nsITableEditor> mEd;
    nsCOMPtr<nsIDOMElement> mTable;
    PRInt32 mCol, mRow, mDirection;
  public:
    nsSetCaretAfterTableEdit(nsITableEditor *aEd, nsIDOMElement* aTable, 
                             PRInt32 aRow, PRInt32 aCol, PRInt32 aDirection) : 
        mEd(do_QueryInterface(aEd))
    { 
      mTable = aTable; 
      mRow = aRow; 
      mCol = aCol; 
      mDirection = aDirection;
    } 
    
    ~nsSetCaretAfterTableEdit() 
    { 
      if (mEd)
        mEd->SetCaretAfterTableEdit(mTable, mRow, mCol, mDirection);
    }
    // This is needed to abort the caret reset in the destructor
    //  when one method yields control to another
    void CancelSetCaret() {mEd = nsnull; mTable = nsnull;}
};

// Table Editing helper utilities (not exposed in IDL)

NS_IMETHODIMP
nsHTMLEditor::InsertCell(nsIDOMElement *aCell, PRInt32 aRowSpan, PRInt32 aColSpan, 
                         PRBool aAfter, nsIDOMElement **aNewCell)
{
  if (!aCell) return NS_ERROR_NULL_POINTER;
  if (aNewCell) *aNewCell = nsnull;

  // And the parent and offsets needed to do an insert
  nsCOMPtr<nsIDOMNode> cellParent;
  nsresult res = aCell->GetParentNode(getter_AddRefs(cellParent));
  if( NS_SUCCEEDED(res) && cellParent)
  {
    PRInt32 cellOffset;
    res = GetChildOffset(aCell, cellParent, cellOffset);
    if( NS_SUCCEEDED(res))
    {
      nsCOMPtr<nsIDOMElement> newCell;
      res = CreateElementWithDefaults("td", getter_AddRefs(newCell));
      if(NS_FAILED(res)) return res;
      if(!newCell) return NS_ERROR_FAILURE;

      //Optional: return new cell created
      if (aNewCell)
      {
        *aNewCell = newCell.get();
        NS_ADDREF(*aNewCell);
      }
      if( aRowSpan > 1)
      {
        nsAutoString newRowSpan(aRowSpan);
        // Note: Do NOT use editor txt for this
        newCell->SetAttribute("rowspan", newRowSpan);
      }
      if( aColSpan > 1)
      {
        nsAutoString newColSpan(aColSpan);
        // Note: Do NOT use editor txt for this
        newCell->SetAttribute("colspan", newColSpan);
      }
      if(aAfter) cellOffset++;

      //Don't let Rules System change the selection
      nsAutoTxnsConserveSelection dontChangeSelection(this);
      res = nsEditor::InsertNode(newCell, cellParent, cellOffset);
    }
  }
  return res;
}

PRBool IsRowNode(nsCOMPtr<nsIDOMNode> &aNode)
{
  nsCOMPtr<nsIAtom> atom;
  nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
  if (content)
  {
    content->GetTag(*getter_AddRefs(atom));
    if (atom && atom.get() == nsIEditProperty::tr)
      return PR_TRUE;
  }
  return PR_FALSE;
}

NS_IMETHODIMP nsHTMLEditor::SetColSpan(nsIDOMElement *aCell, PRInt32 aColSpan)
{
  if (!aCell) return NS_ERROR_NULL_POINTER;
  nsAutoString newSpan;
  newSpan.Append(aColSpan, 10);
  nsAutoString colSpan("colspan");
  return SetAttribute(aCell, colSpan, newSpan);
}

NS_IMETHODIMP nsHTMLEditor::SetRowSpan(nsIDOMElement *aCell, PRInt32 aRowSpan)
{
  if (!aCell) return NS_ERROR_NULL_POINTER;
  nsAutoString newSpan;
  newSpan.Append(aRowSpan, 10);
  nsAutoString rowSpan("rowspan");
  return SetAttribute(aCell, rowSpan, newSpan);
}

/****************************************************************/

// Table Editing interface methods

NS_IMETHODIMP
nsHTMLEditor::InsertTableCell(PRInt32 aNumber, PRBool aAfter)
{
  nsCOMPtr<nsIDOMSelection> selection;
  nsCOMPtr<nsIDOMElement> table;
  nsCOMPtr<nsIDOMElement> curCell;
  nsCOMPtr<nsIDOMNode> cellParent;
  PRInt32 cellOffset, startRowIndex, startColIndex;
  nsresult res = GetCellContext(selection, table, curCell, cellParent, cellOffset, startRowIndex, startColIndex);
  if (NS_FAILED(res)) return res;
  // Don't fail if no cell found
  if (!curCell) return NS_EDITOR_ELEMENT_NOT_FOUND;

  // Get more data for current cell in row we are inserting at (we need COLSPAN)
  PRInt32 curStartRowIndex, curStartColIndex, rowSpan, colSpan, actualRowSpan, actualColSpan;
  PRBool  isSelected;
  res = GetCellDataAt(table, startRowIndex, startColIndex, *getter_AddRefs(curCell),
                      curStartRowIndex, curStartColIndex, rowSpan, colSpan,
                      actualRowSpan, actualColSpan, isSelected);
  if (NS_FAILED(res)) return res;
  if (!curCell) return NS_ERROR_FAILURE;
  PRInt32 newCellIndex = aAfter ? (startColIndex+colSpan) : startColIndex;
  //We control selection resetting after the insert...
  nsSetCaretAfterTableEdit setCaret(this, table, startRowIndex, newCellIndex, ePreviousColumn);
  //...so suppress Rules System selection munging
  nsAutoTxnsConserveSelection dontChangeSelection(this);

  PRInt32 i;
  for (i = 0; i < aNumber; i++)
  {
    nsCOMPtr<nsIDOMElement> newCell;
    res = CreateElementWithDefaults("td", getter_AddRefs(newCell));
    if (NS_SUCCEEDED(res) && newCell)
    {
      if (aAfter) cellOffset++;
      res = nsEditor::InsertNode(newCell, cellParent, cellOffset);
      if(NS_FAILED(res)) break;
    }
  }
  return res;
}

NS_IMETHODIMP 
nsHTMLEditor::GetFirstRow(nsIDOMElement* aTableElement, nsIDOMElement* &aRow)
{
  aRow = nsnull;

  nsCOMPtr<nsIDOMElement> tableElement;
  nsresult res = GetElementOrParentByTagName("table", aTableElement, getter_AddRefs(tableElement));
  if (NS_FAILED(res)) return res;
  if (!tableElement) return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIDOMNode>tableNode = do_QueryInterface(tableElement);
  if (!tableNode) return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIDOMNode> tableChild;
  res = tableNode->GetFirstChild(getter_AddRefs(tableChild));
  if (NS_FAILED(res)) return res;

  while (tableChild)
  {
    nsCOMPtr<nsIContent> content = do_QueryInterface(tableChild);
    if (content)
    {
      nsCOMPtr<nsIDOMElement> element;
      nsCOMPtr<nsIAtom> atom;
      content->GetTag(*getter_AddRefs(atom));
      if (atom.get() == nsIEditProperty::tr)
      {
        // Found a row directly under <table>
        element = do_QueryInterface(tableChild);
        if(element)
        {
          aRow = element.get();
          NS_ADDREF(aRow);
        }
        return NS_OK;
      }
      // Look for row in one of the row container elements      
      if (atom.get() == nsIEditProperty::tbody ||
          atom.get() == nsIEditProperty::thead ||
          atom.get() == nsIEditProperty::tfoot )
      {
        nsCOMPtr<nsIDOMNode> rowNode;
        // All children should be rows
        res = tableChild->GetFirstChild(getter_AddRefs(rowNode));
        if (NS_FAILED(res)) return res;
        if (rowNode && IsRowNode(rowNode))
        {
          element = do_QueryInterface(rowNode);
          if(element)
          {
            aRow = element.get();
            NS_ADDREF(aRow);
          }
          return NS_OK;
        }
      }
    }
    // Here if table child was a CAPTION or COLGROUP
    //  or child of a row-conainer wasn't a row (bad HTML)
    // Look in next table child
    res = tableChild->GetNextSibling(getter_AddRefs(tableChild));
  };
  return res;
}

NS_IMETHODIMP 
nsHTMLEditor::GetNextRow(nsIDOMElement* aTableElement, nsIDOMElement* &aRow)
{
  aRow = nsnull;  

  nsCOMPtr<nsIDOMElement> rowElement;
  nsresult res = GetElementOrParentByTagName("tr", aTableElement, getter_AddRefs(rowElement));
  if (NS_FAILED(res)) return res;
  if (!rowElement) return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIDOMNode> rowNode = do_QueryInterface(rowElement);
  if (!rowNode) return NS_ERROR_NULL_POINTER;
  
  nsCOMPtr<nsIDOMNode> nextRow;
  nsCOMPtr<nsIDOMNode> rowParent;
  nsCOMPtr<nsIDOMNode> parentSibling;
  nsCOMPtr<nsIDOMElement> element;

  rowNode->GetNextSibling(getter_AddRefs(nextRow));
  if(nextRow)
  {
    element = do_QueryInterface(nextRow);
    if(element)
    {
      aRow = element.get();
      NS_ADDREF(aRow);
    }
    return NS_OK;
  }
  // No row found, search for rows in other table sections
  res = rowNode->GetParentNode(getter_AddRefs(rowParent));
  if(NS_FAILED(res)) return res;
  if (!rowParent) return NS_ERROR_NULL_POINTER;

  res = rowParent->GetNextSibling(getter_AddRefs(parentSibling));
  if(NS_FAILED(res)) return res;

  while (parentSibling)
  {
    res = parentSibling->GetFirstChild(getter_AddRefs(nextRow));
    if(NS_FAILED(res)) return res;
    if (nextRow && IsRowNode(nextRow))
    {
      element = do_QueryInterface(nextRow);
      if(element)
      {
        aRow = element.get();
        NS_ADDREF(aRow);
      }
      return NS_OK;
    }
#ifdef DEBUG
    printf("GetNextRow: firstChild of row's parent's sibling is not a TR!\n");
#endif
    // We arrive here only if a table section has no children 
    //  or first child of section is not a row (bad HTML!)
    res = parentSibling->GetNextSibling(getter_AddRefs(parentSibling));
  }
  return res;
}

NS_IMETHODIMP
nsHTMLEditor::InsertTableColumn(PRInt32 aNumber, PRBool aAfter)
{
  nsCOMPtr<nsIDOMSelection> selection;
  nsCOMPtr<nsIDOMElement> table;
  nsCOMPtr<nsIDOMElement> curCell;
  nsCOMPtr<nsIDOMNode> cellParent;
  PRInt32 cellOffset, startRowIndex, startColIndex;
  nsresult res = GetCellContext(selection, table, curCell, cellParent, cellOffset, startRowIndex, startColIndex);
  if (NS_FAILED(res)) return res;
  // Don't fail if no cell found
  if (!curCell) return NS_EDITOR_ELEMENT_NOT_FOUND;

  // Get more data for current cell (we need ROWSPAN)
  PRInt32 curStartRowIndex, curStartColIndex, rowSpan, colSpan, actualRowSpan, actualColSpan;
  PRBool  isSelected;
  res = GetCellDataAt(table, startRowIndex, startColIndex, *getter_AddRefs(curCell),
                      curStartRowIndex, curStartColIndex, rowSpan, colSpan, 
                      actualRowSpan, actualColSpan, isSelected);
  if (NS_FAILED(res)) return res;
  if (!curCell) return NS_ERROR_FAILURE;

  nsAutoEditBatch beginBatching(this);

  // Use column after current cell if requested
  if (aAfter)
  {
    startColIndex += colSpan;
    //Detect when user is adding after a COLSPAN=0 case
    // Assume they want to stop the "0" behavior and
    // really add a new column. Thus we set the 
    // colspan to its true value
    if (colSpan == 0)
      SetColSpan(curCell, actualColSpan);
  }
   
  PRInt32 rowCount, colCount, rowIndex;
  res = GetTableSize(table, rowCount, colCount);
  if (NS_FAILED(res)) return res;

  PRInt32 lastColumn = colCount - 1;

  //We reset caret in destructor...
  nsSetCaretAfterTableEdit setCaret(this, table, startRowIndex, startColIndex, ePreviousRow);
  //.. so suppress Rules System selection munging
  nsAutoTxnsConserveSelection dontChangeSelection(this);

  nsCOMPtr<nsIDOMElement> rowElement;
  for ( rowIndex = 0; rowIndex < rowCount; rowIndex++)
  {
    if (startColIndex < colCount)
    {
      // We are inserting before an existing column
      res = GetCellDataAt(table, rowIndex, startColIndex, *getter_AddRefs(curCell),
                          curStartRowIndex, curStartColIndex, rowSpan, colSpan, 
                          actualRowSpan, actualColSpan, isSelected);
      if (NS_FAILED(res)) return res;

      // Don't fail entire process if we fail to find a cell
      //  (may fail just in particular rows with < adequate cells per row)
      if (curCell)
      {
        if (curStartColIndex < startColIndex)
        {
          // We have a cell spanning this location
          // Simply increase its colspan to keep table rectangular
          // Note: we do nothing if colsSpan=0,
          //  since it should automatically span the new column
          if (colSpan > 0)
            SetColSpan(curCell, colSpan+aNumber);
        } else {
          // Simply set selection to the current cell 
          //  so we can let InsertTableCell() do the work
          // Insert a new cell before current one
          selection->Collapse(curCell, 0);
          res = InsertTableCell(aNumber, PR_FALSE);
        }
      }
    } else {
      // We are inserting after all existing columns
      //TODO: Make sure table is "well formed" (call NormalizeTable)
      //  before appending new column
      
      // Get current row and append new cells after last cell in row
      if(rowIndex == 0)
        res = GetFirstRow(table.get(), *getter_AddRefs(rowElement));
      else
        res = GetNextRow(rowElement.get(), *getter_AddRefs(rowElement));
      if (NS_FAILED(res)) return res;

      nsCOMPtr<nsIDOMNode> lastCell;
      nsCOMPtr<nsIDOMNode> rowNode = do_QueryInterface(rowElement);
      if (!rowElement) return NS_ERROR_FAILURE;
      
      res = rowElement->GetLastChild(getter_AddRefs(lastCell));
      if (NS_FAILED(res)) return res;
      if (!lastCell) return NS_ERROR_FAILURE;
      curCell = do_QueryInterface(lastCell);
      if (curCell)
      {
        // Simply add same number of cells to each row
        // Although tempted to check cell indexes for curCell,
        //  the effects of COLSPAN>1 in some cells makes this futile!
        // We must use NormalizeTable first to assure proper 
        //  that there are cells in each cellmap location
        selection->Collapse(curCell, 0);
        res = InsertTableCell(aNumber, PR_TRUE);
      }
    }
  }
  return res;
}

NS_IMETHODIMP
nsHTMLEditor::InsertTableRow(PRInt32 aNumber, PRBool aAfter)
{
  nsCOMPtr<nsIDOMSelection> selection;
  nsCOMPtr<nsIDOMElement> table;
  nsCOMPtr<nsIDOMElement> curCell;
  nsCOMPtr<nsIDOMNode> cellParent;
  PRInt32 cellOffset, startRowIndex, startColIndex;
  nsresult res = GetCellContext(selection, table, curCell, cellParent, cellOffset, startRowIndex, startColIndex);
  if (NS_FAILED(res)) return res;
  // Don't fail if no cell found
  if (!curCell) return NS_EDITOR_ELEMENT_NOT_FOUND;

  // Get more data for current cell in row we are inserting at (we need COLSPAN)
  PRInt32 curStartRowIndex, curStartColIndex, rowSpan, colSpan, actualRowSpan, actualColSpan;
  PRBool  isSelected;
  res = GetCellDataAt(table, startRowIndex, startColIndex, *getter_AddRefs(curCell),
                      curStartRowIndex, curStartColIndex, rowSpan, colSpan, 
                      actualRowSpan, actualColSpan, isSelected);
  if (NS_FAILED(res)) return res;
  if (!curCell) return NS_ERROR_FAILURE;
  
  nsCOMPtr<nsIDOMElement> parentRow;
  res = GetElementOrParentByTagName("tr", curCell, getter_AddRefs(parentRow));
  if (NS_FAILED(res)) return res;
  if (!parentRow) return NS_ERROR_NULL_POINTER;

  PRInt32 rowCount, colCount;
  res = GetTableSize(table, rowCount, colCount);
  if (NS_FAILED(res)) return res;

  // Get the parent and offset where we will insert new row(s)
  nsCOMPtr<nsIDOMNode> parentOfRow;
  PRInt32 newRowOffset;
  parentRow->GetParentNode(getter_AddRefs(parentOfRow));
  if (!parentOfRow) return NS_ERROR_NULL_POINTER;
  res = GetChildOffset(parentRow, parentOfRow, newRowOffset);
  if (NS_FAILED(res)) return res;
  if (!parentOfRow)   return NS_ERROR_NULL_POINTER;

  nsAutoEditBatch beginBatching(this);

  if (aAfter)
  {
    // Use row after current cell
    startRowIndex += actualRowSpan;
    // offset to use for new row insert
    newRowOffset += actualRowSpan;

    //Detect when user is adding after a ROWSPAN=0 case
    // Assume they want to stop the "0" behavior and
    // really add a new row. Thus we set the 
    // rowspan to its true value
    if (rowSpan == 0)
      SetRowSpan(curCell, actualRowSpan);
  }

  //We control selection resetting after the insert...
  nsSetCaretAfterTableEdit setCaret(this, table, startRowIndex, startColIndex, ePreviousColumn);
  //...so suppress Rules System selection munging
  nsAutoTxnsConserveSelection dontChangeSelection(this);

  PRInt32 cellsInRow = 0;
  if (startRowIndex < rowCount)
  {
    // We are inserting above an existing row
    // Get each cell in the insert row to adjust for COLSPAN effects while we
    //   count how many cells are needed
    PRInt32 colIndex = 0;
    // This returns NS_TABLELAYOUT_CELL_NOT_FOUND when we run past end of row,
    //   which passes the NS_SUCCEEDED macro
    while ( NS_OK == GetCellDataAt(table, newRowOffset, colIndex, *getter_AddRefs(curCell), 
                                   curStartRowIndex, curStartColIndex, rowSpan, colSpan, 
                                   actualRowSpan, actualColSpan, isSelected) )
    {
      if (curCell)
      {
        if (curStartRowIndex < startRowIndex)
        {
          // We have a cell spanning this location
          // Simply increase its rowspan
          //Note that if rowSpan == 0, we do nothing,
          //  since that cell should automatically extend into the new row
          if (rowSpan > 0)
            SetRowSpan(curCell, rowSpan+aNumber);
        } else {
          // Count the number of cells we need to add to the new row
          cellsInRow += actualColSpan;
        }
        // Next cell in row
        colIndex += actualColSpan;
      }
      else
        colIndex++;
    }
  } else {
    // We are adding a new row after all others
    // If it weren't for colspan=0 effect, 
    // we could simply use colCount for number of new cells...
    cellsInRow = colCount;
    
    // ...but we must compensate for all cells with rowSpan = 0 in the last row
    PRInt32 lastRow = rowCount-1;
    PRInt32 tempColIndex = 0;
    while ( NS_OK == GetCellDataAt(table, lastRow, tempColIndex, *getter_AddRefs(curCell), 
                                   curStartRowIndex, curStartColIndex, rowSpan, colSpan, 
                                   actualRowSpan, actualColSpan, isSelected) )
    {
      if (rowSpan == 0)
        cellsInRow -= actualColSpan;
      
      tempColIndex += actualColSpan;
    }
  }

  if (cellsInRow > 0)
  {
    for (PRInt32 row = 0; row < aNumber; row++)
    {
      // Create a new row
      nsCOMPtr<nsIDOMElement> newRow;
      res = CreateElementWithDefaults("tr", getter_AddRefs(newRow));
      if (NS_SUCCEEDED(res))
      {
        if (!newRow) return NS_ERROR_FAILURE;
      
        for (PRInt32 i = 0; i < cellsInRow; i++)
        {
          nsCOMPtr<nsIDOMElement> newCell;
          res = CreateElementWithDefaults("td", getter_AddRefs(newCell));
          if (NS_FAILED(res)) return res;
          if (!newCell) return NS_ERROR_FAILURE;

          // Don't use transaction system yet! (not until entire row is inserted)
          nsCOMPtr<nsIDOMNode>resultNode;
          res = newRow->AppendChild(newCell, getter_AddRefs(resultNode));
          if (NS_FAILED(res)) return res;
        }
        // Use transaction system to insert the entire row+cells
        // (Note that rows are inserted at same childoffset each time)
        res = nsEditor::InsertNode(newRow, parentOfRow, newRowOffset);
        if (NS_FAILED(res)) return res;
      }
    }
  }
  return res;
}

// Editor helper only
NS_IMETHODIMP
nsHTMLEditor::DeleteTable(nsCOMPtr<nsIDOMElement> &aTable, nsCOMPtr<nsIDOMSelection> &aSelection)
{
  nsCOMPtr<nsIDOMNode> tableParent;
  PRInt32 tableOffset;
  if(NS_FAILED(aTable->GetParentNode(getter_AddRefs(tableParent))) || !tableParent)
    return NS_ERROR_FAILURE;

  // Save offset we need to restore the selection
  if(NS_FAILED(GetChildOffset(aTable, tableParent, tableOffset)))
    return NS_ERROR_FAILURE;

  nsresult res = DeleteNode(aTable);

  // Place selection just before the table
  aSelection->Collapse(tableParent, tableOffset);
  
  return res;
}

NS_IMETHODIMP
nsHTMLEditor::DeleteTable()
{
  nsCOMPtr<nsIDOMSelection> selection;
  nsCOMPtr<nsIDOMElement> table;
  nsCOMPtr<nsIDOMElement> cell;
  nsCOMPtr<nsIDOMNode> cellParent;
  PRInt32 cellOffset, startRowIndex, startColIndex;
  nsresult res = GetCellContext(selection, table, cell, cellParent, cellOffset, startRowIndex, startColIndex);
    
  if (NS_SUCCEEDED(res))
  {
    nsAutoEditBatch beginBatching(this);
    res = DeleteTable(table, selection);
  }
  return res;
}

NS_IMETHODIMP
nsHTMLEditor::DeleteTableCell(PRInt32 aNumber)
{
  nsCOMPtr<nsIDOMSelection> selection;
  nsCOMPtr<nsIDOMElement> table;
  nsCOMPtr<nsIDOMElement> cell;
  nsCOMPtr<nsIDOMNode> cellParent;
  PRInt32 cellOffset, startRowIndex, startColIndex;

  nsresult res = nsEditor::GetSelection(getter_AddRefs(selection));
  if (NS_FAILED(res)) return res;
  if (!selection) return NS_ERROR_FAILURE;

  nsAutoEditBatch beginBatching(this);

  for (PRInt32 i = 0; i < aNumber; i++)
  {
    res = GetCellContext(selection, table, cell, cellParent, cellOffset, startRowIndex, startColIndex);
    if (NS_FAILED(res)) return res;
    // Don't fail if no cell found
    if (!cell) return NS_EDITOR_ELEMENT_NOT_FOUND;

    if (1 == GetNumberOfCellsInRow(table, startRowIndex))
    {
      nsCOMPtr<nsIDOMElement> parentRow;
      res = GetElementOrParentByTagName("tr", cell, getter_AddRefs(parentRow));
      if (NS_FAILED(res)) return res;
      if (!parentRow) return NS_ERROR_NULL_POINTER;

      // We should delete the row instead,
      //  but first check if its the only row left
      //  so we can delete the entire table
      PRInt32 rowCount, colCount;
      res = GetTableSize(table, rowCount, colCount);
      if (NS_FAILED(res)) return res;
      
      if (rowCount == 1)
        return DeleteTable(table, selection);
    
      // We need to call DeleteTableRow to handle cells with rowspan 
      res = DeleteTableRow(1);
      if (NS_FAILED(res)) return res;
    } 
    else
    {
      // More than 1 cell in the row

      // We clear the selection to avoid problems when nodes in the selection are deleted,
      // The setCaret object will call SetCaretAfterTableEdit in it's destructor
      selection->ClearSelection();
      nsSetCaretAfterTableEdit setCaret(this, table, startRowIndex, startColIndex, ePreviousColumn);
      nsAutoTxnsConserveSelection dontChangeSelection(this);

      res = DeleteNode(cell);
      // If we fail, don't try to delete any more cells???
      if (NS_FAILED(res)) return res;
    }
  }
  return res;
}

NS_IMETHODIMP
nsHTMLEditor::DeleteTableCellContents()
{
  nsCOMPtr<nsIDOMSelection> selection;
  nsCOMPtr<nsIDOMElement> table;
  nsCOMPtr<nsIDOMElement> cell;
  nsCOMPtr<nsIDOMNode> cellParent;
  PRInt32 cellOffset, startRowIndex, startColIndex;
  nsresult res = NS_OK;

  res = GetCellContext(selection, table, cell, cellParent, cellOffset, startRowIndex, startColIndex);
  if (NS_FAILED(res)) return res;
  // Don't fail if no cell found
  if (!cell) return NS_EDITOR_ELEMENT_NOT_FOUND;

  // We clear the selection to avoid problems when nodes in the selection are deleted,
  selection->ClearSelection();
  nsSetCaretAfterTableEdit setCaret(this, table, startRowIndex, startColIndex, ePreviousColumn);
  //Don't let Rules System change the selection
  nsAutoTxnsConserveSelection dontChangeSelection(this);

  nsAutoEditBatch beginBatching(this);

  nsCOMPtr<nsIDOMNodeList> nodeList;
  res = cell->GetChildNodes(getter_AddRefs(nodeList));
  if (NS_FAILED(res)) return res;

  if (!nodeList) return NS_ERROR_FAILURE;
  PRUint32 nodeListLength; 
  res = nodeList->GetLength(&nodeListLength);
  if (NS_FAILED(res)) return res;

  for (PRUint32 i = 0; i < nodeListLength; i++)
  {
    
    nsCOMPtr<nsIDOMNode> child;
    res = cell->GetLastChild(getter_AddRefs(child));
    if (NS_FAILED(res)) return res;
    res = DeleteNode(child);
    if (NS_FAILED(res)) return res;
  }
  return res;
}

NS_IMETHODIMP
nsHTMLEditor::DeleteTableColumn(PRInt32 aNumber)
{
  nsCOMPtr<nsIDOMSelection> selection;
  nsCOMPtr<nsIDOMElement> table;
  nsCOMPtr<nsIDOMElement> cell;
  nsCOMPtr<nsIDOMNode> cellParent;
  PRInt32 cellOffset, startRowIndex, startColIndex;
  PRInt32 rowCount, colCount;
  nsresult res = NS_OK;

  res = GetCellContext(selection, table, cell, cellParent, cellOffset, startRowIndex, startColIndex);
  if (NS_FAILED(res)) return res;
  // Don't fail if no cell found
  if (!cell) return NS_EDITOR_ELEMENT_NOT_FOUND;

  res = GetTableSize(table, rowCount, colCount);
  if (NS_FAILED(res)) return res;

  // Shortcut the case of deleting all columns in table
  if(startColIndex == 0 && aNumber >= colCount)
    return DeleteTable(table, selection);

  nsAutoEditBatch beginBatching(this);

  // Check for counts too high
  aNumber = PR_MIN(aNumber,(colCount-startColIndex));

  // Scan through cells in row to do rowspan adjustments
  nsCOMPtr<nsIDOMElement> curCell;
  PRInt32 curStartRowIndex, curStartColIndex, rowSpan, colSpan, actualRowSpan, actualColSpan;
  PRBool  isSelected;
  PRInt32 rowIndex = 0;

  for (PRInt32 i = 0; i < aNumber; i++)
  {
    do {
      res = GetCellDataAt(table, rowIndex, startColIndex, *getter_AddRefs(curCell),
                          curStartRowIndex, curStartColIndex, rowSpan, colSpan, 
                          actualRowSpan, actualColSpan, isSelected);

      if (NS_FAILED(res)) return res;

      if (curCell)
      {
        // This must always be >= 1
        NS_ASSERTION((actualRowSpan > 0),"Effective ROWSPAN = 0 in DeleteTableColumn");

        // Find cells that don't start in column we are deleting
        if (curStartColIndex < startColIndex || colSpan > 1 || colSpan == 0)
        {
          // We have a cell spanning this location
          // Decrease its colspan to keep table rectangular,
          // but if colSpan=0, it will adjust automatically
          if (colSpan > 0)
          {
            NS_ASSERTION((colSpan > 1),"Bad COLSPAN in DeleteTableColumn");
            SetColSpan(curCell,colSpan-1);
          }
          if (curStartColIndex == startColIndex)
          {
            // Cell is in column to be deleted, 
            // but delete contents of cell instead of cell itself
            selection->Collapse(curCell,0);
            DeleteTableCellContents();
          }
          // To next cell in column
          rowIndex += actualRowSpan;
        } else {
          // Delete the cell
          if (1 == GetNumberOfCellsInRow(table, rowIndex))
          {
            // Only 1 cell in row - delete the row
            nsCOMPtr<nsIDOMElement> parentRow;
            res = GetElementOrParentByTagName("tr", cell, getter_AddRefs(parentRow));
            if (NS_FAILED(res)) return res;
            if(!parentRow) return NS_ERROR_NULL_POINTER;

            //  But first check if its the only row left
            //  so we can delete the entire table
            //  (This should never happen but it's the safe thing to do)
            PRInt32 rowCount, colCount;
            res = GetTableSize(table, rowCount, colCount);
            if (NS_FAILED(res)) return res;

            if (rowCount == 1)
              return DeleteTable(table, selection);
      
            // Delete the row by placing caret in cell we were to delete
            // We need to call DeleteTableRow to handle cells with rowspan 
            selection->Collapse(cell,0);
            res = DeleteTableRow(1);
            if (NS_FAILED(res)) return res;

            // Note that we don't incremenet rowIndex
            // since a row was deleted and "next" 
            // row now has current rowIndex
          } else {

            selection->ClearSelection();
            nsSetCaretAfterTableEdit setCaret(this, table, curStartRowIndex, curStartColIndex, ePreviousColumn);
            //Don't let Rules System change the selection
            nsAutoTxnsConserveSelection dontChangeSelection(this);

            res = DeleteNode(curCell);
            if (NS_FAILED(res)) return res;

            //Skipover any rows spanned by this cell
            rowIndex += actualRowSpan;
          }
        }
      }
    } while (curCell);    
  }
  return res;
}

NS_IMETHODIMP
nsHTMLEditor::DeleteTableRow(PRInt32 aNumber)
{
  nsCOMPtr<nsIDOMSelection> selection;
  nsCOMPtr<nsIDOMElement> table;
  nsCOMPtr<nsIDOMElement> cell;
  nsCOMPtr<nsIDOMNode> cellParent;
  PRInt32 cellOffset, startRowIndex, startColIndex;
  PRInt32 rowCount, colCount;
  nsresult res = NS_OK;

  res = GetCellContext(selection, table, cell, cellParent, cellOffset, startRowIndex, startColIndex);
  if (NS_FAILED(res)) return res;
  // Don't fail if no cell found
  if (!cell) return NS_EDITOR_ELEMENT_NOT_FOUND;

  res = GetTableSize(table, rowCount, colCount);
  if (NS_FAILED(res)) return res;

  // Shortcut the case of deleting all rows in table
  if(startRowIndex == 0 && aNumber >= rowCount)
    return DeleteTable(table, selection);

  nsAutoEditBatch beginBatching(this);

  // Check for counts too high
  aNumber = PR_MIN(aNumber,(rowCount-startRowIndex));

  // We clear the selection to avoid problems when nodes in the selection are deleted,
  // Be sure to set it correctly later (in SetCaretAfterTableEdit)!
  selection->ClearSelection();
  
  // Scan through cells in row to do rowspan adjustments
  nsCOMPtr<nsIDOMElement> curCell;
  PRInt32 curStartRowIndex, curStartColIndex, rowSpan, colSpan, actualRowSpan, actualColSpan;
  PRBool  isSelected;
  PRInt32 colIndex = 0;
  do {
    res = GetCellDataAt(table, startRowIndex, colIndex, *getter_AddRefs(curCell),
                        curStartRowIndex, curStartColIndex, rowSpan, colSpan, 
                        actualRowSpan, actualColSpan, isSelected);
    
    // We don't fail if we don't find a cell, so this must be real bad
    if(NS_FAILED(res)) return res;

    // Find cells that don't start in row we are deleting
    if (curCell)
    {
      //Real colspan must always be >= 1
      NS_ASSERTION((actualColSpan > 0),"Effective COLSPAN = 0 in DeleteTableRow");
      if (curStartRowIndex < startRowIndex)
      {
        // We have a cell spanning this location
        // Decrease its rowspan to keep table rectangular
        //  but we don't need to do this if rowspan=0,
        //  since it will automatically adjust
        if (rowSpan > 0)
          SetRowSpan(curCell, PR_MAX((startRowIndex - curStartRowIndex), actualRowSpan - aNumber));
      }
      // Skip over locations spanned by this cell
      colIndex += actualColSpan;
    }
  } while (curCell);

  for (PRInt32 i = 0; i < aNumber; i++)
  {
    //TODO: To minimize effect of deleting cells that have rowspan > 1:
    //      Scan for rowspan > 1 and insert extra emtpy cells in 
    //      appropriate rows to take place of spanned regions.
    //      (Hard part is finding appropriate neighbor cell before/after in correct row)

    // Delete the row
    nsCOMPtr<nsIDOMElement> parentRow;
    res = GetElementOrParentByTagName("tr", cell, getter_AddRefs(parentRow));
    if (NS_SUCCEEDED(res) && parentRow)
      res = DeleteNode(parentRow);
    if (NS_FAILED(res))
      startRowIndex++;

    res = GetCellAt(table, startRowIndex, startColIndex, *getter_AddRefs(cell));
    if(!cell)
      break;
  }
  return res;
}



NS_IMETHODIMP 
nsHTMLEditor::SelectTable()
{
  nsCOMPtr<nsIDOMElement> table;
  nsresult res = NS_ERROR_FAILURE;
  res = GetElementOrParentByTagName("table", nsnull, getter_AddRefs(table));
  if (NS_FAILED(res)) return res;
  // Don't fail if we didn't find a table
  if (!table) return NS_OK;

  nsCOMPtr<nsIDOMNode> tableNode = do_QueryInterface(table);
  if (tableNode)
  {
    res = ClearSelection();
    if (NS_SUCCEEDED(res))
      res = AppendNodeToSelectionAsRange(table);
  }
  return res;
}

NS_IMETHODIMP 
nsHTMLEditor::SelectTableCell()
{
  nsCOMPtr<nsIDOMElement> cell;
  nsresult res = NS_ERROR_FAILURE;
  res = GetElementOrParentByTagName("td", nsnull, getter_AddRefs(cell));
  if (NS_FAILED(res)) return res;
  // Don't fail if we didn't find a table
  if (!cell) return NS_EDITOR_ELEMENT_NOT_FOUND;

  nsCOMPtr<nsIDOMNode> cellNode = do_QueryInterface(cell);
  if (cellNode)
  {
    res = ClearSelection();
    if (NS_SUCCEEDED(res))
      res = AppendNodeToSelectionAsRange(cellNode);
  }
  return res;
}

NS_IMETHODIMP 
nsHTMLEditor::SelectAllTableCells()
{
  nsresult res = NS_OK;
  return res;
}

NS_IMETHODIMP 
nsHTMLEditor::SelectTableRow()
{
  nsresult res = NS_OK;
  return res;
}

NS_IMETHODIMP 
nsHTMLEditor::SelectTableColumn()
{
  nsresult res = NS_OK;
  return res;
}

NS_IMETHODIMP 
nsHTMLEditor::JoinTableCells()
{
  nsCOMPtr<nsIDOMSelection> selection;
  nsCOMPtr<nsIDOMElement> table;
  nsCOMPtr<nsIDOMElement> cell;
  nsCOMPtr<nsIDOMNode> cellParent;
  PRInt32 cellOffset, startRowIndex, startColIndex, rowSpan, colSpan, actualRowSpan, actualColSpan;
  PRBool  isSelected;

  PRInt32 rowCount, colCount;
  nsresult res = GetTableSize(table, rowCount, colCount);
  if (NS_FAILED(res)) return res;

  res = GetCellContext(selection, table, cell, cellParent, cellOffset, startRowIndex, startColIndex);
  if (NS_FAILED(res)) return res;
  if(!cell) return NS_ERROR_NULL_POINTER;

  res = GetCellDataAt(table, startRowIndex, startColIndex, *getter_AddRefs(cell),
                      startRowIndex, startColIndex, rowSpan, colSpan, 
                      actualRowSpan, actualColSpan, isSelected);
  if (NS_FAILED(res)) return res;
  if(!cell) return NS_ERROR_NULL_POINTER;

  //*** Initial test: just merge with cell to the right

  nsCOMPtr<nsIDOMElement> cell2;
  PRInt32 startRowIndex2, startColIndex2, rowSpan2, colSpan2, actualRowSpan2, actualColSpan2;
  PRBool  isSelected2;
  res = GetCellDataAt(table, startRowIndex, startColIndex+actualColSpan, *getter_AddRefs(cell2),
                      startRowIndex2, startColIndex2, rowSpan2, colSpan2, 
                      actualRowSpan2, actualColSpan2, isSelected2);
  if (NS_FAILED(res)) return res;
  
  nsAutoEditBatch beginBatching(this);
  //Don't let Rules System change the selection
  nsAutoTxnsConserveSelection dontChangeSelection(this);

  nsCOMPtr<nsIDOMNode> parentNode = do_QueryInterface(cellParent);
  nsCOMPtr<nsIDOMNode> cellNode = do_QueryInterface(cell);
  nsCOMPtr<nsIDOMNode> cellNode2 = do_QueryInterface(cell2);
  if(cellNode && cellNode2 && parentNode)
  {
    PRInt32 insertIndex = 0;

    // Get index of last child in target cell
    nsCOMPtr<nsIDOMNodeList> childNodes;
    res = cellNode->GetChildNodes(getter_AddRefs(childNodes));
    if ((NS_SUCCEEDED(res)) && (childNodes))
    {
      // Start inserting just after last child
      PRUint32 len;
      res = childNodes->GetLength(&len);
      if (NS_FAILED(res)) return res;
      insertIndex = (PRInt32)len;
    }

    // Move content from cell2 to cell
    nsCOMPtr<nsIDOMNode> cellChild;
    res = cell2->GetFirstChild(getter_AddRefs(cellChild));
    if (NS_FAILED(res)) return res;
    while (cellChild)
    {
      nsCOMPtr<nsIDOMNode> nextChild;
      res = cellChild->GetNextSibling(getter_AddRefs(nextChild));
      if (NS_FAILED(res)) return res;

      res = DeleteNode(cellChild);
      if (NS_FAILED(res)) return res;

      res = nsEditor::InsertNode(cellChild, cellNode, insertIndex);
      if (NS_FAILED(res)) return res;

      cellChild = nextChild;
      insertIndex++;
    }
    // Reset target cell's spans
    res = SetColSpan(cell, actualColSpan+actualColSpan2);
    if (NS_FAILED(res)) return res;
    
    // Delete cells whose contents were moved
    res = DeleteNode(cell2);
    if (NS_FAILED(res)) return res;
    
  }
  return res;
}

NS_IMETHODIMP 
nsHTMLEditor::NormalizeTable(nsIDOMElement *aTable)
{
  nsCOMPtr<nsIDOMElement> table;
  nsresult res = NS_ERROR_FAILURE;
  res = GetElementOrParentByTagName("table", aTable, getter_AddRefs(table));
  if (NS_FAILED(res)) return res;
  // Don't fail if we didn't find a table
  if (!table)         return NS_OK;

  PRInt32 rowCount, colCount, rowIndex, colIndex;
  res = GetTableSize(table, rowCount, colCount);
  if (NS_FAILED(res)) return res;

  nsAutoEditBatch beginBatching(this);

  nsCOMPtr<nsIDOMElement> cell;
  PRInt32 startRowIndex, startColIndex, rowSpan, colSpan, actualRowSpan, actualColSpan;
  PRBool  isSelected;

#if 0
// This isn't working yet -- layout errors contribute!

  // First scan all cells in row to detect bad rowspan values
  for(rowIndex = 0; rowIndex < rowCount; rowIndex++)
  {
    PRInt32 minRowSpan = 0x7fffffff; //XXX: Shouldn't there be a define somewhere for MaxInt for PRInt32
    for(colIndex = 0; colIndex < colCount; colIndex++)
    {
      res = GetCellDataAt(aTable, rowIndex, colIndex, *getter_AddRefs(cell),
                          startRowIndex, startColIndex, rowSpan, colSpan, 
                          actualRowSpan, actualColSpan, isSelected);
      // NOTE: This is a *real* failure. 
      // GetCellDataAt passes if cell is missing from cellmap
      if(NS_FAILED(res)) return res;
      if(cell && rowSpan > 0 && rowSpan < minRowSpan)
        minRowSpan = rowSpan;
    }
    if(minRowSpan > 1)
    {
      PRInt32 spanDiff = minRowSpan - 1;
      for(colIndex = 0; colIndex < colCount; colIndex++)
      {
        res = GetCellDataAt(aTable, rowIndex, colIndex, *getter_AddRefs(cell),
                            startRowIndex, startColIndex, rowSpan, colSpan, 
                            actualRowSpan, actualColSpan, isSelected);
        // NOTE: This is a *real* failure. 
        // GetCellDataAt passes if cell is missing from cellmap
        if(NS_FAILED(res)) return res;
        // Fixup rowspans for cells starting in current row
        if(cell && rowSpan > 0 &&
           startRowIndex == rowIndex && 
           startColIndex ==  colIndex )
        {
          // Set rowspan so there's at least one cell with ROWSPAN=1
          SetRowSpan(cell, rowSpan-spanDiff);
        }
      }
    }
  }
  // Get Table size again in case above changed anything
  res = GetTableSize(table, rowCount, colCount);
  if (NS_FAILED(res)) return res;
#endif  

  // Fill in missing cellmap locations with empty cells
  for(rowIndex = 0; rowIndex < rowCount; rowIndex++)
  {
    nsCOMPtr<nsIDOMElement> previousCellInRow;

    for(colIndex = 0; colIndex < colCount; colIndex++)
    {
      res = GetCellDataAt(aTable, rowIndex, colIndex, *getter_AddRefs(cell),
                          startRowIndex, startColIndex, rowSpan, colSpan, 
                          actualRowSpan, actualColSpan, isSelected);
      // NOTE: This is a *real* failure. 
      // GetCellDataAt passes if cell is missing from cellmap
      if(NS_FAILED(res)) return res;
      if (!cell)
      {
        //We are missing a cell at a cellmap location
#ifdef DEBUG
        printf("NormalizeTable found missing cell at row=%d, col=%d\n", rowIndex, colIndex);
#endif
        // Add a cell after the previous Cell in the current row
        if(previousCellInRow)
        {
          // Insert a new cell after (PR_TRUE), and return the new cell to us
          res = InsertCell(previousCellInRow, 1, 1, PR_TRUE, getter_AddRefs(cell));          
          if (NS_FAILED(res)) return res;

          // Set this so we use returned new "cell" to set previousCellInRow below
          if(cell)
            startRowIndex = rowIndex;   
        } else {
          // We don't have any cells in this row -- We are really messed up!
#ifdef DEBUG
          printf("NormalizeTable found no cells in row=%d, col=%d\n", rowIndex, colIndex);
#endif
          return NS_ERROR_FAILURE;
        }
      }
      // Save the last cell found in the same row we are scanning
      if(startRowIndex == rowIndex)
      {
        previousCellInRow = cell;
      }
    }
  }
  return res;
}

NS_IMETHODIMP 
nsHTMLEditor::GetCellIndexes(nsIDOMElement *aCell, PRInt32 &aRowIndex, PRInt32 &aColIndex)
{
  nsresult res=NS_ERROR_NOT_INITIALIZED;
  aColIndex=0; // initialize out params
  aRowIndex=0;
  if (!aCell)
  {
    // Get the selected cell or the cell enclosing the selection anchor
    nsCOMPtr<nsIDOMElement> cell;
    res = GetElementOrParentByTagName("td", nsnull, getter_AddRefs(cell));
    if (NS_SUCCEEDED(res) && cell)
      aCell = cell;
    else
      return NS_ERROR_FAILURE;
  }

  res = NS_ERROR_FAILURE;        // we return an error unless we get the index
  nsISupports *layoutObject=nsnull; // frames are not ref counted, so don't use an nsCOMPtr

  res = nsHTMLEditor::GetLayoutObject(aCell, &layoutObject);

  if ((NS_SUCCEEDED(res)) && (nsnull!=layoutObject))
  { // get the table cell interface from the frame
    nsITableCellLayout *cellLayoutObject=nsnull; // again, frames are not ref-counted
  
    res = layoutObject->QueryInterface(NS_GET_IID(nsITableCellLayout), (void**)(&cellLayoutObject));
    if ((NS_SUCCEEDED(res)) && (nsnull!=cellLayoutObject))
    {
      res = cellLayoutObject->GetCellIndexes(aRowIndex, aColIndex);
    }
  }
  return res;
}

NS_IMETHODIMP
nsHTMLEditor::GetTableLayoutObject(nsIDOMElement* aTable, nsITableLayout **tableLayoutObject)
{
  *tableLayoutObject=nsnull;
  if (!aTable)
    return NS_ERROR_NOT_INITIALIZED;
  
  // frames are not ref counted, so don't use an nsCOMPtr
  nsISupports *layoutObject=nsnull;
  nsresult res = nsHTMLEditor::GetLayoutObject(aTable, &layoutObject); 
  if ((NS_SUCCEEDED(res)) && (nsnull!=layoutObject)) 
  { // get the table interface from the frame 
    
    res = layoutObject->QueryInterface(NS_GET_IID(nsITableLayout), 
                            (void**)(tableLayoutObject)); 
  }
  return res;
}

//Return actual number of cells (a cell with colspan > 1 counts as just 1)
PRBool nsHTMLEditor::GetNumberOfCellsInRow(nsIDOMElement* aTable, PRInt32 rowIndex)
{
  PRInt32 cellCount = 0;
  nsCOMPtr<nsIDOMElement> cell;
  PRInt32 colIndex = 0;
  nsresult res;
  do {
    PRInt32 startRowIndex, startColIndex, rowSpan, colSpan, actualRowSpan, actualColSpan;
    PRBool  isSelected;
    res = GetCellDataAt(aTable, rowIndex, colIndex, *getter_AddRefs(cell),
                        startRowIndex, startColIndex, rowSpan, colSpan, 
                        actualRowSpan, actualColSpan, isSelected);
    if (NS_FAILED(res)) return res;
    if (cell)
    {
      // Only count cells that start in row we are working with
      if (startRowIndex == rowIndex)
        cellCount++;
      
      //Next possible location for a cell
      colIndex += actualColSpan;
    }
    else
      colIndex++;

  } while (cell);

  return cellCount;
}

/* Not scriptable: For convenience in C++ 
   Use GetTableRowCount and GetTableColumnCount from JavaScript
*/
NS_IMETHODIMP
nsHTMLEditor::GetTableSize(nsIDOMElement *aTable, PRInt32& aRowCount, PRInt32& aColCount)
{
  nsresult res = NS_ERROR_FAILURE;
  aRowCount = 0;
  aColCount = 0;
  nsCOMPtr<nsIDOMElement> table;
  // Get the selected talbe or the table enclosing the selection anchor
  res = GetElementOrParentByTagName("table", aTable, getter_AddRefs(table));
  if (NS_FAILED(res)) return res;
  if (!table)         return NS_ERROR_FAILURE;
  
  // frames are not ref counted, so don't use an nsCOMPtr
  nsITableLayout *tableLayoutObject;
  res = GetTableLayoutObject(table.get(), &tableLayoutObject);
  if (NS_FAILED(res)) return res;
  if (!tableLayoutObject)
    return NS_ERROR_FAILURE;

  return tableLayoutObject->GetTableSize(aRowCount, aColCount); 
}

NS_IMETHODIMP 
nsHTMLEditor::GetCellDataAt(nsIDOMElement* aTable, PRInt32 aRowIndex, PRInt32 aColIndex, nsIDOMElement* &aCell, 
                            PRInt32& aStartRowIndex, PRInt32& aStartColIndex, 
                            PRInt32& aRowSpan, PRInt32& aColSpan, 
                            PRInt32& aActualRowSpan, PRInt32& aActualColSpan, 
                            PRBool& aIsSelected)
{
  nsresult res=NS_ERROR_FAILURE;
  aCell = nsnull;
  aStartRowIndex = 0;
  aStartColIndex = 0;
  aRowSpan = 0;
  aColSpan = 0;
  aActualRowSpan = 0;
  aActualColSpan = 0;
  aIsSelected = PR_FALSE;

  if (!aTable)
  {
    // Get the selected table or the table enclosing the selection anchor
    nsCOMPtr<nsIDOMElement> table;
    res = GetElementOrParentByTagName("table", nsnull, getter_AddRefs(table));
    if (NS_FAILED(res)) return res;
    if (table)
      aTable = table;
    else
      return NS_ERROR_FAILURE;
  }
  
  // frames are not ref counted, so don't use an nsCOMPtr
  nsITableLayout *tableLayoutObject;
  res = GetTableLayoutObject(aTable, &tableLayoutObject);
  if (NS_FAILED(res)) return res;
  if (!tableLayoutObject) return NS_ERROR_FAILURE;

  // Note that this returns NS_TABLELAYOUT_CELL_NOT_FOUND when
  //  the index(es) are out of bounds
  res = tableLayoutObject->GetCellDataAt(aRowIndex, aColIndex, aCell, 
                                         aStartRowIndex, aStartColIndex,
                                         aRowSpan, aColSpan, 
                                         aActualRowSpan, aActualColSpan, 
                                         aIsSelected);
  // Convert to editor's generic "not found" return value
  if (res == NS_TABLELAYOUT_CELL_NOT_FOUND) res = NS_EDITOR_ELEMENT_NOT_FOUND;
  return res;
}

// When all you want is the cell
NS_IMETHODIMP 
nsHTMLEditor::GetCellAt(nsIDOMElement* aTable, PRInt32 aRowIndex, PRInt32 aColIndex, nsIDOMElement* &aCell)
{
  PRInt32 startRowIndex, startColIndex, rowSpan, colSpan, actualRowSpan, actualColSpan;
  PRBool  isSelected;
  return GetCellDataAt(aTable, aRowIndex, aColIndex, aCell, 
                       startRowIndex, startColIndex, rowSpan, colSpan, 
                       actualRowSpan, actualColSpan, isSelected);
}

NS_IMETHODIMP
nsHTMLEditor::GetCellContext(nsCOMPtr<nsIDOMSelection> &aSelection,
                             nsCOMPtr<nsIDOMElement> &aTable, nsCOMPtr<nsIDOMElement> &aCell, 
                             nsCOMPtr<nsIDOMNode> &aCellParent, PRInt32& aCellOffset, 
                             PRInt32& aRow, PRInt32& aCol)
{
  nsresult res = nsEditor::GetSelection(getter_AddRefs(aSelection));
  if (NS_FAILED(res)) return res;
  if (!aSelection) return NS_ERROR_FAILURE;

  // Find the first selected cell
  res = GetFirstSelectedCell(getter_AddRefs(aCell));
  if (!aCell)
  {
    //If a cell wasn't selected, then assume the selection is INSIDE 
    //  and use anchor node to search up to the containing cell
    nsCOMPtr<nsIDOMNode> anchorNode;

    res = aSelection->GetAnchorNode(getter_AddRefs(anchorNode));
    if (NS_FAILED(res)) return res;
    if (!anchorNode)    return NS_ERROR_FAILURE;

    // Get the cell enclosing the selection anchor
    res = GetElementOrParentByTagName("td", anchorNode, getter_AddRefs(aCell));
    if (NS_FAILED(res)) return res;
    if (!aCell)         return NS_ERROR_FAILURE;
  }
  // Get containing table and the immediate parent of the cell
  res = GetElementOrParentByTagName("table", aCell, getter_AddRefs(aTable));
  if (NS_FAILED(res)) return res;
  if (!aTable)        return NS_ERROR_FAILURE;

  res = aCell->GetParentNode(getter_AddRefs(aCellParent));
  if (NS_FAILED(res)) return res;
  if (!aCellParent)   return NS_ERROR_FAILURE;

  // Get current cell location so we can put caret back there when done
  res = GetCellIndexes(aCell, aRow, aCol);
  if(NS_FAILED(res)) return res;

  // And the parent and offsets needed to do an insert
  return GetChildOffset(aCell, aCellParent, aCellOffset);
}

NS_IMETHODIMP 
nsHTMLEditor::GetFirstSelectedCell(nsIDOMElement **aCell)
{
  if (!aCell) return NS_ERROR_NULL_POINTER;
  *aCell = nsnull;

  nsCOMPtr<nsIDOMSelection> selection;
  nsresult res = nsEditor::GetSelection(getter_AddRefs(selection));
  if (NS_FAILED(res)) return res;
  if (!selection) return NS_ERROR_FAILURE;

//TODO: Replace this with code below new "table cell mode" flag is implemented

  nsCOMPtr<nsIEnumerator> enumerator;
  res = selection->GetEnumerator(getter_AddRefs(enumerator));
  
  if (NS_FAILED(res)) return res;
  if (!enumerator)    return NS_ERROR_FAILURE;
  enumerator->First(); 
  nsCOMPtr<nsISupports> currentItem;
  res = enumerator->CurrentItem(getter_AddRefs(currentItem));
  if ((NS_SUCCEEDED(res)) && currentItem)
  {
    nsCOMPtr<nsIDOMRange> range( do_QueryInterface(currentItem) );
    nsCOMPtr<nsIContentIterator> iter;
    res = nsComponentManager::CreateInstance(kCContentIteratorCID, nsnull,
                                                NS_GET_IID(nsIContentIterator), 
                                                getter_AddRefs(iter));
    if (NS_FAILED(res)) return res;
    if (!iter)          return NS_ERROR_FAILURE;

    iter->Init(range);
    // loop through the content iterator for each content node
    nsCOMPtr<nsIContent> content;

    while (NS_ENUMERATOR_FALSE == iter->IsDone())
    {
      res = iter->CurrentNode(getter_AddRefs(content));
      // Not likely!
      if (NS_FAILED(res)) return NS_ERROR_FAILURE;

      nsCOMPtr<nsIAtom> atom;
      content->GetTag(*getter_AddRefs(atom));
      if (atom.get() == nsIEditProperty::td ||
          atom.get() == nsIEditProperty::th )
      {
        // We found a cell   
        nsCOMPtr<nsIDOMElement> cellElement = do_QueryInterface(content);
        if (cellElement) 
        {
          *aCell = cellElement.get();
          NS_ADDREF(*aCell);
        }
        return NS_OK;
      }
      iter->Next();
    }
  }
  return NS_EDITOR_ELEMENT_NOT_FOUND;

#if 0
//TODO: Do this only after checking the new "table cell mode" flag
  // The first cell is the starting node in the first selection range
  nsCOMPtr<nsIDOMRange> firstRange;
  res = selection->GetRangeAt(0, getter_AddRefs(firstRange));
  if (NS_FAILED(res)) return res;
  if (!firstRange) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMNode> cellNode;
  res = GetFirstNodeInRange(firstRange, getter_AddRefs(cellNode));
  if (NS_FAILED(res)) return res;
  if (!cellNode) return NS_ERROR_FAILURE;
  nsCOMPtr<nsIDOMElement> cellElement = do_QueryInterface(cellNode);
  
  if (cellElement) 
  {
    *aCell = cellElement.get();
    NS_ADDREF(*aCell);
  }
  else res = NS_EDITOR_ELEMENT_NOT_FOUND;

  return res;  
#endif
}

NS_IMETHODIMP
nsHTMLEditor::SetCaretAfterTableEdit(nsIDOMElement* aTable, PRInt32 aRow, PRInt32 aCol, PRInt32 aDirection)
{
  nsresult res = NS_ERROR_NOT_INITIALIZED;
  if (!aTable) return res;

  nsCOMPtr<nsIDOMSelection>selection;
  res = nsEditor::GetSelection(getter_AddRefs(selection));
  if (NS_FAILED(res)) return res;
  
  if (!selection)
  {
#ifdef DEBUG_cmanske
    printf("Selection not found after table manipulation!\n");
#endif
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIDOMElement> cell;
  PRBool done = PR_FALSE;
  do {
    res = GetCellAt(aTable, aRow, aCol, *getter_AddRefs(cell));
    nsCOMPtr<nsIDOMNode> cellNode = do_QueryInterface(cell);
    if (NS_SUCCEEDED(res))
    {
      if (cell)
      {
        // Set the caret to just before the first child of the cell?
        // TODO: Should we really be placing the caret at the END
        //  of the cell content?
        selection->Collapse(cell, 0);
        return NS_OK;
      } else {
        // Setup index to find another cell in the 
        //   direction requested, but move in
        //   other direction if already at beginning of row or column
        switch (aDirection)
        {
          case ePreviousColumn:
            if (aCol == 0)
            {
              if (aRow > 0)
                aRow--;
              else
                done = PR_TRUE;
            }
            else
              aCol--;
            break;
          case ePreviousRow:
            if (aRow == 0)
            {
              if (aCol > 0)
                aCol--;
              else
                done = PR_TRUE;
            }
            else
              aRow--;
            break;
          default:
            done = PR_TRUE;
        }
      }
    }
    else
      break;
  } while (!done);

  // We didn't find a cell
  // Set selection to just before the table
  nsCOMPtr<nsIDOMNode> tableParent;
  PRInt32 tableOffset;
  res = aTable->GetParentNode(getter_AddRefs(tableParent));
  if(NS_SUCCEEDED(res) && tableParent)
  {
    if(NS_SUCCEEDED(GetChildOffset(aTable, tableParent, tableOffset)))
      return selection->Collapse(tableParent, tableOffset);
  }
  // Last resort: Set selection to start of doc
  // (it's very bad to not have a valid selection!)
  return SetSelectionAtDocumentStart(selection);
}

NS_IMETHODIMP 
nsHTMLEditor::GetSelectedOrParentTableElement(nsIDOMElement* &aTableElement, nsString& aTagName, PRBool &aIsSelected)
{
  aTableElement = nsnull;
  aTagName = "";
  aIsSelected = PR_FALSE;

  nsCOMPtr<nsIDOMSelection> selection;
  nsresult res = nsEditor::GetSelection(getter_AddRefs(selection));
  if (NS_FAILED(res)) return res;
  if (!selection) return NS_ERROR_FAILURE;

  nsAutoString tableName("table");
  nsAutoString trName("tr");
  nsAutoString tdName("td");

  nsCOMPtr<nsIDOMNode> anchorNode;

  // Find the first selected cell
  // TODO: Handle multiple cells selected!
  nsCOMPtr<nsIDOMElement> firstCell;  
 
  nsCOMPtr<nsIDOMElement> tableElement;
  res = GetFirstSelectedCell(getter_AddRefs(tableElement));
  if(NS_FAILED(res)) return res;
  if (tableElement)
  {
    aTagName = tdName;
    aIsSelected = PR_TRUE;
    goto SET_RETURN_ELEMENT;
  }

  // See if table or row is selected
  res = GetSelectedElement(tableName, getter_AddRefs(tableElement));
  if(NS_FAILED(res)) return res;
  if (tableElement)
  {
    aTagName = tableName;
    aIsSelected = PR_TRUE;
    goto SET_RETURN_ELEMENT;
  }
  res = GetSelectedElement(trName, getter_AddRefs(tableElement));
  if(NS_FAILED(res)) return res;
  if (tableElement)
  {
    aTagName = trName;
    aIsSelected = PR_TRUE;
    goto SET_RETURN_ELEMENT;
  }

  // Look for a table cell parent
  res = selection->GetAnchorNode(getter_AddRefs(anchorNode));
  if (NS_FAILED(res)) return res;
  if (!anchorNode)    return NS_ERROR_FAILURE;

  res = GetElementOrParentByTagName(tdName, anchorNode, getter_AddRefs(tableElement));
  if(NS_FAILED(res)) return res;
  if (tableElement)
  {
    aTagName = tdName;
SET_RETURN_ELEMENT:
    aTableElement = tableElement.get();
    NS_ADDREF(aTableElement);
  }
  return res;
}
