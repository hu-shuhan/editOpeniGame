//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_JouleHeatFraction_h
#define kmaO_JouleHeatFraction_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_JouleHeatFraction
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_JouleHeatFraction;

class kmaO_JouleHeatFraction: public kmaO_MaterialOption
{
  public:
    kmaO_JouleHeatFraction( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_JouleHeatFraction( double fraction = 1 );
    kmaO_JouleHeatFraction( const kmaO_JouleHeatFraction& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_JouleHeatFraction& operator=( const kmaO_JouleHeatFraction& rhs );

    virtual ~kmaO_JouleHeatFraction();
    double fraction() const;
    const kmaC_JouleHeatFraction* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_JouleHeatFraction_h
