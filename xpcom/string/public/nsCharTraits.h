/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2000
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Scott Collins <scc@mozilla.org> (original author)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef nsCharTraits_h___
#define nsCharTraits_h___

#ifndef nsStringDefines_h___
#include "nsStringDefines.h"
#endif

#include <ctype.h>
  // for |EOF|, |WEOF|

#include <string.h>
  // for |memcpy|, et al

#ifndef nscore_h___
#include "nscore.h"
  // for |PRUnichar|
#endif

#ifndef nsStringIteratorUtils_h___
#include "nsStringIteratorUtils.h"
#endif

#ifdef HAVE_CPP_BOOL
  typedef bool nsCharTraits_bool;
#else
  typedef PRBool nsCharTraits_bool;
#endif

template <class CharT>
struct nsCharTraits
  {
    typedef CharT     char_type;
    typedef CharT     unsigned_char_type;
    typedef char      incompatible_char_type;

    static
    void
    assign( char_type& lhs, const char_type& rhs )
      {
        lhs = rhs;
      }


      // integer representation of characters:

    typedef int int_type;

    static
    char_type
    to_char_type( const int_type& c )
      {
        return char_type(c);
      }

    static
    int_type
    to_int_type( const char_type& c )
      {
        return int_type(c);
      }

    static
    nsCharTraits_bool
    eq_int_type( const int_type& lhs, const int_type& rhs )
      {
        return lhs == rhs;
      }


      // |char_type| comparisons:

    static
    nsCharTraits_bool
    eq( const char_type& lhs, const char_type& rhs )
      {
        return lhs == rhs;
      }

    static
    nsCharTraits_bool
    lt( const char_type& lhs, const char_type& rhs )
      {
        return lhs < rhs;
      }


      // operations on s[n] arrays:

    static
    char_type*
    copy( char_type* s1, const char_type* s2, size_t n )
      {
        char_type* result = s1;
        while ( n-- )
          assign(*s1++, *s2++);
        return result;
      }

    static
    char_type*
    move( char_type* s1, const char_type* s2, size_t n )
      {
        char_type* result = s1;

        if ( n )
          {
            if ( s2 > s1 )
              copy(s1, s2, n);
            else
              {
                s1 += n;
                s2 += n;
                while ( n-- )
                  assign(*--s1, *--s2);
              }
          }
        
        return result;
      }

    static
    char_type*
    assign( char_type* s, size_t n, const char_type& c )
      {
        char_type* result = s;
        while ( n-- )
          assign(*s++, c);
        return result;
      }

    static
    int
    compare( const char_type* s1, const char_type* s2, size_t n )
      {
        for ( ; n--; ++s1, ++s2 )
          {
            if ( lt(*s1, *s2) )
              return -1;
            if ( lt(*s2, *s1) )
              return 1;
          }

        return 0;
      }

    static
    size_t
    length( const char_type* s )
      {
        size_t result = 0;
        while ( !eq(*s++, CharT(0)) )
          ++result;
        return result;
      }

    static
    const char_type*
    find( const char_type* s, size_t n, const char_type& c )
      {
        while ( n-- )
          {
            if ( eq(*s, c) )
              return s;
            ++s;
          }

        return 0;
      }

#if 0
      // I/O related:

    typedef streamoff off_type;
    typedef streampos pos_type;
    typedef mbstate_t state_type;

    static
    int_type
    eof()
      {
        return EOF;
      }

    static
    int_type
    not_eof( const int_type& c )
      {
        return eq_int_type(c, eof()) ? ~eof() : c;
      }

    // static state_type get_state( pos_type );
#endif
  };

NS_SPECIALIZE_TEMPLATE
struct nsCharTraits<char>
  {
    typedef char           char_type;
    typedef unsigned char  unsigned_char_type;
    typedef PRUnichar      incompatible_char_type;

    static
    void
    assign( char& lhs, char rhs )
      {
        lhs = rhs;
      }


      // integer representation of characters:

    typedef int int_type;

    static
    char
    to_char_type( int c )
      {
        return char(c);
      }

    static
    int
    to_int_type( char c )
      {
        return int( NS_STATIC_CAST(unsigned char, c) );
      }

    static
    nsCharTraits_bool
    eq_int_type( int lhs, int rhs )
      {
        return lhs == rhs;
      }


      // |char_type| comparisons:

    static
    nsCharTraits_bool
    eq( char lhs, char rhs )
      {
        return lhs == rhs;
      }

    static
    nsCharTraits_bool
    lt( char lhs, char rhs )
      {
        return lhs < rhs;
      }


      // operations on s[n] arrays:

    static
    char*
    move( char* s1, const char* s2, size_t n )
      {
        return NS_STATIC_CAST(char*, memmove(s1, s2, n));
      }

    static
    char*
    copy( char* s1, const char* s2, size_t n )
      {
        return NS_STATIC_CAST(char*, memcpy(s1, s2, n));
      }

    static
    char*
    assign( char* s, size_t n, char c )
      {
        return NS_STATIC_CAST(char*, memset(s, to_int_type(c), n));
      }

    static
    int
    compare( const char* s1, const char* s2, size_t n )
      {
        return memcmp(s1, s2, n);
      }

    static
    size_t
    length( const char* s )
      {
        return strlen(s);
      }

    static
    const char*
    find( const char* s, size_t n, char c )
      {
        return NS_REINTERPRET_CAST(const char*, memchr(s, to_int_type(c), n));
      }

#if 0
      // I/O related:

    typedef streamoff off_type;
    typedef streampos pos_type;
    typedef mbstate_t state_type;

    static
    int_type
    eof()
      {
        return EOF;
      }

    static
    int
    not_eof( int c )
      {
        return c==eof() ? ~eof() : c;
      }

    // static state_type get_state( pos_type );
#endif
  };

#if 0
NS_SPECIALIZE_TEMPLATE
struct nsCharTraits<wchar_t>
  {
    typedef wchar_t     char_type;
    /* we need to put these here for real if/when we use 
     * this specialized template.
     *
     * typedef wchar_t     unsigned_char_type;
     * typedef char        incompatible_char_type;
     */

    static
    void
    assign( wchar_t& lhs, wchar_t rhs )
      {
        lhs = rhs;
      }


      // integer representation of characters:

    typedef wint_t int_type;

    static
    wchar_t
    to_char_type( int_type c )
      {
        return wchar_t(c);
      }

    static
    int_type
    to_int_type( wchar_t c )
      {
        return int_type(c);
      }

    static
    nsCharTraits_bool
    eq_int_type( int_type lhs, int_type rhs )
      {
        return lhs == rhs;
      }


      // |char_type| comparisons:

    static
    nsCharTraits_bool
    eq( wchar_t lhs, wchar_t rhs )
      {
        return lhs == rhs;
      }

    static
    nsCharTraits_bool
    lt( wchar_t lhs, wchar_t rhs )
      {
        return lhs < rhs;
      }


      // operations on s[n] arrays:

    static
    wchar_t*
    move( wchar_t* s1, const wchar_t* s2, size_t n )
      {
        return NS_STATIC_CAST(wchar_t*, wmemmove(s1, s2, n));
      }

    static
    wchar_t*
    copy( wchar_t* s1, const wchar_t* s2, size_t n )
      {
        return NS_STATIC_CAST(wchar_t*, wmemcpy(s1, s2, n));
      }

    static
    wchar_t*
    assign( wchar_t* s, size_t n, wchar_t c )
      {
        return NS_STATIC_CAST(wchar_t*, wmemset(s, to_int_type(c), n));
      }

    static
    int
    compare( const wchar_t* s1, const wchar_t* s2, size_t n )
      {
        return wmemcmp(s1, s2, n);
      }

    static
    size_t
    length( const wchar_t* s )
      {
        return wcslen(s);
      }

    static
    const wchar_t*
    find( const wchar_t* s, size_t n, wchar_t c )
      {
        return NS_REINTERPRET_CAST(const wchar_t*, wmemchr(s, to_int_type(c), n));
      }

#if 0
      // I/O related:

    typedef streamoff off_type;
    typedef streampos pos_type;
    typedef mbstate_t state_type;

    static
    int_type
    eof()
      {
        return WEOF;
      }

    static
    int_type
    not_eof( int_type c )
      {
        return c==eof() ? ~eof() : c;
      }

    // static state_type get_state( pos_type );
#endif
  };
#endif

template <class InputIterator>
struct nsCharSourceTraits
  {
    typedef typename InputIterator::difference_type difference_type;

#if 0
    static
    PRUint32
    distance( const InputIterator& first, const InputIterator& last )
      {
        // ...
      }
#endif

    static
    PRUint32
    readable_distance( const InputIterator& iter )
      {
        return iter.size_forward();
      }

    static
    PRUint32
    readable_distance( const InputIterator& first, const InputIterator& last )
      {
        return PRUint32(SameFragment(first, last) ? last.get()-first.get() : first.size_forward());
      }

    static
    const typename InputIterator::value_type*
    read( const InputIterator& iter )
      {
        return iter.get();
      }

    static
    void
    advance( InputIterator& s, difference_type n )
      {
        s.advance(n);
      }
  };

#ifdef HAVE_CPP_PARTIAL_SPECIALIZATION

template <class CharT>
struct nsCharSourceTraits<CharT*>
  {
    typedef ptrdiff_t difference_type;

#if 0
    static
    PRUint32
    distance( CharT* first, CharT* last )
      {
        return PRUint32(last-first);
      }
#endif

    static
    PRUint32
    readable_distance( CharT* s )
      {
        return PRUint32(nsCharTraits<CharT>::length(s));
//      return numeric_limits<PRUint32>::max();
      }

    static
    PRUint32
    readable_distance( CharT* first, CharT* last )
      {
        return PRUint32(last-first);
      }

    static
    const CharT*
    read( CharT* s )
      {
        return s;
      }

    static
    void
    advance( CharT*& s, difference_type n )
      {
        s += n;
      }
  };

#else

NS_SPECIALIZE_TEMPLATE
struct nsCharSourceTraits<const char*>
  {
    typedef ptrdiff_t difference_type;

#if 0
    static
    PRUint32
    distance( const char* first, const char* last )
      {
        return PRUint32(last-first);
      }
#endif

    static
    PRUint32
    readable_distance( const char* s )
      {
        return PRUint32(nsCharTraits<char>::length(s));
//      return numeric_limits<PRUint32>::max();
      }

    static
    PRUint32
    readable_distance( const char* first, const char* last )
      {
        return PRUint32(last-first);
      }

    static
    const char*
    read( const char* s )
      {
        return s;
      }

    static
    void
    advance( const char*& s, difference_type n )
      {
        s += n;
      }
 };


NS_SPECIALIZE_TEMPLATE
struct nsCharSourceTraits<const PRUnichar*>
  {
    typedef ptrdiff_t difference_type;

#if 0
    static
    PRUint32
    distance( const PRUnichar* first, const PRUnichar* last )
      {
        return PRUint32(last-first);
      }
#endif

    static
    PRUint32
    readable_distance( const PRUnichar* s )
      {
        return PRUint32(nsCharTraits<PRUnichar>::length(s));
//      return numeric_limits<PRUint32>::max();
      }

    static
    PRUint32
    readable_distance( const PRUnichar* first, const PRUnichar* last )
      {
        return PRUint32(last-first);
      }

    static
    const PRUnichar*
    read( const PRUnichar* s )
      {
        return s;
      }

    static
    void
    advance( const PRUnichar*& s, difference_type n )
      {
        s += n;
      }
 };

#endif


template <class OutputIterator>
struct nsCharSinkTraits
  {
    static
    PRUint32
    write( OutputIterator& iter, const typename OutputIterator::value_type* s, PRUint32 n )
      {
        return iter.write(s, n);
      }
  };

#ifdef HAVE_CPP_PARTIAL_SPECIALIZATION

template <class CharT>
struct nsCharSinkTraits<CharT*>
  {
    static
    PRUint32
    write( CharT*& iter, const CharT* s, PRUint32 n )
      {
        nsCharTraits<CharT>::move(iter, s, n);
        iter += n;
        return n;
      }
  };

#else

NS_SPECIALIZE_TEMPLATE
struct nsCharSinkTraits<char*>
  {
    static
    PRUint32
    write( char*& iter, const char* s, PRUint32 n )
      {
        nsCharTraits<char>::move(iter, s, n);
        iter += n;
        return n;
      }
  };

NS_SPECIALIZE_TEMPLATE
struct nsCharSinkTraits<PRUnichar*>
  {
    static
    PRUint32
    write( PRUnichar*& iter, const PRUnichar* s, PRUint32 n )
      {
        nsCharTraits<PRUnichar>::move(iter, s, n);
        iter += n;
        return n;
      }
  };

#endif

#endif // !defined(nsCharTraits_h___)
