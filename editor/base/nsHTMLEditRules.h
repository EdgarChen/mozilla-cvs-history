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

#ifndef nsHTMLEditRules_h__
#define nsHTMLEditRules_h__

#include "nsTextEditRules.h"
#include "nsISupportsArray.h"
#include "nsCOMPtr.h"

class nsISupportsArray;
class nsVoidArray;

class nsHTMLEditRules : public nsTextEditRules
{
public:

            nsHTMLEditRules();
  virtual   ~nsHTMLEditRules();

  // nsEditRules methods
  NS_IMETHOD WillDoAction(nsIDOMSelection *aSelection, nsRulesInfo *aInfo, PRBool *aCancel);
  NS_IMETHOD DidDoAction(nsIDOMSelection *aSelection, nsRulesInfo *aInfo, nsresult aResult);

protected:

  enum RulesEndpoint
  {
    kStart,
    kEnd
  };


  // nsHTMLEditRules implementation methods
  nsresult WillInsertText(nsIDOMSelection  *aSelection, 
                            PRBool         *aCancel,
                            PlaceholderTxn **aTxn,
                            const nsString *inString,
                            nsString       *outString,
                            TypeInState     typeInState,
                            PRInt32         aMaxLength);
  nsresult WillInsertBreak(nsIDOMSelection *aSelection, PRBool *aCancel);
  nsresult WillDeleteSelection(nsIDOMSelection *aSelection, nsIEditor::ESelectionCollapseDirection aAction, PRBool *aCancel);
  nsresult WillMakeList(nsIDOMSelection *aSelection, PRBool aOrderd, PRBool *aCancel);
  nsresult WillRemoveList(nsIDOMSelection *aSelection, PRBool aOrderd, PRBool *aCancel);
  nsresult WillIndent(nsIDOMSelection *aSelection, PRBool *aCancel);
  nsresult WillOutdent(nsIDOMSelection *aSelection, PRBool *aCancel);
  nsresult WillAlign(nsIDOMSelection *aSelection, const nsString *alignType, PRBool *aCancel);
  nsresult WillMakeBasicBlock(nsIDOMSelection *aSelection, const nsString *aBlockType, PRBool *aCancel);

  nsresult InsertTab(nsIDOMSelection *aSelection, PRBool *aCancel, PlaceholderTxn **aTxn, nsString *outString);
  nsresult InsertSpace(nsIDOMSelection *aSelection, PRBool *aCancel, PlaceholderTxn **aTxn, nsString *outString);

  nsresult ReturnInHeader(nsIDOMSelection *aSelection, nsIDOMNode *aHeader, nsIDOMNode *aTextNode, PRInt32 aOffset);
  nsresult ReturnInParagraph(nsIDOMSelection *aSelection, nsIDOMNode *aHeader, nsIDOMNode *aTextNode, PRInt32 aOffset, PRBool *aCancel);
  nsresult ReturnInListItem(nsIDOMSelection *aSelection, nsIDOMNode *aHeader, nsIDOMNode *aTextNode, PRInt32 aOffset);

  // helper methods
  static nsresult GetTabAsNBSPs(nsString *outString);
  static nsresult GetTabAsNBSPsAndSpace(nsString *outString);
    
  static PRBool IsHeader(nsIDOMNode *aNode);
  static PRBool IsParagraph(nsIDOMNode *aNode);
  static PRBool IsListItem(nsIDOMNode *aNode);
  static PRBool IsList(nsIDOMNode *aNode);
  static PRBool IsUnorderedList(nsIDOMNode *aNode);
  static PRBool IsOrderedList(nsIDOMNode *aNode);
  static PRBool IsBreak(nsIDOMNode *aNode);
  static PRBool IsBody(nsIDOMNode *aNode);
  static PRBool IsBlockquote(nsIDOMNode *aNode);
  static PRBool IsDiv(nsIDOMNode *aNode);
  static PRBool IsMailCite(nsIDOMNode *aNode);
  
  static PRBool InBody(nsIDOMNode *aNode);
  
  nsresult IsEmptyBlock(nsIDOMNode *aNode, PRBool *outIsEmptyBlock);
  nsresult IsEmptyNode(nsIDOMNode *aNode, PRBool *outIsEmptyNode);
  PRBool IsFirstNode(nsIDOMNode *aNode);
  PRBool IsLastNode(nsIDOMNode *aNode);

  nsresult GetPromotedPoint(RulesEndpoint aWhere, nsIDOMNode *aNode, PRInt32 aOffset, 
                            PRInt32 actionID, nsCOMPtr<nsIDOMNode> *outNode, PRInt32 *outOffset);
  nsresult GetPromotedRanges(nsIDOMSelection *inSelection, 
                             nsCOMPtr<nsISupportsArray> *outArrayOfRanges, 
                             PRInt32 inOperationType);
  nsresult GetNodesForOperation(nsISupportsArray *inArrayOfRanges, 
                                   nsCOMPtr<nsISupportsArray> *outArrayOfNodes, 
                                   PRInt32 inOperationType);
  nsresult GetChildNodesForOperation(nsIDOMNode *inNode, 
                                   nsCOMPtr<nsISupportsArray> *outArrayOfNodes);
  nsresult MakeTransitionList(nsISupportsArray *inArrayOfNodes, 
                                   nsVoidArray *inTransitionArray);
                                   
  nsresult ShouldMakeEmptyBlock(nsIDOMSelection *aSelection, const nsString *blockTag, PRBool *outMakeEmpty);
  nsresult ApplyBlockStyle(nsISupportsArray *arrayOfNodes, const nsString *aBlockTag);

  nsresult ReplaceContainer(nsIDOMNode *inNode, nsCOMPtr<nsIDOMNode> *outNode, const nsString &aNodeType);
  nsresult RemoveContainer(nsIDOMNode *inNode);
  nsresult InsertContainerAbove(nsIDOMNode *inNode, nsCOMPtr<nsIDOMNode> *outNode, nsString &aNodeType);

  nsresult IsFirstEditableChild( nsIDOMNode *aNode, PRBool *aOutIsFirst);
  nsresult IsLastEditableChild( nsIDOMNode *aNode, PRBool *aOutIsLast);

  nsresult JoinNodesSmart( nsIDOMNode *aNodeLeft, 
                           nsIDOMNode *aNodeRight, 
                           nsCOMPtr<nsIDOMNode> *aOutMergeParent, 
                           PRInt32 *aOutMergeOffset);

  nsresult GetTopEnclosingMailCite(nsIDOMNode *aNode, nsCOMPtr<nsIDOMNode> *aOutCiteNode);
  nsresult CleanUpSelection(nsIDOMSelection *aSelection);
  nsresult PopListItem(nsIDOMNode *aListItem, PRBool *aOutOfList);

};

#endif //nsHTMLEditRules_h__

