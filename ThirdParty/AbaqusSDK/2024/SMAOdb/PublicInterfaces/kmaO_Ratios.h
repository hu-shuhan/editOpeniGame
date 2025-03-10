//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_Ratios_h
#define kmaO_Ratios_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_Ratios
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_Ratios;

class kmaO_Ratios: public kmaO_MaterialOption
{
  public:
    kmaO_Ratios( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_Ratios();
    kmaO_Ratios( const odb_SequenceSequenceDouble& table,
                 bool temperatureDependency = false,
                 int dependencies = 0 );
    kmaO_Ratios( const kmaO_Ratios& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_Ratios& operator=( const kmaO_Ratios& rhs );

    virtual ~kmaO_Ratios();
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_Ratios* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_Ratios_h
