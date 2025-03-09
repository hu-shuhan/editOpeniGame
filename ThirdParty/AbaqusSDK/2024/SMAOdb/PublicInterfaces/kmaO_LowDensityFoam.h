//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_LowDensityFoam_h
#define kmaO_LowDensityFoam_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_LowDensityFoam
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_Union.h>
#include <odb_String.h>
// Forward declarations
class kmaO_UniaxialTestData;
class kmaC_LowDensityFoam;

class kmaO_LowDensityFoam: public kmaO_MaterialOption
{
  public:
    kmaO_LowDensityFoam( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_LowDensityFoam( bool elementRemoval = false,
                         odb_Union maxAllowablePrincipalStress = "NONE",
                         bool extrapolateStressStrainCurve = false,
                         odb_String strainRateType = "VOLUMETRIC",
                         double mu0 = 0.0001,
                         double mu1 = 0.005,
                         double alpha = 2.0 );
    kmaO_LowDensityFoam( const kmaO_LowDensityFoam& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_LowDensityFoam& operator=( const kmaO_LowDensityFoam& rhs );

    virtual ~kmaO_LowDensityFoam();
    bool elementRemoval() const;
    odb_Union maxAllowablePrincipalStress() const;
    bool extrapolateStressStrainCurve() const;
    odb_String strainRateType() const;
    double mu0() const;
    double mu1() const;
    double alpha() const;
    kmaO_UniaxialTestData uniaxialTensionTestData() const;
    kmaO_UniaxialTestData uniaxialCompressionTestData() const;
    kmaO_UniaxialTestData UniaxialTensionTestData( const odb_SequenceSequenceDouble& table,
                                                   odb_Union smoothing = "NONE",
                                                   bool lateralNominalStrain = false,
                                                   bool temperatureDependency = false,
                                                   int dependencies = 0 );
    kmaO_UniaxialTestData UniaxialCompressionTestData( const odb_SequenceSequenceDouble& table,
                                                       odb_Union smoothing = "NONE",
                                                       bool lateralNominalStrain = false,
                                                       bool temperatureDependency = false,
                                                       int dependencies = 0 );
    const kmaC_LowDensityFoam* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_LowDensityFoam_h
