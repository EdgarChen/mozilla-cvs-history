/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

// ==============================
// ! Scriptability related code !
// ==============================
//
// nsScriptablePeer - xpconnect scriptable peer
//

#ifndef __nsScriptablePeer_h__
#define __nsScriptablePeer_h__

#include "nsI4xScrPlugin.h"
#include "nsISecurityCheckedComponent.h"

class CPlugin;

// we should add nsISecurityCheckedComponent because the
// Mozilla Security Manager will ask scriptable object about 
// priveleges the plugin requests in order to allow calls
// from JavaScript
class nsScriptablePeer : public nsI4xScrPlugin,
                         public nsISecurityCheckedComponent
{
public:
  nsScriptablePeer(CPlugin* plugin);
  ~nsScriptablePeer();

  NS_DECL_ISUPPORTS
  NS_DECL_NSI4XSCRPLUGIN
  NS_DECL_NSISECURITYCHECKEDCOMPONENT

protected:
  CPlugin* mPlugin;
};

#endif