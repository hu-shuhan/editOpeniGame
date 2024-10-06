#include "iGameVolumeMesh.h"
#include "iGameFaceTable.h"
#include "iGameModelSurfaceFilters/iGameModelGeometryFilter.h"
#include "iGameScene.h"

IGAME_NAMESPACE_BEGIN

IGsize VolumeMesh::GetNumberOfVolumes() const noexcept {
    return m_Volumes ? m_Volumes->GetNumberOfCells() : 0;
}

CellArray* VolumeMesh::GetVolumes() {
    return m_Volumes ? m_Volumes.get() : nullptr;
}

void VolumeMesh::SetVolumes(CellArray::Pointer volumes) {
    if (m_Volumes != volumes) {
        m_Volumes = volumes;
        this->Modified();
    }
}

Volume* VolumeMesh::GetVolume(const IGsize volumeId) {
    const igIndex* cell;
    int ncells = m_Volumes->GetCellIds(volumeId, cell);

    Volume* volume = nullptr;
    if (ncells == Tetra::NumberOfPoints) {
        if (m_Tetra == nullptr) { m_Tetra = Tetra::New(); }
        volume = m_Tetra.get();
    } else if (ncells == Hexahedron::NumberOfPoints) {
        if (m_Hexahedron == nullptr) { m_Hexahedron = Hexahedron::New(); }
        volume = m_Hexahedron.get();
    } else if (ncells == Prism::NumberOfPoints) {
        if (m_Prism == nullptr) { m_Prism = Prism::New(); }
        volume = m_Prism.get();
    } else if (ncells == Pyramid::NumberOfPoints) {
        if (m_Pyramid == nullptr) { m_Pyramid = Pyramid::New(); }
        volume = m_Pyramid.get();
    }

    assert(volume != nullptr);
    volume->PointIds->Reset();
    volume->Points->Reset();

    for (int i = 0; i < ncells; i++) {
        volume->PointIds->AddId(cell[i]);
        volume->Points->AddPoint(this->GetPoint(cell[i]));
    }

    return volume;
}

int VolumeMesh::GetVolumePointIds(const IGsize volumeId, igIndex* ptIds) {
    return m_Volumes->GetCellIds(volumeId, ptIds);
}

int VolumeMesh::GetVolumeEdgeIds(const IGsize volumeId, igIndex* edgeIds) {
    return m_VolumeEdges->GetCellIds(volumeId, edgeIds);
}

int VolumeMesh::GetVolumeFaceIds(const IGsize volumeId, igIndex* faceIds) {
    return m_VolumeFaces->GetCellIds(volumeId, faceIds);
}


void VolumeMesh::BuildFacesAndEdges() {
    FaceTable::Pointer FaceTable = FaceTable::New();
    EdgeTable::Pointer EdgeTable = EdgeTable::New();
    igIndex cell[64]{}, faceIds[64]{}, edgeIds[64]{}, face[64]{}, edge[2]{},
            isInsert[64]{};


    if (!this->IsPolyhedronType) {
        m_VolumeEdges = CellArray::New();
        m_VolumeFaces = CellArray::New();
        m_FaceEdges = CellArray::New();

        for (IGsize i = 0; i < this->GetNumberOfVolumes(); i++) {
            Volume* vol = this->GetVolume(i);
            m_Volumes->GetCellIds(i, cell);
            memset(isInsert, 0, vol->GetNumberOfFaces() * sizeof(igIndex));
            for (int j = 0; j < vol->GetNumberOfFaces(); j++) // number of faces
            {
                const igIndex* index;
                int size = vol->GetFacePointIds(
                        j, index); // this face's number of points
                for (int k = 0; k < size; k++) { face[k] = cell[index[k]]; }
                igIndex idx;
                if ((idx = FaceTable->IsFace(face, size)) == -1) {
                    idx = FaceTable->GetNumberOfFaces();
                    FaceTable->InsertFace(face, size);
                    isInsert[j] = 1;
                }
                faceIds[j] = idx;
            }
            m_VolumeFaces->AddCellIds(faceIds, vol->GetNumberOfFaces());

            for (int j = 0; j < vol->GetNumberOfEdges(); j++) {
                const igIndex* index;
                vol->GetEdgePointIds(j, index); // this edge's number of points
                for (int k = 0; k < 2; k++) { edge[k] = cell[index[k]]; }
                igIndex idx;
                if ((idx = EdgeTable->IsEdge(edge[0], edge[1])) == -1) {
                    idx = EdgeTable->GetNumberOfEdges();
                    EdgeTable->InsertEdge(edge[0], edge[1]);
                }
                edgeIds[j] = idx;
            }
            m_VolumeEdges->AddCellIds(edgeIds, vol->GetNumberOfEdges());

            for (int j = 0; j < vol->GetNumberOfFaces(); j++) {
                if (isInsert[j]) {
                    const igIndex* index;
                    int size = vol->GetFaceEdgeIds(j, index);
                    for (int k = 0; k < size; k++) {
                        face[k] = edgeIds[index[k]];
                    }
                    m_FaceEdges->AddCellIds(face, size);
                }
            }
        }

        m_Faces = FaceTable->GetOutput();
        m_Edges = EdgeTable->GetOutput();
    } else {
        m_VolumeEdges = CellArray::New();
        m_FaceEdges = CellArray::New();
    }
}

void VolumeMesh::BuildVolumeLinks() {
    if (m_VolumeLinks && m_VolumeLinks->GetMTime() > m_Volumes->GetMTime()) {
        return;
    }

    m_VolumeLinks = CellLinks::New();
    IGsize npts = GetNumberOfPoints();
    IGsize nvolumes = GetNumberOfVolumes();
    igIndex cell[32]{};

    m_VolumeLinks->Allocate(npts);
    for (IGsize i = 0; i < nvolumes; i++) {
        int size = m_Volumes->GetCellIds(i, cell);
        for (int j = 0; j < size; j++) {
            m_VolumeLinks->IncrementLinkSize(cell[j]);
        }
    }

    m_VolumeLinks->AllocateLinks(npts);
    for (IGsize i = 0; i < nvolumes; i++) {
        int size = m_Volumes->GetCellIds(i, cell);
        for (int j = 0; j < size; j++) {
            m_VolumeLinks->AddReference(cell[j], i);
        }
    }
}

void VolumeMesh::BuildVolumeEdgeLinks() {
    if (m_VolumeEdgeLinks &&
        m_VolumeEdgeLinks->GetMTime() > m_VolumeEdges->GetMTime()) {
        return;
    }

    m_VolumeEdgeLinks = CellLinks::New();
    IGsize nedges = this->GetNumberOfEdges();
    IGsize nvolumes = this->GetNumberOfVolumes();
    igIndex cell[64]{};

    m_VolumeEdgeLinks->Allocate(nedges);
    for (IGsize i = 0; i < nvolumes; i++) {
        int size = m_VolumeEdges->GetCellIds(i, cell);
        for (int j = 0; j < size; j++) {
            m_VolumeEdgeLinks->IncrementLinkSize(cell[j]);
        }
    }

    m_VolumeEdgeLinks->AllocateLinks(nedges);
    for (IGsize i = 0; i < nvolumes; i++) {
        int size = m_VolumeEdges->GetCellIds(i, cell);
        for (int j = 0; j < size; j++) {
            m_VolumeEdgeLinks->AddReference(cell[j], i);
        }
    }
}

void VolumeMesh::BuildVolumeFaceLinks() {
    if (m_VolumeFaceLinks &&
        m_VolumeFaceLinks->GetMTime() > m_VolumeFaces->GetMTime()) {
        return;
    }

    m_VolumeFaceLinks = CellLinks::New();
    IGsize nfaces = this->GetNumberOfFaces();
    IGsize nvolumes = this->GetNumberOfVolumes();
    igIndex cell[32]{};

    m_VolumeFaceLinks->Allocate(nfaces);
    for (IGsize i = 0; i < nvolumes; i++) {
        int size = m_VolumeFaces->GetCellIds(i, cell);
        for (int j = 0; j < size; j++) {
            m_VolumeFaceLinks->IncrementLinkSize(cell[j]);
        }
    }

    m_VolumeFaceLinks->AllocateLinks(nfaces);
    for (IGsize i = 0; i < nvolumes; i++) {
        int size = m_VolumeFaces->GetCellIds(i, cell);
        for (int j = 0; j < size; j++) {
            m_VolumeFaceLinks->AddReference(cell[j], i);
        }
    }
}

int VolumeMesh::GetNumberOfLinks(const IGsize id, Type type) {
    int size = 0;
    switch (type) {
        case iGame::SurfaceMesh::P2V:
            size = m_VolumeLinks->GetLinkSize(id);
            break;
        case iGame::SurfaceMesh::E2V:
            size = m_VolumeEdgeLinks->GetLinkSize(id);
            break;
        case iGame::SurfaceMesh::F2V:
            size = m_VolumeFaceLinks->GetLinkSize(id);
            break;
        default:
            return SurfaceMesh::GetNumberOfLinks(id, type);
    }
    return size;
}

int VolumeMesh::GetPointToNeighborVolumes(const IGsize ptId,
                                          igIndex* volumeIds) {
    assert(ptId < GetNumberOfPoints() && "ptId too large");
    auto& link = m_VolumeLinks->GetLink(ptId);
    for (int i = 0; i < link.size; i++) { volumeIds[i] = link.pointer[i]; }
    return link.size;
}

bool VolumeMesh::GetPointToNeighborVolumes(const IGsize ptId,
                                           const igIndex*& volumeIds,
                                           int& size) {
    assert(ptId < GetNumberOfPoints() && "ptId too large");
    volumeIds = m_VolumeLinks->GetLinkPointer(ptId);
    size = m_VolumeLinks->GetLinkSize(ptId);
    return true;
}

bool VolumeMesh::GetPointToNeighborVolumes(const IGsize ptId,
                                           IdArray::Pointer volumeIds) {
    assert(ptId < GetNumberOfPoints() && "ptId too large");
    const igIndex* pt = m_VolumeLinks->GetLinkPointer(ptId);
    int size = m_VolumeLinks->GetLinkSize(ptId);
    volumeIds->Reset();
    for (int i = 0; i < size; i++) { volumeIds->AddId(pt[i]); }
    return true;
}

bool VolumeMesh::GetPointToNeighborVolumes(const IGsize ptId,
                                           ReturnContainer& volumeIds) {
    assert(ptId < GetNumberOfPoints() && "ptId too large");
    const igIndex* pt = m_VolumeLinks->GetLinkPointer(ptId);
    int size = m_VolumeLinks->GetLinkSize(ptId);
    volumeIds.reset();
    for (int i = 0; i < size; i++) { volumeIds.push_back(pt[i]); }
    return true;
}

int VolumeMesh::GetEdgeToNeighborVolumes(const IGsize edgeId,
                                         igIndex* volumeIds) {
    assert(edgeId < GetNumberOfEdges() && "edgeId too large");
    auto& link = m_VolumeEdgeLinks->GetLink(edgeId);
    for (int i = 0; i < link.size; i++) { volumeIds[i] = link.pointer[i]; }
    return link.size;
}

bool VolumeMesh::GetEdgeToNeighborVolumes(const IGsize edgeId,
                                          const igIndex*& volumeIds,
                                          int& size) {
    assert(edgeId < GetNumberOfEdges() && "edgeId too large");
    volumeIds = m_VolumeEdgeLinks->GetLinkPointer(edgeId);
    size = m_VolumeEdgeLinks->GetLinkSize(edgeId);
    return true;
}

bool VolumeMesh::GetEdgeToNeighborVolumes(const IGsize edgeId,
                                          IdArray::Pointer volumeIds) {
    assert(edgeId < GetNumberOfEdges() && "edgeId too large");
    const igIndex* pt = m_VolumeEdgeLinks->GetLinkPointer(edgeId);
    int size = m_VolumeEdgeLinks->GetLinkSize(edgeId);
    volumeIds->Reset();
    for (int i = 0; i < size; i++) { volumeIds->AddId(pt[i]); }
    return true;
}

bool VolumeMesh::GetEdgeToNeighborVolumes(const IGsize edgeId,
                                          ReturnContainer& volumeIds) {
    assert(edgeId < GetNumberOfEdges() && "edgeId too large");
    const igIndex* pt = m_VolumeEdgeLinks->GetLinkPointer(edgeId);
    int size = m_VolumeEdgeLinks->GetLinkSize(edgeId);
    volumeIds.reset();
    for (int i = 0; i < size; i++) { volumeIds.push_back(pt[i]); }
    return true;
}

int VolumeMesh::GetFaceToNeighborVolumes(const IGsize faceId,
                                         igIndex* volumeIds) {
    assert(faceId < GetNumberOfFaces() && "faceId too large");
    auto& link = m_VolumeFaceLinks->GetLink(faceId);
    for (int i = 0; i < link.size; i++) { volumeIds[i] = link.pointer[i]; }
    return link.size;
}

bool VolumeMesh::GetFaceToNeighborVolumes(const IGsize faceId,
                                          const igIndex*& volumeIds,
                                          int& size) {
    assert(faceId < GetNumberOfFaces() && "faceId too large");
    volumeIds = m_VolumeFaceLinks->GetLinkPointer(faceId);
    size = m_VolumeFaceLinks->GetLinkSize(faceId);
    return true;
}

bool VolumeMesh::GetFaceToNeighborVolumes(const IGsize faceId,
                                          IdArray::Pointer volumeIds) {
    assert(faceId < GetNumberOfFaces() && "faceId too large");
    const igIndex* pt = m_VolumeFaceLinks->GetLinkPointer(faceId);
    int size = m_VolumeFaceLinks->GetLinkSize(faceId);
    volumeIds->Reset();
    for (int i = 0; i < size; i++) { volumeIds->AddId(pt[i]); }
    return true;
}

bool VolumeMesh::GetFaceToNeighborVolumes(const IGsize faceId,
                                          ReturnContainer& volumeIds) {
    assert(faceId < GetNumberOfFaces() && "faceId too large");
    const igIndex* pt = m_VolumeFaceLinks->GetLinkPointer(faceId);
    int size = m_VolumeFaceLinks->GetLinkSize(faceId);
    volumeIds.reset();
    for (int i = 0; i < size; i++) { volumeIds.push_back(pt[i]); }
    return true;
}

int VolumeMesh::GetVolumeToNeighborVolumesWithPoint(const IGsize vid,
                                                    igIndex* volumeIds) {
    assert(vid < GetNumberOfVolumes() && "volumeId too large");
    igIndex ids[32]{};
    int size = GetVolumePointIds(vid, ids);
    std::set<igIndex> st;
    for (int i = 0; i < size; i++) {
        auto& link = m_VolumeLinks->GetLink(ids[i]);
        for (int j = 0; j < link.size; j++) { st.insert(link.pointer[j]); }
    }
    int i = 0;
    for (auto& _vid: st) {
        if (_vid != vid) { volumeIds[i++] = _vid; }
    }
    return i;
}

bool VolumeMesh::GetVolumeToNeighborVolumesWithPoint(
        const IGsize vid, IdArray::Pointer volumeIds) {
    assert(vid < GetNumberOfVolumes() && "volumeId too large");
    igIndex ids[32]{};
    int size = GetVolumePointIds(vid, ids);
    std::set<igIndex> st;
    for (int i = 0; i < size; i++) {
        auto& link = m_VolumeLinks->GetLink(ids[i]);
        for (int j = 0; j < link.size; j++) { st.insert(link.pointer[j]); }
    }
    volumeIds->Reset();
    for (auto& _vid: st) {
        if (_vid != vid) { volumeIds->AddId(_vid); }
    }
    return true;
}

bool VolumeMesh::GetVolumeToNeighborVolumesWithPoint(
        const IGsize vid, ReturnContainer& volumeIds) {
    assert(vid < GetNumberOfVolumes() && "volumeId too large");
    igIndex ids[32]{};
    int size = GetVolumePointIds(vid, ids);
    std::set<igIndex> st;
    for (int i = 0; i < size; i++) {
        auto& link = m_VolumeLinks->GetLink(ids[i]);
        for (int j = 0; j < link.size; j++) { st.insert(link.pointer[j]); }
    }
    volumeIds.reset();
    for (auto& _vid: st) {
        if (_vid != vid) { volumeIds.push_back(_vid); }
    }
    return true;
}

int VolumeMesh::GetVolumeToNeighborVolumesWithEdge(const IGsize vid,
                                                   igIndex* volumeIds) {
    assert(vid < GetNumberOfVolumes() && "volumeId too large");
    igIndex ids[32]{};
    int size = GetVolumeEdgeIds(vid, ids);
    std::set<igIndex> st;
    for (int i = 0; i < size; i++) {
        auto& link = m_VolumeEdgeLinks->GetLink(ids[i]);
        for (int j = 0; j < link.size; j++) { st.insert(link.pointer[j]); }
    }
    int i = 0;
    for (auto& _vid: st) {
        if (_vid != vid) { volumeIds[i++] = _vid; }
    }
    return i;
}

bool VolumeMesh::GetVolumeToNeighborVolumesWithEdge(
        const IGsize vid, IdArray::Pointer volumeIds) {
    assert(vid < GetNumberOfVolumes() && "volumeId too large");
    igIndex ids[32]{};
    int size = GetVolumeEdgeIds(vid, ids);
    std::set<igIndex> st;
    for (int i = 0; i < size; i++) {
        auto& link = m_VolumeEdgeLinks->GetLink(ids[i]);
        for (int j = 0; j < link.size; j++) { st.insert(link.pointer[j]); }
    }
    volumeIds->Reset();
    for (auto& _vid: st) {
        if (_vid != vid) { volumeIds->AddId(_vid); }
    }
    return true;
}

bool VolumeMesh::GetVolumeToNeighborVolumesWithEdge(
        const IGsize vid, ReturnContainer& volumeIds) {
    assert(vid < GetNumberOfVolumes() && "volumeId too large");
    igIndex ids[32]{};
    int size = GetVolumeEdgeIds(vid, ids);
    std::set<igIndex> st;
    for (int i = 0; i < size; i++) {
        auto& link = m_VolumeEdgeLinks->GetLink(ids[i]);
        for (int j = 0; j < link.size; j++) { st.insert(link.pointer[j]); }
    }
    volumeIds.reset();
    for (auto& _vid: st) {
        if (_vid != vid) { volumeIds.push_back(_vid); }
    }
    return true;
}

int VolumeMesh::GetVolumeToNeighborVolumesWithFace(const IGsize vid,
                                                   igIndex* volumeIds) {
    assert(vid < GetNumberOfVolumes() && "volumeId too large");
    igIndex ids[32]{};
    int size = GetVolumeFaceIds(vid, ids);
    std::set<igIndex> st;
    for (int i = 0; i < size; i++) {
        auto& link = m_VolumeFaceLinks->GetLink(ids[i]);
        for (int j = 0; j < link.size; j++) { st.insert(link.pointer[j]); }
    }
    int i = 0;
    for (auto& _vid: st) {
        if (_vid != vid) { volumeIds[i++] = _vid; }
    }
    return i;
}

bool VolumeMesh::GetVolumeToNeighborVolumesWithFace(
        const IGsize vid, IdArray::Pointer volumeIds) {
    assert(vid < GetNumberOfVolumes() && "volumeId too large");
    igIndex ids[32]{};
    int size = GetVolumeFaceIds(vid, ids);
    std::set<igIndex> st;
    for (int i = 0; i < size; i++) {
        auto& link = m_VolumeFaceLinks->GetLink(ids[i]);
        for (int j = 0; j < link.size; j++) { st.insert(link.pointer[j]); }
    }
    volumeIds->Reset();
    for (auto& _vid: st) {
        if (_vid != vid) { volumeIds->AddId(_vid); }
    }
    return true;
}

bool VolumeMesh::GetVolumeToNeighborVolumesWithFace(
        const IGsize vid, ReturnContainer& volumeIds) {
    assert(vid < GetNumberOfVolumes() && "volumeId too large");
    igIndex ids[32]{};
    int size = GetVolumeFaceIds(vid, ids);
    std::set<igIndex> st;
    for (int i = 0; i < size; i++) {
        auto& link = m_VolumeFaceLinks->GetLink(ids[i]);
        for (int j = 0; j < link.size; j++) { st.insert(link.pointer[j]); }
    }
    volumeIds.reset();
    for (auto& _vid: st) {
        if (_vid != vid) { volumeIds.push_back(_vid); }
    }
    return true;
}

igIndex VolumeMesh::GetVolumeIdFormPointIds(igIndex* ids, int size) {
    IGsize sum = 0;
    for (int i = 0; i < size; i++) {
        if (ids[i] >= this->GetNumberOfPoints()) { return (-1); }
        sum += ids[i];
    }

    igIndex ptIds[32]{};
    ReturnContainer volumeIds;
    GetPointToNeighborVolumes(ids[0], volumeIds);
    for (int i = 0; i < volumeIds.size(); i++) {
        if (size != GetVolumePointIds(volumeIds[i], ptIds)) continue;
        IGsize index_sum = 0;
        for (int j = 0; j < size; j++) { index_sum += ptIds[j]; }
        if (sum == index_sum) {
            int count = 0;
            for (int j = 0; j < size; j++) {
                for (int k = 0; k < size; k++) {
                    if (ids[j] == ptIds[k]) {
                        count++;
                        break;
                    }
                }
            }
            if (count == size) return volumeIds[i];
        }
    }
    return (-1);
}

void VolumeMesh::RequestEditStatus() {
    if (InEditStatus()) { return; }
    RequestPointStatus();
    RequestFaceStatus();
    RequestVolumeStatus();
    MakeEditStatusOn();
}

void VolumeMesh::GarbageCollection() {
    IGsize i, mappedPtId = 0, mappedEdgeId = 0, mappedFaceId = 0;
    igIndex ptIds[32]{}, edgeIds[32]{}, faceIds[32]{}, e[2]{};
    CellArray::Pointer newEdges = CellArray::New();
    CellArray::Pointer newFaces = CellArray::New();
    CellArray::Pointer newFaceEdges = CellArray::New();
    CellArray::Pointer newVolumes = CellArray::New();
    CellArray::Pointer newVolumeEdges = CellArray::New();
    CellArray::Pointer newVolumeFaces = CellArray::New();

    IGsize npts = GetNumberOfPoints();
    IGsize nedges = GetNumberOfEdges();
    IGsize nfaces = GetNumberOfFaces();
    IGsize nvolumes = GetNumberOfVolumes();

    std::vector<igIndex> ptMap(npts);
    std::vector<igIndex> edgeMap(nedges);
    std::vector<igIndex> faceMap(nfaces);

    for (i = 0; i < npts; i++) {
        if (IsPointDeleted(i)) continue;
        if (i != mappedPtId) {
            m_Points->SetPoint(mappedPtId, m_Points->GetPoint(i));
        }
        ptMap[i] = mappedPtId;
        mappedPtId++;
    }
    m_Points->Resize(mappedPtId);

    for (i = 0; i < nedges; i++) {
        if (IsEdgeDeleted(i)) continue;
        m_Edges->GetCellIds(i, e);
        for (int j = 0; j < 2; j++) { e[j] = ptMap[e[j]]; }
        newEdges->AddCellIds(e, 2);
        edgeMap[i] = mappedEdgeId;
        mappedEdgeId++;
    }

    for (i = 0; i < nfaces; i++) {
        if (IsFaceDeleted(i)) continue;
        m_FaceEdges->GetCellIds(i, edgeIds);
        int size = m_Faces->GetCellIds(i, ptIds);
        for (int j = 0; j < size; j++) {
            ptIds[j] = ptMap[ptIds[j]];
            edgeIds[j] = edgeMap[edgeIds[j]];
        }
        newFaces->AddCellIds(ptIds, size);
        newFaceEdges->AddCellIds(edgeIds, size);
        faceMap[i] = mappedFaceId;
        mappedFaceId++;
    }

    for (i = 0; i < nvolumes; i++) {
        if (IsVolumeDeleted(i)) continue;
        int pt_size = m_Volumes->GetCellIds(i, ptIds);
        int edge_size = m_VolumeEdges->GetCellIds(i, edgeIds);
        int face_size = m_VolumeFaces->GetCellIds(i, faceIds);
        for (int j = 0; j < pt_size; j++) { ptIds[j] = ptMap[ptIds[j]]; }
        for (int j = 0; j < edge_size; j++) {
            edgeIds[j] = edgeMap[edgeIds[j]];
        }
        for (int j = 0; j < face_size; j++) {
            faceIds[j] = faceMap[faceIds[j]];
        }
        newVolumes->AddCellIds(ptIds, pt_size);
        newVolumeEdges->AddCellIds(edgeIds, edge_size);
        newVolumeFaces->AddCellIds(faceIds, face_size);
    }

    m_Edges = newEdges;
    m_Faces = newFaces;
    m_FaceEdges = newFaceEdges;
    m_Volumes = newVolumes;
    m_VolumeEdges = newVolumeEdges;
    m_VolumeFaces = newVolumeFaces;

    m_EdgeLinks = nullptr;
    m_FaceLinks = nullptr;
    m_FaceEdgeLinks = nullptr;
    m_VolumeLinks = nullptr;
    m_VolumeEdgeLinks = nullptr;
    m_VolumeFaceLinks = nullptr;

    m_PointDeleteMarker = nullptr;
    m_EdgeDeleteMarker = nullptr;
    m_FaceDeleteMarker = nullptr;
    m_VolumeDeleteMarker = nullptr;
    Modified();
    MakeEditStatusOff();
}

void VolumeMesh::GarbageCollection(bool delete_isolated_element) {
    IGsize i, mappedPtId = 0, mappedEdgeId = 0, mappedFaceId = 0;
    igIndex ptIds[32]{}, edgeIds[32]{}, faceIds[32]{}, e[2]{};
    CellArray::Pointer newEdges = CellArray::New();
    CellArray::Pointer newFaces = CellArray::New();
    CellArray::Pointer newFaceEdges = CellArray::New();
    CellArray::Pointer newVolumes = CellArray::New();
    CellArray::Pointer newVolumeEdges = CellArray::New();
    CellArray::Pointer newVolumeFaces = CellArray::New();

    IGsize npts = GetNumberOfPoints();
    IGsize nedges = GetNumberOfEdges();
    IGsize nfaces = GetNumberOfFaces();
    IGsize nvolumes = GetNumberOfVolumes();

    std::vector<igIndex> ptMap(npts);
    std::vector<igIndex> edgeMap(nedges);
    std::vector<igIndex> faceMap(nfaces);

    for (i = 0; i < npts; i++) {
        if (IsPointDeleted(i)) continue;
        if (delete_isolated_element && GetNumberOfLinks(i, P2V) == 0) continue;
        if (i != mappedPtId) {
            m_Points->SetPoint(mappedPtId, m_Points->GetPoint(i));
        }
        ptMap[i] = mappedPtId;
        mappedPtId++;
    }
    m_Points->Resize(mappedPtId);

    for (i = 0; i < nedges; i++) {
        if (IsEdgeDeleted(i)) continue;
        if (delete_isolated_element && GetNumberOfLinks(i, E2V) == 0) continue;
        m_Edges->GetCellIds(i, e);
        for (int j = 0; j < 2; j++) { e[j] = ptMap[e[j]]; }
        newEdges->AddCellIds(e, 2);
        edgeMap[i] = mappedEdgeId;
        mappedEdgeId++;
    }

    for (i = 0; i < nfaces; i++) {
        if (IsFaceDeleted(i)) continue;
        if (delete_isolated_element && GetNumberOfLinks(i, F2V) == 0) continue;
        m_FaceEdges->GetCellIds(i, edgeIds);
        int size = m_Faces->GetCellIds(i, ptIds);
        for (int j = 0; j < size; j++) {
            ptIds[j] = ptMap[ptIds[j]];
            edgeIds[j] = edgeMap[edgeIds[j]];
        }
        newFaces->AddCellIds(ptIds, size);
        newFaceEdges->AddCellIds(edgeIds, size);
        faceMap[i] = mappedFaceId;
        mappedFaceId++;
    }

    for (i = 0; i < nvolumes; i++) {
        if (IsVolumeDeleted(i)) continue;
        int pt_size = m_Volumes->GetCellIds(i, ptIds);
        int edge_size = m_VolumeEdges->GetCellIds(i, edgeIds);
        int face_size = m_VolumeFaces->GetCellIds(i, faceIds);
        for (int j = 0; j < pt_size; j++) { ptIds[j] = ptMap[ptIds[j]]; }
        for (int j = 0; j < edge_size; j++) {
            edgeIds[j] = edgeMap[edgeIds[j]];
        }
        for (int j = 0; j < face_size; j++) {
            faceIds[j] = faceMap[faceIds[j]];
        }
        newVolumes->AddCellIds(ptIds, pt_size);
        newVolumeEdges->AddCellIds(edgeIds, edge_size);
        newVolumeFaces->AddCellIds(faceIds, face_size);
    }

    m_Edges = newEdges;
    m_Faces = newFaces;
    m_FaceEdges = newFaceEdges;
    m_Volumes = newVolumes;
    m_VolumeEdges = newVolumeEdges;
    m_VolumeFaces = newVolumeFaces;

    m_EdgeLinks = nullptr;
    m_FaceLinks = nullptr;
    m_FaceEdgeLinks = nullptr;
    m_VolumeLinks = nullptr;
    m_VolumeEdgeLinks = nullptr;
    m_VolumeFaceLinks = nullptr;

    m_PointDeleteMarker = nullptr;
    m_EdgeDeleteMarker = nullptr;
    m_FaceDeleteMarker = nullptr;
    m_VolumeDeleteMarker = nullptr;
    Modified();
    MakeEditStatusOff();
}

bool VolumeMesh::IsVolumeDeleted(const IGsize volumeId) {
    return m_VolumeDeleteMarker->IsDeleted(volumeId);
}

IGsize VolumeMesh::AddPoint(const Point& p) {
    if (!InEditStatus()) { RequestEditStatus(); }
    IGsize ptId = m_Points->AddPoint(p);

    m_EdgeLinks->AddLink();
    m_FaceLinks->AddLink();
    m_VolumeLinks->AddLink();

    m_PointDeleteMarker->AddTag();
    return ptId;
}
IGsize VolumeMesh::AddEdge(const IGsize ptId1, const IGsize ptId2) {
    igIndex edgeId = GetEdgeIdFormPointIds(ptId1, ptId2);
    if (edgeId == -1) {
        edgeId = GetNumberOfEdges();
        m_Edges->AddCellId2(ptId1, ptId2);
        m_EdgeLinks->AddReference(ptId1, edgeId);
        m_EdgeLinks->AddReference(ptId2, edgeId);
        m_FaceEdgeLinks->AddLink();
        m_VolumeEdgeLinks->AddLink();
        m_EdgeDeleteMarker->AddTag();
    }
    return edgeId;
}
IGsize VolumeMesh::AddFace(igIndex* ptIds, int size) {
    igIndex edgeIds[64]{};
    for (int i = 0; i < size; i++) {
        edgeIds[i] = AddEdge(ptIds[i], ptIds[(i + 1) % size]);
    }
    igIndex faceId = GetFaceIdFormPointIds(ptIds, size);
    if (faceId == -1) {
        faceId = GetNumberOfFaces();
        m_Faces->AddCellIds(ptIds, size);
        m_FaceEdges->AddCellIds(edgeIds, size);
        for (int i = 0; i < size; i++) {
            m_FaceLinks->AddReference(ptIds[i], faceId);
            m_FaceEdgeLinks->AddReference(edgeIds[i], faceId);
        }
        m_VolumeFaceLinks->AddLink();
        m_FaceDeleteMarker->AddTag();
    }
    return faceId;
}

IGsize VolumeMesh::AddVolume(igIndex* ptIds, int size) {
    igIndex faceIds[64]{}, edgeIds[64]{}, face[64]{}, edge[64]{},
            isInsert[64]{};
    int faceNum{}, edgeNum{};
    if (size == Tetra::NumberOfPoints) {
        edgeNum = Tetra::NumberOfEdges;
        for (int i = 0; i < edgeNum; i++) {
            const igIndex* index;
            int size = Tetra::EdgePointIds(i, index);
            for (int k = 0; k < size; k++) { edge[k] = ptIds[index[k]]; }
            edgeIds[i] = AddEdge(edge[0], edge[1]);
        }
        faceNum = Tetra::NumberOfFaces;
        for (int i = 0; i < faceNum; i++) {
            const igIndex* index;
            int size = Tetra::FacePointIds(i, index);
            for (int k = 0; k < size; k++) { face[k] = ptIds[index[k]]; }
            faceIds[i] = AddFace(face, size);
        }
    }

    //std::cout << ptIds[0] << " " << ptIds[1] << " " << ptIds[2] << " " << ptIds[3] << "\n";
    igIndex volumeId = GetVolumeIdFormPointIds(ptIds, size);
    if (volumeId == -1) {
        volumeId = GetNumberOfVolumes();
        m_Volumes->AddCellIds(ptIds, size);
        m_VolumeEdges->AddCellIds(edgeIds, edgeNum);
        m_VolumeFaces->AddCellIds(faceIds, faceNum);
        for (int i = 0; i < size; i++) {
            m_VolumeLinks->AddReference(ptIds[i], volumeId);
        }
        for (int i = 0; i < edgeNum; i++) {
            m_VolumeEdgeLinks->AddReference(edgeIds[i], volumeId);
        }
        for (int i = 0; i < faceNum; i++) {
            m_VolumeFaceLinks->AddReference(faceIds[i], volumeId);
        }
        m_VolumeDeleteMarker->AddTag();
    }


    //if (size == Tetra::NumberOfPoints) {
    //	memset(isInsert, 0, size * 4);
    //	for (int j = 0; j < Tetra::NumberOfFaces; j++)
    //	{
    //		const igIndex* index;
    //		int size = Tetra::FacePointIds(j, index);
    //		for (int k = 0; k < size; k++) {
    //			face[k] = volumeIds[index[k]];
    //		}
    //		igIndex idx;
    //		if ((idx = GetFaceIdFormPointIds(face, size)) == -1) {
    //			idx = this->GetNumberOfFaces();
    //			m_Faces->AddCellIds(face, size);
    //			m_FaceDeleteMarker->AddTag();
    //			m_VolumeFaceLinks->AddLink();
    //			for (int i = 0; i < size; i++) {
    //				m_FaceLinks->AddReference(face[i], idx);
    //			}
    //			isInsert[j] = idx;
    //		}
    //		faceIds[j] = idx;
    //	}
    //	m_VolumeFaces->AddCellIds(faceIds, Tetra::NumberOfFaces);
    //	for (int i = 0; i < Tetra::NumberOfFaces; i++) {
    //		m_VolumeFaceLinks->AddReference(faceIds[i], volumeId);
    //	}

    //	for (int j = 0; j < Tetra::NumberOfEdges; j++)
    //	{
    //		const igIndex* index;
    //		int size = Tetra::EdgePointIds(j, index);
    //		for (int k = 0; k < 2; k++) {
    //			edge[k] = volumeIds[index[k]];
    //		}
    //		igIndex idx;
    //		if ((idx = GetEdgeIdFormPointIds(edge[0], edge[1])) == -1) {
    //			idx = this->GetNumberOfEdges();
    //			m_Edges->AddCellIds(edge, size);
    //			m_EdgeDeleteMarker->AddTag();
    //			m_FaceEdgeLinks->AddLink();
    //			m_VolumeEdgeLinks->AddLink();
    //			for (int i = 0; i < 2; i++) {
    //				m_EdgeLinks->AddReference(edge[i], idx);
    //			}
    //		}
    //		edgeIds[j] = idx;
    //	}
    //	m_VolumeEdges->AddCellIds(edgeIds, Tetra::NumberOfEdges);
    //	for (int i = 0; i < Tetra::NumberOfEdges; i++) {
    //		m_VolumeEdgeLinks->AddReference(edgeIds[i], volumeId);
    //	}

    //	for (int j = 0; j < Tetra::NumberOfFaces; j++)
    //	{
    //		if (isInsert[j]) {
    //			const igIndex* index;
    //			int size = Tetra::FaceEdgeIds(j, index);
    //			for (int k = 0; k < size; k++) {
    //				face[k] = edgeIds[index[k]];
    //			}
    //			m_FaceEdges->AddCellIds(face, size);
    //			for (int i = 0; i < size; i++) {
    //				m_FaceEdgeLinks->AddReference(face[i], isInsert[j]);
    //			}
    //		}
    //	}
    //}
    return volumeId;
}

bool VolumeMesh::ReplaceEdgeReference(const IGsize edgeId, igIndex ptId1,
                                      igIndex ptId2) {
    assert(IsEdgeDeleted(edgeId));
    //assert(GetEdgeIdFormPointIds(ptId1, ptId2) == -1);
    igIndex e[2]{ptId1, ptId2};
    m_Edges->SetCellIds(edgeId, e, 2);
    m_EdgeDeleteMarker->Undo(edgeId);
    m_EdgeLinks->AddReference(ptId1, edgeId);
    m_EdgeLinks->AddReference(ptId2, edgeId);
    m_FaceEdgeLinks->DeleteLink(edgeId);
    m_VolumeEdgeLinks->DeleteLink(edgeId);
    return true;
}

bool VolumeMesh::ReplaceFaceReference(const IGsize faceId, igIndex* ptIds,
                                      int npts, igIndex* edgeIds) {
    assert(IsFaceDeleted(faceId));
    //assert(GetFaceIdFormPointIds(ptIds, npts) == -1);
    m_Faces->SetCellIds(faceId, ptIds, npts);
    m_FaceEdges->SetCellIds(faceId, edgeIds, npts);
    m_FaceDeleteMarker->Undo(faceId);
    for (int i = 0; i < npts; i++) {
        m_FaceLinks->AddReference(ptIds[i], faceId);
        m_FaceEdgeLinks->AddReference(edgeIds[i], faceId);
    }
    m_VolumeFaceLinks->DeleteLink(faceId);
    return true;
}

bool VolumeMesh::ReplaceVolumeReference(const IGsize volumeId, igIndex* ptIds,
                                        int npts, igIndex* edgeIds, int nedges,
                                        igIndex* faceIds, int nfaces) {
    assert(IsVolumeDeleted(volumeId));
    //assert(GetVolumeIdFormPointIds(ptIds, npts) == -1);
    m_Volumes->SetCellIds(volumeId, ptIds, npts);
    m_VolumeEdges->SetCellIds(volumeId, edgeIds, nedges);
    m_VolumeFaces->SetCellIds(volumeId, faceIds, nfaces);
    m_VolumeDeleteMarker->Undo(volumeId);
    for (int i = 0; i < npts; i++) {
        m_VolumeLinks->AddReference(ptIds[i], volumeId);
    }
    for (int i = 0; i < nedges; i++) {
        m_VolumeEdgeLinks->AddReference(edgeIds[i], volumeId);
    }
    for (int i = 0; i < nfaces; i++) {
        m_VolumeFaceLinks->AddReference(faceIds[i], volumeId);
    }
    return true;
}

void VolumeMesh::DeletePoint(const IGsize ptId) {
    if (!InEditStatus()) { RequestEditStatus(); }
    if (IsPointDeleted(ptId)) { return; }
    ReturnContainer edgeIds;
    GetPointToNeighborEdges(ptId, edgeIds);
    for (int i = 0; i < edgeIds.size(); i++) { DeleteEdge(edgeIds[i]); }
    m_EdgeLinks->DeleteLink(ptId);
    m_FaceLinks->DeleteLink(ptId);
    m_VolumeLinks->DeleteLink(ptId);
    m_PointDeleteMarker->MarkDeleted(ptId);
}

void VolumeMesh::DeleteEdge(const IGsize edgeId) {
    if (!InEditStatus()) { RequestEditStatus(); }
    if (IsEdgeDeleted(edgeId)) { return; }
    igIndex e[2]{};

    GetEdgePointIds(edgeId, e);
    for (int i = 0; i < 2; i++) { m_EdgeLinks->RemoveReference(e[i], edgeId); }

    ReturnContainer faceIds;
    GetEdgeToNeighborFaces(edgeId, faceIds);
    for (int i = 0; i < faceIds.size(); i++) { DeleteFace(faceIds[i]); }
    m_FaceEdgeLinks->DeleteLink(edgeId);
    m_VolumeEdgeLinks->DeleteLink(edgeId);
    m_EdgeDeleteMarker->MarkDeleted(edgeId);
}
void VolumeMesh::DeleteFace(const IGsize faceId) {
    if (!InEditStatus()) { RequestEditStatus(); }
    if (IsFaceDeleted(faceId)) { return; }
    igIndex ptIds[32]{}, edgeIds[32]{};
    int size = GetFacePointIds(faceId, ptIds);
    GetFaceEdgeIds(faceId, edgeIds);
    for (int i = 0; i < size; i++) {
        m_FaceLinks->RemoveReference(ptIds[i], faceId);
        m_FaceEdgeLinks->RemoveReference(edgeIds[i], faceId);
    }

    ReturnContainer volumeIds;
    GetFaceToNeighborVolumes(faceId, volumeIds);
    for (int i = 0; i < volumeIds.size(); i++) { DeleteVolume(volumeIds[i]); }
    m_VolumeFaceLinks->DeleteLink(faceId);
    m_FaceDeleteMarker->MarkDeleted(faceId);
}

void VolumeMesh::DeleteVolume(const IGsize volumeId) {
    if (!InEditStatus()) { RequestEditStatus(); }
    if (IsVolumeDeleted(volumeId)) { return; }
    igIndex ptIds[32]{}, edgeIds[32]{}, faceIds[32]{};
    int pt_size = GetVolumePointIds(volumeId, ptIds);
    int edge_size = GetVolumeEdgeIds(volumeId, edgeIds);
    int face_size = GetVolumeFaceIds(volumeId, faceIds);
    //std::cout << "volume: " << volumeId/* << ptIds[0] << " " << ptIds[1] << " " << ptIds[2] << " " << ptIds[3]*/ << std::endl;
    static int count = 0;
    static int count2 = 0;

    int count1 = 0;
    int ids[1]{1};
    for (int i = 0; i < 1; i++) {
        for (int j = 0; j < 4; j++) {
            if (ptIds[j] == ids[i]) {
                count1++;
                break;
            }
        }
    }
    //std::cout << count++ << std::endl;
    //std::cout << count++ << ": " << count1 << " " << ptIds[0] << " " << ptIds[1] << " " << ptIds[2] << " " << ptIds[3] << std::endl;
    //if (count1 == 1) {
    //	std::cout << ++count2 << std::endl;
    //}

    for (int i = 0; i < pt_size; i++) {
        m_VolumeLinks->RemoveReference(ptIds[i], volumeId);
    }
    for (int i = 0; i < edge_size; i++) {
        m_VolumeEdgeLinks->RemoveReference(edgeIds[i], volumeId);
    }
    for (int i = 0; i < face_size; i++) {
        m_VolumeFaceLinks->RemoveReference(faceIds[i], volumeId);
    }
    m_VolumeDeleteMarker->MarkDeleted(volumeId);
}


bool VolumeMesh::IsBoundaryPoint(const IGsize ptId) {
    const igIndex* ids;
    int size;
    GetPointToNeighborFaces(ptId, ids, size);
    for (int i = 0; i < size; i++) {
        if (IsBoundaryFace(ids[i])) { return true; }
    }
    return false;
}
bool VolumeMesh::IsBoundaryEdge(const IGsize edgeId) {
    const igIndex* ids;
    int size;
    GetEdgeToNeighborFaces(edgeId, ids, size);
    for (int i = 0; i < size; i++) {
        if (IsBoundaryFace(ids[i])) { return true; }
    }
    return false;
}
bool VolumeMesh::IsBoundaryFace(const IGsize faceId) {
    int size = GetNumberOfLinks(faceId, F2V);
    return size == 1;
}
bool VolumeMesh::IsBoundaryVolume(const IGsize volumeId) {
    igIndex faceIds[32];
    int ncell = GetVolumeFaceIds(volumeId, faceIds);
    for (int i = 0; i < ncell; i++) {
        if (this->IsBoundaryFace(faceIds[i])) { return true; }
    }
    return false;
}
bool VolumeMesh::IsCornerPoint(const IGsize ptId) {
    int size = GetNumberOfLinks(ptId, P2V);
    return size == 1;
}
VolumeMesh::VolumeMesh() { m_ViewStyle = IG_SURFACE; };

IGsize VolumeMesh::GetRealMemorySize() {
    IGsize res = this->SurfaceMesh::GetRealMemorySize();
    if (m_Volumes) res += m_Volumes->GetRealMemorySize();
    if (m_VolumeEdges) res += m_VolumeEdges->GetRealMemorySize();
    if (m_VolumeFaces) res += m_VolumeFaces->GetRealMemorySize();
    if (m_VolumeDeleteMarker) res += m_VolumeDeleteMarker->GetRealMemorySize();
    return res + sizeof(IsPolyhedronType);
}
void VolumeMesh::RequestFaceStatus() {
    if (m_Faces == nullptr || (m_Faces->GetMTime() < m_Volumes->GetMTime())) {
        BuildFacesAndEdges();
    }

    if (m_FaceEdgeLinks == nullptr ||
        (m_FaceEdgeLinks->GetMTime() < m_FaceEdges->GetMTime())) {
        BuildFaceEdgeLinks();
    }

    if (m_FaceLinks == nullptr ||
        (m_FaceLinks->GetMTime() < m_Faces->GetMTime())) {
        BuildFaceLinks();
    }

    if (m_EdgeLinks == nullptr ||
        (m_EdgeLinks->GetMTime() < m_Edges->GetMTime())) {
        BuildEdgeLinks();
    }

    if (m_EdgeDeleteMarker == nullptr) {
        m_EdgeDeleteMarker = DeleteMarker::New();
    }
    m_EdgeDeleteMarker->Initialize(GetNumberOfEdges());

    if (m_FaceDeleteMarker == nullptr) {
        m_FaceDeleteMarker = DeleteMarker::New();
    }
    m_FaceDeleteMarker->Initialize(GetNumberOfFaces());
}

void VolumeMesh::RequestVolumeStatus() {
    if (m_VolumeLinks == nullptr ||
        (m_VolumeLinks->GetMTime() < m_Volumes->GetMTime())) {
        BuildVolumeLinks();
    }

    if (m_VolumeEdgeLinks == nullptr ||
        (m_VolumeEdgeLinks->GetMTime() < m_VolumeEdges->GetMTime())) {
        BuildVolumeEdgeLinks();
    }

    if (m_VolumeFaceLinks == nullptr ||
        (m_VolumeFaceLinks->GetMTime() < m_VolumeFaces->GetMTime())) {
        BuildVolumeFaceLinks();
    }

    if (m_VolumeDeleteMarker == nullptr) {
        m_VolumeDeleteMarker = DeleteMarker::New();
    }
    m_VolumeDeleteMarker->Initialize(GetNumberOfEdges());
}

//void VolumeMesh::Draw(Scene* scene) {
//    if (!m_Visibility) { return; }
//    if (m_DrawMesh) {
//        m_DrawMesh->SetViewStyle(m_ViewStyle);
//        return m_DrawMesh->Draw(scene);
//    }
//    //update uniform buffer
//    if (m_UseColor) {
//        scene->UBO().useColor = true;
//    } else {
//        scene->UBO().useColor = false;
//    }
//    scene->UpdateUniformBuffer();
//
//    if (m_UseColor && m_ColorWithCell) {
//        scene->GetShader(Scene::BLINNPHONG)->use();
//
//        m_CellVAO.bind();
//        glad_glDrawArrays(GL_TRIANGLES, 0, m_CellPositionSize);
//        m_CellVAO.release();
//        return;
//    }
//
//    if (m_ViewStyle & IG_POINTS) {
//        scene->GetShader(Scene::NOLIGHT)->use();
//
//        m_PointVAO.bind();
//        glad_glPointSize(m_PointSize);
//        glad_glDepthRange(0.000001, 1);
//        glad_glDrawArrays(GL_POINTS, 0, m_Positions->GetNumberOfValues() / 3);
//        glad_glDepthRange(0, 1);
//        m_PointVAO.release();
//    }
//    if (m_ViewStyle & IG_WIREFRAME) {
//        if (m_UseColor) {
//            scene->GetShader(Scene::NOLIGHT)->use();
//        } else {
//            auto shader = scene->GetShader(Scene::PURECOLOR);
//            shader->use();
//            shader->setUniform(shader->getUniformLocation("inputColor"),
//                               igm::vec3{0.0f, 0.0f, 0.0f});
//        }
//
//        m_LineVAO.bind();
//        glLineWidth(m_LineWidth);
//        glad_glDrawElements(GL_LINES, m_LineIndices->GetNumberOfIds(),
//                            GL_UNSIGNED_INT, 0);
//        m_LineVAO.release();
//    }
//    if (m_ViewStyle & IG_SURFACE) {
//        scene->GetShader(Scene::BLINNPHONG)->use();
//
//        m_TriangleVAO.bind();
//        glEnable(GL_POLYGON_OFFSET_FILL);
//        glPolygonOffset(-0.5f, -0.5f);
//        glad_glDrawElements(GL_TRIANGLES, m_TriangleIndices->GetNumberOfIds(),
//                            GL_UNSIGNED_INT, 0);
//        glDisable(GL_POLYGON_OFFSET_FILL);
//        m_TriangleVAO.release();
//    }
//
//    /*if (m_ViewStyle == IG_SURFACE_WITH_EDGE)
//	{
//		if (m_UseColor)
//		{
//			scene->GetShader(Scene::NOLIGHT)->use();
//		}
//		else
//		{
//			auto shader = scene->GetShader(Scene::PURECOLOR);
//			shader->use();
//			shader->setUniform(shader->getUniformLocation("inputColor"), igm::vec3{ 0.0f, 0.0f, 0.0f });
//		}
//
//		m_LineVAO.bind();
//		glLineWidth(m_LineWidth);
//		glad_glDrawElements(GL_LINES, m_LineIndices->GetNumberOfIds(), GL_UNSIGNED_INT, 0);
//		m_LineVAO.release();
//
//		scene->GetShader(Scene::PATCH)->use();
//		m_TriangleVAO.bind();
//		glad_glDrawElements(GL_TRIANGLES, m_TriangleIndices->GetNumberOfIds(), GL_UNSIGNED_INT, 0);
//		m_TriangleVAO.release();
//	}*/
//}

void VolumeMesh::ConvertToDrawableData() {
    this->Create();
    if (m_Positions && m_Positions->GetMTime() > this->GetMTime()) { return; }

    //if (m_DrawMesh == nullptr || m_DrawMesh->GetMTime() < this->GetMTime()) {
    //    iGameModelGeometryFilter::Pointer extract =
    //            iGameModelGeometryFilter::New();
    //    // update clip status
    //    if (m_Clip.m_Extent.m_Use) {
    //        const auto& a = m_Clip.m_Extent.m_bmin;
    //        const auto& b = m_Clip.m_Extent.m_bmax;
    //        extract->SetExtent(a[0], b[0], a[1], b[1], a[2], b[2],
    //                           m_Clip.m_Extent.m_flip);
    //    }
    //    if (m_Clip.m_Plane.m_Use) {
    //        extract->SetClipPlane(m_Clip.m_Plane.m_origin,
    //                              m_Clip.m_Plane.m_normal,
    //                              m_Clip.m_Plane.m_flip);
    //    }
    //
    //    m_DrawMesh = SurfaceMesh::New();
    //    if (!extract->Execute(this, m_DrawMesh)) { m_DrawMesh = nullptr; }
    //    if (m_DrawMesh) { m_DrawMesh->Modified(); }
    //}
    //if (m_DrawMesh) { return m_DrawMesh->ConvertToDrawableData(); }

    m_Positions = m_Points->ConvertToArray();
    m_Positions->Modified();

    IdArray::Pointer triangleIndices = IdArray::New();
    IdArray::Pointer edgeIndices = IdArray::New();
    if (!this->IsPolyhedronType) {
        for (int i = 0; i < this->GetNumberOfVolumes(); i++) {
            Volume* volume = this->GetVolume(i);
            const igIndex* face;
            for (int j = 0; j < volume->GetNumberOfFaces(); j++) {
                int size = volume->GetFacePointIds(j, face);
                for (int k = 2; k < size; k++) {
                    triangleIndices->AddId(volume->PointIds->GetId(face[0]));
                    triangleIndices->AddId(
                            volume->PointIds->GetId(face[k - 1]));
                    triangleIndices->AddId(volume->PointIds->GetId(face[k]));
                }
            }
            const igIndex* edge;
            for (int j = 0; j < volume->GetNumberOfEdges(); j++) {
                int size = volume->GetEdgePointIds(j, edge);
                edgeIndices->AddId(volume->PointIds->GetId(edge[0]));
                edgeIndices->AddId(volume->PointIds->GetId(edge[1]));
            }
        }
    } else {
        igIndex fcnt = 0;
        igIndex fhs[IGAME_CELL_MAX_SIZE];
        igIndex face[IGAME_CELL_MAX_SIZE];
        for (int i = 0; i < this->GetNumberOfVolumes(); i++) {
            fcnt = this->m_VolumeFaces->GetCellIds(i, fhs);
            for (int j = 0; j < fcnt; j++) {
                int size = this->m_Faces->GetCellIds(fhs[j], face);
                for (int k = 2; k < size; k++) {
                    triangleIndices->AddId(face[0]);
                    triangleIndices->AddId(face[k - 1]);
                    triangleIndices->AddId(face[k]);
                }
            }
        }
    }

    m_TriangleIndices = triangleIndices;
    m_LineIndices = edgeIndices;

    // allocate buffer
    {
        GLAllocateGLBuffer(m_PositionVBO,
                           m_Positions->GetNumberOfValues() * sizeof(float),
                           m_Positions->RawPointer());

        GLAllocateGLBuffer(m_LineEBO,
                           m_LineIndices->GetNumberOfIds() * sizeof(igIndex),
                           m_LineIndices->RawPointer());

        GLAllocateGLBuffer(m_TriangleEBO,
                           m_TriangleIndices->GetNumberOfIds() *
                                   sizeof(igIndex),
                           m_TriangleIndices->RawPointer());
    }

    // set vertex attribute pointer
    {
        // point
        m_PointVAO.vertexBuffer(GL_VBO_IDX_0, m_PositionVBO, 0,
                                3 * sizeof(float));
        GLSetVertexAttrib(m_PointVAO, GL_LOCATION_IDX_0, GL_VBO_IDX_0, 3,
                          GL_FLOAT, GL_FALSE, 0);

        // line
        m_LineVAO.vertexBuffer(GL_VBO_IDX_0, m_PositionVBO, 0,
                               3 * sizeof(float));
        GLSetVertexAttrib(m_LineVAO, GL_LOCATION_IDX_0, GL_VBO_IDX_0, 3,
                          GL_FLOAT, GL_FALSE, 0);
        m_LineVAO.elementBuffer(m_LineEBO);

        // triangle
        m_TriangleVAO.vertexBuffer(GL_VBO_IDX_0, m_PositionVBO, 0,
                                   3 * sizeof(float));
        GLSetVertexAttrib(m_TriangleVAO, GL_LOCATION_IDX_0, GL_VBO_IDX_0, 3,
                          GL_FLOAT, GL_FALSE, 0);
        m_TriangleVAO.elementBuffer(m_TriangleEBO);
    }
}

void VolumeMesh::ViewCloudPicture(Scene* scene, int index, int demension) {
    if (m_DrawMesh) {
        return m_DrawMesh->ViewCloudPicture(scene, index, demension);
    }
    if (index == -1) {
        m_UseColor = false;
        m_ViewAttribute = nullptr;
        m_ViewDemension = -1;
        m_ColorWithCell = false;
        scene->Update();
        return;
    }
    scene->MakeCurrent();
    auto& attr = this->GetAttributeSet()->GetAttribute(index);
    if (!attr.isDeleted) {
        if (attr.attachmentType == IG_POINT)
            this->SetAttributeWithPointData(attr.pointer, attr.dataRange,
                                            demension);
        else if (attr.attachmentType == IG_CELL)
            this->SetAttributeWithCellData(attr.pointer, demension);
    }
    scene->DoneCurrent();
    scene->Update();
}

void VolumeMesh::SetAttributeWithPointData(ArrayObject::Pointer attr,
                                           std::pair<float, float>& range,
                                           igIndex dimension) {
    if (m_ViewAttribute != attr || m_ViewDemension != dimension) {
        m_ViewAttribute = attr;
        m_ViewDemension = dimension;
        m_UseColor = true;
        m_ColorWithCell = false;

        if (range.first != range.second) {
            m_ColorMapper->SetRange(range.first, range.second);
        } else if (dimension == -1) {
            m_ColorMapper->InitRange(attr);
        } else {
            m_ColorMapper->InitRange(attr, dimension);
        }
        range.first = m_ColorMapper->GetRange()[0];
        range.second = m_ColorMapper->GetRange()[1];
        m_Colors = m_ColorMapper->MapScalars(attr, dimension);
        if (m_Colors == nullptr) { return; }

        GLAllocateGLBuffer(m_ColorVBO,
                           m_Colors->GetNumberOfValues() * sizeof(float),
                           m_Colors->RawPointer());

        m_PointVAO.vertexBuffer(GL_VBO_IDX_1, m_ColorVBO, 0, 3 * sizeof(float));
        GLSetVertexAttrib(m_PointVAO, GL_LOCATION_IDX_1, GL_VBO_IDX_1, 3,
                          GL_FLOAT, GL_FALSE, 0);

        m_LineVAO.vertexBuffer(GL_VBO_IDX_1, m_ColorVBO, 0, 3 * sizeof(float));
        GLSetVertexAttrib(m_LineVAO, GL_LOCATION_IDX_1, GL_VBO_IDX_1, 3,
                          GL_FLOAT, GL_FALSE, 0);


        m_TriangleVAO.vertexBuffer(GL_VBO_IDX_1, m_ColorVBO, 0,
                                   3 * sizeof(float));
        GLSetVertexAttrib(m_TriangleVAO, GL_LOCATION_IDX_1, GL_VBO_IDX_1, 3,
                          GL_FLOAT, GL_FALSE, 0);
    }
}

void VolumeMesh::SetAttributeWithCellData(ArrayObject::Pointer attr,
                                          igIndex i) {
    if (m_ViewAttribute != attr || m_ViewDemension != i) {
        m_ViewAttribute = attr;
        m_ViewDemension = i;
        m_UseColor = true;
        m_ColorWithCell = true;


        if (i == -1) {
            m_ColorMapper->InitRange(attr);
        } else {
            m_ColorMapper->InitRange(attr, i);
        }

        FloatArray::Pointer colors = m_ColorMapper->MapScalars(attr, i);
        if (colors == nullptr) { return; }

        FloatArray::Pointer newPositions = FloatArray::New();
        FloatArray::Pointer newColors = FloatArray::New();
        newPositions->SetDimension(3);
        newColors->SetDimension(3);

        float color[3]{};
        for (int i = 0; i < this->GetNumberOfVolumes(); i++) {
            Volume* volume = this->GetVolume(i);
            const igIndex* face;
            colors->GetElement(i, color);
            for (int j = 0; j < volume->GetNumberOfFaces(); j++) {
                int size = volume->GetFacePointIds(j, face);
                for (int k = 2; k < size; k++) {
                    auto& p0 = volume->Points->GetPoint(face[0]);
                    newPositions->AddElement3(p0[0], p0[1], p0[2]);
                    auto& p1 = volume->Points->GetPoint(face[k - 1]);
                    newPositions->AddElement3(p1[0], p1[1], p1[2]);
                    auto& p2 = volume->Points->GetPoint(face[k]);
                    newPositions->AddElement3(p2[0], p2[1], p2[2]);

                    newColors->AddElement3(color[0], color[1], color[2]);
                    newColors->AddElement3(color[0], color[1], color[2]);
                    newColors->AddElement3(color[0], color[1], color[2]);
                }
            }
        }

        m_CellPositionSize = newPositions->GetNumberOfElements();
        GLAllocateGLBuffer(m_CellPositionVBO,
                           newPositions->GetNumberOfValues() * sizeof(float),
                           newPositions->RawPointer());
        GLAllocateGLBuffer(m_CellColorVBO,
                           newColors->GetNumberOfValues() * sizeof(float),
                           newColors->RawPointer());

        m_CellVAO.vertexBuffer(GL_VBO_IDX_0, m_CellPositionVBO, 0,
                               3 * sizeof(float));
        GLSetVertexAttrib(m_CellVAO, GL_LOCATION_IDX_0, GL_VBO_IDX_0, 3,
                          GL_FLOAT, GL_FALSE, 0);
        m_CellVAO.vertexBuffer(GL_VBO_IDX_1, m_CellColorVBO, 0,
                               3 * sizeof(float));
        GLSetVertexAttrib(m_CellVAO, GL_LOCATION_IDX_1, GL_VBO_IDX_1, 3,
                          GL_FLOAT, GL_FALSE, 0);
    }
}
IGAME_NAMESPACE_END