// 集成回归（待提交）：原标准菜单缺少本过滤器实现；保留源仓库的数值、类型及错误输入断言，
// 用于保证迁移后结果和边界行为一致，防止仅有菜单入口而算法不可用。
#include <TextureMapToSphere/iGameTextureMapToSphereFilter.h>

#include <iGameAttributeSet.h>
#include <iGameCellArray.h>
#include <iGameCellType.h>
#include <iGameFileIO.h>
#include <iGameFlatArray.h>
#include <iGamePointSet.h>
#include <iGamePoints.h>
#include <iGameSurfaceMesh.h>
#include <iGameType.h>
#include <iGameUnstructuredMesh.h>
#include <iGameVolumeMesh.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>

namespace
{

bool Check(bool ok, const std::string& name) {
    if (ok) {
        std::cout << "[PASS] " << name << std::endl;
    } else {
        std::cout << "[FAIL] " << name << std::endl;
    }
    return ok;
}

void Step(const std::string& name) { std::cout << "  >> " << name << std::endl; }

bool Near(double value, double expected, double tolerance = 1.0e-5) {
    return std::fabs(value - expected) < tolerance;
}

bool FileExists(const std::string& path) {
    std::ifstream f(path);
    return f.good();
}

std::string FindModel() {
    const std::string candidates[] = {
            "./Examples/Models/TextureMapSphere.vtk",
            "./Models/TextureMapSphere.vtk",
            "../Examples/Models/TextureMapSphere.vtk",
    };
    for (const auto& c: candidates) {
        if (FileExists(c)) { return c; }
    }
    return "";
}

// 从 filter 的输出里取出纹理坐标数组（要求是点属性上的 2 分量数组）
iGame::FloatArray::Pointer GetTexCoords(iGame::DataObject::Pointer out, const std::string& name) {
    if (out.IsNull()) { return nullptr; }
    auto attrs = out->GetAttributeSet();
    if (attrs == nullptr) { return nullptr; }

    const int index = attrs->GetAttributeIndex(name);
    if (index < 0) { return nullptr; }

    auto& attribute = attrs->GetAttribute(static_cast<IGsize>(index));
    if (attribute.isDeleted || attribute.pointer.IsNull()) { return nullptr; }
    if (attribute.type != IG_TCOORD) { return nullptr; }
    if (attribute.attachmentType != IG_POINT) { return nullptr; }
    if (attribute.pointer->GetDimension() != 2) { return nullptr; }

    return iGame::DynamicCast<iGame::FloatArray>(attribute.pointer);
}

// 沿三个方向各造一对点的正八面体（6 个点、8 个三角形），整体平移 (ox, oy, oz)
iGame::SurfaceMesh::Pointer MakeOctahedron(double ox, double oy, double oz) {
    auto mesh = iGame::SurfaceMesh::New();
    mesh->SetName("Octahedron");
    mesh->AddPoint(iGame::Point(static_cast<float>(1.0 + ox), static_cast<float>(0.0 + oy),
                                static_cast<float>(0.0 + oz))); // 0
    mesh->AddPoint(iGame::Point(static_cast<float>(0.0 + ox), static_cast<float>(1.0 + oy),
                                static_cast<float>(0.0 + oz))); // 1
    mesh->AddPoint(iGame::Point(static_cast<float>(-1.0 + ox), static_cast<float>(0.0 + oy),
                                static_cast<float>(0.0 + oz))); // 2
    mesh->AddPoint(iGame::Point(static_cast<float>(0.0 + ox), static_cast<float>(-1.0 + oy),
                                static_cast<float>(0.0 + oz))); // 3
    mesh->AddPoint(iGame::Point(static_cast<float>(0.0 + ox), static_cast<float>(0.0 + oy),
                                static_cast<float>(1.0 + oz))); // 4
    mesh->AddPoint(iGame::Point(static_cast<float>(0.0 + ox), static_cast<float>(0.0 + oy),
                                static_cast<float>(-1.0 + oz))); // 5

    igIndex top0[3] = {0, 1, 4};
    igIndex top1[3] = {1, 2, 4};
    igIndex top2[3] = {2, 3, 4};
    igIndex top3[3] = {3, 0, 4};
    igIndex bottom0[3] = {1, 0, 5};
    igIndex bottom1[3] = {2, 1, 5};
    igIndex bottom2[3] = {3, 2, 5};
    igIndex bottom3[3] = {0, 3, 5};

    auto faces = iGame::CellArray::New();
    mesh->SetFaces(faces);
    faces->AddCellIds(top0, 3);
    faces->AddCellIds(top1, 3);
    faces->AddCellIds(top2, 3);
    faces->AddCellIds(top3, 3);
    faces->AddCellIds(bottom0, 3);
    faces->AddCellIds(bottom1, 3);
    faces->AddCellIds(bottom2, 3);
    faces->AddCellIds(bottom3, 3);
    return mesh;
}

// 正八面体（球心 = 原点）的期望纹理坐标，点序与 MakeOctahedron 一致：
//   0 ( 1, 0, 0)   1 ( 0, 1, 0)   2 (-1, 0, 0)
//   3 ( 0,-1, 0)   4 ( 0, 0, 1)   5 ( 0, 0,-1)
// PreventSeam 开（VTK 默认）：s = acos(dx / rho) / pi
const double OCTAHEDRON_SEAM_ON[6][2] = {
        {0.00, 0.50}, {0.50, 0.50}, {1.00, 0.50}, {0.50, 0.50}, {0.00, 0.00}, {0.50, 1.00},
};
// PreventSeam 关：s = atan2(dy, dx) / (2 pi)（负值 +1）
const double OCTAHEDRON_SEAM_OFF[6][2] = {
        {0.00, 0.50}, {0.25, 0.50}, {0.50, 0.50}, {0.75, 0.50}, {0.00, 0.00}, {0.25, 1.00},
};

// 跑一次 filter 并逐点核对 (s, t)
bool CheckTable(iGame::DataObject::Pointer mesh, double cx, double cy, double cz, bool useCenter,
                bool preventSeam, const double expected[][2], int expectedCount, const std::string& title) {
    std::cout << "\n== " << title << " ==" << std::endl;

    Step("run filter");
    auto filter = iGame::TextureMapToSphereFilter::New();
    filter->SetPreventSeam(preventSeam);
    if (useCenter) { filter->SetCenter(cx, cy, cz); }
    filter->SetInput(0, mesh);
    if (!Check(filter->Execute(), "filter Execute()")) { return false; }

    Step("check output");
    auto out = filter->GetOutput();
    if (!Check(!out.IsNull(), "output exists")) { return false; }

    auto tex = GetTexCoords(out, "TextureCoordinates");
    if (!Check(!tex.IsNull(), "output has point attribute TextureCoordinates (IG_TCOORD, 2 components)")) {
        return false;
    }
    if (!Check(tex->GetNumberOfElements() == static_cast<IGsize>(expectedCount), "texture coordinate count")) {
        return false;
    }

    bool ok = true;
    std::cout << "  point : (s, t)" << std::endl;
    for (int i = 0; i < expectedCount; i++) {
        const double s = tex->GetElementValue(static_cast<IGsize>(i), 0);
        const double t = tex->GetElementValue(static_cast<IGsize>(i), 1);
        std::cout << "    " << i << "   : (" << s << ", " << t << ")" << std::endl;
        ok &= Check(Near(s, expected[i][0]) && Near(t, expected[i][1]),
                    "point " + std::to_string(i) + " = (" + std::to_string(expected[i][0]) + ", " +
                            std::to_string(expected[i][1]) + ")");
    }
    return ok;
}

// 场景 1：默认设置（PreventSeam 开）+ 自动球心（点坐标平均值 = 原点）
bool TestDefaultSettings() {
    auto mesh = MakeOctahedron(0.0, 0.0, 0.0);
    auto filter = iGame::TextureMapToSphereFilter::New();
    if (!Check(filter->GetAutomaticCenter(), "automatic center is on by default")) { return false; }
    if (!Check(filter->GetPreventSeam(), "PreventSeam is on by default (same as VTK/ParaView)")) {
        return false;
    }
    return CheckTable(mesh, 0.0, 0.0, 0.0, false, true, OCTAHEDRON_SEAM_ON, 6,
                      "Test 1: default settings (auto center + PreventSeam on)");
}

// 场景 2：PreventSeam 关掉
bool TestPreventSeamOff() {
    auto mesh = MakeOctahedron(0.0, 0.0, 0.0);
    return CheckTable(mesh, 0.0, 0.0, 0.0, false, false, OCTAHEDRON_SEAM_OFF, 6,
                      "Test 2: PreventSeam off (真实经度，绕一圈 0~1)");
}

// 场景 3：整体平移后自动球心，结果应与场景 1 完全一样
bool TestAutoCenterTranslated() {
    auto mesh = MakeOctahedron(2.0, -3.0, 1.0);
    bool ok = CheckTable(mesh, 0.0, 0.0, 0.0, false, true, OCTAHEDRON_SEAM_ON, 6,
                         "Test 3: auto center (mesh translated by (2, -3, 1))");
    std::cout << "  >> confirm auto center == mean of input points" << std::endl;
    auto filter = iGame::TextureMapToSphereFilter::New();
    filter->SetInput(0, mesh);
    ok &= Check(filter->Execute(), "run again to read back the center");
    const double* center = filter->GetCenter();
    ok &= Check(Near(center[0], 2.0) && Near(center[1], -3.0) && Near(center[2], 1.0),
                "center equals the point mean (2, -3, 1)");
    return ok;
}

// 场景 4：显式指定球心
bool TestExplicitCenter() {
    auto mesh = MakeOctahedron(2.0, -3.0, 1.0);
    auto filter = iGame::TextureMapToSphereFilter::New();
    filter->SetCenter(2.0, -3.0, 1.0);
    if (!Check(!filter->GetAutomaticCenter(), "explicit center turns automatic center off")) { return false; }
    return CheckTable(mesh, 2.0, -3.0, 1.0, true, true, OCTAHEDRON_SEAM_ON, 6,
                      "Test 4: explicit center (2, -3, 1)");
}

// 场景 5：偏心球心 (0, 0, 1)，两个分支都核对
//   偏移：p0 (1,0,-1)  p1 (0,1,-1)  p2 (-1,0,-1)  p3 (0,-1,-1)  p4 (0,0,0)  p5 (0,0,-2)
const double OFF_CENTER_SEAM_ON[6][2] = {
        {0.00, 0.75}, {0.50, 0.75}, {1.00, 0.75}, {0.50, 0.75}, {0.00, 0.00}, {0.50, 1.00},
};
const double OFF_CENTER_SEAM_OFF[6][2] = {
        {0.00, 0.75}, {0.25, 0.75}, {0.50, 0.75}, {0.75, 0.75}, {0.00, 0.00}, {0.25, 1.00},
};

bool TestOffCenterSphere() {
    bool ok = true;
    {
        auto mesh = MakeOctahedron(0.0, 0.0, 0.0);
        ok &= CheckTable(mesh, 0.0, 0.0, 1.0, true, true, OFF_CENTER_SEAM_ON, 6,
                         "Test 5a: center (0, 0, 1), PreventSeam on");
    }
    {
        auto mesh = MakeOctahedron(0.0, 0.0, 0.0);
        ok &= CheckTable(mesh, 0.0, 0.0, 1.0, true, false, OFF_CENTER_SEAM_OFF, 6,
                         "Test 5b: center (0, 0, 1), PreventSeam off");
    }
    return ok;
}

// 场景 6：非结构网格 / 体网格 / 点集
bool TestOtherMeshTypes() {
    std::cout << "\n== Test 6: unstructured mesh / volume mesh / point set ==" << std::endl;
    bool ok = true;

    // 非结构网格：一个三角形，球心固定在原点
    {
        Step("unstructured mesh");
        auto mesh = iGame::UnstructuredMesh::New();
        mesh->AddPoint(iGame::Point(1.0f, 0.0f, 0.0f));
        mesh->AddPoint(iGame::Point(0.0f, 1.0f, 0.0f));
        mesh->AddPoint(iGame::Point(0.0f, 0.0f, 1.0f));
        auto cells = iGame::CellArray::New();
        auto types = iGame::UnsignedIntArray::New();
        mesh->SetCells(cells, types);
        igIndex triangle[3] = {0, 1, 2};
        mesh->AddCell(triangle, 3, iGame::IG_TRIANGLE);

        auto filter = iGame::TextureMapToSphereFilter::New();
        filter->SetCenter(0.0, 0.0, 0.0);
        filter->SetInput(0, mesh);
        if (!Check(filter->Execute(), "unstructured mesh: Execute()")) { return false; }

        auto out = filter->GetOutput();
        auto tex = GetTexCoords(out, "TextureCoordinates");
        ok &= Check(!tex.IsNull(), "unstructured mesh: output has TextureCoordinates");
        ok &= Check(!iGame::DynamicCast<iGame::UnstructuredMesh>(out).IsNull(),
                    "unstructured mesh: output keeps the mesh type");
        if (!tex.IsNull()) {
            ok &= Check(tex->GetNumberOfElements() == 3, "unstructured mesh: 3 texture coordinates");
            ok &= Check(Near(tex->GetElementValue(0, 0), 0.0) && Near(tex->GetElementValue(0, 1), 0.5),
                        "unstructured mesh: point (1, 0, 0) -> (0, 0.5)");
            ok &= Check(Near(tex->GetElementValue(1, 0), 0.5) && Near(tex->GetElementValue(1, 1), 0.5),
                        "unstructured mesh: point (0, 1, 0) -> (0.5, 0.5)");
        }
    }

    // 体网格：一个四面体，自动球心
    {
        Step("volume mesh");
        auto mesh = iGame::VolumeMesh::New();
        mesh->AddPoint(iGame::Point(1.0f, 0.0f, 0.0f));
        mesh->AddPoint(iGame::Point(0.0f, 1.0f, 0.0f));
        mesh->AddPoint(iGame::Point(-1.0f, 0.0f, 0.0f));
        mesh->AddPoint(iGame::Point(0.0f, 0.0f, 1.0f));
        auto volumes = iGame::CellArray::New();
        mesh->SetVolumes(volumes);
        igIndex tet[4] = {0, 1, 2, 3};
        volumes->AddCellIds(tet, 4);

        auto filter = iGame::TextureMapToSphereFilter::New();
        filter->SetInput(0, mesh);
        if (!Check(filter->Execute(), "volume mesh: Execute()")) { return false; }

        auto out = filter->GetOutput();
        auto tex = GetTexCoords(out, "TextureCoordinates");
        ok &= Check(!tex.IsNull(), "volume mesh: output has TextureCoordinates");
        ok &= Check(!iGame::DynamicCast<iGame::VolumeMesh>(out).IsNull(), "volume mesh: output keeps the mesh type");
        if (!tex.IsNull()) {
            ok &= Check(tex->GetNumberOfElements() == 4, "volume mesh: 4 texture coordinates");
            // 回归：非均匀点分布下，包围盒中心与 ParaView 的点均值不同，导致 UV 错位。
            // 四点均值为 (0,0.25,0.25)，必须区别于包围盒中心 (0,0.5,0.5)。
            // 修复提交：待提交。
            ok &= Check(Near(filter->GetCenter()[0], 0.0) &&
                        Near(filter->GetCenter()[1], 0.25) && Near(filter->GetCenter()[2], 0.25),
                        "volume mesh: center = mean of all input points");
        }
    }

    // 点集（点云）
    {
        Step("point set");
        auto cloud = iGame::PointSet::New();
        cloud->AddPoint(iGame::Point(1.0f, 0.0f, 0.0f));
        cloud->AddPoint(iGame::Point(0.0f, 1.0f, 0.0f));

        auto filter = iGame::TextureMapToSphereFilter::New();
        filter->SetCenter(0.0, 0.0, 0.0);
        filter->SetInput(0, cloud);
        if (!Check(filter->Execute(), "point set: Execute()")) { return false; }

        auto out = filter->GetOutput();
        auto tex = GetTexCoords(out, "TextureCoordinates");
        ok &= Check(!tex.IsNull(), "point set: output has TextureCoordinates");
        ok &= Check(!iGame::DynamicCast<iGame::PointSet>(out).IsNull(), "point set: output keeps the mesh type");
        if (!tex.IsNull()) {
            ok &= Check(tex->GetNumberOfElements() == 2, "point set: 2 texture coordinates");
            ok &= Check(Near(tex->GetElementValue(1, 0), 0.5) && Near(tex->GetElementValue(1, 1), 0.5),
                        "point set: point (0, 1, 0) -> (0.5, 0.5)");
        }
    }

    return ok;
}

// 场景 7：独立输出，输入不能被修改
bool TestInputNotModified() {
    std::cout << "\n== Test 7: independent output (input is not modified) ==" << std::endl;

    auto mesh = MakeOctahedron(0.0, 0.0, 0.0);
    auto pressure = iGame::FloatArray::New();
    pressure->SetName("Pressure");
    pressure->Resize(6);
    for (IGsize i = 0; i < 6; i++) { pressure->SetValue(i, static_cast<double>(i) + 1.0); }
    mesh->GetAttributeSet()->AddScalar(IG_POINT, pressure);

    auto filter = iGame::TextureMapToSphereFilter::New();
    filter->SetInput(0, mesh);
    if (!Check(filter->Execute(), "filter Execute()")) { return false; }

    bool ok = true;
    auto out = filter->GetOutput();

    Step("check input");
    ok &= Check(mesh->GetAttributeSet()->GetAttributeIndex("TextureCoordinates") < 0,
                "input has no TextureCoordinates after Execute()");
    ok &= Check(mesh->GetNumberOfPoints() == 6, "input still has 6 points");
    ok &= Check(mesh->GetNumberOfFaces() == 8, "input still has 8 faces");

    Step("check output");
    ok &= Check(out.get() != static_cast<iGame::DataObject*>(mesh.get()), "output is a different object");
    auto outAsPointSet = iGame::DynamicCast<iGame::PointSet>(out);
    ok &= Check(!outAsPointSet.IsNull() && outAsPointSet->GetNumberOfPoints() == 6, "output has 6 points");
    ok &= Check(out->GetAttributeSet()->GetAttributeIndex("Pressure") >= 0,
                "output keeps the original point attribute Pressure");
    ok &= Check(!GetTexCoords(out, "TextureCoordinates").IsNull(), "output has TextureCoordinates");

    // 输出的点坐标要与输入一致（本 filter 只加纹理坐标，不动几何）
    auto outPoints = out->GetPoints();
    auto inPoints = mesh->GetPoints();
    if (!Check(!outPoints.IsNull() && !inPoints.IsNull(), "both input and output have points")) { return false; }
    bool samePoints = true;
    for (IGsize i = 0; i < 6; i++) {
        const auto& a = inPoints->GetPoint(i);
        const auto& b = outPoints->GetPoint(i);
        if (!Near(a[0], b[0]) || !Near(a[1], b[1]) || !Near(a[2], b[2])) { samePoints = false; }
    }
    ok &= Check(samePoints, "output point coordinates equal the input ones");
    return ok;
}

// 场景 8：旋转轴上的点（极点 / 球心）
bool TestPointsOnAxis() {
    std::cout << "\n== Test 8: points on the rotation axis ==" << std::endl;

    auto makeAxisCloud = []() {
        auto cloud = iGame::PointSet::New();
        cloud->SetName("AxisPoints");
        cloud->AddPoint(iGame::Point(0.0f, 0.0f, 1.0f));  // 0 北极
        cloud->AddPoint(iGame::Point(0.0f, 0.0f, -1.0f)); // 1 南极
        cloud->AddPoint(iGame::Point(0.0f, 0.0f, 2.0f));  // 2 北极方向
        cloud->AddPoint(iGame::Point(0.0f, 0.0f, -3.0f)); // 3 南极方向
        cloud->AddPoint(iGame::Point(0.0f, 0.0f, 0.0f));  // 4 正好是球心
        return cloud;
    };

    const double seamOn[5][2] = {
            {0.00, 0.00}, {0.50, 1.00}, {0.00, 0.00}, {0.50, 1.00}, {0.00, 0.00},
    };
    const double seamOff[5][2] = {
            {0.00, 0.00}, {0.25, 1.00}, {0.00, 0.00}, {0.25, 1.00}, {0.00, 0.00},
    };

    bool ok = true;
    ok &= CheckTable(makeAxisCloud(), 0.0, 0.0, 0.0, true, true, seamOn, 5,
                     "Test 8a: axis points, PreventSeam on");
    ok &= CheckTable(makeAxisCloud(), 0.0, 0.0, 0.0, true, false, seamOff, 5,
                     "Test 8b: axis points, PreventSeam off");
    return ok;
}

// 场景 9：演示模型，逐点和 ParaView 参考值对比
// 参考值来自 参考值_生成与核对_ParaView.py（pvpython 跑 vtkTextureMapToSphere）
const double PARAVIEW_DEMO_SEAM_ON[18][2] = {
        {0.000000, 0.000000}, {0.000000, 0.333333}, {0.250000, 0.333333}, {0.500000, 0.333333},
        {0.750000, 0.333333}, {1.000000, 0.333333}, {0.750000, 0.333333}, {0.500000, 0.333333},
        {0.250000, 0.333333}, {0.000000, 0.666667}, {0.250000, 0.666667}, {0.500000, 0.666667},
        {0.750000, 0.666667}, {1.000000, 0.666667}, {0.750000, 0.666667}, {0.500000, 0.666667},
        {0.250000, 0.666667}, {0.500000, 1.000000},
};
const double PARAVIEW_DEMO_SEAM_OFF[18][2] = {
        {0.000000, 0.000000}, {0.000000, 0.333333}, {0.125000, 0.333333}, {0.250000, 0.333333},
        {0.375000, 0.333333}, {0.500000, 0.333333}, {0.625000, 0.333333}, {0.750000, 0.333333},
        {0.875000, 0.333333}, {0.000000, 0.666667}, {0.125000, 0.666667}, {0.250000, 0.666667},
        {0.375000, 0.666667}, {0.500000, 0.666667}, {0.625000, 0.666667}, {0.750000, 0.666667},
        {0.875000, 0.666667}, {0.250000, 1.000000},
};

bool TestDemoModelAgainstParaView() {
    std::cout << "\n== Test 9: demo model vs ParaView reference values ==" << std::endl;

    const std::string model = FindModel();
    if (model.empty()) {
        std::cout << "  [SKIP] model file not found (run the test from the build Examples folder)" << std::endl;
        return true;
    }
    std::cout << "  [info] model = " << model << std::endl;

    Step("read model");
    auto input = iGame::FileIO::ReadFile(model);
    if (!Check(!input.IsNull(), "read model")) { return false; }
    auto inputAsPointSet = iGame::DynamicCast<iGame::PointSet>(input);
    if (!Check(!inputAsPointSet.IsNull() && inputAsPointSet->GetNumberOfPoints() == 18, "model has 18 points")) {
        return false;
    }

    bool ok = true;
    ok &= CheckTable(input, 0.0, 0.0, 0.0, true, true, PARAVIEW_DEMO_SEAM_ON, 18,
                     "Test 9a: demo model, PreventSeam on (= ParaView default)");
    ok &= CheckTable(input, 0.0, 0.0, 0.0, true, false, PARAVIEW_DEMO_SEAM_OFF, 18,
                     "Test 9b: demo model, PreventSeam off");

    Step("check that the input model was not touched");
    ok &= Check(input->GetAttributeSet()->GetAttributeIndex("TextureCoordinates") < 0,
                "input model has no TextureCoordinates after both runs");
    return ok;
}

} // namespace

int main() {
    bool ok = true;
    ok &= TestDefaultSettings();
    ok &= TestPreventSeamOff();
    ok &= TestAutoCenterTranslated();
    ok &= TestExplicitCenter();
    ok &= TestOffCenterSphere();
    ok &= TestOtherMeshTypes();
    ok &= TestInputNotModified();
    ok &= TestPointsOnAxis();
    ok &= TestDemoModelAgainstParaView();

    if (ok) {
        std::cout << "\nALL TESTS PASSED" << std::endl;
        return 0;
    }
    std::cout << "\nSOME TESTS FAILED" << std::endl;
    return 1;
}
