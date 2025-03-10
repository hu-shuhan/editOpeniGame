//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_Velocity_h
#define odb_Velocity_h

// Begin local includes
#include <odb_PredefinedField.h>
#include <odb_Types.h>

// Forward declarations
class ddb_Velocity;

class odb_Velocity: public odb_PredefinedField
{
public:
    
    void setValues( double velocity1=0,
		    double velocity2=0,
		    double velocity3=0,
		    double omega=0,
		    const odb_SequenceFloat& axisBegin= odb_SequenceFloat(),
		    const odb_SequenceFloat& axisEnd= odb_SequenceFloat() );
    
private:

public:   // undocumented and unsupported

    // Needed for container
    odb_Velocity();

    odb_Velocity(const ddb_Velocity& predefinedField);
    
};

#endif // odb_VelocityBC_h
