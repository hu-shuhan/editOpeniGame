//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_FieldValueList_h
#define odb_FieldValueList_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_FieldValue.h>
#include <cow_List.h>
#include <rgnC_ResEnum.h>
#include <res_FieldDescription.h>

// End local includes

// Forwar declaration
class ddb_Field;
class ddb_FieldDescription;
class ddb_FieldContainer;
class ddb_ScalarFloat;
class ddb_ScalarDouble;
class ddb_VectorFloat;
class ddb_VectorDouble;
class ddb_MatrixFloat;
class ddb_MatrixDouble;
class ddb_TensorFloat;
class ddb_TensorDouble;
class ddb_QuaternionFloat;
class ddb_QuaternionDouble;
class odb_FieldBulkData;
class odb_FieldOutput;

class odb_FieldValueList : public mem_AllocationOperators
{
  friend class odb_FieldBulkData;

public:    // undocumented and unsupported
    odb_FieldValueList();
    odb_FieldValueList(const int& size);
    odb_FieldValueList(const odb_FieldValueList&);
    ~odb_FieldValueList();

    odb_FieldValueList& operator=(const odb_FieldValueList & rhs);

    void Construct(const ddb_Field& DdbField,
		   const ddb_FieldDescription& FieldDescript,
		   const odb_Enum::odb_DataTypeEnum& MasterDataType,
		   const cow_ListInt SubsetIndices);

    const odb_FieldValue ConstGet(const int& index) const;
    odb_FieldValue Get(const int& index);
    inline int Size() const { return dataLength; };
    void ClearAll();
    inline void clear() { ClearAll(); };
    void FixConnectorField(const odb_FieldOutput& deformation,  const ddb_FieldContainer& fieldCon);
private:
    float* data;
    float* conjugateData;
    float* orientation;
    double* dataDbl;
    double* conjugateDataDbl;
    double* orientationDbl;
    int* nodeLabel;
    int* elementLabel;
    odb_Enum::odb_ElementFaceEnum * faceId;
    int dataSize;
    int dataLength;
    int dataWidth;
    int numEntities;
    int orientationWidth;
    int ElementClassIndex;
    int MeshIndex;
    odb_Enum::odb_ResultPositionEnum position;
    odb_Enum::odb_PrecisionEnum precision;
    int valuesPerElement;
    int numberOfNodes;
    res_SetTypeEnm setType;
    bool dataBorrowed;
    bool isPair;
    cow_ListInt subsetIndices;
    int numSubsetIndices;
    odb_InstanceCOW instanceCow;
    odb_Enum::odb_DataTypeEnum MasterDataType;
    bool engineeringTensor;
    cow_String elementType;
    odb_SectionPoint SectionPoint;
    cow_ListString compLabels;
    cow_ListInt entities;    // used when number of integ. points
    cow_ListInt integpts;    // per face varies within element
    res_FieldDescription _fieldDescription;

    void Copy(const odb_FieldValueList& src);

    void SetFieldLabels(const ddb_FieldDescription& fieldDescript);

    void GetValuesByDataType(const ddb_Field& field,
			     const int& width,
			     const int& widthDest,
			     const odb_Enum::odb_DataTypeEnum& dataType);

    void GetScalarValues(const ddb_Field& f);
    void GetFloatScalarValues(const ddb_ScalarFloat& f);
    void GetDoubleScalarValues(const ddb_ScalarDouble& f);

    void GetVectorValues(const ddb_Field& f);
    void GetFloatVectorValues(const ddb_VectorFloat& f);
    void GetDoubleVectorValues(const ddb_VectorDouble& f);

    void GetMatrixValues(const ddb_Field& f);
    void GetFloatMatrixValues(const ddb_MatrixFloat& f);
    void GetDoubleMatrixValues(const ddb_MatrixDouble& f);

    void GetTensorValues(const ddb_Field& f, const int& width,
			 const int& widthDest, const odb_Enum::odb_DataTypeEnum& dataType);
    void GetFloatTensorValues(const ddb_TensorFloat& f,
			      const int& width, const int& widthDest,
			      const odb_Enum::odb_DataTypeEnum& dataType);
    void GetDoubleTensorValues(const ddb_TensorDouble& f,
			       const int& width, const int& widthDest,
			       const odb_Enum::odb_DataTypeEnum& dataType);

    void GetQuaternionValues( const ddb_Field& f);
    void GetFloatQuaternionValues( const ddb_QuaternionFloat& f);
    void GetDoubleQuaternionValues( const ddb_QuaternionDouble& f );

    odb_SectionPoint sectionPoint(const ddb_FieldDescription& fieldDescript);

    void SetFloatLCS( const ddb_QuaternionFloat& q );
    void SetDoubleLCS( const ddb_QuaternionDouble& q );    

};

#endif // #ifndef odb_FieldValueList_h
