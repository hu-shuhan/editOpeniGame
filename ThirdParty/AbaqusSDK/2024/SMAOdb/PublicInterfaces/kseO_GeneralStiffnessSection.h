//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kseO_GeneralStiffnessSection_h
#define kseO_GeneralStiffnessSection_h
///////////////////////////////////////////////////////////////////////////////
// kseO_GeneralStiffnessSection
//

// Begin local includes
#include <odb_Types.h>
#include <kseO_GeometryShellSection.h>
#include <odb_Union.h>
#include <odb_String.h>
#include <kseO_SequenceLayerProperties.h>
// Forward declarations
class kseO_RebarLayers;
class kseO_TransverseShearShell;
class kseC_GeneralStiffnessSection;

class kseO_GeneralStiffnessSection: public kseO_GeometryShellSection
{
  public:
    kseO_GeneralStiffnessSection( const kseC_SectionShortcut& shortcut );
    kseO_GeneralStiffnessSection();
    kseO_GeneralStiffnessSection( const kseO_GeneralStiffnessSection& copy );
    virtual odb_Section* Copy() const;
    kseO_GeneralStiffnessSection& operator=( const kseO_GeneralStiffnessSection& rhs );

    virtual ~kseO_GeneralStiffnessSection();
    odb_Union referenceTemperature() const;
    bool applyThermalStress() const;
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceDouble stiffnessMatrix() const;
    odb_SequenceDouble thermalStresses() const;
    odb_SequenceSequenceDouble scalingData() const;
    static unsigned int typeIdentifier();
};

#endif // kseO_GeneralStiffnessSection_h
