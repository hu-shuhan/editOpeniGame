//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_ExpContactControls_h
#define odb_ExpContactControls_h

// Begin local includes
#include <cow_COW.h>
#include <odb_ContactControls.h>

// Forward declarations
class ddb_ExpContactControls;
//COW_COW_DECL(ddb_ExpContactControls);

class odb_ExpContactControls: public odb_ContactControls
{
public:
    //member access
    
    //Options

private:
    //  ddb_ExpContactControlsCOW m_expContactControls;

public:    // undocumented and unsupported

    //Needed for container
    odb_ExpContactControls();
    
    odb_ExpContactControls(const ddb_ExpContactControls &ec);

};

COW_COW_DECL(odb_ExpContactControls);

#endif //odb_ExpContactControls_h

