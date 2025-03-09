//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_Potential_h
#define kmaO_Potential_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_Potential
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_Potential;

class kmaO_Potential: public kmaO_MaterialOption
{
  public:
    kmaO_Potential( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_Potential();
    kmaO_Potential( const odb_SequenceSequenceDouble& table,
                    bool temperatureDependency = false,
                    int dependencies = 0 );
    kmaO_Potential( const kmaO_Potential& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_Potential& operator=( const kmaO_Potential& rhs );

    virtual ~kmaO_Potential();
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_Potential* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_Potential_h
