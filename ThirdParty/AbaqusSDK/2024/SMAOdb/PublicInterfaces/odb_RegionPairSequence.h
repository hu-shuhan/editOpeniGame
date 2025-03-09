//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_RegionPairSequence_h
#define odb_RegionPairSequence_h

// Begin local includes
#include <mem_AllocationOperatorsTiny.h>
#include <ddb_ContactExpStructs.h>


// Forward declarations
class odb_Set;

class odb_RegionPairSequence : public mem_AllocationOperatorsTiny
{
public:
    odb_RegionPairSequence();
    void append(const odb_Set& s1, const odb_Set& s2);
    
private:
    ddb_RegionPairList regionPairList;

public:
    const ddb_RegionPairList& ddbValue() const {return regionPairList;}

};

#endif // #ifndef odb_RegionPairSequence_h
