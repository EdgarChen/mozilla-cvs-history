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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */
#include "nsIDOMHTMLModElement.h"
#include "nsIScriptObjectOwner.h"
#include "nsIDOMEventReceiver.h"
#include "nsIHTMLContent.h"
#include "nsGenericHTMLElement.h"
#include "nsHTMLAtoms.h"
#include "nsHTMLIIDs.h"
#include "nsIStyleContext.h"
#include "nsIMutableStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIPresContext.h"


class nsHTMLInsElement : public nsIDOMHTMLModElement,
                  public nsIJSScriptObject,
                  public nsIHTMLContent
{
public:
  nsHTMLInsElement(nsINodeInfo *aNodeInfo);
  virtual ~nsHTMLInsElement();

  // nsISupports
  NS_DECL_ISUPPORTS

  // nsIDOMNode
  NS_IMPL_IDOMNODE_USING_GENERIC(mInner)

  // nsIDOMElement
  NS_IMPL_IDOMELEMENT_USING_GENERIC(mInner)

  // nsIDOMHTMLElement
  NS_IMPL_IDOMHTMLELEMENT_USING_GENERIC(mInner)

  // nsIDOMHTMLModElement
  NS_DECL_IDOMHTMLMODELEMENT

  // nsIJSScriptObject
  NS_IMPL_IJSSCRIPTOBJECT_USING_GENERIC(mInner)

  // nsIContent
  NS_IMPL_ICONTENT_USING_GENERIC(mInner)

  // nsIHTMLContent
  NS_IMPL_IHTMLCONTENT_USING_GENERIC(mInner)

protected:
  nsGenericHTMLContainerElement mInner;
};

nsresult
NS_NewHTMLInsElement(nsIHTMLContent** aInstancePtrResult,
                     nsINodeInfo *aNodeInfo)
{
  NS_ENSURE_ARG_POINTER(aInstancePtrResult);
  NS_ENSURE_ARG_POINTER(aNodeInfo);

  nsIHTMLContent* it = new nsHTMLInsElement(aNodeInfo);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return it->QueryInterface(NS_GET_IID(nsIHTMLContent), (void**) aInstancePtrResult);
}


nsHTMLInsElement::nsHTMLInsElement(nsINodeInfo *aNodeInfo)
{
  NS_INIT_REFCNT();
  mInner.Init(this, aNodeInfo);
}

nsHTMLInsElement::~nsHTMLInsElement()
{
}

NS_IMPL_ADDREF(nsHTMLInsElement)

NS_IMPL_RELEASE(nsHTMLInsElement)

nsresult
nsHTMLInsElement::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  NS_IMPL_HTML_CONTENT_QUERY_INTERFACE(aIID, aInstancePtr, this)
  if (aIID.Equals(NS_GET_IID(nsIDOMHTMLModElement))) {
    nsIDOMHTMLModElement* tmp = this;
    *aInstancePtr = (void*) tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

nsresult
nsHTMLInsElement::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{
  nsHTMLInsElement* it = new nsHTMLInsElement(mInner.mNodeInfo);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  nsCOMPtr<nsIDOMNode> kungFuDeathGrip(it);
  mInner.CopyInnerTo(this, &it->mInner, aDeep);
  return it->QueryInterface(NS_GET_IID(nsIDOMNode), (void**) aReturn);
}

NS_IMPL_STRING_ATTR(nsHTMLInsElement, Cite, cite)
NS_IMPL_STRING_ATTR(nsHTMLInsElement, DateTime, datetime)

NS_IMETHODIMP
nsHTMLInsElement::StringToAttribute(nsIAtom* aAttribute,
                             const nsAReadableString& aValue,
                             nsHTMLValue& aResult)
{
  // XXX write me
  return NS_CONTENT_ATTR_NOT_THERE;
}

NS_IMETHODIMP
nsHTMLInsElement::AttributeToString(nsIAtom* aAttribute,
                             const nsHTMLValue& aValue,
                             nsAWritableString& aResult) const
{
  // XXX write me
  return mInner.AttributeToString(aAttribute, aValue, aResult);
}

static void
MapAttributesInto(const nsIHTMLMappedAttributes* aAttributes,
                  nsIMutableStyleContext* aContext,
                  nsIPresContext* aPresContext)
{
  // XXX write me
  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aContext, aPresContext);
}

NS_IMETHODIMP
nsHTMLInsElement::GetMappedAttributeImpact(const nsIAtom* aAttribute,
                                           PRInt32& aHint) const
{
  if (! nsGenericHTMLElement::GetCommonMappedAttributesImpact(aAttribute, aHint)) {
    aHint = NS_STYLE_HINT_CONTENT;
  }

  return NS_OK;
}


NS_IMETHODIMP
nsHTMLInsElement::GetAttributeMappingFunctions(nsMapAttributesFunc& aFontMapFunc,
                                               nsMapAttributesFunc& aMapFunc) const
{
  aFontMapFunc = nsnull;
  aMapFunc = &MapAttributesInto;
  return NS_OK;
}


NS_IMETHODIMP
nsHTMLInsElement::HandleDOMEvent(nsIPresContext* aPresContext,
                          nsEvent* aEvent,
                          nsIDOMEvent** aDOMEvent,
                          PRUint32 aFlags,
                          nsEventStatus* aEventStatus)
{
  return mInner.HandleDOMEvent(aPresContext, aEvent, aDOMEvent,
                               aFlags, aEventStatus);
}


NS_IMETHODIMP
nsHTMLInsElement::SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult) const
{
  return mInner.SizeOf(aSizer, aResult, sizeof(*this));
}
