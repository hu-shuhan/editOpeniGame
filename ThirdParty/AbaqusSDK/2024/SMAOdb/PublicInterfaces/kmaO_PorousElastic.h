//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_PorousElastic_h
#define kmaO_PorousElastic_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_PorousElastic
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
// Forward declarations
class kmaC_PorousElastic;

class kmaO_PorousElastic: public kmaO_MaterialOption
{
  public:
    kmaO_PorousElastic( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_PorousElastic();
    kmaO_PorousElastic( const odb_SequenceSequenceDouble& table,
                        odb_String shear = "POISSON",
                        bool temperatureDependency = false,
                        int dependencies = 0 );
    kmaO_PorousElastic( const kmaO_PorousElastic& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_PorousElastic& operator=( const kmaO_PorousElastic& rhs );

    virtual ~kmaO_PorousElastic();
    odb_String shear() const;
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_PorousElastic* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_PorousElastic_h
