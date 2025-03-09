//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_Mesh_h
#define odb_Mesh_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_Types.h>
#include <odb_Element.h>
#include <odb_Node.h>
#include <odb_SectionCategory.h>
#include <cow_ArrayCOW.h>

// Forward Declarations
class ddb_Mesh;
COW_COW_DECL(ddb_Mesh);
class ddb_ElementClass;
class bme_ElementClass;
class odiK_Mesh;
class ddb_ScopedNodeData;
COW_COW_DECL(ddb_ScopedNodeData);
class uti_CoordCont3D;
struct odiK_SubsetStruct;
COW_LIST_DECL(odiK_SubsetStruct, odiK_SubsetStructList);
typedef odiK_SubsetStructList ddb_SubsetStructList;


class odb_Mesh : public mem_AllocationOperators
{
public:   
    odb_Node node(uint index) const;
    odb_Node getNodeFromLabel(int label) const;

    odb_Element element(uint index) const { return getElement(index,true); }
    odb_Element getElementFromLabel(int label) const;
    bool isAssemblyMesh() const { return m_isAssemblyMesh; }

private:
    ddb_MeshCOW          m_mesh;
    odb_String           m_name;

  // the following members are added to improve the access performance
    mutable int          m_classIndex;
    mutable int          m_searched;
    mutable int          m_numElements;
    mutable int          m_numElsInClass;
    mutable int          m_NumNodes;
    mutable int          m_elementIndex;

  // the following members are added for fast element access since they are the same for
  // same element class
    int                    MeshIndex;
    int                    numNodesPerElement;
    const int*             userLabelArray;
    const int*             connectivityArray;
    odb_String             elementType;
    odb_SectionCategory    sectionCategory;
    odb_Enum::odb_ElementGeometryClass geometry;
    int                    numIntegrationPointsPerElement;
    uti_CoordCont3D*       coordCont;
    cow_ListInt     userNodeLabels;
    ddb_ScopedNodeDataCOW  nodeDataScoped;
    bool m_isAssemblyMesh;
  
    void updateElementInformation(const bme_ElementClass& elClass,
				  const odiK_Mesh& mesh) const;
    void CheckHasMesh() const;

public:   // undocumented and unsupported
    odb_Mesh();
    odb_Mesh(const odb_String& name);
    odb_Mesh(const ddb_Mesh&, const odb_String& name, const bool assemblyMesh);

    // nodes
    inline int numNodes( ) const { return m_NumNodes; };
    int getNodeIndexFromLabel(int nodeLabel) const;
    bool isNativeNode(int index) const;

    // elements
    int numElements(bool internal=false) const;
    odb_Element getElementFromElementClassData(int classIndex,
                                            int elementIndexInClass) const;
    int getElementIndexFromLabel(int elementLabel) const;
    int getLabelFromElementIndex(int index) const;
    ddb_ElementClass getElementClassFromLabel(int elementLabel) const;
    odb_Element getElement(uint index, bool internal=false) const;
    odb_String getNodeSetNameFromSetIndex(int setIndex) const;

    void resetCounters() const;
    inline bool isEmpty() const { return m_mesh.IsNull(); };
    void ElIndexForIntEl (int classIndex,
                          int intElNo,
                          int& elClassIndex,
                          int& elIndexInClass,
                          int& elIndexInMesh);
    odb_SequenceInt ConnIndicesForEl (int classIndex,
                                      int intElMeshIndex);

    // 
    odb_String name() const;
    ddb_Mesh mesh () const;

    bool IsSetNameTaken(const odb_String& name) const;

    // edit mesh
    void addNodes(const odb_SequenceInt&           labels,
                  const odb_SequenceSequenceFloat& coor,
                  const odb_String&                setName="");
    void addElements(const odb_SequenceInt&     labels,
                 const odb_SequenceSequenceInt& connect,
                 const odb_String&              elType,
                 const odb_String&              setName="",
                 const odb_SectionCategory&     sectionCategory =
		                                odb_SectionCategory());
		

};

COW_ARRAYCOW_DECL(odb_Mesh);

#endif // #ifndef odb_Mesh_h
