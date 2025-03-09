//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_MaterialOption_h
#define kmaO_MaterialOption_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <kmaC_MaterialOptionShortcut.h>
#include <odb_String.h>
#include <cow_CountedHld.h>
#include <cow_ArrayCOW.h>
// Forward Declarations
class kmaC_MaterialOption;
typedef cow_CountedHld<kmaC_MaterialOption> kmaC_MaterialOptionHld;

class kmaO_MaterialOption  : public mem_AllocationOperators
{
public:
    kmaO_MaterialOption( const kmaC_MaterialOptionShortcut& sc );
    kmaO_MaterialOption();
    virtual ~kmaO_MaterialOption();

    void SetPointer( kmaC_MaterialOption* ptr) { pointer = ptr; }
    const kmaC_MaterialOption* GetPointer() const;
    kmaC_MaterialOptionShortcut Shortcut() const { return shortcut; }
    bool hasValue() const { return shortcut.HasValue(); }
    static unsigned int typeIdentifier();
    unsigned int subTypeIdentifier() const;

    odb_String name() const { if (hasValue()) return shortcut.Name();
      else return odb_String(); }

    virtual kmaO_MaterialOption* Copy() const;
    
    
protected:
    kmaC_MaterialOptionShortcut shortcut;
    kmaC_MaterialOptionHld pointer;
};

COW_ARRAYCOW2_DECL(kmaO_MaterialOption, cow_Virtual);

#endif
