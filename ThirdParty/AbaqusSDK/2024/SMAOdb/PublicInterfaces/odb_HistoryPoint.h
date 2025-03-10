//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_HistoryPoint_h
#define odb_HistoryPoint_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_Set.h>
#include <odb_Instance.h>
#include <odb_SectionPoint.h>
#include <odb_Assembly.h>
// End local includes

//
// Forward declarations
//
class odb_SubField;
class ddb_History;

class odb_HistoryPoint : public mem_AllocationOperators
{
public:
    odb_HistoryPoint( const odb_Element& el,
		      int ipNumber=0);
    odb_HistoryPoint( const odb_Element& el,
		      int ipNumber,
                      const odb_SectionPoint& sectionPoint );
    odb_HistoryPoint( const odb_Element& el,
		      odb_Enum::odb_ElementFaceEnum face,
		      int ipNumber=0 );
    odb_HistoryPoint( const odb_Set& region );
    odb_HistoryPoint( const odb_Element& el,
		      const odb_Node& node ); 
    odb_HistoryPoint( const odb_Instance& instance ); 
    odb_HistoryPoint( const odb_Assembly& assy ); 

    // members
    odb_Enum::odb_ResultPositionEnum position() const;
    odb_Node node() const {return m_node;}
    odb_Element element() const {return m_element;}
    odb_Enum::odb_ElementFaceEnum face() const {return m_face;}
    int ipNumber() const {return m_ipNumber;}
    odb_SectionPoint sectionPoint() const {return m_sectionPoint;}
    const odb_Set& region() const {return m_region;}
    const odb_Instance& instance() const {return m_instance;}
    const odb_Assembly& assembly() const {return m_assembly;}

    //
    bool operator==(const odb_HistoryPoint& rhs) const;
    bool operator!=(const odb_HistoryPoint& rhs) const;

private:
    odb_Node               m_node;
    odb_Element            m_element;
    int                    m_ipNumber;  
    odb_Enum::odb_ElementFaceEnum    m_face;  
    odb_Enum::odb_ResultPositionEnum m_position;  
    odb_SectionPoint       m_sectionPoint;
    odb_Set                m_region;
    odb_Instance           m_instance;
    odb_Assembly           m_assembly;
    odb_String             m_descript;

public:     // undocumented and unsupported
    odb_HistoryPoint();
    odb_HistoryPoint(const ddb_History& hist,
                     const ddb_Assembly& assy);
    odb_HistoryPoint(const odb_Node& node);
    odb_HistoryPoint(const odb_String& ); 

    odb_String CreateDescription() const;
    odb_String CreateRegionName() const;

    void setInstance( const odb_Instance& instance );
    void setAssembly( const odb_Assembly& assembly );

private:
	void setRegion(odb_Set& region) { m_region = region; }
};

#endif // odb_HistoryPoint_h
