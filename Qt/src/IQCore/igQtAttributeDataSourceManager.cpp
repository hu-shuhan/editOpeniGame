#include <IQCore/igQtAttributeDataSourceManager.h>

#include <Attribute/iGameResidentAttributeDataSource.h>

#include <QMetaObject>
#include <QMutexLocker>
#include <QPointer>
#include <QThread>

igQtAttributeDataSourceManager* igQtAttributeDataSourceManager::Instance() {
    static auto* instance = new igQtAttributeDataSourceManager;
    return instance;
}

igQtAttributeDataSourceManager::igQtAttributeDataSourceManager(QObject* parent)
    : QObject(parent) {
    qRegisterMetaType<iGame::AttributeDataTarget>();
    qRegisterMetaType<iGame::AttributeDataLoadState>();
}

void igQtAttributeDataSourceManager::RegisterSource(
    const iGame::DataObject::Pointer& rootObject,
    iGame::AttributeDataSourcePointer source) {
    if (rootObject == nullptr || source == nullptr) return;
    QMutexLocker locker(&m_mutex);
    m_sources.insert(rootObject->GetDataObjectId(), std::move(source));
}

iGame::AttributeDataSourcePointer igQtAttributeDataSourceManager::EnsureSource(
    const iGame::DataObject::Pointer& rootObject) {
    if (rootObject == nullptr) return {};
    const int objectId = rootObject->GetDataObjectId();
    {
        QMutexLocker locker(&m_mutex);
        const auto iterator = m_sources.constFind(objectId);
        if (iterator != m_sources.constEnd()) {
            return iterator.value();
        }
    }
    auto source = std::make_shared<iGame::ResidentAttributeDataSource>(rootObject);
    RegisterSource(rootObject, source);
    return source;
}

iGame::AttributeDataSourcePointer igQtAttributeDataSourceManager::Source(
    const int rootObjectId) const {
    QMutexLocker locker(&m_mutex);
    const auto iterator = m_sources.constFind(rootObjectId);
    return iterator == m_sources.constEnd()
        ? iGame::AttributeDataSourcePointer{}
        : iterator.value();
}

std::vector<iGame::AttributeDataDescriptor> igQtAttributeDataSourceManager::Attributes(
    const iGame::DataObject::Pointer& rootObject) {
    auto source = EnsureSource(rootObject);
    return source != nullptr
        ? source->Attributes()
        : std::vector<iGame::AttributeDataDescriptor>{};
}

void igQtAttributeDataSourceManager::RequestAttribute(
    const int rootObjectId,
    const iGame::AttributeDataTarget& target) {
    SetActiveAttributeTarget(rootObjectId, target);
    auto source = Source(rootObjectId);
    if (source == nullptr) {
        emit AttributeLoadStateChanged(
            rootObjectId,
            target,
            iGame::AttributeDataLoadState::Failed,
            -1,
            QStringLiteral("属性数据源不存在"));
        return;
    }

    quint64 generation = 0u;
    std::shared_ptr<std::stop_source> stopSource;
    {
        QMutexLocker locker(&m_mutex);
        generation = m_attributeRequestGenerations.value(rootObjectId, 0u);
        stopSource = m_attributeStopSources.value(rootObjectId);
    }
    if (stopSource == nullptr) {
        emit AttributeLoadStateChanged(
            rootObjectId,
            target,
            iGame::AttributeDataLoadState::Failed,
            -1,
            QStringLiteral("属性请求状态不存在"));
        return;
    }

    const auto descriptor = source->Attribute(target);
    if (descriptor.has_value() &&
        descriptor->state == iGame::AttributeDataLoadState::Loaded &&
        descriptor->nativeIndex >= 0) {
        emit AttributeLoadStateChanged(
            rootObjectId, target, iGame::AttributeDataLoadState::Loaded,
            descriptor->nativeIndex, {});
        return;
    }
    const auto requestKey = RequestKey(rootObjectId, target, generation);
    {
        QMutexLocker locker(&m_mutex);
        if (m_loadingRequests.contains(requestKey)) return;
        m_loadingRequests.insert(requestKey);
    }
    emit AttributeLoadStateChanged(
        rootObjectId,
        target,
        iGame::AttributeDataLoadState::Loading,
        -1,
        {});

    QPointer<igQtAttributeDataSourceManager> self(this);
    const auto stopToken = stopSource->get_token();
    auto* worker = QThread::create([
        self,
        source = std::move(source),
        rootObjectId,
        target,
        requestKey,
        generation,
        stopToken]() {
        if (!self || stopToken.stop_requested() ||
            !self->IsActiveAttributeTarget(rootObjectId, target, generation)) {
            if (self) {
                QMetaObject::invokeMethod(self, [self, rootObjectId, target, requestKey]() {
                    if (!self) return;
                    {
                        QMutexLocker locker(&self->m_mutex);
                        self->m_loadingRequests.remove(requestKey);
                    }
                    emit self->AttributeLoadStateChanged(
                        rootObjectId,
                        target,
                        iGame::AttributeDataLoadState::Unloaded,
                        -1,
                        {});
                }, Qt::QueuedConnection);
            }
            return;
        }
        auto prepareResult = source->PrepareAttribute(target, stopToken);
        if (!self) return;
        QMetaObject::invokeMethod(self, [self, source, rootObjectId, target, requestKey,
                                         generation, stopToken,
                                         prepareResult = std::move(prepareResult)]() mutable {
            if (!self) return;
            iGame::AttributeDataLoadResult result = std::move(prepareResult);
            const bool active = !stopToken.stop_requested() &&
                self->IsActiveAttributeTarget(rootObjectId, target, generation);
            if (result.success && active) {
                result = source->CommitAttribute(target);
            }
            {
                QMutexLocker locker(&self->m_mutex);
                self->m_loadingRequests.remove(requestKey);
            }
            if (active && result.success && result.object != nullptr) {
                result.object->Modified();
            }
            emit self->AttributeLoadStateChanged(
                rootObjectId,
                target,
                !active
                    ? iGame::AttributeDataLoadState::Unloaded
                    : result.success
                    ? iGame::AttributeDataLoadState::Loaded
                    : iGame::AttributeDataLoadState::Failed,
                active ? result.nativeIndex : -1,
                QString::fromUtf8(result.error.c_str()));
        }, Qt::QueuedConnection);
    });
    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    worker->start();
}

void igQtAttributeDataSourceManager::SetActiveAttributeTarget(
    const int rootObjectId,
    const iGame::AttributeDataTarget& target) {
    QMutexLocker locker(&m_mutex);
    const auto activeIterator = m_activeAttributeTargets.constFind(rootObjectId);
    const auto stopIterator = m_attributeStopSources.constFind(rootObjectId);
    if (activeIterator != m_activeAttributeTargets.constEnd() &&
        activeIterator.value() == target &&
        stopIterator != m_attributeStopSources.constEnd() &&
        stopIterator.value() != nullptr &&
        !stopIterator.value()->stop_requested()) {
        return;
    }
    if (stopIterator != m_attributeStopSources.constEnd() && stopIterator.value() != nullptr) {
        stopIterator.value()->request_stop();
    }
    m_activeAttributeTargets.insert(rootObjectId, target);
    m_attributeRequestGenerations.insert(
        rootObjectId,
        m_attributeRequestGenerations.value(rootObjectId, 0u) + 1u);
    m_attributeStopSources.insert(rootObjectId, std::make_shared<std::stop_source>());
}

void igQtAttributeDataSourceManager::ClearActiveAttributeTarget(
    const int rootObjectId) {
    QMutexLocker locker(&m_mutex);
    const auto stopIterator = m_attributeStopSources.find(rootObjectId);
    if (stopIterator != m_attributeStopSources.end() && stopIterator.value() != nullptr) {
        stopIterator.value()->request_stop();
    }
    m_activeAttributeTargets.remove(rootObjectId);
    m_attributeStopSources.remove(rootObjectId);
    m_attributeRequestGenerations.insert(
        rootObjectId,
        m_attributeRequestGenerations.value(rootObjectId, 0u) + 1u);
}

bool igQtAttributeDataSourceManager::IsActiveAttributeTarget(
    const int rootObjectId,
    const iGame::AttributeDataTarget& target,
    const quint64 generation) const {
    QMutexLocker locker(&m_mutex);
    const auto iterator = m_activeAttributeTargets.constFind(rootObjectId);
    return iterator != m_activeAttributeTargets.constEnd() &&
        iterator.value() == target &&
        m_attributeRequestGenerations.value(rootObjectId, 0u) == generation;
}

void igQtAttributeDataSourceManager::ReleaseSource(const int rootObjectId) {
    QMutexLocker locker(&m_mutex);
    const auto stopIterator = m_attributeStopSources.find(rootObjectId);
    if (stopIterator != m_attributeStopSources.end() && stopIterator.value() != nullptr) {
        stopIterator.value()->request_stop();
    }
    m_sources.remove(rootObjectId);
    m_activeAttributeTargets.remove(rootObjectId);
    m_attributeStopSources.remove(rootObjectId);
    m_attributeRequestGenerations.remove(rootObjectId);
    const auto prefix = QString::number(rootObjectId) + QLatin1Char(':');
    for (auto iterator = m_loadingRequests.begin(); iterator != m_loadingRequests.end();) {
        if (iterator->startsWith(prefix)) {
            iterator = m_loadingRequests.erase(iterator);
        } else {
            ++iterator;
        }
    }
}

QString igQtAttributeDataSourceManager::RequestKey(
    const int rootObjectId,
    const iGame::AttributeDataTarget& target,
    const quint64 generation) {
    return QStringLiteral("%1:%2:%3:%4:%5")
        .arg(rootObjectId)
        .arg(target.frameIndex)
        .arg(QString::fromUtf8(target.blockPath.c_str()))
        .arg(static_cast<qulonglong>(target.sourceIndex))
        .arg(generation);
}
