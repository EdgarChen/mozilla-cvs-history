/*
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

package org.mozilla.webclient.impl.wrapper_native;


import java.util.List;
import org.mozilla.util.ReturnRunnable;
import org.mozilla.webclient.NewWindowEvent;

/**

 * Win32RaptorCanvas provides a concrete realization
 * of the RaptorCanvas.

 * <B>Lifetime And Scope</B> <P>

 * There is one instance of the BrowserControlCanvas per top level awt Frame.

 * @version $Id: Win32BrowserControlCanvas.java,v 1.6 2007/06/28 12:05:07 edburns%acm.org Exp $
 * 
 * @see	org.mozilla.webclient.BrowserControlCanvasFactory
 * 

 */

import sun.awt.*;
import sun.awt.windows.*;

import org.mozilla.webclient.BrowserControlCanvas;
import org.mozilla.webclient.impl.WrapperFactory;

/**
 * Win32BrowserControlCanvas provides a concrete realization
 * of the RaptorCanvas.
 */
public class Win32BrowserControlCanvas extends NativeBrowserControlCanvas {

    //New method for obtaining access to the Native Peer handle
    private native int getHandleToPeer();

	/**
	 * Obtain the native window handle for this
	 * component's peer.
	 *
	 * @returns The native window handle. 
	 */
    protected int getWindow() {
	Integer result = (Integer)
	    NativeEventThread.instance.pushBlockingReturnRunnable(new ReturnRunnable(){
		    public Object run() {
			Integer result = 
			    new Integer(Win32BrowserControlCanvas.this.getHandleToPeer());
			return result;
		    }
                    public String toString() {
                        return "WCRunnable.getHandleToPeer";
                    }

		});
	return result.intValue();
    }
    
    void performPlatformAppropriateNewWindowRealization(final NewWindowEvent event) {
        final List<Runnable> addToList =
                event.getRealizeNewWindowRunnableList();
        NativeEventThread.instance.pushRunnable(new Runnable() {
            public void run() {
                addToList.add(event.getRealizeNewWindowRunnable());
            }

            public String toString() {
                return "EventRegistrationImpl.RealizeNewWindowEvent";
            }
        });
    
        NativeEventThread.instance.runUntilEventOfType(WindowControlImpl.NativeRealizeWCRunnable.class);
    }
    

    public static NativeEventThread newNativeEventThread(WrapperFactory owner) {
        NativeEventThread result = new NativeEventThread("WebclientEventThread",
                owner);
        return result;
    }
    
    
}
