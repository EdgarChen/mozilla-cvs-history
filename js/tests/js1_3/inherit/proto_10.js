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
    File Name:          proto_10.js
    Section:
    Description:        Determining Instance Relationships

    This tests Object Hierarchy and Inheritance, as described in the document
    Object Hierarchy and Inheritance in JavaScript, last modified on 12/18/97
    15:19:34 on http://devedge.netscape.com/.  Current URL:
    http://devedge.netscape.com/docs/manuals/communicator/jsobj/contents.htm

    This tests the syntax ObjectName.prototype = new PrototypeObject using the
    Employee example in the document referenced above.

    Author:             christine@netscape.com
    Date:               12 november 1997
*/

    var SECTION = "proto_10";
    var VERSION = "JS1_3";
    var TITLE   = "Determining Instance Relationships";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();

function InstanceOf( object, constructor ) {
    while ( object != null ) {
        if ( object == constructor.prototype ) {
            return true;
        }
        object = object.__proto__;
    }
    return false;
}
function Employee ( name, dept ) {
     this.name = name || "";
     this.dept = dept || "general";
}

function Manager () {
     this.reports = [];
}
Manager.prototype = new Employee();

function WorkerBee ( name, dept, projs ) {
    this.base = Employee;
    this.base( name, dept)
    this.projects = projs || new Array();
}
WorkerBee.prototype = new Employee();

function SalesPerson () {
    this.dept = "sales";
    this.quota = 100;
}
SalesPerson.prototype = new WorkerBee();

function Engineer ( name, projs, machine ) {
    this.base = WorkerBee;
    this.base( name, "engineering", projs )
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
    var pat = new Engineer()

    testcases[tc++] = new TestCase( SECTION,
                                    "pat.__proto__ == Engineer.prototype",
                                    true,
                                    pat.__proto__ == Engineer.prototype );

    testcases[tc++] = new TestCase( SECTION,
                                    "pat.__proto__.__proto__ == WorkerBee.prototype",
                                    true,
                                    pat.__proto__.__proto__ == WorkerBee.prototype );

    testcases[tc++] = new TestCase( SECTION,
                                    "pat.__proto__.__proto__.__proto__ == Employee.prototype",
                                    true,
                                    pat.__proto__.__proto__.__proto__ == Employee.prototype );

    testcases[tc++] = new TestCase( SECTION,
                                    "pat.__proto__.__proto__.__proto__.__proto__ == Object.prototype",
                                    true,
                                    pat.__proto__.__proto__.__proto__.__proto__ == Object.prototype );

    testcases[tc++] = new TestCase( SECTION,
                                    "pat.__proto__.__proto__.__proto__.__proto__.__proto__ == null",
                                    true,
                                    pat.__proto__.__proto__.__proto__.__proto__.__proto__ == null );


    testcases[tc++] = new TestCase( SECTION,
                                    "InstanceOf( pat, Engineer )",
                                    true,
                                    InstanceOf( pat, Engineer ) );

    testcases[tc++] = new TestCase( SECTION,
                                    "InstanceOf( pat, WorkerBee )",
                                    true,
                                    InstanceOf( pat, WorkerBee ) );

    testcases[tc++] = new TestCase( SECTION,
                                    "InstanceOf( pat, Employee )",
                                    true,
                                    InstanceOf( pat, Employee ) );

    testcases[tc++] = new TestCase( SECTION,
                                    "InstanceOf( pat, Object )",
                                    true,
                                    InstanceOf( pat, Object ) );

    testcases[tc++] = new TestCase( SECTION,
                                    "InstanceOf( pat, SalesPerson )",
                                    false,
                                    InstanceOf ( pat, SalesPerson ) );
    test();
