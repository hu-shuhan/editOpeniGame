//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_SectionAssignment_h
#define odb_SectionAssignment_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_Types.h>
#include <ddr_Shortcut.h>

// Forward declarations
class odb_Section;
class odb_Set;
class odiK_Model;
class ptoC_PartAttribute;
typedef ddr_Shortcut<odiK_Model> odiK_ModelShortcut;
typedef ddr_Shortcut<ptoC_PartAttribute> ptoC_PartAttributeShortcut;

// Class definition

class odb_SectionAssignment : public mem_AllocationOperators
{
public:
    odb_Set     region() const;
    odb_String  sectionName() const;
    double  offset() const;
private:
    ptoC_PartAttributeShortcut m_partAttributeShortcut;
    odiK_ModelShortcut m_modelShortcut;

public: // Undocumented - internal stuff
    odb_Section section() const;
    odb_SectionAssignment(); // required for sequence
    odb_SectionAssignment(const ptoC_PartAttributeShortcut& pas,
			  const odiK_ModelShortcut& ms);
};

#endif // odb_SectionAssignment_h
