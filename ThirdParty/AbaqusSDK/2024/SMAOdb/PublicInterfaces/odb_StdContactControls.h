//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_StdContactControls_h
#define odb_StdContactControls_h

// Begin local includes
#include <cow_COW.h>
#include <odb_ContactControls.h>

// Forward declarations
class ddb_StdContactControls;
//COW_COW_DECL(ddb_StdContactControls);

class odb_StdContactControls: public odb_ContactControls
{
public:
    //member access
    
    //Options

private:
    //  ddb_StdContactControlsCOW m_stdContactControls;

public:    // undocumented and unsupported

    //Needed for container
    odb_StdContactControls();
    
    odb_StdContactControls(const ddb_StdContactControls &sc);

    //ddb_StdContactControls stdContactControls() const;

};

COW_COW_DECL(odb_StdContactControls);

#endif //odb_StdContactControls_h
