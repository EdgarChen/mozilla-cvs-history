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
 * Copyright (C) 2001 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include "nsCOMPtr.h"
#include "nsString.h"

#include "nsIRDFResource.h"
#include "nsISimpleEnumerator.h"
#include "nsIXULWindow.h"

class nsWindowMediator;

/********************************************************************/
/**************************** nsWindowInfo **************************/
/********************************************************************/

struct nsWindowInfo
{
  nsWindowInfo(nsIXULWindow* inWindow, PRInt32 inTimeStamp);
  ~nsWindowInfo();

  nsCOMPtr<nsIRDFResource>  mRDFID;
  nsCOMPtr<nsIXULWindow>    mWindow;
  PRInt32                   mTimeStamp;

  // each struct is in two, independent, circular, doubly-linked lists
  nsWindowInfo              *mYounger, // next younger in sequence
                            *mOlder;
  nsWindowInfo              *mLower,   // next lower in z-order
                            *mHigher;
  
  PRBool TypeEquals(nsAReadableString &aType);
  void   InsertAfter(nsWindowInfo *inOlder, nsWindowInfo *inHigher);
  void   Unlink(PRBool inAge, PRBool inZ);
  void   ReferenceSelf(PRBool inAge, PRBool inZ);
};


/********************************************************************/
/************************ virtual enumerators ***********************/
/********************************************************************/

class nsAppShellWindowEnumerator : public nsISimpleEnumerator {

friend class nsWindowMediator;

public:
  nsAppShellWindowEnumerator (const PRUnichar* aTypeString,
                      nsWindowMediator& inMediator);
  virtual ~nsAppShellWindowEnumerator();
  NS_IMETHOD GetNext(nsISupports **retval) = 0;
  NS_IMETHOD HasMoreElements(PRBool *retval);

  NS_DECL_ISUPPORTS

protected:

  void AdjustInitialPosition();
  virtual nsWindowInfo *FindNext() = 0;

  void WindowRemoved(nsWindowInfo *inInfo);

  nsWindowMediator *mWindowMediator;
  nsString          mType;
  nsWindowInfo     *mCurrentPosition;
};

class nsASDOMWindowEnumerator : public nsAppShellWindowEnumerator {

public:
  nsASDOMWindowEnumerator (const PRUnichar* aTypeString,
                           nsWindowMediator& inMediator);
  virtual ~nsASDOMWindowEnumerator();
  NS_IMETHOD GetNext(nsISupports **retval);
};

class nsASXULWindowEnumerator : public nsAppShellWindowEnumerator {

public:
  nsASXULWindowEnumerator (const PRUnichar* aTypeString,
                           nsWindowMediator& inMediator);
  virtual ~nsASXULWindowEnumerator();
  NS_IMETHOD GetNext(nsISupports **retval);
};

/********************************************************************/
/*********************** concrete enumerators ***********************/
/********************************************************************/

class nsASDOMWindowEarlyToLateEnumerator : public nsASDOMWindowEnumerator {

public:
  nsASDOMWindowEarlyToLateEnumerator(
             const PRUnichar* aTypeString,
             nsWindowMediator& inMediator);

  virtual ~nsASDOMWindowEarlyToLateEnumerator();

protected:
  virtual nsWindowInfo *FindNext();
};

class nsASXULWindowEarlyToLateEnumerator : public nsASXULWindowEnumerator {

public:
  nsASXULWindowEarlyToLateEnumerator(
             const PRUnichar* aTypeString,
             nsWindowMediator& inMediator);

  virtual ~nsASXULWindowEarlyToLateEnumerator();

protected:
  virtual nsWindowInfo *FindNext();
};

class nsASDOMWindowFrontToBackEnumerator : public nsASDOMWindowEnumerator {

public:
  nsASDOMWindowFrontToBackEnumerator(
             const PRUnichar* aTypeString,
             nsWindowMediator& inMediator);

  virtual ~nsASDOMWindowFrontToBackEnumerator();

protected:
  virtual nsWindowInfo *FindNext();
};

class nsASXULWindowFrontToBackEnumerator : public nsASXULWindowEnumerator {

public:
  nsASXULWindowFrontToBackEnumerator(
             const PRUnichar* aTypeString,
             nsWindowMediator& inMediator);

  virtual ~nsASXULWindowFrontToBackEnumerator();

protected:
  virtual nsWindowInfo *FindNext();
};

class nsASDOMWindowBackToFrontEnumerator : public nsASDOMWindowEnumerator {

public:
  nsASDOMWindowBackToFrontEnumerator (
             const PRUnichar* aTypeString,
             nsWindowMediator& inMediator);

  virtual ~nsASDOMWindowBackToFrontEnumerator();

protected:
  virtual nsWindowInfo *FindNext();
};

class nsASXULWindowBackToFrontEnumerator : public nsASXULWindowEnumerator {

public:
  nsASXULWindowBackToFrontEnumerator (
             const PRUnichar* aTypeString,
             nsWindowMediator& inMediator);

  virtual ~nsASXULWindowBackToFrontEnumerator();

protected:
  virtual nsWindowInfo *FindNext();
};


