#include <IQCore/igQtFileLoader.h>
#include <IQCore/igQtPackageDownloader.h>
#include <IQCore/igQtRemotePackageLoader.h>
#include <IQCore/igQtRemoteModelIdentity.h>
#include <IQCore/igQtResidentModelCache.h>
#include <IQCore/igQtRemoteSurfaceSnapshot.h>
#include <IQWidgets/igQtRenderWidget.h>
#include <iGameScene.h>
#include <iGameFileIO.h>
#include <iGameDrawObject.h>
#include <QFileInfo>
#include <QDebug>
#include <QElapsedTimer>
#include <QTimer>
#include <QPointer>
#include <QThread>
#include <QScopedValueRollback>

namespace {
quint64 RemoteUserModelCount(iGame::Scene* scene)
{
    if (!scene || !scene->GetModelList()) { return 0; }
    quint64 count = 0;
    auto* axes = scene->GetCenterAxesModel().get();
    auto pool = scene->GetModelList();
    for (auto it = pool->Begin(); it != pool->End(); ++it) {
        if (it->second && it->second->GetDataObject().get() != axes) { ++count; }
    }
    return count;
}

// Nested-safe: callbacks may arrive from paintGL/frameSwapped or the renderer
// teardown. Never unbind the context that was already current on entry.
class RemoteCacheGLScope {
public:
    explicit RemoteCacheGLScope(igQtRenderWidget* widget)
        : previous(QOpenGLContext::currentContext()),
          surface(previous ? previous->surface() : nullptr), renderer(widget)
    {
        if (renderer && renderer->context() && renderer->context() != previous.data()) {
            renderer->makeCurrent();
            switched = QOpenGLContext::currentContext() != previous.data();
        }
    }
    ~RemoteCacheGLScope()
    {
        if (!switched) { return; }
        if (previous && surface) { previous->makeCurrent(surface); }
        else if (auto* current = QOpenGLContext::currentContext()) { current->doneCurrent(); }
    }
private:
    QPointer<QOpenGLContext> previous;
    QSurface* surface{nullptr};
    QPointer<igQtRenderWidget> renderer;
    bool switched{false};
};
}

struct igQtResidentRemoteState {
    struct RetiredDisplay {
        iGame::DataObject::Pointer data;
        QPointer<QOpenGLContext> context;
    };
    std::vector<RetiredDisplay> retiredDisplays;
    igQtPackageDownloader* probe{nullptr};
    igQtResidentModelCache cache;
    QPointer<igQtRenderWidget> renderer;
    QPointer<QOpenGLContext> cacheContext;
    QElapsedTimer timer;
    QString address, packageId, cacheDirectory, identity, datasetPath, probeFailure;
    quint16 port{0};
    quint64 requestId{0};
    quint64 editEpoch{0}, startEditEpoch{0};
    iGame::Scene* preparedScene{nullptr};
    iGame::Model* preparedModel{nullptr};
    iGame::DataObject* preparedObject{nullptr};
    unsigned int preparedModelId{0};
    quint64 preparedLeaves{0}, preparedPoints{0}, preparedFaces{0};
    bool enabled{true}, active{false}, querying{false}, validInfo{false};
    bool strictValidation{false}, cacheable{false};
    bool clearingCache{false}, cacheContextFailed{false};
    bool awaitingFrame{false}, memoryHit{false}, cancelled{false};
    bool cpuOnly{true}, preloadOnly{false};
    quint64 limitBytes{96ull * 1024 * 1024 * 1024};
};

namespace {
iGame::Model::Pointer MountedRemoteData(iGame::Scene* scene, iGame::DataObject* data)
{
    if (!scene || !data || !scene->GetModelList()) return nullptr;
    auto pool = scene->GetModelList();
    for (auto it = pool->Begin(); it != pool->End(); ++it) {
        if (it->second && it->second->GetDataObject().get() == data) return it->second;
    }
    return nullptr;
}

bool HasRemoteGpu(iGame::DataObject* data)
{
    auto* draw = dynamic_cast<iGame::DrawObject*>(data);
    return draw && draw->HasGpuResources();
}
}

igQtFileLoader::~igQtFileLoader()
{
    ClearResidentRemoteCache();
    ReleaseRetiredRemoteResources(true);
    if (m_ResidentRemote && m_ResidentRemote->probe) {
        disconnect(m_ResidentRemote->probe, nullptr, this, nullptr);
        m_ResidentRemote->probe->Shutdown();
    }
}

void igQtFileLoader::InitializeResidentRemoteSupport()
{
    m_ResidentRemote = std::make_shared<igQtResidentRemoteState>();
    auto& state = *m_ResidentRemote;
    state.probe = new igQtPackageDownloader(this);
    connect(state.probe, &igQtPackageDownloader::StatusChanged, this, [this](const QString& message) {
        igDebug("[RemoteMemoryCache] {}", message.toStdString());
        emit RemotePackageStatusChanged(message);
    });
    connect(state.probe, &igQtPackageDownloader::ValidatedPackageInfoReceived, this,
            [this](const QString& address, quint16 port, const QString& packageId,
                   const QString& fileName, const QString& version, const QByteArray& sha, quint64 bytes) {
        auto& s = *m_ResidentRemote;
        if (!s.active || !s.querying || s.cancelled) { return; }
        s.validInfo = true;
        s.identity = igQtRemoteModelIdentity(address, port, packageId, fileName, version, sha, bytes);
        igDebug("[RemoteMemoryCache] Validated INFO request={} elapsed_ms={} archive_bytes={}",
                s.requestId, s.timer.elapsed(), bytes);
    });
    connect(state.probe, &igQtPackageDownloader::DownloadFailed, this, [this](const QString& message) {
        m_ResidentRemote->probeFailure = message;
    });
    connect(state.probe, &igQtPackageDownloader::RunningChanged, this, [this](bool running) {
        if (!running && m_ResidentRemote->querying) { ContinueResidentRemoteRequest(); }
    });
    // The download performs its own INFO request after a probe miss. Cache
    // exactly the version actually downloaded, not a potentially stale probe.
    connect(m_RemotePackageLoader, &igQtRemotePackageLoader::ValidatedPackageInfoReceived, this,
            [this](const QString& address, quint16 port, const QString& packageId,
                   const QString& fileName, const QString& version, const QByteArray& sha, quint64 bytes) {
        auto& s = *m_ResidentRemote;
        if (s.active && !s.querying && !s.cancelled) {
            s.identity = igQtRemoteModelIdentity(address, port, packageId, fileName, version, sha, bytes);
        }
    });
    // Normal tree deletion has several UI entry points. A GUI-thread timer
    // catches all of them; explicit deletion/benchmark paths also demote inline.
    auto* demoteTimer = new QTimer(this);
    demoteTimer->setInterval(500);
    connect(demoteTimer, &QTimer::timeout, this, &igQtFileLoader::ReleaseDetachedRemoteGpuResources);
    demoteTimer->start();
}

bool igQtFileLoader::PreloadRemotePackage(const QString& address, quint16 port,
                                         const QString& packageId, const QString& directory)
{
    if (!ResidentRemoteEnabled() || !m_ResidentRemote->cpuOnly) {
        emit RemotePackageFailed(QStringLiteral("CPU preloading requires the CPU-only memory cache mode"));
        return false;
    }
    return StartResidentRemoteRequest(address, port, packageId, directory, true);
}

void igQtFileLoader::SetRemoteCpuOnlyCacheEnabled(bool enabled)
{
    if (IsRemotePackageRunning()) return;
    if (m_ResidentRemote->cpuOnly != enabled) {
        ClearResidentRemoteCache();
        m_ResidentRemote->cpuOnly = enabled;
    }
    emit RemoteMemoryCacheChanged();
}

bool igQtFileLoader::HasRemoteMemoryCache() const { return m_ResidentRemote && m_ResidentRemote->cache.HasEntry(); }
quint64 igQtFileLoader::RemoteMemoryCacheBytes() const { return HasRemoteMemoryCache() ? m_ResidentRemote->cache.MemoryBytes() : 0; }
quint64 igQtFileLoader::RemoteMemoryCacheLimitBytes() const { return m_ResidentRemote->limitBytes; }

void igQtFileLoader::SetRemoteMemoryCacheLimitBytes(quint64 bytes)
{
    if (IsRemotePackageRunning() || bytes < 1024ull * 1024 * 1024) return;
    m_ResidentRemote->limitBytes = bytes;
    if (RemoteMemoryCacheBytes() > bytes) InvalidateRemoteMemoryCache(QStringLiteral("CPU cache admission limit reduced"));
    emit RemoteMemoryCacheChanged();
}

QString igQtFileLoader::RemoteMemoryCacheStatus() const
{
    const auto& s = *m_ResidentRemote;
    if (s.active && s.preloadOnly) return QStringLiteral("Preloading to CPU (no new scene model)");
    if (!s.cache.HasEntry()) return QStringLiteral("No model cached in memory");
    auto data = s.cache.PeekData();
    iGame::Scene::Pointer scene = m_SceneManager->GetCurrentScene();
    const bool mounted = static_cast<bool>(MountedRemoteData(scene.get(), data.get()));
    return QStringLiteral("%1 | %2 | %3 | %4 (CPU estimate; excludes GPU/process overhead)")
            .arg(QFileInfo(s.cache.DatasetPath()).fileName(),
                 mounted ? QStringLiteral("displayed") : QStringLiteral("not in model tree"),
                 HasRemoteGpu(data.get()) ? QStringLiteral("CPU + GPU resident") : QStringLiteral("CPU ready; GPU resources = 0"),
                 s.cache.HasPreparedCpuData() ? QStringLiteral("surface + LOD + draw arrays retained")
                                             : QStringLiteral("parsed data; display preparation required"));
}

void igQtFileLoader::ReleaseDetachedRemoteGpuResources()
{
    if (!m_ResidentRemote || !m_ResidentRemote->cpuOnly || IsRemotePackageRunning()) return;
    ReleaseRetiredRemoteResources();
    auto& s = *m_ResidentRemote;
    auto data = s.cache.PeekData();
    if (!data || !s.cacheContext) return;
    iGame::Scene::Pointer scene = m_SceneManager->GetCurrentScene();
    if (MountedRemoteData(scene.get(), data.get())) return;
    if (!s.renderer || s.renderer->context() != s.cacheContext.data()) {
        InvalidateRemoteMemoryCache(QStringLiteral("Cached render context changed"));
        return;
    }
    RemoteCacheGLScope gl(s.renderer);
    if (QOpenGLContext::currentContext() != s.cacheContext.data()) return;
    const auto key = s.cache.Key();
    const auto path = s.cache.DatasetPath();
    // The cache entry has already been validated when the displayed frame was
    // captured.  Removing the Scene::Model can legitimately change display
    // bookkeeping/MTimes without changing the retained CPU draw arrays.  Do
    // not re-run the strict identity lookup here: doing so used to downgrade
    // a prepared entry to raw data during normal model-tree deletion.
    const bool prepared = s.cache.HasPreparedCpuData();
    if (auto* draw = dynamic_cast<iGame::DrawObject*>(data.get())) {
        if (prepared) {
            draw->ReleaseGpuResourcesKeepCpuData();
            QString refreshReason;
            if (!s.cache.RefreshPreparedCpuData(scene, key, refreshReason)) {
                igDebug("[RemoteCpuCache] Prepared CPU refresh failed; falling back to raw CPU cache: {}",
                        refreshReason.toStdString());
                draw->ReleaseDrawableResources();
                s.cache.CaptureCpu(scene, data, key, path);
            }
        } else {
            draw->ReleaseDrawableResources();
            s.cache.CaptureCpu(scene, data, key, path);
        }
    }
    s.cacheContext.clear();
    if (!s.cache.HasEntry() || s.cache.MemoryBytes() > s.limitBytes) {
        // A detached prepared graph may contain shell/meshlet/self references.
        // Full release is still required on rejection/eviction.
        if (auto* draw = dynamic_cast<iGame::DrawObject*>(data.get())) draw->ReleaseDrawableResources();
        s.cache.Clear();
    }
    igDebug("[RemoteCpuCache] Detached model demoted to CPU; gpu_resources={} scene_user_models={} cpu_bytes={}",
            HasRemoteGpu(data.get()) ? 1 : 0, RemoteUserModelCount(scene.get()), s.cache.MemoryBytes());
    igDebug("[RemoteCpuCache] prepared_cpu={} (retained surface/LOD/draw arrays)", s.cache.HasPreparedCpuData());
    emit RemoteMemoryCacheChanged();
}

void igQtFileLoader::ReleaseRetiredRemoteResources(bool includeDisplayed)
{
    if (!m_ResidentRemote) return;
    auto& s = *m_ResidentRemote;
    iGame::Scene::Pointer scene = m_SceneManager->GetCurrentScene();
    for (auto it = s.retiredDisplays.begin(); it != s.retiredDisplays.end();) {
        if (!includeDisplayed && MountedRemoteData(scene.get(), it->data.get())) { ++it; continue; }
        RemoteCacheGLScope gl(s.renderer && s.renderer->context() == it->context.data() ? s.renderer.data() : nullptr);
        if (HasRemoteGpu(it->data.get()) &&
            (!it->context || QOpenGLContext::currentContext() != it->context.data())) {
            if (!includeDisplayed) { ++it; continue; }
            // Same exceptional context-loss policy as the active cache: never
            // delete old GL handles in a different/no context.
            auto* quarantined = new iGame::DataObject::Pointer(it->data);
            Q_UNUSED(quarantined);
            s.cacheContextFailed = true;
            qCritical() << "[RemoteCpuCache] Retired display context lost; restart required";
        } else if (auto* draw = dynamic_cast<iGame::DrawObject*>(it->data.get())) {
            draw->ReleaseDrawableResources();
            igDebug("[RemoteCpuCache] Released retired display resources; gpu_resources={}", draw->HasGpuResources() ? 1 : 0);
        }
        it = s.retiredDisplays.erase(it);
    }
}

void igQtFileLoader::SetRemoteMemoryCacheEnabled(bool enabled)
{
    if (IsRemotePackageRunning()) { return; }
    m_ResidentRemote->enabled = enabled;
    if (!enabled) { ClearResidentRemoteCache(); }
    qInfo() << "[RemoteMemoryCache] independent static-data cache enabled:" << enabled
            << "(one dataset; CPU-only retention by default; editing/restart invalidates)";
    emit RemoteMemoryCacheChanged();
}

void igQtFileLoader::SetRemoteCacheStrictValidation(bool strict)
{
    m_ResidentRemote->strictValidation = strict;
}

void igQtFileLoader::SetRemoteCacheRenderWidget(igQtRenderWidget* widget)
{
    if (m_ResidentRemote->renderer == widget) { return; }
    if (!ClearResidentRemoteCache()) { return; }
    m_ResidentRemote->renderer = widget;
    if (widget) {
        connect(widget, &igQtRenderWidget::ContextAboutToBeReleased, this, [this]() {
            // This direct callback runs while the OLD GL context is current.
            CancelRemotePackage();
            InvalidateRemoteMemoryCache(QStringLiteral("Render context is being released"));
            ReleaseRetiredRemoteResources(true);
        }, Qt::DirectConnection);
    }
}

bool igQtFileLoader::ClearResidentRemoteCache()
{
    if (!m_ResidentRemote) { return true; }
    auto& s = *m_ResidentRemote;
    if (s.clearingCache) { return false; }
    if (!s.cache.HasEntry()) { s.cacheContext.clear(); return true; }
    QScopedValueRollback<bool> clearing(s.clearingCache, true);
    Q_ASSERT(QThread::currentThread() == thread());
    if (!s.cacheContext && !HasRemoteGpu(s.cache.PeekData().get())) {
        auto data = s.cache.PeekData();
        iGame::Scene::Pointer scene = m_SceneManager->GetCurrentScene();
        if (!MountedRemoteData(scene.get(), data.get())) {
            if (auto* draw = dynamic_cast<iGame::DrawObject*>(data.get())) draw->ReleaseDrawableResources();
        }
        s.cache.Clear();
        emit RemoteMemoryCacheChanged();
        return true;
    }
    auto* context = s.cacheContext.data();
    RemoteCacheGLScope gl(s.renderer && s.renderer->context() == context
                                 ? s.renderer.data() : nullptr);
    // Captures require a valid renderer context; its pre-destruction callback
    // above and MainWindow's early teardown release the last owner before GL dies.
    if (!context || QOpenGLContext::currentContext() != context) {
        // Abnormal context loss: never delete old GL names in a new context,
        // including during FileLoader destruction. Retain ONE emergency owner
        // until process exit (OS reclamation), and reject further cached opens.
        // This intentionally sacrifices reclamation, not GL correctness. Normal
        // widget/context teardown releases synchronously before reaching here.
        auto* quarantined = new igQtResidentModelCache(s.cache);
        Q_UNUSED(quarantined);
        s.cacheContextFailed = true;
        s.cache.Clear(); // The quarantined owner prevents resource destruction.
        s.cacheContext.clear();
        qCritical() << "[RemoteMemoryCache] GL context lost: cached resources retained until process exit; restart the client before another cached open";
        return false;
    }
    auto data = s.cache.PeekData();
    iGame::Scene::Pointer scene = m_SceneManager->GetCurrentScene();
    if (!MountedRemoteData(scene.get(), data.get())) {
        if (auto* draw = dynamic_cast<iGame::DrawObject*>(data.get())) draw->ReleaseDrawableResources();
    } else if (s.cpuOnly) {
        // Clearing the cache must not destroy visible geometry. Keep only a
        // cleanup obligation (not a version-addressable cache entry) until the
        // scene model is removed or the originating context is torn down.
        bool tracked = false;
        for (const auto& retired : s.retiredDisplays) tracked |= retired.data.get() == data.get();
        if (!tracked) s.retiredDisplays.push_back({data, s.cacheContext});
    }
    s.cache.Clear();
    s.cacheContext.clear();
    emit RemoteMemoryCacheChanged();
    return true;
}

bool igQtFileLoader::ResidentRemoteEnabled() const
{
    return m_ResidentRemote && m_ResidentRemote->enabled;
}

bool igQtFileLoader::ResidentRemoteActive() const
{
    return m_ResidentRemote && (m_ResidentRemote->active || m_ResidentRemote->probe->IsRunning());
}

bool igQtFileLoader::ResidentRemoteAcceptsDataset() const
{
    return m_ResidentRemote && m_ResidentRemote->active &&
           !m_ResidentRemote->querying && !m_ResidentRemote->cancelled;
}

void igQtFileLoader::InvalidateRemoteMemoryCache(const QString& reason)
{
    if (!m_ResidentRemote) { return; }
    auto& s = *m_ResidentRemote;
    ++s.editEpoch;
    if (s.cache.HasEntry()) {
        igDebug("[RemoteMemoryCache] Invalidated: {}", reason.toStdString());
    }
    ClearResidentRemoteCache();
}

bool igQtFileLoader::StartResidentRemoteRequest(const QString& address, quint16 port,
                                               const QString& packageId, const QString& directory,
                                               bool preloadOnly)
{
    if (IsRemotePackageRunning() || address.trimmed().isEmpty() || port == 0 ||
        packageId.trimmed().isEmpty() || directory.trimmed().isEmpty()) { return false; }
    auto& s = *m_ResidentRemote;
    ReleaseDetachedRemoteGpuResources();
    if (s.cacheContextFailed) {
        emit RemotePackageFailed(QStringLiteral("The render context was lost; restart the client before opening another cached dataset"));
        return false;
    }
    s.address = address; s.port = port; s.packageId = packageId; s.cacheDirectory = directory;
    s.preloadOnly = preloadOnly;
    s.identity.clear(); s.datasetPath.clear(); s.probeFailure.clear();
    s.cancelled = false; s.validInfo = false; s.memoryHit = false; s.awaitingFrame = false;
    s.preparedModel = nullptr; s.preparedObject = nullptr; s.preparedScene = nullptr;
    s.startEditEpoch = s.editEpoch;
    ++s.requestId;
    s.timer.start();
    s.active = true; s.querying = true;
    if (!s.probe->SetServerEndpoint(address, port) || !s.probe->QueryPackageInfo(packageId)) {
        s.active = false; s.querying = false;
        return false;
    }
    igDebug("[RemoteOpen] begin request={} endpoint={}:{} package={} cache_mode={} preload_only={}",
            s.requestId, address.toStdString(), port, packageId.toStdString(),
            s.cpuOnly ? "cpu_only_retention" : "independent_static_data", preloadOnly);
    emit RemoteOpenStarted(s.requestId, s.address, s.port, s.packageId);
    if (s.active && !s.cancelled) { emit RemotePackageRunningChanged(true); }
    emit RemoteMemoryCacheChanged();
    return true;
}

void igQtFileLoader::ContinueResidentRemoteRequest()
{
    auto& s = *m_ResidentRemote;
    RemoteCacheGLScope gl(s.renderer);
    s.querying = false;
    if (!s.active || s.cancelled) { return; }
    if (!s.probeFailure.isEmpty() || !s.validInfo) {
        const QString error = s.probeFailure.isEmpty()
                ? QStringLiteral("INFO query did not return validated metadata") : s.probeFailure;
        FailResidentRemoteRequest(error);
        return;
    }
    iGame::Scene::Pointer scene = m_SceneManager->GetCurrentScene();
    QString reason;
    auto data = s.cache.LookupData(scene, s.identity, reason);
    const bool sameContext = s.renderer && s.cacheContext &&
            s.renderer->context() == s.cacheContext.data();
    const bool cpuReady = data && s.cpuOnly && !s.cacheContext && !HasRemoteGpu(data.get());
    if (data && (sameContext || cpuReady) && s.startEditEpoch == s.editEpoch) {
        s.memoryHit = true;
        s.datasetPath = s.cache.DatasetPath();
        auto model = s.cache.Lookup(scene, s.identity, reason);
        if (s.preloadOnly) {
            if (model || HasRemoteGpu(data.get())) {
                FailResidentRemoteRequest(QStringLiteral("This model is already displayed; remove it from the model tree before CPU-only preloading"));
            } else { FinishResidentRemotePreload(); }
            return;
        }
        const bool reattached = !model;
        if (!model) {
            if (cpuReady) {
                if (!s.renderer || !s.renderer->context() ||
                    QOpenGLContext::currentContext() != s.renderer->context()) {
                    FailResidentRemoteRequest(QStringLiteral("No valid GL context for uploading the CPU-cached model"));
                    return;
                }
                // Bind the resource generation BEFORE any display callback can
                // allocate GL names, so cancel/failure teardown stays safe too.
                s.cacheContext = s.renderer->context();
                if (s.cache.HasPreparedCpuData()) {
                    QElapsedTimer uploadTimer;
                    uploadTimer.start();
                    auto* draw = dynamic_cast<iGame::DrawObject*>(data.get());
                    if (!draw || !draw->UploadPreparedCpuData()) {
                        FailResidentRemoteRequest(QStringLiteral("Prepared CPU buffer upload failed; model was not attached"));
                        return;
                    }
                    igDebug("[RemoteCpuCache] Prepared GPU upload submitted elapsed_ms={} (no CPU geometry conversion; GPU completion not timed)", uploadTimer.elapsed());
                    // Reattach the original objects without the normal initial
                    // scalar setup, which invalidates shell/LOD and color data.
                    emit RemoteCachedDatasetReattach(data);
                } else {
                    emit NewModel(data, ItemSource::File);
                    emit FinishReading();
                }
            } else { emit RemoteCachedDatasetReattach(data); }
            model = MountedRemoteData(scene.get(), data.get());
        }
        if (!model) {
            FailResidentRemoteRequest(QStringLiteral("Cached data could not be attached to the scene/tree"));
            return;
        }
        scene->SetCurrentModel(model);
        scene->ChangeModelVisibility(model, true);
        scene->Update();
        emit RemoteCachedModelSelected(model->GetDataObject());
        igDebug("[RemoteMemoryCache] Attachment request={} reattached={} scene_user_models={} scene_entries={}",
                s.requestId, reattached, RemoteUserModelCount(scene.get()), scene->GetModelList()->GetObjectCount());
        igDebug("[RemoteMemoryCache] HIT request={} elapsed_ms={} model_id={} gpu_reused={}; no archive I/O or parsing",
                s.requestId, s.timer.elapsed(), scene->GetCurrentModelID(), !cpuReady);
        igDebug("[RemoteCpuCache] reuse_prepared_cpu={} gpu_upload_required={}", s.cache.HasPreparedCpuData(), cpuReady);
        emit RemotePackageStatusChanged(QStringLiteral("Using independent memory cache (no model transfer or parsing); requesting normal view refresh"));
        PrepareResidentRemoteFrame(s.datasetPath);
        return;
    }
    if (data && !sameContext) { reason = QStringLiteral("render-context-changed"); }
    igDebug("[RemoteMemoryCache] MISS request={} reason={}", s.requestId, reason.toStdString());
    data = nullptr; // Release local ownership before context-scoped eviction.
    if (!ClearResidentRemoteCache()) {
        FailResidentRemoteRequest(QStringLiteral("Cannot safely evict the previous GL cache"));
        return;
    }
    if (!m_RemotePackageLoader->Start(s.address, s.port, s.packageId, s.cacheDirectory)) {
        const QString error = QStringLiteral("Cannot start normal package loading after the INFO query");
        FailResidentRemoteRequest(error);
    }
}

bool igQtFileLoader::ResidentRemotePreloadOnly() const
{
    return ResidentRemoteAcceptsDataset() && m_ResidentRemote->preloadOnly;
}

bool igQtFileLoader::ReadResidentRemotePreload(const QString& datasetPath)
{
    auto& s = *m_ResidentRemote;
    if (!ResidentRemotePreloadOnly()) return false;
    iGame::Scene::Pointer scene = m_SceneManager->GetCurrentScene();
    const auto userModelsBefore = RemoteUserModelCount(scene.get());
    emit RemotePackageStatusChanged(QStringLiteral("Reading and parsing mesh into CPU memory; not adding a scene model"));
    // Existing readers/progress observers are GUI-thread-only. Deliberately
    // serialize with normal file reading; no unsafe worker-thread GUI callbacks.
    iGame::DataObject::Pointer data;
    try { data = iGame::FileIO::ReadRemoteFile(datasetPath.toUtf8().toStdString()); }
    catch (const std::exception& e) {
        igError("[RemoteCpuCache] CPU reader exception: {}", e.what());
        return false;
    }
    if (!data) return false;
    auto* draw = dynamic_cast<iGame::DrawObject*>(data.get());
    if (!draw) return false;
    // VTM assembly eagerly prepares some derived CPU draw arrays. Drop those,
    // along with any self-owning renderable helpers; keep the original mesh.
    if (HasRemoteGpu(data.get())) {
        RemoteCacheGLScope gl(s.renderer);
        if (!s.renderer || !s.renderer->context() ||
            QOpenGLContext::currentContext() != s.renderer->context()) {
            auto* quarantined = new iGame::DataObject::Pointer(data);
            Q_UNUSED(quarantined);
            s.cacheContextFailed = true;
            qCritical() << "[RemoteCpuCache] Unexpected preload GPU resources without their context; restart required";
            return false;
        }
        draw->ReleaseDrawableResources();
        return false; // A preloader unexpectedly touching GPU is not acceptable.
    }
    draw->ReleaseDrawableResources();
    QString cpuGuardReason;
    if (!igQtResidentModelCache::ValidateCpuSurface(data.get(), cpuGuardReason)) {
        igError("[RemoteCpuCache] CPU preload guard rejected data: {}", cpuGuardReason.toStdString());
        return false;
    }
    data->SetName(QFileInfo(datasetPath).completeBaseName().toUtf8().toStdString());
    data->GetProperties()->AddProperty(iGame::Variant::String, "FilePath")
            ->SetValue(datasetPath.toUtf8().toStdString());
    const auto bytes = static_cast<quint64>(data->GetRealMemorySize());
    if (bytes > s.limitBytes || !bytes) {
        igError("[RemoteCpuCache] Admission refused: CPU array estimate={} limit={}; disk cache is preserved", bytes, s.limitBytes);
        return false;
    }
    if (s.cancelled || !s.active || s.startEditEpoch != s.editEpoch ||
        RemoteUserModelCount(scene.get()) != userModelsBefore) return false;
    s.cache.CaptureCpu(scene, data, s.identity, datasetPath);
    s.cacheContext.clear();
    s.datasetPath = datasetPath;
    return s.cache.HasEntry();
}

void igQtFileLoader::FinishResidentRemotePreload()
{
    auto& s = *m_ResidentRemote;
    if (!s.active || !s.preloadOnly || s.cancelled) return;
    auto data = s.cache.PeekData();
    iGame::Scene::Pointer scene = m_SceneManager->GetCurrentScene();
    if (!data || HasRemoteGpu(data.get()) || MountedRemoteData(scene.get(), data.get())) {
        FailResidentRemoteRequest(QStringLiteral("CPU preload did not produce detached, GPU-free mesh data"));
        return;
    }
    const auto elapsed = s.timer.elapsed();
    const auto bytes = s.cache.MemoryBytes();
    const auto requestId = s.requestId;
    const auto memoryHit = s.memoryHit;
    const auto datasetPath = s.datasetPath;
    const QString detail = QStringLiteral("CPU preload ready; gpu_resources=0; model_in_scene=0; scene_user_models=%1; cpu_array_bytes=%2; no rendering performed")
            .arg(RemoteUserModelCount(scene.get())).arg(bytes);
    s.active = false;
    igDebug("[RemoteCpuCache] PRELOADED request={} elapsed_ms={} memory_hit={} {}",
            requestId, elapsed, memoryHit, detail.toStdString());
    emit RemotePackageStatusChanged(detail);
    emit RemoteMemoryCacheChanged();
    emit RemotePackagePreloaded(datasetPath);
    emit RemotePackageProgressChanged(1.0);
    emit RemotePackageRunningChanged(false);
    emit RemotePackageFinished();
    emit RemotePreloadMeasured(requestId, memoryHit, elapsed, bytes, detail);
}

void igQtFileLoader::PrepareResidentRemoteFrame(const QString& datasetPath)
{
    auto& s = *m_ResidentRemote;
    if (!s.enabled || !s.active || s.cancelled) { return; }
    iGame::Scene::Pointer scene = m_SceneManager->GetCurrentScene();
    auto model = scene ? scene->GetCurrentModel() : nullptr;
    if (!model || !model->GetVisibility() || !model->GetDataObject()) {
        FailResidentRemoteRequest(QStringLiteral("No visible prepared model for remote view refresh"));
        return;
    }
    const auto surface = igQtRemoteSurfaceSnapshot::Inspect(scene.get(), model.get());
    s.cacheable = surface.valid;
    if (!surface.valid && s.strictValidation) { FailResidentRemoteRequest(surface.detail); return; }
    if (!surface.valid) {
        // Automatic caching must not reject otherwise supported remote models.
        // This first implementation caches only the validated static Surface path.
        igDebug("[RemoteMemoryCache] Not cacheable; normal remote display retained: {}", surface.detail.toStdString());
    }
    s.preparedLeaves = surface.leafCount;
    s.preparedPoints = surface.pointCount;
    s.preparedFaces = surface.faceCount;
    igDebug("[RemoteOpen] {}", surface.detail.toStdString());
    s.datasetPath = datasetPath;
    s.preparedScene = scene.get(); s.preparedModel = model.get();
    s.preparedObject = model->GetDataObject().get();
    s.preparedModelId = scene->GetCurrentModelID();
    s.awaitingFrame = true;
    igDebug("[RemoteOpen] CPU-ready request={} elapsed_ms={} memory_hit={}; awaiting normal Qt view refresh (not full-resolution verification)",
            s.requestId, s.timer.elapsed(), s.memoryHit);
    emit RemoteRenderRequested(s.requestId);
    const quint64 id = s.requestId;
    QTimer::singleShot(180000, this, [this, id]() {
        auto& state = *m_ResidentRemote;
        if (state.active && state.awaitingFrame && state.requestId == id) {
            FailResidentRemoteRequest(QStringLiteral("Timed out waiting for the normal Qt view refresh"));
        }
    });
}

void igQtFileLoader::NotifyRemoteFrameCompleted(quint64 requestId, bool success, const QString& detail)
{
    auto& s = *m_ResidentRemote;
    if (!s.active || !s.awaitingFrame || s.requestId != requestId || s.cancelled) { return; }
    iGame::Scene::Pointer scene = m_SceneManager->GetCurrentScene();
    auto model = scene && scene.get() == s.preparedScene
            ? scene->GetModelById(static_cast<int>(s.preparedModelId)) : nullptr;
    success = success && model && model.get() == s.preparedModel && model->GetVisibility() &&
              model->GetDataObject().get() == s.preparedObject && s.editEpoch == s.startEditEpoch;
    if (s.strictValidation) { success = success && RemoteUserModelCount(scene.get()) == 1; }
    const auto surface = igQtRemoteSurfaceSnapshot::Inspect(scene.get(), model.get());
    if (s.cacheable || s.strictValidation) {
        success = success && surface.valid && surface.leafCount == s.preparedLeaves &&
                  surface.pointCount == s.preparedPoints && surface.faceCount == s.preparedFaces;
    }
    const qint64 elapsed = s.timer.elapsed();
    s.awaitingFrame = false; s.active = false;
    if (success && s.cacheable && !s.identity.isEmpty() && s.renderer &&
            s.renderer->context() && s.renderer->isValid()) {
        // Clear while GL is current if this replaces a detached cached object.
        if (s.cache.PeekData().get() == model->GetDataObject().get() || ClearResidentRemoteCache()) {
            s.cache.CapturePrepared(scene, model, s.identity, s.datasetPath);
            if (s.cache.HasEntry()) s.cacheContext = s.renderer->context();
        } else { success = false; }
        if (success && s.cache.HasEntry() && s.cache.MemoryBytes() > s.limitBytes) {
            igDebug("[RemoteCpuCache] Visible model exceeds cache admission limit: estimated_cpu_bytes={} limit_bytes={}; keeping display without caching",
                    s.cache.MemoryBytes(), s.limitBytes);
            ClearResidentRemoteCache();
        }
        if (success && s.cache.HasEntry()) {
            s.cacheContext = s.renderer->context();
            igDebug("[RemoteMemoryCache] STORED request={} independent_owner=true leaves={} point_records={} face_cells={}",
                    requestId, s.preparedLeaves, s.preparedPoints, s.preparedFaces);
            igDebug("[RemoteCpuCache] prepared_cpu={} estimated_cpu_bytes={} limit_bytes={}",
                    s.cache.HasPreparedCpuData(), s.cache.MemoryBytes(), s.limitBytes);
        }
    } else { ClearResidentRemoteCache(); }
    emit RemoteMemoryCacheChanged();
    const QString resultDetail = (success ? detail :
            QStringLiteral("View refresh failed, model changed or was hidden: ") + detail) +
            QStringLiteral("; ") + surface.detail;
    const bool memoryHit = s.memoryHit;
    const QString datasetPath = s.datasetPath;
    igDebug("[RemoteOpen] complete request={} memory_hit={} view_refreshed={} elapsed_ms={} full_resolution_verified=false detail={}",
            requestId, memoryHit, success, elapsed,
            resultDetail.toStdString());
    if (success) {
        emit RemotePackageStatusChanged(QStringLiteral("Remote model loaded; normal view refreshed: ") + datasetPath);
        emit RemotePackageDatasetOpened(datasetPath);
    } else { emit RemotePackageFailed(resultDetail); }
    emit RemotePackageProgressChanged(1.0);
    emit RemotePackageRunningChanged(false);
    emit RemotePackageFinished();
    emit RemoteOpenMeasured(requestId, memoryHit, elapsed, success, resultDetail);
}

void igQtFileLoader::FailResidentRemoteRequest(const QString& reason)
{
    if (!m_ResidentRemote || !m_ResidentRemote->active) { return; }
    auto& s = *m_ResidentRemote;
    const bool wasAwaiting = s.awaitingFrame;
    const quint64 requestId = s.requestId;
    const bool memoryHit = s.memoryHit;
    const bool preloadOnly = s.preloadOnly;
    const qint64 elapsed = s.timer.elapsed();
    s.active = false; s.awaitingFrame = false;
    ClearResidentRemoteCache();
    if (wasAwaiting) { emit RemoteRenderCancelled(requestId); }
    igDebug("[RemoteOpen] failed request={} elapsed_ms={} detail={}",
            requestId, elapsed, reason.toStdString());
    emit RemotePackageFailed(reason);
    emit RemotePackageRunningChanged(false);
    emit RemotePackageFinished();
    emit RemoteMemoryCacheChanged();
    if (!preloadOnly) emit RemoteOpenMeasured(requestId, memoryHit, elapsed, false, reason);
}

void igQtFileLoader::CancelResidentRemoteRequest()
{
    if (!m_ResidentRemote || !m_ResidentRemote->active) { return; }
    auto& s = *m_ResidentRemote;
    s.cancelled = true;
    s.probe->Cancel();
    FailResidentRemoteRequest(QStringLiteral("Remote open cancelled"));
}

bool igQtFileLoader::DetachResidentRemoteModelForBenchmark()
{
    if (IsRemotePackageRunning() || !ResidentRemoteEnabled()) { return false; }
    auto& s = *m_ResidentRemote;
    iGame::Scene::Pointer scene = m_SceneManager->GetCurrentScene();
    QString reason;
    auto model = s.cache.Lookup(scene, s.identity, reason);
    if (!model) { return false; }
    auto data = model->GetDataObject();
    bool treeDetached = false;
    RemoteCacheGLScope gl(s.renderer);
    emit RemoteCachedDatasetDetach(data, &treeDetached);
    // No borrowed Model may keep geometry alive and disguise a missing cache.
    model = nullptr;
    ReleaseDetachedRemoteGpuResources();
    auto cached = s.cache.LookupData(scene, s.identity, reason);
    if (!treeDetached || !cached || cached.get() != data.get() || s.cache.Lookup(scene, s.identity, reason)) {
        return false;
    }
    if (s.strictValidation && RemoteUserModelCount(scene.get()) != 0) { return false; }
    scene->Update();
    igDebug("[RemoteMemoryCache] Detached cached remote model from scene for benchmark; scene_user_models={} scene_entries={} independent_owner=true",
            RemoteUserModelCount(scene.get()), scene->GetModelList()->GetObjectCount());
    return true;
}
