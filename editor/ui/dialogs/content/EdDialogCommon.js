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
 *   Pete Collins
 *   Brian King
 */
/*
  if we ever need to use a different string bundle, use srGetStrBundle
  by including
  <script language="javascript" src="chrome://global/content/strres.js"/>
  e.g.:
  var bundle = srGetStrBundle("chrome://global/locale/filepicker.properties");
*/

// Each editor window must include this file
// Variables  shared by all dialogs:
var editorShell;
// Bummer! Can't get at enums from nsIDocumentEncoder.h
var gOutputSelectionOnly = 1;
var gOutputFormatted     = 2;
var gOutputNoDoctype     = 4;
var gOutputBodyOnly      = 8;
var gOutputPreformatted  = 16;
var gOutputWrap          = 32;
var gOutputFormatFlowed  = 64;
var gOutputAbsoluteLinks = 128;
var gOutputEncodeEntities = 256;
var gStringBundle;
var gValidationError = false;

// Use for 'defaultIndex' param in InitPixelOrPercentMenulist
var gPixel = 0;
var gPercent = 1;

var maxPixels  = 10000;
// For dialogs that expand in size. Default is smaller size see "onMoreFewer()" below
var SeeMore = false;

// A XUL element with id="location" for managing
// dialog location relative to parent window
var gLocation;

// The element being edited - so AdvancedEdit can have access to it
var globalElement;

function InitEditorShell()
{
    // get the editor shell from the parent window

  editorShell = window.opener.editorShell;
  if (editorShell) {
    editorShell = editorShell.QueryInterface(Components.interfaces.nsIEditorShell);
  }
  if (!editorShell) {
    dump("EditorShell not found!!!\n");
    window.close();
    return false;
  }

  // Save as a property of the window so it can be used by child dialogs

  window.editorShell = editorShell;

  return true;
}

function StringExists(string)
{
  if (string != null && string != "undefined" && string.length > 0)
    return true;

  return false;
}

/* Validate contents of an input field 
 *
 *  inputWidget    The 'textbox' XUL element for text input of the attribute's value
 *  listWidget     The 'menulist' XUL element for choosing "pixel" or "percent"
 *                  May be null when no pixel/percent is used.
 *  minVal         minimum allowed for input widget's value
 *  maxVal         maximum allowed for input widget's value
 *                 (when "listWidget" is used, maxVal is used for "pixel" maximum,
 *                  100% is assumed if "percent" is the user's choice)
 *  element        The DOM element that we set the attribute on. May be null.
 *  attName        Name of the attribute to set.  May be null or ignored if "element" is null
 *  mustHaveValue  If true, error dialog is displayed if "value" is empty string
 *
 *  This calls "ValidateNumberRange()", which puts up an error dialog to inform the user. 
 *    If error, we also: 
 *      Shift focus and select contents of the inputWidget,
 *      Switch to appropriate panel of tabbed dialog if user implements "SwitchToValidate()",
 *      and/or will expand the dialog to full size if "More / Fewer" feature is implemented
 *
 *  Returns the "value" as a string, or "" if error or input contents are empty
 *  The global "gValidationError" variable is set true if error was found
 */
function ValidateNumber(inputWidget, listWidget, minVal, maxVal, element, attName, mustHaveValue, mustShowMoreSection)
{
  if (!inputWidget)
  {
    gValidationError = true;
    return "";
  }

  // Global error return value
  gValidationError = false;
  var maxLimit = maxVal;
  var isPercent = false;

  var numString = inputWidget.value.trimString();
  if (numString)
  {
    if (listWidget)
      isPercent = (listWidget.selectedIndex == 1);
    if (isPercent)
      maxLimit = 100;

    // This method puts up the error message
    numString = ValidateNumberRange(numString, minVal, maxLimit, mustHaveValue);
    if(!numString)
    {
      // Switch to appropriate panel for error reporting
      SwitchToValidatePanel();

      // or expand dialog for users of "More / Fewer" button
      if ("dialog" in window && dialog && 
           "MoreSection" in dialog && dialog.MoreSection)
      {
        if ( !SeeMore )
          onMoreFewer();
      }

      // Error - shift to offending input widget
      SetTextboxFocus(inputWidget);
      gValidationError = true;
    }
    else
    {
      if (isPercent)
        numString += "%";
      if (element)
        element.setAttribute(attName, numString);
    }
  } else if (element) {
    element.removeAttribute(attName);
  }
  return numString;
}

/* Validate contents of an input field 
 *
 *  value          number to validate
 *  minVal         minimum allowed for input widget's value
 *  maxVal         maximum allowed for input widget's value
 *                 (when "listWidget" is used, maxVal is used for "pixel" maximum,
 *                  100% is assumed if "percent" is the user's choice)
 *  mustHaveValue  If true, error dialog is displayed if "value" is empty string
 *
 *  If inputWidget's value is outside of range, or is empty when "mustHaveValue" = true,
 *      an error dialog is popuped up to inform the user. The focus is shifted
 *      to the inputWidget.
 *
 *  Returns the "value" as a string, or "" if error or input contents are empty
 *  The global "gValidationError" variable is set true if error was found
 */
function ValidateNumberRange(value, minValue, maxValue, mustHaveValue)
{
  // Initialize global error flag
  gValidationError = false;
  value = TrimString(String(value));

  // We don't show error for empty string unless caller wants to
  if (!value && !mustHaveValue)
    return "";

  var numberStr = "";

  if (value.length > 0)
  {
    // Extract just numeric characters
    var number = Number(value.replace(/\D+/g, ""));
    if (number >= minValue && number <= maxValue )
    {
      // Return string version of the number
      return String(number);
    }
    numberStr = String(number);
  }

  var message = "";

  if (numberStr.length > 0)
  {
    // We have a number from user outside of allowed range
    message = editorShell.GetString( "ValidateRangeMsg");
    message = message.replace(/%n%/, numberStr);
    message += "\n ";
  }
  message += editorShell.GetString( "ValidateNumberMsg");

  // Replace variable placeholders in message with number values
  message = message.replace(/%min%/, minValue).replace(/%max%/, maxValue);
  ShowInputErrorMessage(message);

  // Return an empty string to indicate error
  gValidationError = true;
  return "";
}

function SetTextboxFocusById(id)
{
  SetTextboxFocus(document.getElementById(id));
}

function SetTextboxFocus(textbox)
{
  if (textbox)
  {
    // Select entire contents
    if (textbox.value.length > 0)
      textbox.select();

    textbox.focus();
  }
}

function ShowInputErrorMessage(message)
{
  editorShell.AlertWithTitle(GetString("InputError"), message);
  window.focus();
}

function GetString(name)
{
  if (editorShell)
  {
    return editorShell.GetString(name);
  }
  else
  {
    // Non-editors (like prefs) may use these methods
    if (!gStringBundle)
    {
      gStringBundle = srGetStrBundle("chrome://editor/locale/editor.properties");
      if (!gStringBundle)
        return null;
    }
    return gStringBundle.GetStringFromName(name);
  }
  return null;
}

function TrimStringLeft(string)
{
  if(!string) return "";
  return string.replace(/^\s+/, "");
}

function TrimStringRight(string)
{
  if (!string) return "";
  return string.replace(/\s+$/, '');
}

// Remove whitespace from both ends of a string
function TrimString(string)
{
  if (!string) return "";
  return string.replace(/(^\s+)|(\s+$)/g, '')
}

String.prototype.trimString = function() {
  return this.replace(/(^\s+)|(\s+$)/g, '')
}

function IsWhitespace(string)
{
  return /^\s/.test(string);
}

function TruncateStringAtWordEnd(string, maxLength, addEllipses)
{
    // Return empty if string is null, undefined, or the empty string
    if (!string)
      return "";

    // We assume they probably don't want whitespace at the beginning
    string = string.replace(/^\s+/, '');
    if (string.length <= maxLength)
      return string;

    // We need to truncate the string to maxLength or fewer chars
    if (addEllipses)
      maxLength -= 3;
    string = string.replace(RegExp("(.{0," + maxLength + "})\\s.*"), "$1")

    if (string.length > maxLength)
      string = string.slice(0, maxLength);

    if (addEllipses)
      string += "...";
    return string;
}

// Replace all whitespace characters with supplied character
// E.g.: Use charReplace = " ", to "unwrap" the string by removing line-end chars
//       Use charReplace = "_" when you don't want spaces (like in a URL)
function ReplaceWhitespace(string, charReplace) {
  return string.replace(/(^\s+)|(\s+$)/g,'').replace(/\s+/g,charReplace)
}

// Replace whitespace with "_" and allow only HTML CDATA
//   characters: "a"-"z","A"-"Z","0"-"9", "_", ":", "-", and "."
function ConvertToCDATAString(string)
{
  return string.replace(/\s+/g,"_").replace(/[^a-zA-Z0-9_\.\-\:]+/g,'');
}

// this function takes an elementID and a flag
// if the element can be found by ID, then it is either enabled (by removing "disabled" attr)
// or disabled (setAttribute) as specified in the "doEnable" parameter
function SetElementEnabledById( elementID, doEnable )
{
  var element = document.getElementById(elementID);
  if ( element )
  {
    if ( doEnable )
    {
      element.removeAttribute( "disabled" );
    }
    else
    {
      element.setAttribute( "disabled", "true" );
    }
  }
  else
  {
    dump("Element "+elementID+" not found in SetElementEnabledById\n");
  }
}

// This function relies on css classes for enabling and disabling
// This function takes an ID for a label and a flag
// if an element can be found by its ID, then it is either enabled or disabled
// The class is set to either "enabled" or "disabled" depending on the flag passed in.
// This function relies on css having a special appearance for these two classes.

function SetClassEnabledById( elementID, doEnable )
{
  element = document.getElementById(elementID);
  if ( element )
  {
    if ( doEnable )
    {
     element.setAttribute( "class", "enabled" );
    }
    else
    {
     element.setAttribute( "class", "disabled" );
    }
  }
  else
  {
    dump( "not changing element "+elementID+"\n" );
  }
}

// Get the text appropriate to parent container
//  to determine what a "%" value is refering to.
// elementForAtt is element we are actually setting attributes on
//  (a temporary copy of element in the doc to allow canceling),
//  but elementInDoc is needed to find parent context in document
function GetAppropriatePercentString(elementForAtt, elementInDoc)
{
  var name = elementForAtt.nodeName.toLowerCase();
  if ( name == "td" || name == "th")
    return GetString("PercentOfTable");

  // Check if element is within a table cell
  // Check if current selection anchor node is within a table cell
  if (editorShell.GetElementOrParentByTagName("td", elementInDoc))
    return GetString("PercentOfCell");
  else
    return GetString("PercentOfWindow");
}

function InitPixelOrPercentMenulist(elementForAtt, elementInDoc, attribute, menulistID, defaultIndex)
{
  if (!defaultIndex) defaultIndex = gPixel;

  var size  = elementForAtt.getAttribute(attribute);
  var menulist = document.getElementById(menulistID);
  var pixelItem;
  var percentItem;

  if (!menulist)
  {
    dump("NO MENULIST found for ID="+menulistID+"\n");
    return size;
  }

  ClearMenulist(menulist);
  pixelItem = AppendStringToMenulist(menulist, GetString("Pixels"));

  if (!pixelItem) return 0;

  percentItem = AppendStringToMenulist(menulist, GetAppropriatePercentString(elementForAtt, elementInDoc));
  if (size && size.length > 0)
  {
    // Search for a "%" character
    var percentIndex = size.search(/%/);
    if (percentIndex > 0)
    {
      // Strip out the %
      size = size.substr(0, percentIndex);
      if (percentItem)
        menulist.selectedItem = percentItem;
    }
    else
      menulist.selectedItem = pixelItem;
  }
  else
    menulist.selectedIndex = defaultIndex;

  return size;
}

function AppendStringToMenulistById(menulist, stringID)
{
  return AppendStringToMenulist(menulist, editorShell.GetString(stringID));
}

function AppendStringToMenulist(menulist, string)
{
  if (menulist)
  {
    var menupopup = menulist.firstChild;
    // May not have any popup yet -- so create one
    if (!menupopup)
    {
      menupopup = document.createElementNS("http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul", "menupopup");
      if (menupopup)
        menulist.appendChild(menupopup);
      else
      {
        dump("Failed to create menupoup\n");
        return null;
      }
    }
    var menuItem = document.createElementNS("http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul", "menuitem");
    if (menuItem)
    {
      menuItem.setAttribute("label", string);
      menupopup.appendChild(menuItem);
      return menuItem;
    }
  }
  return null;
}

function AppendLabelAndValueToMenulist(menulist, labelStr, valueStr)
{
  if (menulist)
  {
    var menupopup = menulist.firstChild;
    // May not have any popup yet -- so create one
    if (!menupopup)
    {
      menupopup = document.createElementNS("http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul", "menupopup");
      if (menupopup)
        menulist.appendChild(menupopup);
      else
      {
        dump("Failed to create menupoup\n");
        return null;
      }
    }
    var menuItem = document.createElementNS("http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul", "menuitem");
    if (menuItem)
    {
      menuItem.setAttribute("label", labelStr);
      menuItem.setAttribute("value", valueStr);
      menupopup.appendChild(menuItem);
      return menuItem;
    }
  }
  return null;
}

function ClearMenulist(menulist)
{
  // There is usually not more than 1 menupopup under a menulist,
  //  but look for > 1 children anyway.
  // Note -- this doesn't remove menuitems from the menupopop -- SHOULD WE?
  if (menulist) {
    menulist.selectedItem = null;
    while (menulist.firstChild)
      menulist.removeChild(menulist.firstChild);
  }
}

/* These help using a <tree> for simple lists
  Assumes this simple structure:
  <tree>
    <treecolgroup><treecol flex="1"/></treecolgroup>
    <treechildren>
      <treeitem>
        <treerow>
          <treecell label="the text the user sees"/>
*/

function AppendStringToTreelistById(tree, stringID)
{
  return AppendStringToTreelist(tree, editorShell.GetString(stringID));
}

function AppendStringToTreelist(tree, string)
{
  if (tree)
  {
    var treecols = tree.firstChild;
    if (!treecols)
    {
      dump("Bad XUL: Must have <treecolgroup> as first child\n");
    }
    var treechildren = treecols.nextSibling;
    if (!treechildren)
    {
      treechildren = document.createElementNS("http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul", "treechildren");
      if (treechildren)
      {
        treechildren.setAttribute("flex","1");
        tree.appendChild(treechildren);
      }
      else
      {
        dump("Failed to create <treechildren>\n");
        return null;
      }
    }
    var treeitem = document.createElementNS("http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul", "treeitem");
    var treerow = document.createElementNS("http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul", "treerow");
    var treecell = document.createElementNS("http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul", "treecell");
    if (treeitem && treerow && treecell)
    {
      treerow.appendChild(treecell);
      treeitem.appendChild(treerow);
      treechildren.appendChild(treeitem)
      treecell.setAttribute("label", string);
      //var len = Number(tree.getAttribute("length"));
      //if (!len) len = -1;
      tree.setAttribute("length", treechildren.childNodes.length);
      return treeitem;
    }
  }
  return null;
}

function ReplaceStringInTreeList(tree, index, string)
{
  if (tree)
  {
    var treecols = tree.firstChild;
    if (!treecols)
    {
      dump("Bad XUL: Must have <treecolgroup> as first child\n");
      return;
    }
    var treechildren = treecols.nextSibling;
    if (!treechildren)
      return;

    // Each list item is a <treeitem><treerow><treecell> under <treechildren> node
    var childNodes = treechildren.childNodes;
    if (!childNodes || index >= childNodes.length)
      return;

    var row = childNodes.item(index).firstChild;
    if (row && row.firstChild)
    {
      row.firstChild.setAttribute("label", string);
    }
  }
}

function ClearTreelist(tree)
{
  if (tree)
  {
    tree.clearItemSelection();
    // Skip over the first <treecolgroup> child
    if (tree.firstChild)
    {
      nextChild = tree.firstChild.nextSibling;
      while (nextChild)
      {
        var nextTmp = nextChild.nextSibling;
        tree.removeChild(nextChild);
        nextChild = nextTmp;
      }
    }
    // Count list items
    tree.setAttribute("length", 0);
  }
}

function GetSelectedTreelistAttribute(tree, attr)
{
  if (tree)
  {
    if (tree.selectedIndex >= 0 &&
        tree.selectedItems.length > 0 &&
        tree.selectedItems[0] &&
        tree.selectedItems[0].firstChild &&
        tree.selectedItems[0].firstChild.firstChild)
    {
      return tree.selectedItems[0].firstChild.firstChild.getAttribute(attr);
    }
  }
  return "";
}

function GetSelectedTreelistValue(tree)
{
  return GetSelectedTreelistAttribute(tree,"label")
}

function RemoveSelectedTreelistItem(tree)
{
  if (tree)
  {
    if (tree.selectedIndex >= 0 &&
        tree.selectedItems.length > 0)
    {
      // Get the node to delete
      var treeItem = tree.selectedItems[0];
      if (treeItem)
      {
        tree.clearItemSelection();
        var parent = treeItem.parentNode;
        if (parent)
        {
          parent.removeChild(treeItem);
          tree.setAttribute("length", parent.childNodes.length);
        }
      }
    }
  }
}

function GetTreelistValueAt(tree, index)
{
  if (tree)
  {
    var item = tree.getItemAtIndex(index);
    if (item && item.firstChild && item.firstChild.firstChild)
      return item.firstChild.firstChild.getAttribute("label");
  }
  return "";
}

function forceInteger(elementID)
{
  var editField = document.getElementById( elementID );
  if ( !editField )
    return;

  var stringIn = editField.value;
  if (stringIn && stringIn.length > 0)
  {
    stringIn = stringIn.replace(/\D+/g,"");
    if (!stringIn) stringIn = "";
    // Strip out all nonnumeric characters
    editField.value = stringIn;
  }
}


function onAdvancedEdit()
{
  // First validate data from widgets in the "simpler" property dialog
  if (ValidateData())
  {
    // Set true if OK is clicked in the Advanced Edit dialog
    window.AdvancedEditOK = false;
    // Open the AdvancedEdit dialog, passing in the element to be edited
    //  (the copy named "globalElement")
    window.openDialog("chrome://editor/content/EdAdvancedEdit.xul", "_blank", "chrome,close,titlebar,modal,resizable=yes", "", globalElement);
    window.focus();
    if (window.AdvancedEditOK)
    {
      // Copy edited attributes to the dialog widgets:
      InitDialog();
    }
  }
}

function GetSelectionAsText()
{
  return editorShell.GetContentsAs("text/plain", gOutputSelectionOnly);
}


// ** getSelection ()
// ** This function checks for existence of table around the focus node
// ** Brian King - XML Workshop

function getContainer ()
{
  tagName = "img";
  selImage = editorShell.GetSelectedElement(tagName);
  if (selImage)  // existing image
  {
    oneup = selImage.parentNode;
    return oneup;
  }
  else if (!selImage)  // new image insertion
  {
    dump("Not an image element -- Trying for caret selection\n");
    var selection = window.editorShell.editorSelection;
    if (selection)
    {
        var focusN = selection.focusNode;
        if (focusN.nodeName.toLowerCase == "td")
          return focusN
                else
        {
          oneup = focusN.parentNode;
          return oneup;
         }
    }
    else
      return null;
   }
   else
     return null;
}

function getColor(ColorPickerID)
{
  var colorPicker = document.getElementById(ColorPickerID);
  var color;
  if (colorPicker)
  {
    // Extract color from colorPicker and assign to colorWell.
    color = colorPicker.getAttribute("color");
    if (color && color == "")
      return null;
    // Clear color so next if it's called again before
    //  color picker is actually used, we dedect the "don't set color" state
    colorPicker.setAttribute("color","");
  }

  return color;
}

function setColorWell(ColorWellID, color)
{
  var colorWell = document.getElementById(ColorWellID);
  if (colorWell)
  {
    if (!color || color == "")
    {
      // Don't set color (use default)
      // Trigger change to not show color swatch
      colorWell.setAttribute("default","true");
      // Style in CSS sets "background-color",
      //   but color won't clear unless we do this:
      colorWell.removeAttribute("style");
    }
    else
    {
      colorWell.removeAttribute("default");
      // Use setAttribute so colorwell can be a XUL element, such as button
      colorWell.setAttribute("style", "background-color:"+color);
    }
  }
}

function getColorAndSetColorWell(ColorPickerID, ColorWellID)
{
  var color = getColor(ColorPickerID);
  setColorWell(ColorWellID, color);
  return color;
}

// Test for valid image by sniffing out the extension
function IsValidImage(imageName)
{
  var image = imageName.trimString();
  if ( !image )
    return false;

  /* look for an extension */
  var tailindex = image.lastIndexOf(".");
  if ( tailindex == 0 || tailindex == -1 ) /* -1 is not found */
    return false;

  /* move past period, get the substring from the first character after the '.' to the last character (length) */
  tailindex = tailindex + 1;
  var type = image.substring(tailindex,image.length);

  /* convert extension to lower case */
  if (type)
    type = type.toLowerCase();

  // TODO: Will we convert .BMPs to a web format?
  switch( type )
  {
    case "gif":
    case "jpg":
    case "jpeg":
    case "png":
      return true;
      break;
  }
  return false;
}

function InitMoreFewer()
{
  // Set SeeMore bool to the OPPOSITE of the current state,
  //   which is automatically saved by using the 'persist="more"'
  //   attribute on the dialog.MoreFewerButton button
  //   onMoreFewer will toggle it and redraw the dialog
  SeeMore = (dialog.MoreFewerButton.getAttribute("more") != "1");
  onMoreFewer();
}

function onMoreFewer()
{
  if (SeeMore)
  {
    dialog.MoreSection.setAttribute("collapsed","true");
    window.sizeToContent();
    dialog.MoreFewerButton.setAttribute("more","0");
    dialog.MoreFewerButton.setAttribute("label",GetString("MoreProperties"));
    SeeMore = false;
  }
  else
  {
    dialog.MoreSection.removeAttribute("collapsed");
    window.sizeToContent();
    dialog.MoreFewerButton.setAttribute("more","1");
    dialog.MoreFewerButton.setAttribute("label",GetString("FewerProperties"));
    SeeMore = true;
  }
}

function SwitchToValidatePanel()
{
  // no default implementation
  // Only EdTableProps.js currently implements this
}

function GetPrefs()
{
  var prefs;
  try {
    prefs = Components.classes['@mozilla.org/preferences;1'];
    if (prefs) prefs = prefs.getService();
    if (prefs) prefs = prefs.QueryInterface(Components.interfaces.nsIPref);
    if (prefs)
      return prefs;
    else
      dump("failed to get prefs service!\n");

  }
  catch(ex)
  {
    dump("failed to get prefs service!\n");
  }
  return null;
}

const nsIFilePicker = Components.interfaces.nsIFilePicker;

function GetLocalFileURL(filterType)
{
  var fp = Components.classes["@mozilla.org/filepicker;1"].createInstance(nsIFilePicker);

  if (filterType == "img")
  {
    fp.init(window, GetString("SelectImageFile"), nsIFilePicker.modeOpen);
    fp.appendFilters(nsIFilePicker.filterImages);
  }
  else (filterType == "html")
  {
    fp.init(window, GetString("OpenHTMLFile"), nsIFilePicker.modeOpen);

    // When loading into Composer, direct user to prefer HTML files and text files,
    //   so we call separately to control the order of the filter list
    fp.appendFilters(nsIFilePicker.filterHTML);
    fp.appendFilters(nsIFilePicker.filterText);
  }
  // Default or last filter is "All Files"
  fp.appendFilters(nsIFilePicker.filterAll);

  /* doesn't handle *.shtml files */
  try {
    var ret = fp.show();
    if (ret == nsIFilePicker.returnCancel)
      return null;
  }
  catch (ex) {
    dump("filePicker.chooseInputFile threw an exception\n");
    return null;
  }

  return fp.fileURL.spec;
}

function GetMetaElement(name)
{
  if (name)
  {
    name = name.toLowerCase();
    if (name != "")
    {
      var metaNodes = editorShell.editorDocument.getElementsByTagName("meta");
      if (metaNodes && metaNodes.length > 0)
      {
        for (var i = 0; i < metaNodes.length; i++)
        {
          var metaNode = metaNodes.item(i);
          if (metaNode && metaNode.getAttribute("name") == name)
            return metaNode;
        }
      }
    }
  }
  return null;
}

function CreateMetaElement(name)
{
  metaElement = editorShell.CreateElementWithDefaults("meta");
  if (metaElement)
    metaElement.setAttribute("name", name);
  else
    dump("Failed to create metaElement!\n");

  return metaElement;
}

function GetHTTPEquivMetaElement(name)
{
  if (name)
  {
    name = name.toLowerCase();
    if (name != "")
    {
      var metaNodes = editorShell.editorDocument.getElementsByTagName("meta");
      if (metaNodes && metaNodes.length > 0)
      {
        for (var i = 0; i < metaNodes.length; i++)
        {
          var metaNode = metaNodes.item(i);
          if (metaNode && metaNode.getAttribute("http-equiv").toLowerCase() == name)
            return metaNode;
        }
      }
    }
  }
  return null;
}

function CreateHTTPEquivMetaElement(name)
{
  metaElement = editorShell.CreateElementWithDefaults("meta");
  if (metaElement)
    metaElement.setAttribute("http-equiv", name);
  else
    dump("Failed to create httpequivMetaElement!\n");

  return metaElement;
}

function CreateHTTPEquivElement(name)
{
  metaElement = editorShell.CreateElementWithDefaults("meta");
  if (metaElement)
    metaElement.setAttribute("http-equiv", name);
  else
    dump("Failed to create metaElement for http-equiv!\n");

  return metaElement;
}

// Change "content" attribute on a META element,
//   or delete entire element it if content is empty
// This uses undoable editor transactions
function SetMetaElementContent(metaElement, content, insertNew)
{
  if (metaElement)
  {
    if(!content || content == "")
    {
      if (!insertNew)
        editorShell.DeleteElement(metaElement);
    }
    else
    {
      if (insertNew)
      {
        metaElement.setAttribute("content", content);
        AppendHeadElement(metaElement);
      }
      else
        editorShell.SetAttribute(metaElement, "content", content);
    }
  }
}

function GetHeadElement()
{
  var headList = editorShell.editorDocument.getElementsByTagName("head");
  if (headList)
    return headList.item(0);

  return null;
}

function AppendHeadElement(element)
{
  var head = GetHeadElement();
  if (head)
  {
    var position = 0;
    if (head.hasChildNodes())
      position = head.childNodes.length;

    // Use editor's undoable transaction
    // Last param "true" says "don't change the selection"
    editorShell.InsertElement(element, head, position, true);
  }
}

function SetWindowLocation()
{
  gLocation = document.getElementById("location");
  if (gLocation)
  {
    window.screenX = Math.max(0, Math.min(window.opener.screenX + Number(gLocation.getAttribute("offsetX")),
                                          screen.availWidth - window.outerWidth));
    window.screenY = Math.max(0, Math.min(window.opener.screenY + Number(gLocation.getAttribute("offsetY")),
                                          screen.availHeight - window.outerHeight));
  }
}

function SaveWindowLocation()
{
  if (gLocation)
  {
    var newOffsetX = window.screenX - window.opener.screenX;
    var newOffsetY = window.screenY - window.opener.screenY;
    gLocation.setAttribute("offsetX", window.screenX - window.opener.screenX);
    gLocation.setAttribute("offsetY", window.screenY - window.opener.screenY);
  }
}

function onCancel()
{
  SaveWindowLocation();
  window.close();
}

function GetDefaultBrowserColors()
{
  var prefs = GetPrefs();
  var colors = new Object();
  var useSysColors = false;
  colors.TextColor = 0;
  colors.BackgroundColor = 0;
  try { useSysColors = prefs.GetBoolPref("browser.display.use_system_colors"); } catch (e) {}

  if (!useSysColors)
  {
    try { colors.TextColor = prefs.CopyCharPref("browser.display.foreground_color"); } catch (e) {}

    try { colors.BackgroundColor = prefs.CopyCharPref("browser.display.background_color"); } catch (e) {}
  }
  // Use OS colors for text and background if explicitly asked or pref is not set
  if (!colors.TextColor)
    colors.TextColor = "windowtext";

  if (!colors.BackgroundColor)
    colors.BackgroundColor = "window";

  colors.LinkColor = prefs.CopyCharPref("browser.anchor_color");
  colors.VisitedLinkColor = prefs.CopyCharPref("browser.visited_color");

  return colors;
}
