#ifndef iGameVolumeMesh_h
#define iGameVolumeMesh_h

#include "iGameSurfaceMesh.h"

#include "iGameHexahedron.h"
#include "iGamePolyhedron.h"
#include "iGamePrism.h"
#include "iGamePyramid.h"
#include "iGameTetra.h"

IGAME_NAMESPACE_BEGIN
class VolumeMesh : public SurfaceMesh {
public:
    I_OBJECT(VolumeMesh);
    static Pointer New() { return new VolumeMesh; }
    using ReturnContainer = FlexArray<igIndex, 128>;

    IGenum GetDataObjectType() const override { return IG_VOLUME_MESH; }
    // Get the number of all volumes
    IGsize GetNumberOfVolumes() const noexcept;

    // Get/Set volume array
    CellArray* GetVolumes();
    void SetVolumes(CellArray::Pointer volumes);

    // Get edge cell by index volumeId. Thread-Unsafe, please use GetVolumePointIds and GetVolumeEdgeIds and GetVolumeFaceIds
    Volume* GetVolume(const IGsize volumeId);

    // Get volume's point index. Return PointIds size
    int GetVolumePointIds(const IGsize volumeId, igIndex* ptIds);
    // Get volume's edge index. Return EdgeIds size
    int GetVolumeEdgeIds(const IGsize volumeId, igIndex* edgeIds);
    // Get volume's face index. Return FaceIds size
    int GetVolumeFaceIds(const IGsize volumeId, igIndex* faceIds);

    // Construct all the faces and add the face index to VolumeFaces
    void BuildFaces();
    // Construct all the faces and edges. Add the face index to VolumeFaces.
    // Add the edge index to VolumeEdges,
    void BuildFacesAndEdges();
    // Construct the adjacent volumes of the points
    void BuildVolumeLinks();
    // Construct the adjacent volumes of the edges
    void BuildVolumeEdgeLinks();
    // Construct the adjacent volumes of the faces
    void BuildVolumeFaceLinks();

    int GetNumberOfLinks(const IGsize id, Type type);
    // Get all neighboring volumes of a point. Return the size of indices.
    int GetPointToNeighborVolumes(const IGsize ptId,
                                  igIndex* volumeIds); // Unsafe: Stake overflow
    bool GetPointToNeighborVolumes(const IGsize ptId, const igIndex*& volumeIds,
                                   int& size); // Safe
    bool GetPointToNeighborVolumes(const IGsize ptId,
                                   IdArray::Pointer volumeIds); // Safe
    bool GetPointToNeighborVolumes(const IGsize ptId,
                                   ReturnContainer& volumeIds); // Safe

    // Get all neighboring volumes of a edge. Return the size of indices.
    int GetEdgeToNeighborVolumes(const IGsize edgeId,
                                 igIndex* volumeIds);                                         // Unsafe: Stake overflow
    bool GetEdgeToNeighborVolumes(const IGsize edgeId, const igIndex*& volumeIds, int& size); // Safe
    bool GetEdgeToNeighborVolumes(const IGsize edgeId,
                                  IdArray::Pointer volumeIds); // Safe
    bool GetEdgeToNeighborVolumes(const IGsize edgeId,
                                  ReturnContainer& volumeIds); // Safe

    // Get all neighboring volumes of a face. Return the size of indices.
    int GetFaceToNeighborVolumes(const IGsize faceId,
                                 igIndex* volumeIds);                                         // Unsafe: Stake overflow
    bool GetFaceToNeighborVolumes(const IGsize faceId, const igIndex*& volumeIds, int& size); // Safe
    bool GetFaceToNeighborVolumes(const IGsize faceId,
                                  IdArray::Pointer volumeIds); // Safe
    bool GetFaceToNeighborVolumes(const IGsize faceId,
                                  ReturnContainer& volumeIds); // Safe

    // Get all neighboring volumes of a volume (Shared point). Return the size of indices.
    int GetVolumeToNeighborVolumesWithPoint(const IGsize volumeId,
                                            igIndex* volumeIds); // Unsafe: Stake overflow
    bool GetVolumeToNeighborVolumesWithPoint(const IGsize volumeId,
                                             IdArray::Pointer volumeIds); // Safe
    bool GetVolumeToNeighborVolumesWithPoint(const IGsize volumeId,
                                             ReturnContainer& volumeIds); // Safe

    // Get all neighboring volumes of a volume (Shared edge). Return the size of indices.
    int GetVolumeToNeighborVolumesWithEdge(const IGsize volumeId,
                                           igIndex* volumeIds); // Unsafe: Stake overflow
    bool GetVolumeToNeighborVolumesWithEdge(const IGsize volumeId,
                                            IdArray::Pointer volumeIds); // Safe
    bool GetVolumeToNeighborVolumesWithEdge(const IGsize volumeId,
                                            ReturnContainer& volumeIds); // Safe

    // Get all neighboring volumes of a volume (Shared face). Return the size of indices.
    int GetVolumeToNeighborVolumesWithFace(const IGsize volumeId,
                                           igIndex* volumeIds); // Unsafe: Stake overflow
    bool GetVolumeToNeighborVolumesWithFace(const IGsize volumeId,
                                            IdArray::Pointer volumeIds); // Safe
    bool GetVolumeToNeighborVolumesWithFace(const IGsize volumeId,
                                            ReturnContainer& volumeIds); // Safe

    // Get volume index according to sequence. If don't, return index -1
    igIndex GetVolumeIdFormPointIds(igIndex* ids, int size);

    // Request data edit state, only in this state,
    // can perform the adding and delete operation.
    // Adding operations also can be done via GetVolumes().
    void RequestEditStatus() override;

    // Garbage collection to free memory that has been removed.
    // This function must be called if the topology changes.
    void GarbageCollection() override;
    void GarbageCollection(bool delete_isolated_element);

    // Whether volume is deleted or not
    bool IsVolumeDeleted(const IGsize volumeId);

    // Add element, necessarily called after RequestEditStatus()
    IGsize AddPoint(const Point& p) override;
    IGsize AddPoint(const double x, const double y, const double z) { return AddPoint(Point(x, y, z)); }
    IGsize AddEdge(const IGsize ptId1, const IGsize ptId2) override;
    IGsize AddFace(igIndex* ptIds, int size) override;
    IGsize AddVolume(igIndex* volumeIds, int size);

    bool ReplaceEdgeReference(const IGsize edgeId, igIndex ptId1, igIndex ptId2);
    bool ReplaceFaceReference(const IGsize faceId, igIndex* ptIds, int npts, igIndex* edgeIds);
    bool ReplaceVolumeReference(const IGsize volumeId, igIndex* ptIds, int npts, igIndex* edgeIds, int nedges,
                                igIndex* faceIds, int nfaces);

    // Delete element, necessarily called after RequestEditStatus()
    void DeletePoint(const IGsize ptId) override;
    void DeleteEdge(const IGsize edgeId) override;
    void DeleteFace(const IGsize faceId) override;
    void DeleteVolume(const IGsize volumeId);

    CellArray* GetCells() { return this->GetVolumes(); };
    void GetCell(igIndex cellId, Cell::Pointer& cell) { cell = this->GetVolume(cellId); };
    Cell::Pointer GetCell(igIndex cellId) { return this->GetVolume(cellId); }
    igIndex GetCellDimension(igIndex CellTyp) { return 3; };
    //SurfaceMesh::Pointer GetDrawMesh() { return m_DrawMesh; }

    bool IsBoundaryPoint(const IGsize ptId) override;
    bool IsBoundaryEdge(const IGsize edgeId) override;
    bool IsBoundaryFace(const IGsize faceId) override;
    bool IsBoundaryVolume(const IGsize volumeId);
    bool IsCornerPoint(const IGsize ptId) override;

    void ReplacePointReference(const IGsize fromPtId, const IGsize toPtId) {
        if (fromPtId == toPtId) { return; }
        if (!InEditStatus()) { RequestEditStatus(); }
        igIndex edgeIds[64]{}, faceIds[64]{}, volumeIds[64]{};
        int size1 = GetPointToNeighborEdges(fromPtId, edgeIds);
        int size2 = GetPointToNeighborFaces(fromPtId, faceIds);
        int size3 = GetPointToNeighborVolumes(fromPtId, volumeIds);
        for (int i = 0; i < size1; i++) { m_Edges->ReplaceCellReference(edgeIds[i], fromPtId, toPtId); }
        for (int i = 0; i < size2; i++) { m_Faces->ReplaceCellReference(faceIds[i], fromPtId, toPtId); }
        for (int i = 0; i < size3; i++) { m_Volumes->ReplaceCellReference(volumeIds[i], fromPtId, toPtId); }

        auto& link1 = m_EdgeLinks->GetLink(fromPtId);
        m_EdgeLinks->SetLink(toPtId, link1.pointer, link1.size);

        auto& link2 = m_FaceLinks->GetLink(fromPtId);
        m_FaceLinks->SetLink(toPtId, link2.pointer, link2.size);

        auto& link3 = m_VolumeLinks->GetLink(fromPtId);
        m_VolumeLinks->SetLink(toPtId, link3.pointer, link3.size);
    }
    void PrintSelf() {
        igIndex ids[64]{};

        std::cout << "PointToNeighborEdges\n";
        for (int i = 0; i < GetNumberOfPoints(); i++) {
            int size = GetPointToNeighborEdges(i, ids);
            std::cout << "Point: " << i << ", ";
            for (int j = 0; j < size; j++) { std::cout << " " << ids[j]; }
            std::cout << "\n";
        }

        std::cout << "EdgeToNeighborFaces\n";
        for (int i = 0; i < GetNumberOfFaces(); i++) {
            int size = GetEdgeToNeighborFaces(i, ids);
            std::cout << "Edge: " << i << ", ";
            for (int j = 0; j < size; j++) { std::cout << " " << ids[j]; }
            std::cout << "\n";
        }

        std::cout << "FaceToNeighborVolumes\n";
        for (int i = 0; i < GetNumberOfVolumes(); i++) {
            int size = GetFaceToNeighborVolumes(i, ids);
            std::cout << "Face: " << i << ", ";
            for (int j = 0; j < size; j++) { std::cout << " " << ids[j]; }
            std::cout << "\n";
        }
    }

    void InitPolyhedronVertices() {
        EdgeTable::Pointer EdgeTable = EdgeTable::New();
        m_Volumes = CellArray::New();
        m_VolumeEdges = CellArray::New();
        m_FaceEdges = CellArray::New();
        igIndex CellNum = this->m_VolumeFaces->GetNumberOfCells();
        igIndex ptIds[IGAME_CELL_MAX_SIZE]{}, edgeIds[IGAME_CELL_MAX_SIZE]{}, faceIds[IGAME_CELL_MAX_SIZE]{};
        IGsize npts, nedges;
        for (igIndex i = 0; i < CellNum; i++) {
            std::set<igIndex> vset;
            int fsize = m_VolumeFaces->GetCellIds(i, faceIds);
            for (int j = 0; j < fsize; j++) {
                int size = m_Faces->GetCellIds(faceIds[j], ptIds);
                for (int k = 0; k < size; k++) { vset.insert(ptIds[k]); }
            }
            npts = 0;
            for (auto it: vset) { ptIds[npts++] = it; }
            m_Volumes->AddCellIds(ptIds, npts);
        }
        for (igIndex i = 0; i < CellNum; i++) {
            std::set<igIndex> eset;
            int fsize = m_VolumeFaces->GetCellIds(i, faceIds);
            for (int j = 0; j < fsize; j++) {
                int size = m_Faces->GetCellIds(faceIds[j], ptIds);
                for (int k = 0; k < size; k++) {
                    igIndex idx;
                    if ((idx = EdgeTable->IsEdge(ptIds[k], ptIds[(k + 1) % size])) == -1) {
                        idx = EdgeTable->GetNumberOfEdges();
                        EdgeTable->InsertEdge(ptIds[k], ptIds[(k + 1) % size]);
                    }
                    eset.insert(idx);
                }
            }
            nedges = 0;
            for (auto it: eset) { edgeIds[nedges++] = it; }
            m_VolumeEdges->AddCellIds(edgeIds, nedges);
        }
        for (IGsize i = 0; i < m_Faces->GetNumberOfCells(); i++) {
            int size = m_Faces->GetCellIds(i, ptIds);
            for (int j = 0; j < size; j++) {
                igIndex idx;
                if ((idx = EdgeTable->IsEdge(ptIds[j], ptIds[(j + 1) % size])) == -1) { std::cerr << "error!"; }
                edgeIds[j] = idx;
            }
            m_FaceEdges->AddCellIds(edgeIds, size);
        }
        m_Edges = EdgeTable->GetOutput();
        if (shouldBuildEageLinks)
        BuildEdgeLinks();
        if (shouldBuildFaceLinks)
        BuildFaceLinks();
        if (shouldBuildFaceEageLinks)
        BuildFaceEdgeLinks();
        if (shouldBuildVolumeEageLinks)
        BuildVolumeEdgeLinks();
        if (shouldBuildVolumeFaceLinks)
        BuildVolumeFaceLinks();
        if (shouldBuildVolumeLinks)
        BuildVolumeLinks();
    }
    void InitVolumesWithPolyhedron(CellArray::Pointer faces, CellArray::Pointer VolumeFaces) {
        m_VolumeFaces = VolumeFaces;
        m_Faces = faces;
        m_Volumes = CellArray::New();
        igIndex CellNum = this->m_VolumeFaces->GetNumberOfCells();
        igIndex ptIds[IGAME_CELL_MAX_SIZE]{}, faceIds[IGAME_CELL_MAX_SIZE]{};
        IGsize npts = 0;
        for (igIndex i = 0; i < CellNum; i++) {
            std::set<igIndex> vset;
            int fsize = m_VolumeFaces->GetCellIds(i, faceIds);
            for (int j = 0; j < fsize; j++) {
                int size = m_Faces->GetCellIds(faceIds[j], ptIds);
                for (int k = 0; k < size; k++) { vset.insert(ptIds[k]); }
            }
            npts = 0;
            for (auto it: vset) { ptIds[npts++] = it; }
            m_Volumes->AddCellIds(ptIds, npts);
        }
        this->IsPolyhedronType = true;
    }
    bool GetIsPolyhedronType() { return this->IsPolyhedronType; }
    //Get real size of DataObject
    IGsize GetRealMemorySize() override;
    bool GetClipped() override { return true; }

    void ClearAllLinks() { 
        m_VolumeLinks = nullptr;
        m_VolumeEdges = nullptr;
        m_VolumeEdgeLinks = nullptr;
        m_VolumeFaces = nullptr;
        m_VolumeFaceLinks = nullptr;

        m_Faces = nullptr;
        m_FaceLinks = nullptr;
        m_FaceEdges = nullptr;
        m_FaceEdgeLinks = nullptr;
        m_Edges = nullptr;
        m_EdgeLinks = nullptr;
    }

    static IGenum GetVolumeTypeWithPointNum(int npts) {
        switch (npts) {
            case 4:
                return IG_TETRA;
            case 5:
                return IG_PYRAMID;
            case 6:
                return IG_PRISM;
            case 8:
                return IG_HEXAHEDRON;
            default:
                return IG_EMPTY_CELL;
        }
    }

protected:
    VolumeMesh();
    ~VolumeMesh() override = default;

    void RequestFaceStatus();
    void RequestVolumeStatus();

    DeleteMarker::Pointer m_VolumeDeleteMarker{};

    CellArray::Pointer m_Volumes{};     // The point set of volumes
    CellLinks::Pointer m_VolumeLinks{}; // The adjacent volumes of points

    CellArray::Pointer m_VolumeEdges{};     // The edge set of volumes
    CellLinks::Pointer m_VolumeEdgeLinks{}; // The adjacent volumes of edges

    CellArray::Pointer m_VolumeFaces{};     // The face set of volumes
    CellLinks::Pointer m_VolumeFaceLinks{}; // The adjacent volumes of faces
    bool IsPolyhedronType = false;

private:
    Tetra::Pointer m_Tetra{};           // Used for the returned 'Tetra' object, which is Thread-Unsafe
    Hexahedron::Pointer m_Hexahedron{}; // Used for the returned 'Hexahedron' object, which is Thread-Unsafe
    Prism::Pointer m_Prism{};           // Used for the returned 'Prism' object, which is Thread-Unsafe
    Pyramid::Pointer m_Pyramid{};       // Used for the returned 'Pyramid' object, which is Thread-Unsafe
    Polyhedron::Pointer m_Polyhedron{};
    bool shouldBuildEageLinks = true;
    bool shouldBuildFaceLinks = true;
    bool shouldBuildFaceEageLinks = true;
    bool shouldBuildVolumeEageLinks = true;
    bool shouldBuildVolumeFaceLinks = true;
    bool shouldBuildVolumeLinks = true;

public:
    //void Draw(Scene*) override;
    void SetShouldBuildEageLinks(bool Should) { shouldBuildEageLinks = Should; };
    void SetShouldBuildFaceLinks(bool Should) { shouldBuildFaceLinks = Should; };
    void SetShouldBuildFaceEageLinks(bool Should) { shouldBuildFaceEageLinks = Should; };
    void SetShouldBuildVolumeEageLinks(bool Should) { shouldBuildVolumeEageLinks=Should; };
    void SetShouldBuildVolumeFaceLinks(bool Should) { shouldBuildVolumeFaceLinks = Should; };
    void SetShouldBuildVolumeLinks(bool Should) { shouldBuildVolumeFaceLinks = Should; };

    void ConvertToDrawableData() override;
    //void ViewCloudPicture(Scene* scene, int index, int demension = -1) override;

    //void SetAttributeWithPointData(ArrayObject::Pointer attr,
    //                               std::pair<float, float>& range,
    //                               igIndex i = -1) override;

    void SetAttributeWithCellData(ArrayObject::Pointer attr, DoubleArray::Pointer attrRange,
                                  igIndex dimension = -1) override;
};
IGAME_NAMESPACE_END
#endif