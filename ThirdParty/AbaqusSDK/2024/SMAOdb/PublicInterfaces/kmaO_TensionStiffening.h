//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_TensionStiffening_h
#define kmaO_TensionStiffening_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_TensionStiffening
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
// Forward declarations
class kmaC_TensionStiffening;

class kmaO_TensionStiffening: public kmaO_MaterialOption
{
  public:
    kmaO_TensionStiffening( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_TensionStiffening();
    kmaO_TensionStiffening( const odb_SequenceSequenceDouble& table,
                            odb_String type = "STRAIN",
                            bool temperatureDependency = false,
                            int dependencies = 0 );
    kmaO_TensionStiffening( const kmaO_TensionStiffening& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_TensionStiffening& operator=( const kmaO_TensionStiffening& rhs );

    virtual ~kmaO_TensionStiffening();
    odb_String type() const;
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_TensionStiffening* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_TensionStiffening_h
