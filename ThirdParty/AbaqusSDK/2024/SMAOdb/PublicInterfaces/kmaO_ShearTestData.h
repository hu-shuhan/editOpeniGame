//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_ShearTestData_h
#define kmaO_ShearTestData_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_ShearTestData
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_Union.h>
// Forward declarations
class kmaC_ShearTestData;

class kmaO_ShearTestData: public kmaO_MaterialOption
{
  public:
    kmaO_ShearTestData( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_ShearTestData();
    kmaO_ShearTestData( const odb_SequenceSequenceDouble& table,
                        odb_Union shrinf = "NONE" );
    kmaO_ShearTestData( const kmaO_ShearTestData& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_ShearTestData& operator=( const kmaO_ShearTestData& rhs );

    virtual ~kmaO_ShearTestData();
    odb_Union shrinf() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_ShearTestData* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_ShearTestData_h
