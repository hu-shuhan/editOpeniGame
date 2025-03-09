//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_HeatGeneration_h
#define kmaO_HeatGeneration_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_HeatGeneration
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_HeatGeneration;

class kmaO_HeatGeneration: public kmaO_MaterialOption
{
  public:
    kmaO_HeatGeneration( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_HeatGeneration();
    kmaO_HeatGeneration( const kmaO_HeatGeneration& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_HeatGeneration& operator=( const kmaO_HeatGeneration& rhs );

    virtual ~kmaO_HeatGeneration();
    const kmaC_HeatGeneration* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_HeatGeneration_h
