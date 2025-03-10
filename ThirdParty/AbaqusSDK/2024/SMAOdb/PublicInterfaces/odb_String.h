//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_String_h
#define odb_String_h

// Begin local includes
#include <cow_String.h>

class odb_String : public cow_String
{
public:

    odb_String();
    odb_String(const char* copy_me);

    // copies len bytes of the const char*
    // WARNING! this routine no longer assumes a null-terminated
    // string was passed in. So make sure the length you pass in is
    // CORRECT; that is, it doesn't run past the end of the passed in
    // const char*
    odb_String(const char* copy_me, size_t len);

    // constructs string representations (like sprintf) of the
    // passed in args
    odb_String(int);
    odb_String(unsigned int);
    odb_String(double);

    ~odb_String();


    odb_String(const odb_String& rhs);


    //
    // Methods (In alphabetical order)
    //

    // append 'str' to this
    odb_String&   append(const odb_String& str);

    odb_String& append(const char*);

    // append from 'str' 'len' chars
    // WARNING! This routine does not check the length of the C-string!
    odb_String&   append(const char*, 
                                size_t len);

    // append from 'str'['start'] 'len' chars
    odb_String&   append(const odb_String& str, 
                                size_t start, size_t len);

    // Compatible with strcmp(this, rhs)
    int compare(const odb_String&);
    int compare(const char*);

    // For compatibility with C
    const char * cStr() const;

    bool   empty() const;

    // Remove all chars in string
    odb_String&   erase();

    // Remove 'len' chars beginning at this['start']
    odb_String&   erase(size_t start, 
                        size_t len = atr_StrEnd);

    // Returns index of substring str beginning at this[start]
    // Returns Length() if not found.
    size_t find(const char * str, size_t start = 0) const;

    size_t find(const odb_String& str, 
                             size_t start = 0) const;


    // Returns (conceptually) Find(odb_String(s, len), start)

    //                        size_t len) const;
    size_t find (const char* s, size_t start, 
                              size_t len) const;

    // Insert from 's' 'len' chars at this['start']
    odb_String&  insert (size_t start, const char* s, 
                                size_t len = atr_StrEnd);

    odb_String&  insert (size_t start, const odb_String& s);


    // Insert from 'str'['start2'] 'len' chars at this['start1']

    odb_String&  insert (size_t start1, const odb_String& str,
                                size_t start2, size_t len);


    // Removes any character(s) in "char_set" that appear as a prefix
    // odb_String&  LeftTrim(const char* char_set);
    odb_String&  leftTrim(const char* char_set);

    size_t length() const;

    // Replace 'len' chars of this['start'] with all of 's'
    odb_String& replace(size_t start, size_t len,
                        const char* s);

    odb_String& replace(size_t start, size_t len, 
                               const odb_String& s);

    // Replace 'len1' chars at this['start1'] with
    // 'len2' chars at 's'['start2']

    odb_String& replace(size_t start1, size_t len1, 
                               const odb_String& s,
                               size_t start2, size_t len2);

    odb_String& replace(size_t start, size_t len1, 
                               const char* s,
                               size_t len2);

    // Returns index of substring str before or at this[start]

    size_t rFind(const char * str,
                             size_t start = atr_StrEnd) const;
    
    size_t rFind(const odb_String& str, 
                              size_t start = atr_StrEnd) const;

    // Returns (conceptually) RFind(odb_String(s, len), start)

    size_t rFind(const char* s, size_t start, 
                              size_t len) const;

    // Removes any character(s) in "char_set" that appear as a suffix

    odb_String&   rightTrim(const char* char_set);


    // Returns a copy

    odb_String substr() const;

    // Return a *new* substring given 'start' and 'len'
    odb_String substr(size_t start, size_t len = atr_StrEnd) const;

    // Change the case
    odb_String&   toLower();
    odb_String&   toUpper();

    // Capitalize the first letter
    odb_String&   capitalize();

    // Uncapitalize the first letter
    odb_String&   uncapitalize();

    // Removes any character(s) in "char_set"
    odb_String&   trim(const char* char_set);

    odb_String&   reverse();

    //
    // Member operators
    //

    char operator [](size_t ix) const;

    // will copy string before allowing assignment
    char& operator [](size_t);

    odb_String& operator = (const odb_String& str);

    // This one needs to avoid an allocation, if possible.
           odb_String& operator = (const char* str);

    odb_String& operator += (const odb_String& str);
    odb_String& operator += (const char* str);

    odb_String(const atr_StringBase& rhs);
};


#endif // #ifndef odb_String_h

