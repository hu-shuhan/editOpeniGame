//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_AcousticMedium_h
#define kmaO_AcousticMedium_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_AcousticMedium
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_AcousticMedium;

class kmaO_AcousticMedium: public kmaO_MaterialOption
{
  public:
    kmaO_AcousticMedium( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_AcousticMedium( bool acousticVolumetricDrag = false,
                         bool temperatureDependencyB = false,
                         bool temperatureDependencyV = false,
                         int dependenciesB = 0,
                         int dependenciesV = 0,
                         const odb_SequenceSequenceDouble& bulkTable = odb_SequenceSequenceDouble(),
                         const odb_SequenceSequenceDouble& volumetricTable = odb_SequenceSequenceDouble() );
    kmaO_AcousticMedium( const kmaO_AcousticMedium& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_AcousticMedium& operator=( const kmaO_AcousticMedium& rhs );

    virtual ~kmaO_AcousticMedium();
    bool acousticVolumetricDrag() const;
    bool temperatureDependencyB() const;
    bool temperatureDependencyV() const;
    int dependenciesB() const;
    int dependenciesV() const;
    odb_SequenceSequenceDouble bulkTable() const;
    odb_SequenceSequenceDouble volumetricTable() const;
    const kmaC_AcousticMedium* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_AcousticMedium_h
