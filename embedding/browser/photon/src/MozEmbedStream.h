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
 * The Original Code is mozilla.org code.
 * 
 * The Initial Developer of the Original Code is Christopher Blizzard.
 * Portions created by Christopher Blizzard are Copyright (C)
 * Christopher Blizzard.  All Rights Reserved.
 * 
 * Contributor(s):
 *   Christopher Blizzard <blizzard@mozilla.org>
 */

#include "nsISupports.h"
#include "nsIInputStream.h"

class MozEmbedStream : public nsIInputStream 
{
 public:

  MozEmbedStream();
  virtual ~MozEmbedStream();

  // nsISupports
  NS_DECL_ISUPPORTS
  // nsIInputStream
  NS_DECL_NSIINPUTSTREAM
  // nsIBaseStream
  NS_DECL_NSIBASESTREAM

  NS_METHOD Append(const char *aData, PRUint32 aLen); 

 private:

  char *    mBuffer;
  PRUint32  mLength;
  PRUint32  mBufLen;
};
