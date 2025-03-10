//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_DeformationPlasticity_h
#define kmaO_DeformationPlasticity_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_DeformationPlasticity
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_DeformationPlasticity;

class kmaO_DeformationPlasticity: public kmaO_MaterialOption
{
  public:
    kmaO_DeformationPlasticity( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_DeformationPlasticity();
    kmaO_DeformationPlasticity( const odb_SequenceSequenceDouble& table,
                                bool temperatureDependency = false );
    kmaO_DeformationPlasticity( const kmaO_DeformationPlasticity& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_DeformationPlasticity& operator=( const kmaO_DeformationPlasticity& rhs );

    virtual ~kmaO_DeformationPlasticity();
    bool temperatureDependency() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_DeformationPlasticity* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_DeformationPlasticity_h
