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
 * Copyright (C) 1998-1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include "IMETextTxn.h"
#include "nsEditor.h"
#include "nsIDOMCharacterData.h"
#include "nsIPrivateTextRange.h"
#include "nsIDOMSelection.h"
#include "nsIPresShell.h"
#include "EditAggregateTxn.h"
#include "nsLayoutCID.h"
static NS_DEFINE_IID(kRangeCID, NS_RANGE_CID);

static NS_DEFINE_IID(kIDOMSelectionIID, NS_IDOMSELECTION_IID);

nsIAtom *IMETextTxn::gIMETextTxnName = nsnull;

nsresult IMETextTxn::ClassInit()
{
  if (nsnull==gIMETextTxnName)
    gIMETextTxnName = NS_NewAtom("NS_IMETextTxn");
  return NS_OK;
}

nsresult IMETextTxn::ClassShutdown()
{
  NS_IF_RELEASE(gIMETextTxnName);
  return NS_OK;
}

IMETextTxn::IMETextTxn()
  : EditTxn()
{
  SetTransactionDescriptionID( kTransactionID );
  /* log description initialized in parent constructor */
}

IMETextTxn::~IMETextTxn()
{
  mRangeList = do_QueryInterface(nsnull);
}

NS_IMETHODIMP IMETextTxn::Init(nsIDOMCharacterData     *aElement,
                               PRUint32                 aOffset,
                               PRUint32                 aReplaceLength,
                               nsIPrivateTextRangeList *aTextRangeList,
                               const nsString          &aStringToInsert,
                               nsWeakPtr                aPresShellWeak)
{
  NS_ASSERTION(aElement, "illegal value- null ptr- aElement");
  NS_ASSERTION(aTextRangeList, "illegal value- null ptr - aTextRangeList");
  if((nsnull == aElement) || (nsnull == aTextRangeList))
     return NS_ERROR_NULL_POINTER;
  mElement = do_QueryInterface(aElement);
  mOffset = aOffset;
  mReplaceLength = aReplaceLength;
  mStringToInsert = aStringToInsert;
  mPresShellWeak = aPresShellWeak;
  mRangeList = do_QueryInterface(aTextRangeList);
  mFixed = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP IMETextTxn::Do(void)
{

#if defined(DEBUG_tague) || defined(DEBUG_ftang)
  printf("Do IME Text element = %p replace = %d len = %d\n", mElement.get(), mReplaceLength, mStringToInsert.Length());
#endif

  nsCOMPtr<nsIPresShell> ps = do_QueryReferent(mPresShellWeak);
  if (!ps) return NS_ERROR_NOT_INITIALIZED;

  // advance caret: This requires the presentation shell to get the selection.
  nsresult result = NS_OK;
  if (mReplaceLength==0) {
      result = mElement->InsertData(mOffset,mStringToInsert);
  } else {
      result = mElement->ReplaceData(mOffset,mReplaceLength,mStringToInsert);
  }
  if (NS_SUCCEEDED(result)) {
    result = CollapseTextSelection();
  }

  return result;
}

NS_IMETHODIMP IMETextTxn::Undo(void)
{
#if defined(DEBUG_tague) || defined(DEBUG_ftang)
  printf("Undo IME Text element = %p\n", mElement.get());
#endif

  nsCOMPtr<nsIPresShell> ps = do_QueryReferent(mPresShellWeak);
  if (!ps) return NS_ERROR_NOT_INITIALIZED;

  nsresult result;
  PRUint32 length = mStringToInsert.Length();
  result = mElement->DeleteData(mOffset, length);
  if (NS_SUCCEEDED(result))
  { // set the selection to the insertion point where the string was removed
    nsCOMPtr<nsIDOMSelection> selection;
    result = ps->GetSelection(SELECTION_NORMAL, getter_AddRefs(selection));
    if (NS_SUCCEEDED(result) && selection) {
      result = selection->Collapse(mElement, mOffset);
      NS_ASSERTION((NS_SUCCEEDED(result)), "selection could not be collapsed after undo of IME insert.");
    }
  }
  return result;
}

NS_IMETHODIMP IMETextTxn::Merge(PRBool *aDidMerge, nsITransaction *aTransaction)
{
  NS_ASSERTION(aDidMerge, "illegal vaule- null ptr- aDidMerge");
  NS_ASSERTION(aTransaction, "illegal vaule- null ptr- aTransaction");
  if((nsnull == aDidMerge) || (nsnull == aTransaction))
  	return NS_ERROR_NULL_POINTER;
    
  nsresult  result;
#if defined(DEBUG_tague) || defined(DEBUG_ftang)
  printf("Merge IME Text element = %p\n", mElement.get());
#endif

  //
  // check to make sure we have valid return pointers
  //
  if ((nsnull==aDidMerge) && (nsnull==aTransaction))
  {
    return NS_OK;
  }

  // 
  // check to make sure we aren't fixed, if we are then nothing get's absorbed
  //
  if (mFixed) {
    *aDidMerge = PR_FALSE;
    return NS_OK;
  }

  //
  // if aTransaction is another IMETextTxn then absorbe it
  //
  IMETextTxn*  otherTxn = nsnull;
  result = aTransaction->QueryInterface(IMETextTxn::GetCID(),(void**)&otherTxn);
  if (otherTxn && NS_SUCCEEDED(NS_OK))
  {
    //
    //  we absorbe the next IME transaction by adopting it's insert string as our own
    //
    nsIPrivateTextRangeList* newTextRangeList;
    otherTxn->GetData(mStringToInsert,&newTextRangeList);
    mRangeList = do_QueryInterface(newTextRangeList);
    *aDidMerge = PR_TRUE;
#if defined(DEBUG_tague) || defined(DEBUG_ftang)
    printf("IMETextTxn assimilated IMETextTxn:%p\n", aTransaction);
#endif
    NS_RELEASE(otherTxn);
    return NS_OK;
  }

  *aDidMerge = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP IMETextTxn::Write(nsIOutputStream *aOutputStream)
{
  NS_ASSERTION(aOutputStream, "illegal value- null ptr- aOutputStream");
  if(nsnull == aOutputStream)
  	return NS_ERROR_NULL_POINTER;
  return NS_OK;
}

NS_IMETHODIMP IMETextTxn::GetUndoString(nsString *aString)
{
  NS_ASSERTION(aString, "illegal value- null ptr- aString");
  if(nsnull == aString)
  	return NS_ERROR_NULL_POINTER;
  	
  if (nsnull!=aString)
  {
    *aString="Remove Text: ";
    *aString += mStringToInsert;
  }
  return NS_OK;
}

NS_IMETHODIMP IMETextTxn::GetRedoString(nsString *aString)
{
  NS_ASSERTION(aString, "illegal value- null ptr- aString");
  if(nsnull == aString)
  	return NS_ERROR_NULL_POINTER;
  	
  if (nsnull!=aString)
  {
    *aString="Insert Text: ";
    *aString += mStringToInsert;
  }
  return NS_OK;
}

/* ============= nsISupports implementation ====================== */

NS_IMETHODIMP
IMETextTxn::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (nsnull == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(IMETextTxn::GetCID())) {
    *aInstancePtr = (void*)(IMETextTxn*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return (EditTxn::QueryInterface(aIID, aInstancePtr));
}

/* ============ protected methods ================== */
static SelectionType TextRangeToSelection(int aTextRangeType)
{
   switch(aTextRangeType)
   {
      case nsIPrivateTextRange::TEXTRANGE_RAWINPUT:
           return SELECTION_IME_RAWINPUT;
      case nsIPrivateTextRange::TEXTRANGE_SELECTEDRAWTEXT:
           return SELECTION_IME_SELECTEDRAWTEXT;
      case nsIPrivateTextRange::TEXTRANGE_CONVERTEDTEXT:
           return SELECTION_IME_CONVERTEDTEXT;
      case nsIPrivateTextRange::TEXTRANGE_SELECTEDCONVERTEDTEXT:
           return SELECTION_IME_SELECTEDCONVERTEDTEXT;
      case nsIPrivateTextRange::TEXTRANGE_CARETPOSITION:
      default:
           return SELECTION_NORMAL;
   };
}

NS_IMETHODIMP IMETextTxn::GetData(nsString& aResult,nsIPrivateTextRangeList** aTextRangeList)
{
  NS_ASSERTION(aTextRangeList, "illegal value- null ptr- aTextRangeList");
  if(nsnull == aTextRangeList)
  	return NS_ERROR_NULL_POINTER;
  aResult = mStringToInsert;
  *aTextRangeList = mRangeList;
  return NS_OK;
}

static SelectionType sel[4]=
{
 SELECTION_IME_RAWINPUT,
 SELECTION_IME_SELECTEDRAWTEXT,
 SELECTION_IME_CONVERTEDTEXT,
 SELECTION_IME_SELECTEDCONVERTEDTEXT
};

NS_IMETHODIMP IMETextTxn::CollapseTextSelection(void)
{
    nsresult      result;
    PRUint16      textRangeListLength,selectionStart,selectionEnd,
                  textRangeType, i;
    nsIPrivateTextRange*  textRange;
    

#if defined(DEBUG_tague) || defined(DEBUG_ftang)
    PRUint16 listlen,start,stop,type;
    nsIPrivateTextRange* rangePtr;
    result = mRangeList->GetLength(&listlen);
    printf("nsIPrivateTextRangeList[%p]\n",mRangeList);
    for (i=0;i<listlen;i++) {
      (void)mRangeList->Item(i,&rangePtr);
      rangePtr->GetRangeStart(&start);
      rangePtr->GetRangeEnd(&stop);
      rangePtr->GetRangeType(&type);
      printf("range[%d] start=%d end=%d type=",i,start,stop,type);
      if (type==nsIPrivateTextRange::TEXTRANGE_RAWINPUT)
                             printf("TEXTRANGE_RAWINPUT\n");
      else if (type==nsIPrivateTextRange::TEXTRANGE_SELECTEDRAWTEXT)
                                  printf("TEXTRANGE_SELECTEDRAWTEXT\n");
      else if (type==nsIPrivateTextRange::TEXTRANGE_CONVERTEDTEXT)
                                  printf("TEXTRANGE_CONVERTEDTEXT\n");
      else if (type==nsIPrivateTextRange::TEXTRANGE_SELECTEDCONVERTEDTEXT)
                                  printf("TEXTRANGE_SELECTEDCONVERTEDTEXT\n");
      else if (type==nsIPrivateTextRange::TEXTRANGE_CARETPOSITION)
                                  printf("TEXTRANGE_CARETPOSITION\n");
      else printf("unknown constant\n");
    }
#endif
        
    //
    // run through the text range list, if any
    //
    nsCOMPtr<nsIPresShell> ps = do_QueryReferent(mPresShellWeak);
    if (!ps) return NS_ERROR_NOT_INITIALIZED;
    result = mRangeList->GetLength(&textRangeListLength);
    if(NS_FAILED(result))
        return result;
    nsCOMPtr<nsIDOMSelection> selection;
    result = ps->GetSelection(SELECTION_NORMAL, getter_AddRefs(selection));
    nsCOMPtr<nsIDOMSelection> imeSel;
    if(NS_SUCCEEDED(result))
    {
      result = selection->StartBatchChanges();
      if (NS_SUCCEEDED(result))
      {
        for(PRInt8 selIdx = 0; selIdx < 4;selIdx++)
        {
          result = ps->GetSelection(sel[selIdx], getter_AddRefs(imeSel));
            if(NS_SUCCEEDED(result))
            {
             result = imeSel->ClearSelection();
               NS_ASSERTION(NS_SUCCEEDED(result), "Cannot ClearSelection");
               // we just ignore the result and clean up the next one here
            }
        }
        PRBool setCaret=PR_FALSE;
        for(i=0;i<textRangeListLength;i++)
        {
          result = mRangeList->Item(i,&textRange);
          NS_ASSERTION(NS_SUCCEEDED(result), "cannot get item");
          if(NS_FAILED(result))
               break;

          result = textRange->GetRangeType(&textRangeType);
          NS_ASSERTION(NS_SUCCEEDED(result), "cannot get range type");
          if(NS_FAILED(result))
               break;

          result = textRange->GetRangeStart(&selectionStart);
          NS_ASSERTION(NS_SUCCEEDED(result), "cannot get range start");
          if(NS_FAILED(result))
               break;
          result = textRange->GetRangeEnd(&selectionEnd);
          NS_ASSERTION(NS_SUCCEEDED(result), "cannot get range end");
          if(NS_FAILED(result))
               break;

          if(nsIPrivateTextRange::TEXTRANGE_CARETPOSITION == textRangeType)
          {
             // Set the caret....
             result = selection->Collapse(mElement,
                      mOffset+selectionStart);
             NS_ASSERTION(NS_SUCCEEDED(result), "Cannot Collapse");
             if(NS_SUCCEEDED(result))
             setCaret = PR_TRUE;
          } else {
             // NS_ASSERTION(selectionStart != selectionEnd, "end == start");
             if(selectionStart == selectionEnd)
                continue;

             nsCOMPtr<nsIDOMRange> newRange;

             result= ps->GetSelection(TextRangeToSelection(textRangeType),
                     getter_AddRefs(imeSel));
             NS_ASSERTION(NS_SUCCEEDED(result), "Cannot get selction");
             if(NS_FAILED(result))
                break;

             result = nsComponentManager::CreateInstance(kRangeCID,
                                   nsnull,
                                   nsIDOMRange::GetIID(),
                                   getter_AddRefs(newRange));
             NS_ASSERTION(NS_SUCCEEDED(result), "Cannot create new nsIDOMRange");
             if(NS_FAILED(result))
                break;

             newRange->SetStart(mElement,mOffset+selectionStart);
             NS_ASSERTION(NS_SUCCEEDED(result), "Cannot SetStart");
             if(NS_FAILED(result))
                break;

             newRange->SetEnd(mElement,mOffset+selectionEnd);
             NS_ASSERTION(NS_SUCCEEDED(result), "Cannot SetEnd");
             if(NS_FAILED(result))
                break;

             imeSel->AddRange(newRange);
             NS_ASSERTION(NS_SUCCEEDED(result), "Cannot AddRange");
             if(NS_FAILED(result))
                break;

          } // if GetRangeEnd
        } // for textRangeListLength
        if(! setCaret) {
          // set cursor
          result = selection->Collapse(mElement,mOffset+mStringToInsert.Length());
          NS_ASSERTION(NS_SUCCEEDED(result), "Cannot Collapse");
        }
        result = selection->EndBatchChanges();
        NS_ASSERTION(NS_SUCCEEDED(result), "Cannot EndBatchChanges");
      } // if StartBatchChanges
    } // if GetSelection

    return result;
}

