/*
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
#ifndef _SECSTUBT_H_
#define _SECSTUBT_H_

typedef struct _item {
    unsigned char *data;
    unsigned long len;
} SECItem;

struct CERTCertificateStr {
    SECItem derCert;
};

typedef struct CERTCertificateStr CERTCertificate;
typedef struct _certdb CERTCertDBHandle;
typedef struct _md5context MD5Context;
typedef struct _sha1context SHA1Context;
typedef struct HASHContextStr HASHContext;

#define MD5_LENGTH	16
#define SHA1_LENGTH	20

typedef enum {
    HASH_AlgNULL,
    HASH_AlgMD2,
    HASH_AlgMD5,
    HASH_AlgSHA1,
    HASH_AlgTOTAL
} HASH_HashType;

typedef enum _SECStatus {
    SECWouldBlock = -2,
    SECFailure = -1,
    SECSuccess = 0
} SECStatus;

#define SSL_SECURITY_STATUS_NOOPT -1
#define SSL_SECURITY_STATUS_OFF           0
#define SSL_SECURITY_STATUS_ON_HIGH       1
#define SSL_SECURITY_STATUS_ON_LOW        2
#define SSL_SECURITY_STATUS_FORTEZZA      3

#define SSL_SC_RSA              0x00000001L
#define SSL_SC_MD2              0x00000010L
#define SSL_SC_MD5              0x00000020L
#define SSL_SC_RC2_CBC          0x00001000L
#define SSL_SC_RC4              0x00002000L
#define SSL_SC_DES_CBC          0x00004000L
#define SSL_SC_DES_EDE3_CBC     0x00008000L
#define SSL_SC_IDEA_CBC         0x00010000L

#endif /* _SECSTUBT_H_ */
