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
 * The Original Code is C++ hashtable templates.
 *
 * The Initial Developer of the Original Code is
 * Benjamin Smedberg.
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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

#ifndef nsInterfaceHashtableImpl_h__
#define nsInterfaceHashtableImpl_h__

#ifndef nsInterfaceHashtable_h__
#include "nsInterfaceHashtable.h"
#endif

#ifndef nsBaseHashtableImpl_h__
#include "nsBaseHashtableImpl.h"
#endif

/* nsInterfaceHashtable definitions */

template<class KeyClass,class Interface>
PRBool
nsInterfaceHashtable<KeyClass,Interface>::Get
  (KeyType aKey, UserDataType* pInterface) const
{
  if (mLock)
    PR_RWLock_Rlock(mLock);

  typename nsBaseHashtable<KeyClass, nsCOMPtr<Interface>, Interface*>::EntryType* ent =
    GetEntry(KeyClass::KeyToPointer(aKey));

  if (ent)
  {
    if (pInterface)
    {
      *pInterface = ent->mData;

      NS_IF_ADDREF(*pInterface);
    }

    if (mLock)
      PR_RWLock_Unlock(mLock);

    return PR_TRUE;
  }

  // if the key doesn't exist, set *pInterface to null
  // so that it is a valid XPCOM getter
  if (pInterface)
    *pInterface = nsnull;

  if (mLock)
    PR_RWLock_Unlock(mLock);

  return PR_FALSE;
}

#endif // nsInterfaceHashtableImpl_h__
