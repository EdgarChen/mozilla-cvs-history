/*    -*- Mode: HTML; tab-width: 8; indent-tabs-mode: nil -*-
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
<!--  to hide script contents from old browsers



function go(msg)
{
	return(checkData());
}



function checkData()
{
	return(true);
}



function loadData()
{
	if (parent && parent.controls && parent.controls.generateControls)	{
		parent.controls.generateControls();
		}
}



function saveData()
{
}



// end hiding contents from old browsers  -->
