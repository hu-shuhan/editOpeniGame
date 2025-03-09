//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_EosCompaction_h
#define kmaO_EosCompaction_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_EosCompaction
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_EosCompaction;

class kmaO_EosCompaction: public kmaO_MaterialOption
{
  public:
    kmaO_EosCompaction( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_EosCompaction();
    kmaO_EosCompaction( double soundSpeed,
                        double porosity,
                        double pressure,
                        double compactionPressure );
    kmaO_EosCompaction( const kmaO_EosCompaction& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_EosCompaction& operator=( const kmaO_EosCompaction& rhs );

    virtual ~kmaO_EosCompaction();
    double soundSpeed() const;
    double porosity() const;
    double pressure() const;
    double compactionPressure() const;
    const kmaC_EosCompaction* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_EosCompaction_h
