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
 */

#include "nsMenuItem.h"
#include "nsIMenu.h"
#include "nsIMenuBar.h"
#include "nsIPopUpMenu.h"

static NS_DEFINE_IID(kIMenuIID,     NS_IMENU_IID);
static NS_DEFINE_IID(kIMenuBarIID,  NS_IMENUBAR_IID);
static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIPopUpMenuIID, NS_IPOPUPMENU_IID);
static NS_DEFINE_IID(kIMenuItemIID, NS_IMENUITEM_IID);

nsresult nsMenuItem::QueryInterface(REFNSIID aIID, void** aInstancePtr)      
{                                                                        
  if (NULL == aInstancePtr) {                                            
    return NS_ERROR_NULL_POINTER;                                        
  }                                                                      
                                                                         
  *aInstancePtr = NULL;                                                  
                                                                                        
  if (aIID.Equals(kIMenuItemIID)) {                                         
    *aInstancePtr = (void*)(nsIMenuItem*)this;                                        
    NS_ADDREF_THIS();                                                    
    return NS_OK;                                                        
  }                                                                      
  if (aIID.Equals(NS_GET_IID(nsIMenuListener))) {                                      
    *aInstancePtr = (void*)(nsIMenuListener*)this;                        
    NS_ADDREF_THIS();                                                    
    return NS_OK;                                                        
  }                                                     
  if (aIID.Equals(kISupportsIID)) {                                      
    *aInstancePtr = (void*)(nsISupports*)(nsIMenuItem*)this;                     
    NS_ADDREF_THIS();                                                    
    return NS_OK;                                                        
  }
  return NS_NOINTERFACE;                                                 
}

NS_IMPL_ADDREF(nsMenuItem)
NS_IMPL_RELEASE(nsMenuItem)

nsMenuItem::nsMenuItem() : nsIMenuItem()
{
  NS_INIT_REFCNT();
}

nsMenuItem::~nsMenuItem()
{
}

nsIWidget * nsMenuItem::GetMenuBarParent(nsISupports * aParent)
{
  return nsnull;
}

NS_METHOD nsMenuItem::Create(nsISupports    *aParent,
                             const nsString &aLabel,
                             PRBool          aIsSeparator)
{
  return NS_OK;
}

NS_METHOD nsMenuItem::Create(nsIPopUpMenu   *aParent, 
                             const nsString &aLabel, 
                             PRUint32        aCommand)
{
  return NS_OK;
}

NS_METHOD nsMenuItem::Create(nsIMenu * aParent)
{
  return NS_OK;
}

NS_METHOD nsMenuItem::Create(nsIPopUpMenu * aParent)
{
  return NS_OK;
}


NS_METHOD nsMenuItem::GetLabel(nsString &aText)
{
  return NS_OK;
}

NS_METHOD nsMenuItem::SetLabel(nsString &aText)
{
  return NS_OK;
}

NS_METHOD nsMenuItem::GetCheckboxType(PRBool *aIsCheckbox)
{
  return NS_OK;
}

NS_METHOD nsMenuItem::SetCheckboxType(PRBool aIsCheckbox)
{
  return NS_OK;
}

NS_METHOD nsMenuItem::GetCommand(PRUint32 & aCommand)
{
  return NS_OK;
}

NS_METHOD nsMenuItem::GetTarget(nsIWidget *& aTarget)
{
  return NS_OK;
}

NS_METHOD nsMenuItem::GetNativeData(void *& aData)
{
  return NS_OK;
}

NS_METHOD nsMenuItem::AddMenuListener(nsIMenuListener * aMenuListener)
{
  return NS_OK;
}

NS_METHOD nsMenuItem::RemoveMenuListener(nsIMenuListener * aMenuListener)
{
  return NS_OK;
}

void nsMenuItem::SetCmdId(PRInt32 aId)
{
}

PRInt32 nsMenuItem::GetCmdId()
{
  return 0;
}

NS_METHOD nsMenuItem::IsSeparator(PRBool & aIsSep)
{
  return NS_OK;
}

NS_METHOD nsMenuItem::SetEnabled(PRBool aIsEnabled)
{
  return NS_OK;
}

NS_METHOD nsMenuItem::GetEnabled(PRBool *aIsEnabled)
{
  return NS_OK;
}

NS_METHOD nsMenuItem::SetChecked(PRBool aIsEnabled)
{
  return NS_OK;
}

NS_METHOD nsMenuItem::GetChecked(PRBool *aIsEnabled)
{
  return NS_OK;
}

nsEventStatus nsMenuItem::MenuItemSelected(const nsMenuEvent & aMenuEvent)
{
  return nsEventStatus_eIgnore;
}

nsEventStatus nsMenuItem::MenuSelected(const nsMenuEvent & aMenuEvent)
{
  return nsEventStatus_eIgnore;
}

nsEventStatus nsMenuItem::MenuDeselected(const nsMenuEvent & aMenuEvent)
{
  return nsEventStatus_eIgnore;
}

nsEventStatus nsMenuItem::MenuConstruct(const nsMenuEvent & aMenuEvent,
                                        nsIWidget         * aParentWindow, 
                                        void              * menubarNode,
                                        void              * aWebShell)
{
  return nsEventStatus_eIgnore;
}

nsEventStatus nsMenuItem::MenuDestruct(const nsMenuEvent & aMenuEvent)
{
  return nsEventStatus_eIgnore;
}

NS_METHOD nsMenuItem::SetDOMNode(nsIDOMNode * aDOMNode)
{
  return NS_OK;
}
    
//-------------------------------------------------------------------------
NS_METHOD nsMenuItem::GetDOMNode(nsIDOMNode ** aDOMNode)
{
  return NS_OK;
}

NS_METHOD nsMenuItem::SetDOMElement(nsIDOMElement * aDOMElement)
{
  return NS_OK;
}

NS_METHOD nsMenuItem::GetDOMElement(nsIDOMElement ** aDOMElement)
{
  return NS_OK;
}

NS_METHOD nsMenuItem::SetWebShell(nsIWebShell * aWebShell)
{
  return NS_OK;
}

NS_METHOD nsMenuItem::SetCommand(const nsString & aStrCmd)
{
  return NS_OK;
}

NS_METHOD nsMenuItem::DoCommand()
{
  return NS_OK;
}

NS_IMETHODIMP nsMenuItem::SetShortcutChar(const nsString &aText)
{
  mKeyEquivalent = aText;
  return NS_OK;
}

//----------------------------------------------------------------------
NS_IMETHODIMP nsMenuItem::GetShortcutChar(nsString &aText)
{
  aText = mKeyEquivalent;
  return NS_OK;
}

//----------------------------------------------------------------------
NS_IMETHODIMP nsMenuItem::SetModifiers(PRUint8 aModifiers)
{
  mModifiers = aModifiers;
  return NS_OK;
}

//----------------------------------------------------------------------
NS_IMETHODIMP nsMenuItem::GetModifiers(PRUint8 * aModifiers)
{
  *aModifiers = mModifiers; 
  return NS_OK;
}
