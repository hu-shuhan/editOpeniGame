//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_Assembly_h
#define odb_Assembly_h

// Begin local includes
#include <odb_DiscreteEntity.h>

// Forward declarations
class odb_Section;
class odb_SectionAssignment;
class odb_SequenceSectionAssignment;
class odb_ConnectorOrientation;
class odb_SequenceConnectorOrientation;
class ddb_Assembly;
COW_ARRAYCOW_DECL(ddb_Assembly);
class ddb_Model;
COW_COW_DECL(ddb_Model);
class odb_Set;
class odb_Part;
class odb_Instance;
class odb_InstanceRepository;
COW_COW_DECL(odb_InstanceRepository);
class odb_DatumCsysRepository;
COW_COW_DECL(odb_DatumCsysRepository);
class odb_DatumCsys;
class odb_SequenceSequenceFloat;
class odb_RigidBody;
class odb_SequenceRigidBody;
class odb_SequencePretensionSection;
class odb_SequenceElement;

class odb_Assembly : public odb_DiscreteEntity
{
public:
    // sets, see also odb_DiscreteEntity
    odb_Set& NodeSet(const odb_String& name,
                 const odb_SequenceString& instanceNames,
                     const odb_SequenceSequenceInt& nodeLabels,
                     bool internal = false, bool unsorted = false);
    odb_Set& ElementSet(const odb_String& name,
                    const odb_SequenceString& instanceNames,
                        const odb_SequenceSequenceInt& elementLabels,
                        bool internal = false);
    odb_Set& Surface(const odb_String& name,
                 const odb_SequenceString& instanceNames,
                     const odb_SequenceSequenceInt& elementLabels,
                     const odb_SequenceSequenceElementFace& faces,
                     bool internal = false);

    // instance
    odb_Instance& Instance(const odb_String& name,
                           const odb_Part& part,
                           const odb_SequenceSequenceFloat& transForm =
                         odb_SequenceSequenceFloat());
    odb_InstanceRepository& instances();
    const odb_InstanceRepository& instances() const;
    odb_DatumCsys& DatumCsysByThreePoints(const odb_String &,
                      const odb_Enum::odb_DatumCsysTypeEnum,
                      const odb_SequenceFloat& origin,
                      const odb_SequenceFloat& point1,
                      const odb_SequenceFloat& point2);
    odb_DatumCsys& DatumCsysByThreeNodes(const odb_String &,
                     const odb_Enum::odb_DatumCsysTypeEnum,
                     const odb_Node& origin,
                     const odb_Node& point1,
                     const odb_Node& point2);
    odb_DatumCsys& DatumCsysBy6dofNode(const odb_String &,
                       const odb_Enum::odb_DatumCsysTypeEnum,
                       const odb_Node& origin);
    odb_DatumCsys& DatumCsysByThreeCircNodes(const odb_String &,
                         const odb_Enum::odb_DatumCsysTypeEnum,
                         const odb_Node&,
                         const odb_Node&,
                         const odb_Node&);
    odb_DatumCsys& DatumCsys(const odb_String &, const odb_DatumCsys&);
    void DeleteDatumCsys(const odb_String &);
    odb_DatumCsysRepository& datumCsyses();

    void SectionAssignment(const odb_Set& elset, const odb_Section& section, double offset=0);
    odb_SectionAssignment sectionAssignments(int index) const;
    odb_SequenceSectionAssignment sectionAssignments() const;

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
    odb_RigidBody RigidBody( const odb_Set& refNodeSet,
                             bool isothermal = false,
                             odb_String position = "INPUT" );
    odb_SequenceRigidBody rigidBodies() const;
    odb_SequencePretensionSection pretensionSections() const;

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
    void addElements(
        const odb_SequenceInt& labels,
        const odb_SequenceSequenceInt& connectivity,
        const odb_SequenceString& instanceNames,
        const odb_String& elementType,
        const odb_String& setName = odb_String(),
        const odb_SectionCategory& sectionCategory = odb_SectionCategory(),
        const odb_ElemCtrlParams& elemCtrls = odb_ElemCtrlParams());
    // solverInput snippet
    void setSolverInput(const odb_String& solverInput);
    odb_String solverInput() const;

    void setName(const cow_String& name);

private:
    ddb_AssemblyCOW        m_assembly;
    ddb_ModelCOW           m_model;
    odb_InstanceRepositoryCOW  m_instanceCon;
    odb_DatumCsysRepositoryCOW m_datumCsysCon;

    // utility methods for error checking
    void checkElementLabelsAndConnectivityData( const odb_String& elementType,
                                                const odb_SequenceInt& labels,
                                                const odb_SequenceSequenceInt& connectivity );
    void checkElementType( const odb_String& elementType );
    void updateConnectivityDataForGroundedConnectors( const odb_String& elementType,
                                                      odb_SequenceSequenceInt& conn );

public:   // undocumented and unsupported
    odb_Assembly();
    odb_Assembly(const odb_String& name,
                 const ddb_Assembly& assy,
                 const ddb_Model& model);

    const ddb_Assembly& Assembly() const;
    const ddb_Model& Model() const;
    const odb_MeshList& meshList() const;
    void Read(bool readInternalSets = false);
    void ReadAssembly();
    void ReadInstances(bool readInternalSets = false);
    void ReadDatumCsyses();
    void ReadSets (bool readInternalSets = false);

    odb_Set& ModifyNodeSet(const odb_String& name,
                           const odb_SequenceString& instanceNames,
                           const odb_SequenceSequenceInt& nodeLabels,
                           bool internal = false,
                           bool unsorted = false);
    odb_Set& ModifyElementSet(const odb_String& name,
                              const odb_SequenceString& instanceNames,
                              const odb_SequenceSequenceInt& elementLabels,
                              bool internal = false);
    odb_Set& ModifySurface(const odb_String& name,
                           const odb_SequenceString& instanceNames,
                           const odb_SequenceSequenceInt& elementLabels,
                           const odb_SequenceSequenceElementFace& faces,
                           bool internal = false);
};

COW_COW_DECL(odb_Assembly);

#endif // odb_Assembly_h
