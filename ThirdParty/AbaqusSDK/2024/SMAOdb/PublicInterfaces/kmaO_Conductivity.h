//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_Conductivity_h
#define kmaO_Conductivity_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_Conductivity
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
// Forward declarations
class kmaC_Conductivity;

class kmaO_Conductivity: public kmaO_MaterialOption
{
  public:
    kmaO_Conductivity( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_Conductivity();
    kmaO_Conductivity( const odb_SequenceSequenceDouble& table,
                       odb_String type = "ISOTROPIC",
                       bool temperatureDependency = false,
                       int dependencies = 0 );
    kmaO_Conductivity( const kmaO_Conductivity& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_Conductivity& operator=( const kmaO_Conductivity& rhs );

    virtual ~kmaO_Conductivity();
    odb_String type() const;
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_Conductivity* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_Conductivity_h
