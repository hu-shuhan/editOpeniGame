#include "iGameModelGeometryFilter.h"
#include "Convert/iGameConvertToSurfaceMeshFilter.h"
#include "Mutex/iGameAtomicMutex.h"
#include "iGameThreadPool.h"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#ifndef __EMSCRIPTEN__
#include <omp.h>
#endif
#include <stdexcept>
IGAME_NAMESPACE_BEGIN
#define ArrayList std::vector<ArrayObject>
ModelGeometryFilter::ModelGeometryFilter() {
    this->PointMinimum = 0;
    this->PointMaximum = INT_MAX;

    this->CellMinimum = 0;
    this->CellMaximum = INT_MAX;

    this->Extent[0] = -DBL_MAX;
    this->Extent[1] = DBL_MAX;
    this->Extent[2] = -DBL_MAX;
    this->Extent[3] = DBL_MAX;
    this->Extent[4] = -DBL_MAX;
    this->Extent[5] = DBL_MAX;
    this->PlaneOrigin[0] = 0;
    this->PlaneOrigin[1] = 0;
    this->PlaneOrigin[2] = 0;
    this->PlaneNormal[0] = 1;
    this->PlaneNormal[1] = 0;
    this->PlaneNormal[2] = 0;
    this->PointClipping = false;
    this->CellClipping = false;
    this->ExtentClipping = false;
    this->ExtentClippingFlip = false;
    this->PlaneClipping = false;
    this->PlaneClippingFlip = false;

    this->Merging = true;

    this->RemoveGhostInterfaces = true;

    this->m_PointMap = nullptr;
}
ModelGeometryFilter::~ModelGeometryFilter() {
    this->m_PointMap = nullptr;
    this->input = nullptr;
    this->output = nullptr;
    this->excFaces = nullptr;
}
void ModelGeometryFilter::SetExtent(double xMin, double xMax, double yMin, double yMax, double zMin, double zMax,
                                    bool flip) {
    double extent[6] = {xMin, xMax, yMin, yMax, zMin, zMax};
    this->SetExtent(extent, flip);
}
void ModelGeometryFilter::SetExtent(double extent[6], bool flip) {
    int i;
    bool needSet = false;
    for (i = 0; i < 6; i++) { needSet |= extent[i] != this->Extent[i]; }
    if (needSet) {
        this->Modified();
        for (i = 0; i < 3; i++) {
            if (extent[2 * i + 1] < extent[2 * i]) { std::swap(extent[2 * i + 1], extent[2 * i]); }
            this->Extent[2 * i] = extent[2 * i];
            this->Extent[2 * i + 1] = extent[2 * i + 1];
        }
    }
    this->ExtentClippingFlip = flip;
    this->SetExtentClipping(true);
}
void ModelGeometryFilter::SetClipPlane(double ox, double oy, double oz, double nx, double ny, double nz, bool flip) {
    this->PlaneOrigin[0] = ox;
    this->PlaneOrigin[1] = oy;
    this->PlaneOrigin[2] = oz;
    this->PlaneNormal[0] = nx;
    this->PlaneNormal[1] = ny;
    this->PlaneNormal[2] = nz;
    this->PlaneClippingFlip = flip;
    this->SetPlaneClipping(true);
}
void ModelGeometryFilter::SetClipPlane(double orgin[3], double normal[3], bool flip) {
    this->SetClipPlane(orgin[0], orgin[1], orgin[2], normal[0], normal[1], normal[2], flip);
}
void ModelGeometryFilter::SetPointIndexExtent(igIndex _min, igIndex _max) {
    if (_min > _max) { std::swap(_min, _max); }
    this->PointMinimum = _min;
    this->PointMaximum = _max;
    SetPointClipping(true);
}
void ModelGeometryFilter::SetPointIndexMinimum(igIndex _min) {
    this->PointMinimum = _min;
    this->PointMaximum = std::max(_min, this->PointMaximum);
    SetPointClipping(true);
}
void ModelGeometryFilter::SetPointIndexMaximum(igIndex _max) {
    this->PointMinimum = std::min(_max, this->PointMinimum);
    this->PointMaximum = _max;
    SetPointClipping(true);
}
void ModelGeometryFilter::SetCellIndexExtent(igIndex _min, igIndex _max) {
    if (_min > _max) { std::swap(_min, _max); }
    this->CellMinimum = _min;
    this->CellMaximum = _max;
    SetCellClipping(true);
}
void ModelGeometryFilter::SetCellIndexMinimum(igIndex _min) {
    this->CellMinimum = _min;
    this->CellMaximum = std::max(_min, this->PointMaximum);
    SetCellClipping(true);
}
void ModelGeometryFilter::SetCellIndexMaximum(igIndex _max) {
    this->CellMinimum = std::min(_max, this->PointMinimum);
    this->CellMaximum = _max;
    SetCellClipping(true);
}

bool ModelGeometryFilter::Execute() {

    Execute(this->input);
    return true;
}
bool ModelGeometryFilter::Execute(DataObject::Pointer input) {

    this->output = SurfaceMesh::New();
    return Execute(input, output);
}

bool ModelGeometryFilter::Execute(DataObject::Pointer input, SurfaceMesh::Pointer& output) {
    if (!input) {
        output = nullptr;
        return false;
    }
    if (!output) { output = SurfaceMesh::New(); }
    switch (input->GetDataObjectType()) {
        case IG_NONE:
            return true;
        case IG_VOLUME_MESH:
            return this->ExecuteWithVolumeMesh(input, output, excFaces);
        case IG_SURFACE_MESH:
            return this->ExecuteWithSurfaceMesh(input, output, excFaces);
        case IG_UNSTRUCTURED_MESH:
            return this->ExecuteWithUnstructuredMesh(input, output, excFaces);
        case IG_STRUCTURED_MESH:
            return this->ExecuteWithStructuredMesh(input, output, excFaces);
        default:
            break;
    }
    return true;
}

class FaceMemoryPool;

class GFace {
public:
    GFace* Next = nullptr;
    FaceMemoryPool* Owner = nullptr;
    //父亲cell，用于对cell attribute的map
    igIndex OriginalCellId = 0;
    igIndex* PointIds = nullptr;
    int NumberOfPoints = 0;
    //是否是幽灵面
    bool IsGhost = false;

    GFace() = default;
    GFace(const igIndex& originalCellId, const igIndex& numberOfPoints, const bool& isGhost)
        : OriginalCellId(static_cast<igIndex>(originalCellId)), NumberOfPoints(numberOfPoints), IsGhost(isGhost) {}

    bool operator==(const GFace& other) const {
        if (this->NumberOfPoints != other.NumberOfPoints) { return false; }
        switch (this->NumberOfPoints) {
            case 3: {
                return this->PointIds[0] == other.PointIds[0] &&
                       ((this->PointIds[1] == other.PointIds[2] && this->PointIds[2] == other.PointIds[1]) ||
                        (this->PointIds[1] == other.PointIds[1] && this->PointIds[2] == other.PointIds[2]));
            }
            case 4: {
                return this->PointIds[0] == other.PointIds[0] && this->PointIds[2] == other.PointIds[2] &&
                       ((this->PointIds[1] == other.PointIds[3] && this->PointIds[3] == other.PointIds[1]) ||
                        (this->PointIds[1] == other.PointIds[1] && this->PointIds[3] == other.PointIds[3]));
            }
            default: {
                bool match = true;
                if (this->PointIds[0] == other.PointIds[0]) {
                    // if the first two points match loop through forwards
                    // checking all points
                    if (this->NumberOfPoints > 1 && this->PointIds[1] == other.PointIds[1]) {
                        for (auto i = 2; i < this->NumberOfPoints; ++i) {
                            if (this->PointIds[i] != other.PointIds[i]) {
                                match = false;
                                break;
                            }
                        }
                    } else {
                        // check if the points go in the opposite direction
                        for (auto i = 1; i < this->NumberOfPoints; ++i) {
                            if (this->PointIds[this->NumberOfPoints - i] != other.PointIds[i]) {
                                match = false;
                                break;
                            }
                        }
                    }
                } else {
                    match = false;
                }
                return match;
            }
        }
    }
    bool operator!=(const GFace& other) const { return !(*this == other); }
    [[nodiscard]] int GetSize() const noexcept { return NumberOfPoints; }
};

inline std::uint64_t MixFacePointKey(const igIndex pointId) noexcept {
    auto value = static_cast<std::uint64_t>(pointId);
    value ^= value >> 30u;
    value *= 0xbf58476d1ce4e5b9ULL;
    value ^= value >> 27u;
    value *= 0x94d049bb133111ebULL;
    value ^= value >> 31u;
    return value;
}

inline std::size_t FinishFaceKey(const std::uint64_t key) noexcept {
    return static_cast<std::size_t>(key ^ (key >> 32u));
}


template<int Fcnt>
class StaticFace : public GFace {
private:
    std::array<igIndex, Fcnt> PointIdsContainer{};
    std::size_t HashKey{0u};

public:
    StaticFace(const igIndex& originalCellId, const igIndex* pointIds, const bool& isGhost)
        : GFace(originalCellId, Fcnt, isGhost) {
        this->PointIds = this->PointIdsContainer.data();
        this->Initialize(pointIds);
    }

    inline static constexpr int GetSize() { return Fcnt; }
    [[nodiscard]] std::size_t GetHashKey() const noexcept { return HashKey; }

    void Initialize(const igIndex* pointIds) {
        int offset = 0;
        std::uint64_t key = static_cast<std::uint64_t>(Fcnt);
        for (int index = 0; index < Fcnt; ++index) {
            if (pointIds[index] < pointIds[offset]) { offset = index; }
            key += MixFacePointKey(pointIds[index]);
        }
        HashKey = FinishFaceKey(key);
        for (int index = 0; index < Fcnt; ++index) {
            this->PointIds[index] = static_cast<igIndex>(pointIds[(offset + index) % Fcnt]);
        }
    }
};

class DynamicFace : public GFace {
private:
    std::vector<igIndex> PointIdsContainer;

public:
    DynamicFace(const igIndex& originalCellId, const igIndex& numberOfPoints, const igIndex* pointIds,
                const bool& isGhost)
        : GFace(originalCellId, numberOfPoints, isGhost) {
        assert(this->NumberOfPoints != 0);
        std::cout << NumberOfPoints << std::endl;
        this->PointIdsContainer.resize(static_cast<size_t>(this->NumberOfPoints));
        this->PointIds = this->PointIdsContainer.data();
        this->Initialize(pointIds);
    }

    inline int GetSize() const { return this->NumberOfPoints; }

    void Initialize(const igIndex* pointIds) {
        // find the index to the smallest id
        int offset = 0;
        int index;
        for (index = 1; index < this->NumberOfPoints; ++index) {
            if (pointIds[index] < pointIds[offset]) { offset = index; }
        }
        // copy ids into ordered array with the smallest id first
        for (index = 0; index < this->NumberOfPoints; ++index) {
            this->PointIds[index] = static_cast<igIndex>(pointIds[(offset + index) % this->NumberOfPoints]);
        }
    }
};


using GTriangle = StaticFace<3>;
using GQuad = StaticFace<4>;
using GPentagon = StaticFace<5>;
using GHexagon = StaticFace<6>;
using GHeptagon = StaticFace<7>;
using GOctagon = StaticFace<8>;
using GNonagon = StaticFace<9>;
using GDecagon = StaticFace<10>;
using GPolygon = DynamicFace;

class FaceMemoryPool {
private:
    static constexpr std::size_t ChunkBytes = 4u * 1024u * 1024u;
    std::size_t NumberOfArrays;
    std::size_t ArrayLength;
    std::size_t NextArrayIndex;
    std::size_t NextFaceIndex;
    unsigned char** Arrays;
    std::vector<GFace*> FreeFaces;
    std::mutex Mutex;

    inline static std::size_t SizeofFace(const int& numberOfPoints) {
        const auto rawSize = sizeof(GFace) + static_cast<std::size_t>(numberOfPoints) * sizeof(igIndex);
        const auto alignment = alignof(GFace);
        return (rawSize + alignment - 1u) / alignment * alignment;
    }

    void EnsureFreeFaceList(const int numberOfPoints) {
        if (numberOfPoints < 0) { return; }
        const auto requiredSize = static_cast<std::size_t>(numberOfPoints) + 1u;
        if (requiredSize > FreeFaces.size()) { FreeFaces.resize(requiredSize, nullptr); }
    }

    GFace* AllocateUnlocked(const int numberOfPoints) {
        EnsureFreeFaceList(numberOfPoints);
        if (numberOfPoints >= 0 && FreeFaces[static_cast<std::size_t>(numberOfPoints)] != nullptr) {
            auto* face = FreeFaces[static_cast<std::size_t>(numberOfPoints)];
            FreeFaces[static_cast<std::size_t>(numberOfPoints)] = face->Next;
            face->Next = nullptr;
            face->Owner = this;
            return face;
        }

        const auto polySize = SizeofFace(numberOfPoints);
        if (polySize > this->ArrayLength) {
            throw std::length_error("Face size exceeds FaceMemoryPool block size");
        }
        if (this->NextFaceIndex + polySize > this->ArrayLength) {
            ++this->NextArrayIndex;
            this->NextFaceIndex = 0;
        }

        if (this->NextArrayIndex >= this->NumberOfArrays) {
            std::size_t idx, num;
            unsigned char** newArrays;
            num = this->NumberOfArrays * 2;
            newArrays = new unsigned char*[num];
            for (idx = 0; idx < num; ++idx) {
                newArrays[idx] = nullptr;
                if (idx < this->NumberOfArrays) { newArrays[idx] = this->Arrays[idx]; }
            }
            delete[] this->Arrays;
            this->Arrays = newArrays;
            this->NumberOfArrays = num;
        }
        if (this->Arrays[this->NextArrayIndex] == nullptr) {
            this->Arrays[this->NextArrayIndex] = new unsigned char[this->ArrayLength];
        }

        GFace* face = reinterpret_cast<GFace*>(this->Arrays[this->NextArrayIndex] + this->NextFaceIndex);
        face->Owner = this;
        face->NumberOfPoints = numberOfPoints;
        face->PointIds = reinterpret_cast<igIndex*>(reinterpret_cast<unsigned char*>(face) + sizeof(GFace));
        this->NextFaceIndex += polySize;
        return face;
    }

    void ReleaseUnlocked(GFace* face) {
        if (face == nullptr || face->NumberOfPoints < 0) { return; }
        EnsureFreeFaceList(face->NumberOfPoints);
        const auto index = static_cast<std::size_t>(face->NumberOfPoints);
        face->Next = FreeFaces[index];
        FreeFaces[index] = face;
    }

public:
    FaceMemoryPool()
        : NumberOfArrays(0), ArrayLength(0), NextArrayIndex(0), NextFaceIndex(0),
          Arrays(nullptr) /*, Lock(std::make_unique<std::mutex>()) */ {}

    ~FaceMemoryPool() { this->Destroy(); }

    void Initialize(const igIndex&) {
        this->Destroy();
        this->NumberOfArrays = 16;
        this->NextArrayIndex = 0;
        this->NextFaceIndex = 0;
        this->Arrays = new unsigned char*[this->NumberOfArrays];
        for (auto i = 0; i < this->NumberOfArrays; i++) { this->Arrays[i] = nullptr; }
        this->ArrayLength = ChunkBytes;
        FreeFaces.assign(static_cast<std::size_t>(IGAME_CELL_MAX_SIZE) + 1u, nullptr);
    }

    void Destroy() {
        for (auto i = 0; i < this->NumberOfArrays; i++) {
            delete[] this->Arrays[i];
            this->Arrays[i] = nullptr;
        }
        delete[] this->Arrays;
        this->Arrays = nullptr;
        this->ArrayLength = 0;
        this->NumberOfArrays = 0;
        this->NextArrayIndex = 0;
        this->NextFaceIndex = 0;
        FreeFaces.clear();
    }

    GFace* Allocate(const int& numberOfPoints) {
        std::lock_guard<std::mutex> lock(Mutex);
        return AllocateUnlocked(numberOfPoints);
    }

    GFace* AllocateSerial(const int numberOfPoints) { return AllocateUnlocked(numberOfPoints); }

    void Release(GFace* face) {
        std::lock_guard<std::mutex> lock(Mutex);
        ReleaseUnlocked(face);
    }

    void ReleaseSerial(GFace* face) { ReleaseUnlocked(face); }
};
class FaceHashMap {
private:
    using BucketMutex = iGameAtomicMutex;

    struct Bucket {
        GFace* Head;
        BucketMutex Lock;
        Bucket() : Head(nullptr) {}
    };
    size_t Size;
    std::vector<Bucket> Buckets;
    bool ThreadSafe{true};

    static std::size_t GetKey(const GFace& face) {
        std::uint64_t key = static_cast<std::uint64_t>(face.NumberOfPoints);
        for (int i = 0; i < face.NumberOfPoints; ++i) {
            key += MixFacePointKey(face.PointIds[i]);
        }
        return FinishFaceKey(key);
    }

    template<int Fcnt>
    static std::size_t GetKey(const StaticFace<Fcnt>& face) noexcept {
        return face.GetHashKey();
    }

    template<bool Synchronized, typename TypeFace>
    static void InsertExclusive(
        GFace*& bucketHead,
        const TypeFace& face,
        FaceMemoryPool* pool) {
        auto* current = bucketHead;
        auto* previous = current;
        while (current != nullptr) {
            if (*current == face) {
                if (bucketHead == current) {
                    bucketHead = current->Next;
                } else {
                    previous->Next = current->Next;
                }
                if (current->Owner != nullptr) {
                    if constexpr (Synchronized) {
                        current->Owner->Release(current);
                    } else {
                        current->Owner->ReleaseSerial(current);
                    }
                }
                return;
            }
            previous = current;
            current = current->Next;
        }
        GFace* newFace = nullptr;
        if constexpr (Synchronized) {
            newFace = pool->Allocate(face.GetSize());
        } else {
            newFace = pool->AllocateSerial(face.GetSize());
        }
        newFace->Next = nullptr;
        newFace->OriginalCellId = face.OriginalCellId;
        newFace->IsGhost = face.IsGhost;
        std::copy(face.PointIds, face.PointIds + face.GetSize(), newFace->PointIds);
        if (bucketHead == nullptr) {
            bucketHead = newFace;
        } else {
            previous->Next = newFace;
        }
    }

public:
    FaceHashMap(const size_t& size, const bool threadSafe = true)
        : Size(std::max<std::size_t>(1u, std::min<std::size_t>(size, 1024u * 1024u))),
          ThreadSafe(threadSafe) {
        this->Buckets.resize(this->Size);
    }
    ~FaceHashMap() { std::vector<Bucket>().swap(this->Buckets); }
    std::vector<Bucket>& GetBuckets() { return this->Buckets; }
    //插入面到池子中，如果已经存在就去除，如果不存在就加入
    template<typename TypeFace>
    void Insert(const TypeFace& f, FaceMemoryPool* pool) {
        const size_t key = GetKey(f) % this->Size;
        auto& bucket = this->Buckets[key];
        auto& bucketHead = bucket.Head;
        if (!ThreadSafe) {
            InsertExclusive<false>(bucketHead, f, pool);
            return;
        }
        std::lock_guard<BucketMutex> lock(bucket.Lock);
        InsertExclusive<true>(bucketHead, f, pool);
    }
    void MergeInto(FaceHashMap& output, FaceMemoryPool* pool) const {
        for (const auto& bucket : Buckets) {
            auto* current = bucket.Head;
            while (current != nullptr) {
                output.Insert(*current, pool);
                current = current->Next;
            }
        }
    }
    void CompositeFaces(CellArray::Pointer& Polygons, std::vector<igIndex>& f2c) {
        auto numInputPts = this->Buckets.size();
        size_t i = 0;
        for (i = 0; i < numInputPts; i++) {
            auto current = Buckets[i].Head;
            while (current != nullptr) {
                Polygons->AddCellIds(current->PointIds, current->NumberOfPoints);
                f2c.emplace_back(current->OriginalCellId);
                current = current->Next;
            }
        }
    }
};


struct ExtractCellBoundaries {
    //有用户可能有需求要这个map，一开始没有考虑到，因此内存在此处维护，故对PointMap进行new和delete
    //现在为了传出去这个map给用户，内存不可在此处管理销毁，因此采用共享指针管理，传出去的也是共享指针
    FlatArray<igIndex>::Pointer PointLookup = nullptr;
    igIndex* PointMap = nullptr;
    const char* CellVis;
    const unsigned char* CellGhosts;
    const unsigned char* PointGhost;


    ExtractCellBoundaries(const char* cellVis, const unsigned char* cellGhosts, const unsigned char* pointGhost)
        : PointMap(nullptr), CellVis(cellVis), CellGhosts(cellGhosts), PointGhost(pointGhost) {}

    virtual ~ExtractCellBoundaries() {
        PointMap = nullptr;
        PointLookup = nullptr;
        CellVis = nullptr;
        CellGhosts = nullptr;
        PointGhost = nullptr;
    }

    // If point merging is needed, create the point map (map from old points
    // to new points).
    void CreatePointMap(igIndex numPts) {
        this->PointLookup = FlatArray<igIndex>::New();
        this->PointLookup->Resize(numPts);
        this->PointMap = PointLookup->RawPointer();
        std::fill(this->PointMap, this->PointMap + numPts, -1);
    }

    void UpdatePointMap(CellArray::Pointer& Polygons, Points::Pointer oldPoints, Points::Pointer newPoints) {
        auto ids = Polygons->GetCellIdArray()->RawPointer();
        IGsize num = Polygons->GetNumberOfCellIds();  // 使用实际填充的数据量，而不是 buffer 的大小
        igIndex id = 0;
        igIndex oldId = 0;
        igIndex newId = 0;
        Point p;
        for (IGsize i = 0; i < num; i++) {
            oldId = ids[i];
            if (this->PointMap[oldId] == -1) {
                //p = oldPoints->GetPoint(oldId);
                //newPoints->AddPoint(p);
                this->PointMap[oldId] = newId++;
            }
        }
        for (IGsize i = 0; i < num; i++) {
            oldId = ids[i];
            ids[i] = this->PointMap[oldId];
        }
        newPoints->Resize(newId);
        auto pNum = oldPoints->GetNumberOfPoints();
        for (IGsize i = 0; i < pNum; i++) {
            if (this->PointMap[i] != -1) { newPoints->SetPoint(PointMap[i], oldPoints->GetPoint(i)); }
        }
    }

    FlatArray<igIndex>::Pointer GetPointMap() { return this->PointLookup; }
    virtual void Initialize() {}
};

int ModelGeometryFilter::ExecuteWithSurfaceMesh(DataObject::Pointer input, SurfaceMesh::Pointer& output,
                                                SurfaceMesh::Pointer exc) {
    SurfaceMesh::Pointer Mesh = DynamicCast<SurfaceMesh>(input);
    igDebug("Input has {} points and {} faces.", Mesh->GetNumberOfPoints(), Mesh->GetNumberOfFaces());
    igIndex i = 0, j = 0, k = 0;
    igIndex64 cellId = 0, pointId = 0;
    igIndex64 numCells = Mesh->GetNumberOfFaces();
    igIndex64 numInputPts = Mesh->GetNumberOfPoints();
    igIndex64 numOutputPts = 0;
    auto inPoints = Mesh->GetPoints();
    auto outPoints = Points::New();
    auto inAllDataArray = input->GetAttributeSet();
    auto outAllDataArray = AttributeSet::New();
    CellArray::Pointer Polygons = CellArray::New();
    CharArray::Pointer CellVisibleArray = CharArray::New();
    char* CellVisible = ComputeCellVisibleArray(CellVisibleArray, inPoints, Mesh->GetFaces());
    unsigned char* cellGhosts = nullptr;
    unsigned char* pointGhosts = nullptr;
    if ((!CellVisible) && (Merging == false)) { return 0; }
    std::vector<igIndex> f2c;
    auto Faces = Mesh->GetFaces();
    igIndex vcnt;
    igIndex vhs[IGAME_CELL_MAX_SIZE];
    for (i = 0; i < numCells; i++) {
        if (!CellVisible || CellVisible[i]) {
            vcnt = Faces->GetCellIds(i, vhs);
            Polygons->AddCellIds(vhs, vcnt);
            f2c.emplace_back(i);
        }
    }
    CompositeCellAttribute(f2c, inAllDataArray, outAllDataArray);

    if (Merging) {
        auto* extract = new ExtractCellBoundaries(nullptr, nullptr, nullptr);
        extract->CreatePointMap(numInputPts);
        ProcessPointMergin(extract, inPoints, outPoints, Polygons, outAllDataArray);
        delete extract;
    } else {
        m_PointMap = nullptr;
    }
    output->SetPoints(outPoints);
    output->SetFaces(Polygons);
    output->SetAttributeSet(outAllDataArray);
    output->SetViewStyle(IG_WIREFRAME | IG_SURFACE);

    //igDebug("Extracted " << output->GetNumberOfPoints() << " points,"
    //                     << output->GetNumberOfFaces() << " faces.");
    std::vector<igIndex> temp;
    f2c.swap(temp);
    //igDebug("Extracted surface cost " << time2 - time1 << "ms.");
    return 1;
}

int ModelGeometryFilter::ExecuteWithSurfaceMesh(DataObject::Pointer input, SurfaceMesh::Pointer& output) {
    return this->ExecuteWithSurfaceMesh(input, output, nullptr);
}


void ExtractCellGeometry(VolumeMesh::Pointer input, igIndex cellId, igIndex npts, const igIndex* pts,
                         FaceMemoryPool* FacePool, FaceHashMap* FaceMap, const bool& isGhost) {

    int FaceId, numFaces, FaceVcnt;
    igIndex vhs[IGAME_CELL_MAX_SIZE];
    const igIndex* FaceVerts;

    if (!input->GetIsPolyhedronType()) {
        switch (npts) {
            case 4: {
                for (FaceId = 0; FaceId < 4; FaceId++) {
                    FaceVerts = Tetra::faces[FaceId];
                    vhs[0] = pts[FaceVerts[0]];
                    vhs[1] = pts[FaceVerts[1]];
                    vhs[2] = pts[FaceVerts[2]];
                    FaceMap->Insert(GTriangle(cellId, vhs, isGhost), FacePool);
                }
            } break;
            case 5:
                for (FaceId = 0; FaceId < 5; FaceId++) {
                    FaceVerts = Pyramid::faces[FaceId];
                    vhs[0] = pts[FaceVerts[0]];
                    vhs[1] = pts[FaceVerts[1]];
                    vhs[2] = pts[FaceVerts[2]];
                    if (FaceVerts[3] < 0) {
                        FaceMap->Insert(GTriangle(cellId, vhs, isGhost), FacePool);
                    } else {
                        vhs[3] = pts[FaceVerts[3]];
                        FaceMap->Insert(GQuad(cellId, vhs, isGhost), FacePool);
                    }
                }
                break;
            case 6:
                for (FaceId = 0; FaceId < 6; FaceId++) {
                    FaceVerts = Prism::faces[FaceId];
                    vhs[0] = pts[FaceVerts[0]];
                    vhs[1] = pts[FaceVerts[1]];
                    vhs[2] = pts[FaceVerts[2]];
                    if (FaceVerts[3] < 0) {
                        FaceMap->Insert(GTriangle(cellId, vhs, isGhost), FacePool);
                    } else {
                        vhs[3] = pts[FaceVerts[3]];
                        FaceMap->Insert(GQuad(cellId, vhs, isGhost), FacePool);
                    }
                }
                break;
            case 8:
                for (FaceId = 0; FaceId < 6; FaceId++) {
                    FaceVerts = Hexahedron::faces[FaceId];
                    vhs[0] = pts[FaceVerts[0]];
                    vhs[1] = pts[FaceVerts[1]];
                    vhs[2] = pts[FaceVerts[2]];
                    vhs[3] = pts[FaceVerts[3]];
                    FaceMap->Insert(GQuad(cellId, vhs, isGhost), FacePool);
                }
                break;
            default:
                break;
        }
    } else {
        igIndex fhs[IGAME_CELL_MAX_SIZE];
        auto fcnt = input->GetVolumeFaceIds(cellId, fhs);
        igIndex FaceVcnt = 0;
        for (int i = 0; i < fcnt; i++) {
            FaceVcnt = input->GetFacePointIds(fhs[i], vhs);
            switch (FaceVcnt) {
                case 0:
                case 1:
                case 2:
                    break;
                case 3:
                    FaceMap->Insert(GTriangle(cellId, vhs, isGhost), FacePool);
                    break;
                case 4:
                    FaceMap->Insert(GQuad(cellId, vhs, isGhost), FacePool);
                    break;
                case 5:
                    FaceMap->Insert(GPentagon(cellId, vhs, isGhost), FacePool);
                    break;
                case 6:
                    FaceMap->Insert(GHexagon(cellId, vhs, isGhost), FacePool);
                    break;
                case 7:
                    FaceMap->Insert(GHeptagon(cellId, vhs, isGhost), FacePool);
                    break;
                case 8:
                    FaceMap->Insert(GOctagon(cellId, vhs, isGhost), FacePool);
                    break;
                case 9:
                    FaceMap->Insert(GNonagon(cellId, vhs, isGhost), FacePool);
                    break;
                case 10:
                    FaceMap->Insert(GDecagon(cellId, vhs, isGhost), FacePool);
                    break;
                default:
                    FaceMap->Insert(GPolygon(cellId, FaceVcnt, vhs, isGhost), FacePool);
                    break;
            }
        }
    }
}
struct ExtractVM : public ExtractCellBoundaries {
    // Process volumemesh mesh
    VolumeMesh::Pointer Mesh;
    std::shared_ptr<FaceHashMap> FaceMap;
    bool RemoveGhostInterFaces;

    ExtractVM(VolumeMesh::Pointer mesh, const char* cellVis, const unsigned char* cellGhost,
              const unsigned char* pointGhost, bool merging, bool removeGhostInterFaces)
        : ExtractCellBoundaries(cellVis, cellGhost, pointGhost), Mesh(mesh),
          RemoveGhostInterFaces(removeGhostInterFaces) {
        if (merging) { this->CreatePointMap(mesh->GetNumberOfPoints()); }
        this->FaceMap = std::make_shared<FaceHashMap>(static_cast<size_t>(mesh->GetNumberOfPoints()));
        this->Initialize();
    }

    void Initialize() override { this->ExtractCellBoundaries::Initialize(); }

    void Execute(igIndex beginCellId, igIndex endCellId, FaceMemoryPool* FacePool) {
        igIndex cellId;
        bool isGhost = false;
        igIndex pts[IGAME_CELL_MAX_SIZE];
        igIndex npts = 0;
        auto FaceMap = this->FaceMap.get();
        if (this->Mesh) {
            for (cellId = beginCellId; cellId < endCellId; cellId++) {
                if (isGhost) { continue; }
                // If the cell is visible process it
                if (!this->CellVis || this->CellVis[cellId]) {
                    npts = this->Mesh->GetVolumePointIds(cellId, pts);
                    ExtractCellGeometry(this->Mesh, cellId, npts, pts, FacePool, FaceMap, isGhost);
                }
            }
        }
    } // operator()
};
int ModelGeometryFilter::ExecuteWithVolumeMesh(DataObject::Pointer input, SurfaceMesh::Pointer& output,
                                               SurfaceMesh::Pointer exc) {
    VolumeMesh::Pointer Mesh = DynamicCast<VolumeMesh>(input);
    //igDebug("Input has " << Mesh->GetNumberOfPoints() << " points and "
    //                     << Mesh->GetNumberOfVolumes() << " volumes.");
    igIndex i = 0, j = 0, k = 0;
    igIndex64 cellId = 0, pointId = 0;
    igIndex64 numCells = Mesh->GetNumberOfVolumes();
    igIndex64 numInputPts = Mesh->GetNumberOfPoints();
    igIndex64 numOutputPts = 0;
    auto inPoints = Mesh->GetPoints();
    auto outPoints = inPoints;
    auto inAllDataArray = input->GetAttributeSet();
    auto outAllDataArray = AttributeSet::New();
    CellArray::Pointer Polygons = CellArray::New();
    CharArray::Pointer CellVisibleArray = CharArray::New();
    char* CellVisible = ComputeCellVisibleArray(CellVisibleArray, inPoints, Mesh->GetCells());
    unsigned char* cellGhosts = nullptr;
    unsigned char* pointGhosts = nullptr;


    auto* extract =
            new ExtractVM(Mesh, CellVisible, cellGhosts, pointGhosts, this->Merging, this->RemoveGhostInterfaces);
    FaceMemoryPool** FacePools = new FaceMemoryPool*[this->MaxThreadSize];
    std::fill(FacePools, FacePools + MaxThreadSize, nullptr);
    std::exception_ptr workerException;
    std::mutex workerExceptionMutex;

    auto func = [&](igIndex start, igIndex end, int i) -> void {
        try {
            FacePools[i] = new FaceMemoryPool;
            FacePools[i]->Initialize(Mesh->GetNumberOfPoints());
            extract->Execute(start, end, FacePools[i]);
        } catch (...) {
            std::lock_guard<std::mutex> lock(workerExceptionMutex);
            if (!workerException) { workerException = std::current_exception(); }
        }
    };
    ThreadPool::parallelFor(0, numCells, MaxThreadSize, func);
    if (workerException) {
        delete extract;
        for (int poolIndex = 0; poolIndex < MaxThreadSize; ++poolIndex) { delete FacePools[poolIndex]; }
        delete[] FacePools;
        std::rethrow_exception(workerException);
    }

    std::vector<igIndex> f2c;
    extract->FaceMap.get()->CompositeFaces(Polygons, f2c);

    CompositeCellAttribute(f2c, inAllDataArray, outAllDataArray);
    if (Merging) {
        ProcessPointMergin(extract, inPoints, outPoints, Polygons, outAllDataArray);
    } else {
        m_PointMap = nullptr;
    }
    output->SetPoints(outPoints);
    output->SetFaces(Polygons);
    output->SetAttributeSet(outAllDataArray);
    output->SetViewStyle(IG_WIREFRAME | IG_SURFACE);

    //igDebug("Extracted " << output->GetNumberOfPoints() << " points,"
    //                     << output->GetNumberOfFaces() << " faces.");
    std::vector<igIndex> temp;
    f2c.swap(temp);
    delete extract;
    extract = nullptr;
    for (int i = 0; i < MaxThreadSize; i++) {
        if (FacePools[i]) {
            delete FacePools[i];
            FacePools[i] = nullptr;
        } else {
            break;
        }
    }
    delete[] FacePools;
    FacePools = nullptr;
    //igDebug("Extracted surface cost " << time2 - time1 << "ms.");
    return 1;
}
int ModelGeometryFilter::ExecuteWithVolumeMesh(DataObject::Pointer input, SurfaceMesh::Pointer& output) {
    return ExecuteWithVolumeMesh(input, output, nullptr);
}

void ExtractCellGeometry(UnstructuredMesh::Pointer input, igIndex cellId, int cellType, igIndex npts,
                         const igIndex* pts, FaceMemoryPool* FacePool, FaceHashMap* FaceMap, const bool& isGhost) {
    int FaceId, numFaces, FaceVcnt;
    igIndex ptIds[IGAME_CELL_MAX_SIZE]; // cell GFace point ids
    igIndex Ids[IGAME_CELL_MAX_SIZE];
    const igIndex* FaceVerts;
    static constexpr int pixelConvert[4] = {0, 1, 3, 2};
    switch (cellType) {
        case IG_EMPTY_CELL:
            break;

        case IG_VERTEX:
            //case IG_POLY_VERTEX:
            //verts.InsertNextCell(npts, pts, cellId);
            break;

        case IG_LINE:
            //case IG_POLY_LINE:
            //    lines.InsertNextCell(npts, pts, cellId);
            break;

        case IG_TRIANGLE:
        case IG_QUAD:
        case IG_POLYGON:
            //polys.InsertNextCell(npts, pts, cellId);
            break;

            //case IG_TRIANGLE_STRIP:
            //    strips.InsertNextCell(npts, pts, cellId);
            //break;

            //case IG_PIXEL:
            //    ptIds[0] = pts[pixelConvert[0]];
            //    ptIds[1] = pts[pixelConvert[1]];
            //    ptIds[2] = pts[pixelConvert[2]];
            //    ptIds[3] = pts[pixelConvert[3]];
            //    polys.InsertNextCell(npts, ptIds, cellId);
            //    break;

        case IG_TETRA:
            for (FaceId = 0; FaceId < 4; FaceId++) {
                FaceVerts = Tetra::faces[FaceId];
                ptIds[0] = pts[FaceVerts[0]];
                ptIds[1] = pts[FaceVerts[1]];
                ptIds[2] = pts[FaceVerts[2]];
                FaceMap->Insert(GTriangle(cellId, ptIds, isGhost), FacePool);
            }
            break;


        case IG_HEXAHEDRON:
            for (FaceId = 0; FaceId < 6; FaceId++) {
                FaceVerts = Hexahedron::faces[FaceId];
                ptIds[0] = pts[FaceVerts[0]];
                ptIds[1] = pts[FaceVerts[1]];
                ptIds[2] = pts[FaceVerts[2]];
                ptIds[3] = pts[FaceVerts[3]];
                FaceMap->Insert(GQuad(cellId, ptIds, isGhost), FacePool);
            }
            break;

        case IG_PRISM:
            for (FaceId = 0; FaceId < 5; FaceId++) {
                FaceVerts = Prism::faces[FaceId];
                ptIds[0] = pts[FaceVerts[0]];
                ptIds[1] = pts[FaceVerts[1]];
                ptIds[2] = pts[FaceVerts[2]];
                if (FaceVerts[3] < 0) {
                    FaceMap->Insert(GTriangle(cellId, ptIds, isGhost), FacePool);
                } else {
                    ptIds[3] = pts[FaceVerts[3]];
                    FaceMap->Insert(GQuad(cellId, ptIds, isGhost), FacePool);
                }
            }
            break;

        case IG_PYRAMID:
            for (FaceId = 0; FaceId < 5; FaceId++) {
                FaceVerts = Pyramid::faces[FaceId];
                ptIds[0] = pts[FaceVerts[0]];
                ptIds[1] = pts[FaceVerts[1]];
                ptIds[2] = pts[FaceVerts[2]];
                if (FaceVerts[3] < 0) {
                    FaceMap->Insert(GTriangle(cellId, ptIds, isGhost), FacePool);
                } else {
                    ptIds[3] = pts[FaceVerts[3]];
                    FaceMap->Insert(GQuad(cellId, ptIds, isGhost), FacePool);
                }
            }
            break;


        case IG_POLYHEDRON: {
            input->GetCellPointIds(cellId, Ids);
            igIndex index = 0;
            numFaces = Ids[index++];
            for (FaceId = 0; FaceId < numFaces; FaceId++) {
                FaceVcnt = Ids[index++];
                pts = Ids + index;
                index += FaceVcnt;
                switch (FaceVcnt) {
                    case 0:
                    case 1:
                    case 2:
                        break;
                    case 3:
                        FaceMap->Insert(GTriangle(cellId, pts, isGhost), FacePool);
                        break;
                    case 4:
                        FaceMap->Insert(GQuad(cellId, pts, isGhost), FacePool);
                        break;
                    case 5:
                        FaceMap->Insert(GPentagon(cellId, pts, isGhost), FacePool);
                        break;
                    case 6:
                        FaceMap->Insert(GHexagon(cellId, pts, isGhost), FacePool);
                        break;
                    case 7:
                        FaceMap->Insert(GHeptagon(cellId, pts, isGhost), FacePool);
                        break;
                    case 8:
                        FaceMap->Insert(GOctagon(cellId, pts, isGhost), FacePool);
                        break;
                    case 9:
                        FaceMap->Insert(GNonagon(cellId, pts, isGhost), FacePool);
                        break;
                    case 10:
                        FaceMap->Insert(GDecagon(cellId, pts, isGhost), FacePool);
                        break;
                    default:
                        FaceMap->Insert(GPolygon(cellId, FaceVcnt, pts, isGhost), FacePool);
                        break;
                }
            }
        }

        break;
        case IG_QUADRATIC_EDGE:
            break;
        case IG_QUADRATIC_TRIANGLE:
        case IG_QUADRATIC_QUAD:
        case IG_QUADRATIC_POLYGON:
            break;
        case IG_QUADRATIC_TETRA:
            for (FaceId = 0; FaceId < 4; FaceId++) {
                FaceVerts = QuadraticTetra::faces[FaceId];
                ptIds[0] = pts[FaceVerts[0]];
                ptIds[1] = pts[FaceVerts[3]];
                ptIds[2] = pts[FaceVerts[1]];
                ptIds[3] = pts[FaceVerts[4]];
                ptIds[4] = pts[FaceVerts[2]];
                ptIds[5] = pts[FaceVerts[5]];
                FaceMap->Insert(GHexagon(cellId, ptIds, isGhost), FacePool);
            }
            break;
        case IG_QUADRATIC_HEXAHEDRON:
            for (FaceId = 0; FaceId < 6; FaceId++) {
                FaceVerts = QuadraticHexahedron::faces[FaceId];
                ptIds[0] = pts[FaceVerts[0]];
                ptIds[1] = pts[FaceVerts[1]];
                ptIds[2] = pts[FaceVerts[2]];
                ptIds[3] = pts[FaceVerts[3]];
                FaceMap->Insert(GQuad(cellId, ptIds, isGhost), FacePool);
            }
            break;
        case IG_QUADRATIC_PRISM:
            for (FaceId = 0; FaceId < 5; FaceId++) {
                FaceVerts = QuadraticPrism::faces[FaceId];
                ptIds[0] = pts[FaceVerts[0]];
                ptIds[1] = pts[FaceVerts[1]];
                ptIds[2] = pts[FaceVerts[2]];
                if (FaceVerts[6] < 0) {
                    FaceMap->Insert(GTriangle(cellId, ptIds, isGhost), FacePool);
                } else {
                    ptIds[3] = pts[FaceVerts[3]];
                    FaceMap->Insert(GQuad(cellId, ptIds, isGhost), FacePool);
                }
            }
            break;
        case IG_QUADRATIC_PYRAMID:
            for (FaceId = 0; FaceId < 5; FaceId++) {
                FaceVerts = QuadraticPyramid::faces[FaceId];
                ptIds[0] = pts[FaceVerts[0]];
                ptIds[1] = pts[FaceVerts[1]];
                ptIds[2] = pts[FaceVerts[2]];
                if (FaceVerts[6] < 0) {
                    FaceMap->Insert(GTriangle(cellId, ptIds, isGhost), FacePool);
                } else {
                    ptIds[3] = pts[FaceVerts[3]];
                    FaceMap->Insert(GQuad(cellId, ptIds, isGhost), FacePool);
                }
            }
            break;
        default:
            //一般为多面体，需要通过cell找到面片
            Cell* cell = input->GetCell(cellId);
            auto cellType = input->GetCellType(cellId);
            if (Cell::GetCellDimension(cellType) == 3) {
                for (FaceId = 0, numFaces = cell->GetNumberOfFaces(); FaceId < numFaces; FaceId++) {
                    Cell* Face = cell->GetFace(FaceId);
                    FaceVcnt = static_cast<int>(Face->m_PointIds->GetNumberOfIds());
                    switch (FaceVcnt) {
                        case 3:
                            FaceMap->Insert(GTriangle(cellId, Face->m_PointIds->RawPointer(), isGhost), FacePool);
                            break;
                        case 4:
                            FaceMap->Insert(GQuad(cellId, Face->m_PointIds->RawPointer(), isGhost), FacePool);
                            break;
                        case 5:
                            FaceMap->Insert(GPentagon(cellId, Face->m_PointIds->RawPointer(), isGhost), FacePool);
                            break;
                        case 6:
                            FaceMap->Insert(GHexagon(cellId, Face->m_PointIds->RawPointer(), isGhost), FacePool);
                            break;
                        case 7:
                            FaceMap->Insert(GHeptagon(cellId, Face->m_PointIds->RawPointer(), isGhost), FacePool);
                            break;
                        case 8:
                            FaceMap->Insert(GOctagon(cellId, Face->m_PointIds->RawPointer(), isGhost), FacePool);
                            break;
                        case 9:
                            FaceMap->Insert(GNonagon(cellId, Face->m_PointIds->RawPointer(), isGhost), FacePool);
                            break;
                        case 10:
                            FaceMap->Insert(GDecagon(cellId, Face->m_PointIds->RawPointer(), isGhost), FacePool);
                            break;
                        default:
                            FaceMap->Insert(GPolygon(cellId, FaceVcnt, Face->m_PointIds->RawPointer(), isGhost),
                                            FacePool);
                            break;
                    }
                }
            } else {
                igDebug("Unknown cell type : {}", cellType);
            }
    }
}
struct ExtractUG : public ExtractCellBoundaries {
    UnstructuredMesh::Pointer Mesh;
    std::shared_ptr<FaceHashMap> FaceMap;
    bool RemoveGhostInterFaces;

    ExtractUG(UnstructuredMesh* mesh, const char* cellVis, const unsigned char* cellGhost,
              const unsigned char* pointGhost, bool merging, bool removeGhostInterFaces)
        : ExtractCellBoundaries(cellVis, cellGhost, pointGhost), Mesh(mesh),
          RemoveGhostInterFaces(removeGhostInterFaces) {
        if (merging) { this->CreatePointMap(mesh->GetNumberOfPoints()); }
        this->FaceMap = std::make_shared<FaceHashMap>(static_cast<size_t>(mesh->GetNumberOfPoints()));
        this->Initialize();
    }

    void Initialize() override { this->ExtractCellBoundaries::Initialize(); }

    void Execute(igIndex beginCellId, igIndex endCellId, FaceMemoryPool* FacePool) {
        igIndex cellId;
        bool isGhost = false;
        igIndex pts[IGAME_CELL_MAX_SIZE];
        igIndex npts = 0;
        auto FaceMap = this->FaceMap.get();
        if (this->Mesh) {
            auto cellTypes = Mesh->GetCellTypes()->RawPointer();
            for (cellId = beginCellId; cellId < endCellId; cellId++) {
                igIndex cellType = cellTypes[cellId];
                //如果是虚拟Cell
                if (isGhost && (Cell::GetCellDimension(cellType) < 3 || !this->RemoveGhostInterFaces)) { continue; }
                if (!this->CellVis || this->CellVis[cellId]) {
                    Mesh->GetCellPointIds(cellId, pts);
                    ExtractCellGeometry(this->Mesh, cellId, cellType, npts, pts, FacePool, FaceMap, isGhost);
                }
            }
        }
    }
};

struct ExtractDecodedUG : public ExtractCellBoundaries {
    std::shared_ptr<FaceHashMap> FaceMap;

    explicit ExtractDecodedUG(const igIndex pointCount)
        : ExtractCellBoundaries(nullptr, nullptr, nullptr) {
        CreatePointMap(pointCount);
        FaceMap = std::make_shared<FaceHashMap>(static_cast<std::size_t>(pointCount));
        Initialize();
    }
};

struct ModelGeometryDecodedSurfaceBuilder::Impl {
    explicit Impl(const IGsize pointCountValue, const std::size_t workerCountValue)
        : pointCount(pointCountValue),
          workerCount(std::max<std::size_t>(workerCountValue, 1u)) {
        if (pointCount > static_cast<IGsize>(std::numeric_limits<igIndex>::max())) {
            Fail("decoded surface point count exceeds index capacity");
            return;
        }
        extract = std::make_unique<ExtractDecodedUG>(static_cast<igIndex>(pointCount));
        pools.reserve(workerCount);
        for (std::size_t index = 0u; index < workerCount; ++index) {
            auto pool = std::make_unique<FaceMemoryPool>();
            pool->Initialize(static_cast<igIndex>(pointCount));
            pools.push_back(std::move(pool));
        }
    }

    void Fail(std::string message) {
        valid.store(false, std::memory_order_relaxed);
        std::lock_guard<std::mutex> lock(errorMutex);
        if (error.empty()) { error = std::move(message); }
    }

    void CopyError(std::string* output) const {
        if (output == nullptr) { return; }
        std::lock_guard<std::mutex> lock(errorMutex);
        *output = error;
    }

    IGsize pointCount{0u};
    std::size_t workerCount{1u};
    std::unique_ptr<ExtractDecodedUG> extract;
    std::vector<std::unique_ptr<FaceMemoryPool>> pools;
    std::atomic<bool> valid{true};
    mutable std::mutex errorMutex;
    std::string error;
};

namespace {

bool IsDecodedSurfaceCellTypeSupported(const int cellType) {
    switch (cellType) {
        case IG_TETRA:
        case IG_HEXAHEDRON:
        case IG_PRISM:
        case IG_PYRAMID:
        case IG_QUADRATIC_TETRA:
        case IG_QUADRATIC_HEXAHEDRON:
        case IG_QUADRATIC_PRISM:
        case IG_QUADRATIC_PYRAMID:
            return true;
        default:
            return false;
    }
}

} // namespace

ModelGeometryDecodedSurfaceBuilder::ModelGeometryDecodedSurfaceBuilder(
    const IGsize pointCount,
    const std::size_t workerCount)
    : m_impl(std::make_unique<Impl>(pointCount, workerCount)) {}

ModelGeometryDecodedSurfaceBuilder::~ModelGeometryDecodedSurfaceBuilder() = default;

bool ModelGeometryDecodedSurfaceBuilder::AccumulateBlock(
    const std::size_t workerIndex,
    const std::size_t cellOffset,
    const int fixedCellSize,
    const std::span<const std::uint32_t> connectivity,
    const std::span<const std::uint32_t> offsets,
    const std::span<const std::uint32_t> cellTypes,
    std::string* error) {
    if (m_impl == nullptr || !m_impl->valid.load(std::memory_order_relaxed)) {
        if (m_impl != nullptr) { m_impl->CopyError(error); }
        return false;
    }
    if (workerIndex >= m_impl->pools.size()) {
        m_impl->Fail("decoded surface worker index is invalid");
        m_impl->CopyError(error);
        return false;
    }
    if (cellTypes.empty()) {
        m_impl->Fail("decoded surface requires explicit cell types");
        m_impl->CopyError(error);
        return false;
    }
    if (fixedCellSize > 0) {
        const auto fixedSize = static_cast<std::size_t>(fixedCellSize);
        if (cellTypes.size() > std::numeric_limits<std::size_t>::max() / fixedSize ||
            connectivity.size() != cellTypes.size() * fixedSize) {
            m_impl->Fail("decoded surface fixed topology block shape is invalid");
            m_impl->CopyError(error);
            return false;
        }
    } else if (offsets.size() != cellTypes.size() + 1u ||
               offsets.empty() ||
               offsets.back() != connectivity.size()) {
        m_impl->Fail("decoded surface variable topology block shape is invalid");
        m_impl->CopyError(error);
        return false;
    }
    auto* outputFacePool = m_impl->pools[workerIndex].get();
    auto* outputFaceMap = m_impl->extract->FaceMap.get();
#if defined(__EMSCRIPTEN__)
    FaceMemoryPool localFacePool;
    localFacePool.Initialize(static_cast<igIndex>(m_impl->pointCount));
    const auto localBucketCount = cellTypes.size() <= std::numeric_limits<std::size_t>::max() / 2u
        ? cellTypes.size() * 2u
        : cellTypes.size();
    FaceHashMap localFaceMap((std::max<std::size_t>)(localBucketCount, 1u), false);
    auto* facePool = &localFacePool;
    auto* faceMap = &localFaceMap;
#else
    auto* facePool = outputFacePool;
    auto* faceMap = outputFaceMap;
#endif
    UnstructuredMesh::Pointer input;
    igIndex pointIds[IGAME_CELL_MAX_SIZE]{};
    for (std::size_t localCellIndex = 0u; localCellIndex < cellTypes.size(); ++localCellIndex) {
        if (!m_impl->valid.load(std::memory_order_relaxed)) { break; }
        const auto globalCellIndex = cellOffset + localCellIndex;
        if (globalCellIndex > static_cast<std::size_t>(std::numeric_limits<igIndex>::max())) {
            m_impl->Fail("decoded surface cell index exceeds index capacity");
            break;
        }
        const auto cellType = static_cast<int>(cellTypes[localCellIndex]);
        if (Cell::GetCellDimension(cellType) < 3) { continue; }
        if (!IsDecodedSurfaceCellTypeSupported(cellType)) {
            m_impl->Fail("decoded surface encountered an unsupported volume cell type");
            break;
        }

        std::size_t begin = 0u;
        std::size_t end = 0u;
        if (fixedCellSize > 0) {
            begin = localCellIndex * static_cast<std::size_t>(fixedCellSize);
            end = begin + static_cast<std::size_t>(fixedCellSize);
        } else {
            begin = static_cast<std::size_t>(offsets[localCellIndex]);
            end = static_cast<std::size_t>(offsets[localCellIndex + 1u]);
        }
        if (begin > end || end > connectivity.size() || end - begin > IGAME_CELL_MAX_SIZE) {
            m_impl->Fail("decoded surface cell connectivity range is invalid");
            break;
        }
        const auto pointCount = end - begin;
        for (std::size_t pointIndex = 0u; pointIndex < pointCount; ++pointIndex) {
            const auto pointId = connectivity[begin + pointIndex];
            if (pointId > static_cast<std::uint32_t>(std::numeric_limits<igIndex>::max())) {
                m_impl->Fail("decoded surface point index exceeds index capacity");
                break;
            }
            pointIds[pointIndex] = static_cast<igIndex>(pointId);
        }
        if (!m_impl->valid.load(std::memory_order_relaxed)) { break; }
        ExtractCellGeometry(
            input,
            static_cast<igIndex>(globalCellIndex),
            cellType,
            static_cast<igIndex>(pointCount),
            pointIds,
            facePool,
            faceMap,
            false);
    }

    if (!m_impl->valid.load(std::memory_order_relaxed)) {
        m_impl->CopyError(error);
        return false;
    }
#if defined(__EMSCRIPTEN__)
    localFaceMap.MergeInto(*outputFaceMap, outputFacePool);
#endif
    return true;
}

bool ModelGeometryDecodedSurfaceBuilder::Finalize(
    UnstructuredMesh::Pointer input,
    SurfaceMesh::Pointer& output,
    FlatArray<igIndex>::Pointer& pointMap,
    std::shared_ptr<std::vector<igIndex>>& faceToCellMap,
    std::string* error) {
    if (m_impl == nullptr || !m_impl->valid.load(std::memory_order_relaxed)) {
        if (m_impl != nullptr) { m_impl->CopyError(error); }
        return false;
    }
    if (input == nullptr || input->GetPoints() == nullptr ||
        input->GetNumberOfPoints() != m_impl->pointCount) {
        m_impl->Fail("decoded surface input geometry does not match topology");
        m_impl->CopyError(error);
        return false;
    }

    auto polygons = CellArray::New();
    std::vector<igIndex> faceToCell;
    m_impl->extract->FaceMap->CompositeFaces(polygons, faceToCell);

    auto filter = ModelGeometryFilter::New();
    auto outputAttributes = AttributeSet::New();
    filter->CompositeCellAttribute(faceToCell, input->GetAttributeSet(), outputAttributes);
    auto outputPoints = input->GetPoints();
    filter->ProcessPointMergin(
        m_impl->extract.get(),
        input->GetPoints(),
        outputPoints,
        polygons,
        outputAttributes);

    if (output == nullptr) { output = SurfaceMesh::New(); }
    output->SetPoints(outputPoints);
    output->SetFaces(polygons);
    output->SetAttributeSet(outputAttributes);
    output->SetViewStyle(IG_SURFACE);
    pointMap = filter->GetPointMap();
    faceToCellMap = std::make_shared<std::vector<igIndex>>(std::move(faceToCell));

    m_impl->pools.clear();
    m_impl->extract.reset();
    return true;
}

int ModelGeometryFilter::ExecuteWithUnstructuredMesh(DataObject::Pointer input, SurfaceMesh::Pointer& output,
                                                     SurfaceMesh::Pointer exc) {
    using SurfaceClock = std::chrono::steady_clock;
    const auto totalStart = SurfaceClock::now();
    UnstructuredMesh::Pointer Mesh = DynamicCast<UnstructuredMesh>(input);
    if (Mesh == nullptr) { return 0; }
    //igDebug("Input has " << Mesh->GetNumberOfPoints() << " points and "
    //                     << Mesh->GetNumberOfCells() << " cells.");
    const auto dimensionScanStart = SurfaceClock::now();
    bool isNot3D = true;
    for (int i = 0; i < Mesh->GetNumberOfCells(); i++) {
        if (Cell::GetCellDimension(Mesh->GetCellType(i)) >= 3) {
            isNot3D = false;
            break;
        }
    }
    const auto dimensionScanEnd = SurfaceClock::now();
    if (isNot3D) {
        auto surfaceMesh = Mesh->TransferToSurfaceMesh();
        if (surfaceMesh == nullptr) { return 0; }
        if (!ExecuteWithSurfaceMesh(surfaceMesh, output)) { output = surfaceMesh; }
        return 1;
    }
    igIndex i = 0, j = 0, k = 0;
    igIndex64 cellId = 0, pointId = 0;
    igIndex64 numCells = Mesh->GetNumberOfCells();
    if (numCells <= 0) { return 0; }
    igIndex64 numInputPts = Mesh->GetNumberOfPoints();
    igIndex64 numOutputPts = 0;
    auto inPoints = Mesh->GetPoints();
    auto outPoints = inPoints;
    auto inAllDataArray = input->GetAttributeSet();
    auto outAllDataArray = AttributeSet::New();
    CellArray::Pointer Polygons = CellArray::New();
    CharArray::Pointer CellVisibleArray = CharArray::New();
    const auto visibilityStart = SurfaceClock::now();
    char* CellVisible = ComputeCellVisibleArray(CellVisibleArray, inPoints, Mesh->GetCells(), Mesh->GetCellTypes());
    const auto visibilityEnd = SurfaceClock::now();
    unsigned char* cellGhosts = nullptr;
    unsigned char* pointGhosts = nullptr;

    const auto faceExtractionStart = SurfaceClock::now();
    auto* extract =
            new ExtractUG(Mesh, CellVisible, cellGhosts, pointGhosts, this->Merging, this->RemoveGhostInterfaces);

    int poolCount = this->MaxThreadSize > 0 ? this->MaxThreadSize : 1;
#if defined(__EMSCRIPTEN__) && !defined(__EMSCRIPTEN_PTHREADS__)
    poolCount = 1;
#endif
    FaceMemoryPool** FacePools = new FaceMemoryPool*[poolCount];
    std::fill(FacePools, FacePools + poolCount, nullptr);
    std::exception_ptr workerException;
    std::mutex workerExceptionMutex;
    auto func = [&](igIndex start, igIndex end, int i) -> void {
        try {
            const int poolId = (i >= 0 && i < poolCount) ? i : 0;
            if (FacePools[poolId] == nullptr) {
                FacePools[poolId] = new FaceMemoryPool;
                FacePools[poolId]->Initialize(Mesh->GetNumberOfPoints());
            }
            extract->Execute(start, end, FacePools[poolId]);
        } catch (...) {
            std::lock_guard<std::mutex> lock(workerExceptionMutex);
            if (!workerException) { workerException = std::current_exception(); }
        }
    };
#ifdef __EMSCRIPTEN__
    auto sharedPool = ThreadPool::Instance();
    const std::size_t sharedPoolWorkerCount = sharedPool->WorkerCount();
    const int cellRange = static_cast<int>(numCells);
    const int chunkSize = (cellRange + poolCount - 1) / poolCount;
    std::vector<std::future<void>> futures;
    futures.reserve(poolCount);
    for (int workerIndex = 0; workerIndex < poolCount; ++workerIndex) {
        const int chunkStart = workerIndex * chunkSize;
        if (chunkStart >= cellRange) { break; }
        const int chunkEnd = std::min(cellRange, chunkStart + chunkSize);
        futures.emplace_back(sharedPool->Commit([&, chunkStart, chunkEnd, workerIndex]() {
            func(chunkStart, chunkEnd, workerIndex);
        }));
    }
    for (auto& future: futures) { future.get(); }
#else
    const std::size_t sharedPoolWorkerCount = 0u;
    ThreadPool::parallelFor(0, numCells, poolCount, func);
#endif
    if (workerException) {
        delete extract;
        for (int poolIndex = 0; poolIndex < poolCount; ++poolIndex) { delete FacePools[poolIndex]; }
        delete[] FacePools;
        std::rethrow_exception(workerException);
    }
    const auto faceExtractionEnd = SurfaceClock::now();
    std::vector<igIndex> f2c;
    const auto faceCompositeStart = SurfaceClock::now();
    extract->FaceMap.get()->CompositeFaces(Polygons, f2c);
    const auto faceCompositeEnd = SurfaceClock::now();
    const auto cellAttributeStart = SurfaceClock::now();
    CompositeCellAttribute(f2c, inAllDataArray, outAllDataArray);
    const auto cellAttributeEnd = SurfaceClock::now();
    const auto pointMergeStart = SurfaceClock::now();
    if (Merging) {
        ProcessPointMergin(extract, inPoints, outPoints, Polygons, outAllDataArray);
    } else {
        m_PointMap = nullptr;
    }
    const auto pointMergeEnd = SurfaceClock::now();
    const auto finalizeStart = SurfaceClock::now();
    output->SetPoints(outPoints);
    output->SetFaces(Polygons);
    output->SetAttributeSet(outAllDataArray);
    output->SetViewStyle(IG_WIREFRAME | IG_SURFACE);
    //igDebug("Extracted " << output->GetNumberOfPoints() << " points,"
    //                     << output->GetNumberOfFaces() << " faces.");
    std::vector<igIndex> temp;
    f2c.swap(temp);
    delete extract;
    extract = nullptr;
    for (int i = 0; i < poolCount; i++) {
        if (FacePools[i]) {
            delete FacePools[i];
            FacePools[i] = nullptr;
        } else {
            break;
        }
    }
    delete[] FacePools;
    FacePools = nullptr;
    const auto finalizeEnd = SurfaceClock::now();
    const auto elapsedMs = [](const auto start, const auto end) {
        return std::chrono::duration<double, std::milli>(end - start).count();
    };
    igDebug(
            "[ModelGeometryFilter timing] total={} ms; dimension-scan={} ms; visibility={} ms; "
            "face-extraction={} ms; face-composite={} ms; cell-attributes={} ms; point-merge={} ms; finalize={} ms; "
            "point-merging={}; workers={}; shared-pool-workers={}",
            elapsedMs(totalStart, finalizeEnd), elapsedMs(dimensionScanStart, dimensionScanEnd),
            elapsedMs(visibilityStart, visibilityEnd), elapsedMs(faceExtractionStart, faceExtractionEnd),
            elapsedMs(faceCompositeStart, faceCompositeEnd), elapsedMs(cellAttributeStart, cellAttributeEnd),
            elapsedMs(pointMergeStart, pointMergeEnd), elapsedMs(finalizeStart, finalizeEnd), Merging, poolCount,
            sharedPoolWorkerCount);
    return 1;
}

int ModelGeometryFilter::ExecuteWithUnstructuredMesh(DataObject::Pointer input, SurfaceMesh::Pointer& output) {
    return this->ExecuteWithUnstructuredMesh(input, output, nullptr);
}


struct ExtractSG : public ExtractCellBoundaries {
    StructuredMesh::Pointer Mesh;
    bool RemoveGhostInterFaces;
    CellArray::Pointer Quads;
    std::vector<igIndex> f2c;
    ExtractSG(StructuredMesh::Pointer mesh, const char* cellVis, const unsigned char* cellGhost,
              const unsigned char* pointGhost, bool merging, bool removeGhostInterFaces)
        : ExtractCellBoundaries(cellVis, cellGhost, pointGhost), Mesh(mesh),
          RemoveGhostInterFaces(removeGhostInterFaces) {
        if (merging) { this->CreatePointMap(mesh->GetNumberOfPoints()); }
        Quads = CellArray::New();
        auto size = mesh->GetDimensionSize();
        IGsize initSize = (size[0] - 1) * (size[1] - 1) * (size[2] - 1);
        if (!cellVis) { initSize *= 2; }
        Quads->Reserve(initSize);
        f2c.reserve(initSize);
        this->Initialize();
    }
    ~ExtractSG() {
        std::vector<igIndex> temp;
        f2c.swap(temp);
    }
    void Initialize() override { this->ExtractCellBoundaries::Initialize(); }

    void Execute() {
        auto size = Mesh->GetDimensionSize();
        igIndex i = 0, j = 0, k = 0;
        igIndex vhs[4] = {0};
        igIndex st = 0;
        igIndex tmpvhs[4] = {0, 1, 1 + size[0] * size[1], size[0] * size[1]};
        int faceIndex = 0;
        int VolumeIndex = 0;
        // ij面的定义
        tmpvhs[1] = 1;
        tmpvhs[2] = 1 + size[0];
        tmpvhs[3] = size[0];
        k = 0;
        for (j = 0; j < size[1] - 1; j++) {
            st = j * size[0];
            VolumeIndex = j * (size[0] - 1);
            for (i = 0; i < size[0] - 1; i++) {
                if (!CellVis || CellVis[VolumeIndex]) {
                    for (int it = 0; it < 4; it++) { vhs[it] = st + tmpvhs[it]; }
                    Quads->AddCellIds(vhs, 4);
                    f2c.emplace_back(VolumeIndex);
                }
                st++;
                VolumeIndex++;
            }
        }
        k = size[2] - 1;
        if (k > 0) {
            for (j = 0; j < size[1] - 1; ++j) {
                st = j * size[0] + k * size[0] * size[1];
                VolumeIndex = j * (size[0] - 1) + (k - 1) * (size[0] - 1) * (size[1] - 1);
                for (i = 0; i < size[0] - 1; i++) {
                    if (!CellVis || CellVis[VolumeIndex]) {
                        for (int it = 0; it < 4; it++) { vhs[it] = st + tmpvhs[it]; }
                        Quads->AddCellIds(vhs, 4);
                        f2c.emplace_back(VolumeIndex);
                    }
                    st++;
                    VolumeIndex++;
                }
            }
        }

        // ik方向面的定义
        tmpvhs[1] = 1;
        tmpvhs[2] = 1 + size[0] * size[1];
        tmpvhs[3] = size[0] * size[1];
        j = 0;
        for (k = 0; k < size[2] - 1; k++) {
            st = j * size[0] + k * size[0] * size[1];
            VolumeIndex = k * (size[0] - 1) * (size[1] - 1);
            for (i = 0; i < size[0] - 1; i++) {
                if (!CellVis || CellVis[VolumeIndex]) {
                    for (int it = 0; it < 4; it++) { vhs[it] = st + tmpvhs[it]; }
                    Quads->AddCellIds(vhs, 4);
                    f2c.emplace_back(VolumeIndex);
                }
                st++;
                VolumeIndex++;
            }
        }
        j = size[1] - 1;
        if (j > 0) {
            for (k = 0; k < size[2] - 1; k++) {
                st = j * size[0] + k * size[0] * size[1];
                VolumeIndex = k * (size[0] - 1) * (size[1] - 1) + (j - 1) * (size[0] - 1);
                for (i = 0; i < size[0] - 1; i++) {
                    if (!CellVis || CellVis[VolumeIndex]) {
                        for (int it = 0; it < 4; it++) { vhs[it] = st + tmpvhs[it]; }
                        Quads->AddCellIds(vhs, 4);
                        f2c.emplace_back(VolumeIndex);
                    }
                    st++;
                    VolumeIndex++;
                }
            }
        }

        // jk方向面的定义
        tmpvhs[1] = size[0];
        tmpvhs[2] = size[0] + size[0] * size[1];
        tmpvhs[3] = size[0] * size[1];
        i = 0;
        for (k = 0; k < size[2] - 1; k++) {
            st = i + k * size[0] * size[1];
            VolumeIndex = k * (size[0] - 1) * (size[1] - 1);
            for (j = 0; j < size[1] - 1; j++) {
                if (!CellVis || CellVis[VolumeIndex]) {
                    for (int it = 0; it < 4; it++) { vhs[it] = st + tmpvhs[it]; }
                    Quads->AddCellIds(vhs, 4);
                    f2c.emplace_back(VolumeIndex);
                }
                st += size[0];
                VolumeIndex += size[0] - 1;
            }
        }
        i = size[0] - 1;
        if (i > 0) {
            for (k = 0; k < size[2] - 1; k++) {
                st = i + k * size[0] * size[1];
                VolumeIndex = k * (size[0] - 1) * (size[1] - 1) + i - 1;
                for (j = 0; j < size[1] - 1; j++) {
                    if (!CellVis || CellVis[VolumeIndex]) {
                        for (int it = 0; it < 4; it++) { vhs[it] = st + tmpvhs[it]; }
                        Quads->AddCellIds(vhs, 4);
                        f2c.emplace_back(VolumeIndex);
                    }
                    st += size[0];
                    VolumeIndex += size[0] - 1;
                }
            }
        }
    }
};
int ModelGeometryFilter::ExecuteWithStructuredMesh(DataObject::Pointer input, SurfaceMesh::Pointer& output,
                                                   SurfaceMesh::Pointer exc, bool* extracFace) {
    ;
    StructuredMesh::Pointer Mesh = DynamicCast<StructuredMesh>(input);
    Mesh->GenStructuredCellConnectivities();
    if (Mesh->GetDimension() != 3) {
        ConvertToSurfaceMeshFilter::Pointer filter = ConvertToSurfaceMeshFilter::New();
        filter->SetInput(Mesh);
        filter->Execute();
        bool result = this->ExecuteWithSurfaceMesh(filter->GetSurfaceMesh(), output, exc);
        if (result == 0) {
            output = filter->GetSurfaceMesh();
            return 1;
        }
    }
    //igDebug("Input has " << Mesh->GetNumberOfPoints() << " points and "
    //                     << Mesh->GetNumberOfCells() << " cells.");
    igIndex i = 0, j = 0, k = 0;
    igIndex64 cellId = 0, pointId = 0;
    igIndex64 numCells = Mesh->GetNumberOfCells();
    igIndex64 numInputPts = Mesh->GetNumberOfPoints();
    igIndex64 numOutputPts = 0;
    auto inPoints = Mesh->GetPoints();
    auto outPoints = inPoints;
    auto inAllDataArray = input->GetAttributeSet();
    auto outAllDataArray = AttributeSet::New();
    CellArray::Pointer Polygons = CellArray::New();
    CharArray::Pointer CellVisibleArray = CharArray::New();
    char* CellVisible = ComputeCellVisibleArray(CellVisibleArray, inPoints, Mesh->GetCells());
    if (CellVisible) { return this->ExecuteWithVolumeMesh(input, output); }
    unsigned char* cellGhosts = nullptr;
    unsigned char* pointGhosts = nullptr;

    auto* extract =
            new ExtractSG(Mesh, CellVisible, cellGhosts, pointGhosts, this->Merging, this->RemoveGhostInterfaces);
    extract->Execute();
    Polygons = extract->Quads;

    CompositeCellAttribute(extract->f2c, inAllDataArray, outAllDataArray);
    if (Merging) {
        ProcessPointMergin(extract, inPoints, outPoints, Polygons, outAllDataArray);
    } else {
        m_PointMap = nullptr;
    }
    output->SetPoints(outPoints);
    output->SetFaces(Polygons);
    output->SetAttributeSet(outAllDataArray);
    output->SetViewStyle(IG_WIREFRAME | IG_SURFACE);

    //igDebug("Extracted " << output->GetNumberOfPoints() << " points,"
    //                     << output->GetNumberOfFaces() << " faces.");
    delete extract;
    extract = nullptr;
    return 1;
}

int ModelGeometryFilter::ExecuteWithStructuredMesh(DataObject::Pointer input, SurfaceMesh::Pointer& output,
                                                   bool* extracFace) {
    return this->ExecuteWithStructuredMesh(input, output, nullptr, extracFace);
}

char* ModelGeometryFilter::ComputeCellVisibleArray(CharArray::Pointer& CellVisibleArray, Points::Pointer inPoints,
                                                   CellArray::Pointer Cells, UnsignedIntArray::Pointer Types) {
    IGsize numCells = Cells ? Cells->GetNumberOfCells() : 0;
    char* CellVisible = nullptr;
    if ((!CellClipping) && (!PointClipping) && (!ExtentClipping) && (!PlaneClipping)) {
        return nullptr;
    } else {
        if (!CellVisibleArray) CellVisibleArray = CharArray::New();
        CellVisibleArray->Resize(numCells);
        CellVisible = CellVisibleArray->RawPointer();
    }
    // Mark cells as being visible or not
    if (!CellVisible) return nullptr;
    unsigned int* types = nullptr;
    if (Types) types = Types->RawPointer();
    auto func = [&](igIndex start, igIndex end) -> void {
        igIndex vhs[256] = {0};
        igIndex vnum = 0;
        Point x;
        igIndex cellId = 0, pointId = 0;
        igIndex i = 0, j = 0;
        for (cellId = start; cellId < end; cellId++) {
            CellVisible[cellId] = 1;
            if (CellClipping && (cellId < CellMinimum || cellId > CellMaximum)) {
                CellVisible[cellId] = 0;
            } else {
                vnum = Cells->GetCellIds(cellId, vhs);
                if (types && types[cellId] == IG_POLYHEDRON) {
                    int faceInfoIndex = 1;
                    j = 0;
                    for (i = 1; i < vnum; i++) {
                        if (i == faceInfoIndex) {
                            faceInfoIndex += vhs[i] + 1;
                            continue;
                        }
                        vhs[j++] = vhs[i];
                    }
                    vnum = j;
                }
                for (i = 0; i < vnum; i++) {
                    pointId = vhs[i];
                    x = inPoints->GetPoint(pointId);
                    if (PointClipping && (pointId < PointMinimum || pointId > PointMaximum)) {
                        CellVisible[cellId] = 0;
                        break;
                    } else if (ExtentClipping && !ExtentClippingFlip &&
                               (x[0] < Extent[0] || x[0] > Extent[1] || x[1] < Extent[2] || x[1] > Extent[3] ||
                                x[2] < Extent[4] || x[2] > Extent[5])) {
                        CellVisible[cellId] = 0;
                        break;
                    } else if (ExtentClipping && ExtentClippingFlip &&
                               (x[0] >= Extent[0] && x[0] <= Extent[1] && x[1] >= Extent[2] && x[1] <= Extent[3] &&
                                x[2] >= Extent[4] && x[2] <= Extent[5])) {
                        CellVisible[cellId] = 0;
                        break;
                    } else if (PlaneClipping && !PlaneClippingFlip &&
                               (/*dot product*/
                                ((x[0] - PlaneOrigin[0]) * PlaneNormal[0] + (x[1] - PlaneOrigin[1]) * PlaneNormal[1] +
                                 (x[2] - PlaneOrigin[2]) * PlaneNormal[2]) > 0.)) {
                        CellVisible[cellId] = 0;
                        break;
                    } else if (PlaneClipping && PlaneClippingFlip &&
                               (/*dot product*/
                                ((x[0] - PlaneOrigin[0]) * PlaneNormal[0] + (x[1] - PlaneOrigin[1]) * PlaneNormal[1] +
                                 (x[2] - PlaneOrigin[2]) * PlaneNormal[2]) <= 0.)) {
                        CellVisible[cellId] = 0;
                        break;
                    }
                }
            }
        }
    };
    ThreadPool::parallelFor(0, numCells, func);
    return CellVisibleArray->RawPointer();
}
void ModelGeometryFilter::ProcessPointMergin(ExtractCellBoundaries* extract, Points::Pointer inPoints,
                                             Points::Pointer& outPoints, CellArray::Pointer Polygons,
                                             AttributeSet::Pointer outAllDataArray) {
    outPoints = Points::New();
    extract->UpdatePointMap(Polygons, inPoints, outPoints);
    CompositePointAttribute(extract->GetPointMap()->RawPointer(), inPoints->GetNumberOfPoints(),
                            outPoints->GetNumberOfPoints(), outAllDataArray);
    m_PointMap = extract->GetPointMap();
}
void ModelGeometryFilter::CompositeCellAttribute(std::vector<igIndex>& F2C, AttributeSet::Pointer inAllDataArray,
                                                 AttributeSet::Pointer& outAllDataArray) {
    if (!outAllDataArray) { outAllDataArray = AttributeSet::New(); }
    igIndex i = 0;
    IGsize fcnt = F2C.size();
    auto f2c = F2C.data();
    auto inDataArrayNum = inAllDataArray->GetAllAttributes()->GetNumberOfElements();
    for (i = 0; i < inDataArrayNum; i++) {
        auto& inData = inAllDataArray->GetAttribute(i).pointer;
        ArrayObject::Pointer outData = inData;
        if (inAllDataArray->GetAttribute(i).attachmentType == IG_CELL) {
            auto newData = DoubleArray::New();
            newData->SetDimension(inData->GetDimension());
            newData->Resize(fcnt);
            newData->SetName(inData->GetName());
            auto func = [&](igIndex start, igIndex end) -> void {
                double tmp[IGAME_CELL_MAX_SIZE];
                for (igIndex i = start; i < end; i++) {
                    inData->GetElement(f2c[i], tmp);
                    newData->SetElement(i, tmp);
                }
            };
            ThreadPool::parallelFor(0, fcnt, func);
            outData = newData;
            outAllDataArray->AddAttribute(inAllDataArray->GetAttribute(i).type,
                                          inAllDataArray->GetAttribute(i).attachmentType, newData,
                                          inAllDataArray->GetAttribute(i).GetDataRange());
        } else {
            outAllDataArray->AddAttribute(
                    inAllDataArray->GetAttribute(i).type, inAllDataArray->GetAttribute(i).attachmentType,
                    inAllDataArray->GetAttribute(i).pointer, inAllDataArray->GetAttribute(i).GetDataRange());
        }
    }
}
void ModelGeometryFilter::CompositePointAttribute(igIndex* PointMap, IGsize oldPNum, IGsize newPNum,
                                                  AttributeSet::Pointer outAllDataArray) {
    igIndex i = 0;
    auto inDataArrayNum = outAllDataArray->GetAllAttributes()->GetNumberOfElements();
    for (i = 0; i < inDataArrayNum; i++) {
        auto& inData = outAllDataArray->GetAttribute(i).pointer;
        ArrayObject::Pointer outData = inData;
        if (outAllDataArray->GetAttribute(i).attachmentType == IG_POINT) {
            auto newData = DoubleArray::New();
            newData->SetDimension(inData->GetDimension());
            newData->Resize(newPNum);
            newData->SetName(inData->GetName());
            auto func = [&](igIndex start, igIndex end) -> void {
                double tmp[IGAME_CELL_MAX_SIZE];
                for (igIndex i = start; i < end; i++) {
                    if (PointMap[i] != -1) {
                        inData->GetElement(i, tmp);
                        newData->SetElement(PointMap[i], tmp);
                    }
                }
            };
            ThreadPool::parallelFor(0, oldPNum, func);
            outData = newData;
        }
        outAllDataArray->GetAttribute(i).GetDataRange();
        outAllDataArray->GetAttribute(i).pointer = outData;
    }
}
IGAME_NAMESPACE_END
