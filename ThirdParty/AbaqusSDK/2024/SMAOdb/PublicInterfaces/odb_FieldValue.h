//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_FieldValue_h
#define odb_FieldValue_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <cow_COW.h>
#include <odb_Enum.h>
#include <odb_FieldValueData.h>
#include <odb_Instance.h>
#include <odb_SectionPoint.h>
#include <atr_translate.h>
#include <nex_ExceptionHandling.h>
#include <odb_Exceptions.h>
// End local includes

//
// Forward declarations
//
class odb_SubField;
class odb_Field;
class oda_FieldValueIntObj;

static bool m_invar_warn_printed;


class odb_FieldValue : public mem_AllocationOperators
{
    friend class odb_FieldValueList;
    friend class odb_MathUtil;

 public:
    // documented and supported user interfaces

    odb_Enum::odb_ResultPositionEnum position() const { return _position; };
    odb_Enum::odb_DataTypeEnum type() const { return _dataType; };
    odb_Enum::odb_PrecisionEnum precision() const { return _precision; }
    const odb_Instance& instance() const {return _instance.ConstGet(); }       

    int elementLabel() const { return _elementLabel; };
    int nodeLabel() const { return _nodeLabel; };
    int integrationPoint() const { return _integrationPoint; };
    odb_String baseElementType() const { return _elementType; };
    odb_Enum::odb_ElementFaceEnum face() const { return _face; };
    const odb_SectionPoint& sectionPoint() const { return _sectionPoint; };
    odb_SequenceSequenceFloat localCoordSystem() const;
    odb_SequenceSequenceDouble localCoordSystemDouble() const;

    // data
    const float* data(int& numVal) const {
	if(_precision == odb_Enum::DOUBLE_PRECISION)
	{
	    odb_Exception exc(odb_TEXT_MESSAGE, atr("Data is double precision.  Use dataDouble()"));
	    HKS_THROW(exc);
	}
	numVal = _data.width();
	return _data.ConstGet();
    };
    const float* conjugateData(int& numVal) const {
	if(_precision == odb_Enum::DOUBLE_PRECISION)
	{
	    odb_Exception exc(odb_TEXT_MESSAGE, atr("Data is double precision.  Use conjugateDataDouble()"));
	    HKS_THROW(exc);
	}
	numVal = _data.width();
	return _data.ConstGet2();
    };

    const double* dataDouble(int& numVal) const {
	if(_precision == odb_Enum::SINGLE_PRECISION)
	{
	    odb_Exception exc(odb_TEXT_MESSAGE, atr("Data is single precision.  Use data()"));
	    HKS_THROW(exc);
	}
	numVal = _data.width();
	return _data.ConstGetDouble();
    };

    const double* conjugateDataDouble(int& numVal) const {
	if(_precision == odb_Enum::SINGLE_PRECISION)
	{
	    odb_Exception exc(odb_TEXT_MESSAGE, atr("Data is single precision.  Use conjugateData()"));
	    HKS_THROW(exc);
	}
	numVal = _data.width();
	return _data.ConstGetDouble2();
    };
    
    odb_SequenceFloat data() const {
	if(_precision == odb_Enum::DOUBLE_PRECISION)
	{
	    odb_Exception exc(odb_TEXT_MESSAGE, atr("Data is double precision.  Use dataDouble()"));
	    HKS_THROW(exc);
	}
	return odb_SequenceFloat(_data.ConstGet(), _data.width());
    };
    
    odb_SequenceFloat conjugateData() const { 
	if(_precision == odb_Enum::DOUBLE_PRECISION)
	{
	    odb_Exception exc(odb_TEXT_MESSAGE, atr("Data is double precision.  Use conjugateDataDouble()"));
	    HKS_THROW(exc);
	}
	return odb_SequenceFloat(_data.ConstGet2(), _data.width());
    };

    odb_SequenceDouble dataDouble() const {
	if(_precision == odb_Enum::SINGLE_PRECISION)
	{
	    odb_Exception exc(odb_TEXT_MESSAGE, atr("Data is single precision.  Use data()"));
	    HKS_THROW(exc);
	}
	return odb_SequenceDouble(_data.ConstGetDouble(), _data.width());
    };
    
    odb_SequenceDouble conjugateDataDouble() const { 
	if(_precision == odb_Enum::SINGLE_PRECISION)
	{
	    odb_Exception exc(odb_TEXT_MESSAGE, atr("Data is single precision.  Use conjugateData()"));
	    HKS_THROW(exc);
	}
	return odb_SequenceDouble(_data.ConstGetDouble2(), _data.width());
    };

    // invariants
    float magnitude() const;
    float mises() const;
    float tresca() const;
    float press() const;
    float inv3() const;
    
    float maxPrincipal() const;
    float midPrincipal() const;
    float minPrincipal() const;

    float maxInPlanePrincipal() const;
    float minInPlanePrincipal() const;
    float outOfPlanePrincipal() const;

    // undocumented and unsupported

    odb_FieldValue ();
    odb_FieldValue(odb_Enum::odb_ResultPositionEnum position,
		   odb_Enum::odb_DataTypeEnum dataType,
		   const odb_InstanceCOW&  instance,
		   bool _engineeringTensor,
		   const cow_String& _elementType,
		   const odb_SectionPoint& _SectionPoint,
		   odb_Enum::odb_PrecisionEnum precision );
    odb_FieldValue(odb_Enum::odb_ResultPositionEnum position,
		   odb_Enum::odb_DataTypeEnum dataType,
		   const odb_InstanceCOW&  instance,
		   odb_Enum::odb_PrecisionEnum precision );

    bool isTensor() const {
        return _dataType == odb_Enum::TENSOR_3D_FULL ||
	  _dataType == odb_Enum::TENSOR_3D_PLANAR  ||
	  _dataType == odb_Enum::TENSOR_3D_SURFACE ||
	  _dataType == odb_Enum::TENSOR_2D_PLANAR  ||
	  _dataType == odb_Enum::TENSOR_2D_SURFACE;
    };
    bool hasLocalCoordSys() const { return _localCoordSystem.width() > 0; };
    const odb_FieldValueData& ConstGetData() const { return _data; };
    void SetEngineeringTensorFlag( bool flag ) { _engineeringTensor = flag; };
    bool engineeringTensorFlag() const { return _engineeringTensor; };
    void SetElementType( const odb_String& elType ) { _elementType = elType; };
    void BorrowData( const float* data, int width ) { _data.BorrowData(data, width); };
    void BorrowData( const double* data, int width ) { _data.BorrowData(data, width); };
    // Use this interface carefully, only for the object with same width
    void BorrowData( const float* data ) { _data.BorrowData(data); };
    void BorrowData( const double* data ) { _data.BorrowData(data); };
    void BorrowConjugateData( const float* data ) { _data.BorrowConjugateData(data); };
    void BorrowConjugateData( const double* data) { _data.BorrowConjugateData(data); };
    void SetConjugateData( const float* data ) { _data.SetConjugateData(data); };
    void SetConjugateData( const double* data ) { _data.SetConjugateData(data); };
    void BorrowLCS( const float* data, int width ) { _localCoordSystem = odb_FieldValueData(width, odb_Enum::SINGLE_PRECISION); _localCoordSystem.SetData(data); };
    void BorrowLCS( const double* data, int width ) { _localCoordSystem = odb_FieldValueData(width, odb_Enum::DOUBLE_PRECISION); _localCoordSystem.SetData(data); };
    void SetData( float data );
    void SetData( double data );
    void SetData( const float* data, int width );
    void SetData( const double* data, int width );
    void SetData( const odb_SequenceFloat& ssd ) { _data = odb_FieldValueData(ssd); };
    void SetData( const odb_SequenceDouble& ssd ) { _data = odb_FieldValueData(ssd); };
    void SetData( const odb_FieldValueData& data ) { _data = data; };
    float invariant(odb_Enum::odb_InvariantEnum inv) const;

 private:
    // members
    odb_Enum::odb_ResultPositionEnum      _position;
    odb_Enum::odb_DataTypeEnum            _dataType;
    odb_Enum::odb_PrecisionEnum           _precision;
    odb_InstanceCOW                       _instance;  
    int                                   _elementLabel;
    int                                   _nodeLabel;
    int                                   _integrationPoint;
    odb_Enum::odb_ElementFaceEnum         _face;
    odb_SectionPoint                      _sectionPoint;     
    odb_FieldValueData                    _localCoordSystem;
    odb_FieldValueData                    _data;
    bool                                  _engineeringTensor;
    odb_String                            _elementType;
};

COW_COW_DECL(odb_FieldValue)

#endif // odb_FieldValue_h
