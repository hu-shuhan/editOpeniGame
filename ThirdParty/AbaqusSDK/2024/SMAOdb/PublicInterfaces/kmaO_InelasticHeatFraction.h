//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_InelasticHeatFraction_h
#define kmaO_InelasticHeatFraction_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_InelasticHeatFraction
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_InelasticHeatFraction;

class kmaO_InelasticHeatFraction: public kmaO_MaterialOption
{
  public:
    kmaO_InelasticHeatFraction( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_InelasticHeatFraction( double fraction = 0.9 );
    kmaO_InelasticHeatFraction( const kmaO_InelasticHeatFraction& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_InelasticHeatFraction& operator=( const kmaO_InelasticHeatFraction& rhs );

    virtual ~kmaO_InelasticHeatFraction();
    double fraction() const;
    const kmaC_InelasticHeatFraction* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_InelasticHeatFraction_h
