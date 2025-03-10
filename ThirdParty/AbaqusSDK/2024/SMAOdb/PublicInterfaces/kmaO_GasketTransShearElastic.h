//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_GasketTransverseShearElastic_h
#define kmaO_GasketTransverseShearElastic_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_GasketTransverseShearElastic
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
// Forward declarations
class kmaC_TransShearElastic;

class kmaO_GasketTransverseShearElastic: public kmaO_MaterialOption
{
  public:
    kmaO_GasketTransverseShearElastic( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_GasketTransverseShearElastic();
    kmaO_GasketTransverseShearElastic( const odb_SequenceSequenceDouble& table,
                                       odb_String variableUnits = "STRESS",
                                       bool temperatureDependency = false,
                                       int dependencies = 0 );
    kmaO_GasketTransverseShearElastic( const kmaO_GasketTransverseShearElastic& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_GasketTransverseShearElastic& operator=( const kmaO_GasketTransverseShearElastic& rhs );

    virtual ~kmaO_GasketTransverseShearElastic();
    odb_String variableUnits() const;
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_TransShearElastic* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_GasketTransverseShearElastic_h
