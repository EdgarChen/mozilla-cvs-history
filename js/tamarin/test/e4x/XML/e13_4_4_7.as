/* -*- Mode: java; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * ***** BEGIN LICENSE BLOCK *****
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
 * The Original Code is Rhino code, released
 * May 6, 1999.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1997-2000
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Igor Bukanov
 *   Ethan Hugg
 *   Milen Nankov
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

START("13.4.4.7 - XML childIndex()");

//TEST(1, true, XML.prototype.hasOwnProperty("childIndex"));

emps =
<employees>
    <employee id="0"><name>Jim</name><age>25</age></employee>
    <employee id="1"><name>Joe</name><age>20</age></employee>
</employees>;

TEST(2, 0, emps.employee[0].childIndex());

// Get the ordinal index of the employee named Joe
TEST(3, 1, emps.employee.(age == "20").childIndex());
TEST(4, 1, emps.employee.(name == "Joe").childIndex());

var xmlDoc = "<MLB><Team>Giants</Team><City>San Francisco</City><Team>Padres</Team><City>San Diego</City></MLB>";

// valid test cases
// MYXML.child(0) returns an XMLList with one node
// The [0] returns the first element in the list which should be child 0 
AddTestCase( "MYXML = new XML(xmlDoc), MYXML.child(0)[0].childIndex()", 0, 
             (MYXML = new XML(xmlDoc), MYXML.child(0)[0].childIndex() ));

AddTestCase( "MYXML = new XML(xmlDoc), MYXML.child(1)[0].childIndex()", 1, 
             (MYXML = new XML(xmlDoc), MYXML.child(1)[0].childIndex() ));

// what does childIndex return if there is no parent
AddTestCase( "MYXML = new XML(xmlDoc), MYXML.childIndex()", -1, 
             (MYXML = new XML(xmlDoc), MYXML.childIndex() ));

END();
