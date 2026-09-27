// Load the GL-aware widget before MainWindow's legacy QT_NO_OPENGL includes.
#include <IQWidgets/igQtModelDrawWidget.h>
#include <IQCore/igQtMainWindow.h>
#include <IQCore/igQtFileLoader.h>
#include <IQComponents/igQtModelDialogWidget.h>
#include <iGameDrawObject.h>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QDebug>
#include <QTimer>
#include <QCoreApplication>
#include <QOpenGLContext>
#include <QPointer>
#include <QRegularExpression>
#include <limits>
#include <memory>

namespace {
quint64 BenchmarkUserModelCount(iGame::Scene* scene)
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

// Normal tree deletion can release model-owned GL objects. Match the loader's
// nested-safe scope rather than depending on an earlier frame's current context.
class BenchmarkGLScope {
public:
    explicit BenchmarkGLScope(igQtRenderWidget* renderer)
        : previous(QOpenGLContext::currentContext()),
          surface(previous ? previous->surface() : nullptr)
    {
        if (renderer && renderer->context() && renderer->context() != previous.data()) {
            renderer->makeCurrent();
            switched = QOpenGLContext::currentContext() != previous.data();
        }
    }
    ~BenchmarkGLScope()
    {
        if (!switched) { return; }
        if (previous && surface) { previous->makeCurrent(surface); }
        else if (auto* current = QOpenGLContext::currentContext()) { current->doneCurrent(); }
    }
private:
    QPointer<QOpenGLContext> previous;
    QSurface* surface{nullptr};
    bool switched{false};
};

struct RemoteCacheBenchmarkState {
    QJsonArray results;
    QJsonArray removalControls;
    QJsonObject preload;
    QJsonObject clearControl;
    bool clearAfterBenchmark{false};
    bool preloadFirst{false};
    bool preloadOnly{false};
    bool preloading{false};
    quint64 preloadSceneModelsBefore{0};
    int removalControlCount{0};
    int blankControlFrameCount{0};
    QString failureReason;
    bool failed{false};
    bool finished{false};
    bool issuingOwnRequest{false};
    bool awaitingMeasurement{false};
    quint64 expectedRequestId{0};
    bool blankPending{false};
    quint64 blankRequestId{0};
    QMetaObject::Connection startedConnection;
    QMetaObject::Connection measuredConnection;
    QMetaObject::Connection blankConnection;
    QMetaObject::Connection preloadConnection;
    QMetaObject::Connection preloadFailureConnection;
};
}

void igQtMainWindow::ConfigureRemoteCacheBenchmark(const QString& package, const QString& host,
                                                  quint16 port, const QString& directory,
                                                  int rounds, const QString& outputPath)
{
    auto state = std::make_shared<RemoteCacheBenchmarkState>();
    state->preloadOnly = QCoreApplication::arguments().contains(QStringLiteral("--remote-cpu-preload-only"));
    state->preloadFirst = state->preloadOnly ||
            QCoreApplication::arguments().contains(QStringLiteral("--remote-cpu-preload-first"));
    state->clearAfterBenchmark = QCoreApplication::arguments().contains(
            QStringLiteral("--remote-cpu-clear-after-benchmark"));
    state->preload.insert(QStringLiteral("requested"), state->preloadFirst);
    state->preload.insert(QStringLiteral("complete"), false);
    state->clearControl.insert(QStringLiteral("requested"), state->clearAfterBenchmark);
    state->clearControl.insert(QStringLiteral("complete"), false);
    auto writeResult = [=]() {
        if (outputPath.isEmpty()) { return; }
        QJsonObject report;
        report.insert(QStringLiteral("package"), package);
        report.insert(QStringLiteral("host"), host);
        report.insert(QStringLiteral("port"), port);
        report.insert(QStringLiteral("requestedRounds"), state->preloadOnly ? 0 : rounds);
        report.insert(QStringLiteral("configuredRounds"), rounds);
        report.insert(QStringLiteral("rounds"), state->results);
        report.insert(QStringLiteral("cacheBehavior"), state->preloadFirst
                ? QStringLiteral("independent-cpu-only-data-cache")
                : QStringLiteral("independent-data-object-cache"));
        report.insert(QStringLiteral("cpuPreloadFirst"), state->preloadFirst);
        report.insert(QStringLiteral("cpuPreloadOnly"), state->preloadOnly);
        report.insert(QStringLiteral("preload"), state->preload);
        report.insert(QStringLiteral("clearControl"), state->clearControl);
        report.insert(QStringLiteral("removalControlCount"), state->removalControlCount);
        report.insert(QStringLiteral("blankControlFrameCount"), state->blankControlFrameCount);
        report.insert(QStringLiteral("removalControls"), state->removalControls);
        report.insert(QStringLiteral("complete"), state->finished);
        report.insert(QStringLiteral("failed"), state->failed);
        report.insert(QStringLiteral("failureReason"), state->failureReason);
        report.insert(QStringLiteral("fullResolutionValidationAvailable"), false);
        report.insert(QStringLiteral("cachePolicy"), state->preloadOnly
                ? QStringLiteral("CPU-only preload; no Open rounds, no scene attachment and no GPU-resource allocation by this preload; "
                                 "the cache remains available in this process unless the client exits or the cache is cleared")
                : state->preloadFirst
                ? QStringLiteral("CPU-only preload is measured separately and does not mount a scene model or upload GPU resources; "
                                 "full-resolution Open benchmark is unavailable after reverting Scene instrumentation")
                : QStringLiteral("same-process independent data-object cache; displayed model removed from scene and tree between rounds; "
                                 "full-resolution Open benchmark is unavailable after reverting Scene instrumentation"));
        report.insert(QStringLiteral("timing"), state->preloadOnly
                ? QStringLiteral("Only PreloadRemotePackage is measured; no Open/display time is measured or claimed")
                : QStringLiteral("Unavailable: normal Qt refresh does not certify full-resolution rendering or GPU completion"));
        QSaveFile file(outputPath);
        const QByteArray json = QJsonDocument(report).toJson(QJsonDocument::Indented);
        if (!file.open(QIODevice::WriteOnly) || file.write(json) != json.size() || !file.commit()) {
            qWarning() << "[RemoteOpenBenchmark] Cannot write" << outputPath << file.errorString();
        }
    };

    // All terminal paths share cleanup, reporting and the optional exit policy,
    // including failure before the first request has emitted a measurement.
    auto finish = [=](bool failed, const QString& reason) {
        if (state->finished) { return; }
        QString finalReason = reason;
        if (!failed && state->clearAfterBenchmark &&
            !state->clearControl.value(QStringLiteral("complete")).toBool()) {
            failed = true;
            finalReason = QStringLiteral("Requested CPU-cache clear lifecycle control did not complete");
        }
        state->finished = true;
        state->failed = failed;
        state->failureReason = finalReason;
        state->awaitingMeasurement = false;
        disconnect(state->startedConnection);
        disconnect(state->measuredConnection);
        disconnect(state->blankConnection);
        disconnect(state->preloadConnection);
        disconnect(state->preloadFailureConnection);
        if (state->blankPending) {
            state->blankPending = false;
            rendererWidget->CancelCompletedFrame(state->blankRequestId);
        }
        writeResult();
        if (failed) { qWarning() << "[RemoteOpenBenchmark]" << finalReason; }
        if (QCoreApplication::arguments().contains(QStringLiteral("--remote-cache-exit-after-benchmark"))) {
            QTimer::singleShot(1500, qApp, &QCoreApplication::quit);
        }
    };

    // Keep CPU-only preload tests, but never pass the old 100-second rendering
    // acceptance using an ordinary refresh that may show a previous/LOD frame.
    if (!state->preloadOnly) {
        finish(true, QStringLiteral(
                "Full-resolution render benchmark is unavailable after reverting Scene instrumentation; "
                "normal C/S Open and CPU caching remain available"));
        return;
    }

    state->startedConnection = connect(fileLoader, &igQtFileLoader::RemoteOpenStarted, this,
            [=](quint64 requestId, const QString& address, quint16 startedPort,
                const QString& startedPackage) {
        if (state->finished) { return; }
        const bool sameTarget = address.trimmed().compare(host.trimmed(), Qt::CaseInsensitive) == 0 &&
                                startedPort == port && startedPackage == package;
        if (!state->issuingOwnRequest || state->awaitingMeasurement || !sameTarget || requestId == 0) {
            finish(true, QStringLiteral(
                    "Unexpected remote request %1 (%2:%3, package %4) interrupted the benchmark; "
                    "external requests are not accepted as benchmark rounds")
                    .arg(requestId).arg(address).arg(startedPort).arg(startedPackage));
            // Started is emitted after the probe has begun. Cancel the foreign
            // request only after finish() has disconnected measurement handling,
            // so its synchronous cancellation cannot contaminate our report.
            fileLoader->CancelRemotePackage();
            return;
        }
        state->expectedRequestId = requestId;
        state->awaitingMeasurement = true;
    });

    // RemoteOpenStarted is emitted synchronously by OpenRemotePackage on the
    // GUI thread. Only requests inside this call are owned by this benchmark.
    auto startRound = [=]() {
        if (state->finished) { return; }
        if (state->awaitingMeasurement || fileLoader->IsRemotePackageRunning()) {
            finish(true, QStringLiteral("Cannot start benchmark round: another remote request is active"));
            return;
        }
        state->expectedRequestId = 0;
        state->issuingOwnRequest = true;
        const bool started = fileLoader->OpenRemotePackage(host, port, package, directory);
        state->issuingOwnRequest = false;
        if (state->finished) { return; }
        if (!started) {
            finish(true, QStringLiteral("Cannot start C/S benchmark round %1")
                                  .arg(state->results.size() + 1));
        } else if (!state->awaitingMeasurement) {
            finish(true, QStringLiteral("C/S request started without a matching RemoteOpenStarted identity"));
        }
    };

    auto runClearControl = [=]() {
        if (state->finished) { return; }
        state->clearControl.insert(QStringLiteral("attempted"), true);
        auto failClear = [=](const QString& reason) {
            state->clearControl.insert(QStringLiteral("failureReason"), reason);
            finish(true, QStringLiteral("CPU-cache clear lifecycle control failed: ") + reason);
        };
        auto* scene = rendererWidget->GetScene();
        auto pool = scene ? scene->GetModelList() : nullptr;
        auto model = scene ? scene->GetCurrentModel() : nullptr;
        auto data = model ? model->GetDataObject() : nullptr;
        auto* draw = data ? dynamic_cast<iGame::DrawObject*>(data.get()) : nullptr;
        auto* item = modelTreeWidget && data ? modelTreeWidget->getItemFromObject(data) : nullptr;
        const quint64 usersBefore = BenchmarkUserModelCount(scene);
        const bool cachedBefore = fileLoader->HasRemoteMemoryCache();
        const quint64 bytesBefore = fileLoader->RemoteMemoryCacheBytes();
        state->clearControl.insert(QStringLiteral("sceneUserModelsBefore"), static_cast<double>(usersBefore));
        state->clearControl.insert(QStringLiteral("cachePresentBefore"), cachedBefore);
        state->clearControl.insert(QStringLiteral("cacheBytesBefore"), QString::number(bytesBefore));
        state->clearControl.insert(QStringLiteral("gpuResourcesBefore"), draw && draw->HasGpuResources());
        if (fileLoader->IsRemotePackageRunning() || !pool || !model || !draw || !item ||
            !model->GetVisibility() || usersBefore != 1 || !cachedBefore || bytesBefore == 0 ||
            !draw->HasGpuResources()) {
            failClear(QStringLiteral("Expected one visible, rendered, cached model with a matching model-tree root"));
            return;
        }
        const auto modelId = scene->GetCurrentModelID();
        state->clearControl.insert(QStringLiteral("displayModelId"), QString::number(modelId));
        BenchmarkGLScope gl(rendererWidget);
        if (!rendererWidget->context() || QOpenGLContext::currentContext() != rendererWidget->context()) {
            failClear(QStringLiteral("Cannot make the model's renderer context current for lifecycle cleanup"));
            return;
        }
        fileLoader->InvalidateRemoteMemoryCache(QStringLiteral("benchmark clear while displayed"));
        auto stillDisplayed = scene->GetModelById(static_cast<int>(modelId));
        const bool cacheCleared = !fileLoader->HasRemoteMemoryCache() && fileLoader->RemoteMemoryCacheBytes() == 0;
        const bool displayPreserved = stillDisplayed && stillDisplayed.get() == model.get() &&
                stillDisplayed->GetDataObject().get() == data.get() && stillDisplayed->GetVisibility() &&
                BenchmarkUserModelCount(scene) == 1 && modelTreeWidget->getItemFromObject(data) == item &&
                draw->HasGpuResources();
        state->clearControl.insert(QStringLiteral("cacheClearedWhileDisplayed"), cacheCleared);
        state->clearControl.insert(QStringLiteral("cacheBytesAfterClear"), QString::number(fileLoader->RemoteMemoryCacheBytes()));
        state->clearControl.insert(QStringLiteral("displayPreservedAfterClear"), displayPreserved);
        state->clearControl.insert(QStringLiteral("sceneUserModelsAfterClear"),
                                   static_cast<double>(BenchmarkUserModelCount(scene)));
        if (!cacheCleared || !displayPreserved) {
            failClear(QStringLiteral("Clearing the CPU cache did not leave exactly the original visible model intact"));
            return;
        }
        // After cache removal, do not use the cache-specific detach helper.
        // Release temporary Model owners before exercising normal tree deletion;
        // retain DataObject only long enough to inspect its resource state.
        model = nullptr;
        stillDisplayed = nullptr;
        modelTreeWidget->setCurrentItem(item);
        modelTreeWidget->deleteCurrentModel();
        fileLoader->ReleaseDetachedRemoteGpuResources();
        const bool treeRemoved = modelTreeWidget->getItemFromObject(data) == nullptr;
        const quint64 usersAfter = BenchmarkUserModelCount(scene);
        const quint64 entriesAfter = pool->GetObjectCount();
        bool onlyAxes = entriesAfter == 1 && scene->GetCenterAxesModel();
        for (auto it = pool->Begin(); it != pool->End(); ++it) {
            onlyAxes = onlyAxes && it->second &&
                    it->second->GetDataObject().get() == scene->GetCenterAxesModel().get();
        }
        const bool gpuReleased = !draw->HasGpuResources();
        const bool cacheStillEmpty = !fileLoader->HasRemoteMemoryCache() && fileLoader->RemoteMemoryCacheBytes() == 0;
        const bool complete = treeRemoved && usersAfter == 0 && onlyAxes && gpuReleased && cacheStillEmpty;
        state->clearControl.insert(QStringLiteral("treeItemRemoved"), treeRemoved);
        state->clearControl.insert(QStringLiteral("sceneUserModelsAfterDelete"), static_cast<double>(usersAfter));
        state->clearControl.insert(QStringLiteral("sceneEntriesAfterDelete"), static_cast<double>(entriesAfter));
        state->clearControl.insert(QStringLiteral("onlyAxesRemain"), onlyAxes);
        state->clearControl.insert(QStringLiteral("gpuResourcesAfterDelete"), !gpuReleased);
        state->clearControl.insert(QStringLiteral("cacheEmptyAfterDelete"), cacheStillEmpty);
        state->clearControl.insert(QStringLiteral("complete"), complete);
        state->clearControl.insert(QStringLiteral("detail"), QStringLiteral(
                "Clear while displayed, then normal model-tree deletion and ReleaseDetachedRemoteGpuResources; "
                "resource/ownership assertions only, not a pixel-level visual check"));
        qInfo() << "[RemoteOpenBenchmark] clear control cache_cleared" << cacheCleared
                << "display_preserved" << displayPreserved << "tree_removed" << treeRemoved
                << "scene_user_models" << usersAfter << "only_axes" << onlyAxes
                << "gpu_resources_zero" << gpuReleased << "complete" << complete;
        scene->Update();
        if (!complete) {
            failClear(QStringLiteral("Normal display deletion did not leave an empty cache, an axes-only scene and zero model GPU resources"));
            return;
        }
        finish(false, {});
    };

    state->measuredConnection = connect(fileLoader, &igQtFileLoader::RemoteOpenMeasured, this,
            [=](quint64 requestId, bool hit, qint64 elapsedMs, bool rendered, const QString& detail) {
        if (state->finished) { return; }
        if (state->preloading || !state->awaitingMeasurement || requestId != state->expectedRequestId) {
            finish(true, QStringLiteral("Unexpected measurement for request %1; expected owned request %2")
                                  .arg(requestId).arg(state->expectedRequestId));
            return;
        }
        state->awaitingMeasurement = false;
        const int round = state->results.size() + 1;
        QJsonObject result;
        result.insert(QStringLiteral("round"), round);
        result.insert(QStringLiteral("requestId"), static_cast<double>(requestId));
        result.insert(QStringLiteral("memoryHit"), hit);
        result.insert(QStringLiteral("elapsedMs"), static_cast<double>(elapsedMs));
        result.insert(QStringLiteral("rendered"), rendered);
        result.insert(QStringLiteral("within100Seconds"), rendered && elapsedMs < 100000);
        result.insert(QStringLiteral("detail"), detail);
        state->results.append(result);
        qInfo() << "[RemoteOpenBenchmark] round" << round << "/" << rounds
                << "memory_hit" << hit << "elapsed_ms" << elapsedMs << "rendered" << rendered;
        const bool mustHitCpuCache = state->preloadFirst || round > 1;
        if (!rendered || elapsedMs < 0 || (!state->preloadFirst && round == 1 && hit) ||
            (mustHitCpuCache && (!hit || elapsedMs >= 100000))) {
            finish(true, QStringLiteral("Round %1 failed: rendered=%2, memoryHit=%3, elapsedMs=%4; %5")
                                  .arg(round).arg(rendered).arg(hit).arg(elapsedMs).arg(detail));
            return;
        }
        if (round >= rounds) {
            if (state->removalControlCount != rounds - 1 ||
                state->blankControlFrameCount != rounds - 1) {
                finish(true, QStringLiteral("Missing independent-cache removal/blank-frame controls"));
                return;
            }
            if (state->clearAfterBenchmark) {
                writeResult();
                // Unwind NotifyRemoteFrameCompleted and its temporary Model
                // references before exercising the user's delete path.
                QTimer::singleShot(1000, this, runClearControl);
            } else { finish(false, {}); }
            return;
        }
        writeResult();
        // Detach the displayed Model from both Scene and tree, retaining only
        // the independent cache ownership. A real blank frame must be swapped
        // before the next C/S timer; merely hiding a resident Model is invalid.
        QTimer::singleShot(1000, this, [=]() {
            if (state->finished) { return; }
            if (!fileLoader->DetachResidentRemoteModelForBenchmark()) {
                finish(true, QStringLiteral("Cannot remove cached model from scene and tree for the next benchmark round"));
                return;
            }
            const quint64 controlId = std::numeric_limits<quint64>::max() - static_cast<quint64>(round);
            ++state->removalControlCount;
            const int controlIndex = state->removalControls.size();
            QJsonObject control;
            control.insert(QStringLiteral("afterRound"), round);
            // Control IDs near UINT64_MAX cannot be represented exactly as a JSON double.
            control.insert(QStringLiteral("controlRequestId"), QString::number(controlId));
            control.insert(QStringLiteral("detached"), true);
            control.insert(QStringLiteral("blankFrameCompleted"), false);
            state->removalControls.append(control);
            state->blankRequestId = controlId;
            state->blankPending = true;
            writeResult();
            state->blankConnection = connect(rendererWidget, &igQtRenderWidget::CompletedFrame, this,
                    [=](quint64 id, bool success, const QString& blankDetail) {
                if (state->finished || id != controlId || !state->blankPending) { return; }
                state->blankPending = false;
                disconnect(state->blankConnection);
                QJsonObject completedControl = state->removalControls.at(controlIndex).toObject();
                completedControl.insert(QStringLiteral("blankFrameCompleted"), success);
                completedControl.insert(QStringLiteral("detail"), blankDetail);
                state->removalControls.replace(controlIndex, completedControl);
                if (!success) {
                    finish(true, QStringLiteral("Blank control frame failed: ") + blankDetail);
                    return;
                }
                ++state->blankControlFrameCount;
                qInfo().noquote() << QStringLiteral(
                        "[RemoteOpenBenchmark] detached blank control after_round=%1 request=%2 completed=true detail=%3")
                        .arg(round).arg(controlId).arg(blankDetail);
                writeResult();
                QTimer::singleShot(1000, this, startRound);
            });
            QTimer::singleShot(30000, this, [=]() {
                if (state->finished || !state->blankPending || state->blankRequestId != controlId) { return; }
                finish(true, QStringLiteral("Timed out waiting for detached-model blank control frame"));
            });
            rendererWidget->RequestCompletedFrame(controlId);
        });
    });
    state->preloadFailureConnection = connect(fileLoader, &igQtFileLoader::RemotePackageFailed, this,
            [=](const QString& reason) {
        if (state->finished || !state->preloading) { return; }
        state->preload.insert(QStringLiteral("failureReason"), reason);
        finish(true, QStringLiteral("CPU preload failed: ") + reason);
    });
    state->preloadConnection = connect(fileLoader, &igQtFileLoader::RemotePreloadMeasured, this,
            [=](quint64 requestId, bool hit, qint64 elapsedMs, quint64 bytes, const QString& detail) {
        if (state->finished) { return; }
        if (!state->preloading || !state->awaitingMeasurement || requestId != state->expectedRequestId) {
            finish(true, QStringLiteral("Unexpected CPU preload measurement for request %1; expected %2")
                                  .arg(requestId).arg(state->expectedRequestId));
            return;
        }
        state->awaitingMeasurement = false;
        auto* scene = rendererWidget->GetScene();
        auto pool = scene ? scene->GetModelList() : nullptr;
        const quint64 modelsAfter = pool ? pool->GetObjectCount() : 0;
        const bool noSceneModelAdded = pool && modelsAfter == state->preloadSceneModelsBefore;
        const bool cpuOnlyEvidence = QRegularExpression(
                QStringLiteral("(?:^|[;\\s])gpu_resources=0(?:[;\\s]|$)")).match(detail).hasMatch();
        const bool validCache = fileLoader->HasRemoteMemoryCache() &&
                fileLoader->RemoteMemoryCacheBytes() == bytes && bytes > 0;
        state->preload.insert(QStringLiteral("requestId"), static_cast<double>(requestId));
        state->preload.insert(QStringLiteral("memoryHit"), hit);
        state->preload.insert(QStringLiteral("elapsedMs"), static_cast<double>(elapsedMs));
        // Keep the exact byte count, including values beyond JSON's integer precision.
        state->preload.insert(QStringLiteral("cpuBytes"), QString::number(bytes));
        state->preload.insert(QStringLiteral("cpuOnlyEvidence"), cpuOnlyEvidence);
        state->preload.insert(QStringLiteral("noSceneModelAdded"), noSceneModelAdded);
        state->preload.insert(QStringLiteral("sceneModelsBefore"), static_cast<double>(state->preloadSceneModelsBefore));
        state->preload.insert(QStringLiteral("sceneModelsAfter"), static_cast<double>(modelsAfter));
        state->preload.insert(QStringLiteral("detail"), detail);
        state->preload.insert(QStringLiteral("timing"), QStringLiteral(
                "PreloadRemotePackage entry through validated INFO, disk/download validation and CPU parsing/cache retention; "
                "no Open or rendering; reported separately from all Open round times"));
        const bool complete = validCache && cpuOnlyEvidence && noSceneModelAdded && elapsedMs >= 0;
        state->preload.insert(QStringLiteral("complete"), complete);
        qInfo() << "[RemoteOpenBenchmark] CPU preload memory_hit" << hit
                << "elapsed_ms" << elapsedMs << "cpu_bytes" << bytes
                << "gpu_resources_zero" << cpuOnlyEvidence << "scene_unchanged" << noSceneModelAdded;
        if (!complete) {
            finish(true, QStringLiteral("CPU preload evidence failed: cache=%1, gpuResourcesZero=%2, sceneUnchanged=%3, elapsedMs=%4; %5")
                                  .arg(validCache).arg(cpuOnlyEvidence).arg(noSceneModelAdded).arg(elapsedMs).arg(detail));
            return;
        }
        state->preloading = false;
        if (state->preloadOnly) {
            qInfo() << "[RemoteOpenBenchmark] CPU-only preload complete; no Open requested; cached data retained until clear or client exit";
            finish(false, {});
            return;
        }
        writeResult();
        // Wait until this completion signal has unwound; never start Open from
        // inside the preload callback or reuse its INFO request as an Open round.
        QTimer::singleShot(1000, this, startRound);
    });
    writeResult();
    if (state->preloadOnly && state->clearAfterBenchmark) {
        finish(true, QStringLiteral("--remote-cpu-clear-after-benchmark requires displayed Open rounds; it cannot be combined with --remote-cpu-preload-only"));
        return;
    }
    if (!state->preloadFirst) {
        startRound();
        return;
    }
    if (fileLoader->IsRemotePackageRunning()) {
        finish(true, QStringLiteral("Cannot start CPU preload: another remote request is active"));
        return;
    }
    auto* scene = rendererWidget->GetScene();
    auto pool = scene ? scene->GetModelList() : nullptr;
    if (!pool) {
        finish(true, QStringLiteral("CPU preload benchmark requires an initialized scene for its no-mount control"));
        return;
    }
    state->preloadSceneModelsBefore = pool->GetObjectCount();
    state->preloading = true;
    state->preload.insert(QStringLiteral("attempted"), true);
    state->expectedRequestId = 0;
    state->issuingOwnRequest = true;
    const bool started = fileLoader->PreloadRemotePackage(host, port, package, directory);
    state->issuingOwnRequest = false;
    if (state->finished) { return; }
    if (!started) {
        finish(true, QStringLiteral("Cannot start CPU-only C/S preload"));
    } else if (!state->awaitingMeasurement) {
        finish(true, QStringLiteral("CPU preload started without a matching RemoteOpenStarted identity"));
    } else {
        writeResult();
    }
}
