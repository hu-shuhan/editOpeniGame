//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_TriaxialTestData_h
#define kmaO_TriaxialTestData_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_TriaxialTestData
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_Union.h>
// Forward declarations
class kmaC_TriaxialTestData;

class kmaO_TriaxialTestData: public kmaO_MaterialOption
{
  public:
    kmaO_TriaxialTestData( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_TriaxialTestData();
    kmaO_TriaxialTestData( const odb_SequenceSequenceDouble& table,
                           odb_Union a = "NONE",
                           odb_Union b = "NONE",
                           odb_Union pt = "NONE" );
    kmaO_TriaxialTestData( const kmaO_TriaxialTestData& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_TriaxialTestData& operator=( const kmaO_TriaxialTestData& rhs );

    virtual ~kmaO_TriaxialTestData();
    odb_Union a() const;
    odb_Union b() const;
    odb_Union pt() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_TriaxialTestData* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_TriaxialTestData_h
