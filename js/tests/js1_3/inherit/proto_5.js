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
    File Name:          proto_5.js
    Section:
    Description:        Logical OR || in Constructors

    This tests Object Hierarchy and Inheritance, as described in the document
    Object Hierarchy and Inheritance in JavaScript, last modified on 12/18/97
    15:19:34 on http://devedge.netscape.com/.  Current URL:
    http://devedge.netscape.com/docs/manuals/communicator/jsobj/contents.htm

    This tests the syntax ObjectName.prototype = new PrototypeObject using the
    Employee example in the document referenced above.

    This tests the logical OR opererator || syntax in constructors.

    Author:             christine@netscape.com
    Date:               12 november 1997
*/

    var SECTION = "proto_5";
    var VERSION = "JS1_3";
    var TITLE   = "Logical OR || in Constructors";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();

function Employee ( name, dept ) {
     this.name = name || "";
     this.dept = dept || "general";
}
function Manager () {
     this.reports = [];
}
Manager.prototype = new Employee();

function WorkerBee ( projs ) {
     this.projects = projs || new Array();
}
WorkerBee.prototype = new Employee();

function SalesPerson () {
    this.dept = "sales";
    this.quota = 100;
}
SalesPerson.prototype = new WorkerBee();

function Engineer ( machine ) {
    this.dept = "engineering";
    this.machine = machine || "";
}
Engineer.prototype = new WorkerBee();

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


    var pat = new Engineer( "indy" );

    var les = new Engineer();

    testcases[tc++] = new TestCase( SECTION,
                                    "var pat = new Engineer(\"indy\"); pat.name",
                                    "",
                                    pat.name );

    testcases[tc++] = new TestCase( SECTION,
                                    "pat.dept",
                                    "engineering",
                                    pat.dept );

    testcases[tc++] = new TestCase( SECTION,
                                    "pat.projects.length",
                                    0,
                                    pat.projects.length );

    testcases[tc++] = new TestCase( SECTION,
                                    "pat.machine",
                                    "indy",
                                    pat.machine );

    testcases[tc++] = new TestCase( SECTION,
                                    "pat.__proto__ == Engineer.prototype",
                                    true,
                                    pat.__proto__ == Engineer.prototype );

    testcases[tc++] = new TestCase( SECTION,
                                    "var les = new Engineer(); les.name",
                                    "",
                                    les.name );

    testcases[tc++] = new TestCase( SECTION,
                                    "les.dept",
                                    "engineering",
                                    les.dept );

    testcases[tc++] = new TestCase( SECTION,
                                    "les.projects.length",
                                    0,
                                    les.projects.length );

    testcases[tc++] = new TestCase( SECTION,
                                    "les.machine",
                                    "",
                                    les.machine );

    testcases[tc++] = new TestCase( SECTION,
                                    "les.__proto__ == Engineer.prototype",
                                    true,
                                    les.__proto__ == Engineer.prototype );


    test();