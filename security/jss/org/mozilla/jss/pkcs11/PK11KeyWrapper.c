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

#include "_jni/org_mozilla_jss_pkcs11_PK11KeyWrapper.h"

#include <nspr.h>
#include <plarena.h>
#include <seccomon.h>
#include <pk11func.h>
#include <secitem.h>
#include <keyt.h>

/* JSS includes */
#include <java_ids.h>
#include <jss_exceptions.h>
#include <jssutil.h>
#include <pk11util.h>
#include <Algorithm.h>

#define LITTLE_HCLHACK
#ifdef LITTLE_HCLHACK
/* PKCS #11 HACK */
#include <secmodi.h>
#endif

#define MAX_PRIVATE_KEY_LEN     MAX_RSA_MODULUS_LEN

/*
 * This is bytes, not bits.  A 2048-bit wrapped RSA private key takes up around
 * 1200 bytes, because lots of "helper" values are stored along with the
 * modulus and exponent. 4096 bytes will hold at least a 4096-bit RSA key.
 */
#define MAX_WRAPPED_KEY_LEN     4096

#ifdef LITTLE_HCLHACK

static void
pk11_EnterKeyMonitor(PK11SymKey *symKey) {
    if (!symKey->sessionOwner || !(symKey->slot->isThreadSafe))
        PK11_EnterSlotMonitor(symKey->slot);
}

static void
pk11_ExitKeyMonitor(PK11SymKey *symKey) {
    if (!symKey->sessionOwner || !(symKey->slot->isThreadSafe))
        PK11_ExitSlotMonitor(symKey->slot);
}
#endif

/***********************************************************************
 *
 * PK11KeyWrapper.nativeWrapSymWithSym
 */
JNIEXPORT jbyteArray JNICALL
Java_org_mozilla_jss_pkcs11_PK11KeyWrapper_nativeWrapSymWithSym
    (JNIEnv *env, jobject this, jobject tokenObj, jobject toBeWrappedObj,
        jobject wrappingKeyObj, jobject algObj, jbyteArray ivBA)
{
    PK11SymKey *wrapping = NULL;
    PK11SymKey *toBeWrapped = NULL;
    CK_MECHANISM_TYPE mech;
    SECItem wrapped;
    jbyteArray wrappedBA=NULL;
    SECItem *iv=NULL, *param=NULL;
    SECStatus status;

    /* initialize so we can goto finish */
    wrapped.data = NULL;
    wrapped.len = 0;

    /* get wrapping key */
    if( JSS_PK11_getSymKeyPtr(env, wrappingKeyObj, &wrapping)!= PR_SUCCESS) {
        JSS_throwMsg(env, TOKEN_EXCEPTION, "Unable to extract symmetric "
                "wrapping key");
        return NULL;
    }

    /* get toBeWrapped key */
    if( JSS_PK11_getSymKeyPtr(env, toBeWrappedObj, &toBeWrapped) != PR_SUCCESS){
        JSS_throwMsg(env, TOKEN_EXCEPTION, "Unable to extract symmetric "
            "to be wrapped key");
        return NULL;
    }

    /* get the mechanism */
    mech = JSS_getPK11MechFromAlg(env, algObj);
    if(mech == CKM_INVALID_MECHANISM) {
        JSS_throwMsg(env, TOKEN_EXCEPTION, "Unrecognized algorithm");
        goto finish;
    }

    /* get the parameter */
    if( ivBA ) {
        iv = JSS_ByteArrayToSECItem(env, ivBA);
        if( iv == NULL ) {
            goto finish; /* exception was thrown */
        }
        param = PK11_ParamFromIV(mech, iv);
        if( param == NULL ) {
            JSS_throwMsg(env, TOKEN_EXCEPTION, "Unable to create mechanism"
                " parameter from initialization vector");
            goto finish;
        }
    }

    /* setup space for wrapped key */
    wrapped.len = MAX_WRAPPED_KEY_LEN; /* yuk! */
    wrapped.data = PR_Malloc(wrapped.len);
    if(wrapped.data == NULL) {
        JSS_throw(env, OUT_OF_MEMORY_ERROR);
        goto finish;
    }

    /* perform the wrap */
    status = PK11_WrapSymKey(mech, param, wrapping, toBeWrapped, &wrapped);
    if( status != SECSuccess ) {
        JSS_throwMsg(env, TOKEN_EXCEPTION, "Wrap operation failed on token");
        goto finish;
    }

    /* package the wrapped data into a byte array */
    wrappedBA = JSS_SECItemToByteArray(env, &wrapped);

finish:
    if(iv) {
        SECITEM_FreeItem(iv, PR_TRUE /*freeit*/);
    }
    if(param) {
        SECITEM_FreeItem(param, PR_TRUE /*freeit*/);
    }
    SECITEM_FreeItem(&wrapped, PR_FALSE /*freeit*/);

    return wrappedBA;
}

/***********************************************************************
 *
 * PK11KeyWrapper.nativeWrapSymWithPub
 */
JNIEXPORT jbyteArray JNICALL
Java_org_mozilla_jss_pkcs11_PK11KeyWrapper_nativeWrapSymWithPub
    (JNIEnv *env, jobject this, jobject tokenObj, jobject toBeWrappedObj,
        jobject wrappingKeyObj, jobject algObj, jbyteArray ivBA)
{
    SECKEYPublicKey *wrapping = NULL;
    PK11SymKey *toBeWrapped = NULL;
    CK_MECHANISM_TYPE mech;
    SECItem wrapped;
    jbyteArray wrappedBA=NULL;
    SECStatus status;

    /* initialize so we can goto finish */
    wrapped.data = NULL;
    wrapped.len = 0;

    /* get wrapping key */
    if( JSS_PK11_getPubKeyPtr(env, wrappingKeyObj, &wrapping)!= PR_SUCCESS) {
        JSS_throwMsg(env, TOKEN_EXCEPTION, "Unable to extract public "
                "wrapping key");
        return NULL;
    }

    /* get toBeWrapped key */
    if( JSS_PK11_getSymKeyPtr(env, toBeWrappedObj, &toBeWrapped) != PR_SUCCESS){
        JSS_throwMsg(env, TOKEN_EXCEPTION, "Unable to extract symmetric "
            "to be wrapped key");
        return NULL;
    }

    /* get the mechanism */
    mech = JSS_getPK11MechFromAlg(env, algObj);
    if(mech == CKM_INVALID_MECHANISM) {
        JSS_throwMsg(env, TOKEN_EXCEPTION, "Unrecognized algorithm");
        goto finish;
    }

    /* setup space for wrapped key */
    wrapped.len = MAX_WRAPPED_KEY_LEN; /* yuk! */
    wrapped.data = PR_Malloc(wrapped.len);
    if(wrapped.data == NULL) {
        JSS_throw(env, OUT_OF_MEMORY_ERROR);
        goto finish;
    }

    /* perform the wrap */
    status = PK11_PubWrapSymKey(mech, wrapping, toBeWrapped, &wrapped);
    if( status != SECSuccess ) {
        JSS_throwMsg(env, TOKEN_EXCEPTION, "Wrap operation failed on token");
        goto finish;
    }

    /* package the wrapped data into a byte array */
    wrappedBA = JSS_SECItemToByteArray(env, &wrapped);

finish:
    SECITEM_FreeItem(&wrapped, PR_FALSE /*freeit*/);

    return wrappedBA;
}


/***********************************************************************
 *
 * PK11KeyWrapper.nativeWrapPrivWithSym
 */
JNIEXPORT jbyteArray JNICALL
Java_org_mozilla_jss_pkcs11_PK11KeyWrapper_nativeWrapPrivWithSym
    (JNIEnv *env, jobject this, jobject tokenObj, jobject toBeWrappedObj,
        jobject wrappingKeyObj, jobject algObj, jbyteArray ivBA)
{
    PK11SymKey *wrapping = NULL;
    SECKEYPrivateKey *toBeWrapped= NULL;
    CK_MECHANISM_TYPE mech;
    SECStatus status;
    SECItem wrapped;
    PK11SlotInfo *slot=NULL;
    jbyteArray wrappedBA=NULL;
    SECItem *iv=NULL, *param=NULL;

    /* setup space for wrapped key */
    wrapped.len = MAX_WRAPPED_KEY_LEN;
    wrapped.data = PR_Malloc(wrapped.len);
    if(wrapped.data == NULL) {
        wrapped.len = 0;
        JSS_throw(env, OUT_OF_MEMORY_ERROR);
        goto finish;
    }

    /* get wrapping key */
    if( JSS_PK11_getSymKeyPtr(env, wrappingKeyObj, &wrapping) != PR_SUCCESS) {
        JSS_throwMsg(env, TOKEN_EXCEPTION, "Unable to extract symmetric "
                "wrapping key");
        return NULL;
    }

    /* get toBeWrapped key */
    if( JSS_PK11_getPrivKeyPtr(env, toBeWrappedObj, &toBeWrapped) !=PR_SUCCESS){
        JSS_throwMsg(env, TOKEN_EXCEPTION, "Unable to extract private "
            "to be wrapped key");
        return NULL;
    }

    /* get the slot */
    if( JSS_PK11_getTokenSlotPtr(env, tokenObj, &slot) != PR_SUCCESS) {
        /* exception was thrown */
        goto finish;
    }

    /* get the mechanism */
    mech = JSS_getPK11MechFromAlg(env, algObj);
    if(mech == CKM_INVALID_MECHANISM) {
        JSS_throwMsg(env, TOKEN_EXCEPTION, "Unrecognized algorithm");
        goto finish;
    }

    /* get the mechanism parameter (IV) */
    if( ivBA ) {
        iv = JSS_ByteArrayToSECItem(env, ivBA);
        if( iv == NULL ) {
            goto finish; /* exception was thrown */
        }
        param = PK11_ParamFromIV(mech, iv);
        if( param == NULL ) {
            JSS_throwMsg(env, TOKEN_EXCEPTION,
                "Failed to convert initialization vector to parameter");
            goto finish;
        }
    }

    /* perform the wrap operation */
    status = PK11_WrapPrivKey(slot, wrapping, toBeWrapped, mech, param,
                &wrapped, NULL /* wincx */ );
    if(status != SECSuccess) {
        JSS_throwMsg(env, TOKEN_EXCEPTION,
                "Wrapping operation failed on token");
        goto finish;
    }
    PR_ASSERT(wrapped.len>0 && wrapped.data!=NULL);

    /* package the wrapped data into a byte array */
    wrappedBA = JSS_SECItemToByteArray(env, &wrapped);
    /* fall through if an exception occurs */

finish:
    if(iv) {
        SECITEM_FreeItem(iv, PR_TRUE /*freeit*/);
    }
    if(param) {
        SECITEM_FreeItem(param, PR_TRUE /*freeit*/);
    }
    SECITEM_FreeItem(&wrapped, PR_FALSE /*freeit*/);

    return wrappedBA;
}

/***********************************************************************
 *
 * PK11KeyWrapper.nativeUnwrapPrivWithSym
 */
JNIEXPORT jobject JNICALL
Java_org_mozilla_jss_pkcs11_PK11KeyWrapper_nativeUnwrapPrivWithSym
    (JNIEnv *env, jclass clazz, jobject tokenObj, jobject unwrapperObj,
        jbyteArray wrappedBA, jobject wrapAlgObj, jobject typeAlgObj,
        jbyteArray publicValueBA, jbyteArray ivBA, jboolean temporary)
{
    PK11SlotInfo *slot;
    PK11SymKey *unwrappingKey;
    SECKEYPrivateKey *privk=NULL;
    jobject privkObj=NULL;
    CK_MECHANISM_TYPE wrapType, keyTypeMech;
    CK_KEY_TYPE keyType;
    SECItem *wrapped=NULL, *iv=NULL, *param=NULL, *pubValue=NULL;
    SECItem label; /* empty secitem, doesn't need to be freed */
    PRBool token;
    CK_ATTRIBUTE_TYPE attribs[4];
    int numAttribs;

    if( JSS_PK11_getTokenSlotPtr(env, tokenObj, &slot) != PR_SUCCESS) {
        /* exception was thrown */
        goto finish;
    }

    /* get unwrapping key */
    if( JSS_PK11_getSymKeyPtr(env, unwrapperObj, &unwrappingKey)
            != PR_SUCCESS) {
        /* exception was thrown */
        goto finish;
    }

    /* get the wrapping mechanism */
    wrapType = JSS_getPK11MechFromAlg(env, wrapAlgObj);
    if(wrapType == CKM_INVALID_MECHANISM) {
        JSS_throwMsg(env, TOKEN_EXCEPTION, "Unknown algorithm");
        goto finish;
    }

    /* get the mechanism parameter (IV) */
    if( ivBA ) {
        iv = JSS_ByteArrayToSECItem(env, ivBA);
        if( iv == NULL ) {
            goto finish; /* exception was thrown */
        }
        param = PK11_ParamFromIV(wrapType, iv);
        if( param == NULL ) {
            JSS_throwMsg(env, TOKEN_EXCEPTION,
                "Failed to convert initialization vector to parameter");
            goto finish;
        }
    }

    /* get wrapped key from byte array */
    wrapped = JSS_ByteArrayToSECItem(env, wrappedBA);
    if( wrapped == NULL ) {
        /* exception was thrown */
        goto finish;
    }

    /* initialize the label to be empty */
    label.len = 0;
    label.data = NULL;

    /* get the public value */
    pubValue = JSS_ByteArrayToSECItem(env, publicValueBA);
    if(pubValue == NULL ) {
        /* exception was thrown */
        goto finish;
    }

    if( temporary ) {
        token = PR_FALSE;
    } else {
        token = PR_TRUE;
    }

    /* get key type */
    keyTypeMech = JSS_getPK11MechFromAlg(env, typeAlgObj);
    if( keyTypeMech == CKM_INVALID_MECHANISM ) {
        JSS_throwMsg(env, TOKEN_EXCEPTION, "Unrecognized key type algorithm");
        goto finish;
    }
    keyType = PK11_GetKeyType(keyTypeMech, 0);

    /* figure out which operations to enable for this key */
    if( keyType == CKK_RSA ) {
        attribs[0] = CKA_SIGN;
        attribs[1] = CKA_DECRYPT;
        attribs[2] = CKA_SIGN_RECOVER;
        attribs[3] = CKA_UNWRAP;
        numAttribs = 4;
    } else if(keyType == CKK_DSA) {
        attribs[0] = CKA_SIGN;
        numAttribs = 1;
    } else {
        /* unknown key type */
        PR_ASSERT(PR_FALSE);
    }

    /* perform the unwrap */
    privk = PK11_UnwrapPrivKey(slot, unwrappingKey, wrapType, param, wrapped,
                &label, pubValue, token, PR_TRUE /*sensitive*/, keyType,
                attribs, numAttribs, NULL /*wincx*/);
    if( privk == NULL ) {
        JSS_throwMsg(env, TOKEN_EXCEPTION, "Key Unwrap failed on token");
        goto finish;
    }
                
    /* stuff the privk into a Java private key object. This sets privk to
     * NULL */
    privkObj = JSS_PK11_wrapPrivKey(env, &privk);

finish:
    if(iv) {
        SECITEM_FreeItem(iv, PR_TRUE /*free iv*/);
    }
    if(param) {
        SECITEM_FreeItem(param, PR_TRUE /*free param*/);
    }
    if(wrapped) {
        SECITEM_FreeItem(wrapped, PR_TRUE /*free wrapped*/);
    }
    if(pubValue) {
        SECITEM_FreeItem(pubValue, PR_TRUE /*free pubValue*/);
    }
    PR_ASSERT(privk==NULL);
    PR_ASSERT( privkObj!=NULL || (*env)->ExceptionOccurred(env) );
    return privkObj;
}

/***********************************************************************
 *
 * PK11KeyWrapper.nativeUnwrapSymWithSym
 */
JNIEXPORT jobject JNICALL
Java_org_mozilla_jss_pkcs11_PK11KeyWrapper_nativeUnwrapSymWithSym
    (JNIEnv *env, jclass clazz, jobject tokenObj, jobject unwrapperObj,
        jbyteArray wrappedBA, jobject wrapAlgObj, jobject typeAlgObj,
        jint keyLen, jbyteArray ivBA)
{
    CK_BBOOL cktrue = CK_TRUE; /* sigh */
    CK_OBJECT_CLASS keyClass = CKO_SECRET_KEY;
    CK_KEY_TYPE keyType = CKK_GENERIC_SECRET;
    CK_ULONG key_size = 0;
    CK_ATTRIBUTE keyTemplate[] = {
        { CKA_CLASS, NULL, 0},
        { CKA_KEY_TYPE, NULL, 0},
        { CKA_ENCRYPT, NULL, 0},
        { CKA_DECRYPT, NULL, 0},
        { CKA_WRAP, NULL, 0},
        { CKA_UNWRAP, NULL, 0},
        { CKA_VALUE_LEN, NULL, 0},
    };
    int templateCount = sizeof(keyTemplate)/sizeof(keyTemplate[0]);
    PK11SymKey *symKey=NULL, *wrappingKey=NULL;
    CK_MECHANISM mechanism;
    CK_MECHANISM_TYPE wrappingMech, keyTypeMech;
    CK_RV crv;
    CK_ATTRIBUTE *attrs = keyTemplate;
    PK11SlotInfo *slot=NULL;
    SECItem *wrappedKey=NULL, *iv=NULL, *param=NULL;
    jobject keyObj = NULL;

    /* get the slot */
    if( JSS_PK11_getTokenSlotPtr(env, tokenObj, &slot) != PR_SUCCESS) {
        /* exception was thrown */
        goto finish;
    }

    /* get key type */
    keyTypeMech = JSS_getPK11MechFromAlg(env, typeAlgObj);
    if( keyTypeMech == CKM_INVALID_MECHANISM ) {
        JSS_throwMsg(env, TOKEN_EXCEPTION, "Unrecognized key type algorithm");
        goto finish;
    }

    /* get wrapping key */
    if( JSS_PK11_getSymKeyPtr(env, unwrapperObj, &wrappingKey) != PR_SUCCESS) {
        /* exception was thrown */
        goto finish;
    }

    /* get wrapping mechanism */
    wrappingMech = JSS_getPK11MechFromAlg(env, wrapAlgObj);
    if( keyTypeMech == CKM_INVALID_MECHANISM ) {
        JSS_throwMsg(env, TOKEN_EXCEPTION, "Unrecognized wrapping algorithm");
        goto finish;
    }

    /* set up the attribute template for the key */
    PK11_SETATTRS(attrs, CKA_CLASS, &keyClass, sizeof(keyClass)); attrs++;
    PK11_SETATTRS(attrs, CKA_KEY_TYPE, &keyType, sizeof(keyType)); attrs++;
    PK11_SETATTRS(attrs, CKA_ENCRYPT, &cktrue, 1); attrs++;
    PK11_SETATTRS(attrs, CKA_DECRYPT, &cktrue, 1); attrs++;
    PK11_SETATTRS(attrs, CKA_WRAP, &cktrue, 1); attrs++;
    PK11_SETATTRS(attrs, CKA_UNWRAP, &cktrue, 1); attrs++;
    PK11_SETATTRS(attrs, CKA_VALUE_LEN, &key_size, sizeof(key_size)); attrs++;

    /* create a key Structure */
    symKey = PK11_CreateSymKey(slot,keyTypeMech,NULL /*wincx*/);
    if (symKey == NULL) {
        JSS_throwMsg(env, TOKEN_EXCEPTION,
            "Failed to create new symmetric key handle");
        goto finish;
    }

    keyType = PK11_GetKeyType(keyTypeMech, keyLen);
    key_size = keyLen;
    symKey->size = keyLen;
    symKey->origin = PK11_OriginUnwrap;
    if (key_size == 0) templateCount--;

    mechanism.mechanism = wrappingMech;
    /* get the mechanism parameter (IV) */
    if (ivBA == NULL) {
        param = PK11_ParamFromIV(wrappingMech,NULL);
    } else {
        iv = JSS_ByteArrayToSECItem(env, ivBA);
        if( iv == NULL ) {
            goto finish; /* exception was thrown */
        }
        param = PK11_ParamFromIV(wrappingMech, iv);
        if( param == NULL ) {
            JSS_throwMsg(env, TOKEN_EXCEPTION,
                "Failed to convert initialization vector to parameter");
            goto finish;
        }
    }
    if (param) {
        mechanism.pParameter = param->data;
        mechanism.ulParameterLen = param->len;
    } else {
        mechanism.pParameter = NULL;
        mechanism.ulParameterLen = 0;
    }

    /* get the wrapped key */
    wrappedKey = JSS_ByteArrayToSECItem(env, wrappedBA);
    if( wrappedKey == NULL ) {
        /* exception was thrown */
        goto finish;
    }

    pk11_EnterKeyMonitor(symKey);
    crv = PK11_GETTAB(slot)->C_UnwrapKey(symKey->session,&mechanism,
                wrappingKey->objectID,
                wrappedKey->data, wrappedKey->len, keyTemplate, templateCount,
                                                          &symKey->objectID);
    pk11_ExitKeyMonitor(symKey);
    if( crv != CKR_OK ) {
        JSS_throwMsg(env, TOKEN_EXCEPTION, "Failed to unwrap key");
            goto finish;
   }

    /* wrap the symmetric key in a Java object.  This will clear symKey */
    keyObj = JSS_PK11_wrapSymKey(env, &symKey);

finish:
    if(wrappedKey) {
        SECITEM_FreeItem(wrappedKey, PR_TRUE /*free wrappedKey*/);
    }
    if(iv) {
        SECITEM_FreeItem(iv, PR_TRUE /*free iv*/);
    }
    if(param) {
        SECITEM_FreeItem(param, PR_TRUE /*free param*/);
    }
    if( symKey ) {
        PK11_FreeSymKey(symKey);
    }
    return keyObj;
}

/***********************************************************************
 *
 * PK11KeyWrapper.nativeUnwrapSymWithPriv
 */
JNIEXPORT jobject JNICALL
Java_org_mozilla_jss_pkcs11_PK11KeyWrapper_nativeUnwrapSymWithPriv
    (JNIEnv *env, jclass clazz, jobject tokenObj, jobject unwrapperObj,
        jbyteArray wrappedBA, jobject wrapAlgObj, jobject typeAlgObj,
        jint keyLen, jbyteArray ivBA)
{
    CK_BBOOL cktrue = CK_TRUE; /* sigh */
    CK_OBJECT_CLASS keyClass = CKO_SECRET_KEY;
    CK_KEY_TYPE keyType = CKK_GENERIC_SECRET;
    CK_ULONG key_size = 0;
    CK_ATTRIBUTE keyTemplate[] = {
        { CKA_CLASS, NULL, 0},
        { CKA_KEY_TYPE, NULL, 0},
        { CKA_ENCRYPT, NULL, 0},
        { CKA_DECRYPT, NULL, 0},
        { CKA_WRAP, NULL, 0},
        { CKA_UNWRAP, NULL, 0},
        { CKA_VALUE_LEN, NULL, 0},
    };
    int templateCount = sizeof(keyTemplate)/sizeof(keyTemplate[0]);
    PK11SymKey *symKey=NULL;
    CK_MECHANISM mechanism;
    CK_MECHANISM_TYPE wrappingMech, keyTypeMech;
    CK_RV crv;
    CK_ATTRIBUTE *attrs = keyTemplate;
    PK11SlotInfo *slot=NULL;
    SECItem *wrappedKey=NULL, *iv=NULL, *param=NULL;
    jobject keyObj=NULL;
    SECKEYPrivateKey *wrappingKey=NULL;

    /* get the slot */
    if( JSS_PK11_getTokenSlotPtr(env, tokenObj, &slot) != PR_SUCCESS) {
        /* exception was thrown */
        goto finish;
    }

    /* since this is a private key op, we may need to login */
    PK11_HandlePasswordCheck(slot, NULL /*wincx*/);

    /* get key type */
    keyTypeMech = JSS_getPK11MechFromAlg(env, typeAlgObj);
    if( keyTypeMech == CKM_INVALID_MECHANISM ) {
        JSS_throwMsg(env, TOKEN_EXCEPTION, "Unrecognized key type algorithm");
        goto finish;
    }

    /* get wrapping key */
    if( JSS_PK11_getPrivKeyPtr(env, unwrapperObj, &wrappingKey) != PR_SUCCESS) {
        /* exception was thrown */
        goto finish;
    }

    /* get wrapping mechanism */
    wrappingMech = JSS_getPK11MechFromAlg(env, wrapAlgObj);
    if( keyTypeMech == CKM_INVALID_MECHANISM ) {
        JSS_throwMsg(env, TOKEN_EXCEPTION, "Unrecognized wrapping algorithm");
        goto finish;
    }

    /* set up the attribute template for the key */
    PK11_SETATTRS(attrs, CKA_CLASS, &keyClass, sizeof(keyClass)); attrs++;
    PK11_SETATTRS(attrs, CKA_KEY_TYPE, &keyType, sizeof(keyType)); attrs++;
    PK11_SETATTRS(attrs, CKA_ENCRYPT, &cktrue, 1); attrs++;
    PK11_SETATTRS(attrs, CKA_DECRYPT, &cktrue, 1); attrs++;
    PK11_SETATTRS(attrs, CKA_WRAP, &cktrue, 1); attrs++;
    PK11_SETATTRS(attrs, CKA_UNWRAP, &cktrue, 1); attrs++;
    PK11_SETATTRS(attrs, CKA_VALUE_LEN, &key_size, sizeof(key_size)); attrs++;

    /* create a key Structure */
    symKey = PK11_CreateSymKey(slot,keyTypeMech,NULL /*wincx*/);
    if (symKey == NULL) {
        JSS_throwMsg(env, TOKEN_EXCEPTION,
            "Failed to create new symmetric key handle");
        goto finish;
    }

    keyType = PK11_GetKeyType(keyTypeMech, keyLen);
    key_size = keyLen;
    symKey->size = keyLen;
    symKey->origin = PK11_OriginUnwrap;
    if (key_size == 0) templateCount--;

    mechanism.mechanism = wrappingMech;
    /* get the mechanism parameter (IV) */
    if (ivBA == NULL) {
        param = PK11_ParamFromIV(wrappingMech, NULL);
    } else {
        iv = JSS_ByteArrayToSECItem(env, ivBA);
        if( iv == NULL ) {
            goto finish; /* exception was thrown */
        }
        param = PK11_ParamFromIV(wrappingMech, iv);
        if( param == NULL ) {
            JSS_throwMsg(env, TOKEN_EXCEPTION,
                "Failed to convert initialization vector to parameter");
            goto finish;
        }
    }
    if (param) {
        mechanism.pParameter = param->data;
        mechanism.ulParameterLen = param->len;
    } else {
        mechanism.pParameter = NULL;
        mechanism.ulParameterLen = 0;
    }

    /* get the wrapped key */
    wrappedKey = JSS_ByteArrayToSECItem(env, wrappedBA);
    if( wrappedKey == NULL ) {
        /* exception was thrown */
        goto finish;
    }

    pk11_EnterKeyMonitor(symKey);
    crv = PK11_GETTAB(slot)->C_UnwrapKey(symKey->session,&mechanism,
                wrappingKey->pkcs11ID,
                wrappedKey->data, wrappedKey->len, keyTemplate, templateCount,
                                                          &symKey->objectID);
    pk11_ExitKeyMonitor(symKey);
    if( crv != CKR_OK ) {
        JSS_throwMsg(env, TOKEN_EXCEPTION, "Failed to unwrap key");
        goto finish;
    }
    
    /* Put the symmetric key into a Java object.  This will clear symKey */
    keyObj = JSS_PK11_wrapSymKey(env, &symKey);

finish:
    if(wrappedKey) {
        SECITEM_FreeItem(wrappedKey, PR_TRUE /*free wrappedKey*/);
    }
    if(iv) {
        SECITEM_FreeItem(iv, PR_TRUE /*free iv*/);
    }
    if(param) {
        SECITEM_FreeItem(param, PR_TRUE /*free param*/);
    }
    if( symKey ) {
        PK11_FreeSymKey(symKey);
    }
    return keyObj;
}

/***********************************************************************
 *
 * J S S _ P K 1 1 _ g e t E r r o r S t r i n g
 *
 * Returns a simple error string for a given PKCS #11 error.
 */
PR_IMPLEMENT( char* )
JSS_PK11_getErrorString(CK_RV crv)
{
    switch(crv) {
      case CKR_ATTRIBUTE_READ_ONLY:
        return "CKR_ATTRIBUTE_READ_ONLY";
      case CKR_ATTRIBUTE_TYPE_INVALID:
        return "CKR_ATTRIBUTE_TYPE_INVALID";
      case CKR_ATTRIBUTE_VALUE_INVALID:
        return "CKR_ATTRIBUTE_VALUE_INVALID";
      case CKR_BUFFER_TOO_SMALL:
        return "CKR_BUFFER_TOO_SMALL";
      case CKR_CRYPTOKI_NOT_INITIALIZED:
        return "CKR_CRYPTOKI_NOT_INITIALIZED";
      case CKR_DEVICE_ERROR:
        return "CKR_DEVICE_ERROR";
      case CKR_DEVICE_MEMORY:
        return "CKR_DEVICE_MEMORY";
      case CKR_DEVICE_REMOVED:
        return "CKR_DEVICE_REMOVED";
      case CKR_FUNCTION_CANCELED:
        return "CKR_FUNCTION_CANCELED";
      case CKR_FUNCTION_FAILED:
        return "CKR_FUNCTION_FAILED";
      case CKR_GENERAL_ERROR:
        return "CKR_GENERAL_ERROR";
      case CKR_HOST_MEMORY:
        return "CKR_HOST_MEMORY";
      case CKR_KEY_HANDLE_INVALID:
        return "CKR_KEY_HANDLE_INVALID";
      case CKR_KEY_NOT_WRAPPABLE:
        return "CKR_KEY_NOT_WRAPPABLE";
      case CKR_KEY_SIZE_RANGE:
        return "CKR_KEY_SIZE_RANGE";
      case CKR_KEY_UNEXTRACTABLE:
        return "CKR_KEY_UNEXTRACTABLE";
      case CKR_MECHANISM_INVALID:
        return "CKR_MECHANISM_INVALID";
      case CKR_MECHANISM_PARAM_INVALID:
        return "CKR_MECHANISM_PARAM_INVALID";
      case CKR_OK:
        return "CKR_OK";
      case CKR_OPERATION_ACTIVE:
        return "CKR_OPERATION_ACTIVE";
      case CKR_SESSION_CLOSED:
        return "CKR_SESSION_CLOSED";
      case CKR_SESSION_HANDLE_INVALID:
        return "CKR_SESSION_HANDLE_INVALID";
      case CKR_SESSION_READ_ONLY:
        return "CKR_SESSION_READ_ONLY";
      case CKR_TEMPLATE_INCOMPLETE:
        return "CKR_TEMPLATE_INCOMPLETE";
      case CKR_TEMPLATE_INCONSISTENT:
        return "CKR_TEMPLATE_INCONSISTENT";
      case CKR_TOKEN_WRITE_PROTECTED:
        return "CKR_TOKEN_WRITE_PROTECTED";
      case CKR_UNWRAPPING_KEY_HANDLE_INVALID:
        return "CKR_UNWRAPPING_KEY_HANDLE_INVALID";
      case CKR_UNWRAPPING_KEY_SIZE_RANGE:
        return "CKR_UNWRAPPING_KEY_SIZE_RANGE";
      case CKR_UNWRAPPING_KEY_TYPE_INCONSISTENT:
        return "CKR_UNWRAPPING_KEY_TYPE_INCONSISTENT";
      case CKR_USER_NOT_LOGGED_IN:
        return "CKR_USER_NOT_LOGGED_IN";
      case CKR_WRAPPED_KEY_INVALID:
        return "CKR_WRAPPED_KEY_INVALID";
      case CKR_WRAPPED_KEY_LEN_RANGE:
        return "CKR_WRAPPED_KEY_LEN_RANGE";
      case CKR_WRAPPING_KEY_HANDLE_INVALID:
        return "CKR_WRAPPING_KEY_HANDLE_INVALID";
      case CKR_WRAPPING_KEY_SIZE_RANGE:
        return "CKR_WRAPPING_KEY_SIZE_RANGE";
      case CKR_WRAPPING_KEY_TYPE_INCONSISTENT:
        return "CKR_WRAPPING_KEY_TYPE_INCONSISTENT";
      default:
        return "PKCS #11 error";
    }
}
