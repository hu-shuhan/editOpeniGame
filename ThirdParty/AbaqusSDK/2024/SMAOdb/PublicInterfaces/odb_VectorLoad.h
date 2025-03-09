//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_VectorLoad_h
#define odb_VectorLoad_h

// Begin local includes
#include <odb_Load.h>

// Forward declarations
class ddb_VectorLoad;
class odb_PropagatingComponent;

class odb_VectorLoad: public odb_Load
{
public:

    void setComponentsInStep( const odb_String& stepName,
			      const odb_PropagatingComponent& c1,
			      const odb_PropagatingComponent& c2,
			      const odb_PropagatingComponent& c3 );

private:

public:   // undocumented and unsupported

    // Needed for container
    odb_VectorLoad();

    odb_VectorLoad(const ddb_VectorLoad& load);
    
};

#endif // odb_VectorLoad_h
