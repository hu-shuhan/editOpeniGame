#ifndef iGameSurfaceMesh_h
#define iGameSurfaceMesh_h

#include "iGameCellArray.h"
#include "iGameCellLinks.h"
#include "iGameEdgeTable.h"
#include "iGameFlexArray.h"
#include "iGamePointSet.h"

#include "iGameLine.h"
#include "iGamePolygon.h"
#include "iGameQuad.h"
#include "iGameTriangle.h"

IGAME_NAMESPACE_BEGIN
class SurfaceMesh : public PointSet {
public:
    I_OBJECT(SurfaceMesh);
    static Pointer New() { return new SurfaceMesh; }
    using ReturnContainer = FlexArray<igIndex, 128>;

    IGenum GetDataObjectType() const { return IG_SURFACE_MESH; }
    // Get the number of all edges
    IGsize GetNumberOfEdges() const noexcept;
    // Get the number of all faces
    IGsize GetNumberOfFaces() const noexcept;

    bool ShallowCopy(DataObject::Pointer o) override;
    bool DeepCopy(DataObject::Pointer o) override;
    bool ShallowCopy(SurfaceMesh::Pointer o);
    bool DeepCopy(SurfaceMesh::Pointer o);

    // Get edge array
    CellArray* GetEdges();
    // Get face array
    CellArray* GetFaces();

    // Set face array
    void SetFaces(CellArray::Pointer faces);
    // Set edge array
    void SetEdges(CellArray::Pointer edges);
    // Get edge cell by index edgeId. Thread-Unsafe, please use GetEdgePointId
    Line* GetEdge(const IGsize edgeId);
    // Get face cell by index faceId. Thread-Unsafe, please use GetFacePointId and GetFaceEdgeId
    Face* GetFace(const IGsize faceId);

    // Get edge's point index. Return PointIds size
    int GetEdgePointIds(const IGsize edgeId, igIndex* ptIds);
    // Get face's point index. Return PointIds size
    int GetFacePointIds(const IGsize faceId, igIndex* ptIds);
    // Get face's edge index. Return EdgeIds size
    int GetFaceEdgeIds(const IGsize faceId, igIndex* edgeIds);

    // All edges are constructed according to the point index of the face
    void BuildEdges();
    // Construct the adjacent edges of the points
    void BuildEdgeLinks();
    // Construct the adjacent faces of the points
    void BuildFaceLinks();
    // Construct the adjacent faces of the edges
    void BuildFaceEdgeLinks();

    enum Type {
        P2P = 0,
        P2E,
        P2F,
        P2V,
        E2F,
        E2RF,
        E2V,
        F2F,
        F2RF,
        F2V,
    };
    int GetNumberOfLinks(const IGsize id, Type type);
    // Get all points within one ring of a point. Return the size of indices.
    int GetPointToOneRingPoints(const IGsize ptId,
                                igIndex* ptIds); // Unsafe: Stake overflow
    bool GetPointToOneRingPoints(const IGsize ptId,
                                 IdArray::Pointer ptIds); // Safe
    bool GetPointToOneRingPoints(const IGsize ptId,
                                 ReturnContainer& ptIds); // Safe

    // Get all neighboring edges of a point. Return the size of indices.
    int GetPointToNeighborEdges(const IGsize ptId,
                                igIndex* edgeIds); // Unsafe: Stake overflow
    bool GetPointToNeighborEdges(const IGsize ptId, const igIndex*& edgeIds,
                                 int& size); // Safe but data will be modified
    bool GetPointToNeighborEdges(const IGsize ptId, igIndex* edgeIds,
                                 int& size); // Safe but data will be modified
    bool GetPointToNeighborEdges(const IGsize ptId,
                                 IdArray::Pointer edgeIds); // Safe
    bool GetPointToNeighborEdges(const IGsize ptId,
                                 ReturnContainer& edgeIds); // Safe

    // Get all neighboring faces of a point. Return the size of indices.
    int GetPointToNeighborFaces(const IGsize ptId,
                                igIndex* faceIds); // Unsafe: Stake overflow
    bool GetPointToNeighborFaces(const IGsize ptId, const igIndex*& faceIds,
                                 int& size); // Safe but data will be modified
    bool GetPointToNeighborFaces(const IGsize ptId,
                                 IdArray::Pointer faceIds); // Safe
    bool GetPointToNeighborFaces(const IGsize ptId,
                                 ReturnContainer& faceIds); // Safe

    // Get all neighboring faces of a edge (shared edge). Return the size of
    // indices.
    int GetEdgeToNeighborFaces(const IGsize edgeId,
                               igIndex* faceIds); // Unsafe: Stake overflow
    bool GetEdgeToNeighborFaces(const IGsize edgeId, const igIndex*& faceIds,
                                int& size); // Safe but data will be modified
    bool GetEdgeToNeighborFaces(const IGsize edgeId,
                                IdArray::Pointer faceIds); // Safe
    bool GetEdgeToNeighborFaces(const IGsize edgeId,
                                ReturnContainer& faceIds); // Safe

    // Get all faces within one ring of a edge (shared point). Return the size of
    // indices.
    int GetEdgeToOneRingFaces(const IGsize edgeId,
                              igIndex* faceIds); // Unsafe: Stake overflow
    bool GetEdgeToOneRingFaces(const IGsize edgeId,
                               IdArray::Pointer faceIds); // Safe
    bool GetEdgeToOneRingFaces(const IGsize edgeId,
                               ReturnContainer& faceIds); // Safe

    // Get all neighboring faces of a face (shared edge). Return the size of
    // indices.
    int GetFaceToNeighborFaces(const IGsize faceId,
                               igIndex* faceIds); // Unsafe: Stake overflow
    bool GetFaceToNeighborFaces(const IGsize faceId,
                                IdArray::Pointer faceIds); // Safe
    bool GetFaceToNeighborFaces(const IGsize faceId,
                                ReturnContainer& faceIds); // Safe

    // Get all faces within one ring of a face (shared point). Return the size of
    // indices.
    int GetFaceToOneRingFaces(const IGsize faceId,
                              igIndex* faceIds); // Unsafe: Stake overflow
    bool GetFaceToOneRingFaces(const IGsize faceId,
                               IdArray::Pointer faceIds); // Safe
    bool GetFaceToOneRingFaces(const IGsize faceId,
                               ReturnContainer& faceIds); // Safe

    // Get edge index according to two point index. If don't, return index -1
    igIndex GetEdgeIdFormPointIds(const IGsize ptId1, const IGsize ptId2);
    // Get face index according to sequence. If don't, return index -1
    igIndex GetFaceIdFormPointIds(igIndex* ids, int size);

    // Request data edit state, only in this state,
    // can perform the adding and delete operation.
    // Adding operations also can be done via GetFaces().
    void RequestEditStatus() override;

    // Garbage collection to free memory that has been removed.
    // This function must be called if the topology changes.
    void GarbageCollection() override;

    // Whether edge is deleted or not
    bool IsEdgeDeleted(const IGsize edgeId);
    // Whether face is deleted or not
    bool IsFaceDeleted(const IGsize faceId);

    // Add element, necessarily called after RequestEditStatus()
    IGsize AddPoint(const Point& p) override;
    virtual IGsize AddEdge(const IGsize ptId1, const IGsize ptId2);
    virtual IGsize AddFace(igIndex* ptIds, int size);

    // Delete element, necessarily called after RequestEditStatus()
    void DeletePoint(const IGsize ptId) override;
    virtual void DeleteEdge(const IGsize edgeId);
    virtual void DeleteFace(const IGsize faceId);

    void ReplacePointReference(const IGsize fromPtId, const IGsize toPtId);
    virtual bool IsBoundaryFace(const IGsize faceId);
    virtual bool IsBoundaryEdge(const IGsize edgeId);
    virtual bool IsBoundaryPoint(const IGsize ptId);
    virtual bool IsCornerPoint(const IGsize ptId);

    //Get real size of DataObject
    IGsize GetRealMemorySize() override;
    bool GetClipped() override { return true; }

    //void SetFaceColor(const float color[3]);
    //const float* GetFaceColor() const;
    //void SetFaceTransparency(float val);
    //float GetFaceTransparency() const;

    bool IsCollapsable(igIndex edgeId) {
        if (IsEdgeDeleted(edgeId)) { return false; }

        igIndex e[2]{};
        GetEdgePointIds(edgeId, e);

        // 如果两个顶点在边界，边却没在边界，则不能坍缩
        if (IsBoundaryPoint(e[0]) && IsBoundaryPoint(e[1]) &&
            !IsBoundaryEdge(edgeId)) {
            return false;
        }

        // 只有两个顶点的邻接顶点的交集个数小于2，才能坍缩
        SurfaceMesh::ReturnContainer v1, v2;
        GetPointToOneRingPoints(e[0], v1);
        GetPointToOneRingPoints(e[1], v2);
        int count = 0;
        for (int i = 0; i < v1.size(); i++) {
            for (int j = 0; j < v2.size(); j++) {
                if (v1[i] == v2[j]) { count++; }
            }
        }
        return (count <= 2);
    }

    void CollapseEdge(igIndex edgeId) {
        assert(IsCollapsable(edgeId) &&
               "Please call IsCollapsable before CollapseEdge");

        igIndex e[2]{};
        GetEdgePointIds(edgeId, e);
        igIndex from = e[0];
        igIndex to = e[1];

        if (GetNumberOfLinks(from, SurfaceMesh::P2E) == 1) {
            DeletePoint(from);
            return;
        }
        if (GetNumberOfLinks(to, SurfaceMesh::P2E) == 1) {
            DeletePoint(to);
            return;
        }

        auto GetOppoFaceId = [&](igIndex edgeId, igIndex faceId) -> igIndex {
            igIndex ids[2]{};
            int size = GetEdgeToNeighborFaces(edgeId, ids);
            if (size < 2) { return -1; }
            return ids[0] + ids[1] - faceId;
        };

        struct ColFace {
            igIndex col_edgeId; // 坍缩的边id
            igIndex oppo_ptId;  // 坍缩的边对应的顶点id
            igIndex faceId;     // 坍缩边的邻接面id
            igIndex from_edgeId, from_oppo_faceId; // 非坍缩边以及其对偶面
            igIndex to_edgeId, to_oppo_faceId;
        };

        igIndex ids[2]{}, fpoint[3]{}, fedge[3]{};
        int size = GetEdgeToNeighborFaces(edgeId, ids);
        for (int i = 0; i < size; i++) {
            ColFace f;
            f.faceId = ids[i];
            GetFacePointIds(f.faceId, fpoint);
            GetFaceEdgeIds(f.faceId, fedge);

            // 该边对应的顶点id
            f.oppo_ptId = fpoint[0] + fpoint[1] + fpoint[2] - from - to;

            int k = 0;
            igIndex edge[2]{};
            for (int j = 0; j < 3; j++) {
                if (fedge[j] != edgeId) {
                    GetEdgePointIds(fedge[j], edge);
                    if (edge[0] == from || edge[1] == from) {
                        f.from_edgeId = fedge[j];
                        f.from_oppo_faceId = GetOppoFaceId(fedge[j], f.faceId);
                    } else /* if (edge[0] == to || edge[1] == to) */ {
                        f.to_edgeId = fedge[j];
                        f.to_oppo_faceId = GetOppoFaceId(fedge[j], f.faceId);
                    }
                }
            }
            assert(k == 2);

            if (f.from_oppo_faceId != -1) {
                m_FaceEdgeLinks->ReplaceReference(f.to_edgeId, f.faceId,
                                                  f.from_oppo_faceId);
                m_FaceEdges->ReplaceCellReference(f.from_oppo_faceId,
                                                  f.from_edgeId, f.to_edgeId);
            } else {
                m_FaceEdgeLinks->RemoveReference(f.to_edgeId, f.faceId);
            }


            m_FaceLinks->RemoveReference(f.oppo_ptId, f.faceId);
            m_FaceLinks->RemoveReference(from, f.faceId);
            m_FaceLinks->RemoveReference(to, f.faceId);

            m_EdgeLinks->RemoveReference(f.oppo_ptId, f.from_edgeId);
            m_EdgeLinks->RemoveReference(from, f.from_edgeId);

            m_EdgeDeleteMarker->MarkDeleted(f.from_edgeId);
            m_FaceDeleteMarker->MarkDeleted(f.faceId);
        }
        m_EdgeLinks->RemoveReference(from, edgeId);
        m_EdgeLinks->RemoveReference(to, edgeId);

        ReturnContainer container;
        GetPointToNeighborFaces(from, container);
        for (int i = 0; i < container.size(); i++) {
            m_Faces->ReplaceCellReference(container[i], from, to);
        }
        GetPointToNeighborEdges(from, container);
        for (int i = 0; i < container.size(); i++) {
            m_Edges->ReplaceCellReference(container[i], from, to);
        }

        const igIndex* ptr = m_FaceLinks->GetLinkPointer(from);
        for (int i = 0; i < m_FaceLinks->GetLinkSize(from); i++) {
            m_FaceLinks->AddReference(to, ptr[i]);
        }

        ptr = m_EdgeLinks->GetLinkPointer(from);
        for (int i = 0; i < m_EdgeLinks->GetLinkSize(from); i++) {
            m_EdgeLinks->AddReference(to, ptr[i]);
        }

        m_EdgeDeleteMarker->MarkDeleted(edgeId);
        m_PointDeleteMarker->MarkDeleted(from);
    }

protected:
    SurfaceMesh();
    ~SurfaceMesh() override = default;

    void RequestEdgeStatus();
    void RequestFaceStatus();

    DeleteMarker::Pointer m_EdgeDeleteMarker{};
    DeleteMarker::Pointer m_FaceDeleteMarker{};

    CellArray::Pointer m_Edges{};     // The point set of edges
    CellLinks::Pointer m_EdgeLinks{}; // The adjacent edges of points

    CellArray::Pointer m_Faces{};     // The point set of faces
    CellLinks::Pointer m_FaceLinks{}; // The adjacent faces of points

    CellArray::Pointer m_FaceEdges{};     // The edge set of faces
    CellLinks::Pointer m_FaceEdgeLinks{}; // The adjacent faces of edges

    //float m_FaceColor[3]{1.0f, 1.0f, 1.0f}; // The color of the mesh
    //float m_FaceTransparency{1.0f};         // Transparency of the faces
private:
    // Used for the returned cell object, which is Thread-Unsafe
    Line::Pointer m_Edge{};
    Triangle::Pointer m_Triangle{};
    Quad::Pointer m_Quad{};
    Polygon::Pointer m_Polygon{};

public:
    //void Draw(Scene*) override;
    void ConvertToDrawableData() override;
    //void SetDisplayMesh(SurfaceMesh::Pointer& surfaceMesh);
    //void ViewCloudPicture(Scene* scene, int index, int demension = -1) override;
    //void SetAttributeWithPointData(ArrayObject::Pointer attr,
    //                               std::pair<float, float>& range,
    //                               igIndex i = -1) override;
    void SetAttributeWithCellData(ArrayObject::Pointer attr,
                                  std::pair<float, float>& range,
                                  igIndex dimension = -1) override;

    void GetDrawableArray(FloatArray::Pointer& positions,
                          UnsignedIntArray::Pointer& lineIndices,
                          UnsignedIntArray::Pointer& triangleIndices);

    //protected:
    //    SurfaceMesh::Pointer m_DrawMesh{nullptr};
};
IGAME_NAMESPACE_END
#endif