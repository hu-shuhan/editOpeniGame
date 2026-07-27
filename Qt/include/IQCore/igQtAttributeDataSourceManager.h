#ifndef igQtAttributeDataSourceManager_h
#define igQtAttributeDataSourceManager_h

#include <IQCore/igQtExportModule.h>

#include <Attribute/iGameAttributeDataSource.h>

#include <QHash>
#include <QMutex>
#include <QObject>
#include <QSet>
#include <QString>

#include <memory>
#include <stop_token>
#include <vector>

Q_DECLARE_METATYPE(iGame::AttributeDataTarget)
Q_DECLARE_METATYPE(iGame::AttributeDataLoadState)

class IG_QT_MODULE_EXPORT igQtAttributeDataSourceManager final : public QObject {
    Q_OBJECT

public:
    static igQtAttributeDataSourceManager* Instance();

    void RegisterSource(
        const iGame::DataObject::Pointer& rootObject,
        iGame::AttributeDataSourcePointer source);
    iGame::AttributeDataSourcePointer EnsureSource(
        const iGame::DataObject::Pointer& rootObject);
    [[nodiscard]] iGame::AttributeDataSourcePointer Source(
        int rootObjectId) const;
    [[nodiscard]] std::vector<iGame::AttributeDataDescriptor> Attributes(
        const iGame::DataObject::Pointer& rootObject);
    void RequestAttribute(
        int rootObjectId,
        const iGame::AttributeDataTarget& target);
    void SetActiveAttributeTarget(
        int rootObjectId,
        const iGame::AttributeDataTarget& target);
    void ClearActiveAttributeTarget(int rootObjectId);
    void ReleaseSource(int rootObjectId);

signals:
    void AttributeLoadStateChanged(
        int rootObjectId,
        iGame::AttributeDataTarget target,
        iGame::AttributeDataLoadState state,
        int nativeIndex,
        QString error);

private:
    explicit igQtAttributeDataSourceManager(QObject* parent = nullptr);

    [[nodiscard]] static QString RequestKey(
        int rootObjectId,
        const iGame::AttributeDataTarget& target,
        quint64 generation);
    [[nodiscard]] bool IsActiveAttributeTarget(
        int rootObjectId,
        const iGame::AttributeDataTarget& target,
        quint64 generation) const;

    mutable QMutex m_mutex;
    QHash<int, iGame::AttributeDataSourcePointer> m_sources;
    QHash<int, iGame::AttributeDataTarget> m_activeAttributeTargets;
    QHash<int, quint64> m_attributeRequestGenerations;
    QHash<int, std::shared_ptr<std::stop_source>> m_attributeStopSources;
    QSet<QString> m_loadingRequests;
};

#endif
