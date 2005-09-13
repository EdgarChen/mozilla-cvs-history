# -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is Mozilla.org Code.
#
# The Initial Developer of the Original Code is
# Netscape Communications Corporation.
# Portions created by the Initial Developer are Copyright (C) 2001
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Bob Lord <lord@netscape.com>
#   Terry Hayes <thayes@netscape.com>
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****

const nsPK11TokenDB = "@mozilla.org/security/pk11tokendb;1";
const nsIPK11TokenDB = Components.interfaces.nsIPK11TokenDB;
const nsIDialogParamBlock = Components.interfaces.nsIDialogParamBlock;
const nsPKCS11ModuleDB = "@mozilla.org/security/pkcs11moduledb;1";
const nsIPKCS11ModuleDB = Components.interfaces.nsIPKCS11ModuleDB;
const nsIPKCS11Slot = Components.interfaces.nsIPKCS11Slot;
const nsIPK11Token = Components.interfaces.nsIPK11Token;


var params;
var tokenName="";
var pw1;

function init()
{
  pw1 = document.getElementById("pw1");
	 	 
  process();
}


function process()
{
   var secmoddb = Components.classes[nsPKCS11ModuleDB].getService(nsIPKCS11ModuleDB);
   var bundle = document.getElementById("bundlePreferences");

   // If the token is unitialized, don't use the old password box.
   // Otherwise, do.

   var slot = secmoddb.findSlotByName(tokenName);
   if (slot) {
     var oldpwbox = document.getElementById("oldpw");
     var msgBox = document.getElementById("message");
     var status = slot.status;
     if (status == nsIPKCS11Slot.SLOT_UNINITIALIZED
         || status == nsIPKCS11Slot.SLOT_READY) {
      
       oldpwbox.setAttribute("hidden", "true");
       msgBox.setAttribute("value", bundle.getString("password_not_set")); 
       msgBox.setAttribute("hidden", "false");

       if (status == nsIPKCS11Slot.SLOT_READY) {
         oldpwbox.setAttribute("inited", "empty");
       } else {
         oldpwbox.setAttribute("inited", "true");
       }
      
       // Select first password field
       document.getElementById('pw1').focus();
    
     } else {
       // Select old password field
       oldpwbox.setAttribute("hidden", "false");
       msgBox.setAttribute("hidden", "true");
       oldpwbox.setAttribute("inited", "false");
       oldpwbox.focus();
     }
   }

  if (params) {
    // Return value 0 means "canceled"
    params.SetInt(1, 0);
  }
  
  checkPasswords();
}

function setPassword()
{
  var pk11db = Components.classes[nsPK11TokenDB].getService(nsIPK11TokenDB);
  var promptService = Components.classes["@mozilla.org/embedcomp/prompt-service;1"]
                                .getService(Components.interfaces.nsIPromptService);
  var token = pk11db.findTokenByName(tokenName);
  dump("*** TOKEN!!!! (name = |" + token + "|\n");

  var oldpwbox = document.getElementById("oldpw");
  var initpw = oldpwbox.getAttribute("inited");
  var bundle = document.getElementById("bundlePreferences");
  
  var success = false;
  
  if (initpw == "false" || initpw == "empty") {
    try {
      var oldpw = "";
      var passok = 0;
      
      if (initpw == "empty") {
        passok = 1;
      } else {
        oldpw = oldpwbox.value;
        passok = token.checkPassword(oldpw);
      }
      
      if (passok) {
        if (initpw == "empty" && pw1.value == "") {
          // This makes no sense that we arrive here, 
          // we reached a case that should have been prevented by checkPasswords.
        } else {
          if (pw1.value == "") {
            var secmoddb = Components.classes[nsPKCS11ModuleDB].getService(nsIPKCS11ModuleDB);
            if (secmoddb.isFIPSEnabled) {
              // empty passwords are not allowed in FIPS mode
              promptService.alert(window,
                                  bundle.getString("pw_change_failed_title"),
                                  bundle.getString("pw_change2empty_in_fips_mode"));
              passok = 0;
            }
          }
          if (passok) {
            token.changePassword(oldpw, pw1.value);
            if (pw1.value == "") {
              promptService.alert(window,
                                  bundle.getString("pw_change_success_title"),
                                  bundle.getString("pw_erased_ok") 
                                  + " " + bundle.getString("pw_empty_warning"));
            } else {
              promptService.alert(window,
                                  bundle.getString("pw_change_success_title"),
                                  bundle.getString("pw_change_ok"));
            }
            success = true;
          }
        }
      } else {
        oldpwbox.focus();
        oldpwbox.setAttribute("value", "");
        promptService.alert(window,
                            bundle.getString("pw_change_failed_title"),
                            bundle.getString("incorrect_pw"));
      }
    } catch (e) {
      promptService.alert(window,
                          bundle.getString("pw_change_failed_title"),
                          bundle.getString("failed_pw_change"));
    }
  } else {
    token.initPassword(pw1.value);
    if (pw1.value == "") {
      promptService.alert(window,
                          bundle.getString("pw_change_success_title"),
                          bundle.getString("pw_not_wanted")
                          + " " + bundle.getString("pw_empty_warning"));
    }
    success = true;
  }

  // Terminate dialog
  if (success)
    window.close();
}

function setPasswordStrength()
{
// Here is how we weigh the quality of the password
// number of characters
// numbers
// non-alpha-numeric chars
// upper and lower case characters

  var pw=document.getElementById('pw1').value;

//length of the password
  var pwlength=(pw.length);
  if (pwlength>5)
    pwlength=5;


//use of numbers in the password
  var numnumeric = pw.replace (/[0-9]/g, "");
  var numeric=(pw.length - numnumeric.length);
  if (numeric>3)
    numeric=3;

//use of symbols in the password
  var symbols = pw.replace (/\W/g, "");
  var numsymbols=(pw.length - symbols.length);
  if (numsymbols>3)
    numsymbols=3;

//use of uppercase in the password
  var numupper = pw.replace (/[A-Z]/g, "");
  var upper=(pw.length - numupper.length);
  if (upper>3)
    upper=3;


  var pwstrength=((pwlength*10)-20) + (numeric*10) + (numsymbols*15) + (upper*10);

  // make sure we're give a value between 0 and 100
  if ( pwstrength < 0 ) {
    pwstrength = 0;
  }
  
  if ( pwstrength > 100 ) {
    pwstrength = 100;
  }

  var mymeter=document.getElementById('pwmeter');
  mymeter.value = pwstrength;

  return;
}

function checkPasswords()
{
  var pw1=document.getElementById('pw1').value;
  var pw2=document.getElementById('pw2').value;
  var ok=document.documentElement.getButton("accept");

  var oldpwbox = document.getElementById("oldpw");
  if (oldpwbox) {
    var initpw = oldpwbox.getAttribute("inited");

    if (initpw == "empty" && pw1 == "") {
      // The token has already been initialized, therefore this dialog
      // was called with the intention to change the password.
      // The token currently uses an empty password.
      // We will not allow changing the password from empty to empty.
      ok.setAttribute("disabled","true");
      return;
    }
  }

  if (pw1 == pw2){
    ok.setAttribute("disabled","false");
  } else
  {
    ok.setAttribute("disabled","true");
  }

}
