//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_CapCreepCohesion_h
#define kmaO_CapCreepCohesion_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_CapCreepCohesion
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
// Forward declarations
class kmaC_CapCreepCohesion;

class kmaO_CapCreepCohesion: public kmaO_MaterialOption
{
  public:
    kmaO_CapCreepCohesion( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_CapCreepCohesion();
    kmaO_CapCreepCohesion( const odb_SequenceSequenceDouble& table,
                           odb_String law = "STRAIN",
                           bool temperatureDependency = false,
                           int dependencies = 0 );
    kmaO_CapCreepCohesion( const kmaO_CapCreepCohesion& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_CapCreepCohesion& operator=( const kmaO_CapCreepCohesion& rhs );

    virtual ~kmaO_CapCreepCohesion();
    odb_String law() const;
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_CapCreepCohesion* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_CapCreepCohesion_h
