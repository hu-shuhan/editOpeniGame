//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_MullinsEffect_h
#define kmaO_MullinsEffect_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_MullinsEffect
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
#include <kmaO_SequenceUniaxialTestData.h>
#include <kmaO_SequenceBiaxialTestData.h>
#include <kmaO_SequencePlanarTestData.h>
// Forward declarations
class kmaC_MullinsEffect;

class kmaO_MullinsEffect: public kmaO_MaterialOption
{
  public:
    kmaO_MullinsEffect( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_MullinsEffect( odb_String definition = "CONSTANTS",
                        bool temperatureDependency = false,
                        int dependencies = 0,
                        int properties = 0,
                        const odb_SequenceSequenceDouble& table = odb_SequenceSequenceDouble(),
                        const kmaO_SequenceUniaxialTestData& uniaxialTests = kmaO_SequenceUniaxialTestData(),
                        const kmaO_SequenceBiaxialTestData& biaxialTests = kmaO_SequenceBiaxialTestData(),
                        const kmaO_SequencePlanarTestData& planarTests = kmaO_SequencePlanarTestData() );
    kmaO_MullinsEffect( const kmaO_MullinsEffect& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_MullinsEffect& operator=( const kmaO_MullinsEffect& rhs );

    virtual ~kmaO_MullinsEffect();
    odb_String definition() const;
    bool temperatureDependency() const;
    int dependencies() const;
    int properties() const;
    odb_SequenceSequenceDouble table() const;
    kmaO_SequenceUniaxialTestData uniaxialTests() const;
    kmaO_SequenceBiaxialTestData biaxialTests() const;
    kmaO_SequencePlanarTestData planarTests() const;
    const kmaC_MullinsEffect* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_MullinsEffect_h
