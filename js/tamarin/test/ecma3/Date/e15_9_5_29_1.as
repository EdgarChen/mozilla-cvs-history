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
    var SECTION = "15.9.5.29-1";
    var VERSION = "ECMA_1";
    startTest();

    writeHeaderToLog( SECTION + " Date.prototype.setUTCMinutes( min [, sec, ms] )");

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;
    
    addNewTestCase( 0, 0, void 0, void 0,
                    "TDATE = new Date(0);(TDATE).setUTCMinutes(0);TDATE",
                    UTCDateFromTime(SetUTCMinutes(0,0,0,0)),
                    LocalDateFromTime(SetUTCMinutes(0,0,0,0)) );

    addNewTestCase( 28800000, 59, 59, void 0,
                    "TDATE = new Date(28800000);(TDATE).setUTCMinutes(59,59);TDATE",
                    UTCDateFromTime(SetUTCMinutes(28800000,59,59)),
                    LocalDateFromTime(SetUTCMinutes(28800000,59,59)) );

    addNewTestCase( 28800000, 59, 59, 999,
                    "TDATE = new Date(28800000);(TDATE).setUTCMinutes(59,59,999);TDATE",
                    UTCDateFromTime(SetUTCMinutes(28800000,59,59,999)),
                    LocalDateFromTime(SetUTCMinutes(28800000,59,59,999)) );

    addNewTestCase( 28800000, 59, void 0, void 0,
                    "TDATE = new Date(28800000);(TDATE).setUTCMinutes(59);TDATE",
                    UTCDateFromTime(SetUTCMinutes(28800000,59)),
                    LocalDateFromTime(SetUTCMinutes(28800000,59)) );

    addNewTestCase( 28800000, -480, 0, 0,
                    "TDATE = new Date(28800000);(TDATE).setUTCMinutes(-480);TDATE",
                    UTCDateFromTime(SetUTCMinutes(28800000,-480)),
                    LocalDateFromTime(SetUTCMinutes(28800000,-480)) );

    addNewTestCase( 946684800000, 1234567, void 0, void 0,
                    "TDATE = new Date(946684800000);(TDATE).setUTCMinutes(1234567);TDATE",
                    UTCDateFromTime(SetUTCMinutes(946684800000,1234567)),
                    LocalDateFromTime(SetUTCMinutes(946684800000,1234567)) );

    addNewTestCase( -2208988800000, 59, 999, void 0,
                    "TDATE = new Date(-2208988800000);(TDATE).setUTCMinutes(59,999);TDATE",
                    UTCDateFromTime(SetUTCMinutes(-2208988800000,59,999)),
                    LocalDateFromTime(SetUTCMinutes(-2208988800000,59,999)) );
/*
    addNewTestCase( "TDATE = new Date(-2208988800000);(TDATE).setUTCMilliseconds(123456789);TDATE",
                    UTCDateFromTime(SetUTCMilliseconds(-2208988800000,123456789)),
                    LocalDateFromTime(SetUTCMilliseconds(-2208988800000,123456789)) );

    addNewTestCase( "TDATE = new Date(-2208988800000);(TDATE).setUTCMilliseconds(123456);TDATE",
                    UTCDateFromTime(SetUTCMilliseconds(-2208988800000,123456)),
                    LocalDateFromTime(SetUTCMilliseconds(-2208988800000,123456)) );

    addNewTestCase( "TDATE = new Date(-2208988800000);(TDATE).setUTCMilliseconds(-123456);TDATE",
                    UTCDateFromTime(SetUTCMilliseconds(-2208988800000,-123456)),
                    LocalDateFromTime(SetUTCMilliseconds(-2208988800000,-123456)) );

    addNewTestCase( "TDATE = new Date(0);(TDATE).setUTCMilliseconds(-999);TDATE",
                    UTCDateFromTime(SetUTCMilliseconds(0,-999)),
                    LocalDateFromTime(SetUTCMilliseconds(0,-999)) );
*/

    
    function addNewTestCase( time, min, sec, ms, DateString, UTCDate, LocalDate) {
        var DateCase = new Date( time );
    
        if ( sec == void 0 ) {
            DateCase.setUTCMinutes( min );
        } else {
            if ( ms == void 0 ) {
                DateCase.setUTCMinutes( min, sec );
            } else {
                DateCase.setUTCMinutes( min, sec, ms );
            }
        }
    
    //    fixed_year = ( ExpectDate.year >=1900 || ExpectDate.year < 2000 ) ? ExpectDate.year - 1900 : ExpectDate.year;
    
        array[item++] = new TestCase( SECTION, DateString+".getTime()",             UTCDate.value,       DateCase.getTime() );
        array[item++] = new TestCase( SECTION, DateString+".valueOf()",             UTCDate.value,       DateCase.valueOf() );
    
        array[item++] = new TestCase( SECTION, DateString+".getUTCFullYear()",      UTCDate.year,    DateCase.getUTCFullYear() );
        array[item++] = new TestCase( SECTION, DateString+".getUTCMonth()",         UTCDate.month,  DateCase.getUTCMonth() );
        array[item++] = new TestCase( SECTION, DateString+".getUTCDate()",          UTCDate.date,   DateCase.getUTCDate() );
    //    array[item++] = new TestCase( SECTION, DateString+".getUTCDay()",           UTCDate.day,    DateCase.getUTCDay() );
        array[item++] = new TestCase( SECTION, DateString+".getUTCHours()",         UTCDate.hours,  DateCase.getUTCHours() );
        array[item++] = new TestCase( SECTION, DateString+".getUTCMinutes()",       UTCDate.minutes,DateCase.getUTCMinutes() );
        array[item++] = new TestCase( SECTION, DateString+".getUTCSeconds()",       UTCDate.seconds,DateCase.getUTCSeconds() );
        array[item++] = new TestCase( SECTION, DateString+".getUTCMilliseconds()",  UTCDate.ms,     DateCase.getUTCMilliseconds() );
    
        array[item++] = new TestCase( SECTION, DateString+".getFullYear()",         LocalDate.year,       DateCase.getFullYear() );
        array[item++] = new TestCase( SECTION, DateString+".getMonth()",            LocalDate.month,      DateCase.getMonth() );
        array[item++] = new TestCase( SECTION, DateString+".getDate()",             LocalDate.date,       DateCase.getDate() );
    //    array[item++] = new TestCase( SECTION, DateString+".getDay()",              LocalDate.day,        DateCase.getDay() );
        array[item++] = new TestCase( SECTION, DateString+".getHours()",            LocalDate.hours,      DateCase.getHours() );
        array[item++] = new TestCase( SECTION, DateString+".getMinutes()",          LocalDate.minutes,    DateCase.getMinutes() );
        array[item++] = new TestCase( SECTION, DateString+".getSeconds()",          LocalDate.seconds,    DateCase.getSeconds() );
        array[item++] = new TestCase( SECTION, DateString+".getMilliseconds()",     LocalDate.ms,         DateCase.getMilliseconds() );
    
        //DateCase.toString = Object.prototype.toString;
    
        array[item++] = new TestCase( SECTION,
                                          DateString+".toString=Object.prototype.toString;"+DateString+".toString()",
                                          "[object Date]",
                                          (DateCase.myToString= Object.prototype.toString, DateCase.myToString()) );
    }
    return array;
}

function MyDate() {
    this.year = 0;
    this.month = 0;
    this.date = 0;
    this.hours = 0;
    this.minutes = 0;
    this.seconds = 0;
    this.ms = 0;
}
function LocalDateFromTime(t) {
    t = LocalTime(t);
    return ( MyDateFromTime(t) );
}
function UTCDateFromTime(t) {
 return ( MyDateFromTime(t) );
}
function MyDateFromTime( t ) {
    var d = new MyDate();
    d.year = YearFromTime(t);
    d.month = MonthFromTime(t);
    d.date = DateFromTime(t);
    d.hours = HourFromTime(t);
    d.minutes = MinFromTime(t);
    d.seconds = SecFromTime(t);
    d.ms = msFromTime(t);

    d.time = MakeTime( d.hours, d.minutes, d.seconds, d.ms );
    d.value = TimeClip( MakeDate( MakeDay( d.year, d.month, d.date ), d.time ) );
    d.day = WeekDay( d.value );

    return (d);
}
function SetUTCMinutes( t, min, sec, ms ) {
    var TIME = t;
    var MIN =  Number(min);
    var SEC  = ( sec == void 0) ? SecFromTime(TIME) : Number(sec);
    var MS   = ( ms == void 0 ) ? msFromTime(TIME)  : Number(ms);
    var RESULT5 = MakeTime( HourFromTime( TIME ),
                            MIN,
                            SEC,
                            MS );
    return ( TimeClip(MakeDate(Day(TIME),RESULT5)) );
}
