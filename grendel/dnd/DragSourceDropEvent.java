/* -*- Mode: java; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is the Grendel mail/news client.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1997
 * Netscape Communications Corporation.  All Rights Reserved.
 *
 * Created: Will Scullin <scullin@netscape.com>,  6 Nov 1997.
 */

package grendel.dnd;

public class DragSourceDropEvent
  extends java.util.EventObject {
  boolean fCancelled;
  boolean fSucessful;

  public DragSourceDropEvent(Object aSource, boolean cancelled,
                             boolean sucessful) {
    super(aSource);
    fSucessful = sucessful;
    fCancelled = cancelled;
  }

  public boolean isDragCancelled() {
    return fCancelled;
  }

  public boolean isDropSuccessful() {
    return fSucessful;
  }
}
