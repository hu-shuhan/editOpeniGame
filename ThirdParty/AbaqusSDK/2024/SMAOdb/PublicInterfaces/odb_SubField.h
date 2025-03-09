//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_SubField_h
#define odb_SubField_h
///////////////////////////////////////////////////////////
//
//   name : odb_SubField
//
//   description: Used by odb_Field Class 
//                Manage  - a single field from the DDB
//                        - creates the fieldValues for this field
//                        
//  


// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_FieldValue.h>
#include <odb_FieldValueList.h>
#include <odb_Assembly.h>
// End local includes

// Forward declarations
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
class ddb_Field;
class res_FieldDescription;
COW_COW_DECL(ddb_Field);
class ddb_FieldDescription;
COW_COW_DECL(ddb_FieldDescription);

class odb_SubField : public mem_AllocationOperators
{
public:    
 
private:
    int                      Key;
    cow_String               Label;
    ddb_FieldCOW             DdbField;
    ddb_FieldDescriptionCOW  FieldDescript;
    ddb_AssemblyCOW          RootAssy;   
    int 	             NumValues;
    odb_Enum::odb_DataTypeEnum         MasterDataType;
    int                      ElementClassIndex;
    int                      MeshIndex;
    cow_ListInt       subIndices;
    //
    int  DdbFieldSize() const;

    // Store a ddb set to allow reusing the set
    // when computing a field from subset indices
    ddb_SetCOW sourceSet;

    cow_ListInt GetFieldLabelsSubset(const cow_ListInt& dataIndices,
					    const int& numValsPerEl,
					    const odb_Enum::odb_ResultPositionEnum& dataPos) const;
    // caller is responsible for deleting pointers returned by the following two methods
    int* GetFieldLabels(int& numLabels) const;
    int* GetElementNodalNodeLabels(int& numLabels,
				   const cow_ListInt& elementIndicesInClass) const;

    void GetNodalValuesSubset(const ddb_Field& field,
			      const cow_ListInt& dataIndices,
			      float* data,
			      cow_ListInt& labels,
			      float* conjugateData,
			      const int& numValPerEl,
			      const odb_Enum::odb_ResultPositionEnum& dataPos,
			      float*& coordSystem) const;
    void GetElementNodalValuesSubset(const ddb_Field& field,
				     const cow_ListInt& dataIndices,
				     float* data,
				     cow_ListInt& labels,
				     float* conjugateData,
				     const int& numValPerEl,
				     const odb_Enum::odb_ResultPositionEnum& dataPos,
				     float*& coordSystem) const;
    void GetElementCentroidValuesSubset(const ddb_Field& field,
					const cow_ListInt& dataIndices,
					float* data,
					cow_ListInt& labels,
					float* conjugateData,
					const int& numValPerEl,
					const odb_Enum::odb_ResultPositionEnum& dataPos,
					float*& coordSystem) const;
    void GetIntegrationPointsValuesSubset(const ddb_Field& field,
					  const cow_ListInt& dataIndices,
					  float* data,
					  cow_ListInt& labels,
					  float* conjugateData,
					  const int& numValPerEl,
					  const odb_Enum::odb_ResultPositionEnum& dataPos,
					  float*& coordSystem) const;

    void GetValuesByDataTypeSubset(const ddb_Field& field,
				   float* data,
				   float* conjugateData,
				   const cow_ListInt& dataIndices,
				   float*& coordSystem) const;

    void GetScalarValuesSubset(const ddb_Field& f,
			       float* data,
			       float* conjugateData,
			       const cow_ListInt& dataIndices) const;
    void GetFloatScalarValuesSubset(const ddb_ScalarFloat& f,
				    float* data,
				    float* conjugateData,
				    const cow_ListInt& dataIndices) const;
    void GetDoubleScalarValuesSubset(const ddb_ScalarDouble& f,
				     float* data,
				     float* conjugateData,
				     const cow_ListInt& dataIndices) const;

    void GetVectorValuesSubset(const ddb_Field& f,
			       float* data,
			       float* conjugateData,
			       const cow_ListInt& dataIndices,
			       float*& coordSystem) const;
    void GetFloatVectorValuesSubset(const ddb_VectorFloat& f,
				    float* data,
				    float* conjugateData,
				    const cow_ListInt& dataIndices,
				    float*& coordSystem) const;
    void GetDoubleVectorValuesSubset(const ddb_VectorDouble& f,
				     float* data,
				     float* conjugateData,
				     const cow_ListInt& dataIndices,
				     float*& coordSystem) const;

    void GetMatrixValuesSubset(const ddb_Field& f,
			       float* data,
			       const cow_ListInt& dataIndices,
			       float*& coordSystem) const;
    void GetFloatMatrixValuesSubset(const ddb_MatrixFloat& f,
				    float* data,
				    const cow_ListInt& dataIndices,
				    float*& coordSystem) const;
    void GetDoubleMatrixValuesSubset(const ddb_MatrixDouble& f,
				     float* data,
				     const cow_ListInt& dataIndices,
				     float*& coordSystem) const;

    void GetTensorValuesSubset(const ddb_Field& f,
			       float* data,
			       float* conjugateData,
			       const cow_ListInt& dataIndices,
			       float*& coordSystem) const;
    void GetFloatTensorValuesSubset(const ddb_TensorFloat& f,
				    float* data,
				    float* conjugateData,
				    const cow_ListInt& dataIndices,
				    float*& coordSystem) const;
    void GetDoubleTensorValuesSubset(const ddb_TensorDouble& f,
				     float* data,
				     float* conjugateData,
				     const cow_ListInt& dataIndices,
				     float*& coordSystem) const;

    void SetFloatLCSSubset( const ddb_QuaternionFloat& q,
			    const cow_ListInt& dataIndices,
			    float*& coordSystem ) const;
    void SetDoubleLCSSubset( const ddb_QuaternionDouble& q,
			     const cow_ListInt& dataIndices,
			     float*& coordSystem ) const;


    // create ddb field from data
    void SetData(const odb_SequenceInvariant& validInvar,
                 odb_Enum::odb_ResultPositionEnum position,
                 const ddb_PartInstance& instance,
		 const bool& engineeringTensor,
                 const cow_ListInt& labels,
		 const int& dataWidth,
		 const int& dataLength,
                 const float* data,
                 const float* conjugateData,
                 const odb_SectionPoint& sectionPoint,
                 const float* coordSystem,
                 const odb_SequenceString& compLabels = odb_SequenceString() );
    void SetData(const odb_SequenceInvariant& validInvar,
                 odb_Enum::odb_ResultPositionEnum position,
                 const ddb_PartInstance& instance,
		 const bool& engineeringTensor,
                 const ddb_Set& set,
                 int subSetIndex,
		 const int& dataWidth,
		 const int& dataLength,
                 const float* data,
                 const float* conjugateData,
                 const odb_SectionPoint& sectionPoint,
                 const float* coordSystem,
                 const odb_SequenceString& compLabels = odb_SequenceString() );

    //
    void CreateFieldValuesSubset(const cow_ListInt& dataIndices,
			    float* data,
			    cow_ListInt& labels,
			    float* conjugateData,
			    const int& numValPerEl,
			    const odb_Enum::odb_ResultPositionEnum& dataPos,
			    float*& coordSystem) const;

    // to create new ddb field
    res_FieldDescription* CreateFieldDescriptionForData (
                  const odb_SequenceInvariant& validInvar,
                  odb_Enum::odb_ResultPositionEnum position,
                  const ddb_PartInstance& instanceIn,
                  const ddb_Set& set,
                  int subSetIndex,
                  int numData,
                  const odb_SectionPoint& secPoint,
                  const float* coordSystem,
		  res_FieldDescription*& resOrient,
                  const odb_SequenceString& compLabels = odb_SequenceString());
    void CheckDataLen(res_FieldDescription* fieldD,
                      int dataLen);
    ddb_Field WriteData(res_FieldDescription* fieldD,
			const int& dataSize,
                        const float* data );
    ddb_Field WriteOrientation(res_FieldDescription* fieldD,
                        int numData,
                        const float* coordSystem);
    ddb_Field WriteScalarData(res_FieldDescription* fieldD,
			      const int& dataSize,
			      const float* data);
    ddb_Field WriteScalarData(res_FieldDescription* fieldD,
			      const odb_SequenceSequenceFloat& data);
    ddb_Field WriteVectorData(res_FieldDescription* fieldD,
			      const int& dataSize,
			      const float* data);
    ddb_Field WriteMatrixData(res_FieldDescription* fieldD,
			      const int& dataSize,
			      const float* data);
    ddb_Field WriteTensorData(res_FieldDescription* fieldD,
			      const int& dataSize,
			      const float* data);

public:    // undocumented and unsupported
    odb_SubField();
    odb_SubField(int key,
                 const ddb_Field& field,
	         const ddb_Assembly& assy,
                 odb_Enum::odb_DataTypeEnum masterDataType);
    odb_SubField(const cow_String& label,
	         const ddb_Assembly& assy,
                 odb_Enum::odb_DataTypeEnum masterDataType);

    // member access
    int key() const;
    int Size() const;
    inline cow_ListInt getSubsetIndices() const;
    inline void setSubsetIndices(const cow_ListInt &);
    odb_Enum::odb_ResultPositionEnum position() const;
    odb_SectionPoint sectionPoint() const;
    odb_Enum::odb_DataTypeEnum type() const;
    odb_SequenceInvariant validInvariants() const;
    odb_Enum::odb_DataTypeEnum masterType() const;
    void SetMasterType(odb_Enum::odb_DataTypeEnum type);
    void updateSubField();
    void updateSubFieldDouble();
    ddb_Field WriteData (res_FieldDescription* fieldD,
                         const int& dataSize,
                         const double* data);

    ddb_SetCOW getSourceSet() const { return sourceSet; }
    ddb_FieldDescription fieldDescription() const;
    ddb_Field field() const;
    const ddb_Assembly& Assembly() const;
    ddb_PartInstance PartInstance() const;
    ddb_Set set() const;
    int setIndex() const;
    //
    cow_ListInt subsetIndices( const odb_Set& set ) const;
    cow_ListInt subsetIndicesFromElementLabels(
			 const cow_ListInt& elementLabels ) const;
    cow_ListInt subsetIndicesFromNodeLabels(
			 const cow_ListInt& nodeLabels ) const;

    //
    odb_SubField SubFieldFromDataIndices(
                     const cow_ListInt& dataIndices ) const;
    odb_SubField SubFieldFromDataIndices(
                     const cow_ListInt& dataIndices,
		     odb_Enum::odb_ResultPositionEnum ) const;

    // Set the region name and type to allow fetching this region
    // from the odb file instead of creating a new set
    void SetSet( const ddb_Set& set ) { sourceSet = ddb_SetCOW(set); }

    // outdated
    odb_String setName() const;
    odb_Enum::odb_SetTypeEnum setType() const;
};

inline cow_ListInt
odb_SubField::getSubsetIndices() const
{
    return subIndices;
}

inline void 
odb_SubField::setSubsetIndices(const cow_ListInt & subIndexList)
{
    subIndices = subIndexList;
}

#endif // odb_SubField_h
