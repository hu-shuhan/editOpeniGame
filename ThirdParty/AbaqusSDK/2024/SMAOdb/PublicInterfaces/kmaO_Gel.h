//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_Gel_h
#define kmaO_Gel_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_Gel
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_Gel;

class kmaO_Gel: public kmaO_MaterialOption
{
  public:
    kmaO_Gel( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_Gel();
    kmaO_Gel( const odb_SequenceSequenceDouble& table );
    kmaO_Gel( const kmaO_Gel& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_Gel& operator=( const kmaO_Gel& rhs );

    virtual ~kmaO_Gel();
    odb_SequenceSequenceDouble table() const;
    const kmaC_Gel* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_Gel_h
