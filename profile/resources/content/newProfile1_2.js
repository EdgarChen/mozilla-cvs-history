/* 
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
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 1998-1999 Netscape Communications Corporation. All
 * Rights Reserved.
 * 
 * Contributor(s): 
 *   Ben Goodger (03/11/99)
 */

var bundle = srGetStrBundle("chrome://profile/locale/newProfile1_2.properties");

// the getting procedure is unique to each page, since each page can different
// types of elements (not necessarily form elements). So each page must provide
// its own GetFields function
function GetFields()
{
  var profName = document.getElementById("ProfileName").value;
  var profDir  = document.getElementById("ProfileDir");
  var profDirContent = profDir.hasChildNodes() ? profDir.firstChild.nodeValue : "";
  var profDirRootFolder = profDir.getAttribute("rootFolder");
  var rv = { 
    ProfileName: { id: "ProfileName", value: profName },
    ProfileDir:  { id: "ProfileDir",  value: profDirRootFolder }
  }
  return rv; 
}

// the setting procedure is unique to each page, and as such each page
// must provide its own SetFields function
function SetFields( aElement, aValue, aDataObject )
{
  element = document.getElementById( aElement );
  if(element.id == "ProfileDir" && aValue != "") {
    chooseProfileFolder( aValue );
  }
  else if(element.id == "ProfileName")
    element.value = aValue;
}  

// check to see if some user specified profile folder exists, otherwise use
// default. 
function initFields()
{
  var displayField = document.getElementById( "ProfileDir" );
  if ( !displayField.value || !displayField.rootFolder )
    setDisplayToDefaultFolder();
}

// function createProfileWizard.js::chooseProfileFolder();
// invoke a folder selection dialog for choosing the directory of profile storage.
function chooseProfileFolder( aRootFolder )
{
  if( !aRootFolder ) {
	  try {
      var fileSpec = Components.classes["component://netscape/filespecwithui"].createInstance();
      if(fileSpec) {
        fileSpec = fileSpec.QueryInterface(Components.interfaces.nsIFileSpecWithUI);
        fileSpec.URLString = fileSpec.chooseDirectory( bundle.GetStringFromName("chooseFolder"));    
        aRootFolder = fileSpec.nativePath;
      } 
      else
        aRootFolder = null;
    }
    catch(e) {
      aRootFolder = null;
    }
  }
  if( aRootFolder ) {
    var folderText = document.getElementById("ProfileDir");
    folderText.setAttribute( "rootFolder", aRootFolder );
    if ( aRootFolder != top.profile.defaultProfileParentDir.nativePath )
      document.getElementById( "useDefault" ).removeAttribute("disabled");
    updateProfileName();
  }
}

function clearFolderDisplay()
{
  var folderText = document.getElementById("ProfileDir");
  if ( folderText.hasChildNodes() ) {
    while ( folderText.hasChildNodes() )
      folderText.removeChild( folderText.firstChild );
  }
}

function updateProfileName()
{
  var profileName = document.getElementById( "ProfileName" );
  var folderDisplayElement = document.getElementById( "ProfileDir" );
  var rootFolder = folderDisplayElement.getAttribute( "rootFolder" );
  try {
    var fileSpec = Components.classes["component://netscape/filespec"].createInstance();
    if ( fileSpec )
      fileSpec = fileSpec.QueryInterface( Components.interfaces.nsIFileSpec );
    if ( fileSpec )
      fileSpec.nativePath = rootFolder;
    fileSpec.appendRelativeUnixPath( profileName.value );
    
    clearFolderDisplay();
    var value = document.createTextNode( fileSpec.nativePath );
    folderDisplayElement.appendChild( value );
  }
  catch(e) {
  }
}

function setDisplayToDefaultFolder()
{
  var profileName = document.getElementById( "ProfileName" );
  var profileDisplay = document.getElementById( "ProfileDir" );
  
  var fileSpec;
  try {
    fileSpec = top.profile.defaultProfileParentDir; 
    if ( fileSpec )
      fileSpec = fileSpec.QueryInterface( Components.interfaces.nsIFileSpec );
    if ( fileSpec )
      profileDisplay.setAttribute("rootFolder", fileSpec.nativePath );
  }
  catch(e) {
  }
  
  document.getElementById("useDefault").setAttribute("disabled", "true");
  
  // reset the display field
  updateProfileName();
}