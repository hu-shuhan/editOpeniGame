//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_UserDefinedField_h
#define kmaO_UserDefinedField_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_UserDefinedField
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_UserDefinedField;

class kmaO_UserDefinedField: public kmaO_MaterialOption
{
  public:
    kmaO_UserDefinedField( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_UserDefinedField();
    kmaO_UserDefinedField( const kmaO_UserDefinedField& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_UserDefinedField& operator=( const kmaO_UserDefinedField& rhs );

    virtual ~kmaO_UserDefinedField();
    const kmaC_UserDefinedField* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_UserDefinedField_h
