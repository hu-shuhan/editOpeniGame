// CPU-only resource/lifetime regression tests. Never create a GL context,
// upload a buffer, draw a frame, or load a model file.
#include "iGameDrawObject.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameFileIO.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
void Require(bool condition, const char* name) {
    if (!condition) { throw std::runtime_error(name); }
    std::cout << "PASS " << name << '\n';
}

class MetadataBuffer final : public iGame::GLBuffer {
public:
    using Pointer = iGame::SmartPointer<MetadataBuffer>;
    static Pointer New() { return new MetadataBuffer; }
    void SetMetadataHandle(unsigned handle) { m_Handle = handle; }
};

class ProbeMeshleter final : public iGame::Meshleter {
public:
    using Pointer = iGame::SmartPointer<ProbeMeshleter>;
    static Pointer New() { return new ProbeMeshleter; }
    void SetMetadataBuffer(iGame::GLBuffer::Pointer buffer) {
#ifdef GL_SUPPORTS_MESH_SHADER
        m_ColorBuffer = buffer;
#else
        m_CellColorVBO = buffer; // Previously omitted by ReleaseGpuBuffers.
#endif
    }
};

class ProbeSurface final : public iGame::SurfaceMesh {
public:
    using Pointer = iGame::SmartPointer<ProbeSurface>;
    static Pointer New() { return new ProbeSurface; }
    bool* destroyed{nullptr};
    void SelectCp() { m_AttributeIndex = 0; m_AttributeDimension = 0; m_UseColor = true; m_AttributeChanged = true; }
    void AliasOriginalArrays(iGame::FloatArray::Pointer cp) {
        m_Positions = m_Points->ConvertToArray();
        m_Colors = cp;
    }
    void AddFallbackSelfCycle() { m_RenderableMesh.SimplifiedMesh = this; }
    void PrepareCpuDisplay() {
        ConvertToDrawableData();
        if (m_RenderableMesh.SimplifiedMesh && m_RenderableMesh.SimplifiedMesh.get() != this)
            m_RenderableMesh.SimplifiedMesh->ConvertToDrawableData();
        ConvertToDrawableData(); // settle the shared color mapper timestamp
    }
    std::size_t LineIndexValueCount() const { return m_LineIndices->GetNumberOfValues(); }
    bool HasBuiltEdges() const { return m_Edges != nullptr; }
    void SetTestMeshleter(iGame::Meshleter::Pointer meshleter) { m_RenderableMesh.mMeshleter = meshleter; }
    bool DerivedStateEmpty() const {
        return !m_RenderableMesh.SurfaceMesh && !m_RenderableMesh.SimplifiedMesh && !m_RenderableMesh.mMeshleter &&
               m_Positions->GetNumberOfValues() == 0 && m_Colors->GetNumberOfValues() == 0 &&
               m_TriangleIndices->GetNumberOfValues() == 0 && m_TriangleEdgeMasks->GetNumberOfValues() == 0 &&
               !m_Flag && m_ReConvertToDrawableData && m_AttributeChanged && m_ForceGpuBufferUpload;
    }
    bool CpuDrawRebuilt() const {
        return m_Positions->GetNumberOfValues() == 9 && m_TriangleIndices->GetNumberOfValues() == 3 &&
               m_Colors->GetNumberOfValues() == 12 && m_TriangleEdgeMasks->GetNumberOfValues() == 1 &&
               !m_ReConvertToDrawableData && m_ForceGpuBufferUpload;
    }
private:
    ~ProbeSurface() override { if (destroyed) { *destroyed = true; } }
};

ProbeSurface::Pointer MakeTriangleSurface() {
    auto surface = ProbeSurface::New();
    auto points = iGame::Points::New();
    points->AddPoint(0.f, 0.f, 0.f);
    points->AddPoint(1.f, 0.f, 0.f);
    points->AddPoint(0.f, 1.f, 0.f);
    auto faces = iGame::CellArray::New();
    faces->AddCellId3(0, 1, 2);
    surface->SetPoints(points);
    surface->SetFaces(faces);
    return surface;
}

// BUG: C/S lazy edges and mask availability affected ordinary models as well.
// Local models must eagerly prepare main's line indices; remote opt-in must be
// per object and propagate to derived data without changing another local model.
// Fix commit: 待提交 (C/S rendering isolation).
// BUG: opting in after VTM AddSubDataObject was too late for eager preparation.
// Verify the same real file opened locally stays local, and remote pieces are
// already opted in before their extracted surface is constructed.
// Fix commit: 待提交 (C/S rendering isolation).
void CheckFileEntryIsolation() {
    namespace fs = std::filesystem;
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto dir = fs::temp_directory_path() / ("igame-remote-policy-" + std::to_string(stamp));
    Require(fs::create_directory(dir), "reader-policy-fixture-directory-created");
    const auto piece = dir / "piece.vtu";
    const auto manifest = dir / "model.vtm";
    { std::ofstream f(piece); f << R"(<VTKFile type="UnstructuredGrid" version="0.1" byte_order="LittleEndian"><UnstructuredGrid><Piece NumberOfPoints="3" NumberOfCells="1"><Points><DataArray type="Float32" NumberOfComponents="3" format="ascii">0 0 0 1 0 0 0 1 0</DataArray></Points><Cells><DataArray type="Int32" Name="connectivity" format="ascii">0 1 2</DataArray><DataArray type="Int32" Name="offsets" format="ascii">3</DataArray><DataArray type="UInt8" Name="types" format="ascii">5</DataArray></Cells></Piece></UnstructuredGrid></VTKFile>)"; }
    { std::ofstream f(manifest); f << R"(<VTKFile type="vtkMultiBlockDataSet" version="1.0"><vtkMultiBlockDataSet><DataSet index="0" file="piece.vtu"/></vtkMultiBlockDataSet></VTKFile>)"; }
    const auto path = manifest.u8string();
    const std::string utf8(reinterpret_cast<const char*>(path.data()), path.size());
    auto localData = iGame::FileIO::ReadFile(utf8);
    auto remoteData = iGame::FileIO::ReadRemoteFile(utf8);
    auto* local = dynamic_cast<iGame::DrawObject*>(localData.get());
    auto* remote = dynamic_cast<iGame::DrawObject*>(remoteData.get());
    Require(local && remote && !local->GetRemoteRenderingEnabled() && remote->GetRemoteRenderingEnabled(),
            "local-and-remote-file-entry-policies-are-independent");
    auto* localPiece = dynamic_cast<iGame::DrawObject*>(local->SubDataObjectIteratorBegin()->second.get());
    auto* remotePiece = dynamic_cast<iGame::DrawObject*>(remote->SubDataObjectIteratorBegin()->second.get());
    Require(localPiece && remotePiece && !localPiece->GetRemoteRenderingEnabled() &&
                remotePiece->GetRemoteRenderingEnabled() && remotePiece->GetRenderableObject()->GetRemoteRenderingEnabled(),
            "remote-VTM-piece-and-eager-shell-inherit-policy");
    local->ReleaseDrawableResources(); remote->ReleaseDrawableResources();
    fs::remove(manifest); fs::remove(piece); fs::remove(dir);
}

void CheckLazyWireframeGeometry() {
    auto local = MakeTriangleSurface();
    local->ConvertToDrawableData();
    Require(!local->GetRemoteRenderingEnabled() && local->HasBuiltEdges() &&
                local->LineIndexValueCount() == 6 && local->IsUseSinglePassWireframeRendering(),
            "ordinary-model-keeps-main-edges-and-mask-selection-without-GPU");
    auto surface = MakeTriangleSurface();
    surface->SetRemoteRenderingEnabled(true);
    surface->ConvertToDrawableData();
    Require(!surface->HasBuiltEdges() && surface->LineIndexValueCount() == 0,
            "surface-only-display-skips-edge-topology-and-line-indices");

    surface->SetViewStyle(IG_SURFACE | IG_WIREFRAME);
    surface->ConvertToDrawableData();
    Require(!surface->HasBuiltEdges() && surface->LineIndexValueCount() == 0,
            "opaque-surface-wireframe-uses-single-pass-without-line-indices");

    surface->SetViewStyle(IG_WIREFRAME);
    surface->ConvertToDrawableData();
    Require(surface->HasBuiltEdges() && surface->LineIndexValueCount() == 6,
            "pure-wireframe-builds-explicit-line-indices-on-demand");

    auto transparent = MakeTriangleSurface();
    transparent->SetRemoteRenderingEnabled(true);
    transparent->SetViewStyle(IG_SURFACE | IG_WIREFRAME);
    transparent->SetTransparency(0.5f);
    transparent->ConvertToDrawableData();
    Require(transparent->HasBuiltEdges() && transparent->LineIndexValueCount() == 6,
            "transparent-wireframe-fallback-builds-line-indices-on-demand");
}

class PreparedGrid final : public iGame::UnstructuredMesh {
public:
    using Pointer = iGame::SmartPointer<PreparedGrid>;
    static Pointer New() { return new PreparedGrid; }
    void PrepareCpuDisplay() {
        ConvertToDrawableData();
        if (m_RenderableMesh.SurfaceMesh) m_RenderableMesh.SurfaceMesh->ConvertToDrawableData();
        if (m_RenderableMesh.SimplifiedMesh) m_RenderableMesh.SimplifiedMesh->ConvertToDrawableData();
        if (m_RenderableMesh.SurfaceMesh) m_RenderableMesh.SurfaceMesh->ConvertToDrawableData();
    }
};

void CheckCpuReleaseAndRebuild() {
    bool destroyed = false;
    auto root = iGame::DrawObject::New();
    auto surface = ProbeSurface::New();
    surface->destroyed = &destroyed;
    auto points = iGame::Points::New();
    points->AddPoint(0.f, 0.f, 0.f);
    points->AddPoint(1.f, 0.f, 0.f);
    points->AddPoint(0.f, 1.f, 0.f);
    auto faces = iGame::CellArray::New();
    faces->AddCellId3(0, 1, 2);
    auto cp = iGame::FloatArray::New();
    cp->SetName("PressureCoefficient");
    cp->AddValue(-1.f); cp->AddValue(0.f); cp->AddValue(1.f);
    surface->SetPoints(points);
    surface->SetFaces(faces);
    surface->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, cp);
    surface->SelectCp();
    root->AddSubDataObject(surface);
    surface->ConvertToDrawableData(); // CPU arrays only, including real meshleter input cycle.
    surface->AddFallbackSelfCycle();
    surface->AliasOriginalArrays(cp);
    surface->SetRenderWithMeshlet(true);
    root->SetRemoteRenderingEnabled(true);

    const auto pointsTime = points->GetMTime().GetMTime();
    const auto coordinatesTime = points->ConvertToArray()->GetMTime().GetMTime();
    const auto facesTime = faces->GetMTime().GetMTime();
    const auto cpTime = cp->GetMTime().GetMTime();
    const auto coordinates = points->RawPointer();
    const auto connections = faces->GetCellIdArray()->RawPointer();
    auto* attributes = surface->GetAttributeSet();
    Require(!root->HasGpuResources(), "cpu-conversion-does-not-allocate-GPU-resources");
    root->ReleaseDrawableResources();
    Require(!root->HasGpuResources() && surface->DerivedStateEmpty(), "recursive-release-clears-derived-arrays-and-self-cycles");
    Require(surface->GetPoints().get() == points.get() && surface->GetFaces() == faces.get() &&
            surface->GetAttributeSet() == attributes && attributes->GetAttribute(0).pointer.get() == cp.get(),
            "release-preserves-original-object-and-array-identities");
    Require(points->RawPointer() == coordinates && faces->GetCellIdArray()->RawPointer() == connections &&
            points->GetNumberOfPoints() == 3 && faces->GetNumberOfCells() == 1 && cp->GetNumberOfValues() == 3 &&
            points->GetPoint(1)[0] == 1.f && connections[2] == 2 && cp->GetValue(0) == -1.f && cp->GetValue(2) == 1.f,
            "release-does-not-reset-aliased-original-storage");
    Require(points->GetMTime().GetMTime() == pointsTime &&
            points->ConvertToArray()->GetMTime().GetMTime() == coordinatesTime &&
            faces->GetMTime().GetMTime() == facesTime && cp->GetMTime().GetMTime() == cpTime,
            "release-preserves-raw-data-timestamps");
    Require(surface->GetRenderWithMeshlet() && surface->IsUseColor() && surface->GetAttributeIndex() == 0,
            "release-preserves-scalar-and-meshlet-display-selection");
    root->ReleaseDrawableResources();
    Require(surface->DerivedStateEmpty() && !root->HasGpuResources(), "CPU-only-release-is-idempotent-without-context");
    surface->SetRenderWithMeshlet(false);
    surface->ConvertToDrawableData();
    Require(surface->CpuDrawRebuilt() && !root->HasGpuResources(), "same-parsed-surface-rebuilds-geometry-and-Cp-with-next-upload-forced");
    root->ReleaseDrawableResources();

    // Read-only HasGpuResources inspects handle metadata. Reset the synthetic
    // handle before every assertion/destructor: no GL entry point is invoked.
    auto metadata = MetadataBuffer::New();
    auto meshleter = ProbeMeshleter::New();
    meshleter->SetInput(surface);
    meshleter->SetMetadataBuffer(metadata);
    surface->SetTestMeshleter(meshleter);
    metadata->SetMetadataHandle(123);
    const bool detected = root->HasGpuResources();
    metadata->SetMetadataHandle(0);
    Require(detected, "GPU-metadata-inspection-reaches-composite-child-meshleter-cell-buffer");
    root->ReleaseDrawableResources();
    Require(!meshleter->GetInput() && !meshleter->HasGpuResources(), "release-clears-even-externally-held-meshleter-input");
    root->ClearSubDataObject();
    surface = nullptr;
    Require(destroyed, "released-surface-is-destroyed-after-last-external-owner");
}

void CheckPreparedCpuRetention() {
    bool destroyed = false;
    auto root = iGame::DrawObject::New();
    auto surface = ProbeSurface::New();
    surface->destroyed = &destroyed;
    auto points = iGame::Points::New();
    points->AddPoint(0.f, 0.f, 0.f); points->AddPoint(1.f, 0.f, 0.f); points->AddPoint(0.f, 1.f, 0.f);
    auto cells = iGame::CellArray::New(); cells->AddCellId3(0, 1, 2);
    auto cp = iGame::FloatArray::New(); cp->SetName("PressureCoefficient");
    cp->AddValue(-1); cp->AddValue(0); cp->AddValue(1);
    surface->SetPoints(points); surface->SetFaces(cells);
    surface->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, cp);
    surface->SelectCp(); root->AddSubDataObject(surface);
    root->SetRemoteRenderingEnabled(true);
    surface->PrepareCpuDisplay();
    auto lod = surface->GetRenderableObject(true);
    const auto before = root->InspectCpuDisplayCache();
    if (!before.ready) std::cerr << "Prepared state: " << before.notReadyReason << '\n';
    Require(before.ready, "prepared-CPU-geometry-and-scalar-ready");
    Require(before.estimatedBytes > root->GetRealMemorySize(), "prepared-budget-includes-derived-arrays");
    root->ReleaseGpuResourcesKeepCpuData();
    root->ReleaseGpuResourcesKeepCpuData();
    const auto retained = root->InspectCpuDisplayCache();
    Require(retained.ready && retained.signature == before.signature && retained.estimatedBytes == before.estimatedBytes,
            "GPU-only-release-retains-CPU-array-identities-timestamps-LOD-and-colors");
    Require(!root->HasGpuResources() && surface->GetRenderableObject(true).get() == lod.get(),
            "GPU-only-release-preserves-simplified-object-with-no-GPU-handles");
    surface->PrepareCpuDisplay();
    Require(root->InspectCpuDisplayCache().signature == before.signature,
            "next-CPU-conversion-does-not-rebuild-prepared-data");
    cp->Modified();
    Require(root->InspectCpuDisplayCache().signature != before.signature, "prepared-signature-detects-scalar-edit");
    surface->ForceReConvertToDrawableData();
    Require(!root->InspectCpuDisplayCache().ready, "dirty-geometry-not-upload-only-ready");
    root->ReleaseDrawableResources();
    lod = nullptr; root->ClearSubDataObject(); surface = nullptr;
    Require(destroyed, "prepared-cache-eviction-breaks-retained-ownership-cycles");
}

void CheckUnstructuredShellRelease() {
    auto mesh = iGame::UnstructuredMesh::New();
    auto points = iGame::Points::New();
    points->AddPoint(0.f, 0.f, 0.f); points->AddPoint(1.f, 0.f, 0.f); points->AddPoint(0.f, 1.f, 0.f);
    auto cells = iGame::CellArray::New(); cells->AddCellId3(0, 1, 2);
    auto types = iGame::UnsignedIntArray::New(); types->AddValue(iGame::IG_TRIANGLE);
    mesh->SetPoints(points); mesh->SetCells(cells, types);
    mesh->ConvertToDrawableData();
    auto firstShell = mesh->GetRenderableObject();
    Require(firstShell.get() != mesh.get(), "unstructured-surface-shell-created-without-GL");
    firstShell->ConvertToDrawableData();
    mesh->ReleaseDrawableResources();
    Require(!mesh->HasGpuResources() && !firstShell->HasGpuResources() &&
            mesh->GetRenderableObject().get() == mesh.get() && mesh->GetCells().get() == cells.get() &&
            mesh->GetPoints().get() == points.get(), "release-clears-derived-shell-without-discarding-source-grid");
    mesh->ConvertToDrawableData();
    Require(mesh->GetRenderableObject().get() != mesh.get() && mesh->GetRenderableObject().get() != firstShell.get(),
            "unstructured-source-recreates-a-fresh-renderable-shell");
    mesh->ReleaseDrawableResources();
}

void CheckPreparedGridRetention() {
    auto grid = PreparedGrid::New();
    grid->SetRemoteRenderingEnabled(true);
    auto points = iGame::Points::New();
    points->AddPoint(0.f, 0.f, 0.f); points->AddPoint(1.f, 0.f, 0.f); points->AddPoint(0.f, 1.f, 0.f);
    auto cells = iGame::CellArray::New(); cells->AddCellId3(0, 1, 2);
    auto types = iGame::UnsignedIntArray::New(); types->AddValue(iGame::IG_TRIANGLE);
    grid->SetPoints(points); grid->SetCells(cells, types);
    grid->PrepareCpuDisplay();
    auto shell = grid->GetRenderableObject(); auto lod = grid->GetRenderableObject(true);
    const auto before = grid->InspectCpuDisplayCache();
    Require(before.ready, "VTU-shell-and-LOD-prepared-ready");
    Require(shell->GetRemoteRenderingEnabled() && lod->GetRemoteRenderingEnabled(),
            "remote-rendering-policy-propagates-to-extracted-shell-and-LOD");
    grid->ReleaseGpuResourcesKeepCpuData();
    grid->PrepareCpuDisplay();
    const auto after = grid->InspectCpuDisplayCache();
    Require(after.ready && after.signature == before.signature && grid->GetRenderableObject().get() == shell.get() &&
                grid->GetRenderableObject(true).get() == lod.get(), "VTU-reopen-retains-extracted-shell-and-LOD-without-rebuild");
    grid->ReleaseDrawableResources();
}
}

int main() {
    try {
        CheckFileEntryIsolation();
        CheckLazyWireframeGeometry();
        CheckCpuReleaseAndRebuild();
        CheckUnstructuredShellRelease();
        CheckPreparedCpuRetention();
        CheckPreparedGridRetention();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
