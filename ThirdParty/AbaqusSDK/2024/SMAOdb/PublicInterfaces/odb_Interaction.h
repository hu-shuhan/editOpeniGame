//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_Interaction_h
#define odb_Interaction_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_Types.h>
#include <cow_COW.h>
#include <kinC_InteractionShortcut.h>

// Forward declarations
class ddb_Interaction;
COW_COW2_DECL(ddb_Interaction, cow_Virtual); 


class odb_Interaction : public mem_AllocationOperators
{
public:
    void deactivate(odb_String stepName);
    odb_String name() const;

protected:
    ddb_InteractionCOW m_interaction;

public:   // undocumented and unsupported

    // Needed for container
    odb_Interaction();

    odb_Interaction(const ddb_Interaction& interaction);

    virtual ~odb_Interaction();
    kinC_InteractionShortcut Shortcut() const;

    bool hasValue() const;

    static unsigned int typeIdentifier();
    unsigned int subTypeIdentifier() const;

};

COW_COW_DECL(odb_Interaction); 

#endif // odb_Interaction_h
