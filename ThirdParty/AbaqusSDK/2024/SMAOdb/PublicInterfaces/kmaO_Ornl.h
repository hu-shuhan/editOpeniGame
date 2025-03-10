//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_Ornl_h
#define kmaO_Ornl_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_Ornl
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_Union.h>
// Forward declarations
class kmaC_Ornl;

class kmaO_Ornl: public kmaO_MaterialOption
{
  public:
    kmaO_Ornl( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_Ornl( double a = 0.3,
               odb_Union h = "NONE",
               bool reset = false );
    kmaO_Ornl( const kmaO_Ornl& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_Ornl& operator=( const kmaO_Ornl& rhs );

    virtual ~kmaO_Ornl();
    double a() const;
    odb_Union h() const;
    bool reset() const;
    const kmaC_Ornl* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_Ornl_h
