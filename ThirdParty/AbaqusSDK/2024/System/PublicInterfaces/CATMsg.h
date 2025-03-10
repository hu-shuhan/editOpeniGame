#ifndef CATMSG_H
#define CATMSG_H
// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

#if defined(__NS0S1MSG)
/** @nodoc */
# define ExportedByNS0S1MSG   DSYExport
#else // __NS0S1MSG
/** @nodoc */
# define ExportedByNS0S1MSG   DSYImport
#endif  // __NS0S1MSG



// COPYRIGHT DASSAULT SYSTEMES 1999

#include <stdlib.h>

#include "DSYExport.h"
#include "CATUnicodeString.h"
class CATString;
class CATInterMsg;

/**
 * Message from a message catalog.
 * <b>Role</b>: Processing a message from a message catalog (see @href CATMsgCatalog ).
 * A message is the unit cell that is the a message of a 
 * message catalog (see @href CATMsgCatalog ).  
 * A message is a parameterized alphanumerical
 * string which can be used for any alphanumerical item of 
 * information, a warning, a help or error message, and whenever you need to display any text in a
 * dialog window, such a the caption on a push button.
 * NLS (National Langage Support) is supported.
 * <p>Syntaxically, a CATMsg object (which can be, as 
 * you can see in its syntax below, constructed simply calling 
 * the CATMsg class constructor, with no argument),
 * corresponds precisely to an access to the message itself.
 * <p>A given message of a message catalog contains a key.
 * This key identifies a message. For example, the
 * @href CATMsgCatalog#SubstituteCatalogMsg method, to
 * substitute a message in a given message catalog,
 * takes as the input message identifier its key.
 * <p>Regarding the parameters of a message, you reference them
 * through a syntax of the following form: A parameter begins with /p or /P in 
 * the message text. The following are examples of messages:
 * <pre>
 * Message1 = "This is a simple message.";
 * Message2 = "This message includes the parameter /P1 that
 *             is valued by your application at run-time.";
 * Message3 = "This is a", "compound ", "message.";
 * Message4 = "You can use control characters such as \t or \n in your messages.";</pre>
 * Messages can be easily translated, since they are stored outside of the
 * code that handles them.</br>
 * see @href CATMsgCatalog </br>
 * see @href CATUnicodeString
 */

// CATMsg:
// Class for parameterized message support

// The CATMsg object is returned by CATMsgCatalog::GetCatalogMsg method

class ExportedByNS0S1MSG CATMsg
{
  friend class CATInterMsgCatalog;
  
  public:

  // Constructor
  // ===========

/**
 * Constructs an empty message.</br>
 * <b>Caution</b>: This method should rarely be used, in most cases the use of 
 * @href CATMsgCatalog#BuildMessage is enough.
 */
  CATMsg();

/** 
 * Copy constructor.</br>
 * <b>Caution</b>: This method should rarely be used, in most cases the use of 
 * @href CATMsgCatalog#BuildMessage is enough.
 * @param iMessage 
 *   The message to copy
 */

  CATMsg(const CATMsg &iMessage );  

  // Destructor :
  // ============
   ~CATMsg();
  
  // Operator :
  // ==========

/**
 * Assignment operator.</br>
 * <b>Caution</b>: This method should rarely be used, in most cases the use of 
 * @href CATMsgCatalog#BuildMessage is enough.
 * @param iMessage
 *   Message of the right part of the equality
 * @return  
 *   Message of the left part of the equality
 */

  const CATMsg&	operator=(const CATMsg& iMessage);

  // Methods :
  //==========
  
  // Gives the number of parameters, defined by /P in the message.
//
// * Returns the parameter count.
// See BuildMessage for reasons of no documenting this method.
//
/** @nodoc */
  int GetNbParameters() const;

//
// Returns the highest index in the message parameters.
// This is not exactly the parameter count: 
// the software authorizes parameter indices that are not
// consecutive, which would distinguish the message 
// parameters highest index from the parameter count.
//
/** @nodoc */
  int GetMaxParamIndex () const;

//
// Determines if the CATMsg is empty.
// @return 
//   boolean
//   <br><b>Legal values</b>: <tt>0: False</tt> 
//   if the message is not empty, or <tt>Other: True</tt> 
//   if the message is empty.
// 
/** @nodoc */
  int IsEmpty() const;


//
// Computes an alphanumerical string from a message.
// This  method is not documented because the methodology to use
// for applications is CATMsgCatalog::BuildMessage .
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
// <b>Role</b>:It computes a CATUnicodeString from the
// specified message and returns it.
// This CATUnicodeString instance can then be used for display.
// If the message includes parameters, fill in <tt>iTabOfParameters</tt>
// with their values converted into CATUnicodeString instances.
// <p>
// Example: Assume the following message is stored in a message catalog:
// <pre>
// TheKey="The velocity is /P01 mph, and the weight is /P02 kg.\n";
// </pre>
// To make such a message displayable, you need to value its two parameters,
// retrieve the message from the message catalog using its key,
// and use the @href CATMsg#BuildMessage method.
// <pre>
// CATUnicodeString Params[2];
// Params[0]="50";
// Params[1]="100";
// CATString Key("TheKey");
// CATMsg Message=TheCatalog.GetMsg(Key);
// CATUnicodeString FinalString=Message.BuildMessage(Params);
// cout << FinalString.ConvertToChar() << endl;
// </pre>
// <p>
// This displays:
// <pre>
// The velocity is 50 mph, and the weight is 100 kg.
// </pre>
// @param iTabOfParameters
//   Parameter value table.
//
/** @nodoc */
  CATUnicodeString BuildMessage( CATUnicodeString iTabOfParameters[] = NULL ) const;

/**
 * Constructs a CATMsg instance from a CATUnicodeString instance.</br>
 * <b>Caution</b>: This method should rarely be used, in most cases the use of 
 * @href CATMsgCatalog#BuildMessage is enough.
 */

  CATMsg (const CATUnicodeString& iString);

  private:

/** @nodoc */
  CATUnicodeString _Message;
/** @nodoc */
  CATInterMsg *_CompoundMessage;  
};

#endif
