/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
   nffbp.java (FontBrokerDisplayer.java)

   Interface definition of FontBrokerDisplayer
   dp <dp@netscape.com>
*/


// ----------------------------------------------------------------------
// Notes:
// Used by			: FontDisplayer
// Implemented by	: FontBroker
//	- This is one of the interfaces implemented by the FontBroker
//		b)	FontBrokerDisplayer interface
//			FontDisplayers will use this interface to register themselves
//			with the FontBroker.
// ---------------------------------------------------------------------- 

package netscape.fonts;

import netscape.jmc.*;

public interface nffbp {
	public int RegisterFontDisplayer(nffp displayer);

	//
	// Facility to dynamically add, upgrade a FontDisplayer should be available
	//

	public int CreateFontDisplayerFromDLM(ConstCString dlm_name);
	public int ScanForFontDisplayers(ConstCString dirname);

	//
	// Broker could do caching of rf. Displayer tells the broker when an
	// rf is being destroyed. Displayer Implementors, have your finalize
	// method call this.
	//
	public void RfDone(nfrf rf);
}
