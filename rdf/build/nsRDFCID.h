/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

/*

  XPCOM Class IDs for RDF objects that can be constructed via the RDF
  factory.

 */

#ifndef nsRDFCID_h__
#define nsRDFCID_h__

// {0F78DA56-8321-11d2-8EAC-00805F29F370}
#define NS_RDFDEFAULTRESOURCE_CID \
{ 0xf78da56, 0x8321, 0x11d2, { 0x8e, 0xac, 0x0, 0x80, 0x5f, 0x29, 0xf3, 0x70 } }

// {BFD05264-834C-11d2-8EAC-00805F29F370}
#define NS_RDFSERVICE_CID \
{ 0xbfd05264, 0x834c, 0x11d2, { 0x8e, 0xac, 0x0, 0x80, 0x5f, 0x29, 0xf3, 0x70 } }

// {BFD0526D-834C-11d2-8EAC-00805F29F370}
#define NS_RDFINMEMORYDATASOURCE_CID \
{ 0xbfd0526d, 0x834c, 0x11d2, { 0x8e, 0xac, 0x0, 0x80, 0x5f, 0x29, 0xf3, 0x70 } }

// {E638D760-8687-11d2-B530-000000000001}
#define NS_RDFFILESYSTEMDATASOURCE_CID \
{ 0xe638d760, 0x8687, 0x11d2, { 0xb5, 0x30, 0x0, 0x0, 0x0, 0x0, 0x0, 0x01 } }

// {6bd1d807-1c67-11d3-9820-ed1b357eb3c4}
#define NS_RDFSEARCHDATASOURCE_CID \
{ 0x6bd1d807, 0x1c67, 0x11d3, { 0x98, 0x20, 0xed, 0x1b, 0x35, 0x7e, 0xb3, 0xc4 } }

// {E638D760-8687-11d2-B530-000000000002}
#define NS_RDFFINDDATASOURCE_CID \
{ 0xe638d760, 0x8687, 0x11d2, { 0xb5, 0x30, 0x0, 0x0, 0x0, 0x0, 0x0, 0x02 } }

// {dc2fb186-f7ff-11d2-9820-f65ea652ae3c}
#define NS_RDFRELATEDLINKSDATASOURCE_CID \
{ 0xdc2fb186, 0xf7ff, 0x11d2, { 0x98, 0x20, 0xf6, 0x5e, 0xa6, 0x52, 0xae, 0x3c } }

// {E638D761-8687-11d2-B530-000000000000}
#define NS_RDFCOMPOSITEDATASOURCE_CID \
{ 0xe638d761, 0x8687, 0x11d2, { 0xb5, 0x30, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 } }

// {7BAF62E0-8E61-11d2-8EB1-00805F29F370}
#define NS_RDFXMLDATASOURCE_CID \
{ 0x7baf62e0, 0x8e61, 0x11d2, { 0x8e, 0xb1, 0x0, 0x80, 0x5f, 0x29, 0xf3, 0x70 } }

// {0958B101-9ADA-11d2-8EBC-00805F29F370}
#define NS_RDFCONTENTSINK_CID \
{ 0x958b101, 0x9ada, 0x11d2, { 0x8e, 0xbc, 0x0, 0x80, 0x5f, 0x29, 0xf3, 0x70 } }

// {1EAAFD60-D596-11d2-80BE-006097B76B8E}
#define NS_RDFHISTORYDATASOURCE_CID \
{ 0x1eaafd60, 0xd596, 0x11d2, { 0x80, 0xbe, 0x0, 0x60, 0x97, 0xb7, 0x6b, 0x8e } }

// {D4214E92-FB94-11d2-BDD8-00104BDE6048}
#define NS_RDFCONTAINERUTILS_CID \
{ 0xd4214e92, 0xfb94, 0x11d2, { 0xbd, 0xd8, 0x0, 0x10, 0x4b, 0xde, 0x60, 0x48 } }

// {D4214E93-FB94-11d2-BDD8-00104BDE6048}
#define NS_RDFCONTAINER_CID \
{ 0xd4214e93, 0xfb94, 0x11d2, { 0xbd, 0xd8, 0x0, 0x10, 0x4b, 0xde, 0x60, 0x48 } }

// {0032d852-1dd2-11b2-95f7-e0a1910ed2da}
#define NS_RDFXMLSERIALIZER_CID \
{ 0x0032d852, 0x1dd2, 0x11b2, { 0x95, 0xf7, 0xe0, 0xa1, 0x91, 0x0e, 0xd2, 0xda } }

// {a4048e94-1dd1-11b2-a676-8a06c086cc7d}
#define NS_RDFXMLPARSER_CID \
{ 0xa4048e94, 0x1dd1, 0x11b2, { 0xa6, 0x76, 0x8a, 0x06, 0xc0, 0x86, 0xcc, 0x7d } }

#endif // nsRDFCID_h__
