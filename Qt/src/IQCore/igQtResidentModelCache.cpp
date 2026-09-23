#include <IQCore/igQtResidentModelCache.h>

#include "iGameArrayObject.h"
#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGameCellType.h"
#include "iGamePoints.h"
#include "iGameUnstructuredMesh.h"
#include <functional>
#include <limits>
#include <unordered_set>

namespace {
std::uint64_t Address(const void* object)
{
    return static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(object));
}

QString ObjectName(const iGame::Object* object)
{
    return object ? QString::fromStdString(object->GetName()) : QString{};
}
}

bool igQtResidentModelCache::Stamp::operator==(const Stamp& other) const
{
    return role == other.role && name == other.name && fields == other.fields;
}

bool igQtResidentModelCache::ValidateCpuSurface(iGame::DataObject* data, QString& reason)
{
    reason.clear();
    std::unordered_set<iGame::DataObject*> visited;
    auto fail = [&](const QString& message) { reason = message; return false; };
    std::function<bool(iGame::DataObject*, const QString&, unsigned)> visit;
    visit = [&](iGame::DataObject* object, const QString& path, unsigned depth) {
        if (!object || depth > 256 || !visited.insert(object).second) {
            return fail(path + QStringLiteral(": null, repeated or cyclic object hierarchy"));
        }
        auto frames = object->PeekTimeFrames();
        if (frames && frames->GetTimeNum() != 0) {
            return fail(path + QStringLiteral(": time-dependent data is not CPU-cacheable"));
        }
        if (object->HasSubDataObject()) {
            for (auto it = object->SubDataObjectIteratorBegin();
                 it != object->SubDataObjectIteratorEnd(); ++it) {
                if (!visit(it->second.get(), path + QStringLiteral("/%1")
                               .arg(static_cast<qulonglong>(it->first)), depth + 1)) return false;
            }
            return true;
        }
        const auto type = object->GetDataObjectType();
        if (type != IG_SURFACE_MESH && type != IG_UNSTRUCTURED_MESH) {
            return fail(path + QStringLiteral(": only static triangle SurfaceMesh/VTU leaves are supported"));
        }
        auto points = object->GetPoints();
        auto cells = object->GetCellArray();
        if (!points || !cells) return fail(path + QStringLiteral(": missing points or cells"));
        const std::uint64_t pointCount = points->GetNumberOfPoints();
        const std::uint64_t cellCount = cells->GetNumberOfCells();
        if (!pointCount || !cellCount ||
            pointCount > std::numeric_limits<std::uint64_t>::max() / 3 ||
            cellCount > std::numeric_limits<IGuint>::max() / std::uint64_t{3}) {
            return fail(path + QStringLiteral(": empty geometry or unrepresentable triangle connectivity"));
        }
        auto coordinates = points->ConvertToArray();
        if (!coordinates || coordinates->GetDimension() != 3 ||
            coordinates->GetNumberOfValues() != pointCount * 3) {
            return fail(path + QStringLiteral(": coordinate metadata length mismatch"));
        }
        auto ids = cells->GetCellIdArray();
        const auto expectedIds = cellCount * 3;
        if (!ids || ids->GetNumberOfIds() < expectedIds) {
            return fail(path + QStringLiteral(": triangle connectivity exceeds backing storage"));
        }
        if (!cells->IsUseOffSet()) {
            if (cells->GetCellSize(0) != 3) {
                return fail(path + QStringLiteral(": cell arity must be three"));
            }
        } else {
            auto offsets = cells->GetOffset();
            if (!offsets || offsets->GetDimension() != 1 ||
                offsets->GetNumberOfValues() < cellCount + 1) {
                return fail(path + QStringLiteral(": missing/truncated triangle offset metadata"));
            }
            // Constant-size endpoint checks, never an O(number-of-cells) scan.
            if (offsets->GetValue(0) != 0 || offsets->GetValue(1) != 3 ||
                offsets->GetValue(cellCount - 1) != static_cast<double>(expectedIds - 3) ||
                offsets->GetValue(cellCount) != static_cast<double>(expectedIds)) {
                return fail(path + QStringLiteral(": first/last triangle offsets are invalid"));
            }
        }
        if (cells->GetNumberOfCellIds() != expectedIds) {
            return fail(path + QStringLiteral(": logical triangle connectivity length mismatch"));
        }
        if (auto* mesh = dynamic_cast<iGame::UnstructuredMesh*>(object)) {
            auto* types = mesh->GetCellTypes();
            // The reader converts VTK_TRIANGLE=5 to iGame::IG_TRIANGLE=4.
            // Interior uniformity is covered by reader/offline
            // validation, not by an expensive scan every time a cache is used.
            if (!types || types->GetNumberOfValues() != cellCount ||
                types->GetValue(0) != iGame::IG_TRIANGLE ||
                types->GetValue(cellCount - 1) != iGame::IG_TRIANGLE) {
                return fail(path + QStringLiteral(": VTU triangle cell-type metadata mismatch"));
            }
        }
        auto* attributes = object->GetAttributeSet();
        const int index = attributes ? attributes->GetAttributeIndex("PressureCoefficient") : -1;
        if (index < 0) return fail(path + QStringLiteral(": missing point PressureCoefficient"));
        const auto& attribute = attributes->GetAttribute(index);
        auto scalar = attribute.pointer;
        if (attribute.isDeleted || attribute.type != IG_SCALAR || attribute.attachmentType != IG_POINT ||
            !scalar || scalar->GetArrayType() != IG_FloatArray || scalar->GetDimension() != 1 ||
            scalar->GetNumberOfValues() != pointCount) {
            return fail(path + QStringLiteral(": PressureCoefficient must be one Float32 scalar per point"));
        }
        return true;
    };
    if (!visit(data, QStringLiteral("root"), 0)) return false;
    reason = QStringLiteral("static-point-PressureCoefficient-triangle-surface");
    return true;
}

bool igQtResidentModelCache::BuildSnapshot(iGame::DataObject* object,
                                          std::vector<Stamp>& output,
                                          QString& reason)
{
    output.clear();
    // A malformed cycle must not recurse forever. Repeated/shared child objects
    // still get an identity record, while their contents are inspected once.
    std::unordered_set<const iGame::DataObject*> visited;
    std::unordered_set<const iGame::DataObject*> active;
    auto stamp = [&](const QString& role, iGame::Object* value) -> Stamp& {
        Stamp record;
        record.role = role;
        record.name = ObjectName(value);
        record.fields[0] = Address(value);
        record.fields[1] = value ? value->GetMTime().GetMTime() : 0;
        output.push_back(std::move(record));
        return output.back();
    };
    auto array = [&](const QString& role, iGame::ArrayObject* value) {
        auto& record = stamp(role, value);
        if (!value) return;
        record.fields[2] = static_cast<std::uint64_t>(value->GetNumberOfElements());
        record.fields[3] = static_cast<std::uint64_t>(value->GetNumberOfValues());
        record.fields[4] = static_cast<std::uint64_t>(value->GetDimension());
        record.fields[5] = static_cast<std::uint64_t>(value->GetArrayType());
        record.fields[6] = static_cast<std::uint64_t>(value->GetArrayTypedSize());
    };

    std::function<bool(iGame::DataObject*, const QString&, unsigned)> visit;
    visit = [&](iGame::DataObject* current, const QString& path, unsigned depth) {
        if (depth > 256) {
            reason = QStringLiteral("hierarchy-depth-exceeded");
            return false;
        }
        auto& node = stamp(path, current);
        if (!current) return true;
        if (active.find(current) != active.end()) {
            reason = QStringLiteral("hierarchy-cycle");
            return false;
        }
        node.fields[2] = static_cast<std::uint64_t>(current->GetDataObjectId());
        node.fields[3] = static_cast<std::uint64_t>(current->GetDataObjectType());
        node.fields[4] = static_cast<std::uint64_t>(current->GetNumberOfSubDataObjects());
        if (!visited.insert(current).second) return true;
        active.insert(current);

        // Only access existing data, not getters that lazily create animation
        // state, ranges, derived surfaces, bounding boxes, or drawable arrays.
        auto frames = current->PeekTimeFrames();
        auto& frameRecord = stamp(path + QStringLiteral("/time-frames"), frames.get());
        const auto frameCount = frames ? frames->GetTimeNum() : 0;
        frameRecord.fields[2] = static_cast<std::uint64_t>(frameCount);
        // Animation UI initialization may lazily create an EMPTY container for
        // a static dataset. Keep its identity/MTime/count in the snapshot, but
        // reject only actual time series. AddTimeStep itself need not Modified().
        if (frameCount != 0) {
            reason = QStringLiteral("time-dependent-dataset-not-cacheable");
            return false;
        }

        auto points = current->GetPoints();
        auto& pointRecord = stamp(path + QStringLiteral("/points"), points.get());
        if (points) {
            pointRecord.fields[2] = static_cast<std::uint64_t>(points->GetNumberOfPoints());
            array(path + QStringLiteral("/coordinates"), points->ConvertToArray().get());
        }
        auto cells = current->GetCellArray();
        auto& cellRecord = stamp(path + QStringLiteral("/cells"), cells.get());
        if (cells) {
            cellRecord.fields[2] = static_cast<std::uint64_t>(cells->GetNumberOfCells());
            cellRecord.fields[3] = cells->IsUseOffSet() ? 1 : 0;
            // GetCellSize in fixed-size mode returns metadata, not array values.
            cellRecord.fields[4] = !cells->IsUseOffSet()
                ? static_cast<std::uint64_t>(cells->GetCellSize(0)) : 0;
            // IdArray derives from ElementArray, not ArrayObject.
            auto ids = cells->GetCellIdArray();
            auto& idsRecord = stamp(path + QStringLiteral("/connectivity"), ids.get());
            if (ids) {
                idsRecord.fields[2] = static_cast<std::uint64_t>(ids->GetNumberOfIds());
                idsRecord.fields[3] = sizeof(igIndex);
                idsRecord.fields[4] = Address(ids->RawPointer());
            }
            array(path + QStringLiteral("/offsets"), cells->GetOffset().get());
        }
        auto* unstructured = dynamic_cast<iGame::UnstructuredMesh*>(current);
        array(path + QStringLiteral("/cell-types"),
              unstructured ? unstructured->GetCellTypes() : nullptr);

        auto* attributes = current->GetAttributeSet();
        auto& attrRecord = stamp(path + QStringLiteral("/attributes"), attributes);
        if (attributes) {
            const auto count = attributes->GetNumberOfAttributes();
            attrRecord.fields[2] = static_cast<std::uint64_t>(count);
            // Record backing storage identity too: SetAllAttributes need not
            // modify the AttributeSet timestamp.
            auto backing = attributes->GetAllAttributes();
            auto& backingRecord = stamp(path + QStringLiteral("/attribute-storage"), backing.get());
            backingRecord.fields[2] = backing
                ? static_cast<std::uint64_t>(backing->GetNumberOfElements()) : 0;
            for (std::size_t index = 0; index < count; ++index) {
                const auto& attr = attributes->GetAttribute(index);
                const QString attrPath = path + QStringLiteral("/attribute-%1").arg(static_cast<qulonglong>(index));
                auto& descriptor = stamp(attrPath, attr.pointer.get());
                descriptor.fields[2] = static_cast<std::uint64_t>(attr.type);
                descriptor.fields[3] = static_cast<std::uint64_t>(attr.attachmentType);
                descriptor.fields[4] = attr.isDeleted ? 1 : 0;
                array(attrPath + QStringLiteral("/values"), attr.pointer.get());
                // Range locks / color mapper / current selection are display
                // state, deliberately retained rather than reset on a hit.
            }
        }

        if (current->HasSubDataObject()) {
            for (auto it = current->SubDataObjectIteratorBegin();
                 it != current->SubDataObjectIteratorEnd(); ++it) {
                const QString childPath = path + QStringLiteral("/child-%1").arg(static_cast<qulonglong>(it->first));
                if (!visit(it->second.get(), childPath, depth + 1)) return false;
            }
        }
        active.erase(current);
        return true;
    };
    return visit(object, QStringLiteral("root"), 0);
}

void igQtResidentModelCache::Capture(iGame::Scene::Pointer scene,
                                    iGame::Model::Pointer model,
                                    const QString& key,
                                    const QString& datasetPath)
{
    Clear();
    if (!scene || !model || key.isEmpty()) return;
    auto pool = scene->GetModelList();
    if (!pool || model->GetScene().get() != scene.get()) return;
    bool resident = false;
    for (auto it = pool->Begin(); it != pool->End(); ++it) {
        if (it->second.get() == model.get()) {
            resident = true;
            break;
        }
    }
    auto data = model->GetDataObject();
    if (!resident || !data) return;
    CaptureData(scene, data, key, datasetPath, false);
}

void igQtResidentModelCache::CaptureCpu(iGame::Scene::Pointer scene,
                                       iGame::DataObject::Pointer data,
                                       const QString& key,
                                       const QString& datasetPath)
{
    Clear();
    CaptureData(scene, data, key, datasetPath, true);
}

void igQtResidentModelCache::CapturePrepared(iGame::Scene::Pointer scene,
                                            iGame::Model::Pointer model,
                                            const QString& key, const QString& datasetPath)
{
    Capture(scene, model, key, datasetPath);
    CaptureDisplayState();
}

void igQtResidentModelCache::CapturePreparedCpu(iGame::Scene::Pointer scene,
                                               iGame::DataObject::Pointer data,
                                               const QString& key, const QString& datasetPath)
{
    CaptureCpu(scene, data, key, datasetPath);
    CaptureDisplayState();
}

void igQtResidentModelCache::CaptureDisplayState()
{
    auto* draw = dynamic_cast<iGame::DrawObject*>(m_DataObject.get());
    if (!HasEntry() || !draw) return;
    auto state = draw->InspectCpuDisplayCache();
    // Charge derived allocations even if this display is not eligible for the
    // upload-only path. Never admit tens of GB of LOD/indices as "zero bytes".
    m_MemoryBytes = state.estimatedBytes;
    QString reason;
    m_PreparedCpu = state.ready && ValidateCpuSurface(m_DataObject.get(), reason);
    if (m_PreparedCpu) m_DisplaySignature = std::move(state.signature);
}

bool igQtResidentModelCache::HasPreparedCpuData() const { return HasEntry() && m_PreparedCpu; }

bool igQtResidentModelCache::RefreshPreparedCpuData(iGame::Scene::Pointer scene,
                                                    const QString& key,
                                                    QString& reason)
{
    reason.clear();
    if (!HasPreparedCpuData()) { reason = QStringLiteral("prepared-data-not-present"); return false; }
    if (key != m_Key) { reason = QStringLiteral("package-key-changed"); return false; }
    if (!scene || scene.get() != m_Scene) { reason = QStringLiteral("scene-changed"); return false; }
    auto pool = scene->GetModelList();
    if (!pool || pool.get() != m_ModelPool) {
        reason = QStringLiteral("scene-model-pool-replaced");
        return false;
    }

    std::vector<Stamp> snapshot;
    if (!BuildSnapshot(m_DataObject.get(), snapshot, reason)) return false;
    auto* draw = dynamic_cast<iGame::DrawObject*>(m_DataObject.get());
    if (!draw) { reason = QStringLiteral("prepared-display-missing"); return false; }
    const auto state = draw->InspectCpuDisplayCache();
    if (!state.ready) {
        reason = QStringLiteral("prepared-display-state-changed");
        return false;
    }
    QString surfaceReason;
    if (!ValidateCpuSurface(m_DataObject.get(), surfaceReason)) {
        reason = surfaceReason;
        return false;
    }
    m_Snapshot = std::move(snapshot);
    m_DisplaySignature = state.signature;
    m_MemoryBytes = state.estimatedBytes;
    m_PreparedCpu = true;
    return true;
}

void igQtResidentModelCache::CaptureData(iGame::Scene::Pointer scene,
                                        iGame::DataObject::Pointer data,
                                        const QString& key,
                                        const QString& datasetPath,
                                        bool cpuOnly)
{
    if (!scene || !data || key.isEmpty()) return;
    auto pool = scene->GetModelList();
    if (!pool) return;
    std::vector<Stamp> snapshot;
    QString reason;
    if (cpuOnly && !ValidateCpuSurface(data.get(), reason)) return;
    if (!BuildSnapshot(data.get(), snapshot, reason)) return;
    if (cpuOnly) {
        // A composite root need not itself be a DrawObject. Visit those roots
        // too; HasGpuResources also checks each drawable's derived meshes.
        std::vector<iGame::DataObject*> pending{data.get()};
        std::unordered_set<iGame::DataObject*> checked;
        while (!pending.empty()) {
            auto* current = pending.back();
            pending.pop_back();
            if (!current || !checked.insert(current).second) continue;
            auto* drawable = dynamic_cast<iGame::DrawObject*>(current);
            if (drawable && drawable->HasGpuResources()) return;
            if (current->HasSubDataObject()) {
                for (auto it = current->SubDataObjectIteratorBegin();
                     it != current->SubDataObjectIteratorEnd(); ++it) {
                    pending.push_back(it->second.get());
                }
            }
        }
    }
    // IGsize is uint64 in this build. Do not scan mesh values or construct
    // draw arrays for this estimate. This is not a process-wide hard limit.
    const auto memoryBytes = static_cast<std::uint64_t>(data->GetRealMemorySize());
    m_Scene = scene.get();
    m_ModelPool = pool.get();
    // Retain only the existing dataset. Model owns its Scene, so retaining a
    // Model would also keep the entire Scene (and unrelated models) alive.
    m_DataObject = data;
    m_DataObjectId = data->GetDataObjectId();
    m_Key = key;
    m_DatasetPath = datasetPath;
    m_MemoryBytes = memoryBytes;
    m_Snapshot = std::move(snapshot);
}

iGame::DataObject::Pointer igQtResidentModelCache::LookupData(iGame::Scene::Pointer scene,
                                                           const QString& key,
                                                           QString& reason) const
{
    reason.clear();
    if (!HasEntry()) { reason = QStringLiteral("no-resident-entry"); return nullptr; }
    if (key != m_Key) { reason = QStringLiteral("package-key-changed"); return nullptr; }
    if (!scene || scene.get() != m_Scene) { reason = QStringLiteral("scene-changed"); return nullptr; }
    // Only dereference the caller's live Scene, never the cached raw tokens.
    // Finalize() clears its pool before GL context destruction.
    auto pool = scene->GetModelList();
    if (!pool) { reason = QStringLiteral("scene-finalized"); return nullptr; }
    // Removing/reinserting this model and adding unrelated models are allowed.
    // Pool identity still guards against replacement of the Scene's container;
    // its mutable MTime and recycled numeric handles are deliberately ignored.
    if (pool.get() != m_ModelPool) {
        reason = QStringLiteral("scene-model-pool-replaced");
        return nullptr;
    }
    auto data = m_DataObject;
    if (data->GetDataObjectId() != m_DataObjectId) {
        reason = QStringLiteral("data-object-identity-changed");
        return nullptr;
    }
    std::vector<Stamp> snapshot;
    if (!BuildSnapshot(data.get(), snapshot, reason)) return nullptr;
    if (snapshot.size() != m_Snapshot.size()) {
        reason = QStringLiteral("dataset-structure-changed");
        return nullptr;
    }
    for (std::size_t index = 0; index < snapshot.size(); ++index) {
        if (!(snapshot[index] == m_Snapshot[index])) {
            reason = QStringLiteral("dataset-metadata-changed:%1").arg(snapshot[index].role);
            return nullptr;
        }
    }
    if (m_PreparedCpu) {
        auto* draw = dynamic_cast<iGame::DrawObject*>(data.get());
        if (!draw) { reason = QStringLiteral("prepared-display-missing"); return nullptr; }
        const auto state = draw->InspectCpuDisplayCache();
        if (!state.ready || state.signature != m_DisplaySignature) {
            reason = QStringLiteral("prepared-display-state-changed");
            return nullptr;
        }
    }
    reason = QStringLiteral("resident-data-hit");
    return data;
}

iGame::Model::Pointer igQtResidentModelCache::Lookup(iGame::Scene::Pointer scene,
                                                   const QString& key,
                                                   QString& reason) const
{
    auto data = LookupData(scene, key, reason);
    if (!data) return nullptr;
    // Match data identity, not an old Model pointer or recyclable pool handle.
    // A caller may already have mounted the same cached dataset in a new Model.
    auto pool = scene->GetModelList();
    for (auto it = pool->Begin(); it != pool->End(); ++it) {
        auto model = it->second;
        if (model && model->GetScene().get() == scene.get() &&
            model->GetDataObject().get() == data.get()) {
            reason = QStringLiteral("resident-model-hit");
            return model;
        }
    }
    reason = QStringLiteral("resident-data-not-mounted");
    return nullptr;
}

void igQtResidentModelCache::Clear()
{
    m_Scene = nullptr;
    m_ModelPool = nullptr;
    // May release the last owner of rendered data and call glDeleteBuffers.
    // The caller is responsible for making the originating GL context current.
    m_DataObject = nullptr;
    m_DataObjectId = {};
    m_Key.clear();
    m_DatasetPath.clear();
    m_MemoryBytes = 0;
    m_Snapshot.clear();
    m_PreparedCpu = false;
    m_DisplaySignature.clear();
}

bool igQtResidentModelCache::HasEntry() const { return m_Scene && m_ModelPool && m_DataObject; }
QString igQtResidentModelCache::DatasetPath() const { return m_DatasetPath; }
QString igQtResidentModelCache::Key() const { return m_Key; }
iGame::DataObject::Pointer igQtResidentModelCache::PeekData() const { return m_DataObject; }
std::uint64_t igQtResidentModelCache::MemoryBytes() const { return m_MemoryBytes; }
