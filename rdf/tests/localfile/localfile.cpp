/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

/*

  This is a test script that uses the <tt>nsStreamDataSource</tt>
  class to read a file from the local file system (using a "file:"
  URL) and write it back. The idea is that the RDF that gets written
  out should be equivalent (but not necessarily identical) to the RDF
  that was read in.

 */

#include <io.h>
#include "nsIContent.h"
#include "nsIEventQueueService.h"
#include "nsIInputStream.h"
#include "nsINetService.h"
#include "nsINetService.h"
#include "nsIOutputStream.h"
#include "nsIPostToServer.h"
#include "nsIRDFDataBase.h"
#include "nsIRDFDataSource.h"
#include "nsIRDFDocument.h"
#include "nsIRDFNode.h"
#include "nsIRDFService.h"
#include "nsIRDFXMLSource.h"
#include "nsIServiceManager.h"
#include "nsIStreamListener.h"
#include "nsIURL.h"
#include "nsLayoutCID.h" // for NS_NAMESPACEMANAGER_CID
#include "nsParserCIID.h"
#include "nsRDFCID.h"
#include "nsRDFCID.h"
#include "nsRepository.h"
#include "nsXPComCIID.h"
#include "plevent.h"
#include "plstr.h"

#ifdef XP_PC
#define NETLIB_DLL "netlib.dll"
#define PARSER_DLL "raptorhtmlpars.dll"
#define RDF_DLL    "rdf.dll"
#define LAYOUT_DLL "raptorhtml.dll"
#define XPCOM_DLL  "xpcom32.dll"
#endif

////////////////////////////////////////////////////////////////////////
// CIDs

// netlib
static NS_DEFINE_CID(kNetServiceCID,            NS_NETSERVICE_CID);

// rdf
static NS_DEFINE_CID(kRDFBookMarkDataSourceCID, NS_RDFBOOKMARKDATASOURCE_CID);
static NS_DEFINE_CID(kRDFHTMLDocumentCID,       NS_RDFHTMLDOCUMENT_CID);
static NS_DEFINE_CID(kRDFInMemoryDataSourceCID, NS_RDFINMEMORYDATASOURCE_CID);
static NS_DEFINE_CID(kRDFServiceCID,            NS_RDFSERVICE_CID);
static NS_DEFINE_CID(kRDFDataBaseCID,           NS_RDFDATABASE_CID);
static NS_DEFINE_CID(kRDFSimpleContentSinkCID,  NS_RDFSIMPLECONTENTSINK_CID);
static NS_DEFINE_CID(kRDFStreamDataSourceCID,   NS_RDFSTREAMDATASOURCE_CID);
static NS_DEFINE_CID(kRDFTreeDocumentCID,       NS_RDFTREEDOCUMENT_CID);

// parser
static NS_DEFINE_CID(kParserCID,                NS_PARSER_IID);
static NS_DEFINE_CID(kWellFormedDTDCID,         NS_WELLFORMEDDTD_CID);

// layout
static NS_DEFINE_CID(kNameSpaceManagerCID,      NS_NAMESPACEMANAGER_CID);

// xpcom
static NS_DEFINE_CID(kEventQueueServiceCID,     NS_EVENTQUEUESERVICE_CID);


////////////////////////////////////////////////////////////////////////
// IIDs

NS_DEFINE_IID(kIEventQueueServiceIID,  NS_IEVENTQUEUESERVICE_IID);
NS_DEFINE_IID(kIOutputStreamIID,       NS_IOUTPUTSTREAM_IID);
NS_DEFINE_IID(kIRDFDataSourceIID,      NS_IRDFDATASOURCE_IID);
NS_DEFINE_IID(kIRDFServiceIID,         NS_IRDFSERVICE_IID);
NS_DEFINE_IID(kIRDFXMLSourceIID,       NS_IRDFXMLSOURCE_IID);

static nsresult
SetupRegistry(void)
{
    // netlib
    nsRepository::RegisterFactory(kNetServiceCID,            NETLIB_DLL, PR_FALSE, PR_FALSE);

    // rdf
    nsRepository::RegisterFactory(kRDFBookMarkDataSourceCID, RDF_DLL,    PR_FALSE, PR_FALSE);
    nsRepository::RegisterFactory(kRDFHTMLDocumentCID,       RDF_DLL,    PR_FALSE, PR_FALSE);
    nsRepository::RegisterFactory(kRDFInMemoryDataSourceCID, RDF_DLL,    PR_FALSE, PR_FALSE);
    nsRepository::RegisterFactory(kRDFServiceCID,            RDF_DLL,    PR_FALSE, PR_FALSE);
    nsRepository::RegisterFactory(kRDFSimpleContentSinkCID,  RDF_DLL,    PR_FALSE, PR_FALSE);
    nsRepository::RegisterFactory(kRDFDataBaseCID,           RDF_DLL,    PR_FALSE, PR_FALSE);
    nsRepository::RegisterFactory(kRDFStreamDataSourceCID,   RDF_DLL,    PR_FALSE, PR_FALSE);
    nsRepository::RegisterFactory(kRDFTreeDocumentCID,       RDF_DLL,    PR_FALSE, PR_FALSE);

    // parser
    nsRepository::RegisterFactory(kParserCID,                PARSER_DLL, PR_FALSE, PR_FALSE);
    nsRepository::RegisterFactory(kWellFormedDTDCID,         PARSER_DLL, PR_FALSE, PR_FALSE);

    // layout
    nsRepository::RegisterFactory(kNameSpaceManagerCID,      LAYOUT_DLL, PR_FALSE, PR_FALSE);

    // xpcom
    nsRepository::RegisterFactory(kEventQueueServiceCID,     XPCOM_DLL,  PR_FALSE, PR_FALSE);

    return NS_OK;
}


////////////////////////////////////////////////////////////////////////

int
main(int argc, char** argv)
{
    nsresult rv;

    if (argc < 2) {
        fprintf(stderr, "usage: %s [url]\n", argv[0]);
        return 1;
    }

    SetupRegistry();

    nsIEventQueueService* theEventQueueService = nsnull;
    PLEventQueue* mainQueue      = nsnull;
    nsIRDFService* theRDFService = nsnull;
    nsIRDFDataSource* ds         = nsnull;
    nsIRDFResource* theHomePage  = nsnull;
    nsIRDFResource* NC_title     = nsnull;
    nsIRDFLiteral* theTitle      = nsnull;
    PRInt32 i;

    // Get netlib off the floor...
    if (NS_FAILED(rv = nsServiceManager::GetService(kEventQueueServiceCID,
                                                    kIEventQueueServiceIID,
                                                    (nsISupports**) &theEventQueueService)))
        goto done;

    if (NS_FAILED(rv = theEventQueueService->CreateThreadEventQueue()))
        goto done;

    if (NS_FAILED(rv = theEventQueueService->GetThreadEventQueue(PR_GetCurrentThread(),
                                                                 &mainQueue)))
        goto done;

    // Create a stream data source and initialize it on argv[1], which
    // is hopefully a "file:" URL. (Actually, we can do _any_ kind of
    // URL, but only a "file:" URL will be written back to disk.)
    if (NS_FAILED(rv = nsRepository::CreateInstance(kRDFStreamDataSourceCID,
                                                    nsnull,
                                                    kIRDFDataSourceIID,
                                                    (void**) &ds)))
        goto done;

    if (NS_FAILED(rv = ds->Init(argv[1])))
        goto done;

    // XXX This is really gross. I need to figure out the right way to do it...
    for (i = 0; i < 1000000; ++i) {
        PLEvent* event = PL_GetEvent(mainQueue);
        PL_HandleEvent(event);
    }

    // Now take the graph and munge it a little bit...
    if (NS_FAILED(rv = nsServiceManager::GetService(kRDFServiceCID,
                                                    kIRDFServiceIID,
                                                    (nsISupports**) &theRDFService)))
        goto done;

    if (NS_FAILED(rv = theRDFService->GetResource("http://home.netscape.com", &theHomePage)))
        goto done;

    if (NS_FAILED(rv = theRDFService->GetResource("http://rdf.nescape.com/NC#title", &NC_title)))
        goto done;

    if (NS_FAILED(rv = theRDFService->GetLiteral(nsAutoString("Netscape's Home Page"), &theTitle)))
        goto done;

    if (NS_FAILED(rv = ds->Assert(theHomePage, NC_title, theTitle, PR_TRUE)))
        goto done;

    // And finally, write it back out.
    if (NS_FAILED(rv = ds->Flush()))
        goto done;

done:
    NS_IF_RELEASE(theTitle);
    NS_IF_RELEASE(NC_title);
    NS_IF_RELEASE(theHomePage);
    NS_IF_RELEASE(ds);
    if (theRDFService) {
        nsServiceManager::ReleaseService(kRDFServiceCID, theRDFService);
        theRDFService = nsnull;
    }
    if (theEventQueueService) {
        nsServiceManager::ReleaseService(kEventQueueServiceCID, theEventQueueService);
        theEventQueueService = nsnull;
    }
    return (NS_FAILED(rv) ? 1 : 0);
}






