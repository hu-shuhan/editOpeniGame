//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_SimpleShearTestData_h
#define kmaO_SimpleShearTestData_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_SimpleShearTestData
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_SimpleShearTestData;

class kmaO_SimpleShearTestData: public kmaO_MaterialOption
{
  public:
    kmaO_SimpleShearTestData( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_SimpleShearTestData();
    kmaO_SimpleShearTestData( const odb_SequenceSequenceDouble& table );
    kmaO_SimpleShearTestData( const kmaO_SimpleShearTestData& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_SimpleShearTestData& operator=( const kmaO_SimpleShearTestData& rhs );

    virtual ~kmaO_SimpleShearTestData();
    odb_SequenceSequenceDouble table() const;
    const kmaC_SimpleShearTestData* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_SimpleShearTestData_h
