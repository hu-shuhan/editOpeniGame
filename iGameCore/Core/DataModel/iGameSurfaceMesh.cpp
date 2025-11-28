#include "iGameSurfaceMesh.h"
#include "ModelSurfaceFilter/iGameModelGeometryFilter.h"
#include "SurfaceMeshFilters/iGameMeshSimplifier.h"
#include "iGameScene.h"
#include "iGameThreadPool.h"
#include "iGameTimer.h"

IGAME_NAMESPACE_BEGIN

IGsize SurfaceMesh::GetNumberOfEdges() const noexcept { return m_Edges ? m_Edges->GetNumberOfCells() : 0; }
IGsize SurfaceMesh::GetNumberOfFaces() const noexcept { return m_Faces ? m_Faces->GetNumberOfCells() : 0; }

bool SurfaceMesh::ShallowCopy(DataObject::Pointer o) { return false; }

bool SurfaceMesh::DeepCopy(DataObject::Pointer o) { return false; }

bool SurfaceMesh::ShallowCopy(SurfaceMesh::Pointer o) {
    if (o == nullptr) { return false; }

    this->SetPoints(o->m_Points);
    this->SetFaces(o->m_Faces);
    this->SetAttributeSet(o->m_Attributes);
    return true;
}

bool SurfaceMesh::DeepCopy(SurfaceMesh::Pointer other) {
    if (other == nullptr) { return false; }
    m_Points = Points::New();
    m_Points->DeepCopy(other->m_Points);
    m_Faces = CellArray::New();
    m_Faces->DeepCopy(other->m_Faces);
    m_Attributes = AttributeSet::New();
    m_Attributes->DeepCopy(other->GetAttributeSet());
    this->Modified();
    return true;
}

CellArray* SurfaceMesh::GetEdges() { return m_Edges ? m_Edges.get() : nullptr; }
CellArray* SurfaceMesh::GetFaces() { return m_Faces ? m_Faces.get() : nullptr; }

void SurfaceMesh::SetFaces(CellArray::Pointer faces) {
    if (m_Faces != faces) {
        m_Faces = faces;
        this->Modified();
    }
}
void SurfaceMesh::SetEdges(CellArray::Pointer edges) {
    if (m_Edges != edges) {
        m_Edges = edges;
        this->Modified();
    }
}
Line* SurfaceMesh::GetEdge(const IGsize edgeId) {
    const igIndex* cell;
    int ncells = m_Edges->GetCellIds(edgeId, cell);

    if (m_Edge == nullptr) { m_Edge = Line::New(); }

    m_Edge->m_PointIds->Reset();
    m_Edge->m_PointIds->AddId(cell[0]);
    m_Edge->m_PointIds->AddId(cell[1]);
    m_Edge->m_Points->Reset();
    m_Edge->m_Points->AddPoint(this->GetPoint(cell[0]));
    m_Edge->m_Points->AddPoint(this->GetPoint(cell[1]));
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

    face->m_PointIds->Reset();
    face->m_Points->Reset();

    for (int i = 0; i < ncells; i++) {
        face->m_PointIds->AddId(cell[i]);
        face->m_Points->AddPoint(this->GetPoint(cell[i]));
    }

    return face;
}

int SurfaceMesh::GetEdgePointIds(const IGsize edgeId, igIndex* ptIds) {
    if (m_Edges == nullptr) { this->BuildEdges(); }
    m_Edges->GetCellIds(edgeId, ptIds);
    return 2;
}

int SurfaceMesh::GetFacePointIds(const IGsize faceId, igIndex* ptIds) { return m_Faces->GetCellIds(faceId, ptIds); }

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
            if ((idx = EdgeTable->IsEdge(face[j], face[(j + 1) % size])) == -1) {
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
    if (m_FaceLinks && m_FaceLinks->GetMTime() > m_Faces->GetMTime()) { return; }

    m_FaceLinks = CellLinks::New();
    IGsize npts = GetNumberOfPoints();
    IGsize nfaces = GetNumberOfFaces();
    igIndex face[32]{};

    m_FaceLinks->Allocate(npts);
    for (int i = 0; i < nfaces; i++) {
        int size = m_Faces->GetCellIds(i, face);
        for (int j = 0; j < size; j++) { m_FaceLinks->IncrementLinkSize(face[j]); }
    }

    m_FaceLinks->AllocateLinks(npts);
    for (int i = 0; i < nfaces; i++) {
        int size = m_Faces->GetCellIds(i, face);
        for (int j = 0; j < size; j++) { m_FaceLinks->AddReference(face[j], i); }
    }
    m_FaceLinks->Modified();
}

void SurfaceMesh::BuildFaceEdgeLinks() {
    if (m_FaceEdgeLinks && m_FaceEdgeLinks->GetMTime() > m_FaceEdges->GetMTime()) { return; }

    m_FaceEdgeLinks = CellLinks::New();
    IGsize nedges = GetNumberOfEdges();
    IGsize nfaces = GetNumberOfFaces();
    igIndex face[32]{};

    m_FaceEdgeLinks->Allocate(nedges);
    for (int i = 0; i < nfaces; i++) {
        int size = m_FaceEdges->GetCellIds(i, face);
        for (int j = 0; j < size; j++) { m_FaceEdgeLinks->IncrementLinkSize(face[j]); }
    }

    m_FaceEdgeLinks->AllocateLinks(nedges);
    for (int i = 0; i < nfaces; i++) {
        int size = m_FaceEdges->GetCellIds(i, face);
        for (int j = 0; j < size; j++) { m_FaceEdgeLinks->AddReference(face[j], i); }
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

    auto& link = m_EdgeLinks->GetLink(ptId);
    igIndex e[2]{};
    for (int i = 0; i < link.size; i++) {
        GetEdgePointIds(link.pointer[i], e);
        ptIds[i] = e[0] - ptId + e[1];
        assert(e[0] == ptId || e[1] == ptId);
    }
    return link.size;
}
bool SurfaceMesh::GetPointToOneRingPoints(const IGsize ptId, IdArray::Pointer ptIds) {

    ptIds->Reset();
    auto& link = m_EdgeLinks->GetLink(ptId);
    igIndex e[2]{};
    for (int i = 0; i < link.size; i++) {
        GetEdgePointIds(link.pointer[i], e);
        ptIds->AddId(e[0] - ptId + e[1]);
        assert(e[0] == ptId || e[1] == ptId);
    }
    return true;
}
bool SurfaceMesh::GetPointToOneRingPoints(const IGsize ptId, ReturnContainer& ptIds) {

    ptIds.reset();
    auto& link = m_EdgeLinks->GetLink(ptId);
    igIndex e[2]{};
    for (int i = 0; i < link.size; i++) {
        GetEdgePointIds(link.pointer[i], e);
        ptIds.push_back(e[0] - ptId + e[1]);
        assert(e[0] == ptId || e[1] == ptId);
    }
    return true;
}
int SurfaceMesh::GetPointToNeighborEdges(const IGsize ptId, igIndex* edgeIds) {

    auto& link = m_EdgeLinks->GetLink(ptId);
    for (int i = 0; i < link.size; i++) { edgeIds[i] = link.pointer[i]; }
    return link.size;
}
bool SurfaceMesh::GetPointToNeighborEdges(const IGsize ptId, const igIndex*& edgeIds, int& size) {

    edgeIds = m_EdgeLinks->GetLinkPointer(ptId);
    size = m_EdgeLinks->GetLinkSize(ptId);
    return true;
}
bool SurfaceMesh::GetPointToNeighborEdges(const IGsize ptId, igIndex* edgeIds, int& size) {

    auto& link = m_EdgeLinks->GetLink(ptId);
    for (int i = 0; i < link.size; i++) { edgeIds[i] = link.pointer[i]; }
    size = link.size;
    return true;
}

bool SurfaceMesh::GetPointToNeighborEdges(const IGsize ptId, IdArray::Pointer edgeIds) {

    const igIndex* pt = m_EdgeLinks->GetLinkPointer(ptId);
    int size = m_EdgeLinks->GetLinkSize(ptId);
    edgeIds->Reset();
    for (int i = 0; i < size; i++) { edgeIds->AddId(pt[i]); }
    return true;
}
bool SurfaceMesh::GetPointToNeighborEdges(const IGsize ptId, ReturnContainer& edgeIds) {
    const igIndex* pt = m_EdgeLinks->GetLinkPointer(ptId);
    int size = m_EdgeLinks->GetLinkSize(ptId);
    edgeIds.reset();
    for (int i = 0; i < size; i++) { edgeIds.push_back(pt[i]); }
    return true;
}
int SurfaceMesh::GetPointToNeighborFaces(const IGsize ptId, igIndex* faceIds) {

    auto& link = m_FaceLinks->GetLink(ptId);
    for (int i = 0; i < link.size; i++) { faceIds[i] = link.pointer[i]; }
    return link.size;
}
bool SurfaceMesh::GetPointToNeighborFaces(const IGsize ptId, const igIndex*& faceIds, int& size) {

    faceIds = m_FaceLinks->GetLinkPointer(ptId);
    size = m_FaceLinks->GetLinkSize(ptId);
    return true;
}
bool SurfaceMesh::GetPointToNeighborFaces(const IGsize ptId, IdArray::Pointer faceIds) {

    const igIndex* pt = m_FaceLinks->GetLinkPointer(ptId);
    int size = m_FaceLinks->GetLinkSize(ptId);
    faceIds->Reset();
    for (int i = 0; i < size; i++) { faceIds->AddId(pt[i]); }
    return true;
}
bool SurfaceMesh::GetPointToNeighborFaces(const IGsize ptId, ReturnContainer& faceIds) {

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
bool SurfaceMesh::GetEdgeToNeighborFaces(const IGsize edgeId, const igIndex*& faceIds, int& size) {
    assert(edgeId < GetNumberOfEdges() && "edgeId too large");
    faceIds = m_FaceEdgeLinks->GetLinkPointer(edgeId);
    size = m_FaceEdgeLinks->GetLinkSize(edgeId);
    return true;
}
bool SurfaceMesh::GetEdgeToNeighborFaces(const IGsize edgeId, IdArray::Pointer faceIds) {
    assert(edgeId < GetNumberOfEdges() && "edgeId too large");
    const igIndex* pt = m_FaceEdgeLinks->GetLinkPointer(edgeId);
    int size = m_FaceEdgeLinks->GetLinkSize(edgeId);
    faceIds->Reset();
    for (int i = 0; i < size; i++) { faceIds->AddId(pt[i]); }
    return true;
}
bool SurfaceMesh::GetEdgeToNeighborFaces(const IGsize edgeId, ReturnContainer& faceIds) {
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
bool SurfaceMesh::GetEdgeToOneRingFaces(const IGsize edgeId, IdArray::Pointer faceIds) {
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
bool SurfaceMesh::GetEdgeToOneRingFaces(const IGsize edgeId, ReturnContainer& faceIds) {
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
bool SurfaceMesh::GetFaceToNeighborFaces(const IGsize faceId, IdArray::Pointer faceIds) {
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
bool SurfaceMesh::GetFaceToNeighborFaces(const IGsize faceId, ReturnContainer& faceIds) {
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

bool SurfaceMesh::GetFaceToOneRingFaces(const IGsize faceId, IdArray::Pointer faceIds) {
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

bool SurfaceMesh::GetFaceToOneRingFaces(const IGsize faceId, ReturnContainer& faceIds) {
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

igIndex SurfaceMesh::GetEdgeIdFormPointIds(const IGsize ptId1, const IGsize ptId2) {
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
    if (m_Edges == nullptr || (m_Edges->GetMTime() < m_Faces->GetMTime())) { BuildEdges(); }
    if (m_EdgeLinks == nullptr || (m_EdgeLinks->GetMTime() < m_Edges->GetMTime())) { BuildEdgeLinks(); }

    if (m_EdgeDeleteMarker == nullptr) { m_EdgeDeleteMarker = DeleteMarker::New(); }
    m_EdgeDeleteMarker->Initialize(GetNumberOfEdges());
}

void SurfaceMesh::RequestFaceStatus() {
    if (m_FaceEdgeLinks == nullptr || (m_FaceEdgeLinks->GetMTime() < m_FaceEdges->GetMTime())) { BuildFaceEdgeLinks(); }
    if (m_FaceLinks == nullptr || (m_FaceLinks->GetMTime() < m_Faces->GetMTime())) { BuildFaceLinks(); }

    if (m_FaceDeleteMarker == nullptr) { m_FaceDeleteMarker = DeleteMarker::New(); }
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
        if (i != mappedPtId) { m_Points->SetPoint(mappedPtId, m_Points->GetPoint(i)); }
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

bool SurfaceMesh::IsEdgeDeleted(const IGsize edgeId) { return m_EdgeDeleteMarker->IsDeleted(edgeId); }
bool SurfaceMesh::IsFaceDeleted(const IGsize faceId) { return m_FaceDeleteMarker->IsDeleted(faceId); }

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
    for (int i = 0; i < size; i++) { edgeIds[i] = AddEdge(ptIds[i], ptIds[(i + 1) % size]); }
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
    ReturnContainer edgeIds;
    GetPointToNeighborEdges(ptId, edgeIds);
    for (int i = 0; i < edgeIds.size(); i++) { DeleteEdge(edgeIds[i]); }
    m_EdgeLinks->DeleteLink(ptId);
    m_FaceLinks->DeleteLink(ptId);
    m_PointDeleteMarker->MarkDeleted(ptId);
}
void SurfaceMesh::DeleteEdge(const IGsize edgeId) {
    if (!InEditStatus()) { RequestEditStatus(); }
    if (IsEdgeDeleted(edgeId)) { return; }
    igIndex e[2]{};
    ReturnContainer faceIds;
    GetEdgeToNeighborFaces(edgeId, faceIds);
    GetEdgePointIds(edgeId, e);
    for (int i = 0; i < 2; i++) { m_EdgeLinks->RemoveReference(e[i], edgeId); }
    for (int i = 0; i < faceIds.size(); i++) { DeleteFace(faceIds[i]); }
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

void SurfaceMesh::ReplacePointReference(const IGsize fromPtId, const IGsize toPtId) {
    if (fromPtId == toPtId) { return; }
    if (!InEditStatus()) { RequestEditStatus(); }
    igIndex edgeIds[64]{}, faceIds[64]{};
    int size1 = GetPointToNeighborEdges(fromPtId, edgeIds);
    int size2 = GetPointToNeighborFaces(fromPtId, faceIds);
    for (int i = 0; i < size1; i++) { m_Edges->ReplaceCellReference(edgeIds[i], fromPtId, toPtId); }
    for (int i = 0; i < size2; i++) { m_Faces->ReplaceCellReference(faceIds[i], fromPtId, toPtId); }

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

void SurfaceMesh::ConvertToDrawableData() {
    if (m_Points->GetMTime() > m_Positions->GetMTime() || m_Clipper->GetMTime() > m_Positions->GetMTime()) {
        // 为统一架构，设置可绘制对象为自身
        if (m_IsMainRenderableObject) { SetRenderableObject(this); }

        // 转换为可绘制数据
        GetDrawableArray(m_Positions, m_LineIndices, m_TriangleIndices, m_TriangleEdgeMasks);
        m_Positions->Modified();
        m_LineIndices->Modified();
        m_TriangleIndices->Modified();
        m_TriangleEdgeMasks->Modified();
    }

    // convert scalar data
    if (m_AttributeIndex == -1) {
        m_UseColor = false;
        m_ColorWithCell = false;
    } else {
        m_UseColor = true;

        auto& attr = this->GetAttributeSet()->GetAttribute(m_AttributeIndex);
        if (attr.type == IG_RGB) {
            this->m_ColorMapper->SetVectorModeToRGBColors();
        } else {
            this->m_ColorMapper->SetVectorModeToComponent();
        }
        if (!attr.isDeleted) {
            auto dataRange = attr.GetDataRange();
            if (attr.attachmentType == IG_POINT) {
                if (m_AttributeHelper->GetMTime() > m_Colors->GetMTime() ||
                    m_ColorMapper->GetMTime() > m_Colors->GetMTime()) {
                    m_ColorWithCell = false;
                    this->SetAttributeWithPointData(attr.pointer, dataRange, m_AttributeDimension);
                }
            } else if (attr.attachmentType == IG_CELL) {
                if (m_AttributeHelper->GetMTime() > m_CellColors->GetMTime() ||
                    m_ColorMapper->GetMTime() > m_CellColors->GetMTime()) {
                    m_ColorWithCell = true;
                    this->SetAttributeWithCellData(attr.pointer, dataRange, m_AttributeDimension);
                }
            }
        }
    }
}

void SurfaceMesh::GetDrawableArray(FloatArray::Pointer& positions, UnsignedIntArray::Pointer& lineIndices,
                                   UnsignedIntArray::Pointer& triangleIndices,
                                   UnsignedCharArray::Pointer& triangleEdgeMasks) {
    Timer::Pointer timer = Timer::New();

    positions = m_Points->ConvertToArray();

    lineIndices->Reset();
    lineIndices->SetDimension(2);

    triangleIndices->Reset();
    triangleIndices->SetDimension(3);

    triangleEdgeMasks->Reset();
    triangleEdgeMasks->SetDimension(1);

    // set line indices
    if (this->GetEdges() == nullptr) { this->BuildEdges(); }

    if (m_Clipper->IsAllDisable()) {
        // set triangle indices
        int i, ncell;
        igIndex cell[32]{};

        lineIndices->Reserve(this->GetNumberOfEdges());
        for (i = 0; i < this->GetNumberOfEdges(); i++) {
            ncell = this->GetEdgePointIds(i, cell);
            if (cell[0] < 0 || cell[1] < 0) {
                igError("The index of the edge is negative.");
            } else {
                lineIndices->AddElement2(static_cast<iguIndex>(cell[0]), static_cast<iguIndex>(cell[1]));
            }
        }

        IGsize fcnt = this->GetNumberOfFaces();
        IGsize faceIdNum = this->GetFaces()->GetNumberOfCellIds();
        triangleIndices->Resize(faceIdNum - fcnt * 2);
        triangleEdgeMasks->Resize(faceIdNum - fcnt * 2);
        auto func = [&](igIndex start, igIndex end) -> void {
            int ncell;
            auto faces = this->GetFaces();
            auto cellPointer = faces->GetCellIdArray()->RawPointer();
            cellPointer += faces->GetStartOffset(start);
            igIndex i = 0, j = 0;
            int mask = 0;
            IGsize index = faces->GetStartOffset(start) - start * 2;
            auto trianglePointer = triangleIndices->RawPointer() + index * 3;
            auto triangleEdgeMasksPointer = triangleEdgeMasks->RawPointer() + index;
            for (i = start; i < end; i++) {
                ncell = faces->GetCellSize(i);
                if (ncell == 6) {
                    *trianglePointer++ = cellPointer[0];
                    *trianglePointer++ = cellPointer[1];
                    *trianglePointer++ = cellPointer[5];
                    *trianglePointer++ = cellPointer[1];
                    *trianglePointer++ = cellPointer[2];
                    *trianglePointer++ = cellPointer[3];
                    *trianglePointer++ = cellPointer[3];
                    *trianglePointer++ = cellPointer[4];
                    *trianglePointer++ = cellPointer[5];
                    *trianglePointer++ = cellPointer[5];
                    *trianglePointer++ = cellPointer[3];
                    *trianglePointer++ = cellPointer[1];
                    *triangleEdgeMasksPointer++ = 5;
                    *triangleEdgeMasksPointer++ = 3;
                    *triangleEdgeMasksPointer++ = 3;
                    *triangleEdgeMasksPointer++ = 0;
                } else {
                    for (j = 1; j < ncell - 1; j++) {
                        // add edge mask
                        mask = ncell == 3 ? 7 : j == 1 ? 3 : j == ncell - 2 ? 6 : 2;
                        *trianglePointer++ = cellPointer[0];
                        *trianglePointer++ = cellPointer[j];
                        *trianglePointer++ = cellPointer[j + 1];
                        *triangleEdgeMasksPointer++ = mask;
                    }
                }
                cellPointer += ncell;
            }
        };
        ThreadPool::parallelFor(0, this->GetNumberOfFaces(), func);
        //IGsize fcnt = this->GetNumberOfFaces();
        //IGsize faceIdNum = this->GetFaces()->GetNumberOfCellIds();
        //triangleIndices->Reserve(faceIdNum - fcnt * 2);
        //triangleEdgeMasks->Reserve(faceIdNum - fcnt * 2);
        //for (i = 0; i < this->GetNumberOfFaces(); i++) {
        //	ncell = this->GetFacePointIds(i, cell);
        //	if (ncell == 6) {
        //		triangleIndices->AddElement3(cell[0], cell[1], cell[5]);
        //		triangleEdgeMasks->AddValue(5);
        //		triangleIndices->AddElement3(cell[1], cell[2], cell[3]);
        //		triangleEdgeMasks->AddValue(3);
        //		triangleIndices->AddElement3(cell[3], cell[4], cell[5]);
        //		triangleEdgeMasks->AddValue(3);
        //		triangleIndices->AddElement3(cell[5], cell[3], cell[1]);
        //		triangleEdgeMasks->AddValue(0);
        //	}
        //	else {
        //		for (int j = 1; j < ncell - 1; j++) {
        //			triangleIndices->AddElement3(cell[0], cell[j], cell[j + 1]);
        //			// add edge mask
        //			int mask = ncell == 3 ? 7
        //				: j == 1 ? 3
        //				: j == ncell - 2 ? 6
        //				: 2;
        //			triangleEdgeMasks->AddValue(mask);
        //		}
        //	}
        //}
    } else {
        // set triangle indices
        int i, ncell;
        igIndex cell[32]{};

        for (i = 0; i < this->GetNumberOfEdges(); i++) {
            ncell = this->GetEdgePointIds(i, cell);
            if (cell[0] < 0 || cell[1] < 0) {
                igError("The index of the edge is negative.");
            } else {
                lineIndices->AddElement2(static_cast<iguIndex>(cell[0]), static_cast<iguIndex>(cell[1]));
            }
        }

        for (i = 0; i < this->GetNumberOfFaces(); i++) {
            ncell = this->GetFacePointIds(i, cell);
            bool visible = true;
            for (int j = 0; j < ncell; j++) {
                const auto& point = this->GetPoint(cell[j]);
                if (!m_Clipper->IsVisible(point.pointer())) {
                    visible = false;
                    break;
                }
            }
            if (!visible) continue;

            if (ncell == 6) {
                triangleIndices->AddElement3(cell[0], cell[1], cell[5]);
                triangleEdgeMasks->AddValue(5);
                triangleIndices->AddElement3(cell[1], cell[2], cell[3]);
                triangleEdgeMasks->AddValue(3);
                triangleIndices->AddElement3(cell[3], cell[4], cell[5]);
                triangleEdgeMasks->AddValue(3);
                triangleIndices->AddElement3(cell[5], cell[3], cell[1]);
                triangleEdgeMasks->AddValue(0);
            } else {
                for (int j = 1; j < ncell - 1; j++) {
                    triangleIndices->AddElement3(cell[0], cell[j], cell[j + 1]);
                    // add edge mask
                    int mask = ncell == 3 ? 7 : j == 1 ? 3 : j == ncell - 2 ? 6 : 2;
                    triangleEdgeMasks->AddValue(mask);
                }
            }
        }
    }
    IGAME_CORE_DEBUG("Get draw array cost {} ms.", timer->ElapsedMilliseconds());
}

void SurfaceMesh::SetAttributeWithCellData(ArrayObject::Pointer attr, DoubleArray::Pointer attrRange,
                                           igIndex dimension) {
    if (!m_ColorMapper->GetStable() && m_ColorMapper->GetMTime() <= this->GetMTime()) {
        double magnitude_min = attrRange->GetValue(0);
        double magnitude_max = attrRange->GetValue(1);
        if (magnitude_min < magnitude_max) {
            m_ColorMapper->SetRange(magnitude_min, magnitude_max);
        } else if (dimension == -1) {
            m_ColorMapper->InitRange(attr);
        } else {
            m_ColorMapper->InitRange(attr, dimension);
        }
    }

    attrRange->SetValue(0, m_ColorMapper->GetRange()[0]);
    attrRange->SetValue(1, m_ColorMapper->GetRange()[1]);
    //    range.first = m_ColorMapper->GetRange()[0];
    //    range.second = m_ColorMapper->GetRange()[1];

    FloatArray::Pointer colors = m_ColorMapper->MapScalars(attr, dimension);
    if (colors == nullptr) { return; }

    FloatArray::Pointer newPositions = FloatArray::New();
    FloatArray::Pointer newColors = FloatArray::New();
    UnsignedCharArray::Pointer newEdgeMasks = UnsignedCharArray::New();
    newPositions->SetDimension(3);
    newColors->SetDimension(3);
    newEdgeMasks->SetDimension(3);
    IGsize fcnt = this->GetNumberOfFaces();
    IGsize faceIdNum = this->GetFaces()->GetNumberOfCellIds();
    newPositions->Reserve(faceIdNum - fcnt * 2);
    newColors->Reserve(faceIdNum - fcnt * 2);
    float color[3]{};
    for (int i = 0; i < this->GetNumberOfFaces(); i++) {
        Face* face = this->GetFace(i);
        colors->GetElement(i, color);
        for (int j = 1; j < face->GetCellSize() - 1; j++) {
            auto& p0 = face->m_Points->GetPoint(0);
            newPositions->AddElement3(p0[0], p0[1], p0[2]);
            auto& p1 = face->m_Points->GetPoint(j);
            newPositions->AddElement3(p1[0], p1[1], p1[2]);
            auto& p2 = face->m_Points->GetPoint(j + 1);
            newPositions->AddElement3(p2[0], p2[1], p2[2]);

            newColors->AddElement3(color[0], color[1], color[2]);
            newColors->AddElement3(color[0], color[1], color[2]);
            newColors->AddElement3(color[0], color[1], color[2]);

            int mask = face->GetCellSize() == 3 ? 7 : j == 1 ? 3 : j == face->GetCellSize() - 2 ? 6 : 2;
            newEdgeMasks->AddValue(mask);
        }
    }
    m_CellPositionSize = newPositions->GetNumberOfElements();

    m_CellPositions = newPositions;
    m_CellPositions->Modified();

    m_CellColors = newColors;
    m_CellColors->Modified();

    m_CellTriangleEdgeMasks = newEdgeMasks;
    m_CellTriangleEdgeMasks->Modified();
}


IGAME_NAMESPACE_END
