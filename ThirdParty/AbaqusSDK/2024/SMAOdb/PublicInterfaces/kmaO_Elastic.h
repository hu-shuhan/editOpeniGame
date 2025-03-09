//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_Elastic_h
#define kmaO_Elastic_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_Elastic
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
// Forward declarations
class kmaO_FailStress;
class kmaO_FailStrain;
class kmaC_Elastic;

class kmaO_Elastic: public kmaO_MaterialOption
{
  public:
    kmaO_Elastic( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_Elastic();
    kmaO_Elastic( const odb_SequenceSequenceDouble& table,
                  odb_String type = "ISOTROPIC",
                  bool noCompression = false,
                  bool noTension = false,
                  bool temperatureDependency = false,
                  int dependencies = 0,
                  odb_String moduli = "LONG_TERM" );
    kmaO_Elastic( const kmaO_Elastic& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_Elastic& operator=( const kmaO_Elastic& rhs );

    virtual ~kmaO_Elastic();
    odb_String type() const;
    bool noCompression() const;
    bool noTension() const;
    bool temperatureDependency() const;
    int dependencies() const;
    odb_String moduli() const;
    odb_SequenceSequenceDouble table() const;
    kmaO_FailStress failStress() const;
    kmaO_FailStrain failStrain() const;
    kmaO_FailStress FailStress( const odb_SequenceSequenceDouble& table,
                                bool temperatureDependency = false,
                                int dependencies = 0 );
    kmaO_FailStrain FailStrain( const odb_SequenceSequenceDouble& table,
                                bool temperatureDependency = false,
                                int dependencies = 0 );
    const kmaC_Elastic* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_Elastic_h
