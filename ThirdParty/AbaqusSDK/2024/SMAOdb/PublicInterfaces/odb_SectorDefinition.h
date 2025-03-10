//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_SectorDefinition_h
#define odb_SectorDefinition_h

//
// SECTION: System includes
//

//
// Begin Local includes
//
#include <mem_AllocationOperators.h>
#include <odb_Types.h>


//
// SECTION: Forward declarations
//
class ddb_Ddb;

//
// SECTION: Class definition
//

class odb_SectorDefinition : public mem_AllocationOperators
{
public:
    uint numSectors() { return numberOfSectors; }
    const odb_SequenceSequenceFloat symmetryAxis() const;

private:
    odb_SequenceFloat    startPt, endPt; // define symmetry axis
    uint               numberOfSectors; 

public:
    // Undocumented and unsupported
    odb_SectorDefinition(const ddb_Ddb& database);




};


#endif // #ifndef odb_SectorDefinition_h
