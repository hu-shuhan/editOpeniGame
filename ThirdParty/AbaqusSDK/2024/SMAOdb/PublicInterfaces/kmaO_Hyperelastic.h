//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_Hyperelastic_h
#define kmaO_Hyperelastic_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_Hyperelastic
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
#include <odb_Union.h>
// Forward declarations
class kmaO_BiaxialTestData;
class kmaO_PlanarTestData;
class kmaO_UniaxialTestData;
class kmaO_VolumetricTestData;
class kmaO_Hysteresis;
class kmaC_Hyperelastic;

class kmaO_Hyperelastic: public kmaO_MaterialOption
{
  public:
    kmaO_Hyperelastic( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_Hyperelastic();
    kmaO_Hyperelastic( const odb_SequenceSequenceDouble& table,
                       odb_String type = "UNKNOWN",
                       odb_String moduliTimeScale = "LONG_TERM",
                       bool temperatureDependency = false,
                       int n = 1,
                       odb_Union beta = "FITTED_VALUE",
                       bool testData = true,
                       bool compressible = false,
                       int properties = 0,
                       odb_String deviatoricResponse = "UNIAXIAL",
                       odb_String volumetricResponse = "DEFAULT",
                       double poissonRatio = 0,
                       odb_String materialType = "ISOTROPIC",
                       odb_String anisotropicType = "FUNG_ANISOTROPIC",
                       odb_String formulation = "STRAIN",
                       odb_String behaviorType = "INCOMPRESSIBLE",
                       int dependencies = 0,
                       int localDirections = 0 );
    kmaO_Hyperelastic( const kmaO_Hyperelastic& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_Hyperelastic& operator=( const kmaO_Hyperelastic& rhs );

    virtual ~kmaO_Hyperelastic();
    odb_String type() const;
    odb_String moduliTimeScale() const;
    bool temperatureDependency() const;
    int n() const;
    odb_Union beta() const;
    bool testData() const;
    bool compressible() const;
    int properties() const;
    odb_String deviatoricResponse() const;
    odb_String volumetricResponse() const;
    double poissonRatio() const;
    odb_String materialType() const;
    odb_String anisotropicType() const;
    odb_String formulation() const;
    odb_String behaviorType() const;
    int dependencies() const;
    int localDirections() const;
    odb_SequenceSequenceDouble table() const;
    kmaO_BiaxialTestData biaxialTestData() const;
    kmaO_PlanarTestData planarTestData() const;
    kmaO_UniaxialTestData uniaxialTestData() const;
    kmaO_VolumetricTestData volumetricTestData() const;
    kmaO_Hysteresis hysteresis() const;
    kmaO_BiaxialTestData BiaxialTestData( const odb_SequenceSequenceDouble& table,
                                          odb_Union smoothing = "NONE",
                                          bool lateralNominalStrain = false,
                                          bool temperatureDependency = false,
                                          int dependencies = 0 );
    kmaO_PlanarTestData PlanarTestData( const odb_SequenceSequenceDouble& table,
                                        odb_Union smoothing = "NONE",
                                        bool lateralNominalStrain = false,
                                        bool temperatureDependency = false,
                                        int dependencies = 0 );
    kmaO_UniaxialTestData UniaxialTestData( const odb_SequenceSequenceDouble& table,
                                            odb_Union smoothing = "NONE",
                                            bool lateralNominalStrain = false,
                                            bool temperatureDependency = false,
                                            int dependencies = 0 );
    kmaO_VolumetricTestData VolumetricTestData( const odb_SequenceSequenceDouble& table,
                                                odb_Union volinf = "NONE",
                                                odb_Union smoothing = "NONE",
                                                bool temperatureDependency = false,
                                                int dependencies = 0 );
    kmaO_Hysteresis Hysteresis( const odb_SequenceSequenceDouble& table );
    const kmaC_Hyperelastic* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_Hyperelastic_h
