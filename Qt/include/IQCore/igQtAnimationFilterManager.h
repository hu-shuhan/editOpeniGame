#pragma once

#include <IQCore/igQtAnimationFilterTypes.h>

#include <QList>
#include <QMap>

/**
 * 动画 Filter 的注册和统一执行入口。
 *
 * 本类不负责 QTimer、文件读取、OpenGL 或模型树；这些仍由动画播放层负责。
 * 它只负责把 Filter ID、参数快照和当前帧交给对应 Adapter，并统一返回结果。
 */
class IG_QT_MODULE_EXPORT igQtAnimationFilterManager {
public:
    bool registerFilter(igQtAnimationFilterDescriptor descriptor,
                        QString* error = nullptr);
    bool unregisterFilter(const QString& id);
    void clear();

    bool contains(const QString& id) const;
    QList<QString> filterIds() const;
    QList<QString> filterDisplayNames() const;

    const igQtAnimationFilterDescriptor* descriptor(
            const QString& id) const;

    igQtAnimationFilterParameterSchema parameterSchema(
            const QString& id,
            iGame::DataObject::Pointer input,
            QString* error = nullptr) const;

    bool validateParameters(const QString& id,
                            const QVariantMap& parameters,
                            iGame::DataObject::Pointer input,
                            QString& error) const;

    igQtAnimationFilterResult execute(
            const QString& id,
            const igQtAnimationFrameContext& context,
            const QVariantMap& parameterSnapshot) const;

private:
    QMap<QString, igQtAnimationFilterDescriptor> m_Filters;
};
