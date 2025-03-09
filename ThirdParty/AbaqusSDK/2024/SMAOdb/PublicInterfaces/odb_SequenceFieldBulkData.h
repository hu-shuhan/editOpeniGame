//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_SequenceFieldBulkData_h
#define odb_SequenceFieldBulkData_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <ddb_Set.h>
#include <odb_FieldValueList.h>
#include <odb_SequenceFieldValue.h>
#include <odb_FieldBulkData.h>
#include <cow_List.h>
// End local includes

COW_LIST_DECL(odb_FieldBulkDataCOW, odb_FieldBulkDataList)

class odb_SequenceFieldBulkData : public mem_AllocationOperators
{

public:

    // documented and supported

    int size() const;
    const odb_FieldBulkData& constGet(int index) const; 
    const odb_FieldBulkData& operator[](int index) const;
    odb_FieldBulkData& get(int index);

    // undocumented and unsupported

    odb_SequenceFieldBulkData();
    odb_SequenceFieldBulkData(const odb_SequenceFieldValue& sfCon);
    void clearAll();

private:

    odb_SequenceFieldValue m_fieldValueCont;
    odb_FieldBulkDataList   m_bulkDataList;
};

#endif // #ifndef odb_SequenceFieldBulkData_h
