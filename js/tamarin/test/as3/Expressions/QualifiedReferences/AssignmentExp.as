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
 * The Original Code is [Open Source Virtual Machine.].
 *
 * The Initial Developer of the Original Code is
 * Adobe System Incorporated.
 * Portions created by the Initial Developer are Copyright (C) 2005-2006
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Adobe AS3 Team
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

var SECTION = "Expressions";       // provide a document reference (ie, Actionscript section)
var VERSION = "AS 3.0";        // Version of ECMAScript or ActionScript 
var TITLE   = "Qualified references with AssignmentExpression";       // Provide ECMA section title or a description
var BUGNUMBER = "";

startTest();                // leave this alone



import ns.*;
var f:foo = new foo();

 AddTestCase("f.N2::n2='3'", 3, f.N2::n2);
 AddTestCase("f.1::v3='3'", 3, f.N1::v3);
 AddTestCase("f.v3='5'", 5, f.v3);

 AddTestCase("f.N2::n3=v3", 5, f.N2::n3);
 AddTestCase("f.N2::n3=v3; f.N2::['n3']", 5, f.N2::['n3']);
 AddTestCase("f.N2::n4=o[holder]", 4, f.N2::n4);
 
 AddTestCase("N4 = N3; f.N3::v1 =5; f.N4::v1", 5, f.N4::v1);
 AddTestCase("N4 = N3; f.N3::['v2'] =5; f.N4::v2", 5, f.N4::v2);
 
 test();
