/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla.org Code.
 *
 * The Initial Developer of the Original Code is Aaron Leventhal
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2003
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s): Aaron Leventhal
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

const kTabToLinks = 4
const kTabToForms = 2;
parent.hPrefWindow.registerOKCallbackFunc(saveKeyNavPrefs);
var gData;

function initPrefs()
{
  gData = parent.hPrefWindow.wsm.dataManager.pageData["chrome://communicator/content/pref/pref-keynav.xul"];

  if (!("tabNavPref" in gData)) {
    // Textboxes are always part of the tab order
    gData.tabNavPref = parent.hPrefWindow.getPref('int', 'accessibility.tabfocus') | 1;
    gData.linksOnlyPref = parent.hPrefWindow.getPref('bool', 'accessibility.typeaheadfind.linksonly')? 1: 0;
  }

  document.getElementById('tabNavigationLinks').setChecked((gData.tabNavPref & kTabToLinks) != 0);
  document.getElementById('tabNavigationForms').setChecked((gData.tabNavPref & kTabToForms) != 0);

  // XXX todo: On the mac, only the links checkbox should be exposed.
  //           Whether the other form controls are tabbable is a system setting
  //           that we should adhere to.


  var radioGroup = document.getElementById('findAsYouTypeAutoWhat');
  radioGroup.selectedIndex = gData.linksOnlyPref;
  setLinksOnlyDisabled();
}

function setLinksOnlyDisabled()
{
  try {
    document.getElementById('findAsYouTypeAutoWhat').disabled = 
     (document.getElementById('findAsYouTypeEnableAuto').checked == false);
  }
  catch(e) {}
}

function saveKeyNavPrefs()
{
  var data = parent.hPrefWindow.wsm.dataManager.pageData["chrome://communicator/content/pref/pref-keynav.xul"];
  parent.hPrefWindow.setPref("int", "accessibility.tabfocus", data.tabNavPref);
  parent.hPrefWindow.setPref("bool", "accessibility.typeaheadfind.linksonly", data.linksOnlyPref == 1);
}
