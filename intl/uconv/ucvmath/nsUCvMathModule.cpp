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
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *   Roger B. Sidje <rbs@maths.uq.edu.au>
 *
 * This Original Code has been modified by Roger B. Sidje.
 * Modifications made by Roger B. Sidje described herein are
 * Copyright (C) 2000 The University Of Queensland.
 *
 * Modifications to Mozilla code or documentation
 * identified per MPL Section 3.3
 *
 * Date            Modified by     Description of modification
 * 08/March/2000   RBS.            Support for Mathematical fonts.
 */

#define NS_IMPL_IDS

#include "pratom.h"
#include "nspr.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsIFactory.h"
#include "nsIRegistry.h"
#include "nsIGenericFactory.h"
#include "nsIServiceManager.h"
#include "nsICharsetConverterManager.h"
#include "nsIModule.h"
#include "nsUCvMathCID.h"
#include "nsUCvMathDll.h"

#include "nsUnicodeToTeXCMSYttf.h"
#include "nsUnicodeToTeXCMSYt1.h"
#include "nsUnicodeToTeXCMEXttf.h"
#include "nsUnicodeToTeXCMEXt1.h"
#include "nsUnicodeToMathematica1.h"
#include "nsUnicodeToMathematica2.h"
#include "nsUnicodeToMathematica3.h"
#include "nsUnicodeToMathematica4.h"
#include "nsUnicodeToMathematica5.h"
#include "nsUnicodeToMTExtra.h"

//----------------------------------------------------------------------------
// Global functions and data [declaration]

static NS_DEFINE_CID(kComponentManagerCID, NS_COMPONENTMANAGER_CID);

#define DECODER_NAME_BASE "Unicode Decoder-"
#define ENCODER_NAME_BASE "Unicode Encoder-"

PRInt32 g_InstanceCount = 0;
PRInt32 g_LockCount = 0;

NS_IMPL_NSUCONVERTERREGSELF

NS_UCONV_REG_UNREG(nsUnicodeToTeXCMSYttf, "Unicode", "x-ttf-cmsy",  NS_UNICODETOTEXCMSYTTF_CID);
NS_UCONV_REG_UNREG(nsUnicodeToTeXCMSYt1, "Unicode", "x-t1-cmsy",  NS_UNICODETOTEXCMSYT1_CID);
NS_UCONV_REG_UNREG(nsUnicodeToTeXCMEXttf, "Unicode", "x-ttf-cmex",  NS_UNICODETOTEXCMEXTTF_CID);
NS_UCONV_REG_UNREG(nsUnicodeToTeXCMEXt1, "Unicode", "x-t1-cmex",  NS_UNICODETOTEXCMEXT1_CID);
NS_UCONV_REG_UNREG(nsUnicodeToMathematica1, "Unicode", "x-mathematica1",  NS_UNICODETOMATHEMATICA1_CID);
NS_UCONV_REG_UNREG(nsUnicodeToMathematica2, "Unicode", "x-mathematica2",  NS_UNICODETOMATHEMATICA2_CID);
NS_UCONV_REG_UNREG(nsUnicodeToMathematica3, "Unicode", "x-mathematica3",  NS_UNICODETOMATHEMATICA3_CID);
NS_UCONV_REG_UNREG(nsUnicodeToMathematica4, "Unicode", "x-mathematica4",  NS_UNICODETOMATHEMATICA4_CID);
NS_UCONV_REG_UNREG(nsUnicodeToMathematica5, "Unicode", "x-mathematica5",  NS_UNICODETOMATHEMATICA5_CID);
NS_UCONV_REG_UNREG(nsUnicodeToMTExtra, "Unicode", "x-mtextra",  NS_UNICODETOMTEXTRA_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR(nsUnicodeToTeXCMSYttf);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUnicodeToTeXCMSYt1);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUnicodeToTeXCMEXttf);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUnicodeToTeXCMEXt1);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUnicodeToMathematica1);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUnicodeToMathematica2);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUnicodeToMathematica3);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUnicodeToMathematica4);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUnicodeToMathematica5);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUnicodeToMTExtra);

static nsModuleComponentInfo components[] = 
{
  { 
    ENCODER_NAME_BASE "x-ttf-cmsy" , NS_UNICODETOTEXCMSYTTF_CID, 
    NS_UNICODEENCODER_CONTRACTID_BASE "x-ttf-cmsy",
    nsUnicodeToTeXCMSYttfConstructor, 
    nsUnicodeToTeXCMSYttfRegSelf, nsUnicodeToTeXCMSYttfUnRegSelf
  },
  { 
    ENCODER_NAME_BASE "x-t1-cmsy" , NS_UNICODETOTEXCMSYT1_CID, 
    NS_UNICODEENCODER_CONTRACTID_BASE "x-t1-cmsy",
    nsUnicodeToTeXCMSYt1Constructor, 
    nsUnicodeToTeXCMSYt1RegSelf, nsUnicodeToTeXCMSYt1UnRegSelf
  },
  { 
    ENCODER_NAME_BASE "x-ttf-cmex" , NS_UNICODETOTEXCMEXTTF_CID, 
    NS_UNICODEENCODER_CONTRACTID_BASE "x-ttf-cmex",
    nsUnicodeToTeXCMEXttfConstructor, 
    nsUnicodeToTeXCMEXttfRegSelf, nsUnicodeToTeXCMEXttfUnRegSelf
  },
  { 
    ENCODER_NAME_BASE "x-t1-cmex" , NS_UNICODETOTEXCMEXT1_CID, 
    NS_UNICODEENCODER_CONTRACTID_BASE "x-t1-cmex",
    nsUnicodeToTeXCMEXt1Constructor, 
    nsUnicodeToTeXCMEXt1RegSelf, nsUnicodeToTeXCMEXt1UnRegSelf
  },
  { 
    ENCODER_NAME_BASE "x-mathematica1" , NS_UNICODETOMATHEMATICA1_CID, 
    NS_UNICODEENCODER_CONTRACTID_BASE "x-mathematica1",
    nsUnicodeToMathematica1Constructor, 
    nsUnicodeToMathematica1RegSelf, nsUnicodeToMathematica1UnRegSelf
  },
  { 
    ENCODER_NAME_BASE "x-mathematica2" , NS_UNICODETOMATHEMATICA2_CID, 
    NS_UNICODEENCODER_CONTRACTID_BASE "x-mathematica2",
    nsUnicodeToMathematica2Constructor, 
    nsUnicodeToMathematica2RegSelf, nsUnicodeToMathematica2UnRegSelf
  },
  { 
    ENCODER_NAME_BASE "x-mathematica3" , NS_UNICODETOMATHEMATICA3_CID, 
    NS_UNICODEENCODER_CONTRACTID_BASE "x-mathematica3",
    nsUnicodeToMathematica3Constructor, 
    nsUnicodeToMathematica3RegSelf, nsUnicodeToMathematica3UnRegSelf
  },
  { 
    ENCODER_NAME_BASE "x-mathematica4" , NS_UNICODETOMATHEMATICA4_CID, 
    NS_UNICODEENCODER_CONTRACTID_BASE "x-mathematica4",
    nsUnicodeToMathematica4Constructor, 
    nsUnicodeToMathematica4RegSelf, nsUnicodeToMathematica4UnRegSelf
  },
  { 
    ENCODER_NAME_BASE "x-mathematica5" , NS_UNICODETOMATHEMATICA5_CID, 
    NS_UNICODEENCODER_CONTRACTID_BASE "x-mathematica5",
    nsUnicodeToMathematica5Constructor, 
    nsUnicodeToMathematica5RegSelf, nsUnicodeToMathematica5UnRegSelf
  },
  { 
    ENCODER_NAME_BASE "x-mtextra" , NS_UNICODETOMTEXTRA_CID, 
    NS_UNICODEENCODER_CONTRACTID_BASE "x-mtextra",
    nsUnicodeToMTExtraConstructor, 
    nsUnicodeToMTExtraRegSelf, nsUnicodeToMTExtraUnRegSelf
  }
};

NS_IMPL_NSGETMODULE("nsUCvMathModule", components);

