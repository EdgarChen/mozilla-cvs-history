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
 * The Original Code is OEone Calendar Code, released October 31st, 2001.
 *
 * The Initial Developer of the Original Code is
 * OEone Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s): Garth Smedley <garths@oeone.com>
 *                 Mike Potter <mikep@oeone.com>
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

var gAllEvents = new Array();
var CreateAlarmBox = true;
var gDateFormatter = new DateFormater();  // used to format dates and times
var kungFooDeathGripOnEventBoxes = new Array();

function onLoad()
{
   var args = window.arguments[0];
   
   if( args.calendarEvent )
   {
      onAlarmCall( args.calendarEvent );
   }
   
   if( "pendingEvents" in window )
   {
        //dump( "\n GETTING PENDING ___________________" );
        for( var i in window.pendingEvents )
        {
           gAllEvents[ gAllEvents.length ] = window.pendingEvents[i]; 

           //createAlarmBox( window.pendingEvents[ i ] );
        }
        buildEventBoxes();
   }
   
   setupOkCancelButtons( onOkButton, 0 );
}

function buildEventBoxes()
{
   //remove all the old event boxes.
   var EventContainer = document.getElementById( "event-container-rows" );
   var NumberOfChildElements = EventContainer.childNodes.length;
   for( i = 0; i < NumberOfChildElements; i++ )   
   {
      EventContainer.removeChild( EventContainer.lastChild );
   }
   
   //start at length - 10 or 0 if that is < 0

   var Start = gAllEvents.length - 10;
   if( Start < 0 )
      Start = 0;

   //build all event boxes again.
   for( var i = Start; i < gAllEvents.length; i++ )
   {
      createAlarmBox( gAllEvents[i] );   
   }
   
   //reset the text
   if( gAllEvents.length > 10 )
   {
      var TooManyDesc = document.getElementById( "too-many-alarms-description" );
      TooManyDesc.removeAttribute( "collapsed" );         
      TooManyDesc.setAttribute( "value", "You have "+ (gAllEvents.length )+" total alarms. We've shown you the last 10. Click Acknowledge All to clear them all." );
   }
   else
   {
      var TooManyDesc = document.getElementById( "too-many-alarms-description" );
      TooManyDesc.setAttribute( "collapsed", "true" );
   }
}

function onAlarmCall( Event )
{
   var AddToArray = true;
   //check and make sure that the event is not already in the array
   for( i = 0; i < gAllEvents.length; i++ )
   {
      if( gAllEvents[i].id == Event.id )
         AddToArray = false;   
   }
   if( AddToArray )
      gAllEvents[ gAllEvents.length ] = Event;

   buildEventBoxes();
}

function createAlarmBox( Event )
{
   var OuterBox = document.getElementsByAttribute( "name", "sample-row" )[0].cloneNode( true );
   OuterBox.removeAttribute( "name" );
   OuterBox.setAttribute( "id", Event.id );
   OuterBox.setAttribute( "eventbox", "true" );
   OuterBox.removeAttribute( "collapsed" );

   OuterBox.getElementsByAttribute( "name", "AcknowledgeButton" )[0].event = Event;
   
   OuterBox.getElementsByAttribute( "name", "AcknowledgeButton" )[0].setAttribute( "onclick", "acknowledgeAlarm( this.event );removeAlarmBox( this.event );" ); 
   
   kungFooDeathGripOnEventBoxes.push( OuterBox.getElementsByAttribute( "name", "AcknowledgeButton" )[0] );
   
   OuterBox.getElementsByAttribute( "name", "SnoozeButton" )[0].event = Event;
   
   OuterBox.getElementsByAttribute( "name", "SnoozeButton" )[0].setAttribute( "onclick", "snoozeAlarm( this.event );removeAlarmBox( this.event );" ); 
   
   kungFooDeathGripOnEventBoxes.push( OuterBox.getElementsByAttribute( "name", "SnoozeButton" )[0] );
   
   /*
   ** The first part of the box, the title and description
   */
   var EventTitle = document.createTextNode( Event.title );
   OuterBox.getElementsByAttribute( "name", "Title" )[0].appendChild( EventTitle );
   
   var EventDescription = document.createTextNode( Event.description );
   OuterBox.getElementsByAttribute( "name", "Description" )[0].appendChild( EventDescription );

   var startDate = new Date( Event.start.getTime() );

   var EventStartDate = document.createTextNode( getFormatedDate( startDate ) );
   OuterBox.getElementsByAttribute( "name", "StartDate" )[0].appendChild( EventStartDate );

   var EventStartTime = document.createTextNode( getFormatedTime( startDate ) );
   OuterBox.getElementsByAttribute( "name", "StartTime" )[0].appendChild( EventStartTime );

   /*
   ** 3rd part of the row: the number of times that alarm went off (sometimes hidden)
   */
   OuterBox.getElementsByAttribute( "name", "NumberOfTimes" )[0].setAttribute( "id", "description-"+Event.id );
   
   //document.getElementById( "event-container-rows" ).insertBefore( OuterBox, document.getElementById( "event-container-rows" ).childNodes[1] );
   document.getElementById( "event-container-rows" ).appendChild( OuterBox );
}

function removeAlarmBox( Event )
{
   
   //get the box for the event
   var EventAlarmBox = document.getElementById( Event.id );
   
   if( EventAlarmBox )
   {
      //remove the box from the body
      EventAlarmBox.parentNode.removeChild( EventAlarmBox );
   }
   
   //if there's no more events left, close the dialog
   EventAlarmBoxes = document.getElementsByAttribute( "eventbox", "true" );
      
   if( EventAlarmBoxes.length > 0 )
   {
      //there are still boxes left.
      return( false );
   }
   else
   {
      //close the dialog
      closeDialog();
      //return( true );
   }
}

function getArrayId( Event )
{
   for( i = 0; i < gAllEvents.length; i++ )
   {
      if( gAllEvents[i].id == Event.id )
         return( i );
   }
   
   return( false );

}

function onOkButton( )
{
   //this would acknowledge all the alarms
   for( i = 0; i < gAllEvents.length; i++ )
   {
      gAllEvents[i].lastAlarmAck = new Date();

      var calendarEventService = pendialog.getService( "org.penzilla.calendar" );
   
      gICalLib = calendarEventService.getICalLib();

      gICalLib.modifyEvent( gAllEvents[i] );
   }

   return( true );
}

function onCancelButton()
{
   //just close the dialog
   return( true );

}

function acknowledgeAlarm( Event )
{
   Event.lastAlarmAck = new Date();

   var calendarEventService = pendialog.getService( "org.penzilla.calendar" );
   
   gICalLib = calendarEventService.getICalLib();

   gICalLib.modifyEvent( Event );

   var Id = getArrayId( Event )

   gAllEvents.splice( Id, 1 );

   buildEventBoxes();
}

function snoozeAlarm( Event )
{
   var OuterBox = document.getElementById( Event.id );
   
   var SnoozeInteger = OuterBox.getElementsByAttribute( "name", "alarm-length-field" )[0].value;

   SnoozeInteger = parseInt( SnoozeInteger );

   var SnoozeUnits = document.getElementsByAttribute( "name", "alarm-length-units" )[0].value;

   var Now = new Date();

   Now = Now.getTime();

   var TimeToNextAlarm;
   
   switch (SnoozeUnits )
   {
      case "minutes":
         TimeToNextAlarm = 1000 * 60 * SnoozeInteger;
         break;

      case "hours":
         TimeToNextAlarm = 1000 * 60 * 60 * SnoozeInteger;
         break;

      case "days":
         TimeToNextAlarm = 1000 * 60 * 60 * 24 * SnoozeInteger;
         break;
   }
   
   var MSOfNextAlarm = Now + TimeToNextAlarm; //10 seconds.
   
   var DateObjOfNextAlarm = new Date( MSOfNextAlarm );
   
   Event.setSnoozeTime( DateObjOfNextAlarm );
   
   var calendarEventService = pendialog.getService( "org.penzilla.calendar" );
   
   gICalLib = calendarEventService.getICalLib();

   gICalLib.modifyEvent( Event );

   buildEventBoxes();
}

function getFormatedDate( date )
{
   var monthDayString = gDateFormatter.getFormatedDate( date );
   
   return  monthDayString + ", " + date.getFullYear();
}


/**
*   Take a Date object and return a displayable time string i.e.: 12:30 PM
*/

function getFormatedTime( date )
{
   var timeString = gDateFormatter.getFormatedTime( date );
   
   return timeString;
}
