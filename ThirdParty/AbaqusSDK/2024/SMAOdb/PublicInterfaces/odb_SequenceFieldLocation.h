//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_SequenceFieldLocation_h
#define odb_SequenceFieldLocation_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_FieldLocation.h>
#include <cow_List.h>
// End local includes

COW_LIST_DECL(odb_FieldLocation, odb_FieldLocationList);

class odb_SequenceFieldLocation : public mem_AllocationOperators
{
public:
    int size() const;
    const odb_FieldLocation& constGet(uint index) const;
    odb_FieldLocation& get(uint index);

    const odb_FieldLocation& operator[](uint index) const;

private:
    odb_FieldLocationList m_locList;

public:    // undocumented and unsupported
    odb_SequenceFieldLocation();

    void append(const odb_FieldLocation& val);
    void append(const odb_SequenceFieldLocation& vals);
    void append(odb_Enum::odb_ResultPositionEnum position);
    void append(odb_Enum::odb_ResultPositionEnum position,
                const odb_SectionPoint& sectionPoint);
    void append(odb_Enum::odb_ResultPositionEnum position,
                const odb_SequenceSectionPoint& sectionPoint);

    bool isMember(const odb_FieldLocation& loc) const;
};

#endif // #ifndef odb_SequenceFieldLocation_h
