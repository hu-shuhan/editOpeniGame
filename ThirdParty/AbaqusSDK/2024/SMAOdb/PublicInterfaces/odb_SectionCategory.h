//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_SectionCategory_h
#define odb_SectionCategory_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_SectionPoint.h>
#include <odb_SequenceSectionPoint.h>
// End local includes


// Forward declarations
class ddb_ElementClass;
class bme_ElementClass;

class odb_SectionCategory : public mem_AllocationOperators
{
public:
    // members
    odb_String name() const {return m_name;}
    odb_String description() const {return m_description;}

    // sectionPoints
    odb_SectionPoint& SectionPoint(int number,
                                   const odb_String& desc);
    const odb_SectionPoint& sectionPoints(int index) const;
    odb_SequenceSectionPoint& sectionPoints();
    const odb_SequenceSectionPoint& sectionPoints() const;

private:
    odb_String m_name;
    odb_String m_description;
    odb_SequenceSectionPoint m_spCon;

public:    // undocumented and unsupported
    odb_SectionCategory();
    odb_SectionCategory(const ddb_ElementClass& elClass,
                        const odb_String& desc = "");
    odb_SectionCategory(const bme_ElementClass& elClass,
                        const odb_String& desc = "");
    odb_SectionCategory(const odb_String& name,
                        const odb_String& desc);
    
    void setDescription(const odb_String&);

    void summaryValue1( odb_String & ) const {};
    void summaryValue2( odb_String & ) const {};
};

#endif // odb_SectionCategory_h
