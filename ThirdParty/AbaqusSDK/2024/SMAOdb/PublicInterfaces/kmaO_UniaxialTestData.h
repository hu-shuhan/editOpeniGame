//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_UniaxialTestData_h
#define kmaO_UniaxialTestData_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_UniaxialTestData
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_Union.h>
// Forward declarations
class kmaC_UniaxialTestData;

class kmaO_UniaxialTestData: public kmaO_MaterialOption
{
  public:
    kmaO_UniaxialTestData( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_UniaxialTestData();
    kmaO_UniaxialTestData( const odb_SequenceSequenceDouble& table,
                           odb_Union smoothing = "NONE",
                           bool lateralNominalStrain = false,
                           bool temperatureDependency = false,
                           int dependencies = 0 );
    kmaO_UniaxialTestData( const kmaO_UniaxialTestData& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_UniaxialTestData& operator=( const kmaO_UniaxialTestData& rhs );

    virtual ~kmaO_UniaxialTestData();
    odb_Union smoothing() const;
    bool lateralNominalStrain() const;
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_UniaxialTestData* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_UniaxialTestData_h
