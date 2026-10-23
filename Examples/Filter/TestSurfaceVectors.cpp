#include <cstdlib>
// 迁移回归（待提交）：覆盖标准过滤器的算法/界面行为及源测试中的边界条件，防止仅迁移菜单占位而遗漏实现。
/**
 * TestSurfaceVectors.cpp
 * 测试 SurfaceVectorsFilter — 将向量投影到表面切平面
 *
 * 运行方式：双击运行，自动完成所有测试并输出 PASS/FAIL
 * 测试模型：Examples/Models/SurfaceVectors_plane.vtk
 *
 * 【输出语义（与 ParaView vtkSurfaceVectors 一致）】
 *   结果写回输入向量数组的同名数组（替换原值），不新增独立输出数组。
 *   因此每个测试用例独立加载模型，避免用例间互相污染。
 */
#include <SurfaceVectors/iGameSurfaceVectorsFilter.h>
#include <iGameFileIO.h>
#include <iostream>
#include <string>
#include <vector>
#include <array>
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

static bool Near(float a, float b, float eps = 1e-4f) {
    return std::abs(a - b) < eps;
}

int main(int argc, char* argv[]) {
    std::cout << "===== TestSurfaceVectors =====" << std::endl;

    // 每个用例独立加载模型（filter 为替换语义，会修改输入数据）
    const std::string modelPath = "./Models/SurfaceVectors_plane.vtk";
    auto loadMesh = [&]() -> SurfaceMesh::Pointer {
        auto data = FileIO::ReadFile(modelPath);
        return DynamicCast<SurfaceMesh>(data);
    };
    {
        auto probe = loadMesh();
        if (!probe) {
            std::cout << "ERROR: Failed to read " << modelPath << std::endl;
            std::cout << "Please run from the Examples/ directory." << std::endl;
            if (!std::getenv("IGAME_EXAMPLE_NO_RENDER")) std::cin.get();
            return 1;
        }
        std::cout << "Loaded model: " << probe->GetNumberOfPoints() << " points, "
                  << probe->GetNumberOfFaces() << " faces" << std::endl;
    }

    // ===================================================================
    // Test 1: 点向量投影到切平面（结果写回同名数组 Velocity）
    // 平面在 z=0，法向量为 (0,0,1)，所以投影后 z 分量应该为 0
    // ===================================================================
    std::cout << "\n[Test 1] Point vector projection to tangent plane (in-place 'Velocity')" << std::endl;
    {
        auto mesh = loadMesh();
        // 备份原始 Velocity，供执行后比对 x,y 分量
        std::vector<std::array<float, 3>> origVel;
        {
            auto& inAttr = mesh->GetAttributeSet()->GetAttribute("Velocity");
            origVel.resize(inAttr.pointer->GetNumberOfElements());
            for (IGsize i = 0; i < inAttr.pointer->GetNumberOfElements(); i++) {
                inAttr.pointer->GetElement(i, origVel[i].data());
            }
        }

        auto filter = SurfaceVectorsFilter::New();
        filter->SetInput(mesh);
        filter->SetVectorAttributeByName("Velocity");
        bool ok = filter->Execute();
        TestCheck("Execute returns true", ok);

        auto outMesh = DynamicCast<SurfaceMesh>(filter->GetOutput());
        TestCheck("Output is SurfaceMesh", outMesh != nullptr);

        if (outMesh) {
            auto attrSet = outMesh->GetAttributeSet();
            int idx = attrSet ? attrSet->GetAttributeIndex("Velocity") : -1;
            TestCheck("Output attribute 'Velocity' exists (same name as input)", idx >= 0);

            if (idx >= 0) {
                auto& attr = attrSet->GetAttribute(idx);
                TestCheck("Output is point attribute", attr.attachmentType == IG_POINT);
                TestCheck("Output dimension is 3", attr.pointer->GetDimension() == 3);
                TestCheck("Output element count == 9",
                          attr.pointer->GetNumberOfElements() == 9);

                // 验证每个点的投影：z 分量应该接近 0（平面法向是 z 方向）
                bool allZZero = true;
                bool allXYPreserved = true;
                for (IGsize i = 0; i < attr.pointer->GetNumberOfElements(); i++) {
                    float vals[3];
                    attr.pointer->GetElement(i, vals);
                    if (std::abs(vals[2]) > 1e-4f) allZZero = false;
                    // x,y 分量应该和原始 Velocity 的 x,y 分量接近
                    if (!Near(vals[0], origVel[i][0]) || !Near(vals[1], origVel[i][1])) {
                        allXYPreserved = false;
                    }
                }
                TestCheck("All tangent vectors have z≈0 (plane normal is z)", allZZero);
                TestCheck("Tangent vectors preserve x,y components", allXYPreserved);
            }
        }
    }

    // ===================================================================
    // Test 2: 单元向量投影到切平面（结果写回同名数组 CellVector）
    // ===================================================================
    std::cout << "\n[Test 2] Cell vector projection to tangent plane (in-place 'CellVector')" << std::endl;
    {
        auto mesh = loadMesh();
        auto filter = SurfaceVectorsFilter::New();
        filter->SetInput(mesh);
        filter->SetVectorAttributeByName("CellVector");
        bool ok = filter->Execute();
        TestCheck("Execute returns true", ok);

        auto outMesh = DynamicCast<SurfaceMesh>(filter->GetOutput());
        if (outMesh) {
            auto attrSet = outMesh->GetAttributeSet();
            int idx = attrSet ? attrSet->GetAttributeIndex("CellVector") : -1;
            TestCheck("Output cell attribute 'CellVector' exists (same name)", idx >= 0);

            if (idx >= 0) {
                auto& attr = attrSet->GetAttribute(idx);
                TestCheck("Output is cell attribute", attr.attachmentType == IG_CELL);
                TestCheck("Output element count == 8",
                          attr.pointer->GetNumberOfElements() == 8);

                // 单元法向量也是 z 方向，投影后 z≈0
                bool allZZero = true;
                for (IGsize i = 0; i < attr.pointer->GetNumberOfElements(); i++) {
                    float vals[3];
                    attr.pointer->GetElement(i, vals);
                    if (std::abs(vals[2]) > 1e-4f) allZZero = false;
                }
                TestCheck("All cell tangent vectors have z≈0", allZZero);
            }
        }
    }

    // ===================================================================
    // Test 3: 按索引指定向量属性（同名替换，不新增数组）
    // ===================================================================
    std::cout << "\n[Test 3] Set vector attribute by index" << std::endl;
    {
        auto mesh = loadMesh();
        const int attrCountBefore = mesh->GetAttributeSet()->GetNumberOfAttributes();

        auto filter = SurfaceVectorsFilter::New();
        filter->SetInput(mesh);
        filter->SetVectorAttributeByIndex(0); // Velocity 是第一个向量属性
        bool ok = filter->Execute();
        TestCheck("Execute returns true", ok);

        auto outMesh = DynamicCast<SurfaceMesh>(filter->GetOutput());
        if (outMesh) {
            int idx = outMesh->GetAttributeSet()->GetAttributeIndex("Velocity");
            TestCheck("Attribute 'Velocity' exists (written back, same name)", idx >= 0);
            // 替换语义：不新增可见数组（删除标记 + 追加，净数量不变）
            int visibleAfter = 0;
            auto attrSet = outMesh->GetAttributeSet();
            for (int i = 0; i < attrSet->GetNumberOfAttributes(); i++) {
                auto& a = attrSet->GetAttribute(i);
                if (!a.isDeleted && a.pointer) visibleAfter++;
            }
            int visibleBefore = 0;
            {
                auto mesh2 = loadMesh();
                auto as2 = mesh2->GetAttributeSet();
                for (int i = 0; i < as2->GetNumberOfAttributes(); i++) {
                    auto& a = as2->GetAttribute(i);
                    if (!a.isDeleted && a.pointer) visibleBefore++;
                }
            }
            TestCheck("No extra attribute added (replace semantics)",
                      visibleAfter == visibleBefore);
        }
    }

    // ===================================================================
    // Test 4: 不存在的属性名（错误处理）
    // ===================================================================
    std::cout << "\n[Test 4] Invalid attribute name (error handling)" << std::endl;
    {
        auto mesh = loadMesh();
        auto filter = SurfaceVectorsFilter::New();
        filter->SetInput(mesh);
        filter->SetVectorAttributeByName("NoSuchVector");
        bool ok = filter->Execute();
        TestCheck("Execute returns false for invalid attribute", !ok);
        TestCheck("GetMessage is not empty", !filter->GetMessage().empty());
    }

    // ===================================================================
    // Test 5: 投影向量的长度验证
    // 对于平面，v_tangent 的长度应该 = |v| * sin(theta)，其中 theta 是 v 与法向的夹角
    // 对于 z 法向平面，|v_tangent| = sqrt(vx^2 + vy^2)
    // ===================================================================
    std::cout << "\n[Test 5] Tangent vector magnitude verification" << std::endl;
    {
        auto mesh = loadMesh();
        // 备份原始 Velocity 用于计算期望值
        std::vector<std::array<float, 3>> origVel;
        {
            auto& inAttr = mesh->GetAttributeSet()->GetAttribute("Velocity");
            origVel.resize(inAttr.pointer->GetNumberOfElements());
            for (IGsize i = 0; i < inAttr.pointer->GetNumberOfElements(); i++) {
                inAttr.pointer->GetElement(i, origVel[i].data());
            }
        }

        auto filter = SurfaceVectorsFilter::New();
        filter->SetInput(mesh);
        filter->SetVectorAttributeByName("Velocity");
        filter->Execute();

        auto outMesh = DynamicCast<SurfaceMesh>(filter->GetOutput());
        if (outMesh) {
            auto attrSet = outMesh->GetAttributeSet();
            int idx = attrSet->GetAttributeIndex("Velocity");
            auto& outAttr = attrSet->GetAttribute(idx);

            bool magnitudeCorrect = true;
            for (IGsize i = 0; i < outAttr.pointer->GetNumberOfElements(); i++) {
                float outV[3];
                outAttr.pointer->GetElement(i, outV);

                // 切向量长度应该等于 sqrt(origV.x^2 + origV.y^2)
                float expectedMag = std::sqrt(origVel[i][0] * origVel[i][0] +
                                              origVel[i][1] * origVel[i][1]);
                float actualMag = std::sqrt(outV[0] * outV[0] + outV[1] * outV[1] + outV[2] * outV[2]);

                if (!Near(actualMag, expectedMag, 1e-3f)) {
                    magnitudeCorrect = false;
                    break;
                }
            }
            TestCheck("Tangent vector magnitude matches sqrt(vx^2+vy^2)", magnitudeCorrect);
        }
    }

    // ===================================================================
    // Test 6: Perpendicular 模式（结果写回同名数组，仅法向分量）
    // 平面法向为 z：输出应只保留 z 分量，x,y ≈ 0
    // ===================================================================
    std::cout << "\n[Test 6] Perpendicular mode keeps only normal component" << std::endl;
    {
        auto mesh = loadMesh();
        std::vector<std::array<float, 3>> origVel;
        {
            auto& inAttr = mesh->GetAttributeSet()->GetAttribute("Velocity");
            origVel.resize(inAttr.pointer->GetNumberOfElements());
            for (IGsize i = 0; i < inAttr.pointer->GetNumberOfElements(); i++) {
                inAttr.pointer->GetElement(i, origVel[i].data());
            }
        }

        auto filter = SurfaceVectorsFilter::New();
        filter->SetInput(mesh);
        filter->SetVectorAttributeByName("Velocity");
        filter->SetConstraintModeToPerpendicular();
        bool ok = filter->Execute();
        TestCheck("Execute returns true", ok);

        auto outMesh = DynamicCast<SurfaceMesh>(filter->GetOutput());
        if (outMesh) {
            int idx = outMesh->GetAttributeSet()->GetAttributeIndex("Velocity");
            TestCheck("Attribute 'Velocity' exists (same name)", idx >= 0);
            if (idx >= 0) {
                auto& attr = outMesh->GetAttributeSet()->GetAttribute(idx);
                bool allXYZero = true;
                bool allZPreserved = true;
                for (IGsize i = 0; i < attr.pointer->GetNumberOfElements(); i++) {
                    float vals[3];
                    attr.pointer->GetElement(i, vals);
                    if (std::abs(vals[0]) > 1e-4f || std::abs(vals[1]) > 1e-4f) allXYZero = false;
                    if (!Near(vals[2], origVel[i][2])) allZPreserved = false;
                }
                TestCheck("Perpendicular result has x,y≈0", allXYZero);
                TestCheck("Perpendicular result preserves z", allZPreserved);
            }
        }
    }

    // ===================================================================
    // Test 7: PerpendicularScale 模式（同名标量数组，维度 1，值 = v·n = vz）
    // ===================================================================
    std::cout << "\n[Test 7] PerpendicularScale mode outputs scalar (same name)" << std::endl;
    {
        auto mesh = loadMesh();
        std::vector<std::array<float, 3>> origVel;
        {
            auto& inAttr = mesh->GetAttributeSet()->GetAttribute("Velocity");
            origVel.resize(inAttr.pointer->GetNumberOfElements());
            for (IGsize i = 0; i < inAttr.pointer->GetNumberOfElements(); i++) {
                inAttr.pointer->GetElement(i, origVel[i].data());
            }
        }

        auto filter = SurfaceVectorsFilter::New();
        filter->SetInput(mesh);
        filter->SetVectorAttributeByName("Velocity");
        filter->SetConstraintModeToPerpendicularScale();
        bool ok = filter->Execute();
        TestCheck("Execute returns true", ok);

        auto outMesh = DynamicCast<SurfaceMesh>(filter->GetOutput());
        if (outMesh) {
            int idx = outMesh->GetAttributeSet()->GetAttributeIndex("Velocity");
            TestCheck("Attribute 'Velocity' exists (same name)", idx >= 0);
            if (idx >= 0) {
                auto& attr = outMesh->GetAttributeSet()->GetAttribute(idx);
                TestCheck("Output dimension is 1 (scalar)", attr.pointer->GetDimension() == 1);
                bool allMatch = true;
                for (IGsize i = 0; i < attr.pointer->GetNumberOfElements(); i++) {
                    // 法向为 z：v·n = vz
                    if (!Near(attr.pointer->GetValue(i), origVel[i][2])) allMatch = false;
                }
                TestCheck("Scalar values equal v·n (= vz on this plane)", allMatch);
            }
        }
    }

    // ===================================================================
    // Test 8: A 模型（r=5 球面 + 旋涡场）Parallel 投影，对照 ParaView 实测值
    // 点 1: p=(0.975452, 0, 4.90393)，ParaView vtkPolyDataNormals 结果为
    // (-0.214496, 0.984389, 0.0594882)。点法向必须采用 VTK 语义
    // （单位面法向等权累加），面积加权会在此模型产生十几度偏差
    // ===================================================================
    std::cout << "\n[Test 8] Sphere model (A) Parallel matches ParaView reference" << std::endl;
    {
        auto data = FileIO::ReadFile("./Models/SurfaceVectors_A_sphere.vtk");
        auto mesh = DynamicCast<SurfaceMesh>(data);
        TestCheck("Sphere model loaded as SurfaceMesh", mesh != nullptr);
        if (mesh) {
            auto filter = SurfaceVectorsFilter::New();
            filter->SetInput(mesh);
            filter->SetVectorAttributeByName("Velocity");
            bool ok = filter->Execute();
            TestCheck("Execute returns true", ok);

            auto outMesh = DynamicCast<SurfaceMesh>(filter->GetOutput());
            if (outMesh) {
                int idx = outMesh->GetAttributeSet()->GetAttributeIndex("Velocity");
                if (idx >= 0) {
                    auto& attr = outMesh->GetAttributeSet()->GetAttribute(idx);
                    float vals[3];
                    attr.pointer->GetElement(1, vals); // Point ID = 1
                    const float ref[3] = {-0.214496f, 0.984389f, 0.0594882f};
                    bool match = Near(vals[0], ref[0], 1e-4f) &&
                                 Near(vals[1], ref[1], 1e-4f) &&
                                 Near(vals[2], ref[2], 1e-4f);
                    std::cout << "  igamevis: (" << vals[0] << ", " << vals[1] << ", " << vals[2]
                              << ")  ParaView: (" << ref[0] << ", " << ref[1] << ", " << ref[2] << ")"
                              << std::endl;
                    TestCheck("Point 1 tangent vector matches ParaView (tol 1e-4)", match);

                    // 极点（Point 0）：真值严格为 0，double 化后残差应 < 1e-12
                    // （float 计算时对称抵消残差 ~6e-10，与 ParaView double 的 2e-16 可见不一致）
                    float pole[3];
                    attr.pointer->GetElement(0, pole);
                    bool poleClean = std::fabs(pole[0]) < 1e-12f &&
                                     std::fabs(pole[1]) < 1e-12f &&
                                     std::fabs(pole[2]) < 1e-12f;
                    std::cout << "  pole point0: (" << pole[0] << ", " << pole[1] << ", " << pole[2] << ")" << std::endl;
                    TestCheck("Pole point residual < 1e-12 (double precision path)", poleClean);
                }
            }
        }
    }

    // ===================================================================
    // Test 9: B 模型（quad 山丘 + 常量场）Parallel 投影，对照 ParaView 实测
    // vtkSurfaceVectors 对 quad 用前 3 顶点三角形法向（非 Newell/扇形），
    // 非平面 quad 上与扇形拆分差异 ~1e-4。参照值为 pvpython 实测
    // ===================================================================
    std::cout << "\n[Test 9] Hill model (B, quad) Parallel matches ParaView reference" << std::endl;
    {
        auto data = FileIO::ReadFile("./Models/SurfaceVectors_B_hill.vtk");
        auto mesh = DynamicCast<SurfaceMesh>(data);
        TestCheck("Hill model loaded as SurfaceMesh", mesh != nullptr);
        if (mesh) {
            auto filter = SurfaceVectorsFilter::New();
            filter->SetInput(mesh);
            filter->SetVectorAttributeByName("Velocity");
            bool ok = filter->Execute();
            TestCheck("Execute returns true", ok);

            auto outMesh = DynamicCast<SurfaceMesh>(filter->GetOutput());
            if (outMesh) {
                int idx = outMesh->GetAttributeSet()->GetAttributeIndex("Velocity");
                if (idx >= 0) {
                    auto& attr = outMesh->GetAttributeSet()->GetAttribute(idx);
                    auto checkPt = [&](IGsize pid, const float ref[3], const char* tag) {
                        float vals[3];
                        attr.pointer->GetElement(pid, vals);
                        bool match = Near(vals[0], ref[0], 1e-4f) &&
                                     Near(vals[1], ref[1], 1e-4f) &&
                                     Near(vals[2], ref[2], 1e-4f);
                        std::cout << "  " << tag << " igamevis: (" << vals[0] << ", " << vals[1]
                                  << ", " << vals[2] << ")  ParaView: (" << ref[0] << ", " << ref[1]
                                  << ", " << ref[2] << ")" << std::endl;
                        TestCheck(tag, match);
                    };
                    const float ref0[3] = {0.3004678f, 0.4009575f, 0.0005247f};
                    const float ref12[3] = {0.3f, 0.440664f, 0.0182523f};
                    checkPt(0, ref0, "Point 0 (corner, single quad) matches");
                    checkPt(12, ref12, "Point 12 (boundary) matches");
                }
            }
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
