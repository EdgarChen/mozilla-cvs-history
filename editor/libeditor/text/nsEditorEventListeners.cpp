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
 *   Pierre Phaneuf <pp@ludusdesign.com>
 *
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
#include "nsEditorEventListeners.h"
#include "nsEditor.h"
#include "nsVoidArray.h"
#include "nsString.h"

#include "nsIDOMEvent.h"
#include "nsIDOMNSEvent.h"
#include "nsIDOMDocument.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsIDOMElement.h"
#include "nsISelection.h"
#include "nsISelectionController.h"
#include "nsIDOMCharacterData.h"
#include "nsISupportsArray.h"
#include "nsIStringStream.h"
#include "nsIDOMKeyEvent.h"
#include "nsIDOMMouseEvent.h"
#include "nsIDOMNSUIEvent.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIPrivateTextEvent.h"
#include "nsIPrivateCompositionEvent.h"
#include "nsIEditorMailSupport.h"
#include "nsIDocumentEncoder.h"
#include "nsIDOMNSUIEvent.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsILookAndFeel.h"
#include "nsIPresContext.h"
#include "nsGUIEvent.h"
#include "nsICaret.h"
// for repainting hack only
#include "nsIView.h"
#include "nsIViewManager.h"
// end repainting hack only

// Drag & Drop, Clipboard
#include "nsIServiceManager.h"
#include "nsWidgetsCID.h"
#include "nsIClipboard.h"
#include "nsIDragService.h"
#include "nsIDragSession.h"
#include "nsITransferable.h"
#include "nsIFormatConverter.h"
#include "nsIContentIterator.h"
#include "nsIContent.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsLayoutCID.h"
#include "nsIDOMNSRange.h"
#include "nsEditorUtils.h"

// Drag & Drop, Clipboard Support
static NS_DEFINE_CID(kLookAndFeelCID,          NS_LOOKANDFEEL_CID);
static NS_DEFINE_CID(kCaretCID,                NS_CARET_CID);

//#define DEBUG_IME

/*
 * nsTextEditorKeyListener implementation
 */

NS_IMPL_ISUPPORTS2(nsTextEditorKeyListener, nsIDOMEventListener, nsIDOMKeyListener)


nsTextEditorKeyListener::nsTextEditorKeyListener()
{
}



nsTextEditorKeyListener::~nsTextEditorKeyListener() 
{
}


nsresult
nsTextEditorKeyListener::HandleEvent(nsIDOMEvent* aEvent)
{
  return NS_OK;
}

// individual key handlers return NS_OK to indicate NOT consumed
// by default, an error is returned indicating event is consumed
// joki is fixing this interface.
nsresult
nsTextEditorKeyListener::KeyDown(nsIDOMEvent* aKeyEvent)
{
  return NS_OK;
}


nsresult
nsTextEditorKeyListener::KeyUp(nsIDOMEvent* aKeyEvent)
{
  return NS_OK;
}


nsresult
nsTextEditorKeyListener::KeyPress(nsIDOMEvent* aKeyEvent)
{
  nsCOMPtr<nsIDOMKeyEvent>keyEvent;
  keyEvent = do_QueryInterface(aKeyEvent);
  if (!keyEvent) 
  {
    //non-key event passed to keydown.  bad things.
    return NS_OK;
  }
  
  // DOM event handling happens in two passes, the client pass and the system
  // pass.  We do all of our processing in the system pass, to allow client
  // handlers the opporunity to cancel events and prevent typing in the editor.
  // If the client pass cancelled the event, defaultPrevented will be true
  // below.

  nsCOMPtr<nsIDOMNSUIEvent> nsUIEvent = do_QueryInterface(aKeyEvent);
  if(nsUIEvent) 
  {
    PRBool defaultPrevented;
    nsUIEvent->GetPreventDefault(&defaultPrevented);
    if(defaultPrevented)
      return NS_OK;
  }

  // we should check a flag here to see if we should be using built-in key bindings
  // mEditor->GetFlags(&flags);
  // if (flags & ...)

  PRUint32 keyCode;
  PRUint32 flags;
  keyEvent->GetKeyCode(&keyCode);

  // if we are readonly or disabled, then do nothing.
  if (NS_SUCCEEDED(mEditor->GetFlags(&flags)))
  {
    if (flags & nsIPlaintextEditor::eEditorReadonlyMask || 
        flags & nsIPlaintextEditor::eEditorDisabledMask) 
      return NS_OK;
  }
  else
    return NS_ERROR_FAILURE;  // Editor unable to handle this.

  nsCOMPtr<nsIPlaintextEditor> textEditor (do_QueryInterface(mEditor));
  if (!textEditor) return NS_ERROR_NO_INTERFACE;

  // if there is no charCode, then it's a key that doesn't map to a character,
  // so look for special keys using keyCode.
  if (0 != keyCode)
  {
    PRBool isAnyModifierKeyButShift;
    nsresult rv;
    rv = keyEvent->GetAltKey(&isAnyModifierKeyButShift);
    if (NS_FAILED(rv)) return rv;
    
    if (!isAnyModifierKeyButShift)
    {
      rv = keyEvent->GetMetaKey(&isAnyModifierKeyButShift);
      if (NS_FAILED(rv)) return rv;
      
      if (!isAnyModifierKeyButShift)
      {
        rv = keyEvent->GetCtrlKey(&isAnyModifierKeyButShift);
        if (NS_FAILED(rv)) return rv;
      }
    }

    switch (keyCode)
    {
      case nsIDOMKeyEvent::DOM_VK_META:
      case nsIDOMKeyEvent::DOM_VK_SHIFT:
      case nsIDOMKeyEvent::DOM_VK_CONTROL:
      case nsIDOMKeyEvent::DOM_VK_ALT:
        aKeyEvent->PreventDefault(); // consumed
        return NS_OK;
        break;

      case nsIDOMKeyEvent::DOM_VK_BACK_SPACE: 
        if (isAnyModifierKeyButShift)
          return NS_OK;

        mEditor->DeleteSelection(nsIEditor::ePrevious);
        aKeyEvent->PreventDefault(); // consumed
        return NS_OK;
        break;
 
      case nsIDOMKeyEvent::DOM_VK_DELETE:
        /* on certain platforms (such as windows) the shift key
           modifies what delete does (cmd_cut in this case).
           bailing here to allow the keybindings to do the cut.*/
        PRBool isShiftModifierKey;
        rv = keyEvent->GetShiftKey(&isShiftModifierKey);
        if (NS_FAILED(rv)) return rv;

        if (isAnyModifierKeyButShift || isShiftModifierKey)
           return NS_OK;
        mEditor->DeleteSelection(nsIEditor::eNext);
        aKeyEvent->PreventDefault(); // consumed
        return NS_OK; 
        break;
 
      case nsIDOMKeyEvent::DOM_VK_TAB:
        if ((flags & nsIPlaintextEditor::eEditorSingleLineMask) ||
            (flags & nsIPlaintextEditor::eEditorPasswordMask)   ||
            (flags & nsIPlaintextEditor::eEditorWidgetMask))
          return NS_OK; // let it be used for focus switching

        if (isAnyModifierKeyButShift)
          return NS_OK;

        // else we insert the tab straight through
        textEditor->HandleKeyPress(keyEvent);
        // let HandleKeyPress consume the event
        return NS_OK; 

      case nsIDOMKeyEvent::DOM_VK_RETURN:
      case nsIDOMKeyEvent::DOM_VK_ENTER:
        if (isAnyModifierKeyButShift)
          return NS_OK;

        if (!(flags & nsIPlaintextEditor::eEditorSingleLineMask))
        {
          textEditor->HandleKeyPress(keyEvent);
          aKeyEvent->PreventDefault(); // consumed
        }
        return NS_OK;
    }
  }

  textEditor->HandleKeyPress(keyEvent);
  return NS_OK; // we don't PreventDefault() here or keybindings like control-x won't work 
}


nsresult
nsTextEditorKeyListener::ProcessShortCutKeys(nsIDOMEvent* aKeyEvent, PRBool& aProcessed)
{
  aProcessed=PR_FALSE;
  return NS_OK;
}


/*
 * nsTextEditorMouseListener implementation
 */

NS_IMPL_ISUPPORTS2(nsTextEditorMouseListener, nsIDOMEventListener, nsIDOMMouseListener)


nsTextEditorMouseListener::nsTextEditorMouseListener() 
{
}



nsTextEditorMouseListener::~nsTextEditorMouseListener() 
{
}


nsresult
nsTextEditorMouseListener::HandleEvent(nsIDOMEvent* aEvent)
{
  return NS_OK;
}



nsresult
nsTextEditorMouseListener::MouseClick(nsIDOMEvent* aMouseEvent)
{
  if (!aMouseEvent)
    return NS_OK;   // NS_OK means "we didn't process the event".  Go figure.

  nsCOMPtr<nsIDOMMouseEvent> mouseEvent ( do_QueryInterface(aMouseEvent) );
  if (!mouseEvent) {
    //non-ui event passed in.  bad things.
    return NS_OK;
  }

  // If we got a mouse down inside the editing area, we should force the 
  // IME to commit before we change the cursor position
  nsCOMPtr<nsIEditorIMESupport> imeEditor = do_QueryInterface(mEditor);
  if(imeEditor)
  	imeEditor->ForceCompositionEnd();

   nsCOMPtr<nsIEditor> editor (do_QueryInterface(mEditor));
  if (!editor) { return NS_OK; }

  PRUint16 button = (PRUint16)-1;
  mouseEvent->GetButton(&button);
  // middle-mouse click (paste);
  if (button == 1)
  {
    nsresult rv;
    nsCOMPtr<nsIPrefBranch> prefBranch =
      do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv) && prefBranch)
    {
      PRBool doMiddleMousePaste = PR_FALSE;;
      rv = prefBranch->GetBoolPref("middlemouse.paste", &doMiddleMousePaste);
      if (NS_SUCCEEDED(rv) && doMiddleMousePaste)
      {
        // Set the selection to the point under the mouse cursor:
        nsCOMPtr<nsIDOMNSUIEvent> nsuiEvent (do_QueryInterface(aMouseEvent));

        if (!nsuiEvent)
          return NS_ERROR_NULL_POINTER;
        nsCOMPtr<nsIDOMNode> parent;
        if (NS_FAILED(nsuiEvent->GetRangeParent(getter_AddRefs(parent))))
          return NS_ERROR_NULL_POINTER;
        PRInt32 offset = 0;
        if (NS_FAILED(nsuiEvent->GetRangeOffset(&offset)))
          return NS_ERROR_NULL_POINTER;

        nsCOMPtr<nsISelection> selection;
        if (NS_SUCCEEDED(editor->GetSelection(getter_AddRefs(selection))))
          (void)selection->Collapse(parent, offset);

        // If the ctrl key is pressed, we'll do paste as quotation.
        // Would've used the alt key, but the kde wmgr treats alt-middle specially. 
        nsCOMPtr<nsIEditorMailSupport> mailEditor;
        mouseEvent = do_QueryInterface(aMouseEvent);
        PRBool ctrlKey = PR_FALSE;
        mouseEvent->GetCtrlKey(&ctrlKey);
        if (ctrlKey)
          mailEditor = do_QueryInterface(mEditor);

        PRInt32 clipboard;

#if defined(XP_OS2) || defined(XP_WIN32)
        clipboard = nsIClipboard::kGlobalClipboard;
#else
        clipboard = nsIClipboard::kSelectionClipboard;
#endif

        if (mailEditor)
          mailEditor->PasteAsQuotation(clipboard);
        else
          editor->Paste(clipboard);

        // Prevent the event from bubbling up to be possibly handled
        // again by the containing window:
        nsCOMPtr<nsIDOMNSEvent> nsevent(do_QueryInterface(mouseEvent));

        if (nsevent) {
          nsevent->PreventBubble();
        }

        mouseEvent->PreventDefault();

        // We processed the event, whether drop/paste succeeded or not
        return NS_OK;
      }
    }
   }
   return NS_OK;
}

nsresult
nsTextEditorMouseListener::MouseDown(nsIDOMEvent* aMouseEvent)
{
  return NS_OK;
}

nsresult
nsTextEditorMouseListener::MouseUp(nsIDOMEvent* aMouseEvent)
{
  return NS_OK;
}


nsresult
nsTextEditorMouseListener::MouseDblClick(nsIDOMEvent* aMouseEvent)
{
  return NS_OK;
}



nsresult
nsTextEditorMouseListener::MouseOver(nsIDOMEvent* aMouseEvent)
{
  return NS_OK;
}



nsresult
nsTextEditorMouseListener::MouseOut(nsIDOMEvent* aMouseEvent)
{
  return NS_OK;
}


/*
 * nsTextEditorTextListener implementation
 */

NS_IMPL_ISUPPORTS2(nsTextEditorTextListener, nsIDOMEventListener, nsIDOMTextListener)


nsTextEditorTextListener::nsTextEditorTextListener()
:   mCommitText(PR_FALSE),
   mInTransaction(PR_FALSE)
{
}


nsTextEditorTextListener::~nsTextEditorTextListener() 
{
}

nsresult
nsTextEditorTextListener::HandleEvent(nsIDOMEvent* aEvent)
{
#ifdef DEBUG_IME
   printf("nsTextEditorTextListener::HandleEvent\n");
#endif
  return NS_OK;
}



nsresult
nsTextEditorTextListener::HandleText(nsIDOMEvent* aTextEvent)
{
#ifdef DEBUG_IME
   printf("nsTextEditorTextListener::HandleText\n");
#endif
   nsAutoString            composedText;
   nsresult            result = NS_OK;
   nsCOMPtr<nsIPrivateTextEvent> textEvent;
   nsIPrivateTextRangeList      *textRangeList;
   nsTextEventReply         *textEventReply;

   textEvent = do_QueryInterface(aTextEvent);
   if (!textEvent) {
      //non-ui event passed in.  bad things.
      return NS_OK;
   }

   textEvent->GetText(composedText);
   textEvent->GetInputRange(&textRangeList);
   textEvent->GetEventReply(&textEventReply);
   textRangeList->AddRef();
   nsCOMPtr<nsIEditorIMESupport> imeEditor = do_QueryInterface(mEditor, &result);
   if (imeEditor) {
     PRUint32 flags;
     // if we are readonly or disabled, then do nothing.
     if (NS_SUCCEEDED(mEditor->GetFlags(&flags))) {
       if (flags & nsIPlaintextEditor::eEditorReadonlyMask || 
           flags & nsIPlaintextEditor::eEditorDisabledMask) {
#if DEBUG_IME
         printf("nsTextEditorTextListener::HandleText,  Readonly or Disabled\n");
#endif
         return NS_OK;
       }
     }
     result = imeEditor->SetCompositionString(composedText,textRangeList,textEventReply);
   }
   return result;
}

/*
 * nsTextEditorDragListener implementation
 */

nsTextEditorDragListener::nsTextEditorDragListener() 
: mEditor(nsnull)
, mPresShell(nsnull)
, mCaretDrawn(PR_FALSE)
{
}

nsTextEditorDragListener::~nsTextEditorDragListener() 
{
}

NS_IMPL_ISUPPORTS2(nsTextEditorDragListener, nsIDOMEventListener, nsIDOMDragListener);

nsresult
nsTextEditorDragListener::HandleEvent(nsIDOMEvent* aEvent)
{
  return NS_OK;
}


nsresult
nsTextEditorDragListener::DragGesture(nsIDOMEvent* aDragEvent)
{
  PRBool canDrag = PR_FALSE;
  if ( !mEditor )
    return NS_ERROR_NULL_POINTER;
  
  // ...figure out if a drag should be started...
  nsresult rv = mEditor->CanDrag(aDragEvent, &canDrag);
  if ( NS_SUCCEEDED(rv) && canDrag )
    rv = mEditor->DoDrag(aDragEvent);

  return rv;
}


nsresult
nsTextEditorDragListener::DragEnter(nsIDOMEvent* aDragEvent)
{
  if (mPresShell)
  {
    if (!mCaret)
    {
      mCaret = do_CreateInstance(kCaretCID);
      if (mCaret)
      {
        mCaret->Init(mPresShell);
        mCaret->SetCaretReadOnly(PR_TRUE);
      }
      mCaretDrawn = PR_FALSE;
    }
  }
  
  return DragOver(aDragEvent);
}


nsresult
nsTextEditorDragListener::DragOver(nsIDOMEvent* aDragEvent)
{
  // XXX cache this between drag events?
  nsresult rv;
  nsCOMPtr<nsIDragService> dragService = do_GetService("@mozilla.org/widget/dragservice;1", &rv);
  if (!dragService) return rv;

  // does the drag have flavors we can accept?
  nsCOMPtr<nsIDragSession> dragSession;
  dragService->GetCurrentSession(getter_AddRefs(dragSession));
  if (!dragSession) return NS_ERROR_FAILURE;

  PRBool canDrop = CanDrop(aDragEvent);
  if (canDrop)
  {
    nsCOMPtr<nsIDOMDocument> domdoc;
    mEditor->GetDocument(getter_AddRefs(domdoc));
    canDrop = nsEditorHookUtils::DoAllowDropHook(domdoc, aDragEvent, dragSession);
  }

  dragSession->SetCanDrop(canDrop);

  // We need to consume the event to prevent the browser's
  // default drag listeners from being fired. (Bug 199133)

  aDragEvent->PreventDefault(); // consumed
    
  if (canDrop)
  {
    if (mCaret)
    {
      nsCOMPtr<nsIDOMNSUIEvent> nsuiEvent (do_QueryInterface(aDragEvent));
      if (nsuiEvent)
      {
        nsCOMPtr<nsIDOMNode> parent;
        rv = nsuiEvent->GetRangeParent(getter_AddRefs(parent));
        if (NS_FAILED(rv)) return rv;
        if (!parent) return NS_ERROR_FAILURE;

        PRInt32 offset = 0;
        rv = nsuiEvent->GetRangeOffset(&offset);
        if (NS_FAILED(rv)) return rv;

        // to avoid flicker, we could track the node and offset to see if we moved
        if (mCaretDrawn)
          mCaret->EraseCaret();
        
        //mCaret->SetCaretVisible(PR_TRUE);   // make sure it's visible
        mCaret->DrawAtPosition(parent, offset);
        mCaretDrawn = PR_TRUE;
      }
    }
  }
  else
  {
    if (mCaret && mCaretDrawn)
    {
      mCaret->EraseCaret();
      mCaretDrawn = PR_FALSE;
    } 
  }

  return NS_OK;
}


nsresult
nsTextEditorDragListener::DragExit(nsIDOMEvent* aDragEvent)
{
  if (mCaret)
  {
    if (mCaretDrawn)
      mCaret->EraseCaret();
    mCaretDrawn = PR_FALSE;
  }

  return NS_OK;
}



nsresult
nsTextEditorDragListener::DragDrop(nsIDOMEvent* aMouseEvent)
{
  if (mCaret)
  {
    if (mCaretDrawn)
      mCaret->EraseCaret();
    mCaretDrawn = PR_FALSE;
    mCaret->SetCaretVisible(PR_FALSE);    // hide it, so that it turns off its timer
    mCaret = nsnull;      // release it
  }

  if (!mEditor)
    return NS_ERROR_FAILURE;

  PRBool canDrop = CanDrop(aMouseEvent);
  if (!canDrop)
  {
    // was it because we're read-only?

    PRUint32 flags;
    if (NS_SUCCEEDED(mEditor->GetFlags(&flags))
        && ((flags & nsIPlaintextEditor::eEditorDisabledMask) ||
            (flags & nsIPlaintextEditor::eEditorReadonlyMask)) )
    {
      // it was decided to "eat" the event as this is the "least surprise"
                                      // since someone else handling it might be unintentional and the 
                                      // user could probably re-drag to be not over the disabled/readonly 
                                      // editfields if that is what is desired.
      return aMouseEvent->StopPropagation();
    }
    return NS_OK;
  }

  //some day we want to use another way to stop this from bubbling.
  nsCOMPtr<nsIDOMNSEvent> nsevent(do_QueryInterface(aMouseEvent));
  if (nsevent)
    nsevent->PreventBubble();

  aMouseEvent->PreventDefault();
  return mEditor->InsertFromDrop(aMouseEvent);
}

PRBool
nsTextEditorDragListener::CanDrop(nsIDOMEvent* aEvent)
{
  // if the target doc is read-only, we can't drop
  PRUint32 flags = 0;
  nsresult rv = mEditor->GetFlags(&flags);
  if (NS_FAILED(rv)) return PR_FALSE;

  if ((flags & nsIPlaintextEditor::eEditorDisabledMask) || 
      (flags & nsIPlaintextEditor::eEditorReadonlyMask)) {
    return PR_FALSE;
  }

  // XXX cache this between drag events?
  nsCOMPtr<nsIDragService> dragService = do_GetService("@mozilla.org/widget/dragservice;1", &rv);
  if (!dragService) return PR_FALSE;

  // does the drag have flavors we can accept?
  nsCOMPtr<nsIDragSession> dragSession;
  dragService->GetCurrentSession(getter_AddRefs(dragSession));
  if (!dragSession) return PR_FALSE;

  PRBool flavorSupported = PR_FALSE;
  dragSession->IsDataFlavorSupported(kUnicodeMime, &flavorSupported);

  // if we aren't plaintext editing, we can accept more flavors
  if (!flavorSupported 
     && (flags & nsIPlaintextEditor::eEditorPlaintextMask) == 0)
  {
    dragSession->IsDataFlavorSupported(kHTMLMime, &flavorSupported);
    if (!flavorSupported)
      dragSession->IsDataFlavorSupported(kFileMime, &flavorSupported);
#if 0
    if (!flavorSupported)
      dragSession->IsDataFlavorSupported(kJPEGImageMime, &flavorSupported);
#endif
  }

  if (!flavorSupported)
    return PR_FALSE;     

  nsCOMPtr<nsISelection> selection;
  rv = mEditor->GetSelection(getter_AddRefs(selection));
  if (NS_FAILED(rv) || !selection)
    return PR_FALSE;
    
  nsCOMPtr<nsIDOMDocument> domdoc;
  rv = mEditor->GetDocument(getter_AddRefs(domdoc));
  if (NS_FAILED(rv)) return PR_FALSE;

  nsCOMPtr<nsIDOMDocument> sourceDoc;
  rv = dragSession->GetSourceDocument(getter_AddRefs(sourceDoc));
  if (NS_FAILED(rv)) return PR_FALSE;
  if (domdoc == sourceDoc)      // source and dest are the same document; disallow drops within the selection
  {
    PRBool isCollapsed;
    rv = selection->GetIsCollapsed(&isCollapsed);
    if (NS_FAILED(rv)) return PR_FALSE;
  
    // Don't bother if collapsed - can always drop
    if (!isCollapsed)
    {
      nsCOMPtr<nsIDOMNSUIEvent> nsuiEvent (do_QueryInterface(aEvent));
      if (!nsuiEvent) return PR_FALSE;

      nsCOMPtr<nsIDOMNode> parent;
      rv = nsuiEvent->GetRangeParent(getter_AddRefs(parent));
      if (NS_FAILED(rv) || !parent) return PR_FALSE;

      PRInt32 offset = 0;
      rv = nsuiEvent->GetRangeOffset(&offset);
      if (NS_FAILED(rv)) return PR_FALSE;

      PRInt32 rangeCount;
      rv = selection->GetRangeCount(&rangeCount);
      if (NS_FAILED(rv)) return PR_FALSE;

      for (PRInt32 i = 0; i < rangeCount; i++)
      {
        nsCOMPtr<nsIDOMRange> range;
        rv = selection->GetRangeAt(i, getter_AddRefs(range));
        if (NS_FAILED(rv) || !range) 
        continue; //dont bail yet, iterate through them all

        nsCOMPtr<nsIDOMNSRange> nsrange(do_QueryInterface(range));
        if (NS_FAILED(rv) || !nsrange) 
        continue; //dont bail yet, iterate through them all

        PRBool inRange = PR_TRUE;
        (void)nsrange->IsPointInRange(parent, offset, &inRange);
        if (inRange)
          return PR_FALSE;  //okay, now you can bail, we are over the orginal selection
      }
    }
  }
  
  return PR_TRUE;
}


nsTextEditorCompositionListener::nsTextEditorCompositionListener()
{
}

nsTextEditorCompositionListener::~nsTextEditorCompositionListener() 
{
}

NS_IMPL_ISUPPORTS2(nsTextEditorCompositionListener, nsIDOMEventListener, nsIDOMCompositionListener)

nsresult
nsTextEditorCompositionListener::HandleEvent(nsIDOMEvent* aEvent)
{
#ifdef DEBUG_IME
   printf("nsTextEditorCompositionListener::HandleEvent\n");
#endif
  return NS_OK;
}

void nsTextEditorCompositionListener::SetEditor(nsIEditor *aEditor)
{
  nsCOMPtr<nsIEditorIMESupport> imeEditor = do_QueryInterface(aEditor);
  if (!imeEditor) return;      // should return an error here!
  
  // note that we don't hold an extra reference here.
  mEditor = imeEditor;
}

nsresult
nsTextEditorCompositionListener::HandleStartComposition(nsIDOMEvent* aCompositionEvent)
{
#ifdef DEBUG_IME
   printf("nsTextEditorCompositionListener::HandleStartComposition\n");
#endif
  nsCOMPtr<nsIPrivateCompositionEvent> pCompositionEvent = do_QueryInterface(aCompositionEvent);
  nsTextEventReply* eventReply;

  if (!pCompositionEvent) return NS_ERROR_FAILURE;
  
  nsresult rv = pCompositionEvent->GetCompositionReply(&eventReply);
  if (NS_FAILED(rv)) return rv;

  return mEditor->BeginComposition(eventReply);
}
nsresult
nsTextEditorCompositionListener::HandleQueryComposition(nsIDOMEvent* aCompositionEvent)
{
#ifdef DEBUG_IME
   printf("nsTextEditorCompositionListener::HandleQueryComposition\n");
#endif
  nsCOMPtr<nsIPrivateCompositionEvent> pCompositionEvent = do_QueryInterface(aCompositionEvent);
  nsTextEventReply* eventReply;

  if (!pCompositionEvent) return NS_ERROR_FAILURE;
  
  nsresult rv = pCompositionEvent->GetCompositionReply(&eventReply);
  if (NS_FAILED(rv)) return rv;

  return mEditor->QueryComposition(eventReply);
}

nsresult
nsTextEditorCompositionListener::HandleEndComposition(nsIDOMEvent* aCompositionEvent)
{
#ifdef DEBUG_IME
   printf("nsTextEditorCompositionListener::HandleEndComposition\n");
#endif
   return mEditor->EndComposition();
}


nsresult
nsTextEditorCompositionListener::HandleQueryReconversion(nsIDOMEvent* aReconversionEvent)
{
#ifdef DEBUG_IME
  printf("nsTextEditorCompositionListener::HandleQueryReconversion\n");
#endif
  nsCOMPtr<nsIPrivateCompositionEvent> pCompositionEvent = do_QueryInterface(aReconversionEvent);
  nsReconversionEventReply* eventReply;

  if (!pCompositionEvent)
    return NS_ERROR_FAILURE;

  nsresult rv = pCompositionEvent->GetReconversionReply(&eventReply);
  if (NS_FAILED(rv))
    return rv;

  return mEditor->GetReconversionString(eventReply);
}

/*
 * Factory functions
 */



nsresult 
NS_NewEditorKeyListener(nsIDOMEventListener ** aInstancePtrResult, 
                        nsIEditor *aEditor)
{
  nsTextEditorKeyListener* it = new nsTextEditorKeyListener();
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  it->SetEditor(aEditor);

  return it->QueryInterface(NS_GET_IID(nsIDOMEventListener), (void **) aInstancePtrResult);   
}



nsresult
NS_NewEditorMouseListener(nsIDOMEventListener ** aInstancePtrResult, 
                          nsIEditor *aEditor)
{
  nsTextEditorMouseListener* it = new nsTextEditorMouseListener();
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  it->SetEditor(aEditor);

  return it->QueryInterface(NS_GET_IID(nsIDOMEventListener), (void **) aInstancePtrResult);   
}


nsresult
NS_NewEditorTextListener(nsIDOMEventListener** aInstancePtrResult, nsIEditor* aEditor)
{
   nsTextEditorTextListener*   it = new nsTextEditorTextListener();
   if (nsnull==it) {
      return NS_ERROR_OUT_OF_MEMORY;
   }

   it->SetEditor(aEditor);

   return it->QueryInterface(NS_GET_IID(nsIDOMEventListener), (void **) aInstancePtrResult);
}



nsresult
NS_NewEditorDragListener(nsIDOMEventListener ** aInstancePtrResult, nsIPresShell* aPresShell,
                          nsIEditor *aEditor)
{
  nsTextEditorDragListener* it = new nsTextEditorDragListener();
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  it->SetEditor(aEditor);
  it->SetPresShell(aPresShell);

  return it->QueryInterface(NS_GET_IID(nsIDOMEventListener), (void **) aInstancePtrResult);   
}

nsresult
NS_NewEditorCompositionListener(nsIDOMEventListener** aInstancePtrResult, nsIEditor* aEditor)
{
   nsTextEditorCompositionListener*   it = new nsTextEditorCompositionListener();
   if (nsnull==it) {
      return NS_ERROR_OUT_OF_MEMORY;
   }
   it->SetEditor(aEditor);
  return it->QueryInterface(NS_GET_IID(nsIDOMEventListener), (void **) aInstancePtrResult);
}

nsresult 
NS_NewEditorFocusListener(nsIDOMEventListener ** aInstancePtrResult, 
                          nsIEditor *aEditor)
{
  nsTextEditorFocusListener* it = new nsTextEditorFocusListener();
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  it->SetEditor(aEditor);
   return it->QueryInterface(NS_GET_IID(nsIDOMEventListener), (void **) aInstancePtrResult);
}



/*
 * nsTextEditorFocusListener implementation
 */

NS_IMPL_ISUPPORTS2(nsTextEditorFocusListener, nsIDOMEventListener, nsIDOMFocusListener)


nsTextEditorFocusListener::nsTextEditorFocusListener() 
{
}

nsTextEditorFocusListener::~nsTextEditorFocusListener() 
{
}

nsresult
nsTextEditorFocusListener::HandleEvent(nsIDOMEvent* aEvent)
{
  return NS_OK;
}

nsresult
nsTextEditorFocusListener::Focus(nsIDOMEvent* aEvent)
{
  // turn on selection and caret
  if (mEditor)
  {
    PRUint32 flags;

    nsCOMPtr<nsIDOMNSEvent> nsevent(do_QueryInterface(aEvent));

    if (nsevent) {
      nsevent->PreventBubble();
    }

    mEditor->GetFlags(&flags);
    if (! (flags & nsIPlaintextEditor::eEditorDisabledMask))
    { // only enable caret and selection if the editor is not disabled
      nsCOMPtr<nsIEditor>editor = do_QueryInterface(mEditor);
      if (editor)
      {
        nsCOMPtr<nsISelectionController>selCon;
        editor->GetSelectionController(getter_AddRefs(selCon));
        if (selCon)
        {
          if (! (flags & nsIPlaintextEditor::eEditorReadonlyMask))
          { // only enable caret if the editor is not readonly
            PRInt32 pixelWidth;
            nsresult result;

            nsCOMPtr<nsILookAndFeel> look = 
                     do_GetService(kLookAndFeelCID, &result);

            if (NS_SUCCEEDED(result) && look)
            {
              if(flags & nsIPlaintextEditor::eEditorSingleLineMask)
                look->GetMetric(nsILookAndFeel::eMetric_SingleLineCaretWidth, pixelWidth);
              else
                look->GetMetric(nsILookAndFeel::eMetric_MultiLineCaretWidth, pixelWidth);
            }

            selCon->SetCaretWidth(pixelWidth);
            selCon->SetCaretEnabled(PR_TRUE);

          }
          selCon->SetDisplaySelection(nsISelectionController::SELECTION_ON);
#ifdef USE_HACK_REPAINT
  // begin hack repaint
          nsCOMPtr<nsIViewManager> viewmgr;
          ps->GetViewManager(getter_AddRefs(viewmgr));
          if (viewmgr) {
            nsIView* view;
            viewmgr->GetRootView(view);         // views are not refCounted
            if (view) {
              viewmgr->UpdateView(view,NS_VMREFRESH_IMMEDIATE);
            }
          }
  // end hack repaint
#else
          selCon->RepaintSelection(nsISelectionController::SELECTION_NORMAL);
#endif
        }
      }
    }
  }
  return NS_OK;
}

nsresult
nsTextEditorFocusListener::Blur(nsIDOMEvent* aEvent)
{
  // turn off selection and caret
  if (mEditor)
  {
    PRUint32 flags;

    nsCOMPtr<nsIDOMNSEvent> nsevent(do_QueryInterface(aEvent));

    if (nsevent) {
      nsevent->PreventBubble();
    }

    mEditor->GetFlags(&flags);
    nsCOMPtr<nsIEditor>editor = do_QueryInterface(mEditor);

    // when imeEditor exists, call ForceCompositionEnd() to tell
    // the input focus is leaving first
    nsCOMPtr<nsIEditorIMESupport> imeEditor = do_QueryInterface(mEditor);
    if (imeEditor)
      imeEditor->ForceCompositionEnd();

    if (editor)
    {
      nsCOMPtr<nsISelectionController>selCon;
      editor->GetSelectionController(getter_AddRefs(selCon));
      if (selCon)
      {
        selCon->SetCaretEnabled(PR_FALSE);
        if((flags & nsIPlaintextEditor::eEditorWidgetMask)  ||
          (flags & nsIPlaintextEditor::eEditorPasswordMask) ||
          (flags & nsIPlaintextEditor::eEditorReadonlyMask) ||
          (flags & nsIPlaintextEditor::eEditorDisabledMask) ||
          (flags & nsIPlaintextEditor::eEditorFilterInputMask))
        {
          selCon->SetDisplaySelection(nsISelectionController::SELECTION_HIDDEN);//hide but do NOT turn off
        }
        else
        {
          selCon->SetDisplaySelection(nsISelectionController::SELECTION_DISABLED);
        }

#ifdef USE_HACK_REPAINT
// begin hack repaint
        nsCOMPtr<nsIViewManager> viewmgr;
        ps->GetViewManager(getter_AddRefs(viewmgr));
        if (viewmgr) 
        {
          nsIView* view;
          viewmgr->GetRootView(view);         // views are not refCounted
          if (view) {
            viewmgr->UpdateView(view,NS_VMREFRESH_IMMEDIATE);
          }
        }
// end hack repaint
#else
        selCon->RepaintSelection(nsISelectionController::SELECTION_NORMAL);
#endif
      }
    }
  }
  return NS_OK;
}

