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
 * Contributor(s): Mike Potter <mikep@oeone.com>
 *                 ArentJan Banck <ajbanck@planet.nl>
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

// Preferences observer object; implements nsIObserver
function calendarPrefObserver( CalendarPreferences )
{
   this.CalendarPreferences = CalendarPreferences;
   try {
     var pbi = rootPrefNode.QueryInterface(Components.interfaces.nsIPrefBranchInternal);
     pbi.addObserver("calendar", this, false);
  } catch(ex) {
    dump("Calendar: Failed to observe prefs: " + ex + "\n");
  }
}

calendarPrefObserver.prototype =
{
  domain: "calendar.",
  observe: function(subject, topic, prefName)
  {
     // when calendar pref was changed, we reinitialize 
     this.CalendarPreferences.loadPreferences();
     
     switch( prefName )
     {
        case "calendar.week.start":
         this.CalendarPreferences.calendarWindow.currentView.refresh();
        break

        case "calendar.date.format":
         this.CalendarPreferences.calendarWindow.currentView.refresh();
         unifinderRefesh();
        default:
        break;

     }
     
     //this causes Mozilla to freeze
     //firePendingEvents(); 
  }
}

function calendarPreferences( CalendarWindow )
{
   function getDefaultCategories()
   {  
      try {
         var categoriesStringBundle = srGetStrBundle("chrome://calendar/locale/categories.properties");
         return categoriesStringBundle.GetStringFromName("categories" );
      }
      catch(e) { return "" }
   }
   
   window.calendarPrefObserver = new calendarPrefObserver( this );
   
   this.calendarWindow = CalendarWindow;

   this.arrayOfPrefs = new Object();
   
   this.calendarPref = prefService.getBranch("calendar."); // preferences mozgest node
   
   // read prefs or set Defaults on the first run
  try {
    this.loadPreferences();  
  }
  catch(e) {
     this.calendarPref.setBoolPref( "alarms.show", true );
     this.calendarPref.setBoolPref( "alarms.playsound", false );
     this.calendarPref.setIntPref( "date.format", 0 );
     this.calendarPref.setIntPref( "week.start", 0 );
     this.calendarPref.setIntPref( "event.defaultlength", 60 );
     this.calendarPref.setIntPref( "alarms.defaultsnoozelength", 10 );
     this.calendarPref.setCharPref( "categories.names", getDefaultCategories() );
     this.loadPreferences();
  }
  
  
}

calendarPreferences.prototype.loadPreferences = function()
{
   this.arrayOfPrefs.showalarms = this.calendarPref.getBoolPref( "alarms.show" );

   this.arrayOfPrefs.alarmsplaysound = this.calendarPref.getBoolPref( "alarms.playsound" );

   this.arrayOfPrefs.dateformat = this.calendarPref.getIntPref( "date.format" );
   
   this.arrayOfPrefs.weekstart = this.calendarPref.getIntPref( "week.start" );

   this.arrayOfPrefs.defaulteventlength = this.calendarPref.getIntPref( "event.defaultlength" );

   this.arrayOfPrefs.defaultsnoozelength = this.calendarPref.getIntPref( "alarms.defaultsnoozelength" );

   this.arrayOfPrefs.categories = this.calendarPref.getCharPref( "categories.names" );
}

calendarPreferences.prototype.getPref = function( Preference )
{
   var ThisPref = eval( "this.arrayOfPrefs."+Preference );

   return( ThisPref );
}
