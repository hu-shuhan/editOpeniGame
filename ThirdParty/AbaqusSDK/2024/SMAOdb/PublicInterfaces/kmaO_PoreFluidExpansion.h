//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_PoreFluidExpansion_h
#define kmaO_PoreFluidExpansion_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_PoreFluidExpansion
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_PoreFluidExpansion;

class kmaO_PoreFluidExpansion: public kmaO_MaterialOption
{
  public:
    kmaO_PoreFluidExpansion( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_PoreFluidExpansion();
    kmaO_PoreFluidExpansion( const odb_SequenceSequenceDouble& table,
                             double zero = 0,
                             bool temperatureDependency = false,
                             int dependencies = 0 );
    kmaO_PoreFluidExpansion( const kmaO_PoreFluidExpansion& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_PoreFluidExpansion& operator=( const kmaO_PoreFluidExpansion& rhs );

    virtual ~kmaO_PoreFluidExpansion();
    double zero() const;
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_PoreFluidExpansion* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_PoreFluidExpansion_h
