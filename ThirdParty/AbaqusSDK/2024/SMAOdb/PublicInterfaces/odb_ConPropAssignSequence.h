//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_ConPropAssignSequence_h
#define odb_ConPropAssignSequence_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <ddb_ContactExpStructs.h>


// Forward declarations
class odb_Set;
class odb_InteractionProperty;

class odb_ConPropAssignSequence  : public mem_AllocationOperators
{
public:
    odb_ConPropAssignSequence();
    void append(const odb_Set& s1, const odb_Set& s2, const odb_InteractionProperty& ip);
    
private:
    ddb_ContactPropertyAssignmentList cpaList;

public:
    const ddb_ContactPropertyAssignmentList& ddbValue() const {return cpaList;}

};

#endif // #ifndef odb_ConPropAssignSequence_h
