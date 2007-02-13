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
    var SECTION = "15.7.4.2-1";
    var VERSION = "ECMA_1";
    startTest();
    var testcases = getTestCases();

    writeHeaderToLog( SECTION + " Number.prototype.toString()");
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    var thisError="no error thrown";
    try{
       	thisError = Number.prototype.toString();
    }
    catch(e)
	{
        thisError=e.toString();
    }finally{
    	array[item++] = new TestCase(SECTION, "Number.prototype.toString()",       
											  "0",       
											  thisError);
    }

    array[item++] = new TestCase(SECTION, "typeof(Number.prototype.toString())", "string",      typeof(Number.prototype.toString()) );


    thisError="no error thrown";
    try{
       	s = Number.prototype.toString;
       	o = new Number(); 
    	o.toString = s;
    }
    catch(e1)
	{
        thisError=e1.toString();
    }finally{
    	array[item++] = new TestCase(SECTION, "s = Number.prototype.toString",       
											"ReferenceError: Error #1056",       
											referenceError(thisError));
    }
 
    thisError="no error thrown";
    try{
        s = Number.prototype.toString;
        o = new Number(1); 
        o.toString = s;
    }
    catch(e2){
        thisError=e2.toString();
    }finally{
    	array[item++] = new TestCase(SECTION, "s = Number.prototype.toString",       
												"ReferenceError: Error #1056",       
												referenceError(thisError));
    }
  
    thisError="no error thrown";
    try{
        s = Number.prototype.toString;
        o = new Number(-1); 
        o.toString = s; 
    }
    catch(e3){
        thisError=e3.toString();
    }finally{
        array[item++] = new TestCase(SECTION, "s = Number.prototype.toString",       
											"ReferenceError: Error #1056",       
											referenceError(thisError));
    }
  
    var MYNUM = new Number(255);
    array[item++] = new TestCase(SECTION, "var MYNUM = new Number(255); MYNUM.toString(10)",          "255",      MYNUM.toString(10) );

    var MYNUM = new Number(Number.NaN);
    array[item++] = new TestCase(SECTION, "var MYNUM = new Number(Number.NaN); MYNUM.toString(10)",   "NaN",      MYNUM.toString(10) );

    var MYNUM = new Number(Infinity);
    array[item++] = new TestCase(SECTION, "var MYNUM = new Number(Infinity); MYNUM.toString(10)",   "Infinity",   MYNUM.toString(10) );

    var MYNUM = new Number(-Infinity);
    array[item++] = new TestCase(SECTION, "var MYNUM = new Number(-Infinity); MYNUM.toString(10)",   "-Infinity", MYNUM.toString(10) );

    return ( array );
}
