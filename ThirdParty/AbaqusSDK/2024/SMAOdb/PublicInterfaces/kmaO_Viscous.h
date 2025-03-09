//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_Viscous_h
#define kmaO_Viscous_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_Viscous
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
// Forward declarations
class kmaO_Potential;
class kmaC_Viscous;

class kmaO_Viscous: public kmaO_MaterialOption
{
  public:
    kmaO_Viscous( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_Viscous();
    kmaO_Viscous( const odb_SequenceSequenceDouble& table,
                  odb_String law = "STRAIN",
                  bool temperatureDependency = false,
                  int dependencies = 0 );
    kmaO_Viscous( const kmaO_Viscous& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_Viscous& operator=( const kmaO_Viscous& rhs );

    virtual ~kmaO_Viscous();
    odb_String law() const;
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    kmaO_Potential potential() const;
    kmaO_Potential Potential( const odb_SequenceSequenceDouble& table,
                              bool temperatureDependency = false,
                              int dependencies = 0 );
    const kmaC_Viscous* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_Viscous_h
