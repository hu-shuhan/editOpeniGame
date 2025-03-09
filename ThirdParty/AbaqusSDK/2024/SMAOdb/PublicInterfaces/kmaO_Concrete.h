//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_Concrete_h
#define kmaO_Concrete_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_Concrete
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
// Forward declarations
class kmaO_FailureRatios;
class kmaO_ShearRetention;
class kmaO_TensionStiffening;
class kmaC_Concrete;

class kmaO_Concrete: public kmaO_MaterialOption
{
  public:
    kmaO_Concrete( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_Concrete();
    kmaO_Concrete( const odb_SequenceSequenceDouble& table,
                   bool temperatureDependency = false,
                   int dependencies = 0 );
    kmaO_Concrete( const kmaO_Concrete& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_Concrete& operator=( const kmaO_Concrete& rhs );

    virtual ~kmaO_Concrete();
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    kmaO_FailureRatios failureRatios() const;
    kmaO_ShearRetention shearRetention() const;
    kmaO_TensionStiffening tensionStiffening() const;
    kmaO_FailureRatios FailureRatios( const odb_SequenceSequenceDouble& table,
                                      bool temperatureDependency = false,
                                      int dependencies = 0 );
    kmaO_ShearRetention ShearRetention( const odb_SequenceSequenceDouble& table,
                                        bool temperatureDependency = false,
                                        int dependencies = 0 );
    kmaO_TensionStiffening TensionStiffening( const odb_SequenceSequenceDouble& table,
                                              odb_String type = "STRAIN",
                                              bool temperatureDependency = false,
                                              int dependencies = 0 );
    const kmaC_Concrete* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_Concrete_h
