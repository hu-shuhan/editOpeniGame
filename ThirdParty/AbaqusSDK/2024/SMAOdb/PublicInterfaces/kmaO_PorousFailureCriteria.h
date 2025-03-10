//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_PorousFailureCriteria_h
#define kmaO_PorousFailureCriteria_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_PorousFailureCriteria
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_PorousFailureCriteria;

class kmaO_PorousFailureCriteria: public kmaO_MaterialOption
{
  public:
    kmaO_PorousFailureCriteria( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_PorousFailureCriteria( double fraction = 1,
                                double criticalFraction = 1 );
    kmaO_PorousFailureCriteria( const kmaO_PorousFailureCriteria& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_PorousFailureCriteria& operator=( const kmaO_PorousFailureCriteria& rhs );

    virtual ~kmaO_PorousFailureCriteria();
    double fraction() const;
    double criticalFraction() const;
    const kmaC_PorousFailureCriteria* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_PorousFailureCriteria_h
