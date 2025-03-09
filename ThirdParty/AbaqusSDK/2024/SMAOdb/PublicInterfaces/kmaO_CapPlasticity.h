//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_CapPlasticity_h
#define kmaO_CapPlasticity_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_CapPlasticity
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
// Forward declarations
class kmaO_CapCreepCohesion;
class kmaO_CapCreepConsolidation;
class kmaO_CapHardening;
class kmaC_CapPlasticity;

class kmaO_CapPlasticity: public kmaO_MaterialOption
{
  public:
    kmaO_CapPlasticity( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_CapPlasticity();
    kmaO_CapPlasticity( const odb_SequenceSequenceDouble& table,
                        bool temperatureDependency = false,
                        int dependencies = 0 );
    kmaO_CapPlasticity( const kmaO_CapPlasticity& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_CapPlasticity& operator=( const kmaO_CapPlasticity& rhs );

    virtual ~kmaO_CapPlasticity();
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    kmaO_CapCreepCohesion capCreepCohesion() const;
    kmaO_CapCreepConsolidation capCreepConsolidation() const;
    kmaO_CapHardening capHardening() const;
    kmaO_CapCreepCohesion CapCreepCohesion( const odb_SequenceSequenceDouble& table,
                                            odb_String law = "STRAIN",
                                            bool temperatureDependency = false,
                                            int dependencies = 0 );
    kmaO_CapCreepConsolidation CapCreepConsolidation( const odb_SequenceSequenceDouble& table,
                                                      odb_String law = "STRAIN",
                                                      bool temperatureDependency = false,
                                                      int dependencies = 0 );
    kmaO_CapHardening CapHardening( const odb_SequenceSequenceDouble& table,
                                    bool temperatureDependency = false,
                                    int dependencies = 0 );
    const kmaC_CapPlasticity* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_CapPlasticity_h
