/* -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
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

package netscape.plugin.composer.mapedit;

public class Debug {
  static public void println(Object msg) {
    if (debugOn)
      System.out.println("MapEditDebug: " + msg);
  }

  static public void assert(Object o) {
    if (debugOn && o == null) {
      assert(false);
    }
  }

  static public void assert(Object o,String msg) {
    if (debugOn && o == null) {
      assert(false,msg);
    }
  }

  static public void assert(boolean val) {
    if (debugOn) {
      assert(val,null);
    }
  }

  static public void assert(boolean val,String msg) {
    if (debugOn && !val) {
      if (msg != null && msg.length() > 0) {
        System.out.println("Assertion failed:" + msg);
      }
      else {
        System.out.println("Assertion failed");
      }
      Thread.dumpStack();
    }
  }

  static public boolean debug() {return debugOn;}

  // Set/unset this to control debug mode.
  private static boolean debugOn = false;
}


