#pragma once

#include <IQCore/igQtExportModule.h>
#include "iGameModel.h"
#include "iGameScene.h"
#include <QString>
#include <array>
#include <cstdint>
#include <vector>

// GUI-thread-only, single-entry RAM cache. It strongly owns the original
// DataObject (including its existing drawable/GPU resources), never a Model or
// Scene. Removing a model/tree item therefore does not discard the cached data.
// Capture after the first frame has settled. LookupData validates metadata;
// Lookup additionally finds an existing Scene model, but never mounts a new one.
// Neither lookup reads array values, rebuilds drawable data, or remaps scalars.
//
// The caller MUST validate the originating GL context before reuse, Clear before
// Scene/context destruction, and release/replace/destroy this cache with that GL
// context current on the GUI thread. Scene/pool addresses are non-owning tokens,
// not lifetime handles; context-generation tracking belongs to the caller.
//
// IMPORTANT: iGame's MTime is not recursively aggregated, and some raw/in-place
// array and deletion-mask edits do not call Modified(). The metadata snapshot
// below cannot detect such edits. Use only for explicitly opted-in static
// datasets and call Clear() conservatively before editing/animation/filtering.
// It is not a content hash or a general modification detector.
class IG_QT_MODULE_EXPORT igQtResidentModelCache
{
public:
    void Capture(iGame::Scene::Pointer scene, iGame::Model::Pointer model,
                 const QString& key, const QString& datasetPath);
    // Store an already parsed dataset without creating/adding a Model or doing
    // any rendering. All descendants must be free of GPU resources; callers
    // downgrading rendered data must first ReleaseDrawableResources() with the
    // originating context current. First rendering changes derived metadata,
    // so refresh via Capture after that frame completes.
    void CaptureCpu(iGame::Scene::Pointer scene, iGame::DataObject::Pointer data,
                    const QString& key, const QString& datasetPath);
    // Opt-in prepared display cache. Call the mounted variant after a frame;
    // the detached variant after ReleaseGpuResourcesKeepCpuData(). Falls back
    // to ordinary raw-data metadata if the CPU drawing state is not ready.
    void CapturePrepared(iGame::Scene::Pointer scene, iGame::Model::Pointer model,
                         const QString& key, const QString& datasetPath);
    void CapturePreparedCpu(iGame::Scene::Pointer scene, iGame::DataObject::Pointer data,
                            const QString& key, const QString& datasetPath);
    bool HasPreparedCpuData() const;
    // Refresh the identity/display snapshot after a remote model is detached
    // from the Scene.  Tree removal may update harmless MTimes while the
    // retained CPU draw arrays remain valid; this keeps the prepared entry
    // reusable without rebuilding geometry.
    bool RefreshPreparedCpuData(iGame::Scene::Pointer scene, const QString& key,
                                QString& reason);
    // Eligibility for this first CPU-preload implementation: a static triangle
    // surface with one Float32 PressureCoefficient value per point on each leaf.
    // Does not require a Scene, Model, selected scalar, visibility, or GL context.
    // Checks sizes and first/last type/offset metadata only; full connectivity and
    // uniform cell-type validation remain the reader/offline validator's job.
    static bool ValidateCpuSurface(iGame::DataObject* data, QString& reason);
    iGame::DataObject::Pointer LookupData(iGame::Scene::Pointer scene,
                                        const QString& key, QString& reason) const;
    // A null Model with reason "resident-data-not-mounted" is NOT a data-cache
    // miss: LookupData can return it for the caller to mount into a new Model.
    iGame::Model::Pointer Lookup(iGame::Scene::Pointer scene,
                                const QString& key, QString& reason) const;
    void Clear();
    bool HasEntry() const;
    QString DatasetPath() const;
    QString Key() const;
    // Unvalidated retained pointer, for lifecycle/eviction only. Opening a
    // dataset must use LookupData; PeekData does not check identity or MTime.
    iGame::DataObject::Pointer PeekData() const;
    // Prepared variants include extracted surfaces, LOD and drawing arrays
    // (conservative capacity estimate; shared source data may be counted twice).
    // Legacy variants snapshot GetRealMemorySize(), in BYTES. This
    // estimates original CPU arrays; it omits some reserved capacity, drawable
    // arrays, LOD, parsing temporaries, GPU memory and allocator overhead. It is
    // NOT a hard memory limit or process private-byte/working-set measurement.
    std::uint64_t MemoryBytes() const;

private:
    struct Stamp {
        QString role;
        QString name;
        // Object address, copied MTime, identity/type/count/dimension/flags.
        std::array<std::uint64_t, 9> fields{};
        bool operator==(const Stamp& other) const;
    };

    static bool BuildSnapshot(iGame::DataObject* object,
                              std::vector<Stamp>& output, QString& reason);
    void CaptureData(iGame::Scene::Pointer scene, iGame::DataObject::Pointer data,
                     const QString& key, const QString& datasetPath, bool cpuOnly);

    const iGame::Scene* m_Scene{nullptr};
    const iGame::Object* m_ModelPool{nullptr};
    iGame::DataObject::Pointer m_DataObject;
    DataObjectId m_DataObjectId{};
    QString m_Key;
    QString m_DatasetPath;
    std::uint64_t m_MemoryBytes{0};
    std::vector<Stamp> m_Snapshot;
    void CaptureDisplayState();
    bool m_PreparedCpu{false};
    std::vector<std::uint64_t> m_DisplaySignature;
};
