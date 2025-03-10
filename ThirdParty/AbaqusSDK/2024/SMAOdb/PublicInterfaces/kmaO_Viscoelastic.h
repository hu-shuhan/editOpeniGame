//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_Viscoelastic_h
#define kmaO_Viscoelastic_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_Viscoelastic
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
#include <odb_Union.h>
// Forward declarations
class kmaO_CombinedTestData;
class kmaO_ShearTestData;
class kmaO_Trs;
class kmaO_VolumetricTestData;
class kmaC_Viscoelastic;

class kmaO_Viscoelastic: public kmaO_MaterialOption
{
  public:
    kmaO_Viscoelastic( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_Viscoelastic();
    kmaO_Viscoelastic( odb_String domain,
                       const odb_SequenceSequenceDouble& table,
                       odb_String frequency = "FORMULA",
                       odb_String type = "ISOTROPIC",
                       odb_String preload = "NONE",
                       odb_String time = "PRONY",
                       double errtol = 0.01,
                       int nmax = 13,
                       const odb_SequenceSequenceDouble& volumetricTable = odb_SequenceSequenceDouble() );
    kmaO_Viscoelastic( const kmaO_Viscoelastic& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_Viscoelastic& operator=( const kmaO_Viscoelastic& rhs );

    virtual ~kmaO_Viscoelastic();
    odb_String domain() const;
    odb_String frequency() const;
    odb_String type() const;
    odb_String preload() const;
    odb_String time() const;
    double errtol() const;
    int nmax() const;
    odb_SequenceSequenceDouble table() const;
    odb_SequenceSequenceDouble volumetricTable() const;
    kmaO_CombinedTestData combinedTestData() const;
    kmaO_ShearTestData shearTestData() const;
    kmaO_Trs trs() const;
    kmaO_VolumetricTestData volumetricTestData() const;
    kmaO_CombinedTestData CombinedTestData( const odb_SequenceSequenceDouble& table,
                                            odb_Union volinf = "NONE",
                                            odb_Union shrinf = "NONE" );
    kmaO_ShearTestData ShearTestData( const odb_SequenceSequenceDouble& table,
                                      odb_Union shrinf = "NONE" );
    kmaO_Trs Trs( odb_String definition = "WLF",
                  const odb_SequenceSequenceDouble& table = odb_SequenceSequenceDouble() );
    kmaO_VolumetricTestData VolumetricTestData( const odb_SequenceSequenceDouble& table,
                                                odb_Union volinf = "NONE",
                                                odb_Union smoothing = "NONE",
                                                bool temperatureDependency = false,
                                                int dependencies = 0 );
    const kmaC_Viscoelastic* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_Viscoelastic_h
