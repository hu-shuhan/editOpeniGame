// Headless metadata/lifetime tests. No Scene::Initialize, renderer, GL context,
// model files, array-value hashing, or GPU upload is used.
#include <IQCore/igQtResidentModelCache.h>
#include <IQCore/igQtRemoteSurfaceSnapshot.h>
#include "iGameCommand.h"
#include "iGameCellType.h"
#include "iGameFileIO.h"
#include "iGamePoints.h"
#include "iGameUnstructuredMesh.h"
#include <iostream>
#include <stdexcept>

namespace {
class ProbeData final : public iGame::DataObject {
public:
    using Pointer = iGame::SmartPointer<ProbeData>;
    static Pointer New() { return new ProbeData; }
    iGame::Points::Pointer GetPoints() override { return points; }
    iGame::CellArray::Pointer GetCellArray() override { return cells; }
    void AddChild(DataObject::Pointer child) {
        if (!m_SubDataObjectsHelper) m_SubDataObjectsHelper = SubDataObjectsHelper::New();
        m_SubDataObjectsHelper->AddSubDataObject(child);
    }
    iGame::Points::Pointer points = iGame::Points::New();
    iGame::CellArray::Pointer cells = iGame::CellArray::New();
    bool* destroyed{nullptr};
private:
    ~ProbeData() override { if (destroyed) *destroyed = true; }
};

class ProbeModel final : public iGame::Model {
public:
    using Pointer = iGame::SmartPointer<ProbeModel>;
    static Pointer New() { return new ProbeModel; }
    bool* destroyed{nullptr};
private:
    ~ProbeModel() override { if (destroyed) *destroyed = true; }
};

class ProbeScene final : public iGame::Scene {
public:
    using Pointer = iGame::SmartPointer<ProbeScene>;
    static Pointer New() { return new ProbeScene; }
    bool* destroyed{nullptr};
private:
    ~ProbeScene() override { if (destroyed) *destroyed = true; }
};

class LargeMemoryEstimateData final : public iGame::SurfaceMesh {
public:
    using Pointer = iGame::SmartPointer<LargeMemoryEstimateData>;
    static Pointer New() { return new LargeMemoryEstimateData; }
    IGsize GetRealMemorySize() override { return (IGsize{1} << 33) + 123; }
};

void Require(bool condition, const char* name) {
    if (!condition) throw std::runtime_error(name);
    std::cout << "PASS " << name << '\n';
}

unsigned RegisterHidden(iGame::Scene::Pointer scene, iGame::Model::Pointer model) {
    model->SetScene(scene);
    model->SetVisibility(false);
    // Scene::AddModel also performs drawable/camera work. Direct pool insertion
    // exercises the same ownership and handle lookup without that rendering path.
    return scene->GetModelList()->AllocateObject(model);
}

class PreparedSurface final : public iGame::SurfaceMesh {
public:
    using Pointer = iGame::SmartPointer<PreparedSurface>;
    static Pointer New() { return new PreparedSurface; }
    void Prepare() {
        m_AttributeIndex = 0; m_AttributeDimension = 0; m_UseColor = true;
        m_AttributeChanged = true;
        ConvertToDrawableData();
        if (m_RenderableMesh.SimplifiedMesh && m_RenderableMesh.SimplifiedMesh.get() != this)
            m_RenderableMesh.SimplifiedMesh->ConvertToDrawableData();
        ConvertToDrawableData();
    }
};

void CheckPreparedCpuCache() {
    auto mesh = PreparedSurface::New();
    auto points = iGame::Points::New();
    points->AddPoint(0, 0, 0); points->AddPoint(1, 0, 0); points->AddPoint(0, 1, 0);
    auto faces = iGame::CellArray::New(); faces->AddCellId3(0, 1, 2);
    auto cp = iGame::FloatArray::New(); cp->SetName("PressureCoefficient");
    cp->AddValue(-1); cp->AddValue(0); cp->AddValue(1);
    mesh->SetPoints(points); mesh->SetFaces(faces);
    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, cp);
    mesh->Prepare();
    auto scene = iGame::Scene::New();
    auto model = iGame::Model::New(); model->SetDataObject(mesh);
    const auto id = RegisterHidden(scene, model);
    igQtResidentModelCache cache;
    const QString key = QStringLiteral("prepared-cpu-v1");
    cache.CapturePrepared(scene, model, key, QStringLiteral("prepared.vtp"));
    Require(cache.HasPreparedCpuData(), "prepared-cache-captures-settled-drawing-state");
    Require(cache.MemoryBytes() > mesh->GetRealMemorySize(), "prepared-cache-charges-derived-memory");
    QString reason;
    scene->RemoveModel(id); model->SetScene(nullptr);
    mesh->ReleaseGpuResourcesKeepCpuData();
    Require(cache.LookupData(scene, key, reason).get() == mesh.get(), "GPU-demotion-preserves-prepared-cache-hit");
    cache.CapturePreparedCpu(scene, mesh, key, QStringLiteral("prepared.vtp"));
    Require(cache.HasPreparedCpuData() && !mesh->HasGpuResources(), "detached-prepared-cache-has-no-GPU-resources");
    Require(!cache.LookupData(scene, key + "-changed", reason), "prepared-cache-version-mismatch-rejected");
    mesh->GetColorMapper()->Modified();
    Require(!cache.LookupData(scene, key, reason) && reason == QStringLiteral("prepared-display-state-changed"),
            "prepared-cache-color-mapping-edit-invalidates");
    mesh->Prepare(); cache.CapturePreparedCpu(scene, mesh, key, QStringLiteral("prepared.vtp"));
    cp->Modified();
    Require(!cache.LookupData(scene, key, reason), "prepared-cache-physical-scalar-edit-invalidates");
    mesh->ReleaseDrawableResources(); cache.Clear(); scene->Finalize();
    Require(!cache.HasPreparedCpuData(), "prepared-clear-removes-display-signature");
}

void CheckCpuPreload() {
    // Parse before even constructing a Scene. No GUI application or OpenGL
    // context exists, so any actual GPU call would fail this headless test.
    const std::string xml = R"xml(<?xml version="1.0"?>
<VTKFile type="UnstructuredGrid" version="0.1" byte_order="LittleEndian">
  <UnstructuredGrid><Piece NumberOfPoints="3" NumberOfCells="1">
    <PointData><DataArray type="Float32" Name="PressureCoefficient"
      NumberOfComponents="1" format="ascii">-1 0 1</DataArray></PointData>
    <CellData/>
    <Points><DataArray type="Float32" NumberOfComponents="3"
      format="ascii">0 0 0 1 0 0 0 1 0</DataArray></Points>
    <Cells>
      <DataArray type="Int32" Name="connectivity" format="ascii">0 1 2</DataArray>
      <DataArray type="Int32" Name="offsets" format="ascii">3</DataArray>
      <DataArray type="UInt8" Name="types" format="ascii">5</DataArray>
    </Cells>
  </Piece></UnstructuredGrid>
</VTKFile>)xml";
    auto data = iGame::FileIO::ReadVTUFromMemory(xml.data(), xml.size());
    auto* mesh = dynamic_cast<iGame::UnstructuredMesh*>(data.get());
    Require(mesh && mesh->GetNumberOfPoints() == 3 && mesh->GetNumberOfCells() == 1,
            "cpu-preload-parses-before-scene-exists");
    Require(!mesh->HasGpuResources(), "cpu-preload-parser-has-no-gpu-resources");
    const auto* coordinates = mesh->GetPoints()->ConvertToArray()->RawPointer();
    const auto* connectivity = mesh->GetCellArray()->GetCellIdArray()->RawPointer();
    auto cp = iGame::DynamicCast<iGame::FloatArray>(mesh->GetAttributeSet()->GetAttribute(0).pointer);
    Require(cp && cp->GetName() == "PressureCoefficient" && cp->GetNumberOfValues() == 3,
            "cpu-preload-keeps-physical-point-scalar");
    const auto* scalarValues = cp->RawPointer();
    QString reason;
    Require(!mesh->IsUseColor(), "cpu-preload-parser-has-no-selected-scalar-mapping");
    const bool validCpuSurface = igQtResidentModelCache::ValidateCpuSurface(data.get(), reason);
    if (!validCpuSurface) std::cerr << "CPU surface guard rejected parsed VTU: "
                                    << reason.toStdString() << '\n';
    Require(validCpuSurface, "cpu-surface-guard-does-not-require-scene-or-scalar-mapping");
    Require(mesh->GetCellTypes()->GetValue(0) == iGame::IG_TRIANGLE,
            "cpu-preload-reader-converts-vtk-type-to-internal-triangle");
    auto& cpAttribute = mesh->GetAttributeSet()->GetAttribute(0);
    cp->SetName("UnrelatedScalar");
    Require(!igQtResidentModelCache::ValidateCpuSurface(data.get(), reason), "cpu-surface-missing-cp-rejected");
    cp->SetName("PressureCoefficient");
    cpAttribute.attachmentType = IG_CELL;
    Require(!igQtResidentModelCache::ValidateCpuSurface(data.get(), reason), "cpu-surface-cell-cp-rejected");
    cpAttribute.attachmentType = IG_POINT;
    cpAttribute.isDeleted = true;
    Require(!igQtResidentModelCache::ValidateCpuSurface(data.get(), reason), "cpu-surface-deleted-cp-rejected");
    cpAttribute.isDeleted = false;
    cp->Resize(2);
    Require(!igQtResidentModelCache::ValidateCpuSurface(data.get(), reason), "cpu-surface-short-point-cp-rejected");
    cp->Resize(3);
    cp->SetValue(2, 1.0);
    auto frames = data->GetTimeFrames();
    frames->AddTimeStep(0.f, iGame::StringArray::New(), StreamingType::NONE);
    Require(!igQtResidentModelCache::ValidateCpuSurface(data.get(), reason), "cpu-surface-time-series-rejected");
    data->SetTimeFrames(nullptr);
    auto cells = mesh->GetCellArray();
    iGame::UnsignedIntArray::Pointer cellTypes = mesh->GetCellTypes();
    auto tetraCells = iGame::CellArray::New();
    auto tetraIds = iGame::IdArray::New();
    tetraIds->AddId(0); tetraIds->AddId(1); tetraIds->AddId(2); tetraIds->AddId(0);
    tetraCells->SetData(tetraIds, 4);
    mesh->SetCells(tetraCells, cellTypes);
    Require(!igQtResidentModelCache::ValidateCpuSurface(data.get(), reason), "cpu-surface-four-node-cells-rejected");
    mesh->SetCells(cells, cellTypes);
    mesh->GetCellTypes()->SetValue(0, iGame::IG_TETRA);
    Require(!igQtResidentModelCache::ValidateCpuSurface(data.get(), reason), "cpu-surface-nontriangle-type-rejected");
    mesh->GetCellTypes()->SetValue(0, iGame::IG_TRIANGLE);
    Require(igQtResidentModelCache::ValidateCpuSurface(data.get(), reason), "cpu-surface-valid-metadata-restored");
    mesh->ReleaseDrawableResources(); // Also safe for a never-rendered object.
    const auto expectedMemory = static_cast<std::uint64_t>(data->GetRealMemorySize());

    auto scene = iGame::Scene::New(); // No Initialize, Model, or scene insertion.
    igQtResidentModelCache cache;
    const QString key = QStringLiteral("cpu-package-sha:entry.vtu");
    cache.CaptureCpu(scene, data, key, QStringLiteral("entry.vtu"));
    Require(cache.HasEntry() && cache.PeekData().get() == data.get() && cache.Key() == key,
            "cpu-preload-captured-without-mounted-model");
    Require(scene->GetModelList()->GetObjectCount() == 0 &&
                cache.LookupData(scene, key, reason).get() == data.get(),
            "cpu-preload-data-hit-does-not-insert-model");
    Require(!cache.Lookup(scene, key, reason) && reason == QStringLiteral("resident-data-not-mounted"),
            "cpu-preload-model-lookup-does-not-mount");
    Require(cache.MemoryBytes() == expectedMemory && expectedMemory > 0,
            "cpu-preload-memory-estimate-is-byte-snapshot");
    Require(mesh->GetPoints()->ConvertToArray()->RawPointer() == coordinates &&
                mesh->GetCellArray()->GetCellIdArray()->RawPointer() == connectivity &&
                cp->RawPointer() == scalarValues && !mesh->HasGpuResources(),
            "cpu-preload-retains-original-array-storage-without-gpu");
    cp->Modified();
    Require(!cache.LookupData(scene, key, reason), "cpu-preload-scalar-modified-miss");
    cache.CaptureCpu(scene, data, key, QStringLiteral("entry.vtu"));
    mesh->GetPoints()->ConvertToArray()->Modified();
    Require(!cache.LookupData(scene, key, reason), "cpu-preload-coordinate-modified-miss");
    cache.Clear();
    Require(!cache.PeekData() && cache.Key().isEmpty() && cache.MemoryBytes() == 0,
            "cpu-preload-clear-resets-inspection-getters");

    auto largeEstimate = LargeMemoryEstimateData::New();
    largeEstimate->SetPoints(mesh->GetPoints());
    largeEstimate->SetFaces(mesh->GetCellArray());
    largeEstimate->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, cp);
    cache.CaptureCpu(scene, largeEstimate, key, QStringLiteral("large-estimate"));
    Require(cache.MemoryBytes() == (std::uint64_t{1} << 33) + 123,
            "cpu-memory-estimate-exceeds-32bit-without-large-allocation");
    auto cycleA = ProbeData::New();
    auto cycleB = ProbeData::New();
    cycleA->AddChild(cycleB); cycleB->AddChild(cycleA);
    cache.CaptureCpu(scene, cycleA, key, QStringLiteral("invalid-cycle"));
    Require(!cache.HasEntry(), "cpu-capture-rejects-cycle-before-recursive-memory-estimate");
    cycleA->ClearSubDataObject(); cycleB->ClearSubDataObject();
    cache.CaptureCpu(scene, data, key, QStringLiteral("entry.vtu"));
    scene->Finalize();
    Require(!cache.LookupData(scene, key, reason) && reason == QStringLiteral("scene-finalized"),
            "cpu-preload-finalized-scene-miss");
    cache.Clear();
}

// Regression (2026-09-22). Fix commit subject:
// fix: preserve C/S cache and support portable full-feature runtime
// Locate this regression in the existing test file:
// git log --format="%h %s" -S "void CheckSolidColorCacheReopen()" -- Examples/IO/ResidentModelCacheValidation.cpp
// Selecting a model row calls
// ViewCloudPicture(-1). A subsequent prepared-cache hit used to fail the Cp
// coloring guard and evict the cache after the frame. Preserve that legitimate
// solid-color state across repeated detach/reattach, while still rejecting
// malformed physical Cp arrays and unsupported geometry/display metadata.
void CheckSolidColorCacheReopen() {
    auto scene = iGame::Scene::New(); // No GL context or Scene::Initialize.
    auto mesh = PreparedSurface::New();
    auto points = iGame::Points::New();
    points->AddPoint(0, 0, 0); points->AddPoint(1, 0, 0); points->AddPoint(0, 1, 0);
    auto faces = iGame::CellArray::New(); faces->AddCellId3(0, 1, 2);
    auto cp = iGame::FloatArray::New(); cp->SetName("PressureCoefficient");
    cp->AddValue(-1); cp->AddValue(0); cp->AddValue(1);
    mesh->SetPoints(points); mesh->SetFaces(faces);
    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, cp);
    mesh->Prepare();
    auto model = iGame::Model::New(); model->SetDataObject(mesh);
    auto id = RegisterHidden(scene, model); model->SetVisibility(true);
    auto inspect = [&] { return igQtRemoteSurfaceSnapshot::Inspect(scene.get(), model.get()); };
    Require(inspect().valid, "reopen-colored-surface-eligible");
    const QString key = QStringLiteral("solid-color-reopen-v1");
    igQtResidentModelCache cache;
    cache.CapturePrepared(scene, model, key, QStringLiteral("solid.vtp"));
    Require(cache.HasPreparedCpuData(), "reopen-initial-prepared-cache-ready");

    // Use the actual model-tree operation, then settle CPU drawing state as a
    // normal frame does. No reparse, GPU upload or forced default coloring.
    model->ViewCloudPicture(-1);
    mesh->ConvertToDrawableData();
    Require(!mesh->IsUseColor() && mesh->GetCurrentAttributeIndex() == -1,
            "model-row-operation-disables-scalar-coloring");
    Require(inspect().valid, "solid-color-surface-remains-cache-eligible");
    QString reason;
    for (int round = 0; round < 3; ++round) {
        scene->RemoveModel(id); model->SetScene(nullptr);
        mesh->ReleaseGpuResourcesKeepCpuData();
        Require(cache.RefreshPreparedCpuData(scene, key, reason),
                "solid-color-detach-refreshes-prepared-cache");
        auto hit = cache.LookupData(scene, key, reason);
        Require(hit.get() == mesh.get() && cache.HasPreparedCpuData(),
                "solid-color-reopen-reuses-original-prepared-data");
        model = iGame::Model::New(); model->SetDataObject(hit);
        id = RegisterHidden(scene, model); model->SetVisibility(true);
        const auto snapshot = inspect();
        Require(snapshot.valid && snapshot.leafCount == 1 && snapshot.pointCount == 3 && snapshot.faceCount == 1,
                "solid-color-reattached-frame-passes-cache-admission");
        cache.CapturePrepared(scene, model, key, QStringLiteral("solid.vtp"));
        Require(cache.HasPreparedCpuData() && !mesh->IsUseColor() && mesh->GetCurrentAttributeIndex() == -1,
                "solid-color-frame-retains-cache-without-enabling-coloring");
    }

    // Dropping a display-selection requirement must not admit broken data.
    auto& attribute = mesh->GetAttributeSet()->GetAttribute(0);
    cp->SetName("UnrelatedScalar");
    Require(!inspect().valid, "solid-color-missing-cp-still-rejected");
    cp->SetName("PressureCoefficient");
    attribute.isDeleted = true;
    Require(!inspect().valid, "solid-color-deleted-cp-still-rejected");
    attribute.isDeleted = false;
    attribute.attachmentType = IG_CELL;
    Require(!inspect().valid, "solid-color-cell-cp-still-rejected");
    attribute.attachmentType = IG_POINT;
    cp->Resize(2);
    Require(!inspect().valid, "solid-color-truncated-cp-still-rejected");
    cp->Resize(3); cp->SetValue(2, 1);
    mesh->SetViewStyle(IG_WIREFRAME);
    Require(!inspect().valid, "solid-color-wireframe-still-rejected");
    mesh->SetViewStyle(IG_SURFACE);
    model->SetVisibility(false);
    Require(!inspect().valid, "solid-color-hidden-model-still-rejected");
    model->SetVisibility(true);
    Require(inspect().valid, "solid-color-valid-metadata-restored");
    model->ViewCloudPicture(0, 0);
    Require(inspect().valid && mesh->IsUseColor(), "cp-coloring-can-still-be-enabled");
    scene->RemoveModel(id); model->SetScene(nullptr);
    mesh->ReleaseDrawableResources(); cache.Clear(); scene->Finalize();
}

void CheckSurfaceSnapshotLogicalConnectivity() {
    auto scene = iGame::Scene::New(); // Deliberately never Initialize or Draw.
    auto surface = iGame::SurfaceMesh::New();
    auto points = iGame::Points::New();
    points->AddPoint(0.f, 0.f, 0.f); points->AddPoint(1.f, 0.f, 0.f);
    points->AddPoint(0.f, 1.f, 0.f); points->AddPoint(1.f, 1.f, 0.f);
    surface->SetPoints(points);
    auto faces = iGame::CellArray::New();
    faces->AddCellId3(0, 1, 2);
    faces->AddCellId3(1, 3, 2);
    faces->AddCellId3(0, 2, 3);
    surface->SetFaces(faces);
    auto cp = iGame::FloatArray::New();
    cp->SetName("PressureCoefficient");
    cp->AddValue(-1.f); cp->AddValue(0.f); cp->AddValue(1.f); cp->AddValue(0.5f);
    surface->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, cp);
    surface->ViewCloudPicture(scene.get(), 0, 0); // CPU selection metadata only.
    auto model = iGame::Model::New();
    model->SetScene(scene);
    model->SetDataObject(surface);
    model->SetVisibility(true);
    auto inspect = [&] { return igQtRemoteSurfaceSnapshot::Inspect(scene.get(), model.get()); };
    auto backing = faces->GetCellIdArray();
    const auto backingCount = backing->GetNumberOfIds();
    Require(faces->GetNumberOfCellIds() == 9 && backingCount > 9,
            "surface-add-cell-geometric-backing-exercised");
    auto snapshot = inspect();
    Require(snapshot.valid && snapshot.leafCount == 1 && snapshot.pointCount == 4 && snapshot.faceCount == 3,
            "surface-fixed-logical-connectivity-accepts-unused-backing");
    backing->Resize(8);
    Require(!inspect().valid, "surface-logical-connectivity-must-fit-backing");
    backing->Resize(backingCount);

    auto offsets = iGame::UnsignedIntArray::New();
    offsets->AddValue(0); offsets->AddValue(3); offsets->AddValue(6); offsets->AddValue(9);
    faces->SetData(backing, offsets);
    snapshot = inspect();
    Require(snapshot.valid && snapshot.variableArityLeafCount == 1 && snapshot.faceCount == 3,
            "surface-offset-logical-connectivity-accepts-unused-backing");
    offsets->Resize(3);
    Require(!inspect().valid, "surface-truncated-offset-metadata-rejected-before-read");
    offsets->Resize(4); offsets->SetValue(3, 9);
    offsets->SetValue(0, 1);
    Require(!inspect().valid, "surface-nonzero-first-offset-rejected");
    offsets->SetValue(0, 0); offsets->SetValue(1, 4);
    Require(!inspect().valid, "surface-invalid-first-triangle-boundary-rejected");
    offsets->SetValue(1, 3); offsets->SetValue(2, 5);
    Require(!inspect().valid, "surface-invalid-last-triangle-boundary-rejected");
    offsets->SetValue(2, 6); offsets->SetValue(3, 12);
    Require(!inspect().valid, "surface-logical-connectivity-count-mismatch-rejected");
    offsets->SetValue(3, 9);
    Require(inspect().valid, "surface-offset-metadata-restored-valid");
    faces->SetNumberOfCells(0);
    Require(!inspect().valid, "surface-empty-cells-rejected-without-end-offset-underflow");
    model->SetScene(nullptr);
    scene->Finalize();
}
}

int main() {
    try {
        CheckPreparedCpuCache();
        CheckCpuPreload();
        CheckSurfaceSnapshotLogicalConnectivity();
        CheckSolidColorCacheReopen();
        auto scene = iGame::Scene::New();
        auto otherScene = iGame::Scene::New();
        auto data = ProbeData::New();
        data->points->AddPoint(0.f, 0.f, 0.f);
        data->points->AddPoint(1.f, 0.f, 0.f);
        data->points->AddPoint(0.f, 1.f, 0.f);
        auto ids = iGame::IdArray::New();
        ids->AddId(0); ids->AddId(1); ids->AddId(2);
        data->cells->SetData(ids, 3);
        auto cp = iGame::FloatArray::New();
        cp->SetName("PressureCoefficient");
        cp->Resize(3);
        data->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, cp);
        auto model = iGame::Model::New();
        model->SetDataObject(data);
        const auto modelId = RegisterHidden(scene, model);
        igQtResidentModelCache cache;
        QString reason;
        const QString key = QStringLiteral("package-sha256:entry.vtp");
        auto capture = [&] { cache.Capture(scene, model, key, QStringLiteral("entry.vtp")); };
        auto hit = [&] { return cache.Lookup(scene, key, reason); };
        auto dataHit = [&] { return cache.LookupData(scene, key, reason); };
        Require(!cache.HasEntry() && !hit() && !dataHit(), "empty-cache-miss");
        capture();
        Require(cache.HasEntry() && cache.DatasetPath() == QStringLiteral("entry.vtp"), "capture-static-model");
        Require(hit().get() == model.get(), "same-resident-model-hit");
        Require(dataHit().get() == data.get(), "same-data-object-hit-without-copy");
        Require(!cache.Lookup(scene, QStringLiteral("different-sha"), reason), "package-key-miss");
        Require(!cache.Lookup(otherScene, key, reason), "other-scene-miss");
        Require(!cache.LookupData(scene, QStringLiteral("different-sha"), reason), "data-package-key-miss");
        Require(!cache.LookupData(otherScene, key, reason), "data-other-scene-miss");

        auto unrelated = iGame::Model::New();
        unrelated->SetDataObject(ProbeData::New());
        const auto unrelatedId = RegisterHidden(scene, unrelated);
        Require(hit().get() == model.get() && dataHit().get() == data.get(),
                "unrelated-model-added-keeps-cache");
        scene->RemoveModel(unrelatedId);
        unrelated->SetScene(nullptr);
        Require(hit().get() == model.get() && dataHit().get() == data.get(),
                "unrelated-model-removed-keeps-cache");

        data->Modified();
        Require(!hit(), "root-mtime-miss"); capture();
        data->SetUniqueDataObjectId();
        Require(!dataHit() && reason == QStringLiteral("data-object-identity-changed"),
                "data-identity-change-miss"); capture();
        data->points->SetPoint(0, 0.5f, 0.f, 0.f);
        Require(!hit(), "points-mtime-miss"); capture();
        data->points->ConvertToArray()->Modified();
        Require(!hit(), "coordinates-mtime-miss"); capture();
        cp->Modified();
        Require(!hit(), "scalar-mtime-miss"); capture();
        cp->Resize(4); // Resize need not call Modified: metadata still catches it.
        Require(!hit(), "scalar-size-miss-without-modified");
        cp->Resize(3); capture();
        cp->SetName("renamed");
        Require(!hit(), "scalar-name-miss-without-modified"); capture();
        data->GetAttributeSet()->GetAttribute(0).isDeleted = true;
        Require(!hit(), "attribute-deleted-flag-miss");
        data->GetAttributeSet()->GetAttribute(0).isDeleted = false; capture();
        data->GetAttributeSet()->GetAttribute(0).attachmentType = IG_CELL;
        Require(!hit(), "attribute-attachment-miss");
        data->GetAttributeSet()->GetAttribute(0).attachmentType = IG_POINT; capture();
        auto replacementIds = iGame::IdArray::New();
        replacementIds->AddId(0); replacementIds->AddId(1); replacementIds->AddId(2);
        data->cells->SetData(replacementIds, 3);
        Require(!hit(), "connectivity-array-replacement-miss"); capture();
        data->cells->SetFixedSize(2);
        Require(!hit(), "fixed-cell-size-miss-without-modified");
        data->cells->SetFixedSize(3); capture();

        auto child = ProbeData::New();
        data->AddChild(child);
        Require(!hit(), "child-added-miss"); capture();
        child->Modified();
        Require(!hit(), "child-mtime-miss"); capture();
        data->RemoveSubDataObject(child->GetDataObjectId());
        Require(!hit(), "child-removed-miss"); capture();
        model->SetVisibility(true);
        Require(hit().get() == model.get(), "display-visibility-retained");
        model->SetVisibility(false);

        // Document the known contract honestly: an unmarked, same-size write
        // is invisible to metadata. The caller MUST Clear before such edits.
        cp->SetValue(0, 42.0);
        Require(hit().get() == model.get(), "raw-write-requires-explicit-invalidation");
        cache.Clear();
        Require(!hit(), "explicit-invalidation-miss"); capture();
        model->SetDataObject(ProbeData::New());
        Require(!hit(), "model-data-replacement-miss");
        Require(dataHit().get() == data.get(), "model-replacement-does-not-replace-cached-data");
        model->SetDataObject(data); capture();

        auto emptyFrames = data->GetTimeFrames();
        Require(!hit(), "lazy-time-container-created-after-capture-miss");
        capture();
        Require(cache.HasEntry() && hit().get() == model.get(), "lazy-empty-time-container-cacheable");
        emptyFrames->Modified();
        Require(!hit(), "empty-time-container-mtime-miss"); capture();
        auto replacementFrames = iGame::StreamingData::New();
        data->SetTimeFrames(replacementFrames);
        Require(!hit(), "empty-time-container-replacement-miss"); capture();
        Require(hit().get() == model.get(), "replacement-empty-time-container-cacheable");
        const auto frameMTime = replacementFrames->GetMTime().GetMTime();
        replacementFrames->AddTimeStep(0.f, iGame::StringArray::New(), StreamingType::NONE);
        Require(replacementFrames->GetMTime().GetMTime() == frameMTime,
                "time-step-added-without-modified-exercised");
        Require(!hit() && reason == QStringLiteral("time-dependent-dataset-not-cacheable"),
                "actual-time-step-invalidates-static-cache");
        cache.Capture(scene, model, key, QStringLiteral("animated.vtp"));
        Require(!cache.HasEntry(), "time-dependent-dataset-not-captured");
        data->SetTimeFrames(nullptr); capture();

        bool deleteEvent = false;
        data->AddObserver(iGame::Command::DeleteEvent, [&] { deleteEvent = true; });
        scene->RemoveModel(modelId);
        Require(deleteEvent && !hit() && reason == QStringLiteral("resident-data-not-mounted"),
                "scene-remove-event-and-no-mounted-model");
        Require(cache.HasEntry() && dataHit().get() == data.get(), "scene-remove-retains-cached-data");
        const auto reusedId = RegisterHidden(scene, model);
        // Other handles were previously released, so do not assume FIFO reuse
        // returns the immediately removed handle. Data identity is authoritative.
        Require(hit().get() == model.get(), "removed-then-reinserted-same-model-hit");
        auto replacement = iGame::Model::New();
        replacement->SetDataObject(ProbeData::New());
        replacement->SetScene(scene);
        replacement->SetVisibility(false);
        scene->SetModelById(static_cast<int>(reusedId), replacement);
        Require(!hit() && dataHit().get() == data.get(), "reused-handle-different-data-not-returned");
        scene->RemoveModel(reusedId);
        model->SetScene(nullptr);
        replacement->SetScene(nullptr);

        auto remounted = iGame::Model::New();
        remounted->SetDataObject(dataHit());
        const auto remountedId = RegisterHidden(scene, remounted);
        Require(hit().get() == remounted.get(), "cached-data-remounted-in-new-model-hit");
        scene->RemoveModel(remountedId);
        remounted->SetScene(nullptr);
        cp->Modified();
        Require(!dataHit(), "detached-cached-scalar-mtime-miss");
        cache.Clear();

        bool destroyed = false;
        bool modelDestroyed = false;
        auto lifetimeData = ProbeData::New();
        lifetimeData->destroyed = &destroyed;
        const auto* lifetimeIdentity = lifetimeData.get();
        auto lifetimeModel = ProbeModel::New();
        lifetimeModel->destroyed = &modelDestroyed;
        lifetimeModel->SetDataObject(lifetimeData);
        const auto lifetimeId = RegisterHidden(scene, lifetimeModel);
        cache.Capture(scene, lifetimeModel, key, QStringLiteral("lifetime.vtp"));
        lifetimeData = nullptr;
        lifetimeModel = nullptr;
        scene->RemoveModel(lifetimeId);
        Require(modelDestroyed && !destroyed && !hit() && cache.HasEntry(),
                "cache-retains-data-but-not-removed-model");
        auto retainedData = dataHit();
        Require(retainedData.get() == lifetimeIdentity, "detached-data-identity-preserved");
        cache.Clear();
        Require(!destroyed && !cache.HasEntry(), "lookup-result-keeps-data-after-cache-clear");
        retainedData = nullptr;
        Require(destroyed, "data-released-after-cache-and-caller-release");

        bool sceneDestroyed = false;
        bool detachedDestroyed = false;
        auto lifetimeScene = ProbeScene::New();
        lifetimeScene->destroyed = &sceneDestroyed;
        auto detachedData = ProbeData::New();
        detachedData->destroyed = &detachedDestroyed;
        auto detachedModel = iGame::Model::New();
        detachedModel->SetDataObject(detachedData);
        RegisterHidden(lifetimeScene, detachedModel);
        cache.Capture(lifetimeScene, detachedModel, key, QStringLiteral("detached.vtp"));
        detachedModel = nullptr;
        detachedData = nullptr;
        lifetimeScene->Finalize();
        Require(!cache.LookupData(lifetimeScene, key, reason) &&
                    reason == QStringLiteral("scene-finalized"),
                "finalized-scene-data-safe-miss");
        lifetimeScene = nullptr;
        Require(sceneDestroyed && !detachedDestroyed && cache.HasEntry(),
                "cached-data-does-not-retain-scene");
        Require(!cache.LookupData(otherScene, key, reason), "destroyed-scene-token-never-dereferenced");
        cache.Clear();
        Require(detachedDestroyed, "clear-releases-detached-data");

        auto finalModel = iGame::Model::New();
        finalModel->SetDataObject(data);
        RegisterHidden(scene, finalModel);
        cache.Capture(scene, finalModel, key, QStringLiteral("final.vtp"));
        scene->Finalize();
        Require(!hit() && reason == QStringLiteral("scene-finalized"), "finalized-scene-safe-miss");
        finalModel->SetScene(nullptr);
        otherScene->Finalize();
        cache.Clear();
        Require(!cache.HasEntry() && cache.DatasetPath().isEmpty(), "clear-metadata");
        std::cout << "ResidentModelCacheValidation PASS (CPU metadata only)\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "ResidentModelCacheValidation FAIL: " << error.what() << '\n';
        return 1;
    }
}
