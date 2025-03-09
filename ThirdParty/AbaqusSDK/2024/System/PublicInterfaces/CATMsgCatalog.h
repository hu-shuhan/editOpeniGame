#ifndef CATMSGCATALOG_H
#define CATMSGCATALOG_H

// COPYRIGHT DASSAULT SYSTEMES 1999 

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

#include "CATMsg.h"
#include "CATString.h"
#include "CATUnicodeString.h"
#include "CATErrors.h"
class CATInterMsgCatalog;
class CATListValCATString;

/**
 * Message catalog class. 
 * <b>Role</b>: Processing a message catalog file.
 * A message catalog file contains parameterized messages (see @href CATMsg ).
 * A message is a parameterized alphanumerical
 * string which can be used for any alphanumerical item of 
 * information, a warning, a help or error message, and whenever you need to display any text in a
 * dialog window, such a the caption on a push button.</br>
 * NLS (National Langage Support) is supported but, in the same way, a message catalog file should only
 * contain strings which must be translated into other languages: otherwise, resource catalog files
 * should be used (see @href CATRscCatalog ).</br>
 * A message catalog is built from a file that contains messages identified by a key.
 * A message can be parameterized ie contains parameters you
 * value at run-time. A parameter begins with /p or /P in the message text.</br>
 * The following are examples of messages:
 * <pre>
 * Message1 = "This is a simple message.";
 * Message2 = "This message includes the parameter /P1 that
 *             is valued by your application at run-time.";
 * Message3 = "This is a", "compound ", "message.";
 * Message4 = "You can use control characters such as
 *           \t or \n in your messages.";</pre>
 * A message catalog file posess the .CATNls suffix and is retrieved from the folders specified by the
 * CATMsgCatalogPath variable.
 * The localized version of the message catalog is searched first. If it does not exist,
 * the english version is taken.</br>
 * Regarding the localized version, the language taken is the UI language. The UI language is determined the following way:
 * <ul>
 * <li>the Tools\Customize\Options\"User Interface Language" combo is read.</li>
 * <li>in the case where this combo is set to the "Environment language" value (default), the thread language locale is taken (see,
 * on Windows, the "Settings for the current user" frame of the "General" tab of the "Regional Options" panel of the Control Panel and,
 * on Unix, the window locale)</li>
 * <li>in the case where this combo is set to an explicit language, the given language is taken</li>
 * </ul>
 * Example: Suppose
 * <ul>
 * <li>we are on Windows</li>
 * <li>the environment is Japanese</li>
 * <li>the Tools\Customize\Options\"User Interface Language" combo is set to "Environment language (default)"</li>
 * </ul>
 * then, if the CATMsgCatalogPath variable is valued the following way:
 * <pre>   CATMsgCatalogPath=C:\PersonalFolder;C:\Program Files\Dassault Systemes\B14</pre>
 * CATIA will search the message catalog file into the following paths:
 * <pre>   1 - C:\PersonalFolder\Japanese
 *   2 - C:\PersonalFolder
 *   3 - C:\Program Files\Dassault Systemes\B14\Japanese
 *   4 - C:\Program Files\Dassault Systemes\B14</pre>
 * The character set for the message key characters is the following:
 * <ul>
 * <li>within the A - Z range</li>
 * <li>within the a - z range</li>
 * <li>within the 0 - 9 range</li>
 * <li>PERIOD (.), SPACING UNDERSCORE (_), LEFT SQUARE BRACKET ([) and RIGHT SQUARE BRACKET (]) characters</li>
 * </ul>
 */

// CATMsgCatalog:
// Class for parameterized message support

// This class provides services to manage message catalogs
// A message can be parameterized. See class CATMsg.
//
// A catalog is a file in a human-readable form, ie you can edit it with
// any editor.
//
// Catalog sample
// --------------
// | // This is a comment.
// |
// |  FirstKey="Hello /P01"; // /P01 is the first parameter
// |
// |  SecondKey = "This is a ", "message ",
// |               "from Mars.";
// |
// |  AnotherKey = "You can use control characters \t \n ",
// |               "defined in the C++ language";

class ExportedByNS0S1MSG CATMsgCatalog
{
  public:
  // Constructors
  // ============

/**
 * Constructs a message catalog. 
 */
     CATMsgCatalog();

/** Copy constructor.</br>
 * <b>Caution</b>: This method should rarely be used, in most cases the use of 
 * @href CATMsgCatalog#BuildMessage is enough.
 * @param iMsgCatalog
 *   The message catalog to copy
 */
     CATMsgCatalog(const CATMsgCatalog &iMsgCatalog);

  // Destructor :
  // ============
     ~CATMsgCatalog();

/**
 * Assignment operator.</br>
 * <b>Caution</b>: This method should rarely be used, in most cases the use of 
 * @href CATMsgCatalog#BuildMessage is enough.
 * @param iMsgCatalog
 *   Message catalog of the right part of the equality
 * @return 
 *   Message catalog of the left part of the equality
 */ 
     CATMsgCatalog &operator =(const CATMsgCatalog &iMsgCatalog) ;

/**
 * Loads a message catalog.
 * <br><b>Role</b>: Loads a message catalog  by means of its name and (if desired)
 * of a path.</br>
 * <b>Caution</b>: This method should rarely be used, in most cases the use of 
 * @href CATMsgCatalog#BuildMessage is enough.
 *  @param iCatalogMessageName
 *    Name of the message catalog file, without the .CATNls suffix
 *  @param iPath
 *    Absolute path where the catalog message must be searched for.</br>
 *    The localized version of the message catalog is searched first. If it does not exist,
 *    the english version is taken.</br>
 *    Example:
 *    In a japanese Windows environment, if the iPath variable is valued the following way:
 *    <pre>      iPath="C:\PersonalFolder;C:\Program Files\Dassault Systemes\B14"</pre>
 *    then, CATIA search the message catalog file into the following paths:
 *    <pre>      1 - C:\PersonalFolder\Japanese
 *      2 - C:\PersonalFolder
 *      3 - C:\Program Files\Dassault Systemes\B14\Japanese
 *      4 - C:\Program Files\Dassault Systemes\B14</pre>
 * <b>Caution</b>: This parameter should always be given a null value. Actually, the english version of a message catalog
 * file should always be located at:<pre>    %CATInstallPath%\resources\msgcatalog</pre>
 * Otherwise, the message catalog will not be taken into account by the following tools:
 * <ul>
 * <li>translation tools</li>
 * <li>test message catalog generation tools</li>
 * <li>the installation dialog regarding the language of the user interface files which must be installed</li>
 * </ul>
 * Consequently, a non-null value for iPath has no meaning.
 * @param iLanguage
 *   Language in which the message must be taken:
 *   <ul>
 *   <li>English</li>
 *   <li>French</li>
 *   <li>German</li>
 *   <li>Italian</li>
 *   <li>Japanese</li>
 *   <li>Korean</li>
 *   <li>Simplified_Chinese</li>
 *   </ul>
 *   NULL default value coresponds to the UI language (see above).
 *  @return
 *   <br><b>Legal values</b>: <tt>0: False</tt> if an error occured, or <tt>1</tt> if the execution succeeded.</br>
 *   If the execution gives <tt>0</tt>, debug the message catalog file using @href CATMsgCatalog#GetError , which may return:
 *   <ul>
 *   <li>Line %d Unexpected EOF !</li>
 *   <li>Line %d = expected, and '%d' found</li>
 *   <li>Line %d \" expected, and '%d' found</li>
 *   <li>Line %d  Expected , or ; not found</li>
 *   <li>File %s.CATNls has not read access</li>
 *   <li>File %s.CATNls not found</li>
 *   <li>Error while reading file : %s.CATNls</li>
 *   </ul>
 */

// This  method is rarely used because the methodology to use
// for applications is CATMsgCatalog::BuildMessage . 
// BuildMessage is encapsulated. It takes into account the 
// following things:
//    . if the catalog is not loaded, it does it (this 
//      enables the calling code not to bother about the loading:
//      it lets the managing of it to BuildMessage, the message
//       catalog will be persent into the memory only once,
//      whatever the count of softwares using it.
//    . from the input message catalog identifier (CATString),
//      the software find in its own data the message catalog
//      ppointer, through a method that has its performances
//      optimized: a hashtable. This technology ensures there
//      is not any performance problem.
// Effectively  , we remind that  two solutions exist:
//   - calling only CATMsgCatalog::BuildMessage
//   - calling successively:
//        .  MyCatalog = new CATMsgCatalog()
//        . MyCatalog->LoadMsgCatalog(nom du catalogue de messages ...)
//        . eventuel MyCatalog->GetCatalogKeys(...)
//                   * MyMsg = MyCatalog->GetCatalogMsg(cle ...)   
//                              /* The MyMsg outputCATMsg is
//                                 a parameterized message:
//                                 the parameters are not under their value form
//                                 at this level */
//                   * MyMsg->GetNbParameters()
//                   * MyMsg->BuildMessage
//                   * delete MyMsg
//        . MyCatalog->UnloadMsgCatalog(...)
//        . delete MyCatalog
// For example:
//   . In the interactive drafting application, only
//     the first solution is used
//   . In the Dialog  framework also. When it computes a
//     given message, it stores it in its own member data 
//     (thus the message is duplicated) for future  use
//     (remind that most  messages have no parameters). The
//     purpose  is to factorize the possible error information
//     detected during the file loading (this duplication is
//     justified in any way for any performances reason).
//
//  Don't forget to call CATMsgCatalog#GetError after calling this
//  service to check potential syntax errors in the message
//  catalog.
     int LoadMsgCatalog (const CATString &iCatalogMessageName, 
                         const char *iPath = NULL,
                         CATString* iLanguage = NULL);

/** 
 * Returns the first syntax error found.
 * <br><b>Role</b>: Returns the first syntax error found after a call to @href CATMsgCatalog#BuildMessage
 * or @href CATMsgCatalog#LoadMsgCatalog .<br>
 * <b>CAUTION</b>: Although this method is not static, the @href CATString returned does not depend from
 * the @href CATMsgCatalog instance used.<br>
 * See @href CATMsgCatalog#BuildMessage and @href CATMsgCatalog#LoadMsgCatalog .
 * @return
 *   Error message. It may be:
 * <ul>
 * <li>File %s.CATNls has not read access</li>
 * <li>File %s.CATNls not found</li>
 * <li>Error while reading file : %s.CATNls</li>
 * <li>Line %d Unexpected EOF !</li>
 * <li>Line %d = expected, and '%d' found</li>
 * <li>Line %d \" expected, and '%d' found</li>
 * <li>Line %d  Expected , or ; not found</li>
 * <li>GetCatalogMsg() called with a bad key format : '%d' can't be used in a key</li>
 * <li>Key %s not defined.</li>
 * </ul>
 */ 
     const CATString GetError ();

/**
 * Returns a message from the message catalog.
 * <br><b>Role</b>: Returns a message from a message catalog using the message key.</br>
 * <b>Caution</b>: This method should rarely be used, in most cases the use of 
 * @href CATMsgCatalog#BuildMessage is enough.
 *  @param iMessageKey
 *    Key of the message to be returned
 * @param iLanguage
 *   Language in which the message must be taken:
 *   <ul>
 *   <li>English</li>
 *   <li>French</li>
 *   <li>German</li>
 *   <li>Italian</li>
 *   <li>Japanese</li>
 *   <li>Korean</li>
 *   <li>Simplified_Chinese</li>
 *   </ul>
 *   NULL default value coresponds to the UI language (see above).
 *  @return
 *    The returned parameterized message. If an error occured the return value is an empty string.
 *    If the execution gives an empty string, debug the message catalog file using @href CATMsgCatalog#GetError , which
 *    may return:
 *    <ul>
 *    <li>Line %d Unexpected EOF !</li>
 *    <li>Line %d = expected, and '%d' found</li>
 *    <li>Line %d \" expected, and '%d' found</li>
 *    <li>Line %d  Expected , or ; not found</li>
 *    <li>File %s.CATNls has not read access</li>
 *    <li>File %s.CATNls not found</li>
 *    <li>Error while reading file : %s.CATNls</li>
 *    </ul>
 */
     const CATMsg  GetCatalogMsg(const CATString &iMessageKey,
                                 CATString* iLanguage = NULL); 

/**
 * Computes an alphanumeric message string from an identified
 * parameterized message of the catalog, given specified
 * parameters values.
 * <br><b>Role</b>: Computes a alphanumeric string from a 
 * message from a message catalog using the catalog name, the 
 * message key, the message parameters and (if needed) a default 
 * NLS message.
 * <p>
 * Regarding messages, for most use, the <tt>GetResourceValueFromKey</tt> method 
 * of the <tt>CATDialog</tt> class will be
 * sufficient. We remind you also that Dialog Engine Acquisition Agents
 * support messages without having to use any message service, just
 * using the file and key name convention rules.
 * <p>
 * Anyway, sometimes (for combo values for example), you may need 
 * to access to the messages from soft. You will then use this
 * service.
 * <p>
 * The use of this service is as follows: call it at any moment
 * you want during execution. You do not have to
 * store the computed message for a future re-use. Effectively,
 * this service is encapsulated. It takes into account the 
 * following things:
 * <ul>
 * <li>If the catalog is not loaded, it does it (this 
 * enables the calling code not to bother about the loading:
 * it lets the managing of it to this service, the message
 * catalog will be present into the memory only once,
 * whatever the count of softwares using it.</li>
 * <li>Otherwise, from the input message catalog identifier 
 * (iCatalogName parameter, see below), the method find in its 
 * own data the message catalog pointer, among the other 
 * message catalog already loaded, through a method that 
 * has its performances optimized: a hashtable. This technology 
 * ensures there isn't any performance problem.</li>
 * </ul>
 * The only need for an application to factorize a given
 * message the first time it computes it (storing it for future 
 * uses), would be to make an optimized management of exceptions 
 * that would occur during the message catalog loading: the 
 * purpose beeing to mention the message catalog syntax errors
 * only once. Such applications needs seldom occur.
 * <p>
 * @param iCatalogName
 *   Name of the catalog containing the message, without the .CATNls suffix
 * @param iMessageKey
 *   Key of the message to be retrieved
 * @param iMsgParameters
 *   Array giving to the method possible parameter values
 *   which the method will integrate into the parameterized
 *   message. 
 *   The parameter value count should correspond to the message 
 *   parameter highest index (this is not exactly the parameter count: 
 *   the software authorizes parameter indices that are not
 *   consecutive, which would distinguish the message 
 *   parameters highest index from the parameter count).
 *   If the input parameter value count is not sufficient, a default 
 *   behaviour is foreseen: "?" characters are introduced into the
 *   computed output resource string. 
 *   NULL default value coresponds to no parameter values.
 * @param iParamNb
 *   Parameter value count
 * @param iDefaultMsg
 *   Message to be used if a problem occured while
 *   accessing the message catalog file or the key. You may, for
 *   example, put in this message an information about an
 *   access problem. 
 *   We suggest you to deliver a valid value to this parameter, so that,
 *   if an error occurs, a valid return value be delivered by
 *   the service. 
 * @param iLanguage
 *   Language in which the message must be taken:
 *   <ul>
 *   <li>English</li>
 *   <li>French</li>
 *   <li>German</li>
 *   <li>Italian</li>
 *   <li>Japanese</li>
 *   <li>Korean</li>
 *   <li>Simplified_Chinese</li>
 *   </ul>
 *   NULL default value coresponds to the UI language (see above).
 * @return
 *   The built message. If the execution gives the <tt>iDefaultMsg</tt> return value, debug the message catalog file
 *   using @href CATMsgCatalog#GetError , which may return:
 *   <ul>
 *   <li>Line %d Unexpected EOF !</li>
 *   <li>Line %d = expected, and '%d' found</li>
 *   <li>Line %d \" expected, and '%d' found</li>
 *   <li>Line %d  Expected , or ; not found</li>
 *   <li>File %s.CATNls has not read access</li>
 *   <li>File %s.CATNls not found</li>
 *   <li>Error while reading file : %s.CATNls</li>
 *   </ul>
 */
     static const CATUnicodeString BuildMessage ( 
                 const CATString &iCatalogName,
				 const CATString &iMessageKey,
				 CATUnicodeString *iMsgParameters = NULL,
				 int iParamNb = 0,
				 const CATUnicodeString &iDefaultMsg = (char *)NULL,
     CATString* iLanguage = NULL);
/**
 * Computes an english alphanumeric message string.
 * <br><b>Role</b>: Does the same thing as @href CATMsgCatalog#BuildMessage , except that it takes the english version
 * and not the UI language version. 
 * <b>Caution</b>: This method should rarely be used, in most cases the use of 
 * @href CATMsgCatalog#BuildMessage is enough.
 * <p>
 * @param iCatalogName
 *   Name of the catalog containing the message
 * @param iMessageKey
 *   Key of the message to be retrieved
 * @param iMsgParameters
 *   Array giving to the method possible parameter values
 *   which the method will integrate into the parameterized
 *   message. 
 * @param iParamNb
 *   Parameter value count
 * @param iDefaultMsg
 *   Message to be used if a problem occured while
 *   accessing the message catalog file or the key.
 * @return
 *   The built message. If the execution gives the <tt>iDefaultMsg</tt> return value, debug the message catalog file
 *   using @href CATMsgCatalog#GetError , which may return:
 *   <ul>
 *   <li>Line %d Unexpected EOF !</li>
 *   <li>Line %d = expected, and '%d' found</li>
 *   <li>Line %d \" expected, and '%d' found</li>
 *   <li>Line %d  Expected , or ; not found</li>
 *   <li>File %s.CATNls has not read access</li>
 *   <li>File %s.CATNls not found</li>
 *   <li>Error while reading file : %s.CATNls</li>
 *   </ul>
 */
     static const CATUnicodeString BuildEnglishMessage ( 
                 const CATString &iCatalogName,
				 const CATString &iMessageKey,
				 CATUnicodeString *iMsgParameters = NULL,
				 int iParamNb = 0,
				 const CATUnicodeString &iDefaultMsg = 
                                                  (char *)NULL );
  
/**
 * Get the message catalog keys.
 * @param oKey
 *   The key list.
 * <br><b>Lifecycle rules deviation</b>: the caller must allocate the object and manage so that, in input,
 * the oKey size be equal to zero. 
 * @return
 *   The message catalog key count.
 *   <br><b>Legal values</b>: <tt>0</tt> if an error occured, or <tt>Other</tt> otherwise.</br>
 *   If the execution gives a <tt>0</tt> value, debug the message catalog file using @href CATMsgCatalog#GetError , which
 *   may return:</br>
 *   <ul>
 *   <li>Line %d Unexpected EOF !</li>
 *   <li>Line %d = expected, and '%d' found</li>
 *   <li>Line %d \" expected, and '%d' found</li>
 *   <li>Line %d  Expected , or ; not found</li>
 *   <li>File %s.CATNls has not read access</li>
 *   <li>File %s.CATNls not found</li>
 *   <li>Error while reading file : %s.CATNls</li>
 *   </ul>
 */ 
     int GetCatalogKeys (CATListValCATString* oKey=NULL);

//
// Unloads a message catalog by means of its name.
// @param iCatalogName
//    Name of the catalog to be unloaded.
// @return
//    <br><b>Legal values</b>: <tt>0: False</tt> if the reloading didn't succeed, or <tt>1: True</tt>  otherwise.
//    If the execution gives a <tt>0</tt> value, debug the message catalog file using @href CATMsgCatalog#GetError ,
//    which may return:<br>
//    <ul>
//    <li>Line %d Unexpected EOF !</li>
//    <li>Line %d = expected, and '%d' found</li>
//    <li>File %s.CATNls has not read access</li>
//    <li>File %s.CATNls not found</li>
//    <li>Error while reading file : %s.CATNls</li>
//    </ul>
/** @nodoc */
     static int UnloadMsgCatalog ( const CATString &iCatalogName );
  
/**
 * Substitutes a message into an internalized message catalog.
 * <br><b>Role</b>: Substitutes a message into an internalized message catalog
 * using the message key. This enable to use messages known only during execution
 * into a CATStateCommand.
 * <p>Example:</br>
 * The MyApplicativeCommand.CATNls file contains:
 * <pre>     MyApplicativeCommand.MessageSink="fake message";</pre>
 * The MyApplicativeCommand::BuildGraph method will contain:
 * <pre>     CATDialogState* MyState = NULL;
 *     CATMsgCatalog ApplicativeCommandCatalogMessage;
 *     CATString MessageKey = "MessageSink";
 *     CATUnicodeString StringOfTheMessageKnownDuringExecution = "message obtained during execution";
 *     ApplicativeCommandCatalogMessage.LoadMsgCatalog("MyApplicativeCommand");
 *     CATMsg MessageKnownDuringExecution(StringOfTheMessageKnownDuringExecution);
 *     ApplicativeCommandCatalogMessage.SubstituteCatalogMsg("MyApplicativeCommand.MessageSink", MessageKnownDuringExecution);
 *     MyState = AddDialogState(MessageKey);</pre>
 * @param iMessageKey
 *   Key of the message to substitute
 * @param iReplacingMessage
 *   The new message which must replace the current one
 *  @return
 *    <br><b>Legal values</b>: <tt>0: False</tt> 
 *    if the substitution didn't succeed, or <tt>Other: True</tt> otherwise.</br> If the execution gives a <tt>0</tt>
 *    value, debug the message catalog file using @href CATMsgCatalog#GetError , which may return:<br>
 *    <ul>
 *    <li>Line %d Unexpected EOF !</li>
 *    <li>Line %d = expected, and '%d' found</li>
 *    <li>Line %d \" expected, and '%d' found</li>
 *    <li>Line %d  Expected , or ; not found</li>
 *    <li>File %s.CATNls has not read access</li>
 *    <li>File %s.CATNls not found</li>
 *    <li>Error while reading file : %s.CATNls</li>
 *    </ul>
 */
     int  SubstituteCatalogMsg( const CATString &iMessageKey,
                                const CATMsg &iReplacingMessage ); 

  private:

/** @nodoc */
// Loads a message catalog.
// <br><b>Role</b>: Loads a message catalog  by means of its name and (if desired)
// of a path.</br>
// <b>Caution</b>: This method should rarely be used, in most cases the use of 
// @href CATMsgCatalog#BuildMessage is enough.
//  @param iCatalogMessageName
//    Name of the message catalog file, without the .CATNls suffix
//  @param iPath
//    Absolute path where the catalog message must be searched for.</br>
//    The localized version of the message catalog is searched first. If it does not exist,
//    the english version is taken.</br>
//    Example:
//    In a japanese Windows environment, if the iPath variable is valued the following way:
//    <pre>      iPath="C:\PersonalFolder;C:\Program Files\Dassault Systemes\B14"</pre>
//    then, CATIA search the message catalog file into the following paths:
//    <pre>      1 - C:\PersonalFolder\Japanese
//      2 - C:\PersonalFolder
//      3 - C:\Program Files\Dassault Systemes\B14\Japanese
//      4 - C:\Program Files\Dassault Systemes\B14</pre>
//  @return
//    <br><b>Legal values</b>: <tt>0: False</tt> an error occured, or <tt>1</tt> if the execution succeeded.</br>
//    If the execution gives a <tt>0</tt> value, debug the message catalog file using @href CATMsgCatalog#GetError , 
//    which may return:
//    <ul>
//    <li>Line %d Unexpected EOF !</li>
//    <li>Line %d = expected, and '%d' found</li>
//    <li>Line %d \" expected, and '%d' found</li>
//    <li>Line %d  Expected , or ; not found</li>
//    <li>File %s.CATNls has not read access</li>
//    <li>File %s.CATNls not found</li>
//    <li>Error while reading file : %s.CATNls</li>
//    </ul>
// This  method is rarely used because the methodology to use
// for applications is CATMsgCatalog::EnglishBuildMessage . 
// EnglishBuildMessage is encapsulated. It takes into account the 
// following things:
//    . if the english catalog is not loaded, it does it (this 
//      enables the calling code not to bother about the loading:
//      it lets the managing of it to BuildEnglishMessage, the message
//       catalog will be persent into the memory only once,
//      whatever the count of softwares using it.
//    . from the input message catalog identifier (CATString),
//      the software find in its own data the message catalog
//      ppointer, through a method that has its performances
//      optimized: a hashtable. This technology ensures there
//      is not any performance problem.
// Effectively  , we remind that  two solutions exist:
//   - calling only CATMsgCatalog::BuildEnglishMessage
//   - calling successively:
//        . MyCatalog = new CATMsgCatalog()
//        . MyCatalog->LoadEnglishMsgCatalog(nom du catalogue de messages ...)
//        . eventuel MyCatalog->GetCatalogKeys(...)
//                   * MyMsg = MyCatalog->GetEnglishCatalogMsg(cle ...)   
//                              /* The MyMsg outputCATMsg is
//                                 a parameterized message:
//                                 the parameters are not under their value form
//                                 at this level */
//                   * MyMsg->GetNbParameters()
//                   * MyMsg->BuildMessage
//                   * delete MyMsg
//        . MyCatalog->UnloadMsgCatalog(...)
//        . delete MyCatalog
     int LoadEnglishMsgCatalog (const CATString &iCatalogMessageName,const char *iPath = NULL);

/** @nodoc */
// Returns the english version of a message from the message catalog.
// <br><b>Role</b>: Returns a message from a message catalog using the message key.</br>
// <b>Caution</b>: This method should rarely be used, in most cases the use of 
// @href CATMsgCatalog#BuildEnglishMessage is enough.
//  @param iMessageKey
//    Key of the message to be returned
//  @return
//    The returned message. If the message is not found, or the catalog invalid, an empty string is returned. 
//    If an empty string is returned, debug the message catalog file using @href CATMsgCatalog#GetError , which may
//    return:<br>
//    <ul>
//    <li>Line %d Unexpected EOF !</li>
//    <li>Line %d = expected, and '%d' found</li>
//    <li>Line %d \" expected, and '%d' found</li>
//    <li>Line %d  Expected , or ; not found</li>
//    <li>File %s.CATNls has not read access</li>
//    <li>File %s.CATNls not found</li>
//    <li>Error while reading file : %s.CATNls</li>
//    </ul>
     const CATMsg  GetEnglishCatalogMsg( const CATString &iMessageKey ); 
/** @nodoc */
     CATInterMsgCatalog *_CatalogRef;
// Name of the message catalog (without the ".CATNls" suffix)
/** @nodoc */
     CATString _MsgCatalogName;
// Tree path for message catalogs when precised through the LoadMsgCatalog method
/** @nodoc */
     CATUnicodeString _MsgCatalogTreePathWhenSpecified;
};

#endif


