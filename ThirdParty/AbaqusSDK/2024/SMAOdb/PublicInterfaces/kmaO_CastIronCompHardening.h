//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_CastIronCompressionHardening_h
#define kmaO_CastIronCompressionHardening_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_CastIronCompressionHardening
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_CastIronCompHard;

class kmaO_CastIronCompressionHardening: public kmaO_MaterialOption
{
  public:
    kmaO_CastIronCompressionHardening( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_CastIronCompressionHardening();
    kmaO_CastIronCompressionHardening( const odb_SequenceSequenceDouble& table,
                                       bool temperatureDependency = false,
                                       int dependencies = 0 );
    kmaO_CastIronCompressionHardening( const kmaO_CastIronCompressionHardening& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_CastIronCompressionHardening& operator=( const kmaO_CastIronCompressionHardening& rhs );

    virtual ~kmaO_CastIronCompressionHardening();
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_CastIronCompHard* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_CastIronCompressionHardening_h
