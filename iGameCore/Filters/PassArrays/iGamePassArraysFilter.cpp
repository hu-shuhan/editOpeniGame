#include "iGamePassArraysFilter.h"

#include "iGameArrayObject.h"
#include "iGameAttributeSet.h"
#include "iGameCellArray.h"

#include "iGamePoints.h"
#include "iGameStructuredMesh.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"

#include <algorithm>

IGAME_NAMESPACE_BEGIN

void PassArrays::SetPointArrayNames(const std::vector<std::string>& names) {
    if (m_PointArrayNames == names) { return; }
    m_PointArrayNames = names;
    this->Modified();
}

void PassArrays::AddPointArrayName(const std::string& name) {
    m_PointArrayNames.push_back(name);
    this->Modified();
}

void PassArrays::SetCellArrayNames(const std::vector<std::string>& names) {
    if (m_CellArrayNames == names) { return; }
    m_CellArrayNames = names;
    this->Modified();
}

void PassArrays::AddCellArrayName(const std::string& name) {
    m_CellArrayNames.push_back(name);
    this->Modified();
}

void PassArrays::SetArrayNames(const std::vector<std::string>& pointNames,
                               const std::vector<std::string>& cellNames) {
    if (m_PointArrayNames == pointNames && m_CellArrayNames == cellNames) { return; }
    m_PointArrayNames = pointNames;
    m_CellArrayNames = cellNames;
    this->Modified();
}

void PassArrays::ClearArrayNames() {
    if (m_PointArrayNames.empty() && m_CellArrayNames.empty()) { return; }
    m_PointArrayNames.clear();
    m_CellArrayNames.clear();
    this->Modified();
}

namespace {

bool ContainsName(const std::vector<std::string>& names, const std::string& name) {
    return std::find(names.begin(), names.end(), name) != names.end();
}

// 按 attachmentType 只在本侧（点数据 / 单元数据）查找并保留选中的属性。
// 点数据与单元数据的名字各自匹配，因此同名属性（例如点、单元上都叫 test）可以只保留一项。
// 命中的数组与数据范围都按引用共享给输出的属性集，不做深拷贝。
void ShareSelectedAttributes(AttributeSet* inAttrSet, const std::vector<std::string>& names,
                            IGenum attachmentType, const AttributeSet::Pointer& outAttrSet) {
    if (inAttrSet == nullptr || names.empty() || outAttrSet == nullptr) { return; }

    auto attrs = (attachmentType == IG_POINT) ? inAttrSet->GetAllPointAttributes()
                                              : inAttrSet->GetAllCellAttributes();
    if (attrs == nullptr) { return; }

    for (IGsize i = 0; i < attrs->GetNumberOfElements(); ++i) {
        auto& attr = attrs->GetElement(i);
        if (attr.IsNone() || attr.pointer == nullptr) { continue; }
        if (!ContainsName(names, attr.pointer->GetName())) { continue; }
        outAttrSet->AddAttribute(attr.type, attachmentType, attr.pointer, attr.dataRange);
    }
}

} // namespace

bool PassArrays::Execute() {
    DataObject::Pointer input = GetInput(0);
    if (!input) {
        igError("iGamePassArrays needs a valid input data object.");
        return false;
    }

    const IGenum type = input->GetDataObjectType();
    DataObject::Pointer output = DataObject::CreateDataObject(type);
    if (!output) {
        igError("iGamePassArrays does not support this data type ({}).", type);
        return false;
    }

    // 几何与拓扑直接共享输入对象的数组（点坐标、单元连接关系都不复制）。
    if (type == IG_UNSTRUCTURED_MESH) {
        auto inMesh = DynamicCast<UnstructuredMesh>(input);
        auto outMesh = DynamicCast<UnstructuredMesh>(output);
        if (!inMesh || !outMesh) { return false; }

        outMesh->SetPoints(inMesh->GetPoints());
        outMesh->SetCells(inMesh->GetCells(), inMesh->GetCellTypes());
    } else if (type == IG_SURFACE_MESH) {
        auto inMesh = DynamicCast<SurfaceMesh>(input);
        auto outMesh = DynamicCast<SurfaceMesh>(output);
        if (!inMesh || !outMesh) { return false; }

        outMesh->SetPoints(inMesh->GetPoints());
        outMesh->SetFaces(inMesh->GetFaces());
    } else if (type == IG_VOLUME_MESH) {
        auto inMesh = DynamicCast<VolumeMesh>(input);
        auto outMesh = DynamicCast<VolumeMesh>(output);
        if (!inMesh || !outMesh) { return false; }

        outMesh->SetPoints(inMesh->GetPoints());
        outMesh->SetVolumes(inMesh->GetVolumes());
    } else if (type == IG_POINT_SET) {
        auto inMesh = DynamicCast<PointSet>(input);
        auto outMesh = DynamicCast<PointSet>(output);
        if (!inMesh || !outMesh) { return false; }

        outMesh->SetPoints(inMesh->GetPoints());
        // PointSet 无单元，跳过
    } else if (type == IG_STRUCTURED_MESH) {
        auto inMesh = DynamicCast<StructuredMesh>(input);
        auto outMesh = DynamicCast<StructuredMesh>(output);
        if (!inMesh || !outMesh) { return false; }

        // 结构化网格的拓扑由维度隐含，输出共享同一套维度与点坐标
        igIndex* dims = inMesh->GetDimensionSize();
        if (dims) {
            igIndex newDims[3] = {dims[0], dims[1], dims[2]};
            outMesh->SetDimensionSize(newDims);
        }
        outMesh->SetPoints(inMesh->GetPoints());

        // 输入已经生成过结构化连接关系时直接共享，避免在输出上重复生成
        if (auto volumes = inMesh->GetVolumes()) {
            outMesh->SetVolumes(volumes);
        } else if (auto faces = inMesh->GetFaces()) {
            outMesh->SetFaces(faces);
        }
    } else {
        igError("iGamePassArrays does not support this data type ({}).", type);
        return false;
    }

    // 点数据与单元数据分别按名称匹配、分别保留，两者互不影响
    auto outAttrSet = AttributeSet::New();
    auto inAttrSet = input->GetAttributeSet();
    ShareSelectedAttributes(inAttrSet, m_PointArrayNames, IG_POINT, outAttrSet);
    ShareSelectedAttributes(inAttrSet, m_CellArrayNames, IG_CELL, outAttrSet);
    output->SetAttributeSet(outAttrSet);

    output->Modified();
    if (outAttrSet->GetNumberOfAttributes() > 0) { outAttrSet->ForceReConvertToDrawableData(); }

    SetOutput(0, output);
    return true;
}

IGAME_NAMESPACE_END
