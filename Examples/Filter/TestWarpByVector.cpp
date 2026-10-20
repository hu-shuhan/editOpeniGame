// 迁移回归（待提交）：标准菜单原先缺少该功能；保留源仓库数值、输入保留和异常边界检查，
// 防止仅接入菜单或漏迁依赖导致行为退化。交互演示保留原启动方式。
/**
 * @file TestWarpByVector.cpp
 * @brief iGameWarpByVectorFilter 的自动测试示例。
 *
 * 测试模型为仓库内 Examples/Models/Streamline_tornado_structuredmesh.vtk
 * (龙卷风漏斗网格 + 旋转上升速度场),构建时由 iGameCopyExampleAssets 拷贝到
 * 运行目录下的 ./Models/。示例代码写死相对路径,无需任何手动输入,直接运行
 * 即可自动完成全部测试项并输出结果。
 *
 * 覆盖内容:
 *   1. 基本形变:输出为独立对象,点数/单元数不变,属性集合完整拷贝
 *   2. 位移正确性:逐点满足 新坐标 = 原坐标 + scale × 向量
 *   3. 缩放系数线性关系:scale 缩放后位移按比例变化
 *   4. 自动缩放:最大位移与模型尺度同量级
 *   5. 位移上限:SetMaxDisplacementRatio 生效
 *   6. 错误处理:向量名不存在 / 单元关联向量 均应失败并返回 false
 */
// Windows 控制台默认使用 GBK 代码页,直接输出 UTF-8 文本会显示为乱码,
// 这里在包含其它头文件之前设置好代码页(并禁用 min/max 宏,避免影响标准库)。
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "WarpByVector/iGameWarpByVectorFilter.h"
#include "iGameFileIO.h"
#include "iGameUnstructuredMesh.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

namespace {

// 测试模型(相对 Examples 运行目录,无需手动输入)
const char* kWarpByVectorModel = "./Models/Streamline_tornado_structuredmesh.vtk";
const char* kWarpVectorName = "velocity";

int g_Failures = 0;

void Check(bool ok, const std::string& what) {
    std::cout << (ok ? "  [ok]   " : "  [FAIL] ") << what << "\n";
    if (!ok) { ++g_Failures; }
}

const char* AssociationName(IGenum attachmentType) {
    return attachmentType == IG_CELL ? "Cell" : "Point";
}

void PrintMeshSummary(const char* title, iGame::DataObject::Pointer obj) {
    std::cout << title << "\n";
    if (!obj) {
        std::cout << "  (null)\n";
        return;
    }

    auto mesh = iGame::UnstructuredMesh::TransDataObjToUnstructuredMesh(obj);
    if (mesh) {
        std::cout << "  points: " << mesh->GetNumberOfPoints()
                  << ", cells: " << mesh->GetNumberOfCells() << "\n";
    }

    auto attrs = obj->GetAttributeSet();
    if (!attrs) {
        std::cout << "  attributes: none\n";
        return;
    }
    std::cout << "  attributes: " << attrs->GetNumberOfAttributes() << "\n";
    for (IGsize i = 0; i < attrs->GetNumberOfAttributes(); ++i) {
        auto& attr = attrs->GetAttribute(i);
        if (attr.isDeleted || !attr.pointer) { continue; }
        std::cout << "    [" << i << "] " << attr.pointer->GetName()
                  << " type=" << (attr.type == IG_VECTOR ? "vector" : "scalar")
                  << " attach=" << AssociationName(attr.attachmentType)
                  << " dim=" << attr.pointer->GetDimension()
                  << " count=" << attr.pointer->GetNumberOfElements() << "\n";
    }
}

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

iGame::DataObject::Pointer ReadModel(const char* path) {
    std::cout << "\n[Read] " << path << "\n";
    auto object = iGame::FileIO::ReadFile(path);
    if (!object) {
        std::cerr << "[FAIL] 读取测试模型失败: " << path << "\n";
        ++g_Failures;
    }
    return object;
}

// 统计"输出点相对输入点的位移"与"scale × 向量"不一致的采样点数
int CountBadDisplacements(iGame::UnstructuredMesh::Pointer inputMesh,
                          iGame::UnstructuredMesh::Pointer outputMesh,
                          iGame::ArrayObject::Pointer vectorArray, double scale) {
    if (!inputMesh || !outputMesh || !vectorArray) { return -1; }

    int bad = 0;
    for (IGsize i = 0; i < inputMesh->GetNumberOfPoints(); i += 17) {
        const auto& p = inputMesh->GetPoints()->GetPoint(i);
        const auto& q = outputMesh->GetPoints()->GetPoint(i);
        for (int d = 0; d < 3; ++d) {
            const double expect = p[d] + scale * vectorArray->GetElementValue(i, d);
            if (std::fabs(q[d] - expect) > 1e-6) {
                ++bad;
                break;
            }
        }
    }
    return bad;
}

// 返回最大点位移(相对输入网格)
double MaxDisplacement(iGame::UnstructuredMesh::Pointer inputMesh,
                       iGame::UnstructuredMesh::Pointer outputMesh) {
    if (!inputMesh || !outputMesh) { return 0.0; }

    double maxDisplacement = 0.0;
    for (IGsize i = 0; i < inputMesh->GetNumberOfPoints(); ++i) {
        const auto& p = inputMesh->GetPoints()->GetPoint(i);
        const auto& q = outputMesh->GetPoints()->GetPoint(i);
        const double dx = q[0] - p[0];
        const double dy = q[1] - p[1];
        const double dz = q[2] - p[2];
        maxDisplacement = std::max(maxDisplacement, std::sqrt(dx * dx + dy * dy + dz * dz));
    }
    return maxDisplacement;
}

} // namespace

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    std::cout << "=== iGameVis 滤波器自动测试:WarpByVector ===\n";
    std::cout << "测试模型:Examples/Models 内置数据,直接运行即可,无需手动输入\n";

    // ---------------- 输入模型 ----------------
    auto input = ReadModel(kWarpByVectorModel);
    if (!input) {
        std::cout << "\n[FAIL] 自动测试存在 " << g_Failures << " 项失败。\n";
        return 1;
    }
    PrintMeshSummary("[输入模型]", input);

    auto inputMesh = iGame::UnstructuredMesh::TransDataObjToUnstructuredMesh(input);
    auto velocity = FindAttribute(input->GetAttributeSet(), kWarpVectorName, IG_POINT);
    Check(inputMesh != nullptr, "输入可转换为非结构化网格");
    Check(velocity != nullptr, std::string("存在点关联向量数组 ") + kWarpVectorName);
    if (!inputMesh || !velocity) {
        std::cout << "\n[FAIL] 自动测试存在 " << g_Failures << " 项失败。\n";
        return 1;
    }

    const IGsize pointCount = inputMesh->GetNumberOfPoints();
    const IGsize cellCount = inputMesh->GetNumberOfCells();
    const IGsize inputAttrCount = input->GetAttributeSet()
                                          ? input->GetAttributeSet()->GetNumberOfAttributes()
                                          : 0;
    const auto& before = inputMesh->GetPoints()->GetPoint(0);
    const double beforeX = before[0];
    const double beforeY = before[1];
    const double beforeZ = before[2];

    const double diagonal = [&]() {
        const auto& box = input->GetBoundingBox();
        const double dx = box.max[0] - box.min[0];
        const double dy = box.max[1] - box.min[1];
        const double dz = box.max[2] - box.min[2];
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }();
    std::cout << "  模型对角线=" << diagonal << ", 点数=" << pointCount << ", 单元数=" << cellCount
              << "\n";

    // ---------------- 1) 基本形变(scale = 0.5) ----------------
    std::cout << "\n=== 1) 基本形变(scale = 0.5) ===\n";
    const double scale = 0.5;
    auto warp = iGame::WarpByVectorFilter::New();
    warp->SetInput(input);
    warp->SetVectorArrayName(kWarpVectorName);
    warp->SetScaleFactor(scale);
    if (!warp->Execute()) {
        Check(false, "WarpByVectorFilter::Execute()");
    } else {
        auto output = warp->GetOutput();
        Check(output.GetPointer() != input.GetPointer(), "输出是独立的数据对象(不是原对象)");

        auto outputMesh = iGame::UnstructuredMesh::TransDataObjToUnstructuredMesh(output);
        Check(outputMesh && outputMesh->GetNumberOfPoints() == pointCount &&
                      outputMesh->GetNumberOfCells() == cellCount,
              "点数与单元数保持不变");
        Check(output->GetAttributeSet() &&
                      output->GetAttributeSet()->GetNumberOfAttributes() == inputAttrCount,
              "属性集合完整拷贝到输出(数量一致)");
        Check(FindAttribute(output->GetAttributeSet(), kWarpVectorName, IG_POINT) != nullptr,
              "输出仍包含向量数组 " + std::string(kWarpVectorName));
        Check(CountBadDisplacements(inputMesh, outputMesh, velocity->pointer, scale) == 0,
              "抽样点位移严格等于 scale × 向量");
        Check(std::fabs(warp->GetAppliedScale() - scale) < 1e-12,
              "GetAppliedScale() 等于设定的缩放系数");
    }

    // ---------------- 2) 缩放系数线性关系 ----------------
    std::cout << "\n=== 2) 缩放系数线性关系(0.25 相对 0.5) ===\n";
    auto warpSmall = iGame::WarpByVectorFilter::New();
    warpSmall->SetInput(input);
    warpSmall->SetVectorArrayName(kWarpVectorName);
    warpSmall->SetScaleFactor(0.25);
    if (!warpSmall->Execute()) {
        Check(false, "缩放 0.25 时 Execute()");
    } else {
        auto smallMesh =
                iGame::UnstructuredMesh::TransDataObjToUnstructuredMesh(warpSmall->GetOutput());
        auto warpBig = iGame::WarpByVectorFilter::New();
        warpBig->SetInput(input);
        warpBig->SetVectorArrayName(kWarpVectorName);
        warpBig->SetScaleFactor(0.5);
        if (!warpBig->Execute()) {
            Check(false, "缩放 0.5 时 Execute()");
        } else {
            auto bigMesh =
                    iGame::UnstructuredMesh::TransDataObjToUnstructuredMesh(warpBig->GetOutput());
            const double smallMax = MaxDisplacement(inputMesh, smallMesh);
            const double bigMax = MaxDisplacement(inputMesh, bigMesh);
            std::cout << "  最大位移:0.25→" << smallMax << ", 0.5→" << bigMax << "\n";
            Check(bigMax > 0.0 && std::fabs(smallMax / bigMax - 0.5) < 1e-6,
                  "位移与缩放系数成正比");
        }
    }

    // ---------------- 3) 自动缩放 ----------------
    std::cout << "\n=== 3) 自动缩放(按模型尺度归一化) ===\n";
    auto warpAuto = iGame::WarpByVectorFilter::New();
    warpAuto->SetInput(input);
    warpAuto->SetVectorArrayName(kWarpVectorName);
    warpAuto->SetAutoScale(true);
    if (!warpAuto->Execute()) {
        Check(false, "自动缩放时 Execute()");
    } else {
        auto autoMesh =
                iGame::UnstructuredMesh::TransDataObjToUnstructuredMesh(warpAuto->GetOutput());
        const double autoMax = MaxDisplacement(inputMesh, autoMesh);
        std::cout << "  实际缩放=" << warpAuto->GetAppliedScale() << ", 最大位移=" << autoMax
                  << " (对角线 " << diagonal << ")\n";
        Check(warpAuto->GetAppliedScale() > 0.0, "自动缩放系数为正");
        Check(autoMax <= diagonal * 1.0000001, "最大位移不超过模型对角线");
    }

    // ---------------- 4) 位移上限 ----------------
    std::cout << "\n=== 4) 单点位移上限(5% 对角线) ===\n";
    const double ratio = 0.05;
    auto warpClamp = iGame::WarpByVectorFilter::New();
    warpClamp->SetInput(input);
    warpClamp->SetVectorArrayName(kWarpVectorName);
    warpClamp->SetScaleFactor(1.0);
    warpClamp->SetMaxDisplacementRatio(ratio);
    if (!warpClamp->Execute()) {
        Check(false, "限制位移时 Execute()");
    } else {
        auto clampMesh =
                iGame::UnstructuredMesh::TransDataObjToUnstructuredMesh(warpClamp->GetOutput());
        const double clampMax = MaxDisplacement(inputMesh, clampMesh);
        std::cout << "  最大位移=" << clampMax << ", 上限=" << ratio * diagonal << "\n";
        // 点坐标以 float 存储,限幅后存在浮点舍入,这里给出 1e-4 的绝对容差
        Check(clampMax <= ratio * diagonal + 1e-4, "所有点位移均不超过设定上限");
    }

    // ---------------- 5) 错误处理 ----------------
    std::cout << "\n=== 5) 错误处理 ===\n";
    auto warpMissing = iGame::WarpByVectorFilter::New();
    warpMissing->SetInput(input);
    warpMissing->SetVectorArrayName("not_exist_vector");
    Check(!warpMissing->Execute(), "向量名不存在时 Execute() 返回 false");

    // 原模型必须保持不变
    const auto& after = inputMesh->GetPoints()->GetPoint(0);
    Check(std::fabs(after[0] - beforeX) < 1e-12 && std::fabs(after[1] - beforeY) < 1e-12 &&
                  std::fabs(after[2] - beforeZ) < 1e-12,
          "原模型点坐标未被修改");
    Check(input->GetAttributeSet() &&
                  input->GetAttributeSet()->GetNumberOfAttributes() == inputAttrCount,
          "原模型属性集合未被修改");

    std::cout << "\n";
    if (g_Failures == 0) {
        std::cout << "[PASS] WarpByVector 自动测试全部通过。\n";
        return 0;
    }
    std::cout << "[FAIL] 自动测试存在 " << g_Failures << " 项失败。\n";
    return 1;
}
