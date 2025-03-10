//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_Hyperfoam_h
#define kmaO_Hyperfoam_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_Hyperfoam
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_Union.h>
#include <odb_String.h>
// Forward declarations
class kmaO_BiaxialTestData;
class kmaO_VolumetricTestData;
class kmaO_PlanarTestData;
class kmaO_SimpleShearTestData;
class kmaO_UniaxialTestData;
class kmaC_Hyperfoam;

class kmaO_Hyperfoam: public kmaO_MaterialOption
{
  public:
    kmaO_Hyperfoam( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_Hyperfoam( bool testData = false,
                    odb_Union poisson = "NONE",
                    int n = 1,
                    bool temperatureDependency = false,
                    odb_String moduli = "LONG_TERM",
                    const odb_SequenceSequenceDouble& table = odb_SequenceSequenceDouble() );
    kmaO_Hyperfoam( const kmaO_Hyperfoam& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_Hyperfoam& operator=( const kmaO_Hyperfoam& rhs );

    virtual ~kmaO_Hyperfoam();
    bool testData() const;
    odb_Union poisson() const;
    int n() const;
    bool temperatureDependency() const;
    odb_String moduli() const;
    odb_SequenceSequenceDouble table() const;
    kmaO_BiaxialTestData biaxialTestData() const;
    kmaO_VolumetricTestData volumetricTestData() const;
    kmaO_PlanarTestData planarTestData() const;
    kmaO_SimpleShearTestData simpleShearTestData() const;
    kmaO_UniaxialTestData uniaxialTestData() const;
    kmaO_BiaxialTestData BiaxialTestData( const odb_SequenceSequenceDouble& table,
                                          odb_Union smoothing = "NONE",
                                          bool lateralNominalStrain = false,
                                          bool temperatureDependency = false,
                                          int dependencies = 0 );
    kmaO_VolumetricTestData VolumetricTestData( const odb_SequenceSequenceDouble& table,
                                                odb_Union volinf = "NONE",
                                                odb_Union smoothing = "NONE",
                                                bool temperatureDependency = false,
                                                int dependencies = 0 );
    kmaO_PlanarTestData PlanarTestData( const odb_SequenceSequenceDouble& table,
                                        odb_Union smoothing = "NONE",
                                        bool lateralNominalStrain = false,
                                        bool temperatureDependency = false,
                                        int dependencies = 0 );
    kmaO_SimpleShearTestData SimpleShearTestData( const odb_SequenceSequenceDouble& table );
    kmaO_UniaxialTestData UniaxialTestData( const odb_SequenceSequenceDouble& table,
                                            odb_Union smoothing = "NONE",
                                            bool lateralNominalStrain = false,
                                            bool temperatureDependency = false,
                                            int dependencies = 0 );
    const kmaC_Hyperfoam* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_Hyperfoam_h
