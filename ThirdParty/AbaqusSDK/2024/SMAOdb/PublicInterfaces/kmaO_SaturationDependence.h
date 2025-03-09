//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_SaturationDependence_h
#define kmaO_SaturationDependence_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_SaturationDependence
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_SaturationDependence;

class kmaO_SaturationDependence: public kmaO_MaterialOption
{
  public:
    kmaO_SaturationDependence( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_SaturationDependence();
    kmaO_SaturationDependence( const odb_SequenceSequenceDouble& table );
    kmaO_SaturationDependence( const kmaO_SaturationDependence& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_SaturationDependence& operator=( const kmaO_SaturationDependence& rhs );

    virtual ~kmaO_SaturationDependence();
    odb_SequenceSequenceDouble table() const;
    const kmaC_SaturationDependence* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_SaturationDependence_h
