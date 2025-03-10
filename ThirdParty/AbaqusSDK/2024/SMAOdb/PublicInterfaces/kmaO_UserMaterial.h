//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_UserMaterial_h
#define kmaO_UserMaterial_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_UserMaterial
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
// Forward declarations
class kmaC_UserMaterial;

class kmaO_UserMaterial: public kmaO_MaterialOption
{
  public:
    kmaO_UserMaterial( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_UserMaterial( odb_String type = "MECHANICAL",
                       bool unsymm = false,
                       const odb_SequenceDouble& mechanicalConstants = odb_SequenceDouble(),
                       const odb_SequenceDouble& thermalConstants = odb_SequenceDouble() );
    kmaO_UserMaterial( const kmaO_UserMaterial& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_UserMaterial& operator=( const kmaO_UserMaterial& rhs );

    virtual ~kmaO_UserMaterial();
    odb_String type() const;
    bool unsymm() const;
    odb_SequenceDouble mechanicalConstants() const;
    odb_SequenceDouble thermalConstants() const;
    const kmaC_UserMaterial* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_UserMaterial_h
