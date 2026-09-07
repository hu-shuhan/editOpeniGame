#pragma once

#include <IQCore/igQtExportModule.h>
#include <iGameDataObject.h>

#include <QVariant>
#include <QString>
#include <QStringList>

#include <functional>
#include <vector>

/**
 * 动画 Filter 的输出如何参与当前帧显示。
 * ModifyInput 适用于涡量、梯度等在输入上增加属性的 Filter；
 * ReplaceFrame 适用于等值面、切片等生成新网格的 Filter。
 */
enum class igQtAnimationFilterOutputPolicy {
    ModifyInput,
    ReplaceFrame
};

/** 通用参数编辑器支持的基础控件类型。 */
enum class igQtAnimationFilterParameterType {
    String,
    Integer,
    Double,
    Boolean,
    Choice
};

/**
 * 一个 Filter 参数的界面及校验描述。
 * choices 仅用于 Choice；minimum/maximum 为空表示不限制。
 */
struct IG_QT_MODULE_EXPORT igQtAnimationFilterParameter {
    QString key;
    QString title;
    igQtAnimationFilterParameterType type{
            igQtAnimationFilterParameterType::String};
    QVariant defaultValue;
    QVariant minimum;
    QVariant maximum;
    QStringList choices;
};

using igQtAnimationFilterParameterSchema =
        std::vector<igQtAnimationFilterParameter>;

/**
 * 动画系统传给 Filter Adapter 的单帧上下文。
 * input 必须是完成原始帧加载或数值插值后的本帧对象。
 */
struct IG_QT_MODULE_EXPORT igQtAnimationFrameContext {
    iGame::DataObject::Pointer input;
    int sourceFrameIndex{-1};
    double sourceTime{0.0};
    int outputFrameIndex{-1};
    double outputTime{0.0};
    bool exporting{false};
};

/** Filter Adapter 执行一帧后返回给动画系统的统一结果。 */
struct IG_QT_MODULE_EXPORT igQtAnimationFilterResult {
    bool success{false};
    iGame::DataObject::Pointer output;
    QString displayAttribute;
    int displayDimension{-1};
    QString error;
};

/**
 * 一个可用于动画的 Filter 描述。
 * 动画系统只认识这个结构，不直接依赖 ContourFilter 等具体类型。
 */
struct IG_QT_MODULE_EXPORT igQtAnimationFilterDescriptor {
    QString id;
    QString displayName;
    igQtAnimationFilterOutputPolicy outputPolicy{
            igQtAnimationFilterOutputPolicy::ModifyInput};

    std::function<bool(iGame::DataObject::Pointer, QString&)> supports;
    std::function<igQtAnimationFilterParameterSchema(
            iGame::DataObject::Pointer)> parameterSchema;
    std::function<bool(const QVariantMap&,
                       iGame::DataObject::Pointer,
                       QString&)> validateParameters;
    std::function<igQtAnimationFilterResult(
            const igQtAnimationFrameContext&,
            const QVariantMap&)> execute;
};
