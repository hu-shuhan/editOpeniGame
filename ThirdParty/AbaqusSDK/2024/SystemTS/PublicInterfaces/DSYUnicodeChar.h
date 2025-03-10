#ifndef DSYUnicodeChar_H
#define DSYUnicodeChar_H    42500
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

// COPYRIGHT DASSAULT SYSTEMES 2014

#include "CATSysTS.h"
#include "CATI18NTypes.h"
#include <string.h>

/**
 * Unicode character support. 
 * <b>Role</b>: Unicode character class. Use CATUnicodeChar whenever you need to manipulate a single character, for
 * example when using a CATUnicodeString.

 */
/** 
 * Class to use for language meaningful character. 
 * <b>Role</b>:
 * CATUnicodeChar can be used whenever a character is needed, and that is either seen, entered, or manipulated by the 
 * end user. In this way, the client application is shielded from implementation details regarding language, locale,
 * bytecoding of characters, and so forth. Client applications thus become portable since they can handle character
 * strings expressed in any language.
 * <p>
 * When NLS support is not required, you can use instead the @href CATChar class from the System framework. It does
 * support only ISO 646, so that to supress all the problems that arise with NLS complexity. You should never use the 
 * raw char type.
 * <p>
 * <b>Note</b>: Among the methods to manipulate instances of this class, you will find below some references to the
 * Unicode and UTF-8 standards. See @href CATUnicodeString for precisions.
 * <p><b>Note</b>: You will find also below some references to the SBCS and MBCS acronym, see @href CATUnicodeString
 * for precisions.
 * <p><b>CAUTION</b>: The methods manipulating the "char" type of the C language (or "char*", see 
 * @href CATUnicodeChar#BuildFromChar ) should rarely be used. They may be used:
 * <ul>
 * <li>combined with the use of Unix MOTIF services</li>
 * <li>to display debug traces on the stdout</li>
 * </ul>
 * The best way to manipulate strings is to use the Unicode encoding, the "char" type of the C language has many
 * limitations.
 * <br>Suppose, for example, you call @href CATUnicodeChar#BuildFromChar on Windows, and, during the execution, the
 * given "char*" contains one byte, B1 in hexa, followed by the NULL byte which ends the string. Then, 
 * @href CATUnicodeChar#BuildFromChar will behave the following way:
 * <ul>
 * <li>if the General\"Settings for the current user" field in the "Control Panel"\"Regional Options" window is set to
 * English, the built CATUnicodeChar will contain the "PLUS-MINUS SIGN" character</li>
 * <li>if it set on Japanese, the built CATUnicodeChar will contain the "HALFWIDTH KATAKANA LETTER A" character:
 * the B1 code belongs to the range of one-byte code representations</li>
 * <li>if it set on Korean, the built CATUnicodeChar will contain the "QUESTION MARK" character, which is the default
 * character. Actually, the B1 code belongs to the range of two-bytes code representations</li>
 * </ul>
 */
class ExportedByCATSysTS CATUnicodeChar
{

public:

  // Constructors
  // ============

/**
 * Constructs an empty Unicode character instance.
 */
  CATUnicodeChar();

/**
 * Constructs a CATUnicodeChar instance from a Unicode code point.
 * @param iUChar32 UTF-32 encoded character
 */
  static CATUnicodeChar BuildFromChar32(const char32_t iUChar32);
  
  /** @nodoc */
  static CATUnicodeChar BuildFromUCChar32(const CATUC4Bytes iUChar32);
  
/**
 * Constructs a CATUnicodeChar instance from a byte.
 * <br><b>Role</b>: Constructs a CATUnicodeChar instance from a byte, supposing the thread code page is a 
 * SBCS code page. If, in the thread code page, the given byte does not correspond to any character coded
 * representation, the instance will be filled with the default character (QUESTION MARK).
 * <br><b>CAUTION</b>: This method, which manipulates the "char" type of the C language should rarely be used (see
 * above).
 * @param iByte 
 *   The byte, corresponding to a coded representation in the thread code page.
 */
  CATUnicodeChar(const char iByte);

/** 
 * Copy constructor. 
 * @param iChar 
 *   The Unicode character to copy 
 */
  CATUnicodeChar(const CATUnicodeChar& iChar);

  // Destructor
  // ==========
  ~CATUnicodeChar();


  // Pseudoconstructors
  // ==================
/**
 * Builds a CATUnicodeChar instance from a character in a localized encoding.
 * <br><b>Role</b>: Builds a CATUnicodeChar instance from a character in a localized encoding. The character code page,
 * which must be equal to the thread code page, may be either a SBCS or a MBCS code page. If, in the thread code page,
 * the given byte(s) do(es) not correspond to any character coded representation, the instance will be filled with the 
 * default character (QUESTION MARK).
 * <br><b>CAUTION</b>: This method, which manipulates the "char*" type of the C language should rarely be used (see
 * above).
 * @param iCodedRepresentation 
 *   Character in the thread locale localized encoding.
 * @param iMaxBytes 
 *   Maximum number of bytes of iCodedRepresentation to consider to decode the character.
 */
  int BuildFromChar(const char * iCodedRepresentation, size_t iMaxBytes = (size_t)-1);


  // Operators :
  //============

  //--- Assignment


/**
 * Assignment operator from a byte.
 * <br><b>Role</b>: This operator assigns a byte to the current instance. This operator supposes:
 * <ul>
 * <li>the thread code page is a SBCS code page</li>
 * <li>iByte corresponds to a coded representation in the thread code page</li>
 * </ul>
 * If, in the thread code page,the given byte does not correspond to any character coded representation, the instance
 * will be filled with the default character (QUESTION MARK).
 * <br><b>CAUTION</b>: This method, which manipulates the "char" type of the C language should rarely be used (see
 * above).
 * @param iByte
 *   The byte, corresponding to a coded representation in the thread code page.
 */
  CATUnicodeChar&	operator=( const char iByte);
/**
 * Assignment operator from a MBCS char.
 */
/**
 * Assignment operator from a character in a localized encoding.
 * <br><b>Role</b>: Assigns a coded representation, supposed to belong to the thread code page, to the current instance.
 * If, in the thread code page, the given byte(s) do(es) not correspond to any character coded representation, the
 * instance will be filled with the default character (QUESTION MARK).
 * <br><b>CAUTION</b>: This method, which manipulates the "char*" type of the C language should rarely be used (see
 * above).
 * @param iCodedRepresentation
 *   Character in the thread locale localized encoding.
 */
  CATUnicodeChar&	operator=( const char * iCodedRepresentation);	

/**
 * Assignment operator from a CATUnicodeChar instance.
 * @param iCharacter
 *   The provided character
 */
  CATUnicodeChar&	operator=( const CATUnicodeChar & iCharacter);


  //--- Comparison


/**
 * Equality operator.
 * @param iCharacter
 *   The character to compare to the current one
 * @return 
 *   boolean
 *   <br><b>Legal values</b>: <tt>false</tt> 
 *   the condition is not fullfilled, or <tt>true</tt> 
 *   if the condition is fullfilled.
 */
  bool operator == ( const CATUnicodeChar & iCharacter) const;

/**
 * Inequality operator.
 * @param iCharacter
 *   The character to compare to the current one
 * @return 
 *   boolean
 *   <br><b>Legal values</b>: <tt>false</tt> 
 *   the condition is not fullfilled, or <tt>true</tt> 
 *   if the condition is fullfilled.
 */
  bool operator != ( const CATUnicodeChar & iCharacter) const;
/**
 * Less-than operator.
 * For the comparison, the current locale is taken into account.
 * This service calls the C strcoll service, which, for
 * the comparison, compares collation weights computed for
 * each character.
 * @param iCharacter
 *   The character to compare to the current one
 * @return 
 *   boolean
 *   <br><b>Legal values</b>: <tt>false</tt> 
 *   the condition is not fullfilled, or <tt>true</tt> 
 *   if the condition is fullfilled.
 */
  bool operator <  ( const CATUnicodeChar & iCharacter) const;
/**
 * Less-than or equal operator.
 * For the comparison, the current locale is taken into account.
 * @param iCharacter
 *   The character to compare to the current one
 * @return 
 *   boolean
 *   <br><b>Legal values</b>: <tt>false</tt> 
 *   the condition is not fullfilled, or <tt>true</tt> 
 *   if the condition is fullfilled.
 */
  bool operator <= ( const CATUnicodeChar & iCharacter) const;
/**
 * Greater-than operator.
 * For the comparison, the current locale is taken into account.
 * @param iCharacter
 *   The character to compare to the current one
 * @return 
 *   boolean
 *   <br><b>Legal values</b>: <tt>false</tt> 
 *   the condition is not fullfilled, or <tt>true</tt> 
 *   if the condition is fullfilled.
 */
  bool operator >  ( const CATUnicodeChar & iCharacter) const;
/**
 * Greater-than or equal operator.
 * For the comparison, the current locale is taken into account.
 * @param iCharacter
 *   The character to compare to the current one
 * @return 
 *   boolean
 *   <br><b>Legal values</b>: <tt>false</tt> 
 *   the condition is not fullfilled, or <tt>true</tt> 
 *   if the condition is fullfilled.
 */
  bool operator >= ( const CATUnicodeChar & iCharacter) const;
 
/**
 * Cast to char.
 * @return 
 *   The character
 * @deprecated R423 rather use @href #ConvertToChar32 instead to be able to retrieve all Unicode characters.
 */ 
  operator  char() const;


  // Methods :
  //==========
  
/**
 * Get the current Unicode character in UTF-32 encoding.
 * @return 
 *   The CATUC2Bytes character.
 */
  char32_t ConvertToChar32() const;
  
  /** @nodoc */
  CATUC4Bytes ConvertToUCChar32() const;
  
/**
 * Converts the current character into a CATUC2Bytes.
 * @return 
 *   The CATUC2Bytes character.
 * @deprecated R423 rather use @href #ConvertToChar32 instead to be able to retrieve all Unicode characters.
 * (including the ones that are not part of the Unicode BMP).
 */
  CATUC2Bytes ConvertToUC2Bytes() const;
  
/**
 * Converts the current character into a character in the thread locale localized encoding.
 * <br><b>Role</b>: Converts the current character into a character in the thread locale localized encoding. 
 * The character must belong the character set of the thread locale code page. If the character does not belong to the
 * character set of the thread locale code page, the default character coded representation is returned.
 * <br><b>CAUTION</b>: This method, which manipulates the "char*" type of the C language should rarely be used (see
 * above).
 * @return 
 *   Character in the thread locale localized encoding.
 */
  const char *ConvertToChar() const;
  
/**
 * Builds a character from a CATUC2Bytes.
 * @param iCharacter
 *   The CATUC2Bytes character.
 * @deprecated R423 rather use @href #BuildFromChar32 instead to be able to retrieve all Unicode characters.
 */
  void BuildFromUC2Bytes(const CATUC2Bytes &iCharacter);
  
/**
 * Converts the current character, supposed to be a lower case one to upper case.
 */
  void ToUpper();
  
/**
 * Converts the current character, supposed to be a upper case one to lower case.
 */
  void ToLower();


  // Tests :
  //========

/**
 * Determines if the current character is an alphabetic character. 
 * <br><b>Role</b>: Determines if the current character is an alphabetic character. This service takes into account the 
 * current locale. 
 * @return 
 *   boolean
 *   <br><b>Legal values</b>: <tt>true: Yes</tt> if the current
 *   character is an alphabetic character, or <tt>false: No</tt> 
 *   otherwise.
 */
  bool IsAlpha() const;
  

/**
 * Determines if the current character is an alphanumeric character. 
 * <br><b>Role</b>: Determines if the current character is an alphanumeric character. This service takes into account
 * the current locale. 
 * @return 
 *   boolean
 *   <br><b>Legal values</b>: <tt>true: Yes</tt> if the current
 *   character is an alphanumeric character, or <tt>false: No</tt> 
 *   otherwise.
 */
  bool IsAlnum() const;
  

/**
 * Determines if the current character is a control character. 
 * <br><b>Role</b>: Determines if the current character is a control character. This service takes into account the
 * current locale. 
 * @return 
 *   boolean
 *   <br><b>Legal values</b>: <tt>true: Yes</tt> if the current
 *   character is a control character, or <tt>false: No</tt> 
 *   otherwise.
 */
  bool IsCntrl() const;
  
/**
 * Determines if the current character is a digit character. 
 * <br><b>Role</b>: Determines if the current character is a digit character. This service takes into account the 
 * current locale. 
 * @return 
 *   boolean
 *   <br><b>Legal values</b>: <tt>true: Yes</tt> if the current
 *   character is a digit character, or <tt>false: No</tt> 
 *   otherwise.
 */
  bool IsDigit() const;
  
/**
 * Determines if the current character is a graphic character for printing. 
 * <br><b>Role</b>: Determines if the current character is a graphic character for printing. A graphic character is a
 * character which is neither the space character nor a control character. This service takes into account the current
 * locale. 
 * @return 
 *   boolean
 *   <br><b>Legal values</b>: <tt>true: Yes</tt> if the current
 *   character is a graphic character for printing, 
 *   or <tt>false: No</tt> otherwise.
 */
  bool IsGraph() const;
  
/**
 * Determines if the current character is a lowercase character. 
 * <br><b>Role</b>: Determines if the current character is a lowercase character. This service takes into account the 
 * current locale. 
 * @return 
 *   boolean
 *   <br><b>Legal values</b>: <tt>true: Yes</tt> if the current
 *   character is a lowercase character, or <tt>false: No</tt> 
 *   otherwise.
 */
  bool IsLower() const;
  
/**
 * Determines if the current character is a uppercase character. 
 * <br><b>Role</b>: Determines if the current character is a uppercase character. This service takes into account the 
 * current locale. 
 * @return 
 *   boolean
 *   <br><b>Legal values</b>: <tt>true: Yes</tt> if the current
 *   character is a uppercase character, or <tt>false: No</tt> 
 *   otherwise.
 */
  bool IsUpper() const;
  
/**
 * Determines if the current character is a print character. 
 * <br><b>Role</b>: Determines if the current character is a print character. This service takes into account the 
 * current locale. 
 * @return 
 *   boolean
 *   <br><b>Legal values</b>: <tt>true: Yes</tt> if the current
 *   character is a print character, or <tt>false: No</tt> 
 *   otherwise.
 */
  bool IsPrint() const;
  
/**
 * Determines if the current character is a punctuation character. 
 * <br><b>Role</b>: Determines if the current character is a punctuation character. This service takes into account the 
 * current locale. 
 * @return 
 *   boolean
 *   <br><b>Legal values</b>: <tt>true: Yes</tt> if the current
 *   character is a punctuation character, or <tt>false: No</tt> 
 *   otherwise.
 */
  bool IsPunct() const;
  
 
/**
 * Determines if the current character is a space character. 
 * <br><b>Role</b>: Determines if the current character is a space character. This service takes into account the 
 * current locale. 
 * @return 
 *   boolean
 *   <br><b>Legal values</b>: <tt>true: Yes</tt> if the current
 *   character is a space character, or <tt>false: No</tt> 
 *   otherwise.
 */
  bool IsSpace() const;
  
/**
 * Determines if the current character is a hexadecimal digit character. 
 * <br><b>Role</b>: Determines if the current character is a hexadecimal digit character. This service takes into
 * account the current locale. 
 * @return 
 *   boolean
 *   <br><b>Legal values</b>: <tt>true: Yes</tt> if the current
 *   character is a hexadecimal digit character, or <tt>false: No</tt> 
 *   otherwise.
 */
  bool IsXDigit() const;
  
private:
/** @nodoc */
  void _BuildFromChar(const char iByte);

/** @nodoc */
  char        *_EnsureMbChar () const;

/** @nodoc */
  CATUC4Bytes           _UChar32;
/** @nodoc */
  mutable char          _MbChar [4+1];

};


inline CATUnicodeChar CATUnicodeChar::BuildFromChar32(const char32_t iUChar32) { return BuildFromUCChar32(static_cast<CATUC4Bytes>(iUChar32)); }
inline bool CATUnicodeChar::operator != (const CATUnicodeChar & iCharacter) const { return !(*this == iCharacter); }
inline bool CATUnicodeChar::operator >  (const CATUnicodeChar & iCharacter) const { return !operator <= (iCharacter); }
inline bool CATUnicodeChar::operator >= (const CATUnicodeChar & iCharacter) const { return !operator < (iCharacter); }
inline char32_t    CATUnicodeChar::ConvertToChar32 () const { return static_cast<char32_t>(_UChar32); }
inline CATUC4Bytes CATUnicodeChar::ConvertToUCChar32() const { return _UChar32; }

#endif  // DSYUnicodeChar_H
