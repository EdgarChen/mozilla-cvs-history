/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#ifndef nsInt64_h__
#define nsInt64_h__

#include "prlong.h"
#include "nscore.h"

/**
 * This class encapsulates full 64-bit integer functionality and
 * provides simple arithmetic and conversion operations.
 */

// If you ever decide that you need to add a non-inline method to this
// class, be sure to change the class declaration to "class NS_BASE
// nsInt64".

class nsInt64
{
private:
    PRInt64 mValue;

public:
    /**
     * Construct a new 64-bit integer.
     */
    nsInt64(void) : mValue(LL_ZERO) {
    }

    /**
     * Construct a new 64-bit integer from a 32-bit signed integer
     */
    nsInt64(const PRInt32 aInt32) {
        LL_I2L(mValue, aInt32);
    }

    /**
     * Construct a new 64-bit integer from a 32-bit unsigned integer
     */
    nsInt64(const PRUint32 aUint32) {
        LL_UI2L(mValue, aUint32);
    }

    /**
     * Construct a new 64-bit integer from a floating point value.
     */
    nsInt64(const PRFloat64 aFloat64) {
        LL_D2L(mValue, aFloat64);
    }

    /**
     * Construct a new 64-bit integer from a native 64-bit integer
     */
    nsInt64(const PRInt64 aInt64) : mValue(aInt64) {
    }

    /**
     * Construct a new 64-bit integer from another 64-bit integer
     */
    nsInt64(const nsInt64& aObject) : mValue(aObject.mValue) {
    }

    // ~nsInt64(void) -- XXX destructor unnecessary

    /**
     * Assign a 64-bit integer to another 64-bit integer
     */
    const nsInt64& operator =(const nsInt64& aObject) {
        mValue = aObject.mValue;
        return *this;
    }

    /**
     * Convert a 64-bit integer to a signed 32-bit value
     */
    operator PRInt32(void) const {
        PRInt32 result;
        LL_L2I(result, mValue);
        return result;
    }

    /**
     * Convert a 64-bit integer to an unsigned 32-bit value
     */
    operator PRUint32(void) const {
        PRUint32 result;
        LL_L2UI(result, mValue);
        return result;
    }

    /**
     * Convert a 64-bit integer to a floating point value
     */
    operator PRFloat64(void) const {
        PRFloat64 result;
        LL_L2D(result, mValue);
        return result;
    }

    /**
     * Convert a 64-bit integer to a native 64-bit integer.
     */
    operator PRInt64(void) const {
        return mValue;
    }

    /**
     * Perform unary negation on a 64-bit integer.
     */
    const nsInt64 operator -(void) {
        nsInt64 result;
        LL_NEG(result.mValue, mValue);
        return result;
    }

    // Arithmetic operators
    friend const nsInt64 operator +(const nsInt64& aObject1, const nsInt64& aObject2);
    friend const nsInt64 operator -(const nsInt64& aObject1, const nsInt64& aObject2);
    friend const nsInt64 operator *(const nsInt64& aObject1, const nsInt64& aObject2);
    friend const nsInt64 operator /(const nsInt64& aObject1, const nsInt64& aObject2);
    friend const nsInt64 operator %(const nsInt64& aObject1, const nsInt64& aObject2);

    /**
     * Increment a 64-bit integer by a 64-bit integer amount.
     */
    nsInt64& operator +=(const nsInt64& aObject) {
        LL_ADD(mValue, mValue, aObject.mValue);
        return *this;
    }

    /**
     * Decrement a 64-bit integer by a 64-bit integer amount.
     */
    nsInt64& operator -=(const nsInt64& aObject) {
        LL_SUB(mValue, mValue, aObject.mValue);
        return *this;
    }

    /**
     * Multiply a 64-bit integer by a 64-bit integer amount.
     */
    nsInt64& operator *=(const nsInt64& aObject) {
        LL_MUL(mValue, mValue, aObject.mValue);
        return *this;
    }

    /**
     * Divide a 64-bit integer by a 64-bit integer amount.
     */
    nsInt64& operator /=(const nsInt64& aObject) {
        LL_DIV(mValue, mValue, aObject.mValue);
        return *this;
    }

    /**
     * Compute the modulus of a 64-bit integer to a 64-bit value.
     */
    nsInt64& operator %=(const nsInt64& aObject) {
        LL_MOD(mValue, mValue, aObject.mValue);
        return *this;
    }

    // Comparison operators
    friend PRBool operator ==(const nsInt64& aObject1, const nsInt64& aObject2);
    friend PRBool operator !=(const nsInt64& aObject1, const nsInt64& aObject2);
    friend PRBool operator >(const nsInt64& aObject1, const nsInt64& aObject2);
    friend PRBool operator >=(const nsInt64& aObject1, const nsInt64& aObject2);
    friend PRBool operator <(const nsInt64& aObject1, const nsInt64& aObject2);
    friend PRBool operator <=(const nsInt64& aObject1, const nsInt64& aObject2);

    // Bitwise operators
    friend const nsInt64 operator &(const nsInt64& aObject1, const nsInt64& aObject2);
    friend const nsInt64 operator |(const nsInt64& aObject1, const nsInt64& aObject2);
    friend const nsInt64 operator ^(const nsInt64& aObject1, const nsInt64& aObject2);

    /**
     * Compute the bitwise NOT of a 64-bit integer
     */
    const nsInt64 operator ~(void) {
        nsInt64 result;
        LL_NOT(result.mValue, mValue);
        return result;
    }

    /**
     * Compute the bitwise AND with another 64-bit integer
     */
    nsInt64& operator &=(const nsInt64& aObject) {
        LL_AND(mValue, mValue, aObject.mValue);
        return *this;
    }

    /**
     * Compute the bitwise OR with another 64-bit integer
     */
    nsInt64& operator |=(const nsInt64& aObject) {
        LL_OR(mValue, mValue, aObject.mValue);
        return *this;
    }

    /**
     * Compute the bitwise XOR with another 64-bit integer
     */
    nsInt64& operator ^=(const nsInt64& aObject) {
        LL_XOR(mValue, mValue, aObject.mValue);
        return *this;
    }
};


/**
 * Add two 64-bit integers.
 */
inline const nsInt64
operator +(const nsInt64& aObject1, const nsInt64& aObject2) {
    return nsInt64(aObject1) += aObject2;
}

/**
 * Subtract one 64-bit integer from another.
 */
inline const nsInt64
operator -(const nsInt64& aObject1, const nsInt64& aObject2) {
    return nsInt64(aObject1) -= aObject2;
}

/**
 * Multiply two 64-bit integers
 */
inline const nsInt64
operator *(const nsInt64& aObject1, const nsInt64& aObject2) {
    return nsInt64(aObject1) *= aObject2;
}

/**
 * Divide one 64-bit integer by another
 */
inline const nsInt64
operator /(const nsInt64& aObject1, const nsInt64& aObject2) {
    return nsInt64(aObject1) /= aObject2;
}

/**
 * Compute the modulus of two 64-bit integers
 */
inline const nsInt64
operator %(const nsInt64& aObject1, const nsInt64& aObject2) {
    return nsInt64(aObject1) %= aObject2;
}

/**
 * Determine if two 64-bit integers are equal
 */
inline PRBool
operator ==(const nsInt64& aObject1, const nsInt64& aObject2) {
    return LL_EQ(aObject1.mValue, aObject2.mValue);
}

/**
 * Determine if two 64-bit integers are not equal
 */
inline PRBool
operator !=(const nsInt64& aObject1, const nsInt64& aObject2) {
    return LL_NE(aObject1.mValue, aObject2.mValue);
}

/**
 * Determine if one 64-bit integer is strictly greater than another, using signed values
 */
inline PRBool
operator >(const nsInt64& aObject1, const nsInt64& aObject2) {
    return LL_CMP(aObject1.mValue, >, aObject2.mValue);
}

/**
 * Determine if one 64-bit integer is greater than or equal to another, using signed values
 */
inline PRBool
operator >=(const nsInt64& aObject1, const nsInt64& aObject2) {
    return LL_CMP(aObject1.mValue, >=, aObject2.mValue);
}

/**
 * Determine if one 64-bit integer is strictly less than another, using signed values
 */
inline PRBool 
operator <(const nsInt64& aObject1, const nsInt64& aObject2) {
    return LL_CMP(aObject1.mValue, <, aObject2.mValue);
}

/**
 * Determine if one 64-bit integers is less than or equal to another, using signed values
 */
inline PRBool
operator <=(const nsInt64& aObject1, const nsInt64& aObject2) {
    return LL_CMP(aObject1.mValue, <=, aObject2.mValue);
}

/**
 * Perform a bitwise AND of two 64-bit integers
 */
inline const nsInt64
operator &(const nsInt64& aObject1, const nsInt64& aObject2) {
    return nsInt64(aObject1) &= aObject2;
}

/**
 * Perform a bitwise OR of two 64-bit integers
 */
inline const nsInt64
operator |(const nsInt64& aObject1, const nsInt64& aObject2) {
    return nsInt64(aObject1) |= aObject2;
}

/**
 * Perform a bitwise XOR of two 64-bit integers
 */
inline const nsInt64
operator ^(const nsInt64& aObject1, const nsInt64& aObject2) {
    return nsInt64(aObject1) ^= aObject2;
}


#endif // nsInt64_h__
