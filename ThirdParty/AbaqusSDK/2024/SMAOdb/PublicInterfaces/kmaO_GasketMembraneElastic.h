//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_GasketMembraneElastic_h
#define kmaO_GasketMembraneElastic_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_GasketMembraneElastic
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_MembraneElastic;

class kmaO_GasketMembraneElastic: public kmaO_MaterialOption
{
  public:
    kmaO_GasketMembraneElastic( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_GasketMembraneElastic();
    kmaO_GasketMembraneElastic( const odb_SequenceSequenceDouble& table,
                                bool temperatureDependency = false,
                                int dependencies = 0 );
    kmaO_GasketMembraneElastic( const kmaO_GasketMembraneElastic& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_GasketMembraneElastic& operator=( const kmaO_GasketMembraneElastic& rhs );

    virtual ~kmaO_GasketMembraneElastic();
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_MembraneElastic* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_GasketMembraneElastic_h
