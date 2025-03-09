#ifndef CATScriptLanguage_h
#define CATScriptLanguage_h

// COPYRIGHT DASSAULT SYSTEMES 2000

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

/**
 * Possible languages for the generated scripts.
 * @param CATVBScriptLanguage
 * VB Script language.
 * @param CATVBALanguage
 * VBA language.
 * @param CATBasicScriptLanguage
 * BasicScript language.
 * @param CATJavaLanguage
 * Java language.
 * @param CATJScriptLanguage
 * JScript language.
 * @param CATCSharpLanguage
 * C# language.
 * @param CATVBNetLanguage
 * VB.Net language.
 * @see CATIScriptManager
 */
enum CATScriptLanguage{CATVBScriptLanguage,
                       CATVBALanguage,
                       CATBasicScriptLanguage,
                       CATJavaLanguage,
                       CATJScriptLanguage,
											 CATCSharpLanguage, 
											 CATVBNetLanguage
};


#endif

