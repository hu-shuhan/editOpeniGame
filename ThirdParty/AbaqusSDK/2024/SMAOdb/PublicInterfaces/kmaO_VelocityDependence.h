//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_VelocityDependence_h
#define kmaO_VelocityDependence_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_VelocityDependence
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_VelocityDependence;

class kmaO_VelocityDependence: public kmaO_MaterialOption
{
  public:
    kmaO_VelocityDependence( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_VelocityDependence();
    kmaO_VelocityDependence( const odb_SequenceSequenceDouble& table );
    kmaO_VelocityDependence( const kmaO_VelocityDependence& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_VelocityDependence& operator=( const kmaO_VelocityDependence& rhs );

    virtual ~kmaO_VelocityDependence();
    odb_SequenceSequenceDouble table() const;
    const kmaC_VelocityDependence* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_VelocityDependence_h
