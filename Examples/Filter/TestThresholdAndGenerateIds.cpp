// Find the integration commit: git log --diff-filter=A --format="%h %s" -- Examples/Filter/TestThresholdAndGenerateIds.cpp
// Batch 3 acceptance regression: the destination lacked these filter examples.
// Run against the imported models and synthetic boundary cases below; failures must
// return nonzero. IGAME_EXAMPLE_NO_RENDER retains numerical checks without a GPU.
// Integration fix: feat: integrate third-batch standard filters. Source: dayuwan77/igamevis, fdafcbb.
#include <cstdlib>
/**
 * @file TestThresholdAndGenerateIds.cpp
 * @brief GenerateIds 与 Threshold 两个滤波器的自动测试示例。
 *
 * 测试数据全部来自仓库内 Examples/Models 目录(构建时由 iGameCopyExampleAssets
 * 拷贝到运行目录下的 ./Models/),示例代码写死相对路径,无需任何手动输入,
 * 直接运行即可自动完成全部测试项并输出结果。
 *
 * 覆盖内容:
 *   1. GenerateIds:点/单元 Id 生成,同名 Point/Cell 属性共存,64 位 Id 精度
 *   2. GenerateIds:混合单元类型(四边形 + 三角形)网格上的 Id 生成
 *   3. Threshold:点关联标量 + AllScalars,筛选后 Id 数组无损传递
 *   4. Threshold:单元关联标量筛选
 *   5. Threshold:边界模式(Closed / Open)语义对比
 */
#include "Threshold/iGameThresholdFilter.h"
#include "GenerateIds/iGameGenerateIdsFilter.h"
#include "iGameFileIO.h"
#include "iGameFlatArray.h"
#include "iGameUnstructuredMesh.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

namespace {

// ---------------- 测试模型(相对 Examples 运行目录,无需手动输入) ----------------
const char* kThresholdScalarModel = "./Models/ThresholdScalarField.vtk";
const char* kThresholdVolumeModel = "./Models/ThresholdVolumeData.vtk";
const char* kGenerateIdsModel = "./Models/GenerateIdsTestData.vtk";
const char* kGenerateIdsMixedModel = "./Models/GenerateIdsMixedCells.vtk";

// 2^53 + 1:double 无法精确表示的奇数(该区间 double 间距为 2),
// 用于验证 64 位 Id 不经过 double 丢失精度
const long long kLargeStartId = 9007199254740993LL;

int g_Failures = 0;

void Check(bool ok, const std::string& what) {
    std::cout << (ok ? "  [ok]   " : "  [FAIL] ") << what << "\n";
    if (!ok) { ++g_Failures; }
}

const char* AssociationName(IGenum attachmentType) {
    return attachmentType == IG_CELL ? "Cell" : "Point";
}

const char* DataTypeName(IGenum type) {
    switch (type) {
        case IG_UNSTRUCTURED_MESH: return "UnstructuredMesh";
        case IG_SURFACE_MESH: return "SurfaceMesh";
        case IG_VOLUME_MESH: return "VolumeMesh";
        case IG_POINT_SET: return "PointSet";
        default: return "Other";
    }
}

void PrintMeshSummary(const char* title, iGame::DataObject::Pointer obj) {
    std::cout << title << "\n";
    if (!obj) {
        std::cout << "  (null)\n";
        return;
    }

    auto mesh = iGame::UnstructuredMesh::TransDataObjToUnstructuredMesh(obj);
    std::cout << "  type: " << DataTypeName(obj->GetDataObjectType()) << "\n";
    if (mesh) {
        std::cout << "  points: " << mesh->GetNumberOfPoints() << "\n";
        std::cout << "  cells: " << mesh->GetNumberOfCells() << "\n";
    } else if (obj->GetPoints()) {
        std::cout << "  points: " << obj->GetPoints()->GetNumberOfPoints() << "\n";
    }

    auto attrs = obj->GetAttributeSet();
    if (!attrs) {
        std::cout << "  attributes: none\n";
        return;
    }

    std::cout << "  attributes: " << attrs->GetNumberOfAttributes() << "\n";
    for (IGsize i = 0; i < attrs->GetNumberOfAttributes(); ++i) {
        auto& attr = attrs->GetAttribute(i);
        if (attr.isDeleted || !attr.pointer) continue;
        std::cout << "    [" << i << "] " << attr.pointer->GetName()
                  << " type=" << (attr.type == IG_VECTOR ? "vector" : "scalar")
                  << " attach=" << AssociationName(attr.attachmentType)
                  << " dim=" << attr.pointer->GetDimension()
                  << " count=" << attr.pointer->GetNumberOfElements() << "\n";
    }
}

// 按名称与(可选的)挂载类型查找属性,attachmentType == IG_NONE 表示不限挂载类型
iGame::AttributeSet::Attribute* FindAttribute(iGame::AttributeSet* attrs, const std::string& name,
                                              IGenum attachmentType = IG_NONE) {
    if (!attrs) return nullptr;
    for (IGsize i = 0; i < attrs->GetNumberOfAttributes(); ++i) {
        auto& attr = attrs->GetAttribute(i);
        if (attr.isDeleted || !attr.pointer) continue;
        if (attr.pointer->GetName() != name) continue;
        if (attachmentType != IG_NONE && attr.attachmentType != attachmentType) continue;
        return &attr;
    }
    return nullptr;
}

int CountAttributes(iGame::AttributeSet* attrs, const std::string& name, IGenum attachmentType) {
    if (!attrs) return 0;
    int found = 0;
    for (IGsize i = 0; i < attrs->GetNumberOfAttributes(); ++i) {
        auto& attr = attrs->GetAttribute(i);
        if (attr.isDeleted || !attr.pointer) continue;
        if (attr.pointer->GetName() == name && attr.attachmentType == attachmentType) ++found;
    }
    return found;
}

bool ComputeScalarRange(iGame::ArrayObject::Pointer array, int dimension, double& minValue,
                        double& maxValue) {
    if (!array || dimension < 0 || dimension >= array->GetDimension()) return false;
    const IGsize count = array->GetNumberOfElements();
    if (count == 0) return false;

    minValue = 1e300;
    maxValue = -1e300;
    IGsize finiteCount = 0;
    for (IGsize i = 0; i < count; ++i) {
        const double value = array->GetElementValue(i, dimension);
        if (!std::isfinite(value)) continue;
        minValue = std::min(minValue, value);
        maxValue = std::max(maxValue, value);
        ++finiteCount;
    }
    return finiteCount > 0;
}

iGame::DataObject::Pointer ReadModel(const char* path) {
    std::cout << "\n[Read] " << path << "\n";
    auto object = iGame::FileIO::ReadFile(path);
    if (!object) {
        std::cerr << "[FAIL] 读取测试模型失败: " << path << "\n";
        ++g_Failures;
    }
    return object;
}

iGame::DataObject::Pointer RunGenerateIds(iGame::DataObject::Pointer input, IGenum dataType,
                                          const std::string& arrayName, long long startId = 0) {
    auto filter = iGame::iGameGenerateIdsFilter::New(dataType);
    filter->SetInput(input);
    filter->SetArrayName(arrayName);
    filter->SetStartId(startId);
    if (!filter->Execute()) {
        std::cerr << "[FAIL] iGameGenerateIdsFilter::Execute() 失败: " << arrayName << "\n";
        ++g_Failures;
        return nullptr;
    }
    auto output = filter->GetOutput();
    if (!output) {
        std::cerr << "[FAIL] iGameGenerateIdsFilter 输出为空: " << arrayName << "\n";
        ++g_Failures;
    }
    return output;
}

// 校验 Id 以 LongLongArray 保存,且超过 2^53 的取值精确无损
void CheckIdPrecision(iGame::AttributeSet* attrs, const std::string& name, long long startId) {
    auto attr = FindAttribute(attrs, name, IG_POINT);
    if (!attr) {
        Check(false, "找到点关联 Id 数组 " + name);
        return;
    }

    Check(attr->pointer->GetArrayType() == IG_LongLongArray, name + " 使用 LongLongArray 存储");

    auto typed = iGame::DynamicCast<iGame::LongLongArray>(attr->pointer);
    if (!typed) {
        Check(false, name + " 可转换为 LongLongArray");
        return;
    }

    const long long first = typed->ValueAt(0);
    const long long second = typed->ValueAt(1);
    Check(first == startId, name + " 首元素精确等于起始编号(" + std::to_string(startId) + ")");
    Check(second == startId + 1,
          name + " 次元素精确等于起始编号 + 1(" + std::to_string(startId + 1) + ")");
    Check(static_cast<long long>(static_cast<double>(first)) != first,
          name + " 若经 double 中转会丢失精度(证明类型化直写必要)");
}

// 校验 Id 数组为 start..start+count-1 的连续序列(与 ParaView GenerateIds 语义一致)
bool CheckSequentialIds(iGame::AttributeSet* attrs, const std::string& name, IGenum attachmentType,
                        IGsize expectedCount, long long start = 0) {
    auto attr = FindAttribute(attrs, name, attachmentType);
    if (!attr) {
        Check(false, "找到 " + name + "(" + AssociationName(attachmentType) + " 关联)");
        return false;
    }
    if (attr->pointer->GetNumberOfElements() != expectedCount) {
        Check(false, name + " 元素数等于 " + std::to_string(expectedCount) + "(实际 " +
                             std::to_string(attr->pointer->GetNumberOfElements()) + ")");
        return false;
    }

    auto typed = iGame::DynamicCast<iGame::LongLongArray>(attr->pointer);
    if (!typed) {
        Check(false, name + " 可转换为 LongLongArray");
        return false;
    }
    for (IGsize i = 0; i < expectedCount; ++i) {
        if (typed->ValueAt(i) != start + static_cast<long long>(i)) {
            Check(false, name + " 第 " + std::to_string(i) + " 个 Id 应为 " +
                                 std::to_string(start + static_cast<long long>(i)) + "(实际 " +
                                 std::to_string(typed->ValueAt(i)) + ")");
            return false;
        }
    }
    Check(true, name + " 为 0.." + std::to_string(expectedCount == 0 ? 0 : expectedCount - 1) +
                        " 的连续序列,按源数据顺序编号");
    return true;
}

// 校验两张网格的单元连接关系逐一致(证明输出未打乱单元顺序)
bool CheckSameCellOrder(iGame::UnstructuredMesh::Pointer a, iGame::UnstructuredMesh::Pointer b) {
    if (!a || !b) return false;
    if (a->GetNumberOfCells() != b->GetNumberOfCells()) return false;
    if (a->GetNumberOfPoints() != b->GetNumberOfPoints()) return false;

    igIndex idsA[IGAME_CELL_MAX_SIZE]{};
    igIndex idsB[IGAME_CELL_MAX_SIZE]{};
    for (IGsize c = 0; c < a->GetNumberOfCells(); ++c) {
        const int nA = a->GetCells()->GetCellIds(c, idsA);
        const int nB = b->GetCells()->GetCellIds(c, idsB);
        if (nA != nB) return false;
        for (int i = 0; i < nA; ++i) {
            if (idsA[i] != idsB[i]) return false;
        }
    }
    return true;
}

// 在一个模型上执行 GenerateIds + Threshold,并校验 Id 数组无损传递
void RunThresholdCase(const char* title, const char* modelPath, const std::string& scalarName,
                      iGame::ThresholdFilter::Association association) {
    std::cout << "\n=== " << title << " ===\n";
    auto input = ReadModel(modelPath);
    if (!input) return;
    PrintMeshSummary("[输入模型]", input);

    auto withIds = RunGenerateIds(input, IG_POINT, "PointIds");
    if (!withIds) return;
    withIds = RunGenerateIds(withIds, IG_CELL, "CellIds");
    if (!withIds) return;
    PrintMeshSummary("[GenerateIds 之后]", withIds);

    const IGenum attachment =
            association == iGame::ThresholdFilter::Association::Cell ? IG_CELL : IG_POINT;
    auto attrs = withIds->GetAttributeSet();
    auto scalar = FindAttribute(attrs, scalarName, attachment);
    if (!scalar) {
        Check(false, "找到标量数组 " + scalarName + "(" + AssociationName(attachment) + " 关联)");
        return;
    }

    double minValue = 0.0;
    double maxValue = 0.0;
    if (!ComputeScalarRange(scalar->pointer, 0, minValue, maxValue)) {
        Check(false, "计算标量 " + scalarName + " 的取值范围");
        return;
    }
    const double span = maxValue - minValue;
    const double lower = minValue + 0.2 * span;
    const double upper = minValue + 0.8 * span;
    std::cout << "  标量 " << scalarName << " 范围=[" << minValue << ", " << maxValue
              << "],阈值区间=[" << lower << ", " << upper << "]\n";

    auto inputMesh = iGame::UnstructuredMesh::TransDataObjToUnstructuredMesh(withIds);
    const IGsize inputCellCount = inputMesh ? inputMesh->GetNumberOfCells() : 0;

    auto threshold = iGame::ThresholdFilter::New();
    threshold->SetInput(withIds);
    threshold->SetScalarData(scalar->pointer, association, 0);
    threshold->SetThreshold(lower, upper);
    threshold->SetBoundaryMode(iGame::ThresholdFilter::BoundaryMode::Closed);
    threshold->SetPointEvaluation(iGame::ThresholdFilter::PointEvaluation::AllScalars);
    if (!threshold->Execute()) {
        Check(false, "ThresholdFilter::Execute()");
        return;
    }

    auto output = threshold->GetOutput();
    PrintMeshSummary("[Threshold 之后]", output);
    if (!output) {
        Check(false, "ThresholdFilter 输出非空");
        return;
    }

    auto outMesh = iGame::UnstructuredMesh::TransDataObjToUnstructuredMesh(output);
    Check(outMesh && outMesh->GetNumberOfCells() > 0, "筛选结果包含单元");
    Check(outMesh && outMesh->GetNumberOfCells() < inputCellCount,
          "筛选结果单元数少于输入(阈值区间有效收紧)");

    auto outPointIds = FindAttribute(output->GetAttributeSet(), "PointIds", IG_POINT);
    auto outCellIds = FindAttribute(output->GetAttributeSet(), "CellIds", IG_CELL);
    Check(outPointIds != nullptr, "点关联 Id 数组穿过滤波器");
    Check(outCellIds != nullptr, "单元关联 Id 数组穿过滤波器");
    if (outMesh && outPointIds && outCellIds) {
        Check(outPointIds->pointer->GetNumberOfElements() == outMesh->GetNumberOfPoints(),
              "点 Id 数量与新网格点数一致");
        Check(outCellIds->pointer->GetNumberOfElements() == outMesh->GetNumberOfCells(),
              "单元 Id 数量与新网格单元数一致");
    }
}

// 校验边界模式:开区间保留单元数不多于闭区间
void CheckBoundaryModes(const char* modelPath, const std::string& scalarName) {
    std::cout << "\n=== Threshold 边界模式对比(Closed / Open) ===\n";
    auto input = ReadModel(modelPath);
    if (!input) return;

    auto attrs = input->GetAttributeSet();
    auto scalar = FindAttribute(attrs, scalarName, IG_POINT);
    if (!scalar) {
        Check(false, "找到标量数组 " + scalarName);
        return;
    }

    double minValue = 0.0;
    double maxValue = 0.0;
    if (!ComputeScalarRange(scalar->pointer, 0, minValue, maxValue)) {
        Check(false, "计算标量 " + scalarName + " 的取值范围");
        return;
    }
    const double span = maxValue - minValue;
    const double lower = minValue + 0.2 * span;
    const double upper = minValue + 0.8 * span;

    IGsize cellCounts[2] = {0, 0};
    const iGame::ThresholdFilter::BoundaryMode modes[2] = {
            iGame::ThresholdFilter::BoundaryMode::Closed,
            iGame::ThresholdFilter::BoundaryMode::Open};
    for (int m = 0; m < 2; ++m) {
        auto threshold = iGame::ThresholdFilter::New();
        threshold->SetInput(input);
        threshold->SetScalarData(scalar->pointer, iGame::ThresholdFilter::Association::Point, 0);
        threshold->SetThreshold(lower, upper);
        threshold->SetBoundaryMode(modes[m]);
        threshold->SetPointEvaluation(iGame::ThresholdFilter::PointEvaluation::AnyScalar);
        if (!threshold->Execute()) {
            Check(false, "边界模式 Execute()");
            return;
        }
        auto mesh = iGame::UnstructuredMesh::TransDataObjToUnstructuredMesh(threshold->GetOutput());
        cellCounts[m] = mesh ? mesh->GetNumberOfCells() : 0;
    }

    std::cout << "  Closed 保留单元: " << cellCounts[0] << ", Open 保留单元: " << cellCounts[1]
              << "\n";
    Check(cellCounts[0] >= cellCounts[1], "闭区间保留单元数不少于开区间");
}

} // namespace

int main() {
    std::cout << "=== iGameVis 滤波器自动测试:GenerateIds / Threshold ===\n";
    std::cout << "测试模型:Examples/Models 内置数据,直接运行即可,无需手动输入\n";

    // 1) GenerateIds:标准四边形网格
    std::cout << "\n=== GenerateIds 基础用例(GenerateIdsTestData) ===\n";
    auto base = ReadModel(kGenerateIdsModel);
    if (base) {
        PrintMeshSummary("[输入模型]", base);

        const IGsize baseAttrCount =
                base->GetAttributeSet() ? base->GetAttributeSet()->GetNumberOfAttributes() : 0;
        auto baseMesh = iGame::UnstructuredMesh::TransDataObjToUnstructuredMesh(base);
        const IGsize basePointCount = baseMesh ? baseMesh->GetNumberOfPoints() : 0;
        const IGsize baseCellCount = baseMesh ? baseMesh->GetNumberOfCells() : 0;

        auto withPointIds = RunGenerateIds(base, IG_POINT, "PointIds");
        if (withPointIds) {
            PrintMeshSummary("[GenerateIds 点关联]", withPointIds);
            Check(withPointIds.GetPointer() != base.GetPointer(),
                  "点关联输出是独立的数据对象(不是原对象)");
        }

        auto withCellIds = RunGenerateIds(withPointIds, IG_CELL, "CellIds");
        if (withCellIds) {
            PrintMeshSummary("[GenerateIds 单元关联]", withCellIds);
            auto attrs = withCellIds->GetAttributeSet();
            Check(FindAttribute(attrs, "PointIds", IG_POINT) != nullptr, "存在点关联 PointIds");
            Check(FindAttribute(attrs, "CellIds", IG_CELL) != nullptr, "存在单元关联 CellIds");
        }

        // 原模型必须保持不变(不再被 GenerateIds 直接修改)
        const IGsize baseAttrCountAfter =
                base->GetAttributeSet() ? base->GetAttributeSet()->GetNumberOfAttributes() : 0;
        Check(baseAttrCountAfter == baseAttrCount,
              "原模型属性数量保持 " + std::to_string(baseAttrCount) + " 不变(输入未被修改)");

        // Id 与源数据顺序一一对应(与 ParaView GenerateIds 语义一致)
        if (withCellIds) {
            auto attrs = withCellIds->GetAttributeSet();
            CheckSequentialIds(attrs, "PointIds", IG_POINT, basePointCount);
            CheckSequentialIds(attrs, "CellIds", IG_CELL, baseCellCount);

            auto outMesh = iGame::UnstructuredMesh::TransDataObjToUnstructuredMesh(withCellIds);
            Check(CheckSameCellOrder(baseMesh, outMesh),
                  "输出网格与输入的单元顺序、连接关系完全一致");
        }

        // 同名 Point/Cell 属性:两个 "Ids" 应共存且各仅一份,重复执行不新增
        auto sameName = RunGenerateIds(withCellIds, IG_POINT, "Ids");
        sameName = RunGenerateIds(sameName, IG_CELL, "Ids");
        sameName = RunGenerateIds(sameName, IG_POINT, "Ids");
        if (sameName) {
            auto attrs = sameName->GetAttributeSet();
            Check(CountAttributes(attrs, "Ids", IG_POINT) == 1, "同名点属性 Ids 仅一份");
            Check(CountAttributes(attrs, "Ids", IG_CELL) == 1, "同名单元属性 Ids 仅一份");
        }

        // 64 位 Id 精度(起始编号 > 2^53)
        auto bigIds = RunGenerateIds(sameName, IG_POINT, "BigIds", kLargeStartId);
        if (bigIds) {
            PrintMeshSummary("[大整数 Id]", bigIds);
            CheckIdPrecision(bigIds->GetAttributeSet(), "BigIds", kLargeStartId);
        }
    }

    // 2) GenerateIds:混合单元类型
    std::cout << "\n=== GenerateIds 混合单元类型(GenerateIdsMixedCells) ===\n";
    auto mixed = ReadModel(kGenerateIdsMixedModel);
    if (mixed) {
        PrintMeshSummary("[输入模型]", mixed);
        auto mixedIds = RunGenerateIds(mixed, IG_POINT, "PointIds");
        mixedIds = RunGenerateIds(mixedIds, IG_CELL, "CellIds");
        if (mixedIds) {
            PrintMeshSummary("[GenerateIds 之后]", mixedIds);
            auto attrs = mixedIds->GetAttributeSet();
            Check(FindAttribute(attrs, "PointIds", IG_POINT) != nullptr, "混合单元网格生成点 Id");
            Check(FindAttribute(attrs, "CellIds", IG_CELL) != nullptr, "混合单元网格生成单元 Id");
        }
    }

    // 3) Threshold:点关联 + AllScalars(平滑标量场)
    RunThresholdCase("[3] Threshold 点关联(ThresholdScalarField / Pressure)", kThresholdScalarModel,
                     "Pressure", iGame::ThresholdFilter::Association::Point);

    // 4) Threshold:单元关联(体网格 + 单元标量)
    RunThresholdCase("[4] Threshold 单元关联(ThresholdVolumeData / CellQuality)",
                     kThresholdVolumeModel, "CellQuality",
                     iGame::ThresholdFilter::Association::Cell);

    // 5) Threshold:边界模式语义
    CheckBoundaryModes(kThresholdScalarModel, "Pressure");

    std::cout << "\n";
    if (g_Failures == 0) {
        std::cout << "[PASS] GenerateIds + Threshold 自动测试全部通过。\n";
        return 0;
    }
    std::cout << "[FAIL] 自动测试存在 " << g_Failures << " 项失败。\n";
    return 1;
}
