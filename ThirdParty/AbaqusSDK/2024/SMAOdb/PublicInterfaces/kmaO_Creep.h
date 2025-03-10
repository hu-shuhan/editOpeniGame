//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_Creep_h
#define kmaO_Creep_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_Creep
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
#include <odb_Union.h>
// Forward declarations
class kmaO_Ornl;
class kmaO_Potential;
class kmaC_Creep;

class kmaO_Creep: public kmaO_MaterialOption
{
  public:
    kmaO_Creep( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_Creep();
    kmaO_Creep( const odb_SequenceSequenceDouble& table,
                odb_String law = "STRAIN",
                bool temperatureDependency = false,
                int dependencies = 0 );
    kmaO_Creep( const kmaO_Creep& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_Creep& operator=( const kmaO_Creep& rhs );

    virtual ~kmaO_Creep();
    odb_String law() const;
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    kmaO_Ornl ornl() const;
    kmaO_Potential potential() const;
    kmaO_Ornl Ornl( double a = 0.3,
                    odb_Union h = "NONE",
                    bool reset = false );
    kmaO_Potential Potential( const odb_SequenceSequenceDouble& table,
                              bool temperatureDependency = false,
                              int dependencies = 0 );
    const kmaC_Creep* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_Creep_h
