/*
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

/*
	JNIUtils.java
 */

package netscape.oji;

public class JNIUtils {
	/**
	 * Returns a local reference for a passed-in global reference.
	 */
	public static Object NewLocalRef(Object object) {
		return object;
	}
	
	/**
	 * Returns the currently running thread.
	 */
	public static Object GetCurrentThread() {
		return Thread.currentThread();
	}
	
	
	/**
	 * Stub SecurityManager class, to expose access to class loaders.
	 */
	static class StubSecurityManager extends SecurityManager {
		public ClassLoader getCurrentClassLoader() {
			return currentClassLoader();
		}
	}
	
	private static StubSecurityManager stubManager = new StubSecurityManager();
	
	/**
	 * Returns the current class loader.
	 */
	public static Object GetCurrentClassLoader() {
		return stubManager.getCurrentClassLoader();
	}
	
	public static Object GetObjectClassLoader(Object object) {
		return object.getClass().getClassLoader();
	}
}
