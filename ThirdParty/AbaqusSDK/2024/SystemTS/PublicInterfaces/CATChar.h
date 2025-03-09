#ifndef CATChar_H
#define CATChar_H   42500

#include "CATSysTS.h"

// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

/**
 * Class to represent non-NLS Characters.
 * <b>Role</b>: Class for ISO 646 characters, i.e. codes from
 * 0 to 127 in decimal:
 *   - C0 ASCII control codes (see the unicode standard
 *     documentation): 0 to 31 in decimal
 *   - ASCII (contains digit characters, latin capital
 *     letters, latin small letters, punctuation characters ...)       
 * Thus, this class does not support NLS
 * (National Langage Support), at the opposite of
 * @href CATUnicodeChar.
 * Comparing this class to @href CATUnicodeChar,
 * the very simple character set supported here, 
 * once accepted the limitations it implies, enables to supress
 * all the problems that arise with NLS complexity.
 * Before using this class instead of CATUnicodeChar,
 * check carefully that you will never need the NLS
 * support. 
 */

class ExportedByCATSysTS   CATChar
{
public:
/**
 * Construct a ISO 646 char from a char.
 */
	CATChar( const char );
/**
 * Construct an empty CATChar.
 */
	CATChar();
/*
 * Copy constructor.
 * @param iChar
 *   The character to copy
 */
	CATChar( const CATChar& iChar);
/**
 * Destructor.
 */
	virtual ~CATChar();


/**
 * Assignment operator.
 * @param iChar
 *   The character to assign to the current one
 * @return 
 *   The class resulting from the assignment
 */
	CATChar&	operator=(const char iChar);	
/**
 * Assignment operator.
 * @param iChar
 *   The character to assign to the current one
 * @return 
 *   The class resulting from the assignment
 */
	CATChar&	operator=(const CATChar& iChar);	// Replace string
 

/**
 * Equality operator.
 * @param iChar1
 *   First operand of the comparison
 * @param iChar2
 *   Second operand of the comparison
 * @return 
 *   boolean
 *   <br><b>Legal values</b>: <tt>false</tt> 
 *   the condition is not fullfilled, or <tt>true</tt> 
 *   if the condition is fullfilled.
 */
	ExportedByCATSysTS friend  bool operator==(const CATChar& iChar1, 
                                            const CATChar& iChar2);

/**
 * Inequality operator.
 * @param iChar1
 *   First operand of the comparison
 * @param iChar2
 *   Second operand of the comparison
 * @return 
 *   boolean
 *   <br><b>Legal values</b>: <tt>false</tt> 
 *   the condition is not fullfilled, or <tt>true</tt> 
 *   if the condition is fullfilled.
 */
	ExportedByCATSysTS friend bool operator!=(const CATChar& iChar1, 
                                           const CATChar& iChar2);

/**
 * Less than operator.
 * @param iChar1
 *   First operand of the comparison
 * @param iChar2
 *   Second operand of the comparison
 * @return 
 *   boolean
 *   <br><b>Legal values</b>: <tt>false</tt> 
 *   the condition is not fullfilled, or <tt>true</tt> 
 *   if the condition is fullfilled.
 */
	ExportedByCATSysTS friend  bool operator<(const CATChar& iChar1, 
                                           const CATChar& iChar2);

/**
 * Less than or equal to operator.
 * @param iChar1
 *   First operand of the comparison
 * @param iChar2
 *   Second operand of the comparison
 * @return 
 *   boolean
 *   <br><b>Legal values</b>: <tt>false</tt> 
 *   the condition is not fullfilled, or <tt>true</tt> 
 *   if the condition is fullfilled.
 */
	ExportedByCATSysTS friend bool operator<=(const CATChar& iChar1, 
                                           const CATChar& iChar2);

/**
 * Greater than operator.
 * @param iChar1
 *   First operand of the comparison
 * @param iChar2
 *   Second operand of the comparison
 * @return 
 *   boolean
 *   <br><b>Legal values</b>: <tt>false</tt> 
 *   the condition is not fullfilled, or <tt>true</tt> 
 *   if the condition is fullfilled.
 */
	ExportedByCATSysTS friend bool operator>(const CATChar& iChar1, 
                                          const CATChar& iChar2);

/**
 * Greater than or equal operator.
 * @param iChar1
 *   First operand of the comparison
 * @param iChar2
 *   Second operand of the comparison
 * @return 
 *   boolean
 *   <br><b>Legal values</b>: <tt>false</tt> 
 *   the condition is not fullfilled, or <tt>true</tt> 
 *   if the condition is fullfilled.
 */
	ExportedByCATSysTS friend bool operator>=(const CATChar& iChar1, 
                                           const CATChar& iChar2);
/**
 * Cast operator.
 * @param iChar
 *   char* character
 */
	operator char() const;


  protected:

  private:
/** @nodoc */ 
	char _Character;
};

#ifdef _CAT_ANSI_STREAMS
/** @nodoc */
#include <iosfwd.h>

ExportedByCATSysTS ostream& operator<<(ostream&, const CATChar&);
#endif

#endif

