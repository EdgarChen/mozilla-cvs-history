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
    File Name:          7.2-1.js
    ECMA Section:       7.2 Line Terminators
    Description:        - readability
                        - separate tokens
                        - may occur between any two tokens
                        - cannot occur within any token, not even a string
                        - affect the process of automatic semicolon insertion.

                        white space characters are:
                        unicode     name            formal name     string representation
                        \u000A      line feed       <LF>            \n
                        \u000D      carriage return <CR>            \r

    Author:             christine@netscape.com
    Date:               11 september 1997
*/
    var SECTION = "7.2-1";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Line Terminators";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION,    "var a\nb =  5; ab=10;ab;",     10,     eval("var a\nb =  5; ab=10;ab") );
    array[item++] = new TestCase( SECTION,    "var a\nb =  5; ab=10;b;",      5,      eval("var a\nb =  5; ab=10;b") );
    array[item++] = new TestCase( SECTION,    "var a\rb =  5; ab=10;ab;",     10,     eval("var a\rb =  5; ab=10;ab") );
    array[item++] = new TestCase( SECTION,    "var a\rb =  5; ab=10;b;",      5,      eval("var a\rb =  5; ab=10;b") );
    array[item++] = new TestCase( SECTION,    "var a\r\nb =  5; ab=10;ab;",     10,     eval("var a\r\nb =  5; ab=10;ab") );
    array[item++] = new TestCase( SECTION,    "var a\r\nb =  5; ab=10;b;",      5,      eval("var a\r\nb =  5; ab=10;b") );

    return ( array );
}

function test() {
    for ( tc=0; tc < testcases.length; tc++ ) {
        testcases[tc].passed = writeTestCaseResult(
                            testcases[tc].expect,
                            testcases[tc].actual,
                            testcases[tc].description +" = "+
                            testcases[tc].actual );

        testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value ";
    }
    stopTest();
    return ( testcases );
}
