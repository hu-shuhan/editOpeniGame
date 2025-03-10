//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_VoidNucleation_h
#define kmaO_VoidNucleation_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_VoidNucleation
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_VoidNucleation;

class kmaO_VoidNucleation: public kmaO_MaterialOption
{
  public:
    kmaO_VoidNucleation( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_VoidNucleation();
    kmaO_VoidNucleation( const odb_SequenceSequenceDouble& table,
                         bool temperatureDependency = false,
                         int dependencies = 0 );
    kmaO_VoidNucleation( const kmaO_VoidNucleation& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_VoidNucleation& operator=( const kmaO_VoidNucleation& rhs );

    virtual ~kmaO_VoidNucleation();
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_VoidNucleation* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_VoidNucleation_h
