/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * ***** BEGIN LICENSE BLOCK *****
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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Chris Waterson <waterson@netscape.com>
 *   Pierre Phaneuf <pp@ludusdesign.com>
 *   Peter Annema <disttsc@bart.nl>
 *   Brendan Eich <brendan@mozilla.org>
 *   Mike Shaver <shaver@mozilla.org>
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
 * ***** END LICENSE BLOCK *****
 *
 * This Original Code has been modified by IBM Corporation.
 * Modifications made by IBM described herein are
 * Copyright (c) International Business Machines
 * Corporation, 2000
 *
 * Modifications to Mozilla code or documentation
 * identified per MPL Section 3.3
 *
 * Date         Modified by     Description of modification
 * 03/27/2000   IBM Corp.       Added PR_CALLBACK for Optlink
 *                               use in OS2
 */

#include "jsapi.h"      // for JS_AddNamedRoot and JS_RemoveRootRT
#include "jsxdrapi.h"
#include "nsCOMPtr.h"
#include "nsDOMCID.h"
#include "nsDOMError.h"
#include "nsDOMString.h"
#include "nsIDOMEvent.h"
#include "nsIPrivateDOMEvent.h"
#include "nsHashtable.h"
#include "nsIAtom.h"
#include "nsIDOMAttr.h"
#include "nsIDOMDocument.h"
#include "nsIDOMElement.h"
#include "nsIDOMMouseListener.h"
#include "nsIDOMMouseMotionListener.h"
#include "nsIDOMLoadListener.h"
#include "nsIDOMFocusListener.h"
#include "nsIDOMPaintListener.h"
#include "nsIDOMKeyListener.h"
#include "nsIDOMFormListener.h"
#include "nsIDOMXULListener.h"
#include "nsIDOMScrollListener.h"
#include "nsIDOMContextMenuListener.h"
#include "nsIDOMDragListener.h"
#include "nsIDOMEventListener.h"
#include "nsIDOMEventReceiver.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMXULCommandDispatcher.h"
#include "nsIDOMXULElement.h"
#include "nsIDOMXULSelectCntrlItemEl.h"
#include "nsIDocument.h"
#include "nsIEventListenerManager.h"
#include "nsIEventStateManager.h"
#include "nsIFastLoadService.h"
#include "nsHTMLStyleSheet.h"
#include "nsINameSpaceManager.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"
#include "nsIPresShell.h"
#include "nsIPrincipal.h"
#include "nsIRDFCompositeDataSource.h"
#include "nsIRDFNode.h"
#include "nsIRDFService.h"
#include "nsIScriptContext.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptGlobalObjectOwner.h"
#include "nsIServiceManager.h"
#include "nsICSSStyleRule.h"
#include "nsIStyleSheet.h"
#include "nsIURL.h"
#include "nsIViewManager.h"
#include "nsIWidget.h"
#include "nsIXULDocument.h"
#include "nsIXULPopupListener.h"
#include "nsIXULPrototypeDocument.h"
#include "nsIXULTemplateBuilder.h"
#include "nsIXBLService.h"
#include "nsLayoutCID.h"
#include "nsContentCID.h"
#include "nsRDFCID.h"
#include "nsStyleConsts.h"
#include "nsXPIDLString.h"
#include "nsXULControllers.h"
#include "nsIBoxObject.h"
#include "nsPIBoxObject.h"
#include "nsXULDocument.h"
#include "nsRuleWalker.h"
#include "nsIDOMViewCSS.h"
#include "nsIDOMCSSStyleDeclaration.h"
#include "nsCSSDeclaration.h"
#include "nsIListBoxObject.h"
#include "nsContentUtils.h"
#include "nsContentList.h"
#include "nsMutationEvent.h"
#include "nsIDOMMutationEvent.h"
#include "nsPIDOMWindow.h"
#include "nsDOMAttributeMap.h"
#include "nsDOMCSSDeclaration.h"
#include "nsGenericHTMLElement.h"
#include "nsHTMLAtoms.h"
#include "nsLayoutAtoms.h"
#include "nsXULContentUtils.h"

#include "prlog.h"
#include "rdf.h"

#include "nsIControllers.h"

// The XUL doc interface
#include "nsIDOMXULDocument.h"

#include "nsReadableUtils.h"
#include "nsITimelineService.h"
#include "nsIFrame.h"
#include "nsNodeInfoManager.h"
#include "nsXBLBinding.h"
#include "nsRange.h"
#include "nsEventDispatcher.h"

/**
 * Three bits are used for XUL Element's lazy state.
 */
#define XUL_ELEMENT_CHILDREN_MUST_BE_REBUILT \
  (nsXULElement::eChildrenMustBeRebuilt << XUL_ELEMENT_LAZY_STATE_OFFSET)

#define XUL_ELEMENT_TEMPLATE_CONTENTS_BUILT \
  (nsXULElement::eTemplateContentsBuilt << XUL_ELEMENT_LAZY_STATE_OFFSET)

#define XUL_ELEMENT_CONTAINER_CONTENTS_BUILT \
  (nsXULElement::eContainerContentsBuilt << XUL_ELEMENT_LAZY_STATE_OFFSET)

class nsIDocShell;

// Global object maintenance
nsICSSParser* nsXULPrototypeElement::sCSSParser = nsnull;
nsIXULPrototypeCache* nsXULPrototypeScript::sXULPrototypeCache = nsnull;
nsIXBLService * nsXULElement::gXBLService = nsnull;
nsICSSOMFactory* nsXULElement::gCSSOMFactory = nsnull;

//----------------------------------------------------------------------

static NS_DEFINE_CID(kXULPopupListenerCID,        NS_XULPOPUPLISTENER_CID);
static NS_DEFINE_CID(kCSSOMFactoryCID,            NS_CSSOMFACTORY_CID);

//----------------------------------------------------------------------


// XXX This function is called for every attribute on every element for
// XXX which we SetDocument, among other places.  A linear search might
// XXX not be what we want.
static PRBool
IsEventHandler(nsIAtom* aName)
{
    const char* name;
    aName->GetUTF8String(&name);

    if (name[0] != 'o' || name[1] != 'n') {
        return PR_FALSE;
    }
    
    return aName == nsLayoutAtoms::onclick            ||
           aName == nsLayoutAtoms::ondblclick         ||
           aName == nsLayoutAtoms::onmousedown        ||
           aName == nsLayoutAtoms::onmouseup          ||
           aName == nsLayoutAtoms::onmouseover        ||
           aName == nsLayoutAtoms::onmouseout         ||
           aName == nsLayoutAtoms::onmousemove        ||

           aName == nsLayoutAtoms::onkeydown          ||
           aName == nsLayoutAtoms::onkeyup            ||
           aName == nsLayoutAtoms::onkeypress         ||

           aName == nsLayoutAtoms::oncompositionstart ||
           aName == nsLayoutAtoms::oncompositionend   ||

           aName == nsLayoutAtoms::onload             ||
           aName == nsLayoutAtoms::onunload           ||
           aName == nsLayoutAtoms::onabort            ||
           aName == nsLayoutAtoms::onerror            ||

           aName == nsLayoutAtoms::onpopupshowing     ||
           aName == nsLayoutAtoms::onpopupshown       ||
           aName == nsLayoutAtoms::onpopuphiding      ||
           aName == nsLayoutAtoms::onpopuphidden      ||
           aName == nsLayoutAtoms::onclose            ||
           aName == nsLayoutAtoms::oncommand          ||
           aName == nsLayoutAtoms::onbroadcast        ||
           aName == nsLayoutAtoms::oncommandupdate    ||

           aName == nsLayoutAtoms::onoverflow         ||
           aName == nsLayoutAtoms::onunderflow        ||
           aName == nsLayoutAtoms::onoverflowchanged  ||

           aName == nsLayoutAtoms::onfocus            ||
           aName == nsLayoutAtoms::onblur             ||

           aName == nsLayoutAtoms::onsubmit           ||
           aName == nsLayoutAtoms::onreset            ||
           aName == nsLayoutAtoms::onchange           ||
           aName == nsLayoutAtoms::onselect           ||
           aName == nsLayoutAtoms::oninput            ||

           aName == nsLayoutAtoms::onpaint            ||

           aName == nsLayoutAtoms::ondragenter        ||
           aName == nsLayoutAtoms::ondragover         ||
           aName == nsLayoutAtoms::ondragexit         ||
           aName == nsLayoutAtoms::ondragdrop         ||
           aName == nsLayoutAtoms::ondraggesture      ||

           aName == nsLayoutAtoms::oncontextmenu;
}

//----------------------------------------------------------------------

#ifdef XUL_PROTOTYPE_ATTRIBUTE_METERING
PRUint32             nsXULPrototypeAttribute::gNumElements;
PRUint32             nsXULPrototypeAttribute::gNumAttributes;
PRUint32             nsXULPrototypeAttribute::gNumEventHandlers;
PRUint32             nsXULPrototypeAttribute::gNumCacheTests;
PRUint32             nsXULPrototypeAttribute::gNumCacheHits;
PRUint32             nsXULPrototypeAttribute::gNumCacheSets;
PRUint32             nsXULPrototypeAttribute::gNumCacheFills;
#endif

//----------------------------------------------------------------------
// nsXULElement
//

nsXULElement::nsXULElement(nsINodeInfo* aNodeInfo)
    : nsGenericElement(aNodeInfo),
      mBindingParent(nsnull)
{
    XUL_PROTOTYPE_ATTRIBUTE_METER(gNumElements);
}

nsXULElement::~nsXULElement()
{
    //XXX UnbindFromTree is not called always before dtor.
    //XXX Related to templates or overlays?
    //XXXbz probably related to the cloning thing!
    if (IsInDoc()) {
      UnbindFromTree();
    }

    nsDOMSlots* slots = GetExistingDOMSlots();
    if (slots) {
      NS_IF_RELEASE(slots->mControllers); // Forces release
    }
}

/* static */
already_AddRefed<nsXULElement>
nsXULElement::Create(nsXULPrototypeElement* aPrototype, nsINodeInfo *aNodeInfo,
                     PRBool aIsScriptable)
{
    nsXULElement *element = new nsXULElement(aNodeInfo);
    if (element) {
        NS_ADDREF(element);

        element->mPrototype = aPrototype;

        if (aIsScriptable) {
            // Check each attribute on the prototype to see if we need to do
            // any additional processing and hookup that would otherwise be
            // done 'automagically' by SetAttr().
            for (PRUint32 i = 0; i < aPrototype->mNumAttributes; ++i) {
                element->AddListenerFor(aPrototype->mAttributes[i].mName,
                                        PR_TRUE);
            }
        }
    }

    return element;
}

nsresult
nsXULElement::Create(nsXULPrototypeElement* aPrototype,
                     nsIDocument* aDocument,
                     PRBool aIsScriptable,
                     nsIContent** aResult)
{
    // Create an nsXULElement from a prototype
    NS_PRECONDITION(aPrototype != nsnull, "null ptr");
    if (! aPrototype)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aResult != nsnull, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    nsCOMPtr<nsINodeInfo> nodeInfo;
    nsresult rv;
    if (aDocument) {
        nsINodeInfo* ni = aPrototype->mNodeInfo;
        rv = aDocument->NodeInfoManager()->GetNodeInfo(ni->NameAtom(),
                                                       ni->GetPrefixAtom(),
                                                       ni->NamespaceID(),
                                                       getter_AddRefs(nodeInfo));
        NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
        nodeInfo = aPrototype->mNodeInfo;
    }

    nsRefPtr<nsXULElement> element = Create(aPrototype, nodeInfo,
                                            aIsScriptable);
    if (!element) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    element->mPrototype = aPrototype;

    NS_ADDREF(*aResult = element.get());

    return NS_OK;
}

nsresult
NS_NewXULElement(nsIContent** aResult, nsINodeInfo *aNodeInfo)
{
    NS_PRECONDITION(aNodeInfo, "need nodeinfo for non-proto Create");

    *aResult = nsnull;

    // Create an nsXULElement with the specified namespace and tag.
    nsXULElement* element = new nsXULElement(aNodeInfo);
    NS_ENSURE_TRUE(element, NS_ERROR_OUT_OF_MEMORY);

    NS_ADDREF(*aResult = element);

    return NS_OK;
}

//----------------------------------------------------------------------
// nsISupports interface

NS_IMPL_ADDREF_INHERITED(nsXULElement, nsGenericElement)
NS_IMPL_RELEASE_INHERITED(nsXULElement, nsGenericElement)

NS_IMETHODIMP
nsXULElement::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
    NS_ENSURE_ARG_POINTER(aInstancePtr);
    *aInstancePtr = nsnull;


    nsresult rv = nsGenericElement::QueryInterface(aIID, aInstancePtr);
    if (NS_SUCCEEDED(rv))
        return rv;

    nsISupports *inst = nsnull;

    if (aIID.Equals(NS_GET_IID(nsIDOMNode))) {
        inst = NS_STATIC_CAST(nsIDOMNode *, this);
    } else if (aIID.Equals(NS_GET_IID(nsIDOMElement))) {
        inst = NS_STATIC_CAST(nsIDOMElement *, this);
    } else if (aIID.Equals(NS_GET_IID(nsIDOMXULElement))) {
        inst = NS_STATIC_CAST(nsIDOMXULElement *, this);
    } else if (aIID.Equals(NS_GET_IID(nsIXMLContent))) {
        inst = NS_STATIC_CAST(nsIXMLContent *, this);
    } else if (aIID.Equals(NS_GET_IID(nsIScriptEventHandlerOwner))) {
        inst = NS_STATIC_CAST(nsIScriptEventHandlerOwner *, this);
    } else if (aIID.Equals(NS_GET_IID(nsIChromeEventHandler))) {
        inst = NS_STATIC_CAST(nsIChromeEventHandler *, this);
    } else if (aIID.Equals(NS_GET_IID(nsIClassInfo))) {
        inst = nsContentUtils::GetClassInfoInstance(eDOMClassInfo_XULElement_id);
        NS_ENSURE_TRUE(inst, NS_ERROR_OUT_OF_MEMORY);
    } else {
        return PostQueryInterface(aIID, aInstancePtr);
    }

    NS_ADDREF(inst);
 
    *aInstancePtr = inst;
    return NS_OK;
}

//----------------------------------------------------------------------
// nsIDOMNode interface

nsresult
nsXULElement::Clone(nsINodeInfo *aNodeInfo, PRBool aDeep,
                    nsIContent **aResult) const
{
    *aResult = nsnull;

    // XXX setting document on some nodes not in a document so XBL will bind
    // and chrome won't break. Make XBL bind to document-less nodes!
    // XXXbz Once this is fixed, fix up the asserts in all implementations of
    // BindToTree to assert what they would like to assert, and fix the
    // ChangeDocumentFor() call in nsXULElement::BindToTree as well.  Also,
    // remove the UnbindFromTree call in ~nsXULElement, and add back in the
    // precondition in nsXULElement::UnbindFromTree.
    // Note: Make sure to do this witchery _after_ we've done any deep
    // cloning, so kids of the new node aren't confused about whether they're
    // in a document.
    
    PRBool fakeBeingInDocument = PR_TRUE;
    
    // If we have a prototype, so will our clone.
    nsRefPtr<nsXULElement> element;
    if (mPrototype) {
        element = nsXULElement::Create(mPrototype, aNodeInfo, PR_TRUE);

        fakeBeingInDocument = IsInDoc();
    }
    else {
        element = new nsXULElement(aNodeInfo);
    }

    if (!element) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    // XXX TODO: set up RDF generic builder n' stuff if there is a
    // 'datasources' attribute? This is really kind of tricky,
    // because then we'd need to -selectively- copy children that
    // -weren't- generated from RDF. Ugh. Forget it.

    // Note that we're _not_ copying mControllers.

    nsresult rv = CopyInnerTo(element, aDeep);
    if (NS_SUCCEEDED(rv)) {
        NS_ADDREF(*aResult = element);
    }

    if (fakeBeingInDocument) {
        // Don't use BindToTree here so we don't confuse the descendant
        // non-XUL nodes.
        element->mParentPtrBits |= PARENT_BIT_INDOCUMENT;
    }

    return rv;
}

NS_IMETHODIMP
nsXULElement::CloneNode(PRBool aDeep, nsIDOMNode **aResult)
{
    return nsGenericElement::CloneNode(aDeep, this, aResult);
}

//----------------------------------------------------------------------

NS_IMETHODIMP
nsXULElement::GetElementsByAttribute(const nsAString& aAttribute,
                                     const nsAString& aValue,
                                     nsIDOMNodeList** aReturn)
{
    nsCOMPtr<nsIAtom> attrAtom(do_GetAtom(aAttribute));
    NS_ENSURE_TRUE(attrAtom, NS_ERROR_OUT_OF_MEMORY);

    nsContentList *list = 
        new nsContentList(GetDocument(),
                          nsXULDocument::MatchAttribute,
                          aValue,
                          this,
                          PR_TRUE,
                          attrAtom,
                          kNameSpaceID_None);
    NS_ENSURE_TRUE(list, NS_ERROR_OUT_OF_MEMORY);

    NS_ADDREF(*aReturn = list);
    return NS_OK;
}


//----------------------------------------------------------------------
// nsIXMLContent interface

NS_IMETHODIMP
nsXULElement::MaybeTriggerAutoLink(nsIDocShell *aShell)
{
  return NS_OK;
}

nsresult
nsXULElement::GetEventListenerManagerForAttr(nsIEventListenerManager** aManager,
                                             nsISupports** aTarget,
                                             PRBool* aDefer)
{
    // XXXbz sXBL/XBL2 issue: should we instead use GetCurrentDoc()
    // here, override BindToTree for those classes and munge event
    // listeners there?
    nsIDocument* doc = GetOwnerDoc();
    if (!doc)
        return NS_ERROR_UNEXPECTED; // XXX

    nsIContent *root = doc->GetRootContent();
    if ((!root || root == this) && !mNodeInfo->Equals(nsXULAtoms::overlay)) {
        nsPIDOMWindow *window = doc->GetInnerWindow();

        nsCOMPtr<nsIDOMEventReceiver> receiver = do_QueryInterface(window);
        if (!receiver)
            return NS_ERROR_UNEXPECTED;

        nsresult rv = receiver->GetListenerManager(PR_TRUE, aManager);
        if (NS_SUCCEEDED(rv)) {
            NS_ADDREF(*aTarget = window);
        }
        *aDefer = PR_FALSE;
        return rv;
    }

    return nsGenericElement::GetEventListenerManagerForAttr(aManager,
                                                            aTarget,
                                                            aDefer);
}

nsresult
nsXULElement::GetListenerManager(PRBool aCreateIfNotFound,
                                 nsIEventListenerManager** aResult)
{
    if (!mListenerManager) {
        if (!aCreateIfNotFound) {
            *aResult = nsnull;
            return NS_OK;
        }

        nsresult rv =
            NS_NewEventListenerManager(getter_AddRefs(mListenerManager));
        if (NS_FAILED(rv))
            return rv;

        mListenerManager->SetListenerTarget(NS_STATIC_CAST(nsIContent*, this));
    }

    *aResult = mListenerManager;
    NS_ADDREF(*aResult);
    return NS_OK;
}

PRBool
nsXULElement::IsFocusable(PRInt32 *aTabIndex)
{
  // Use incoming tabindex as default value
  PRInt32 tabIndex = aTabIndex? *aTabIndex : -1;
  PRBool disabled = tabIndex < 0;
  nsCOMPtr<nsIDOMXULControlElement> xulControl = 
    do_QueryInterface(NS_STATIC_CAST(nsIContent*, this));
  if (xulControl) {
    xulControl->GetDisabled(&disabled);
    if (disabled) {
      tabIndex = -1;  // Can't tab to disabled elements
    }
    else if (HasAttr(kNameSpaceID_None, nsHTMLAtoms::tabindex)) {
      // If attribute not set, will use default value passed in
      xulControl->GetTabIndex(&tabIndex);
    }
    if (tabIndex != -1 && sTabFocusModelAppliesToXUL &&
        !(sTabFocusModel & eTabFocus_formElementsMask)) {
      // By default, the tab focus model doesn't apply to xul element on any system but OS X.
      // on OS X we're following it for UI elements (XUL) as sTabFocusModel is based on
      // "Full Keyboard Access" system setting (see mac/nsILookAndFeel).
      // both textboxes and list elements (i.e. trees and list) should always be focusable
      // (textboxes are handled as html:input)
      if (!mNodeInfo->Equals(nsXULAtoms::tree) && !mNodeInfo->Equals(nsXULAtoms::listbox))
        tabIndex = -1; 
    }
  }

  if (aTabIndex) {
    *aTabIndex = tabIndex;
  }

  return tabIndex >= 0 || (!disabled && HasAttr(kNameSpaceID_None, nsHTMLAtoms::tabindex));
}


//----------------------------------------------------------------------
// nsIScriptEventHandlerOwner interface

nsresult
nsXULElement::GetCompiledEventHandler(nsIAtom *aName, void** aHandler)
{
    XUL_PROTOTYPE_ATTRIBUTE_METER(gNumCacheTests);
    *aHandler = nsnull;

    nsXULPrototypeAttribute *attr =
        FindPrototypeAttribute(kNameSpaceID_None, aName);
    if (attr) {
        XUL_PROTOTYPE_ATTRIBUTE_METER(gNumCacheHits);
        *aHandler = attr->mEventHandler;
    }

    return NS_OK;
}

nsresult
nsXULElement::CompileEventHandler(nsIScriptContext* aContext,
                                  void* aTarget,
                                  nsIAtom *aName,
                                  const nsAString& aBody,
                                  const char* aURL,
                                  PRUint32 aLineNo,
                                  void** aHandler)
{
    nsresult rv;
    JSObject* scopeObject;

    XUL_PROTOTYPE_ATTRIBUTE_METER(gNumCacheSets);

    // XXX sXBL/XBL2 issue! Owner or current document?
    nsCOMPtr<nsIXULDocument> xuldoc = do_QueryInterface(GetOwnerDoc());

    nsIScriptContext *context;
    if (mPrototype && xuldoc) {
        // It'll be shared among the instances of the prototype.
        // Use null for the scope object when precompiling shared
        // prototype scripts.
        scopeObject = nsnull;

        // Use the prototype document's special context.  Because
        // scopeObject is null, the JS engine has no other source of
        // <the-new-shared-event-handler>.__proto__ than to look in
        // cx->globalObject for Function.prototype.  That prototype
        // keeps the global object alive, so if we use this document's
        // global object, we'll be putting something in the prototype
        // that protects this document's global object from GC.
        nsCOMPtr<nsIXULPrototypeDocument> protodoc;
        rv = xuldoc->GetMasterPrototype(getter_AddRefs(protodoc));
        NS_ENSURE_SUCCESS(rv, rv);
        NS_ENSURE_TRUE(protodoc, NS_ERROR_UNEXPECTED);

        nsCOMPtr<nsIScriptGlobalObjectOwner> globalOwner =
            do_QueryInterface(protodoc);
        nsIScriptGlobalObject* global = globalOwner->GetScriptGlobalObject();
        NS_ENSURE_TRUE(global, NS_ERROR_UNEXPECTED);

        context = global->GetContext();
    }
    else {
        // We don't have a prototype; do a one-off compile.
        NS_ASSERTION(aTarget != nsnull, "no prototype and no target?!");
        scopeObject = NS_REINTERPRET_CAST(JSObject*, aTarget);
        context = aContext;
    }

    // Compile the event handler
    const char *eventName = nsContentUtils::GetEventArgName(kNameSpaceID_XUL);
    rv = context->CompileEventHandler(scopeObject, aName, eventName, aBody,
                                      aURL, aLineNo, !scopeObject,
                                      aHandler);
    if (NS_FAILED(rv)) return rv;

    if (! scopeObject) {
        // If it's a shared handler, we need to bind the shared
        // function object to the real target.

        // XXX: Shouldn't this use context and not aContext?
        rv = aContext->BindCompiledEventHandler(aTarget, aName, *aHandler);
        if (NS_FAILED(rv)) return rv;
    }

    nsXULPrototypeAttribute *attr =
        FindPrototypeAttribute(kNameSpaceID_None, aName);
    if (attr) {
        XUL_PROTOTYPE_ATTRIBUTE_METER(gNumCacheFills);
        attr->mEventHandler = *aHandler;

        if (attr->mEventHandler) {
            JSContext *cx = (JSContext*) context->GetNativeContext();
            if (!cx)
                return NS_ERROR_UNEXPECTED;

            rv = nsContentUtils::AddJSGCRoot(&attr->mEventHandler,
                                             "nsXULPrototypeAttribute::mEventHandler");
            if (NS_FAILED(rv)) {
                attr->mEventHandler = nsnull;
                return rv;
            }
        }
    }

    return NS_OK;
}


void
nsXULElement::AddListenerFor(const nsAttrName& aName,
                             PRBool aCompileEventHandlers)
{
    // If appropriate, add a popup listener and/or compile the event
    // handler. Called when we change the element's document, create a
    // new element, change an attribute's value, etc.
    // Eventlistenener-attributes are always in the null namespace
    if (aName.IsAtom()) {
        nsIAtom *attr = aName.Atom();
        MaybeAddPopupListener(attr);
        if (aCompileEventHandlers && IsEventHandler(attr)) {
            nsAutoString value;
            GetAttr(kNameSpaceID_None, attr, value);
            AddScriptEventListener(attr, value);
        }
    }
}

void
nsXULElement::MaybeAddPopupListener(nsIAtom* aLocalName)
{
    // If appropriate, add a popup listener. Called when we change the
    // element's document, create a new element, change an attribute's
    // value, etc.
    if (aLocalName == nsXULAtoms::menu ||
        aLocalName == nsXULAtoms::contextmenu ||
        // XXXdwh popup and context are deprecated
        aLocalName == nsXULAtoms::popup ||
        aLocalName == nsXULAtoms::context) {
        AddPopupListener(aLocalName);
    }
}

//----------------------------------------------------------------------
//
// nsIContent interface
//

nsresult
nsXULElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                         nsIContent* aBindingParent,
                         PRBool aCompileEventHandlers)
{
    NS_PRECONDITION(aParent || aDocument, "Must have document if no parent!");
    // XXXbz XUL elements are confused about their current doc when they're
    // cloned, so we don't assert if aParent is a XUL element and aDocument is
    // null, even if aParent->GetCurrentDoc() is non-null
    //  NS_PRECONDITION(!aParent || aDocument == aParent->GetCurrentDoc(),
    //                  "aDocument must be current doc of aParent");
    NS_PRECONDITION(!aParent ||
                    (aParent->IsContentOfType(eXUL) && aDocument == nsnull) ||
                    aDocument == aParent->GetCurrentDoc(),
                    "aDocument must be current doc of aParent");
    // XXXbz we'd like to assert that GetCurrentDoc() is null, but the cloning
    // mess makes that impossible.  We can't even assert that aDocument ==
    // GetCurrentDoc() when GetCurrentDoc() is non-null, since we may be
    // getting inserted into a different document.  :(
    //    NS_PRECONDITION(!GetCurrentDoc(),
    //                    "Already have a document.  Unbind first!");

    // Note that as we recurse into the kids, they'll have a non-null
    // parent.  So only assert if our parent is _changing_ while we
    // have a parent.
    NS_PRECONDITION(!GetParent() || aParent == GetParent(),
                    "Already have a parent.  Unbind first!");
    NS_PRECONDITION(!GetBindingParent() ||
                    aBindingParent == GetBindingParent() ||
                    (!aBindingParent && aParent &&
                     aParent->GetBindingParent() == GetBindingParent()),
                    "Already have a binding parent.  Unbind first!");

    nsresult rv;

    if (!aBindingParent && aParent) {
        aBindingParent = aParent->GetBindingParent();
    }

    // First set the binding parent
    mBindingParent = aBindingParent;
    
    // Now set the parent; make sure to preserve the bits we have
    // stashed there Note that checking whether aParent == GetParent()
    // is probably not worth it here.
    PtrBits new_bits = NS_REINTERPRET_CAST(PtrBits, aParent);
    new_bits |= mParentPtrBits & nsIContent::kParentBitMask;
    mParentPtrBits = new_bits;

    // Finally, set the document
    if (aDocument && aDocument != GetCurrentDoc()) {
        // Notify XBL- & nsIAnonymousContentCreator-generated
        // anonymous content that the document is changing.
        // XXXbz ordering issues here?  Probably not, since ChangeDocumentFor
        // is just pretty broken anyway....  Need to get it working.
        // XXXbz XBL doesn't handle this (asserts), and we don't really want
        // to be doing this during parsing anyway... sort this out.    
        //    aDocument->BindingManager()->ChangeDocumentFor(this, nsnull,
        //                                                   aDocument);
        
        // Being added to a document.
        mParentPtrBits |= PARENT_BIT_INDOCUMENT;

        // check the document on the nodeinfo to see whether we need a
        // new nodeinfo
        // XXXbz sXBL/XBL2 issue!
        nsIDocument *ownerDocument = GetOwnerDoc();
        if (aDocument != ownerDocument) {
            if (ownerDocument && HasProperties()) {
                // Copy UserData to the new document.
                ownerDocument->CopyUserData(this, aDocument);

                // Remove all properties.
                ownerDocument->PropertyTable()->
                  DeleteAllPropertiesFor(NS_STATIC_CAST(nsINode*, this));
            }

            // get a new nodeinfo
            nsNodeInfoManager* nodeInfoManager = aDocument->NodeInfoManager();
            if (nodeInfoManager) {
                nsCOMPtr<nsINodeInfo> newNodeInfo;
                nsresult rv =
                    nodeInfoManager->GetNodeInfo(mNodeInfo->NameAtom(),
                                                 mNodeInfo->GetPrefixAtom(),
                                                 mNodeInfo->NamespaceID(),
                                                 getter_AddRefs(newNodeInfo));
                NS_ENSURE_SUCCESS(rv, rv);
                NS_ASSERTION(newNodeInfo, "GetNodeInfo lies");
                mNodeInfo.swap(newNodeInfo);
            }

            // set a new nodeinfo on attribute nodes
            nsDOMSlots *slots = GetExistingDOMSlots();
            if (slots && slots->mAttributeMap) {
              rv = slots->mAttributeMap->SetOwnerDocument(aDocument);
              NS_ENSURE_SUCCESS(rv, rv);
            }
        }

        // we need to (re-)initialize several attributes that are dependant on
        // the document. Do that now.
        // XXXbz why do we have attributes depending on the current document?
        // Shouldn't they depend on the owner document?  Or is this code just
        // misplaced, basically?
        
        PRInt32 count = mAttrsAndChildren.AttrCount();
        PRBool haveLocalAttributes = (count > 0);
        PRInt32 i;
        for (i = 0; i < count; i++) {
            AddListenerFor(*mAttrsAndChildren.AttrNameAt(i),
                           aCompileEventHandlers);
        }

        if (mPrototype) {
            PRInt32 count = mPrototype->mNumAttributes;
            for (i = 0; i < count; i++) {
                nsXULPrototypeAttribute *protoattr =
                    &mPrototype->mAttributes[i];

                // Don't clobber a locally modified attribute.
                if (haveLocalAttributes &&
                    mAttrsAndChildren.GetAttr(protoattr->mName.LocalName(), 
                                              protoattr->mName.NamespaceID())) {
                    continue;
                }

                AddListenerFor(protoattr->mName, aCompileEventHandlers);
            }
        }
    }

    // Now recurse into our kids
    PRUint32 i, n = GetChildCount();

    for (i = 0; i < n; ++i) {
        rv = mAttrsAndChildren.ChildAt(i)->BindToTree(aDocument, this,
                                                      aBindingParent,
                                                      aCompileEventHandlers);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    // XXXbz script execution during binding can trigger some of these
    // postcondition asserts....  But we do want that, since things will
    // generally be quite broken when that happens.
    // XXXbz we'd like to assert that we have the right GetCurrentDoc(), but
    // we may be being bound to a null document while we already have a
    // current doc, due to the cloneNode hack...  So can't assert that yet.
    //    NS_POSTCONDITION(aDocument == GetCurrentDoc(),
    //                     "Bound to wrong document");
    NS_POSTCONDITION(aParent == GetParent(), "Bound to wrong parent");
    NS_POSTCONDITION(aBindingParent == GetBindingParent(),
                     "Bound to wrong binding parent");

    return NS_OK;
}

void
nsXULElement::UnbindFromTree(PRBool aDeep, PRBool aNullParent)
{
    // XXXbz we'd like to assert that called didn't screw up aDeep, but I'm not
    // sure we can....
    //    NS_PRECONDITION(aDeep || (!GetCurrentDoc() && !GetBindingParent()),
    //                    "Shallow unbind won't clear document and binding "
    //                    "parent on kids!");
    // Make sure to unbind this node before doing the kids
    nsIDocument *document = GetCurrentDoc();
    if (document) {
        // Notify XBL- & nsIAnonymousContentCreator-generated
        // anonymous content that the document is changing.
        document->BindingManager()->ChangeDocumentFor(this, document, nsnull);

        nsCOMPtr<nsIDOMNSDocument> nsDoc(do_QueryInterface(document));
        nsDoc->SetBoxObjectFor(this, nsnull);
    }

    // mControllers can own objects that are implemented
    // in JavaScript (such as some implementations of
    // nsIControllers.  These objects prevent their global
    // object's script object from being garbage collected,
    // which means JS continues to hold an owning reference
    // to the nsGlobalWindow, which owns the document,
    // which owns this content.  That's a cycle, so we break
    // it here.  (It might be better to break this by releasing
    // mDocument in nsGlobalWindow::SetDocShell, but I'm not
    // sure whether that would fix all possible cycles through
    // mControllers.)
    nsDOMSlots* slots = GetExistingDOMSlots();
    if (slots) {
        NS_IF_RELEASE(slots->mControllers);
    }

    // XXXbz why are we nuking our listener manager?  We can get events while
    // not in a document!
    if (mListenerManager) {
        mListenerManager->Disconnect();
        mListenerManager = nsnull;
    }

    // Unset things in the reverse order from how we set them in BindToTree
    mParentPtrBits &= ~PARENT_BIT_INDOCUMENT;
  
    if (aNullParent) {
        // Just mask it out
        mParentPtrBits &= nsIContent::kParentBitMask;
    }
  
    mBindingParent = nsnull;

    if (aDeep) {
        // Do the kids.  Note that we don't want to GetChildCount(), because
        // that will force content generation... if we never had to generate
        // the content, we shouldn't force it now!
        PRUint32 i, n = PeekChildCount();

        for (i = 0; i < n; ++i) {
            // Note that we pass PR_FALSE for aNullParent here, since we don't
            // want the kids to forget us.  We _do_ want them to forget their
            // binding parent, though, since this only walks non-anonymous
            // kids.
            mAttrsAndChildren.ChildAt(i)->UnbindFromTree(PR_TRUE, PR_FALSE);
        }
    }
}

PRBool
nsXULElement::IsNativeAnonymous() const
{
    // XXX Workaround for bug 280541, wallpaper for bug 326644
    return NodeInfo()->Equals(nsXULAtoms::popupgroup) &&
           nsGenericElement::IsNativeAnonymous();
}

PRUint32
nsXULElement::GetChildCount() const
{
    if (NS_FAILED(EnsureContentsGenerated())) {
        return 0;
    }

    return PeekChildCount();
}

nsIContent *
nsXULElement::GetChildAt(PRUint32 aIndex) const
{
    if (NS_FAILED(EnsureContentsGenerated())) {
        return nsnull;
    }

    return mAttrsAndChildren.GetSafeChildAt(aIndex);
}

PRInt32
nsXULElement::IndexOf(nsIContent* aPossibleChild) const
{
    if (NS_FAILED(EnsureContentsGenerated())) {
        return -1;
    }

    return mAttrsAndChildren.IndexOfChild(aPossibleChild);
}

nsresult
nsXULElement::RemoveChildAt(PRUint32 aIndex, PRBool aNotify)
{
    nsresult rv = EnsureContentsGenerated();
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIContent> oldKid = mAttrsAndChildren.ChildAt(aIndex);
    NS_ENSURE_TRUE(oldKid, NS_ERROR_FAILURE);

    nsIDocument* doc = GetCurrentDoc();
    mozAutoDocUpdate updateBatch(doc, UPDATE_CONTENT_MODEL, aNotify);

    if (HasMutationListeners(this, NS_EVENT_BITS_MUTATION_NODEREMOVED)) {
      nsMutationEvent mutation(PR_TRUE, NS_MUTATION_NODEREMOVED);
      mutation.mRelatedNode =
          do_QueryInterface(NS_STATIC_CAST(nsIContent*, this));

      nsEventStatus status = nsEventStatus_eIgnore;
      nsEventDispatcher::Dispatch(oldKid, nsnull, &mutation, nsnull, &status);
    }

    // On the removal of a <treeitem>, <treechildren>, or <treecell> element,
    // the possibility exists that some of the items in the removed subtree
    // are selected (and therefore need to be deselected). We need to account for this.
    nsCOMPtr<nsIDOMXULMultiSelectControlElement> controlElement;
    nsCOMPtr<nsIListBoxObject> listBox;
    PRBool fireSelectionHandler = PR_FALSE;

    // -1 = do nothing, -2 = null out current item
    // anything else = index to re-set as current
    PRInt32 newCurrentIndex = -1;

    if (oldKid->NodeInfo()->Equals(nsXULAtoms::listitem, kNameSpaceID_XUL)) {
      // This is the nasty case. We have (potentially) a slew of selected items
      // and cells going away.
      // First, retrieve the tree.
      // Check first whether this element IS the tree
      controlElement = do_QueryInterface(NS_STATIC_CAST(nsIContent*, this));

      // If it's not, look at our parent
      if (!controlElement)
        rv = GetParentTree(getter_AddRefs(controlElement));

      nsCOMPtr<nsIDOMElement> oldKidElem = do_QueryInterface(oldKid);
      if (controlElement && oldKidElem) {
        // Iterate over all of the items and find out if they are contained inside
        // the removed subtree.
        PRInt32 length;
        controlElement->GetSelectedCount(&length);
        for (PRInt32 i = 0; i < length; i++) {
          nsCOMPtr<nsIDOMXULSelectControlItemElement> node;
          controlElement->GetSelectedItem(i, getter_AddRefs(node));
          // we need to QI here to do an XPCOM-correct pointercompare
          nsCOMPtr<nsIDOMElement> selElem = do_QueryInterface(node);
          if (selElem == oldKidElem &&
              NS_SUCCEEDED(controlElement->RemoveItemFromSelection(node))) {
            length--;
            i--;
            fireSelectionHandler = PR_TRUE;
          }
        }

        nsCOMPtr<nsIDOMXULSelectControlItemElement> curItem;
        controlElement->GetCurrentItem(getter_AddRefs(curItem));
        nsCOMPtr<nsIContent> curNode = do_QueryInterface(curItem);
        if (curNode && isSelfOrAncestor(curNode, oldKid)) {
            // Current item going away
            nsCOMPtr<nsIBoxObject> box;
            controlElement->GetBoxObject(getter_AddRefs(box));
            listBox = do_QueryInterface(box);
            if (listBox && oldKidElem) {
              listBox->GetIndexOfItem(oldKidElem, &newCurrentIndex);
            }

            // If any of this fails, we'll just set the current item to null
            if (newCurrentIndex == -1)
              newCurrentIndex = -2;
        }
      }
    }

    nsRange::OwnerChildRemoved(this, aIndex, oldKid);
    mAttrsAndChildren.RemoveChildAt(aIndex);
    if (aNotify && doc) {
        doc->ContentRemoved(this, oldKid, aIndex);
    }

    if (newCurrentIndex == -2)
        controlElement->SetCurrentItem(nsnull);
    else if (newCurrentIndex > -1) {
        // Make sure the index is still valid
        PRInt32 treeRows;
        listBox->GetRowCount(&treeRows);
        if (treeRows > 0) {
            newCurrentIndex = PR_MIN((treeRows - 1), newCurrentIndex);
            nsCOMPtr<nsIDOMElement> newCurrentItem;
            listBox->GetItemAtIndex(newCurrentIndex, getter_AddRefs(newCurrentItem));
            nsCOMPtr<nsIDOMXULSelectControlItemElement> xulCurItem = do_QueryInterface(newCurrentItem);
            if (xulCurItem)
                controlElement->SetCurrentItem(xulCurItem);
        } else {
            controlElement->SetCurrentItem(nsnull);
        }
    }

    if (fireSelectionHandler && doc) {
      nsContentUtils::DispatchTrustedEvent(doc,
                                           NS_STATIC_CAST(nsIContent*, this),
                                           NS_LITERAL_STRING("select"),
                                           PR_FALSE,
                                           PR_TRUE);
    }

    // This will cause the script object to be unrooted for each
    // element in the subtree.
    oldKid->UnbindFromTree();

    return NS_OK;
}

void
nsXULElement::UnregisterAccessKey(const nsAString& aOldValue)
{
    // If someone changes the accesskey, unregister the old one
    //
    nsIDocument* doc = GetCurrentDoc();
    if (doc && !aOldValue.IsEmpty()) {
        nsIPresShell *shell = doc->GetShellAt(0);

        if (shell) {
            nsIContent *content = this;

            // find out what type of content node this is
            if (mNodeInfo->Equals(nsXULAtoms::label)) {
                // For anonymous labels the unregistering must
                // occur on the binding parent control.
                content = GetBindingParent();
            }

            if (content) {
                shell->GetPresContext()->EventStateManager()->
                    UnregisterAccessKey(content, aOldValue.First());
            }
        }
    }
}

nsresult
nsXULElement::BeforeSetAttr(PRInt32 aNamespaceID, nsIAtom* aName,
                            const nsAString* aValue, PRBool aNotify)
{
    if (aNamespaceID == kNameSpaceID_None && aName == nsXULAtoms::accesskey &&
        IsInDoc()) {
        const nsAttrValue* attrVal = FindLocalOrProtoAttr(aNamespaceID, aName);
        if (attrVal) {
            nsAutoString oldValue;
            attrVal->ToString(oldValue);
            UnregisterAccessKey(oldValue);
        }
    }

    return nsGenericElement::BeforeSetAttr(aNamespaceID, aName,
                                           aValue, aNotify);
}

nsresult
nsXULElement::AfterSetAttr(PRInt32 aNamespaceID, nsIAtom* aName,
                           const nsAString* aValue, PRBool aNotify)
{
    if (aNamespaceID == kNameSpaceID_None) {
        // XXX UnsetAttr handles more attributes then we do. See bug 233642.

        // Add popup and event listeners. We can't call AddListenerFor since
        // the attribute isn't set yet.
        MaybeAddPopupListener(aName);
        if (IsEventHandler(aName) && aValue) {
            AddScriptEventListener(aName, *aValue);
        }

        // Hide chrome if needed
        if (aName == nsXULAtoms::hidechrome &&
            mNodeInfo->Equals(nsXULAtoms::window)) {
            HideWindowChrome(aValue && NS_LITERAL_STRING("true").Equals(*aValue));
        }

        // XXX need to check if they're changing an event handler: if
        // so, then we need to unhook the old one.  Or something.
    }

    return nsGenericElement::AfterSetAttr(aNamespaceID, aName,
                                          aValue, aNotify);
}

PRBool
nsXULElement::ParseAttribute(PRInt32 aNamespaceID,
                             nsIAtom* aAttribute,
                             const nsAString& aValue,
                             nsAttrValue& aResult)
{
    // Parse into a nsAttrValue

    // WARNING!!
    // This code is largely duplicated in nsXULPrototypeElement::SetAttrAt.
    // Any changes should be made to both functions.
    if (aNamespaceID == kNameSpaceID_None) {
        if (aAttribute == nsXULAtoms::style) {
            nsGenericHTMLElement::ParseStyleAttribute(this, PR_TRUE, aValue,
                                                      aResult);
            return PR_TRUE;
        }

        if (aAttribute == nsXULAtoms::_class) {
            aResult.ParseAtomArray(aValue);
            return PR_TRUE;
        }
    }

    if (!nsGenericElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                          aResult)) {
        // Fall back to parsing as atom for short values
        aResult.ParseStringOrAtom(aValue);
    }

    return PR_TRUE;
}

const nsAttrName*
nsXULElement::InternalGetExistingAttrNameFromQName(const nsAString& aStr) const
{
    NS_ConvertUTF16toUTF8 name(aStr);
    const nsAttrName* attrName =
        mAttrsAndChildren.GetExistingAttrNameFromQName(name);
    if (attrName) {
        return attrName;
    }

    if (mPrototype) {
        PRUint32 i;
        for (i = 0; i < mPrototype->mNumAttributes; ++i) {
            attrName = &mPrototype->mAttributes[i].mName;
            if (attrName->QualifiedNameEquals(name)) {
                return attrName;
            }
        }
    }

    return nsnull;
}

PRBool
nsXULElement::GetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                      nsAString& aResult) const
{
    NS_ASSERTION(nsnull != aName, "must have attribute name");
    NS_ASSERTION(aNameSpaceID != kNameSpaceID_Unknown,
                 "must have a real namespace ID!");

    const nsAttrValue* val = FindLocalOrProtoAttr(aNameSpaceID, aName);

    if (!val) {
        // Since we are returning a success code we'd better do
        // something about the out parameters (someone may have
        // given us a non-empty string).
        aResult.Truncate();
        return PR_FALSE;
    }

    val->ToString(aResult);

    return PR_TRUE;
}

PRBool
nsXULElement::HasAttr(PRInt32 aNameSpaceID, nsIAtom* aName) const
{
    NS_ASSERTION(nsnull != aName, "must have attribute name");
    NS_ASSERTION(aNameSpaceID != kNameSpaceID_Unknown,
                 "must have a real namespace ID!");

    return mAttrsAndChildren.GetAttr(aName, aNameSpaceID) ||
           FindPrototypeAttribute(aNameSpaceID, aName);
}

PRBool
nsXULElement::AttrValueIs(PRInt32 aNameSpaceID,
                          nsIAtom* aName,
                          const nsAString& aValue,
                          nsCaseTreatment aCaseSensitive) const
{
  NS_ASSERTION(aName, "Must have attr name");
  NS_ASSERTION(aNameSpaceID != kNameSpaceID_Unknown, "Must have namespace");

  const nsAttrValue* val = FindLocalOrProtoAttr(aNameSpaceID, aName);
  return val && val->Equals(aValue, aCaseSensitive);
}

PRBool
nsXULElement::AttrValueIs(PRInt32 aNameSpaceID,
                          nsIAtom* aName,
                          nsIAtom* aValue,
                          nsCaseTreatment aCaseSensitive) const
{
  NS_ASSERTION(aName, "Must have attr name");
  NS_ASSERTION(aNameSpaceID != kNameSpaceID_Unknown, "Must have namespace");
  NS_ASSERTION(aValue, "Null value atom");

  const nsAttrValue* val = FindLocalOrProtoAttr(aNameSpaceID, aName);
  return val && val->Equals(aValue, aCaseSensitive);
}

PRInt32
nsXULElement::FindAttrValueIn(PRInt32 aNameSpaceID,
                              nsIAtom* aName,
                              AttrValuesArray* aValues,
                              nsCaseTreatment aCaseSensitive) const
{
  NS_ASSERTION(aName, "Must have attr name");
  NS_ASSERTION(aNameSpaceID != kNameSpaceID_Unknown, "Must have namespace");
  NS_ASSERTION(aValues, "Null value array");
  
  const nsAttrValue* val = FindLocalOrProtoAttr(aNameSpaceID, aName);
  if (val) {
    for (PRInt32 i = 0; aValues[i]; ++i) {
      if (val->Equals(*aValues[i], aCaseSensitive)) {
        return i;
      }
    }
    return ATTR_VALUE_NO_MATCH;
  }
  return ATTR_MISSING;
}

nsresult
nsXULElement::UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aName, PRBool aNotify)
{
    NS_ASSERTION(nsnull != aName, "must have attribute name");
    nsresult rv;

    // Because It's Hard to maintain a magic ``unset'' value in
    // the local attributes, we'll fault all the attributes,
    // unhook ourselves from the prototype, and then remove the
    // local copy of the attribute that we want to unset. In
    // other words, we'll become ``heavyweight''.
    //
    // We can avoid this if the attribute isn't in the prototype,
    // then we just need to remove it locally

    nsXULPrototypeAttribute *protoattr =
        FindPrototypeAttribute(aNameSpaceID, aName);
    if (protoattr) {
        // We've got an attribute on the prototype, so we need to
        // fully fault and remove the local copy.
        rv = MakeHeavyweight();
        NS_ENSURE_SUCCESS(rv, rv);
    }

    PRInt32 index = mAttrsAndChildren.IndexOfAttr(aName, aNameSpaceID);
    if (index < 0) {
        NS_ASSERTION(!protoattr, "we used to have a protoattr, we should now "
                                 "have a normal one");

        return NS_OK;
    }

    nsAutoString oldValue;
    GetAttr(aNameSpaceID, aName, oldValue);

    nsIDocument* doc = GetCurrentDoc();
    mozAutoDocUpdate updateBatch(doc, UPDATE_CONTENT_MODEL, aNotify);
    if (aNotify && doc) {
        doc->AttributeWillChange(this, aNameSpaceID, aName);
    }

    PRBool hasMutationListeners =
        HasMutationListeners(this, NS_EVENT_BITS_MUTATION_ATTRMODIFIED);
    nsCOMPtr<nsIDOMAttr> attrNode;
    if (hasMutationListeners) {
        nsAutoString attrName;
        aName->ToString(attrName);
        GetAttributeNode(attrName, getter_AddRefs(attrNode));
    }

    nsDOMSlots *slots = GetExistingDOMSlots();
    if (slots && slots->mAttributeMap) {
      slots->mAttributeMap->DropAttribute(aNameSpaceID, aName);
    }

    rv = mAttrsAndChildren.RemoveAttrAt(index);
    NS_ENSURE_SUCCESS(rv, rv);

    // XXX if the RemoveAttrAt() call fails, we might end up having removed
    // the attribute from the attribute map even though the attribute is still
    // on the element
    // https://bugzilla.mozilla.org/show_bug.cgi?id=296205

    // Deal with modification of magical attributes that side-effect
    // other things.
    // XXX Know how to remove POPUP event listeners when an attribute is unset?

    if (aNameSpaceID == kNameSpaceID_None) {
        if (aName == nsXULAtoms::hidechrome &&
            mNodeInfo->Equals(nsXULAtoms::window)) {
            HideWindowChrome(PR_FALSE);
        }

        // If the accesskey attribute is removed, unregister it here
        // Also see nsAreaFrame, nsBoxFrame and nsTextBoxFrame's AttributeChanged
        if (aName == nsXULAtoms::accesskey || aName == nsXULAtoms::control) {
            UnregisterAccessKey(oldValue);
        }

        // Check to see if the OBSERVES attribute is being unset.  If so, we
        // need to remove our broadcaster goop completely.
        if (doc && (aName == nsXULAtoms::observes ||
                          aName == nsXULAtoms::command)) {
            nsCOMPtr<nsIDOMXULDocument> xuldoc = do_QueryInterface(doc);
            if (xuldoc) {
                // Do a getElementById to retrieve the broadcaster
                nsCOMPtr<nsIDOMElement> broadcaster;
                nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(doc);
                domDoc->GetElementById(oldValue, getter_AddRefs(broadcaster));
                if (broadcaster) {
                    xuldoc->RemoveBroadcastListenerFor(broadcaster, this,
                                                       NS_LITERAL_STRING("*"));
                }
            }
        }
    }

    if (doc) {
        if (hasMutationListeners) {
            nsMutationEvent mutation(PR_TRUE, NS_MUTATION_ATTRMODIFIED);

            mutation.mRelatedNode = attrNode;
            mutation.mAttrName = aName;

            if (!oldValue.IsEmpty())
              mutation.mPrevAttrValue = do_GetAtom(oldValue);
            mutation.mAttrChange = nsIDOMMutationEvent::REMOVAL;

            nsEventDispatcher::Dispatch(NS_STATIC_CAST(nsIContent*, this),
                                        nsnull, &mutation);
        }

        nsXBLBinding *binding = doc->BindingManager()->GetBinding(this);
        if (binding)
            binding->AttributeChanged(aName, aNameSpaceID, PR_TRUE, aNotify);

        if (aNotify) {
            doc->AttributeChanged(this, aNameSpaceID, aName,
                                  nsIDOMMutationEvent::REMOVAL);
        }
    }

    return NS_OK;
}

const nsAttrName*
nsXULElement::GetAttrNameAt(PRUint32 aIndex) const
{
    PRUint32 localCount = mAttrsAndChildren.AttrCount();
    PRUint32 protoCount = mPrototype ? mPrototype->mNumAttributes : 0;

    if (localCount > protoCount) {
        // More local than proto, put local first

        // Is the index low enough to just grab a local attr?
        if (aIndex < localCount) {
            return mAttrsAndChildren.AttrNameAt(aIndex);
        }

        aIndex -= localCount;

        // Search though prototype attributes while skipping names that exist in
        // the local array.
        for (PRUint32 i = 0; i < protoCount; i++) {
            const nsAttrName* name = &mPrototype->mAttributes[i].mName;
            if (mAttrsAndChildren.GetAttr(name->LocalName(), name->NamespaceID())) {
                aIndex++;
            }
            if (i == aIndex) {
                return name;
            }
        }
    }
    else {
        // More proto than local, put proto first

        // Is the index low enough to just grab a proto attr?
        if (aIndex < protoCount) {
            return &mPrototype->mAttributes[aIndex].mName;
        }

        aIndex -= protoCount;

        // Search though local attributes while skipping names that exist in
        // the prototype array.
        for (PRUint32 i = 0; i < localCount; i++) {
            const nsAttrName* name = mAttrsAndChildren.AttrNameAt(i);

            for (PRUint32 j = 0; j < protoCount; j++) {
                if (mPrototype->mAttributes[j].mName.Equals(*name)) {
                    aIndex++;
                    break;
                }
            }
            if (i == aIndex) {
                return name;
            }
        }
    }

    return nsnull;
}

PRUint32
nsXULElement::GetAttrCount() const
{
    PRUint32 localCount = mAttrsAndChildren.AttrCount();
    PRUint32 protoCount = mPrototype ? mPrototype->mNumAttributes : 0;

    if (localCount > protoCount) {
        // More local than proto, remove dups from proto array
        PRUint32 count = localCount;

        for (PRUint32 i = 0; i < protoCount; i++) {
            const nsAttrName* name = &mPrototype->mAttributes[i].mName;
            if (!mAttrsAndChildren.GetAttr(name->LocalName(), name->NamespaceID())) {
                count++;
            }
        }

        return count;
    }

    // More proto than local, remove dups from local array
    PRUint32 count = protoCount;

    for (PRUint32 i = 0; i < localCount; i++) {
        const nsAttrName* name = mAttrsAndChildren.AttrNameAt(i);

        count++;
        for (PRUint32 j = 0; j < protoCount; j++) {
            if (mPrototype->mAttributes[j].mName.Equals(*name)) {
                count--;
                break;
            }
        }
    }

    return count;
}


#ifdef DEBUG
static void
rdf_Indent(FILE* out, PRInt32 aIndent)
{
    for (PRInt32 i = aIndent; --i >= 0; ) fputs("  ", out);
}

void
nsXULElement::List(FILE* out, PRInt32 aIndent) const
{
    NS_PRECONDITION(IsInDoc(), "bad content");

    PRUint32 i;

    rdf_Indent(out, aIndent);
    fputs("<XUL", out);
    if (HasDOMSlots()) fputs("*", out);
    fputs(" ", out);

    nsAutoString as;
    mNodeInfo->GetQualifiedName(as);
    fputs(NS_LossyConvertUTF16toASCII(as).get(), out);

    fprintf(out, "@%p", (void *)this);

    PRUint32 nattrs = GetAttrCount();

    for (i = 0; i < nattrs; ++i) {
        const nsAttrName* name = GetAttrNameAt(i);

        nsAutoString v;
        GetAttr(name->NamespaceID(), name->LocalName(), v);

        fputs(" ", out);

        nsAutoString s;
        name->GetQualifiedName(s);

        fputs(NS_LossyConvertUTF16toASCII(s).get(), out);
        fputs("=", out);
        fputs(NS_LossyConvertUTF16toASCII(v).get(), out);
    }

    PRUint32 nchildren = GetChildCount();

    if (nchildren) {
        fputs("\n", out);

        for (i = 0; i < nchildren; ++i) {
            GetChildAt(i)->List(out, aIndent + 1);
        }

        rdf_Indent(out, aIndent);
    }
    fputs(">\n", out);

    // XXX sXBL/XBL2 issue! Owner or current document?
    nsIDocument* doc = GetCurrentDoc();
    if (doc) {
        nsIBindingManager *bindingManager = doc->BindingManager();
        nsCOMPtr<nsIDOMNodeList> anonymousChildren;
        bindingManager->GetAnonymousNodesFor(NS_STATIC_CAST(nsIContent*, NS_CONST_CAST(nsXULElement*, this)),
                                             getter_AddRefs(anonymousChildren));

        if (anonymousChildren) {
            PRUint32 length;
            anonymousChildren->GetLength(&length);
            if (length) {
                rdf_Indent(out, aIndent);
                fputs("anonymous-children<\n", out);

                for (PRUint32 i2 = 0; i2 < length; ++i2) {
                    nsCOMPtr<nsIDOMNode> node;
                    anonymousChildren->Item(i2, getter_AddRefs(node));
                    nsCOMPtr<nsIContent> child = do_QueryInterface(node);
                    child->List(out, aIndent + 1);
                }

                rdf_Indent(out, aIndent);
                fputs(">\n", out);
            }
        }

        PRBool hasContentList;
        bindingManager->HasContentListFor(NS_STATIC_CAST(nsIContent*, NS_CONST_CAST(nsXULElement*, this)),
                                          &hasContentList);

        if (hasContentList) {
            nsCOMPtr<nsIDOMNodeList> contentList;
            bindingManager->GetContentListFor(NS_STATIC_CAST(nsIContent*, NS_CONST_CAST(nsXULElement*, this)),
                                              getter_AddRefs(contentList));

            NS_ASSERTION(contentList != nsnull, "oops, binding manager lied");

            PRUint32 length;
            contentList->GetLength(&length);
            if (length) {
                rdf_Indent(out, aIndent);
                fputs("content-list<\n", out);

                for (PRUint32 i2 = 0; i2 < length; ++i2) {
                    nsCOMPtr<nsIDOMNode> node;
                    contentList->Item(i2, getter_AddRefs(node));
                    nsCOMPtr<nsIContent> child = do_QueryInterface(node);
                    child->List(out, aIndent + 1);
                }

                rdf_Indent(out, aIndent);
                fputs(">\n", out);
            }
        }
    }
}
#endif

nsresult
nsXULElement::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
    aVisitor.mForceContentDispatch = PR_TRUE; //FIXME! Bug 329119
    nsIAtom* tag = Tag();
    if (aVisitor.mEvent->message == NS_XUL_COMMAND &&
        aVisitor.mEvent->originalTarget == NS_STATIC_CAST(nsIContent*, this) &&
        tag != nsXULAtoms::command) {
        // See if we have a command elt.  If so, we execute on the command
        // instead of on our content element.
        nsAutoString command;
        GetAttr(kNameSpaceID_None, nsXULAtoms::command, command);
        if (!command.IsEmpty()) {
            aVisitor.mCanHandle = PR_FALSE;
            // XXX sXBL/XBL2 issue! Owner or current document?
            nsCOMPtr<nsIDOMDocument> domDoc(do_QueryInterface(GetCurrentDoc()));
            nsCOMPtr<nsIDOMElement> commandElt;
            domDoc->GetElementById(command, getter_AddRefs(commandElt));
            nsCOMPtr<nsIContent> commandContent(do_QueryInterface(commandElt));
            if (commandContent &&
                commandContent->IsContentOfType(nsIContent::eXUL) &&
                commandContent->Tag() == nsXULAtoms::command) {
                // Reusing the event here, but DISPATCH_DONE/STARTED hack
                // is needed.
                NS_MARK_EVENT_DISPATCH_DONE(aVisitor.mEvent);
                aVisitor.mEvent->flags &=
                  ~NS_EVENT_FLAG_STOP_DISPATCH_IMMEDIATELY;
                // Dispatch will set the right target.
                aVisitor.mEvent->target = nsnull;
                nsEventDispatcher::Dispatch(commandContent,
                                            aVisitor.mPresContext,
                                            aVisitor.mEvent,
                                            aVisitor.mDOMEvent,
                                            &aVisitor.mEventStatus);
                NS_MARK_EVENT_DISPATCH_STARTED(aVisitor.mEvent);
            } else {
                NS_WARNING("A XUL element is attached to a command that doesn't exist!\n");
            }
            return NS_OK;
        }
    }

    return nsGenericElement::PreHandleEvent(aVisitor);
}

nsresult
nsXULElement::RangeAdd(nsIDOMRange* aRange)
{
    // rdf content does not yet support DOM ranges
    return NS_OK;
}


void
nsXULElement::RangeRemove(nsIDOMRange* aRange)
{
    // rdf content does not yet support DOM ranges
}


const nsVoidArray *
nsXULElement::GetRangeList() const
{
    // XUL content does not yet support DOM ranges
    return nsnull;
}

// XXX This _should_ be an implementation method, _not_ publicly exposed :-(
NS_IMETHODIMP
nsXULElement::GetResource(nsIRDFResource** aResource)
{
    nsAutoString id;
    GetAttr(kNameSpaceID_None, nsXULAtoms::ref, id);
    if (id.IsEmpty()) {
        GetAttr(kNameSpaceID_None, nsXULAtoms::id, id);
    }

    if (!id.IsEmpty()) {
        return nsXULContentUtils::RDFService()->
            GetUnicodeResource(id, aResource);
    }
    *aResource = nsnull;

    return NS_OK;
}


NS_IMETHODIMP
nsXULElement::GetDatabase(nsIRDFCompositeDataSource** aDatabase)
{
    nsCOMPtr<nsIXULTemplateBuilder> builder;
    GetBuilder(getter_AddRefs(builder));

    if (builder)
        builder->GetDatabase(aDatabase);
    else
        *aDatabase = nsnull;

    return NS_OK;
}


NS_IMETHODIMP
nsXULElement::GetBuilder(nsIXULTemplateBuilder** aBuilder)
{
    *aBuilder = nsnull;

    // XXX sXBL/XBL2 issue! Owner or current document?
    nsCOMPtr<nsIXULDocument> xuldoc = do_QueryInterface(GetCurrentDoc());
    if (xuldoc)
        xuldoc->GetTemplateBuilderFor(this, aBuilder);

    return NS_OK;
}


//----------------------------------------------------------------------
// Implementation methods

nsresult
nsXULElement::EnsureContentsGenerated(void) const
{
    if (GetFlags() & XUL_ELEMENT_CHILDREN_MUST_BE_REBUILT) {
        // Ensure that the element is actually _in_ the document tree;
        // otherwise, somebody is trying to generate children for a node
        // that's not currently in the content model.
        NS_PRECONDITION(IsInDoc(), "element not in tree");
        if (!IsInDoc())
            return NS_ERROR_NOT_INITIALIZED;

        // XXX hack because we can't use "mutable"
        nsXULElement* unconstThis = NS_CONST_CAST(nsXULElement*, this);

        // Clear this value *first*, so we can re-enter the nsIContent
        // getters if needed.
        unconstThis->ClearLazyState(eChildrenMustBeRebuilt);

        // Walk up our ancestor chain, looking for an element with a
        // XUL content model builder attached to it.
        nsIContent* element = unconstThis;

        do {
            nsCOMPtr<nsIDOMXULElement> xulele = do_QueryInterface(element);
            if (xulele) {
                nsCOMPtr<nsIXULTemplateBuilder> builder;
                xulele->GetBuilder(getter_AddRefs(builder));
                if (builder) {
                    if (HasAttr(kNameSpaceID_None, nsXULAtoms::xulcontentsgenerated)) {
                        unconstThis->ClearLazyState(eChildrenMustBeRebuilt);
                        return NS_OK;
                    }

                    return builder->CreateContents(unconstThis);
                }
            }

            element = element->GetParent();
        } while (element);

        NS_ERROR("lazy state set with no XUL content builder in ancestor chain");
        return NS_ERROR_UNEXPECTED;
    }

    return NS_OK;
}

nsresult 
nsXULElement::WillAddOrRemoveChild(nsIContent* aKid,
                                   PRUint32 aIndex,
                                   PRBool aRemove)
{
  // nsXULElement has its own RemoveChildAt, so no need to do
  // anything here when removing a child.
  return aRemove ? NS_OK : EnsureContentsGenerated();
}

/// XXX GetID must be defined here because we have proto attrs.
nsIAtom*
nsXULElement::GetID() const
{
    const nsAttrValue* attrVal = FindLocalOrProtoAttr(kNameSpaceID_None, nsXULAtoms::id);

    NS_ASSERTION(!attrVal ||
                 attrVal->Type() == nsAttrValue::eAtom ||
                 (attrVal->Type() == nsAttrValue::eString &&
                  attrVal->GetStringValue().IsEmpty()),
                 "unexpected attribute type");

    if (attrVal && attrVal->Type() == nsAttrValue::eAtom) {
        return attrVal->GetAtomValue();
    }
    return nsnull;
}

const nsAttrValue*
nsXULElement::GetClasses() const
{
    return FindLocalOrProtoAttr(kNameSpaceID_None, nsXULAtoms::_class);
}

NS_IMETHODIMP
nsXULElement::WalkContentStyleRules(nsRuleWalker* aRuleWalker)
{
    return NS_OK;
}

nsICSSStyleRule*
nsXULElement::GetInlineStyleRule()
{
    // Fetch the cached style rule from the attributes.
    const nsAttrValue* attrVal = FindLocalOrProtoAttr(kNameSpaceID_None, nsXULAtoms::style);

    if (attrVal && attrVal->Type() == nsAttrValue::eCSSStyleRule) {
        return attrVal->GetCSSStyleRuleValue();
    }

    return nsnull;
}

NS_IMETHODIMP
nsXULElement::SetInlineStyleRule(nsICSSStyleRule* aStyleRule, PRBool aNotify)
{
    PRBool hasListeners = PR_FALSE;
    PRBool modification = PR_FALSE;
    nsAutoString oldValueStr;

    if (IsInDoc()) {
        hasListeners = nsGenericElement::HasMutationListeners(this,
            NS_EVENT_BITS_MUTATION_ATTRMODIFIED);

        // We can't compare the stringvalues of the old and the new rules
        // since both will point to the same declaration and thus will be
        // the same.
        if (hasListeners || aNotify) {
            modification = !!mAttrsAndChildren.GetAttr(nsXULAtoms::style);
        }
    }

    nsAttrValue attrValue(aStyleRule);

    return SetAttrAndNotify(kNameSpaceID_None, nsXULAtoms::style, nsnull,
                            oldValueStr, attrValue, modification, hasListeners,
                            aNotify);
}

nsChangeHint
nsXULElement::GetAttributeChangeHint(const nsIAtom* aAttribute,
                                     PRInt32 aModType) const
{
    nsChangeHint retval(NS_STYLE_HINT_NONE);

    if (aAttribute == nsXULAtoms::value &&
        (aModType == nsIDOMMutationEvent::REMOVAL ||
         aModType == nsIDOMMutationEvent::ADDITION)) {
      nsIAtom *tag = Tag();
      if (tag == nsXULAtoms::label || tag == nsXULAtoms::description)
        // Label and description dynamically morph between a normal
        // block and a cropping single-line XUL text frame.  If the
        // value attribute is being added or removed, then we need to
        // return a hint of frame change.  (See bugzilla bug 95475 for
        // details.)
        retval = NS_STYLE_HINT_FRAMECHANGE;
    } else {
        // if left or top changes we reflow. This will happen in xul
        // containers that manage positioned children such as a
        // bulletinboard.
        if (nsXULAtoms::left == aAttribute || nsXULAtoms::top == aAttribute)
            retval = NS_STYLE_HINT_REFLOW;
    }

    return retval;
}

NS_IMETHODIMP_(PRBool)
nsXULElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
    return PR_FALSE;
}

nsIAtom *
nsXULElement::GetIDAttributeName() const
{
    return nsXULAtoms::id;
}

nsIAtom *
nsXULElement::GetClassAttributeName() const
{
    return nsXULAtoms::_class;
}

// Controllers Methods
NS_IMETHODIMP
nsXULElement::GetControllers(nsIControllers** aResult)
{
    if (! Controllers()) {
        nsDOMSlots* slots = GetDOMSlots();
        if (!slots)
          return NS_ERROR_OUT_OF_MEMORY;

        nsresult rv;
        rv = NS_NewXULControllers(nsnull, NS_GET_IID(nsIControllers),
                                  NS_REINTERPRET_CAST(void**, &slots->mControllers));

        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create a controllers");
        if (NS_FAILED(rv)) return rv;
    }

    *aResult = Controllers();
    NS_IF_ADDREF(*aResult);
    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::GetBoxObject(nsIBoxObject** aResult)
{
  *aResult = nsnull;

  // XXX sXBL/XBL2 issue! Owner or current document?
  nsCOMPtr<nsIDOMNSDocument> nsDoc(do_QueryInterface(GetCurrentDoc()));
  NS_ENSURE_TRUE(nsDoc, NS_ERROR_FAILURE);
  return nsDoc->GetBoxObjectFor(this, aResult);
}

// Methods for setting/getting attributes from nsIDOMXULElement
#define NS_IMPL_XUL_STRING_ATTR(_method, _atom)                     \
  NS_IMETHODIMP                                                     \
  nsXULElement::Get##_method(nsAString& aReturn)                    \
  {                                                                 \
    GetAttr(kNameSpaceID_None, nsXULAtoms::_atom, aReturn);         \
    return NS_OK;                                                   \
  }                                                                 \
  NS_IMETHODIMP                                                     \
  nsXULElement::Set##_method(const nsAString& aValue)               \
  {                                                                 \
    return SetAttr(kNameSpaceID_None, nsXULAtoms::_atom, aValue,    \
                   PR_TRUE);                                        \
  }

#define NS_IMPL_XUL_BOOL_ATTR(_method, _atom)                       \
  NS_IMETHODIMP                                                     \
  nsXULElement::Get##_method(PRBool* aResult)                       \
  {                                                                 \
    *aResult = BoolAttrIsTrue(nsXULAtoms::_atom);                   \
                                                                    \
    return NS_OK;                                                   \
  }                                                                 \
  NS_IMETHODIMP                                                     \
  nsXULElement::Set##_method(PRBool aValue)                         \
  {                                                                 \
    if (aValue)                                                     \
      SetAttr(kNameSpaceID_None, nsXULAtoms::_atom,                 \
              NS_LITERAL_STRING("true"), PR_TRUE);                  \
    else                                                            \
      UnsetAttr(kNameSpaceID_None, nsXULAtoms::_atom, PR_TRUE);     \
                                                                    \
    return NS_OK;                                                   \
  }


NS_IMPL_XUL_STRING_ATTR(Id, id)
NS_IMPL_XUL_STRING_ATTR(ClassName, _class)
NS_IMPL_XUL_STRING_ATTR(Align, align)
NS_IMPL_XUL_STRING_ATTR(Dir, dir)
NS_IMPL_XUL_STRING_ATTR(Flex, flex)
NS_IMPL_XUL_STRING_ATTR(FlexGroup, flexgroup)
NS_IMPL_XUL_STRING_ATTR(Ordinal, ordinal)
NS_IMPL_XUL_STRING_ATTR(Orient, orient)
NS_IMPL_XUL_STRING_ATTR(Pack, pack)
NS_IMPL_XUL_BOOL_ATTR(Hidden, hidden)
NS_IMPL_XUL_BOOL_ATTR(Collapsed, collapsed)
NS_IMPL_XUL_BOOL_ATTR(AllowEvents, allowevents)
NS_IMPL_XUL_STRING_ATTR(Observes, observes)
NS_IMPL_XUL_STRING_ATTR(Menu, menu)
NS_IMPL_XUL_STRING_ATTR(ContextMenu, contextmenu)
NS_IMPL_XUL_STRING_ATTR(Tooltip, tooltip)
NS_IMPL_XUL_STRING_ATTR(Width, width)
NS_IMPL_XUL_STRING_ATTR(Height, height)
NS_IMPL_XUL_STRING_ATTR(MinWidth, minwidth)
NS_IMPL_XUL_STRING_ATTR(MinHeight, minheight)
NS_IMPL_XUL_STRING_ATTR(MaxWidth, maxwidth)
NS_IMPL_XUL_STRING_ATTR(MaxHeight, maxheight)
NS_IMPL_XUL_STRING_ATTR(Persist, persist)
NS_IMPL_XUL_STRING_ATTR(Left, left)
NS_IMPL_XUL_STRING_ATTR(Top, top)
NS_IMPL_XUL_STRING_ATTR(Datasources, datasources)
NS_IMPL_XUL_STRING_ATTR(Ref, ref)
NS_IMPL_XUL_STRING_ATTR(TooltipText, tooltiptext)
NS_IMPL_XUL_STRING_ATTR(StatusText, statustext)

nsresult
nsXULElement::GetStyle(nsIDOMCSSStyleDeclaration** aStyle)
{
    nsDOMSlots* slots = GetDOMSlots();
    NS_ENSURE_TRUE(slots, NS_ERROR_OUT_OF_MEMORY);

    if (!slots->mStyle) {
        nsresult rv;
        if (!gCSSOMFactory) {
            rv = CallGetService(kCSSOMFactoryCID, &gCSSOMFactory);
            NS_ENSURE_SUCCESS(rv, rv);
        }

        rv = gCSSOMFactory->CreateDOMCSSAttributeDeclaration(this,
                getter_AddRefs(slots->mStyle));
        NS_ENSURE_SUCCESS(rv, rv);
    }

    NS_IF_ADDREF(*aStyle = slots->mStyle);

    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::GetParentTree(nsIDOMXULMultiSelectControlElement** aTreeElement)
{
    for (nsIContent* current = GetParent(); current;
         current = current->GetParent()) {
        if (current->NodeInfo()->Equals(nsXULAtoms::listbox,
                                        kNameSpaceID_XUL)) {
            CallQueryInterface(current, aTreeElement);
            // XXX returning NS_OK because that's what the code used to do;
            // is that the right thing, though?

            return NS_OK;
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::Focus()
{
    if (!nsGenericElement::ShouldFocus(this)) {
        return NS_OK;
    }

    nsIDocument* doc = GetCurrentDoc();
    // What kind of crazy tries to focus an element without a doc?
    if (!doc)
        return NS_OK;

    // Obtain a presentation context and then call SetFocus.
    if (doc->GetNumberOfShells() == 0)
        return NS_OK;

    nsIPresShell *shell = doc->GetShellAt(0);

    // Set focus
    nsCOMPtr<nsPresContext> context = shell->GetPresContext();
    SetFocus(context);

    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::Blur()
{
    nsIDocument* doc = GetCurrentDoc();
    // What kind of crazy tries to blur an element without a doc?
    if (!doc)
        return NS_OK;

    // Obtain a presentation context and then call SetFocus.
    if (doc->GetNumberOfShells() == 0)
        return NS_OK;

    nsIPresShell *shell = doc->GetShellAt(0);

    // Set focus
    nsCOMPtr<nsPresContext> context = shell->GetPresContext();
    RemoveFocus(context);

    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::Click()
{
    if (BoolAttrIsTrue(nsXULAtoms::disabled))
        return NS_OK;

    nsCOMPtr<nsIDocument> doc = GetCurrentDoc(); // Strong just in case
    if (doc) {
        PRUint32 numShells = doc->GetNumberOfShells();
        // strong ref to PresContext so events don't destroy it
        nsCOMPtr<nsPresContext> context;

        for (PRUint32 i = 0; i < numShells; ++i) {
            nsIPresShell *shell = doc->GetShellAt(i);
            context = shell->GetPresContext();

            PRBool isCallerChrome = nsContentUtils::IsCallerChrome();

            nsMouseEvent eventDown(isCallerChrome, NS_MOUSE_LEFT_BUTTON_DOWN,
                                   nsnull, nsMouseEvent::eReal);
            nsMouseEvent eventUp(isCallerChrome, NS_MOUSE_LEFT_BUTTON_UP,
                                 nsnull, nsMouseEvent::eReal);
            nsMouseEvent eventClick(isCallerChrome, NS_MOUSE_LEFT_CLICK, nsnull,
                                    nsMouseEvent::eReal);

            // send mouse down
            nsEventStatus status = nsEventStatus_eIgnore;
            nsEventDispatcher::Dispatch(NS_STATIC_CAST(nsIContent*, this),
                                        context, &eventDown,  nsnull, &status);

            // send mouse up
            status = nsEventStatus_eIgnore;  // reset status
            nsEventDispatcher::Dispatch(NS_STATIC_CAST(nsIContent*, this),
                                        context, &eventUp, nsnull, &status);

            // send mouse click
            status = nsEventStatus_eIgnore;  // reset status
            nsEventDispatcher::Dispatch(NS_STATIC_CAST(nsIContent*, this),
                                        context, &eventClick, nsnull, &status);
        }
    }

    // oncommand is fired when an element is clicked...
    return DoCommand();
}

NS_IMETHODIMP
nsXULElement::DoCommand()
{
    nsCOMPtr<nsIDocument> doc = GetCurrentDoc(); // strong just in case
    if (doc) {
        PRUint32 numShells = doc->GetNumberOfShells();
        nsCOMPtr<nsPresContext> context;

        for (PRUint32 i = 0; i < numShells; ++i) {
            nsIPresShell *shell = doc->GetShellAt(i);
            context = shell->GetPresContext();

            nsEventStatus status = nsEventStatus_eIgnore;
            nsMouseEvent event(PR_TRUE, NS_XUL_COMMAND, nsnull,
                               nsMouseEvent::eReal);
            nsEventDispatcher::Dispatch(NS_STATIC_CAST(nsIContent*, this),
                                        context, &event, nsnull, &status);
        }
    }

    return NS_OK;
}

// nsIFocusableContent interface and helpers

void
nsXULElement::SetFocus(nsPresContext* aPresContext)
{
    if (BoolAttrIsTrue(nsXULAtoms::disabled))
        return;

    aPresContext->EventStateManager()->SetContentState(this,
                                                       NS_EVENT_STATE_FOCUS);
}

void
nsXULElement::RemoveFocus(nsPresContext* aPresContext)
{
}

nsIContent *
nsXULElement::GetBindingParent() const
{
    return mBindingParent;
}

PRBool
nsXULElement::IsContentOfType(PRUint32 aFlags) const
{
    return !(aFlags & ~(eELEMENT | eXUL));
}

nsresult
nsXULElement::AddPopupListener(nsIAtom* aName)
{
    // Add a popup listener to the element
    nsresult rv;

    nsCOMPtr<nsIXULPopupListener> popupListener =
        do_CreateInstance(kXULPopupListenerCID, &rv);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Unable to create an instance of the popup listener object.");
    if (NS_FAILED(rv)) return rv;

    XULPopupType popupType;
    if (aName == nsXULAtoms::context || aName == nsXULAtoms::contextmenu) {
        popupType = eXULPopupType_context;
    }
    else {
        popupType = eXULPopupType_popup;
    }

    // Add a weak reference to the node.
    popupListener->Init(this, popupType);

    // Add the popup as a listener on this element.
    nsCOMPtr<nsIDOMEventListener> eventListener = do_QueryInterface(popupListener);
    nsCOMPtr<nsIDOMEventTarget> target(do_QueryInterface(NS_STATIC_CAST(nsIContent *, this)));
    NS_ENSURE_TRUE(target, NS_ERROR_FAILURE);
    target->AddEventListener(NS_LITERAL_STRING("mousedown"), eventListener, PR_FALSE);
    target->AddEventListener(NS_LITERAL_STRING("contextmenu"), eventListener, PR_FALSE);

    return NS_OK;
}

//*****************************************************************************
// nsXULElement::nsIChromeEventHandler
//*****************************************************************************

NS_IMETHODIMP
nsXULElement::PreHandleChromeEvent(nsEventChainPreVisitor& aVisitor)
{
  return PreHandleEvent(aVisitor);
}

NS_IMETHODIMP
nsXULElement::PostHandleChromeEvent(nsEventChainPostVisitor& aVisitor)
{
  return NS_OK;
}

//----------------------------------------------------------------------

nsGenericElement::nsAttrInfo
nsXULElement::GetAttrInfo(PRInt32 aNamespaceID, nsIAtom *aName) const
{

    nsAttrInfo info(nsGenericElement::GetAttrInfo(aNamespaceID, aName));
    if (!info.mValue) {
        nsXULPrototypeAttribute *protoattr =
            FindPrototypeAttribute(aNamespaceID, aName);
        if (protoattr) {
            return nsAttrInfo(&protoattr->mName, &protoattr->mValue);
        }
    }

    return info;
}


nsXULPrototypeAttribute *
nsXULElement::FindPrototypeAttribute(PRInt32 aNamespaceID,
                                     nsIAtom* aLocalName) const
{
    if (!mPrototype) {
        return nsnull;
    }

    PRUint32 i, count = mPrototype->mNumAttributes;
    if (aNamespaceID == kNameSpaceID_None) {
        // Common case so optimize for this
        for (i = 0; i < count; ++i) {
            nsXULPrototypeAttribute *protoattr = &mPrototype->mAttributes[i];
            if (protoattr->mName.Equals(aLocalName)) {
                return protoattr;
            }
        }
    }
    else {
        for (i = 0; i < count; ++i) {
            nsXULPrototypeAttribute *protoattr = &mPrototype->mAttributes[i];
            if (protoattr->mName.Equals(aLocalName, aNamespaceID)) {
                return protoattr;
            }
        }
    }

    return nsnull;
}

nsresult nsXULElement::MakeHeavyweight()
{
    if (!mPrototype)
        return NS_OK;           // already heavyweight

    nsRefPtr<nsXULPrototypeElement> proto;
    proto.swap(mPrototype);

    PRBool hadAttributes = mAttrsAndChildren.AttrCount() > 0;

    PRUint32 i;
    nsresult rv;
    for (i = 0; i < proto->mNumAttributes; ++i) {
        nsXULPrototypeAttribute* protoattr = &proto->mAttributes[i];

        // We might have a local value for this attribute, in which case
        // we don't want to copy the prototype's value.
        if (hadAttributes &&
            mAttrsAndChildren.GetAttr(protoattr->mName.LocalName(),
                                      protoattr->mName.NamespaceID())) {
            continue;
        }

        // XXX we might wanna have a SetAndTakeAttr that takes an nsAttrName
        nsAttrValue attrValue(protoattr->mValue);
        if (protoattr->mName.IsAtom()) {
            rv = mAttrsAndChildren.SetAndTakeAttr(protoattr->mName.Atom(), attrValue);
        }
        else {
            rv = mAttrsAndChildren.SetAndTakeAttr(protoattr->mName.NodeInfo(),
                                                  attrValue);
        }
        NS_ENSURE_SUCCESS(rv, rv);
    }
    return NS_OK;
}

nsresult
nsXULElement::HideWindowChrome(PRBool aShouldHide)
{
    nsIDocument* doc = GetCurrentDoc();
    if (!doc)
      return NS_ERROR_UNEXPECTED;

    nsIPresShell *shell = doc->GetShellAt(0);

    if (shell) {
        nsIContent* content = NS_STATIC_CAST(nsIContent*, this);
        nsIFrame* frame = shell->GetPrimaryFrameFor(content);

        nsPresContext *presContext = shell->GetPresContext();

        if (frame && presContext) {
            nsIView* view = frame->GetClosestView();

            if (view) {
                // XXXldb Um, not all views have widgets...
                view->GetWidget()->HideWindowChrome(aShouldHide);
            }
        }
    }

    return NS_OK;
}

PRBool
nsXULElement::BoolAttrIsTrue(nsIAtom* aName)
{
    const nsAttrValue* attr =
        FindLocalOrProtoAttr(kNameSpaceID_None, aName);

    return attr && attr->Type() == nsAttrValue::eAtom &&
           attr->GetAtomValue() == nsXULAtoms::_true;
}

//----------------------------------------------------------------------
//
// nsXULPrototypeAttribute
//

nsXULPrototypeAttribute::~nsXULPrototypeAttribute()
{
    MOZ_COUNT_DTOR(nsXULPrototypeAttribute);
    if (mEventHandler)
        nsContentUtils::RemoveJSGCRoot(&mEventHandler);
}


//----------------------------------------------------------------------
//
// nsXULPrototypeElement
//

nsresult
nsXULPrototypeElement::Serialize(nsIObjectOutputStream* aStream,
                                 nsIScriptContext* aContext,
                                 const nsCOMArray<nsINodeInfo> *aNodeInfos)
{
    nsresult rv;

    // Write basic prototype data
    rv = aStream->Write32(mType);

    // Write Node Info
    PRInt32 index = aNodeInfos->IndexOf(mNodeInfo);
    NS_ASSERTION(index >= 0, "unknown nsINodeInfo index");
    rv |= aStream->Write32(index);

    // Write Attributes
    rv |= aStream->Write32(mNumAttributes);

    nsAutoString attributeValue;
    PRUint32 i;
    for (i = 0; i < mNumAttributes; ++i) {
        nsCOMPtr<nsINodeInfo> ni;
        if (mAttributes[i].mName.IsAtom()) {
            mNodeInfo->NodeInfoManager()->
                GetNodeInfo(mAttributes[i].mName.Atom(), nsnull,
                            kNameSpaceID_None, getter_AddRefs(ni));
            NS_ASSERTION(ni, "the nodeinfo should already exist");
        }
        else {
            ni = mAttributes[i].mName.NodeInfo();
        }

        index = aNodeInfos->IndexOf(ni);
        NS_ASSERTION(index >= 0, "unknown nsINodeInfo index");
        rv |= aStream->Write32(index);

        mAttributes[i].mValue.ToString(attributeValue);
        rv |= aStream->WriteWStringZ(attributeValue.get());
    }

    // Now write children
    rv |= aStream->Write32(PRUint32(mNumChildren));
    for (i = 0; i < mNumChildren; i++) {
        nsXULPrototypeNode* child = mChildren[i];
        switch (child->mType) {
        case eType_Element:
        case eType_Text:
            rv |= child->Serialize(aStream, aContext, aNodeInfos);
            break;
        case eType_Script:
            rv |= aStream->Write32(child->mType);
            nsXULPrototypeScript* script = NS_STATIC_CAST(nsXULPrototypeScript*, child);

            rv |= aStream->Write8(script->mOutOfLine);
            if (! script->mOutOfLine) {
                rv |= script->Serialize(aStream, aContext, aNodeInfos);
            } else {
                rv |= aStream->WriteCompoundObject(script->mSrcURI,
                                                   NS_GET_IID(nsIURI),
                                                   PR_TRUE);

                if (script->mJSObject) {
                    // This may return NS_OK without muxing script->mSrcURI's
                    // data into the FastLoad file, in the case where that
                    // muxed document is already there (written by a prior
                    // session, or by an earlier FastLoad episode during this
                    // session).
                    rv |= script->SerializeOutOfLine(aStream, aContext);
                }
            }
            break;
        }
    }

    return rv;
}

nsresult
nsXULPrototypeElement::Deserialize(nsIObjectInputStream* aStream,
                                   nsIScriptContext* aContext,
                                   nsIURI* aDocumentURI,
                                   const nsCOMArray<nsINodeInfo> *aNodeInfos)
{
    NS_PRECONDITION(aNodeInfos, "missing nodeinfo array");
    nsresult rv;

    // Read Node Info
    PRUint32 number;
    rv = aStream->Read32(&number);
    mNodeInfo = aNodeInfos->SafeObjectAt(number);
    if (!mNodeInfo)
        return NS_ERROR_UNEXPECTED;

    // Read Attributes
    rv |= aStream->Read32(&number);
    mNumAttributes = PRInt32(number);

    PRUint32 i;
    if (mNumAttributes > 0) {
        mAttributes = new nsXULPrototypeAttribute[mNumAttributes];
        if (! mAttributes)
            return NS_ERROR_OUT_OF_MEMORY;

        nsAutoString attributeValue;
        for (i = 0; i < mNumAttributes; ++i) {
            rv |= aStream->Read32(&number);
            nsINodeInfo* ni = aNodeInfos->SafeObjectAt(number);
            if (!ni)
                return NS_ERROR_UNEXPECTED;

            mAttributes[i].mName.SetTo(ni);

            rv |= aStream->ReadString(attributeValue);
            rv |= SetAttrAt(i, attributeValue, aDocumentURI);
        }
    }

    rv |= aStream->Read32(&number);
    mNumChildren = PRInt32(number);

    if (mNumChildren > 0) {
        mChildren = new nsXULPrototypeNode*[mNumChildren];
        if (! mChildren)
            return NS_ERROR_OUT_OF_MEMORY;

        memset(mChildren, 0, sizeof(nsXULPrototypeNode*) * mNumChildren);

        for (i = 0; i < mNumChildren; i++) {
            rv |= aStream->Read32(&number);
            Type childType = (Type)number;

            nsXULPrototypeNode* child = nsnull;

            switch (childType) {
            case eType_Element:
                child = new nsXULPrototypeElement();
                if (! child)
                    return NS_ERROR_OUT_OF_MEMORY;
                child->mType = childType;

                rv |= child->Deserialize(aStream, aContext, aDocumentURI,
                                         aNodeInfos);
                break;
            case eType_Text:
                child = new nsXULPrototypeText();
                if (! child)
                    return NS_ERROR_OUT_OF_MEMORY;
                child->mType = childType;

                rv |= child->Deserialize(aStream, aContext, aDocumentURI,
                                         aNodeInfos);
                break;
            case eType_Script: {
                // language version/options obtained during deserialization.
                // Don't clobber rv here, since it might already be a failure!
                nsresult result;
                nsXULPrototypeScript* script =
                    new nsXULPrototypeScript(0, nsnull, PR_FALSE, &result);
                if (! script)
                    return NS_ERROR_OUT_OF_MEMORY;
                if (NS_FAILED(result)) {
                    delete script;
                    return result;
                }
                child = script;
                child->mType = childType;

                rv |= aStream->Read8(&script->mOutOfLine);
                if (! script->mOutOfLine) {
                    rv |= script->Deserialize(aStream, aContext, aDocumentURI,
                                              aNodeInfos);
                } else {
                    rv |= aStream->ReadObject(PR_TRUE, getter_AddRefs(script->mSrcURI));

                    rv |= script->DeserializeOutOfLine(aStream, aContext);
                }
                break;
            }
            }

            mChildren[i] = child;

            // Oh dear. Something failed during the deserialization.
            // We don't know what.  But likely consequences of failed
            // deserializations included calls to |AbortFastLoads| which
            // shuts down the FastLoadService and closes our streams.
            // If that happens, next time through this loop, we die a messy
            // death. So, let's just fail now, and propagate that failure
            // upward so that the ChromeProtocolHandler knows it can't use
            // a cached chrome channel for this.
            if (NS_FAILED(rv))
                return rv;
        }
    }

    return rv;
}

nsresult
nsXULPrototypeElement::SetAttrAt(PRUint32 aPos, const nsAString& aValue,
                                 nsIURI* aDocumentURI)
{
    NS_PRECONDITION(aPos < mNumAttributes, "out-of-bounds");

    // WARNING!!
    // This code is largely duplicated in nsXULElement::SetAttr.
    // Any changes should be made to both functions.

    if (!mNodeInfo->NamespaceEquals(kNameSpaceID_XUL)) {
        mAttributes[aPos].mValue.ParseStringOrAtom(aValue);

        return NS_OK;
    }

    if (mAttributes[aPos].mName.Equals(nsXULAtoms::id) &&
        !aValue.IsEmpty()) {
        // Store id as atom.
        // id="" means that the element has no id. Not that it has
        // emptystring as id.
        mAttributes[aPos].mValue.ParseAtom(aValue);

        return NS_OK;
    }
    else if (mAttributes[aPos].mName.Equals(nsXULAtoms::_class)) {
        // Compute the element's class list
        mAttributes[aPos].mValue.ParseAtomArray(aValue);
        
        return NS_OK;
    }
    else if (mAttributes[aPos].mName.Equals(nsXULAtoms::style)) {
        // Parse the element's 'style' attribute
        nsCOMPtr<nsICSSStyleRule> rule;
        nsICSSParser* parser = GetCSSParser();
        NS_ENSURE_TRUE(parser, NS_ERROR_OUT_OF_MEMORY);

        // XXX Get correct Base URI (need GetBaseURI on *prototype* element)
        parser->ParseStyleAttribute(aValue, aDocumentURI, aDocumentURI,
                                    getter_AddRefs(rule));
        if (rule) {
            mAttributes[aPos].mValue.SetTo(rule);

            return NS_OK;
        }
        // Don't abort if parsing failed, it could just be malformed css.
    }

    mAttributes[aPos].mValue.ParseStringOrAtom(aValue);

    return NS_OK;
}

//----------------------------------------------------------------------
//
// nsXULPrototypeScript
//

nsXULPrototypeScript::nsXULPrototypeScript(PRUint32 aLineNo,
                                           const char *aVersion,
                                           PRBool aHasE4XOption,
                                           nsresult* rv)
    : nsXULPrototypeNode(eType_Script),
      mLineNo(aLineNo),
      mSrcLoading(PR_FALSE),
      mOutOfLine(PR_TRUE),
      mHasE4XOption(aHasE4XOption),
      mSrcLoadWaiters(nsnull),
      mJSObject(nsnull),
      mLangVersion(aVersion)
{
    NS_LOG_ADDREF(this, 1, ClassName(), ClassSize());
    *rv = nsContentUtils::AddJSGCRoot(&mJSObject,
                                      "nsXULPrototypeScript::mJSObject");
    mAddedGCRoot = NS_SUCCEEDED(*rv);
}


nsXULPrototypeScript::~nsXULPrototypeScript()
{
    if (mAddedGCRoot) {
        nsContentUtils::RemoveJSGCRoot(&mJSObject);
    }
}


nsresult
nsXULPrototypeScript::Serialize(nsIObjectOutputStream* aStream,
                                nsIScriptContext* aContext,
                                const nsCOMArray<nsINodeInfo> *aNodeInfos)
{
    NS_ASSERTION(!mSrcLoading || mSrcLoadWaiters != nsnull || !mJSObject,
                 "script source still loading when serializing?!");
    if (!mJSObject)
        return NS_ERROR_FAILURE;

    nsresult rv;

    // Write basic prototype data
    aStream->Write32(mLineNo);

    JSContext* cx = NS_REINTERPRET_CAST(JSContext*,
                                        aContext->GetNativeContext());
    JSXDRState *xdr = ::JS_XDRNewMem(cx, JSXDR_ENCODE);
    if (! xdr)
        return NS_ERROR_OUT_OF_MEMORY;
    xdr->userdata = (void*) aStream;

    JSScript *script = NS_REINTERPRET_CAST(JSScript*,
                                           ::JS_GetPrivate(cx, mJSObject));
    if (! ::JS_XDRScript(xdr, &script)) {
        rv = NS_ERROR_FAILURE;  // likely to be a principals serialization error
    } else {
        // Get the encoded JSXDRState data and write it.  The JSXDRState owns
        // this buffer memory and will free it beneath ::JS_XDRDestroy.
        //
        // If an XPCOM object needs to be written in the midst of the JS XDR
        // encoding process, the C++ code called back from the JS engine (e.g.,
        // nsEncodeJSPrincipals in caps/src/nsJSPrincipals.cpp) will flush data
        // from the JSXDRState to aStream, then write the object, then return
        // to JS XDR code with xdr reset so new JS data is encoded at the front
        // of the xdr's data buffer.
        //
        // However many XPCOM objects are interleaved with JS XDR data in the
        // stream, when control returns here from ::JS_XDRScript, we'll have
        // one last buffer of data to write to aStream.

        uint32 size;
        const char* data = NS_REINTERPRET_CAST(const char*,
                                               ::JS_XDRMemGetData(xdr, &size));
        NS_ASSERTION(data, "no decoded JSXDRState data!");

        rv = aStream->Write32(size);
        if (NS_SUCCEEDED(rv))
            rv = aStream->WriteBytes(data, size);
    }

    ::JS_XDRDestroy(xdr);
    if (NS_FAILED(rv)) return rv;

    PRUint32 version = PRUint32(mLangVersion
                                ? ::JS_StringToVersion(mLangVersion)
                                : JSVERSION_DEFAULT);
    rv = aStream->Write32(version);
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}

nsresult
nsXULPrototypeScript::SerializeOutOfLine(nsIObjectOutputStream* aStream,
                                         nsIScriptContext* aContext)
{
    nsIXULPrototypeCache* cache = GetXULCache();
#ifdef NS_DEBUG
    PRBool useXULCache = PR_TRUE;
    cache->GetEnabled(&useXULCache);
    NS_ASSERTION(useXULCache,
                 "writing to the FastLoad file, but the XUL cache is off?");
#endif

    nsCOMPtr<nsIFastLoadService> fastLoadService;
    cache->GetFastLoadService(getter_AddRefs(fastLoadService));

    nsresult rv = NS_OK;
    if (!fastLoadService)
        return NS_ERROR_NOT_AVAILABLE;

    nsCAutoString urispec;
    rv = mSrcURI->GetAsciiSpec(urispec);
    if (NS_FAILED(rv))
        return rv;

    PRBool exists = PR_FALSE;
    fastLoadService->HasMuxedDocument(urispec.get(), &exists);
    /* return will be NS_OK from GetAsciiSpec.
     * that makes no sense.
     * nor does returning NS_OK from HasMuxedDocument.
     * XXX return something meaningful.
     */
    if (exists)
        return NS_OK;

    // Allow callers to pass null for aStream, meaning
    // "use the FastLoad service's default output stream."
    // See nsXULDocument.cpp for one use of this.
    nsCOMPtr<nsIObjectOutputStream> objectOutput = aStream;
    if (! objectOutput) {
        fastLoadService->GetOutputStream(getter_AddRefs(objectOutput));
        if (! objectOutput)
            return NS_ERROR_NOT_AVAILABLE;
    }

    rv = fastLoadService->
         StartMuxedDocument(mSrcURI, urispec.get(),
                            nsIFastLoadService::NS_FASTLOAD_WRITE);
    NS_ASSERTION(rv != NS_ERROR_NOT_AVAILABLE, "reading FastLoad?!");

    nsCOMPtr<nsIURI> oldURI;
    rv |= fastLoadService->SelectMuxedDocument(mSrcURI, getter_AddRefs(oldURI));
    rv |= Serialize(objectOutput, aContext, nsnull);
    rv |= fastLoadService->EndMuxedDocument(mSrcURI);

    if (oldURI) {
        nsCOMPtr<nsIURI> tempURI;
        rv |= fastLoadService->
              SelectMuxedDocument(oldURI, getter_AddRefs(tempURI));
    }

    if (NS_FAILED(rv))
        cache->AbortFastLoads();
    return rv;
}


nsresult
nsXULPrototypeScript::Deserialize(nsIObjectInputStream* aStream,
                                  nsIScriptContext* aContext,
                                  nsIURI* aDocumentURI,
                                  const nsCOMArray<nsINodeInfo> *aNodeInfos)
{
    NS_TIMELINE_MARK_FUNCTION("chrome js deserialize");
    nsresult rv;

    // Read basic prototype data
    aStream->Read32(&mLineNo);

    NS_ASSERTION(!mSrcLoading || mSrcLoadWaiters != nsnull || !mJSObject,
                 "prototype script not well-initialized when deserializing?!");

    PRUint32 size;
    rv = aStream->Read32(&size);
    if (NS_FAILED(rv)) return rv;

    char* data;
    rv = aStream->ReadBytes(size, &data);
    if (NS_SUCCEEDED(rv)) {
        JSContext* cx = NS_REINTERPRET_CAST(JSContext*,
                                            aContext->GetNativeContext());

        JSXDRState *xdr = ::JS_XDRNewMem(cx, JSXDR_DECODE);
        if (! xdr) {
            rv = NS_ERROR_OUT_OF_MEMORY;
        } else {
            xdr->userdata = (void*) aStream;
            ::JS_XDRMemSetData(xdr, data, size);

            JSScript *script = nsnull;
            if (! ::JS_XDRScript(xdr, &script)) {
                rv = NS_ERROR_FAILURE;  // principals deserialization error?
            } else {
                mJSObject = ::JS_NewScriptObject(cx, script);
                if (! mJSObject) {
                    rv = NS_ERROR_OUT_OF_MEMORY;    // certain error
                    ::JS_DestroyScript(cx, script);
                }
            }

            // Update data in case ::JS_XDRScript called back into C++ code to
            // read an XPCOM object.
            //
            // In that case, the serialization process must have flushed a run
            // of counted bytes containing JS data at the point where the XPCOM
            // object starts, after which an encoding C++ callback from the JS
            // XDR code must have written the XPCOM object directly into the
            // nsIObjectOutputStream.
            //
            // The deserialization process will XDR-decode counted bytes up to
            // but not including the XPCOM object, then call back into C++ to
            // read the object, then read more counted bytes and hand them off
            // to the JSXDRState, so more JS data can be decoded.
            //
            // This interleaving of JS XDR data and XPCOM object data may occur
            // several times beneath the call to ::JS_XDRScript, above.  At the
            // end of the day, we need to free (via nsMemory) the data owned by
            // the JSXDRState.  So we steal it back, nulling xdr's buffer so it
            // doesn't get passed to ::JS_free by ::JS_XDRDestroy.

            uint32 junk;
            data = (char*) ::JS_XDRMemGetData(xdr, &junk);
            if (data)
                ::JS_XDRMemSetData(xdr, NULL, 0);
            ::JS_XDRDestroy(xdr);
        }

        // If data is null now, it must have been freed while deserializing an
        // XPCOM object (e.g., a principal) beneath ::JS_XDRScript.
        if (data)
            nsMemory::Free(data);
    }
    if (NS_FAILED(rv)) return rv;

    PRUint32 version;
    rv = aStream->Read32(&version);
    if (NS_FAILED(rv)) return rv;

    mLangVersion = ::JS_VersionToString(JSVersion(version));
    return NS_OK;
}


nsresult
nsXULPrototypeScript::DeserializeOutOfLine(nsIObjectInputStream* aInput,
                                           nsIScriptContext* aContext)
{
    // Keep track of FastLoad failure via rv, so we can
    // AbortFastLoads if things look bad.
    nsresult rv = NS_OK;

    nsIXULPrototypeCache* cache = GetXULCache();
    nsCOMPtr<nsIFastLoadService> fastLoadService;
    cache->GetFastLoadService(getter_AddRefs(fastLoadService));

    // Allow callers to pass null for aInput, meaning
    // "use the FastLoad service's default input stream."
    // See nsXULContentSink.cpp for one use of this.
    nsCOMPtr<nsIObjectInputStream> objectInput = aInput;
    if (! objectInput && fastLoadService)
        fastLoadService->GetInputStream(getter_AddRefs(objectInput));

    if (objectInput) {
        PRBool useXULCache = PR_TRUE;
        if (mSrcURI) {
            // NB: we must check the XUL script cache early, to avoid
            // multiple deserialization attempts for a given script, which
            // would exhaust the multiplexed stream containing the singly
            // serialized script.  Note that nsXULDocument::LoadScript
            // checks the XUL script cache too, in order to handle the
            // serialization case.
            //
            // We need do this only for <script src='strres.js'> and the
            // like, i.e., out-of-line scripts that are included by several
            // different XUL documents multiplexed in the FastLoad file.
            cache->GetEnabled(&useXULCache);

            if (useXULCache) {
                cache->GetScript(mSrcURI, NS_REINTERPRET_CAST(void**, &mJSObject));
            }
        }

        if (! mJSObject) {
            nsCOMPtr<nsIURI> oldURI;

            if (mSrcURI) {
                nsCAutoString spec;
                mSrcURI->GetAsciiSpec(spec);
                rv = fastLoadService->StartMuxedDocument(mSrcURI, spec.get(),
                                                         nsIFastLoadService::NS_FASTLOAD_READ);
                if (NS_SUCCEEDED(rv))
                    rv = fastLoadService->SelectMuxedDocument(mSrcURI, getter_AddRefs(oldURI));
            } else {
                // An inline script: check FastLoad multiplexing direction
                // and skip Deserialize if we're not reading from a
                // muxed stream to get inline objects that are contained in
                // the current document.
                PRInt32 direction;
                fastLoadService->GetDirection(&direction);
                if (direction != nsIFastLoadService::NS_FASTLOAD_READ)
                    rv = NS_ERROR_NOT_AVAILABLE;
            }

            // We do reflect errors into rv, but our caller may want to
            // ignore our return value, because mJSObject will be null
            // after any error, and that suffices to cause the script to
            // be reloaded (from the src= URI, if any) and recompiled.
            // We're better off slow-loading than bailing out due to a
            // FastLoad error.
            if (NS_SUCCEEDED(rv))
                rv = Deserialize(objectInput, aContext, nsnull, nsnull);

            if (NS_SUCCEEDED(rv) && mSrcURI) {
                rv = fastLoadService->EndMuxedDocument(mSrcURI);

                if (NS_SUCCEEDED(rv) && oldURI) {
                    nsCOMPtr<nsIURI> tempURI;
                    rv = fastLoadService->SelectMuxedDocument(oldURI, getter_AddRefs(tempURI));

                    NS_ASSERTION(NS_SUCCEEDED(rv) && (!tempURI || tempURI == mSrcURI),
                                 "not currently deserializing into the script we thought we were!");
                }
            }

            if (NS_SUCCEEDED(rv)) {
                if (useXULCache && mSrcURI) {
                    PRBool isChrome = PR_FALSE;
                    mSrcURI->SchemeIs("chrome", &isChrome);
                    if (isChrome) {
                        cache->PutScript(mSrcURI, NS_REINTERPRET_CAST(void*, mJSObject));
                    }
                }
            } else {
                // If mSrcURI is not in the FastLoad multiplex,
                // rv will be NS_ERROR_NOT_AVAILABLE and we'll try to
                // update the FastLoad file to hold a serialization of
                // this script, once it has finished loading.
                if (rv != NS_ERROR_NOT_AVAILABLE)
                    cache->AbortFastLoads();
            }
        }
    }

    return rv;
}

nsresult
nsXULPrototypeScript::Compile(const PRUnichar* aText,
                              PRInt32 aTextLength,
                              nsIURI* aURI,
                              PRUint32 aLineNo,
                              nsIDocument* aDocument,
                              nsIXULPrototypeDocument* aPrototypeDocument)
{
    // We'll compile the script using the prototype document's special
    // script object as the parent. This ensures that we won't end up
    // with an uncollectable reference.
    //
    // Compiling it using (for example) the first document's global
    // object would cause JS to keep a reference via the __proto__ or
    // __parent__ pointer to the first document's global. If that
    // happened, our script object would reference the first document,
    // and the first document would indirectly reference the prototype
    // document because it keeps the prototype cache alive. Circularity!
    nsresult rv;

    // Use the prototype document's special context
    nsIScriptContext *context;

    {
        nsCOMPtr<nsIScriptGlobalObjectOwner> globalOwner =
            do_QueryInterface(aPrototypeDocument);
        nsIScriptGlobalObject* global = globalOwner->GetScriptGlobalObject();
        NS_ASSERTION(global != nsnull, "prototype doc has no script global");
        if (! global)
            return NS_ERROR_UNEXPECTED;

        context = global->GetContext();

        NS_ASSERTION(context != nsnull, "no context for script global");
        if (! context)
            return NS_ERROR_UNEXPECTED;
    }

    // Use the enclosing document's principal
    // XXX is this right? or should we use the protodoc's?
    nsIPrincipal *principal = aDocument->GetNodePrincipal();
    if (!principal)
        return NS_ERROR_FAILURE;

    nsCAutoString urlspec;
    aURI->GetSpec(urlspec);

    // Ok, compile it to create a prototype script object!

    // XXXbe violate nsIScriptContext layering because its version parameter
    // is mis-typed as const char * -- if it were uint32, we could more easily
    // extend version to include compile-time option, as the JS engine does.
    // It'd also be more efficient than converting to and from a C string.

    JSContext* cx = NS_REINTERPRET_CAST(JSContext*,
                                        context->GetNativeContext());
    uint32 options = ::JS_GetOptions(cx);
    JSBool changed = (mHasE4XOption ^ !!(options & JSOPTION_XML));
    if (changed) {
        ::JS_SetOptions(cx,
                        mHasE4XOption
                        ? options | JSOPTION_XML
                        : options & ~JSOPTION_XML);
    }

    rv = context->CompileScript(aText,
                                aTextLength,
                                nsnull,
                                principal,
                                urlspec.get(),
                                aLineNo,
                                mLangVersion,
                                (void**)&mJSObject);

    if (changed) {
        ::JS_SetOptions(cx, options);
    }
    return rv;
}

//----------------------------------------------------------------------
//
// nsXULPrototypeText
//

nsresult
nsXULPrototypeText::Serialize(nsIObjectOutputStream* aStream,
                              nsIScriptContext* aContext,
                              const nsCOMArray<nsINodeInfo> *aNodeInfos)
{
    nsresult rv;

    // Write basic prototype data
    rv = aStream->Write32(mType);

    rv |= aStream->WriteWStringZ(mValue.get());

    return rv;
}

nsresult
nsXULPrototypeText::Deserialize(nsIObjectInputStream* aStream,
                                nsIScriptContext* aContext,
                                nsIURI* aDocumentURI,
                                const nsCOMArray<nsINodeInfo> *aNodeInfos)
{
    nsresult rv;

    // Write basic prototype data
    rv = aStream->ReadString(mValue);

    return rv;
}
