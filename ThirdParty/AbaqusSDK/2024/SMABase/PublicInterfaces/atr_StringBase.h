//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef atr_StringBase_h
#define atr_StringBase_h

#include <limits.h>            // for INT_MAX
#include <string.h>

// Begin local includes
#include <mem_AllocationOperatorsTiny.h>

// atr_StrEnd is useful for calls to Append to specify the remaining part
// of the string.
#define ATR_EOS 0
#define atr_StrEnd INT_MAX

// Has it's own specialized new and delete, do not derive from 
// mem_ALlocationOperators
class atr_StringRep
{
private:
    friend class atr_StringBase;

    int  m_References; // reference count
    int  m_AllocSize;  // allocated size of buffer
    int  m_Length;     // length of string

    // ctor(len)       -- borrowed string
    // ctor(len,len+1) -- adopted string
    atr_StringRep(int ln, int sz=0)
      : m_References(1), m_Length(ln), m_AllocSize(sz) {}

    // The reference count interface was copied from cow_Counter.
    //
    int References() const { return m_References; }
    void Increment() { m_References++; }
    bool Decrement() {
        --m_References;
#ifdef NDEBUG
        return m_References == 0;
#else
        return m_References <= 0;
#endif
    }

    inline void* operator new(size_t);
    inline void* operator new(size_t, int);
    inline void operator delete(void*);
    inline void operator delete(void*, int);
};

//
// A string class that provides Copy On Write behavior.
// The interface is a combination of ISO C++ (changed to reflect 
// our naming conventions) and some other methods that were used 
// heavily in our application. For documentation on the methods,
// see http://web.hks.com/ipgrp/RogueWave/libstd/bas_0007.htm for
// the ISO C++ methods.
//
// Many of these methods return a atr_StringBase&. In every case this is a
// reference to 'this', so no, this is not an 'immutable string' class. In
// fact, after the first Get() of the internal COW, all the methods
// that are mutators will mutate the atr_StringBase directly with *no
// further copies*.
//
// The additions to the ISO interface are Trim, RightTrim, LeftTrim,
// ToUpper and ToLower, and the constructors taking an int, unsigned int,
// float and double.
//
// Note: the CStr() method can be used to pass a atr_StringBase to a function
// expecting a const char *. *DO NOT* hang on to this pointer, it ain't
// yours, and if you mutate the atr_StringBase it may be invalid. Also, 
// *DO NOT* cast away the result of CStr to a char* so you can play with
// the underlying representation. You will be sorry. In short, pretend
// the method's name was KEYSTER, cause that will be the place that hurts
// if you abuse CStr().
//

class atr_StringBase  : public mem_AllocationOperatorsTiny
{
private:
    // Buffer policy:
    // Allocate m_Buff and m_StringRep.
    // This is called by constructors and COW methods.
    void allocate(int sz, int len = 0);

    // Extend m_Buff. Returns true if successful.
    // The is called by COW methods, when applicable.
    bool extend(int sz);

    // Copy a C-string. Calls allocate. Use sz=len if absent.
    void replicate(const char* copy_me, int len, int sz = 0);

public:
    enum PreSizeEnm { Presized };
    ~atr_StringBase()
    {
        if (m_StringRep && m_StringRep->Decrement())
            Delete();
    }

    // The following methods initialize/finalize the private allocator
    // and optional statistician.
    static void Initialize();
    static void Finalize();

protected:
    enum Adopt { ADOPT };
    enum Borrow { BORROW };

    //
    // Constructors
    //

    atr_StringBase()
        : m_Buff(0),
          m_StringRep(0)
    {}

    // BORROW the passed in literal string, and never free it nor modify it
    atr_StringBase(const char* borrow_me, int len, Borrow);

    // ADOPT the passed in buffer and FREE it when done
    atr_StringBase(char* adopt_me, int len, Adopt);

    // ctor of empty string with a presized buffer of size sz
    atr_StringBase(int sz, PreSizeEnm);

    // what you'd expect; COPIES the passed in data
    atr_StringBase(const char* copy_me);

    // copies len bytes of the const char*
    // WARNING! This routine does not check the length of the C-string!

    atr_StringBase(const char* copy_me, int len);

    // constructs string representations (like sprintf) of the
    // passed in args
    atr_StringBase(int);
    atr_StringBase(unsigned int);
    atr_StringBase(float);
    atr_StringBase(double);

    // Need this now that it is not automatically generated
    // correctly as it used to be when using a real cow
    atr_StringBase(const atr_StringBase& rhs)
        : m_Buff(rhs.m_Buff),
          m_StringRep(rhs.m_StringRep)
    {
        if (m_StringRep)
            m_StringRep->Increment();
    }

    //
    // Methods (In alphabetical order)
    //
    inline atr_StringBase&   Append(const atr_StringBase& str);

    inline atr_StringBase&   Append(const char*);

    // Append from 'str' 'len' chars.
    // WARNING! This routine does not check the length of the C-string!
    inline atr_StringBase&   Append(const char*, int len);


    // Append from 'str'['start'] 'len' chars
    inline atr_StringBase&   Append(const atr_StringBase& str, 
                                    int start, int len);



    // Compatible with strcmp(this, rhs)
    inline int Compare(const atr_StringBase&) const;
    inline int Compare(const char*) const;

    // For compatibility with C
    inline const char * CStr() const;

    inline bool   Empty() const { return bool(Length() == 0 ); }

    // Remove 'len' chars beginning at this['start']
    atr_StringBase&   Erase(int start, 
                            int len = atr_StrEnd);

    // Returns index of substring str beginning at this[start]
    // Returns Length() if not found.
    int Find(const char * str, int start = 0) const;
    inline int Find(const atr_StringBase& str, 
                       int start = 0) const;


    // Removes any character(s) in "char_set" that appear as a prefix
    atr_StringBase&  LeftTrim(const char* char_set);

    int Length() const { return m_StringRep == 0 ? 0 : m_StringRep->m_Length;}

    // Replace 'len' chars of this['start'] with all of 's'
    inline atr_StringBase& Replace(int start, int len, 
                                   const char* s);
    inline atr_StringBase& Replace(int start, int len, 
                                   const atr_StringBase& s);

    // Replace 'len1' chars at this['start1'] with
    // 'len2' chars at 's'['start2']
    inline atr_StringBase& Replace(int start1, int len1, 
                                   const atr_StringBase& s,
                                   int start2, int len2);

    atr_StringBase& Replace(int start, int len1, 
                            const char* s,
                            int len2);

    // Returns index of substring str before or at this[start]
    inline int RFind(const char * str, 
                        int start = atr_StrEnd) const;
    inline int RFind(const atr_StringBase& str, 
                        int start = atr_StrEnd) const;

    // Returns (conceptually) RFind(atr_StringBase(s, len), start)
    inline int RFind(const char* s, int start, 
                        int len) const;

    // Removes any character(s) in "char_set" that appear as a suffix
    atr_StringBase&   RightTrim(const char* char_set);

    // Simple-minded Substitute (for a more general one that can
    // take regular expressions, see rgx_funcs.h)
    // use atr_StrEnd as replace count if you want a global
    // substitution
    atr_StringBase&   Substitute(const atr_StringBase& source,
                                 const atr_StringBase& replacement,
                                 int replaceCount = 1);

    // Return trimmed length of a substring given 'start' and 'len'. 
    // The leaf class interface must create a new object!
    int Substr(int start = 0, int len = atr_StrEnd) const;

    // Change the case
    atr_StringBase&   ToLower();
    atr_StringBase&   ToUpper();

    // Capitalize the first letter
    atr_StringBase&   Capitalize();

    // Uncapitalize the first letter
    atr_StringBase&   Uncapitalize();

    // Removes any character(s) in "char_set"
    atr_StringBase&   Trim(const char* char_set);

    atr_StringBase&   Reverse();

    //
    // Member operators
    //

    char operator [](int ix) const;

    // will copy string before allowing assignment
    char& operator [](int);

    inline atr_StringBase& operator = (const atr_StringBase& str);
    // This one needs to avoid an allocation, if possible.
    atr_StringBase& operator = (const char* str);

    inline atr_StringBase& operator += (const atr_StringBase& str);
    inline atr_StringBase& operator += (const char* str);

    // Do the dirty work for destructor.
    void Delete();

    // Implement COW behavior.
    void isolate(int sz);
    char* get(int sz);

    atr_StringBase&   append(const char* cstr, int clen,
                             int start, int len);

    atr_StringBase&   insert(int start1, const char* s,
                             int slen, int start2, 
                             int len);

    atr_StringBase&   replace(int start1, int len1, 
                              const char* s, int slen,
                              int start2, int len2);

    int rfind(const char *s, int slen, 
                 int start) const;

    // data members; both pointers are null for an empty string
    char*             m_Buff;      // the growable char* buffer
    atr_StringRep* m_StringRep;

    inline int AllocatedSize() const ; // return allocated size of buffer

    // This method is used by derived string to massage the data in the buffer.
    // ***NOTE:***  It should never be called without calling the copy-on-
    // write memory management functions to get a buffer of the appropriate
    // size.  Never add more data into the buffer that what was retrieved
    // through the memory management call.  
    inline char*  GetBuffPtr();

    // Set the appropriate string length after massaging the data.
    // This will check that eos was placed at buff[len]
    void SetStringLength(int len);

    // Returns lhs.append(rhs)
    inline friend atr_StringBase operator + (atr_StringBase lhs, const atr_StringBase& rhs);
    inline friend atr_StringBase operator + (atr_StringBase lhs, const char* rhs);

    // Returns lhs.compare(rhs) == 0
    inline friend bool operator == (const atr_StringBase& lhs, const atr_StringBase& rhs);
    inline friend bool operator == (const char* lhs, const atr_StringBase& rhs);
    inline friend bool operator == (const atr_StringBase& lhs, const char* rhs);

    // Returns !(lhs == rhs)
    inline friend bool operator != (const atr_StringBase& lhs, const atr_StringBase& rhs);
    inline friend bool operator != (const char* lhs, const atr_StringBase& rhs);
    inline friend bool operator != (const atr_StringBase& lhs, const char* rhs);

    // Returns lhs.compare(rhs) < 0
    inline friend bool operator < (const atr_StringBase& lhs, const atr_StringBase& rhs);
    inline friend bool operator < (const char* lhs, const atr_StringBase& rhs);
    inline friend bool operator < (const atr_StringBase& lhs, const char* rhs);

    // Returns lhs.compare(rhs) > 0
    inline friend bool operator > (const atr_StringBase& lhs, const atr_StringBase& rhs);
    inline friend bool operator > (const char* lhs, const atr_StringBase& rhs);
    inline friend bool operator > (const atr_StringBase& lhs, const char* rhs);

    // Returns lhs.compare(rhs) <= 0
    inline friend bool operator <= (const atr_StringBase& lhs, const atr_StringBase& rhs);
    inline friend bool operator <= (const char* lhs, const atr_StringBase& rhs);
    inline friend bool operator <= (const atr_StringBase& lhs, const char* rhs);

    // Returns lhs.compare(rhs) >= 0
    inline friend bool operator >= (const atr_StringBase& lhs, const atr_StringBase& rhs);
    inline friend bool operator >= (const char* lhs, const atr_StringBase& rhs);
    inline friend bool operator >= (const atr_StringBase& lhs, const char* rhs);

};


// Append another string to the end of this string
inline atr_StringBase& atr_StringBase::Append(const atr_StringBase& str)
{ 
  return append(str.CStr(), str.Length(), 0, atr_StrEnd); 
}

// Append a C-string to the end of this string
inline atr_StringBase& atr_StringBase::Append(const char* cstr)
{
  size_t sln = strlen(cstr);
  return append(cstr, sln < INT_MAX ? static_cast<int>(sln) : INT_MAX, 0, atr_StrEnd);
}

// Append len characters of the C-string to the end of this string
inline atr_StringBase& atr_StringBase::Append(const char* cstr, int len)
{
  return append(cstr, len, 0, len);
}



// Append from 'str' 'len' chars beginning at  this['start']
inline atr_StringBase& atr_StringBase::Append(const atr_StringBase &str, 
                                              int           start, 
                                              int           len)
{
    return append(str.CStr(), str.Length(), start, len);
}


// compatible with strcmp
inline int atr_StringBase::Compare(const atr_StringBase& rhs) const
{
    return strcmp(CStr(), rhs.CStr());
}

inline int atr_StringBase::Compare(const char* rhs) const
{
    return strcmp(CStr(), rhs);
}

inline const char* atr_StringBase::CStr() const
{
    return m_StringRep == 0 ? "" : m_Buff;
}

inline int atr_StringBase::Find(const atr_StringBase& str, 
                                   int start) const
{
    return Find(str.CStr(), start);
}


// Replace 'len' chars of this['start'] with all of 's'
inline atr_StringBase& atr_StringBase::Replace(int start, 
                                               int len, const char* s)
{
  size_t sln = strlen(s);
  return replace(start, len, s, sln < INT_MAX ? static_cast<int>(sln) : INT_MAX, 0, atr_StrEnd);
}

inline atr_StringBase& atr_StringBase::Replace(int            start, 
                                               int            len, 
                                               const atr_StringBase& s)
{
    return replace(start, len, s.CStr(), s.Length(), 0, atr_StrEnd);
}


// Replace 'len1' chars at this['start1'] with
// 'len2' chars at 's'['start2']
inline atr_StringBase& atr_StringBase::Replace(int            start1, 
                                               int            len1, 
                                               const atr_StringBase& s,
                                               int            start2, 
                                               int            len2)
{
    return replace(start1, len1, s.CStr(), s.Length(), start2, len2);
}

// Returns index of substring str before or at this[start]
inline int atr_StringBase::RFind(const char * str, 
                                    int       start) const
{
  size_t sln( strlen(str) );
  return rfind(str, sln < INT_MAX ? static_cast<int>(sln) : INT_MAX, start);
}

inline int atr_StringBase::RFind(const atr_StringBase& str, 
                                    int                start) const
{
    return rfind(str.CStr(), str.Length(), start);
}

// Returns (conceptually) RFind(atr_StringBase(s, len), start)
inline int atr_StringBase::RFind(const char* s, 
                                    int start, 
                                    int len) const
{
    return RFind(atr_StringBase(s, len), start);
}

inline int atr_StringBase::AllocatedSize() const
{
    return m_StringRep == 0 ? 0 : m_StringRep->m_AllocSize;
}

inline char*  atr_StringBase::GetBuffPtr()
{
    return m_StringRep == 0 ? 0 : m_Buff;
}


//
// Member Operators
//

atr_StringBase& atr_StringBase::operator = (const atr_StringBase& rhs)
{
    if (rhs.m_StringRep)
        rhs.m_StringRep->Increment();

    if (m_StringRep && m_StringRep->Decrement())
        Delete();

    m_Buff = rhs.m_Buff;
    m_StringRep = rhs.m_StringRep;
    return *this;
}

inline atr_StringBase& atr_StringBase::operator += (const atr_StringBase& str)
{ return Append(str); }

inline atr_StringBase& atr_StringBase::operator += (const char* str)
{ return Append(str); }


//
// Non-member Operators
//

inline atr_StringBase operator + (atr_StringBase lhs, const atr_StringBase& rhs)
{ return lhs.Append(rhs); }

inline atr_StringBase operator + (atr_StringBase lhs, const char* rhs)
{ return lhs.Append(rhs); }


// Operator ==
inline bool operator == (const atr_StringBase& lhs, const atr_StringBase& rhs)
{ return bool(lhs.Compare(rhs) == 0); }

inline bool operator == (const atr_StringBase& lhs, const char* rhs)
{ return bool(lhs.Compare(rhs) == 0); }

inline bool operator == (const char* lhs, const atr_StringBase& rhs)
{ return bool(rhs.Compare(lhs) == 0); }

// Operator !=
inline bool operator != (const atr_StringBase& lhs, const atr_StringBase& rhs)
{ return bool(lhs.Compare(rhs) != 0); }

inline bool operator != (const atr_StringBase& lhs, const char* rhs)
{ return bool(lhs.Compare(rhs) != 0); }

inline bool operator != (const char* lhs, const atr_StringBase& rhs)
{ return bool(rhs.Compare(lhs) != 0); }

// Operator <
inline bool operator < (const atr_StringBase& lhs, const atr_StringBase& rhs)
{ return bool(lhs.Compare(rhs) < 0); }

inline bool operator < (const atr_StringBase& lhs, const char* rhs)
{ return bool(lhs.Compare(rhs) < 0); }

inline bool operator < (const char* lhs, const atr_StringBase& rhs)
{ return bool(rhs.Compare(lhs) > 0); }

// Operator >
inline bool operator > (const atr_StringBase& lhs, const atr_StringBase& rhs)
{ return bool(lhs.Compare(rhs) > 0); }

inline bool operator > (const atr_StringBase& lhs, const char* rhs)
{ return bool(lhs.Compare(rhs) > 0); }

inline bool operator > (const char* lhs, const atr_StringBase& rhs)
{ return bool(rhs.Compare(lhs) < 0); }

// Operator <=
inline bool operator <= (const atr_StringBase& lhs, const atr_StringBase& rhs)
{ return bool(lhs.Compare(rhs) <= 0); }

inline bool operator <= (const atr_StringBase& lhs, const char* rhs)
{ return bool(lhs.Compare(rhs) <= 0); }

inline bool operator <= (const char* lhs, const atr_StringBase& rhs)
{ return bool(rhs.Compare(lhs) >= 0); }

// Operator >=
inline bool operator >= (const atr_StringBase& lhs, const atr_StringBase& rhs)
{ return bool(lhs.Compare(rhs) >= 0); }

inline bool operator >= (const atr_StringBase& lhs, const char* rhs)
{ return bool(lhs.Compare(rhs) >= 0); }

inline bool operator >= (const char* lhs, const atr_StringBase& rhs)
{ return bool(rhs.Compare(lhs) <= 0); }


#endif // atr_StringBase_h
