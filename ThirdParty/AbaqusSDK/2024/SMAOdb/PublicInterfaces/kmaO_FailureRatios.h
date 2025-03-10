//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_FailureRatios_h
#define kmaO_FailureRatios_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_FailureRatios
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_FailureRatios;

class kmaO_FailureRatios: public kmaO_MaterialOption
{
  public:
    kmaO_FailureRatios( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_FailureRatios();
    kmaO_FailureRatios( const odb_SequenceSequenceDouble& table,
                        bool temperatureDependency = false,
                        int dependencies = 0 );
    kmaO_FailureRatios( const kmaO_FailureRatios& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_FailureRatios& operator=( const kmaO_FailureRatios& rhs );

    virtual ~kmaO_FailureRatios();
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_FailureRatios* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_FailureRatios_h
