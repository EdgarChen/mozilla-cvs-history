/*
* The contents of this file are subject to the Netscape Public
* License Version 1.1 (the "License"); you may not use this file
* except in compliance with the License. You may obtain a copy of
* the License at http://www.mozilla.org/NPL/
*
* Software distributed under the License is distributed on an "AS  IS"
* basis, WITHOUT WARRANTY OF ANY KIND, either expressed
* or implied. See the License for the specific language governing
* rights and limitations under the License.
*
* The Original Code is mozilla.org code.
*
* The Initial Developer of the Original Code is Netscape
* Communications Corporation.  Portions created by Netscape are
* Copyright (C) 1998 Netscape Communications Corporation.
* All Rights Reserved.
*
* Contributor(s): pschwartau@netscape.com
* Date: 22 June 2001
*
* SUMMARY:  Regression test for Bugzilla bug 87231:
* "Regular expression /(A)?(A.*)/ picks 'A' twice"
*
* See http://bugzilla.mozilla.org/show_bug.cgi?id=87231
* Key case:
*            pattern = /^(A)?(A.*)$/;
*            string = 'A';
*            expectedmatch = Array( 'A',  '',  'A' );
*
* We expect the 1st subexpression (A)? NOT to consume the single 'A'.
* Recall the "?" means "match 0 or 1 times". It should NOT do greedy matching;
* it should match 0 times instead of 1. It thus allows the 2nd subexpression to
* make the only match it can: the single 'A'. This is the only way there can be
* a successful global match...
*/
//-------------------------------------------------------------------------------------------------
var i = 0;
var bug = 87231;
var cnEmptyString = '';
var summary = 'Testing regular expression /(A)?(A.*)/';
var pattern = ''; var patterns = new Array();
var string = ''; var strings = new Array();
var actualmatch = '';  var actualmatches = new Array();
var expectedmatch = ''; var expectedmatches = new Array();


pattern = /^(A)?(A.*)$/;
    string = 'AAA';
    actualmatch = string.match(pattern);
    expectedmatch = Array(string,  'A',  'AA');
    addThis();

    string = 'AA';
    actualmatch = string.match(pattern);
    expectedmatch = Array(string,  'A',  'A');
    addThis();

    string = 'A';
    actualmatch = string.match(pattern);
    expectedmatch = Array(string, cnEmptyString,  'A');
    addThis();



//-------------------------------------------------------------------------------------------------
test();
//-------------------------------------------------------------------------------------------------



function addThis()
{
  patterns[i] = pattern;
  strings[i] = string;
  actualmatches[i] = actualmatch;
  expectedmatches[i] = expectedmatch;
  i++;
}


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  testRegExp(patterns, strings, actualmatches, expectedmatches);
  exitFunc ('test');
}
