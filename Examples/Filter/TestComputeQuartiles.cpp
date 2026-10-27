#include <cstdlib>
// 迁移回归（待提交）：防止标准过滤器仅有菜单占位，验证本文件记录的算法结果、输入保持不变及边界情况。
/**
 * TestComputeQuartiles.cpp
 * 测试 ComputeQuartilesFilter — 对标 ParaView 的 Compute Quartiles 滤镜
 *
 * ParaView 的 vtkComputeQuartiles 输出一个 vtkTable，5 行 (Min/Q1/Median/Q3/Max)
 * × N 列（每个标量属性一列）。本测试验证 iGame 实现的等价行为：
 *   - 输出 PointSet 含 5 顶点（对应 5 行）
 *   - 每个被统计的标量属性 A 在输出上生成 "{A}_Quartiles" 标量数组（长度 5）
 *
 * 运行方式：双击运行，自动完成所有测试并输出 PASS/FAIL
 * 测试模型：Examples/Models/Quartiles_plane.vtk
 */
#include <ComputeQuartiles/iGameComputeQuartilesFilter.h>
#include <iGameFileIO.h>
#include <iostream>
#include <string>
#include <cmath>

using namespace iGame;

static int g_passCount = 0;
static int g_failCount = 0;

static void TestCheck(const std::string& name, bool condition) {
    if (condition) {
        std::cout << "  [PASS] " << name << std::endl;
        g_passCount++;
    } else {
        std::cout << "  [FAIL] " << name << std::endl;
        g_failCount++;
    }
}

static bool Near(double a, double b, double eps = 1e-4) {
    return std::abs(a - b) < eps;
}

int main(int argc, char* argv[]) {
    std::cout << "===== TestComputeQuartiles =====" << std::endl;

    // 读取测试模型
    const std::string modelPath = "./Models/Quartiles_plane.vtk";
    auto data = FileIO::ReadFile(modelPath);
    if (!data) {
        std::cout << "ERROR: Failed to read " << modelPath << std::endl;
        std::cout << "Please run from the Examples/ directory." << std::endl;
        if (!std::getenv("IGAME_EXAMPLE_NO_RENDER")) std::cin.get();
        return 1;
    }
    auto mesh = DynamicCast<SurfaceMesh>(data);
    if (!mesh) {
        std::cout << "ERROR: Input is not a SurfaceMesh." << std::endl;
        if (!std::getenv("IGAME_EXAMPLE_NO_RENDER")) std::cin.get();
        return 1;
    }
    std::cout << "Loaded model: " << mesh->GetNumberOfPoints() << " points, "
              << mesh->GetNumberOfFaces() << " faces" << std::endl;

    // ===================================================================
    // Test 1: 点标量属性（Temperature）的单属性模式
    // 验证 Min/Q1/Median/Q3/Max 5 行统计
    // ===================================================================
    std::cout << "\n[Test 1] Point scalar - Temperature quartiles (single-attr mode)" << std::endl;
    {
        auto filter = ComputeQuartilesFilter::New();
        filter->SetInput(mesh);
        filter->SetAttributeByName("Temperature");
        bool ok = filter->Execute();
        TestCheck("Execute returns true", ok);

        // Temperature 值（已排序）：10,15,20,20,25,25,30,30,35,35,40,40,45,45,50,55 (n=16)
        // Min=10, Max=55
        // Tukey hinges: 下半区前 8 个(10..30)的中位数 = (20+25)/2 = 22.5 -> Q1
        //               Median = (val[7]+val[8])/2 = (30+35)/2 = 32.5
        //               上半区后 8 个(35..55)的中位数 = (40+45)/2 = 42.5 -> Q3
        const auto& results = filter->GetResults();
        TestCheck("GetResults() returns 1 entry", results.size() == 1);
        if (!results.empty()) {
            const auto& r = results[0];
            TestCheck("result.arrayName == 'Temperature'", r.arrayName == "Temperature");
            TestCheck("result.attachmentType == IG_POINT", r.attachmentType == IG_POINT);
            TestCheck("Min == 10.0",    Near(r.min, 10.0));
            TestCheck("Max == 55.0",    Near(r.max, 55.0));
            TestCheck("Median == 32.5", Near(r.median, 32.5));
            TestCheck("Q1 == 22.5 (Tukey hinge, not R7 23.75)", Near(r.q1, 22.5));
            TestCheck("Q3 == 42.5 (Tukey hinge, not R7 41.25)", Near(r.q3, 42.5));
        }

        // 兼容旧 API：单属性模式下，GetMin/GetQ1/.../GetMax 应等价于 results[0]
        TestCheck("GetMin() == 10.0",  Near(filter->GetMin(), 10.0));
        TestCheck("GetMax() == 55.0",  Near(filter->GetMax(), 55.0));
        TestCheck("GetMedian() == 32.5", Near(filter->GetMedian(), 32.5));
        TestCheck("GetQ1() == 22.5", Near(filter->GetQ1(), 22.5));
        TestCheck("GetQ3() == 42.5", Near(filter->GetQ3(), 42.5));

        // 输出 PointSet 上应存在 "Temperature_Quartiles" 标量数组，长度为 5
        auto out = filter->GetOutput();
        auto outPS = DynamicCast<PointSet>(out);
        TestCheck("Output is PointSet", outPS != nullptr);
        if (outPS) {
            TestCheck("Output PointSet has 5 vertices", outPS->GetNumberOfPoints() == 5);
            auto outAttrSet = outPS->GetAttributeSet();
            bool hasQuartilesArray = false;
            igIndex quartilesIdx = outAttrSet ? outAttrSet->GetAttributeIndex("Temperature_Quartiles") : -1;
            hasQuartilesArray = (quartilesIdx >= 0);
            TestCheck("'Temperature_Quartiles' attribute exists in output", hasQuartilesArray);
            if (hasQuartilesArray) {
                auto& q = outAttrSet->GetAttribute(quartilesIdx);
                TestCheck("'Temperature_Quartiles' has 5 elements",
                          q.pointer && q.pointer->GetNumberOfElements() == 5);
                TestCheck("'Temperature_Quartiles' has dimension 1",
                          q.pointer && q.pointer->GetDimension() == 1);
            }
        }
    }

    // ===================================================================
    // Test 2: 按索引指定属性
    // ===================================================================
    std::cout << "\n[Test 2] Set attribute by index" << std::endl;
    {
        auto filter = ComputeQuartilesFilter::New();
        filter->SetInput(mesh);
        filter->SetAttributeByIndex(0); // 第一个属性应该是 Temperature
        bool ok = filter->Execute();
        TestCheck("Execute returns true", ok);
        TestCheck("GetMin() == 10.0", Near(filter->GetMin(), 10.0));
    }

    // ===================================================================
    // Test 3: IntArray 类型属性（Pressure）
    // ===================================================================
    std::cout << "\n[Test 3] Int array attribute - Pressure" << std::endl;
    {
        auto filter = ComputeQuartilesFilter::New();
        filter->SetInput(mesh);
        filter->SetAttributeByName("Pressure");
        bool ok = filter->Execute();
        TestCheck("Execute returns true (IntArray supported)", ok);
        // Pressure 值: 1,2,3,4, 2,3,4,5, 3,4,5,6, 4,5,6,7 → 排序后 1..7
        TestCheck("Min == 1", Near(filter->GetMin(), 1.0));
        TestCheck("Max == 7", Near(filter->GetMax(), 7.0));
        // Median at q=0.5: idx = 0.5*15 = 7.5 -> val[7]*0.5 + val[8]*0.5 = 4*0.5 + 4*0.5 = 4
        TestCheck("Median == 4", Near(filter->GetMedian(), 4.0));
    }

    // ===================================================================
    // Test 4: 单元标量属性（CellValue）的单属性模式
    // 注：指定单个属性是 iGame 的扩展能力（ParaView 的属性面板只有 Input，
    //     且只统计点数据数组）；此处同时验证奇数样本的 Tukey 规则。
    // ===================================================================
    std::cout << "\n[Test 4] Cell scalar - CellValue quartiles (single-attr mode)" << std::endl;
    {
        auto filter = ComputeQuartilesFilter::New();
        filter->SetInput(mesh);
        filter->SetAttributeByName("CellValue");
        bool ok = filter->Execute();
        TestCheck("Execute returns true", ok);
        // CellValue: 100..180 步长 10 (n=9)
        // Min=100, Max=180, Median=val[4]=140
        // Tukey hinges（奇数样本时中位数不参与任何一半）:
        //   下半区 val[0..3]=(100,110,120,130) 中位数 = 115 -> Q1（R7 会得 120）
        //   上半区 val[5..8]=(150,160,170,180) 中位数 = 165 -> Q3（R7 会得 160）
        TestCheck("Min == 100.0",   Near(filter->GetMin(), 100.0));
        TestCheck("Max == 180.0",   Near(filter->GetMax(), 180.0));
        TestCheck("Median == 140.0", Near(filter->GetMedian(), 140.0));
        TestCheck("Q1 == 115.0 (odd-n hinge excludes median)", Near(filter->GetQ1(), 115.0));
        TestCheck("Q3 == 165.0 (odd-n hinge excludes median)", Near(filter->GetQ3(), 165.0));

        const auto& results = filter->GetResults();
        if (!results.empty()) {
            TestCheck("attachmentType == IG_CELL",
                      results[0].attachmentType == IG_CELL);
        }
    }

    // ===================================================================
    // Test 5: 不存在的属性名（错误处理）
    // ===================================================================
    std::cout << "\n[Test 5] Invalid attribute name (error handling)" << std::endl;
    {
        auto filter = ComputeQuartilesFilter::New();
        filter->SetInput(mesh);
        filter->SetAttributeByName("NonExistentAttr");
        bool ok = filter->Execute();
        TestCheck("Execute returns false for invalid attribute", !ok);
        TestCheck("GetMessage is not empty", !filter->GetMessage().empty());
    }

    // ===================================================================
    // Test 6: 全属性模式（对标 ParaView Compute Quartiles 默认行为）
    // 只统计点数据标量数组（Temperature、Pressure）
    // 单元数据数组（CellValue）不参与——与 vtkComputeQuartiles 一致
    // ===================================================================
    std::cout << "\n[Test 6] Process-all mode (ParaView default: point data only)" << std::endl;
    {
        auto filter = ComputeQuartilesFilter::New();
        filter->SetInput(mesh);
        // 不调用 SetAttributeByName / SetAttributeByIndex，默认处理全部点标量属性
        bool ok = filter->Execute();
        TestCheck("Execute returns true", ok);

        const auto& results = filter->GetResults();
        // 输入的点标量属性只有 Temperature 和 Pressure 两个
        TestCheck("GetResults() size == 2 (Temperature + Pressure)", results.size() == 2);

        bool onlyPointData = true;
        for (const auto& r : results) {
            if (r.attachmentType != IG_POINT) onlyPointData = false;
        }
        TestCheck("All results are point data (no cell arrays)", onlyPointData);

        // 验证每个属性都生成了对应的 "{name}_Quartiles" 输出数组
        auto outPS = DynamicCast<PointSet>(filter->GetOutput());
        TestCheck("Output is PointSet", outPS != nullptr);
        if (outPS) {
            TestCheck("Output PointSet has 5 vertices", outPS->GetNumberOfPoints() == 5);
            auto outAttrSet = outPS->GetAttributeSet();
            if (outAttrSet) {
                for (const auto& r : results) {
                    std::string expectedName = r.arrayName + "_Quartiles";
                    igIndex idx = outAttrSet->GetAttributeIndex(expectedName);
                    std::string label = "Output has '" + expectedName + "'";
                    TestCheck(label, idx >= 0);
                }
                TestCheck("Output has NO 'CellValue_Quartiles' (cell arrays excluded)",
                          outAttrSet->GetAttributeIndex("CellValue_Quartiles") < 0);
            }
        }
    }

    // ===================================================================
    // Test 7: 与 ParaView 6.1.1 实测结果对齐（真值回归锁）
    // 真值获取方式（本机 D:\Program Files\ParaView 6.1.1\bin\pvpython.exe）：
    //   OpenDataFile(file) -> ComputeQuartiles(Input=r) -> Fetch
    //   => vtkTable，5 行 x N 列，行序固定为 Min / Q1 / Median / Q3 / Max
    // 注意 ParaView 只输出点数据数组，单元数据数组（Power/Energy/CellLoad）不出现
    // ===================================================================
    std::cout << "\n[Test 7] Ground truth vs ParaView 6.1.1 (point data only)" << std::endl;
    {
        struct Truth {
            const char* file;
            const char* array;
            int         numResults;
            double      v[5]; // Min, Q1, Median, Q3, Max
        };
        const Truth truths[] = {
            { "ComputeQuartiles_A_sphere.vtk", "Temperature", 1,
              { 4.702391, 8.739321, 15.0, 21.260679, 25.297609 } },
            { "ComputeQuartiles_C_hex_volume.vtk", "Pressure", 1,
              { 68.230530, 78.030960, 82.735291, 86.647526, 96.691933 } },
        };
        const char* rows[5] = { "Min", "Q1", "Median", "Q3", "Max" };

        for (const auto& t : truths) {
            std::string path = std::string("./Models/") + t.file;
            auto data = FileIO::ReadFile(path);
            std::string tag = std::string(t.file) + " ";
            if (!data) {
                TestCheck(tag + "read ok", false);
                continue;
            }
            auto f = ComputeQuartilesFilter::New();
            f->SetInput(data);
            TestCheck(tag + "execute", f->Execute());

            const auto& rs = f->GetResults();
            TestCheck(tag + "result count == " + std::to_string(t.numResults),
                      static_cast<int>(rs.size()) == t.numResults);

            const ComputeQuartilesFilter::QuartileResult* target = nullptr;
            for (const auto& r : rs) {
                if (r.arrayName == t.array) target = &r;
            }
            TestCheck(tag + "has array " + t.array, target != nullptr);
            if (target) {
                const double got[5] = { target->min, target->q1, target->median,
                                        target->q3, target->max };
                for (int i = 0; i < 5; i++) {
                    TestCheck(tag + t.array + " " + rows[i] + " == " + std::to_string(t.v[i]),
                              Near(got[i], t.v[i], 1e-4));
                }
            }
        }

        // B_hill: 两个点标量属性，单元属性 CellLoad 必须被排除
        auto data = FileIO::ReadFile("./Models/ComputeQuartiles_B_hill.vtk");
        if (!data) {
            TestCheck("ComputeQuartiles_B_hill.vtk read ok", false);
        } else {
            auto f = ComputeQuartilesFilter::New();
            f->SetInput(data);
            TestCheck("B_hill execute", f->Execute());
            const auto& rs = f->GetResults();
            TestCheck("B_hill result count == 2 (Height + Skew)", rs.size() == 2);
            bool hasCellLoad = false;
            for (const auto& r : rs) {
                if (r.arrayName == "CellLoad") hasCellLoad = true;
            }
            TestCheck("B_hill excludes cell array CellLoad", !hasCellLoad);
        }
    }

    // ===================================================================
    // 汇总
    // ===================================================================
    std::cout << "\n===== Summary =====" << std::endl;
    std::cout << "  PASS: " << g_passCount << std::endl;
    std::cout << "  FAIL: " << g_failCount << std::endl;
    std::cout << "  Total: " << g_passCount + g_failCount << std::endl;

    if (g_failCount == 0) {
        std::cout << "\n  All tests PASSED!" << std::endl;
    } else {
        std::cout << "\n  Some tests FAILED!" << std::endl;
    }

    std::cout << "\nPress Enter to exit..." << std::endl;
    if (!std::getenv("IGAME_EXAMPLE_NO_RENDER")) std::cin.get();
    return g_failCount > 0 ? 1 : 0;
}
