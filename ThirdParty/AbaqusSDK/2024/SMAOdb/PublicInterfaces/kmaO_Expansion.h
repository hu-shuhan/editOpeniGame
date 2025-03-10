//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_Expansion_h
#define kmaO_Expansion_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_Expansion
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
// Forward declarations
class kmaC_Expansion;

class kmaO_Expansion: public kmaO_MaterialOption
{
  public:
    kmaO_Expansion( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_Expansion( odb_String type = "ISOTROPIC",
                    bool userSubroutine = false,
                    double zero = 0,
                    bool temperatureDependency = false,
                    int dependencies = 0,
                    const odb_SequenceSequenceDouble& table = odb_SequenceSequenceDouble() );
    kmaO_Expansion( const kmaO_Expansion& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_Expansion& operator=( const kmaO_Expansion& rhs );

    virtual ~kmaO_Expansion();
    odb_String type() const;
    bool userSubroutine() const;
    double zero() const;
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_Expansion* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_Expansion_h
