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

package com.netscape.jsdebugging.apitests.analyzing.data;

import com.netscape.jsdebugging.apitests.xml.Tags;
import com.netscape.jsdebugging.apitests.analyzing.tree.*;

/**
 * Parent of all data classes which will be put into a pool and given a serial number.
 *
 * @author Alex Rakhlin
 */

public abstract class DataSerializable extends Data {

    public DataSerializable (TreeNode head, DataPoolManager dpm){
        super (dpm);
        _real_pointer = null;
        _head = head;
        _sn = TreeUtils.getChildIntegerData (head, Tags.serial_number_tag).intValue();
    }

    /**
     * Get the serial number of this object.
     */
    public int getSerialNumber () { return _sn; }
    
    /**
     * Set the real pointer for this object.
     */
    public void setRealPointer (DataSerializable d) { _real_pointer = d; }
    
    /**
     * Get the real pointer for this object. If this object is a duplicate, then this 
     * refers to the single real object. Otherwise, null.
     */    
    public DataSerializable getRealPointer () { return _real_pointer; }
    
    /**
     * if this object is a duplicate, then this refers to the single real object. Otherwise, null.
     */
    protected DataSerializable _real_pointer;

    /* serial number */
    protected int _sn;    
    
    protected TreeNode _head;
}