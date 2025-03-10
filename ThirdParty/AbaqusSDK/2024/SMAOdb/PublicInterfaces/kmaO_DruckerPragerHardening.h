//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_DruckerPragerHardening_h
#define kmaO_DruckerPragerHardening_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_DruckerPragerHardening
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
// Forward declarations
class kmaC_DruckerPragerHardening;

class kmaO_DruckerPragerHardening: public kmaO_MaterialOption
{
  public:
    kmaO_DruckerPragerHardening( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_DruckerPragerHardening();
    kmaO_DruckerPragerHardening( const odb_SequenceSequenceDouble& table,
                                 odb_String type = "COMPRESSION",
                                 bool rate = false,
                                 bool temperatureDependency = false,
                                 int dependencies = 0 );
    kmaO_DruckerPragerHardening( const kmaO_DruckerPragerHardening& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_DruckerPragerHardening& operator=( const kmaO_DruckerPragerHardening& rhs );

    virtual ~kmaO_DruckerPragerHardening();
    odb_String type() const;
    bool rate() const;
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_DruckerPragerHardening* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_DruckerPragerHardening_h
