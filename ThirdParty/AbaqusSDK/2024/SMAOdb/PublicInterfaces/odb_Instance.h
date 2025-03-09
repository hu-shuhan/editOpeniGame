//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_Instance_h
#define odb_Instance_h

// Begin local includes
#include <odb_Enum.h>
#include <odb_Node.h>
#include <odb_SequenceNode.h>
#include <odb_Element.h>
#include <odb_SequenceElement.h>
#include <odb_Set.h>
#include <odb_SetRepository.h>
#include <odb_DiscreteEntity.h>

#include <cow_COW.h>
#include <cow_List.h>
// End local includes

// Forward Declarations
class ddb_PartInstance;
COW_COW_DECL(ddb_PartInstance);
class odb_DatumCsys;
class odb_Set;
class odb_Section;
class odb_SectionAssignment;
class odb_SequenceSectionAssignment;
class odb_MaterialOrientation;
class odb_SequenceMaterialOrientation;
class odb_RebarOrientation;
class odb_SequenceRebarOrientation;
class odb_ConnectorOrientation;
class odb_SequenceConnectorOrientation;
class odb_BeamOrientation;
class odb_SequenceBeamOrientation;
class odb_AnalyticSurface;
struct odiK_SubsetStruct;
COW_LIST_DECL(odiK_SubsetStruct, odiK_SubsetStructList);
typedef odiK_SubsetStructList ddb_SubsetStructList;
class odb_RigidBody;
class odb_SequenceRigidBody;
class odb_SequenceAnalyticSurfaceSegment;
class odb_SequenceElement;

class odb_Instance : public odb_DiscreteEntity
{
public:
    // set
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

    // Interface for adding nodes and elements to the component
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

    // solverInput snippet
    void setSolverInput(const odb_String& solverInput);
    odb_String solverInput() const;
    // analytic rigid surfaces
    odb_AnalyticSurface AnalyticRigidSurface( odb_String surfaceName,
                                              odb_String type,
                                              double filletRadius,
                                              const odb_SequenceAnalyticSurfaceSegment& segments,
                                              const odb_SequenceSequenceFloat& localCoordData = odb_SequenceSequenceFloat()  );
    odb_AnalyticSurface AnalyticRigidSurf2DPlanar( odb_String surfaceName,
                                                   double filletRadius,
                                                   const odb_SequenceAnalyticSurfaceSegment& segments );
    odb_AnalyticSurface AnalyticRigidSurfExtrude( odb_String surfaceName,
                                                  double filletRadius,
                                                  const odb_SequenceAnalyticSurfaceSegment& segments,
                                                  const odb_SequenceSequenceFloat& localCoordData = odb_SequenceSequenceFloat()  );
    odb_AnalyticSurface AnalyticRigidSurfRevolve( odb_String surfaceName,
                                                  double filletRadius,
                                                  const odb_SequenceAnalyticSurfaceSegment& segments,
                                                  const odb_SequenceSequenceFloat& localCoordData = odb_SequenceSequenceFloat()  );
    bool hasAnalyticSurface() const;
    odb_AnalyticSurface analyticSurface() const;
    odb_RigidBody RigidBody( const odb_Set& refNodeSet,
                             bool isothermal = false,
                             odb_String position = "INPUT" );
    odb_SequenceRigidBody rigidBodies() const;

protected:
    ddb_PartInstanceCOW    m_instance;

public:   // undocumented and unsupported
    odb_Instance();
    odb_Instance(const odb_String& name,
                 odb_Enum::odb_DimensionEnum space,
         const ddb_PartInstance& i,
                 const odb_Mesh& mesh);
    odb_Instance(const odb_String& name,
                 odb_Enum::odb_DimensionEnum space,
                 odb_Enum::odb_PartTypeEnum type,
                 const ddb_PartInstance& i,
                 const odb_Mesh& mesh,
                 bool readInternalSets = false);

    ddb_PartInstance instance() const;
    const odb_Mesh& mesh() const;

    bool IsSetNameTaken(const odb_String& name);

    void summaryValue1( odb_String & ) const {};
    void summaryValue2( odb_String & ) const {};
};

COW_COW_DECL(odb_Instance);

#endif // odb_Instance_h
