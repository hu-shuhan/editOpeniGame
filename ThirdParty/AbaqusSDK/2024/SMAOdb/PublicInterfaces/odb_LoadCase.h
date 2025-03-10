//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_LoadCase_h
#define odb_LoadCase_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_Types.h>
// End local includes

// Forward declarations

class odb_LoadCase : public mem_AllocationOperators
{
public:
    // member access
    odb_String name() const;
 
    // methods

private:
    odb_String m_name;

public:   // undocumented and unsupported
    odb_LoadCase();
    odb_LoadCase(const odb_String& name);

    bool operator==(const odb_LoadCase& rhs) const;
    bool operator!=(const odb_LoadCase& rhs) const;

    void summaryValue1( odb_String & ) const {};
    void summaryValue2( odb_String & ) const {};
};

#endif // odb_LoadCase_h
