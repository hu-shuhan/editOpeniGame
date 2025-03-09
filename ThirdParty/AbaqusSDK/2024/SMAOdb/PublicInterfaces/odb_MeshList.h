//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_MeshList_h
#define odb_MeshList_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_Mesh.h>
#include <cow_List.h>
// End local includes

COW_LIST_DECL(odb_MeshCOW, SequenceMesh);

class odb_MeshList  : public mem_AllocationOperators
{
public:   // undocumented and unsupported
    odb_MeshList();
    odb_MeshList(const odb_MeshCOW&);
    odb_MeshList(const SequenceMesh&);

    void Append(const odb_MeshCOW& m);
    void Append(odb_MeshList& ml);
    void Insert(const odb_MeshCOW& m);
    void Insert(odb_MeshList& ml);

    int numMeshes () const {return m_meshSeq.Length();}
    const odb_MeshCOW mesh (int i) const; 
    odb_MeshCOW mesh (const odb_String& name) const;
    odb_MeshCOW meshFromMeshIndex (int meshIndex) const;
    odb_String nameFromMeshIndex (int meshIndex) const;
    odb_SequenceString instanceNames() const ;

    bool IsSetNameTaken(const odb_String& name) const;

    bool IsMember(const odb_String& name) const;
    bool IsMember(const odb_Mesh& mesh) const;

    void ElIndexForIntEl(int partIndex,
                         int classIndex, int intElNo,
                         int& elClassIndex,
                         int& elIndexInClass,
                         int& elIndexInMesh);

    odb_SequenceInt ConnIndicesForEl(int partIndex,
                                     int classIndex,
                                     int intElMeshIndex);

private:
    SequenceMesh            m_meshSeq;
};


#endif // #ifndef odb_MeshList_h
