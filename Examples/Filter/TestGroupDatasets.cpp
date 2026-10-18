// 集成回归（待提交）：原标准菜单缺少本过滤器实现；保留源仓库的数值、类型及错误输入断言，
// 用于保证迁移后结果和边界行为一致，防止仅有菜单入口而算法不可用。
// Regression: 多选合组（Group Datasets）必须只做容器打包、为每个输入建深拷贝克隆，不合并几何。
// 实现方式：克隆是【同类型深拷贝】—— 几何（Points/Cells/…）、属性表、显隐样式全部各持一份，
// 新旧对象完全独立（改一边不影响另一边，见 Test 3 的实改验证）；源对象零修改。
// 错误表现(1)：若误用 AppendReduceFilter 会强制合并顶点与重新编号单元，导致不同拓扑（如体网格与面片）
// 无法组合，或丢失独立块结构。
// 错误表现(2)：若把输入对象直接挂进组（历史实现），AddSubDataObject 会劫持其父指针，
// 删除组后源对象父指针悬空，走导出等 FindParent() 上溯路径会崩溃（评审 P1）。
// 正确行为：输出为 MultiBlock 复合容器，各克隆保留原有类型、拓扑、点单元数与属性（各自一份）；
// 关键边界：多块输入嵌套保留而不展平；允许单输入包一层；nullptr 输入安全过滤；空输入给出错误信息。
// 提交主题: feat(filters): 新增 GroupDatasets 合并数据集（多块容器）
// 查询命令: git log --diff-filter=A --format="%h %s" -- Examples/Filter/TestGroupDatasets.cpp
// ─────────────────────────────────────────────────────────────────────────────
// Windows 平台前置处理：必须在【所有 iGameVis 头文件之前】完成。
// windows.h 会定义宏 `#define GetMessage GetMessageA`，若先被某个 iGameVis 头文件
// 间接引入，随后解析 iGameGroupDatasetsFilter.h 时类内的 GetMessage() 声明会被
// 宏改名，导致调用 filter->GetMessage() 报 C2039。这里先引入并立即 #undef。
// ─────────────────────────────────────────────────────────────────────────────
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#undef GetMessage
#endif

#include <GroupDatasets/iGameGroupDatasetsFilter.h>
#include <iGameDrawObject.h>
#include <iGameFileIO.h>
#include <iGamePointSet.h>
#include <iGameScene.h>
#include <iGameSurfaceMesh.h>
#include <iGameUnstructuredMesh.h>

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

int g_Passed = 0;
int g_Failed = 0;

void Check(bool condition, const std::string& name, const std::string& detail = "") {
    if (condition) {
        ++g_Passed;
        std::cout << "    [PASS] " << name << "\n";
    } else {
        ++g_Failed;
        std::cout << "    [FAIL] " << name;
        if (!detail.empty()) { std::cout << "  -> " << detail; }
        std::cout << "\n";
    }
}

std::string FindModelPath(const std::string& modelName) {
    const std::vector<std::string> searchPaths = {
            "./Models/" + modelName,
            "./Examples/Models/" + modelName,
            "../Examples/Models/" + modelName,
            "../../Examples/Models/" + modelName,
    };
    for (const auto& path: searchPaths) {
        if (std::filesystem::exists(path)) { return path; }
    }
    return "./Models/" + modelName;
}

/// 取点数（UnstructuredMesh / SurfaceMesh 都继承自 PointSet）
int PointCount(const iGame::DataObject::Pointer& obj) {
    auto ps = iGame::DynamicCast<iGame::PointSet>(obj);
    return ps ? static_cast<int>(ps->GetNumberOfPoints()) : -1;
}

/// 取单元/面片数
int CellCount(const iGame::DataObject::Pointer& obj) {
    if (auto m = iGame::DynamicCast<iGame::UnstructuredMesh>(obj)) {
        return static_cast<int>(m->GetNumberOfCells());
    }
    if (auto m = iGame::DynamicCast<iGame::SurfaceMesh>(obj)) {
        return static_cast<int>(m->GetNumberOfFaces());
    }
    return -1;
}

/// 取属性个数（用于验证属性是否随块保留）
int AttributeCount(const iGame::DataObject::Pointer& obj) {
    auto set = obj ? obj->GetAttributeSet() : nullptr;
    return set ? static_cast<int>(set->GetNumberOfAttributes()) : -1;
}

std::string TypeName(const iGame::DataObject::Pointer& obj) {
    if (!obj) { return "null"; }
    switch (obj->GetDataObjectType()) {
        case IG_UNSTRUCTURED_MESH: return "UnstructuredMesh";
        case IG_SURFACE_MESH:      return "SurfaceMesh";
        case IG_VOLUME_MESH:       return "VolumeMesh";
        case IG_STRUCTURED_MESH:   return "StructuredMesh";
        default:                   return "Other";
    }
}

/// 把多块组的子块收集成数组。
///
/// ⚠️ 重要：多块子对象由 `SubDataObjectsHelper` 以
/// `std::map<DataObjectId, DataObject::Pointer>` 保存，
/// 因此【迭代顺序 = DataObjectId 顺序（≈对象创建顺序）】，
/// **不是** AddSubDataObject 的调用顺序。
/// 所以下面所有断言一律用「按名字或按指针查找」，绝不用下标，
/// 否则会依赖一个未被保证的顺序。
std::vector<iGame::DataObject::Pointer> CollectBlocks(const iGame::DataObject::Pointer& group) {
    std::vector<iGame::DataObject::Pointer> blocks;
    if (!group) { return blocks; }
    for (auto it = group->SubDataObjectIteratorBegin(); it != group->SubDataObjectIteratorEnd(); ++it) {
        blocks.push_back(it->second);
    }
    return blocks;
}

/// 按对象指针在子块中查找对应块；找不到返回 nullptr。
/// 浅拷贝壳方案下用于【负断言】：源对象不应出现在组里（出现 = 父指针被劫持）。
iGame::DataObject::Pointer FindBlock(const std::vector<iGame::DataObject::Pointer>& blocks,
                                     const iGame::DataObject::Pointer& target) {
    for (const auto& b: blocks) {
        if (b.GetPointer() == target.GetPointer()) { return b; }
    }
    return nullptr;
}

/// 按【名字】在子块中查找对应壳（壳名复制自源名）；找不到返回 nullptr。
/// 注意：重名子块会命中第一个 —— 测试夹具保证各输入名字互不相同。
iGame::DataObject::Pointer FindBlockByName(const std::vector<iGame::DataObject::Pointer>& blocks,
                                           const std::string& name) {
    for (const auto& b: blocks) {
        if (b->GetName() == name) { return b; }
    }
    return nullptr;
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(65001); // 控制台切 UTF-8，保证中文提示不乱码
#endif
    std::cout << std::unitbuf;

    std::cout << "\n==============================================================================\n";
    std::cout << "  【iGameVis】GroupDatasetsFilter 自检（对齐 ParaView Group Datasets）\n";
    std::cout << "  语义：容器组合不合并几何；子块为深拷贝克隆（数据/显示完全独立）；多块输入嵌套不展平。\n";
    std::cout << "==============================================================================\n";

    // ---------- 准备输入 ----------
    auto tet = iGame::FileIO::ReadFile(FindModelPath("group_tet.vtk"));
    auto hex = iGame::FileIO::ReadFile(FindModelPath("group_hex.vtk"));
    auto quad = iGame::FileIO::ReadFile(FindModelPath("group_quad.vtk"));

    std::cout << "\n[准备] 载入测试模型\n";
    std::cout << "    group_tet.vtk  : " << TypeName(tet) << ", Points = " << PointCount(tet)
              << ", Cells = " << CellCount(tet) << "\n";
    std::cout << "    group_hex.vtk  : " << TypeName(hex) << ", Points = " << PointCount(hex)
              << ", Cells = " << CellCount(hex) << "\n";
    std::cout << "    group_quad.vtk : " << TypeName(quad) << ", Points = " << PointCount(quad)
              << ", Cells = " << CellCount(quad) << "\n";

    // =========================================================================
    std::cout << "\n[Test 1] 基本组合：3 个不同类型（体网格 + 体网格 + 面片）\n";
    // =========================================================================
    {
        std::vector<iGame::DataObject::Pointer> inputs = {tet, hex, quad};
        const int sumPoints = PointCount(tet) + PointCount(hex) + PointCount(quad);
        const int sumCells = CellCount(tet) + CellCount(hex) + CellCount(quad);

        auto filter = iGame::GroupDatasetsFilter::New();
        filter->SetInputs(inputs);
        filter->SetOutputName("GroupDatasets_1");

        bool ok = filter->Execute();
        Check(ok, "Execute() 返回 true", filter->GetMessage());
        Check(filter->GetGroupedCount() == 3, "合并对象数 == 3",
              "actual=" + std::to_string(filter->GetGroupedCount()));

        auto group = filter->GetOutput(0);
        Check(group != nullptr, "输出对象非空");
        Check(group && group->HasSubDataObject(), "输出是多块组");

        auto blocks = CollectBlocks(group);
        Check(static_cast<int>(blocks.size()) == 3, "顶层块数 == 输入数",
              "actual=" + std::to_string(blocks.size()));

        // 逐个输入验证：在输出中能找到对应【深拷贝克隆】（克隆名复制自源名）。
        // 用「按名字查找」而非下标 —— 子块迭代顺序由 DataObjectId 决定。
        for (size_t i = 0; i < inputs.size(); ++i) {
            auto block = FindBlockByName(blocks, inputs[i]->GetName());
            const std::string tag = "输入[" + std::to_string(i) + "] " + inputs[i]->GetName();

            Check(block != nullptr, tag + " 在输出中能找到对应克隆");
            if (!block) { continue; }

            Check(block.GetPointer() != inputs[i].GetPointer(),
                  tag + " 克隆是独立对象（≠ 源对象，未被直接挂载）");
            Check(block->GetName() == inputs[i]->GetName(), tag + " 克隆名 == 输入名");
            Check(block->GetDataObjectType() == inputs[i]->GetDataObjectType(),
                  tag + " 类型保留", TypeName(block));
            Check(PointCount(block) == PointCount(inputs[i]), tag + " 点数未被改动");
            Check(CellCount(block) == CellCount(inputs[i]), tag + " 单元数未被改动");
            Check(AttributeCount(block) == AttributeCount(inputs[i]), tag + " 属性保留");
            Check(block->GetPoints().GetPointer() != inputs[i]->GetPoints().GetPointer(),
                  tag + " 与源不共享 Points 数组（深拷贝，几何独立）");
            Check(block->GetAttributeSet() != inputs[i]->GetAttributeSet(),
                  tag + " 与源不共享属性表（深拷贝，属性独立）");
            Check(block->GetColorMapper().GetPointer() != inputs[i]->GetColorMapper().GetPointer(),
                  tag + " 与源不共享配色器（伪彩独立）");
        }

        // P1 回归（评审第一条）：输出中不存在"输入对象被直接挂载"的子块 ——
        // 若出现，说明父指针又被劫持（删组后源对象会悬空、导出崩溃）。
        bool anyInputHijacked = false;
        for (const auto& src: inputs) {
            if (FindBlock(blocks, src) != nullptr) { anyInputHijacked = true; }
        }
        Check(!anyInputHijacked, "所有子块都是克隆：无输入对象被直接挂载（父指针未被劫持）");

        // 关键：点数/单元数是"各自独立"，不是"合并后"
        int blockPoints = 0, blockCells = 0;
        for (const auto& b: blocks) { blockPoints += PointCount(b); blockCells += CellCount(b); }
        Check(blockPoints == sumPoints, "各块点数之和 == 各输入点数之和（未合并几何）",
              std::to_string(blockPoints) + " vs " + std::to_string(sumPoints));
        Check(blockCells == sumCells, "各块单元数之和 == 各输入单元数之和（未合并几何）",
              std::to_string(blockCells) + " vs " + std::to_string(sumCells));

        // 混合类型确实被保留（体网格仍是体网格，面片仍是面片）
        auto quadBlock = FindBlockByName(blocks, quad->GetName());
        Check(quadBlock && quadBlock->GetDataObjectType() == IG_SURFACE_MESH,
              "面片输入在输出中仍是 SurfaceMesh");
    }

    // =========================================================================
    std::cout << "\n[Test 2] 多块输入：嵌套不展平\n";
    // =========================================================================
    {
        // 先用 tet + hex 做出一个 2 块的多块组
        auto innerFilter = iGame::GroupDatasetsFilter::New();
        innerFilter->SetInputs({tet, hex});
        innerFilter->SetOutputName("InnerGroup");
        Check(innerFilter->Execute(), "内层多块组构建成功", innerFilter->GetMessage());
        auto inner = innerFilter->GetOutput(0);

        // 再用 [内层多块组, quad] 做外层组合
        auto filter = iGame::GroupDatasetsFilter::New();
        filter->SetInputs({inner, quad});
        filter->SetOutputName("GroupDatasets_nested");
        bool ok = filter->Execute();
        Check(ok, "外层组合执行成功", filter->GetMessage());

        auto outer = filter->GetOutput(0);
        auto blocks = CollectBlocks(outer);
        Check(static_cast<int>(blocks.size()) == 2,
              "顶层块数 == 输入数（多块输入算 1 个，未展平）",
              "actual=" + std::to_string(blocks.size()));

        // 按名字找，不按下标 —— 迭代顺序是 DataObjectId 序，不是加入顺序
        auto innerBlock = FindBlockByName(blocks, inner->GetName());
        auto quadBlock = FindBlockByName(blocks, quad->GetName());

        Check(innerBlock != nullptr, "多块输入在输出中能找到对应克隆");
        Check(innerBlock && innerBlock.GetPointer() != inner.GetPointer(),
              "该克隆是递归深拷贝（≠ 内层组对象，未复用子克隆指针）");
        Check(innerBlock && innerBlock->HasSubDataObject(),
              "该块仍是多块组（结构被保留，未展平）");
        Check(innerBlock && innerBlock->GetNumberOfSubDataObjects() == 2,
              "该块内部仍有 2 个子块",
              innerBlock ? "actual=" + std::to_string(innerBlock->GetNumberOfSubDataObjects()) : "");

        Check(quadBlock != nullptr, "面片输入在输出中能找到对应克隆");
        Check(quadBlock && quadBlock->GetDataObjectType() == IG_SURFACE_MESH,
              "该块是面片", TypeName(quadBlock));

        // 顶层块数 == 2 而不是 3，说明多块输入没有被展开
        Check(static_cast<int>(blocks.size()) == 2, "多块输入未被展平（否则会是 3 个顶层块）");
    }

    // =========================================================================
    std::cout << "\n[Test 3] 单输入：允许，等价于包一层\n";
    // =========================================================================
    {
        auto filter = iGame::GroupDatasetsFilter::New();
        filter->SetInputs({tet});
        filter->SetOutputName("GroupDatasets_single");
        bool ok = filter->Execute();
        Check(ok, "单输入 Execute() 返回 true", filter->GetMessage());
        Check(filter->GetGroupedCount() == 1, "合并对象数 == 1");

        auto blocks = CollectBlocks(filter->GetOutput(0));
        Check(static_cast<int>(blocks.size()) == 1, "顶层块数 == 1",
              "actual=" + std::to_string(blocks.size()));
        // 单输入的块同样是克隆：对象独立（≠ 输入），数据也独立（深拷贝）
        Check(!blocks.empty() && blocks[0].GetPointer() != tet.GetPointer(),
              "块是深拷贝克隆，与输入不是同一对象");
        Check(!blocks.empty() && blocks[0]->GetDataObjectType() == tet->GetDataObjectType(),
              "克隆类型与输入一致", blocks.empty() ? "" : TypeName(blocks[0]));

        // 深拷贝独立性（负责人拍板：新旧模型的属性/数据不能一起变）——
        // 改克隆的几何/属性，源必须纹丝不动；克隆自身必须真的改掉。
        if (!blocks.empty()) {
            auto clone = blocks[0];
            auto srcPoints = tet->GetPoints();
            auto clonePoints = clone->GetPoints();
            Check(clonePoints.GetPointer() != srcPoints.GetPointer(), "克隆与源不共享 Points 数组");
            const float srcX = srcPoints->RawPointer()[0];
            const float cloneX = clonePoints->RawPointer()[0];
            clonePoints->SetPoint(0, cloneX + 50.0f, 0.0f, 0.0f);
            Check(srcPoints->RawPointer()[0] == srcX, "改克隆的顶点坐标后，源的坐标不变");
            Check(clonePoints->RawPointer()[0] == cloneX + 50.0f, "克隆自身的顶点坐标改动生效");

            // 显隐独立：改克隆的显隐，源不受影响（显隐接口在 DrawObject 上）
            auto srcDraw = iGame::DynamicCast<iGame::DrawObject>(tet);
            auto cloneDraw = iGame::DynamicCast<iGame::DrawObject>(clone);
            Check(srcDraw && cloneDraw, "显隐测试：源与克隆都是可绘制对象");
            if (srcDraw && cloneDraw) {
                const bool srcVisible = srcDraw->GetVisibility();
                cloneDraw->SetVisibility(!srcVisible);
                Check(srcDraw->GetVisibility() == srcVisible, "改克隆的显隐后，源的显隐不变");
                cloneDraw->SetVisibility(srcVisible);
            }

            auto srcAttrs = tet->GetAttributeSet();
            auto cloneAttrs = clone->GetAttributeSet();
            Check(srcAttrs != cloneAttrs, "克隆与源不共享属性表");
            bool attributeChecked = false;
            if (srcAttrs && cloneAttrs
                && srcAttrs->GetNumberOfAttributes() == cloneAttrs->GetNumberOfAttributes()) {
                for (IGsize i = 0; i < static_cast<IGsize>(srcAttrs->GetNumberOfAttributes()); ++i) {
                    auto& srcAttr = srcAttrs->GetAttribute(i);
                    auto& cloneAttr = cloneAttrs->GetAttribute(i);
                    if (srcAttr.isDeleted || !srcAttr.pointer || !cloneAttr.pointer
                        || srcAttr.pointer->GetNumberOfValues() == 0
                        || cloneAttr.pointer->GetNumberOfValues() == 0) {
                        continue;
                    }
                    const double srcValue = srcAttr.pointer->GetValue(0);
                    cloneAttr.pointer->SetValue(0, srcValue + 1000000.0);
                    Check(srcAttr.pointer->GetValue(0) == srcValue, "改克隆的属性值后，源的属性值不变");
                    Check(cloneAttr.pointer->GetValue(0) != srcValue, "克隆自身的属性值改动生效");
                    attributeChecked = true;
                    break;
                }
            }
            Check(attributeChecked, "至少验证了一组属性值的独立性");

            // 伪彩隔离：克隆与源、与组各持一份配色器（改克隆的色带范围不影响源）
            auto srcMapper = tet->GetColorMapper();
            auto cloneMapper = clone->GetColorMapper();
            auto groupObj = filter->GetOutput(0);
            Check(cloneMapper.GetPointer() != srcMapper.GetPointer(), "克隆与源不共享配色器");
            Check(groupObj && cloneMapper.GetPointer() != groupObj->GetColorMapper().GetPointer(),
                  "克隆与组不共享配色器（伪彩不在同一个组里）");
            const double srcRangeMin = srcMapper->GetRange()[0];
            cloneMapper->SetRange(srcRangeMin + 111.0, srcRangeMin + 222.0);
            Check(srcMapper->GetRange()[0] == srcRangeMin, "改克隆配色器的范围后，源的范围不变");
            Check(cloneMapper->GetRange()[0] == srcRangeMin + 111.0, "克隆自身的配色器范围改动生效");
        }
    }

    // =========================================================================
    std::cout << "\n[Test 3b] 克隆的着色语义：出生未着色，经 ViewCloudPicture 全链路建立\n";
    // =========================================================================
    {
        // 设计依据（代码审查确认）：m_AttributeIndex 是"已执行过 ViewCloudPicture
        // 的记录"，不是可直拷的普通字段 —— 直拷会伪造记录、绕过 ViewCloudPicture 内的
        // dataRange 共享 / 壳传播 / m_ColorWithCell 推导，还可能命中其早退检查而整个
        // 返回（"子块点属性不渲染"的 root cause）。克隆必须与"新加载的模型未点属性前"
        // 一致：以未着色状态出生，由用户操作经 ViewCloudPicture 全链路正常着色。
        auto scene = iGame::Scene::New(); // Update() 无 UpdateFunctor，安全 no-op
        auto colorSrc = iGame::FileIO::ReadFile(FindModelPath("group_tet.vtk"));
        Check(colorSrc != nullptr, "着色语义：载入用例模型");

        auto colorSrcDraw = iGame::DynamicCast<iGame::DrawObject>(colorSrc);
        if (!colorSrcDraw) {
            Check(false, "着色语义：源是可绘制对象");
        } else {
            colorSrcDraw->ViewCloudPicture(scene, 0, 0); // 模拟用户点属性行（走全链路）
            Check(colorSrc->GetAttributeIndex() == 0 && colorSrcDraw->IsUseColor(),
                  "着色语义：源经 ViewCloudPicture 着色成功");
        }

        auto filter = iGame::GroupDatasetsFilter::New();
        filter->SetInputs({colorSrc});
        filter->SetOutputName("ColorSemantics");
        Check(filter->Execute(), "着色语义：组合执行成功", filter->GetMessage());
        auto blocks = CollectBlocks(filter->GetOutput(0));
        if (!blocks.empty()) {
            auto clone = blocks[0];
            auto cloneDraw = iGame::DynamicCast<iGame::DrawObject>(clone);
            if (!cloneDraw) {
                Check(false, "着色语义：克隆是可绘制对象");
            } else {
                Check(clone->GetAttributeIndex() == -1 && !cloneDraw->IsUseColor(),
                      "克隆以未着色状态出生（不直拷源的着色记录，与'新加载未点属性前'一致）");
                // 克隆经 ViewCloudPicture 着色：等价于"点击子块属性行"在 Core 层的操作链路
                cloneDraw->ViewCloudPicture(scene, 0, 0);
                Check(clone->GetAttributeIndex() == 0, "克隆经 ViewCloudPicture 着色：活动标量下标就位");
                Check(cloneDraw->IsUseColor(), "克隆经 ViewCloudPicture 着色：着色开关就位");
                Check(clone->GetCurrentAttributeDimension() == 0, "克隆经 ViewCloudPicture 着色：维度就位");
            }
        } else {
            Check(false, "着色语义：应产出 1 个块");
        }
    }

    // =========================================================================
    std::cout << "\n[Test 4] 输入校验\n";
    // =========================================================================
    {
        // 4.1 空输入
        auto empty = iGame::GroupDatasetsFilter::New();
        empty->SetInputs({});
        bool ok = empty->Execute();
        Check(!ok, "空输入时 Execute() 返回 false");
        Check(!empty->GetMessage().empty(), "空输入时给出提示信息");
        std::cout << "      * 提示内容: " << empty->GetMessage() << "\n";

        // 4.2 含 nullptr：应被过滤掉，只统计有效对象
        auto mixed = iGame::GroupDatasetsFilter::New();
        mixed->SetInputs({nullptr, tet, nullptr, quad});
        mixed->SetOutputName("GroupDatasets_test");
        bool ok2 = mixed->Execute();
        Check(ok2, "含 nullptr 的输入仍能执行", mixed->GetMessage());
        Check(mixed->GetGroupedCount() == 2, "nullptr 被过滤，只合并 2 个有效对象",
              "actual=" + std::to_string(mixed->GetGroupedCount()));

        auto blocks = CollectBlocks(mixed->GetOutput(0));
        Check(static_cast<int>(blocks.size()) == 2, "顶层块数 == 2（不含 nullptr）",
              "actual=" + std::to_string(blocks.size()));

        // 4.3 输出名字被正确设置
        Check(mixed->GetOutput(0) && mixed->GetOutput(0)->GetName() == "GroupDatasets_test",
              "输出名 == SetOutputName 指定的名字",
              mixed->GetOutput(0) ? mixed->GetOutput(0)->GetName() : "");

        // 4.4 同一个对象重复加入：SetInputs 按对象指针去重，只保留 1 份
        //（浅拷贝壳方案下底层 map 的"同一指针只留一份"兜底不再生效，去重前移到输入侧）
        auto dup = iGame::GroupDatasetsFilter::New();
        dup->SetInputs({tet, tet});
        dup->SetOutputName("GroupDatasets_dup");
        dup->Execute();
        auto dupBlocks = CollectBlocks(dup->GetOutput(0));
        Check(static_cast<int>(dupBlocks.size()) == 1,
              "同一对象重复加入只保留 1 块（SetInputs 按源对象去重）",
              "actual=" + std::to_string(dupBlocks.size()));
    }

    // =========================================================================
    std::cout << "\n[Test 5] P1 回归：源父指针零修改 + 删组后真实导出不崩\n";
    // =========================================================================
    {
        auto src = iGame::FileIO::ReadFile(FindModelPath("group_tet.vtk"));
        Check(src != nullptr, "载入 P1 用例模型");
        Check(src && !src->HasParentDataObject(), "组合前：源没有父对象");

        auto filter = iGame::GroupDatasetsFilter::New();
        filter->SetInputs({src});
        filter->SetOutputName("P1_Group");
        Check(filter->Execute(), "组合执行成功", filter->GetMessage());
        Check(src && !src->HasParentDataObject(), "组合后：源仍没有父对象（父指针未被劫持）");
        Check(src && src->FindParent() == src.GetPointer(), "组合后：FindParent() 仍是源自己");

        // 销毁组（模拟 UI 删除结果组）：filter 持有输出，释放后组与克隆一并析构
        filter = nullptr;

        std::error_code ec;

        // (a) 原翻车路径回归：.igcm 分支在 WriteFile 内先做 FindParent() 上溯并据此构造共享
        //     指针（iGameFileIO.cpp:629）。浅拷贝方案下此时父链指向【已释放的组】→ 对悬空
        //     对象构造共享指针 → SIGSEGV（评审 P1 实测路径）。深拷贝方案下源从未被挂载，
        //     FindParent() 恒为自己，该分支不会构造父指针；能执行到本断言即证明"存活"。
        //     返回 false 属格式语义：.igcm 要求 multiblock 容器（IGCMWriter::GenerateBuffers
        //     检查 HasSubDataObject），单个叶子网格本就写不出，与 P1 无关。
        const std::string igcmPath = (std::filesystem::current_path() / "p1_regression_export.igcm").string();
        const bool wroteIgcm = iGame::FileIO::WriteFile(igcmPath, src);
        Check(src && src->FindParent() == src.GetPointer(),
              "删组后：走 .igcm 导出路径（P1 原始翻车点）存活，源父指针仍为自己",
              "write=" + std::string(wroteIgcm ? "true" : "false(叶子非容器，格式语义)"));
        std::filesystem::remove(igcmPath, ec);

        // (b) 正向证据：叶子网格用叶子格式（.vtk）导出，应真实成功且文件非空
        const std::string vtkPath = (std::filesystem::current_path() / "p1_regression_export.vtk").string();
        const bool wroteVtk = iGame::FileIO::WriteFile(vtkPath, src);
        Check(wroteVtk, "删组后：对源导出 .vtk 真实成功");
        if (wroteVtk) {
            const auto bytes = std::filesystem::file_size(vtkPath, ec);
            Check(!ec && bytes > 0, "导出文件非空", "bytes=" + std::to_string(bytes));

            // (c) roundtrip：导出的 .vtk 能重新读回，点数/单元数一致（证明文件内容有效）
            auto reloaded = iGame::FileIO::ReadFile(vtkPath);
            Check(reloaded != nullptr, "导出的 .vtk 能重新读回");
            if (reloaded && src) {
                Check(PointCount(reloaded) == PointCount(src) && CellCount(reloaded) == CellCount(src),
                      "读回后点数/单元数与原源一致",
                      "reload=" + std::to_string(PointCount(reloaded)) + "/" + std::to_string(CellCount(reloaded)) +
                              " src=" + std::to_string(PointCount(src)) + "/" + std::to_string(CellCount(src)));
            }
        }
        std::filesystem::remove(vtkPath, ec);

        Check(src && !src->HasParentDataObject(), "删组后：源仍无父对象");
    }

    // =========================================================================
    std::cout << "\n============================================================\n";
    std::cout << "  Result: " << g_Passed << " passed, " << g_Failed << " failed\n";
    std::cout << "============================================================\n";
    if (g_Failed == 0) {
        std::cout << "[SUCCESS] all assertions passed.\n";
        return 0;
    }
    std::cout << "[FAILURE] some assertions failed.\n";
    return 1;
}
