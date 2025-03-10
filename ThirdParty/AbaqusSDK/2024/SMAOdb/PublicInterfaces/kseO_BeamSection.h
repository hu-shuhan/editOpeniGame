//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kseO_BeamSection_h
#define kseO_BeamSection_h
///////////////////////////////////////////////////////////////////////////////
// kseO_BeamSection
//

// Begin local includes
#include <odb_Types.h>
#include <odb_Section.h>
#include <odb_String.h>
#include <odb_Union.h>
// Forward declarations
class kseO_TransverseShearBeam;
class kseC_BeamSection;

class kseO_BeamSection: public odb_Section
{
  public:
    kseO_BeamSection( const kseC_SectionShortcut& shortcut );
    kseO_BeamSection();
    kseO_BeamSection( const kseO_BeamSection& copy );
    virtual odb_Section* Copy() const;
    kseO_BeamSection& operator=( const kseO_BeamSection& rhs );

    virtual ~kseO_BeamSection();
    odb_String integration() const;
    double poissonRatio() const;
    bool thermalExpansion() const;
    bool temperatureDependency() const;
    int dependencies() const;
    odb_Union density() const;
    odb_Union referenceTemperature() const;
    odb_String temperatureVar() const;
    double alphaDamping() const;
    double betaDamping() const;
    double compositeDamping() const;
    bool useFluidInertia() const;
    odb_String submerged() const;
    odb_Union fluidMassDensity() const;
    odb_Union crossSectionRadius() const;
    double lateralMassCoef() const;
    double axialMassCoef() const;
    double massOffsetX() const;
    double massOffsetY() const;
    odb_String profile() const;
    odb_String material() const;
    odb_SequenceSequenceDouble table() const;
    odb_SequenceSequenceDouble outputPts() const;
    kseO_TransverseShearBeam beamTransverseShear() const;
    odb_SequenceDouble centroid() const;
    odb_SequenceDouble shearCenter() const;
    kseO_TransverseShearBeam BeamTransverseShear( odb_Union k23 = "NONE",
                                                  odb_Union k13 = "NONE",
                                                  odb_Union slendernessCompensation = 0.25 );
    static unsigned int typeIdentifier();
};

#endif // kseO_BeamSection_h
