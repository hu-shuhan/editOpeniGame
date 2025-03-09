//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_NormalBehavior_h
#define odb_NormalBehavior_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <cow_COW.h>

// Forward declarations
class ddb_NormalBehavior;
COW_COW_DECL(ddb_NormalBehavior);

class odb_NormalBehavior : public mem_AllocationOperators
{
public:

    // member access

    // Options

private:
    ddb_NormalBehaviorCOW normalBehavior;

public:   // undocumented and unsupported

    // Needed for container
    odb_NormalBehavior();

    odb_NormalBehavior(const ddb_NormalBehavior& nb);
    
};

#endif // odb_NormalBehavior_h
