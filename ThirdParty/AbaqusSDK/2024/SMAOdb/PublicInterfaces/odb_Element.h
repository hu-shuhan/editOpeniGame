//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_Element_h
#define odb_Element_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_Types.h>
#include <odb_Enum.h>
#include <odb_SectionCategory.h>
#include <cow_ArrayCOW.h>

// Forward declarations
class ddb_ScopedNodeData;
class ddb_Mesh;
COW_COW_DECL(ddb_Mesh);
// End local includes

class odb_Element  : public mem_AllocationOperators
{

    friend class odb_SequenceElement;

 public:

    // documented and supported user interfaces

    // member access
    int label() const { return _userLabel; };
    odb_String type() const { return _elementType; };
    const int* connectivity(int& numNodes) const { numNodes = _numNodes; return _connectivity; };
    odb_SequenceInt connectivity() const { return odb_SequenceInt(_connectivity, _numNodes); };
    odb_SequenceString instanceNames() const;
    const odb_SectionCategory& sectionCategory() const { return _sectionCategory; };
    int numberOfIntegrationPoints() const { return _numIntegrationPointsPerElement; };
    //comparison operator
    bool operator==(const odb_Element& rhs) const { return !notEqual(rhs); };
    bool operator!=(const odb_Element& rhs) const { return notEqual(rhs); };

    // undocumented and unsupported

    odb_Element();
    odb_Element(const odb_Element&); // copy constructor
    odb_Element& operator=(const odb_Element& that); // assignment
    ~odb_Element();

    odb_Element(int index,
		int userLabel,
		const int* connectivity,
		int meshIndex,
		int numNodes,
		const odb_String& elementType,
		const odb_SectionCategory& sectionCategory,
		odb_Enum::odb_ElementGeometryClass geometry,
		int numIntegrationPointsPerElement,
		int classIndex,
		const cow_ListInt& userNodeLabels,
		const ddb_ScopedNodeData& nodeDataScoped,
                const ddb_Mesh& ddbMesh);
    odb_Element(int index,
		int userLabel,
		const int* connectivity,
		int meshIndex,
		int numNodes,
		const odb_String& elementType,
		const odb_SectionCategory& sectionCategory,
		odb_Enum::odb_ElementGeometryClass geometry,
		int numIntegrationPointsPerElement,
		int classIndex,
		const cow_ListInt& userNodeLabels,
                const ddb_Mesh& ddbMesh);

    int meshIndex() const { return _meshIndex; };
    int index() const { return _index; };    

    odb_Enum::odb_ElementGeometryClass geometry() const { return _geometry; };

 private:

    int _index;
    int _userLabel;
    int *_connectivity;
    odb_SequenceString _instanceNames;
    int _meshIndex;
    int _numNodes;
    odb_String _elementType;
    odb_SectionCategory _sectionCategory;
    odb_Enum::odb_ElementGeometryClass _geometry;
    int _numIntegrationPointsPerElement;
    int _classIndex;
    ddb_MeshCOW _mesh; 
    bool notEqual(const odb_Element& rhs) const;

    // undocumented and unsupported methods
 public:

    int inSeqIndex() const;
};

#endif // odb_Element_h

