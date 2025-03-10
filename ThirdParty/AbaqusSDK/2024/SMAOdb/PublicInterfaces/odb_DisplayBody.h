//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_DisplayBody_h
#define odb_DisplayBody_h

// Begin local includes
#include <odb_Constraint.h>
#include <odb_String.h>
// Forward declarations
class ddb_DisplayBody;

class odb_DisplayBody: public odb_Constraint
{
public:

    odb_String instanceName() const;

    int referenceNode1Label() const;
    odb_String referenceNode1InstanceName() const;

    int referenceNode2Label() const;
    odb_String referenceNode2InstanceName() const;

    int referenceNode3Label() const;
    odb_String referenceNode3InstanceName() const;

private:

public:   // Undocumented and unsupported
    static unsigned int typeIdentifier();
    // Needed for container
    odb_DisplayBody();

    odb_DisplayBody(const ddb_DisplayBody& constraint);
};

#endif // odb_DisplayBody_h
