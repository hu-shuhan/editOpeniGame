//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_ElectricalConductivity_h
#define kmaO_ElectricalConductivity_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_ElectricalConductivity
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
// Forward declarations
class kmaC_ElectricalConductivity;

class kmaO_ElectricalConductivity: public kmaO_MaterialOption
{
  public:
    kmaO_ElectricalConductivity( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_ElectricalConductivity();
    kmaO_ElectricalConductivity( const odb_SequenceSequenceDouble& table,
                                 odb_String type = "ISOTROPIC",
                                 bool temperatureDependency = false,
                                 int dependencies = 0 );
    kmaO_ElectricalConductivity( const kmaO_ElectricalConductivity& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_ElectricalConductivity& operator=( const kmaO_ElectricalConductivity& rhs );

    virtual ~kmaO_ElectricalConductivity();
    odb_String type() const;
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_ElectricalConductivity* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_ElectricalConductivity_h
