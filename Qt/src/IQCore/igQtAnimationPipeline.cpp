#include <IQCore/igQtAnimationPipeline.h>

#include <IQCore/igQtAnimationFilterManager.h>

#include <QString>

bool igQtDescribeAnimationPipelineInput(
        const igQtAnimationFilterManager& manager, const igQtAnimationPipelineSteps& steps,
        int beforeStep, const igQtAnimationDataInfo& source,
        igQtAnimationDataInfo& input, QString& error) {
    error.clear(); input = source;
    if (beforeStep < 0 || beforeStep > steps.size()) {
        error = QStringLiteral("Filter 位置无效。"); return false;
    }
    for (int i = 0; i < beforeStep; ++i) {
        igQtAnimationDataInfo output;
        if (!manager.describeOutput(steps[i].filterId, input, steps[i].parameters, output, error)) {
            error = QStringLiteral("第 %1 步：%2").arg(i + 1).arg(error); return false;
        }
        input = std::move(output);
    }
    return true;
}

bool igQtExecuteAnimationPipeline(
        const igQtAnimationFilterManager& manager,
        const igQtAnimationPipelineSteps& steps,
        const igQtAnimationFrameContext& context,
        igQtAnimationFilterResult& finalResult,
        QString* error) {
    finalResult = {};
    if (error) error->clear();

    if (!context.input) {
        if (error) *error = QStringLiteral("动画 Pipeline 没有输入对象。");
        return false;
    }

    iGame::DataObject::Pointer working = context.input;
    for (int index = 0; index < steps.size(); ++index) {
        const auto& step = steps.at(index);
        const auto* descriptor = manager.descriptor(step.filterId);
        if (!descriptor) {
            finalResult.error =
                    QStringLiteral("Pipeline 第 %1 步引用了未注册 Filter“%2”。")
                            .arg(index + 1)
                            .arg(step.filterId);
            if (error) *error = finalResult.error;
            return false;
        }

        igQtAnimationFrameContext stepContext = context;
        stepContext.input = working;
        const auto result =
                manager.execute(step.filterId, stepContext, step.parameters);
        if (!result.success) {
            finalResult = result;
            if (error) *error = result.error;
            return false;
        }

        // 若 Filter 返回新对象，这一步会释放上一个中间对象的本地引用；
        // 若仍是原始帧对象，Model/时间帧仍持有它，不会被误删。
        working = result.output;

        if (!result.displayAttribute.isEmpty()) {
            finalResult.displayAttribute = result.displayAttribute;
            finalResult.displayDimension = result.displayDimension;
        }
    }

    finalResult.success = true;
    finalResult.output = working;
    return true;
}
