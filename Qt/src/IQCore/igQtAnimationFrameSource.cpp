#include <IQCore/igQtAnimationFrameSource.h>
#include <iGameUnstructuredMesh.h>
#include <cmath>

namespace {
using namespace iGame;

DataObject::Pointer loadFrame(DataObject::Pointer source, int index, QString& error) {
    auto frames = source->PeekTimeFrames();
    auto loaded = frames->GetTargetTimeFrameData(index);
    if (loaded.empty()) { error = QStringLiteral("时间帧没有数据。"); return nullptr; }
    if (frames->GetTargetFrameType(index) == StreamingType::SingleFieldAttributes) {
        auto attributes = DynamicCast<AttributeSet>(loaded.front());
        auto mesh = UnstructuredMesh::TransDataObjToUnstructuredMesh(source);
        if (!attributes || !mesh) {
            error = QStringLiteral("无法为单字段动画建立独立网格。"); return nullptr;
        }
        auto output = UnstructuredMesh::New();
        auto points = Points::New(); points->DeepCopy(mesh->GetPoints());
        auto cells = CellArray::New(); cells->DeepCopy(mesh->GetCells());
        auto types = UnsignedIntArray::New(); types->DeepCopy(mesh->GetCellTypes());
        output->SetPoints(points); output->SetCells(cells, types);
        output->SetAttributeSet(attributes);
        return output;
    }
    auto output = DrawObject::New();
    for (auto& object : loaded) {
        auto child = DynamicCast<DataObject>(object);
        if (!child) { error = QStringLiteral("读取动画帧失败。"); return nullptr; }
        // Parent arrays describe the blocks, without duplicating their values.
        if (!output->HasSubDataObject()) {
            auto attributes = child->GetAttributeSet();
            for (IGsize i = 0; attributes && i < attributes->GetNumberOfAttributes(); ++i) {
                const auto& attr = attributes->GetAttribute(i);
                if (attr.isDeleted || !attr.pointer) continue;
                auto placeholder = DoubleArray::New();
                placeholder->SetName(attr.pointer->GetName());
                placeholder->SetDimension(attr.pointer->GetDimension());
                auto range = DoubleArray::New();
                if (attr.dataRange) range->DeepCopy(attr.dataRange);
                else { range->SetDimension(2); range->Resize(attr.pointer->GetDimension() + 1); }
                output->GetAttributeSet()->AddAttribute(attr.type, attr.attachmentType, placeholder, range);
            }
        }
        // Readers may allocate IDs on parallel workers. Restore manifest order
        // before attaching blocks so corresponding blocks interpolate together.
        child->SetUniqueDataObjectId();
        output->AddSubDataObject(child);
    }
    return output;
}

bool interpolate(DataObject::Pointer first, DataObject::Pointer second, float weight, QString& error) {
    if (first->HasSubDataObject() || second->HasSubDataObject()) {
        if (first->GetNumberOfSubDataObjects() != second->GetNumberOfSubDataObjects()) {
            error = QStringLiteral("相邻帧的数据块数量不同，不能插值。"); return false;
        }
        auto right = second->SubDataObjectIteratorBegin();
        for (auto left = first->SubDataObjectIteratorBegin(); left != first->SubDataObjectIteratorEnd(); ++left, ++right)
            if (!interpolate(left->second, right->second, weight, error)) return false;
        first->ReCollectSubDataObjectDataRange(); first->UpdateSubDataObjectDataRange();
        return true;
    }
    auto a = DynamicCast<PointSet>(first), b = DynamicCast<PointSet>(second);
    if (!a || !b || a->GetDataObjectType() != b->GetDataObjectType() || a->GetNumberOfPoints() != b->GetNumberOfPoints()) {
        error = QStringLiteral("相邻帧的网格类型或点数不同，不能插值。"); return false;
    }
    auto ca = a->GetCellArray(), cb = b->GetCellArray();
    if (bool(ca) != bool(cb) || (ca && ca->GetNumberOfCells() != cb->GetNumberOfCells())) {
        error = QStringLiteral("相邻帧的单元数量不同，不能插值。"); return false;
    }
    auto ua = DynamicCast<UnstructuredMesh>(a), ub = DynamicCast<UnstructuredMesh>(b);
    for (IGsize cell = 0; ca && cell < ca->GetNumberOfCells(); ++cell) {
        const igIndex *ia, *ib;
        const int na = ca->GetCellIds(cell, ia), nb = cb->GetCellIds(cell, ib);
        if (na != nb || (ua && ua->GetCellType(cell) != ub->GetCellType(cell)) || !std::equal(ia, ia + na, ib)) {
            error = QStringLiteral("相邻帧的单元连接关系不同，不能插值。"); return false;
        }
    }
    auto aa = a->GetAttributeSet(), ab = b->GetAttributeSet();
    if (aa->GetNumberOfAttributes() != ab->GetNumberOfAttributes()) {
        error = QStringLiteral("相邻帧的字段数量不同，不能插值。"); return false;
    }
    for (IGsize field = 0; field < aa->GetNumberOfAttributes(); ++field) {
        auto& left = aa->GetAttribute(field); auto& right = ab->GetAttribute(field);
        if (!left.pointer || !right.pointer || left.pointer->GetName() != right.pointer->GetName() ||
            left.attachmentType != right.attachmentType || left.type != right.type ||
            left.pointer->GetDimension() != right.pointer->GetDimension() ||
            left.pointer->GetNumberOfValues() != right.pointer->GetNumberOfValues()) {
            error = QStringLiteral("相邻帧的字段名称、归属或尺寸不同，不能插值。"); return false;
        }
        for (IGsize value = 0; value < left.pointer->GetNumberOfValues(); ++value) {
            const double x = left.pointer->GetValue(value);
            left.pointer->SetValue(value, x + weight * (right.pointer->GetValue(value) - x));
        }
        left.pointer->Modified(); left.UpdateAllDataRange();
    }
    for (IGsize point = 0; point < a->GetNumberOfPoints(); ++point) {
        const auto p = a->GetPoint(point);
        a->SetPoint(point, p + weight * (b->GetPoint(point) - p));
    }
    a->GetPoints()->Modified(); aa->Modified();
    return true;
}
}

bool igQtLoadAnimationFrame(iGame::DataObject::Pointer source,
        const igQtAnimationFrameRequest& request, igQtAnimationFrameContext& context, QString& error) {
    error.clear(); context = {};
    auto frames = source ? source->PeekTimeFrames() : nullptr;
    if (!frames || request.sourceFrame < 0 || request.sourceFrame >= frames->GetTimeNum() ||
        (request.interpolate && (request.sourceFrame + 1 >= frames->GetTimeNum() ||
         !std::isfinite(request.weight) || request.weight < 0 || request.weight > 1))) {
        error = QStringLiteral("动画时间帧或插值比例无效。"); return false;
    }
    // This output path owns caching. Disable the old source-frame cache even if
    // another legacy control enabled it, avoiding a second persistent cache.
    frames->DisableCache();
    iGame::DataObject::DeferDrawableConversionScope cpu;
    const int index = request.sourceFrame + (request.interpolate && request.weight == 1 ? 1 : 0);
    auto output = loadFrame(source, index, error);
    if (!output) return false;
    if (request.interpolate && request.weight > 0 && request.weight < 1) {
        auto next = loadFrame(source, request.sourceFrame + 1, error);
        if (!next || !interpolate(output, next, request.weight, error)) return false;
    }
    output->SetName(source->GetName());
    output->SetTimeFrames(frames);
    auto draw = iGame::DynamicCast<iGame::DrawObject>(output);
    auto sourceDraw = iGame::DynamicCast<iGame::DrawObject>(source);
    if (draw && sourceDraw) draw->SetViewStyle(sourceDraw->GetViewStyle());
    context.input = output;
    context.sourceFrameIndex = request.sourceFrame;
    context.sourceTime = frames->GetTargetTimeValue(request.sourceFrame);
    context.outputFrameIndex = request.sourceFrame;
    context.outputTime = context.sourceTime;
    if (request.interpolate)
        context.outputTime += request.weight * (frames->GetTargetTimeValue(request.sourceFrame + 1) - context.sourceTime);
    return true;
}
