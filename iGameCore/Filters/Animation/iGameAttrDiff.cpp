#include "iGameAttrDiff.h"
#include "iGameDataObject.h"
#include "iGameDrawObject.h"
#include <cmath>
#include <limits>


IGAME_NAMESPACE_BEGIN

iGameAttrDiff::iGameAttrDiff() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

bool iGameAttrDiff::Execute() {
    auto input = GetInput(0);
    if (input.IsNull()) {
        m_Message = "Input is null.";
        return false;
    }
    auto obj = DynamicCast<DataObject>(input);
    if (obj.IsNull()) {
        m_Message = "Input is not a DataObject.";
        return false;
    }
    auto frames = obj->PeekTimeFrames();
    if (frames.IsNull() || frames->GetTimeNum() == 0) {
        m_Message = "Input has no time frames.";
        return false;
    }
    const size_t frameNum = frames->GetTimeNum();
    if (m_AttrName.empty()) {
        auto attrSet = obj->GetAttributeSet();
        if (!attrSet || m_AttrIndex < 0 || m_AttrIndex >= (int)attrSet->GetNumberOfAttributes()) {
            m_Message = "Input has no attributes.";
            return false;
        }
        auto attr = attrSet->GetAttribute(m_AttrIndex);
        if (attr.IsNone() || attr.pointer.IsNull()) {
            m_Message = "Input attribute is null.";
            return false;
        }
        m_AttrName = attr.pointer->GetName();
    }

    if (m_outputName.empty()) { m_outputName = m_AttrName + "_diff"; }
    if (m_FrameIndex >= 0) {
        if ((size_t) m_FrameIndex >= frameNum) {
            m_Message = "Frame index out of range.";
            return false;
        }
        if (!ComputeFrame(obj, frames, (unsigned int) m_FrameIndex)) {
            m_Message += " Failed to compute single frame.";
            return false;
        }
    } else {
        for (size_t i = 0; i < frameNum; ++i) {
            UpdateProgress((double) i / (double) frameNum);
            if (!ComputeFrame(obj, frames, (unsigned int) i)) {
                m_Message += " Failed to compute NO. " + std::to_string(i) + " frame.";
                return false;
            }
        }
        UpdateProgress(1.0);
    }
    SyncParentAttribute(obj, m_outputName);
    if (auto drawObj = DynamicCast<DrawObject>(obj)) { drawObj->ForceReConvertToDrawableData(); }
    SetOutput(obj);
    return true;
}

bool iGameAttrDiff::ComputeFrame(DataObject::Pointer obj, StreamingData::Pointer frames, unsigned int frameIndex) {

    const bool useMounted = (m_FrameIndex >= 0 && obj->HasSubDataObject());
    auto curAttrs = CollectFrameAttrs(obj, frames, frameIndex, useMounted);
    if (curAttrs.empty()) {
        m_Message = "frame " + std::to_string(frameIndex) + " has no attributes.";
        return false;
    }
    std::vector<AttributeSet::Pointer> prevAttrs;
    if (frameIndex > 0) {
        prevAttrs = CollectFrameAttrs(obj, frames, frameIndex - 1, false);
        if (prevAttrs.size() != curAttrs.size()) {
            m_Message = "attribute mismatch between frames" + std::to_string(frameIndex - 1) + " and " +
                        std::to_string(frameIndex);
            return false;
        }
    }
    for (size_t i = 0; i < curAttrs.size(); ++i) {
        if (!ApplyDiffToObject(curAttrs[i], (frameIndex > 0 ? prevAttrs[i] : nullptr), frameIndex)) {
            m_Message = "Failed to apply diff to object at frame " + std::to_string(frameIndex);
            return false;
        }
    }
    return true;
}

std::vector<AttributeSet::Pointer> iGameAttrDiff::CollectFrameAttrs(DataObject::Pointer obj,
                                                                    StreamingData::Pointer frames, unsigned int index,
                                                                    bool useMounted) {
    std::vector<AttributeSet::Pointer> attrs;
    if (useMounted) {
        for (auto it = obj->SubDataObjectIteratorBegin(); it != obj->SubDataObjectIteratorEnd(); ++it) {
            auto subObj = DynamicCast<DataObject>(it->second);
            if (subObj) {
                if (auto subAttrs = subObj->GetAttributeSet()) { attrs.push_back(subAttrs); }
            }
        }
        return attrs;
    }
    auto frameData = frames->GetTargetTimeFrameData(index);
    for (auto& data: frameData) {
        if (auto subObj = DynamicCast<DataObject>(data)) {
            if (auto subAttrs = subObj->GetAttributeSet()) { attrs.push_back(subAttrs); }
        } else if (auto frameAttrSet = DynamicCast<AttributeSet>(data)) {
            attrs.push_back(frameAttrSet);
        }
    }
    return attrs;
}

bool iGameAttrDiff::ApplyDiffToObject(AttributeSet::Pointer curAttrs, AttributeSet::Pointer prevAttrs,
                                      unsigned int frameIndex) {
    if (curAttrs.IsNull()) {
        m_Message = "Current attributes are null.";
        return false;
    }
    auto& srcAttr = curAttrs->GetAttribute(m_AttrName);
    if (srcAttr.IsNone() || srcAttr.pointer.IsNull()) {
        m_Message = "attribute " + m_AttrName + " not found in frame " + std::to_string(frameIndex);
        return false;
    }
    ArrayObject::Pointer src = srcAttr.pointer;
    const int dim = src->GetDimension();
    const IGsize numElements = src->GetNumberOfElements();
    FloatArray::Pointer out = FloatArray::New();
    out->SetDimension(1);
    out->SetName(m_outputName);
    out->Resize(numElements);
    if (frameIndex == 0) {
        for (IGsize i = 0; i < numElements; ++i) { out->SetValue(i, 0.0f); }
    } else {
        if (prevAttrs.IsNull()) {
            m_Message = "Previous attributes are null for frame " + std::to_string(frameIndex - 1);
            return false;
        }
        auto& prevAttr = prevAttrs->GetAttribute(m_AttrName);
        if (prevAttr.IsNone() || prevAttr.pointer.IsNull()) {
            m_Message = "attribute " + m_AttrName + " not found in previous frame " + std::to_string(frameIndex - 1);
            return false;
        }
        ArrayObject::Pointer prev = prevAttr.pointer;
        if (prev->GetDimension() != dim || prev->GetNumberOfElements() != numElements) {
            m_Message = "attribute " + m_AttrName + " dimension or size mismatch between frames";
            return false;
        }
        for (IGsize i = 0; i < numElements; ++i) {
            double curVal = GetAttrValue(src, i, dim, m_component);
            double prevVal = GetAttrValue(prev, i, dim, m_component);
            double diff = 0.0;
            switch (m_DiffMode) {
                case 1: // 绝对值
                    diff = std::abs(curVal - prevVal);
                    break;
                case 2: // 相对变化率
                    diff = (std::abs(prevVal) > 1e-30 ? (curVal - prevVal) / std::abs(prevVal) : 0.0);
                    break;
                default: // 带符号
                    diff = curVal - prevVal;
                    break;
            }
            out->SetValue(i, diff);
        }
    }
    auto range = ComputeRange(out);
    int outIdx = curAttrs->GetAttributeIndex(m_outputName);
    if (outIdx >= 0) { // 已存在
        auto& attr = curAttrs->GetAttribute(outIdx);
        attr.SetType(IG_SCALAR);
        attr.SetAttachmentType(srcAttr.attachmentType);
        attr.SetPointer(out);
        attr.SetDataRange(range);
    } else {
        curAttrs->AddScalar(srcAttr.attachmentType, out, range);
    }
    return true;
}

double iGameAttrDiff::GetAttrValue(ArrayObject::Pointer arr, IGsize i, int dim, int component) {
    if (dim <= 1) return arr->GetValue(i);
    if (component >= 0 && component < dim) { return arr->GetElementValue(i, component); }
    double sum = 0.0;
    for (int j = 0; j < dim; j++) { double v = arr->GetElementValue(i, j); sum += v * v; }
    return std::sqrt(sum);
}

DoubleArray::Pointer iGameAttrDiff::ComputeRange(ArrayObject::Pointer arr) {
    double minv = std::numeric_limits<double>::max();
    double maxv = std::numeric_limits<double>::lowest();
    const IGsize num = arr->GetNumberOfElements();
    for (IGsize i = 0; i < num; ++i) {
        double val = arr->GetValue(i);
        if (val < minv) minv = val;
        if (val > maxv) maxv = val;
    }
    if (num == 0) { minv = maxv = 0.0; }
    auto range = DoubleArray::New();
    range->SetDimension(2);
    range->Resize(2);
    range->SetValue(0, minv);
    range->SetValue(1, maxv);
    return range;
}

void iGameAttrDiff::SyncParentAttribute(DataObject::Pointer obj, const std::string& outName) {
    if (!obj->HasSubDataObject()) { return; }
    auto parentAttrSet = obj->GetAttributeSet();
    if (!parentAttrSet) { return; }
    if (parentAttrSet->GetAttributeIndex(outName) >= 0) { return; }
    auto firstSubObj = obj->SubDataObjectIteratorBegin()->second;
    auto firstSubAttrSet = firstSubObj ? firstSubObj->GetAttributeSet() : nullptr;
    if (!firstSubAttrSet) { return; }

    int subIdx = firstSubAttrSet->GetAttributeIndex(outName);
    if (subIdx < 0) return;
    auto& firstSubAttr = firstSubAttrSet->GetAttribute(subIdx);
    const int vdim = firstSubAttr.pointer ? firstSubAttr.pointer->GetDimension() : 1;

    DoubleArray::Pointer placeholder = DoubleArray::New();
    placeholder->SetDimension(vdim);
    placeholder->SetName(outName);
    DoubleArray::Pointer range = DoubleArray::New();
    range->SetDimension(2);
    range->Resize(vdim + 1);

    parentAttrSet->AddScalar(firstSubAttr.attachmentType, placeholder, range);
    obj->ReCollectSubDataObjectDataRange();
    obj->UpdateSubDataObjectDataRange();
}

IGAME_NAMESPACE_END
