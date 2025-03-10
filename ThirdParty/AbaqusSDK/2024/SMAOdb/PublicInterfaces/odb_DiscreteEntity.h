//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_DiscreteEntity_h
#define odb_DiscreteEntity_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_Enum.h>
#include <odb_SequenceElementClass.h>
#include <odb_SetRepository.h>
#include <odb_ElemCtrlParams.h>
// End local includes



class odb_DiscreteEntity  : public mem_AllocationOperators
{
public:
    struct ElementsByClass : public mem_AllocationOperators
    {
        odb_SequenceSequenceInt connectivity;
        odb_SequenceString types;
        odb_SequenceString sectionCategoryNames;
        odb_SequenceInt numNodesPerElement;
        odb_SequenceInt userNodeLables;
        odb_SequenceInt labels;
        odb_SequenceString instanceNames;
    };
    // member access
    odb_String name() const;
    odb_Enum::odb_DimensionEnum embeddedSpace() const;
    odb_Enum::odb_PartTypeEnum type() const;

    // node and elements 
    odb_Element elements(int i) const;
    odb_SequenceElement&  elements() const;
    odb_Element getElementFromLabel(int label) const;
    odb_Node nodes(int i) const;
    odb_SequenceNode& nodes() const;
    odb_Node getNodeFromLabel(int label) const;
    const odb_SequenceElementClass & elementClasses() const;


    // node, element, and surface sets
    // retrieve set containers (create: see Instance, Part, Assembly)
    odb_SetRepository& nodeSets(bool internal=false) const;
    odb_SetRepository& elementSets(bool internal=false) const;
    odb_SetRepository& surfaces(bool internal=false) const;

    // Interface for adding nodes and elements to the component
    virtual void addNodes(
	const odb_SequenceInt& labels,
	const odb_SequenceSequenceFloat& coordinates,
	const odb_String& setName = odb_String()) = 0;
    virtual void addElements(
	const odb_SequenceInt& labels,
	const odb_SequenceSequenceInt& connectivity,
	const odb_String& elementType,
	const odb_String& setName = odb_String(),
	const odb_SectionCategory& sectionCategory = odb_SectionCategory(), 
	const odb_ElemCtrlParams& elemCtrls = odb_ElemCtrlParams()) =0;

    static odb_SequenceInt nodesLabelsFromNodeTypeFace (
	const odb_SequenceInt& elementNodeLabels,
	const odb_String& elementType,
	odb_Enum::odb_ElementFaceEnum);


protected:

    odb_String m_name;
    odb_Enum::odb_DimensionEnum m_embeddedSpace;
    odb_Enum::odb_PartTypeEnum m_type;
    odb_MeshCOW m_mesh;
    odb_SequenceElement m_elCon;
    odb_SequenceNode m_nCon;

public:   // undocumented and unsupported
    odb_DiscreteEntity();
    odb_DiscreteEntity(const odb_String& name,
                       odb_Enum::odb_DimensionEnum space,
                       odb_Enum::odb_PartTypeEnum type,
                       const odb_Mesh& mesh,
                       bool readInternalSets = false);
    ElementsByClass readElementsByClass();
    void readElementsConnectivity(bool readConnectivityByClass);

private:

    mutable odb_SequenceElementClass m_classCon;
    mutable odb_SetRepository m_nodeSetCon;
    mutable odb_SetRepository m_elementSetCon;
    mutable odb_SetRepository m_surfaceSetCon;
    bool m_readInternalSets;
    bool m_initBulk;
    bool m_readConnectivityByClass;
    odb_SequenceSequenceInt m_elemConnect;
    odb_SequenceString m_elemClassTypes;
    odb_SequenceString m_elemSectionCatNames;
    odb_SequenceInt m_nodesPerElemPerClass;
    odb_SequenceInt m_userNodeLables;
    odb_SequenceInt m_ElementsLabel;

    void InitBulk();
    void ReadElementsAsBulk();

};

#endif // odb_DiscreteEntity_h
