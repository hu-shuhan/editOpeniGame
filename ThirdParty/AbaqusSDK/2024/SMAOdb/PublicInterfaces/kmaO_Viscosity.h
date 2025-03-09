//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_Viscosity_h
#define kmaO_Viscosity_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_Viscosity
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
// Forward declarations
class kmaO_Trs;
class kmaC_Viscosity;

class kmaO_Viscosity: public kmaO_MaterialOption
{
  public:
    kmaO_Viscosity( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_Viscosity();
    kmaO_Viscosity( const odb_SequenceSequenceDouble& table,
                    odb_String type = "NEWTONIAN",
                    bool temperatureDependency = false,
                    int dependencies = 0 );
    kmaO_Viscosity( const kmaO_Viscosity& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_Viscosity& operator=( const kmaO_Viscosity& rhs );

    virtual ~kmaO_Viscosity();
    odb_String type() const;
    bool temperatureDependency() const;
    int dependencies() const;
    kmaO_Trs trs() const;
    odb_SequenceSequenceDouble table() const;
    kmaO_Trs Trs( odb_String definition = "WLF",
                  const odb_SequenceSequenceDouble& table = odb_SequenceSequenceDouble() );
    const kmaC_Viscosity* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_Viscosity_h
