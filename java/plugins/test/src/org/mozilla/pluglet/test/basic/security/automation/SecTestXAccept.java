/* 
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Sun Microsystems,
 * Inc. Portions created by Sun are
 * Copyright (C) 1999 Sun Microsystems, Inc. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

package org.mozilla.pluglet.test.basic.security.automation;

import org.mozilla.pluglet.test.basic.*;

public class SecTestXAccept implements Test {

private String description = " ckeckAccept";
private String fHost;
private String fPort;
private TestContext context;
private boolean mustPass;
private SecurityManager sm;

public void doAction() {
    sm.checkAccept( fHost, Integer.parseInt(fPort) );
}
	
public void execute( TestContext c ) {
 context=c;
 mustPass = false;

 if (c.getProperty("SecTestXAccept.mustPass").equals( new String("true") )) {
	mustPass = true;
 };

 fHost = c.getProperty("SecTestXAccept.fHost");
 fPort = c.getProperty("SecTestXAccept.fPort");
  sm = System.getSecurityManager();
  if (sm == null) {
      if( mustPass ) {
	  context.registerPASSED("Security manager isn't present.Access allowed");
      } else {
	  context.registerFAILED("Security manager isn't present.Access allowed");
      }
      return;
  }
 try {
 	doAction();
     if( mustPass )	
		c.registerPASSED(new String("OK")); else
		c.registerFAILED(new String(description) + new String(" failed."));
 } catch ( SecurityException e ) {
     if( mustPass )	
		c.registerFAILED(e.toString()); else
		c.registerPASSED(e.toString());
 } catch ( Exception e ) {
	c.registerFAILED(e.toString());
 }

 }
}



