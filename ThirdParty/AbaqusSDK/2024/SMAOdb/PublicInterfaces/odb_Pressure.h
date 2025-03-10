//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_Pressure_h
#define odb_Pressure_h

// Begin local includes
#include <odb_Load.h>
#include <odb_Types.h>
#include <cow_COW.h>

// Forward declarations
class ddb_Pressure;
class odb_PropagatingComponent;

class odb_Pressure: public odb_Load
{
public:

    void setMagnitudeInStep(const odb_String& stepName, 
                            const odb_PropagatingComponent& mag);    
    void setHzeroInStep( const odb_String& stepName,
			 double hzero );
    void setHreferenceInStep( const odb_String& stepName,
			      double href );

private:

public:   // undocumented and unsupported

    // Needed for container
    odb_Pressure();

    odb_Pressure(const ddb_Pressure& load);
    
};

#endif // odb_Pressure_h
