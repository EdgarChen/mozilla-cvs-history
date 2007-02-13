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

START("13.5.4.7 - XMLList contains()");

//TEST(1, true, XMLList.prototype.hasOwnProperty("contains"));

emps =
<employees>
    <employee id="0"><name>Jim</name><age>25</age></employee>
    <employee id="1"><name>Joe</name><age>20</age></employee>
</employees>;

TEST(2, true, emps.employee.contains(<employee id="0"><name>Jim</name><age>25</age></employee>));
TEST(3, true, emps.employee.contains(<employee id="1"><name>Joe</name><age>20</age></employee>));
TEST(4, false, emps.employee.contains(<employee><name>Joe</name><age>20</age></employee>));

var xmlDoc = "<Team>Giants</Team><City>San Francisco</City><Team>Padres</Team><City>San Diego</City>";
var xmlobj1 = "<City>San Francisco</City>";
var xmlobj2 = "<Street>56 Hill Street</Street>";


// XML object passed as value exists in the XML List
AddTestCase( "MYXML = new XMLList(xmlDoc),MYOBJ = new XML(xmlobj1), MYXML.contains(MYOBJ)",true, 
             (MYXML = new XMLList(xmlDoc),MYOBJ = new XML(xmlobj1), MYXML.contains(MYOBJ)));

//XML object passed as value does not exist in the XML List
AddTestCase( "MYXML = new XMLList(xmlDoc), MYOBJ = new XML(xmlobj2), MYXML.contains(MYOBJ)",false, 
             (MYXML = new XMLList(xmlDoc), MYOBJ = new XML(xmlobj2), MYXML.contains(MYOBJ)));

END();
