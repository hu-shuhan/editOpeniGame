//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_FieldLocation_h
#define odb_FieldLocation_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_Enum.h>
#include <odb_SequenceSectionPoint.h>
// End local includes

//
// Forward declarations
//
class ddb_FieldDescription;
class res_FieldDescription;

class odb_FieldLocation : public mem_AllocationOperators
{
public:
    // member access
    odb_Enum::odb_ResultPositionEnum position() const {return m_position;}
    const odb_SequenceSectionPoint& sectionPoint() const
				{return m_sectionPointCon;}
    const odb_SectionPoint& sectionPoint(int index) const
				 {return m_sectionPointCon.constGet(index);}

    //
    bool operator== (const odb_FieldLocation& rhs) const;
    bool operator!= (const odb_FieldLocation& rhs) const;

private:
    odb_Enum::odb_ResultPositionEnum    m_position;
    odb_SequenceSectionPoint  m_sectionPointCon;     

public:   // undocumented and unsupported
    odb_FieldLocation();
    odb_FieldLocation (const odb_Enum::odb_ResultPositionEnum position);
    odb_FieldLocation (const odb_Enum::odb_ResultPositionEnum position,
                       const odb_SectionPoint& sectionPoint);
    odb_FieldLocation (const odb_Enum::odb_ResultPositionEnum position,
                       const odb_SequenceSectionPoint& sectionPoint);
    odb_FieldLocation (const ddb_FieldDescription& fieldD);
    odb_FieldLocation (const res_FieldDescription& fieldD);

    void Append (const odb_SectionPoint& sp);
    void Append (const odb_SequenceSectionPoint& spCon);

};


#endif // odb_FieldLocation_h
