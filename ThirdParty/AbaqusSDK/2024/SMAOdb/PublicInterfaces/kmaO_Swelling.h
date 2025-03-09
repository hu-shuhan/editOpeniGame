//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_Swelling_h
#define kmaO_Swelling_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_Swelling
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
// Forward declarations
class kmaO_Ratios;
class kmaC_Swelling;

class kmaO_Swelling: public kmaO_MaterialOption
{
  public:
    kmaO_Swelling( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_Swelling();
    kmaO_Swelling( const odb_SequenceSequenceDouble& table,
                   odb_String law = "INPUT",
                   bool temperatureDependency = false,
                   int dependencies = 0 );
    kmaO_Swelling( const kmaO_Swelling& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_Swelling& operator=( const kmaO_Swelling& rhs );

    virtual ~kmaO_Swelling();
    odb_String law() const;
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    kmaO_Ratios ratios() const;
    kmaO_Ratios Ratios( const odb_SequenceSequenceDouble& table,
                        bool temperatureDependency = false,
                        int dependencies = 0 );
    const kmaC_Swelling* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_Swelling_h
