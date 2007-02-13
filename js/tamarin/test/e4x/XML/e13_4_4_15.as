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

START("13.4.4.15 - hasComplexContent()");

//TEST(1, true, XML.prototype.hasOwnProperty("hasComplexContent"));

x1 = 
<alpha attr1="value1">
    <bravo>one</bravo>
    <charlie>
        two
        three
        <echo>four</echo>
    </charlie>
    <delta />
    <foxtrot attr2="value2">five</foxtrot>
    <golf attr3="value3" />
    <hotel>
        six
        seven
    </hotel>
    <india><juliet /></india>
</alpha>;

TEST(2, true, x1.hasComplexContent());
TEST(3, false, x1.bravo.hasComplexContent());
TEST(4, true, x1.charlie.hasComplexContent());
TEST(5, false, x1.delta.hasComplexContent());
TEST(6, false, x1.foxtrot.hasComplexContent());
TEST(7, false, x1.golf.hasComplexContent());
TEST(8, false, x1.hotel.hasComplexContent());
TEST(9, false, x1.@attr1.hasComplexContent());
TEST(10, false, x1.bravo.child(0).hasComplexContent());
TEST(11, true, x1.india.hasComplexContent());
    
var xmlDoc = "<employee id='1'><firstname>John</firstname><lastname>Walton</lastname><age dob='1/2/1978'>25</age></employee>"

// propertyName as a string
AddTestCase( "MYXML = new XML(xmlDoc), MYXML.hasComplexContent()", true, 
             (MYXML = new XML(xmlDoc), MYXML.hasComplexContent()));

xmlDoc = "<firstname>John</firstname>"  
AddTestCase( "MYXML = new XML(xmlDoc), MYXML.hasComplexContent()", false, 
             (MYXML = new XML(xmlDoc), MYXML.hasComplexContent()));

XML.ignoreComments = false;
xmlDoc = "<XML><!-- firstname --></XML>"  
AddTestCase( "MYXML = new XML(xmlDoc), MYXML.hasComplexContent()", false, 
             (MYXML = new XML(xmlDoc), MYXML.hasComplexContent()));

XML.ignoreProcessingInstructions = false;
xmlDoc = "<XML><?xml-stylesheet href=\"classic.xsl\" type=\"text/xml\"?></XML>"  
AddTestCase( "MYXML = new XML(xmlDoc), MYXML.hasComplexContent()", false, 
             (MYXML = new XML(xmlDoc), MYXML.hasComplexContent()));

XML.ignoreComments = false;
xmlDoc = "<XML>foo</XML>"  
AddTestCase( "MYXML = new XML(xmlDoc), MYXML.hasComplexContent()", false, 
             (MYXML = new XML(xmlDoc), MYXML.hasComplexContent()));

END();
