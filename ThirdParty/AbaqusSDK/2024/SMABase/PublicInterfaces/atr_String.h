//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef atr_String_h
#define atr_String_h

#include <stdarg.h>

// Begin local includes
#include <atr_StringBase.h>

//
// Forwards
//
class SMABasStringMode;

//
// A string class that provides an i18n translated string.  This class has a private 
// constructors so that it can only be created through the proper use of utl_tr.

// APIs that currently take strings or characters for displayed text will be modified to take
// a translated string only. 
//
//
// atr_AdoptedString allows atr_String to *ADOPT* an allocated char*
// NULL-TERMINATED string.  This must NEVER be used with a literal
// string and once the char* is passed to the atr_String, do not
// delete it!

// WARNING: use of atr_AdoptedString must be "blessed" by
//          an Architecture developer. Client should pass
//          in length if s/he knows it, thus saving a strlen.
struct atr_AdoptedString {
    atr_AdoptedString(char* s, size_t sz) : str(s), size(sz) { }
    char*  str;
    size_t size;
};


class atr_String :public atr_StringBase
{
public:
    friend atr_String atr_raw(const char *pStr, const SMABasStringMode);
friend atr_String atr_MakeAtrString(char const *pStr);

    //
    // Constructors (private in the future) except for empty one
    //

    atr_String(){ }


    // Need this now that it is not automatically generated correctly
    atr_String(const atr_String& rhs)
        : atr_StringBase(rhs)
    { }


    //
    // Methods (In alphabetical order)
    //

    // Append 'str' to this 
    inline atr_String&   Append(const atr_String& str);
    inline atr_String&   AppendNewLine();

    // Compatible with strcmp(this, rhs)
    inline int Compare(const atr_String&) const;

    // For compatibility with C
    inline const char * text() const;
    inline const char * CStr() const;

    inline bool   empty() const { return bool(Length() == 0 ); }
    inline bool   Empty() const { return bool(Length() == 0 ); }

    // Remove all chars in string
    inline atr_String&   Erase();
    /// Extract partition of delimiter separated string
    atr_String extract(int part,char delim) const;

    /// Extract partition of delimiter separated string
    atr_String extract(int part,char delim,char esc) const;


    inline size_t Find(const atr_String& str, 
                             size_t start = 0) const;

    inline atr_String&  Insert (size_t start, const atr_String& s);

    inline atr_String&  LeftTrim(const atr_String& char_set);


    inline size_t Length() const { return atr_StringBase::Length();}
    inline size_t length() const { return atr_StringBase::Length();}

    atr_String&   ReplaceNonTranslated(int position, int repVal); // Substitute for %d, %1$d %2$d
    atr_String&   ReplaceNonTranslated(int position, const char * repVal); // Substitute for %s, %1$s %2$s
    atr_String&   ReplaceNonTranslated(int position, const atr_StringBase& repVal); // Substitute for %s, %1$s %2$s
    atr_String&   ReplaceNonTranslated(const char* pattern,const atr_StringBase&   repVal); // Substitute for %PART_NAME
    atr_String&   ReplaceNonTranslated(const char* pattern,const char*   repVal); // Substitute for %PART_NAME


    inline atr_String& Replace(size_t start, size_t len, 
                               const atr_String& s);

    inline size_t RFind(const atr_String& str, 
                              size_t start = atr_StrEnd) const;

    // Support conversion to/from UTF8/Locale
    atr_String LocaleToUTF8() const;
    atr_String UTF8ToLocale() const;

       // Return a *new* substring given 'start' and 'len'
    atr_String Substr(size_t start, size_t len = atr_StrEnd) const;

    // Change the case
    inline atr_String&   ToLower();
    inline atr_String&   ToUpper();


    // utility methods to break apart lines
    static atr_String AddLineBreaks(
        const atr_String&,
        const size_t,
        bool noOpIfPreformatted = false );

    static atr_String BreakLineString(const atr_String&, const size_t );

    //
    // Member operators
    //

    inline atr_String& operator = (const atr_String& str);

    inline atr_String& operator += (const atr_String& str);


    // numeric constructors

    atr_String(int i) 
        : atr_StringBase(i)
    { }

    atr_String(float f)
        : atr_StringBase(f)
    { }

    atr_String(unsigned int u)
        : atr_StringBase(u)
    { }

    atr_String(double d)
        : atr_StringBase(d)
    { }


    private:

    // what you'd expect; COPIES the passed in data
    // outlined cause some silly users do a atr_String command("");
    // and I'd like to optimize for that case
    atr_String(const char* copy_me) 
        :atr_StringBase(copy_me?copy_me:"")
    { }

    // The conversion constructor from atr_AdoptedString is optimized
    // to avoid copying the underlying C-string.
    // WARNING: use of atr_AdoptedString must be "blessed" by
    //          an Architecture developer
    atr_String(atr_AdoptedString s)
      : atr_StringBase(s.str, s.size < INT_MAX ? static_cast<int>(s.size):INT_MAX, ADOPT)
    {}

    // copies len bytes of the const char*
    // WARNING! this routine does not assumes a null-terminated
    // string was passed in. So make sure the length you pass in is
    // CORRECT; that is, it doesn't run past the end of the passed in
    // const char*
    atr_String(const char* copy_me, size_t len)
      : atr_StringBase(copy_me, len<INT_MAX ? static_cast<int>(len) : INT_MAX)
    { }

    atr_String(const atr_StringBase& rhs)
        : atr_StringBase(rhs)
    { }


};

// Returns lhs.append(rhs)
inline atr_String operator + (atr_String lhs, const atr_String& rhs);

// Returns lhs.compare(rhs) == 0
inline bool operator == (const atr_String& lhs, const atr_String& rhs);

// Returns !(lhs == rhs)
inline bool operator != (const atr_String& lhs, const atr_String& rhs);

// Returns lhs.compare(rhs) < 0
inline bool operator < (const atr_String& lhs, const atr_String& rhs);

// Returns lhs.compare(rhs) > 0
inline bool operator > (const atr_String& lhs, const atr_String& rhs);

// Returns lhs.compare(rhs) <= 0
inline bool operator <= (const atr_String& lhs, const atr_String& rhs);

// Returns lhs.compare(rhs) >= 0
inline bool operator >= (const atr_String& lhs, const atr_String& rhs);



// Append another string to the end of this string
inline atr_String& atr_String::Append(const atr_String& str)
{ 
    size_t sln( str.Length() );
    atr_StringBase::append(str.text(), sln<INT_MAX ? static_cast<int>(sln) : INT_MAX, 0, atr_StrEnd);
    return *this;

}

// Append another string to the end of this string
inline atr_String& atr_String::AppendNewLine()
{ 
    atr_String str("\n");
    size_t sln( str.Length() );
    atr_StringBase::append(str.CStr(), sln<INT_MAX ? static_cast<int>(sln) : INT_MAX, 0, atr_StrEnd);
    return *this;
}


inline atr_String&   atr_String::Erase()
{
    atr_StringBase::Erase(0, atr_StrEnd);
    return *this; 
}

inline size_t atr_String::Find(const atr_String& str, 
                               size_t start) const
{
  return atr_StringBase::Find(str.text(), start<INT_MAX ? static_cast<int>(start) : INT_MAX);
}

// Insert all from 's' at this['start']
inline atr_String& atr_String::Insert(size_t start, 
                                      const atr_String& str)
{
    size_t sln( str.Length() );
    atr_StringBase::insert(start < INT_MAX ? static_cast<int>(start) : INT_MAX, str.text(), sln < INT_MAX ? static_cast<int>(sln) : INT_MAX, 0, atr_StrEnd);   
    return *this; 
}


inline atr_String& atr_String::LeftTrim(const atr_String& str)
{
    atr_StringBase::LeftTrim(str.text());   
    return *this; 
}


inline atr_String& atr_String::Replace(
    size_t            start, 
    size_t            len, 
    const atr_String& s
)
{
    size_t sln( s.Length() );
    atr_StringBase::replace(start < INT_MAX ? static_cast<int>(start) : INT_MAX, len < INT_MAX ? static_cast<int>(len) : INT_MAX, s.text(), sln < INT_MAX ? static_cast<int>(sln) : INT_MAX, 0, atr_StrEnd);
    return *this; 
}


inline size_t atr_String::RFind(
    const atr_String& str, 
    size_t            start
) const
{
    size_t sln( str.Length() );
    return atr_StringBase::rfind(str.text(), sln < INT_MAX ? static_cast<int>(sln) : INT_MAX, start < INT_MAX ? static_cast<int>(start) : INT_MAX);
}


// Change the case
inline atr_String&   atr_String::ToLower()
{
    atr_StringBase::ToLower();
    return *this; 
}

inline atr_String&   atr_String::ToUpper()
{
    atr_StringBase::ToUpper();
    return *this; 
}



// compatible with strcmp
inline int atr_String::Compare(const atr_String& rhs) const
{
    return atr_StringBase::Compare(rhs);
}


inline const char* atr_String::text() const { return atr_StringBase::CStr(); }
inline const char* atr_String::CStr() const { return atr_StringBase::CStr(); }



//
// Member Operators
//

inline atr_String& atr_String::operator += (const atr_String& str)
{ return Append(str); }

inline atr_String& atr_String::operator = (const atr_String& rhs)
{
    atr_StringBase::operator = (rhs);
    return *this; 
}


//
// Non-member Operators
//

inline atr_String operator + (atr_String lhs, const atr_String& rhs)
{ return lhs.Append(rhs); }


// Operator ==
inline bool operator == (const atr_String& lhs, const atr_String& rhs)
{ return bool(lhs.Compare(rhs) == 0); }


// Operator !=
inline bool operator != (const atr_String& lhs, const atr_String& rhs)
{ return bool(lhs.Compare(rhs) != 0); }


// Operator <
inline bool operator < (const atr_String& lhs, const atr_String& rhs)
{ return bool(lhs.Compare(rhs) < 0); }

// Operator >
inline bool operator > (const atr_String& lhs, const atr_String& rhs)
{ return bool(lhs.Compare(rhs) > 0); }

// Operator <=
inline bool operator <= (const atr_String& lhs, const atr_String& rhs)
{ return bool(lhs.Compare(rhs) <= 0); }

// Operator >=
inline bool operator >= (const atr_String& lhs, const atr_String& rhs)
{ return bool(lhs.Compare(rhs) >= 0); }

#endif // atr_String_h
