//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_SequenceElement_h
#define odb_SequenceElement_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_Element.h>
#include <odb_Mesh.h>
// End local includes

// Forward Declarations
class odb_Instance;
class odb_Assembly;


class odb_SequenceElement : public mem_AllocationOperators
{

public:
    odb_SequenceElement(const odb_Instance& instance);
    odb_SequenceElement(const odb_Assembly& assembly);
    odb_Element element(uint index) const;
    odb_Element constGet(uint index) const;
    odb_Element get(uint index);
    odb_Element operator[](uint index) const;
    void append(const odb_Element& element);
    void append(const odb_SequenceElement& elementSequence);
    int size() const;
    bool isMember(const odb_Element& element) const;
    bool isEqual(const odb_SequenceElement& elementSequence) const;
    odb_SequenceElement slice(const int& lo, const int& hi) const;

private:
    bool            m_allElements;
    odb_SequenceInt m_elIndexList;
    odb_MeshCOW        m_mesh;

public:  // undocumented and unsupported
    odb_SequenceElement();
    odb_SequenceElement(const odb_SequenceInt& elIndexList,
                        const odb_MeshCOW& mesh);
    odb_SequenceElement(const odb_MeshCOW& mesh);

    int numElements() const;
    odb_Element anyElement(uint index) const; // includes internal elements

    odb_String instanceName() const;

    void SetList(const odb_SequenceInt& list);
    void AppendLabel(int elementLabel);
    void AppendIndex(int elementIndex);

    odb_Element ConstGet(uint index) const;
    odb_Element Get(uint index);
    void Append(const odb_Element& element);
    void Append(const odb_SequenceElement& elementSequence);
    inline const odb_Mesh& mesh() const { return m_mesh.ConstGet(); };

    int Size() const;
};

inline bool operator==(const odb_SequenceElement& a,
               const odb_SequenceElement& b)
{ return a.isEqual(b); }

inline bool operator!=(const odb_SequenceElement& a,
               const odb_SequenceElement& b)
{ return !(a.isEqual(b)); }

#endif // #ifndef odb_SequenceElement_h
