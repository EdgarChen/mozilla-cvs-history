/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
#ifndef nsIDocumentLoaderObserver_h___
#define nsIDocumentLoaderObserver_h___

#include "nsISupports.h"

// Forward declarations... 
class nsIURI;
class nsIContentViewer;
class nsIDocumentLoader;
class nsIChannel;

/* f6b4f550-317c-11d2-bd8c-00805f8ae3f4 */
#define NS_IDOCUMENT_LOADER_OBSERVER_IID   \
{ 0xf6b4f550, 0x317c, 0x11d2, \
  {0xbd, 0x8c, 0x00, 0x80, 0x5f, 0x8a, 0xe3, 0xf4} }

/*
 *
 *
 */
class nsIDocumentLoaderObserver : public nsISupports 
{
public:
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_IDOCUMENT_LOADER_OBSERVER_IID)

  /**
   * Notify the observer that a new document will be loaded.  
   *
   * This notification occurs before any DNS resolution occurs, or
   * a connection is established with the server...
   */
  NS_IMETHOD OnStartDocumentLoad(nsIDocumentLoader* loader, nsIURI* aURL, const char* aCommand) = 0;

  /**
   * Notify the observer that a document has been completely loaded.
   */
  NS_IMETHOD OnEndDocumentLoad(nsIDocumentLoader* loader, nsIChannel* channel, nsresult aStatus) = 0;

  /**
   * Notify the observer that the specified nsIURI has just started to load.
   *
   * This notification occurs after DNS resolution, and a connection to the
   * server has been established.
   */
  NS_IMETHOD OnStartURLLoad(nsIDocumentLoader* loader, nsIChannel* channel) = 0;
  
  /**
   * Notify the observer that progress has occurred in the loading of the 
   * specified URL...
   */
  NS_IMETHOD OnProgressURLLoad(nsIDocumentLoader* loader,
                               nsIChannel* channel, PRUint32 aProgress, 
                               PRUint32 aProgressMax) = 0;

  /**
   * Notify the observer that status text is available regarding the URL
   * being loaded...
   */
  NS_IMETHOD OnStatusURLLoad(nsIDocumentLoader* loader, nsIChannel* channel, nsString& aMsg) = 0;

  /**
   * Notify the observer that the specified nsIURI has finished loading.
   */
  NS_IMETHOD OnEndURLLoad(nsIDocumentLoader* loader, nsIChannel* channel, nsresult aStatus) = 0;

  /**
   * Notify the observer that some content of unknown type has been
   * encountered...
   */
  NS_IMETHOD HandleUnknownContentType( nsIDocumentLoader* loader,
                                       nsIChannel* channel,
                                       const char *aContentType,
                                       const char *aCommand ) = 0;
};


#endif /* nsIDocumentLoaderObserver_h___ */
