/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
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
package netscape.ldap;

import netscape.ldap.ber.stream.*;

/**
 * Specifies changes to be made to the values of an attribute.  The change is
 * specified in terms of the following aspects:
 * <P>
 *
 * <UL>
 * <LI>the type of modification (add, replace, or delete the value of an attribute)
 * <LI>the type of value being modified (string or binary)
 * <LI>the name of the attribute being modified
 * <LI>the actual value
 * </UL>
 * <P>
 *
 * After you specify a change to an attribute, you can execute the change
 * by calling the <CODE>LDAPConnection.modify</CODE> method and specifying
 * the DN of the entry that you want to modify.
 * <P>
 *
 * @version 1.0
 * @see netscape.ldap.LDAPConnection#modify(java.lang.String, netscape.ldap.LDAPModification)
 */
public class LDAPModification {

    /**
     * Specifies that a value should be added to an attribute.
     */
    public static final int ADD     = 0;

    /**
     * Specifies that a value should be removed from an attribute.
     */
    public static final int DELETE  = 1;

    /**
     * Specifies that a value should replace the existing value in an attribute.
     */
    public static final int REPLACE = 2;

    /**
     * Internal variables
     */
    private int operation;
    private LDAPAttribute attribute;

    /**
     * Specifies a modification to be made to an attribute.
     * @param op The type of modification to make, which can be one of the following:
     *   <P>
     *   <UL>
     *   <LI><CODE>LDAPModification.ADD</CODE> (the value should be added to the attribute)
     *   <LI><CODE>LDAPModification.DELETE</CODE> (the value should be removed from the attribute)
     *   <LI><CODE>LDAPModification.REPLACE</CODE> (the value should replace the existing value of the attribute)
     *   </UL><P>
     * @param attr The attribute (possibly with values) to be modified.
     * @see netscape.ldap.LDAPAttribute
     */
    public LDAPModification( int op, LDAPAttribute attr ) {
        operation = op;
        attribute = attr;
    }

    /**
     * Returns the type of modification specified by this object.
     * @return One of the following types of modifications:
     *   <P>
     *   <UL>
     *   <LI><CODE>LDAPModification.ADD</CODE> (the value should be added to the attribute)
     *   <LI><CODE>LDAPModification.DELETE</CODE> (the value should be removed from the attribute)
     *   <LI><CODE>LDAPModification.REPLACE</CODE> (the value should replace the existing value of the attribute)
     *   </UL><P>
     */
    public int getOp() {
        return operation;
    }

    /**
     * Returns the attribute (possibly with values) to be modified.
     * @return The attribute to be modified.
     * @see netscape.ldap.LDAPAttribute
     */
    public LDAPAttribute getAttribute() {
        return attribute;
    }

    /**
     * Retrieves the BER (Basic Encoding Rules) representation
     * of the current modification.
     * @return BER representation of the modification.
     */
    public BERElement getBERElement() {
        BERSequence seq = new BERSequence();
        seq.addElement(new BEREnumerated(operation));
        seq.addElement(attribute.getBERElement());
        return seq;
    }

    /**
     * Retrieves the string representation of the current
     * modification. For example:
     *
     * <PRE>
     * LDAPModification: REPLACE, LDAPAttribute {type='mail', values='babs@ace.com'}
     * LDAPModification: ADD, LDAPAttribute {type='description', values='This entry was modified with the modattrs program'}
     * </PRE>
     *
     * @return String representation of the current modification.
     */
    public String toString() {
        String s = "LDAPModification: ";
        if ( operation == ADD )
            s += "ADD, ";
        else if ( operation == DELETE )
            s += "DELETE, ";
        else if ( operation == REPLACE )
            s += "REPLACE, ";
        else
            s += "INVALID OP, ";
        s += attribute;
        return s;
    }
}
