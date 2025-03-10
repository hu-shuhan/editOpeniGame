/* -*-C++-*- */
// COPYRIGHT DASSAULT SYSTEMES 1997
//=============================================================================

//=============================================================================
//
// CATToken
// Class designed to break a string up into separate tokens
// The tokens are separated by arbitrary characters
//
//=============================================================================
//
// 
//
//=============================================================================
//
// December 95   Creation                                              O Fresse
// March    96   Migration to Adele V2
//
//=============================================================================

#ifndef CATToken_H
#define CATToken_H

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

#include "CATSysTS.h"
#include "DSYString.h"
#include "CATBoolean.h"
#include <stdlib.h>

/**
 * Class to analyse the contents of a CATUnicodeString instance.
 * Use this class when you want to extract substrings delimited by
 * sequences of characters chosen by delimiter characters you pass as
 * argument of the <a href="#GetNextToken">GetNextToken</a> method.
 * A CATToken instance contains the CATUnicodeString instance as a data member,
 * and a position index initialized to 0. This position index is updated
 * according to the searches made by the
 * <a href="#GetNextToken">GetNextToken</a> method, and can be reset to 0
 * thanks to the <a href="#Reset">Reset</a> method.
 * <p>
 * For example, the character string:
 * <pre>
 * This is/// a nice %% character\tstring.  
 * </pre>
 * <p>
 * can be analysed using the delimiter characters / % \t and the blank.
 */
class ExportedByCATSysTS CATToken /*final*/
{
 public:
  // Constructor :
  // =============
  
	/**
	 * Constructs a CATToken instance from a CATUnicodeSTring instance.
	 * @param iStringToAnalyse
	 *   The string to analyse.
	 * @param iAcceptEmptyStrings
	 *   Specifies whether empty strings between separators must be returned by the 
	 *   <a href="#GetNextToken">GetNextToken</a> method or not.
	 */ 
  CATToken(const CATUnicodeString& iStringToAnalyse, CATBoolean iAcceptEmptyStrings = FALSE);

  // Destructor :
  // ============
  virtual ~CATToken();
  
  // Give the next token separated by one of the characters of iDelimitersStrings

  // Methods :
  //==========
  

/**
 * Gets the next token, that is the character substring enclosed by sequences
 * of delimiter characters. 
 * For example, assume the CATToken instance is constructed from the following
 * character string:
 * <pre>
 * CATUnicodeString TheString("This is/// a nice %% character\tstring.")  
 * </pre>
 * <p>
 * and assume that the position index is set to 8 due to a previous call to the
 * method. 
 * The code below extracts the next token substring from this string:
 * <pre>
 * CATUnicodeString iDelimiterCharacters("/%");
 * CATUnicodeString NextToken = TheString.GetNextToken(iDelimiterCharacters);
 * cout << "The next token is: -" << NextToken.ConvertToChar() << "-" << endl;
 * </pre>
 * <p>
 * This code issues:
 * <pre>
 * The next token is: - a nice - 
 * </pre>
 * and sets the position index to 19.
 * @param iDelimiterCharacters
 *   Characters to take into account to find an enclosed substring.
 * @param oMoreAvailable
 *   Optional parameter set to TRUE if there are more available tokens or to FALSE if the 
 *   end of the analysed string has been reached. This parameter must be used if the CATToken
 *   has been created with the parameter iAcceptEmptyStrings set to TRUE in order to check
 *   the end of the parse.
 * @return
 *   The substring found.
 */ 
  CATUnicodeString GetNextToken(const CATUnicodeString& iDelimiterCharacters, CATBoolean * oMoreAvailable = nullptr);
  
/**
 * @nodoc
 * Gets the next token, that is the character substring enclosed by sequences of delimiter characters.
 */ 
  CATUnicodeString GetNextToken(const char * iDelimiterCharacters, CATBoolean * oMoreAvailable = nullptr);

  // Return the next token separated by one of iDelimitersStrings character

/**
 * Gets the next token, that is the character substring enclosed by sequences
 * of the following delimiter characters: blanks, tabulations (\t) and carriage
 * returns (\n). 
 * @param oMoreAvailable
 *   Optional parameter set to TRUE if there are more available tokens or to FALSE if the 
 *   end of the analysed string has been reached. This parameter must be used if the CATToken
 *   has been created with the parameter iAcceptEmptyStrings set to TRUE in order to check
 *   the end of the parse.
 * @return
 *   The substring found.
 */
  CATUnicodeString GetNextToken(CATBoolean * oMoreAvailable = nullptr); // give the next token : the separator can be a space, a tabulation, an end of line
  // The default value for iDelimitersStrings is "\t " ( A tabulation and a space )
  // but this doesn't compile under HP. So, the following method is provided

/**
 * Resets the position index to 0.
 */
  void Reset(); // reinitialisation of the tokenizer.

    // Sample of use
    // CATUnicodeString TheStringToAnalayse("I feel partially hydrogenated!");
    // CATToken Tokenizer(TheStringToAnalayse);
    // int count=0;
    // while(!Tokenizer.GetNextToken().IsEmpty())
    // 	count++;
    // cout << "Nb of tokens in [" << TheStringToAnalayse.ConvertToChar() << "] = " << count << endl;
    // Result :
    // Nb of tokens in [I feel partially hydrogenated!] = 4
    //
 private:

  CATUnicodeString _TheString;
  unsigned int  _Pos;
  unsigned char _LastDelimOffset;
  CATBoolean _acceptEmptyStrings;
  
  friend class CATTokenInternal;
};

#endif
