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

package netscape.jsdebug;

/**
* This instances of this class represent the JavaScript string object. This
* class is intended to only be constructed by the underlying native code.
* The DebugController will construct an instance of this class when scripts
* are created and that instance will always be used to reference the underlying
* script throughout the lifetime of that script.
*
* @author  John Bandhauer
* @version 1.0
* @since   1.0
*/
public final class Script
{
    public String   getURL()            {return _url;           }
    public String   getFunction()       {return _function;      }
    public int      getBaseLineNumber() {return _baseLineNumber;}
    public int      getLineExtent()     {return _lineExtent;    }
    public boolean  isValid()           {return 0 != _nativePtr;}

    /**
    * Get the PC of the first executable code on or after the given 
    * line in this script. returns null if none
    */
    public native JSPC      getClosestPC(int line);

    public String toString()
    {
        int end = _baseLineNumber+_lineExtent-1;
        if( null == _function )
            return "<Script "+_url+"["+_baseLineNumber+","+end+"]>";
        else
            return "<Script "+_url+"#"+_function+"()["+_baseLineNumber+","+end+"]>";
    }

    public int hashCode()
    {
        return _nativePtr;
    }

    public boolean equals(Object obj)
    {
        if( obj == this )
            return true;

        // In our system the native code is the only code that generates
        // these objects. They are never duplicated
        return false;
/*
        if( !(obj instanceof Script) )
            return false;
        return 0 != _nativePtr && _nativePtr == ((Script)obj)._nativePtr;
*/
    }

    private synchronized void _setInvalid() {_nativePtr = 0;}

    private String  _url;
    private String  _function;
    private int     _baseLineNumber;
    private int     _lineExtent;
    private int     _nativePtr;     // used internally
}    
