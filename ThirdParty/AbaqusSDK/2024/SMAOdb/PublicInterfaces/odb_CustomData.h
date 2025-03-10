//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_CustomData_h
#define odb_CustomData_h

// Begin local includes
#include <ddb_CustomData.h>
#include <mem_AllocationOperators.h>

// Forward Declarations

// Class definition
class odb_CustomData : public mem_AllocationOperators
{
public:
    odb_CustomData();
    odb_CustomData(const ddb_CustomData &customData );
    ~odb_CustomData();

    ddb_CustomData &customData() { return m_customData.Get(); }

private:
    ddb_CustomDataCOW m_customData;
};

#endif
