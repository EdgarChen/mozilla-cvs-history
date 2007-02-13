/* -*- Mode: java; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * ***** BEGIN LICENSE BLOCK *****
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
 * The Original Code is Rhino code, released
 * May 6, 1999.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1997-2000
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Igor Bukanov
 *   Ethan Hugg
 *   Milen Nankov
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

START("13.4.3.8 - XML.setSettings(settings)");

// a) called with a settings object
var settings = XML.settings();

settings.ignoreComments = false;
settings.ignoreProcessingInstructions = false;
settings.ignoreWhitespace = false;
settings.prettyPrinting = false;
settings.prettyIndent = 4;

AddTestCase( "XML.settings().ignoreComments; ", true, (XML.settings().ignoreComments) );
AddTestCase( "XML.settings().ignoreProcessingInstructions; ", true, (XML.settings().ignoreProcessingInstructions) );
AddTestCase( "XML.settings().ignoreWhitespace; ", true, (XML.settings().ignoreWhitespace) );
AddTestCase( "XML.settings().prettyPrinting; ", true, (XML.settings().prettyPrinting) );
AddTestCase( "XML.settings().prettyIndent; ", 2, (XML.settings().prettyIndent) );

XML.setSettings (settings);

AddTestCase( "XML.settings().ignoreComments; ", false, (XML.settings().ignoreComments) );
AddTestCase( "XML.settings().ignoreProcessingInstructions; ", false, (XML.settings().ignoreProcessingInstructions) );
AddTestCase( "XML.settings().ignoreWhitespace; ", false, (XML.settings().ignoreWhitespace) );
AddTestCase( "XML.settings().prettyPrinting; ", false, (XML.settings().prettyPrinting) );
AddTestCase( "XML.settings().prettyIndent; ", 4, (XML.settings().prettyIndent) );

// setting null restores defaults
XML.setSettings (null);

AddTestCase( "XML.settings(null); XML.settings().ignoreComments; ", true, (XML.settings().ignoreComments) );
AddTestCase( "XML.settings(null); XML.settings().ignoreProcessingInstructions; ", true, (XML.settings().ignoreProcessingInstructions) );
AddTestCase( "XML.settings(null); XML.settings().ignoreWhitespace; ", true, (XML.settings().ignoreWhitespace) );
AddTestCase( "XML.settings(null); XML.settings().prettyPrinting; ", true, (XML.settings().prettyPrinting) );
AddTestCase( "XML.settings(null); XML.settings().prettyIndent; ", 2, (XML.settings().prettyIndent) );

XML.setSettings (settings);
// does setting a bogus value restore defaults? No.
XML.setSettings ([1, 2, 3, 4]);

AddTestCase( "XML.settings().ignoreComments; ", false, (XML.settings().ignoreComments) );
AddTestCase( "XML.settings().ignoreProcessingInstructions; ", false, (XML.settings().ignoreProcessingInstructions) );
AddTestCase( "XML.settings().ignoreWhitespace; ", false, (XML.settings().ignoreWhitespace) );
AddTestCase( "XML.settings().prettyPrinting; ", false, (XML.settings().prettyPrinting) );
AddTestCase( "XML.settings().prettyIndent; ", 4, (XML.settings().prettyIndent) );

// does setting a bogus value restore defaults? No.
XML.setSettings (5);

AddTestCase( "XML.settings().ignoreComments; ", false, (XML.settings().ignoreComments) );
AddTestCase( "XML.settings().ignoreProcessingInstructions; ", false, (XML.settings().ignoreProcessingInstructions) );
AddTestCase( "XML.settings().ignoreWhitespace; ", false, (XML.settings().ignoreWhitespace) );
AddTestCase( "XML.settings().prettyPrinting; ", false, (XML.settings().prettyPrinting) );
AddTestCase( "XML.settings().prettyIndent; ", 4, (XML.settings().prettyIndent) );

// does setting a bogus value restore defaults? No.
XML.setSettings ("");

AddTestCase( "XML.setSettings (\"\"); XML.settings().ignoreComments; ", false, (XML.settings().ignoreComments) );
AddTestCase( "XML.setSettings (\"\"); XML.settings().ignoreProcessingInstructions; ", false, (XML.settings().ignoreProcessingInstructions) );
AddTestCase( "XML.setSettings (\"\"); XML.settings().ignoreWhitespace; ", false, (XML.settings().ignoreWhitespace) );
AddTestCase( "XML.setSettings (\"\"); XML.settings().prettyPrinting; ", false, (XML.settings().prettyPrinting) );
AddTestCase( "XML.setSettings (\"\"); XML.settings().prettyIndent; ", 4, (XML.settings().prettyIndent) );

// this restores defaults
XML.setSettings (undefined);

AddTestCase( "XML.setSettings (undefined); XML.settings().ignoreComments; ", true, (XML.settings().ignoreComments) );
AddTestCase( "XML.setSettings (undefined); XML.settings().ignoreProcessingInstructions; ", true, (XML.settings().ignoreProcessingInstructions) );
AddTestCase( "XML.setSettings (undefined); XML.settings().ignoreWhitespace; ", true, (XML.settings().ignoreWhitespace) );
AddTestCase( "XML.setSettings (undefined); XML.settings().prettyPrinting; ", true, (XML.settings().prettyPrinting) );
AddTestCase( "XML.setSettings (undefined); XML.settings().prettyIndent; ", 2, (XML.settings().prettyIndent) );

// this restore defaults
XML.setSettings (settings);
XML.setSettings ();

AddTestCase( "XML.setSettings (); XML.settings().ignoreComments; ", true, (XML.settings().ignoreComments) );
AddTestCase( "XML.setSettings (); XML.settings().ignoreProcessingInstructions; ", true, (XML.settings().ignoreProcessingInstructions) );
AddTestCase( "XML.setSettings (); XML.settings().ignoreWhitespace; ", true, (XML.settings().ignoreWhitespace) );
AddTestCase( "XML.setSettings (); XML.settings().prettyPrinting; ", true, (XML.settings().prettyPrinting) );
AddTestCase( "XML.setSettings (); XML.settings().prettyIndent; ", 2, (XML.settings().prettyIndent) );

END();
