//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_CastIronPlasticity_h
#define kmaO_CastIronPlasticity_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_CastIronPlasticity
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaO_CastIronTensionHardening;
class kmaO_CastIronCompressionHardening;
class kmaC_CastIronPlasticity;

class kmaO_CastIronPlasticity: public kmaO_MaterialOption
{
  public:
    kmaO_CastIronPlasticity( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_CastIronPlasticity();
    kmaO_CastIronPlasticity( const odb_SequenceSequenceDouble& table,
                             bool temperatureDependency = false,
                             int dependencies = 0 );
    kmaO_CastIronPlasticity( const kmaO_CastIronPlasticity& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_CastIronPlasticity& operator=( const kmaO_CastIronPlasticity& rhs );

    virtual ~kmaO_CastIronPlasticity();
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    kmaO_CastIronTensionHardening castIronTensionHardening() const;
    kmaO_CastIronCompressionHardening castIronCompressionHardening() const;
    kmaO_CastIronTensionHardening CastIronTensionHardening( const odb_SequenceSequenceDouble& table,
                                                            bool temperatureDependency = false,
                                                            int dependencies = 0 );
    kmaO_CastIronCompressionHardening CastIronCompressionHardening( const odb_SequenceSequenceDouble& table,
                                                                    bool temperatureDependency = false,
                                                                    int dependencies = 0 );
    const kmaC_CastIronPlasticity* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_CastIronPlasticity_h
