/* -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil -*-
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

//the file that includes this must also include settings.js!

function loadData()
{
	
	var acctSetupFile		=	parent.parent.globals.getAcctSetupFilename(parent.parent.globals);
	
 	var ttyFlagValue 		= 	"";
 	var asFormName			=	"askcc";
 
 	modeSectionName			=	"Mode Selection"
 	newSectionName			=	"New Acct Mode"	
 	existingSectionName		=	"Existing Acct Mode"
 		
	//set the checkbox flag
	setCardTypes(asFormName, parent.parent.globals.GetNameValuePair(acctSetupFile,newSectionName,"CardTypes"));			

	if (parent.controls.generateControls)	parent.controls.generateControls();

}//function loadData()


function checkData()
{
	return true;
}


function saveData()
{
	// save globals here

	var acctSetupFile 		=	parent.parent.globals.getAcctSetupFilename(parent.parent.globals);
	var modeSectionName		= 	"Mode Selection";
	var existingSectionName	=	"Existing Acct Mode";
	var newSectionName		=	"New Acct Mode";
	var	asFormName			=	"askcc";

	if (acctSetupFile != null && acctSetupFile != "")	{
	
		var cardTypes = getCardTypes(asFormName);
	 parent.parent.globals.SetNameValuePair(acctSetupFile, newSectionName, "CardTypes", cardTypes);	
	}  // if (acctSetupFile != null && acctSetupFile != "")	
}


 //	For easy reference, this is the list of flags that ACCTSET.INI currently supports
 //	ForceExisting
 // ExistingSRFile
 //	ForceNew
 //	IntlMode
 //	ShowNewPathInfo
 //	AskSaveAcctInfo
 //	SavePasswords
 //	AskPersonalInfo
 //	//AskSurvey		//defunct
 //	//AskShareInfo	//defunct
 //	AskBillingInfo
 //	RegServer
 //	CardTypes
 //	ShowExistingPathInfo
 //	RegPodURL
 //	ShowIntro2
 // ShowPhones
 //	AskName
 //	AskLogin
 //	AskTTY
 //	AskEmail
 //	AskPhone
 //	AskDNS
 //	AskHosts
 // AskPublishing
 //	AskRegister
 //	AskLDAP
 //	AskIMAP



// end hiding contents from old browsers  -->
