//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_ContactArea_h
#define kmaO_ContactArea_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_ContactArea
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_ContactArea;

class kmaO_ContactArea: public kmaO_MaterialOption
{
  public:
    kmaO_ContactArea( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_ContactArea();
    kmaO_ContactArea( const odb_SequenceSequenceDouble& table,
                      bool temperatureDependency = false,
                      int dependencies = 0 );
    kmaO_ContactArea( const kmaO_ContactArea& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_ContactArea& operator=( const kmaO_ContactArea& rhs );

    virtual ~kmaO_ContactArea();
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_ContactArea* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_ContactArea_h
