//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_PlanarTestData_h
#define kmaO_PlanarTestData_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_PlanarTestData
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_Union.h>
// Forward declarations
class kmaC_PlanarTestData;

class kmaO_PlanarTestData: public kmaO_MaterialOption
{
  public:
    kmaO_PlanarTestData( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_PlanarTestData();
    kmaO_PlanarTestData( const odb_SequenceSequenceDouble& table,
                         odb_Union smoothing = "NONE",
                         bool lateralNominalStrain = false,
                         bool temperatureDependency = false,
                         int dependencies = 0 );
    kmaO_PlanarTestData( const kmaO_PlanarTestData& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_PlanarTestData& operator=( const kmaO_PlanarTestData& rhs );

    virtual ~kmaO_PlanarTestData();
    odb_Union smoothing() const;
    bool lateralNominalStrain() const;
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_PlanarTestData* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_PlanarTestData_h
