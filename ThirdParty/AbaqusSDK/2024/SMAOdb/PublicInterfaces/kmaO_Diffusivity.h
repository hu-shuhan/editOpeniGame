//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_Diffusivity_h
#define kmaO_Diffusivity_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_Diffusivity
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
// Forward declarations
class kmaO_PressureEffect;
class kmaO_SoretEffect;
class kmaC_Diffusivity;

class kmaO_Diffusivity: public kmaO_MaterialOption
{
  public:
    kmaO_Diffusivity( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_Diffusivity();
    kmaO_Diffusivity( const odb_SequenceSequenceDouble& table,
                      odb_String type = "ISOTROPIC",
                      odb_String law = "GENERAL",
                      bool temperatureDependency = false,
                      int dependencies = 0 );
    kmaO_Diffusivity( const kmaO_Diffusivity& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_Diffusivity& operator=( const kmaO_Diffusivity& rhs );

    virtual ~kmaO_Diffusivity();
    odb_String type() const;
    odb_String law() const;
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    kmaO_PressureEffect pressureEffect() const;
    kmaO_SoretEffect soretEffect() const;
    kmaO_PressureEffect PressureEffect( const odb_SequenceSequenceDouble& table,
                                        bool temperatureDependency = false,
                                        int dependencies = 0 );
    kmaO_SoretEffect SoretEffect( const odb_SequenceSequenceDouble& table,
                                  bool temperatureDependency = false,
                                  int dependencies = 0 );
    const kmaC_Diffusivity* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_Diffusivity_h
