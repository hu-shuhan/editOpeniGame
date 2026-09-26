// Find the integration commit: git log --diff-filter=A --format="%h %s" -- Examples/Filter/TestGenerateProcessIds.cpp
// Regression/example imported from dayuwan77/igamevis at
// eccac729b57aeacbe9312d7d5189f6990bb4eebd (same relative path).
// Integration regression: these filters and their example assets were missing
// from iGameVis-multiFilter. The checks below cover their output/attribute and
// geometry contracts; visual examples retain an interactive default mode.
// Local integration fix: feat: integrate second-batch standard filters.
#include <iostream>
#include <iGameCellArray.h>
#include <iGameFileIO.h>
#include <iGamePoints.h>
#include <iGamePointSet.h>
#include <iGameSurfaceMesh.h>
#include <iGameUnstructuredMesh.h>
#include <iGameVolumeMesh.h>
#include <ProcessGet/iGameGenerateProcessIdsFilter.h>

IGAME_NAMESPACE_BEGIN
// 模拟多进程分区场景：第 i 个点/单元属于进程 (i % 2)。
// 未来接入真实并行/分区机制时，子类以同样方式重写这两个方法即可，Execute 无需改动。
class MockPartitionedProcessIdsFilter : public GenerateProcessIdsFilter {
    I_OBJECT(MockPartitionedProcessIdsFilter)

public:
    static Pointer New() { return new MockPartitionedProcessIdsFilter; }
    MockPartitionedProcessIdsFilter() = default;

protected:
    long long GetPointProcessId(IGsize index) override { return index % 2; }
    long long GetCellProcessId(IGsize index) override { return index % 2; }
};
IGAME_NAMESPACE_END

namespace {

// 统计指定挂载类型上同名数组的个数：用于校验输入未被写入、结果中同名数组唯一
int CountArrays(iGame::DataObject::Pointer object, bool pointData, const std::string& arrayName) {
    if (object == nullptr || object->GetAttributeSet() == nullptr) return 0;
    auto attrs = pointData ? object->GetAttributeSet()->GetAllPointAttributes()
                           : object->GetAttributeSet()->GetAllCellAttributes();
    int count = 0;
    if (attrs != nullptr) {
        for (int i = 0; i < attrs->GetNumberOfElements(); ++i) {
            auto arr = attrs->GetElement(i).pointer;
            if (arr != nullptr && arr->GetName() == arrayName) ++count;
        }
    }
    return count;
}

// 输入点集的几何时间戳：用于校验执行后输入没有被标记为"已修改"
// （输入被标记修改会连累输入模型重跑表面提取，正是输出结果后卡顿的来源）
unsigned int InputPointsMTime(iGame::DataObject::Pointer object) {
    auto pointSet = iGame::DynamicCast<iGame::PointSet>(object);
    if (pointSet == nullptr || pointSet->GetPoints() == nullptr) { return 0; }
    return pointSet->GetPoints()->GetMTime();
}

// 独立输出节点的公共校验：结果非空、是新对象、类型不变、几何共享、输入未被修改、结果数组唯一
bool VerifyIndependentOutput(iGame::DataObject::Pointer input, iGame::DataObject::Pointer output, bool pointData,
                             const std::string& arrayName) {
    const char* label = pointData ? "point" : "cell";
    if (output == nullptr) {
        std::cout << "FAIL: " << label << " output is null\n";
        return false;
    }
    if (output == input) {
        std::cout << "FAIL: " << label << " output should be a new DataObject\n";
        return false;
    }
    if (output->GetDataObjectType() != input->GetDataObjectType()) {
        std::cout << "FAIL: " << label << " output type should stay unchanged\n";
        return false;
    }
    auto inputPointSet = iGame::DynamicCast<iGame::PointSet>(input);
    auto outputPointSet = iGame::DynamicCast<iGame::PointSet>(output);
    if (inputPointSet == nullptr || outputPointSet == nullptr || inputPointSet->GetPoints() == nullptr ||
        outputPointSet->GetPoints() == nullptr) {
        std::cout << "FAIL: " << label << " output should keep a point set\n";
        return false;
    }
    // 几何数据共享（底层缓冲相同），但点集对象独立：这样不会顶掉输入模型的几何时间戳
    if (inputPointSet->GetPoints() == outputPointSet->GetPoints()) {
        std::cout << "FAIL: " << label << " output should own an independent point set\n";
        return false;
    }
    if (inputPointSet->GetPoints()->RawPointer() != outputPointSet->GetPoints()->RawPointer()) {
        std::cout << "FAIL: " << label << " output should share input geometry data\n";
        return false;
    }
    if (CountArrays(input, pointData, arrayName) != 0) {
        std::cout << "FAIL: " << label << " input should not be modified\n";
        return false;
    }
    if (CountArrays(output, pointData, arrayName) != 1) {
        std::cout << "FAIL: " << label << " result array should be unique\n";
        return false;
    }
    return true;
}

// 结果数组逐元素校验（含挂载类型）
template <typename ExpectValue>
bool VerifyResultValues(iGame::DataObject::Pointer output, bool pointData, const std::string& arrayName,
                        IGsize expectCount, ExpectValue expectValue) {
    auto& attr = output->GetAttributeSet()->GetScalar(arrayName);
    auto arr = attr.pointer;
    bool ok = (arr != nullptr) && (arr->GetNumberOfElements() == expectCount) &&
              (attr.attachmentType == (pointData ? IG_POINT : IG_CELL));
    for (IGsize i = 0; ok && i < expectCount; ++i) ok = (arr->GetValue(i) == expectValue(i));
    return ok;
}

// 常数进程号：结果为独立节点，值全部等于常数（对齐 vtkGenerateProcessIds 的官方测试：
// 数组存在、长度等于点数/单元数、每个元素都等于当前进程号）
bool VerifyConstant(iGame::DataObject::Pointer mesh, bool pointData, const std::string& arrayName, IGsize expectCount,
                    int expectValue) {
    const unsigned int pointsMTimeBefore = InputPointsMTime(mesh);
    auto filter = iGame::GenerateProcessIdsFilter::New();
    filter->SetInput(mesh);
    filter->SetGeneratePointData(pointData);
    filter->SetGenerateCellData(!pointData);
    filter->SetProcessId(expectValue);
    if (!filter->Execute()) {
        std::cout << "FAIL: Execute\n";
        return false;
    }
    if (InputPointsMTime(mesh) != pointsMTimeBefore) {
        std::cout << "FAIL: input geometry should not be marked modified\n";
        return false;
    }
    auto output = filter->GetOutput();
    if (!VerifyIndependentOutput(mesh, output, pointData, arrayName)) return false;
    return VerifyResultValues(output, pointData, arrayName, expectCount,
                              [expectValue](IGsize) { return static_cast<long long>(expectValue); });
}

// 分区进程号：派生类按 index % 2 分配
bool VerifyPartitioned(iGame::DataObject::Pointer mesh, bool pointData, const std::string& arrayName,
                       IGsize expectCount) {
    const unsigned int pointsMTimeBefore = InputPointsMTime(mesh);
    auto filter = iGame::MockPartitionedProcessIdsFilter::New();
    filter->SetInput(mesh);
    filter->SetGeneratePointData(pointData);
    filter->SetGenerateCellData(!pointData);
    if (!filter->Execute()) {
        std::cout << "FAIL: Execute (partitioned)\n";
        return false;
    }
    if (InputPointsMTime(mesh) != pointsMTimeBefore) {
        std::cout << "FAIL: input geometry should not be marked modified (partitioned)\n";
        return false;
    }
    auto output = filter->GetOutput();
    if (!VerifyIndependentOutput(mesh, output, pointData, arrayName)) return false;
    return VerifyResultValues(output, pointData, arrayName, expectCount,
                              [](IGsize i) { return static_cast<long long>(i % 2); });
}

// 重复执行：每次都得到新的独立结果，输入始终不被写入、结果中同名数组始终唯一
bool VerifyRepeatedExecution(iGame::DataObject::Pointer mesh, bool pointData, const std::string& arrayName,
                             IGsize expectCount, int expectValue) {
    const unsigned int pointsMTimeBefore = InputPointsMTime(mesh);
    for (int run = 0; run < 2; ++run) {
        auto filter = iGame::GenerateProcessIdsFilter::New();
        filter->SetInput(mesh);
        filter->SetGeneratePointData(pointData);
        filter->SetGenerateCellData(!pointData);
        filter->SetProcessId(expectValue);
        if (!filter->Execute()) {
            std::cout << "FAIL: Execute (repeated run " << run << ")\n";
            return false;
        }
        if (InputPointsMTime(mesh) != pointsMTimeBefore) {
            std::cout << "FAIL: input geometry should not be marked modified (run " << run << ")\n";
            return false;
        }
        auto output = filter->GetOutput();
        if (!VerifyIndependentOutput(mesh, output, pointData, arrayName)) return false;
        if (!VerifyResultValues(output, pointData, arrayName, expectCount,
                                [expectValue](IGsize) { return static_cast<long long>(expectValue); })) {
            std::cout << "FAIL: repeated run " << run << " values\n";
            return false;
        }
    }
    return true;
}

// 输入自带的 process_id 数组：对齐 vtkGenerateProcessIds —— 结果只写当前进程号，不沿用输入值；
// 该数组本身被原样拷入结果属性集，输入保持不变。
bool VerifyInputProcessIdsIgnored(iGame::DataObject::Pointer mesh, bool pointData, const std::string& arrayName,
                                  IGsize expectCount, int expectValue) {
    auto pidArray = iGame::LongLongArray::New();
    pidArray->SetName("process_id");
    pidArray->Resize(expectCount);
    for (IGsize i = 0; i < expectCount; ++i) pidArray->SetValue(i, static_cast<long long>(i % 3));
    if (pointData) {
        mesh->GetAttributeSet()->AddScalar(IG_POINT, pidArray);
    } else {
        mesh->GetAttributeSet()->AddScalar(IG_CELL, pidArray);
    }

    const unsigned int pointsMTimeBefore = InputPointsMTime(mesh);
    auto filter = iGame::GenerateProcessIdsFilter::New();
    filter->SetInput(mesh);
    filter->SetGeneratePointData(pointData);
    filter->SetGenerateCellData(!pointData);
    filter->SetProcessId(expectValue);
    if (!filter->Execute()) {
        std::cout << "FAIL: Execute (input process_id)\n";
        return false;
    }
    if (InputPointsMTime(mesh) != pointsMTimeBefore) {
        std::cout << "FAIL: input geometry should not be marked modified (input process_id)\n";
        return false;
    }
    auto output = filter->GetOutput();
    if (!VerifyIndependentOutput(mesh, output, pointData, arrayName)) return false;

    // 结果值必须全部是当前进程号，而不是输入 process_id 里的 i % 3
    if (!VerifyResultValues(output, pointData, arrayName, expectCount,
                            [expectValue](IGsize) { return static_cast<long long>(expectValue); })) {
        std::cout << "FAIL: result should ignore the input process_id array\n";
        return false;
    }

    // 输入上的 process_id 未被修改，并原样拷入结果属性集。
    // 必须按「名字 + 挂载类型」定位：GetScalar(name) 不区分挂载类型，点 / 单元同时存在
    // 同名数组时会取到另一个（点数与单元数不同，校验必然失败）。
    const IGenum attachment = pointData ? IG_POINT : IG_CELL;
    auto* inputPid = mesh->GetAttributeSet()->GetArrayPointer(IG_SCALAR, attachment, "process_id");
    auto* outputPid = output->GetAttributeSet()->GetArrayPointer(IG_SCALAR, attachment, "process_id");
    bool ok = (inputPid != nullptr) && (outputPid != nullptr) && (outputPid->GetNumberOfElements() == expectCount);
    for (IGsize i = 0; ok && i < expectCount; ++i) {
        ok = (inputPid->GetValue(i) == static_cast<long long>(i % 3)) &&
             (outputPid->GetValue(i) == static_cast<long long>(i % 3));
    }
    if (!ok) {
        std::cout << "FAIL: input process_id array should be copied unchanged\n";
    }
    return ok;
}
}  // namespace

// 读取 AI 生成的测试模型（相对路径，需在 Examples 构建目录下运行，模型由构建时自动拷贝）
iGame::UnstructuredMesh::Pointer LoadModel(const std::string& fileName) {
    auto obj = iGame::FileIO::ReadFile(fileName);
    auto mesh = iGame::DynamicCast<iGame::UnstructuredMesh>(obj);
    if (mesh == nullptr) {
        std::cout << "FAIL: read model " << fileName << "\n";
        return nullptr;
    }
    return mesh;
}

iGame::SurfaceMesh::Pointer CreateSurfaceMesh() {
    auto mesh = iGame::SurfaceMesh::New();
    auto points = iGame::Points::New();
    points->AddPoint(0.f, 0.f, 0.f);
    points->AddPoint(1.f, 0.f, 0.f);
    points->AddPoint(0.f, 1.f, 0.f);
    points->AddPoint(0.f, 0.f, 1.f);
    mesh->SetPoints(points);
    // 与 FileIO 一致：直接注入 CellArray，不走 AddFace（AddFace 依赖未初始化的 m_Edges 等成员，会崩溃）
    auto faces = iGame::CellArray::New();
    igIndex tri1[3]{0, 1, 2};
    igIndex tri2[3]{0, 2, 3};
    faces->AddCellIds(tri1, 3);
    faces->AddCellIds(tri2, 3);
    mesh->SetFaces(faces);
    return mesh;
}

iGame::VolumeMesh::Pointer CreateVolumeMesh() {
    auto mesh = iGame::VolumeMesh::New();
    auto points = iGame::Points::New();
    points->AddPoint(0.f, 0.f, 0.f);
    points->AddPoint(1.f, 0.f, 0.f);
    points->AddPoint(0.f, 1.f, 0.f);
    points->AddPoint(0.f, 0.f, 1.f);
    mesh->SetPoints(points);
    auto volumes = iGame::CellArray::New();
    igIndex volume[4] = {0, 1, 2, 3};
    volumes->AddCellIds(volume, 4);
    mesh->SetVolumes(volumes);
    return mesh;
}

// 纯点云：单元数据被跳过（点进程号照常生成、返回成功并给出提示），两个开关都关时执行失败
bool VerifyCellDataSkippedOnPointSet() {
    auto mesh = iGame::PointSet::New();
    mesh->AddPoint(iGame::Point(0.f, 0.f, 0.f));

    // 1) 只勾单元数据：跳过单元并给出提示，执行仍成功，结果里没有 CellProcessIds
    auto cellOnly = iGame::GenerateProcessIdsFilter::New();
    cellOnly->SetInput(mesh);
    cellOnly->SetGeneratePointData(false);
    cellOnly->SetGenerateCellData(true);
    if (!cellOnly->Execute()) {
        std::cout << "FAIL: skipped cell data should still succeed\n";
        return false;
    }
    if (cellOnly->GetMessage().empty()) {
        std::cout << "FAIL: skipped cell data should leave a message\n";
        return false;
    }
    auto cellOnlyOutput = cellOnly->GetOutput();
    if (cellOnlyOutput == nullptr) {
        std::cout << "FAIL: skipped cell data should still produce an output\n";
        return false;
    }
    if (CountArrays(cellOnlyOutput, false, "CellProcessIds") != 0) {
        std::cout << "FAIL: PointSet result should not carry CellProcessIds\n";
        return false;
    }

    // 2) 点 + 单元都勾：点数组照常生成，单元部分跳过
    const unsigned int pointsMTimeBefore = InputPointsMTime(mesh);
    auto both = iGame::GenerateProcessIdsFilter::New();
    both->SetInput(mesh);
    both->SetGeneratePointData(true);
    both->SetGenerateCellData(true);
    both->SetProcessId(3);
    if (!both->Execute()) {
        std::cout << "FAIL: point data on PointSet should succeed\n";
        return false;
    }
    if (both->GetMessage().empty()) {
        std::cout << "FAIL: skipped cell data should leave a message\n";
        return false;
    }
    if (InputPointsMTime(mesh) != pointsMTimeBefore) {
        std::cout << "FAIL: input geometry should not be marked modified (PointSet)\n";
        return false;
    }
    auto output = both->GetOutput();
    if (!VerifyIndependentOutput(mesh, output, true, "PointProcessIds")) return false;
    if (CountArrays(output, false, "CellProcessIds") != 0) {
        std::cout << "FAIL: PointSet result should not carry CellProcessIds\n";
        return false;
    }
    if (!VerifyResultValues(output, true, "PointProcessIds", 1, [](IGsize) { return 3LL; })) {
        std::cout << "FAIL: point PointProcessIds values on PointSet\n";
        return false;
    }

    // 3) 两个开关都关：执行失败并给出提示，不保留输出
    auto none = iGame::GenerateProcessIdsFilter::New();
    none->SetInput(mesh);
    none->SetGeneratePointData(false);
    none->SetGenerateCellData(false);
    if (none->Execute()) {
        std::cout << "FAIL: no requested data should fail\n";
        return false;
    }
    if (none->GetMessage().empty()) {
        std::cout << "FAIL: GetMessage should be non-empty when nothing is requested\n";
        return false;
    }
    if (none->GetOutput() != nullptr) {
        std::cout << "FAIL: failed execution should not keep an output\n";
        return false;
    }
    return true;
}

int main(int argc, char* argv[]) {
    bool allOk = true;

    // 默认使用仓库自带的 AI 测试模型，无参即可完整运行；第一个命令行参数可覆盖主模型路径
    const std::string mainModel =
            (argc > 1) ? std::string(argv[1]) : std::string("./Models/GenerateProcessIds_SteppedPipe.vtk");

    auto mesh = LoadModel(mainModel);
    if (mesh == nullptr) return 1;

    IGsize pointNum = mesh->GetNumberOfPoints();
    bool pointOk = VerifyConstant(mesh, true, "PointProcessIds", pointNum, 7);
    std::cout << (pointOk ? "PASS" : "FAIL") << ": point PointProcessIds count=" << pointNum << " value=7\n";
    allOk = allOk && pointOk;

    IGsize cellNum = mesh->GetNumberOfCells();
    bool cellOk = VerifyConstant(mesh, false, "CellProcessIds", cellNum, 7);
    std::cout << (cellOk ? "PASS" : "FAIL") << ": cell CellProcessIds count=" << cellNum << " value=7\n";
    allOk = allOk && cellOk;

    auto partMesh = LoadModel(mainModel);
    if (partMesh == nullptr) return 1;

    IGsize partPointNum = partMesh->GetNumberOfPoints();
    bool pointPartOk = VerifyPartitioned(partMesh, true, "PointProcessIds", partPointNum);
    std::cout << (pointPartOk ? "PASS" : "FAIL") << ": partitioned point PointProcessIds count=" << partPointNum
              << "\n";
    allOk = allOk && pointPartOk;

    IGsize partCellNum = partMesh->GetNumberOfCells();
    bool cellPartOk = VerifyPartitioned(partMesh, false, "CellProcessIds", partCellNum);
    std::cout << (cellPartOk ? "PASS" : "FAIL") << ": partitioned cell CellProcessIds count=" << partCellNum << "\n";
    allOk = allOk && cellPartOk;

    bool pointRepeatOk = VerifyRepeatedExecution(mesh, true, "PointProcessIds", pointNum, 7);
    std::cout << (pointRepeatOk ? "PASS" : "FAIL") << ": repeated point PointProcessIds count=" << pointNum << "\n";
    allOk = allOk && pointRepeatOk;

    bool cellRepeatOk = VerifyRepeatedExecution(mesh, false, "CellProcessIds", cellNum, 7);
    std::cout << (cellRepeatOk ? "PASS" : "FAIL") << ": repeated cell CellProcessIds count=" << cellNum << "\n";
    allOk = allOk && cellRepeatOk;

    // 输入自带 process_id 数组：结果仍写当前进程号（对齐 VTK，不再沿用输入值）
    auto extMesh = LoadModel(mainModel);
    if (extMesh == nullptr) return 1;

    IGsize extPointNum = extMesh->GetNumberOfPoints();
    bool extPointOk = VerifyInputProcessIdsIgnored(extMesh, true, "PointProcessIds", extPointNum, 7);
    std::cout << (extPointOk ? "PASS" : "FAIL") << ": input process_id ignored (point) count=" << extPointNum << "\n";
    allOk = allOk && extPointOk;

    IGsize extCellNum = extMesh->GetNumberOfCells();
    bool extCellOk = VerifyInputProcessIdsIgnored(extMesh, false, "CellProcessIds", extCellNum, 7);
    std::cout << (extCellOk ? "PASS" : "FAIL") << ": input process_id ignored (cell) count=" << extCellNum << "\n";
    allOk = allOk && extCellOk;

    // 第二个 AI 测试模型（文丘里缩放喷管）：常数 / 分区两类校验
    auto venturiMesh = LoadModel("./Models/GenerateProcessIds_VenturiTube.vtk");
    if (venturiMesh == nullptr) return 1;

    IGsize venturiPointNum = venturiMesh->GetNumberOfPoints();
    bool venturiPointOk = VerifyConstant(venturiMesh, true, "PointProcessIds", venturiPointNum, 7);
    std::cout << (venturiPointOk ? "PASS" : "FAIL") << ": venturi point PointProcessIds count=" << venturiPointNum
              << " value=7\n";
    allOk = allOk && venturiPointOk;

    IGsize venturiCellNum = venturiMesh->GetNumberOfCells();
    bool venturiCellOk = VerifyConstant(venturiMesh, false, "CellProcessIds", venturiCellNum, 7);
    std::cout << (venturiCellOk ? "PASS" : "FAIL") << ": venturi cell CellProcessIds count=" << venturiCellNum
              << " value=7\n";
    allOk = allOk && venturiCellOk;

    bool venturiPointPartOk = VerifyPartitioned(venturiMesh, true, "PointProcessIds", venturiPointNum);
    std::cout << (venturiPointPartOk ? "PASS" : "FAIL") << ": venturi partitioned point PointProcessIds count="
              << venturiPointNum << "\n";
    allOk = allOk && venturiPointPartOk;

    bool venturiCellPartOk = VerifyPartitioned(venturiMesh, false, "CellProcessIds", venturiCellNum);
    std::cout << (venturiCellPartOk ? "PASS" : "FAIL") << ": venturi partitioned cell CellProcessIds count="
              << venturiCellNum << "\n";
    allOk = allOk && venturiCellPartOk;

    auto surfMesh = CreateSurfaceMesh();
    IGsize surfFaceNum = surfMesh->GetNumberOfFaces();
    bool surfCellOk = VerifyConstant(surfMesh, false, "CellProcessIds", surfFaceNum, 7);
    std::cout << (surfCellOk ? "PASS" : "FAIL") << ": surface cell CellProcessIds count=" << surfFaceNum << "\n";
    allOk = allOk && surfCellOk;

    bool surfCellPartOk = VerifyPartitioned(surfMesh, false, "CellProcessIds", surfFaceNum);
    std::cout << (surfCellPartOk ? "PASS" : "FAIL") << ": surface partitioned cell CellProcessIds count="
              << surfFaceNum << "\n";
    allOk = allOk && surfCellPartOk;

    auto volMesh = CreateVolumeMesh();
    IGsize volNum = volMesh->GetNumberOfVolumes();
    bool volCellOk = VerifyConstant(volMesh, false, "CellProcessIds", volNum, 7);
    std::cout << (volCellOk ? "PASS" : "FAIL") << ": volume cell CellProcessIds count=" << volNum << "\n";
    allOk = allOk && volCellOk;

    bool volCellPartOk = VerifyPartitioned(volMesh, false, "CellProcessIds", volNum);
    std::cout << (volCellPartOk ? "PASS" : "FAIL") << ": volume partitioned cell CellProcessIds count=" << volNum
              << "\n";
    allOk = allOk && volCellPartOk;

    bool skippedCellOk = VerifyCellDataSkippedOnPointSet();
    std::cout << (skippedCellOk ? "PASS" : "FAIL") << ": skipped cell data on PointSet\n";
    allOk = allOk && skippedCellOk;

    return allOk ? 0 : 1;
}
