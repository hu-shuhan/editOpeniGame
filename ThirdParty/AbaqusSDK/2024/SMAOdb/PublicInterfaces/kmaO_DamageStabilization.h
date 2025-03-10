//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_DamageStabilization_h
#define kmaO_DamageStabilization_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_DamageStabilization
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_DamageStabilization;

class kmaO_DamageStabilization: public kmaO_MaterialOption
{
  public:
    kmaO_DamageStabilization( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_DamageStabilization();
    kmaO_DamageStabilization( double fiberTensileCoeff,
                              double fiberCompressiveCoeff,
                              double matrixTensileCoeff,
                              double matrixCompressiveCoeff );
    kmaO_DamageStabilization( const kmaO_DamageStabilization& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_DamageStabilization& operator=( const kmaO_DamageStabilization& rhs );

    virtual ~kmaO_DamageStabilization();
    double fiberTensileCoeff() const;
    double fiberCompressiveCoeff() const;
    double matrixTensileCoeff() const;
    double matrixCompressiveCoeff() const;
    const kmaC_DamageStabilization* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_DamageStabilization_h
