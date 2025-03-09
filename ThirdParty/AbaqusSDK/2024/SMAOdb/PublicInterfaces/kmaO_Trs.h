//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_Trs_h
#define kmaO_Trs_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_Trs
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
// Forward declarations
class kmaC_Trs;

class kmaO_Trs: public kmaO_MaterialOption
{
  public:
    kmaO_Trs( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_Trs( odb_String definition = "WLF",
              const odb_SequenceSequenceDouble& table = odb_SequenceSequenceDouble() );
    kmaO_Trs( const kmaO_Trs& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_Trs& operator=( const kmaO_Trs& rhs );

    virtual ~kmaO_Trs();
    odb_String definition() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_Trs* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_Trs_h
