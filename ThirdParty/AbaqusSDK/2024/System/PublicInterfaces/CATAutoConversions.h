#ifndef CATAutoConversions_h
#define CATAutoConversions_h

// COPYRIGHT DASSAULT SYSTEMES 2000

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

#include "JS0DSPA.h"
#include "CATSafeArray.h"
#include "CATVariant.h"
#include "CATCORBAAny.h"
#include "CATUnicodeString.h"
#include "CATBoolean.h"

//-----------------------------------------------------------------------------
//  ConvertSafeArrayVariant:
//-----------------------------------------------------------------------------

/**
 * Converts a CATSafeArrayVariant into a C++ array of shorts.
 * @param iSafeArray
 * the array to convert
 * @param ioShortArray
 * a pointer to a C++ array of <tt>shorts</tt> large enough to
 * contain all the converted values.
 * <br><b>Legal values:</b>
 * <dl>
 * <dt><tt>NULL</tt><dd>The function will simply return the size of the 
 *  <tt>CATSafeArrayVariant</tt> without converting anything. This size
 *  can later be used to allocate the C++ array to the proper size.</dd></dt>
 * <dt>valid non <tt>NULL</tt> pointer<dd> The function will convert the
 *  <tt>CATSafeArrayVariant</tt> and put the results in the array.</dd></dt>
 * </dl>
 * @param iSize
 * the number of elements to convert.
 * <br><b>Legal values:</b>Must be smaller than or equal to the
 * size of <tt>iSafeArray</tt>.
 * @return
 * the number of converted elements if <tt>ioShortArray</tt> is not NULL,
 * or the size of <tt>iSafeArray</tt> otherwise.
 */
ExportedByJS0DSPA long ConvertSafeArrayVariant(const CATSafeArrayVariant* iSafeArray, 
					       short * ioShortArray,
					       long iSize = 0);

/**
 * Converts a CATSafeArrayVariant into a C++ array of longs.
 * @param iSafeArray
 * the array to convert
 * @param ioLongArray
 * a pointer to a C++ array of <tt>longs</tt> large enough to
 * contain all the converted values.
 * <br><b>Legal values:</b>
 * <dl>
 * <dt><tt>NULL</tt><dd>The function will simply return the size of the 
 *  <tt>CATSafeArrayVariant</tt> without converting anything. This size
 *  can later be used to allocate the C++ array to the proper size.</dd></dt>
 * <dt>valid non <tt>NULL</tt> pointer<dd> The function will convert the
 *  <tt>CATSafeArrayVariant</tt> and put the results in the array.</dd></dt>
 * </dl>
 * @param iSize
 * the number of elements to convert.
 * <br><b>Legal values:</b>Must be smaller than or equal to the
 * size of <tt>iSafeArray</tt>.
 * @return
 * the number of converted elements if <tt>ioLongArray</tt> is not NULL,
 * or the size of <tt>iSafeArray</tt> otherwise.
 */
ExportedByJS0DSPA long ConvertSafeArrayVariant(const CATSafeArrayVariant* iSafeArray, 
					       long * ioLongArray,
					       long iSize = 0);

/**
 * Converts a CATSafeArrayVariant into a C++ array of floats.
 * @param iSafeArray
 * the array to convert
 * @param ioFloatArray
 * a pointer to a C++ array of <tt>floats</tt> large enough to
 * contain all the converted values.
 * <br><b>Legal values:</b>
 * <dl>
 * <dt><tt>NULL</tt><dd>The function will simply return the size of the 
 *  <tt>CATSafeArrayVariant</tt> without converting anything. This size
 *  can later be used to allocate the C++ array to the proper size.</dd></dt>
 * <dt>valid non <tt>NULL</tt> pointer<dd> The function will convert the
 *  <tt>CATSafeArrayVariant</tt> and put the results in the array.</dd></dt>
 * </dl>
 * @param iSize
 * the number of elements to convert.
 * <br><b>Legal values:</b>Must be smaller than or equal to the
 * size of <tt>iSafeArray</tt>.
 * @return
 * the number of converted elements if <tt>ioFloatArray</tt> is not NULL,
 * or the size of <tt>iSafeArray</tt> otherwise.
 */
ExportedByJS0DSPA long ConvertSafeArrayVariant(const CATSafeArrayVariant* iSafeArray, 
					       float * ioFloatArray,
					       long iSize = 0);

/**
 * Converts a CATSafeArrayVariant into a C++ array of doubles.
 * @param iSafeArray
 * the array to convert
 * @param ioDoubleArray
 * a pointer to a C++ array of <tt>doubles</tt> large enough to
 * contain all the converted values.
 * <br><b>Legal values:</b>
 * <dl>
 * <dt><tt>NULL</tt><dd>The function will simply return the size of the 
 *  <tt>CATSafeArrayVariant</tt> without converting anything. This size
 *  can later be used to allocate the C++ array to the proper size.</dd></dt>
 * <dt>valid non <tt>NULL</tt> pointer<dd> The function will convert the
 *  <tt>CATSafeArrayVariant</tt> and put the results in the array.</dd></dt>
 * </dl>
 * @param iSize
 * the number of elements to convert.
 * <br><b>Legal values:</b>Must be smaller than or equal to the
 * size of <tt>iSafeArray</tt>.
 * @return
 * the number of converted elements if <tt>ioDoubleArray</tt> is not NULL,
 * or the size of <tt>iSafeArray</tt> otherwise.
 */
ExportedByJS0DSPA long ConvertSafeArrayVariant(const CATSafeArrayVariant* iSafeArray, 
					       double * ioDoubleArray, 
					       long iSize = 0);

/**
 * Converts a CATSafeArrayVariant into a C++ array of CATBoolean.
 * @param iSafeArray
 * the array to convert
 * @param ioBooleanArray
 * a pointer to a C++ array of <tt>CATBooleans</tt> large enough to
 * contain all the converted values.
 * <br><b>Legal values:</b>
 * <dl>
 * <dt><tt>NULL</tt><dd>The function will simply return the size of the 
 *  <tt>CATSafeArrayVariant</tt> without converting anything. This size
 *  can later be used to allocate the C++ array to the proper size.</dd></dt>
 * <dt>valid non <tt>NULL</tt> pointer<dd> The function will convert the
 *  <tt>CATSafeArrayVariant</tt> and put the results in the array.</dd></dt>
 * </dl>
 * @param iSize
 * the number of elements to convert.
 * <br><b>Legal values:</b>Must be smaller than or equal to the
 * size of <tt>iSafeArray</tt>.
 * @return
 * the number of converted elements if <tt>ioBooleanArray</tt> is not NULL,
 * or the size of <tt>iSafeArray</tt> otherwise.
 */
ExportedByJS0DSPA long ConvertSafeArrayVariant(const CATSafeArrayVariant* iSafeArray, 
					       CATBoolean * ioBooleanArray,
					       long iSize = 0);

/**
 * Converts a CATSafeArrayVariant into a C++ array of CATUnicodeStrings.
 * @param iSafeArray
 * the array to convert
 * @param ioStringArray
 * a pointer to a C++ array of <tt>CATUnicodeStrings</tt> large enough to
 * contain all the converted values.
 * <br><b>Legal values:</b>
 * <dl>
 * <dt><tt>NULL</tt><dd>The function will simply return the size of the 
 *  <tt>CATSafeArrayVariant</tt> without converting anything. This size
 *  can later be used to allocate the C++ array to the proper size.</dd></dt>
 * <dt>valid non <tt>NULL</tt> pointer<dd> The function will convert the
 *  <tt>CATSafeArrayVariant</tt> and put the results in the array.</dd></dt>
 * </dl>
 * @param iSize
 * the number of elements to convert.
 * <br><b>Legal values:</b>Must be smaller than or equal to the
 * size of <tt>iSafeArray</tt>.
 * @return
 * the number of converted elements if <tt>ioStringArray</tt> is not NULL,
 * or the size of <tt>iSafeArray</tt> otherwise.
 */
ExportedByJS0DSPA long ConvertSafeArrayVariant(const CATSafeArrayVariant* iSafeArray, 
					       CATUnicodeString * ioStringArray, 
					       long iSize = 0);

/**
 * Converts a CATSafeArrayVariant into a C++ array of char*.
 * @param iSafeArray
 * the array to convert
 * @param ioStringArray
 * a pointer to a C++ array of <tt>char*</tt> large enough to
 * contain all the converted values.
 * <br><b>Legal values:</b>
 * <dl>
 * <dt><tt>NULL</tt><dd>The function will simply return the size of the 
 *  <tt>CATSafeArrayVariant</tt> without converting anything. This size
 *  can later be used to allocate the C++ array to the proper size.</dd></dt>
 * <dt>valid non <tt>NULL</tt> pointer<dd> The function will convert the
 *  <tt>CATSafeArrayVariant</tt> and put the results in the array.</dd></dt>
 * </dl>
 * @param iSize
 * the number of elements to convert.
 * <br><b>Legal values:</b>Must be smaller than or equal to the
 * size of <tt>iSafeArray</tt>.
 * @return
 * the number of converted elements if <tt>ioStringArray</tt> is not NULL,
 * or the size of <tt>iSafeArray</tt> otherwise.
 */
ExportedByJS0DSPA long ConvertSafeArrayVariant(const CATSafeArrayVariant* iSafeArray, 
					       char ** ioStringArray, 
					       long iSize = 0);

/**
 * Converts a CATSafeArrayVariant into a C++ array of CATBaseDispatch.
 * interfaces.
 * @param iSafeArray
 * the array to convert
 * @param ioObjectArray
 * a pointer to a C++ array of <tt>CATBaseDispatch</tt> interfaces large enough to
 * contain all the converted values.
 * <br><b>Legal values:</b>
 * <dl>
 * <dt><tt>NULL</tt><dd>The function will simply return the size of the 
 *  <tt>CATSafeArrayVariant</tt> without converting anything. This size
 *  can later be used to allocate the C++ array to the proper size.</dd></dt>
 * <dt>valid non <tt>NULL</tt> pointer<dd> The function will convert the
 *  <tt>CATSafeArrayVariant</tt> and put the results in the array.</dd></dt>
 * </dl>
 * <br><b>Lifecycle rules deviation:</b>the reference count of the 
 * <tt>CATBaseDispatch</tt> objects returned in the C++ array is not
 * altered, so there is no need to call <tt>Release</tt> on these objects
 * after the <tt>ConvertSafeArrayVariant</tt> call.
 * @param iSize
 * the number of elements to convert.
 * <br><b>Legal values:</b>Must be smaller than or equal to the
 * size of <tt>iSafeArray</tt>.
 * @return
 * the number of converted elements if <tt>ioObjectArray</tt> is not NULL,
 * or the size of <tt>iSafeArray</tt> otherwise.
 */
ExportedByJS0DSPA long ConvertSafeArrayVariant(const CATSafeArrayVariant* iSafeArray, 
					       CATBaseDispatch ** ioObjectArray, 
					       long iSize = 0);






//-----------------------------------------------------------------------------
//  BuildSafeArrayVariant:
//-----------------------------------------------------------------------------
/**
 * Creates a new CATSafeArrayVariant and initializes it with
 * a C++ array of shorts.
 * <br> Use the global function @href FreeVariantSafeArray to free
 * the <tt>CATSafeArrayVariant</tt> created.
 * @param iShortArray
 * a pointer to a C++ array of <tt>shorts</tt>.
 * @param iSize
 * the size of the <tt>short</tt> array.
 * @return
 * a pointer to an initialized <tt>CATSafeArrayVariant</tt>.
 */
ExportedByJS0DSPA CATSafeArrayVariant* BuildSafeArrayVariant(const short * iShortArray, 
							     long iSize);

/**
 * Creates a new CATSafeArrayVariant and initializes it with
 * a C++ array of longs.
 * <br> Use the global function @href FreeVariantSafeArray to free
 * the <tt>CATSafeArrayVariant</tt> created.
 * @param iLongArray
 * a pointer to a C++ array of <tt>longs</tt>.
 * @param iSize
 * the size of the <tt>long</tt> array.
 * @return
 * a pointer to an initialized <tt>CATSafeArrayVariant</tt>.
 */
ExportedByJS0DSPA CATSafeArrayVariant* BuildSafeArrayVariant(const long * iLongArray, 
							     long iSize);

/**
 * Creates a new CATSafeArrayVariant and initializes it with
 * a C++ array of floats.
 * @param iFloatArray
 * a pointer to a C++ array of <tt>floats</tt>.
 * @param iSize
 * the size of the <tt>float</tt> array.
 * @return
 * a pointer to an initialized <tt>CATSafeArrayVariant</tt>.
 */
ExportedByJS0DSPA CATSafeArrayVariant* BuildSafeArrayVariant(const float * iFloatArray, 
							     long iSize);

/**
 * Creates a new CATSafeArrayVariant and initializes it with
 * a C++ array of doubles.
 * <br> Use the global function @href FreeVariantSafeArray to free
 * the <tt>CATSafeArrayVariant</tt> created.
 * @param iDoubleArray
 * a pointer to a C++ array of <tt>doubles</tt>.
 * @param iSize
 * the size of the <tt>double</tt> array.
 * @return
 * a pointer to an initialized <tt>CATSafeArrayVariant</tt>.
 */
ExportedByJS0DSPA CATSafeArrayVariant* BuildSafeArrayVariant(const double * iDoubleArray, 
							     long iSize);

/**
 * Creates a new CATSafeArrayVariant and initializes it with
 * a C++ array of CATBooleans.
 * <br> Use the global function @href FreeVariantSafeArray to free
 * the <tt>CATSafeArrayVariant</tt> created.
 * @param iBooleanArray
 * a pointer to a C++ array of boolean
 * @param iSize
 * the size of the <tt>CATBoolean</tt> array.
 * @return
 * a pointer to an initialized <tt>CATSafeArrayVariant</tt>.
 */
ExportedByJS0DSPA CATSafeArrayVariant* BuildSafeArrayVariant(const CATBoolean * iBooleanArray, 
							     long iSize);

/**
 * Creates a new CATSafeArrayVariant and initializes it with
 * a C++ array of CATUnicodeStrings.
 * <br> Use the global function @href FreeVariantSafeArray to free
 * the <tt>CATSafeArrayVariant</tt> created.
 * @param iStringArray
 * a pointer to a C++ array of <tt>CATUnicodeStrings</tt>.
 * @param iSize
 * the size of the <tt>CATUnicodeString</tt> array.
 * @return
 * a pointer to an initialized <tt>CATSafeArrayVariant</tt>.
 */
ExportedByJS0DSPA CATSafeArrayVariant* BuildSafeArrayVariant(const CATUnicodeString * iStringArray, 
							     long iSize);

/**
 * Creates a new CATSafeArrayVariant and initializes it with
 * a C++ array of char*.
 * <br> Use the global function @href FreeVariantSafeArray to free
 * the <tt>CATSafeArrayVariant</tt> created.
 * @param iStringArray
 * a pointer to a C++ array of <tt>char*</tt>.
 * @param iSize
 * the size of the <tt>char*</tt> array.
 * @return
 * a pointer to an initialized <tt>CATSafeArrayVariant</tt>.
 */
ExportedByJS0DSPA CATSafeArrayVariant* BuildSafeArrayVariant(const char ** iStringArray, 
							     long iSize);

/**
 * Creates a new CATSafeArrayVariant and initializes it with
 * a C++ array of CATBaseDispatch.
 * <br> Use the global function @href FreeVariantSafeArray to free
 * the <tt>CATSafeArrayVariant</tt> created.
 * @param iObjectArray
 * a pointer to a C++ array of <tt>CATBaseDispatch</tt>.
 * @param iSize
 * the size of the <tt>CATBaseDispatch</tt> array.
 * @return
 * a pointer to an initialized <tt>CATSafeArrayVariant</tt>.
 */
ExportedByJS0DSPA CATSafeArrayVariant* BuildSafeArrayVariant(const CATBaseDispatch ** iObjectArray, 
							     long iSize);





//-----------------------------------------------------------------------------
//  FillSafeArrayVariant:
//-----------------------------------------------------------------------------

/**
 * Fills an existing CATSafeArrayVariant with
 * a C++ array of shorts.
 * @param iShortArray
 * a pointer to a C++ array of <tt>shorts</tt>.
 * @param iSize
 * the size of the <tt>short</tt> array.
 */
ExportedByJS0DSPA HRESULT FillSafeArrayVariant(CATSafeArrayVariant * SafeArray, 
					       const short * iShortArray, 
					       long iSize);

/**
 * Fills an existing CATSafeArrayVariant with
 * a C++ array of <tt>longs</tt>.
 * @param iLongArray
 * a pointer to a C++ array of <tt>longs</tt>.
 * @param iSize
 * the size of the <tt>long</tt> array.
 */
ExportedByJS0DSPA HRESULT FillSafeArrayVariant(CATSafeArrayVariant * SafeArray, 
					       const long * iLongArray, 
					       long iSize);

/**
 * Fills an existing CATSafeArrayVariant with
 * a C++ array of <tt>floats</tt>.
 * @param iFloatArray
 * a pointer to a C++ array of <tt>floats</tt>.
 * @param iSize
 * the size of the <tt>float</tt> array.
 */
ExportedByJS0DSPA HRESULT FillSafeArrayVariant(CATSafeArrayVariant * SafeArray, 
					       const float * iFloatArray, 
					       long iSize);

/**
 * Fills an existing CATSafeArrayVariant with
 * a C++ array of <tt>doubles</tt>.
 * @param iDoubleArray
 * a pointer to a C++ array of <tt>doubles</tt>.
 * @param iSize
 * the size of the <tt>double</tt> array.
 */
ExportedByJS0DSPA HRESULT FillSafeArrayVariant(CATSafeArrayVariant * SafeArray, 
					       const double * iDoubleArray, 
					       long iSize);

/**
 * Fills an existing CATSafeArrayVariant with
 * a C++ array of CATBooleans.
 * @param iBooleanArray
 * a pointer to a C++ array of <tt>CATBooleans</tt>.
 * @param iSize
 * the size of the <tt>CATBoolean</tt> array.
 */
ExportedByJS0DSPA HRESULT FillSafeArrayVariant(CATSafeArrayVariant * SafeArray, 
					       const CATBoolean * iBooleanArray, 
					       long iSize);

/**
 * Fills an existing CATSafeArrayVariant with
 * a C++ array of CATUnicodeStrings.
 * @param iStringArray
 * a pointer to a C++ array of <tt>CATUnicodeStrings</tt>.
 * @param iSize
 * the size of the <tt>CATUnicodeString</tt> array.
 */
ExportedByJS0DSPA HRESULT FillSafeArrayVariant(CATSafeArrayVariant * SafeArray, 
					       const CATUnicodeString * iStringArray, 
					       long iSize);

/**
 * Fills an existing CATSafeArrayVariant with
 * a C++ array of char*.
 * @param iStringArray
 * a pointer to a C++ array of <tt>char*</tt>.
 * @param iSize
 * the size of the <tt>char*</tt> array.
 */
ExportedByJS0DSPA HRESULT FillSafeArrayVariant(CATSafeArrayVariant * SafeArray, 
					       const char ** iStringArray, 
					       long iSize);

/**
 * Fills an existing CATSafeArrayVariant with
 * a C++ array of CATBaseDispatch.
 * @param iObjectArray
 * a pointer to a C++ array of <tt>CATBaseDispatch</tt>.
 * @param iSize
 * the size of the <tt>CATBaseDispatch</tt> array.
 */
ExportedByJS0DSPA HRESULT FillSafeArrayVariant(CATSafeArrayVariant * SafeArray, 
					       const CATBaseDispatch ** iObjectArray, 
					       long iSize);




//-----------------------------------------------------------------------------
//  TransferSafeArrayVariant:
//-----------------------------------------------------------------------------
/**
 * Copy a CATSafeArrayVariant into another existing CATSafeArrayVariant. 
 * The two CATSafeArrayVariant must have the same size.
 * @param iSource
 * The source <tt>CATSafeArrayVariant</tt>.
 * @param ioDest
 * The destination <tt>CATSafeArrayVariant</tt>.
 */
ExportedByJS0DSPA HRESULT TransferSafeArrayVariant(const CATSafeArrayVariant * iSource, 
						         CATSafeArrayVariant & ioDest);




//-----------------------------------------------------------------------------
//  ConvertVariant:
//-----------------------------------------------------------------------------
/**
 * Converts a CATVariant into a C++ short.
 * @param iVar
 * the CATVariant to convert
 * @param oShort
 * a C++ <tt>short</tt> where the converted value will be stored.
 * @return
 * <tt>S_OK</tt> upon success, <tt>E_FAIL</tt> if the CATVariant
 * did not actually contain a <tt>short</tt>.
 */
ExportedByJS0DSPA HRESULT ConvertVariant(const CATVariant & iVar, 
					 short & oShort);

/**
 * Converts a CATVariant into a C++ <tt>long</tt>.
 * @param iVar
 * the CATVariant to convert
 * @param oLong
 * a C++ <tt>long</tt> where the converted value will be stored.
 * @return
 * <tt>S_OK</tt> upon success, <tt>E_FAIL</tt> if the CATVariant
 * did not actually contain a <tt>long</tt>.
 */
ExportedByJS0DSPA HRESULT ConvertVariant(const CATVariant & iVar, 
					 long & oLong);

/**
 * Converts a CATVariant into a C++ <tt>float</tt>.
 * @param iVar
 * the CATVariant to convert
 * @param oFloat
 * a C++ <tt>float</tt> where the converted value will be stored.
 * @return
 * <tt>S_OK</tt> upon success, <tt>E_FAIL</tt> if the CATVariant
 * did not actually contain a <tt>float</tt>.
 */
ExportedByJS0DSPA HRESULT ConvertVariant(const CATVariant & iVar, 
					 float & oFloat);

/**
 * Converts a CATVariant into a C++ double.
 * @param iVar
 * the CATVariant to convert
 * @param oDouble
 * a C++ <tt>double</tt> where the converted value will be stored.
 * @return
 * <tt>S_OK</tt> upon success, <tt>E_FAIL</tt> if the CATVariant
 * did not actually contain a <tt>double</tt>.
 */
ExportedByJS0DSPA HRESULT ConvertVariant(const CATVariant & iVar, 
					 double & oDouble);

/**
 * Converts a CATVariant into a C++ CATBoolean.
 * @param iVar
 * the CATVariant to convert
 * @param oBoolean
 * a C++ <tt>CATBoolean</tt> where the converted value will be stored.
 * @return
 * <tt>S_OK</tt> upon success, <tt>E_FAIL</tt> if the CATVariant
 * did not actually contain a <tt>CATBoolean</tt>.
 */
ExportedByJS0DSPA HRESULT ConvertVariant(const CATVariant & iVar, 
					 CATBoolean & oBoolean);

/**
 * Converts a CATVariant into a C++ CATUnicodeString.
 * @param iVar
 * the CATVariant to convert
 * @param oString
 * a C++ <tt>CATUnicodeString</tt> where the converted value will be stored.
 * @return
 * <tt>S_OK</tt> upon success, <tt>E_FAIL</tt> if the CATVariant
 * did not actually contain a <tt>CATBSTR</tt>.
 */
ExportedByJS0DSPA HRESULT ConvertVariant(const CATVariant & iVar, 
					 CATUnicodeString & oString);

/**
 * Converts a CATVariant into a C++ char*.
 * @param iVar
 * the CATVariant to convert
 * @param oString
 * a C++ <tt>char*</tt> where the converted value will be stored.
 * @return
 * <tt>S_OK</tt> upon success, <tt>E_FAIL</tt> if the CATVariant
 * did not actually contain a <tt>CATBSTR</tt>.
 */
ExportedByJS0DSPA HRESULT ConvertVariant(const CATVariant & iVar, 
					 char *& oString);

/**
 * Converts a CATVariant into a C++ CATBaseDispatch.
 * @param iVar
 * the CATVariant to convert
 * @param oObject
 * a C++ <tt>CATBaseDispatch</tt> pointer where the converted value will be stored.
 * <br><b>Lifecycle rules deviation:</b>the reference count of the 
 * <tt>CATBaseDispatch</tt> object returned is not
 * altered, so there is no need to call <tt>Release</tt> on this object
 * after the <tt>ConvertVariant</tt> call.
 * @return
 * <tt>S_OK</tt> upon success, <tt>E_FAIL</tt> if the CATVariant
 * did not actually contain a <tt>CATBaseDispatch</tt>.
 */
ExportedByJS0DSPA HRESULT ConvertVariant(const CATVariant & iVar, 
					 CATBaseDispatch *& oObject);





//-----------------------------------------------------------------------------
//  BuildVariant:
//-----------------------------------------------------------------------------

/**
 * Initializes a CATVariant with a C++ short.
 * @param iShort
 * a  C++ <tt>short</tt>.
 * @param ioVar
 * the CATVariant to initialize.
 */
ExportedByJS0DSPA HRESULT BuildVariant(const short & iShort, 
				       CATVariant & ioVar);

/**
 * Initializes a CATVariant with a C++ long.
 * @param iLong
 * a  C++ <tt>long</tt>.
 * @param ioVar
 * the CATVariant to initialize.
 */
ExportedByJS0DSPA HRESULT BuildVariant(const long & iLong, 
				       CATVariant & ioVar);

/**
 * Initializes a CATVariant with a C++ float.
 * @param iFloat
 * a  C++ <tt>float</tt>.
 * @param ioVar
 * the CATVariant to initialize.
 */
ExportedByJS0DSPA HRESULT BuildVariant(const float & iFloat, 
				       CATVariant & ioVar);

/**
 * Initializes a CATVariant with a C++ double.
 * @param iDouble
 * a  C++ <tt>double</tt>.
 * @param ioVar
 * the CATVariant to initialize.
 */
ExportedByJS0DSPA HRESULT BuildVariant(const double & iDouble, 
				       CATVariant & ioVar);

/**
 * Initializes a CATVariant with a C++ CATBoolean.
 * @param iBoolean
 * a  C++ <tt>CATBoolean</tt>.
 * @param ioVar
 * the CATVariant to initialize.
 */
ExportedByJS0DSPA HRESULT BuildVariant(const CATBoolean & iBoolean, 
				       CATVariant & ioVar);

/**
 * Initializes a CATVariant with a C++ CATUnicodeString.
 * @param iString
 * a  C++ <tt>CATUnicodeString</tt>.
 * @param ioVar
 * the CATVariant to initialize.
 */
ExportedByJS0DSPA HRESULT BuildVariant(const CATUnicodeString & iString, 
				       CATVariant & ioVar);

/**
 * Initializes a CATVariant with a C++ <tt>char*</tt>.
 * @param iString
 * a  C++ <tt>char*</tt>.
 * @param ioVar
 * the CATVariant to initialize.
 */
ExportedByJS0DSPA HRESULT BuildVariant(const char * iString, 
				       CATVariant & ioVar);

/**
 * Initializes a CATVariant with a C++ CATBaseDispatch.
 * @param iObject
 * a  C++ <tt>CATBaseDispatch</tt>.
 * @param ioVar
 * the CATVariant to initialize.
 */
ExportedByJS0DSPA HRESULT BuildVariant(const CATBaseDispatch * iObject, 
				       CATVariant & ioVar);

#endif

