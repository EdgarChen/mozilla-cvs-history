/* The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 * 
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * The Original Code is Mozilla Communicator client code, released March
 * 31, 1998.
 * 
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation. Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation. All Rights Reserved.
 * 
 */
/**
    File Name:          15.5.4.2-1.js
    ECMA Section:       15.5.4.2 String.prototype.toString()

    Description:        Returns this string value.  Note that, for a String
                        object, the toString() method happens to return the same
                        thing as the valueOf() method.

                        The toString function is not generic; it generates a
                        runtime error if its this value is not a String object.
                        Therefore it connot be transferred to the other kinds of
                        objects for use as a method.

    Author:             christine@netscape.com
    Date:               1 october 1997
*/

    var SECTION = "15.5.4.2-1";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "String.prototype.toString";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;
    array[item++] = new TestCase( SECTION,   "String.prototype.toString()",        "",     String.prototype.toString() );
    array[item++] = new TestCase( SECTION,   "(new String()).toString()",          "",     (new String()).toString() );
    array[item++] = new TestCase( SECTION,   "(new String(\"\")).toString()",      "",     (new String("")).toString() );
    array[item++] = new TestCase( SECTION,   "(new String( String() )).toString()","",    (new String(String())).toString() );
    array[item++] = new TestCase( SECTION,  "(new String( \"h e l l o\" )).toString()",       "h e l l o",    (new String("h e l l o")).toString() );
    array[item++] = new TestCase( SECTION,   "(new String( 0 )).toString()",       "0",    (new String(0)).toString() );
    return ( array );
}
function test( array ) {
    for ( tc = 0; tc < testcases.length; tc++ ) {

        testcases[tc].passed = writeTestCaseResult(
                            testcases[tc].expect,
                            testcases[tc].actual,
                            testcases[tc].description +" = "+ testcases[tc].actual );
    }
    stopTest();
    return ( testcases );
}
