//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_SectionPoint_h
#define odb_SectionPoint_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_Types.h>
// End local includes

class odb_SectionPoint : public mem_AllocationOperators
{
public:
    int number() const {return m_number;}
    const odb_String description() const {return m_description;}

private:
    int        m_number;
    odb_String m_description;

public:    // undocumented and unsupported
    odb_SectionPoint();
    odb_SectionPoint(const int number, const odb_String& desc);

    bool operator==(const odb_SectionPoint& rhs) const;
    bool operator!=(const odb_SectionPoint& rhs) const;
};


#endif // odb_SectionPoint_h
