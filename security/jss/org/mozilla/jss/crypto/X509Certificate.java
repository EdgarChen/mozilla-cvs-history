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
 * The Original Code is the Netscape Security Services for Java.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 1998-2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s):
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL"), in which case the provisions of the GPL are applicable 
 * instead of those above.  If you wish to allow use of your 
 * version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by
 * the GPL.  If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the
 * GPL.
 */
package org.mozilla.jss.crypto;

import java.security.Principal;
import java.math.BigInteger;

/**
 * Certificates handled by JSS.  All certificates handled by JSS are
 * of this type, which is a subtype of java.security.cert.X509Certificate.
 */
public interface X509Certificate
{
    /**
     * Returns the DER encoding of this certificate.
     */
    public byte[] getEncoded()
		throws java.security.cert.CertificateEncodingException;

    /**
     * Returns the possibly-null nickname of this certificate.
     */
    public abstract String getNickname();

    /**
     * Extracts the Public Key from this certificate.
     */
    public abstract java.security.PublicKey getPublicKey();

    /**
     * Returns the RFC 1485 ASCII encoding of the Subject Name.
     */
    public abstract Principal
    getSubjectDN();

    /**
     * Returns the RFC 1485 ASCII encoding of the issuer's Subject Name.
     */
    public abstract Principal
    getIssuerDN();

    /**
     * Returns the serial number of this certificate.
     */
    public abstract BigInteger
    getSerialNumber();

    /**
     * Returns the version number of this X.509 certificate.
     * 0 means v1, 1 means v2, 2 means v3.
     */
    public abstract int
    getVersion();

}
