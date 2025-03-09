//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_FluidLeakoff_h
#define kmaO_FluidLeakoff_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_FluidLeakoff
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
// Forward declarations
class kmaC_FluidLeakoff;

class kmaO_FluidLeakoff: public kmaO_MaterialOption
{
  public:
    kmaO_FluidLeakoff( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_FluidLeakoff( bool temperatureDependency = false,
                       int dependencies = 0,
                       odb_String type = "COEFFICIENTS",
                       const odb_SequenceSequenceDouble& table = odb_SequenceSequenceDouble() );
    kmaO_FluidLeakoff( const kmaO_FluidLeakoff& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_FluidLeakoff& operator=( const kmaO_FluidLeakoff& rhs );

    virtual ~kmaO_FluidLeakoff();
    bool temperatureDependency() const;
    int dependencies() const;
    odb_String type() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_FluidLeakoff* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_FluidLeakoff_h
