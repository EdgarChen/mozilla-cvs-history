/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */
#ifndef nsCSSScanner_h___
#define nsCSSScanner_h___

#include "nsString.h"
class nsIUnicharInputStream;

// Token types
enum nsCSSTokenType {
  // A css identifier (e.g. foo)
  eCSSToken_Ident = 0,          // mIdent

  // A css at keyword (e.g. @foo)
  eCSSToken_AtKeyword = 1,      // mIdent

  // A css number without a percentage or dimension; with percentage;
  // without percentage but with a dimension
  eCSSToken_Number = 2,         // mNumber
  eCSSToken_Percentage = 3,     // mNumber
  eCSSToken_Dimension = 4,      // mNumber + mIdent

  // A css string (e.g. "foo" or 'foo')
  eCSSToken_String = 5,         // mSymbol + mIdent + mSymbol

  // Whitespace (e.g. " " or "/* abc */" or "// foo <eol>")
  eCSSToken_WhiteSpace = 6,     // mIdent

  // A css symbol (e.g. ':', ';', '+', etc.)
  eCSSToken_Symbol = 7,         // mSymbol

  // A css1 id (e.g. #foo3)
  eCSSToken_ID = 8,             // mIdent

  eCSSToken_Function = 9,       // mIdent

  eCSSToken_URL = 10,            // mIdent
  eCSSToken_InvalidURL = 11,    // doesn't matter

};

struct nsCSSToken {
  nsCSSTokenType  mType;
  nsAutoString    mIdent;
  float           mNumber;
  PRInt32         mInteger;
  PRBool          mIntegerValid;
  PRUnichar       mSymbol;

  nsCSSToken();

  PRBool isDimension() {
    return (PRBool)
      ((eCSSToken_Dimension == mType) ||
       ((eCSSToken_Number == mType) && (mNumber == 0.0f)));
  }
};

// CSS Scanner API. Used to tokenize an input stream using the CSS
// forward compatible tokenization rules. This implementation is
// private to this package and is only used internally by the css
// parser.
class nsCSSScanner {
  public:
  nsCSSScanner();
  ~nsCSSScanner();

  // Init the scanner.
  void Init(nsIUnicharInputStream* aInput);

  // Get the next token. Return nsfalse on EOF or ERROR. aTokenResult
  // is filled in with the data for the token.
  PRBool Next(PRInt32& aErrorCode, nsCSSToken& aTokenResult);

  // Get the next token that may be a string or unquoted URL or whitespace
  PRBool NextURL(PRInt32& aErrorCode, nsCSSToken& aTokenResult);

protected:
  void Close();
  PRInt32 Read(PRInt32& aErrorCode);
  PRInt32 Peek(PRInt32& aErrorCode);
  void Unread();
  void Pushback(PRUnichar aChar);
  PRBool LookAhead(PRInt32& aErrorCode, PRUnichar aChar);
  PRBool EatWhiteSpace(PRInt32& aErrorCode);
  PRBool EatNewline(PRInt32& aErrorCode);

  PRInt32 ParseEscape(PRInt32& aErrorCode);
  PRBool ParseIdent(PRInt32& aErrorCode, PRInt32 aChar, nsCSSToken& aResult);
  PRBool ParseAtKeyword(PRInt32& aErrorCode, PRInt32 aChar,
                        nsCSSToken& aResult);
  PRBool ParseNumber(PRInt32& aErrorCode, PRInt32 aChar, nsCSSToken& aResult);
  PRBool ParseID(PRInt32& aErrorCode, PRInt32 aChar, nsCSSToken& aResult);
  PRBool ParseString(PRInt32& aErrorCode, PRInt32 aChar, nsCSSToken& aResult);
  PRBool ParseEOLComment(PRInt32& aErrorCode, nsCSSToken& aResult);
  PRBool ParseCComment(PRInt32& aErrorCode, nsCSSToken& aResult);

  PRBool GatherString(PRInt32& aErrorCode, PRInt32 aStop,
                      nsString& aString);
  PRBool GatherIdent(PRInt32& aErrorCode, PRInt32 aChar, nsString& aIdent);

  nsIUnicharInputStream* mInput;
  PRUnichar* mBuffer;
  PRInt32 mOffset;
  PRInt32 mCount;
  PRUnichar* mPushback;
  PRInt32 mPushbackCount;
  PRInt32 mPushbackSize;
  PRInt32 mLastRead;
  PRUnichar mLocalPushback[4];
};

#endif /* nsCSSScanner_h___ */
