//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_Permeability_h
#define kmaO_Permeability_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_Permeability
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
// Forward declarations
class kmaO_SaturationDependence;
class kmaO_VelocityDependence;
class kmaC_Permeability;

class kmaO_Permeability: public kmaO_MaterialOption
{
  public:
    kmaO_Permeability( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_Permeability();
    kmaO_Permeability( double specificWeight,
                       const odb_SequenceSequenceDouble& table,
                       odb_String type = "ISOTROPIC",
                       bool temperatureDependency = false,
                       int dependencies = 0 );
    kmaO_Permeability( const kmaO_Permeability& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_Permeability& operator=( const kmaO_Permeability& rhs );

    virtual ~kmaO_Permeability();
    odb_String type() const;
    double specificWeight() const;
	double inertialDragCoefficient() const;
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    kmaO_SaturationDependence saturationDependence() const;
    kmaO_VelocityDependence velocityDependence() const;
    kmaO_SaturationDependence SaturationDependence( const odb_SequenceSequenceDouble& table );
    kmaO_VelocityDependence VelocityDependence( const odb_SequenceSequenceDouble& table );
    const kmaC_Permeability* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_Permeability_h
