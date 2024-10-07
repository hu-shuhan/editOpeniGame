#include "iGameSurfaceMesh.h"
#include "iGameModelSurfaceFilters/iGameModelGeometryFilter.h"
#include "iGameScene.h"
IGAME_NAMESPACE_BEGIN

IGsize SurfaceMesh::GetNumberOfEdges() const noexcept {
    return m_Edges ? m_Edges->GetNumberOfCells() : 0;
}
IGsize SurfaceMesh::GetNumberOfFaces() const noexcept {
    return m_Faces ? m_Faces->GetNumberOfCells() : 0;
}

CellArray* SurfaceMesh::GetEdges() { return m_Edges ? m_Edges.get() : nullptr; }
CellArray* SurfaceMesh::GetFaces() { return m_Faces ? m_Faces.get() : nullptr; }

void SurfaceMesh::SetFaces(CellArray::Pointer faces) {
    if (m_Faces != faces) {
        m_Faces = faces;
        this->Modified();
    }
}

Line* SurfaceMesh::GetEdge(const IGsize edgeId) {
    const igIndex* cell;
    int ncells = m_Edges->GetCellIds(edgeId, cell);

    if (m_Edge == nullptr) { m_Edge = Line::New(); }

    m_Edge->PointIds->Reset();
    m_Edge->PointIds->AddId(cell[0]);
    m_Edge->PointIds->AddId(cell[1]);
    m_Edge->Points->Reset();
    m_Edge->Points->AddPoint(this->GetPoint(cell[0]));
    m_Edge->Points->AddPoint(this->GetPoint(cell[1]));
    return m_Edge.get();
}

Face* SurfaceMesh::GetFace(const IGsize faceId) {
    const igIndex* cell;
    int ncells = m_Faces->GetCellIds(faceId, cell);

    Face* face = nullptr;
    if (ncells == 3) {
        if (m_Triangle == nullptr) { m_Triangle = Triangle::New(); }
        face = m_Triangle.get();
        assert(ncells == 3);
    } else if (ncells == 4) {
        if (!m_Quad) { m_Quad = Quad::New(); }
        face = m_Quad.get();
        assert(ncells == 4);
    } else {
        if (!m_Polygon) { m_Polygon = Polygon::New(); }
        face = m_Polygon.get();
        assert(ncells > 4);
    }

    face->PointIds->Reset();
    face->Points->Reset();

    for (int i = 0; i < ncells; i++) {
        face->PointIds->AddId(cell[i]);
        face->Points->AddPoint(this->GetPoint(cell[i]));
    }

    return face;
}

int SurfaceMesh::GetEdgePointIds(const IGsize edgeId, igIndex* ptIds) {
    if (m_Edges == nullptr) { this->BuildEdges(); }
    m_Edges->GetCellIds(edgeId, ptIds);
    return 2;
}

int SurfaceMesh::GetFacePointIds(const IGsize faceId, igIndex* ptIds) {
    return m_Faces->GetCellIds(faceId, ptIds);
}

int SurfaceMesh::GetFaceEdgeIds(const IGsize faceId, igIndex* edgeIds) {
    return m_FaceEdges->GetCellIds(faceId, edgeIds);
}

void SurfaceMesh::BuildEdges() {
    EdgeTable::Pointer EdgeTable = EdgeTable::New();
    IGsize nfaces = GetNumberOfFaces();
    igIndex edgeIds[32]{}, face[32]{};

    m_FaceEdges = CellArray::New();
    EdgeTable->Initialize(GetNumberOfPoints());

    for (IGsize i = 0; i < nfaces; i++) {
        int size = this->GetFacePointIds(i, face);
        for (int j = 0; j < size; j++) {
            igIndex idx;
            if ((idx = EdgeTable->IsEdge(face[j], face[(j + 1) % size])) ==
                -1) {
                idx = EdgeTable->GetNumberOfEdges();
                EdgeTable->InsertEdge(face[j], face[(j + 1) % size]);
            }
            edgeIds[j] = idx;
        }
        m_FaceEdges->AddCellIds(edgeIds, size);
    }
    m_Edges = EdgeTable->GetOutput();
}

void SurfaceMesh::BuildEdgeLinks() {
    if (m_EdgeLinks && m_EdgeLinks->GetMTime() > m_Edge->GetMTime()) { return; }

    m_EdgeLinks = CellLinks::New();
    IGsize npts = GetNumberOfPoints();
    IGsize nedges = GetNumberOfEdges();
    igIndex e[2]{};

    m_EdgeLinks->Allocate(npts);
    for (int i = 0; i < nedges; i++) {
        int size = m_Edges->GetCellIds(i, e);
        m_EdgeLinks->IncrementLinkSize(e[0]);
        m_EdgeLinks->IncrementLinkSize(e[1]);
    }

    m_EdgeLinks->AllocateLinks(npts);

    for (int i = 0; i < nedges; i++) {
        int size = m_Edges->GetCellIds(i, e);
        m_EdgeLinks->AddReference(e[0], i);
        m_EdgeLinks->AddReference(e[1], i);
    }
    m_EdgeLinks->Modified();
}

void SurfaceMesh::BuildFaceLinks() {
    if (m_FaceLinks && m_FaceLinks->GetMTime() > m_Faces->GetMTime()) {
        return;
    }

    m_FaceLinks = CellLinks::New();
    IGsize npts = GetNumberOfPoints();
    IGsize nfaces = GetNumberOfFaces();
    igIndex face[32]{};

    m_FaceLinks->Allocate(npts);
    for (int i = 0; i < nfaces; i++) {
        int size = m_Faces->GetCellIds(i, face);
        for (int j = 0; j < size; j++) {
            m_FaceLinks->IncrementLinkSize(face[j]);
        }
    }

    m_FaceLinks->AllocateLinks(npts);
    for (int i = 0; i < nfaces; i++) {
        int size = m_Faces->GetCellIds(i, face);
        for (int j = 0; j < size; j++) {
            m_FaceLinks->AddReference(face[j], i);
        }
    }
    m_FaceLinks->Modified();
}

void SurfaceMesh::BuildFaceEdgeLinks() {
    if (m_FaceEdgeLinks &&
        m_FaceEdgeLinks->GetMTime() > m_FaceEdges->GetMTime()) {
        return;
    }

    m_FaceEdgeLinks = CellLinks::New();
    IGsize nedges = GetNumberOfEdges();
    IGsize nfaces = GetNumberOfFaces();
    igIndex face[32]{};

    m_FaceEdgeLinks->Allocate(nedges);
    for (int i = 0; i < nfaces; i++) {
        int size = m_FaceEdges->GetCellIds(i, face);
        for (int j = 0; j < size; j++) {
            m_FaceEdgeLinks->IncrementLinkSize(face[j]);
        }
    }

    m_FaceEdgeLinks->AllocateLinks(nedges);
    for (int i = 0; i < nfaces; i++) {
        int size = m_FaceEdges->GetCellIds(i, face);
        for (int j = 0; j < size; j++) {
            m_FaceEdgeLinks->AddReference(face[j], i);
        }
    }
}

int SurfaceMesh::GetNumberOfLinks(const IGsize id, Type type) {
    int size = 0;
    switch (type) {
        case iGame::SurfaceMesh::P2P:
        case iGame::SurfaceMesh::P2E:
            size = m_EdgeLinks->GetLinkSize(id);
            break;
        case iGame::SurfaceMesh::P2F:
            size = m_FaceLinks->GetLinkSize(id);
            break;
        case iGame::SurfaceMesh::E2F:
            size = m_FaceEdgeLinks->GetLinkSize(id);
            break;
        default:
            break;
    }
    return size;
}
int SurfaceMesh::GetPointToOneRingPoints(const IGsize ptId, igIndex* ptIds) {
    assert(ptId < GetNumberOfPoints() && "ptId too large");
    auto& link = m_EdgeLinks->GetLink(ptId);
    igIndex e[2]{};
    for (int i = 0; i < link.size; i++) {
        GetEdgePointIds(link.pointer[i], e);
        ptIds[i] = e[0] - ptId + e[1];
        assert(e[0] == ptId || e[1] == ptId);
    }
    return link.size;
}
bool SurfaceMesh::GetPointToOneRingPoints(const IGsize ptId,
                                          IdArray::Pointer ptIds) {
    assert(ptId < GetNumberOfPoints() && "ptId too large");
    ptIds->Reset();
    auto& link = m_EdgeLinks->GetLink(ptId);
    igIndex e[2]{};
    for (int i = 0; i < link.size; i++) {
        GetEdgePointIds(link.pointer[i], e);
        ptIds->AddId(e[0] - ptId + e[1]);
        assert(e[0] == ptId || e[1] == ptId);
    }
    return link.size;
}
int SurfaceMesh::GetPointToNeighborEdges(const IGsize ptId, igIndex* edgeIds) {
    assert(ptId < GetNumberOfPoints() && "ptId too large");
    auto& link = m_EdgeLinks->GetLink(ptId);
    for (int i = 0; i < link.size; i++) { edgeIds[i] = link.pointer[i]; }
    return link.size;
}
bool SurfaceMesh::GetPointToNeighborEdges(const IGsize ptId,
                                          const igIndex*& edgeIds, int& size) {
    assert(ptId < GetNumberOfPoints() && "ptId too large");
    edgeIds = m_EdgeLinks->GetLinkPointer(ptId);
    size = m_EdgeLinks->GetLinkSize(ptId);
    return true;
}
bool SurfaceMesh::GetPointToNeighborEdges(const IGsize ptId,
                                          IdArray::Pointer edgeIds) {
    assert(ptId < GetNumberOfPoints() && "ptId too large");
    const igIndex* pt = m_EdgeLinks->GetLinkPointer(ptId);
    int size = m_EdgeLinks->GetLinkSize(ptId);
    edgeIds->Reset();
    for (int i = 0; i < size; i++) { edgeIds->AddId(pt[i]); }
    return true;
}
bool SurfaceMesh::GetPointToNeighborEdges(const IGsize ptId,
                                          ReturnContainer& edgeIds) {
    const igIndex* pt = m_EdgeLinks->GetLinkPointer(ptId);
    int size = m_EdgeLinks->GetLinkSize(ptId);
    edgeIds.reset();
    for (int i = 0; i < size; i++) { edgeIds.push_back(pt[i]); }
    return true;
}
int SurfaceMesh::GetPointToNeighborFaces(const IGsize ptId, igIndex* faceIds) {
    assert(ptId < GetNumberOfPoints() && "ptId too large");
    auto& link = m_FaceLinks->GetLink(ptId);
    for (int i = 0; i < link.size; i++) { faceIds[i] = link.pointer[i]; }
    return link.size;
}
bool SurfaceMesh::GetPointToNeighborFaces(const IGsize ptId,
                                          const igIndex*& faceIds, int& size) {
    assert(ptId < GetNumberOfPoints() && "ptId too large");
    faceIds = m_FaceLinks->GetLinkPointer(ptId);
    size = m_FaceLinks->GetLinkSize(ptId);
    return true;
}
bool SurfaceMesh::GetPointToNeighborFaces(const IGsize ptId,
                                          IdArray::Pointer faceIds) {
    assert(ptId < GetNumberOfPoints() && "ptId too large");
    const igIndex* pt = m_FaceLinks->GetLinkPointer(ptId);
    int size = m_FaceLinks->GetLinkSize(ptId);
    faceIds->Reset();
    for (int i = 0; i < size; i++) { faceIds->AddId(pt[i]); }
    return true;
}
bool SurfaceMesh::GetPointToNeighborFaces(const IGsize ptId,
                                          ReturnContainer& faceIds) {
    assert(ptId < GetNumberOfPoints() && "ptId too large");
    const igIndex* pt = m_FaceLinks->GetLinkPointer(ptId);
    int size = m_FaceLinks->GetLinkSize(ptId);
    faceIds.reset();
    for (int i = 0; i < size; i++) { faceIds.push_back(pt[i]); }
    return true;
}
int SurfaceMesh::GetEdgeToNeighborFaces(const IGsize edgeId, igIndex* faceIds) {
    assert(edgeId < GetNumberOfEdges() && "edgeId too large");
    auto& link = m_FaceEdgeLinks->GetLink(edgeId);
    for (int i = 0; i < link.size; i++) { faceIds[i] = link.pointer[i]; }
    return link.size;
}
bool SurfaceMesh::GetEdgeToNeighborFaces(const IGsize edgeId,
                                         const igIndex*& faceIds, int& size) {
    assert(edgeId < GetNumberOfEdges() && "edgeId too large");
    faceIds = m_FaceEdgeLinks->GetLinkPointer(edgeId);
    size = m_FaceEdgeLinks->GetLinkSize(edgeId);
    return true;
}
bool SurfaceMesh::GetEdgeToNeighborFaces(const IGsize edgeId,
                                         IdArray::Pointer faceIds) {
    assert(edgeId < GetNumberOfEdges() && "edgeId too large");
    const igIndex* pt = m_FaceEdgeLinks->GetLinkPointer(edgeId);
    int size = m_FaceEdgeLinks->GetLinkSize(edgeId);
    faceIds->Reset();
    for (int i = 0; i < size; i++) { faceIds->AddId(pt[i]); }
    return true;
}
bool SurfaceMesh::GetEdgeToNeighborFaces(const IGsize edgeId,
                                         ReturnContainer& faceIds) {
    assert(edgeId < GetNumberOfEdges() && "edgeId too large");
    const igIndex* pt = m_FaceEdgeLinks->GetLinkPointer(edgeId);
    int size = m_FaceEdgeLinks->GetLinkSize(edgeId);
    faceIds.reset();
    for (int i = 0; i < size; i++) { faceIds.push_back(pt[i]); }
    return true;
}
int SurfaceMesh::GetEdgeToOneRingFaces(const IGsize edgeId, igIndex* faceIds) {
    assert(edgeId < GetNumberOfEdges() && "edgeId too large");
    igIndex e[2]{};
    GetEdgePointIds(edgeId, e);
    std::set<igIndex> st;
    for (int i = 0; i < 2; i++) {
        auto& link = m_FaceLinks->GetLink(e[i]);
        for (int j = 0; j < link.size; j++) { st.insert(link.pointer[j]); }
    }
    int i = 0;
    for (auto& fid: st) { faceIds[i++] = fid; }
    return i;
}
bool SurfaceMesh::GetEdgeToOneRingFaces(const IGsize edgeId,
                                        IdArray::Pointer faceIds) {
    assert(edgeId < GetNumberOfEdges() && "edgeId too large");
    igIndex e[2]{};
    GetEdgePointIds(edgeId, e);
    std::set<igIndex> st;
    for (int i = 0; i < 2; i++) {
        auto& link = m_FaceLinks->GetLink(e[i]);
        for (int j = 0; j < link.size; j++) { st.insert(link.pointer[j]); }
    }
    faceIds->Reset();
    for (auto& fid: st) { faceIds->AddId(fid); }
    return true;
}
bool SurfaceMesh::GetEdgeToOneRingFaces(const IGsize edgeId,
                                        ReturnContainer& faceIds) {
    assert(edgeId < GetNumberOfEdges() && "edgeId too large");
    igIndex e[2]{};
    GetEdgePointIds(edgeId, e);
    std::set<igIndex> st;
    for (int i = 0; i < 2; i++) {
        auto& link = m_FaceLinks->GetLink(e[i]);
        for (int j = 0; j < link.size; j++) { st.insert(link.pointer[j]); }
    }
    faceIds.reset();
    for (auto& fid: st) { faceIds.push_back(fid); }
    return true;
}
int SurfaceMesh::GetFaceToNeighborFaces(const IGsize faceId, igIndex* faceIds) {
    assert(faceId < GetNumberOfFaces() && "faceId too large");
    igIndex edgeIds[32]{};
    int size = GetFaceEdgeIds(faceId, edgeIds);
    std::set<igIndex> st;
    for (int i = 0; i < size; i++) {
        auto& link = m_FaceEdgeLinks->GetLink(edgeIds[i]);
        for (int j = 0; j < link.size; j++) { st.insert(link.pointer[j]); }
    }
    int i = 0;
    for (auto& fid: st) {
        if (fid != faceId) { faceIds[i++] = fid; }
    }
    return i;
}
bool SurfaceMesh::GetFaceToNeighborFaces(const IGsize faceId,
                                         IdArray::Pointer faceIds) {
    assert(faceId < GetNumberOfFaces() && "faceId too large");
    igIndex edgeIds[32]{};
    int size = GetFaceEdgeIds(faceId, edgeIds);
    std::set<igIndex> st;
    for (int i = 0; i < size; i++) {
        auto& link = m_FaceEdgeLinks->GetLink(edgeIds[i]);
        for (int j = 0; j < link.size; j++) { st.insert(link.pointer[j]); }
    }
    faceIds->Reset();
    for (auto& fid: st) {
        if (fid != faceId) { faceIds->AddId(fid); }
    }
    return true;
}
bool SurfaceMesh::GetFaceToNeighborFaces(const IGsize faceId,
                                         ReturnContainer& faceIds) {
    assert(faceId < GetNumberOfFaces() && "faceId too large");
    igIndex edgeIds[32]{};
    int size = GetFaceEdgeIds(faceId, edgeIds);
    std::set<igIndex> st;
    for (int i = 0; i < size; i++) {
        auto& link = m_FaceEdgeLinks->GetLink(edgeIds[i]);
        for (int j = 0; j < link.size; j++) { st.insert(link.pointer[j]); }
    }
    faceIds.reset();
    for (auto& fid: st) {
        if (fid != faceId) { faceIds.push_back(fid); }
    }
    return true;
}
int SurfaceMesh::GetFaceToOneRingFaces(const IGsize faceId, igIndex* faceIds) {
    assert(faceId < GetNumberOfFaces() && "faceId too large");
    igIndex ptIds[32]{};
    int size = GetFacePointIds(faceId, ptIds);
    std::set<igIndex> st;
    for (int i = 0; i < size; i++) {
        auto& link = m_FaceLinks->GetLink(ptIds[i]);
        for (int j = 0; j < link.size; j++) { st.insert(link.pointer[j]); }
    }
    int i = 0;
    for (auto& fid: st) {
        if (fid != faceId) { faceIds[i++] = fid; }
    }
    return i;
}

bool SurfaceMesh::GetFaceToOneRingFaces(const IGsize faceId,
                                        IdArray::Pointer faceIds) {
    assert(faceId < GetNumberOfFaces() && "faceId too large");
    igIndex ptIds[32]{};
    int size = GetFacePointIds(faceId, ptIds);
    std::set<igIndex> st;
    for (int i = 0; i < size; i++) {
        auto& link = m_FaceLinks->GetLink(ptIds[i]);
        for (int j = 0; j < link.size; j++) { st.insert(link.pointer[j]); }
    }
    faceIds->Reset();
    for (auto& fid: st) {
        if (fid != faceId) { faceIds->AddId(fid); }
    }
    return true;
}

bool SurfaceMesh::GetFaceToOneRingFaces(const IGsize faceId,
                                        ReturnContainer& faceIds) {
    assert(faceId < GetNumberOfFaces() && "faceId too large");
    igIndex ptIds[32]{};
    int size = GetFacePointIds(faceId, ptIds);
    std::set<igIndex> st;
    for (int i = 0; i < size; i++) {
        auto& link = m_FaceLinks->GetLink(ptIds[i]);
        for (int j = 0; j < link.size; j++) { st.insert(link.pointer[j]); }
    }
    faceIds.reset();
    for (auto& fid: st) {
        if (fid != faceId) { faceIds.push_back(fid); }
    }
    return true;
}

igIndex SurfaceMesh::GetEdgeIdFormPointIds(const IGsize ptId1,
                                           const IGsize ptId2) {
    const igIndex* edgeIds;
    int size;
    igIndex e[2]{};
    GetPointToNeighborEdges(ptId1, edgeIds, size);
    for (int i = 0; i < size; i++) {
        GetEdgePointIds(edgeIds[i], e);
        if (ptId1 + ptId2 == e[0] + e[1]) { return edgeIds[i]; }
    }
    return (-1);
}
igIndex SurfaceMesh::GetFaceIdFormPointIds(igIndex* ids, int size) {
    IGsize sum = 0;
    for (int i = 0; i < size; i++) {
        if (ids[i] >= this->GetNumberOfPoints()) { return (-1); }
        sum += ids[i];
    }

    const igIndex* faceIds;
    igIndex ptIds[32]{};
    int size1;
    GetPointToNeighborFaces(ids[0], faceIds, size1);
    for (int i = 0; i < size1; i++) {
        if (size != GetFacePointIds(faceIds[i], ptIds)) continue;
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
            if (count == size) return faceIds[i];
        }
    }
    return (-1);
}

void SurfaceMesh::RequestEditStatus() {
    if (InEditStatus()) { return; }
    RequestPointStatus();
    RequestEdgeStatus();
    RequestFaceStatus();
    MakeEditStatusOn();
}
void SurfaceMesh::RequestEdgeStatus() {
    if (m_Edges == nullptr || (m_Edges->GetMTime() < m_Faces->GetMTime())) {
        BuildEdges();
    }
    if (m_EdgeLinks == nullptr ||
        (m_EdgeLinks->GetMTime() < m_Edges->GetMTime())) {
        BuildEdgeLinks();
    }

    if (m_EdgeDeleteMarker == nullptr) {
        m_EdgeDeleteMarker = DeleteMarker::New();
    }
    m_EdgeDeleteMarker->Initialize(GetNumberOfEdges());
}

void SurfaceMesh::RequestFaceStatus() {
    if (m_FaceEdgeLinks == nullptr ||
        (m_FaceEdgeLinks->GetMTime() < m_FaceEdges->GetMTime())) {
        BuildFaceEdgeLinks();
    }
    if (m_FaceLinks == nullptr ||
        (m_FaceLinks->GetMTime() < m_Faces->GetMTime())) {
        BuildFaceLinks();
    }

    if (m_FaceDeleteMarker == nullptr) {
        m_FaceDeleteMarker = DeleteMarker::New();
    }
    m_FaceDeleteMarker->Initialize(GetNumberOfFaces());
}

void SurfaceMesh::GarbageCollection() {
    IGsize i, mappedPtId = 0, mappedEdgeId = 0;
    igIndex ptIds[32]{}, edgeIds[32]{}, e[2]{};
    CellArray::Pointer newEdges = CellArray::New();
    CellArray::Pointer newFaces = CellArray::New();
    CellArray::Pointer newFaceEdges = CellArray::New();

    IGsize npts = GetNumberOfPoints();
    IGsize nedges = GetNumberOfEdges();
    IGsize nfaces = GetNumberOfFaces();

    std::vector<igIndex> ptMap(npts);
    std::vector<igIndex> edgeMap(nedges);

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
    }

    m_Edges = newEdges;
    m_Faces = newFaces;
    m_FaceEdges = newFaceEdges;
    m_EdgeLinks = nullptr;
    m_FaceLinks = nullptr;
    m_FaceEdgeLinks = nullptr;
    m_PointDeleteMarker = nullptr;
    m_EdgeDeleteMarker = nullptr;
    m_FaceDeleteMarker = nullptr;
    Modified();
    MakeEditStatusOff();
}

bool SurfaceMesh::IsEdgeDeleted(const IGsize edgeId) {
    return m_EdgeDeleteMarker->IsDeleted(edgeId);
}
bool SurfaceMesh::IsFaceDeleted(const IGsize faceId) {
    return m_FaceDeleteMarker->IsDeleted(faceId);
}

IGsize SurfaceMesh::AddPoint(const Point& p) {
    if (!InEditStatus()) { RequestEditStatus(); }
    IGsize ptId = m_Points->AddPoint(p);

    m_EdgeLinks->AddLink();
    m_FaceLinks->AddLink();

    m_PointDeleteMarker->AddTag();
    return ptId;
}
IGsize SurfaceMesh::AddEdge(const IGsize ptId1, const IGsize ptId2) {
    igIndex edgeId = GetEdgeIdFormPointIds(ptId1, ptId2);
    if (edgeId == -1) {
        edgeId = GetNumberOfEdges();
        m_Edges->AddCellId2(ptId1, ptId2);
        m_EdgeLinks->AddReference(ptId1, edgeId);
        m_EdgeLinks->AddReference(ptId2, edgeId);
        m_FaceEdgeLinks->AddLink();
        m_EdgeDeleteMarker->AddTag();
    }
    return edgeId;
}
IGsize SurfaceMesh::AddFace(igIndex* ptIds, int size) {
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

        m_FaceDeleteMarker->AddTag();
    }

    return faceId;
}

void SurfaceMesh::DeletePoint(const IGsize ptId) {
    if (!InEditStatus()) { RequestEditStatus(); }
    if (IsPointDeleted(ptId)) { return; }
    const igIndex* edgeIds;
    int size;
    GetPointToNeighborEdges(ptId, edgeIds, size);
    for (int i = 0; i < size; i++) { DeleteEdge(edgeIds[i]); }
    m_EdgeLinks->DeleteLink(ptId);
    m_FaceLinks->DeleteLink(ptId);
    m_PointDeleteMarker->MarkDeleted(ptId);
}
void SurfaceMesh::DeleteEdge(const IGsize edgeId) {
    if (!InEditStatus()) { RequestEditStatus(); }
    if (IsEdgeDeleted(edgeId)) { return; }
    igIndex e[2]{};
    const igIndex* faceIds;
    int size;
    GetEdgeToNeighborFaces(edgeId, faceIds, size);
    GetEdgePointIds(edgeId, e);
    for (int i = 0; i < 2; i++) { m_EdgeLinks->RemoveReference(e[i], edgeId); }
    for (int i = 0; i < size; i++) { DeleteFace(faceIds[i]); }
    m_FaceEdgeLinks->DeleteLink(edgeId);
    m_EdgeDeleteMarker->MarkDeleted(edgeId);
}
void SurfaceMesh::DeleteFace(const IGsize faceId) {
    if (!InEditStatus()) { RequestEditStatus(); }
    if (IsFaceDeleted(faceId)) { return; }
    igIndex ptIds[32]{}, edgeIds[32]{};
    int size = GetFacePointIds(faceId, ptIds);
    GetFaceEdgeIds(faceId, edgeIds);
    for (int i = 0; i < size; i++) {
        m_FaceLinks->RemoveReference(ptIds[i], faceId);
        m_FaceEdgeLinks->RemoveReference(edgeIds[i], faceId);
    }
    m_FaceDeleteMarker->MarkDeleted(faceId);
}

bool SurfaceMesh::IsBoundaryFace(const IGsize faceId) {
    igIndex ehs[64];
    int ecnt = this->GetFaceEdgeIds(faceId, ehs);
    for (int i = 0; i < ecnt; i++) {
        if (this->IsBoundaryEdge(ehs[i])) return true;
    }
    return false;
}
bool SurfaceMesh::IsBoundaryEdge(const IGsize edgeId) {
    auto& link = m_FaceEdgeLinks->GetLink(edgeId);
    return link.size <= 1;
}
bool SurfaceMesh::IsBoundaryPoint(const IGsize ptId) {
    const igIndex* edgeIds;
    int size;
    this->GetPointToNeighborEdges(ptId, edgeIds, size);
    for (int i = 0; i < size; i++) {
        if (this->IsBoundaryEdge(edgeIds[i])) return true;
    }
    return false;
}
bool SurfaceMesh::IsCornerPoint(const IGsize PointId) {
    auto& link = m_FaceLinks->GetLink(PointId);
    return link.size == 1;
}

void SurfaceMesh::ReplacePointReference(const IGsize fromPtId,
                                        const IGsize toPtId) {
    assert(fromPtId < GetNumberOfPoints() && "ptId too large");
    assert(toPtId < GetNumberOfPoints() && "ptId too large");
    if (fromPtId == toPtId) { return; }
    if (!InEditStatus()) { RequestEditStatus(); }
    igIndex edgeIds[64]{}, faceIds[64]{};
    int size1 = GetPointToNeighborEdges(fromPtId, edgeIds);
    int size2 = GetPointToNeighborFaces(fromPtId, faceIds);
    for (int i = 0; i < size1; i++) {
        m_Edges->ReplaceCellReference(edgeIds[i], fromPtId, toPtId);
    }
    for (int i = 0; i < size2; i++) {
        m_Faces->ReplaceCellReference(faceIds[i], fromPtId, toPtId);
    }

    auto& link1 = m_EdgeLinks->GetLink(fromPtId);
    m_EdgeLinks->SetLink(toPtId, link1.pointer, link1.size);

    auto& link2 = m_FaceLinks->GetLink(fromPtId);
    m_FaceLinks->SetLink(toPtId, link2.pointer, link2.size);
}

SurfaceMesh::SurfaceMesh() { m_ViewStyle = IG_SURFACE; };

IGsize SurfaceMesh::GetRealMemorySize() {
    IGsize res = this->PointSet::GetRealMemorySize();
    if (m_Faces) res += m_Faces->GetRealMemorySize();
    if (m_Edges) res += m_Edges->GetRealMemorySize();
    if (m_EdgeDeleteMarker) res += m_EdgeDeleteMarker->GetRealMemorySize();
    if (m_FaceDeleteMarker) res += m_FaceDeleteMarker->GetRealMemorySize();
    return res + sizeof(IGsize);
}

void SurfaceMesh::SetFaceColor(const float color[3]) {
    m_FaceColor[0] = color[0];
    m_FaceColor[1] = color[1];
    m_FaceColor[2] = color[2];
    m_UseColor = true;
    this->Modified();
}

const float* SurfaceMesh::GetFaceColor() const { return this->m_FaceColor; }

void SurfaceMesh::SetFaceTransparency(float val) {
    this->m_FaceTransparency = val;
    this->Modified();
}

float SurfaceMesh::GetFaceTransparency() const {
    return this->m_FaceTransparency;
}

void SurfaceMesh::Draw(Scene* scene) {
    if (!m_Visibility) { return; }

    if (m_DrawMesh) {
        m_DrawMesh->SetViewStyle(m_ViewStyle);
        return m_DrawMesh->Draw(scene);
    }

    // update uniform buffer
    if (m_UseColor) {
        scene->UBO().useColor = true;
    } else {
        scene->UBO().useColor = false;
    }
    scene->UpdateUniformBuffer();

    if (m_UseColor && m_ColorWithCell) {
        scene->GetShader(Scene::BLINNPHONG)->use();
        m_CellVAO.bind();
        glad_glDrawArrays(GL_TRIANGLES, 0, m_CellPositionSize);
        m_CellVAO.release();
        return;
    }

    if (m_ViewStyle & IG_POINTS) {
        scene->GetShader(Scene::NOLIGHT)->use();

        m_PointVAO.bind();
        glad_glPointSize(8);
        glad_glDepthRange(0.000001, 1);
        glad_glDrawArrays(GL_POINTS, 0, m_Positions->GetNumberOfValues() / 3);
        glad_glDepthRange(0, 1);
        m_PointVAO.release();
    }

    if (m_ViewStyle & IG_WIREFRAME) {
        if (m_UseColor) {
            scene->GetShader(Scene::NOLIGHT)->use();
        } else {
            auto shader = scene->GetShader(Scene::PURECOLOR);
            shader->use();
            shader->setUniform(shader->getUniformLocation("inputColor"),
                               igm::vec3{0.0f, 0.0f, 0.0f});
        }

        m_LineVAO.bind();
        glLineWidth(m_LineWidth);
        glad_glDrawElements(GL_LINES, m_LineIndices->GetNumberOfIds(),
                            GL_UNSIGNED_INT, 0);
        m_LineVAO.release();
    }

    if (m_ViewStyle & IG_SURFACE) {
        auto shader = scene->GetShader(Scene::BLINNPHONG);
        shader->use();
        shader->setUniform(shader->getUniformLocation("transparency"),
                           m_FaceTransparency);

        m_TriangleVAO.bind();
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-0.5f, -0.5f);
        glad_glDrawElements(GL_TRIANGLES, m_TriangleIndices->GetNumberOfIds(),
                            GL_UNSIGNED_INT, 0);
        glDisable(GL_POLYGON_OFFSET_FILL);
        m_TriangleVAO.release();
    }
}

void SurfaceMesh::DrawPhase1(Scene* scene) {
#ifdef IGAME_OPENGL_VERSION_460
    // std::cout << "Draw phase 1:" << std::endl;
    if (!m_Visibility) { return; }

    // update uniform buffer
    {
        if (m_UseColor) {
            scene->UBO().useColor = true;
        } else {
            scene->UBO().useColor = false;
        }
        scene->UpdateUniformBuffer();
    }

    // draw
    if (m_UseColor && m_ColorWithCell) {}

    if (m_ViewStyle & IG_POINTS) {}
    if (m_ViewStyle & IG_WIREFRAME) {}
    if (m_ViewStyle & IG_SURFACE) {
        scene->GetShader(Scene::BLINNPHONG)->use();
        m_TriangleVAO.bind();
        unsigned int count = 0;
        m_Meshlets->VisibleMeshletBuffer().getSubData(0, sizeof(unsigned int),
                                                      &count);
        m_Meshlets->FinalDrawCommandBuffer().target(GL_DRAW_INDIRECT_BUFFER);
        m_Meshlets->FinalDrawCommandBuffer().bind();
        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr,
                                    count, 0);
        std::cout << "Draw phase 1: [render count: " << count;
        std::cout << ", meshlet count: " << m_Meshlets->MeshletsCount() << "]"
                  << std::endl;
        m_TriangleVAO.release();
    }
#endif
}

void SurfaceMesh::DrawPhase2(Scene* scene) {
#ifdef IGAME_OPENGL_VERSION_460
    // std::cout << "Draw phase 2:" << std::endl;
    if (!m_Visibility) { return; }

    // update uniform buffer
    {
        if (m_UseColor) {
            scene->UBO().useColor = true;
        } else {
            scene->UBO().useColor = false;
        }
        scene->UpdateUniformBuffer();
    }

    // draw
    if (m_UseColor && m_ColorWithCell) {}

    if (m_ViewStyle & IG_POINTS) {}
    if (m_ViewStyle & IG_WIREFRAME) {}
    if (m_ViewStyle & IG_SURFACE) {
        // compute culling
        {
            auto shader = scene->GetShader(Scene::MESHLETCULL);
            shader->use();

            GLUniform workMode = shader->getUniformLocation("workMode");
            shader->setUniform(workMode, 0);

            m_Meshlets->MeshletsBuffer().target(GL_SHADER_STORAGE_BUFFER);
            m_Meshlets->MeshletsBuffer().bindBase(1);

            m_Meshlets->DrawCommandBuffer().target(GL_SHADER_STORAGE_BUFFER);
            m_Meshlets->DrawCommandBuffer().bindBase(2);

            unsigned int data = 0;
            m_Meshlets->VisibleMeshletBuffer().subData(0, sizeof(unsigned int),
                                                       &data);
            m_Meshlets->VisibleMeshletBuffer().target(GL_SHADER_STORAGE_BUFFER);
            m_Meshlets->VisibleMeshletBuffer().bindBase(3);

            m_Meshlets->FinalDrawCommandBuffer().target(
                    GL_SHADER_STORAGE_BUFFER);
            m_Meshlets->FinalDrawCommandBuffer().bindBase(4);

            scene->GetDrawCullDataBuffer().target(GL_UNIFORM_BUFFER);
            scene->GetDrawCullDataBuffer().bindBase(5);

            scene->DepthPyramid().active(GL_TEXTURE1);
            shader->setUniform(shader->getUniformLocation("depthPyramid"), 1);

            auto count = m_Meshlets->MeshletsCount();
            glDispatchCompute(((count + 255) / 256), 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }

        scene->GetShader(Scene::BLINNPHONG)->use();
        m_TriangleVAO.bind();

        unsigned int count = 0;
        m_Meshlets->VisibleMeshletBuffer().getSubData(0, sizeof(unsigned int),
                                                      &count);

        m_Meshlets->FinalDrawCommandBuffer().target(GL_DRAW_INDIRECT_BUFFER);
        m_Meshlets->FinalDrawCommandBuffer().bind();
        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr,
                                    count, 0);

        std::cout << "Draw phase 2: [render count: " << count;
        std::cout << ", meshlet count: " << m_Meshlets->MeshletsCount() << "]"
                  << std::endl;

        m_TriangleVAO.release();
    }

#endif
}

void SurfaceMesh::TestOcclusionResults(Scene* scene) {
#ifdef IGAME_OPENGL_VERSION_460
    // std::cout << "Test Occlusion:" << std::endl;
    if (!m_Visibility) { return; }

    if (m_UseColor && m_ColorWithCell) {}

    if (m_ViewStyle & IG_POINTS) {}
    if (m_ViewStyle & IG_WIREFRAME) {}
    if (m_ViewStyle & IG_SURFACE) {
        // compute culling
        {
            auto shader = scene->GetShader(Scene::MESHLETCULL);
            shader->use();

            GLUniform workMode = shader->getUniformLocation("workMode");
            shader->setUniform(workMode, 1);

            m_Meshlets->MeshletsBuffer().target(GL_SHADER_STORAGE_BUFFER);
            m_Meshlets->MeshletsBuffer().bindBase(1);

            m_Meshlets->DrawCommandBuffer().target(GL_SHADER_STORAGE_BUFFER);
            m_Meshlets->DrawCommandBuffer().bindBase(2);

            unsigned int data = 0;
            m_Meshlets->VisibleMeshletBuffer().subData(0, sizeof(unsigned int),
                                                       &data);
            m_Meshlets->VisibleMeshletBuffer().target(GL_SHADER_STORAGE_BUFFER);
            m_Meshlets->VisibleMeshletBuffer().bindBase(3);

            m_Meshlets->FinalDrawCommandBuffer().target(
                    GL_SHADER_STORAGE_BUFFER);
            m_Meshlets->FinalDrawCommandBuffer().bindBase(4);

            scene->GetDrawCullDataBuffer().target(GL_UNIFORM_BUFFER);
            scene->GetDrawCullDataBuffer().bindBase(5);

            scene->DepthPyramid().active(GL_TEXTURE1);
            shader->setUniform(shader->getUniformLocation("depthPyramid"), 1);

            auto count = m_Meshlets->MeshletsCount();
            glDispatchCompute(((count + 255) / 256), 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }
    }

    unsigned int count = 0;
    m_Meshlets->VisibleMeshletBuffer().getSubData(0, sizeof(unsigned int),
                                                  &count);
    // std::cout << "Test Occlusion: [render count: " << count;
    // std::cout << ", meshlet count: " << m_Meshlets->MeshletsCount() << "]"
    //           << std::endl;

    // std::vector<DrawElementsIndirectCommand> readBackCommands(
    //         m_Meshlets->MeshletsCount());
    // m_Meshlets->DrawCommandBuffer().getSubData(
    //         0, readBackCommands.size() * sizeof(DrawElementsIndirectCommand),
    //         readBackCommands.data());
    // for (const auto& cmd: readBackCommands) {
    //     //std::cout << "count: " << cmd.count << std::endl;
    //     std::cout << "primCount: " << cmd.primCount << std::endl;
    //     //std::cout << "firstIndex: " << cmd.firstIndex << std::endl;
    //     //std::cout << "baseVertex: " << cmd.baseVertex << std::endl;
    //     //std::cout << "baseInstance: " << cmd.baseInstance << std::endl;
    // }
#endif
}

void SurfaceMesh::ConvertToDrawableData() {
    if (m_Positions && m_Positions->GetMTime() > this->GetMTime()) { return; }
    if (m_DrawMesh == nullptr || m_DrawMesh->GetMTime() < this->GetMTime()) {
        iGameModelGeometryFilter::Pointer extract =
                iGameModelGeometryFilter::New();
        // update clip status
        if (m_Clip.m_Extent.m_Use) {
            const auto& a = m_Clip.m_Extent.m_bmin;
            const auto& b = m_Clip.m_Extent.m_bmax;
            extract->SetExtent(a[0], b[0], a[1], b[1], a[2], b[2],
                               m_Clip.m_Extent.m_flip);
        }
        if (m_Clip.m_Plane.m_Use) {
            extract->SetClipPlane(m_Clip.m_Plane.m_origin,
                                  m_Clip.m_Plane.m_normal,
                                  m_Clip.m_Plane.m_flip);
        }
        m_DrawMesh = SurfaceMesh::New();
        if (!extract->Execute(this, m_DrawMesh)) { m_DrawMesh = nullptr; }
        if (m_DrawMesh) { m_DrawMesh->Modified(); }
    }
    if (m_DrawMesh) { return m_DrawMesh->ConvertToDrawableData(); }
    if (!m_Flag) {
        m_PointVAO.create();
        m_LineVAO.create();
        m_TriangleVAO.create();
        m_PositionVBO.create();
        m_PositionVBO.target(GL_ARRAY_BUFFER);
        m_ColorVBO.create();
        m_ColorVBO.target(GL_ARRAY_BUFFER);
        m_NormalVBO.create();
        m_NormalVBO.target(GL_ARRAY_BUFFER);
        m_TextureVBO.create();
        m_TextureVBO.target(GL_ARRAY_BUFFER);
        m_PointEBO.create();
        m_PointEBO.target(GL_ELEMENT_ARRAY_BUFFER);
        m_LineEBO.create();
        m_LineEBO.target(GL_ELEMENT_ARRAY_BUFFER);
        m_TriangleEBO.create();
        m_TriangleEBO.target(GL_ELEMENT_ARRAY_BUFFER);

        m_CellVAO.create();
        m_CellPositionVBO.create();
        m_CellPositionVBO.target(GL_ARRAY_BUFFER);
        m_CellColorVBO.create();
        m_CellColorVBO.target(GL_ARRAY_BUFFER);

#ifdef IGAME_OPENGL_VERSION_460
        m_Meshlets->CreateBuffer();
#endif

        m_Flag = true;
    }

    m_Positions = m_Points->ConvertToArray();
    m_Positions->Modified();

    // set line indices
    if (this->GetEdges() == nullptr) { this->BuildEdges(); }
    m_LineIndices = this->GetEdges()->GetCellIdArray();

    // set triangle indices
    IdArray::Pointer triangleIndices = IdArray::New();
    int i, ncell;
    igIndex cell[32]{};

    for (i = 0; i < this->GetNumberOfFaces(); i++) {
        ncell = this->GetFacePointIds(i, cell);
        for (int j = 2; j < ncell; j++) {
            triangleIndices->AddId(cell[0]);
            triangleIndices->AddId(cell[j - 1]);
            triangleIndices->AddId(cell[j]);
        }
    }
    m_TriangleIndices = triangleIndices;

    // set triangles
    if (m_UseColor) {
        IGsize numberOfPoints = m_Positions->GetNumberOfElements();
        m_Colors = FloatArray::New();
        m_Colors->SetDimension(3);
        m_Colors->Resize(numberOfPoints);
        for (IGsize i = 0; i < numberOfPoints; ++i) {
            m_Colors->SetElement(i, m_FaceColor);
        }
    }

    // allocate buffer
    {
        GLAllocateGLBuffer(m_PositionVBO,
                           m_Positions->GetNumberOfValues() * sizeof(float),
                           m_Positions->RawPointer());

        if (m_UseColor) {
            GLAllocateGLBuffer(m_ColorVBO,
                               m_Colors->GetNumberOfValues() * sizeof(float),
                               m_Colors->RawPointer());
        }

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
        if (m_UseColor) {
            m_TriangleVAO.vertexBuffer(GL_VBO_IDX_1, m_ColorVBO, 0,
                                       3 * sizeof(float));
            GLSetVertexAttrib(m_TriangleVAO, GL_LOCATION_IDX_1, GL_VBO_IDX_1, 3,
                              GL_FLOAT, GL_FALSE, 0);
        }
        m_TriangleVAO.elementBuffer(m_TriangleEBO);

#ifdef IGAME_OPENGL_VERSION_460
        bool debug = true;
        if (debug) {
            m_Meshlets->BuildMeshlet(m_Positions->RawPointer(),
                                     m_Positions->GetNumberOfValues() / 3,
                                     m_TriangleIndices->RawPointer(),
                                     m_TriangleIndices->GetNumberOfIds(),
                                     m_TriangleEBO);
        }
#endif
    }
}

void SurfaceMesh::ViewCloudPicture(Scene* scene, int index, int demension) {
    if (m_DrawMesh) {
        m_DrawMesh->SetColorMapper(m_ColorMapper);
        m_AttributeIndex = index;
        m_AttributeDimension = demension;
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
    m_AttributeIndex = index;
    auto& attr = this->GetAttributeSet()->GetAttribute(index);
    if (!attr.isDeleted) {
        if (attr.attachmentType == IG_POINT)
            this->SetAttributeWithPointData(attr.pointer, attr.dataRange,
                                            demension);
        else if (attr.attachmentType == IG_CELL)
            this->SetAttributeWithCellData(attr.pointer, attr.dataRange,
                                           demension);
    }
    scene->DoneCurrent();
    scene->Update();
}

void SurfaceMesh::SetAttributeWithPointData(ArrayObject::Pointer attr,
                                            std::pair<float, float>& range,
                                            igIndex dimension) {
    if (m_ViewAttribute != attr || m_ViewDemension != dimension ||
        m_ColorMapper->GetMTime() > this->GetMTime()) {
        m_ViewAttribute = attr;
        m_ViewDemension = dimension;
        m_UseColor = true;
        m_ColorWithCell = false;

        if (m_ColorMapper->GetMTime() <= this->GetMTime()) {
            if (range.first != range.second) {
                m_ColorMapper->SetRange(range.first, range.second);
            } else if (dimension == -1) {
                m_ColorMapper->InitRange(attr);
            } else {
                m_ColorMapper->InitRange(attr, dimension);
            }
        }
        m_Colors = m_ColorMapper->MapScalars(attr, dimension);
        if (m_Colors == nullptr) { return; }
        range.first = m_ColorMapper->GetRange()[0];
        range.second = m_ColorMapper->GetRange()[1];
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

void SurfaceMesh::SetAttributeWithCellData(ArrayObject::Pointer attr,
                                           std::pair<float, float>& range,
                                           igIndex dimension) {
    if (m_ViewAttribute != attr || m_ViewDemension != dimension ||
        m_ColorMapper->GetMTime() > this->GetMTime()) {
        m_ViewAttribute = attr;
        m_ViewDemension = dimension;
        m_UseColor = true;
        m_ColorWithCell = true;

        if (m_ColorMapper->GetMTime()<=this->GetMTime()) {
            if (dimension == -1) {
                m_ColorMapper->InitRange(attr);
            } else {
                m_ColorMapper->InitRange(attr, dimension);
            }
        }

        //        if (dimension == -1) {
        //            mapper->InitRange(attr);
        //        } else {
        //            mapper->InitRange(attr, dimension);
        //        }

        FloatArray::Pointer colors = m_ColorMapper->MapScalars(attr, dimension);
        if (colors == nullptr) { return; }

        FloatArray::Pointer newPositions = FloatArray::New();
        FloatArray::Pointer newColors = FloatArray::New();
        newPositions->SetDimension(3);
        newColors->SetDimension(3);

        float color[3]{};
        for (int i = 0; i < this->GetNumberOfFaces(); i++) {
            Face* face = this->GetFace(i);
            colors->GetElement(i, color);
            for (int j = 2; j < face->GetCellSize(); j++) {
                auto& p0 = face->Points->GetPoint(0);
                newPositions->AddElement3(p0[0], p0[1], p0[2]);
                auto& p1 = face->Points->GetPoint(j - 1);
                newPositions->AddElement3(p1[0], p1[1], p1[2]);
                auto& p2 = face->Points->GetPoint(j);
                newPositions->AddElement3(p2[0], p2[1], p2[2]);

                newColors->AddElement3(color[0], color[1], color[2]);
                newColors->AddElement3(color[0], color[1], color[2]);
                newColors->AddElement3(color[0], color[1], color[2]);
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