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
 * The Original Code is the Netscape security libraries.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 1994-2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s):
 *  John Gardiner Myers <jgmyers@speakeasy.net>
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

#include "cert.h"
#include "secoid.h"
#include "secder.h"	/* XXX remove this when remove the DERTemplates */
#include "secasn1.h"
#include "secitem.h"
#include <stdarg.h>
#include "secerr.h"
#include "certi.h"

static const SEC_ASN1Template cert_AVATemplate[] = {
    { SEC_ASN1_SEQUENCE,
	  0, NULL, sizeof(CERTAVA) },
    { SEC_ASN1_OBJECT_ID,
	  offsetof(CERTAVA,type), },
    { SEC_ASN1_ANY,
	  offsetof(CERTAVA,value), },
    { 0, }
};

const SEC_ASN1Template CERT_RDNTemplate[] = {
    { SEC_ASN1_SET_OF,
	  offsetof(CERTRDN,avas), cert_AVATemplate, sizeof(CERTRDN) }
};


static int
CountArray(void **array)
{
    int count = 0;
    if (array) {
	while (*array++) {
	    count++;
	}
    }
    return count;
}

static void **
AddToArray(PRArenaPool *arena, void **array, void *element)
{
    unsigned count;
    void **ap;

    /* Count up number of slots already in use in the array */
    count = 0;
    ap = array;
    if (ap) {
	while (*ap++) {
	    count++;
	}
    }

    if (array) {
	array = (void**) PORT_ArenaGrow(arena, array,
					(count + 1) * sizeof(void *),
					(count + 2) * sizeof(void *));
    } else {
	array = (void**) PORT_ArenaAlloc(arena, (count + 2) * sizeof(void *));
    }
    if (array) {
	array[count] = element;
	array[count+1] = 0;
    }
    return array;
}


SECOidTag
CERT_GetAVATag(CERTAVA *ava)
{
    SECOidData *oid;
    if (!ava->type.data) return (SECOidTag)-1;

    oid = SECOID_FindOID(&ava->type);
    
    if ( oid ) {
	return(oid->offset);
    }
    return (SECOidTag)-1;
}

static SECStatus
SetupAVAType(PRArenaPool *arena, SECOidTag type, SECItem *it, unsigned *maxLenp)
{
    unsigned char *oid;
    unsigned oidLen;
    unsigned char *cp;
    int      maxLen;
    SECOidData *oidrec;

    oidrec = SECOID_FindOIDByTag(type);
    if (oidrec == NULL)
	return SECFailure;

    oid = oidrec->oid.data;
    oidLen = oidrec->oid.len;

    maxLen = cert_AVAOidTagToMaxLen(type);
    if (maxLen < 0) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }

    it->data = cp = (unsigned char*) PORT_ArenaAlloc(arena, oidLen);
    if (cp == NULL) {
	return SECFailure;
    }
    it->len = oidLen;
    PORT_Memcpy(cp, oid, oidLen);
    *maxLenp = (unsigned)maxLen;
    return SECSuccess;
}

static SECStatus
SetupAVAValue(PRArenaPool *arena, int valueType, char *value, SECItem *it,
	      unsigned maxLen)
{
    unsigned valueLen, valueLenLen, total;
    unsigned ucs4Len = 0, ucs4MaxLen;
    unsigned char *cp, *ucs4Val;

    switch (valueType) {
      case SEC_ASN1_PRINTABLE_STRING:
      case SEC_ASN1_IA5_STRING:
      case SEC_ASN1_T61_STRING:
      case SEC_ASN1_UTF8_STRING: /* no conversion required */
	valueLen = PORT_Strlen(value);
	break;
      case SEC_ASN1_UNIVERSAL_STRING:
	valueLen = PORT_Strlen(value);
	ucs4MaxLen = valueLen * 6;
	ucs4Val = (unsigned char *)PORT_ArenaZAlloc(arena, ucs4MaxLen);
	if(!ucs4Val || !PORT_UCS4_UTF8Conversion(PR_TRUE, 
	                                (unsigned char *)value, valueLen,
					ucs4Val, ucs4MaxLen, &ucs4Len)) {
	    PORT_SetError(SEC_ERROR_INVALID_ARGS);
	    return SECFailure;
	}
	value = (char *)ucs4Val;
	valueLen = ucs4Len;
    	maxLen *= 4;
	break;
      default:
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }

    if (valueLen > maxLen) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    } 

    valueLenLen = DER_LengthLength(valueLen);
    total = 1 + valueLenLen + valueLen;
    it->data = cp = (unsigned char*) PORT_ArenaAlloc(arena, total);
    if (!cp) {
	return SECFailure;
    }
    it->len = total;
    cp = (unsigned char*) DER_StoreHeader(cp, valueType, valueLen);
    PORT_Memcpy(cp, value, valueLen);
    return SECSuccess;
}

CERTAVA *
CERT_CreateAVA(PRArenaPool *arena, SECOidTag kind, int valueType, char *value)
{
    CERTAVA *ava;
    int rv;
    unsigned maxLen;

    ava = (CERTAVA*) PORT_ArenaZAlloc(arena, sizeof(CERTAVA));
    if (ava) {
	rv = SetupAVAType(arena, kind, &ava->type, &maxLen);
	if (rv) {
	    /* Illegal AVA type */
	    return 0;
	}
	rv = SetupAVAValue(arena, valueType, value, &ava->value, maxLen);
	if (rv) {
	    /* Illegal value type */
	    return 0;
	}
    }
    return ava;
}

CERTAVA *
CERT_CopyAVA(PRArenaPool *arena, CERTAVA *from)
{
    CERTAVA *ava;
    int rv;

    ava = (CERTAVA*) PORT_ArenaZAlloc(arena, sizeof(CERTAVA));
    if (ava) {
	rv = SECITEM_CopyItem(arena, &ava->type, &from->type);
	if (rv) goto loser;
	rv = SECITEM_CopyItem(arena, &ava->value, &from->value);
	if (rv) goto loser;
    }
    return ava;

  loser:
    return 0;
}

/************************************************************************/
/* XXX This template needs to go away in favor of the new SEC_ASN1 version. */
static const SEC_ASN1Template cert_RDNTemplate[] = {
    { SEC_ASN1_SET_OF,
	  offsetof(CERTRDN,avas), cert_AVATemplate, sizeof(CERTRDN) }
};


CERTRDN *
CERT_CreateRDN(PRArenaPool *arena, CERTAVA *ava0, ...)
{
    CERTAVA *ava;
    CERTRDN *rdn;
    va_list ap;
    unsigned count;
    CERTAVA **avap;

    rdn = (CERTRDN*) PORT_ArenaAlloc(arena, sizeof(CERTRDN));
    if (rdn) {
	/* Count number of avas going into the rdn */
	count = 0;
	if (ava0) {
	    count++;
	    va_start(ap, ava0);
	    while ((ava = va_arg(ap, CERTAVA*)) != 0) {
		count++;
	    }
	    va_end(ap);
	}

	/* Now fill in the pointers */
	rdn->avas = avap =
	    (CERTAVA**) PORT_ArenaAlloc( arena, (count + 1)*sizeof(CERTAVA*));
	if (!avap) {
	    return 0;
	}
	if (ava0) {
	    *avap++ = ava0;
	    va_start(ap, ava0);
	    while ((ava = va_arg(ap, CERTAVA*)) != 0) {
		*avap++ = ava;
	    }
	    va_end(ap);
	}
	*avap++ = 0;
    }
    return rdn;
}

SECStatus
CERT_AddAVA(PRArenaPool *arena, CERTRDN *rdn, CERTAVA *ava)
{
    rdn->avas = (CERTAVA**) AddToArray(arena, (void**) rdn->avas, ava);
    return rdn->avas ? SECSuccess : SECFailure;
}

SECStatus
CERT_CopyRDN(PRArenaPool *arena, CERTRDN *to, CERTRDN *from)
{
    CERTAVA **avas, *fava, *tava;
    SECStatus rv = SECSuccess;

    /* Copy each ava from from */
    avas = from->avas;
    if (avas) {
	if (avas[0] == NULL) {
	    rv = CERT_AddAVA(arena, to, NULL);
	    return rv;
	}
	while ((fava = *avas++) != 0) {
	    tava = CERT_CopyAVA(arena, fava);
	    if (!tava) {
	    	rv = SECFailure;
		break;
	    }
	    rv = CERT_AddAVA(arena, to, tava);
	    if (rv != SECSuccess) 
	    	break;
	}
    }
    return rv;
}

/************************************************************************/

const SEC_ASN1Template CERT_NameTemplate[] = {
    { SEC_ASN1_SEQUENCE_OF,
	  offsetof(CERTName,rdns), CERT_RDNTemplate, sizeof(CERTName) }
};

SEC_ASN1_CHOOSER_IMPLEMENT(CERT_NameTemplate)

CERTName *
CERT_CreateName(CERTRDN *rdn0, ...)
{
    CERTRDN *rdn;
    CERTName *name;
    va_list ap;
    unsigned count;
    CERTRDN **rdnp;
    PRArenaPool *arena;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( !arena ) {
	return(0);
    }
    
    name = (CERTName*) PORT_ArenaAlloc(arena, sizeof(CERTName));
    if (name) {
	name->arena = arena;
	
	/* Count number of RDNs going into the Name */
	if (!rdn0) {
	    count = 0;
	} else {
	    count = 1;
	    va_start(ap, rdn0);
	    while ((rdn = va_arg(ap, CERTRDN*)) != 0) {
		count++;
	    }
	    va_end(ap);
	}

	/* Allocate space (including space for terminal null ptr) */
	name->rdns = rdnp =
	    (CERTRDN**) PORT_ArenaAlloc(arena, (count + 1) * sizeof(CERTRDN*));
	if (!name->rdns) {
	    goto loser;
	}

	/* Now fill in the pointers */
	if (count > 0) {
	    *rdnp++ = rdn0;
	    va_start(ap, rdn0);
	    while ((rdn = va_arg(ap, CERTRDN*)) != 0) {
		*rdnp++ = rdn;
	    }
	    va_end(ap);
	}

	/* null terminate the list */
	*rdnp++ = 0;
    }
    return name;

loser:
    PORT_FreeArena(arena, PR_FALSE);
    return(0);
}

void
CERT_DestroyName(CERTName *name)
{
    if (name)
    {
        PRArenaPool *arena = name->arena;
        name->rdns = NULL;
	name->arena = NULL;
	if (arena) PORT_FreeArena(arena, PR_FALSE);
    }
}

SECStatus
CERT_AddRDN(CERTName *name, CERTRDN *rdn)
{
    name->rdns = (CERTRDN**) AddToArray(name->arena, (void**) name->rdns, rdn);
    return name->rdns ? SECSuccess : SECFailure;
}

SECStatus
CERT_CopyName(PRArenaPool *arena, CERTName *to, CERTName *from)
{
    CERTRDN **rdns, *frdn, *trdn;
    SECStatus rv = SECSuccess;

    if (!to || !from) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }

    CERT_DestroyName(to);
    to->arena = arena;

    /* Copy each rdn from from */
    rdns = from->rdns;
    if (rdns) {
    	if (rdns[0] == NULL) {
	    rv = CERT_AddRDN(to, NULL);
	    return rv;
	}
	while ((frdn = *rdns++) != NULL) {
	    trdn = CERT_CreateRDN(arena, 0);
	    if (!trdn) {
		rv = SECFailure;
		break;
	    }
	    rv = CERT_CopyRDN(arena, trdn, frdn);
	    if (rv != SECSuccess) 
	        break;
	    rv = CERT_AddRDN(to, trdn);
	    if (rv != SECSuccess) 
	        break;
	}
    }
    return rv;
}

/************************************************************************/

static void
canonicalize(SECItem * foo)
{
    int ch, lastch, len, src, dest;

    /* strip trailing whitespace. */
    len = foo->len;
    while (len > 0 && ((ch = foo->data[len - 1]) == ' ' || 
           ch == '\t' || ch == '\r' || ch == '\n')) {
	len--;
    }

    src = 0;
    /* strip leading whitespace. */
    while (src < len && ((ch = foo->data[src]) == ' ' || 
           ch == '\t' || ch == '\r' || ch == '\n')) {
	src++;
    }
    dest = 0; lastch = ' ';
    while (src < len) {
        ch = foo->data[src++];
	if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
	    ch = ' ';
	    if (ch == lastch)
	        continue;
	} else if (ch >= 'A' && ch <= 'Z') {
	    ch |= 0x20;  /* downshift */
	}
	foo->data[dest++] = lastch = ch;
    }
    foo->len = dest;
}

/* SECItems a and b contain DER-encoded printable strings. */
SECComparison
CERT_CompareDERPrintableStrings(const SECItem *a, const SECItem *b)
{
    SECComparison rv = SECLessThan;
    SECItem * aVal = CERT_DecodeAVAValue(a);
    SECItem * bVal = CERT_DecodeAVAValue(b);

    if (aVal && aVal->len && aVal->data &&
	bVal && bVal->len && bVal->data) {
	canonicalize(aVal);
	canonicalize(bVal);
	rv = SECITEM_CompareItem(aVal, bVal);
    }
    SECITEM_FreeItem(aVal, PR_TRUE);
    SECITEM_FreeItem(bVal, PR_TRUE);
    return rv;
}

SECComparison
CERT_CompareAVA(const CERTAVA *a, const CERTAVA *b)
{
    SECComparison rv;

    rv = SECITEM_CompareItem(&a->type, &b->type);
    if (SECEqual != rv)
	return rv;  /* Attribute types don't match. */
    /* Let's be optimistic.  Maybe the values will just compare equal. */
    rv = SECITEM_CompareItem(&a->value, &b->value);
    if (SECEqual == rv)
        return rv;  /* values compared exactly. */
    if (a->value.len && a->value.data && b->value.len && b->value.data) {
	/* Here, the values did not match.  
	** If the values had different encodings, convert them to the same
	** encoding and compare that way.
	*/
	if (a->value.data[0] != b->value.data[0]) {
	    /* encodings differ.  Convert both to UTF-8 and compare. */
	    SECItem * aVal = CERT_DecodeAVAValue(&a->value);
	    SECItem * bVal = CERT_DecodeAVAValue(&b->value);
	    if (aVal && aVal->len && aVal->data &&
	        bVal && bVal->len && bVal->data) {
		rv = SECITEM_CompareItem(aVal, bVal);
	    }
	    SECITEM_FreeItem(aVal, PR_TRUE);
	    SECITEM_FreeItem(bVal, PR_TRUE);
	} else if (a->value.data[0] == 0x13) { /* both are printable strings. */
	    /* printable strings */
	    rv = CERT_CompareDERPrintableStrings(&a->value, &b->value);
	}
    }
    return rv;
}

SECComparison
CERT_CompareRDN(CERTRDN *a, CERTRDN *b)
{
    CERTAVA **aavas, *aava;
    CERTAVA **bavas, *bava;
    int ac, bc;
    SECComparison rv = SECEqual;

    aavas = a->avas;
    bavas = b->avas;

    /*
    ** Make sure array of ava's are the same length. If not, then we are
    ** not equal
    */
    ac = CountArray((void**) aavas);
    bc = CountArray((void**) bavas);
    if (ac < bc) return SECLessThan;
    if (ac > bc) return SECGreaterThan;

    for (;;) {
	aava = *aavas++;
	bava = *bavas++;
	if (!aava) {
	    break;
	}
	rv = CERT_CompareAVA(aava, bava);
	if (rv) return rv;
    }
    return rv;
}

SECComparison
CERT_CompareName(CERTName *a, CERTName *b)
{
    CERTRDN **ardns, *ardn;
    CERTRDN **brdns, *brdn;
    int ac, bc;
    SECComparison rv = SECEqual;

    ardns = a->rdns;
    brdns = b->rdns;

    /*
    ** Make sure array of rdn's are the same length. If not, then we are
    ** not equal
    */
    ac = CountArray((void**) ardns);
    bc = CountArray((void**) brdns);
    if (ac < bc) return SECLessThan;
    if (ac > bc) return SECGreaterThan;

    for (;;) {
	ardn = *ardns++;
	brdn = *brdns++;
	if (!ardn) {
	    break;
	}
	rv = CERT_CompareRDN(ardn, brdn);
	if (rv) return rv;
    }
    return rv;
}

/* Moved from certhtml.c */
SECItem *
CERT_DecodeAVAValue(const SECItem *derAVAValue)
{
          SECItem          *retItem; 
    const SEC_ASN1Template *theTemplate       = NULL;
          enum { conv_none, conv_ucs4, conv_ucs2, conv_iso88591 } convert = conv_none;
          SECItem           avaValue          = {siBuffer, 0}; 
          PLArenaPool      *newarena          = NULL;

    if (!derAVAValue || !derAVAValue->len || !derAVAValue->data) {
	return NULL;
    }

    switch(derAVAValue->data[0]) {
	case SEC_ASN1_UNIVERSAL_STRING:
	    convert = conv_ucs4;
	    theTemplate = SEC_UniversalStringTemplate;
	    break;
	case SEC_ASN1_IA5_STRING:
	    theTemplate = SEC_IA5StringTemplate;
	    break;
	case SEC_ASN1_PRINTABLE_STRING:
	    theTemplate = SEC_PrintableStringTemplate;
	    break;
	case SEC_ASN1_T61_STRING:
	    /*
	     * Per common practice, we're not decoding actual T.61, but instead
	     * treating T61-labeled strings as containing ISO-8859-1.
	     */
	    convert = conv_iso88591;
	    theTemplate = SEC_T61StringTemplate;
	    break;
	case SEC_ASN1_BMP_STRING:
	    convert = conv_ucs2;
	    theTemplate = SEC_BMPStringTemplate;
	    break;
	case SEC_ASN1_UTF8_STRING:
	    /* No conversion needed ! */
	    theTemplate = SEC_UTF8StringTemplate;
	    break;
	default:
	    return NULL;
    }

    PORT_Memset(&avaValue, 0, sizeof(SECItem));
    newarena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (!newarena) {
        return NULL;
    }
    if(SEC_QuickDERDecodeItem(newarena, &avaValue, theTemplate, derAVAValue) 
				!= SECSuccess) {
	PORT_FreeArena(newarena, PR_FALSE);
	return NULL;
    }

    if (convert != conv_none) {
	unsigned int   utf8ValLen = avaValue.len * 3;
	unsigned char *utf8Val    = (unsigned char*)
				    PORT_ArenaZAlloc(newarena, utf8ValLen);

        switch (convert) {
        case conv_ucs4:
           if(avaValue.len % 4 != 0 ||
              !PORT_UCS4_UTF8Conversion(PR_FALSE, avaValue.data, avaValue.len,
					utf8Val, utf8ValLen, &utf8ValLen)) {
                PORT_FreeArena(newarena, PR_FALSE);
                PORT_SetError(SEC_ERROR_INVALID_AVA);
		return NULL;
	   }
	   break;
	case conv_ucs2:
           if(avaValue.len % 2 != 0 ||
              !PORT_UCS2_UTF8Conversion(PR_FALSE, avaValue.data, avaValue.len,
					utf8Val, utf8ValLen, &utf8ValLen)) {
                PORT_FreeArena(newarena, PR_FALSE);
                PORT_SetError(SEC_ERROR_INVALID_AVA);
		return NULL;
	   }
	   break;
	case conv_iso88591:
           if(!PORT_ISO88591_UTF8Conversion(avaValue.data, avaValue.len,
					utf8Val, utf8ValLen, &utf8ValLen)) {
                PORT_FreeArena(newarena, PR_FALSE);
                PORT_SetError(SEC_ERROR_INVALID_AVA);
		return NULL;
	   }
	   break;
	case conv_none: ;
	}
	  
	avaValue.data = utf8Val;
	avaValue.len = utf8ValLen;
    }

    retItem = SECITEM_DupItem(&avaValue);
    PORT_FreeArena(newarena, PR_FALSE);
    return retItem;
}
