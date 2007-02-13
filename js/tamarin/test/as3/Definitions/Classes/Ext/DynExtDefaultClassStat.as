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
 * Portions created by the Initial Developer are Copyright (C) 2004-2006
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


import DefaultClass.*;

var SECTION = "Definitions";       // provide a document reference (ie, ECMA section)
var VERSION = "AS 3.0";  // Version of JavaScript or ECMA
var TITLE   = "static extend Default Class";       // Provide ECMA section title or a description
var BUGNUMBER = "";

startTest();                // leave this alone

/**
 * Calls to AddTestCase here. AddTestCase is a function that is defined
 * in shell.js and takes three arguments:
 * - a string representation of what is being tested
 * - the expected result
 * - the actual result
 *
 * For example, a test might look like this:
 *
 * var helloWorld = "Hello World";
 *
 * AddTestCase(
 * "var helloWorld = 'Hello World'",   // description of the test
 *  "Hello World",                     // expected result
 *  helloWorld );                      // actual result
 *
 */

EXTDCLASS = new DynExtDefaultClassStat();
arr = new Array(1, 2, 3);

//*******************************************
// access static method of parent class
// from outside of class
//*******************************************
/*
AddTestCase( "Access static method of parent class from outside of class", arr, 
             (EXTDCLASS.setStatArray(arr), EXTDCLASS.statArray) );

*/
// ********************************************
// access static method from a default
// method of a sub class
//
// ********************************************

EXTDCLASS = new DynExtDefaultClassStat();
AddTestCase( "*** Access static method from default method of sub class ***", 1, 1 );
AddTestCase( "EXTDCLASS.testSubArray(arr)", arr, (EXTDCLASS.testSubArray(arr)) );


// ********************************************
// access static method from a public 
// method of a sub class
//
// ********************************************

EXTDCLASS = new DynExtDefaultClassStat();
AddTestCase( "*** Access static method from public method of sub class ***", 1, 1 );
AddTestCase( "EXTDCLASS.pubSubSetArray(arr), EXTDCLASS.pubSubGetArray()", arr, (EXTDCLASS.pubSubSetArray(arr), EXTDCLASS.pubSubGetArray()) );

// ********************************************
// access static method from a private 
// method of a sub class
//
// ********************************************

EXTDCLASS = new DynExtDefaultClassStat();
AddTestCase( "*** Access static method from private method of sub class ***", 1, 1 );
AddTestCase( "EXTDCLASS.testPrivSubArray(arr)", arr, EXTDCLASS.testPrivSubArray(arr) );

// ********************************************
// access static method from a final 
// method of a sub class
//
// ********************************************

EXTDCLASS = new DynExtDefaultClassStat();
AddTestCase( "access static method from final method of sub class", arr, (EXTDCLASS.testFinSubArray(arr)) );

// ********************************************
// access static method from a static 
// method of a sub class
//
// ********************************************
/*
AddTestCase( "*** Access static method from static method of sub class ***", 1, 1 );
AddTestCase( "EXTDCLASS.testStatSubArray(arr)", arr, 
             (EXTDCLASS.testStatSubArray(arr)) );

// ********************************************
// access static method from a public static 
// method of a sub class
//
// ********************************************

AddTestCase( "*** Access static method from public static method of sub class ***", 1, 1 );
AddTestCase( "EXTDCLASS.testPubStatSubArray(arr)", arr, 
             (EXTDCLASS.testPubStatSubArray(arr)) );
*/
// ********************************************
// access static method from a private static 
// method of a sub class
//
// ********************************************

var EXTDEFAULTCLASS = new DynExtDefaultClassStat();
AddTestCase( "*** Access static method from private static method of sub class ***", 1, 1 );
AddTestCase( "EXTDEFAULTCLASS.testPrivStatSubArray(arr)", arr, 
              EXTDEFAULTCLASS.testPrivStatSubArray(arr) );
              
// ********************************************
// access static property from 
// default method in sub class
// ********************************************

EXTDCLASS = new DynExtDefaultClassStat();
AddTestCase( "*** Access static property from default method in sub class ***", 1, 1 );
AddTestCase( "EXTDCLASS.testSubPArray(arr)", arr, (EXTDCLASS.testSubPArray(arr)) );


// ********************************************
// access static property from 
// public method in sub class
// ********************************************

EXTDCLASS = new DynExtDefaultClassStat();
AddTestCase( "*** Access static property from public method in sub class ***", 1, 1 );
AddTestCase( "EXTDCLASS.pubSubSetDPArray(arr), EXTDCLASS.pubSubGetDPArray()", arr, (EXTDCLASS.pubSubSetDPArray(arr), EXTDCLASS.pubSubGetDPArray()) );


// ********************************************
// access static property from 
// private method in sub class
// ********************************************

EXTDCLASS = new DynExtDefaultClassStat();
AddTestCase( "*** Access static property from private method in sub class ***", 1, 1 );
AddTestCase( "EXTDCLASS.testPrivSubPArray(arr)", arr, (EXTDCLASS.testPrivSubPArray(arr)) );


// ********************************************
// access static property from
// static method in sub class 
// ********************************************

AddTestCase( "*** Access static property from static method in sub class ***", 1, 1 );
/*AddTestCase( "ExtDefaultClassStat.statSubSetSPArray(arr), ExtDefaultClassStat.statSubGetSPArray()", arr,
             (ExtDefaultClassStat.statSubSetSPArray(arr), ExtDefaultClassStat.statSubGetSPArray()) );
*/
// ********************************************
// access static property from
// public static method in sub class 
// ********************************************

AddTestCase( "*** Access static property from public static method in sub class ***", 1, 1 );
/*AddTestCase( "ExtDefaultClassStat.pubStatSubSetSPArray(arr), ExtDefaultClassStat.pubStatSubGetSPArray()", arr,
             (ExtDefaultClassStat.pubStatSubSetSPArray(arr), ExtDefaultClassStat.pubStatSubGetSPArray()) );
*/
// ********************************************
// access static property from 
// private static method in sub class
// ********************************************

EXTDCLASS = new DynExtDefaultClassStat();
AddTestCase( "*** Access static property from private static method in sub class ***", 1, 1 );
AddTestCase( "EXTDCLASS.testPrivStatSubPArray(arr)", arr, 
              EXTDCLASS.testPrivStatSubPArray(arr));

// ********************************************
// access static property from 
// final method in sub class
// ********************************************

EXTDCLASS = new DynExtDefaultClassStat();
AddTestCase( "*** Access static property from final method in sub class ***", 1, 1 );
AddTestCase( "EXTDCLASS.testFinSubPArray(arr)", arr, (EXTDCLASS.testFinSubPArray(arr)) );

test();       // leave this alone.  this executes the test cases and
              // displays results.
