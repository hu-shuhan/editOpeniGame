//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef cow_String_h
#define cow_String_h


// Begin local includes

#include <atr_StringBase.h>
#include <omn_FixedString.h>
#include <omn_pragma.h>

//
// Forwards
//

class aio_ostream;
class bio_Ifilter;

// cow_AdoptedString allows cow_String to *ADOPT* an allocated char*
// NULL-TERMINATED string.  This must NEVER be used with a literal
// string and once the char* is passed to the cow, do not
// delete it!

// WARNING: use of cow_AdoptedString must be "blessed" by
//          an Architecture developer. Client should pass
//          in length if s/he knows it, thus saving a strlen.
struct cow_AdoptedString {
    cow_AdoptedString(char* s, size_t sz) : str(s), size(sz < INT_MAX ? static_cast<int>(sz) : INT_MAX) { }
    char*  str;
    int size;
};

//
// A string class that provides Copy On Write behavior.
// The interface is a combination of ISO C++ (changed to reflect 
// our naming conventions) and some other methods that were used 
// heavily in our application. For documentation on the methods,
// see http://web.hks.com/ipgrp/RogueWave/libstd/bas_0007.htm for
// the ISO C++ methods.
//
// Many of these methods return a cow_String&. In every case this is a
// reference to 'this', so no, this is not an 'immutable string' class. In
// fact, after the first Get() of the internal COW, all the methods
// that are mutators will mutate the cow_String directly with *no
// further copies*.
//
// The additions to the ISO interface are Trim, RightTrim, LeftTrim,
// ToUpper and ToLower, and the constructors taking an int, int,
// float and double.
//
// Note: the CStr() method can be used to pass a cow_String to a function
// expecting a const char *. *DO NOT* hang on to this pointer, it ain't
// yours, and if you mutate the cow_String it may be invalid. Also, 
// *DO NOT* cast away the result of CStr to a char* so you can play with
// the underlying representation. You will be sorry. In short, pretend
// the method's name was KEYSTER, cause that will be the place that hurts
// if you abuse CStr().
//

class cow_String : public atr_StringBase
{
public:

    //
    // Constructors
    //

    cow_String() {}

    // Critical conversion constructor from base text representation.
    cow_String(const atr_StringBase& rhs)
        : atr_StringBase(rhs)
    {}

    // The conversion constructor from omn_FixedString is optimized
    // to avoid copying the underlying C-string.
    cow_String(omn_FixedString s, int len = 0)
      : atr_StringBase(s, len ? len : (strlen(s) < INT_MAX ? static_cast<int>(strlen(s)) : INT_MAX), BORROW)
    {}

    // The conversion constructor from cow_AdoptedString is optimized
    // to avoid copying the underlying C-string.
    // WARNING: use of cow_AdoptedString must be "blessed" by
    //          an Architecture developer
    cow_String(cow_AdoptedString s)
        : atr_StringBase(s.str, s.size, ADOPT)
    {}

    // ctor of empty string with a presized buffer of size sz
    cow_String(int sz, PreSizeEnm en)
        : atr_StringBase(sz, en)
    {}

    // what you'd expect; COPIES the passed in data
    // outlined cause some silly users do a cow_String command("");
    // and I'd like to optimize for that case
    cow_String(const char* copy_me) 
        :atr_StringBase(copy_me)
    {}

    // copies len bytes of the const char*
    // WARNING! This routine does not check the length of the C-string!

    cow_String(const char* copy_me, int len)
        : atr_StringBase(copy_me, len)
    { }

    // constructs string representations (like sprintf) of the
    // passed in args
    cow_String(int i) 
        : atr_StringBase(i)
    {}

    cow_String(unsigned int ui)
        : atr_StringBase(ui)
    {}

    cow_String(float f )
        : atr_StringBase(f)
    {}

    cow_String(double d)
        : atr_StringBase(d)
    {}

    //
    // Methods (In alphabetical order)
    //

    // Append 'str' to this
    inline cow_String&   Append(const cow_String& str);
    inline cow_String&   Append(const omn_FixedString str);

    inline cow_String&   Append(const char*);
    
    // Append from 'str' 'len' chars.
    // WARNING! This routine does not check the length of the C-string!
    inline cow_String&   Append(const char*, int len);

    // Append from 'str'['start'] 'len' chars
    inline cow_String&   Append(const cow_String& str, 
                                int start, int len);

    // Compatible with strcmp(this, rhs)
    inline int Compare(const cow_String&) const;
    inline int Compare(omn_FixedString) const;
    inline int Compare(const char*) const;

    // For compatibility with C
    inline const char * CStr() const;

    inline bool   Empty() const { return bool(Length() == 0 ); }

    // Remove all chars in string
    inline cow_String&   Erase();

    // Remove 'len' chars beginning at this['start']
    inline cow_String&   Erase(int start, 
                               int len = atr_StrEnd);

    // Returns index of substring str beginning at this[start]
    // Returns Length() if not found.
    inline int Find(const char * str, int start = 0) const;
    inline int Find(const cow_String& str, 
                       int start = 0) const;
    inline int Find(omn_FixedString str, 
                       int start = 0) const;

    // Returns (conceptually) Find(cow_String(s, len), start)
    inline int Find (const char* s, int start, 
                        int len) const;

    // Insert from 's' 'len' chars at this['start']
    inline cow_String&  Insert (int start, const char* s, 
                                int len = atr_StrEnd);

    inline cow_String&  Insert (int start, const cow_String& s);
    inline cow_String&  Insert (int start, omn_FixedString s);

    // Insert from 'str'['start2'] 'len' chars at this['start1']
    inline cow_String&  Insert (int start1, const cow_String& str,
                                int start2, int len);


    // Removes any character(s) in "char_set" that appear as a prefix
    inline cow_String&  LeftTrim(const char* char_set);

    inline int Length() const { return atr_StringBase::Length();}

    // Replace 'len' chars of this['start'] with all of 's'
    inline cow_String& Replace(int start, int len, 
                               const char* s);
    inline cow_String& Replace(int start, int len, 
                               const cow_String& s);
    inline cow_String& Replace(int start, int len, 
                               omn_FixedString s);

    // Replace 'len1' chars at this['start1'] with
    // 'len2' chars at 's'['start2']
    inline cow_String& Replace(int start1, int len1, 
                               const cow_String& s,
                               int start2, int len2);

    inline cow_String& Replace(int start, int len1, 
                               const char* s,
                               int len2);

    inline cow_String&   Reverse();

    // Returns index of substring str before or at this[start]
    inline int RFind(const char * str, 
                        int start = atr_StrEnd) const;
    inline int RFind(const cow_String& str, 
                        int start = atr_StrEnd) const;
    inline int RFind(omn_FixedString str, 
                        int start = atr_StrEnd) const;

    // Returns (conceptually) RFind(cow_String(s, len), start)
    inline int RFind(const char* s, int start, 
                        int len) const;

    // Removes any character(s) in "char_set" that appear as a suffix
    inline cow_String&   RightTrim(const char* char_set);

    // Simple-minded Substitute (for a more general one that can
    // take regular expressions, see rgx_funcs.h)
    // use atr_StrEnd as replace count if you want a global
    // substitution
    inline cow_String&   Substitute(const cow_String& source,
                                    const cow_String& replacement,
                                    int replaceCount = 1);

    // Return a *new* substring given 'start' and 'len'
    // Substr() returns a copy.
    inline cow_String Substr(int start = 0, int len = atr_StrEnd) const;

    // Change the case
    inline cow_String&   ToLower();
    inline cow_String&   ToUpper();

    // Capitalize the first letter
    inline cow_String&   Capitalize();

    // Uncapitalize the first letter
    inline cow_String&   Uncapitalize();

    // Removes any character(s) in "char_set"
    inline cow_String&   Trim(const char* char_set);


    //
    // Member operators
    //

    inline char operator [](int ix) const;

    // will copy string before allowing assignment
    inline char& operator [](int);

    inline cow_String& operator = (const cow_String& str);
    inline cow_String& operator = (omn_FixedString str);
    inline cow_String& operator = (const atr_StringBase& str);
    // This one needs to avoid an allocation, if possible.
    inline cow_String& operator = (const char* str);

    inline cow_String& operator += (const cow_String& str);
    inline cow_String& operator += (omn_FixedString str);
    inline cow_String& operator += (const char* str);

private:

    // Friends (for efficiency; saves a copy)
    friend bio_Ifilter& operator>>(bio_Ifilter &, cow_String *&);
    friend bio_Ifilter& operator>>(bio_Ifilter &, cow_String &);

};

// Returns lhs.append(rhs)
inline cow_String operator + (cow_String lhs, const cow_String& rhs);
inline cow_String operator + (cow_String lhs, const char* rhs);
inline cow_String operator + (cow_String lhs, omn_FixedString rhs);

// Returns lhs.compare(rhs) == 0
inline bool operator == (const cow_String& lhs, const cow_String& rhs);
inline bool operator == (const char* lhs, const cow_String& rhs);
inline bool operator == (const cow_String& lhs, const char* rhs);

// Returns !(lhs == rhs)
inline bool operator != (const cow_String& lhs, const cow_String& rhs);
inline bool operator != (const char* lhs, const cow_String& rhs);
inline bool operator != (const cow_String& lhs, const char* rhs);

// Returns lhs.compare(rhs) < 0
inline bool operator < (const cow_String& lhs, const cow_String& rhs);
inline bool operator < (const char* lhs, const cow_String& rhs);
inline bool operator < (const cow_String& lhs, const char* rhs);

// Returns lhs.compare(rhs) > 0
inline bool operator > (const cow_String& lhs, const cow_String& rhs);
inline bool operator > (const char* lhs, const cow_String& rhs);
inline bool operator > (const cow_String& lhs, const char* rhs);

// Returns lhs.compare(rhs) <= 0
inline bool operator <= (const cow_String& lhs, const cow_String& rhs);
inline bool operator <= (const char* lhs, const cow_String& rhs);
inline bool operator <= (const cow_String& lhs, const char* rhs);

// Returns lhs.compare(rhs) >= 0
inline bool operator >= (const cow_String& lhs, const cow_String& rhs);
inline bool operator >= (const char* lhs, const cow_String& rhs);
inline bool operator >= (const cow_String& lhs, const char* rhs);


// Append another string to the end of this string
inline cow_String& cow_String::Append(const cow_String& str)
{ 
    atr_StringBase::append(str.CStr(), str.Length(), 0, atr_StrEnd);
    return *this;

}

// Append another string to the end of this string
inline cow_String& cow_String::Append(const omn_FixedString str)
{ 
    size_t sln( strlen(str) );
    atr_StringBase::append(str, sln < INT_MAX ? static_cast<int>(sln) : INT_MAX, 0, atr_StrEnd);
    return *this; 
}

// Append a C-string to the end of this string
inline cow_String& cow_String::Append(const char* cstr)
{
    size_t sln( strlen(cstr) );
    atr_StringBase::append(cstr, sln < INT_MAX ? static_cast<int>(sln) : INT_MAX, 0, atr_StrEnd);
    return *this;
}

// Append len characters of the C-string to the end of this string
inline cow_String& cow_String::Append(const char* cstr, int len)
{
  atr_StringBase::append(cstr, len, 0, len);
  return *this;
}


// Append from 'str' 'len' chars beginning at  this['start']
inline cow_String& cow_String::Append(const cow_String &str, 
                                      int           start, 
                                      int           len)
{
    atr_StringBase::append(str.CStr(), str.Length(), start, len);
    return *this; 
}

// compatible with strcmp
inline int cow_String::Compare(const cow_String& rhs) const
{
    return atr_StringBase::Compare(rhs);
}

inline int cow_String::Compare(omn_FixedString rhs) const
{
    return atr_StringBase::Compare(rhs);
}

inline int cow_String::Compare(const char* rhs) const
{
    return atr_StringBase::Compare(rhs);
}

inline const char* cow_String::CStr() const { return atr_StringBase::CStr(); }

inline cow_String&   cow_String::Erase(int start, int len )
{
    atr_StringBase::Erase(start, len);
    return *this; 
}

inline cow_String&   cow_String::Erase()
{
    atr_StringBase::Erase(0, atr_StrEnd);
    return *this; 
}

inline int cow_String::Find(const char * str, int start) const
{
    return atr_StringBase::Find(str,start);
}
inline int cow_String::Find(const cow_String& str, 
                               int start) const
{
    return atr_StringBase::Find(str.CStr(), start);
}

inline int cow_String::Find(omn_FixedString str, 
                               int start) const
{
    return atr_StringBase::Find((const char*)str, start);
}

// Returns (conceptually) Find(cow_String(s, len), start)
inline int cow_String::Find(const char* s, int start, 
                               int len) const
{
    return atr_StringBase::Find(cow_String(s, len), start);
}

// Insert all from 's' at this['start']
inline cow_String& cow_String::Insert(int start, 
                                      const cow_String& str)
{
    atr_StringBase::insert(start, str.CStr(), str.Length(), 0, atr_StrEnd);   
    return *this; 
}

inline cow_String& cow_String::Insert(int start, 
                                      omn_FixedString str)
{
    size_t sln( strlen(str) );
    atr_StringBase::insert(start, str, sln < INT_MAX ? static_cast<int>(sln) : INT_MAX, 0, atr_StrEnd);   
    return *this; 
}

// Insert from 'str'['start2'] 'len' chars at this['start1']
inline cow_String& cow_String::Insert(int            start1, 
                                      const cow_String& str,
                                      int            start2, 
                                      int            len)
{
    atr_StringBase::insert(start1, str.CStr(), str.Length(), start2, len);
    return *this; 
}

// Insert from 's' 'len' chars at this['start']
inline cow_String& cow_String::Insert(int            start, 
                                      const char*       s, 
                                      int            len)
{
    size_t sln( strlen(s) );
    atr_StringBase::insert(start, s, sln < INT_MAX ? static_cast<int>(sln) : INT_MAX, 0, len);
    return *this; 
}

inline cow_String&  cow_String::LeftTrim(const char* char_set)
{
    atr_StringBase::LeftTrim(char_set);
    return *this; 
}


// Replace 'len' chars of this['start'] with all of 's'
inline cow_String& cow_String::Replace(int start, 
                                       int len, const char* s)
{
    size_t sln( strlen(s) );
    atr_StringBase::replace(start, len, s, sln < INT_MAX ? static_cast<int>(sln) : INT_MAX, 0, atr_StrEnd);
    return *this; 
}

inline cow_String& cow_String::Replace(int start, 
                                       int len, omn_FixedString s)
{
    size_t sln( strlen(s) );
    atr_StringBase::replace(start, len, s, sln < INT_MAX ? static_cast<int>(sln) : INT_MAX, 0, atr_StrEnd);
    return *this; 
}

inline cow_String& cow_String::Replace(int            start, 
                                       int            len, 
                                       const cow_String& s)
{
    atr_StringBase::replace(start, len, s.CStr(), s.Length(), 0, atr_StrEnd);
    return *this; 
}


// Replace 'len1' chars at this['start1'] with
// 'len2' chars at 's'['start2']
inline cow_String& cow_String::Replace(int            start1, 
                                       int            len1, 
                                       const cow_String& s,
                                       int            start2, 
                                       int            len2)
{
    atr_StringBase::replace(start1, len1, s.CStr(), s.Length(), start2, len2);
    return *this; 
}

inline cow_String& cow_String::Replace(int start, 
                                       int len1,
                                       const char* s,
                                       int len2)
{
    atr_StringBase::Replace(start, len1, s,  len2);
    return *this; 
}


inline cow_String&   cow_String::Reverse()
{
    atr_StringBase::Reverse();
    return *this; 
}

// Returns index of substring str before or at this[start]
inline int cow_String::RFind(const char * str, 
                                int       start) const
{
    size_t sln( strlen(str) );
    return atr_StringBase::rfind(str, sln < INT_MAX ? static_cast<int>(sln) : INT_MAX, start);
}

inline int cow_String::RFind(const cow_String& str, 
                                int            start) const
{
    return atr_StringBase::rfind(str.CStr(), str.Length(), start);
}

inline int cow_String::RFind(omn_FixedString   str, 
                                int            start) const
{
    size_t sln( strlen(str) );
    return atr_StringBase::rfind(str, sln < INT_MAX ? static_cast<int>(sln) : INT_MAX, start);
}

// Returns (conceptually) RFind(cow_String(s, len), start)
inline int cow_String::RFind(const char* s, 
                                int start, 
                                int len) const
{
    return atr_StringBase::RFind(cow_String(s, len), start);
}

inline cow_String&   cow_String::RightTrim(const char* char_set)
{
    atr_StringBase::RightTrim(char_set);
    return *this; 
}

inline cow_String&   cow_String::Substitute(const cow_String& source,
                                            const cow_String& replacement,
                                            int replaceCount)
{
    atr_StringBase::Substitute(source,replacement,replaceCount);
    return *this; 
}


inline cow_String cow_String::Substr(int start, int len ) const
{
    return cow_String(CStr()+start, atr_StringBase::Substr(start, len));
}

// Removes any character(s) in "char_set"
inline cow_String&   cow_String::Trim(const char* char_set)
{
    atr_StringBase::Trim(char_set);
    return *this; 
}

// Change the case
inline cow_String&   cow_String::ToLower()
{
    atr_StringBase::ToLower();
    return *this; 
}

inline cow_String&   cow_String::ToUpper()
{
    atr_StringBase::ToUpper();
    return *this; 
}


// Capitalize the first letter
inline cow_String&   cow_String::Capitalize()
{
    atr_StringBase::Capitalize();
    return *this; 
}


// Uncapitalize the first letter
inline cow_String&   cow_String::Uncapitalize()
{
    atr_StringBase::Uncapitalize();
    return *this; 
}


inline char cow_String::operator [](int ix) const
{
    return atr_StringBase::operator [](ix);
}

// will copy string before allowing assignment
inline char& cow_String::operator [](int ix)
{
    return atr_StringBase::operator [](ix);
}


//
// Member Operators
//

inline cow_String& cow_String::operator += (const cow_String& str)
{ return Append(str); }

inline cow_String& cow_String::operator += (omn_FixedString str)
{ return Append(str); }

inline cow_String& cow_String::operator += (const char* str)
{ return Append(str); }

inline cow_String& cow_String::operator = (const cow_String& rhs)
{
    atr_StringBase::operator = (rhs);
    return *this; 
}

inline cow_String& cow_String::operator = (omn_FixedString rhs)
{
    atr_StringBase::operator = (rhs);   
    return *this; 
}

inline cow_String& cow_String::operator = (const char* rhs)
{
    atr_StringBase::operator = (rhs);   
    return *this; 
}

inline cow_String& cow_String::operator = (const atr_StringBase& rhs)
{
    atr_StringBase::operator = (rhs);
    return *this; 
}
//
// Non-member Operators
//

inline cow_String operator + (cow_String lhs, const cow_String& rhs)
{ return lhs.Append(rhs); }

inline cow_String operator + (cow_String lhs, const char* rhs)
{ return lhs.Append(rhs); }

inline cow_String operator + (cow_String lhs, omn_FixedString rhs)
{ return lhs.Append(rhs); }


// Operator ==
inline bool operator == (const cow_String& lhs, const cow_String& rhs)
{ return bool(lhs.Compare(rhs) == 0); }

inline bool operator == (const cow_String& lhs, const char* rhs)
{ return bool(lhs.Compare(rhs) == 0); }

inline bool operator == (const char* lhs, const cow_String& rhs)
{ return bool(rhs.Compare(lhs) == 0); }

inline bool operator == (const cow_String& lhs, omn_FixedString rhs)
{ return bool(lhs.Compare(rhs) == 0); }

inline bool operator == (omn_FixedString lhs, const cow_String& rhs)
{ return bool(rhs.Compare(lhs) == 0); }

// Operator !=
inline bool operator != (const cow_String& lhs, const cow_String& rhs)
{ return bool(lhs.Compare(rhs) != 0); }

inline bool operator != (const cow_String& lhs, const char* rhs)
{ return bool(lhs.Compare(rhs) != 0); }

inline bool operator != (const char* lhs, const cow_String& rhs)
{ return bool(rhs.Compare(lhs) != 0); }

inline bool operator != (const cow_String& lhs, omn_FixedString rhs)
{ return bool(lhs.Compare(rhs) != 0); }

inline bool operator != (omn_FixedString lhs, const cow_String& rhs)
{ return bool(rhs.Compare(lhs) != 0); }

// Operator <
inline bool operator < (const cow_String& lhs, const cow_String& rhs)
{ return bool(lhs.Compare(rhs) < 0); }

inline bool operator < (const cow_String& lhs, const char* rhs)
{ return bool(lhs.Compare(rhs) < 0); }

inline bool operator < (const char* lhs, const cow_String& rhs)
{ return bool(rhs.Compare(lhs) > 0); }

inline bool operator < (const cow_String& lhs, omn_FixedString rhs)
{ return bool(lhs.Compare(rhs) < 0); }

inline bool operator < (omn_FixedString lhs, const cow_String& rhs)
{ return bool(rhs.Compare(lhs) > 0); }

// Operator >
inline bool operator > (const cow_String& lhs, const cow_String& rhs)
{ return bool(lhs.Compare(rhs) > 0); }

inline bool operator > (const cow_String& lhs, const char* rhs)
{ return bool(lhs.Compare(rhs) > 0); }

inline bool operator > (const char* lhs, const cow_String& rhs)
{ return bool(rhs.Compare(lhs) < 0); }

inline bool operator > (const cow_String& lhs, omn_FixedString rhs)
{ return bool(lhs.Compare(rhs) > 0); }

inline bool operator > (omn_FixedString lhs, const cow_String& rhs)
{ return bool(rhs.Compare(lhs) < 0); }

// Operator <=
inline bool operator <= (const cow_String& lhs, const cow_String& rhs)
{ return bool(lhs.Compare(rhs) <= 0); }

inline bool operator <= (const cow_String& lhs, const char* rhs)
{ return bool(lhs.Compare(rhs) <= 0); }

inline bool operator <= (const char* lhs, const cow_String& rhs)
{ return bool(rhs.Compare(lhs) >= 0); }

inline bool operator <= (const cow_String& lhs, omn_FixedString rhs)
{ return bool(lhs.Compare(rhs) <= 0); }

inline bool operator <= (omn_FixedString lhs, const cow_String& rhs)
{ return bool(rhs.Compare(lhs) >= 0); }

// Operator >=
inline bool operator >= (const cow_String& lhs, const cow_String& rhs)
{ return bool(lhs.Compare(rhs) >= 0); }

inline bool operator >= (const cow_String& lhs, const char* rhs)
{ return bool(lhs.Compare(rhs) >= 0); }

inline bool operator >= (const char* lhs, const cow_String& rhs)
{ return bool(rhs.Compare(lhs) <= 0); }

inline bool operator >= (const cow_String& lhs, omn_FixedString rhs)
{ return bool(lhs.Compare(rhs) >= 0); }

inline bool operator >= (omn_FixedString lhs, const cow_String& rhs)
{ return bool(rhs.Compare(lhs) <= 0); }

// We provide this because cow_String is very close to a built-in.
aio_ostream& operator<< (aio_ostream&, const cow_String&);

#endif // cow_String_h
