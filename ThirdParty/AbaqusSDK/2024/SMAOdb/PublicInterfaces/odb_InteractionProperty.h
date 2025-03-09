//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_InteractionProperty_h
#define odb_InteractionProperty_h

// Begin local includes
#include <mem_AllocationOperatorsTiny.h>
#include <odb_Types.h>
#include <cow_COW.h>
#include <kinC_IPShortcut.h>

// End local includes

// Forward declarations
class ddb_InteractionProperty;
COW_COW_DECL(ddb_InteractionProperty);


class odb_InteractionProperty : public mem_AllocationOperatorsTiny
{
public:
    // member access

    // Options

protected:
    ddb_InteractionPropertyCOW m_interactionProperty;

public:   // undocumented and unsupported

    // Needed for container
    odb_InteractionProperty();
    odb_InteractionProperty(const ddb_InteractionProperty& ip);

    virtual ~odb_InteractionProperty();
    kinC_InteractionPropertyShortcut Shortcut() const;
    const ddb_InteractionProperty& ddbValue() const { return m_interactionProperty.ConstGet(); }

    bool hasValue() const;
    static unsigned int typeIdentifier();
    unsigned int subTypeIdentifier() const;

};

COW_COW_DECL(odb_InteractionProperty);
 
#endif // odb_InteractionProperty_h
