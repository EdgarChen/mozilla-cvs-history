// profile_main.cpp

#include "nscore.h"
#include "prlong.h"
#include <iostream>
#include <string>
#include <iomanip>

#ifndef TEST_STD_STRING
#include "nsString.h"
#else
#include "nsStdStringWrapper.h"
typedef nsStdCString nsCString;
#endif

#include <Timer.h>

static const int kTestSucceeded = 0;
static const int kTestFailed = 1;


template <class T>
inline
PRUint32
TotalLength( const T& s )
  {
    return s.Length();
  }

template <>
inline
PRUint32
TotalLength( const string& s )
  {
    return s.length();
  }

template <class T>
inline
PRUint32
Find( const T& text, const T& pattern )
  {
    return text.Find(pattern);
  }

template <>
inline
PRUint32
Find( const string& text, const string& pattern )
  {
    return text.find(pattern);
  }

inline
UnsignedWide
operator-( const UnsignedWide& lhs, const UnsignedWide& rhs )
  {
    PRInt64 a = LL_INIT(lhs.hi, lhs.lo);
    PRInt64 b = LL_INIT(rhs.hi, rhs.lo);
    PRInt64 c;
    LL_SUB(c, a, b);
    
    UnsignedWide result = {c.hi, c.lo};

    return result;
  }

class TestTimer
  {
    public:
      TestTimer() { Microseconds(&mStartTime); }

     ~TestTimer()
        {
          UnsignedWide stopTime;
          Microseconds(&stopTime);
          UnsignedWide totalTime = stopTime - mStartTime;
          if ( totalTime.hi )
            cout << "*";
          cout << setw(10) << totalTime.lo << " �s : ";
        }

    private:
      UnsignedWide mStartTime;
  };

static
int
test_concat()
  {
                //---------|---------|---------|---------|---------|---------|---------|
    nsCString s1("This is a reasonable length string with some text in it and it is good.");
    nsCString s2("This is another string that I will use in the concatenation test.");
    nsCString s3("This is yet a third string that I will use in the concatenation test.");

    PRUint32 len = TotalLength( s1 + s2 + s3 + s1 + s2 + s3 );
    if ( len != (71 + 65 + 69 + 71 + 65 + 69) )
      {
        cout << "|test_concat()| FAILED" << endl;
        return kTestFailed;
      }


    {
      TestTimer timer;
      for ( int i=0; i<100000; ++i )
        len += TotalLength( s1 + s2 + s3 + s1 + s2 + s3 );
    }
    cout << "TotalLength( s1 + s2 + s3 + s1 + s2 + s3 )" << endl;


    {
      TestTimer timer;
      for ( int i=0; i<100000; ++i )
        len += TotalLength( s1 + s2 );
    }
    cout << "TotalLength( s1 + s2 )" << endl;

    return kTestSucceeded;
  }

static
int
test_concat_and_assign()
  {
                //---------|---------|---------|---------|---------|---------|---------|
    nsCString s1("This is a reasonable length string with some text in it and it is good.");
    nsCString s2("This is another string that I will use in the concatenation test.");
    nsCString s3("This is yet a third string that I will use in the concatenation test.");

    nsCString s4 = s1 + s2 + s3 + s1 + s2 + s3;
    if ( TotalLength(s4) != (71 + 65 + 69 + 71 + 65 + 69) )
      {
        cout << "|test_concat()| FAILED" << endl;
        return kTestFailed;
      }


    
    {
      TestTimer timer;
      for ( int i=0; i<100000; ++i )
        s4 = s1 + s2 + s3 + s1 + s2 + s3;
    }
    cout << "s4 = s1 + s2 + s3 + s1 + s2 + s3" << endl;

    {
      TestTimer timer;
      for ( int i=0; i<100000; ++i )
        s4 = s1 + s2;
    }
    cout << "s4 = s1 + s2" << endl;


    return kTestSucceeded;
  }

static
int
test_compare()
  {
    nsCString s1("This is a reasonable length string with some text in it and it is good.");
    nsCString s2("This is a reasonable length string with some text in it and it is bad.");

    int count = 0;
    {
      TestTimer timer;
      for ( int i=0; i<100000; ++i )
        if ( s1 > s2 )
          ++count;
    }
    cout << "s1 > s2" << endl;

    {
      TestTimer timer;
      for ( int i=0; i<100000; ++i )
        if ( s1 == s1 )
          ++count;
    }
    cout << "s1 == s1" << endl;

    return kTestSucceeded;
  }

static
int
test_countchar()
  {
    nsCString s1("This is a reasonable length string with some text in it and it is good.");

    if ( s1.CountChar('e') != 5 )
      {
        cout << "|test_countchar()| FAILED: found a  count of " << s1.CountChar('e') << endl;
        return kTestFailed;
      }

    PRUint32 total = 0;
    {
      TestTimer timer;
      for ( int i=0; i<100000; ++i )
        total += s1.CountChar('e');
    }
    cout << "s1.CountChar('e')" << endl;

    return kTestSucceeded;
  }

static
int
test_append_string()
  {
    nsCString s1("This is a reasonable length string with some text in it and it is good.");
    nsCString s2("This is another string that I will use in the concatenation test.");

    PRUint32 len = 0;
    
    {
      TestTimer timer;
      for ( int i=0; i<100000; ++i )
        {
          nsCString s3;
          s3.Append(s1);
          s3.Append(s2);
          len += TotalLength(s3);
        }
    }
    cout << "s3.Append(s1); s3.Append(s2)" << endl;

    return kTestSucceeded;
  }

static
int
test_repeated_append_string()
  {
    nsCString s1("This is a reasonable length string with some text in it and it is good.");
    nsCString s2("This is another string that I will use in the concatenation test.");

    PRUint32 len = 0;
    
    {
      TestTimer timer;
      for ( int i=0; i<1000; ++i )
        {
          nsCString s3;
          for ( int j=0; j<100; ++j )
            {
              s3.Append(s1);
              s3.Append(s2);
              len += TotalLength(s3);
            }
        }
    }
    cout << "repeated s3.Append(s1); s3.Append(s2)" << endl;

    return kTestSucceeded;
  }

static
int
test_repeated_append_char()
  {
    PRUint32 len = 0;
    
    {
      TestTimer timer;
      for ( int i=0; i<1000; ++i )
        {
          nsCString s1;
          for ( int j=0; j<1000; ++j )
            {
              s1.AppendChar('e');
              len += TotalLength(s1);
            }
        }
    }
    cout << "repeated s1.Append('e')" << endl;

    return kTestSucceeded;
  }

static
int
test_insert_string()
  {
    nsCString s1("This is a reasonable length string with some text in it and it is good.");

    PRUint32 len = 0;
    
    {
      TestTimer timer;
      for ( int i=0; i<100000; ++i )
        {
          nsCString s2("This is another string that I will use in the concatenation test.");
          s2.Insert(s1, 3);
          len += TotalLength(s2);
        }
    }
    cout << "s2.Insert(s1, 3)" << endl;

    return kTestSucceeded;
  }

#if 0
static
int
test_find_string()
  {
    nsCString text("aaaaaaaaaab");
    nsCString pattern("aab");

    PRUint32 position = 0;
    {
      TestTimer timer;
      for ( int i=0; i<100000; ++i )
        position = Find(text, pattern);
    }
    cout << "text.Find(pattern)" << endl;

    return kTestSucceeded;
  }
#endif

int
main()
  {
    cout << "String performance profiling.  Compiled " __DATE__ " " __TIME__ << endl;
#ifdef TEST_STD_STRING
    cout << "Testing std::string." << endl;
#elif defined(NEW_STRING_APIS)
    cout << "Testing factored nsString." << endl;
#else
    cout << "Testing raw nsString." << endl;
#endif

    int tests_failed = 0;

    tests_failed += test_concat();
    tests_failed += test_concat_and_assign();
    tests_failed += test_compare();
    tests_failed += test_countchar();
    tests_failed += test_append_string();
    tests_failed += test_repeated_append_string();
    tests_failed += test_repeated_append_char();
    tests_failed += test_insert_string();
    // tests_failed += test_find_string();

    if ( tests_failed )
      cout << "One or more tests FAILED.  Measurements may be invalid." << endl;

    cout << "End of string performance profiling." << endl;
    return 0;
  }
