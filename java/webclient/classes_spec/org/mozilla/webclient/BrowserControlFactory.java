/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * 
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is RaptorCanvas.
 *
 * The Initial Developer of the Original Code is Kirk Baker and
 * Ian Wilkinson. Portions created by Kirk Baker and Ian Wilkinson are
 * Copyright (C) 1999 Kirk Baker and Ian Wilkinson. All
 * Rights Reserved.
 *
 * Contributor(s): Kirk Baker <kbaker@eb.com>
 *               Ian Wilkinson <iw@ennoble.com>
 *               Mark Lin <mark.lin@eng.sun.com>
 *               Ed Burns <edburns@acm.org>
 *               Ashutosh Kulkarni <ashuk@eng.sun.com>
 */

package org.mozilla.webclient;

// BrowserControlFactory.java

import org.mozilla.util.Assert;
import org.mozilla.util.Log;
import org.mozilla.util.ParameterCheck;

import java.io.File;
import java.io.FileNotFoundException;

/**
 *
 *  <B>BrowserControlFactory</B> creates concrete instances of BrowserControl

 * <B>Lifetime And Scope</B> <P>

 * This is a static class, it is neven instantiated.

 *
 * @version $Id: BrowserControlFactory.java,v 1.5 2001/07/27 20:57:51 ashuk%eng.sun.com Exp $
 * 
 * @see	org.mozilla.webclient.test.EmbeddedMozilla

 */

public class BrowserControlFactory extends Object
{
//
// Protected Constants
//

//
// Class Variables
//

    private static boolean appDataHasBeenSet = false;
    private static Class browserControlCanvasClass = null;
    private static String platformCanvasClassName = null;
    private static String browserType = null;

//
// Instance Variables
//

// Attribute Instance Variables

// Relationship Instance Variables

//
// Constructors and Initializers    
//

public BrowserControlFactory()
{
    Assert.assert_it(false, "This class shouldn't be constructed.");
}

//
// Class methods
//

public static void setAppData(String absolutePathToNativeBrowserBinDir) throws FileNotFoundException, ClassNotFoundException
{
    BrowserControlFactory.setAppData(BrowserControl.BROWSER_TYPE_NATIVE, absolutePathToNativeBrowserBinDir);
}


    /**

     * This method is used to set per-application instance data, such as
     * the location of the browser binary.

     * @param myBrowserType.  Either "native" or "nonnative"

     * @param absolutePathToNativeBrowserBinDir the path to the bin dir
     * of the native browser, including the bin.  ie:
     * "D:\Projects\mozilla\dist\win32_d.obj\bin"

     */

public static void setAppData(String myBrowserType, String absolutePathToNativeBrowserBinDir) throws FileNotFoundException, ClassNotFoundException
{
    browserType = myBrowserType;
    if (!appDataHasBeenSet) {
        // figure out the correct value for platformCanvasClassName
        if (browserType.equals(BrowserControl.BROWSER_TYPE_NON_NATIVE)) {
            platformCanvasClassName = "org.mozilla.webclient.wrapper_nonnative.JavaBrowserControlCanvas";
        }
        else {
            ParameterCheck.nonNull(absolutePathToNativeBrowserBinDir);
            
            // verify that the directory exists:
            File binDir = new File(absolutePathToNativeBrowserBinDir);
            if (!binDir.exists()) {
                throw new FileNotFoundException("Directory " + absolutePathToNativeBrowserBinDir + " is not found.");
            }
            
            // This hack is necessary for Sun Bug #4303996
            java.awt.Canvas c = new java.awt.Canvas();
            platformCanvasClassName = determinePlatformCanvasClassName();
        }
        // end of figuring out the correct value for platformCanvasClassName
        if (platformCanvasClassName != null) {
            browserControlCanvasClass = Class.forName(platformCanvasClassName);
        }
        else {
            throw new ClassNotFoundException("Could not determine BrowserControlCanvas class to load\n");
        }
        
        try {
            BrowserControlImpl.appInitialize(browserType, absolutePathToNativeBrowserBinDir);
        }
        catch (Exception e) {
            throw new ClassNotFoundException("Can't initialize native browser: " + 
                                             e.getMessage());
        }
        appDataHasBeenSet = true;
    }
}

public static void appTerminate() throws Exception
{
    BrowserControlImpl.appTerminate();
}

public static BrowserControl newBrowserControl() throws InstantiationException, IllegalAccessException, IllegalStateException
{
    if (!appDataHasBeenSet) {
        throw new IllegalStateException("Can't create BrowserControl instance: setAppData() has not been called.");
    }
    Assert.assert_it(null != browserControlCanvasClass);
    
    BrowserControlCanvas newCanvas = null;
    BrowserControl result = null; 
    newCanvas = (BrowserControlCanvas) browserControlCanvasClass.newInstance();
    if (null != newCanvas &&
        null != (result = new BrowserControlImpl(browserType, newCanvas))) {
        newCanvas.initialize(result);
    }

    return result;
}

/**

 * BrowserControlFactory.deleteBrowserControl is called with a
 * BrowserControl instance obtained from
 * BrowserControlFactory.newBrowserControl.  This method renders the
 * argument instance completely un-usable.  It should be called when the
 * BrowserControl instance is no longer needed.  This method simply
 * calls through to the non-public BrowserControlImpl.delete() method.

 * @see org.mozilla.webclient.ImplObject#delete

 */

public static void deleteBrowserControl(BrowserControl toDelete)
{
    ParameterCheck.nonNull(toDelete);
    ((BrowserControlImpl)toDelete).delete();
}

//
// General Methods
//

/**

 * Called from setAppData() in the native case.  This method simply
 * figures out the proper name for the class that is the
 * BrowserControlCanvas. 

 * @return  "org.mozilla.webclient.wrapper_native.win32.Win32BrowserControlCanvas" or "org.mozilla.webclient.wrapper_native.motif.MotifBrowserControlCanvas"

 */

private static String determinePlatformCanvasClassName()
{
    String result = null;
    // cause the native library to be loaded
    // PENDING(edburns): do some magic to determine the right kind of
    // MozWebShellCanvas to instantiate
    
    // How about this:
    // I try loading sun.awt.windows.WDrawingSurfaceInfo. If it doesn't
    // load, then I try loading sun.awt.motif.MDrawingSufaceInfo. If
    // none loads, then I return a error message.
    // If you think up of a better way, let me know.
    // -- Mark
    
    
    Class win32DrawingSurfaceInfoClass;
    
    try {
        win32DrawingSurfaceInfoClass = 
            Class.forName("sun.awt.windows.WDrawingSurfaceInfo");
    }
    catch (Exception e) {
        win32DrawingSurfaceInfoClass = null;
    }
    
    if (win32DrawingSurfaceInfoClass != null) {
        result = "org.mozilla.webclient.wrapper_native.win32.Win32BrowserControlCanvas";
    }
    
    if (null == result) {
        Class motifDrawingSurfaceInfoClass; 
        try {
            motifDrawingSurfaceInfoClass = 
                Class.forName("sun.awt.motif.MDrawingSurfaceInfo");
        }
        catch (Exception e) {
            motifDrawingSurfaceInfoClass = null;
        }
        
        if (motifDrawingSurfaceInfoClass != null) {
            result = "org.mozilla.webclient.wrapper_native.motif.MotifBrowserControlCanvas";
        }
    }
    
    return result;
}

// ----UNIT_TEST_START

//
// Test methods
//

public static void main(String [] args)
{
    System.out.println("doing asserts");
    Assert.setEnabled(true);
    Log.setApplicationName("BrowserControlFactory");
    Log.setApplicationVersion("0.0");
    Log.setApplicationVersionDate("$Id: BrowserControlFactory.java,v 1.5 2001/07/27 20:57:51 ashuk%eng.sun.com Exp $");

    BrowserControlCanvas canvas = null;
    BrowserControl control = null;
    try {
        BrowserControlFactory.setAppData("nonnative", args[0]);
        control = BrowserControlFactory.newBrowserControl();
        Assert.assert_it(control != null);
        canvas = (BrowserControlCanvas) control.queryInterface("webclient.BrowserControlCanvas");
        Assert.assert_it(canvas != null);
    }
    catch (Exception e) {
        System.out.println("\n BrowserControl not getting created \n");
        System.out.println(e.getMessage());
    }
}

// ----UNIT_TEST_END

} // end of class BrowserControlFactory
