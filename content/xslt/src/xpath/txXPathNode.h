/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 * The Original Code is TransforMiiX XSLT processor code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2003
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Peter Van der Beken <peterv@propagandism.org>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
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

#ifndef txXPathNode_h__
#define txXPathNode_h__

#ifdef TX_EXE
#include "dom.h"
#else
#include "nsAutoPtr.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIDOMNode.h"
#include "nsINameSpaceManager.h"

extern nsINameSpaceManager* gTxNameSpaceManager;
#endif

#ifdef TX_EXE
typedef Node txXPathNodeType;
#else
typedef nsIDOMNode txXPathNodeType;
#endif

class txXPathNode
{
public:
    PRBool operator==(const txXPathNode& aNode) const;
    PRBool operator!=(const txXPathNode& aNode) const
    {
        return !(*this == aNode);
    }

private:
    friend class txNodeSet;
    friend class txXPathNativeNode;
    friend class txXPathNodeUtils;
    friend class txXPathTreeWalker;

#ifdef TX_EXE
    txXPathNode(NodeDefinition* aNode) : mInner(aNode)
    {
    }
    txXPathNode(const txXPathNode& aNode);

    NodeDefinition* mInner;
#else
    txXPathNode(nsIDocument* aDocument) : mDocument(aDocument),
                                          mIndex(eDocument)
    {
    }
    txXPathNode(nsIContent* aContent, PRUint32 aIndex = eContent)
        : mContent(aContent),
          mIndex(aIndex)
    {
    }
    txXPathNode(const txXPathNode& aNode);

    PRBool isDocument() const
    {
        return mIndex == eDocument;
    };
    PRBool isContent() const
    {
        return mIndex == eContent;
    };
    PRBool isAttribute() const
    {
        return mIndex != eDocument && mIndex != eContent;
    };

    enum PositionType
    {
        eDocument = (PRUint32)-2,
        eContent = (PRUint32)-1
    };
    union {
        nsIDocument* mDocument;        // eDocument
        nsIContent* mContent;          // eContent, eAttribute
    };
    PRUint32 mIndex;
#endif
};

class txNamespaceManager
{
public:
    static PRInt32 getNamespaceID(const nsAString& aNamespaceURI);
    static nsresult getNamespaceURI(const PRInt32 aID, nsAString& aResult);
};

/* static */
inline PRInt32
txNamespaceManager::getNamespaceID(const nsAString& aNamespaceURI)
{
#ifdef TX_EXE
    return txStandaloneNamespaceManager::getNamespaceID(aNamespaceURI);
#else
    NS_ASSERTION(gTxNameSpaceManager, "No namespace manager");

    PRInt32 namespaceID = kNameSpaceID_Unknown;
    gTxNameSpaceManager->RegisterNameSpace(aNamespaceURI, namespaceID);
    return namespaceID;
#endif
}

/* static */
inline nsresult
txNamespaceManager::getNamespaceURI(const PRInt32 aID, nsAString& aResult)
{
#ifdef TX_EXE
    return txStandaloneNamespaceManager::getNamespaceURI(aID, aResult);
#else
    NS_ASSERTION(gTxNameSpaceManager, "No namespace manager");

    return gTxNameSpaceManager->GetNameSpaceURI(aID, aResult);
#endif
}

#endif /* txXPathNode_h__ */
