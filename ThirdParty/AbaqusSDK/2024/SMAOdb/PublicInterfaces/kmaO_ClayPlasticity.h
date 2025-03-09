//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_ClayPlasticity_h
#define kmaO_ClayPlasticity_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_ClayPlasticity
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_Union.h>
#include <odb_String.h>
// Forward declarations
class kmaO_ClayHardening;
class kmaC_ClayPlasticity;

class kmaO_ClayPlasticity: public kmaO_MaterialOption
{
  public:
    kmaO_ClayPlasticity( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_ClayPlasticity();
    kmaO_ClayPlasticity( const odb_SequenceSequenceDouble& table,
                         odb_Union intercept = "NONE",
                         odb_String hardening = "EXPONENTIAL",
                         bool temperatureDependency = false,
                         int dependencies = 0 );
    kmaO_ClayPlasticity( const kmaO_ClayPlasticity& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_ClayPlasticity& operator=( const kmaO_ClayPlasticity& rhs );

    virtual ~kmaO_ClayPlasticity();
    odb_Union intercept() const;
    odb_String hardening() const;
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    kmaO_ClayHardening clayHardening() const;
    kmaO_ClayHardening ClayHardening( const odb_SequenceSequenceDouble& table,
                                      bool temperatureDependency = false,
                                      int dependencies = 0 );
    const kmaC_ClayPlasticity* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_ClayPlasticity_h
