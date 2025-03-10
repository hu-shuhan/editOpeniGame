//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kflO_Filter_h
#define kflO_Filter_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <kflC_FilterShortcut.h>
#include <odb_Enum.h>
#include <cow_ArrayCOW.h>
#include <odb_Types.h>
#include <odb_String.h>
#include <odb_Union.h>
// Forward declarations
class kflC_Filter;

typedef cow_CountedHld<kflC_Filter> kflC_FilterHld;
class kflO_Filter : public mem_AllocationOperators
{
public:
    kflO_Filter( const kflC_FilterShortcut& shortcut );
    kflO_Filter();

    virtual ~kflO_Filter();
    kflC_FilterShortcut Shortcut() const { return shortcut; }

    bool hasValue() const { return shortcut.HasValue(); }

    static unsigned int typeIdentifier();

    unsigned int subTypeIdentifier() const;

    odb_String name() const { if (hasValue()) return shortcut.Name();
      else return odb_String(); }

    virtual kflO_Filter* Copy() const;

protected:
    kflC_FilterShortcut shortcut;
    kflC_FilterHld pointer;
};

COW_ARRAYCOW2_DECL(kflO_Filter, cow_Virtual);

#endif
