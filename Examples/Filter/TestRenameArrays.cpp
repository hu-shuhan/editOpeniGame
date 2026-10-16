// 集成回归（待提交）：原标准菜单缺少本过滤器实现；保留源仓库的数值、类型及错误输入断言，
// 用于保证迁移后结果和边界行为一致，防止仅有菜单入口而算法不可用。
/**
 * RenameArrays 自检示例
 *
 * 合成一个单位六面体，带：
 *   - 2 个点数组： "value"（FloatArray）、"alpha"（FloatArray）
 *   - 1 个单元数组："cid"  （IntArray）
 *
 * 逐条断言（对齐 ParaView / VTK vtkArrayRename 的语义）：
 *   1) 输出是一个**全新的同类型网格**（不是输入对象本身），输入对象完全不被修改
 *      （输入里仍是旧名，且输入的数组与输出的数组**不是同一个对象**）；
 *   2) 改名生效：新名可查、旧名消失；数据值逐元素不变；
 *   3) 支持按名字配置与按（关联, 下标）配置，且允许"先配置后 SetInput"；
 *   4) 空新名被拒绝；新名与同组已有数组重名被拒绝并在 GetMessage() 中报告；
 *   5) 未配置的数组保持原名与原值。
 *
 * 说明：本 filter 只处理 AttributeSet 的两种关联（IG_POINT / IG_CELL）。
 * 框架没有 vertex 关联；字段数据（DataObject::Metadata）不属于 AttributeSet，本 filter 不涉及。
 */
#include "RenameArrays/iGameRenameArrays.h"

#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGameFlatArray.h"
#include "iGamePoints.h"
#include "iGameUnstructuredMesh.h"

#include <iGameFileIO.h>

#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

int g_Failed = 0;

void Check(bool ok, const std::string& what) {
    std::cout << (ok ? "  [ OK ] " : "  [FAIL] ") << what << std::endl;
    if (!ok) { ++g_Failed; }
}

/** 测试网格文件名（与 TestResampleWithDataSet 共用，位于 Examples/Models/ 下） */
const char* kTestMeshName = "RenameResample_test.vtk";

/**
 * 定位测试网格：兼容从 <build>/Examples（ctest 的工作目录）、构建根目录、
 * 源码根目录等不同工作目录下运行。
 */
std::string ResolveTestMeshPath() {
    const std::vector<std::string> candidates = {
            std::string("Models/") + kTestMeshName,
            std::string("./Models/") + kTestMeshName,
            std::string("../Models/") + kTestMeshName,
            std::string("../../Models/") + kTestMeshName,
            std::string("Examples/Models/") + kTestMeshName,
            std::string("../../../Examples/Models/") + kTestMeshName,
    };
    for (const std::string& candidate : candidates) {
        if (std::filesystem::exists(candidate)) { return candidate; }
    }
    return std::string();
}

/**
 * 读入测试网格（单位六面体）：点数组 value(0..7) 与 alpha(100..107)，单元数组 cid=42。
 * 见 Examples/Models/RenameResample_test.vtk。
 */
iGame::UnstructuredMesh::Pointer MakeMesh() {
    const std::string path = ResolveTestMeshPath();
    if (path.empty()) {
        Check(false, std::string("找到测试网格 ") + kTestMeshName +
                             "（请在 <build>/Examples 下运行，或确认 Examples/Models 已拷到构建目录）");
        return nullptr;
    }

    iGame::DataObject::Pointer object = iGame::FileIO::ReadFile(path);
    auto mesh = iGame::DynamicCast<iGame::UnstructuredMesh>(object);
    if (mesh == nullptr) {
        Check(false, "读取测试网格失败：" + path);
        return nullptr;
    }
    return mesh;
}

/** 按名字+关联找数组；hits 返回命中个数 */
iGame::ArrayObject::Pointer FindArray(iGame::DataObject* object, const std::string& name, IGenum attachmentType,
                                      int* hits) {
    if (hits != nullptr) { *hits = 0; }
    if (object == nullptr) { return nullptr; }

    iGame::AttributeSet* attrs = object->GetAttributeSet();
    if (attrs == nullptr) { return nullptr; }

    auto all = attrs->GetAllAttributes();
    if (all == nullptr) { return nullptr; }

    iGame::ArrayObject::Pointer found;
    for (IGsize i = 0; i < all->GetNumberOfElements(); ++i) {
        auto& attr = all->GetElement(i);
        if (attr.isDeleted || attr.pointer == nullptr) { continue; }
        if (attr.attachmentType != attachmentType) { continue; }
        if (attr.pointer->GetName() != name) { continue; }
        if (hits != nullptr) { ++(*hits); }
        found = attr.pointer;
    }
    return found;
}

/** 两个数组的维数与全部元素是否完全一致 */
bool SameValues(iGame::ArrayObject* a, iGame::ArrayObject* b) {
    if (a == nullptr || b == nullptr) { return false; }
    if (a->GetDimension() != b->GetDimension()) { return false; }
    if (a->GetNumberOfElements() != b->GetNumberOfElements()) { return false; }
    for (IGsize i = 0; i < a->GetNumberOfElements(); ++i) {
        for (int c = 0; c < a->GetDimension(); ++c) {
            if (std::fabs(a->GetElementValue(i, c) - b->GetElementValue(i, c)) > 1e-12) { return false; }
        }
    }
    return true;
}

/* ------------------------------------------------------------------ */
/* 用例 1：点/单元改名 + 输入不被修改 + 输出独立                        */
/* ------------------------------------------------------------------ */
void TestBasicRename() {
    std::cout << "== 用例 1：点数组/单元数组改名（新网格 + 输入不变 + 输出独立）" << std::endl;

    auto mesh = MakeMesh();
    auto inValue = FindArray(mesh.GetPointer(), "value", IG_POINT, nullptr);

    auto filter = iGame::RenameArrays::New();
    filter->SetInput(mesh);
    filter->SetPointArrayName("value", "pressure");
    filter->SetCellArrayName("cid", "cell_id");
    Check(filter->Execute(), "Execute() 成功");

    auto output = filter->GetOutput(0);
    if (output == nullptr) {
        Check(false, "输出非空");
        return;
    }
    Check(output.GetPointer() != mesh.GetPointer(), "输出是新的对象（不是输入本身）");
    Check(output->GetDataObjectType() == mesh->GetDataObjectType(), "输出类型与输入一致");

    auto outMesh = iGame::DynamicCast<iGame::UnstructuredMesh>(output);
    Check(outMesh != nullptr && outMesh->GetNumberOfPoints() == 8 && outMesh->GetNumberOfCells() == 1,
          "输出点数/单元数与输入一致");

    Check(FindArray(output.GetPointer(), "pressure", IG_POINT, nullptr) != nullptr, "新名 pressure 存在");
    Check(FindArray(output.GetPointer(), "value", IG_POINT, nullptr) == nullptr, "旧名 value 已不存在");
    Check(FindArray(output.GetPointer(), "cell_id", IG_CELL, nullptr) != nullptr, "新名 cell_id 存在");
    Check(FindArray(output.GetPointer(), "cid", IG_CELL, nullptr) == nullptr, "旧名 cid 已不存在");

    auto outValue = FindArray(output.GetPointer(), "pressure", IG_POINT, nullptr);
    Check(SameValues(inValue.GetPointer(), outValue.GetPointer()), "改名后数据值逐元素不变");
    Check(inValue.GetPointer() != outValue.GetPointer(), "输出的数组与输入的数组不是同一对象（独立副本）");

    // 输入完全没被修改
    Check(FindArray(mesh.GetPointer(), "value", IG_POINT, nullptr) != nullptr, "输入仍叫 value（未被污染）");
    Check(FindArray(mesh.GetPointer(), "pressure", IG_POINT, nullptr) == nullptr, "输入里没有出现新名");

    std::cout << "        " << filter->GetMessage() << std::endl;
}

/* ------------------------------------------------------------------ */
/* 用例 2：按（关联, 下标）配置，且允许先配置后 SetInput                */
/* ------------------------------------------------------------------ */
void TestByIndexRename() {
    std::cout << "\n== 用例 2：按下标配置（先配置、后 SetInput）" << std::endl;

    auto mesh = MakeMesh();

    auto filter = iGame::RenameArrays::New();
    // 点数组顺序为 value(0), alpha(1)
    filter->SetPointArrayName("value", "p0");
    filter->SetArrayName(IG_POINT, 1, "temperature");
    Check(filter->GetNumberOfMappings() == 2, "已配置 2 条映射");

    filter->SetInput(mesh);   // 注意：输入在下标配置之后才设置
    Check(filter->GetNumberOfArrays(IG_POINT) == 2, "GetNumberOfArrays(IG_POINT) == 2");
    Check(filter->GetArrayOriginalName(IG_POINT, 1) == "alpha", "GetArrayOriginalName(IG_POINT,1) == alpha");

    Check(filter->Execute(), "Execute() 成功");
    auto output = filter->GetOutput(0);
    Check(output != nullptr && FindArray(output.GetPointer(), "p0", IG_POINT, nullptr) != nullptr,
          "按名字配置的 p0 生效");
    Check(output != nullptr && FindArray(output.GetPointer(), "temperature", IG_POINT, nullptr) != nullptr,
          "按下标配置的 temperature 生效（下标在 Execute 时解析）");
    Check(output != nullptr && FindArray(output.GetPointer(), "alpha", IG_POINT, nullptr) == nullptr,
          "alpha 旧名消失");
    std::cout << "        " << filter->GetMessage() << std::endl;
}

/* ------------------------------------------------------------------ */
/* 用例 3：空名与重名被拒绝                                             */
/* ------------------------------------------------------------------ */
void TestRejections() {
    std::cout << "\n== 用例 3：空新名 / 重名被拒绝" << std::endl;

    auto mesh = MakeMesh();

    auto filter = iGame::RenameArrays::New();
    filter->SetInput(mesh);
    filter->SetPointArrayName("value", "");        // 空名：与 vtkArrayRename 一致，直接拒绝
    filter->SetPointArrayName("value", "alpha");   // 重名：alpha 已存在
    Check(filter->Execute(), "Execute() 成功（拒绝不导致失败）");

    auto output = filter->GetOutput(0);
    Check(output != nullptr && FindArray(output.GetPointer(), "value", IG_POINT, nullptr) != nullptr,
          "空名/重名都被拒绝，value 保持原名");
    Check(output != nullptr && FindArray(output.GetPointer(), "alpha", IG_POINT, nullptr) != nullptr,
          "被撞名的 alpha 不受影响");
    const std::string message = filter->GetMessage();
    Check(message.find("conflict") != std::string::npos, "GetMessage() 报告了重名冲突");
    std::cout << "        " << message << std::endl;
}

/* ------------------------------------------------------------------ */
/* 用例 4：未配置任何映射                                               */
/* ------------------------------------------------------------------ */
void TestNoMapping() {
    std::cout << "\n== 用例 4：未配置映射时全部数组保持原名原值" << std::endl;

    auto mesh = MakeMesh();

    auto filter = iGame::RenameArrays::New();
    filter->SetInput(mesh);
    Check(filter->Execute(), "Execute() 成功");

    auto output = filter->GetOutput(0);
    Check(output != nullptr && FindArray(output.GetPointer(), "value", IG_POINT, nullptr) != nullptr,
          "value 保持原名");
    Check(output != nullptr && FindArray(output.GetPointer(), "alpha", IG_POINT, nullptr) != nullptr,
          "alpha 保持原名");
    Check(output != nullptr && FindArray(output.GetPointer(), "cid", IG_CELL, nullptr) != nullptr, "cid 保持原名");

    auto inValue = FindArray(mesh.GetPointer(), "value", IG_POINT, nullptr);
    auto outValue = (output != nullptr) ? FindArray(output.GetPointer(), "value", IG_POINT, nullptr) : nullptr;
    Check(SameValues(inValue.GetPointer(), outValue.GetPointer()), "值逐元素不变");
    std::cout << "        " << filter->GetMessage() << std::endl;
}

} // namespace

int main() {
    std::cout << "==== RenameArrays 自检 ====" << std::endl;
    std::cout << "测试网格：" << ResolveTestMeshPath() << std::endl;

    // 先确认测试网格可读，避免后续用例在空指针上崩溃
    if (MakeMesh() == nullptr) {
        std::cout << "\n==== 结果：测试网格不可用 ====" << std::endl;
        return 1;
    }

    TestBasicRename();
    TestByIndexRename();
    TestRejections();
    TestNoMapping();

    std::cout << "\n==== 结果：" << (g_Failed == 0 ? "全部通过" : std::to_string(g_Failed) + " 项失败") << " ===="
              << std::endl;
    return g_Failed == 0 ? 0 : 1;
}
