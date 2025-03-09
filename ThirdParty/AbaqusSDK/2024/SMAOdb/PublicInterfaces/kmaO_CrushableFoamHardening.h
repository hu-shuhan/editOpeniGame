//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_CrushableFoamHardening_h
#define kmaO_CrushableFoamHardening_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_CrushableFoamHardening
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_FoamHardening;

class kmaO_CrushableFoamHardening: public kmaO_MaterialOption
{
  public:
    kmaO_CrushableFoamHardening( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_CrushableFoamHardening();
    kmaO_CrushableFoamHardening( const odb_SequenceSequenceDouble& table,
                                 bool temperatureDependency = false,
                                 int dependencies = 0 );
    kmaO_CrushableFoamHardening( const kmaO_CrushableFoamHardening& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_CrushableFoamHardening& operator=( const kmaO_CrushableFoamHardening& rhs );

    virtual ~kmaO_CrushableFoamHardening();
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_FoamHardening* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_CrushableFoamHardening_h
