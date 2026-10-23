#include "iGameRandomAttributesFilter.h"
#include <ctime>

IGAME_NAMESPACE_BEGIN

RandomAttributesFilter::RandomAttributesFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}
RandomAttributesFilter::~RandomAttributesFilter() = default;

// ---- 改进点3：逐项输入校验 ----
bool RandomAttributesFilter::ValidateInputs(std::string& why) const {
    // min/max 校验
    if (_min > _max) {
        why = "校值范围错误：最小值 (" + std::to_string(_min) + ") 大于最大值 (" + std::to_string(_max) + ")";  // 最小值大于最大值
        return false;
    }
    // 数据类型校验
    switch (_dataType) {
    case IG_CHAR: case IG_UNSIGNED_CHAR:
    case IG_SHORT: case IG_UNSIGNED_SHORT:
    case IG_INT: case IG_UNSIGNED_INT:
    case IG_LONG_LONG: case IG_UNSIGNED_LONG_LONG:
    case IG_FLOAT: case IG_DOUBLE:
        break;
    case IG_INDEX:
        break;
    default:
        why = "不支持的数据类型";  // 不支持的数据类型
        return false;
    }
    // 挂载类型校验
    if (_attach != IG_POINT && _attach != IG_CELL) {
        why = "挂载类型错误：必须为 IG_POINT 或 IG_CELL";  // 挂载类型必须为 IG_POINT 或 IG_CELL
        return false;
    }
    return true;
}

// ---- 改进点1+2：创建独立输出副本 ----
DataObject::Pointer RandomAttributesFilter::CreateIndependentOutput(DataObject::Pointer input) {
    if (!input) return nullptr;
    auto out = DataObject::CreateDataObject(input->GetDataObjectType());
    if (!out) return nullptr;

    auto copyPS = [](PointSet* dst, PointSet* src) {
        auto pts = Points::New();
        if (src->GetPoints()) pts->DeepCopy(src->GetPoints());
        dst->SetPoints(pts);
        auto as = AttributeSet::New();
        if (auto sas = src->GetAttributeSet()) as->DeepCopy(sas);
        dst->SetAttributeSet(as);
    };
    auto copyFaces = [](SurfaceMesh* dst, SurfaceMesh* src) {
        if (auto f = src->GetFaces()) {
            auto nf = CellArray::New(); nf->DeepCopy(f); dst->SetFaces(nf);
        }
    };
    auto copyVolumes = [](VolumeMesh* dst, VolumeMesh* src) {
        // 改进点2：保留体单元
        if (auto v = src->GetVolumes()) {
            auto nv = CellArray::New(); nv->DeepCopy(v); dst->SetVolumes(nv);
        }
    };

    // 注意：类型匹配必须从「最具体」到「最一般」。
    // 继承关系为 StructuredMesh : VolumeMesh : SurfaceMesh : PointSet，
    // 若先匹配 SurfaceMesh，体网格/结构网格会提前进入表面复制分支，
    // 导致体单元 (m_Volumes) 与结构尺寸 (size/extent) 丢失。

    if (auto stm = DynamicCast<StructuredMesh>(input)) {
        if (auto dst = DynamicCast<StructuredMesh>(out)) {
            copyPS(dst, stm);
            copyFaces(dst, stm);
            copyVolumes(dst, stm);
            // 改进点2：保留结构尺寸与维度范围
            if (auto* srcSize = stm->GetDimensionSize()) {
                igIndex s[3] = { srcSize[0], srcSize[1], srcSize[2] };
                dst->SetDimensionSize(s);
            }
            if (auto* srcExt = stm->GetExtent()) {
                igIndex e[6] = { srcExt[0], srcExt[1], srcExt[2],
                                 srcExt[3], srcExt[4], srcExt[5] };
                dst->SetExtent(e);
            }
            return out;
        }
    }
    if (auto vm = DynamicCast<VolumeMesh>(input)) {
        if (auto dst = DynamicCast<VolumeMesh>(out)) {
            copyPS(dst, vm);
            copyFaces(dst, vm);
            copyVolumes(dst, vm);  // 改进点2：保留体单元
            return out;
        }
    }
    if (auto sm = DynamicCast<SurfaceMesh>(input)) {
        if (auto dst = DynamicCast<SurfaceMesh>(out)) {
            if (dst->DeepCopy(sm)) return out;
            copyPS(dst, sm);
            copyFaces(dst, sm);
            return out;
        }
    }
    if (auto um = DynamicCast<UnstructuredMesh>(input)) {
        if (auto dst = DynamicCast<UnstructuredMesh>(out)) {
            copyPS(dst, um);
            if (auto c = um->GetCellArray()) {
                auto nc = CellArray::New(); nc->DeepCopy(c);
                dst->SetCells(nc, um->GetCellTypes());
            }
            return out;
        }
    }
    if (auto ps = DynamicCast<PointSet>(input)) {
        if (auto dst = DynamicCast<PointSet>(out)) { copyPS(dst, ps); return out; }
    }
    return out;
}

ArrayObject::Pointer RandomAttributesFilter::CreateDataArray(IGsize num, double lo, double hi, unsigned int seed) {
    switch (_dataType) {
    case IG_CHAR:                    return CreateTypedArray<CharArray, char>(num, lo, hi, seed);
    case IG_UNSIGNED_CHAR:           return CreateTypedArray<UnsignedCharArray, unsigned char>(num, lo, hi, seed);
    case IG_SHORT:                   return CreateTypedArray<ShortArray, short>(num, lo, hi, seed);
    case IG_UNSIGNED_SHORT:          return CreateTypedArray<UnsignedShortArray, unsigned short>(num, lo, hi, seed);
    case IG_INT:
    case IG_INDEX:                   return CreateTypedArray<IntArray, int>(num, lo, hi, seed);
    case IG_UNSIGNED_INT:            return CreateTypedArray<UnsignedIntArray, unsigned int>(num, lo, hi, seed);
    case IG_LONG_LONG:               return CreateTypedArray<LongLongArray, long long>(num, lo, hi, seed);
    case IG_UNSIGNED_LONG_LONG:      return CreateTypedArray<UnsignedLongLongArray, unsigned long long>(num, lo, hi, seed);
    case IG_FLOAT:                   return CreateTypedArray<FloatArray, float>(num, lo, hi, seed);
    case IG_DOUBLE:                  return CreateTypedArray<DoubleArray, double>(num, lo, hi, seed);
    default:
        _msg = "不支持的数据类型。";
        return nullptr;
    }
}

// ---- 改进点4：修正二维结构网格的 Cell 计数 ----
bool RandomAttributesFilter::ComputeCount(DataObject::Pointer input, IGsize& out, std::string& why) const {
    if (_attach == IG_POINT) {
        auto ps = DynamicCast<PointSet>(input);
        if (!ps) { why = "输入不是 PointSet 派生类型，不支持 IG_POINT 挂载。"; return false; }
        out = ps->GetNumberOfPoints();
        return true;
    }
    // IG_CELL 分支：先精确匹配最具体的类型
    if (auto stm = DynamicCast<StructuredMesh>(input)) {
        // 改进点4：结构化网格的单元数是隐式的，由结构尺寸决定，
        // 不能依赖 m_Volumes/m_Faces 是否已构建。
        // 二维(size[2]<=1)取 (nx-1)*(ny-1) 个面单元；三维取 (nx-1)*(ny-1)*(nz-1) 个体单元。
        igIndex s[3] = { 0, 0, 0 };
        if (auto* srcSize = stm->GetDimensionSize()) {
            s[0] = srcSize[0]; s[1] = srcSize[1]; s[2] = srcSize[2];
        }
        if (s[0] >= 2 && s[1] >= 2) {
            IGsize nCells = static_cast<IGsize>(s[0] - 1);
            nCells *= static_cast<IGsize>(s[1] - 1);
            if (s[2] >= 2) nCells *= static_cast<IGsize>(s[2] - 1);
            out = nCells;
        } else {
            // 尺寸信息缺失时回退到显式单元计数
            IGsize nVol = stm->GetNumberOfVolumes();
            out = (nVol > 0) ? nVol : stm->GetNumberOfFaces();
        }
        return true;
    }
    if (auto vm = DynamicCast<VolumeMesh>(input)) {
        out = vm->GetNumberOfVolumes();
        return true;
    }
    if (auto sm = DynamicCast<SurfaceMesh>(input)) {
        out = sm->GetNumberOfFaces();
        return true;
    }
    if (auto um = DynamicCast<UnstructuredMesh>(input)) {
        out = um->GetNumberOfCells();
        return true;
    }
    why = "输入类型不支持 IG_CELL 挂载。";
    return false;
}

// ---- 改进点5：处理同名数组 ----
bool RandomAttributesFilter::ApplyArrayToOutput(DataObject::Pointer output,
    ArrayObject::Pointer data, const std::string& name, std::string& why)
{
    auto attrSet = output->GetAttributeSet();
    if (!attrSet) { why = "输出没有属性集。"; return false; }

    data->SetName(name);

    if (_conflictMode == NameConflictMode::Append) {
        attrSet->AddScalar(_attach, data);
        return true;
    }

    // 查找同名数组
    int existIdx = -1;
    auto N = attrSet->GetNumberOfAttributes();
    for (int i = 0; i < (int)N; ++i) {
        auto& ref = attrSet->GetAttribute(i);
        if (ref.isDeleted || !ref.pointer) continue;
        if (ref.attachmentType != _attach) continue;
        if (ref.pointer->GetName() == name) { existIdx = i; break; }
    }

    if (existIdx < 0) {
        // 没有同名数组，直接添加
        attrSet->AddScalar(_attach, data);
        return true;
    }

    if (_conflictMode == NameConflictMode::Replace) {
        // 替换已有同名数组
        auto& ref = attrSet->GetAttribute(existIdx);
        ref.pointer = data;
        ref.type = IG_SCALAR;
        ref.attachmentType = _attach;
        return true;
    }

    if (_conflictMode == NameConflictMode::AutoRename) {
        // 自动改名：name_1, name_2, ...
        int suffix = 1;
        std::string newName;
        bool conflict;
        do {
            newName = name + "_" + std::to_string(suffix++);
            conflict = false;
            for (int i = 0; i < (int)N; ++i) {
                auto& ref = attrSet->GetAttribute(i);
                if (ref.isDeleted || !ref.pointer) continue;
                if (ref.attachmentType != _attach) continue;
                if (ref.pointer->GetName() == newName) { conflict = true; break; }
            }
        } while (conflict);
        data->SetName(newName);
        attrSet->AddScalar(_attach, data);
        return true;
    }

    attrSet->AddScalar(_attach, data);
    return true;
}

bool RandomAttributesFilter::Execute() {
    // 改进点3：逐项输入校验
    std::string why;
    if (!ValidateInputs(why)) {
        _msg = why;
        return false;
    }

    auto input = GetInput(0);
    if (!input) { _msg = "输入为空。"; return false; }

    // 改进点1：创建独立输出副本，确保原模型属性不变
    auto output = CreateIndependentOutput(input);
    if (!output) { _msg = "创建独立输出副本失败。"; return false; }

    auto attrSet = output->GetAttributeSet();
    if (!attrSet) { _msg = "输出没有属性集。"; return false; }

    // 计算要生成几个随机数（改进点4：修正二维结构网格）
    IGsize num = 0;
    if (!ComputeCount(input, num, why)) {
        _msg = why;
        return false;
    }
    if (num == 0) {
        _msg = "挂载目标为零（没有点或单元）。";
        return false;
    }

    const double lo = (_max < _min) ? _max : _min;
    const double hi = (_max < _min) ? _min : _max;

    auto data = CreateDataArray(num, lo, hi, _seed);
    if (!data) return false;

    // 确定属性名
    std::string attrName;
    if (!_name.empty()) {
        attrName = _name;
    } else {
        attrName = (_attach == IG_POINT) ? "RandomPointScalars" : "RandomCellScalars";
    }

    // 改进点5：处理同名数组（替换/改名/追加）
    if (!ApplyArrayToOutput(output, data, attrName, why)) {
        _msg = why;
        return false;
    }

    SetOutput(output);
    return true;
}

IGAME_NAMESPACE_END
