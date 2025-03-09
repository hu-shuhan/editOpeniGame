//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_Dielectric_h
#define kmaO_Dielectric_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_Dielectric
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
// Forward declarations
class kmaC_Dielectric;

class kmaO_Dielectric: public kmaO_MaterialOption
{
  public:
    kmaO_Dielectric( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_Dielectric();
    kmaO_Dielectric( const odb_SequenceSequenceDouble& table,
                     odb_String type = "ISOTROPIC",
                     bool temperatureDependency = false,
                     int dependencies = 0 );
    kmaO_Dielectric( const kmaO_Dielectric& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_Dielectric& operator=( const kmaO_Dielectric& rhs );

    virtual ~kmaO_Dielectric();
    odb_String type() const;
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_Dielectric* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_Dielectric_h
