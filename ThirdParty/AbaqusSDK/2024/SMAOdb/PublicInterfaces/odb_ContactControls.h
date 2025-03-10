//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_ContactControls_h
#define odb_ContactControls_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <cow_COW.h>

// Forward declarations
class ddb_ContactControls;
COW_COW_DECL(ddb_ContactControls);

class odb_ContactControls : public mem_AllocationOperators
{
public:
    //member access
    
    //options
    
private:
    ddb_ContactControlsCOW m_ContactControls;

public:    // undocumented and unsupported

    //Needed for container
    odb_ContactControls();
    
    odb_ContactControls(const ddb_ContactControls &cc);

    ddb_ContactControls contactControls() const;

};

COW_COW_DECL(odb_ContactControls);

#endif //odb_StdContactControls_h
