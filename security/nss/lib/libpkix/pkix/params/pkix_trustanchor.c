/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Netscape security libraries.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1994-2000
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Sun Microsystems
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
/*
 * pkix_trustanchor.c
 *
 * TrustAnchor Object Functions
 *
 */

#include "pkix_trustanchor.h"

/* --Private-Functions-------------------------------------------- */

/*
 * FUNCTION: pkix_TrustAnchor_Destroy
 * (see comments for PKIX_PL_DestructorCallback in pkix_pl_system.h)
 */
static PKIX_Error *
pkix_TrustAnchor_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_TrustAnchor *anchor = NULL;

        PKIX_ENTER(TRUSTANCHOR, "pkix_TrustAnchor_Destroy");
        PKIX_NULLCHECK_ONE(object);

        /* Check that this object is a trust anchor */
        PKIX_CHECK(pkix_CheckType(object, PKIX_TRUSTANCHOR_TYPE, plContext),
                    PKIX_OBJECTNOTTRUSTANCHOR);

        anchor = (PKIX_TrustAnchor *)object;

        PKIX_DECREF(anchor->trustedCert);
        PKIX_DECREF(anchor->caName);
        PKIX_DECREF(anchor->caPubKey);
        PKIX_DECREF(anchor->nameConstraints);

cleanup:

        PKIX_RETURN(TRUSTANCHOR);
}

/*
 * FUNCTION: pkix_TrustAnchor_Equals
 * (see comments for PKIX_PL_EqualsCallback in pkix_pl_system.h)
 */
static PKIX_Error *
pkix_TrustAnchor_Equals(
        PKIX_PL_Object *first,
        PKIX_PL_Object *second,
        PKIX_Boolean *pResult,
        void *plContext)
{
        PKIX_UInt32 secondType;
        PKIX_Boolean cmpResult;
        PKIX_TrustAnchor *firstAnchor = NULL;
        PKIX_TrustAnchor *secondAnchor = NULL;
        PKIX_PL_Cert *firstCert = NULL;
        PKIX_PL_Cert *secondCert = NULL;

        PKIX_ENTER(TRUSTANCHOR, "pkix_TrustAnchor_Equals");
        PKIX_NULLCHECK_THREE(first, second, pResult);

        PKIX_CHECK(pkix_CheckType(first, PKIX_TRUSTANCHOR_TYPE, plContext),
                    PKIX_FIRSTOBJECTNOTTRUSTANCHOR);

        PKIX_CHECK(PKIX_PL_Object_GetType(second, &secondType, plContext),
                    PKIX_COULDNOTGETTYPEOFSECONDARGUMENT);

        *pResult = PKIX_FALSE;

        if (secondType != PKIX_TRUSTANCHOR_TYPE) goto cleanup;

        firstAnchor = (PKIX_TrustAnchor *)first;
        secondAnchor = (PKIX_TrustAnchor *)second;

        firstCert = firstAnchor->trustedCert;
        secondCert = secondAnchor->trustedCert;

        if ((firstCert && !secondCert) || (!firstCert && secondCert)){
                goto cleanup;
        }

        if (firstCert && secondCert){
                PKIX_CHECK(PKIX_PL_Object_Equals
                            ((PKIX_PL_Object *)firstCert,
                            (PKIX_PL_Object *)secondCert,
                            &cmpResult,
                            plContext),
                            PKIX_OBJECTEQUALSFAILED);
        } else {
                PKIX_CHECK(PKIX_PL_Object_Equals
                            ((PKIX_PL_Object *)firstAnchor->caName,
                            (PKIX_PL_Object *)secondAnchor->caName,
                            &cmpResult,
                            plContext),
                            PKIX_OBJECTEQUALSFAILED);

                if (!cmpResult) goto cleanup;

                PKIX_CHECK(PKIX_PL_Object_Equals
                            ((PKIX_PL_Object *)firstAnchor->caPubKey,
                            (PKIX_PL_Object *)secondAnchor->caPubKey,
                            &cmpResult,
                            plContext),
                            PKIX_OBJECTEQUALSFAILED);

                if (!cmpResult) goto cleanup;

                PKIX_EQUALS
                        (firstAnchor->nameConstraints,
                        secondAnchor->nameConstraints,
                        &cmpResult,
                        plContext,
                        PKIX_OBJECTEQUALSFAILED);

                if (!cmpResult) goto cleanup;

        }

        *pResult = cmpResult;

cleanup:

        PKIX_RETURN(TRUSTANCHOR);
}

/*
 * FUNCTION: pkix_TrustAnchor_Hashcode
 * (see comments for PKIX_PL_HashcodeCallback in pkix_pl_system.h)
 */
static PKIX_Error *
pkix_TrustAnchor_Hashcode(
        PKIX_PL_Object *object,
        PKIX_UInt32 *pHashcode,
        void *plContext)
{
        PKIX_TrustAnchor *anchor = NULL;
        PKIX_PL_Cert *cert = NULL;
        PKIX_UInt32 hash = 0;
        PKIX_UInt32 certHash = 0;
        PKIX_UInt32 nameHash = 0;
        PKIX_UInt32 pubKeyHash = 0;
        PKIX_UInt32 ncHash = 0;

        PKIX_ENTER(TRUSTANCHOR, "pkix_TrustAnchor_Hashcode");
        PKIX_NULLCHECK_TWO(object, pHashcode);

        PKIX_CHECK(pkix_CheckType(object, PKIX_TRUSTANCHOR_TYPE, plContext),
                    PKIX_OBJECTNOTTRUSTANCHOR);

        anchor = (PKIX_TrustAnchor*)object;
        cert = anchor->trustedCert;

        if (cert){
                PKIX_CHECK(PKIX_PL_Object_Hashcode
                            ((PKIX_PL_Object *)cert,
                            &certHash,
                            plContext),
                            PKIX_OBJECTHASHCODEFAILED);

                hash = certHash;

        } else {
                PKIX_CHECK(PKIX_PL_Object_Hashcode
                            ((PKIX_PL_Object *)anchor->caName,
                            &nameHash,
                            plContext),
                            PKIX_OBJECTHASHCODEFAILED);

                PKIX_CHECK(PKIX_PL_Object_Hashcode
                            ((PKIX_PL_Object *)anchor->caPubKey,
                            &pubKeyHash,
                            plContext),
                            PKIX_OBJECTHASHCODEFAILED);

                PKIX_HASHCODE(anchor->nameConstraints, &ncHash, plContext,
                        PKIX_OBJECTHASHCODEFAILED);

                hash = 31 * nameHash + pubKeyHash + ncHash;

        }

        *pHashcode = hash;

cleanup:

        PKIX_RETURN(TRUSTANCHOR);
}

/*
 * FUNCTION: pkix_TrustAnchor_ToString
 * (see comments for PKIX_PL_ToStringCallback in pkix_pl_system.h)
 */
static PKIX_Error *
pkix_TrustAnchor_ToString(
        PKIX_PL_Object *object,
        PKIX_PL_String **pString,
        void *plContext)
{
        PKIX_TrustAnchor *anchor = NULL;
        char *asciiFormat = NULL;
        PKIX_PL_String *formatString = NULL;
        PKIX_PL_String *anchorString = NULL;
        PKIX_PL_String *certString = NULL;
        PKIX_PL_String *nameString = NULL;
        PKIX_PL_String *pubKeyString = NULL;
        PKIX_PL_String *nameConstraintsString = NULL;

        PKIX_ENTER(TRUSTANCHOR, "pkix_TrustAnchor_ToString");
        PKIX_NULLCHECK_TWO(object, pString);

        PKIX_CHECK(pkix_CheckType(object, PKIX_TRUSTANCHOR_TYPE, plContext),
                    PKIX_OBJECTNOTTRUSTANCHOR);

        anchor = (PKIX_TrustAnchor*)object;

        if (anchor->trustedCert){
                asciiFormat =
                        "[\n"
                        "\tTrusted Cert:	%s\n"
                        "]\n";

                PKIX_CHECK(PKIX_PL_String_Create
                            (PKIX_ESCASCII,
                            asciiFormat,
                            0,
                            &formatString,
                            plContext),
                            PKIX_STRINGCREATEFAILED);

                PKIX_CHECK(PKIX_PL_Object_ToString
                            ((PKIX_PL_Object *)anchor->trustedCert,
                            &certString,
                            plContext),
                            PKIX_OBJECTTOSTRINGFAILED);

                PKIX_CHECK(PKIX_PL_Sprintf
                            (&anchorString,
                            plContext,
                            formatString,
                            certString),
                            PKIX_SPRINTFFAILED);
        } else {
                asciiFormat =
                        "[\n"
                        "\tTrusted CA Name:         %s\n"
                        "\tTrusted CA PublicKey:    %s\n"
                        "\tInitial Name Constraints:%s\n"
                        "]\n";

                PKIX_CHECK(PKIX_PL_String_Create
                            (PKIX_ESCASCII,
                            asciiFormat,
                            0,
                            &formatString,
                            plContext),
                            PKIX_STRINGCREATEFAILED);

                PKIX_CHECK(PKIX_PL_Object_ToString
                            ((PKIX_PL_Object *)anchor->caName,
                            &nameString,
                            plContext),
                            PKIX_OBJECTTOSTRINGFAILED);

                PKIX_CHECK(PKIX_PL_Object_ToString
                            ((PKIX_PL_Object *)anchor->caPubKey,
                            &pubKeyString,
                            plContext),
                            PKIX_OBJECTTOSTRINGFAILED);

                PKIX_TOSTRING
                        (anchor->nameConstraints,
                        &nameConstraintsString,
                        plContext,
                        PKIX_OBJECTTOSTRINGFAILED);

                PKIX_CHECK(PKIX_PL_Sprintf
                            (&anchorString,
                            plContext,
                            formatString,
                            nameString,
                            pubKeyString,
                            nameConstraintsString),
                            PKIX_SPRINTFFAILED);
        }

        *pString = anchorString;

cleanup:

        PKIX_DECREF(formatString);
        PKIX_DECREF(certString);
        PKIX_DECREF(nameString);
        PKIX_DECREF(pubKeyString);
        PKIX_DECREF(nameConstraintsString);

        PKIX_RETURN(TRUSTANCHOR);
}

/*
 * FUNCTION: pkix_TrustAnchor_RegisterSelf
 * DESCRIPTION:
 *  Registers PKIX_TRUSTANCHOR_TYPE and its related functions with
 *  systemClasses[]
 * THREAD SAFETY:
 *  Not Thread Safe - for performance and complexity reasons
 *
 *  Since this function is only called by PKIX_PL_Initialize, which should
 *  only be called once, it is acceptable that this function is not
 *  thread-safe.
 */
PKIX_Error *
pkix_TrustAnchor_RegisterSelf(void *plContext)
{
        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER(TRUSTANCHOR, "pkix_TrustAnchor_RegisterSelf");

        entry.description = "TrustAnchor";
        entry.destructor = pkix_TrustAnchor_Destroy;
        entry.equalsFunction = pkix_TrustAnchor_Equals;
        entry.hashcodeFunction = pkix_TrustAnchor_Hashcode;
        entry.toStringFunction = pkix_TrustAnchor_ToString;
        entry.comparator = NULL;
        entry.duplicateFunction = pkix_duplicateImmutable;

        systemClasses[PKIX_TRUSTANCHOR_TYPE] = entry;

        PKIX_RETURN(TRUSTANCHOR);
}

/* --Public-Functions--------------------------------------------- */


/*
 * FUNCTION: PKIX_TrustAnchor_CreateWithCert (see comments in pkix_params.h)
 */
PKIX_Error *
PKIX_TrustAnchor_CreateWithCert(
        PKIX_PL_Cert *cert,
        PKIX_TrustAnchor **pAnchor,
        void *plContext)
{
        PKIX_TrustAnchor *anchor = NULL;

        PKIX_ENTER(TRUSTANCHOR, "PKIX_TrustAnchor_CreateWithCert");
        PKIX_NULLCHECK_TWO(cert, pAnchor);

        PKIX_CHECK(PKIX_PL_Object_Alloc
                    (PKIX_TRUSTANCHOR_TYPE,
                    sizeof (PKIX_TrustAnchor),
                    (PKIX_PL_Object **)&anchor,
                    plContext),
                    PKIX_COULDNOTCREATETRUSTANCHOROBJECT);

        /* initialize fields */
        PKIX_INCREF(cert);
        anchor->trustedCert = cert;

        anchor->caName = NULL;
        anchor->caPubKey = NULL;
        anchor->nameConstraints = NULL;

        *pAnchor = anchor;

cleanup:

        PKIX_RETURN(TRUSTANCHOR);

}

/*
 * FUNCTION: PKIX_TrustAnchor_CreateWithNameKeyPair
 *      (see comments in pkix_params.h)
 */
PKIX_Error *
PKIX_TrustAnchor_CreateWithNameKeyPair(
        PKIX_PL_X500Name *name,
        PKIX_PL_PublicKey *pubKey,
        PKIX_PL_CertNameConstraints *nameConstraints,
        PKIX_TrustAnchor **pAnchor,
        void *plContext)
{
        PKIX_TrustAnchor *anchor = NULL;

        PKIX_ENTER(TRUSTANCHOR, "PKIX_TrustAnchor_CreateWithNameKeyPair");
        PKIX_NULLCHECK_THREE(name, pubKey, pAnchor);

        PKIX_CHECK(PKIX_PL_Object_Alloc
                    (PKIX_TRUSTANCHOR_TYPE,
                    sizeof (PKIX_TrustAnchor),
                    (PKIX_PL_Object **)&anchor,
                    plContext),
                    PKIX_COULDNOTCREATETRUSTANCHOROBJECT);

        /* initialize fields */
        anchor->trustedCert = NULL;

        PKIX_INCREF(name);
        anchor->caName = name;

        PKIX_INCREF(pubKey);
        anchor->caPubKey = pubKey;

        PKIX_INCREF(nameConstraints);
        anchor->nameConstraints = nameConstraints;

        *pAnchor = anchor;

cleanup:

        PKIX_RETURN(TRUSTANCHOR);
}

/*
 * FUNCTION: PKIX_TrustAnchor_GetTrustedCert (see comments in pkix_params.h)
 */
PKIX_Error *
PKIX_TrustAnchor_GetTrustedCert(
        PKIX_TrustAnchor *anchor,
        PKIX_PL_Cert **pCert,
        void *plContext)
{
        PKIX_ENTER(TRUSTANCHOR, "PKIX_TrustAnchor_GetTrustedCert");
        PKIX_NULLCHECK_TWO(anchor, pCert);

        PKIX_INCREF(anchor->trustedCert);

        *pCert = anchor->trustedCert;

        PKIX_RETURN(TRUSTANCHOR);

}

/*
 * FUNCTION: PKIX_TrustAnchor_GetCAName (see comments in pkix_params.h)
 */
PKIX_Error *
PKIX_TrustAnchor_GetCAName(
        PKIX_TrustAnchor *anchor,
        PKIX_PL_X500Name **pCAName,
        void *plContext)
{
        PKIX_ENTER(TRUSTANCHOR, "PKIX_TrustAnchor_GetCAName");
        PKIX_NULLCHECK_TWO(anchor, pCAName);

        PKIX_INCREF(anchor->caName);

        *pCAName = anchor->caName;

        PKIX_RETURN(TRUSTANCHOR);

}

/*
 * FUNCTION: PKIX_TrustAnchor_GetCAPublicKey (see comments in pkix_params.h)
 */
PKIX_Error *
PKIX_TrustAnchor_GetCAPublicKey(
        PKIX_TrustAnchor *anchor,
        PKIX_PL_PublicKey **pPubKey,
        void *plContext)
{
        PKIX_ENTER(TRUSTANCHOR, "PKIX_TrustAnchor_GetCAPublicKey");
        PKIX_NULLCHECK_TWO(anchor, pPubKey);

        PKIX_INCREF(anchor->caPubKey);

        *pPubKey = anchor->caPubKey;

        PKIX_RETURN(TRUSTANCHOR);
}

/*
 * FUNCTION: PKIX_TrustAnchor_GetNameConstraints
 *      (see comments in pkix_params.h)
 */
PKIX_Error *
PKIX_TrustAnchor_GetNameConstraints(
        PKIX_TrustAnchor *anchor,
        PKIX_PL_CertNameConstraints **pNameConstraints,
        void *plContext)
{
        PKIX_ENTER(TRUSTANCHOR, "PKIX_TrustAnchor_GetNameConstraints");
        PKIX_NULLCHECK_TWO(anchor, pNameConstraints);

        PKIX_INCREF(anchor->nameConstraints);

        *pNameConstraints = anchor->nameConstraints;

        PKIX_RETURN(TRUSTANCHOR);
}
