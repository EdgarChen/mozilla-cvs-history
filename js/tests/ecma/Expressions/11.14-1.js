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
    File Name:          11.14-1.js
    ECMA Section:       11.14 Comma operator (,)
    Description:
    Expression :

    AssignmentExpression
    Expression , AssignmentExpression

    Semantics

    The production Expression : Expression , AssignmentExpression is evaluated as follows:

    1.  Evaluate Expression.
    2.  Call GetValue(Result(1)).
    3.  Evaluate AssignmentExpression.
    4.  Call GetValue(Result(3)).
    5.  Return Result(4).

    Author:             christine@netscape.com
    Date:               12 november 1997
*/
    var SECTION = "11.14-1";
    var VERSION = "ECMA_1";
    startTest();
    var testcases = getTestCases();

    writeHeaderToLog( SECTION + " Comma operator (,)");
    test();

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
function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION,    "true, false",                    false,  eval("true, false") );
    array[item++] = new TestCase( SECTION,    "VAR1=true, VAR2=false",          false,  eval("VAR1=true, VAR2=false") );
    array[item++] = new TestCase( SECTION,    "VAR1=true, VAR2=false;VAR1",     true,   eval("VAR1=true, VAR2=false; VAR1") );
    return ( array );
}
