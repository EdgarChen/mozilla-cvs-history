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

// API class

package org.mozilla.javascript;

/**
 * The class of exceptions raised by the engine as described in
 * ECMA edition 3. See section 15.11.6 in particular.
 */
public class EcmaError extends RuntimeException {

    /**
     * Create an exception with the specified detail message.
     *
     * Errors internal to the JavaScript engine will simply throw a
     * RuntimeException.
     *
     * @param sourceName the name of the source reponsible for the error
     * @param lineNumber the line number of the source
     * @param columnNumber the columnNumber of the source (may be zero if
     *                     unknown)
     * @param lineSource the source of the line containing the error (may be
     *                   null if unknown)
     */
    EcmaError(String errorName, String errorMessage,
              String sourceName, int lineNumber, int columnNumber,
              String lineSource)
    {
        super("EcmaError: "+errorName+": "+errorMessage);
        this.errorName = errorName;
        this.errorMessage = errorMessage;
        this.sourceName = sourceName;
        this.lineNumber = lineNumber;
        this.columnNumber = columnNumber;
        this.lineSource = lineSource;
    }

    /**
     * @deprecated Use
     * {@link #EcmaError(String, String, String, int, int, String)} instead.
     */
    public EcmaError(Scriptable nativeError, String sourceName,
                     int lineNumber, int columnNumber, String lineSource)
    {
        this("InternalError", ScriptRuntime.toString(nativeError),
             sourceName, lineNumber, columnNumber, lineSource);
    }

    /**
     * Return a string representation of the error, which currently consists
     * of the name of the error together with the message.
     */
    public String toString()
    {
        StringBuffer buf = new StringBuffer();
        buf.append(errorName);
        buf.append(": ");
        buf.append(errorMessage);
        if (sourceName != null || lineNumber > 0) {
            buf.append(" (");
            if (sourceName != null) {
                buf.append(sourceName);
                buf.append("; ");
            }
            if (lineNumber > 0) {
                buf.append("line ");
                buf.append(lineNumber);
            }
            buf.append(')');
        }
        return buf.toString();
    }

    /**
     * Gets the name of the error.
     *
     * ECMA edition 3 defines the following
     * errors: EvalError, RangeError, ReferenceError,
     * SyntaxError, TypeError, and URIError. Additional error names
     * may be added in the future.
     *
     * See ECMA edition 3, 15.11.7.9.
     *
     * @return the name of the error.
     */
    public String getName() {
        return errorName;
    }

    /**
     * Gets the message corresponding to the error.
     *
     * See ECMA edition 3, 15.11.7.10.
     *
     * @return an implemenation-defined string describing the error.
     */
    public String getMessage() {
        return errorMessage;
    }

    /**
     * Get the name of the source containing the error, or null
     * if that information is not available.
     */
    public String getSourceName() {
        return sourceName;
    }

    /**
     * Returns the line number of the statement causing the error,
     * or zero if not available.
     */
    public int getLineNumber() {
        return lineNumber;
    }

    /**
     * The column number of the location of the error, or zero if unknown.
     */
    public int getColumnNumber() {
        return columnNumber;
    }

    /**
     * The source of the line causing the error, or zero if unknown.
     */
    public String getLineSource() {
        return lineSource;
    }

    /**
     * @deprecated Always returns result of {@link Context#getUndefinedValue()}.
     *
     */
    public Scriptable getErrorObject()
    {
        return Undefined.instance;
    }

    private String errorName;
    private String errorMessage;
    private String sourceName;
    private int lineNumber;
    private int columnNumber;
    private String lineSource;
}
