//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_Part_h
#define odb_Part_h

// Begin local includes
#include <odb_Enum.h>
#include <odb_DiscreteEntity.h>
#include <odb_SectionCategory.h>
#include <odb_ElemCtrlParams.h>
// End local includes

// Forward Declarations

class ddb_Part;
COW_COW_DECL(ddb_Part);
class odb_DatumCsys;
class odb_Set;
class odb_Section;
class odb_SectionAssignment;
class odb_SequenceSectionAssignment;
class odb_ConnectorOrientation;
class odb_SequenceConnectorOrientation;
class odb_MaterialOrientation;
class odb_SequenceMaterialOrientation;
class odb_RebarOrientation;
class odb_SequenceRebarOrientation;
class odb_BeamOrientation;
class odb_SequenceBeamOrientation;

struct odiK_SubsetStruct;
COW_LIST_DECL(odiK_SubsetStruct, odiK_SubsetStructList);
typedef odiK_SubsetStructList ddb_SubsetStructList;
class odb_AnalyticSurface;
class odb_RigidBody;
class odb_SequenceRigidBody;
class odb_SequenceAnalyticSurfaceSegment;

class odb_Part : public odb_DiscreteEntity
{
public:

    virtual void addNodes(
	const odb_SequenceInt& labels,
	const odb_SequenceSequenceFloat& coordinates,
	const odb_String& setName = odb_String());
    virtual void addElements(
	const odb_SequenceInt& labels,
	const odb_SequenceSequenceInt& connectivity,
	const odb_String& elementType,
	const odb_String& setName = odb_String(),
	const odb_SectionCategory& sectionCategory = odb_SectionCategory(), 
	const odb_ElemCtrlParams& elemCtrls = odb_ElemCtrlParams());

    odb_Set& NodeSet(const odb_String& name,
                     const odb_SequenceNode& nodes);
    odb_Set& NodeSet(const odb_String& name,
                     const odb_SequenceInt& nodeLabels,
                     bool internal = false, bool unsorted = false);

    odb_Set& ElementSet(const odb_String& name,
                        const odb_SequenceElement& els);
    odb_Set& ElementSet(const odb_String& name,
                        const odb_SequenceInt& elementLabels,
                        bool internal = false);

    odb_Set& Surface(const odb_String& name,
                     const odb_SequenceElement& els,
		     const odb_SequenceElementFace& faces);
    odb_Set& Surface(const odb_String& name,
                     const odb_SequenceInt& elementLabels,
		     const odb_SequenceElementFace& faces,
                     bool internal = false);    

    void assignSection(const odb_Set& elset, const odb_Section& section, float offset=0);
    odb_SectionAssignment sectionAssignment(int index) const;
    odb_SequenceSectionAssignment sectionAssignments() const;

    odb_DatumCsys  DatumCsysByThreePoints(const odb_String& name,
					  const odb_Enum::odb_DatumCsysTypeEnum,
					  const odb_SequenceFloat& origin,
					  const odb_SequenceFloat& point1,
					  const odb_SequenceFloat& point2);
    bool hasDatumCsys(const cow_String& name) const;
    odb_DatumCsys datumCsys(const cow_String& name) const;

    void assignMaterialOrientation(const odb_Set& elset,
                                   const odb_DatumCsys& localCsys,
                                   odb_Enum::odb_AxisEnum axis = odb_Enum::AXIS_1,
                                   float angle = 0,
                                   odb_Enum::odb_StackDirectionEnum stack = odb_Enum::STACK_3);
    odb_MaterialOrientation materialOrientation(int index) const;
    odb_SequenceMaterialOrientation materialOrientations() const;

    void assignRebarOrientation(const odb_Set& elset,
                                const odb_DatumCsys& localCsys,
                                odb_Enum::odb_AxisEnum axis = odb_Enum::AXIS_1,
                                float angle = 0);
    odb_RebarOrientation rebarOrientation(int index) const;
    odb_SequenceRebarOrientation rebarOrientations() const;

    void assignBeamOrientation(const odb_Set& elset,
                               odb_Enum::odb_OrientationMethodEnum method,
                               const odb_SequenceFloat& vector);
    odb_BeamOrientation beamOrientation(int index) const;
    odb_SequenceBeamOrientation beamOrientations() const;

    void ConnectorOrientation(const odb_Set& elset,
                              const odb_DatumCsys& csys,
                              odb_Enum::odb_AxisEnum axis,
                              double angle,
                              bool orient2sameAs1 = false);
    void ConnectorOrientation(const odb_Set& elset,
                              const odb_DatumCsys& csys1,
                              odb_Enum::odb_AxisEnum axis1,
                              double angle1,
                              const odb_DatumCsys& csys2,
                              odb_Enum::odb_AxisEnum axis2,
                              double angle2 );
    odb_ConnectorOrientation connectorOrientation(int index) const;
    odb_SequenceConnectorOrientation connectorOrientations() const;

    // solverInput snippet
    void setSolverInput(const odb_String& solverInput);
    odb_String solverInput() const;
    // rigid bodies and analytic_surface
    // analytic rigid surfaces
    odb_AnalyticSurface AnalyticRigidSurface( odb_String surfaceName, 
                                              odb_String type,
                                              double filletRadius,
                                              const odb_SequenceAnalyticSurfaceSegment& segments );
    odb_AnalyticSurface AnalyticRigidSurf2DPlanar( odb_String surfaceName, 
                                                   double filletRadius,
                                                   const odb_SequenceAnalyticSurfaceSegment& segments );
    odb_AnalyticSurface AnalyticRigidSurfExtrude( odb_String surfaceName, 
                                                  double filletRadius,
                                                  const odb_SequenceAnalyticSurfaceSegment& segments );
    odb_AnalyticSurface AnalyticRigidSurfRevolve( odb_String surfaceName, 
                                                  double filletRadius,
                                                  const odb_SequenceAnalyticSurfaceSegment& segments );
    bool hasAnalyticSurface() const;
    odb_AnalyticSurface analyticSurface() const;
    odb_RigidBody RigidBody( const odb_Set& refNodeSet, 
                             bool isothermal = false,
                             odb_String position = "INPUT" );
    odb_SequenceRigidBody rigidBodies() const;

private:
    ddb_PartCOW m_part;

public:   // undocumented and unsupported
    odb_Part();
    odb_Part(const odb_String& name,
                     odb_Enum::odb_DimensionEnum space,
                     const ddb_Part& p,
                     const odb_Mesh& m);
    ddb_Part part() const;
    const odb_Mesh& mesh() const;
    void summaryValue1( odb_String & ) const {};
    void summaryValue2( odb_String & ) const {};
};

typedef odb_Part odb_DiscretePart; // for backward compatibility


#endif // odb_Part_h
