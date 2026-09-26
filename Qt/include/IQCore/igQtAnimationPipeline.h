#pragma once

#include <IQCore/igQtAnimationFilterTypes.h>

#include <QList>
#include <QString>
#include <QVariantMap>

class igQtAnimationFilterManager;

/** 动画 Pipeline 中的一个 Filter 节点。 */
struct IG_QT_MODULE_EXPORT igQtAnimationPipelineStep {
    QString filterId;
    QVariantMap parameters;
};

using igQtAnimationPipelineSteps = QList<igQtAnimationPipelineStep>;

/**
 * 按顺序执行动画 Pipeline。
 *
 * 生命周期约定：
 * - Filter 返回的对象与输入对象相同：视为原地修改属性，继续复用，不释放；
 * - Filter 返回了新对象：下游使用新对象，并释放上一个中间对象的本地引用。
 * 调用结束后 finalResult.output 是最终可显示对象；调用方负责在播放/导出后清理自己的引用。
 */
IG_QT_MODULE_EXPORT bool igQtExecuteAnimationPipeline(
        const igQtAnimationFilterManager& manager,
        const igQtAnimationPipelineSteps& steps,
        const igQtAnimationFrameContext& context,
        igQtAnimationFilterResult& finalResult,
        QString* error = nullptr);
