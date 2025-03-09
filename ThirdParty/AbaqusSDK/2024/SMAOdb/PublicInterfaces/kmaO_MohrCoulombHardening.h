//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_MohrCoulombHardening_h
#define kmaO_MohrCoulombHardening_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_MohrCoulombHardening
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_MohrCoulombHardening;

class kmaO_MohrCoulombHardening: public kmaO_MaterialOption
{
  public:
    kmaO_MohrCoulombHardening( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_MohrCoulombHardening();
    kmaO_MohrCoulombHardening( const odb_SequenceSequenceDouble& table,
                               bool temperatureDependency = false,
                               int dependencies = 0 );
    kmaO_MohrCoulombHardening( const kmaO_MohrCoulombHardening& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_MohrCoulombHardening& operator=( const kmaO_MohrCoulombHardening& rhs );

    virtual ~kmaO_MohrCoulombHardening();
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_MohrCoulombHardening* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_MohrCoulombHardening_h
