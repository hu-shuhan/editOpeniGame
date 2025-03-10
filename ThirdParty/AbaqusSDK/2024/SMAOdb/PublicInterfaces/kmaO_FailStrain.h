//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_FailStrain_h
#define kmaO_FailStrain_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_FailStrain
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_FailStrain;

class kmaO_FailStrain: public kmaO_MaterialOption
{
  public:
    kmaO_FailStrain( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_FailStrain();
    kmaO_FailStrain( const odb_SequenceSequenceDouble& table,
                     bool temperatureDependency = false,
                     int dependencies = 0 );
    kmaO_FailStrain( const kmaO_FailStrain& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_FailStrain& operator=( const kmaO_FailStrain& rhs );

    virtual ~kmaO_FailStrain();
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_FailStrain* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_FailStrain_h
