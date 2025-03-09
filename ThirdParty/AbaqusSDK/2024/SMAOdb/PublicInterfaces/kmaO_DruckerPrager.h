//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_DruckerPrager_h
#define kmaO_DruckerPrager_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_DruckerPrager
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
#include <odb_Union.h>
// Forward declarations
class kmaO_DruckerPragerCreep;
class kmaO_DruckerPragerHardening;
class kmaO_RateDependent;
class kmaO_TriaxialTestData;
class kmaC_DruckerPrager;

class kmaO_DruckerPrager: public kmaO_MaterialOption
{
  public:
    kmaO_DruckerPrager( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_DruckerPrager();
    kmaO_DruckerPrager( const odb_SequenceSequenceDouble& table,
                        odb_String shearCriterion = "LINEAR",
                        double eccentricity = 0.1,
                        bool testData = false,
                        bool temperatureDependency = false,
                        int dependencies = 0 );
    kmaO_DruckerPrager( const kmaO_DruckerPrager& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_DruckerPrager& operator=( const kmaO_DruckerPrager& rhs );

    virtual ~kmaO_DruckerPrager();
    odb_String shearCriterion() const;
    double eccentricity() const;
    bool testData() const;
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    kmaO_DruckerPragerCreep druckerPragerCreep() const;
    kmaO_DruckerPragerHardening druckerPragerHardening() const;
    kmaO_RateDependent rateDependent() const;
    kmaO_TriaxialTestData triaxialTestData() const;
    kmaO_DruckerPragerCreep DruckerPragerCreep( const odb_SequenceSequenceDouble& table,
                                                odb_String law = "STRAIN",
                                                bool temperatureDependency = false,
                                                int dependencies = 0 );
    kmaO_DruckerPragerHardening DruckerPragerHardening( const odb_SequenceSequenceDouble& table,
                                                        odb_String type = "COMPRESSION",
                                                        bool rate = false,
                                                        bool temperatureDependency = false,
                                                        int dependencies = 0 );
    kmaO_RateDependent RateDependent( const odb_SequenceSequenceDouble& table,
                                      odb_String type = "POWER_LAW",
                                      bool temperatureDependency = false,
                                      int dependencies = 0 );
    kmaO_TriaxialTestData TriaxialTestData( const odb_SequenceSequenceDouble& table,
                                            odb_Union a = "NONE",
                                            odb_Union b = "NONE",
                                            odb_Union pt = "NONE" );
    const kmaC_DruckerPrager* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_DruckerPrager_h
