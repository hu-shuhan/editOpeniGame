#ifndef CATUnicodeString_H
#define CATUnicodeString_H

// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

#include "DSYString.h"
#include "CATBaseUnknown.h"
#include "CATBSTR.h"

#if 0

/** 
 * Class to use for NLS character strings. 
 * <b>Role</b>:
 * CATUnicodeString (to compare to the @href CATString class) must 
 * be used whenever a character string may contain localized text, 
 * that is either seen, entered, or manipulated by the end user. 
 * Note that file system paths can also contain localized text (for e.g., subdirectories of the user's folder).
 * In this way, the client application is shielded from 
 * implementation details regarding language, locale, bytecoding
 * of characters, and so forth. Client applications thus become 
 * portable since they can handle character strings expressed in 
 * any language.
 * <p>
 * When NLS support is not required, you can use instead the
 * @href CATString class from the System framework. It does support 
 * only ISO 646, so that to supress all the problems that arise 
 * with NLS complexity. You should never use the raw char * type.
 * <p>
 * <b>Note</b>: Among the methods to manipulate instances of this class, you will find below some references to the
 * Unicode standard, to UTF-8, and to UTF-16. Unicode is a standard designed and promoted by the Unicode consortium. 
 * UTF-8 is a transformation format used as a file code set, in particular for CATIA persistent data. 
 * UTF-16 is used in particular by XML processors, in particular by XML parsers. 
 * <p><b>Note</b>: You will also find below some references to the STEP Standard. It is descibed in ISO 10133.
 * <p><b>Note</b>: You may also find below some references to the SBCS acronym. SBCS is for Single Byte Character
 * Set, and MBCS for Multi Byte Character Set. They both correspond to localized character set (specific to a given
 * country langage, at the opposite of Unicode, which describes several country character sets simutaneously). In
 * internationalization history, SBCS and MBCS are anterior to Unicode. For example, the ISO 8859-1 encoding describes
 * several west european languages. It is a SBCS encoding, meaning that a char* in this encoding had to be interpreted
 * at the byte level (each byte corresponding to one and only one character in the Code Page, which could be described 
 * graphically as a 16 per 16 table, ordered trough the 4 first bits horizontally and its 4 last bits vertical, i.e. 
 * hexa code). The IBM-932 Code Page is an example of MBCS encoding, i.e. a  char * having to be interpreted in this
 * Code Page would have to be read the following way: 
 *  . read the first byte, if the dedicated columns (see the
 *    hexa code graphic representation mentionned above),
 *    i.e. the 8,9,E and F column for IBM-932, are set
 *    to zero, the byte will be interpreted alone
 *    (latin or single-width katakana)
 *  . otherwise, i.e. if one of the 8,9,E and F column is set,
 *    the byte, to be interpreted, will have to have the next
 *    byte read (consequently, it makes two bytes together). This
 *    solutions enables to give access to the several thousands
 *    of characters that the IBM-932 describes.
 *  . and so on ...
 * <p><b>CAUTION</b>: The methods manipulating char* should be avoided unless you are manipulating US-ASCII data. Indeed, 
 * unless specified otherwise, the encoding of char* strings is assumed to match environment settings and may depend on the locale.
 * Here are some common uses of these methods:
 * <ul>
 * <li>combined with the use of Unix MOTIF services</li>
 * <li>to display debug traces on the stdout</li>
 * </ul>
 * <p>
 * Suppose, for example, you call the CATUnicodeString constructor from a "const char*" on Windows, and, during
 * execution, the given char* contains one byte, B1 in hexa, followed by the NULL byte which ends the string. Then, the 
 * constructor from a char* will behave the following way:
 * <ul>
 * <li>if the General\"Settings for the current user" field in the "Control Panel"\"Regional Options" window is set to
 * English, the built CATUnicodeString will contain the "PLUS-MINUS SIGN" character</li>
 * <li>if it set on Japanese, the built CATUnicodeString will contain the "HALFWIDTH KATAKANA LETTER A" character</li>
 * <li>if it set on Korean, the built CATUnicodeString will have the following behavior:
 * <ul>
 * <li>a call to @href CATUnicodeString#ConvertToUCChar onto the CATUnicodeString will give one CATUC2Bytes containing the U+003F character 
 * ("QUESTION MARK", which is the default character)</li>
 * <li>a call to @href CATUnicodeString#ConvertToChar onto the CATUnicodeString will give a char* containing one byte, the B1 byte</li>
 * </ul>
 * Actually, the B1 code belongs to the range of two-bytes code representations</li>
 * </ul>
 * The best way to manipulate strings is to use Unicode-compliant encodings (UTF-8, UTF-16/CATUC2Bytes*, wchar_t).<br>
 * <p><b>CAUTION</b>: This class uses the current C locale. As a general rule, do not change the C locale in your applicative code 
 * (using the setlocale sytem call), because it is not thread-safe and any attempt to restore the locale may fail in case of exception.
 * <br>More generally, if your applicative code does change the locale anyway, manage so that, before calling any CAA method, the
 * current locale be the locale which was the current locale when CATIA was run. 
 */
class ExportedByCATSysTS  CATUnicodeString  
{

   public :

    /**
     * Constructs an empty Unicode string.
     */
    CATUnicodeString();

    /** 
     * Copy constructor. 
     * @param iString 
     *   The Unicode string to copy 
     */
    CATUnicodeString(const CATUnicodeString& iString);

  
    /**
     * Constructs a CATUnicodeString instance from a char * null-terminated string (whose encoding is assumed to match environment settings and may depend on the locale).
     * <br><b>CAUTION</b>: if the input string is known to be UTF-8 encoded, use @href #BuildFromUTF8 instead.
     * @param iString
     *   The pointer to the character string
     */
    CATUnicodeString(const char* const iString); 

    /**
     * Constructs a CATUnicodeString instance from a char * string (whose encoding is assumed to match environment settings and may depend on the locale) 
     * and a number of characters.
     * <br><b>CAUTION</b>: if the input string is known to be UTF-8 encoded, use @href #BuildFromUTF8 instead.
     * @param iString
     *   The pointer to the character string
     * @param iLength
     *   The number of characters to be taken into account, starting at 
     *   the first character.
     *   <b>Legal values</b>: Must be less than or equal to the total 
     *   number of characters contained in <tt>iString</tt> 
     */
    CATUnicodeString(const char* const iString, size_t iLength); 
   

    /**
     * Constructs a CATUnicodeString instance by repeating a given 
     * character.
     * @param iChar
     *   The character to be repeated
     * @param iRepeatCount
     *   The number of times <tt>iChar</tt> is to be repeated
     */
    CATUnicodeString(const CATUnicodeChar& iChar,   
                    size_t iRepeatCount=1);
     
    /**
     * Constructs a CATUnicodeString instance by repeating a given 
     * character string.
     * @param iString
     *   The character string to be repeated
     * @param iRepeatCount
     *   The number of times <tt>iString</tt> is to be repeated
     */
    CATUnicodeString(const CATUnicodeString&  iString, 
                    size_t iRepeatCount);

    ~CATUnicodeString() ;
	
	/**
     * Constructs a CATUnicodeString instance from a null-terminated UTF-16 encoded sequence of bytes
     * <p>About the Unicode standard, see above.
     * @param iUnicodeString
     *   The pointer to the UTF-16 encoded character string
     * @return
     *   Boolean specifying the success or not of the action
     *   <br><b>Legal values</b>: <tt>1</tt> if the string has been
     *   successfully converted, or <tt>-1</tt> if a problem 
     *   occured during the conversion.
     */
	int BuildFromUCChar(const CATUC2Bytes *iUnicodeString);

    /**
     * Constructs a CATUnicodeString instance from an UTF-16 encoded sequence of bytes
     * <p>About the Unicode standard, see above.
     * @param iUnicodeString
     *   The pointer to the UTF-16 encoded character string
     * @param iStringLength
     *   The length of <tt>iUnicodeString</tt> in CATUC2Bytes characters
     * @return
     *   Boolean specifying the success or not of the action
     *   <br><b>Legal values</b>: <tt>1</tt> if the string has been
     *   successfully converted, or <tt>-1</tt> if a problem 
     *   occured during the conversion.
     */
    int BuildFromUCChar(const CATUC2Bytes *iUnicodeString, int iStringLength);

    /**
     * Constructs a CATUnicodeString instance from a null-terminated wide-character string.
     * @param iUnicodeString
     *   The wide-character string
     * @return
     *   Integer specifying the success or not of the action
     *   <br><b>Legal values</b>: <tt>0</tt> if the string has been
     *   successfully converted and its length equals to zero,
     *   <tt>1</tt> if the string has been successfully converted
     *   and its length does not equal to zero, or <tt>-1</tt> 
     *   if a problem occured during the conversion.
     */
    int BuildFromWChar(const wchar_t *iUnicodeString);
	
	/**
     * Constructs a CATUnicodeString instance from a wide-character string.
     * @param iUnicodeString
     *   The wide-character string
	 * @param iCharCount
     *   The length of <tt>iUnicodeString</tt> in wchar_t characters
     * @return
     *   Integer specifying the success or not of the action
     *   <br><b>Legal values</b>: <tt>0</tt> if the string has been
     *   successfully converted and its length equals to zero,
     *   <tt>1</tt> if the string has been successfully converted
     *   and its length does not equal to zero, or <tt>-1</tt> 
     *   if a problem occured during the conversion.
     */
    int BuildFromWChar(const wchar_t *iUnicodeString, size_t iCharCount);
    
    /**
     * Converts, formats and stores a specified integer, under control of the format parameter, into the current string.
     * @param iIntegerValue
     *   The integer value
     * @param iCFormat
     *   The format of the integer value, defaulted to "%d", according to the sprintf
     *   library function base conversions
     * @return
     *   Output string length
     *   <br><b>Legal values</b>: <tt>0</tt> if the conversion didn't
     *   succeed, or <tt>Other</tt> if the conversion did succeed,
     *   in which case the value will be the byte count of the
     *   char * equivalent to the result string.
     */
    int BuildFromNum(int iIntegerValue, const char *iCFormat="%d");

    /**
     * Converts, formats and stores a specified unsigned integer, under control of the format parameter, into the current
     * string.
     * @param iUnsIntegerValue
     *   The unsigned integer value
     * @param iCFormat
     *   The format of the unsigned integer value, defaulted to "%u", according 
     *   to the sprintf library function base conversions
     * @return
     *   Output string length
     *   <br><b>Legal values</b>: <tt>0</tt> if the conversion didn't
     *   succeed, or <tt>Other</tt> if the conversion did succeed,
     *   in which case the value will be the byte count of the
     *   char * equivalent to the result string.
     */
    int BuildFromNum(unsigned int iUnsIntegerValue, const char *iCFormat="%u");

    /**
     * Converts, formats and stores a specified long integer, under control of the format parameter, into the current
     * string.
     * @param iLongIntegerValue
     *   The long integer value
     * @param iCFormat
     *   The format of the long integer value, defaulted to "%ld", according to the
     *   sprintf library function base conversions
     * @return
     *   Output string length
     *   <br><b>Legal values</b>: <tt>0</tt> if the conversion didn't
     *   succeed, or <tt>Other</tt> if the conversion did succeed,
     *   in which case the value will be the byte count of the
     *   char * equivalent to the result string. 
     */
    int BuildFromNum(long iLongIntegerValue, const char *iCFormat="%ld");

    /**
     * Converts, formats and stores a specified unsigned long integer, under control of the format parameter, into the
     * current string.
     * @param iUnsLongIntegerValue
     *   The unsigned long integer value
     * @param iCFormat
     *   The format of the unsigned long integer value, defaulted to 
     *   "%lu", according to the sprintf library function base 
     *   conversions
     * @return
     *   Output string length
     *   <br><b>Legal values</b>: <tt>0</tt> if the conversion didn't
     *   succeed, or <tt>Other</tt> if the conversion did succeed,
     *   in which case the value will be the byte count of the
     *   char * equivalent to the result string.
     */
    int BuildFromNum(unsigned long iUnsLongIntegerValue, const char *iCFormat="%lu");

    /**
     * Converts, formats and stores a specified double, under control of the format parameter, into the current string.
     * <br><b>CAUTION</b>: Regarding the decimal symbol, i.e. the character separing the integer whole number portion from
     * its decimal portion, this service consider that it is the one corresponding to the locale. For example, on Windows,
     * if the value for the Numbers\"Decimal symbol" field of the "Control Panel"\"Regional Options" window is equal to
     * the following string:
     * <pre>
     *   {
     * </pre>
     * then, a 1.1 number given to this service will create the following @href CATUnicodeString :
     * <pre>
     *   1{1
     * </pre>
     * @param iDoubleValue
     *   The double value
     * @param iCFormat
     *   The format of the double value, defaulted to "%g", according 
     *   to the sprintf library function base conversions
     * @return
     *   Output string length
     *   <br><b>Legal values</b>: <tt>0</tt> if the conversion didn't
     *   succeed, or <tt>Other</tt> if the conversion did succeed,
     *   in which case the value will be the byte count of the
     *   char * equivalent to the result string.
     */
    int BuildFromNum(double iDoubleValue, const char *iCFormat="%g");

    /**
     * Converts, formats and stores a specified unsigned long integer, under control of the format parameter, into the
     * current string.
     * <br><b>CAUTION</b>: Regarding the decimal symbol, i.e. the character separing the integer whole number portion from
     * its decimal portion, this service consider that it is the one corresponding to the locale, See 
     * @href CATUnicodeString#BuildFromNum , overloading version with a double.
     * @param iLongDoubleValue
     *   The long double value
     * @param iCFormat
     *   The format of the long double value, defaulted to "%Lg", 
     *   according to the sprintf library function base conversions
     * @return
     *   Output string length
     *   <br><b>Legal values</b>: <tt>0</tt> if the conversion didn't
     *   succeed, or <tt>Other</tt> if the conversion did succeed,
     *   in which case the value will be the byte count of the
     *   char * equivalent to the result string.
     */
    int BuildFromNum(long double iLongDoubleValue, const char *iCFormat="%Lg");    

    /**
     * Converts, formats and stores a specified 64-bit signed long integer,
     * under control of the format parameter, into the current string.
     * Defined on 64-bits platforms only.
     * @param iLong64Value
     *   The 64-bit long integer value
     * @param iCFormat
     *   The format of the 64-bit long integer value, defaulted
     *   to "%I64d" on 64-bit Windows, 
     *   according to the sprintf library function base conversions
     * @return
     *   Output string length
     *   <br><b>Legal values</b>: <tt>0</tt> if the conversion didn't
     *   succeed, or <tt>Other</tt> if the conversion did succeed,
     *   in which case the value will be the byte count of the
     *   char * equivalent to the result string.
     */
    int BuildFromNum(CATLONG64  iIntegerValue, const char *iCFormat=FMTLONGI64"d");
    
	/**
     * Converts, formats and stores a specified 64-bit unsigned long integer,
     * under control of the format parameter, into the current string.
     * Defined on 64-bits platforms only.
     * @param iUnsIntegerValue
     *   The 64-bit unsigned long integer value
     * @param iCFormat
     *   The format of the 64-bit unsigned long integer value, defaulted  
     *   to "%I64u" on 64-bit Windows,
     *   according to the sprintf library function base conversions
     * @return
     *   Output string length
     *   <br><b>Legal values</b>: <tt>0</tt> if the conversion didn't
     *   succeed, or <tt>Other</tt> if the conversion did succeed,
     *   in which case the value will be the byte count of the
     *   char * equivalent to the result string.
     */
    int BuildFromNum(CATULONG64 iUnsIntegerValue, const char *iCFormat=FMTLONGI64"u");
    
	/**
     * Converts, formats and stores a specified input stream,
     * into the current string.
     * @param iInputStream
     *   The input stream
     * @param iSkipWhite
     *   The flag to indicate whether white lines must be skipped (!=0) or not (=0)
     * @return
     *   The input stream.
     */
    istream& BuildFromStream( istream& iInputStream, 
                             int iSkipWhite = 1); // White lines skipped if != 0

    /**
     * Converts, formats and stores a specified STEP character string,
     * into the current string.
     * <p>
     * Refer to part 21 of STEP for details about string encoding.
     * @param iStepString
     *   The pointer to the Step-encoded character string
     * @return
     *   boolean specifying the success or not of the action
     *   <br><b>Legal values</b>: <tt>0</tt> if the conversion 
     *   succeeded, or <tt>-1</tt> if the conversion did not succeed.
     */
    int  BuildFromSTEP(const char *iStepString);
  
    /**
	 * Constructs a CATUnicodeString instance from an UTF-8 encoded sequence of bytes (about UTF-8, see above).
     * @param iUTF8Data
     *   The pointer to the UTF-8 encoded character string
     * @param iByteCount
     *   The length of <tt>iUTF8Data</tt> in bytes
     * @return
     *   boolean specifying the success or not of the action
     *   <br><b>Legal values</b>: <tt>0</tt> if the conversion 
     *   succeeded, or <tt>negative value</tt> if the conversion did 
     *   not succeed.
     */
    int  BuildFromUTF8(const char *iUTF8Data, size_t iByteCount);

    /**
	 * Constructs a CATUnicodeString instance from an UTF-16 encoded sequence of bytes
     * <p>About the Unicode standard, see above.
     * @param iUTF16Data
     *   The pointer to the UTF-16 encoded character string
     * @param iUTF16DataLength
     *   The length of <tt>iUTF16Data</tt> in CATUC2Bytes characters
     * @return
     *   boolean specifying the success or not of the action
     *   <br><b>Legal values</b>: <tt>1</tt> if the conversion 
     *   succeeded, or <tt>negative value</tt> if the conversion did 
     *   not succeed.
     */
    int  BuildFromUTF16(const CATUC2Bytes *iUTF16Data, size_t iUTF16DataLength);

    /**
     * Constructs a CATUnicodeString instance from a BSTR-encoded character string.
     * A BSTR is a Basic string, or binary string, i.e. a pointer to a wide
     * character string.
     * @param iBSTR
     *   The pointer to the BSTR-wide character string
     * @return
     *   boolean specifying the success or not of the action
     *   <br><b>Legal values</b>: <tt>0</tt> if the conversion 
     *   succeeded, or <tt>negative value</tt> if the conversion did 
     *   not succeed.
     */
    int  BuildFromBSTR(const CATBSTR &iBSTR);

    // (Re)Initializing an instance (empties it)
    // Deprecated
    /** @nodoc */
    void Reset(); 

    //----------
    // Operators
    //----------

    // Assignment
    /**
     * Assignment operator from a char * string (whose encoding is assumed to match environment settings and may depend on the locale).
	 * <br><b>CAUTION</b>: if the input string is known to be UTF-8 encoded, use @href #BuildFromUTF8 instead.
     * @param iString
     *   The string to be copied
     * @return  
     *   The string resulting from the assignment
     */
    CATUnicodeString &operator =(const char *iString);

    /**
     * Assignment operator from a CATUnicodeString instance.
     * @param iString
     *   The provided string
     * @return  
     *   The string resulting from the assignment
     */
    CATUnicodeString &operator =(const CATUnicodeString &iString);

    // Comparison
    //               CAUTION: the locale is taken into account when comparing.
    //               As a consequence, comparing two instances with different
    //               locales can lead to non-significant results.
    /**
     * Equality operator.
     * @param iString
     *   The string to compare to the current one
     * @return 
     *   boolean
     *   <br><b>Legal values</b>: <tt>false</tt> 
     *   the condition is not fullfilled, or <tt>true</tt> 
     *   if the condition is fullfilled.
     */
    bool operator == ( const CATUnicodeString &iString ) const;
	
    /**
     * Inequality operator.
     * @param iString
     *   The string to compare to the current one
     * @return 
     *   boolean
     *   <br><b>Legal values</b>: <tt>false</tt> 
     *   the condition is not fullfilled, or <tt>true</tt> 
     *   if the condition is fullfilled.
     */
    bool operator != ( const CATUnicodeString &iString ) const;
	
    /**
     * Less-than operator.
     * @param iString
     *   The string to compare to the current one
     * @return 
     *   boolean
     *   <br><b>Legal values</b>: <tt>false</tt> 
     *   the condition is not fullfilled, or <tt>true</tt> 
     *   if the condition is fullfilled.
     */
    bool operator <  ( const CATUnicodeString &iString ) const;

    /**
     * Less-than or equal operator.
     * @param iString
     *   The string to compare to the current one
     * @return 
     *   boolean
     *   <br><b>Legal values</b>: <tt>false</tt> 
     *   the condition is not fullfilled, or <tt>true</tt> 
     *   if the condition is fullfilled.
     */
    bool operator <= ( const CATUnicodeString &iString ) const;

    /**
     * Greater-than or equal operator.
     * @param iString
     *   The string to compare to the current one
     * @return 
     *   boolean
     *   <br><b>Legal values</b>: <tt>false</tt> 
     *   the condition is not fullfilled, or <tt>true</tt> 
     *   if the condition is fullfilled.
     */
    bool operator >= ( const CATUnicodeString &iString ) const;

    /**
     * Greater-than operator.
     * @param iString
     *   The string to compare to the current one
     * @return 
     *   boolean
     *   <br><b>Legal values</b>: <tt>false</tt> 
     *   the condition is not fullfilled, or <tt>true</tt> 
     *   if the condition is fullfilled.
     */
    bool operator >  ( const CATUnicodeString &iString ) const;

    /**
     * Returns the character at the specified index.
     * An index ranges from 0 to the string length - 1. 
     * Bound checks are performed.
     * @param iIndex
     *   The index of the character to extract
     */
    CATUnicodeChar operator[](size_t iIndex) const;
    
    //-----------------
    // Pseudooperators
    //-----------------

    /**
     * Compares CATUnicodeString instances.
     * @return
     *   2 if the strings are identical, 1 if only case differences are
     *   found (<tt>"Hello" == "hello"</tt>), and 0 otherwise.
     */
    int Compare(const CATUnicodeString &iString) const;

    /**
     * Compares CATUnicodeString instances in a non locale specific way.
     * The SortCompare method differs from the operators > (and <) in that the comparison is 
     * not affected by locale, whereas the manner of operators > (and <) comparisons is determined
     * by the LC_COLLATE category of the current locale. 
     * @return
     *   < 0 if the current string is less than iStringToCompare.
     *   0 if the strings are identical.
     *   > 0 if the current string is greater than iStringToCompare.
     */
    int SortCompare(const CATUnicodeString &iStringToCompare) const;

    
    /** @nodoc */
    int SortCompare(const CATUnicodeString &iStringToCompare , int iFlags) const;

    // Concatenation
    /**
     * Concatenates CATUnicodeString instances.
     * The string passed as a parameter is appended to the current string.
     * For example:
     * <pre>
     * CATUnicodeString Hello("Hello");
     * CATUnicodeString Goodbye("Goodbye");
     * Hello.Append(Goodbye);      // Hello now contains "HelloGoodbye"
     * </pre>
     */
    CATUnicodeString& Append( const CATUnicodeString &iString );

    // Capitalization
    /**
     * Converts all of the lower characters in this String to upper 
     * case.
     */
    void ToUpper(); 

    /**
     * Converts all of the upper characters in this String to lower 
     * case.
     */
    void ToLower();

    // Substring extraction
    /**
     * Returns a new string that is a substring of this string.
     * @param iSubStringFirstIndex
     *   The first character to be extracted
     * @param iNbCharsToExtract
     *   The number of characters to be extracted
     */
    CATUnicodeString SubString ( int  iSubStringFirstIndex, int  iSubStringCharCount) const;

   //------------------
   // Format conversion
   //------------------

    /**
     * Read character data, supposed to describe an integer
     * in an alphanumerical form, interpret it according to
     * a format, and store the converted result into the output 
     * parameter.
     * @param oIntegerValue
     *   The resulting integer value
     * @param iCFormat
     *   The format of the integer value, defaulted to "%d", according to the sprintf
     *   library function base conversions
     * @return
     *   Integer value specifying the success or not of the action 
     *   <br><b>Legal values</b>: <tt>1</tt> if the conversion did succeed, 
     *   or another value if it failed.
     */
    int ConvertToNum(int *oIntegerValue, const char *iCFormat="%d") const;

    /**
     * Read character data, supposed to describe an unsigned 
     * integer in an alphanumerical form, interpret it according to
     * a format, and store the converted result into the output 
     * parameter.
     * @param oUnsIntegerValue
     *   The resulting unsigned integer value
     * @param iCFormat
     *   The format of the unsigned integer value, defaulted to "%u", according to the sprintf
     *   library function base conversions
     * @return
     *   Integer value specifying the success or not of the action 
     *   <br><b>Legal values</b>: <tt>1</tt> if the conversion did succeed, 
     *   or another value if it failed.
     */
    int ConvertToNum(unsigned int *oUnsIntegerValue, const char *iCFormat="%u") const;

    /**
     * Read character data, supposed to describe a long integer
     * in an alphanumerical form, interpret it according to
     * a format, and store the converted result into the output 
     * parameter.
     * @param olongIntegerValue
     *   The resulting long integer value
     * @param iCFormat
     *   The format of the long integer value, defaulted to "%ld", according to the sprintf
     *   library function base conversions
     * @return
     *   Integer value specifying the success or not of the action 
     *   <br><b>Legal values</b>: <tt>1</tt> if the conversion did succeed, 
     *   or another value if it failed.
     */
    int ConvertToNum(long *olongIntegerValue, const char *iCFormat="%ld")  const;

    /**
     * Read character data, supposed to describe an unsigned long 
     * integer in an alphanumerical form, interpret it according to
     * a format, and store the converted result into the output 
     * parameter.
     * @param oUnslongIntegerValue
     *   The resulting unsigned long integer value
     * @param iCFormat
     *   The format of the unsigned long integer value, defaulted to "%lu", according to the sprintf
     *   library function base conversions
     * @return
     *   Integer value specifying the success or not of the action 
     *   <br><b>Legal values</b>: <tt>1</tt> if the conversion did succeed, 
     *   or another value if it failed.
     */
    int ConvertToNum(unsigned long *oUnslongIntegerValue, const char *iCFormat="%lu") const;

    /**
     * Read character data, supposed to describe a 64-bit long integer
     * in an alphanumerical form, interpret it according to
     * a format, and store the converted result into the output 
     * parameter.
     * @param olongIntegerValue
     *   The resulting 64-bit long integer value
     * @param iCFormat
     *   The format of the 64-bit long integer value, defaulted to "%I64d" on 64-bit Windows, 
     *   according to the sprintf library function base conversions
     * @return
     *   Integer value specifying the success or not of the action 
     *   <br><b>Legal values</b>: <tt>1</tt> if the conversion did succeed, 
     *   or another value if it failed.
     */
    int ConvertToNum(CATLONG64 *olongIntegerValue, const char *iCFormat=FMTLONGI64"d") const;

    /**
     * Read character data, supposed to describe a 64-bit unsigned long 
     * integer in an alphanumerical form, interpret it according to
     * a format, and store the converted result into the output 
     * parameter.
     * @param oUnslongIntegerValue
     *   The resulting 64-bit unsigned long integer value
     * @param iCFormat
     *   The format of the 64-bit unsigned long integer value, defaulted to "%I64u" on 64-bit Windows
     *   according to the sprintf library function base conversions
     * @return
     *   Integer value specifying the success or not of the action 
     *   <br><b>Legal values</b>: <tt>1</tt> if the conversion did succeed, 
     *   or another value if it failed.
     */
    int ConvertToNum(CATULONG64 *oUnslongIntegerValue, const char *iCFormat=FMTLONGI64"u") const;

    /**
     * Read character data, supposed to describe an double in an 
     * alphanumerical form, interpret it according to
     * a format, and store the converted result into the output 
     * parameter.
     * <br><b>CAUTION</b>: Regarding the decimal symbol, i.e. the character separing the integer whole number portion from
     * its decimal portion, this service consider that it is the one corresponding to the locale. For example, on Windows,
     * if the value for the Numbers\"Decimal symbol" field of the "Control Panel"\"Regional Options" window is equal to
     * the following string:
     * <pre>
     *   {
     * </pre>
     * then, so that the current method give the correct value, a 1.1 number must be stored the following way in the
     * instance:
     * <pre>
     *   1{1
     * </pre>
     * @param oDouble
     *   The resulting double value
     * @param iCFormat
     *   The format of the double value, defaulted to "%le", according to the sprintf
     *   library function base conversions
     * @return
     *   Integer value specifying the success or not of the action 
     *   <br><b>Legal values</b>: <tt>1</tt> if the conversion did succeed, 
     *   or another value if it failed.
     */
    int ConvertToNum(double *oDouble, const char *iCFormat="%le") const;

    /**
     * Read character data, supposed to describe an long double in an alphanumerical form, interpret it according to
     * a format, and store the converted result into the output parameter.
     * <br><b>CAUTION</b>: Regarding the decimal symbol, i.e. the character separing the integer whole number portion from
     * its decimal portion, this service consider that it is the one corresponding to the locale, See 
     * @href CATUnicodeString#ConvertToNum , overloading version with a double.
     * @param oLongDouble
     *   The resulting long double value
     * @param iCFormat
     *   The format of the long double value, defaulted to "%Le", according to the sprintf
     *   library function base conversions
     * @return
     *   Integer value specifying the success or not of the action 
     *   <br><b>Legal values</b>: <tt>1</tt> if the conversion did succeed, 
     *   or another value if it failed.
     */
    int ConvertToNum(long double *oLongDouble, const char *iCFormat="%Le") const;
 
    /**
     * Convert the current string into a STEP character string.
     * @param oStepString
     *   The resulting STEP string
     */
    void ConvertToSTEP( char *oStepString) const;

    /**
	 * Convert the current string into a UTF-8 character string.
	 * For example:
     * <pre>
     * size_t length = 0;
	 * cus.ConvertToUTF8(nullptr, &length);
	 * char *pBuffer = new char[length + 1];	// Or use malloc (see ConvertToUTF16 example), a C++ dynamic array...
	 * cus.ConvertToUTF8(pBuffer, &length);
	 * pBuffer[length] = '\0';
	 * // ... Use data ...
	 * delete[] pBuffer;
     * </pre>
	 * @param oUTF8String
	 *   If set to nullptr, ConvertToUTF8 will return the size required to store the string in oByteCount (not including space for a terminal NULL character).
	 *   If oUTF8String is not null, ConvertToUTF8 will store the string in the buffer and return its length in oByteCount.
	 * @param oByteCount
	 *   Depending on the context, either set to the size required to store the string or to the actual string length (in bytes).
	 */
    void ConvertToUTF8( char *oUTF8String, size_t *oByteCount) const; 


    /**
	 * Convert the current string into a UTF-16 character string.
	 * For example:
     * <pre>
     * size_t length = 0;
	 * cus.ConvertToUTF16(nullptr, &length);
	 * CATUC2Bytes *pBuffer = (CATUC2Bytes *)malloc((length + 1) * sizeof(CATUC2Bytes));	// Or use new[] (see ConvertToUTF8 example), a C++ dynamic array...
	 * cus.ConvertToUTF16(pBuffer, &length);
	 * pBuffer[length] = '\0';
	 * // ... Use data ...
	 * free(pBuffer);
     * </pre>
	 * @param oUTF16String
	 *   If set to nullptr, ConvertToUTF16 will return the size required to store the string in oUCCharCount (not including space for a terminal NULL character)
	 *   If oUTF16String is not null, ConvertToUTF16 will store the string in the buffer and return its length in oUCCharCount.
	 * @param oUCCharCount
	 *   Depending on the context, either set to the size required to store the string or to the actual string length (in CATUC2Bytes count).
	 */
    void ConvertToUTF16(CATUC2Bytes *oUTF16String, size_t *oUCCharCount) const;
  
    /**
     * Convert the current string into a BSTR (OLE basic string).
     * @param oBSTR
     *   The resulting BSTR string
     */
    void ConvertToBSTR( CATBSTR *oBSTR) const;
	
    /**
     * Allows to get a char * representation of the string.</br>
     * <b>Role</b>: Converts the current string into a char* string using an environment dependent encoding (which may depend on the locale or be UTF-8).
     * <p><b>Note</b>: The length of the returned string is given by @href #GetLengthInByte.
  	 * <br><b>CAUTION</b>: if the string contains Unicode characters that doesn't exist in the destination charset, 
	 * they will be replaced in the returned C string, usually by the question mark character (U+003F).
     * <br><b>CAUTION</b>: The best way to manipulate strings is to use the Unicode encoding. 
	 * Using the following methods is safer, because the data returned will not depend on the environment: 
	 * <ul>
     * <li>@href #ConvertToUTF8 </li>
     * <li>
     * <pre>operator const CATUC2Bytes*() const;</pre>
     * </li>
     * </ul>
	 * In fact, CATIA uses the current method for the rare conversions from Unicode to the environment code page, i.e. :
     * <ul>
     * <li>the display of message strings in dialog windows on Unix</li>
     * <li>output traces</li>
     * </ul>
     * @return 
     *   The char* string
     */
    const char* ConvertToChar() const;

    // Deprecated since the implicit use of the cast is too dangerous
    // (very often the users don't know how to use it)
    /** @nodoc */
    operator  const char *  () const;
	
    /** @nodoc */
    const CATUC2Bytes *ConvertToUCChar() const;
	
    /**
     * Converts the current string to a CATUC2Bytes character encoded 
     * string.
     * The length of the returned string is given by @href #GetLengthInChar.
     * @return
     *   The resulting CATUC2Bytes string
     */
    operator  const CATUC2Bytes *  () const;
	
    /**
	 * Converts the current string to a wchar_t character encoded null-terminated string.
	 * For example:
     * <pre>
     * size_t length = cus.ConvertToWChar(nullptr);
	 * wchar_t *pBuffer = (wchar_t *)malloc(length * sizeof(wchar_t));	// Or use new[] (see ConvertToUTF8 example), a C++ dynamic array...
	 * length = cus.ConvertToWChar(pBuffer);
	 * // ... Use data ...
	 * free(pBuffer);
     * </pre>
	 * @param oWString
	 *   If set to nullptr, ConvertToWChar will return the size required to store the string in wchar_t count (including space for a terminal NULL character).
	 *   If oWString is not null, ConvertToWChar will store the string in the buffer and return its length.
	 * @return
	 *   Depending on the context, either the size required to store the string or the actual string length (in wchar_t count).
	 */
    size_t ConvertToWChar ( wchar_t *oString ) const;
 
    //--------------------
    // String manipulation
    //--------------------

    /**
     * Extends or truncates the current string.
     * @param iCharCountToReach
     *   The number of characters to be assigned to the current string
     * @param iFillingChar
     *   The character that will be added at the end of the
     *   current string so that to map the required length
     * @param iTruncationAllowed
     *   The flag allowing the truncation or refusing it.
     *   <br><b>Legal values</b>: <tt>0</tt> to forbid truncation,
     *   or <tt>Other</tt> to allow the possible truncation.
     */
    void        Resize(int                   iCharCountToReach,
                       const CATUnicodeChar& iFillingChar, 
                       int                   iTruncationAllowed = 1);

    /**
     * Extends or truncates the current string.
     * Same as the preceeding version, except that the filling
     * character is forced to be set to the space character and truncation is allowed.
     * @param iCharCountToReach
     *   The number of characters to be assigned to the current string
     */
    void Resize(int iCharCountToReach);
     
    /**
     * Replace the first occurence of the given substring by the
     * other given substring.
     * @param iLookedForString
     *   The substring whose first occurence must be replaced
     * @param iSubstitutionString
     *   The string to substitute to the previous one
     * @return 
     *   Location of the input substring occurence.
     *   <br><b>Legal values</b>: 
     *   <tt>-1: Not found</tt> 
     *   <tt>Other: Location index, from 0 to the current CATString
     *   length minus 1</tt> 
     */  
    int  ReplaceSubString(const CATUnicodeString &iLookedForString, 
                    const CATUnicodeString &iSubstitutionString);

    /**
     * Modify the current CATUnicodeString, supressing a given part of it
     * and putting instead a given replacement substring.
     * @param iModificationStartingPosition
     *   The modification starting position
     * @param iModificationCharCount
     *   The modification char count
     * @param iReplacementString
     *   The replacement string
     */  
    void ReplaceSubString(int   iModificationStartingPosition,
                      int   iModificationCharCount,
                      const CATUnicodeString &iReplacementString);

    /**
     * Modify the current CATUnicodeString, supressing a given part of it
     * and putting instead a given replacement substring.
     * @param iModificationStartingPosition
     *   The modification starting position
     * @param iModificationCharCount
     *   The modification char count
     * @param iReplacementString
     *   The replacement string
     * @param iReplacementStringParametering
     *   The replacement string parametering.
     *   Enables to specify not to take the replacement string as is, 
     *   but transformed first by a resizing action. 
     *   This tuning specification is the character count of the
     *   real CATUnicodeString that will be used for the substitution, 
     *   i.e. the CATUnicodeString resized through the space filling
     *   character. 
     */  
    void ReplaceSubString(int   iModificationStartingPosition,
                   int   iModificationCharCount,
                   const CATUnicodeString &iReplacementString, 
                   int   iReplacementStringParametering) ; 

    /**
     * Modify the current CATUnicodeString, replacing all the occurences of the specified substring
     * with another substring.
     * @param iToReplace
     * The substring to replace
     * @param iReplacement
     * The replacement substring
     */
     void ReplaceAll(
       const CATUnicodeString& iToReplace,
       const CATUnicodeString& iReplacement);

    /**
     * Modify the current CATUnicodeString, supressing a given part 
     * of it.
     * @param iModificationStartingPosition
     *   The modification starting position
     * @param iModificationCharCount
     *   The modification char count
     */    
    void Remove(int iModificationStartingPosition, 
               int iModificationCharCount=1) ;

    /**
     * Modify the current CATUnicodeString, inserting a given 
     * substring at the middle of it.
     * @param iInsertionStartingPosition
     *   The insertion starting position
     * @param iStringToInsert
     *   The string to insert into the current CATString
     */   
    void Insert(int iInsertionStartingPosition, 
               const CATUnicodeString &iStringToInsert) ;

   //-----------------
   // String stripping
   //-----------------

    /** 
     * Strip mode.
     * <b>Role</b>: Stripping is the process consisting
     * of removing a given character occurences from a given
     * string. Once the character specified, you can, as you want, 
     * use one of the following options:
     * @param CATStripModeLeading
     *   Remove all the consecutive occurences of the  
     *   character from the beginning of the string 
     *   (of course it supposes that the first character of the 
     *   string is the specified character, if it is not the case, 
     *   nothing is done).
     * @param CATStripModeTrailing
     *   Remove all the consecutive occurences from the end
     *   of the string (It means that the first 
     *   character of the string is really the specified 
     *   character).
     * @param CATStripModeBoth
     *   Remove all the consecutive occurences both from the 
     *   beginning of the string and from the end (this option cumulates
     *   the two preceeding options, it does not affects the 
     *   intermediate consecutive occurences).
     * @param CATStripModeAll
     *   Removes all the occurences of the character
     *   from the string. 
     * <p>
     * Warning : CATStripMode is also defined in CATInternalString.cpp
     */
    enum CATStripMode {CATStripModeLeading = 0x1, 
                      CATStripModeTrailing = 0x2, 
                      CATStripModeBoth =     0x3, 
                      CATStripModeAll =      0x4};

    /** 
     * Returns a new stripped CATUnicodeString.
     * <b>Role</b>: Removes the specified character occurences from a 
     * given string. Note that this method returns a new stripped string but does
     * not modify the current CATUnicodeString.
     * @param iMode
     *   Option parameterizing the action.
     *   <br><b>Legal values</b>: 
     *   <tt>CATStripModeLeading</tt> Removes all the consecutive 
     *   occurences of the given character from the beginning of the 
     *   given string (of course it supposes that the first character 
     *   of the string is the delivered character, if it is not the 
     *   case, nothing is done).
     *   <tt>CATStripModeTrailing</tt> Removes all the consecutive 
     *   occurences of the given character from the end of the string 
     *   (the same way, it means that the first character of the 
     *   delivered string is really the delivered character).
     *   <tt>CATStripModeBoth</tt> Removes all the consecutive 
     *   occurences of the given character both from the beginning 
     *   of the string and from the end (this option cumulates the 
     *   two preceeding options, it does not affects the intermediate 
     *   consecutive occurences).
     *   <tt>CATStripModeAll</tt> Removes all the occurences of the 
     *   given character from the string. 
     * <p>
     * @param iCharacter
     *   The specified character whose occurences are to be removed
     *   from the current CATUnicodeString.
     */
    CATUnicodeString Strip(CATUnicodeString::CATStripMode iMode, 
                           const CATUnicodeChar& iCharacter) const;
 
    /** 
     * Returns a new stripped CATUnicodeString.
     * <b>Role</b>: Removes the space character occurences from a 
     * given string. Note that this method returns a new stripped string but does
     * not modify the current CATUnicodeString.
     * @param iMode
     *   See above
     */
    CATUnicodeString Strip(CATUnicodeString::CATStripMode iMode = CATStripModeLeading) const;

   //----------------
   // String analysis
   //----------------

    /** 
     * Get the length of the CATUnicodeString in CATUC2Bytes characters 
	 * (potentially different from the length of the string in Unicode characters, if it contains characters out of the Unicode BMP).
     * <b>Role</b>: Get the length in CATUC2Bytes characters count.
     * This method returns the length of the string returned by @href #ConvertToUCChar.
	 * <p><b>Note</b>: don't call this method to check for emptiness ( use @href #IsEmpty ) or 
     * when converting the string using either @href #ConvertToUTF8, @href #ConvertToUTF16 or @href #ConvertToWChar (see conversion method documentation).
     * @return 
     *   Character count.
     */
    int GetLengthInChar() const;

    /** 
     * Get the length of the CATUnicodeString, as byte count.
     * <b>Role</b>: Get the length as byte count.
     * This method returns the length of the string returned by @href #ConvertToChar.
	 * <p><b>Note</b>: don't call this method to check for emptiness ( use @href #IsEmpty ).
     * @return 
     *   Byte count.
     */
    int GetLengthInByte() const;
   
    /** 
     * Search mode.
     * <b>Role</b>: The search mode is the direction to which
     * you search the specified substring in the current string.
     * @param CATSearchModeForward
     *   Starting from the specified current string character index,
     *   the search will go forward to search the specified substring.
     * @param CATSearchModeBackward
     *   Starting from the specified current string character index,
     *   the search will go backward to search the specified 
     *   substring.
     */
    enum CATSearchMode {CATSearchModeForward  = 0x1,  
                       CATSearchModeBackward = 0x2}; 

    /**
     * Search the first occurence of the specified substring in the 
     * current string.
     * <b>Role</b>: Search the first occurence of the specified 
     * substring in the current string, from a specified character 
     * index, the search following the specified direction.
     * @param iLookedForSubString
     *   The character string to search for
     * @param iSearchBeginning
     *   The character index localizing the search beginning
     * @param iSearchMode
     *   Direction to which you search the specified substring in the 
     *   current string.
     *   <br><b>Legal values</b>: <tt>CATSearchModeForward</tt> if, 
     *   starting from the specified current string character index,
     *   the search will go forward to search the specified substring,
     *   or <tt>CATSearchModeBackward</tt> in the other direction.
     *   Note that in CATSearchModeBackward mode, the <tt>
     *   iSearchBeginning</tt> index corresponds to a count of characters
     *   from the end of the string, not the beginning. 
     *   <br>For instance:
     * <pre>
     * CATUnicodeString s = "012345678901test6789";
     * s.SearchSubString("test", 4, CATUnicodeString::CATSearchModeBackward)) returns 12
     * </pre>
     * whereas
     * <pre>
     * s.SearchSubString("test", 5, CATUnicodeString::CATSearchModeBackward)) returns -1
     * </pre>
     * @return 
     *   Location of the substring.
     *   <br><b>Legal values</b>: 
     *   <tt>-1: Not found</tt> 
     *   <tt>Other: Location index, from 0 to the current CATString
     *   length minus 1</tt> 
     * 
     */
    int SearchSubString(const CATUnicodeString &iLookedForSubString , 
          int iSearchBeginning = 0,
      CATUnicodeString::CATSearchMode iSearchMode = 
          CATSearchModeForward) const ;

   //----------------------
   // Hash Table management
   //----------------------

    //  Returns a hashcode for this string.
    //  There is no garantee about unicity.
    /** @nodoc */
   unsigned ComputeHashKey() const; // The returned value can be used as an entry 
                                    // in hash tables provided by Collections
                                    // framework

    // Deprecated because of homogenization with the collection
    // terminology. The Append method should be used.
    /** @nodoc */
    CATUnicodeString & operator += ( 
          const CATUnicodeString & iString );

    /**
     * Character concatenation assignment operator.
     * @param iChar
     *   The character to append to the current string
     * @return 
     *   The class resulting from the concatenation
     */
    CATUnicodeString & operator += ( 
          const CATUnicodeChar & iChar );

    // Deprecated because of homogenization with the collections
    // terminology. The Append method should be used.
    /** @nodoc */
    CATUnicodeString operator + (
          const CATUnicodeString &iString) const ;

    /**
     * String concatenation operator.
	 * <br><b>CAUTION</b>: if iString is known to be UTF-8 encoded, build a temporary CATUnicodeString first using @href #BuildFromUTF8, 
	 * then call the @href CATUnicodeString#Append method.
     * @param iString
     *   The string to append to the current one
     * @return 
     *   The class resulting from the concatenation
     */
    CATUnicodeString operator + (
          const char *iString) const;

    /**
     * String concatenation operator.
	 * <br><b>CAUTION</b>: if iCString is known to be UTF-8 encoded, build a CATUnicodeString first using @href #BuildFromUTF8, 
	 * then call the @href CATUnicodeString#Append method.
     * @param iCString
     *   The char *
     * @param iString
     *   The CATUnicodeString string
     * @return 
     *   The CATUnicodeString resulting from the concatenation
     */
    ExportedByCATSysTS friend CATUnicodeString operator + (
          const char* iCString, const CATUnicodeString& iString);

    /** @nodoc */
    CATBoolean WildMatch(
                const CATUnicodeString& iPattern, 
        const CATUnicodeChar& iWildCardChar,
        const CATUnicodeChar& iEscapeChar);

    //  Deprecated since the implicit use of the cast is too dangerous
    //  (very often the users don't know how to use it)
    //  Same as ConvertToChar
    /** @nodoc */
    const char  *  CastToCharPtr() const;
	
	/**
     * Test wether the string is empty or not.
	 * <p><b>Note</b>: don't call @href #GetLengthInChar or @href #GetLengthInByte for this purpose because it is inefficient.
     * @return 
     *   boolean
	 *   <br><b>Legal values</b>: <tt>true</tt> if the string is empty, 
     *   or <tt>false</tt> otherwise.
     */
    bool IsEmpty() const;
	
	/**
     * Test wether the string is empty or not.
	 * @href #IsEmpty
	 */
	bool IsNull() const { return IsEmpty(); }

    // Deprecated since the SearchSubString can be used
    /** @nodoc */
    int FindPosition(const CATUnicodeString &iLookedForSubString, 
                     int iSearchBeginning = 0) const ;

    //  Deprecated since the homogenization to the "Build" terminology
    /** @nodoc */
    int ConvertNumToString( int iIntegerValue, const char *iCFormat="%d" );

    //  Deprecated since the homogenization to the "Build" terminology
    /** @nodoc */
    int ConvertNumToString( long iLongIntegerValue, 
                            const char *iCFormat="%ld" );

    //  Deprecated since the homogenization to the "Build" terminology
    /** @nodoc */
    int ConvertNumToString( unsigned long iUnsLongIntegerValue, 
                            const char *iCFormat="%lu" );
    //  Deprecated since the homogenization to the "Build" terminology
    /** @nodoc */
    int ConvertNumToString( double iDoubleValue, 
                            const char *iCFormat="%g" );
    //  Deprecated since the homogenization to the "Build" terminology
    /** @nodoc */
    int ConvertNumToString( long double iLongDoubleValue, 
                            const char *iCFormat="%Lg");
    // Deprecated 
    /** @nodoc */
     bool operator == ( const char *iString ) const ;
    // Deprecated 
    /** @nodoc */
     bool operator != ( const char *iString ) const ;
  
    /** @nodoc */ 
    static const CLSID & ClassId();    
  
private:
    /** @nodoc */
    union
    {
        /*
         * @nodoc
         * _Str & _WStr are defined for debugging purposes.
         */
        /** @nodoc */
        char *_Str;
        /** @nodoc To enhance debug experience */
        char16_t   *_u16Str;
        /** @nodoc */
        CATULONGPTR _StringRef;
    };
    
    
    void _AllocNewRefString(
                int iCharLength, 
                CATBoolean iMakeCopy = FALSE, 
                CATBoolean iMakeDeepCopy = FALSE,
                CATBoolean iComputeBestLength = FALSE);
    
    void _UpdateCharView();
    
    void _BuildFromChar(const char* ipsz, int iLength = -1);
};

#endif

#endif  // CATUnicodeString_H
