/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

package com.netscape.jsdebugging.api;

/**
 * InterruptHook must be subclassed to respond to hooks
 * at a particular program instruction.
 */
public class InterruptHook extends Hook {

    /**
     * Override this method to respond just before a thread
     * reaches a particular instruction.
     */
    /* jband - 03/31/97 - I made this public to allow chaining */
    public void aboutToExecute(ThreadStateBase debug, PC pc) {
//        System.out.println( "called the wrong hook..." );
        // defaults to no action
    }
}
