//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_BiaxialTestData_h
#define kmaO_BiaxialTestData_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_BiaxialTestData
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_Union.h>
// Forward declarations
class kmaC_BiaxialTestData;

class kmaO_BiaxialTestData: public kmaO_MaterialOption
{
  public:
    kmaO_BiaxialTestData( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_BiaxialTestData();
    kmaO_BiaxialTestData( const odb_SequenceSequenceDouble& table,
                          odb_Union smoothing = "NONE",
                          bool lateralNominalStrain = false,
                          bool temperatureDependency = false,
                          int dependencies = 0 );
    kmaO_BiaxialTestData( const kmaO_BiaxialTestData& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_BiaxialTestData& operator=( const kmaO_BiaxialTestData& rhs );

    virtual ~kmaO_BiaxialTestData();
    odb_Union smoothing() const;
    bool lateralNominalStrain() const;
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_BiaxialTestData* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_BiaxialTestData_h
