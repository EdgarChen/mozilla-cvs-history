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
package netscape.ldap.ber.stream;

import java.util.*;
import java.io.*;

/**
 * This is the base class for integral types such as Integer and
 * Enumerated.
 *
 * <pre>
 * ENCODING RULE:
 *   Primitive Definite length.
 *   tag = << depends on type >>
 *   length = (short or long form)
 *   one or more contents octets hold integral value
 *   value in two's complement
 *
 * Example 1:  (Integer - zero)
 *   02 01 00
 * Example 2:  (Enumerated - 1)
 *   0A 01 01
 * Example 3:  (Integer - 300, short form)
 *   02 02 01 2C
 * Example 4:  (Integer - 300, long form)
 *   02 84 00 00 01 2C
 * </pre>
 *
 * @version 1.0
 * @seeAlso CCITT X.209
 */
public abstract class BERIntegral extends BERElement {
    /**
     * Internal variables
     */
    private int m_value;

    /**
     * Constructs an integral type.
     * @param value integer value
     */
    public BERIntegral(int value) {
        m_value = value;
    }

    /**
     * Constructs an integral element with the input stream.
     * @param stream input stream
     * @param bytes_read array of 1 int; value incremented by
     *        number of bytes read from stream
     * @exception IOException failed to construct
     */
    public BERIntegral(InputStream stream, int[] bytes_read) throws IOException {
        int contents_length = super.readLengthOctets(stream, bytes_read);

        /* Definite length content octets string. */
        if (contents_length > 0) {
            boolean negative = false;
            int octet = stream.read();
            bytes_read[0]++;
            if ((octet & 0x80) > 0)  /* left-most bit is 1. */
                negative = true;

            for (int i = 0; i < contents_length; i++) {
                if (i > 0) {
                    octet = stream.read();
                    bytes_read[0]++;
                }
                if (negative)
                    m_value = (m_value<<8) + (int)(octet^0xFF)&0xFF;
                else
                    m_value = (m_value<<8) + (int)(octet&0xFF);
            }
            if (negative)  /* convert to 2's complement */
                m_value = (m_value + 1) * -1;
        }
    }


    /**
     * Writes BER to stream.
     * @return number of bytes written to stream.
     * @exception IOException failed to send
     */
    public void write(OutputStream stream) throws IOException {
        int binary_value = m_value;
        int num_content_octets = 0;
        int total_ber_octets;
        int offset=1;
        int lead;
        int i;

        byte[] content_octets = new byte[10];  /* should be plenty big */
        byte[] net_octets = new byte[10]; /* pse need this in network order */

        /* Get content octets - need to determine length */
        if (m_value == 0) {
            num_content_octets = 1;
            content_octets[offset] = (byte)0x00;
            net_octets[offset] = (byte)0x00;
        } else {

            if (m_value < 0)  /* convert from 2's complement */
                binary_value = (m_value * -1) -1;

            while (binary_value > 0) {
                if (m_value < 0)
                    content_octets[num_content_octets+offset] = (byte)((binary_value^0xFF)&0xFF);
                else
                    content_octets[num_content_octets+offset] = (byte)(binary_value&0xFF);

                binary_value = (binary_value>>8);
                num_content_octets++;
            }
            /* pse 1/16/96 we've just created a string that is in non-network order
               flip it for net order */
            for (i=0; i<num_content_octets; i++)
                net_octets[offset+num_content_octets-1-i] = content_octets[offset+i];

            /* pse 1/16/96 if +ve value encoded and the leading encoding bit is set
               then stuff in a leading zero byte */
            lead = (int)net_octets[offset];

            if ((m_value > 0) && ((lead & 0x80) > 0)) {
                offset = 0;
                net_octets[offset] = (byte)0x00;
                num_content_octets++;
            }

        }
        stream.write(getType());
        sendDefiniteLength(stream, num_content_octets);
        stream.write(net_octets,offset,num_content_octets);  /* contents */
    }

    /**
     * Gets the integral value.
     * @param element type
     */
    public int getValue() {
        return m_value;
    }

    /**
     * Gets the element type.
     * @param element type
     */
    public abstract int getType();

    /**
     * Gets the string representation.
     * @return string representation of tag
     */
    public abstract String toString();
}

