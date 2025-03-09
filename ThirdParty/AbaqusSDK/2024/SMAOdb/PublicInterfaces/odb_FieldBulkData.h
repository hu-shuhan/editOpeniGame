//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_FieldBulkData_h
#define odb_FieldBulkData_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_Enum.h>
#include <odb_SectionPoint.h>
#include <odb_FieldValueList.h>
#include <rgnC_ResEnum.h>
#include <ddb_FieldDescription.h>
#include <cow_ArrayCOW.h>
#include <cow_COW.h>
// End local includes

//
// Forward declarations
//
class odb_Instance;
COW_COW_DECL(odb_Instance);



class odb_FieldBulkData : public mem_AllocationOperators
{

public:

    // documented and supported

    odb_Enum::odb_ResultPositionEnum position() const { return Position;} ;
    odb_Enum::odb_DataTypeEnum type() const { return DataType; };
    odb_Enum::odb_PrecisionEnum precision() const { return outputPrecision; }
    const odb_Instance& instance() const { return Instance.ConstGet();};
    const odb_SectionPoint& sectionPoint() const { return SectionPoint; };

    int* elementLabels() const;
    int* nodeLabels() const { return nodeLabel; };
    int* integrationPoints() const;
    odb_String baseElementType() const {return elementType; };
    odb_Enum::odb_ElementFaceEnum* faces() const;
    int valuesPerElement() const { return _valuesPerElement;};
    int numberOfElements() const;
    int numberOfNodes() const { return _numberOfNodes; }
    odb_SequenceString componentLabels() const {return compLabels;}

    // data
    float* data() const { return dataPtr; };
    float* conjugateData() const { return conjugateDataPtr; };
    float* localCoordSystem() const { return orientation; };
    double* dataDouble() const { return dataDoublePtr; };
    double* conjugateDataDouble() const { return conjugateDataDoublePtr; };
    double* localCoordSystemDouble() const { return orientationDouble; };
    float* mises() const;
    int orientationWidth() const { return OrientationWidth; };
    int width() const { return dataWidth; };
    int length() const { return dataLength; };

    // undocumented and unsupported

    odb_FieldBulkData();
    odb_FieldBulkData& operator=(const odb_FieldBulkData & rhs);
    odb_FieldBulkData(const odb_FieldValueList& fieldValueList, ddb_FieldDescription fd);
    odb_FieldBulkData(const odb_FieldBulkData& );
    ~odb_FieldBulkData();
    const ddb_FieldDescription & fieldDescription() const { return _fd; }

private:
    float* dataPtr;
    float* conjugateDataPtr;
    float* orientation;
    double* dataDoublePtr;
    double* conjugateDataDoublePtr;
    double* orientationDouble;
    float* misesPtr;
    int* nodeLabel;
    int* elementLabel;
    int* integrationPoint;
    odb_Enum::odb_ElementFaceEnum * faceId;
    int dataLength;
    int dataWidth;
    int OrientationWidth;
    odb_Enum::odb_ResultPositionEnum Position;
    odb_Enum::odb_DataTypeEnum       DataType;
    odb_Enum::odb_PrecisionEnum      outputPrecision;
    odb_InstanceCOW                  Instance;
    odb_String                       elementType;
    odb_SectionPoint                 SectionPoint;
    odb_SequenceString               compLabels;
    odb_SequenceInt                  entities;
    odb_SequenceInt                  integpts;
    bool engineeringTensor;
    res_SetTypeEnm setType;
    int* _elementLabel;
    int _valuesPerElement;
    int _numberOfNodes;
    odb_Enum::odb_ElementFaceEnum * _faceId;
    ddb_FieldDescription _fd;
    odb_FieldValueList _fieldValueList;

    void Copy(const odb_FieldBulkData& rhs);
    void Construct(const odb_FieldValueList& fieldValueList);
    void clear();
};

COW_ARRAYCOW_DECL(odb_FieldBulkData)

#endif // #ifndef odb_FieldBulkData_h
