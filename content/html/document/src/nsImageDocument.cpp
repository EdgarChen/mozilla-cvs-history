/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Morten Nilsen <morten@nilsen.com>
 *   Christian Biesinger <cbiesinger@web.de>
 *   Jan Varga <varga@netscape.com>
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

#include "nsRect.h"
#include "nsHTMLDocument.h"
#include "nsIImageDocument.h"
#include "nsGenericHTMLElement.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsIDOMEvent.h"
#include "nsIDOMKeyEvent.h"
#include "nsIDOMEventListener.h"
#include "nsHTMLAtoms.h"
#include "imgIRequest.h"
#include "imgILoader.h"
#include "imgIContainer.h"
#include "imgIDecoderObserver.h"
#include "nsIURL.h"
#include "nsIPresShell.h"
#include "nsIPresContext.h"
#include "nsStyleContext.h"
#include "nsIStringBundle.h"
#include "nsIPrefService.h"
#include "nsITextToSubURI.h"
#include "nsAutoPtr.h"
#include "nsMediaDocument.h"

#define NSIMAGEDOCUMENT_PROPERTIES_URI "chrome://communicator/locale/layout/ImageDocument.properties"
#define AUTOMATIC_IMAGE_RESIZING_PREF "browser.enable_automatic_image_resizing"

class nsImageDocument;

class ImageListener: public nsMediaDocumentStreamListener
{
public:
  ImageListener(nsImageDocument* aDocument);
  virtual ~ImageListener();

  NS_DECL_ISUPPORTS

  NS_DECL_NSIREQUESTOBSERVER
};

class nsImageDocument : public nsMediaDocument,
                        public nsIImageDocument,
                        public imgIDecoderObserver,
                        public nsIDOMEventListener
{
public:
  nsImageDocument();
  virtual ~nsImageDocument();

  NS_DECL_ISUPPORTS

  // nsIHTMLDocument
  nsresult Init();

  NS_IMETHOD StartDocumentLoad(const char*         aCommand,
                               nsIChannel*         aChannel,
                               nsILoadGroup*       aLoadGroup,
                               nsISupports*        aContainer,
                               nsIStreamListener** aDocListener,
                               PRBool              aReset = PR_TRUE,
                               nsIContentSink*     aSink = nsnull);

  NS_IMETHOD SetScriptGlobalObject(nsIScriptGlobalObject* aScriptGlobalObject);

  NS_DECL_NSIIMAGEDOCUMENT

  NS_DECL_IMGIDECODEROBSERVER

  NS_DECL_IMGICONTAINEROBSERVER

  // nsIDOMEventListener
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent);

  friend class ImageListener;
protected:
  nsresult CreateSyntheticDocument();

  nsresult CheckOverflowing();

  nsresult UpdateTitle();

  nsRefPtr<nsMediaDocumentStreamListener>  mStreamListener;

  nsCOMPtr<nsIStringBundle>     mStringBundle;
  nsCOMPtr<nsIDOMElement>       mImageElement;
  nsCOMPtr<imgIRequest>         mImageRequest;

  nscoord                       mVisibleWidth;
  nscoord                       mVisibleHeight;
  nscoord                       mImageWidth;
  nscoord                       mImageHeight;

  PRPackedBool                  mImageResizingEnabled;
  PRPackedBool                  mImageIsOverflowing;
  PRPackedBool                  mImageIsResized;
};

NS_IMPL_ADDREF_INHERITED(ImageListener, nsMediaDocumentStreamListener)
NS_IMPL_RELEASE_INHERITED(ImageListener, nsMediaDocumentStreamListener)

NS_INTERFACE_MAP_BEGIN(ImageListener)
NS_INTERFACE_MAP_END_INHERITING(nsMediaDocumentStreamListener)

ImageListener::ImageListener(nsImageDocument* aDocument)
  : nsMediaDocumentStreamListener(aDocument)
{
}


ImageListener::~ImageListener()
{
};

NS_IMETHODIMP
ImageListener::OnStartRequest(nsIRequest* request, nsISupports *ctxt)
{
  nsImageDocument *imgDoc = (nsImageDocument*)mDocument.get();
  NS_PRECONDITION(!imgDoc->mImageRequest, "OnStartRequest called twice!");
  nsCOMPtr<nsIChannel> channel = do_QueryInterface(request);
  if (!channel) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<imgILoader> il(do_GetService("@mozilla.org/image/loader;1"));
  nsCOMPtr<nsISupports> docSupports;
  CallQueryInterface(imgDoc, NS_STATIC_CAST(nsISupports**,
                                            getter_AddRefs(docSupports)));
  il->LoadImageWithChannel(channel, imgDoc, docSupports,
                           getter_AddRefs(mNextStream), 
                           getter_AddRefs(imgDoc->mImageRequest));

  return nsMediaDocumentStreamListener::OnStartRequest(request, ctxt);
}

NS_IMETHODIMP
ImageListener::OnStopRequest(nsIRequest* request, nsISupports *ctxt,
                             nsresult status)
{
  ((nsImageDocument*) mDocument.get())->UpdateTitle();

  return nsMediaDocumentStreamListener::OnStopRequest(request, ctxt, status);
}


  // NOTE! nsDocument::operator new() zeroes out all members, so don't
  // bother initializing members to 0.

nsImageDocument::nsImageDocument()
{

  // NOTE! nsDocument::operator new() zeroes out all members, so don't
  // bother initializing members to 0.

}

nsImageDocument::~nsImageDocument()
{
  if (mImageRequest) {
    mImageRequest->Cancel(NS_ERROR_FAILURE);
  }
}

NS_IMPL_ADDREF_INHERITED(nsImageDocument, nsMediaDocument)
NS_IMPL_RELEASE_INHERITED(nsImageDocument, nsMediaDocument)

NS_INTERFACE_MAP_BEGIN(nsImageDocument)
  NS_INTERFACE_MAP_ENTRY(nsIImageDocument)
  NS_INTERFACE_MAP_ENTRY(imgIDecoderObserver)
  NS_INTERFACE_MAP_ENTRY(imgIContainerObserver)
  NS_INTERFACE_MAP_ENTRY(nsIDOMEventListener)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(ImageDocument)
NS_INTERFACE_MAP_END_INHERITING(nsMediaDocument)


nsresult
nsImageDocument::Init()
{
  nsresult rv = nsHTMLDocument::Init();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPrefBranch> prefBranch(do_GetService(NS_PREFSERVICE_CONTRACTID));
  if (prefBranch) {
    PRBool temp = PR_FALSE;
    prefBranch->GetBoolPref(AUTOMATIC_IMAGE_RESIZING_PREF, &temp);
    mImageResizingEnabled = temp;
  }

  // Create a bundle for the localization
  nsCOMPtr<nsIStringBundleService> stringService(
    do_GetService(NS_STRINGBUNDLE_CONTRACTID));
  if (stringService) {
    stringService->CreateBundle(NSIMAGEDOCUMENT_PROPERTIES_URI,
                                getter_AddRefs(mStringBundle));
  }

  return NS_OK;
}

NS_IMETHODIMP
nsImageDocument::StartDocumentLoad(const char*         aCommand,
                                   nsIChannel*         aChannel,
                                   nsILoadGroup*       aLoadGroup,
                                   nsISupports*        aContainer,
                                   nsIStreamListener** aDocListener,
                                   PRBool              aReset,
                                   nsIContentSink*     aSink)
{
  nsresult rv =
    nsMediaDocument::StartDocumentLoad(aCommand, aChannel, aLoadGroup,
                                       aContainer, aDocListener, aReset,
                                       aSink);
  if (NS_FAILED(rv)) {
    return rv;
  }

  mStreamListener = new ImageListener(this);
  if (!mStreamListener)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ASSERTION(aDocListener, "null aDocListener");
  NS_ADDREF(*aDocListener = mStreamListener);

  return NS_OK;
}

NS_IMETHODIMP
nsImageDocument::SetScriptGlobalObject(nsIScriptGlobalObject* aScriptGlobalObject)
{
  if (!aScriptGlobalObject) {
    // If the global object is being set to null, then it means we are going
    // away soon. Drop our ref to imgRequest so that we don't end up leaking
    // due to cycles through imgLib.  This should not be really necessary
    // anymore, but it's a belt-and-suspenders thing.
    if (mImageRequest) {
      mImageRequest->Cancel(NS_ERROR_FAILURE);
      mImageRequest = nsnull;
    }

    if (mImageResizingEnabled) {
      nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(mImageElement);
      target->RemoveEventListener(NS_LITERAL_STRING("click"), this, PR_FALSE);

      target = do_QueryInterface(mScriptGlobalObject);
      target->RemoveEventListener(NS_LITERAL_STRING("resize"), this, PR_FALSE);
      target->RemoveEventListener(NS_LITERAL_STRING("keypress"), this,
                                  PR_FALSE);
    }
  }

  // Set the script global object on the superclass before doing
  // anything that might require it....
  nsresult rv = nsHTMLDocument::SetScriptGlobalObject(aScriptGlobalObject);
  if (NS_FAILED(rv)) {
    return rv;
  }

  if (aScriptGlobalObject) {
    // Create synthetic document
    rv = CreateSyntheticDocument();
    if (NS_FAILED(rv)) {
      return rv;
    }

    if (mImageResizingEnabled) {
      nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(mImageElement);
      target->AddEventListener(NS_LITERAL_STRING("click"), this, PR_FALSE);

      target = do_QueryInterface(aScriptGlobalObject);
      target->AddEventListener(NS_LITERAL_STRING("resize"), this, PR_FALSE);
      target->AddEventListener(NS_LITERAL_STRING("keypress"), this, PR_FALSE);
    }
  }

  return NS_OK;
}


NS_IMETHODIMP
nsImageDocument::GetImageResizingEnabled(PRBool* aImageResizingEnabled)
{
  *aImageResizingEnabled = mImageResizingEnabled;
  return NS_OK;
}

NS_IMETHODIMP
nsImageDocument::GetImageIsOverflowing(PRBool* aImageIsOverflowing)
{
  *aImageIsOverflowing = mImageIsOverflowing;
  return NS_OK;
}

NS_IMETHODIMP
nsImageDocument::GetImageIsResized(PRBool* aImageIsResized)
{
  *aImageIsResized = mImageIsResized;
  return NS_OK;
}

NS_IMETHODIMP
nsImageDocument::ShrinkToFit()
{
  if (mImageResizingEnabled) {
    nsCOMPtr<nsIDOMHTMLImageElement> image = do_QueryInterface(mImageElement);

    float ratio = PR_MIN((float)mVisibleWidth / mImageWidth,
                         (float)mVisibleHeight / mImageHeight);
    image->SetWidth(NSToCoordFloor(mImageWidth * ratio));

    mImageElement->SetAttribute(NS_LITERAL_STRING("style"),
                                NS_LITERAL_STRING("cursor: move"));

    mImageIsResized = PR_TRUE;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsImageDocument::RestoreImage()
{
  if (mImageResizingEnabled) {
    mImageElement->RemoveAttribute(NS_LITERAL_STRING("width"));

    if (!mImageIsOverflowing) {
      mImageElement->RemoveAttribute(NS_LITERAL_STRING("style"));
    }

    mImageIsResized = PR_FALSE;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsImageDocument::ToggleImageSize()
{
  if (mImageResizingEnabled) {
    if (mImageIsResized) {
      RestoreImage();
    }
    else if (mImageIsOverflowing) {
      ShrinkToFit();
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsImageDocument::OnStartDecode(imgIRequest* aRequest)
{
  return NS_OK;
}

NS_IMETHODIMP
nsImageDocument::OnStartContainer(imgIRequest* aRequest, imgIContainer* aImage)
{
  aImage->GetWidth(&mImageWidth);
  aImage->GetHeight(&mImageHeight);
  if (mImageResizingEnabled) {
    CheckOverflowing();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsImageDocument::OnStartFrame(imgIRequest* aRequest, gfxIImageFrame* aFrame)
{
  return NS_OK;
}

NS_IMETHODIMP
nsImageDocument::OnDataAvailable(imgIRequest* aRequest,
                                 gfxIImageFrame* aFrame,
                                 const nsRect* aRect)
{
  return NS_OK;
}

NS_IMETHODIMP
nsImageDocument::OnStopFrame(imgIRequest* aRequest,
                             gfxIImageFrame* aFrame)
{
  return NS_OK;
}

NS_IMETHODIMP
nsImageDocument::OnStopContainer(imgIRequest* aRequest,
                                 imgIContainer* aImage)
{
  return NS_OK;
}

NS_IMETHODIMP
nsImageDocument::OnStopDecode(imgIRequest* aRequest,
                              nsresult status,
                              const PRUnichar* statusArg)
{
  return NS_OK;
}

NS_IMETHODIMP
nsImageDocument::FrameChanged(imgIContainer* aContainer,
                              gfxIImageFrame* aFrame,
                              nsRect* aDirtyRect)
{
  return NS_OK;
}


NS_IMETHODIMP
nsImageDocument::HandleEvent(nsIDOMEvent* aEvent)
{
  nsAutoString eventType;
  aEvent->GetType(eventType);
  if (eventType.Equals(NS_LITERAL_STRING("resize"))) {
    CheckOverflowing();
  }
  else if (eventType.Equals(NS_LITERAL_STRING("click"))) {
    ToggleImageSize();
  }
  else if (eventType.Equals(NS_LITERAL_STRING("keypress"))) {
    nsCOMPtr<nsIDOMKeyEvent> keyEvent = do_QueryInterface(aEvent);
    PRUint32 charCode;
    keyEvent->GetCharCode(&charCode);
    // plus key
    if (charCode == 0x2B) {
      if (mImageIsResized) {
        RestoreImage();
      }
    }
    // minus key
    else if (charCode == 0x2D) {
      if (mImageIsOverflowing) {
        ShrinkToFit();
      }
    }
  }

  return NS_OK;
}

nsresult
nsImageDocument::CreateSyntheticDocument()
{
  // Synthesize an html document that refers to the image
  nsresult rv = nsMediaDocument::CreateSyntheticDocument();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIHTMLContent> body = do_QueryInterface(mBodyContent);
  if (!body) {
    NS_WARNING("no body on image document!");
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsINodeInfo> nodeInfo;
  rv = mNodeInfoManager->GetNodeInfo(nsHTMLAtoms::img, nsnull,
                                     kNameSpaceID_None,
                                     *getter_AddRefs(nodeInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIHTMLContent> image;
  rv = NS_NewHTMLImageElement(getter_AddRefs(image), nodeInfo);
  if (NS_FAILED(rv)) {
    return rv;
  }
  image->SetDocument(this, PR_FALSE, PR_TRUE);
  mImageElement = do_QueryInterface(image);

  nsCAutoString src;
  mDocumentURL->GetSpec(src);

  NS_ConvertUTF8toUCS2 srcString(src);

  image->SetAttr(kNameSpaceID_None, nsHTMLAtoms::src, srcString, PR_FALSE);

  if (mStringBundle) {
    const PRUnichar* formatString[1] = { srcString.get() };
    nsXPIDLString errorMsg;
    NS_NAMED_LITERAL_STRING(str, "InvalidImage");
    mStringBundle->FormatStringFromName(str.get(), formatString, 1,
                                        getter_Copies(errorMsg));

    image->SetAttr(kNameSpaceID_None, nsHTMLAtoms::alt, errorMsg, PR_FALSE);
  }

  body->AppendChildTo(image, PR_FALSE, PR_FALSE);

  return NS_OK;
}

nsresult
nsImageDocument::CheckOverflowing()
{
  nsCOMPtr<nsIPresShell> shell;
  GetShellAt(0, getter_AddRefs(shell));
  if (!shell) {
    return NS_OK;
  }

  nsCOMPtr<nsIPresContext> context;
  shell->GetPresContext(getter_AddRefs(context));

  nsRect visibleArea;
  context->GetVisibleArea(visibleArea);

  nsCOMPtr<nsIContent> content = do_QueryInterface(mBodyContent);
  nsRefPtr<nsStyleContext> styleContext =
    context->ResolveStyleContextFor(content, nsnull);

  const nsStyleMargin* marginData =
    (const nsStyleMargin*)styleContext->GetStyleData(eStyleStruct_Margin);
  nsMargin margin;
  marginData->GetMargin(margin);
  visibleArea.Deflate(margin);

  nsStyleBorderPadding bPad;
  styleContext->GetBorderPaddingFor(bPad);
  bPad.GetBorderPadding(margin);
  visibleArea.Deflate(margin);

  float t2p;
  context->GetTwipsToPixels(&t2p);
  mVisibleWidth = NSTwipsToIntPixels(visibleArea.width, t2p);
  mVisibleHeight = NSTwipsToIntPixels(visibleArea.height, t2p);

  mImageIsOverflowing =
    mImageWidth > mVisibleWidth || mImageHeight > mVisibleHeight;

  if (mImageIsOverflowing) {
    ShrinkToFit();
  }
  else if (mImageIsResized) {
    RestoreImage();
  }

  return NS_OK;
}

nsresult
nsImageDocument::UpdateTitle()
{
  if (mStringBundle) {
    nsXPIDLString fileStr;
    nsAutoString widthStr;
    nsAutoString heightStr;
    nsAutoString typeStr;
    nsXPIDLString valUni;

    nsCOMPtr<nsIURL> url = do_QueryInterface(mDocumentURL);
    if (url) {
      nsresult rv;
      nsCAutoString fileName;
      url->GetFileName(fileName);
      nsCOMPtr<nsITextToSubURI> textToSubURI =
        do_GetService(NS_ITEXTTOSUBURI_CONTRACTID, &rv);
      if (NS_SUCCEEDED(rv))
      {
        nsCAutoString originCharset;
        rv = url->GetOriginCharset(originCharset);
        if (NS_SUCCEEDED(rv))
          rv = textToSubURI->UnEscapeURIForUI(originCharset, fileName, fileStr);
      }
      if (NS_FAILED(rv))
        fileStr.Assign(NS_ConvertUTF8toUCS2(fileName));
    }

    widthStr.AppendInt(mImageWidth);
    heightStr.AppendInt(mImageHeight);

    if (mImageRequest) {
      nsXPIDLCString mimeType;
      mImageRequest->GetMimeType(getter_Copies(mimeType));
      ToUpperCase(mimeType);
      nsXPIDLCString::const_iterator start, end;
      mimeType.BeginReading(start);
      mimeType.EndReading(end);
      nsXPIDLCString::const_iterator iter = end;
      if (FindInReadable(NS_LITERAL_CSTRING("IMAGE/"), start, iter) &&
          iter != end) {
        // strip out "X-" if any
        if (*iter == 'X') {
          ++iter;
          if (iter != end && *iter == '-') {
            ++iter;
            if (iter == end) {
              // looks like "IMAGE/X-" is the type??  Bail out of here.
              mimeType.BeginReading(iter);
            }
          } else {
            --iter;
          }
        }
        CopyASCIItoUCS2(Substring(iter, end), typeStr);
      } else {
        CopyASCIItoUCS2(mimeType, typeStr);
      }
    }

    // If we got a filename, display it
    if (!fileStr.IsEmpty()) {
      // if we got a valid size (sometimes we do not) then display it
      if (mImageWidth != 0 && mImageHeight != 0){
        const PRUnichar *formatStrings[4] =
          {
            fileStr.get(),
            typeStr.get(),
            widthStr.get(),
            heightStr.get()
          };
        NS_NAMED_LITERAL_STRING(str, "ImageTitleWithDimensionsAndFile");
        mStringBundle->FormatStringFromName(str.get(), formatStrings, 4,
                                            getter_Copies(valUni));
      } else {
        const PRUnichar *formatStrings[2] =
          {
            fileStr.get(),
            typeStr.get()
          };
        NS_NAMED_LITERAL_STRING(str, "ImageTitleWithoutDimensions");
        mStringBundle->FormatStringFromName(str.get(), formatStrings, 2,
                                            getter_Copies(valUni));
      }
    } else {
      // if we got a valid size (sometimes we do not) then display it
      if (mImageWidth != 0 && mImageHeight != 0){
        const PRUnichar *formatStrings[3] =
          {
            typeStr.get(),
            widthStr.get(),
            heightStr.get()
          };
        NS_NAMED_LITERAL_STRING(str, "ImageTitleWithDimensions");
        mStringBundle->FormatStringFromName(str.get(), formatStrings, 3,
                                            getter_Copies(valUni));
      } else {
        const PRUnichar *formatStrings[1] =
          {
            typeStr.get()
          };
        NS_NAMED_LITERAL_STRING(str, "ImageTitleWithNeitherDimensionsNorFile");
        mStringBundle->FormatStringFromName(str.get(), formatStrings, 1,
                                            getter_Copies(valUni));
      }
    }

    if (valUni) {
      // set it on the document
      SetTitle(valUni);
    }
  }

  return NS_OK;
}


nsresult
NS_NewImageDocument(nsIDocument** aResult)
{
  nsImageDocument* doc = new nsImageDocument();
  if (!doc) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsresult rv = doc->Init();

  if (NS_FAILED(rv)) {
    delete doc;
    return rv;
  }

  NS_ADDREF(*aResult = doc);

  return NS_OK;
}
