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

#include <stdio.h>
#include "nsLDAPConnection.h"

// constructor
//
nsLDAPConnection::nsLDAPConnection()
{
}

// destructor
// XXX better error-handling than fprintf
//
nsLDAPConnection::~nsLDAPConnection()
{
  int rc;

  rc = ldap_unbind_s(this->connectionHandle);
  if (rc != LDAP_SUCCESS) {
    fprintf(stderr, "nsLDAPConnection::~nsLDAPConnection: %s\n", 
	    ldap_err2string(rc));
  }
}

// wrapper for ldap_init()
//
bool
nsLDAPConnection::Init(const char *defhost, const int defport)
{
  this->connectionHandle = ldap_init(defhost, defport);
  return (this->connectionHandle == NULL ? false : true);
}

// wrapper for ldap_get_lderrno
//
int
nsLDAPConnection::GetLdErrno(char **matched, char **string)
{
  return ldap_get_lderrno(this->connectionHandle, matched, string);
}

// return the error string corresponding to GetLdErrno.
// result should be freed with ldap_memfree()
//
// XXX - deal with optional params
// XXX - how does ldap_perror know to look at the global errno?
char *
nsLDAPConnection::GetErrorString(void)
{
  return ldap_err2string(this->GetLdErrno(NULL, NULL));
}
