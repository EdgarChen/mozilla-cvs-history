/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#include "nsIStreamListener.h"
#include "nsIURL.h"
#include "nsIInputStream.h"
#include "nsString.h"
#include "rdf-int.h"
#include "rdfparse.h"

class rdfStreamListener : public nsIStreamListener
{
public:

  NS_DECL_ISUPPORTS

  rdfStreamListener(RDFFile);
  ~rdfStreamListener();

  NS_METHOD GetBindInfo(nsIURL* aURL);

  NS_METHOD OnProgress(nsIURL* aURL, PRInt32 Progress, PRInt32 ProgressMax,  const nsString &aMsg);

  NS_METHOD OnStatus(nsIURL* aURL, const nsString& aMsg);

  NS_METHOD OnStartBinding(nsIURL* aURL, const char *aContentType);

  NS_METHOD OnDataAvailable(nsIURL* aURL, nsIInputStream *pIStream, PRInt32 length);

  NS_METHOD OnStopBinding(nsIURL* aURL, PRInt32 status, const nsString& aMsg);

protected:
  rdfStreamListener::rdfStreamListener();

private:
  RDFFile mFile;
};

rdfStreamListener::rdfStreamListener(RDFFile f) : mFile(f)
{
}


// ISupports implementation...
NS_IMPL_ADDREF(rdfStreamListener)
NS_IMPL_RELEASE(rdfStreamListener)

nsresult rdfStreamListener::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
    if (NULL == aInstancePtr) {
        return NS_ERROR_NULL_POINTER;
    }
    return NS_NOINTERFACE;
}


rdfStreamListener::~rdfStreamListener()
{
}

NS_METHOD
rdfStreamListener::GetBindInfo(nsIURL* aURL)
{
  return NS_OK;
}

NS_METHOD
rdfStreamListener::OnProgress(nsIURL* aURL,
			      PRInt32 Progress,
			      PRInt32 ProgressMax,  const nsString &aMsg)
{
  return NS_OK;
}

NS_METHOD
rdfStreamListener::OnStatus(nsIURL* aURL, 
			    const nsString& aMsg)
{
  return NS_OK;
}

NS_METHOD
rdfStreamListener::OnStartBinding(nsIURL* aURL, 
				  const char *aContentType)
{
  return NS_OK;
}


NS_METHOD
rdfStreamListener::OnDataAvailable(nsIURL* aURL, 
				   nsIInputStream *pIStream, 
				   PRInt32 length)
{
  PRInt32 len;

  // PRLOG(("\n+++ rdfStreamListener::OnDataAvailable: URL: %p, %d bytes available...\n", aURL, length));

  do {
    const PRUint32 buffer_size = 80;
    char buffer[buffer_size];
    PRInt32 err;

    len = pIStream->Read(&err, buffer, 0, buffer_size);
    if (err == NS_OK) {
      (void) parseNextRDFXMLBlobInt(mFile, buffer, len);
    } // else XXX ?
  } while (len > 0);
  
  return NS_OK;
}

NS_METHOD
rdfStreamListener::OnStopBinding(nsIURL* aURL,
				 PRInt32 status,
				 const nsString& aMsg)
{
  nsresult result = NS_OK;

  switch( status ) {

  case NS_BINDING_SUCCEEDED:
    /* finishRDFParse( mFile ); */
    break;

  case NS_BINDING_FAILED:
  case NS_BINDING_ABORTED:
    //    abortRDFParse( mFile );
    // XXX status code?
    break;

  default:
    PR_ASSERT(PR_FALSE);
    result = NS_ERROR_ILLEGAL_VALUE;

  }

  return result;
}

/* 
 * beginReadingRDFFile is called whenever we need to read something of
 * the net (or local drive). The url of the file to be read is at
 * file->url.  As the bits are read in (and it can take the bits in
 * any sized chunks) it should call parseNextRDFBlobInt(file, nextBlock,
 * blobSize) when its done, it should call void finishRDFParse
 * (RDFFile f) to abort, it should call void abortRDFParse (RDFFile f)
 * [which will undo all that has been read from that file] 
 */

void
beginReadingRDFFile (RDFFile file)
{
   rdfStreamListener* pListener = new rdfStreamListener(file);
   pListener->AddRef(); // XXX is this evil?  Can't see any reason to use factories but...
   nsIURL* pURL = NULL;
   nsString url_address( file->url );
   nsresult r = NS_NewURL( &pURL, url_address );
   if( NS_OK != r ) {
       // XXX what to do?
   }

   r = pURL->Open(pListener);
   if( NS_OK != r ) {
       // XXX what to do?
   }

}
