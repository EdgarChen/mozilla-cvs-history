# -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
# The Original Code is the Thunderbird Preferences System.
#
# The Initial Developer of the Original Code is
# Scott MacGregor.
# Portions created by the Initial Developer are Copyright (C) 2005
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Scott MacGregor <mscott@mozilla.org>
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

var gGeneralPane = {
  mPane: null,

  init: function ()
  {
    this.mPane = document.getElementById("paneGeneral"); 
    
    this.startPageCheck();
  },
  
#ifdef HAVE_SHELL_SERVICE
  /**
   * Checks whether Thunderbird is currently registered with the operating
   * system as the default app for mail, rss and news.  If Thunderbird is not currently the
   * default app, the user is given the option of making it the default for each type;
   * otherwise, the user is informed that Thunderbird is already the default.
   */
  checkDefaultNow: function (aAppType) 
  {   
    var nsIShellService = Components.interfaces.nsIShellService;
    var shellSvc;
    try {
      shellSvc = Components.classes["@mozilla.org/mail/shell-service;1"].getService(nsIShellService);
    } catch (ex) { return; }
    
    // if we are already the default for all the types we handle, then alert the user.
    if (shellSvc.isDefaultClient(false, nsIShellService.MAIL | nsIShellService.NEWS | nsIShellService.RSS))
    {
      var brandBundle = document.getElementById("bundleBrand");
      var shellBundle = document.getElementById("bundleShell");
      var brandShortName = brandBundle.getString("brandShortName");
      var promptTitle = shellBundle.getString("alreadyDefaultClientTitle");
      var promptMessage;
      const IPS = Components.interfaces.nsIPromptService;
      var psvc = Components.classes["@mozilla.org/embedcomp/prompt-service;1"]
                           .getService(IPS);

        promptMessage = shellBundle.getFormattedString("alreadyDefault", [brandShortName]);
        psvc.alert(window, promptTitle, promptMessage);
    }
    else
    {
      // otherwise, bring up the default client dialog
      window.openDialog("chrome://messenger/content/defaultClientDialog.xul", "Default Client", 
                        "modal,centerscreen,chrome,resizable=no");
    }
  },
#endif

  startPageCheck: function() 
  {
    document.getElementById("mailnewsStartPageUrl").disabled = !document.getElementById("mailnewsStartPageEnabled").checked;
  },
  
  setHomePageToDefaultPage: function ()
  {
    var prefService = Components.classes["@mozilla.org/preferences-service;1"]
                                .getService(Components.interfaces.nsIPrefService);
    var pref = prefService.getDefaultBranch(null);
    var url = pref.getComplexValue("mailnews.start_page.url",
                                   Components.interfaces.nsIPrefLocalizedString).data;
    var startPageUrlField = document.getElementById("mailnewsStartPageUrl");
    startPageUrlField.value = url;
    
    this.mPane.userChangedValue(startPageUrlField);
  },

  showConnections: function ()
  {
    document.documentElement
            .openSubDialog("chrome://messenger/content/preferences/connection.xul",
                           "", null);
  },

  customizeNewMailBehavior: function()
  {
    document.documentElement
            .openSubDialog("chrome://messenger/content/preferences/notifications.xul",
                            "", null);
  }
};
