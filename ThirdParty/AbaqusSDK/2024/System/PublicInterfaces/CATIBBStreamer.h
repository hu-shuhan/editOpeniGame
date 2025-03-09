#ifndef CATIBBStreamer_H
#define CATIBBStreamer_H
#include "CATBaseUnknown.h"
#include "CATSysCommunication.h"
#include <IUnknown.h>

// COPYRIGHT DASSAULT SYSTEMES 2000

//=======================================================================
//
//=======================================================================
/**
 * @CAA2Level L1
 * @CAA2Usage U3
 */


#ifndef LOCAL_DEFINITION_FOR_IID
extern   ExportedByCATSysCommunication   IID IID_CATIBBStreamer;
#else
extern "C" const IID IID_CATIBBStreamer;
#endif


/**
 * Interface to stream and unstream backbone messages.
 * <b>Role:</b> CATIBBStreamer helps the implementation of the 
 * StreamData and UnstreamData methods of the @href CATIStreamMsg interface.
 * It provides streaming and unstreaming methods for the main simple data types.
 * <p>CATIBBStreamer is implemented by the supplied @href CATBBMessage component. 
 * Since your message component derives from this component, it inherits
 * the implementation of CATIBBStreamer.
 */
class    ExportedByCATSysCommunication CATIBBStreamer : public CATBaseUnknown
{
  public:
   /**
   * @nodoc
   */
  CATDeclareInterface;

  /**
   * Initializes a backbone message streaming operation.
   * <br><b>Role:</b> Mandatory call at the beginning of a streaming operation.
   * <tt>BeginStream</tt> must be called at the beginning of a streaming operation
   * to set or reset the streamed buffer.
   */
  virtual HRESULT BeginStream()=0 ;
  /**
   * Streams a char.
   *   @param iVal
   *       the char to stream
   */
  virtual HRESULT StreamByte(char  iVal)=0;

  /**
   * Streams a short.
   * @param iShort
   *   The short to stream
   */
  virtual HRESULT StreamShort(short iVal)=0;

  /**
   * Streams an unsigned short.
   * @param iUnsignedShort
   *   The unsigned short to stream
   */
  virtual HRESULT StreamUnsignedShort( unsigned short iVal)=0;

  /**
   * Streams an int.
   * @param iInt
   *   The int to stream
   */
  virtual HRESULT StreamInt(int iInt)=0;

  /**
   * Streams an unsigned int.
   * @param iUnsignedInt
   *   The unsigned int to stream
   */
  virtual HRESULT StreamUnsigned(unsigned int iUnsignedInt)=0;
 
  /**
   * Streams a float.
   * @param iFloat
   *   The float to stream
   */
  virtual HRESULT StreamFloat(float iFloat)=0;
 
  /**
   * Streams a double.
   * @param iDouble
   *   The double to stream
   */
  virtual HRESULT StreamDouble(double iDouble)=0;
 
  /**
   * Streams a string.
   * @param iString
   *   The string to stream
   */
  virtual HRESULT StreamString(const char *iString)=0;

  /**
   * Streams a fixed size array of bytes.
   * @param iTab  
   *   The array to stream
   * @param iLength
   *   The size of the array to stream
   */
  virtual HRESULT StreamFixedByteArray(const void *iTab, int iLength)=0;

  /**
   * Streams a variable size array of bytes.
   * @param iTab  
   *   The array to stream
   * @param iLength  
   *   The size of the array to stream
   */
  virtual HRESULT StreamVariableByteArray(const void *iTab, int iLength)=0;

  /**
   * Streams a fixed size array of int.
   * @param iTab  
   *   The array to stream
   * @param iLength  
   *   The size of the array to stream
   */
  virtual HRESULT StreamFixedIntArray(const int iTab[], int iLength)=0;

  /**
   * Streams a variable size array of int. 
   * @param iTab  
   *   The array of int to stream
   * @param iLength  
   *   The size of the array to stream
   */
  virtual HRESULT StreamVariableIntArray(const int iTab[], int iLength)=0;

  /**
   * Streams a fixed size array of unsigned int.
   * @param iTab  
   *   The array to stream
   * @param iLength  
   *   The size of the array to stream
   */
  virtual HRESULT StreamFixedUnsignedArray(const unsigned int iTab[], int iLength)=0;

  /**
   * Streams a variable size array of unsigned int.
   * @param iTab  
   *   The array to stream
   * @param iLength  
   *   The size of the array to stream
   */
  virtual HRESULT StreamVariableUnsignedArray(const  unsigned int iTab[], int iLength)=0;

  /**
   * Streams a fixed size array of short.
   * @param iTab  
   *   The array to stream
   * @param iLength  
   *   The size of the array to stream
   */
  virtual HRESULT StreamFixedShortArray(const short iTab[], int iLength)=0;

  /**
   * Streams a variable size array of short. 
   * @param iTab  
   *   The array to stream
   * @param iLength  
   *   The size of the array to stream
   */
  virtual HRESULT StreamVariableShortArray(const short iTab[], int iLength)=0;

  /**
   * Streams a fixed size array of unsigned short.
   * @param iTab  
   *   The array to stream
   * @param iLength  
   *   The size of the array to stream
   */
  virtual HRESULT StreamFixedUnsignedShortArray(const unsigned short iTab[], int iLength)=0;

  /**
   * Streams  variable size array of unsigned short. 
   * @param iTab  
   *   The array to stream
   * @param iLength  
   *   The size of the array to stream
   */
  virtual HRESULT StreamVariableUnsignedShortArray(const unsigned short iTab[], int iLength)=0;

  /**
   * Streams  fixed size array of float.
   * @param iTab  
   *   The array to stream
   * @param iLength  
   *   The size of the array to stream
   */
  virtual HRESULT StreamFixedFloatArray(const float iTab[], int iLength)=0;

  /**
   * Streams  variable size array of float.
   * @param iTab  
   *   The array to stream
   * @param iLength  
   *   The size of the array to stream
   */
  virtual HRESULT StreamVariableFloatArray(const float iTab[], int iLength)=0;

  /**
   * Streams  fixed size array of doubles.
   * @param iTab  
   *   The array to stream
   * @param iLength  
   *   The size of the array to stream
   */
  virtual HRESULT StreamFixedDoubleArray(const double iTab[], int iLength)=0;

  /**
   * Streams  variable size array of doubles.
   * @param iTab  
   *   The array to stream
   * @param iLength  
   *   The size of the array to stream
   */
  virtual HRESULT StreamVariableDoubleArray(const double iTab[], int iLength)=0;

  /**
   * @nodoc
   * Streams an UUID .
   */
  //virtual HRESULT StreamUUID()=0;

  /**
   * Ends a backbone message streaming operation and returns the streamed buffer.
   * <br><b>Role:</b> Mandatory call at the end of a streaming operation.
   * <tt>EndStream</tt> must be called at the end of a streaming operation
   * to close the streamed buffer and to retrieve it.
   * @param oLength
   *   The length of the streamed buffer
   * @return 
   *   The streamed buffer 
   */
  virtual void * EndStream(int *oLength)=0;

  /**
   * Frees the streamed buffer created by a backbone message streaming operation. 
   * <br><b>Role:</b> Once the backbone message is streamed and sent, the buffer
   * that contains the streamed message becomes of no use and must be freed
   * using <tt>ResetStreamData</tt>.
   */
   virtual HRESULT ResetStreamData()=0;

  /**
   * Begins a backbone message unstreaming operation.
   * <br><b>Role:</b> Mandatory call at the beginning of an unstreaming operation.
   * <tt>BeginUnstream</tt> must be called at the beginning of an unstreaming operation
   * to designate the streamed buffer to unstream.
   * @param iData
   *    The buffer of data to unstream
   * @param iLength
   *    The length of the buffer to unstream expressed in bytes
   */
  virtual HRESULT BeginUnstream(void *iData, int iLength)=0;

  /**
   * Unstreams a char.
   * @param oChar
   *   The unstreamed char
   */
  virtual HRESULT UnstreamByte(char  *oChar)=0;
  /**
   * Unstreams a short.
   * @param oShort
   *   The unstreamed short
   */
  virtual HRESULT UnstreamShort(short *oShort)=0;
  /**
   * Unstreams an unsigned short.
   * @param oUnsignedShort
   *   The unstreamed  unsigned short
   */
  virtual HRESULT UnstreamUnsignedShort(unsigned short *oUnsignedShort)=0;
  /**
   * Unstreams an int.
   * @param oInt
   *   The unstreamed int
   */
  virtual HRESULT UnstreamInt(int *oInt)=0;
  /**
   * Unstreams an unsigned int.
   * @param oUnsignedInt
   *   The unstreamed unsigned int
   */
  virtual HRESULT UnstreamUnsigned(unsigned int *oUnsignedInt)=0;
  /**
   * Unstreams a float.
   * @param oFloat
   *   The unstreamed float
   */
  virtual HRESULT UnstreamFloat(float *oFloat)=0;
  /**
   * Unstreams a double.
   * @param oDouble
   *   The unstreamed double
   */
  virtual HRESULT UnstreamDouble(double *oDouble)=0;
  /**
   * Unstreams a string.
   * @param oString
   *   The unstreamed string
   */
  virtual HRESULT UnstreamString(char *oString)=0;

  /**
   * Unstreams the size of a variable size array.
   * <br><b>Role</b>: A variable size array can be unstreamed as a fixed-size array
   * as soon as its size is known.
   * Before calling the appropriate unstreaming method for a fixed-size array (<tt>UnstreamFixed(type)Array</tt>),
   * <tt>UnstreamVariableArrayLength</tt> enables you to retrieve the array size and
   * to allocate the array accordingly.
   * @param oLength
   *  The length (number of elements) of the array to unstream.
   */
  virtual HRESULT UnstreamVariableArrayLength(int *oLength)=0;

  /**
   * Unstreams a fixed size byte array.
   * @param oByteArray
   *   The unstreamed array
   * @param iLength
   *   The length of the fixed size array
   */
  virtual HRESULT UnstreamFixedByteArray( void *oByteArray, int iLength)=0;


  /**
   * Unstreams the length of a string.
   * @param oLength
   *   The length of the string to unstream
   */
  virtual HRESULT UnstreamNeededStringLength(int *oLength)=0;

  /**
   * Unstreams a fixed float array.
   * @param iTab
   *   The array to fill with floats
   * @param iLength
   *   The length of the fixed size array (or obtained using @href #UnstreamVariableArrayLength )
   */
  virtual HRESULT UnstreamFixedFloatArray(float iTab[], int iLength)=0;

  /**
   * Unstreams a fixed double array.
   * @param iTab
    *  The array to fill with doubles
   * @param iLength
   *   The length of the fixed size array (or obtained using @href #UnstreamVariableArrayLength )
   */
  virtual HRESULT UnstreamFixedDoubleArray(double iTab[], int iLength)=0;


  /**
   * Unstreams a fixed short array.
   * @param iTab
   *   The array to fill with shorts
   * @param iLength
   *   The length of the fixed size array (or obtained using @href #UnstreamVariableArrayLength )
   */
  virtual HRESULT UnstreamFixedShortArray(short iTab[], int iLength)=0;


  /**
   * Unstreams a fixed unsigned short array.
   * @param iTab
   *   The array to fill with unsigned shorts
   * @param iLength
   *   The length of the fixed size array (or obtained using @href #UnstreamVariableArrayLength )
   */
  virtual HRESULT UnstreamFixedUnsignedShortArray(unsigned short iTab[], int iLength)=0;

  /**
   * Unstreams a fixed int array.
   * @param iTab
   *   The array to fill with ints
   * @param iLength
   *   The length of the fixed size array (or obtained using @href #UnstreamVariableArrayLength )
   */
  virtual HRESULT UnstreamFixedIntArray(int iTab[], int iLength) =0;

  /**
   * Unstreams a fixed size array of unsigned int.
   * @param iTab
   *   The array to fill with unsigned ints
   * @param iLength
   *   The length of the fixed size array (or obtained using @href #UnstreamVariableArrayLength )
   */
  virtual HRESULT UnstreamFixedUnsignedArray(unsigned iTab[], int iLength)=0;

  //virtual HRESULT UnstreamUUID()=0;

  /**
   * Ends a backbone message unstreaming operation.
   * <br><b>Role:</b> Mandatory call at the end of an unstreaming operation.
   */
  virtual HRESULT EndUnstream()=0;
};

/**
 * @nodoc 
 * Declaration of the handler class  for CATIStreamMsg : CATIStreamMsg_var
 */
CATDeclareHandler( CATIBBStreamer, CATBaseUnknown );


#endif
