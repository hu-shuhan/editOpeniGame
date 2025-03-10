#ifndef CATRSCCATALOG_H
#define CATRSCCATALOG_H

// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

#include "CATMsg.h"
#include "CATString.h"
class CATInterRscCatalog;
class CATListValCATString;

/**
 * Resource catalog class. 
 * <b>Role</b>: Processing a resource catalog file.
 * The resource catalog file contains keys and their corresponding values. Values are parameterized non translatable
 * messages (see @href CATString ) which may, for example, identify resources. NLS (National Langage Support) is not
 * supported, this class must be compared to the @href CATMsgCatalog class: the current class must be used for all non
 * hard-coded strings which must not be translated into other languages.</br>
 * A common use of this class corresponds to the storage of resource identifications through the use of dedicated
 * sub-keys. See for example the use of the Icon.Normal sub-key described into the @href CATImplementHeaderResources
 * documentation.</br>
 * A resource catalog file value may be used to represent a directly resource (for example, a
 * workbench category, i.e. the place where a given
 * workbench appears in the menu tree). It may also be used to represent a reference to a resource (for example,
 * for an icon, you will put the icon file name).</br>
 * The class does not support control characters (for example \a, \n, \t).</br>
 * A value can be parameterized, ie contains parameters you
 * value at run-time. A parameter begins with /p or /P in the value.</br>
 * Here is an example of a resource catalog file content:
 * <pre>
 * Message1 = "This is a simple parameterized string.";
 * Message2 = "This parameterized string includes the parameter /P1 that
 *             is valued by your application at run-time.";
 * Message3 = "This is a", "compound ", "parameterized string.";</pre>
 * A resource catalog file posess the .CATRsc suffix and is retrieved from the folders specified by the
 * CATMsgCatalogPath variable. The localized version of the resource catalog is searched first. If it does not exist,
 * the english version is taken.</br>
 * Example: Suppose
 * <ul>
 * <li>we are on Windows</li>
 * <li>the environment is Japanese</li>
 * <li>the Tools\Customize\Options\"User Interface Language" combo is set to "Environment language (default)"</li>
 * </ul>
 * then, if the CATMsgCatalogPath variable is valued the following way:
 * <pre>   CATMsgCatalogPath=C:\PersonalFolder;C:\Program Files\Dassault Systemes\B14</pre>
 * CATIA will search the resource catalog file into the following paths:
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
//

class ExportedByNS0S1MSG CATRscCatalog
{
  public:
  // Constructors
  // ============

/**
 * Constructs a resource catalog.
 */
     CATRscCatalog();

/**
 * Copy constructor.</br>
 * <b>Caution</b>: This method should rarely be used, in most cases the use of 
 * @href CATRscCatalog#BuildResource is enough.
 * @param iRscCatalog
 *   Resource catalog to copy
 * 
 */
     CATRscCatalog(const CATRscCatalog &iRscCatalog);

     ~CATRscCatalog();

/**
 * Assignment operator.</br>
 * <b>Caution</b>: This method should rarely be used, in most cases the use of 
 * @href CATRscCatalog#BuildResource is enough.
 * @param iRscCatalog
 *   The resource catalog to be copied
 * @return  
 *   The resource catalog resulting from the assignment
 */
     CATRscCatalog &operator =(const CATRscCatalog &iRscCatalog) ;

/**
 * Loads a resource catalog by means of its name and (if desired)
 * of a path.</br>
 * <b>Caution</b>: This method should rarely be used, in most cases the use of 
 * @href CATRscCatalog#BuildResource is enough.
 * @param iCatalogName
 *   Name of the resource catalog file, without the .CATRsc suffix
 * @param iPath
 *    Absolute path where the catalog must be searched for.</br>
 *    The localized version of the resource catalog is searched first. If it does not exist,
 *    the english version is taken.</br>
 *    Example:
 *    In a japanese Windows environment, if the iPath variable is valued the following way:
 *    <pre>      iPath="C:\PersonalFolder;C:\Program Files\Dassault Systemes\B14"</pre>
 *    then, CATIA search the resource catalog file into the following paths:
 *    <pre>      1 - C:\PersonalFolder\Japanese
 *      2 - C:\PersonalFolder
 *      3 - C:\Program Files\Dassault Systemes\B14\Japanese
 *      4 - C:\Program Files\Dassault Systemes\B14</pre>
 * @return
 *    <br><b>Legal values</b>: <tt>0: False</tt> 
 *    if the resource catalog is not loaded, or <tt>Other: True</tt> 
 *    if the resource catalog is loaded.</br>
 * If the execution gives a 0 value, debug the resource catalog file using @href CATRscCatalog#GetError .
 */
     int LoadRscCatalog (const CATString &iCatalogName, 
                         const char *iPath = NULL);

/**
 * Returns the first syntax error found.
 * <br><b>Role</b>: Returns the first syntax error found after a call to @href CATRscCatalog#BuildResource
 * or @href CATRscCatalog#LoadRscCatalog .
 * See @href CATMsgCatalog#BuildResource and @href CATRscCatalog#LoadRscCatalog .
 * @return
 *   String containing the error message.
 */
     static const CATString GetError ();

/**
 * Computes a message from the resource catalog.
 * <br><b>Role</b>: Computes a message from the resource catalog using the message key.</br>
 * <b>Caution</b>: This method should rarely be used, in most cases the use of 
 * @href CATRscCatalog#BuildResource is enough.
 * @param iKey
 *   Key of the resource to be retrieved
 * @param oNonTranslatableMessage
 *   The computed output resource string.
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
 * @return
 *   <br><b>Legal values</b>: <tt>0: False</tt> 
 *   if a problem occured, or <tt>Other: True</tt> 
 *   otherwise.
 *   Use @href CATRscCatalog#GetError to retrieve the current error
 */
     int GetCatalogRsc( const CATString &iKey  , 
		        CATString &oNonTranslatableMessage     ,
		        CATString *iMsgParameters = NULL ,
		        int iParamNb = 0 ); 

/**
 * Computes a message from an identified non translatable parameterized message of the catalog, given specified
 * parameters values.
 * <br><b>Role</b>: Computes an alphanumeric string from a non transtable message from a resource catalog using the 
 * catalog name, the message key, the message parameters and (if needed) a default non translatable message.
 * <p>
 * The use of this service is as follows: call it at any moment
 * you want during execution. You do not have to
 * store the computed message for a future re-use. Effectively,
 * this service is encapsulated. It takes into account the 
 * following things:
 * <ul>
 * <li>If the catalog is not loaded, it does it (this 
 * enables the calling code not to bother about the loading:
 * it lets the managing of it to this service, the resource
 * catalog will be present into the memory only once,
 * whatever the count of softwares using it.</li>
 * <li>Otherwise, from the input resource catalog identifier 
 * (iCatalogName parameter, see below), the method find in its 
 * own data the resource catalog pointer, among the other 
 * resource catalog already loaded, through a method that 
 * has its performances optimized: a hashtable. This technology 
 * ensures there isn't any performance problem.</li>
 * </ul>
 * @param iCatalogName
 *   Name of the resource catalog file, without the .CATRsc suffix
 * @param iKey
 *   Key of the non translatable message to be retrieved
 * @param oNonTranslatableMessage
 *   The computed output resource string.
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
 *   Non translatable message to be used if a problem occured while
 *   accessing the resource catalog file or the key. You may, for
 *   example, put in this message an information about an
 *   access problem. 
 *   We suggest you to deliver a valid value to this parameter, so that,
 *   if an error occurs, a valid return value be delivered by
 *   the service. 
 * @return
 *   <br><b>Legal values</b>: <tt>0: False</tt> 
 *   if a problem has occured, or <tt>Other: True</tt> 
 *   otherwise.
 *   Use @href CATRscCatalog#GetError to retrieve the current error
 */
     static int BuildResource (const CATString &iCatalogName,
			       const CATString &iKey,
			       CATString &oNonTranslatableMessage,
			       CATString *iMsgParameters = NULL,
			       int iParamNb = 0,
			       const CATString &iDefaultMsg = (char*)NULL );

/**
 * Get the list of keys of the catalog.
 * @param oKey
 *   The key list. 
 * <br><b>Lifecycle rules deviation</b>: the caller must allocate the object and manage so that, in input,
 * the oKey size be equal to zero. 
 * @return
 *    Key count.
 */  
     int GetCatalogKeys ( CATListValCATString *oKey =  NULL );

/**
 * Unloads a resource catalog by means of its name.
 * @param iCatalogName
 *    Name of the resource catalog file, without the .CATRsc suffix
 * @return
 *   <br><b>Legal values</b>: <tt>0: False</tt> 
 *   if a problem has occured, or <tt>Other: True</tt> 
 *   otherwise.
 */
     static int UnloadRscCatalog ( const CATString &iCatalogName );

  private:
/** @nodoc */     
     CATInterRscCatalog *_CatalogRef;
};

#endif


