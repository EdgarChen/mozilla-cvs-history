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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include "nsIFormSubmission.h"

#include "nsIPresContext.h"
#include "nsCOMPtr.h"
#include "nsIForm.h"
#include "nsILinkHandler.h"
#include "nsIDocument.h"
#include "nsHTMLAtoms.h"
#include "nsIHTMLDocument.h"
#include "nsIFormControl.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsDOMError.h"
#include "nsHTMLValue.h"
#include "nsGenericElement.h"
#include "nsISaveAsCharset.h"

// JBK added for submit move from content frame
#include "nsIFile.h"
#include "nsIFileStreams.h"
#include "nsIFileSpec.h"
#include "nsDirectoryServiceDefs.h"
#include "nsIFormProcessor.h"
static NS_DEFINE_CID(kFormProcessorCID, NS_FORMPROCESSOR_CID);
#include "nsIURI.h"
#include "nsNetUtil.h"
static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);
#include "nsIPref.h"
#include "nsSpecialSystemDirectory.h"
#include "nsLinebreakConverter.h"
#include "nsICharsetConverterManager.h"
static NS_DEFINE_CID(kCharsetConverterManagerCID,
                     NS_ICHARSETCONVERTERMANAGER_CID);
#include "xp_path.h"
#include "nsICharsetAlias.h"
#include "nsEscape.h"
#include "nsUnicharUtils.h"
#include "nsIMultiplexInputStream.h"
#include "nsIMIMEInputStream.h"

//BIDI
#ifdef IBMBIDI
#include "nsBidiUtils.h"
#else
//
// Make BIDI stuff work when BIDI is off
//
#define GET_BIDI_OPTION_CONTROLSTEXTMODE(x) 0
#define GET_BIDI_OPTION_DIRECTION(x) 0
#endif
//end


/**
 * Helper superclass implementation of nsIFormSubmission, providing common
 * methods that most of the specific implementations need and use.
 */
class nsFormSubmission : public nsIFormSubmission {

public:

  /**
   * @param aCharset the charset of the form as a string
   * @param aEncoder an encoder that will encode Unicode names and values into
   *        bytes to be sent over the wire (usually a charset transformation)
   * @param aFormProcessor a form processor who can listen to 
   * @param aBidiOptions the BIDI options flags for the current pres context
   */
  nsFormSubmission(const nsAString& aCharset,
                   nsISaveAsCharset* aEncoder,
                   nsIFormProcessor* aFormProcessor,
                   PRInt32 aBidiOptions)
    : mCharset(aCharset),
      mEncoder(aEncoder),
      mFormProcessor(aFormProcessor),
      mBidiOptions(aBidiOptions)
  { NS_INIT_ISUPPORTS(); };
  virtual ~nsFormSubmission() { };

  NS_DECL_ISUPPORTS

  //
  // nsIFormSubmission
  //
  NS_IMETHOD SubmitTo(nsIURI* aActionURL, const nsAString& aTarget,
                      nsIContent* aSource, nsIPresContext* aPresContext,
                      nsIDocShell** aDocShell, nsIRequest** aRequest);

  /**
   * Called to initialize the submission.  Perform any initialization that may
   * fail here.  Subclasses *must* implement this.
   */
  NS_IMETHOD Init() = 0;

protected:
  // this is essentially the nsFormSubmission interface (to be overridden)
  /**
   * Given a URI and the current submission, create the final URI and data
   * stream that will be submitted.  Subclasses *must* implement this.
   *
   * @param aURL the URL being submitted to [INOUT]
   * @param aPostDataStream a data stream for POST data [OUT]
   */
  NS_IMETHOD GetEncodedSubmission(nsIURI* aURL,
                                  nsIInputStream** aPostDataStream) = 0;

  // Helpers
  /**
   * Call to have the form processor listen in when a name/value pair is found
   * to be submitted.
   *
   * @param aSource the HTML element the name/value is associated with
   * @param aName the name that will be submitted
   * @param aValue the value that will be submitted
   * @return the new value (or null if the value was not changed)
   */
  nsString* ProcessValue(nsIDOMHTMLElement* aSource,
                         const nsAString& aName, const nsAString& aValue);

  // Encoding Helpers
  /**
   * Encode a Unicode string to bytes using the encoder (or just do ToNewCString
   * if there is no encoder).
   * @param aStr the string to encode
   * @return a char* pointing to the encoded bytes (use nsMemory::Free to free)
   */
  char* EncodeVal(const nsAString& aStr);
  /**
   * Encode a Unicode string to bytes using an encoder.  (Used by EncodeVal)
   * @param aStr the string to encode
   * @param aLen the length of aStr
   * @param aEncoder the encoder to encode the bytes with (cannot be null)
   * @return a char* pointing to the encoded bytes (use nsMemory::Free to free)
   */
  char* UnicodeToNewBytes(const PRUnichar* aStr, PRUint32 aLen,
                          nsISaveAsCharset* aEncoder);

  /** The name of the encoder charset */
  nsString mCharset;
  /** The encoder that will encode Unicode names and values into
   *  bytes to be sent over the wire (usually a charset transformation)
   */
  nsCOMPtr<nsISaveAsCharset> mEncoder;
  /** A form processor who can listen to values */
  nsCOMPtr<nsIFormProcessor> mFormProcessor;
  /** The BIDI options flags for the current pres context */
  PRInt32 mBidiOptions;

public:
  // Static helpers

  /**
   * Get the submit charset for a form (suitable to pass in to the constructor).
   * @param aForm the form in question
   * @param aCtrlsModAtSubmit BIDI controls text mode.  Unused in non-BIDI
   *        builds.
   * @param aCharset the returned charset [OUT]
   */
  static void GetSubmitCharset(nsIForm* aForm,
                               PRUint8 aCtrlsModAtSubmit,
                               nsAString& aCharset);
  /**
   * Get the encoder for a form (suitable to pass in to the constructor).
   * @param aForm the form in question
   * @param aPresContext the pres context in which we are submitting
   * @param aCharset the charset of the form
   * @param aEncoder the returned encoder [OUT]
   */
  static nsresult GetEncoder(nsIForm* aForm,
                             nsIPresContext* aPresContext,
                             const nsAString& aCharset,
                             nsISaveAsCharset** aEncoder);
  /**
   * Get an attribute of a form as int, provided that it is an enumerated value.
   * @param aForm the form in question
   * @param aAtom the attribute (for example, nsHTMLAtoms::enctype) to get
   * @param aValue the result (will not be set at all if the attribute does not
   *        exist on the form, so *make sure you provide a default value*.)
   *        [OUT]
   */
  static void GetEnumAttr(nsIForm* aForm, nsIAtom* aAtom, PRInt32* aValue);
};


/**
 * Handles URL encoded submissions, both of the POST and GET variety.
 */
class nsFSURLEncoded : public nsFormSubmission
{
public:
  /**
   * @param aCharset the charset of the form as a string
   * @param aEncoder an encoder that will encode Unicode names and values into
   *        bytes to be sent over the wire (usually a charset transformation)
   * @param aFormProcessor a form processor who can listen to 
   * @param aBidiOptions the BIDI options flags for the current pres context
   * @param aMethod the method of the submit (either NS_FORM_METHOD_GET or
   *        NS_FORM_METHOD_POST).
   */
  nsFSURLEncoded(const nsAString& aCharset,
                 nsISaveAsCharset* aEncoder,
                 nsIFormProcessor* aFormProcessor,
                 PRInt32 aBidiOptions,
                 PRInt32 aMethod)
    : nsFormSubmission(aCharset, aEncoder, aFormProcessor, aBidiOptions),
      mMethod(aMethod)
  { }
  virtual ~nsFSURLEncoded() { }
 
  NS_DECL_ISUPPORTS_INHERITED

  // nsIFormSubmission
  NS_IMETHOD AddNameValuePair(nsIDOMHTMLElement* aSource,
                              const nsAString& aName,
                              const nsAString& aValue);
  NS_IMETHOD AddNameFilePair(nsIDOMHTMLElement* aSource,
                             const nsAString& aName,
                             const nsAString& aFilename,
                             nsIInputStream* aStream,
                             const nsACString& aContentType,
                             PRBool aMoreFilesToCome);
  NS_IMETHOD AcceptsFiles(PRBool* aAcceptsFiles)
      { *aAcceptsFiles = PR_FALSE; return NS_OK; }

  NS_IMETHOD Init();

protected:
  // nsFormSubmission
  NS_IMETHOD GetEncodedSubmission(nsIURI* aURI,
                                  nsIInputStream** aPostDataStream);

  // Helpers
  /**
   * URL encode a Unicode string by encoding it to bytes, converting linebreaks
   * properly, and then escaping many bytes as %xx.
   *
   * @param aStr the string to encode
   * @param aEncoded the encoded string [OUT]
   */
  void URLEncode(const nsAString& aStr, nsCString& aEncoded);

private:
  /**
   * The method of the submit (either NS_FORM_METHOD_GET or
   * NS_FORM_METHOD_POST).
   */
  PRInt32 mMethod;

  /** The query string so far (the part after the ?) */
  nsCString mQueryString;
};

NS_IMPL_RELEASE_INHERITED(nsFSURLEncoded, nsFormSubmission)
NS_IMPL_ADDREF_INHERITED(nsFSURLEncoded, nsFormSubmission)
NS_IMPL_QUERY_INTERFACE_INHERITED0(nsFSURLEncoded, nsFormSubmission)

NS_IMETHODIMP
nsFSURLEncoded::AddNameValuePair(nsIDOMHTMLElement* aSource,
                                 const nsAString& aName,
                                 const nsAString& aValue)
{
  //
  // Let external code process (and possibly change) value
  //
  nsString* processedValue = ProcessValue(aSource, aName, aValue);

  //
  // Encode name
  //
  nsCString convName;
  URLEncode(aName, convName);

  //
  // Encode value
  //
  nsCString convValue;
  if (processedValue) {
    URLEncode(*processedValue, convValue);
  } else {
    URLEncode(aValue, convValue);
  }

  //
  // Append data to string
  //
  if (mQueryString.IsEmpty()) {
    mQueryString += convName + NS_LITERAL_CSTRING("=") + convValue;
  } else {
    mQueryString += NS_LITERAL_CSTRING("&") + convName
                  + NS_LITERAL_CSTRING("=") + convValue;
  }

  delete processedValue;

  return NS_OK;
}

NS_IMETHODIMP
nsFSURLEncoded::AddNameFilePair(nsIDOMHTMLElement* aSource,
                                const nsAString& aName,
                                const nsAString& aFilename,
                                nsIInputStream* aStream,
                                const nsACString& aContentType,
                                PRBool aMoreFilesToCome)
{
  return AddNameValuePair(aSource,aName,aFilename);
}

//
// nsFormSubmission
//
NS_IMETHODIMP
nsFSURLEncoded::Init()
{
  mQueryString.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
nsFSURLEncoded::GetEncodedSubmission(nsIURI* aURI,
                                     nsIInputStream** aPostDataStream)
{
  nsresult rv = NS_OK;

  *aPostDataStream = nsnull;

  if (mMethod == NS_FORM_METHOD_POST) {
    nsCOMPtr<nsIInputStream> dataStream;
    // XXX We *really* need to either get the string to disown its data (and
    // not destroy it), or make a string input stream that owns the CString
    // that is passed to it.  Right now this operation does a copy.
    rv = NS_NewCStringInputStream(getter_AddRefs(dataStream), mQueryString);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIMIMEInputStream> mimeStream(
      do_CreateInstance("@mozilla.org/network/mime-input-stream;1", &rv));
    NS_ENSURE_SUCCESS(rv, rv);

#ifdef SPECIFY_CHARSET_IN_CONTENT_TYPE
    mimeStream->AddHeader("Content-Type",
                          PromiseFlatString(
                            "application/x-www-form-urlencoded; charset="
                            + mCharset
                          ).get());
#else
    mimeStream->AddHeader("Content-Type",
                          "application/x-www-form-urlencoded");
#endif
    mimeStream->SetAddContentLength(PR_TRUE);
    mimeStream->SetData(dataStream);

    *aPostDataStream = mimeStream;
    NS_ADDREF(*aPostDataStream);

  } else {
    //
    // Get the full query string
    //
    PRBool schemeIsJavaScript;
    rv = aURI->SchemeIs("javascript", &schemeIsJavaScript);
    NS_ENSURE_SUCCESS(rv, rv);
    if (schemeIsJavaScript) {
      return NS_OK;
    }

    nsCAutoString path;
    rv = aURI->GetPath(path);
    NS_ENSURE_SUCCESS(rv, rv);
    // Bug 42616: Trim off named anchor and save it to add later
    PRInt32 namedAnchorPos = path.FindChar('#');
    nsCAutoString namedAnchor;
    if (kNotFound != namedAnchorPos) {
      path.Right(namedAnchor, (path.Length() - namedAnchorPos));
      path.Truncate(namedAnchorPos);
    }

    // Chop off old query string (bug 25330, 57333)
    // Only do this for GET not POST (bug 41585)
    PRInt32 queryStart = path.FindChar('?');
    if (kNotFound != queryStart) {
      path.Truncate(queryStart);
    }

    path.Append('?');
    // Bug 42616: Add named anchor to end after query string
    path.Append(mQueryString + namedAnchor);

    aURI->SetPath(path);
  }

  return rv;
}

// i18n helper routines
void
nsFSURLEncoded::URLEncode(const nsAString& aStr, nsCString& aEncoded)
{
  char* inBuf = EncodeVal(aStr);

  if (!inBuf)
    inBuf = ToNewCString(aStr);

  // convert to CRLF breaks
  char* convertedBuf = nsLinebreakConverter::ConvertLineBreaks(inBuf,
                           nsLinebreakConverter::eLinebreakAny,
                           nsLinebreakConverter::eLinebreakNet);
  nsMemory::Free(inBuf);

  char* escapedBuf = nsEscape(convertedBuf, url_XPAlphas);
  nsMemory::Free(convertedBuf);

  aEncoded.Adopt(escapedBuf);
}



/**
 * Handle multipart/form-data encoding, which does files as well as normal
 * inputs.  This always does POST.
 */
class nsFSMultipartFormData : public nsFormSubmission
{
public:
  /**
   * @param aCharset the charset of the form as a string
   * @param aEncoder an encoder that will encode Unicode names and values into
   *        bytes to be sent over the wire (usually a charset transformation)
   * @param aFormProcessor a form processor who can listen to 
   * @param aBidiOptions the BIDI options flags for the current pres context
   */
  nsFSMultipartFormData(const nsAString& aCharset,
                        nsISaveAsCharset* aEncoder,
                        nsIFormProcessor* aFormProcessor,
                        PRInt32 aBidiOptions);
  virtual ~nsFSMultipartFormData() { }
 
  NS_DECL_ISUPPORTS_INHERITED

  // nsIFormSubmission
  NS_IMETHOD AddNameValuePair(nsIDOMHTMLElement* aSource,
                              const nsAString& aName,
                              const nsAString& aValue);
  NS_IMETHOD AddNameFilePair(nsIDOMHTMLElement* aSource,
                             const nsAString& aName,
                             const nsAString& aFilename,
                             nsIInputStream* aStream,
                             const nsACString& aContentType,
                             PRBool aMoreFilesToCome);
  NS_IMETHOD AcceptsFiles(PRBool* aAcceptsFiles)
      { *aAcceptsFiles = PR_TRUE; return NS_OK; }

  NS_IMETHOD Init();

protected:
  // nsFormSubmission
  NS_IMETHOD GetEncodedSubmission(nsIURI* aURI,
                                  nsIInputStream** aPostDataStream);

  // Helpers
  nsresult AddPostDataStream();

private:
  /**
   * Get whether we are supposed to be doing backwards compatible submit, which
   * causes us to leave off the mandatory Content-Transfer-Encoding header.
   * This used to cause Bad Things, including server crashes.
   *
   * It is hoped that we can get rid of this at some point, but that will take
   * a lot of testing or some other browsers that send the header and have not
   * had problems.
   */
  PRBool mBackwardsCompatibleSubmit;

  /**
   * The post data stream as it is so far.  This is a collection of smaller
   * chunks--string streams and file streams interleaved to make one big POST
   * stream.
   */
  nsCOMPtr<nsIMultiplexInputStream> mPostDataStream;

  /**
   * The current string chunk.  When a file is hit, the string chunk gets
   * wrapped up into an input stream and put into mPostDataStream so that the
   * file input stream can then be appended and everything is in the right
   * order.  Then the string chunk gets appended to again as we process more
   * name/value pairs.
   */
  nsCString mPostDataChunk;

  /**
   * The boundary string to use after each "part" (the boundary that marks the
   * end of a value).  This is computed randomly and is different for each
   * submission.
   */
  nsCString mBoundary;
};

NS_IMPL_RELEASE_INHERITED(nsFSMultipartFormData, nsFormSubmission)
NS_IMPL_ADDREF_INHERITED(nsFSMultipartFormData, nsFormSubmission)
NS_IMPL_QUERY_INTERFACE_INHERITED0(nsFSMultipartFormData, nsFormSubmission)

//
// Constructor
//
nsFSMultipartFormData::nsFSMultipartFormData(const nsAString& aCharset,
                                             nsISaveAsCharset* aEncoder,
                                             nsIFormProcessor* aFormProcessor,
                                             PRInt32 aBidiOptions)
    : nsFormSubmission(aCharset, aEncoder, aFormProcessor, aBidiOptions)
{
  // XXX I can't *believe* we have a pref for this.  ifdef, anyone?
  mBackwardsCompatibleSubmit = PR_FALSE;
  nsCOMPtr<nsIPref> prefService(do_GetService(NS_PREF_CONTRACTID));
  if (prefService)
    prefService->GetBoolPref("browser.forms.submit.backwards_compatible",
                             &mBackwardsCompatibleSubmit);
}

//
// nsIFormSubmission
//
NS_IMETHODIMP
nsFSMultipartFormData::AddNameValuePair(nsIDOMHTMLElement* aSource,
                                        const nsAString& aName,
                                        const nsAString& aValue)
{
  //
  // Let external code process (and possibly change) value
  //
  nsString* processedValue = ProcessValue(aSource, aName, aValue);

  //
  // Get name
  //
  nsCString nameStr;
  nameStr.Adopt(EncodeVal(aName));

  //
  // Get value
  //
  nsCString valueStr;
  if (processedValue) {
    valueStr.Adopt(EncodeVal(*processedValue));
  } else {
    valueStr.Adopt(EncodeVal(aValue));
  }

  //
  // Convert linebreaks in value
  //
  valueStr.Adopt(nsLinebreakConverter::ConvertLineBreaks(valueStr.get(),
                 nsLinebreakConverter::eLinebreakAny,
                 nsLinebreakConverter::eLinebreakNet));

  //
  // Make MIME block for name/value pair
  //
  mPostDataChunk += NS_LITERAL_CSTRING("--") + mBoundary
                 + NS_LITERAL_CSTRING(CRLF)
                 + NS_LITERAL_CSTRING("Content-Disposition: form-data; name=\"")
                 + nameStr + NS_LITERAL_CSTRING("\"" CRLF CRLF)
                 + valueStr + NS_LITERAL_CSTRING(CRLF);

  delete processedValue;

  return NS_OK;
}

NS_IMETHODIMP
nsFSMultipartFormData::AddNameFilePair(nsIDOMHTMLElement* aSource,
                                       const nsAString& aName,
                                       const nsAString& aFilename,
                                       nsIInputStream* aStream,
                                       const nsACString& aContentType,
                                       PRBool aMoreFilesToCome)
{
  //
  // Let external code process (and possibly change) value
  //
  nsString* processedValue = ProcessValue(aSource, aName, aFilename);

  //
  // Get name
  //
  nsCString nameStr;
  nameStr.Adopt(EncodeVal(aName));

  //
  // Get filename
  //
  nsCString filenameStr;
  if (processedValue) {
    filenameStr.Adopt(EncodeVal(*processedValue));
  } else {
    filenameStr.Adopt(EncodeVal(aFilename));
  }

  //
  // Convert linebreaks in filename
  //
  filenameStr.Adopt(nsLinebreakConverter::ConvertLineBreaks(filenameStr.get(),
                    nsLinebreakConverter::eLinebreakAny,
                    nsLinebreakConverter::eLinebreakNet));

  //
  // Make MIME block for name/value pair
  //
  // more appropriate than always using binary?
  mPostDataChunk += NS_LITERAL_CSTRING("--") + mBoundary
                 + NS_LITERAL_CSTRING(CRLF);
  if (!mBackwardsCompatibleSubmit) {
    // XXX Is there any way to tell when "8bit" or "7bit" etc may be
    mPostDataChunk +=
          NS_LITERAL_CSTRING("Content-Transfer-Encoding: binary" CRLF);
  }
  mPostDataChunk +=
         NS_LITERAL_CSTRING("Content-Disposition: form-data; name=\"")
       + nameStr + NS_LITERAL_CSTRING("\"; filename=\"")
       + filenameStr + NS_LITERAL_CSTRING("\"" CRLF)
       + NS_LITERAL_CSTRING("Content-Type: ") + aContentType
       + NS_LITERAL_CSTRING(CRLF CRLF);

  //
  // Add the file to the stream
  //
  if (aStream) {
    // We need to dump the data up to this point into the POST data stream here,
    // since we're about to add the file input stream
    AddPostDataStream();

    mPostDataStream->AppendStream(aStream);
  }

  //
  // CRLF after file
  //
  mPostDataChunk += NS_LITERAL_CSTRING(CRLF);

  delete processedValue;

  return NS_OK;
}

//
// nsFormSubmission
//
NS_IMETHODIMP
nsFSMultipartFormData::Init()
{
  nsresult rv;

  //
  // Create the POST stream
  //
  mPostDataStream =
    do_CreateInstance("@mozilla.org/io/multiplex-input-stream;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!mPostDataStream) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  //
  // Build boundary
  //
  mBoundary = NS_LITERAL_CSTRING("---------------------------");
  mBoundary.AppendInt(rand());
  mBoundary.AppendInt(rand());
  mBoundary.AppendInt(rand());

  return NS_OK;
}

NS_IMETHODIMP
nsFSMultipartFormData::GetEncodedSubmission(nsIURI* aURI,
                                            nsIInputStream** aPostDataStream)
{
  nsresult rv;

  //
  // Finish data
  //
  mPostDataChunk += NS_LITERAL_CSTRING("--") + mBoundary
                  + NS_LITERAL_CSTRING("--" CRLF);

  //
  // Add final data input stream
  //
  AddPostDataStream();

  //
  // Make header
  //
  nsCOMPtr<nsIMIMEInputStream> mimeStream
    = do_CreateInstance("@mozilla.org/network/mime-input-stream;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString boundaryHeaderValue(
    NS_LITERAL_CSTRING("multipart/form-data; boundary=") + mBoundary);

  mimeStream->AddHeader("Content-Type", boundaryHeaderValue.get());
  mimeStream->SetAddContentLength(PR_TRUE);
  mimeStream->SetData(mPostDataStream);

  *aPostDataStream = mimeStream;

  NS_ADDREF(*aPostDataStream);

  return NS_OK;
}

nsresult
nsFSMultipartFormData::AddPostDataStream()
{
  nsresult rv = NS_OK;
  
  nsCOMPtr<nsIInputStream> postDataChunkStream;
  rv = NS_NewCStringInputStream(getter_AddRefs(postDataChunkStream),
                                mPostDataChunk);
  NS_ASSERTION(postDataChunkStream, "Could not open a stream for POST!");
  if (postDataChunkStream) {
    mPostDataStream->AppendStream(postDataChunkStream);
  }

  mPostDataChunk.Truncate();

  return rv;
}


//
// CLASS nsFSTextPlain
//
class nsFSTextPlain : public nsFormSubmission
{
public:
  nsFSTextPlain(const nsAString& aCharset,
                nsISaveAsCharset* aEncoder,
                nsIFormProcessor* aFormProcessor,
                PRInt32 aBidiOptions)
    : nsFormSubmission(aCharset, aEncoder, aFormProcessor, aBidiOptions)
  { }
  virtual ~nsFSTextPlain() { }
 
  NS_DECL_ISUPPORTS_INHERITED

  // nsIFormSubmission
  NS_IMETHOD AddNameValuePair(nsIDOMHTMLElement* aSource,
                              const nsAString& aName,
                              const nsAString& aValue);
  NS_IMETHOD AddNameFilePair(nsIDOMHTMLElement* aSource,
                             const nsAString& aName,
                             const nsAString& aFilename,
                             nsIInputStream* aStream,
                             const nsACString& aContentType,
                             PRBool aMoreFilesToCome);

  NS_IMETHOD Init();

protected:
  // nsFormSubmission
  NS_IMETHOD GetEncodedSubmission(nsIURI* aURI,
                                  nsIInputStream** aPostDataStream);
  NS_IMETHOD AcceptsFiles(PRBool* aAcceptsFiles)
      { *aAcceptsFiles = PR_FALSE; return NS_OK; }

private:
  nsString mBody;
};

NS_IMPL_RELEASE_INHERITED(nsFSTextPlain, nsFormSubmission)
NS_IMPL_ADDREF_INHERITED(nsFSTextPlain, nsFormSubmission)
NS_IMPL_QUERY_INTERFACE_INHERITED0(nsFSTextPlain, nsFormSubmission)

NS_IMETHODIMP
nsFSTextPlain::AddNameValuePair(nsIDOMHTMLElement* aSource,
                                const nsAString& aName,
                                const nsAString& aValue)
{
  //
  // Let external code process (and possibly change) value
  //
  nsString* processedValue = ProcessValue(aSource, aName, aValue);

  // XXX This won't work well with a name like "a=b" or "a\nb" but I suppose
  // text/plain doesn't care about that.  Parsers aren't built for escaped
  // values so we'll have to live with it.
  if (processedValue) {
    mBody.Append(aName + NS_LITERAL_STRING("=") + *processedValue +
                 NS_LITERAL_STRING(CRLF));

    delete processedValue;
  } else {
    mBody.Append(aName + NS_LITERAL_STRING("=") + aValue +
                 NS_LITERAL_STRING(CRLF));
  }

  return NS_OK;
}

NS_IMETHODIMP
nsFSTextPlain::AddNameFilePair(nsIDOMHTMLElement* aSource,
                               const nsAString& aName,
                               const nsAString& aFilename,
                               nsIInputStream* aStream,
                               const nsACString& aContentType,
                               PRBool aMoreFilesToCome)
{
  AddNameValuePair(aSource,aName,aFilename);
  return NS_OK;
}

//
// nsFormSubmission
//
NS_IMETHODIMP
nsFSTextPlain::Init()
{
  mBody.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
nsFSTextPlain::GetEncodedSubmission(nsIURI* aURI,
                                    nsIInputStream** aPostDataStream)
{
  nsresult rv = NS_OK;

  // XXX HACK We are using the standard URL mechanism to give the body to the
  // mailer instead of passing the post data stream to it, since that sounds
  // hard.
  PRBool isMailto = PR_FALSE;
  aURI->SchemeIs("mailto", &isMailto);
  if (isMailto) {
    nsCAutoString path;
    rv = aURI->GetPath(path);
    NS_ENSURE_SUCCESS(rv, rv);

    // Walk through the string and see if we have a subject already.
    PRBool hasSubject = PR_FALSE;
    PRBool hasParams = PR_FALSE;
    PRInt32 paramSep = path.FindChar('?');
    while (paramSep != kNotFound && paramSep < (PRInt32)path.Length()) {
      hasParams = PR_TRUE;

      // Get the end of the name at the = op.  If it is *after* the next &,
      // assume that someone made a parameter without an = in it
      PRInt32 nameEnd = path.FindChar('=', paramSep+1);
      PRInt32 nextParamSep = path.FindChar('&', paramSep+1);
      if (nextParamSep == kNotFound) {
        nextParamSep = path.Length();
      }

      // If the = op is after the &, this parameter is a name without value.
      // If there is no = op, same thing.
      if (nameEnd == kNotFound || nextParamSep < nameEnd) {
        nameEnd = nextParamSep;
      }

      if (nameEnd != kNotFound) {
        if (Substring(path, paramSep+1, nameEnd-(paramSep+1)) ==
            NS_LITERAL_CSTRING("subject")) {
          hasSubject = PR_TRUE;
          break;
        }
      }

      paramSep = nextParamSep;
    }

    // If there is no subject, append a preformed subject to the mailto line
    if (!hasSubject) {
      if (hasParams) {
        path.Append('&');
      } else {
        path.Append('?');
      }

      path += NS_LITERAL_CSTRING("subject=Form%20Post%20From%20Mozilla&");
    }

    // Append the body to and force-plain-text args to the mailto line
    nsCString escapedBody;
    escapedBody.Adopt(nsEscape(NS_ConvertUCS2toUTF8(mBody).get(), url_XAlphas));

    path += NS_LITERAL_CSTRING("&force-plain-text=Y&body=") + escapedBody;

    rv = aURI->SetPath(path);

  } else {

    // Create data stream
    nsCOMPtr<nsIInputStream> bodyStream;
    rv = NS_NewStringInputStream(getter_AddRefs(bodyStream),
                                          mBody);
    if (!bodyStream) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    // Create mime stream with headers and such
    nsCOMPtr<nsIMIMEInputStream> mimeStream
        = do_CreateInstance("@mozilla.org/network/mime-input-stream;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    mimeStream->AddHeader("Content-Type", "text/plain");
    mimeStream->SetAddContentLength(PR_TRUE);
    mimeStream->SetData(bodyStream);
    CallQueryInterface(mimeStream, aPostDataStream);
    NS_ADDREF(*aPostDataStream);
  }

  return rv;
}


//
// CLASS nsFormSubmission
//

//
// nsISupports stuff
//

NS_IMPL_ADDREF(nsFormSubmission)
NS_IMPL_RELEASE(nsFormSubmission)

NS_INTERFACE_MAP_BEGIN(nsFormSubmission)
  NS_INTERFACE_MAP_ENTRY(nsIFormSubmission)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END


// JBK moved from nsFormFrame - bug 34297
// submission

// static
nsresult
GetSubmissionFromForm(nsIForm* aForm,
                      nsIPresContext* aPresContext,
                      nsIFormSubmission** aFormSubmission)
{
  nsresult rv = NS_OK;

  //
  // Get all the information necessary to encode the form data
  //

  // Get BIDI options
  PRUint32 bidiOptions = 0;
  PRUint8 ctrlsModAtSubmit = 0;
#ifdef IBMBIDI
  aPresContext->GetBidi(&bidiOptions);
  ctrlsModAtSubmit = GET_BIDI_OPTION_CONTROLSTEXTMODE(bidiOptions);
#endif

  // Get encoding type (default: urlencoded)
  PRInt32 enctype = NS_FORM_ENCTYPE_URLENCODED;
  nsFormSubmission::GetEnumAttr(aForm, nsHTMLAtoms::enctype, &enctype);

  // Get method (default: GET)
  PRInt32 method = NS_FORM_METHOD_GET;
  nsFormSubmission::GetEnumAttr(aForm, nsHTMLAtoms::method, &method);

  // Get charset
  nsAutoString charset;
  nsFormSubmission::GetSubmitCharset(aForm, ctrlsModAtSubmit, charset);

  // Get unicode encoder
  nsCOMPtr<nsISaveAsCharset> encoder;
  nsFormSubmission::GetEncoder(aForm, aPresContext, charset,
                               getter_AddRefs(encoder));

  // Get form processor
  nsCOMPtr<nsIFormProcessor> formProcessor =
    do_GetService(kFormProcessorCID, &rv);

  //
  // Choose encoder
  //
  // If enctype=multipart/form-data and method=post, do multipart
  // Else do URL encoded
  // NOTE:
  // The rule used to be, if enctype=multipart/form-data, do multipart
  // Else do URL encoded
  if (method == NS_FORM_METHOD_POST &&
      enctype == NS_FORM_ENCTYPE_MULTIPART) {
    *aFormSubmission = new nsFSMultipartFormData(charset, encoder,
                                                 formProcessor, bidiOptions);
  } else if (method == NS_FORM_METHOD_POST &&
             enctype == NS_FORM_ENCTYPE_TEXTPLAIN) {
    *aFormSubmission = new nsFSTextPlain(charset, encoder,
                                         formProcessor, bidiOptions);
  } else {
    *aFormSubmission = new nsFSURLEncoded(charset, encoder,
                                          formProcessor, bidiOptions, method);
  }
  NS_ENSURE_TRUE(*aFormSubmission, NS_ERROR_OUT_OF_MEMORY);
  NS_ADDREF(*aFormSubmission);


  // This ASSUMES that all encodings above inherit from nsFormSubmission, which
  // they currently do.  If that changes, change this too.
  NS_STATIC_CAST(nsFormSubmission*, *aFormSubmission)->Init();

  return NS_OK;
}

NS_IMETHODIMP
nsFormSubmission::SubmitTo(nsIURI* aActionURL, const nsAString& aTarget,
                           nsIContent* aSource, nsIPresContext* aPresContext,
                           nsIDocShell** aDocShell, nsIRequest** aRequest)
{
  nsresult rv;

  //
  // Finish encoding (get post data stream and URL)
  //
  nsCOMPtr<nsIInputStream> postDataStream;
  rv = GetEncodedSubmission(aActionURL, getter_AddRefs(postDataStream));
  NS_ENSURE_SUCCESS(rv, rv);

  //
  // Actually submit the data
  //
  nsCOMPtr<nsILinkHandler> handler;
  aPresContext->GetLinkHandler(getter_AddRefs(handler));
  NS_ENSURE_TRUE(handler, NS_ERROR_FAILURE);

  nsCAutoString actionURLSpec;
  aActionURL->GetSpec(actionURLSpec);

  return handler->OnLinkClickSync(aSource, eLinkVerb_Replace,
                                  NS_ConvertUTF8toUCS2(actionURLSpec).get(),
                                  PromiseFlatString(aTarget).get(),
                                  postDataStream, nsnull,
                                  aDocShell, aRequest);
}

// JBK moved from nsFormFrame - bug 34297
// static
void
nsFormSubmission::GetSubmitCharset(nsIForm* form,
                                   PRUint8 aCtrlsModAtSubmit,
                                   nsAString& oCharset)
{
  oCharset = NS_LITERAL_STRING("UTF-8"); // default to utf-8

  nsresult rv = NS_OK;
  nsAutoString acceptCharsetValue;
  nsCOMPtr<nsIHTMLContent> formContent = do_QueryInterface(form);
  nsHTMLValue value;
  rv = formContent->GetHTMLAttribute(nsHTMLAtoms::acceptcharset, value);
  if (rv == NS_CONTENT_ATTR_HAS_VALUE && value.GetUnit() == eHTMLUnit_String) {
    value.GetStringValue(acceptCharsetValue);
  }

  PRInt32 charsetLen = acceptCharsetValue.Length();
  if (charsetLen > 0) {
    PRInt32 offset=0;
    PRInt32 spPos=0;
    // get charset from charsets one by one
    nsCOMPtr<nsICharsetAlias> calias(do_GetService(kCharsetAliasCID, &rv));
    if (NS_FAILED(rv)) {
      return;
    }
    if (calias) {
      do {
        spPos = acceptCharsetValue.FindChar(PRUnichar(' '), offset);
        PRInt32 cnt = ((-1==spPos)?(charsetLen-offset):(spPos-offset));
        if (cnt > 0) {
          nsAutoString charset;
          acceptCharsetValue.Mid(charset, offset, cnt);
          if (NS_SUCCEEDED(calias->GetPreferred(charset, oCharset)))
            return;
        }
        offset = spPos + 1;
      } while (spPos != -1);
    }
  }
  // if there are no accept-charset or all the charset are not supported
  // Get the charset from document
  nsCOMPtr<nsGenericElement> formElement = do_QueryInterface(form);
  if (formElement) {
    nsIDocument* doc = nsnull;
    formElement->GetDocument(doc);
    if (doc) {
      rv = doc->GetDocumentCharacterSet(oCharset);
      NS_RELEASE(doc);
    }
  }

#ifdef IBMBIDI
  if (aCtrlsModAtSubmit==IBMBIDI_CONTROLSTEXTMODE_VISUAL
     && oCharset.Equals(NS_LITERAL_STRING("windows-1256"),
                 nsCaseInsensitiveStringComparator())) {
//Mohamed
    oCharset = NS_LITERAL_STRING("IBM864");
  }
  else if (aCtrlsModAtSubmit==IBMBIDI_CONTROLSTEXTMODE_LOGICAL
          && oCharset.Equals(NS_LITERAL_STRING("IBM864"),
                     nsCaseInsensitiveStringComparator())) {
    oCharset = NS_LITERAL_STRING("IBM864i");
  }
  else if (aCtrlsModAtSubmit==IBMBIDI_CONTROLSTEXTMODE_VISUAL
          && oCharset.Equals(NS_LITERAL_STRING("ISO-8859-6"),
                     nsCaseInsensitiveStringComparator())) {
    oCharset = NS_LITERAL_STRING("IBM864");
  }
  else if (aCtrlsModAtSubmit==IBMBIDI_CONTROLSTEXTMODE_VISUAL
          && oCharset.Equals(NS_LITERAL_STRING("UTF-8"),
                     nsCaseInsensitiveStringComparator())) {
    oCharset = NS_LITERAL_STRING("IBM864");
  }

#endif
}

// JBK moved from nsFormFrame - bug 34297
// static
nsresult
nsFormSubmission::GetEncoder(nsIForm* form,
                             nsIPresContext* aPresContext,
                             const nsAString& aCharset,
                             nsISaveAsCharset** aEncoder)
{
  *aEncoder = nsnull;
  nsresult rv = NS_OK;

  nsAutoString charset(aCharset);
  if(charset.Equals(NS_LITERAL_STRING("ISO-8859-1")))
    charset.Assign(NS_LITERAL_STRING("windows-1252"));

  rv = CallCreateInstance( NS_SAVEASCHARSET_CONTRACTID, aEncoder);
  NS_ASSERTION(NS_SUCCEEDED(rv), "create nsISaveAsCharset failed");
  NS_ENSURE_SUCCESS(rv, rv);

  rv = (*aEncoder)->Init(NS_ConvertUCS2toUTF8(charset).get(),
                         (nsISaveAsCharset::attr_EntityAfterCharsetConv + 
                          nsISaveAsCharset::attr_FallbackDecimalNCR),
                         0);
  NS_ASSERTION(NS_SUCCEEDED(rv), "initialize nsISaveAsCharset failed");
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

// i18n helper routines
char*
nsFormSubmission::UnicodeToNewBytes(const PRUnichar* aStr, PRUint32 aLen,
                                    nsISaveAsCharset* aEncoder)
{
  nsresult rv = NS_OK;

#ifdef IBMBIDI
  PRUint8 ctrlsModAtSubmit = GET_BIDI_OPTION_CONTROLSTEXTMODE(mBidiOptions);
  PRUint8 textDirAtSubmit = GET_BIDI_OPTION_DIRECTION(mBidiOptions);
  //ahmed 15-1
  nsAutoString temp;
  nsAutoString newBuffer;
  //This condition handle the RTL,LTR for a logical file
  if (ctrlsModAtSubmit == IBMBIDI_CONTROLSTEXTMODE_VISUAL
     && mCharset.Equals(NS_LITERAL_STRING("windows-1256"),
                nsCaseInsensitiveStringComparator())) {
    Conv_06_FE_WithReverse(nsString(aStr),
                           newBuffer,
                           textDirAtSubmit);
    aStr = (PRUnichar*)newBuffer.get();
    aLen=newBuffer.Length();
  }
  else if (ctrlsModAtSubmit == IBMBIDI_CONTROLSTEXTMODE_LOGICAL
          && mCharset.Equals(NS_LITERAL_STRING("IBM864"),
                             nsCaseInsensitiveStringComparator())) {
    //For 864 file, When it is logical, if LTR then only convert
    //If RTL will mak a reverse for the buffer
    Conv_FE_06(nsString(aStr), newBuffer);
    aStr = (PRUnichar*)newBuffer.get();
    temp = newBuffer;
    aLen=newBuffer.Length();
    if (textDirAtSubmit == 2) { //RTL
    //Now we need to reverse the Buffer, it is by searching the buffer
      PRUint32 loop = aLen;
      PRUint32 z;
      for (z=0; z<=aLen; z++) {
        temp.SetCharAt((PRUnichar)aStr[loop], z);
        loop--;
      }
    }
    aStr = (PRUnichar*)temp.get();
  }
  else if (ctrlsModAtSubmit == IBMBIDI_CONTROLSTEXTMODE_VISUAL
          && mCharset.Equals(NS_LITERAL_STRING("IBM864"),
                             nsCaseInsensitiveStringComparator())
                  && textDirAtSubmit == IBMBIDI_TEXTDIRECTION_RTL) {

    Conv_FE_06(nsString(aStr), newBuffer);
    aStr = (PRUnichar*)newBuffer.get();
    temp = newBuffer;
    aLen=newBuffer.Length();
    //Now we need to reverse the Buffer, it is by searching the buffer
    PRUint32 loop = aLen;
    PRUint32 z;
    for (z=0; z<=aLen; z++) {
      temp.SetCharAt((PRUnichar)aStr[loop], z);
      loop--;
    }
    aStr = (PRUnichar*)temp.get();
  }
#endif

  
  char* res = nsnull;
  if(aStr && aStr[0]) {
    rv = aEncoder->Convert(aStr, &res);
    NS_ASSERTION(NS_SUCCEEDED(rv), "conversion failed");
  } 
  if(! res)
    res = nsCRT::strdup("");
  return res;
}


// static
void
nsFormSubmission::GetEnumAttr(nsIForm* form, nsIAtom* atom, PRInt32* aValue)
{
  nsCOMPtr<nsIHTMLContent> content = do_QueryInterface(form);
  if (content) {
    nsHTMLValue value;
    if (content->GetHTMLAttribute(atom, value) == NS_CONTENT_ATTR_HAS_VALUE) {
      if (eHTMLUnit_Enumerated == value.GetUnit()) {
        *aValue = value.GetIntValue();
      }
    }
  }
}

char*
nsFormSubmission::EncodeVal(const nsAString& aStr)
{
  char* retval;
  if (mEncoder) {
    retval = UnicodeToNewBytes(PromiseFlatString(aStr).get(), aStr.Length(),
                               mEncoder);
  } else {
    retval = ToNewCString(aStr);
  }

  return retval;
}

nsString*
nsFormSubmission::ProcessValue(nsIDOMHTMLElement* aSource,
                               const nsAString& aName, const nsAString& aValue)
{
  nsString* retval = nsnull;
  if (mFormProcessor) {
    // XXX We need to change the ProcessValue interface to take nsAString
    nsString tmpNameStr(aName);
    retval = new nsString(aValue);
    if (!retval) {
      return nsnull;
    }

    nsresult rv = mFormProcessor->ProcessValue(aSource, tmpNameStr, *retval);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Unable to Notify form process observer");
  }

  return retval;
}
