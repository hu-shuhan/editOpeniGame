//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_Regularization_h
#define kmaO_Regularization_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_Regularization
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
// Forward declarations
class kmaC_Regularization;

class kmaO_Regularization: public kmaO_MaterialOption
{
  public:
    kmaO_Regularization( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_Regularization( double rtol = 0.03,
                         odb_String strainRateRegularization = "LOGARITHMIC" );
    kmaO_Regularization( const kmaO_Regularization& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_Regularization& operator=( const kmaO_Regularization& rhs );

    virtual ~kmaO_Regularization();
    double rtol() const;
    odb_String strainRateRegularization() const;
    const kmaC_Regularization* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_Regularization_h
