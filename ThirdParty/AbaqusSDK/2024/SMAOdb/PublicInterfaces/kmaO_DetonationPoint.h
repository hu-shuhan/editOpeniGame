//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_DetonationPoint_h
#define kmaO_DetonationPoint_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_DetonationPoint
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_DetonationPoint;

class kmaO_DetonationPoint: public kmaO_MaterialOption
{
  public:
    kmaO_DetonationPoint( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_DetonationPoint();
    kmaO_DetonationPoint( const odb_SequenceSequenceDouble& table );
    kmaO_DetonationPoint( const kmaO_DetonationPoint& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_DetonationPoint& operator=( const kmaO_DetonationPoint& rhs );

    virtual ~kmaO_DetonationPoint();
    odb_SequenceSequenceDouble table() const;
    const kmaC_DetonationPoint* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_DetonationPoint_h
