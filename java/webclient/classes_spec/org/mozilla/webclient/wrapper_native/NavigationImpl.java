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
 * Contributor(s):  Ed Burns <edburns@acm.org>
 */

package org.mozilla.webclient.wrapper_native;

import org.mozilla.util.Assert;
import org.mozilla.util.Log;
import org.mozilla.util.ParameterCheck;
import org.mozilla.util.RangeException;

import org.mozilla.webclient.BrowserControl;
import org.mozilla.webclient.Navigation;
import org.mozilla.webclient.WindowControl;
import org.mozilla.webclient.WrapperFactory;
import org.mozilla.webclient.Prompt;

import java.io.InputStream;
import java.util.Properties;

public class NavigationImpl extends ImplObjectNative implements Navigation
{
//
// Protected Constants
//

//
// Class Variables
//

//
// Instance Variables
//

// Attribute Instance Variables

// Relationship Instance Variables

//
// Constructors and Initializers    
//

public NavigationImpl(WrapperFactory yourFactory, 
		      BrowserControl yourBrowserControl)
{
    super(yourFactory, yourBrowserControl);
}

//
// Class methods
//

//
// General Methods
//

//
// Methods from Navigation    
//

public void loadURL(String absoluteURL)
{
    ParameterCheck.nonNull(absoluteURL);
    myFactory.throwExceptionIfNotInitialized();
    Assert.assert(-1 != nativeWebShell);
    
    synchronized(myBrowserControl) {
        nativeLoadURL(nativeWebShell, absoluteURL);
    }
}

public void loadFromStream(InputStream stream, String uri,
                           String contentType, int contentLength,
                           Properties loadInfo)
{
    ParameterCheck.nonNull(stream);
    ParameterCheck.nonNull(uri);
    ParameterCheck.nonNull(contentType);
    if (contentLength < -1 || contentLength == 0) {
        throw new RangeException("contentLength value " + contentLength +
                                 " is out of range.  It is should be either -1 or greater than 0.");
    }

    myFactory.throwExceptionIfNotInitialized();
    Assert.assert(-1 != nativeWebShell);
    
    synchronized(myBrowserControl) {
        nativeLoadFromStream(nativeWebShell, stream,
                             uri, contentType, contentLength,
                             loadInfo);
    }
}


public void refresh(long loadFlags)
{
    ParameterCheck.noLessThan(loadFlags, 0);
    myFactory.throwExceptionIfNotInitialized();
    Assert.assert(-1 != nativeWebShell);
    
    synchronized(myBrowserControl) {
        nativeRefresh(nativeWebShell, loadFlags);
    }
}

public void stop()
{
    myFactory.throwExceptionIfNotInitialized();
    Assert.assert(-1 != nativeWebShell);
    
    synchronized(myBrowserControl) {
        nativeStop(nativeWebShell);
    }
}

public void setPrompt(Prompt yourPrompt)
{
    ParameterCheck.nonNull(yourPrompt);
    myFactory.throwExceptionIfNotInitialized();
    Assert.assert(-1 != nativeWebShell);
    
    synchronized(myBrowserControl) {
        nativeSetPrompt(nativeWebShell, yourPrompt);
    }

}

// 
// Native methods
//

public native void nativeLoadURL(int webShellPtr, String absoluteURL);

public native void nativeLoadFromStream(int webShellPtr, InputStream stream,
                                        String uri, 
                                        String contentType, 
                                        int contentLength,
                                        Properties loadInfo);

public native void nativeRefresh(int webShellPtr, long loadFlags);

public native void nativeStop(int webShellPtr);

public native void nativeSetPrompt(int webShellPtr, Prompt yourPrompt);

// ----VERTIGO_TEST_START

//
// Test methods
//

public static void main(String [] args)
{
    Assert.setEnabled(true);

    Log.setApplicationName("NavigationImpl");
    Log.setApplicationVersion("0.0");
    Log.setApplicationVersionDate("$Id: NavigationImpl.java,v 1.4 2001/04/02 21:13:59 ashuk%eng.sun.com Exp $");

    try {
        org.mozilla.webclient.BrowserControlFactory.setAppData(args[0]);
	org.mozilla.webclient.BrowserControl control = 
	    org.mozilla.webclient.BrowserControlFactory.newBrowserControl();
        Assert.assert(control != null);
	
	Navigation wc = (Navigation)
	    control.queryInterface(org.mozilla.webclient.BrowserControl.WINDOW_CONTROL_NAME);
	Assert.assert(wc != null);
    }
    catch (Exception e) {
	System.out.println("got exception: " + e.getMessage());
    }
}

// ----VERTIGO_TEST_END

} // end of class NavigationImpl
