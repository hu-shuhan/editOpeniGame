//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kbeO_ConnectorBehaviorOption_h
#define kbeO_ConnectorBehaviorOption_h
///////////////////////////////////////////////////////////////////////////////
// kbeO_ConnectorBehaviorOption
//

// Begin local includes
#include <cow_ArrayCOW.h>
#include <odb_Types.h>
#include <mem_AllocationOperators.h>
#include <kbeC_BehaviorOptionShortcut.h>
// Forward declarations
class kbeC_ConnectorBehaviorOption;
typedef cow_CountedHld<kbeC_ConnectorBehaviorOption> kbeC_ConnectorBehaviorOptionHld;

class kbeO_ConnectorBehaviorOption: public mem_AllocationOperators
{
  public:
    kbeO_ConnectorBehaviorOption( const kbeC_ConnectorBehaviorOptionShortcut& shortcut );
    kbeO_ConnectorBehaviorOption();
    virtual kbeO_ConnectorBehaviorOption* Copy() const;
    virtual ~kbeO_ConnectorBehaviorOption();
    void SetPointer( kbeC_ConnectorBehaviorOption* ptr) { pointer = ptr; }
    const kbeC_ConnectorBehaviorOption* GetPointer() const;
    kbeC_ConnectorBehaviorOptionShortcut Shortcut() const { return shortcut; } 

    bool hasValue() const { return shortcut.HasValue(); }

    static unsigned int typeIdentifier();
    unsigned int subTypeIdentifier() const;

protected:
    kbeC_ConnectorBehaviorOptionShortcut shortcut;
    kbeC_ConnectorBehaviorOptionHld pointer;
};

COW_ARRAYCOW2_DECL(kbeO_ConnectorBehaviorOption, cow_Virtual);

#endif // kbeO_ConnectorBehaviorOption_h
