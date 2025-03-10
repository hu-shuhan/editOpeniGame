//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_Section_h
#define odb_Section_h
///////////////////////////////////////////////////////////////////////////////
// odb_Section
//

// Begin local includes
#include <cow_ArrayCOW.h>
#include <odb_Types.h>
#include <mem_AllocationOperators.h>
#include <kseC_SectionShortcut.h>
// Forward declarations
class kseC_Section;

class odb_Section: public mem_AllocationOperators
{
  public:
    odb_Section( const kseC_SectionShortcut& shortcut );
    odb_Section();
    virtual odb_Section* Copy() const;
    virtual ~odb_Section();
    kseC_SectionShortcut Shortcut() const { return shortcut; } 

    bool hasValue() const { return shortcut.HasValue(); }

    odb_String name() const { if (hasValue()) return shortcut.Name();
      else return odb_String(); }

    static unsigned int typeIdentifier();
    unsigned int subTypeIdentifier() const;

protected:
    kseC_SectionShortcut shortcut;
};

COW_ARRAYCOW2_DECL(odb_Section, cow_Virtual);

#endif // odb_Section_h
