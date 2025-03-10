//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_Depvar_h
#define kmaO_Depvar_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_Depvar
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_Depvar;

class kmaO_Depvar: public kmaO_MaterialOption
{
  public:
    kmaO_Depvar( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_Depvar( int deleteVar = 0,
                 int n = 0 );
    kmaO_Depvar( const kmaO_Depvar& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_Depvar& operator=( const kmaO_Depvar& rhs );

    virtual ~kmaO_Depvar();
    int deleteVar() const;
    int n() const;
    const kmaC_Depvar* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_Depvar_h
