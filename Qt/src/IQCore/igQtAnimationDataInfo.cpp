#include <IQCore/igQtAnimationFilterTypes.h>

bool igQtDescribeAnimationData(iGame::DataObject::Pointer input,
                               igQtAnimationDataInfo& info, QString& error) {
    info = {};
    error.clear();
    if (!input) { error = QStringLiteral("没有动画输入数据。"); return false; }
    info.meshType = input->GetDataObjectType();
    info.hasGeometry = input->GetPoints() && input->GetCellArray();
    if (auto attrs = input->GetAttributeSet()) {
        for (IGsize i = 0; i < attrs->GetNumberOfAttributes(); ++i) {
            const auto& attr = attrs->GetAttribute(i);
            if (attr.isDeleted) continue;
            if (!attr.pointer || attr.pointer->GetDimension() < 1) {
                error = QStringLiteral("输入包含空字段或无效的字段维度。");
                return false;
            }
            info.fields.push_back({QString::fromStdString(attr.pointer->GetName()),
                                   attr.attachmentType, attr.pointer->GetDimension(), attr.type});
        }
    }
    if (input->HasSubDataObject()) {
        for (auto it = input->SubDataObjectIteratorBegin(); it != input->SubDataObjectIteratorEnd(); ++it) {
            igQtAnimationDataInfo child;
            if (!igQtDescribeAnimationData(it->second, child, error)) return false;
            info.blocks.push_back(std::move(child));
        }
    }
    return true;
}
