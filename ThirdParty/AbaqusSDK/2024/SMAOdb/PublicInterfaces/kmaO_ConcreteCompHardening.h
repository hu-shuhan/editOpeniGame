//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_ConcreteCompressionHardening_h
#define kmaO_ConcreteCompressionHardening_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_ConcreteCompressionHardening
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_ConcreteCompressionHardening;

class kmaO_ConcreteCompressionHardening: public kmaO_MaterialOption
{
  public:
    kmaO_ConcreteCompressionHardening( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_ConcreteCompressionHardening();
    kmaO_ConcreteCompressionHardening( const odb_SequenceSequenceDouble& table,
                                       bool rate = false,
                                       bool temperatureDependency = false,
                                       int dependencies = 0 );
    kmaO_ConcreteCompressionHardening( const kmaO_ConcreteCompressionHardening& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_ConcreteCompressionHardening& operator=( const kmaO_ConcreteCompressionHardening& rhs );

    virtual ~kmaO_ConcreteCompressionHardening();
    bool rate() const;
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_ConcreteCompressionHardening* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_ConcreteCompressionHardening_h
