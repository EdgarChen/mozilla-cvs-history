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
 * The Original Code is the Application Update Service.
 *
 * The Initial Developer of the Original Code is
 * Robert Strong <robert.bugzilla@gmail.com>.
 *
 * Portions created by the Initial Developer are Copyright (C) 2008
 * the Mozilla Foundation <http://www.mozilla.org/>. All Rights Reserved.
 *
 * Contributor(s):
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
 * ***** END LICENSE BLOCK *****
 */

// const Cc, Ci, and Cr are defined in netwerk/test/httpserver/httpd.js so we
// need to define unique ones.
const AUS_Cc = Components.classes;
const AUS_Ci = Components.interfaces;
const AUS_Cr = Components.results;

const NS_APP_PROFILE_DIR_STARTUP   = "ProfDS";
const NS_APP_USER_PROFILE_50_DIR   = "ProfD";
const NS_GRE_DIR                   = "GreD";
const NS_XPCOM_CURRENT_PROCESS_DIR = "XCurProcD"
const XRE_UPDATE_ROOT_DIR          = "UpdRootD";

const PREF_APP_UPDATE_URL_OVERRIDE      = "app.update.url.override";
const PREF_APP_UPDATE_SHOW_INSTALLED_UI = "app.update.showInstalledUI";

const URI_UPDATES_PROPERTIES = "chrome://mozapps/locale/update/updates.properties";
const gUpdateBundle = AUS_Cc["@mozilla.org/intl/stringbundle;1"].
                      getService(AUS_Ci.nsIStringBundleService).
                      createBundle(URI_UPDATES_PROPERTIES);

const STATE_NONE            = "null";
const STATE_DOWNLOADING     = "downloading";
const STATE_PENDING         = "pending";
const STATE_APPLYING        = "applying";
const STATE_SUCCEEDED       = "succeeded";
const STATE_DOWNLOAD_FAILED = "download-failed";
const STATE_FAILED          = "failed";

const FILE_UPDATES_DB     = "updates.xml";
const FILE_UPDATE_ACTIVE  = "active-update.xml";

const MODE_RDONLY   = 0x01;
const MODE_WRONLY   = 0x02;
const MODE_CREATE   = 0x08;
const MODE_APPEND   = 0x10;
const MODE_TRUNCATE = 0x20;

const PERMS_FILE      = 0644;
const PERMS_DIRECTORY = 0755;

const URL_HOST   = "http://localhost:4444/"
const DIR_DATA   = "data"
const URL_PREFIX = URL_HOST + DIR_DATA + "/";

const REL_TEST_PATH = "toolkit/mozapps/update/test/unit/";

const POST_UPDATE_CONTRACTID = "@mozilla.org/updates/post-update;1";

var gDirSvc = AUS_Cc["@mozilla.org/file/directory_service;1"].
              getService(AUS_Ci.nsIProperties);

var gAUS;
var gUpdateChecker;
var gUpdateManager;

var gTestserver;

var gCheckFunc;
var gResponseBody;
var gRequestURL;
var gUpdateCount;
var gUpdates;
var gStatusCode;
var gStatusText;


function getPrefBranch() {
  return AUS_Cc["@mozilla.org/preferences;1"].getService(AUS_Ci.nsIPrefBranch);
}

/**
 * Nulls out the most commonly used global vars used by tests as appropriate.
 * This was moved here from the tail file due to check-interactive executing
 * the tail file prior to _execute_test(); (bug 384339). It hasn't been moved
 * back since it is easier to comment out the call to cleanUp when needed.
 */
function cleanUp() {
  // Always call app update's observe method passing xpcom-shutdown to test that
  // the shutdown of app update runs without throwing or leaking. The observer
  // method is used directly instead of calling notifyObservers so components
  // outside of the scope of this test don't assert and thereby cause app update
  // tests to fail.
  if (gAUS)
    gAUS.observe(null, "xpcom-shutdown", "");

  removeUpdateDirsAndFiles();
  gDirSvc.unregisterProvider(gDirProvider);

  gUpdateManager = null;
  gUpdateChecker = null;
  gAUS           = null;
  gTestserver    = null;
}

/**
 * Initializes the most commonly used global vars used by tests and
 * nsIApplicationUpdateService
 */
function startAUS() {
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1.0", "2.0");

  var pb = getPrefBranch();

  // Don't display UI for a successful installation. Some apps may not set this
  // pref to false like Firefox does.
  pb.setBoolPref(PREF_APP_UPDATE_SHOW_INSTALLED_UI, false);
  // Enable Update logging
  pb.setBoolPref("app.update.log.all", true);
  // Lessens the noise in the logs when using Update Service logging
  pb.setBoolPref("app.update.enabled", false);
  pb.setBoolPref("extensions.blocklist.enabled", false);
  pb.setBoolPref("extensions.update.enabled", false);
  pb.setBoolPref("browser.search.update", false);
  pb.setBoolPref("browser.microsummary.updateGenerators", false);

  // Override nsPostUpdateWin if it exists to prevent errors in the test logs.
  if (POST_UPDATE_CONTRACTID in Components.classes) {
    var registrar = Components.manager.QueryInterface(AUS_Ci.nsIComponentRegistrar);
    registrar.registerFactory(gPostUpdateWin.classID,
                              gPostUpdateWin.classDescription,
                              gPostUpdateWin.contractID, gPostUpdateWin);
  }

  gAUS = AUS_Cc["@mozilla.org/updates/update-service;1"].
         getService(AUS_Ci.nsIApplicationUpdateService).
         QueryInterface(AUS_Ci.nsIObserver);
  var os = AUS_Cc["@mozilla.org/observer-service;1"].
           getService(AUS_Ci.nsIObserverService);
  os.notifyObservers(null, "profile-after-change", null);
  os.notifyObservers(null, "final-ui-startup", null);
}

/* Initializes nsIUpdateChecker */
function startUpdateChecker() {
  gUpdateChecker = AUS_Cc["@mozilla.org/updates/update-checker;1"].
                   createInstance(AUS_Ci.nsIUpdateChecker);
}

/* Initializes nsIUpdateManager */
function startUpdateManager() {
  gUpdateManager = AUS_Cc["@mozilla.org/updates/update-manager;1"].
                   getService(AUS_Ci.nsIUpdateManager);
}

/**
 * Constructs a string representing a remote update xml file.
 * @param   aUpdates
 *          The string representing the update elements.
 * @returns The string representing a remote update xml file.
 */
function getRemoteUpdatesXMLString(aUpdates) {
  return "<?xml version=\"1.0\"?>\n" +
         "<updates>\n" +
           aUpdates +
         "</updates>\n";
}

/**
 * Constructs a string representing an update element for a remote update xml
 * file.
 * See getUpdateString
 * @returns The string representing an update element for an update xml file.
 */
function getRemoteUpdateString(aPatches, aName, aType, aVersion,
                               aPlatformVersion, aExtensionVersion, aBuildID,
                               aLicenseURL, aDetailsURL) {
  return  getUpdateString(aName, aType, aVersion, aPlatformVersion,
                         aExtensionVersion, aBuildID, aLicenseURL,
                         aDetailsURL) + ">\n" +
              aPatches + 
         "  </update>\n";
}

/**
 * Constructs a string representing a patch element for a remote update xml
 * file
 * See getPatchString
 * @returns The string representing a patch element for a remote update xml
 *          file.
 */
function getRemotePatchString(aType, aURL, aHashFunction, aHashValue, aSize) {
  return getPatchString(aType, aURL, aHashFunction, aHashValue, aSize) +
         "/>\n";
}

/**
 * Constructs a string representing a local update xml file.
 * @param   aUpdates
 *          The string representing the update elements.
 * @returns The string representing a local update xml file.
 */
function getLocalUpdatesXMLString(aUpdates) {
  if (!aUpdates || aUpdates == "")
    return "<updates xmlns=\"http://www.mozilla.org/2005/app-update\"/>"
  return ("<updates xmlns=\"http://www.mozilla.org/2005/app-update\">" +
           aUpdates +
         "</updates>").replace(/>\s+\n*</g,'><');
}

/**
 * Constructs a string representing an update element for a local update xml
 * file.
 * See getUpdateString
 * @param   aServiceURL
 *          The update's xml url.
 *          If null this will default to 'http://dummyservice/'.
 * @param   aIsCompleteUpdate
 *          The string 'true' if this update was a complete update or the string
 *          'false' if this update was a partial update.
 *          If null this will default to 'true'.
 * @param   aChannel
 *          The update channel name.
 *          If null this will default to 'bogus_channel'.
 * @param   aForegroundDownload
 *          The string 'true' if this update was manually downloaded or the
 *          string 'false' if this update was automatically downloaded.
 *          If null this will default to 'true'.
 * @returns The string representing an update element for an update xml file.
 */
function getLocalUpdateString(aPatches, aName, aType, aVersion, aPlatformVersion,
                              aExtensionVersion, aBuildID, aLicenseURL,
                              aDetailsURL, aServiceURL, aInstallDate, aStatusText,
                              aIsCompleteUpdate, aChannel, aForegroundDownload) {
  var serviceURL = aServiceURL ? aServiceURL : "http://dummyservice/";
  var installDate = aInstallDate ? aInstallDate : "1238441400314";
  var statusText = aStatusText ? aStatusText : "Install Pending";
  var isCompleteUpdate =
    typeof(aIsCompleteUpdate) == "string" ? aIsCompleteUpdate : "true";
  var channel = aChannel ? aChannel : "bogus_channel";
  var foregroundDownload =
    typeof(aForegroundDownload) == "string" ? aForegroundDownload : "true";
  return getUpdateString(aName, aType, aVersion, aPlatformVersion,
                         aExtensionVersion, aBuildID, aLicenseURL,
                         aDetailsURL) + " " +
                   "serviceURL=\"" + serviceURL + "\" " +
                   "installDate=\"" + installDate + "\" " +
                   "statusText=\"" + statusText + "\" " +
                   "isCompleteUpdate=\"" + isCompleteUpdate + "\" " +
                   "channel=\"" + channel + "\" " +
                   "foregroundDownload=\"" + foregroundDownload + "\">"  +
              aPatches + 
         "  </update>";
}

/**
 * Constructs a string representing a patch element for a local update xml file.
 * See getPatchString
 * @param   aSelected
 *          Whether this patch is selected represented or not. The string 'true'
 *          denotes selected and the string 'false' denotes not selected.
 *          If null this will default to the string 'true'.
 * @param   aState
 *          The patch's state.
 *          If null this will default to STATE_SUCCEEDED (e.g. 'succeeded').
 * @returns The string representing a patch element for a local update xml file.
 */
function getLocalPatchString(aType, aURL, aHashFunction, aHashValue, aSize,
                             aSelected, aState) {
  var selected = typeof(aSelected) == "string" ? aSelected : "true";
  var state = aState ? aState : STATE_SUCCEEDED;
  return getPatchString(aType, aURL, aHashFunction, aHashValue, aSize) + " " +
         "selected=\"" + selected + "\" " +
         "state=\"" + state + "\"/>\n";
}

/**
 * Constructs a string representing an update element for a remote update xml
 * file.
 * @param   aPatches
 *          The string representing the update's patches.
 * @param   aName
 *          The update's name.
 *          If null this will default to 'XPCShell App Update Test'.
 * @param   aType
 *          The update's type which should be major or minor.
 *          If null this will default to 'major'.
 * @param   aVersion
 *          The update's app version.
 *          If null this will default to '4.0'.
 * @param   aPlatformVersion
 *          The update's platform version.
 *          If null this will default to '4.0'.
 * @param   aExtensionVersion
 *          The update's extension version.
 *          If null this will default to '4.0'.
 * @param   aBuildID
 *          The update's build id.
 *          If null this will default to '20080811053724'.
 * @param   aLicenseURL
 *          The update's license url.
 *          If null this will default to 'http://dummylicense/'.
 * @param   aDetailsURL
 *          The update's details url.
 *          If null this will default to 'http://dummydetails/'.
 * @returns The string representing an update element for an update xml file.
 */
function getUpdateString(aName, aType, aVersion, aPlatformVersion,
                         aExtensionVersion, aBuildID, aLicenseURL, aDetailsURL) {
  var name = aName ? aName : "XPCShell App Update Test";
  var type = aType ? aType : "major";
  var version = aVersion ? aVersion : "4.0";
  var platformVersion = aPlatformVersion ? aPlatformVersion : "4.0";
  var extensionVersion = aExtensionVersion ? aExtensionVersion : "4.0";
  var buildID = aBuildID ? aBuildID : "20080811053724";
  var licenseURL = aLicenseURL ? aLicenseURL : "http://dummylicense/";
  var detailsURL = aDetailsURL ? aDetailsURL : "http://dummydetails/";
  return "  <update name=\"" + name + "\" " +
                   "type=\"" + type + "\" " +
                   "version=\"" + version + "\" " +
                   "platformVersion=\"" + platformVersion + "\" " +
                   "extensionVersion=\"" + extensionVersion + "\" " +
                   "buildID=\"" + buildID + "\" " +
                   "licenseURL=\"" + licenseURL + "\" " +
                   "detailsURL=\"" + detailsURL + "\"";
}

/**
 * Constructs a string representing a patch element for an update xml file.
 * @param   aType
 *          The patch's type which should be complete or partial.
 *          If null this will default to 'complete'.
 * @param   aURL
 *          The patch's url to the mar file.
 *          If null this will default to 'http://localhost:4444/data/empty.mar'.
 * @param   aHashFunction
 *          The patch's hash function used to verify the mar file.
 *          If null this will default to 'MD5'.
 * @param   aHashValue
 *          The patch's hash value used to verify the mar file.
 *          If null this will default to '6232cd43a1c77e30191c53a329a3f99d'
 *          which is the MD5 hash value for the empty.mar.
 * @param   aSize
 *          The patch's file size for the mar file.
 *          If null this will default to '775' which is the file size for the
 *          empty.mar.
 * @returns The string representing a patch element for an update xml file.
 */
function getPatchString(aType, aURL, aHashFunction, aHashValue, aSize) {
  var type = aType ? aType : "complete";
  var url = aURL ? aURL : URL_HOST + DIR_DATA + "/empty.mar";
  var hashFunction = aHashFunction ? aHashFunction : "MD5";
  var hashValue = aHashValue ? aHashValue : "6232cd43a1c77e30191c53a329a3f99d";
  var size = aSize ? aSize : "775";
  return "    <patch type=\"" + type + "\" " +
                     "URL=\"" + url + "\" " +
                     "hashFunction=\"" + hashFunction + "\" " +
                     "hashValue=\"" + hashValue + "\" " +
                     "size=\"" + size + "\"";
}

/**
 * Writes the updates specified to either the active-update.xml or the
 * updates.xml.
 * @param   updates
 *          The updates represented as a string to write to the XML file.
 * @param   isActiveUpdate
 *          If true this will write to the active-update.xml otherwise it will
 *          write to the updates.xml file.
 */
function writeUpdatesToXMLFile(aContent, aIsActiveUpdate) {
  var file = getCurrentProcessDir();
  file.append(aIsActiveUpdate ? FILE_UPDATE_ACTIVE : FILE_UPDATES_DB);
  writeFile(file, aContent);
}

/**
 * Writes the current update operation/state to a file in the patch
 * directory, indicating to the patching system that operations need
 * to be performed.
 * @param   aStatus
 *          The status value to write.
 */
function writeStatusFile(aStatus) {
  var file = getCurrentProcessDir();
  file.append("updates");
  file.append("0");
  file.append("update.status");
  aStatus += "\n";
  writeFile(file, aStatus);
}

/**
 * Writes text to a file. This will replace existing text if the file exists
 * and create the file if it doesn't exist.
 * @param   aFile
 *          The file to write to. Will be created if it doesn't exist.
 * @param   aText
 *          The text to write to the file. If there is existing text it will be
 *          replaced.
 */
function writeFile(aFile, aText) {
  var fos = AUS_Cc["@mozilla.org/network/file-output-stream;1"].
            createInstance(AUS_Ci.nsIFileOutputStream);
  if (!aFile.exists())
    aFile.create(AUS_Ci.nsILocalFile.NORMAL_FILE_TYPE, PERMS_FILE);
  fos.init(aFile, MODE_WRONLY | MODE_CREATE | MODE_TRUNCATE, PERMS_FILE, 0);
  fos.write(aText, aText.length);
  fos.close();
}

/**
 * Reads text from a file and returns the string.
 * @param   aFile
 *          The file to read from.
 * @returns The string of text read from the file.
 */
function readFile(aFile) {
  var fis = AUS_Cc["@mozilla.org/network/file-input-stream;1"].
            createInstance(AUS_Ci.nsIFileInputStream);
  if (!aFile.exists())
    return null;
  fis.init(aFile, MODE_RDONLY, PERMS_FILE, 0);
  var sis = AUS_Cc["@mozilla.org/scriptableinputstream;1"].
            createInstance(AUS_Ci.nsIScriptableInputStream);
  sis.init(fis);
  var text = sis.read(sis.available());
  sis.close();
  return text;
}

/* Returns human readable status text from the updates.properties bundle */
function getStatusText(aErrCode) {
  return getString("check_error-" + aErrCode);
}

/* Returns a string from the updates.properties bundle */
function getString(aName) {
  try {
    return gUpdateBundle.GetStringFromName(aName);
  }
  catch (e) {
  }
  return null;
}

/* Update check listener */
const updateCheckListener = {
  onProgress: function(request, position, totalSize) {
  },

  onCheckComplete: function(request, updates, updateCount) {
    gRequestURL = request.channel.originalURI.spec;
    gUpdateCount = updateCount;
    gUpdates = updates;
    dump("onError: url = " + gRequestURL + ", " +
         "request.status = " + request.status + ", " +
         "update.statusText = " + request.statusText + ", " +
         "updateCount = " + updateCount + "\n");
    // Use a timeout to allow the XHR to complete
    do_timeout(0, "gCheckFunc()");
  },

  onError: function(request, update) {
    gRequestURL = request.channel.originalURI.spec;
    gStatusCode = request.status;
    gStatusText = update.statusText;
    dump("onError: url = " + gRequestURL + ", " +
         "request.status = " + gStatusCode + ", " +
         "update.statusText = " + gStatusText + "\n");
    // Use a timeout to allow the XHR to complete
    do_timeout(0, "gCheckFunc()");
  },

  QueryInterface: function(aIID) {
    if (!aIID.equals(AUS_Ci.nsIUpdateCheckListener) &&
        !aIID.equals(AUS_Ci.nsISupports))
      throw AUS_Cr.NS_ERROR_NO_INTERFACE;
    return this;
  }
};

/**
 * Removes the updates directory, updates.xml file, and active-update.xml file
 * if they exist. This prevents some tests from failing due to files being left
 * behind when the tests are interrupted.
 */
function removeUpdateDirsAndFiles() {
  var appDir = getCurrentProcessDir();
  var file = appDir.clone();
  file.append("active-update.xml");
  try {
    if (file.exists())
      file.remove(false);
  }
  catch (e) {
    dump("Unable to remove file\npath: " + file.path +
         "\nException: " + e + "\n");
  }

  file = appDir.clone();
  file.append("updates.xml");
  try {
    if (file.exists())
      file.remove(false);
  }
  catch (e) {
    dump("Unable to remove file\npath: " + file.path +
         "\nException: " + e + "\n");
  }

  file = appDir.clone();
  file.append("updates");
  file.append("last-update.log");
  try {
    if (file.exists())
      file.remove(false);
  }
  catch (e) {
    dump("Unable to remove file\npath: " + file.path +
         "\nException: " + e + "\n");
  }

  // This fails sporadically on Mac OS X so wrap it in a try catch
  var updatesDir = appDir.clone();
  updatesDir.append("updates");
  try {
    removeDirRecursive(updatesDir);
  }
  catch (e) {
    dump("Unable to remove directory\npath: " + updatesDir.path +
         "\nException: " + e + "\n");
  }
}

/**
 * Deletes a directory and its children. First it tries nsIFile::Remove(true).
 * If that fails it will fall back to recursing, setting the appropriate
 * permissions, and deleting the current entry.
 * @param   dir
 *          A nsIFile for the directory to be deleted
 */
function removeDirRecursive(aDir) {
  if (!aDir.exists())
    return;

  try {
    aDir.remove(true);
    return;
  }
  catch (e) {
  }

  var dirEntries = aDir.directoryEntries;
  while (dirEntries.hasMoreElements()) {
    var entry = dirEntries.getNext().QueryInterface(AUS_Ci.nsIFile);

    if (entry.isDirectory()) {
      removeDirRecursive(entry);
    }
    else {
      entry.permissions = PERMS_FILE;
      entry.remove(false);
    }
  }
  aDir.permissions = PERMS_DIRECTORY;
  aDir.remove(true);
}

/**
 * Helper for starting the http server used by the tests
 * @param   aRelativeDirName
 *          The directory name to register relative to
 *          toolkit/mozapps/update/test/unit/
 */
function start_httpserver(aRelativeDirName) {
  var dir = do_get_file(REL_TEST_PATH + aRelativeDirName);
  if (!dir.exists())
    do_throw("The directory used by nsHttpServer does not exist! path: " +
             dir.path + "\n");

  if (!dir.isDirectory())
    do_throw("A file instead of a directory was specified for nsHttpServer " +
             "registerDirectory! path: " + dir.path + "\n");

  do_import_script("netwerk/test/httpserver/httpd.js");
  gTestserver = new nsHttpServer();
  gTestserver.registerDirectory("/data/", dir);
  gTestserver.start(4444);
}

/* Helper for stopping the http server used by the tests */
function stop_httpserver() {
  gTestserver.stop();
}

/**
 * Creates an nsIXULAppInfo
 * @param   id
 *          The ID of the test application
 * @param   name
 *          A name for the test application
 * @param   version
 *          The version of the application
 * @param   platformVersion
 *          The gecko version of the application
 */
function createAppInfo(id, name, version, platformVersion) {
  const XULAPPINFO_CONTRACTID = "@mozilla.org/xre/app-info;1";
  const XULAPPINFO_CID = Components.ID("{c763b610-9d49-455a-bbd2-ede71682a1ac}");
  var XULAppInfo = {
    vendor: "Mozilla",
    name: name,
    ID: id,
    version: version,
    appBuildID: "2007010101",
    platformVersion: platformVersion,
    platformBuildID: "2007010101",
    inSafeMode: false,
    logConsoleErrors: true,
    OS: "XPCShell",
    XPCOMABI: "noarch-spidermonkey",

    QueryInterface: function QueryInterface(iid) {
      if (iid.equals(AUS_Ci.nsIXULAppInfo) ||
          iid.equals(AUS_Ci.nsIXULRuntime) ||
          iid.equals(AUS_Ci.nsISupports))
        return this;
      throw AUS_Cr.NS_ERROR_NO_INTERFACE;
    }
  };
  
  var XULAppInfoFactory = {
    createInstance: function (outer, iid) {
      if (outer == null)
        return XULAppInfo.QueryInterface(iid);
      throw AUS_Cr.NS_ERROR_NO_AGGREGATION;
    }
  };

  var registrar = Components.manager.QueryInterface(AUS_Ci.nsIComponentRegistrar);
  registrar.registerFactory(XULAPPINFO_CID, "XULAppInfo",
                            XULAPPINFO_CONTRACTID, XULAppInfoFactory);
}

/**
 * Returns the Gecko Runtime Engine directory. This is used to locate the the
 * updater binary (Windows and Linux) or updater package (Mac OS X). For
 * XULRunner applications this is different than the currently running process
 * directory.
 */
function getGREDir() {
  return gDirSvc.get(NS_GRE_DIR, AUS_Ci.nsIFile);
}

/**
 * Returns the directory for the currently running process. This is used to
 * clean up after the tests and to locate the active-update.xml and updates.xml
 * files.
 */
function getCurrentProcessDir() {
  return gDirSvc.get(NS_XPCOM_CURRENT_PROCESS_DIR, AUS_Ci.nsIFile);
}

const gPostUpdateWin = {
  run: function() {
    dump("Mock nsPostUpdateWin component run method called\n");
  },
  flags: AUS_Ci.nsIClassInfo.SINGLETON,
  implementationLanguage: AUS_Ci.nsIProgrammingLanguage.JAVASCRIPT,
  getHelperForLanguage: function(language) null,
  getInterfaces: function(count) {
    var interfaces = [AUS_Ci.nsPostUpdateWin, AUS_Ci.nsIRunnable];
    count.value = interfaces.length;
    return interfaces;
  },
  classDescription: "nsPostUpdateWin",
  contractID: POST_UPDATE_CONTRACTID,
  classID: Components.ID("{d15b970b-5472-40df-97e8-eb03a04baa82}"),
  createInstance: function (outer, aIID) {
    if (outer == null)
      return gPostUpdateWin.QueryInterface(aIID);
    throw AUS_Cr.NS_ERROR_NO_AGGREGATION;
  },
  QueryInterface: function(aIID) {
    if (aIID.equals(AUS_Ci.nsPostUpdateWin) ||
        aIID.equals(AUS_Ci.nsIRunnable) ||
        aIID.equals(AUS_Ci.nsISupports))
      return gPostUpdateWin;
    throw AUS_Cr.NS_ERROR_NO_INTERFACE;
  }
};

// Use a custom profile dir to keep the bin dir clean... not necessary but nice
var gTestRoot = gDirSvc.get("CurProcD", AUS_Ci.nsILocalFile);
gTestRoot = gTestRoot.parent.parent;
gTestRoot.append("_tests");
gTestRoot.append("xpcshell-simple");
gTestRoot.append("test_update");
gTestRoot.normalize();

// Create and register a custom profile directory to keep dist/bin clean.
var gProfD = gTestRoot.clone();
gProfD.append("profile");
if (gProfD.exists())
  gProfD.remove(true);
gProfD.create(AUS_Ci.nsIFile.DIRECTORY_TYPE, PERMS_DIRECTORY);

var gDirProvider = {
  getFile: function(prop, persistent) {
    switch (prop) {
      case NS_APP_USER_PROFILE_50_DIR:
      case NS_APP_PROFILE_DIR_STARTUP:
        persistent.value = true;
        return gProfD.clone();
      case XRE_UPDATE_ROOT_DIR:
        persistent.value = true;
        return getCurrentProcessDir();
    }
    return null;
  },
  QueryInterface: function(iid) {
    if (iid.equals(AUS_Ci.nsIDirectoryServiceProvider) ||
        iid.equals(AUS_Ci.nsISupports))
      return this;
    throw AUS_Cr.NS_ERROR_NO_INTERFACE;
  }
};
gDirSvc.QueryInterface(AUS_Ci.nsIDirectoryService).registerProvider(gDirProvider);
