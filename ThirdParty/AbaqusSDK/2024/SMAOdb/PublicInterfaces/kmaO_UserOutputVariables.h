//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_UserOutputVariables_h
#define kmaO_UserOutputVariables_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_UserOutputVariables
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_UserOutputVariables;

class kmaO_UserOutputVariables: public kmaO_MaterialOption
{
  public:
    kmaO_UserOutputVariables( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_UserOutputVariables( int n = 0 );
    kmaO_UserOutputVariables( const kmaO_UserOutputVariables& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_UserOutputVariables& operator=( const kmaO_UserOutputVariables& rhs );

    virtual ~kmaO_UserOutputVariables();
    int n() const;
    const kmaC_UserOutputVariables* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_UserOutputVariables_h
