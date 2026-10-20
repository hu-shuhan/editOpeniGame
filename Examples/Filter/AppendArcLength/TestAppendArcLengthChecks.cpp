// 迁移回归（待提交）：标准菜单原先缺少该功能；保留源仓库数值、输入保留和异常边界检查，
// 防止仅接入菜单或漏迁依赖导致行为退化。交互演示保留原启动方式。
// ============================================================================
// AppendArcLength 数值/逻辑一致性回归检查（无窗口，可直接进 CI）
//
// 基础测试模型：Models/AppendArcLength_test.vtk（8 点）
//   折线 A：(0,0,0)-(3,0,0)-(3,4,0)  → 弧长 0 / 3 / 7
//   折线 B：(10,0,0)-(13,0,0)        → 弧长 0 / 3
//   三角形：(1,5,0)-(2,6,0)-(3,7,0)  → 不是线单元，保持 0
//
// 期望值由 VTK vtkAppendArcLength 实测取得（ParaView 6.2 的 vtkAppendArcLength，
//   输出 vtkFloatArray "arc_length"，1 分量）：[0, 3, 7, 0, 3, 0, 0, 0]
//
// 失败时返回非 0，成功返回 0。运行方式（工作目录为 Examples 构建目录）：
//     ./testAppendArcLengthChecks
// ============================================================================
#include <AppendArcLength/iGameAppendArcLengthFilter.h>
#include <VTK/iGameGhostVTKReader.h>
#include <VTK/iGameVTKReader.h>
#include <iGameAttributeSet.h>
#include <iGameFileIO.h>
#include <iGamePointSet.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace iGame;

namespace
{

int g_failed = 0;
int g_total = 0;

// CPU conversion only: expose the existing drawing indices without creating GL resources.
class SurfaceMeshDrawProbe : public SurfaceMesh {
public:
    static SmartPointer<SurfaceMeshDrawProbe> New() { return new SurfaceMeshDrawProbe; }
    std::vector<iguIndex> PointIds() {
        std::vector<iguIndex> ids;
        for (IGsize i = 0; i < m_PointIndices->GetNumberOfValues(); ++i) { ids.push_back(m_PointIndices->GetValue(i)); }
        return ids;
    }
    IGsize LineSegmentCount() { return m_LineIndices->GetNumberOfElements(); }
    IGsize TriangleCount() { return m_TriangleIndices->GetNumberOfElements(); }
    bool NeedsLineGeometry(IGenum style) { return NeedsExplicitWireframeGeometry(style); }
};

/// 定位测试模型：依次尝试「当前工作目录 / exe 同目录 / 仓库相对路径」，便于直接双击或任意目录运行。
std::string ResolveModelPath(const char* executable, const std::string& relative) {
    std::vector<std::string> candidates;
    candidates.push_back(relative);
    candidates.push_back("Examples/" + relative);
    candidates.push_back("../Examples/" + relative);
    candidates.push_back("../../Examples/" + relative);
    if (executable != nullptr) {
        const std::string exePath(executable);
        const size_t separator = exePath.find_last_of("/\\");
        if (separator != std::string::npos) { candidates.push_back(exePath.substr(0, separator + 1) + relative); }
    }
    for (const auto& candidate: candidates) {
        std::ifstream file(candidate);
        if (file.good()) { return candidate; }
    }
    return candidates.front();
}


void Check(bool condition, const std::string& what) {
    ++g_total;
    if (!condition) { ++g_failed; }
    std::cout << (condition ? "  [ OK ] " : "  [FAIL] ") << what << "\n";
}

int CountArraysNamed(DataObject::Pointer object, const std::string& name, IGenum attachmentType) {
    AttributeSet* attributeSet = object ? object->GetAttributeSet() : nullptr;
    if (attributeSet == nullptr) { return 0; }
    int count = 0;
    for (IGsize i = 0; i < attributeSet->GetNumberOfAttributes(); ++i) {
        auto& attribute = attributeSet->GetAttribute(i);
        if (attribute.isDeleted || attribute.pointer == nullptr) { continue; }
        if (attribute.attachmentType != attachmentType) { continue; }
        if (attribute.pointer->GetName() == name) { ++count; }
    }
    return count;
}

ArrayObject::Pointer FindArray(DataObject::Pointer object, const std::string& name) {
    AttributeSet* attributeSet = object ? object->GetAttributeSet() : nullptr;
    if (attributeSet == nullptr) { return nullptr; }
    for (IGsize i = 0; i < attributeSet->GetNumberOfAttributes(); ++i) {
        auto& attribute = attributeSet->GetAttribute(i);
        if (attribute.isDeleted || attribute.pointer == nullptr) { continue; }
        if (attribute.pointer->GetName() == name) { return attribute.pointer; }
    }
    return nullptr;
}

const std::vector<double> kExpected = {0.0, 3.0, 7.0, 0.0, 3.0, 0.0, 0.0, 0.0};

#ifdef _WIN32
extern "C" __declspec(dllimport) unsigned long __stdcall GetConsoleProcessList(unsigned long* processList,
                                                                               unsigned long count);
#endif

/// 双击运行时控制台只属于本进程，此时暂停等待回车，避免"一闪而过"；脚本/CI（共享控制台或无控制台、
/// 输出被重定向）自动跳过，不会卡住流水线。`--pause` 强制暂停，`--no-pause` 强制不暂停。
void PauseIfNeeded(int argc, char** argv) {
    bool force = false;
    bool skip = false;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--pause") { force = true; }
        if (argument == "--no-pause") { skip = true; }
    }
    if (skip) { return; }
    bool ownConsole = false;
#ifdef _WIN32
    unsigned long processes[4] = {0, 0, 0, 0};
    ownConsole = GetConsoleProcessList(processes, 4) == 1;
#endif
    if (!force && !ownConsole) { return; }
    std::cout << "\n按回车键退出..." << std::flush;
    std::cin.get();
}


bool SameValues(ArrayObject::Pointer array, const std::vector<double>& want) {
    if (!array) { return false; }
    const int dimension = std::max(1, array->GetDimension());
    if (array->GetNumberOfValues() != want.size() * static_cast<size_t>(dimension)) { return false; }
    for (size_t i = 0; i < want.size(); ++i) {
        if (std::fabs(array->GetElementValue(i, 0) - want[i]) > 1e-6) { return false; }
    }
    return true;
}

// BUG (2026-10-14, 修复提交 d6fdeb20): arc_length_lines.vtk 的点数显示与 ParaView 不一致，
// 且过滤器原地修改输入，模型树没有新结果。保留重复坐标和孤立点，按原 point id
// 验证全部 12 个点及共享端点的值；输出必须独立，原模型和已有属性不能改变。
void CheckLinesOnlyRegression(const std::string& modelPath) {
    static const char vtk[] = R"vtk(# vtk DataFile Version 3.0
arc length alignment case (LINES only)
ASCII
DATASET POLYDATA
POINTS 12 float
0 0 0
1 0 0
1 2 0
0 0 0
1 1 0
2 1 0
3 1 0
5 5 5
-1 0 0
-1 -1 0
10 0 0
10 0 3
LINES 4 15
3 0 1 2
4 3 4 5 6
2 10 11
2 11 1
)vtk";
    auto reader = VTKReader::New();
    reader->SetMemoryBuffer(vtk, std::strlen(vtk));
    const bool read = reader->Execute();
    auto input = read ? reader->GetOutput() : nullptr;
    if (!modelPath.empty()) { input = GhostVTKReader::ReadFile(modelPath); }
    auto mesh = DynamicCast<SurfaceMesh>(input);
    Check(mesh && mesh->GetNumberOfPoints() == 12, "LINES 文件保留全部 12 个点（含重复坐标和孤立点）");
    Check(mesh && mesh->GetNumberOfEdges() == 4, "LINES 文件保留 4 条完整折线");
    if (!mesh || mesh->GetNumberOfPoints() != 12) { return; }
    std::cout << "LINES input points=" << mesh->GetNumberOfPoints() << "\n";
    const auto pointTime = mesh->GetPoints()->GetMTime();
    auto filter = AppendArcLengthFilter::New();
    filter->SetInput(input);
    Check(filter->Execute(), "LINES 追加弧长成功");
    auto output = filter->GetOutput();
    auto result = DynamicCast<SurfaceMesh>(output);
    Check(result && output != input && output->GetDataObjectId() != input->GetDataObjectId(),
          "结果为独立模型，可在模型树登记新节点");
    Check(result && result->GetNumberOfPoints() == 12 && result->GetNumberOfEdges() == 4,
          "输出保留 12 个点和 4 条折线");
    Check(FindArray(input, "arc_length") == nullptr, "原模型不新增 arc_length");
    Check(mesh->GetPoints()->GetMTime() == pointTime, "执行不修改输入坐标时间戳");
    const std::vector<double> expected = {
            0, std::sqrt(90.0), 3, 0, std::sqrt(2.0), std::sqrt(2.0) + 1, std::sqrt(2.0) + 2, 0, 0, 0, 0, 3};
    Check(SameValues(FindArray(output, "arc_length"), expected), "12 个点逐点匹配 ParaView 参考（共享首点 11 保持 3）");
    Check(filter->GetNumberOfProcessedLines() == 4, "实际处理 4 条折线");

    // BUG: DrawArrays displayed three unreferenced points as well as the line
    // vertices. Statistics must stay at 12 while point drawing uses 9 IDs;
    // coincident coordinates of IDs 0 and 3 result in 8 visible positions.
    auto draw = SurfaceMeshDrawProbe::New();
    auto drawPoints = Points::New();
    drawPoints->DeepCopy(mesh->GetPoints());
    draw->SetPoints(drawPoints);
    draw->SetFaces(mesh->GetFaces());
    draw->SetEdges(mesh->GetEdges());
    draw->SetViewStyle(IG_POINTS);
    draw->ConvertToDrawableData();
    const std::vector<iguIndex> expectedIds = {0, 1, 2, 3, 4, 5, 6, 10, 11};
    Check(draw->PointIds() == expectedIds, "点绘制只引用 9 个折线顶点（8 个可见位置），不绘制孤立点 7/8/9");
    Check(draw->GetNumberOfPoints() == 12, "点绘制不删除原始点，统计仍为 12");
    draw->SetViewStyle(IG_WIREFRAME);
    draw->ConvertToDrawableData();
    draw->SetViewStyle(IG_POINTS);
    draw->ConvertToDrawableData();
    Check(draw->PointIds() == expectedIds, "切换线框和点显示后引用集合不变");
    draw->ReleaseDrawableResources();

    // Existing point arc_length must be replaced only in the result. A cell
    // array with the same name belongs to a different association and survives.
    auto oldArc = FloatArray::New();
    oldArc->SetName("arc_length");
    oldArc->Resize(12);
    for (IGsize i = 0; i < 12; ++i) { oldArc->SetValue(i, 42); }
    input->GetAttributeSet()->AddScalar(IG_POINT, oldArc);
    auto cellArc = FloatArray::New();
    cellArc->SetName("arc_length");
    cellArc->Resize(4);
    for (IGsize i = 0; i < 4; ++i) { cellArc->SetValue(i, 8); }
    input->GetAttributeSet()->AddScalar(IG_CELL, cellArc);
    auto originalIds = IntArray::New();
    originalIds->SetName("original_ids");
    originalIds->Resize(12);
    for (IGsize i = 0; i < 12; ++i) { originalIds->SetValue(i, static_cast<int>(i)); }
    input->GetAttributeSet()->AddScalar(IG_POINT, originalIds);
    Check(filter->Execute(), "输入已有同名点和单元属性时执行成功");
    output = filter->GetOutput();
    Check(CountArraysNamed(output, "arc_length", IG_POINT) == 1 && CountArraysNamed(output, "arc_length", IG_CELL) == 1,
          "结果替换同名点数组，同时保留同名单元数组");
    Check(oldArc->GetValue(2) == 42 && cellArc->GetValue(0) == 8, "输入同名属性值不变");
    auto copiedIds = DynamicCast<IntArray>(FindArray(output, "original_ids"));
    Check(copiedIds && copiedIds != originalIds && copiedIds->GetValue(11) == 11, "继承独立的整数点属性，类型和值不变");
    auto outputCellArc = output->GetAttributeSet()->GetArrayPointer(IG_SCALAR, IG_CELL, "arc_length");
    Check(outputCellArc && outputCellArc != cellArc.get() && outputCellArc->GetElementValue(0, 0) == 8,
          "结果继承独立的单元属性数组");
    Check(SameValues(output->GetAttributeSet()->GetArrayPointer(IG_SCALAR, IG_POINT, "arc_length"), expected),
          "同名覆盖后点数组仍为正确弧长");
    filter->SetInput(output);
    Check(filter->Execute() && CountArraysNamed(filter->GetOutput(), "arc_length", IG_POINT) == 1,
          "对输出再次追加不堆积同名点数组");
    filter->SetInput(nullptr);
    Check(!filter->Execute() && !filter->GetOutput(), "空输入失败且清除上一次输出");

    auto faceOnly = SurfaceMesh::New();
    faceOnly->SetPoints(drawPoints);
    auto faces = CellArray::New();
    faces->AddCellId3(0, 1, 2);
    faceOnly->SetFaces(faces);
    faceOnly->BuildEdges();
    filter->SetInput(faceOnly);
    Check(filter->Execute() && filter->GetNumberOfProcessedLines() == 0 &&
                  SameValues(FindArray(filter->GetOutput(), "arc_length"), std::vector<double>(12, 0)),
          "面派生边不当作折线，结果为全零");
    filter->SetInput(filter->GetOutput());
    Check(filter->Execute() && filter->GetNumberOfProcessedLines() == 0, "对无折线的输出再次执行仍为全零");

    auto longLine = UnstructuredMesh::New();
    std::vector<igIndex> longIds(2000);
    for (int i = 0; i < 2000; ++i) {
        longLine->AddPoint(Point{static_cast<float>(i), 0, 0});
        longIds[i] = i;
    }
    longLine->AddCell(longIds.data(), static_cast<int>(longIds.size()), IG_POLY_LINE);
    filter->SetInput(longLine);
    Check(filter->Execute(), "2000 点非结构化折线执行成功");
    auto longOutput = DynamicCast<UnstructuredMesh>(filter->GetOutput());
    Check(longOutput && longOutput->GetNumberOfPoints() == 2000 && longOutput->GetNumberOfCells() == 1 &&
                  FindArray(longOutput, "arc_length")->GetElementValue(1999, 0) == 1999,
          "长折线输出保留点数、连通性和末点累计弧长");
}

} // namespace

// BUG (2026-10-15, 修复提交 d6fdeb20): LINES + POLYGONS 在 Surface 中只显示三角形；
// Surface+Wireframe 的单通道三角形边绘制也丢失独立折线。验证实际混合模型的绘制策略、
// CPU 三角形/折线索引、表示切换以及过滤器输出；不创建 OpenGL 上下文，画面由用户验证。
void CheckMixedSurfaceRegression(const std::string& modelPath) {
    static const char vtk[] = R"vtk(# vtk DataFile Version 3.0
arc length mixed LINES+POLYGONS case
ASCII
DATASET POLYDATA
POINTS 12 float
0 0 0
1 0 0
1 2 0
0 0 0
1 1 0
2 1 0
3 1 0
5 5 5
-1 0 0
-1 -1 0
10 0 0
10 0 3
LINES 4 15
3 0 1 2
4 3 4 5 6
2 10 11
2 11 1
POLYGONS 1 4
3 7 8 9
)vtk";
    auto reader = VTKReader::New();
    reader->SetMemoryBuffer(vtk, std::strlen(vtk));
    const bool read = reader->Execute();
    auto input = read ? reader->GetOutput() : nullptr;
    if (!modelPath.empty()) { input = GhostVTKReader::ReadFile(modelPath); }
    auto mesh = DynamicCast<SurfaceMesh>(input);
    Check(mesh && mesh->GetNumberOfPoints() == 12 && mesh->GetNumberOfEdges() == 4 && mesh->GetNumberOfFaces() == 1,
          "混合模型读取为 12 点、4 条折线和 1 个三角形");
    if (!mesh) { return; }
    auto makeProbe = [](SurfaceMesh::Pointer source) {
        auto draw = SurfaceMeshDrawProbe::New();
        auto points = Points::New();
        points->DeepCopy(source->GetPoints());
        draw->SetPoints(points);
        draw->SetEdges(source->GetEdges());
        draw->SetFaces(source->GetFaces());
        return draw;
    };
    auto draw = makeProbe(mesh);
    draw->SetViewStyle(IG_SURFACE);
    draw->ConvertToDrawableData();
    Check(draw->IsLineRenderingEnabled() && draw->TriangleCount() == 1 && draw->LineSegmentCount() == 7,
          "Surface 同时提交 1 个三角形和全部 7 个折线段");
    draw->AddViewStyle(IG_WIREFRAME);
    draw->ConvertToDrawableData();
    Check(draw->IsLineRenderingEnabled() && !draw->IsUseSinglePassWireframeRendering() &&
                  draw->LineSegmentCount() == 10 && draw->TriangleCount() == 1,
          "Surface+Wireframe 保留折线，并额外绘制三角形 3 条边");
    draw->RemoveViewStyle(IG_WIREFRAME);
    draw->ConvertToDrawableData();
    Check(draw->LineSegmentCount() == 7 && draw->TriangleCount() == 1, "关闭线框后恢复 Surface，独立折线仍显示");
    draw->SetViewStyle(IG_POINTS);
    draw->ConvertToDrawableData();
    Check(!draw->IsLineRenderingEnabled() && draw->PointIds().size() == 12,
          "纯点表示不意外叠加折线，包含面及折线引用的全部点");
    draw->SetRemoteRenderingEnabled(true);
    draw->SetViewStyle(IG_SURFACE);
    draw->ConvertToDrawableData();
    Check(draw->NeedsLineGeometry(IG_SURFACE) && draw->LineSegmentCount() == 7 && draw->IsLineRenderingEnabled(),
          "远程渲染 Surface 也准备独立折线索引");
    draw->ReleaseDrawableResources();

    auto filter = AppendArcLengthFilter::New();
    filter->SetInput(input);
    Check(filter->Execute(), "混合模型追加弧长成功");
    auto output = DynamicCast<SurfaceMesh>(filter->GetOutput());
    if (!output) {
        Check(false, "混合模型产出表面网格");
        return;
    }
    draw = makeProbe(output);
    auto attributes = AttributeSet::New();
    attributes->DeepCopy(output->GetAttributeSet());
    draw->SetAttributeSet(attributes);
    draw->SetAttributeIndex(attributes->GetAttributeIndex("arc_length"));
    draw->SetViewStyle(IG_SURFACE);
    draw->ConvertToDrawableData();
    Check(draw->IsLineRenderingEnabled() && draw->TriangleCount() == 1 && draw->LineSegmentCount() == 7,
          "追加弧长结果的 Surface 同时保留面和着色折线");
    auto arc = FindArray(output, "arc_length");
    Check(arc && arc->GetElementValue(7, 0) == 0 && arc->GetElementValue(8, 0) == 0 &&
                  arc->GetElementValue(9, 0) == 0 && filter->GetNumberOfProcessedLines() == 4,
          "显示模式不改变折线单元，三角形顶点弧长仍为 0");
    draw->ReleaseDrawableResources();

    auto faceOnly = SurfaceMeshDrawProbe::New();
    auto points = Points::New();
    points->DeepCopy(mesh->GetPoints());
    faceOnly->SetPoints(points);
    faceOnly->SetFaces(mesh->GetFaces());
    faceOnly->SetViewStyle(IG_SURFACE);
    faceOnly->ConvertToDrawableData();
    Check(!faceOnly->HasExplicitLineCells() && !faceOnly->IsLineRenderingEnabled(), "纯面网格 Surface 不叠加线框");
    faceOnly->AddViewStyle(IG_WIREFRAME);
    faceOnly->ConvertToDrawableData();
    Check(faceOnly->IsUseSinglePassWireframeRendering(), "普通面网格保留单通道线框优化");
    faceOnly->ReleaseDrawableResources();
}

int main(int argc, char** argv) {
    std::cout << "=== AppendArcLength 回归检查（Models/AppendArcLength_test.vtk）===\n";

    const std::string modelPath = ResolveModelPath(argc > 0 ? argv[0] : nullptr, "Models/AppendArcLength_test.vtk");
    std::cout << "模型：" << modelPath << "\n";
    DataObject::Pointer input = FileIO::ReadFile(modelPath);
    Check(input != nullptr, "读取 " + modelPath);
    if (!input) { return 1; }

    auto points = DynamicCast<PointSet>(input);
    Check(points != nullptr && points->GetNumberOfPoints() == 8, "输入有 8 个点");

    auto filter = AppendArcLengthFilter::New();
    filter->SetInput(input);
    const bool executed = filter->Execute();
    Check(executed, "Execute() 成功");
    if (!executed) { return 1; }

    DataObject::Pointer output = filter->GetOutput(0);
    Check(output != nullptr, "有输出数据");
    if (!output) { return 1; }

    auto outputPoints = DynamicCast<PointSet>(output);
    Check(outputPoints != nullptr && outputPoints->GetNumberOfPoints() == 8, "输出保留 8 个点");

    std::cout << "\n=== 1. arc_length 数值（VTK vtkAppendArcLength 参考）===\n";
    ArrayObject::Pointer arcLength = FindArray(output, "arc_length");
    Check(arcLength != nullptr, "产出点属性数组 arc_length");
    if (arcLength) {
        Check(arcLength->GetDimension() == 1, "维数 = 1");
        Check(CountArraysNamed(output, "arc_length", IG_POINT) == 1, "点属性里只有一份 arc_length");
        Check(SameValues(arcLength, kExpected), "数值 = [0, 3, 7, 0, 3, 0, 0, 0]（两条折线累加，三角形顶点保持 0）");
        Check(std::fabs(arcLength->GetElementValue(2, 0) - 7.0) < 1e-6, "折线 A 末点 = 3 + 4 = 7");
        Check(std::fabs(arcLength->GetElementValue(4, 0) - 3.0) < 1e-6, "折线 B 末点 = 3");
        Check(std::fabs(arcLength->GetElementValue(5, 0)) < 1e-6 &&
                      std::fabs(arcLength->GetElementValue(6, 0)) < 1e-6 &&
                      std::fabs(arcLength->GetElementValue(7, 0)) < 1e-6,
              "非线单元（三角形）的顶点保持 0");
    }

    std::cout << "\n=== 2. 重复执行按覆盖语义处理（不堆积同名数组）===\n";
    Check(filter->Execute(), "再次 Execute() 成功");
    DataObject::Pointer second = filter->GetOutput(0);
    Check(CountArraysNamed(second, "arc_length", IG_POINT) == 1, "仍然只有一份 arc_length");
    Check(SameValues(FindArray(second, "arc_length"), kExpected), "数值不变");

    std::string linesPath;
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::string(argv[i]) == "--lines-model") { linesPath = argv[++i]; }
    }
    CheckLinesOnlyRegression(linesPath);
    std::string mixedPath;
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::string(argv[i]) == "--mixed-model") { mixedPath = argv[++i]; }
    }
    CheckMixedSurfaceRegression(mixedPath);

    std::cout << "\n结果：通过 " << (g_total - g_failed) << " / " << g_total << "，失败 " << g_failed << "\n";
    std::cout << (g_failed == 0 ? "[PASS] AppendArcLength 回归检查全部通过\n"
                                : "[FAIL] AppendArcLength 回归检查存在失败项\n");
    PauseIfNeeded(argc, argv);
    return g_failed == 0 ? 0 : 1;
}
