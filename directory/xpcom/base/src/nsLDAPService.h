/* 
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is the mozilla.org LDAP XPCOM component.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s): Dan Mosedale <dmose@mozilla.org>
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL"), in which case the provisions of the GPL are applicable 
 * instead of those above.  If you wish to allow use of your 
 * version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by
 * the GPL.  If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the
 * GPL.
 */

#ifndef nsLDAPService_h___
#define nsLDAPService_h___

#include "nsIRunnable.h"
#include "nsCOMPtr.h"
#include "nsIThread.h"

// 6a89ae33-7a90-430d-888c-0dede53a951a 
//
#define NS_LDAPSERVICE_CID \
{ \
  0x6a89ae33, 0x7a90, 0x430d, \
  {0x88, 0x8c, 0x0d, 0xed, 0xe5, 0x3a, 0x95, 0x1a} \
}

class nsLDAPService : public nsIRunnable
{
 public: 
  // interface decls
  //
  NS_DECL_ISUPPORTS;
  NS_DECL_NSIRUNNABLE;

  // constructor and destructor
  //
  nsLDAPService();
  virtual ~nsLDAPService();

  // initialize; should only be called by the generic 
  // constructor after creation
  //
  NS_METHOD Init(void);

 protected:
  nsCOMPtr<nsIThread> mThread;

};

#endif // nsLDAPService_h___
