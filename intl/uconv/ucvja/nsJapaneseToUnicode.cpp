/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation.  All Rights Reserved.
 */
#include "nsJapaneseToUnicode.h"

#include "nsUCvJaSupport.h"

static PRUint16 gJis0208map[] = {
#include "jis0208.map" 
};
static PRUint16 gJis0212map[] = {
#include "jis0212.map" 
};

class nsShiftJISToUnicode : public nsBasicDecoderSupport
{
public:

 nsShiftJISToUnicode() { mState=0; mData=0; };
 virtual ~nsShiftJISToUnicode() {};

 NS_IMETHOD Convert(const char * aSrc, PRInt32 * aSrcLength,
     PRUnichar * aDest, PRInt32 * aDestLength) ;
 NS_IMETHOD GetMaxLength(const char * aSrc, PRInt32 aSrcLength,
     PRInt32 * aDestLength) ;
 NS_IMETHOD Reset();

private:
 PRInt32  mState;
 PRInt32 mData;
};


NS_IMETHODIMP nsShiftJISToUnicode::Convert(
   const char * aSrc, PRInt32 * aSrcLen,
     PRUnichar * aDest, PRInt32 * aDestLen)
{
   static PRUint16 fbIdx[128] =
   {
     0xFFFD, 0,      188,    188*2,  188*3,  188*4,  188*5,  188*6,
     188*7,  188*8,  188*9,  188*10, 188*11, 188*12, 188*13, 188*14,
     188*15, 188*16, 188*17, 188*18, 188*19, 188*20, 188*21, 188*22,
     188*23, 188*24, 188*25, 188*26, 188*27, 188*28, 188*29, 188*30,
     0xFFFD, 0xFF61, 0xFF62, 0xFF63, 0xFF64, 0xFF65, 0xFF66, 0xFF67,
     0xFF68, 0xFF69, 0xFF6A, 0xFF6B, 0xFF6C, 0xFF6D, 0xFF6E, 0xFF6F,
     0xFF70, 0xFF71, 0xFF72, 0xFF73, 0xFF74, 0xFF75, 0xFF76, 0xFF77,
     0xFF78, 0xFF79, 0xFF7A, 0xFF7B, 0xFF7C, 0xFF7D, 0xFF7E, 0xFF7F,
     0xFF80, 0xFF81, 0xFF82, 0xFF83, 0xFF84, 0xFF85, 0xFF86, 0xFF87,
     0xFF88, 0xFF89, 0xFF8A, 0xFF8B, 0xFF8C, 0xFF8D, 0xFF8E, 0xFF8F,
     0xFF90, 0xFF91, 0xFF92, 0xFF93, 0xFF94, 0xFF85, 0xFF96, 0xFF97,
     0xFF98, 0xFF99, 0xFF9A, 0xFF9B, 0xFF9C, 0xFF9D, 0xFF9E, 0xFF9F,
     188*31, 188*32, 188*33, 188*34, 188*35, 188*36, 188*37, 188*38,
     188*39, 188*40, 188*41, 188*42, 188*43, 188*44, 188*45, 188*46,
     0xE000, 0xE000+188, 0xE000+188*2, 0xE000+188*3, 0xE000+188*4, 0xE000+188*5, 0xE000+188*6, 0xE000+188*7,
     0xE000+188*8, 0xE000+188*9, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD,
   };
   static PRUint8 sbIdx[256] =
   {
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  /* 0x00 */
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  /* 0x08 */
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  /* 0x10 */
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  /* 0x18 */
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  /* 0x20 */
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  /* 0x28 */
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  /* 0x30 */
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  /* 0x38 */
        0,    1,    2,    3,    4,    5,    6,    7,  /* 0x40 */
        8,    9,   10,   11,   12,   13,   14,   15,  /* 0x48 */
       16,   17,   18,   19,   20,   21,   22,   23,  /* 0x50 */
       24,   25,   26,   27,   28,   29,   30,   31,  /* 0x58 */
       32,   33,   34,   35,   36,   37,   38,   39,  /* 0x60 */
       40,   41,   42,   43,   44,   45,   46,   47,  /* 0x68 */
       48,   49,   50,   51,   52,   53,   54,   55,  /* 0x70 */
       56,   57,   58,   59,   60,   61,   62, 0xFF,  /* 0x78 */
       63,   64,   65,   66,   67,   68,   69,   70,  /* 0x80 */
       71,   72,   73,   74,   75,   76,   77,   78,  /* 0x88 */
       79,   80,   81,   82,   83,   84,   85,   86,  /* 0x90 */
       87,   88,   89,   90,   91,   92,   93,   94,  /* 0x98 */
       95,   96,   97,   98,   99,  100,  101,  102,  /* 0xa0 */
      103,  104,  105,  106,  107,  108,  109,  110,  /* 0xa8 */
      111,  112,  113,  114,  115,  116,  117,  118,  /* 0xb0 */
      119,  120,  121,  122,  123,  124,  125,  126,  /* 0xb8 */
      137,  138,  139,  130,  131,  132,  133,  134,  /* 0xc0 */
      135,  136,  137,  138,  139,  140,  141,  142,  /* 0xc8 */
      143,  144,  145,  146,  147,  148,  149,  150,  /* 0xd0 */
      151,  152,  153,  154,  155,  156,  157,  158,  /* 0xd8 */
      159,  160,  161,  162,  163,  164,  165,  166,  /* 0xe0 */
      167,  168,  169,  170,  171,  172,  173,  174,  /* 0xe8 */
      175,  176,  177,  178,  179,  180,  181,  182,  /* 0xf0 */
      183,  184,  185,  186,  187, 0xFF, 0xFF, 0xFF,  /* 0xf8 */
   };

   const unsigned char* srcEnd = (unsigned char*)aSrc + *aSrcLen;
   const unsigned char* src =(unsigned char*) aSrc;
   PRUnichar* destEnd = aDest + *aDestLen;
   PRUnichar* dest = aDest;
   while((src < srcEnd))
   {
       switch(mState)
       {
          case 0:
          if(*src & 0x80)
          {
            mData = fbIdx[*src & 0x7F];
            if(mData < 0xE000 )
            {
               mState = 1; // two bytes 
            } else {
               if( mData > 0xFF00)
               {
                 if(0xFFFD == mData) {
                   if((0x80 != *src) && (0xa0 != *src) & (0xfd != *src) &&
                      (0xfd != *src) && (0xfe != *src) & (0xff != *src))
                      mState = 3;  // two byte undefined
                 } else {
                   *dest++ = mData; // JIS 0201
                   if(dest >= destEnd)
                      goto error1;
                 }
               } else {
                 mState = 2; // EUDC
               }
            }
          } else {
            // ASCII
            *dest++ = (PRUnichar) *src;
            if(dest >= destEnd)
              goto error1;
          }
          break;

          case 1: // Index to table
          {
            PRUint8 off = sbIdx[*src];
            if(0xFF == off) {
               *dest++ = 0xFFFD;
            } else {
               *dest++ = gJis0208map[mData+off];
            }
            if(dest >= destEnd)
              goto error1;
            mState = 0;
          }
          break;

          case 2: // EUDC
          {
            PRUint8 off = sbIdx[*src];
            if(0xFF == off) {
               *dest++ = 0xFFFD;
            } else {
               *dest++ = mData + off;
            }
            if(dest >= destEnd)
              goto error1;
            mState = 0;
          }
          break;

          case 3: // two bytes undefined
          {
            *dest++ = 0xFFFD;
            if(dest >= destEnd)
              goto error1;
            mState = 0;
          }
          break;
       }
       src++;
   }
   *aDestLen = dest - aDest;
   return NS_OK;
error1:
   *aDestLen = dest-aDest;
   *aSrcLen = src-(unsigned char*)aSrc;
   return NS_OK_UDEC_MOREOUTPUT;
}
NS_IMETHODIMP nsShiftJISToUnicode::GetMaxLength(
     const char * aSrc, PRInt32 aSrcLength,
     PRInt32 * aDestLength)
{
   *aDestLength = aSrcLength;
   return NS_OK;
}
NS_IMETHODIMP nsShiftJISToUnicode::Reset()
{
   mState = 0;
   return NS_OK;
}


class nsEUCJPToUnicodeV2 : public nsBasicDecoderSupport
{
public:

 nsEUCJPToUnicodeV2() { mState=0; mData=0; };
 virtual ~nsEUCJPToUnicodeV2() {};

 NS_IMETHOD Convert(const char * aSrc, PRInt32 * aSrcLength,
     PRUnichar * aDest, PRInt32 * aDestLength) ;
 NS_IMETHOD GetMaxLength(const char * aSrc, PRInt32 aSrcLength,
     PRInt32 * aDestLength) ;
 NS_IMETHOD Reset();

private:
 PRInt32  mState;
 PRInt32 mData;
};


NS_IMETHODIMP nsEUCJPToUnicodeV2::Convert(
   const char * aSrc, PRInt32 * aSrcLen,
     PRUnichar * aDest, PRInt32 * aDestLen)
{
   static PRUint16 fbIdx[128] =
   {
/* 0x8X */
     0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD,
     0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD,
/* 0x9X */
     0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD,
     0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD,
/* 0xAX */
     0xFFFD, 0,      94,     94* 2,  94* 3,  94* 4,  94* 5,  94* 6,  
     94* 7,  94* 8 , 94* 9,  94*10,  94*11,  94*12,  94*13,  94*14,
/* 0xBX */
     94*15,  94*16,  94*17,  94*18,  94*19,  94*20,  94*21,  94*22,
     94*23,  94*24,  94*25,  94*26,  94*27,  94*28,  94*29,  94*30,
/* 0xCX */
     94*31,  94*32,  94*33,  94*34,  94*35,  94*36,  94*37,  94*38,
     94*39,  94*40,  94*41,  94*42,  94*43,  94*44,  94*45,  94*46,
/* 0xDX */
     94*47,  94*48,  94*49,  94*50,  94*51,  94*52,  94*53,  94*54,
     94*55,  94*56,  94*57,  94*58,  94*59,  94*60,  94*61,  94*62,
/* 0xEX */
     94*63,  94*64,  94*65,  94*66,  94*67,  94*68,  94*69,  94*70,
     94*71,  94*72,  94*73,  94*74,  94*75,  94*76,  94*77,  94*78,
/* 0xFX */
     94*79,  94*80,  94*81,  94*82,  94*83,  94*84,  94*85,  94*86,
     94*87,  94*88,  94*89,  94*90,  94*91,  94*92,  94*93,  0xFFFD,
   };
   static PRUint8 sbIdx[256] =
   {
/* 0x0X */
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
/* 0x1X */
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
/* 0x2X */
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
/* 0x3X */
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
/* 0x4X */
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
/* 0x5X */
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
/* 0x6X */
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
/* 0x7X */
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
/* 0x8X */
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
/* 0x9X */
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
/* 0xAX */
     0xFF, 0,    1,    2,    3,    4,    5,    6,  
     7,    8 ,   9,    10,   11,   12,   13,   14,
/* 0xBX */
     15,   16,   17,   18,   19,   20,   21,   22, 
     23,   24,   25,   26,   27,   28,   29,   30, 
/* 0xCX */
     31,   32,   33,   34,   35,   36,   37,   38, 
     39,   40,   41,   42,   43,   44,   45,   46, 
/* 0xDX */
     47,   48,   49,   50,   51,   52,   53,   54, 
     55,   56,   57,   58,   59,   60,   61,   62, 
/* 0xEX */
     63,   64,   65,   66,   67,   68,   69,   70, 
     71,   72,   73,   74,   75,   76,   77,   78, 
/* 0xFX */
     79,   80,   81,   82,   83,   84,   85,   86, 
     87,   88,   89,   90,   91,   92,   93,   0xFF, 
   };

   const unsigned char* srcEnd = (unsigned char*)aSrc + *aSrcLen;
   const unsigned char* src =(unsigned char*) aSrc;
   PRUnichar* destEnd = aDest + *aDestLen;
   PRUnichar* dest = aDest;
   while((src < srcEnd))
   {
       switch(mState)
       {
          case 0:
          if(*src & 0x80)
          {
            mData = fbIdx[*src & 0x7F];
            if(mData != 0xFFFD )
            {
               mState = 1; // two byte JIS0208
            } else {
               if( 0x8e == *src) {
                 // JIS 0201
                 mState = 2; // JIS0201
               } else if(0x8f == *src) {
                 // JIS 0212
                 mState = 3; // JIS0212
               } else {
                 // others 
                 *dest++ = 0xFFFD;
                 if(dest >= destEnd)
                   goto error1;
               }
            }
          } else {
            // ASCII
            *dest++ = (PRUnichar) *src;
            if(dest >= destEnd)
              goto error1;
          }
          break;

          case 1: // Index to table
          {
            PRUint8 off = sbIdx[*src];
            if(0xFF == off) {
               *dest++ = 0xFFFD;
            } else {
               *dest++ = gJis0208map[mData+off];
            }
            if(dest >= destEnd)
              goto error1;
            mState = 0;
          }
          break;

          case 2: // JIS 0201
          {
            if((0xA1 <= *src) && (*src <= 0xDF)) {
              *dest++ = (0xFF61-0x00A1) + *src;
            } else {
              *dest++ = 0xFFFD;
            }
            if(dest >= destEnd)
              goto error1;
            mState = 0;
          }
          break;

          case 3: // JIS 0212
          {
            if(*src & 0x80)
            {
              mData = fbIdx[*src & 0x7F];
              if(mData != 0xFFFD )
              {
                 mState = 4; 
              } else {
                 mState = 5; // error
              }
            } else {
              mState = 5; // error
            }
          }
          break;
          case 4: // two bytes undefined
          {
            PRUint8 off = sbIdx[*src];
            if(0xFF == off) {
               *dest++ = 0xFFFD;
            } else {
               *dest++ = gJis0212map[mData+off];
            }
            if(dest >= destEnd)
              goto error1;
            mState = 0;
          }
          break;
          case 5: // two bytes undefined
          {
            *dest++ = 0xFFFD;
            if(dest >= destEnd)
              goto error1;
            mState = 0;
          }
          break;
       }
       src++;
   }
   *aDestLen = dest - aDest;
   return NS_OK;
error1:
   *aDestLen = dest-aDest;
   *aSrcLen = src-(unsigned char*)aSrc;
   return NS_OK_UDEC_MOREOUTPUT;
}
NS_IMETHODIMP nsEUCJPToUnicodeV2::GetMaxLength(
     const char * aSrc, PRInt32 aSrcLength,
     PRInt32 * aDestLength)
{
   *aDestLength = aSrcLength;
   return NS_OK;
}
NS_IMETHODIMP nsEUCJPToUnicodeV2::Reset()
{
   mState = 0;
   return NS_OK;
}


nsresult NEW_ShiftJISToUnicode(nsISupports **aResult)
{
   *aResult = new nsShiftJISToUnicode();
   return (NULL == *aResult) ? NS_ERROR_OUT_OF_MEMORY : NS_OK;
}
nsresult NEW_EUCJPToUnicode(nsISupports **aResult)
{
   *aResult = new nsEUCJPToUnicodeV2();
   return (NULL == *aResult) ? NS_ERROR_OUT_OF_MEMORY : NS_OK;
}
