//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_RateDependent_h
#define kmaO_RateDependent_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_RateDependent
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
// Forward declarations
class kmaC_RateDependent;

class kmaO_RateDependent: public kmaO_MaterialOption
{
  public:
    kmaO_RateDependent( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_RateDependent();
    kmaO_RateDependent( const odb_SequenceSequenceDouble& table,
                        odb_String type = "POWER_LAW",
                        bool temperatureDependency = false,
                        int dependencies = 0 );
    kmaO_RateDependent( const kmaO_RateDependent& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_RateDependent& operator=( const kmaO_RateDependent& rhs );

    virtual ~kmaO_RateDependent();
    odb_String type() const;
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_RateDependent* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_RateDependent_h
