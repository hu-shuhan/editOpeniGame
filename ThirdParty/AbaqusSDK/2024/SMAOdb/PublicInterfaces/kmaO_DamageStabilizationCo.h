//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_DamageStabilizationCohesive_h
#define kmaO_DamageStabilizationCohesive_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_DamageStabilizationCohesive
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_Union.h>
// Forward declarations
class kmaC_DamageStabilizationCohesive;

class kmaO_DamageStabilizationCohesive: public kmaO_MaterialOption
{
  public:
    kmaO_DamageStabilizationCohesive( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_DamageStabilizationCohesive( odb_Union cohesiveCoeff = "NONE" );
    kmaO_DamageStabilizationCohesive( const kmaO_DamageStabilizationCohesive& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_DamageStabilizationCohesive& operator=( const kmaO_DamageStabilizationCohesive& rhs );

    virtual ~kmaO_DamageStabilizationCohesive();
    odb_Union cohesiveCoeff() const;
    const kmaC_DamageStabilizationCohesive* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_DamageStabilizationCohesive_h
