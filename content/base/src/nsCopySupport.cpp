/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
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
 *   Kathleen Brade <brade@netscape.com>
 *   David Gardiner <david.gardiner@unisa.edu.au>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsCopySupport.h"
#include "nsIDocumentEncoder.h"
#include "nsISupports.h"
#include "nsIContent.h"
#include "nsIComponentManager.h" 
#include "nsIServiceManager.h"
#include "nsIClipboard.h"
#include "nsISelection.h"
#include "nsWidgetsCID.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsIDOMRange.h"

#include "nsIDocShell.h"
#include "nsIClipboardDragDropHooks.h"
#include "nsIClipboardDragDropHookList.h"

#include "nsIDocument.h"
#include "nsIDOMNode.h"
#include "nsIDOMElement.h"
#include "nsIHTMLDocument.h"
#include "nsHTMLAtoms.h"

// image copy stuff
#include "nsIImageLoadingContent.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIImage.h"
#include "nsContentUtils.h"

static NS_DEFINE_CID(kCClipboardCID,           NS_CLIPBOARD_CID);
static NS_DEFINE_CID(kCTransferableCID,        NS_TRANSFERABLE_CID);
static NS_DEFINE_CID(kHTMLConverterCID,        NS_HTMLFORMATCONVERTER_CID);

// private clipboard data flavors for html copy, used by editor when pasting
#define kHTMLContext   "text/_moz_htmlcontext"
#define kHTMLInfo      "text/_moz_htmlinfo"


nsresult nsCopySupport::HTMLCopy(nsISelection *aSel, nsIDocument *aDoc, PRInt16 aClipboardID)
{
  nsresult rv = NS_OK;
  
  PRBool bIsPlainTextContext = PR_FALSE;

  rv = IsPlainTextContext(aSel, aDoc, &bIsPlainTextContext);
  if (NS_FAILED(rv)) 
    return rv;

  PRBool bIsHTMLCopy = !bIsPlainTextContext;
  nsAutoString mimeType;

  nsCOMPtr<nsIDocumentEncoder> docEncoder;

  docEncoder = do_CreateInstance(NS_HTMLCOPY_ENCODER_CONTRACTID);
  NS_ENSURE_TRUE(docEncoder, NS_ERROR_FAILURE);

  // We always require a plaintext version
  mimeType.AssignLiteral(kUnicodeMime);
  PRUint32 flags = nsIDocumentEncoder::OutputPreformatted;

  rv = docEncoder->Init(aDoc, mimeType, flags);
  if (NS_FAILED(rv)) 
    return rv;
  rv = docEncoder->SetSelection(aSel);
  if (NS_FAILED(rv)) 
    return rv;

  nsAutoString buffer, parents, info, shortcut, textBuffer, plaintextBuffer;

  rv = docEncoder->EncodeToString(textBuffer);
  if (NS_FAILED(rv)) 
    return rv;

  // this string may still contain HTML formatting, so we need to remove that too.
  nsCOMPtr<nsIFormatConverter> htmlConverter = do_CreateInstance(kHTMLConverterCID);
  NS_ENSURE_TRUE(htmlConverter, NS_ERROR_FAILURE);

  nsCOMPtr<nsISupportsString> plainHTML;
  plainHTML = do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID);
  NS_ENSURE_TRUE(plainHTML, NS_ERROR_FAILURE);
  nsresult plainhtml_rv = plainHTML->SetData(textBuffer);

  nsCOMPtr<nsISupportsString> ConvertedData;
  PRUint32 ConvertedLen;
  htmlConverter->Convert(kHTMLMime, plainHTML, textBuffer.Length() * 2, kUnicodeMime, getter_AddRefs(ConvertedData), &ConvertedLen);

  nsresult converted_rv = ConvertedData->GetData(plaintextBuffer);

  // sometimes we also need the HTML version
  if (bIsHTMLCopy) {
    mimeType.AssignLiteral(kHTMLMime);

    flags = 0;

    rv = docEncoder->Init(aDoc, mimeType, flags);
    if (NS_FAILED(rv)) 
      return rv;
    rv = docEncoder->SetSelection(aSel);
    if (NS_FAILED(rv)) 
      return rv;

    // encode the selection as html with contextual info
    rv = docEncoder->EncodeToStringWithContext(buffer, parents, info);
    if (NS_FAILED(rv)) 
      return rv;

  }
  
  // Get the Clipboard
  nsCOMPtr<nsIClipboard> clipboard(do_GetService(kCClipboardCID, &rv));
  if (NS_FAILED(rv)) 
    return rv;

  if ( clipboard ) 
  {
    // Create a transferable for putting data on the Clipboard
    nsCOMPtr<nsITransferable> trans = do_CreateInstance(kCTransferableCID);
    if ( trans ) 
    {
      nsCOMPtr<nsISupportsString> textWrapper;

      textWrapper = do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID);
      NS_ENSURE_TRUE(textWrapper, NS_ERROR_FAILURE);

      nsresult text_rv = textWrapper->SetData(plaintextBuffer);

      if (bIsHTMLCopy)
      {
        // set up the data converter
        trans->SetConverter(htmlConverter);

        // get wStrings to hold clip data
        nsCOMPtr<nsISupportsString> dataWrapper, contextWrapper, infoWrapper, urlWrapper;

        dataWrapper = do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID);
        NS_ENSURE_TRUE(dataWrapper, NS_ERROR_FAILURE);
        contextWrapper = do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID);
        NS_ENSURE_TRUE(contextWrapper, NS_ERROR_FAILURE);
        infoWrapper = do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID);
        NS_ENSURE_TRUE(infoWrapper, NS_ERROR_FAILURE);
        urlWrapper = do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID);
        NS_ENSURE_TRUE(urlWrapper, NS_ERROR_FAILURE);

        // populate the strings
        nsresult data_rv = dataWrapper->SetData(buffer);
        nsresult context_rv = contextWrapper->SetData(parents);
        nsresult info_rv = infoWrapper->SetData(info);

        // Try and get source URI of the items that are being dragged
        nsIURI *uri = aDoc->GetDocumentURI();

        nsCAutoString spec;
        uri->GetSpec(spec);

        AppendUTF8toUTF16(spec, shortcut);
        shortcut.Append(PRUnichar('\n'));

        // and get document title
        shortcut.Append(aDoc->GetDocumentTitle());

        nsresult url_rv = urlWrapper->SetData(shortcut);

        // QI the data object an |nsISupports| so that when the transferable holds
        // onto it, it will addref the correct interface.
        nsCOMPtr<nsISupports> genericDataObj;

        if (!buffer.IsEmpty() && NS_SUCCEEDED(data_rv))
        {
          // Add the html DataFlavor to the transferable
          trans->AddDataFlavor(kHTMLMime);
          genericDataObj = do_QueryInterface(dataWrapper);
          trans->SetTransferData(kHTMLMime, genericDataObj, buffer.Length()*2);
        }
        if (!parents.IsEmpty() && NS_SUCCEEDED(context_rv))
        {
          // Add the htmlcontext DataFlavor to the transferable
          trans->AddDataFlavor(kHTMLContext);
          genericDataObj = do_QueryInterface(contextWrapper);
          trans->SetTransferData(kHTMLContext, genericDataObj, parents.Length()*2);
        }
        if (!info.IsEmpty() && NS_SUCCEEDED(info_rv))
        {
          // Add the htmlinfo DataFlavor to the transferable
          trans->AddDataFlavor(kHTMLInfo);
          genericDataObj = do_QueryInterface(infoWrapper);
          trans->SetTransferData(kHTMLInfo, genericDataObj, info.Length()*2);
        }
        if (!plaintextBuffer.IsEmpty() && NS_SUCCEEDED(text_rv))
        {
          // unicode text
          // Add the unicode DataFlavor to the transferable
          // If we didn't have this, then nsDataObj::GetData matches text/unicode against
          // the kURLMime flavour which is not desirable (eg. when pasting into Notepad)
          trans->AddDataFlavor(kUnicodeMime);
          genericDataObj = do_QueryInterface(textWrapper);
          trans->SetTransferData(kUnicodeMime, genericDataObj, plaintextBuffer.Length()*2);
        }
        // url
        if (!shortcut.IsEmpty() && NS_SUCCEEDED(url_rv))
        {
          // Add the URL DataFlavor to the transferable
          trans->AddDataFlavor(kURLMime);
          genericDataObj = do_QueryInterface(urlWrapper);
          trans->SetTransferData(kURLMime, genericDataObj, shortcut.Length()*2);
        }

      }
      else
      {
        // QI the data object an |nsISupports| so that when the transferable holds
        // onto it, it will addref the correct interface.
        nsCOMPtr<nsISupports> genericDataObj ( do_QueryInterface(textWrapper) );

        if (!plaintextBuffer.IsEmpty() && NS_SUCCEEDED(text_rv))
        {
         // Add the unicode DataFlavor to the transferable
          trans->AddDataFlavor(kUnicodeMime);
          trans->SetTransferData(kUnicodeMime, genericDataObj, plaintextBuffer.Length()*2);
        }
      }

      PRBool doPutOnClipboard = PR_TRUE;
      DoHooks(aDoc, trans, &doPutOnClipboard);

      // put the transferable on the clipboard
      if (doPutOnClipboard)
        clipboard->SetData(trans, nsnull, aClipboardID);
    }
  }
  return rv;
}

nsresult nsCopySupport::DoHooks(nsIDocument *aDoc, nsITransferable *aTrans,
                                PRBool *aDoPutOnClipboard)
{
  NS_ENSURE_ARG(aDoc);

  *aDoPutOnClipboard = PR_TRUE;

  nsCOMPtr<nsISupports> container = aDoc->GetContainer();
  nsCOMPtr<nsIClipboardDragDropHookList> hookObj = do_GetInterface(container);
  if (!hookObj) return NS_ERROR_FAILURE;

  nsCOMPtr<nsISimpleEnumerator> enumerator;
  hookObj->GetHookEnumerator(getter_AddRefs(enumerator));
  if (!enumerator) return NS_ERROR_FAILURE;

  // the logic here should follow the behavior specified in
  // nsIClipboardDragDropHooks.h

  nsCOMPtr<nsIClipboardDragDropHooks> override;
  nsCOMPtr<nsISupports> isupp;
  PRBool hasMoreHooks = PR_FALSE;
  nsresult rv = NS_OK;
  while (NS_SUCCEEDED(enumerator->HasMoreElements(&hasMoreHooks))
         && hasMoreHooks)
  {
    rv = enumerator->GetNext(getter_AddRefs(isupp));
    if (NS_FAILED(rv)) break;
    override = do_QueryInterface(isupp);
    if (override)
    {
#ifdef DEBUG
      nsresult hookResult =
#endif
      override->OnCopyOrDrag(nsnull, aTrans, aDoPutOnClipboard);
      NS_ASSERTION(NS_SUCCEEDED(hookResult), "OnCopyOrDrag hook failed");
      if (!*aDoPutOnClipboard)
        break;
    }
  }

  return rv;
}

nsresult nsCopySupport::IsPlainTextContext(nsISelection *aSel, nsIDocument *aDoc, PRBool *aIsPlainTextContext)
{
  nsresult rv;

  if (!aSel || !aIsPlainTextContext)
    return NS_ERROR_NULL_POINTER;

  *aIsPlainTextContext = PR_FALSE;
  
  nsCOMPtr<nsIDOMRange> range;
  nsCOMPtr<nsIDOMNode> commonParent;
  PRInt32 count = 0;

  rv = aSel->GetRangeCount(&count);
  NS_ENSURE_SUCCESS(rv, rv);

  // if selection is uninitialized return
  if (!count)
    return NS_ERROR_FAILURE;
  
  // we'll just use the common parent of the first range.  Implicit assumption
  // here that multi-range selections are table cell selections, in which case
  // the common parent is somewhere in the table and we don't really care where.
  rv = aSel->GetRangeAt(0, getter_AddRefs(range));
  NS_ENSURE_SUCCESS(rv, rv);
  if (!range)
    return NS_ERROR_NULL_POINTER;
  range->GetCommonAncestorContainer(getter_AddRefs(commonParent));

  for (nsCOMPtr<nsIContent> selContent(do_QueryInterface(commonParent));
       selContent;
       selContent = selContent->GetParent())
  {
    // checking for selection inside a plaintext form widget

    if (!selContent->IsContentOfType(nsIContent::eHTML)) {
      continue;
    }

    nsIAtom *atom = selContent->Tag();

    if (atom == nsHTMLAtoms::input ||
        atom == nsHTMLAtoms::textarea)
    {
      *aIsPlainTextContext = PR_TRUE;
      break;
    }

    if (atom == nsHTMLAtoms::body)
    {
      // check for moz prewrap style on body.  If it's there we are 
      // in a plaintext editor.  This is pretty cheezy but I haven't 
      // found a good way to tell if we are in a plaintext editor.
      nsCOMPtr<nsIDOMElement> bodyElem = do_QueryInterface(selContent);
      nsAutoString wsVal;
      rv = bodyElem->GetAttribute(NS_LITERAL_STRING("style"), wsVal);
      if (NS_SUCCEEDED(rv) && (kNotFound != wsVal.Find(NS_LITERAL_STRING("-moz-pre-wrap"))))
      {
        *aIsPlainTextContext = PR_TRUE;
        break;
      }
    }
  }
  
  // also consider ourselves in a text widget if we can't find an html
  // document. Note that XHTML is not counted as HTML here, because we can't
  // copy it properly (all the copy code for non-plaintext assumes using HTML
  // serializers and parsers is OK, and those mess up XHTML).
  nsCOMPtr<nsIHTMLDocument> htmlDoc = do_QueryInterface(aDoc);
  if (!htmlDoc || aDoc->IsCaseSensitive())
    *aIsPlainTextContext = PR_TRUE;

  return NS_OK;
}

nsresult
nsCopySupport::GetContents(const nsACString& aMimeType, PRUint32 aFlags, nsISelection *aSel, nsIDocument *aDoc, nsAString& outdata)
{
  nsresult rv = NS_OK;
  
  nsCOMPtr<nsIDocumentEncoder> docEncoder;

  nsCAutoString encoderContractID(NS_DOC_ENCODER_CONTRACTID_BASE);
  encoderContractID.Append(aMimeType);
    
  docEncoder = do_CreateInstance(encoderContractID.get());
  NS_ENSURE_TRUE(docEncoder, NS_ERROR_FAILURE);

  PRUint32 flags = aFlags;
  
  if (aMimeType.Equals("text/plain"))
    flags |= nsIDocumentEncoder::OutputPreformatted;

  NS_ConvertASCIItoUCS2 unicodeMimeType(aMimeType);
  rv = docEncoder->Init(aDoc, unicodeMimeType, flags);
  if (NS_FAILED(rv)) return rv;
  
  if (aSel)
  {
    rv = docEncoder->SetSelection(aSel);
    if (NS_FAILED(rv)) return rv;
  } 
  
  // encode the selection
  return docEncoder->EncodeToString(outdata);
}


nsresult
nsCopySupport::ImageCopy(nsIImageLoadingContent* imageElement, PRInt16 aClipboardID)
{
  nsresult rv;

  nsCOMPtr<nsIImage> image = nsContentUtils::GetImageFromContent(imageElement);
  if (!image) return NS_ERROR_FAILURE;

  // Get the Clipboard
  nsCOMPtr<nsIClipboard> clipboard(do_GetService(kCClipboardCID, &rv));
  if (NS_FAILED(rv)) return rv;
  if (!clipboard) return NS_ERROR_FAILURE;

  // Create a transferable for putting data on the Clipboard
  nsCOMPtr<nsITransferable> trans = do_CreateInstance(kCTransferableCID, &rv);
  if (NS_FAILED(rv)) return rv;
  if (!trans) return NS_ERROR_FAILURE;

  nsCOMPtr<nsISupportsInterfacePointer> ptrPrimitive(do_CreateInstance(NS_SUPPORTS_INTERFACE_POINTER_CONTRACTID, &rv));
  if (NS_FAILED(rv)) return rv;
  if (!ptrPrimitive)  return NS_ERROR_FAILURE;
  ptrPrimitive->SetData(image);

  trans->SetTransferData(kNativeImageMime, ptrPrimitive, sizeof(nsISupports*));

  // put the transferable on the clipboard
  return clipboard->SetData(trans, nsnull, aClipboardID);
}
