// 迁移回归（待提交）：防止标准过滤器仅有菜单占位，验证本文件记录的算法结果、输入保持不变及边界情况。
// ============================================================================
// TemporalStatistics 回归测试
//
// 被测滤波器：iGameCore/Filters/Animation/iGameTemporalStatistics.{h,cpp}
//
// 测试数据：test/Animation Test/result-case1
//   - 401 帧 legacy VTK 序列（VTKfile-0.vtk ... VTKfile-400.vtk），
//     每帧 3212 点 / 2880 单元；
//   - 点数组：Displacement / Velocity / Acceleration / Force（3 分量）、Group / Contact（标量）
//   - 单元数组：Material、Stress-Sig_XY（标量）、Stress-Sig（3 分量）
//
// 检查内容：
//   1) 逐帧读入后能构造 MultiSubFiles 时间序列，滤波器执行成功且无非致命提示；
//   2) 每个源数组都产出 <名>_average / _minimum / _maximum / _stddev 四个统计数组，
//      隶属（点/单元）与元素数、维度与源一致；
//   3) 抽样元素上，平均 / 最小 / 最大 / 标准差与“测试自己按帧重算”的参考值一致；
//   4) 单调性检查：最小值 ≤ 平均值 ≤ 最大值，标准差 ≥ 0。
//
// 用法：
//   TestTemporalStatistics [数据目录] [--all | 帧数上限]
//   默认数据目录 test/Animation Test/result-case1，默认取前 48 帧（全量 401 帧较慢）。
//   数据目录不存在时打印跳过信息并返回 77（CMake 侧已配置 SKIP_RETURN_CODE）。
// ============================================================================

#include <Animation/iGameTemporalStatistics.h>

#include <iGameAttributeSet.h>
#include <iGameCellArray.h>
#include <iGameCompositeDataObject.h>
#include <iGameDataObject.h>
#include <iGameFileIO.h>
#include <iGameFlatArray.h>
#include <iGameLagrangeUnstructuredMesh.h>
#include <iGamePoints.h>
#include <iGameStreamingData.h>
#include <iGameStringArray.h>
#include <iGameStructuredMesh.h>
#include <iGameUnstructuredMesh.h>
#include <iGameVolumeMesh.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

using namespace iGame;

int g_checked = 0;
int g_failed = 0;

void Check(bool ok, const std::string& what) {
    ++g_checked;
    if (ok) {
        std::cout << "[ok]   " << what << "\n";
    } else {
        std::cout << "[FAIL] " << what << "\n";
        ++g_failed;
    }
}

void CheckClose(double actual, double expected, const std::string& what) {
    const double tolerance = 1e-3 * std::max(1.0, std::abs(expected));
    const bool ok = std::abs(actual - expected) <= tolerance;
    ++g_checked;
    if (ok) {
        std::cout << "[ok]   " << what << "\n";
    } else {
        std::cout << "[FAIL] " << what << "（实际 " << actual << "，期望 " << expected << "）\n";
        ++g_failed;
    }
}

// 收集目录里的 VTKfile-<数字>.vtk，按编号升序；limit==0 表示全部
std::vector<std::string> CollectFrameFiles(const std::string& dir, size_t limit) {
    namespace fs = std::filesystem;
    std::vector<std::pair<int, std::string>> numbered;
    std::error_code ec;
    for (const auto& entry: fs::directory_iterator(dir, ec)) {
        if (!entry.is_regular_file()) { continue; }
        const std::string name = entry.path().filename().string();
        const bool legacy = name.rfind("VTKfile-", 0) == 0 && entry.path().extension() == ".vtk";
        const bool supplied = name.rfind("deform_", 0) == 0 && entry.path().extension() == ".vtu";
        if (!legacy && !supplied) continue;
        const size_t prefix = legacy ? 8 : 7;
        const std::string digits = name.substr(prefix, name.size() - prefix - 4);
        if (digits.empty() || digits.find_first_not_of("0123456789") != std::string::npos) { continue; }
        numbered.emplace_back(std::stoi(digits), entry.path().string());
    }
    std::sort(numbered.begin(), numbered.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });

    std::vector<std::string> files;
    for (const auto& item: numbered) {
        if (limit > 0 && files.size() >= limit) { break; }
        files.push_back(item.second);
    }
    return files;
}

// 在某一帧对象上按“名字 + 隶属”找数组
iGame::ArrayObject::Pointer FindFrameArray(const iGame::DataObject::Pointer& obj, const std::string& name,
                                           IGenum attachment) {
    using namespace iGame;
    if (obj.IsNull()) { return nullptr; }
    auto* attributes = obj->GetAttributeSet();
    if (attributes == nullptr) { return nullptr; }
    auto all = attributes->GetAllAttributes();
    for (IGsize i = 0; i < all->GetNumberOfElements(); ++i) {
        auto& attribute = all->GetElement(i);
        if (attribute.IsNone() || attribute.pointer.IsNull()) { continue; }
        if (attribute.pointer->GetName() == name && attribute.attachmentType == attachment) {
            return attribute.pointer;
        }
    }
    return nullptr;
}

std::string AttachmentName(IGenum attachment) {
    return attachment == IG_CELL ? "单元" : "点";
}

// ---------------- 几何类型覆盖用的合成网格（每种类型造"两帧"） ----------------

DoubleArray::Pointer MakeRange(const std::vector<double>& values) {
    auto range = DoubleArray::New();
    range->SetDimension(2);
    range->Resize(2);
    range->SetValue(0, *std::min_element(values.begin(), values.end()));
    range->SetValue(1, *std::max_element(values.begin(), values.end()));
    return range;
}

FloatArray::Pointer MakeScalarArray(const std::string& name, const std::vector<double>& values) {
    auto array = FloatArray::New();
    array->SetName(name);
    array->SetDimension(1);
    array->Resize(values.size());
    for (size_t i = 0; i < values.size(); ++i) {
        array->SetValue(static_cast<IGsize>(i), values[i]);
    }
    return array;
}

// 非结构网格：4 点 2 三角形，带单元标量数组
UnstructuredMesh::Pointer MakeSmallTriMesh(double firstCellValue, double secondCellValue,
                                           const std::string& arrayName) {
    auto mesh = UnstructuredMesh::New();
    auto points = Points::New();
    points->AddPoint(0.f, 0.f, 0.f);
    points->AddPoint(1.f, 0.f, 0.f);
    points->AddPoint(1.f, 1.f, 0.f);
    points->AddPoint(0.f, 1.f, 0.f);
    mesh->SetPoints(points);
    auto cells = CellArray::New();
    igIndex tri0[3] = {0, 1, 2};
    igIndex tri1[3] = {0, 2, 3};
    cells->AddCellIds(tri0, 3);
    cells->AddCellIds(tri1, 3);
    auto types = UnsignedIntArray::New();
    types->AddValue(IG_TRIANGLE);
    types->AddValue(IG_TRIANGLE);
    mesh->SetCells(cells, types);
    const std::vector<double> values = {firstCellValue, secondCellValue};
    mesh->GetAttributeSet()->AddScalar(IG_CELL, MakeScalarArray(arrayName, values), MakeRange(values));
    return mesh;
}

// 结构化网格：2x2x2 点、1 个六面体单元（单元由尺寸/范围隐含）
StructuredMesh::Pointer MakeSmallStructuredMesh(double cellValue, const std::string& arrayName) {
    StructuredMesh::Pointer mesh = StructuredMesh::New();
    auto points = Points::New();
    for (int k = 0; k < 2; ++k) {
        for (int j = 0; j < 2; ++j) {
            for (int i = 0; i < 2; ++i) {
                points->AddPoint(static_cast<float>(i), static_cast<float>(j), static_cast<float>(k));
            }
        }
    }
    mesh->SetPoints(points);
    // 注意调用顺序：与 VTI 读取器一致，先 SetExtent 再 SetDimensionSize。
    // 框架里 StructuredMesh::SetExtent 会把 6 个值写进只有 3 个元素的 size（既有缺陷，
    // 见 doc/FilterUserNotice/MedianFilter使用说明.md 第 10 条），所以尺寸必须最后设置。
    igIndex extent[6] = {0, 1, 0, 1, 0, 1};
    mesh->SetExtent(extent);
    igIndex dimensions[3] = {2, 2, 2};
    mesh->SetDimensionSize(dimensions);
    // 与读取器（VTI/CGNS）一致：结构化网格的单元由尺寸推导，需要显式构建连通性
    mesh->GenStructuredCellConnectivities();
    const std::vector<double> values = {cellValue};
    mesh->GetAttributeSet()->AddScalar(IG_CELL, MakeScalarArray(arrayName, values), MakeRange(values));
    return mesh;
}

// 多面体体网格：8 点 + 6 个四边形面 + 1 个体（体→面索引）
VolumeMesh::Pointer MakeSmallPolyhedronMesh(double cellValue, const std::string& arrayName) {
    auto mesh = VolumeMesh::New();
    auto points = Points::New();
    for (int k = 0; k < 2; ++k) {
        for (int j = 0; j < 2; ++j) {
            for (int i = 0; i < 2; ++i) {
                points->AddPoint(static_cast<float>(i), static_cast<float>(j), static_cast<float>(k));
            }
        }
    }
    mesh->SetPoints(points);

    // 六面体的 6 个面（点序号按 i + 2*j + 4*k 排）
    const igIndex facePointIds[6][4] = {
            {0, 1, 3, 2}, {4, 5, 7, 6}, {0, 1, 5, 4}, {2, 3, 7, 6}, {0, 2, 6, 4}, {1, 3, 7, 5}};
    auto faces = CellArray::New();
    for (const auto& face: facePointIds) {
        faces->AddCellIds(face, 4);
    }
    auto volumeFaces = CellArray::New();
    const igIndex volumeFaceIds[6] = {0, 1, 2, 3, 4, 5};
    volumeFaces->AddCellIds(volumeFaceIds, 6);
    mesh->InitVolumesWithPolyhedron(faces, volumeFaces);

    const std::vector<double> values = {cellValue};
    mesh->GetAttributeSet()->AddScalar(IG_CELL, MakeScalarArray(arrayName, values), MakeRange(values));
    return mesh;
}

// Lagrange 非结构网格：4 点 1 个单元（带精确单元类型与阶次）
LagrangeUnstructuredMesh::Pointer MakeSmallLagrangeMesh(double cellValue, const std::string& arrayName) {
    LagrangeUnstructuredMesh::Pointer mesh = LagrangeUnstructuredMesh::New();
    auto points = Points::New();
    points->AddPoint(0.f, 0.f, 0.f);
    points->AddPoint(1.f, 0.f, 0.f);
    points->AddPoint(0.f, 1.f, 0.f);
    points->AddPoint(0.f, 0.f, 1.f);
    mesh->SetPoints(points);
    igIndex cellIds[4] = {0, 1, 2, 3};
    mesh->AddCell(cellIds, 4, IG_TETRA, 1);
    const std::vector<double> values = {cellValue};
    mesh->GetAttributeSet()->AddScalar(IG_CELL, MakeScalarArray(arrayName, values), MakeRange(values));
    return mesh;
}

// 用两帧合成序列跑一次滤波器，并核对指定统计数组的首元素
void CheckSyntheticSeries(const std::string& label, const std::string& arrayName,
                          const DataObject::Pointer& frameA, const DataObject::Pointer& frameB,
                          double expectedAverage) {
    auto frames = StreamingData::New();
    auto namesA = StringArray::New();
    namesA->AddElement("frameA");
    auto namesB = StringArray::New();
    namesB->AddElement("frameB");
    frames->AddTimeStep(0.0f, namesA, StreamingType::MultiSubFiles);
    frames->AddTimeStep(1.0f, namesB, StreamingType::MultiSubFiles);
    frames->EnableCache(2);
    frames->GetArrays()[0].SetCache(std::vector<Object::Pointer>{frameA});
    frames->GetArrays()[1].SetCache(std::vector<Object::Pointer>{frameB});

    auto root = DataObject::New();
    root->SetName(label);
    root->SetTimeFrames(frames);

    auto filter = iGameTemporalStatistics::New();
    filter->SetInput(root);
    const bool ok = filter->Execute();
    Check(ok, label + "：滤波器执行成功" +
                      (ok ? std::string() : std::string("（") + filter->GetMessage() + "）"));
    if (!ok) { return; }
    auto output = filter->GetOutput(0);
    Check(output.IsNotNull(), label + "：产生输出");
    if (output.IsNull()) { return; }
    auto* attributes = output->GetAttributeSet();
    const int index = (attributes != nullptr) ? attributes->GetAttributeIndex(arrayName + "_average") : -1;
    Check(index >= 0, label + "：输出含 " + arrayName + "_average");
    if (index < 0) { return; }
    CheckClose(attributes->GetAttribute(static_cast<IGsize>(index)).pointer->GetElementValue(0, 0),
               expectedAverage, label + "：" + arrayName + "_average 首元素");
}

} // namespace

int main(int argc, char** argv) {
    using namespace iGame;

    std::string dataDir = (argc > 1) ? argv[1] : "test/Animation Test/result-case1";
    size_t frameLimit = 48;
    if (argc > 2) {
        const std::string arg = argv[2];
        frameLimit = (arg == "--all") ? 0 : static_cast<size_t>(std::max(0, std::atoi(arg.c_str())));
    }

    std::cout << "=== TemporalStatistics 回归测试 ===\n"
              << "数据目录: " << dataDir << "\n";

    if (!std::filesystem::exists(dataDir)) {
        std::cout << "[skip] 数据目录不存在（code-only checkout 未拉取 LFS 素材？），跳过测试。\n";
        return 77;
    }

    const auto files = CollectFrameFiles(dataDir, frameLimit);
    if (files.empty()) {
        std::cout << "[skip] 目录里没有 VTKfile-<数字>.vtk，跳过测试。\n";
        return 77;
    }
    std::cout << "使用帧数: " << files.size()
              << (frameLimit == 0 ? "（全部）" : "（默认前 N 帧，可用 --all 跑全量）") << "\n\n";

    // ---- 读入每一帧（只读一次：既作为时间序列缓存，也作为参考值来源）----
    std::vector<DataObject::Pointer> frameObjects;
    frameObjects.reserve(files.size());
    for (size_t i = 0; i < files.size(); ++i) {
        auto obj = FileIO::ReadFile(files[i]);
        if (obj.IsNull()) {
            std::cerr << "[FAIL] 读取失败: " << files[i] << "\n";
            return 1;
        }
        frameObjects.push_back(obj);
        if (i == 0 || (i + 1) % 16 == 0) {
            std::cout << "  已读入 " << (i + 1) << " / " << files.size() << " 帧\n";
        }
    }
    std::cout << "\n";

    // ---- 构造 MultiSubFiles 时间序列（每帧一个文件），并开启帧缓存 ----
    auto frames = StreamingData::New();
    for (size_t i = 0; i < frameObjects.size(); ++i) {
        auto names = StringArray::New();
        names->AddElement(files[i]);
        frames->AddTimeStep(static_cast<float>(i), names, StreamingType::MultiSubFiles);
    }
    frames->EnableCache(static_cast<unsigned int>(frameObjects.size()));
    for (size_t i = 0; i < frameObjects.size(); ++i) {
        frames->GetArrays()[i].SetCache(std::vector<Object::Pointer>{frameObjects[i]});
    }

    auto root = DataObject::New();
    root->SetName("AnimationTest");
    root->SetTimeFrames(frames);

    // ---- 执行滤波器（四项统计全开，与面板默认一致）----
    auto filter = iGameTemporalStatistics::New();
    filter->SetInput(root);
    filter->SetComputeAverage(true);
    filter->SetComputeMinimum(true);
    filter->SetComputeMaximum(true);
    filter->SetComputeStandardDeviation(true);
    if (!filter->Execute()) {
        std::cerr << "[FAIL] 滤波器执行失败: " << filter->GetMessage() << "\n";
        return 1;
    }
    auto output = filter->GetOutput(0);
    Check(output.IsNotNull(), "滤波器执行成功并产生输出");
    if (output.IsNull()) { return 1; }
    Check(filter->GetWarningsText().empty(),
          "该数据集各帧数组一致，执行过程无非致命提示" + std::string("（") + filter->GetWarningsText() + "）");

    auto* outputAttributes = output->GetAttributeSet();
    Check(outputAttributes != nullptr, "输出对象带属性集");
    if (outputAttributes == nullptr) { return 1; }

    // ---- 逐个源数组校验 ----
    auto* firstFrameAttributes = frameObjects.front()->GetAttributeSet();
    auto sourceAttributes = firstFrameAttributes->GetAllAttributes();
    size_t arrayCount = 0;
    int sampledValues = 0;

    for (IGsize a = 0; a < sourceAttributes->GetNumberOfElements(); ++a) {
        auto& source = sourceAttributes->GetElement(a);
        if (source.IsNone() || source.pointer.IsNull()) { continue; }
        ++arrayCount;

        const std::string name = source.pointer->GetName();
        const IGenum attachment = source.attachmentType;
        const int dimension = source.pointer->GetDimension();
        const IGsize count = source.pointer->GetNumberOfElements();

        // 四个统计数组是否都存在，且隶属 / 维度 / 元素数与源一致
        const std::vector<std::pair<std::string, const char*>> statistics = {
                {"_average", "平均"}, {"_minimum", "最小"}, {"_maximum", "最大"}, {"_stddev", "标准差"}};
        std::vector<ArrayObject::Pointer> outputs;
        for (const auto& item: statistics) {
            const int index = outputAttributes->GetAttributeIndex(name + item.first);
            if (index < 0) {
                Check(false, "输出缺少数组 " + name + item.first);
                outputs.push_back(nullptr);
                continue;
            }
            auto& attribute = outputAttributes->GetAttribute(static_cast<IGsize>(index));
            outputs.push_back(attribute.pointer);
            Check(attribute.pointer.IsNotNull() && attribute.attachmentType == attachment &&
                          attribute.pointer->GetDimension() == dimension &&
                          attribute.pointer->GetNumberOfElements() == count,
                  "输出数组 " + name + item.first + " 的隶属 / 维度 / 元素数与源一致");
            // 与 ParaView 一致：输出数组的数据类型沿用源数组
            Check(attribute.pointer.IsNotNull() &&
                          attribute.pointer->GetArrayType() == source.pointer->GetArrayType(),
                  "输出数组 " + name + item.first + " 的数据类型与源一致");
        }

        // 抽样元素：拿滤波器结果与“测试自己按帧重算”的参考值对比
        const IGsize samples[5] = {0, count / 4, count / 2, count * 3 / 4, count > 0 ? count - 1 : 0};
        for (IGsize s = 0; s < 5; ++s) {
            const IGsize element = samples[s];
            if (element >= count) { continue; }
            for (int c = 0; c < dimension; ++c) {
                std::vector<double> values;
                values.reserve(frameObjects.size());
                bool missing = false;
                for (const auto& frame: frameObjects) {
                    auto array = FindFrameArray(frame, name, attachment);
                    if (array.IsNull()) { missing = true; break; }
                    values.push_back(array->GetElementValue(element, c));
                }
                if (missing || values.empty()) { continue; }

                double sum = 0.0;
                double minimum = values.front();
                double maximum = values.front();
                for (double value: values) {
                    sum += value;
                    minimum = std::min(minimum, value);
                    maximum = std::max(maximum, value);
                }
                const double average = sum / static_cast<double>(values.size());
                double squared = 0.0;
                for (double value: values) {
                    const double delta = value - average;
                    squared += delta * delta;
                }
                const double stddev = std::sqrt(squared / static_cast<double>(values.size()));

                const std::string where = " (" + AttachmentName(attachment) + " " +
                                          std::to_string(element) + " 元素, 分量 " + std::to_string(c) + ")";
                if (outputs[0]) { CheckClose(outputs[0]->GetElementValue(element, c), average, name + "_average" + where); }
                if (outputs[1]) { CheckClose(outputs[1]->GetElementValue(element, c), minimum, name + "_minimum" + where); }
                if (outputs[2]) { CheckClose(outputs[2]->GetElementValue(element, c), maximum, name + "_maximum" + where); }
                if (outputs[3]) { CheckClose(outputs[3]->GetElementValue(element, c), stddev, name + "_stddev" + where); }
                ++sampledValues;

                // 单调性与非负
                if (outputs[0] && outputs[1] && outputs[2]) {
                    const double avgOut = outputs[0]->GetElementValue(element, c);
                    const double minOut = outputs[1]->GetElementValue(element, c);
                    const double maxOut = outputs[2]->GetElementValue(element, c);
                    Check(minOut <= avgOut + 1e-6 && avgOut <= maxOut + 1e-6,
                          name + " 满足 最小 ≤ 平均 ≤ 最大" + where);
                }
                if (outputs[3]) {
                    Check(outputs[3]->GetElementValue(element, c) >= -1e-6,
                          name + "_stddev 非负" + where);
                }
            }
        }
    }

    Check(arrayCount > 0, "源数据里至少有 1 个数组参与统计");

    // 与 ParaView 对应：输出里记录了本次处理的时间步（iGame 写在 Metadata，ParaView 写在 FieldData）
    if (auto* metadata = output->GetMetadata()) {
        auto timeSteps = metadata->GetDoubleArray("time_steps");
        Check(timeSteps.IsNotNull() &&
                      timeSteps->GetNumberOfElements() == static_cast<IGsize>(files.size()),
              "输出 Metadata 记录的时间步数与帧数一致（time_steps）");
    } else {
        Check(false, "输出对象带 Metadata");
    }

    // ---- 几何类型覆盖（合成两帧）----
    std::cout << "\n---- 几何类型覆盖（合成两帧：结构化 / 多面体 / Lagrange / 多块容器）----\n";
    {
        // 先确认"合成结构化网格"本身是合法的（单元由尺寸推导，需要构建连通性）
        auto structured = MakeSmallStructuredMesh(1.0, "structStress");
        Check(DynamicCast<StructuredMesh>(structured).IsNotNull(), "合成结构化网格：类型正确");
        Check(structured->GetNumberOfPoints() == 8, "合成结构化网格：点数 = 8");
        Check(structured->GetNumberOfCells() == 1, "合成结构化网格：单元数 = 1");
    }
    CheckSyntheticSeries("StructuredMesh", "structStress",
                         MakeSmallStructuredMesh(2.0, "structStress"),
                         MakeSmallStructuredMesh(6.0, "structStress"), 4.0);
    CheckSyntheticSeries("PolyhedronVolumeMesh", "polyStress",
                         MakeSmallPolyhedronMesh(1.0, "polyStress"),
                         MakeSmallPolyhedronMesh(5.0, "polyStress"), 3.0);
    CheckSyntheticSeries("LagrangeUnstructuredMesh", "lagStress",
                         MakeSmallLagrangeMesh(3.0, "lagStress"),
                         MakeSmallLagrangeMesh(7.0, "lagStress"), 5.0);

    // 多块容器帧：每帧一个容器 + 2 个子网格（同名数组）→ 展开成 2 块，输出容器根 + 2 个子对象
    {
        auto makeContainerFrame = [](double base) {
            DataObject::Pointer container = CompositeDataObject::New();
            container->AddSubDataObject(MakeSmallTriMesh(base, base + 10.0, "partStress"));
            container->AddSubDataObject(MakeSmallTriMesh(base + 100.0, base + 110.0, "partStress"));
            return container;
        };
        auto frames = StreamingData::New();
        auto namesA = StringArray::New();
        namesA->AddElement("blockFrameA");
        auto namesB = StringArray::New();
        namesB->AddElement("blockFrameB");
        frames->AddTimeStep(0.0f, namesA, StreamingType::MultiSubFiles);
        frames->AddTimeStep(1.0f, namesB, StreamingType::MultiSubFiles);
        frames->EnableCache(2);
        frames->GetArrays()[0].SetCache(std::vector<Object::Pointer>{makeContainerFrame(1.0)});
        frames->GetArrays()[1].SetCache(std::vector<Object::Pointer>{makeContainerFrame(3.0)});

        auto root = DataObject::New();
        root->SetName("MultiBlock");
        root->SetTimeFrames(frames);
        auto filter = iGameTemporalStatistics::New();
        filter->SetInput(root);
        const bool ok = filter->Execute();
        Check(ok, "多块容器：滤波器执行成功" +
                          (ok ? std::string() : std::string("（") + filter->GetMessage() + "）"));
        if (ok) {
            auto output = filter->GetOutput(0);
            Check(output.IsNotNull() && output->HasSubDataObject(), "多块容器：输出是容器（根 + 子对象）");
            if (output.IsNotNull()) {
                int children = 0;
                for (auto it = output->SubDataObjectIteratorBegin(); it != output->SubDataObjectIteratorEnd(); ++it) {
                    auto child = it->second;
                    if (child.IsNull() || child->GetAttributeSet() == nullptr) { continue; }
                    const int index = child->GetAttributeSet()->GetAttributeIndex("partStress_average");
                    if (index < 0) { continue; }
                    const double expected = (children == 0) ? 2.0 : 102.0;
                    CheckClose(child->GetAttributeSet()
                                       ->GetAttribute(static_cast<IGsize>(index))
                                       .pointer->GetElementValue(0, 0),
                               expected,
                               "多块容器：子对象 " + std::to_string(children) + " 的 partStress_average 首元素");
                    ++children;
                }
                Check(children == 2, "多块容器：输出含 2 个带统计的子对象");
            }
        }
    }

    // ---- 汇总 ----
    std::cout << "\n=== 汇总 ===\n"
              << "帧数: " << files.size() << "，源数组数: " << arrayCount
              << "，抽样比对值: " << sampledValues << "，断言数: " << g_checked
              << "，失败: " << g_failed << "\n";
    if (g_failed == 0) {
        std::cout << "全部通过。\n";
        return 0;
    }
    std::cout << "存在失败项，见上面 [FAIL] 行。\n";
    return 1;
}
