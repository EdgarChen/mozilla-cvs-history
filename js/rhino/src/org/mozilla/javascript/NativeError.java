/* -*- Mode: java; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * The Original Code is Rhino code, released
 * May 6, 1999.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1997-1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 * Igor Bukanov
 * Roger Lawrence
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU Public License (the "GPL"), in which case the
 * provisions of the GPL are applicable instead of those above.
 * If you wish to allow use of your version of this file only
 * under the terms of the GPL and not to allow others to use your
 * version of this file under the NPL, indicate your decision by
 * deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL.  If you do not delete
 * the provisions above, a recipient may use your version of this
 * file under either the NPL or the GPL.
 */


package org.mozilla.javascript;

/**
 *
 * The class of error objects
 *
 *  ECMA 15.11
 */
final class NativeError extends IdScriptable
{

    static void init(Context cx, Scriptable scope, boolean sealed)
    {
        NativeError obj = new NativeError();
        obj.prototypeFlag = true;
        ScriptableObject.putProperty(obj, "name", "Error");
        ScriptableObject.putProperty(obj, "message", "");
        ScriptableObject.putProperty(obj, "fileName", "");
        ScriptableObject.putProperty(obj, "lineNumber", new Integer(0));
        obj.addAsPrototype(MAX_PROTOTYPE_ID, cx, scope, sealed);
    }

    protected String toSource(Context cx, Scriptable scope, Object[] args)
        throws JavaScriptException
    {
        // Emulation of SpiderMonkey behavior
        Object name = ScriptableObject.getProperty(this, "name");
        Object message = ScriptableObject.getProperty(this, "message");
        Object fileName = ScriptableObject.getProperty(this, "fileName");
        Object lineNumber = ScriptableObject.getProperty(this, "lineNumber");

        StringBuffer sb = new StringBuffer();
        sb.append("(new ");
        if (name == NOT_FOUND) {
            name = Undefined.instance;
        }
        sb.append(ScriptRuntime.toString(name));
        sb.append("(");
        if (message != NOT_FOUND
            || fileName != NOT_FOUND
            || lineNumber != NOT_FOUND)
        {
            if (message == NOT_FOUND) {
                message = "";
            }
            sb.append(ScriptRuntime.uneval(cx, scope, message));
            if (fileName != NOT_FOUND || lineNumber != NOT_FOUND) {
                sb.append(", ");
                if (fileName == NOT_FOUND) {
                    fileName = "";
                }
                sb.append(ScriptRuntime.uneval(cx, scope, fileName));
                if (lineNumber != NOT_FOUND) {
                    int line = ScriptRuntime.toInt32(lineNumber);
                    if (line != 0) {
                        sb.append(", ");
                        sb.append(ScriptRuntime.toString(line));
                    }
                }
            }
        }
        sb.append("))");
        return sb.toString();
    }

    static NativeError make(Context cx, Scriptable scope, IdFunction ctorObj,
                            Object[] args)
    {
        Scriptable proto = (Scriptable)(ctorObj.get("prototype", ctorObj));

        NativeError obj = new NativeError();
        obj.setPrototype(proto);
        obj.setParentScope(scope);

        if (args.length >= 1) {
            ScriptableObject.putProperty(obj, "message",
                                         ScriptRuntime.toString(args[0]));
            if (args.length >= 2) {
                ScriptableObject.putProperty(obj, "fileName", args[1]);
                if (args.length >= 3) {
                    int line = ScriptRuntime.toInt32(args[2]);
                    ScriptableObject.putProperty(obj, "lineNumber",
                                                 new Integer(line));
                }
            }
        }
        return obj;
    }

    public int methodArity(int methodId)
    {
        if (prototypeFlag) {
            if (methodId == Id_constructor) return 1;
            if (methodId == Id_toString) return 0;
        }
        return super.methodArity(methodId);
    }

    public Object execMethod
        (int methodId, IdFunction f,
         Context cx, Scriptable scope, Scriptable thisObj, Object[] args)
        throws JavaScriptException
    {
        if (prototypeFlag) {
            if (methodId == Id_constructor) {
                return make(cx, scope, f, args);
            }
            else if (methodId == Id_toString) {
                return js_toString(thisObj);
            }
        }
        return super.execMethod(methodId, f, cx, scope, thisObj, args);
    }

    private static String js_toString(Scriptable thisObj)
    {
        return getString(thisObj, "name")+": "+getString(thisObj, "message");
    }

    public String getClassName()
    {
        return "Error";
    }

    public String toString()
    {
        return js_toString(this);
    }

    private static String getString(Scriptable obj, String id)
    {
        Object value = ScriptableObject.getProperty(obj, id);
        if (value == NOT_FOUND) return "";
        return ScriptRuntime.toString(value);
    }

    protected String getIdName(int id)
    {
        if (prototypeFlag) {
            if (id == Id_constructor) return "constructor";
            if (id == Id_toString) return "toString";
        }
        return null;
    }

    protected int mapNameToId(String s)
    {
        int id;
        if (!prototypeFlag) { return 0; }

// #string_id_map#
// #generated# Last update: 2001-05-19 21:55:23 CEST
        L0: { id = 0; String X = null;
            int s_length = s.length();
            if (s_length==8) { X="toString";id=Id_toString; }
            else if (s_length==11) { X="constructor";id=Id_constructor; }
            if (X!=null && X!=s && !X.equals(s)) id = 0;
        }
// #/generated#
        return id;
    }

    private static final int
        Id_constructor    = 1,
        Id_toString       = 2,

        MAX_PROTOTYPE_ID  = 2;

// #/string_id_map#

    private boolean prototypeFlag;
}
