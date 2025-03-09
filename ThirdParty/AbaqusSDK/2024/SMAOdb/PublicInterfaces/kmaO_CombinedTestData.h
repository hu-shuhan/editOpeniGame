//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_CombinedTestData_h
#define kmaO_CombinedTestData_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_CombinedTestData
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_Union.h>
// Forward declarations
class kmaC_CombinedTestData;

class kmaO_CombinedTestData: public kmaO_MaterialOption
{
  public:
    kmaO_CombinedTestData( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_CombinedTestData();
    kmaO_CombinedTestData( const odb_SequenceSequenceDouble& table,
                           odb_Union volinf = "NONE",
                           odb_Union shrinf = "NONE" );
    kmaO_CombinedTestData( const kmaO_CombinedTestData& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_CombinedTestData& operator=( const kmaO_CombinedTestData& rhs );

    virtual ~kmaO_CombinedTestData();
    odb_Union volinf() const;
    odb_Union shrinf() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_CombinedTestData* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_CombinedTestData_h
