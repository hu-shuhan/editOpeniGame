//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_VolumetricTestData_h
#define kmaO_VolumetricTestData_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_VolumetricTestData
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_Union.h>
// Forward declarations
class kmaC_VolumetricTestData;

class kmaO_VolumetricTestData: public kmaO_MaterialOption
{
  public:
    kmaO_VolumetricTestData( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_VolumetricTestData();
    kmaO_VolumetricTestData( const odb_SequenceSequenceDouble& table,
                             odb_Union volinf = "NONE",
                             odb_Union smoothing = "NONE",
                             bool temperatureDependency = false,
                             int dependencies = 0 );
    kmaO_VolumetricTestData( const kmaO_VolumetricTestData& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_VolumetricTestData& operator=( const kmaO_VolumetricTestData& rhs );

    virtual ~kmaO_VolumetricTestData();
    odb_Union volinf() const;
    odb_Union smoothing() const;
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_VolumetricTestData* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_VolumetricTestData_h
