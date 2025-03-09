//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_SequenceNode_h
#define odb_SequenceNode_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_Node.h>
#include <odb_Mesh.h>
// End local includes

class odb_SequenceNode  : public mem_AllocationOperators
{

public:  // documented and supported

    odb_Node node(uint index) const;
    const odb_Node constGet(const uint& index) const {return node(index);};
    odb_Node get(const uint& index) {return node(index);};
    odb_Node operator[](uint index) const;
    void append(const odb_Node& node);
    void append(const odb_SequenceNode& nodeSequence);
    int  size() const;
    bool isMember(const odb_Node& node) const;
    bool isEqual(const odb_SequenceNode& nodeSequence) const;
    odb_SequenceNode slice(const int& lo, const int& hi) const;

    // undocumented and unsupported

    odb_SequenceNode();
    odb_SequenceNode(const odb_SequenceInt& nodeIndexList,
                     const odb_MeshCOW& mesh); 
    odb_SequenceNode(const odb_MeshCOW& mesh); 

    int numNodes() const;

    odb_String instanceName() const;
    const odb_Node ConstGet(const uint& index) const {return node(index);};
    odb_Node Get(const uint& index) {return node(index);};
    void Append(const odb_Node& node);
    void Append(const odb_SequenceNode& nodeSequence);

    int Size() const;
    bool isNativeNode(int index) const;

    void SetList(const odb_SequenceInt& list);
    void AppendLabel(int nodeLabel);
    void AppendIndex(int nodeIndex);
    const odb_Mesh& mesh() const { return m_mesh.ConstGet(); };

private:
    bool            m_allNodes;
    odb_SequenceInt m_nIndexList;
    odb_MeshCOW        m_mesh;
};

inline bool operator==(const odb_SequenceNode& a, 
		       const odb_SequenceNode& b)
{ return a.isEqual(b); }

inline bool operator!=(const odb_SequenceNode& a, 
		       const odb_SequenceNode& b)
{ return !(a.isEqual(b)); }

#endif // #ifndef odb_SequenceNode_h
