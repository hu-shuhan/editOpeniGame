//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_ConcreteTensionStiffening_h
#define kmaO_ConcreteTensionStiffening_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_ConcreteTensionStiffening
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
// Forward declarations
class kmaC_ConcreteTensionStiffening;

class kmaO_ConcreteTensionStiffening: public kmaO_MaterialOption
{
  public:
    kmaO_ConcreteTensionStiffening( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_ConcreteTensionStiffening();
    kmaO_ConcreteTensionStiffening( const odb_SequenceSequenceDouble& table,
                                    bool rate = false,
                                    odb_String type = "STRAIN",
                                    bool temperatureDependency = false,
                                    int dependencies = 0 );
    kmaO_ConcreteTensionStiffening( const kmaO_ConcreteTensionStiffening& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_ConcreteTensionStiffening& operator=( const kmaO_ConcreteTensionStiffening& rhs );

    virtual ~kmaO_ConcreteTensionStiffening();
    bool rate() const;
    odb_String type() const;
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_ConcreteTensionStiffening* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_ConcreteTensionStiffening_h
