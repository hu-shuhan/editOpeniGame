//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_CrushableFoam_h
#define kmaO_CrushableFoam_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_CrushableFoam
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
// Forward declarations
class kmaO_CrushableFoamHardening;
class kmaO_RateDependent;
class kmaC_Foam;

class kmaO_CrushableFoam: public kmaO_MaterialOption
{
  public:
    kmaO_CrushableFoam( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_CrushableFoam();
    kmaO_CrushableFoam( const odb_SequenceSequenceDouble& table,
                        odb_String hardening = "VOLUMETRIC",
                        bool temperatureDependency = false,
                        int dependencies = 0 );
    kmaO_CrushableFoam( const kmaO_CrushableFoam& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_CrushableFoam& operator=( const kmaO_CrushableFoam& rhs );

    virtual ~kmaO_CrushableFoam();
    odb_String hardening() const;
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    kmaO_CrushableFoamHardening crushableFoamHardening() const;
    kmaO_RateDependent rateDependent() const;
    kmaO_CrushableFoamHardening CrushableFoamHardening( const odb_SequenceSequenceDouble& table,
                                                        bool temperatureDependency = false,
                                                        int dependencies = 0 );
    kmaO_RateDependent RateDependent( const odb_SequenceSequenceDouble& table,
                                      odb_String type = "POWER_LAW",
                                      bool temperatureDependency = false,
                                      int dependencies = 0 );
    const kmaC_Foam* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_CrushableFoam_h
