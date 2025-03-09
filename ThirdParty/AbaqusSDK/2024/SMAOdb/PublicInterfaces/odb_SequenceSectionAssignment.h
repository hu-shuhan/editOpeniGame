//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_SequenceSectionAssignment_h
#define odb_SequenceSectionAssignment_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <ddr_Shortcut.h>
// End local includes

// Forward declarations
class odiK_Model;
class odb_SectionAssignment;
class ptoC_PartAttributeList;
typedef ddr_Shortcut<ptoC_PartAttributeList> ptoC_PartAttributeListShortcut;
typedef ddr_Shortcut<odiK_Model> odiK_ModelShortcut;

// Class definition

class odb_SequenceSectionAssignment : public mem_AllocationOperators
{
  public:
    int size() const;
    odb_SectionAssignment constGet(int index) const;
    odb_SectionAssignment operator[](int index) const;

  private:

    ptoC_PartAttributeListShortcut m_partAttributeListShortcut;
    odiK_ModelShortcut m_modelShortcut;

  public: // Undocumented and unsupported

    odb_SequenceSectionAssignment(const ptoC_PartAttributeListShortcut& sc,
				  const odiK_ModelShortcut& model);
};

#endif // odb_SequenceSectionAssignment_h
