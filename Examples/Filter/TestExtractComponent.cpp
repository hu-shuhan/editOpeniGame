// Find the integration commit: git log --diff-filter=A --format="%h %s" -- Examples/Filter/TestExtractComponent.cpp
// Regression/example imported from dayuwan77/igamevis at
// eccac729b57aeacbe9312d7d5189f6990bb4eebd (same relative path).
// Integration regression: these filters and their example assets were missing
// from iGameVis-multiFilter. The checks below cover their output/attribute and
// geometry contracts; visual examples retain an interactive default mode.
// Local integration fix: feat: integrate second-batch standard filters.
#include <iostream>
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <string>
#include <vector>
#include <iGameFileIO.h>
#include <iGameUnstructuredMesh.h>
#include <Attribute/iGameExtractComponentFilter.h>

// 「提取分量」测试用例：默认读取 Examples/Models 下 AI 生成的两个测试模型（无参即可完整运行）。
// 分量提取语义对齐 VTK vtkImageExtractComponents（官方示例 ExtractComponents.cxx 的用法：
// 同一输入分别 SetComponents(0)/(1)/(2) 输出多路结果，再逐路核对）。
// 用法：testExtractComponent.exe [主模型路径]
//   不带参数：主模型 = ./Models/ExtractComponent_FlowPipe.vtk（需在 Examples 构建目录下运行）

// 输入点集的几何时间戳：校验执行后输入没有被标记为"已修改"
// （输入被标记修改会连累输入模型重跑表面提取，正是"输出结果后卡顿"的来源）
unsigned int InputPointsMTime(iGame::PointSet::Pointer input) {
    if (input == nullptr || input->GetPoints() == nullptr) { return 0; }
    return input->GetPoints()->GetMTime();
}

// 几何共享校验：结果持有独立点集对象（不再顶掉输入的时间戳），但底层缓冲与输入相同
bool VerifyGeometrySharing(iGame::PointSet::Pointer input, iGame::PointSet::Pointer output) {
    if (input == nullptr || output == nullptr || input->GetPoints() == nullptr || output->GetPoints() == nullptr) {
        std::cout << "FAIL: point set is null\n";
        return false;
    }
    if (input->GetPoints() == output->GetPoints()) {
        std::cout << "FAIL: output should own an independent point set\n";
        return false;
    }
    if (input->GetPoints()->RawPointer() != output->GetPoints()->RawPointer()) {
        std::cout << "FAIL: output should share input geometry data\n";
        return false;
    }
    return true;
}

// 程序化构造：4 点四面体 + 指定维度的点向量属性 vec（值 = i*dim + j）
iGame::UnstructuredMesh::Pointer CreateMeshWithDimVector(int dim) {
    auto mesh = iGame::UnstructuredMesh::New();
    mesh->AddPoint(iGame::Point(0.f, 0.f, 0.f));
    mesh->AddPoint(iGame::Point(1.f, 0.f, 0.f));
    mesh->AddPoint(iGame::Point(0.f, 1.f, 0.f));
    mesh->AddPoint(iGame::Point(0.f, 0.f, 1.f));
    igIndex cell[4] = {0, 1, 2, 3};
    mesh->AddCell(cell, 4, iGame::IG_TETRA);

    iGame::FloatArray::Pointer vec = iGame::FloatArray::New();
    vec->SetName("vec");
    vec->SetDimension(dim);
    std::vector<double> value(dim);
    for (IGsize i = 0; i < 4; ++i) {
        for (int j = 0; j < dim; ++j) value[j] = static_cast<double>(i * dim + j);
        vec->AddElement(value.data());
    }
    mesh->GetAttributeSet()->AddVector(IG_POINT, vec);
    return mesh;
}

// 程序化构造：4 点四面体 + 3 维点向量属性 vec（X/Y/Z 分别取 0/1/2 起，逐点 +3）
iGame::UnstructuredMesh::Pointer CreateMeshWithPointVector() {
    return CreateMeshWithDimVector(3);
}

// 程序化构造：5 点 2 四面体 + 单元向量属性 cellVec（两个单元 (10,20,30)/(40,50,60)）
iGame::UnstructuredMesh::Pointer CreateMeshWithCellVector() {
    auto mesh = iGame::UnstructuredMesh::New();
    mesh->AddPoint(iGame::Point(0.f, 0.f, 0.f));
    mesh->AddPoint(iGame::Point(1.f, 0.f, 0.f));
    mesh->AddPoint(iGame::Point(0.f, 1.f, 0.f));
    mesh->AddPoint(iGame::Point(0.f, 0.f, 1.f));
    mesh->AddPoint(iGame::Point(1.f, 1.f, 0.f));
    igIndex cell0[4] = {0, 1, 2, 3};
    igIndex cell1[4] = {1, 4, 2, 3};
    mesh->AddCell(cell0, 4, iGame::IG_TETRA);
    mesh->AddCell(cell1, 4, iGame::IG_TETRA);

    iGame::FloatArray::Pointer vec = iGame::FloatArray::New();
    vec->SetName("cellVec");
    vec->SetDimension(3);
    vec->AddElement3(10.f, 20.f, 30.f);
    vec->AddElement3(40.f, 50.f, 60.f);
    mesh->GetAttributeSet()->AddVector(IG_CELL, vec);
    return mesh;
}

// 读取 AI 生成的测试模型（相对路径，需在 Examples 构建目录下运行，模型由构建时自动拷贝）
iGame::UnstructuredMesh::Pointer ReadModelFromFile(const std::string& fileName) {
    auto obj = iGame::FileIO::ReadFile(fileName);
    auto mesh = iGame::DynamicCast<iGame::UnstructuredMesh>(obj);
    if (mesh == nullptr) {
        std::cout << "FAIL: read model " << fileName << "\n";
        return nullptr;
    }
    return mesh;
}

// 3 维向量 vec：第 i 个元素的第 comp 个分量，期望值 = 3*i + comp（comp 0/1/2）
double ExpectPointValue(IGsize i, int comp) { return 3.0 * i + comp; }

namespace {

// 提取单分量并校验：输出为继承新对象（几何共享、输入不被修改）、值逐元素正确、dataRange 正确
bool VerifyExtract(int component, const std::string& outputName, IGenum expectAttachment,
                   const std::string& inputName = "") {
    auto mesh = CreateMeshWithPointVector();
    const unsigned int pointsMTimeBefore = InputPointsMTime(mesh);
    auto filter = iGame::ExtractComponentFilter::New();
    filter->SetInput(mesh);
    filter->SetInputArrayName(inputName);
    filter->SetOutputArrayName(outputName);
    filter->SetComponent(component);
    if (!filter->Execute()) {
        std::cout << "FAIL: Execute (" << filter->GetMessage() << ")\n";
        return false;
    }
    auto outMesh = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    if (outMesh == nullptr) {
        std::cout << "FAIL: output is not UnstructuredMesh\n";
        return false;
    }
    // 继承语义：输入属性集不含输出数组（输入不被修改）；几何数据共享、点集对象独立
    bool ok = mesh->GetAttributeSet()->GetAttribute(outputName).IsNone();
    ok = ok && (InputPointsMTime(mesh) == pointsMTimeBefore);
    ok = ok && VerifyGeometrySharing(mesh, outMesh);
    // 输出对象属性校验
    auto& attr = outMesh->GetAttributeSet()->GetScalar(outputName);
    auto arr = attr.pointer;
    ok = ok && (arr != nullptr) && (attr.attachmentType == expectAttachment);
    ok = ok && (arr->GetDimension() == 1);
    for (IGsize i = 0; ok && i < arr->GetNumberOfElements(); ++i) {
        ok = (arr->GetValue(i) == ExpectPointValue(i, component));
    }
    // dataRange 断言：magnitude 范围与第一维范围均为所选分量的 [min, max]
    // （分量值全为正，magnitude 与符号值相等；dataRange 布局见 iGameAttributeSet.h 注释）
    auto range = attr.GetDataRange();
    ok = ok && (range != nullptr) && (range->GetValue(0) == ExpectPointValue(0, component))
         && (range->GetValue(1) == ExpectPointValue(3, component))
         && (range->GetValue(2) == ExpectPointValue(0, component))
         && (range->GetValue(3) == ExpectPointValue(3, component));
    return ok;
}

// 单元挂载用例：空输入名取单元向量，校验值、挂载类型与 dataRange
bool VerifyExtractCell() {
    auto mesh = CreateMeshWithCellVector();
    const unsigned int pointsMTimeBefore = InputPointsMTime(mesh);
    auto filter = iGame::ExtractComponentFilter::New();
    filter->SetInput(mesh);
    filter->SetOutputArrayName("Result");
    filter->SetComponent(0);
    if (!filter->Execute()) {
        std::cout << "FAIL: Execute (" << filter->GetMessage() << ")\n";
        return false;
    }
    auto outMesh = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    if (outMesh == nullptr) {
        std::cout << "FAIL: output is not UnstructuredMesh\n";
        return false;
    }
    bool ok = mesh->GetAttributeSet()->GetAttribute("Result").IsNone();
    ok = ok && (InputPointsMTime(mesh) == pointsMTimeBefore);
    ok = ok && VerifyGeometrySharing(mesh, outMesh);
    auto& attr = outMesh->GetAttributeSet()->GetScalar("Result");
    auto arr = attr.pointer;
    ok = ok && (arr != nullptr) && (attr.attachmentType == IG_CELL)
         && (arr->GetNumberOfElements() == 2);
    if (ok) {
        ok = (arr->GetValue(0) == 10.0) && (arr->GetValue(1) == 40.0);
        auto range = attr.GetDataRange();
        ok = ok && (range != nullptr) && (range->GetValue(2) == 10.0) && (range->GetValue(3) == 40.0);
    }
    return ok;
}

// 空输入名取第一个向量（vec），显式名取第二个（test_2，值翻倍）
bool VerifyInputArraySelection(bool useExplicitName) {
    auto mesh = CreateMeshWithPointVector();
    auto extra = iGame::FloatArray::New();
    extra->SetName("test_2");
    extra->SetDimension(3);
    for (IGsize i = 0; i < 4; ++i) {
        extra->AddElement3(2.f + 6.f * i, 4.f + 6.f * i, 6.f + 6.f * i);
    }
    mesh->GetAttributeSet()->AddVector(IG_POINT, extra);

    auto filter = iGame::ExtractComponentFilter::New();
    filter->SetInput(mesh);
    if (useExplicitName) filter->SetInputArrayName("test_2");
    filter->SetOutputArrayName("Result");
    filter->SetComponent(useExplicitName ? 1 : 0);
    if (!filter->Execute()) {
        std::cout << "FAIL: Execute (" << filter->GetMessage() << ")\n";
        return false;
    }
    auto outMesh = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    if (outMesh == nullptr) {
        std::cout << "FAIL: output is not UnstructuredMesh\n";
        return false;
    }
    bool ok = mesh->GetAttributeSet()->GetAttribute("Result").IsNone();
    auto& attr = outMesh->GetAttributeSet()->GetScalar("Result");
    auto arr = attr.pointer;
    ok = ok && (arr != nullptr);
    for (IGsize i = 0; ok && i < arr->GetNumberOfElements(); ++i) {
        ok = (arr->GetValue(i) == (useExplicitName ? 4.0 + 6.0 * i : ExpectPointValue(i, 0)));
    }
    return ok;
}

// 重名覆盖：输出名与输入已有向量属性 vec 重名时不报错，结果属性集中该名字唯一且为新提取值
bool VerifyOverwriteDuplicateName() {
    auto mesh = CreateMeshWithPointVector();
    auto filter = iGame::ExtractComponentFilter::New();
    filter->SetInput(mesh);
    filter->SetOutputArrayName("vec");  // 与输入已有向量属性重名 → 覆盖（不报错）
    filter->SetComponent(0);
    if (!filter->Execute()) {
        std::cout << "FAIL: Execute (" << filter->GetMessage() << ")\n";
        return false;
    }
    auto outMesh = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    if (outMesh == nullptr) {
        std::cout << "FAIL: output is not UnstructuredMesh\n";
        return false;
    }
    // 输入不被修改（输入属性集里 vec 仍是向量）
    if (!mesh->GetAttributeSet()->GetScalar("vec").IsNone()) {
        std::cout << "FAIL: input should not be modified\n";
        return false;
    }
    // 结果属性集中名为 vec 的活属性唯一（旧向量被覆盖删除，只有新标量）
    int count = 0;
    auto all = outMesh->GetAttributeSet()->GetAllAttributes();
    for (IGsize i = 0; i < all->GetNumberOfElements(); ++i) {
        auto& attr = all->GetElement(i);
        if (!attr.IsNone() && attr.pointer->GetName() == "vec") ++count;
    }
    if (count != 1) {
        std::cout << "FAIL: overwritten name should be unique (count=" << count << ")\n";
        return false;
    }
    // 值 = 新提取的分量（分量 0 → 0,3,6,9），且类型为标量
    auto& attr = outMesh->GetAttributeSet()->GetScalar("vec");
    auto arr = attr.pointer;
    bool ok = (arr != nullptr) && (attr.type == IG_SCALAR);
    for (IGsize i = 0; ok && i < arr->GetNumberOfElements(); ++i) {
        ok = (arr->GetValue(i) == ExpectPointValue(i, 0));
    }
    return ok;
}

// 需求 4：1/2 维数组不能提取不存在的分量（2 维 X/Y 合法、Z 非法；1 维 X 合法、Y 非法）
bool VerifyDimensionGuard() {
    bool allOk = true;
    // 2 维数组：元素 i 值为 {2i, 2i+1}
    {
        auto mesh = CreateMeshWithDimVector(2);
        auto f0 = iGame::ExtractComponentFilter::New();
        f0->SetInput(mesh);
        f0->SetOutputArrayName("R0");
        f0->SetComponent(0);
        bool ok0 = f0->Execute();
        if (ok0) {
            auto outMesh = iGame::DynamicCast<iGame::UnstructuredMesh>(f0->GetOutput());
            auto& attr = outMesh->GetAttributeSet()->GetScalar("R0");
            auto arr = attr.pointer;
            for (IGsize i = 0; ok0 && i < arr->GetNumberOfElements(); ++i) {
                ok0 = (arr->GetValue(i) == static_cast<double>(2 * i));
            }
        }
        auto f1 = iGame::ExtractComponentFilter::New();
        f1->SetInput(mesh);
        f1->SetOutputArrayName("R1");
        f1->SetComponent(1);
        bool ok1 = f1->Execute();
        if (ok1) {
            auto outMesh = iGame::DynamicCast<iGame::UnstructuredMesh>(f1->GetOutput());
            auto& attr = outMesh->GetAttributeSet()->GetScalar("R1");
            auto arr = attr.pointer;
            for (IGsize i = 0; ok1 && i < arr->GetNumberOfElements(); ++i) {
                ok1 = (arr->GetValue(i) == static_cast<double>(2 * i + 1));
            }
        }
        auto f2 = iGame::ExtractComponentFilter::New();
        f2->SetInput(mesh);
        f2->SetOutputArrayName("R2");
        f2->SetComponent(2);
        bool ok2 = !f2->Execute() && !f2->GetMessage().empty();
        allOk = allOk && ok0 && ok1 && ok2;
        if (!ok0) std::cout << "FAIL: 2D component 0\n";
        if (!ok1) std::cout << "FAIL: 2D component 1\n";
        if (!ok2) std::cout << "FAIL: 2D component 2 should reject\n";
    }
    // 1 维数组：元素 i 值为 {i}
    {
        auto mesh = CreateMeshWithDimVector(1);
        auto f0 = iGame::ExtractComponentFilter::New();
        f0->SetInput(mesh);
        f0->SetOutputArrayName("R0");
        f0->SetComponent(0);
        bool ok0 = f0->Execute();
        if (ok0) {
            auto outMesh = iGame::DynamicCast<iGame::UnstructuredMesh>(f0->GetOutput());
            auto& attr = outMesh->GetAttributeSet()->GetScalar("R0");
            auto arr = attr.pointer;
            for (IGsize i = 0; ok0 && i < arr->GetNumberOfElements(); ++i) {
                ok0 = (arr->GetValue(i) == static_cast<double>(i));
            }
        }
        auto f1 = iGame::ExtractComponentFilter::New();
        f1->SetInput(mesh);
        f1->SetOutputArrayName("R1");
        f1->SetComponent(1);
        bool ok1 = !f1->Execute() && !f1->GetMessage().empty();
        allOk = allOk && ok0 && ok1;
        if (!ok0) std::cout << "FAIL: 1D component 0\n";
        if (!ok1) std::cout << "FAIL: 1D component 1 should reject\n";
    }
    return allOk;
}

// 回归：对提取分量结果再次提取（触发含 null dataRange 属性的拷贝）不应崩溃
bool VerifyExtractOnExtractedResult() {
    auto mesh = CreateMeshWithPointVector();
    auto f1 = iGame::ExtractComponentFilter::New();
    f1->SetInput(mesh);
    f1->SetOutputArrayName("Result");
    f1->SetComponent(0);
    if (!f1->Execute()) {
        std::cout << "FAIL: first Execute (" << f1->GetMessage() << ")\n";
        return false;
    }
    auto out1 = iGame::DynamicCast<iGame::UnstructuredMesh>(f1->GetOutput());
    if (out1 == nullptr) {
        std::cout << "FAIL: first output is not UnstructuredMesh\n";
        return false;
    }

    // 对结果再次提取：输出名同为 Result（触发覆盖），输入属性集含 dataRange=null 的数组
    auto f2 = iGame::ExtractComponentFilter::New();
    f2->SetInput(out1);
    f2->SetOutputArrayName("Result");
    f2->SetComponent(1);
    if (!f2->Execute()) {
        std::cout << "FAIL: second Execute (" << f2->GetMessage() << ")\n";
        return false;
    }
    auto out2 = iGame::DynamicCast<iGame::UnstructuredMesh>(f2->GetOutput());
    if (out2 == nullptr) {
        std::cout << "FAIL: second output is not UnstructuredMesh\n";
        return false;
    }

    // 结果属性集中名为 Result 的活属性唯一（旧 Result 被覆盖删除）
    int count = 0;
    auto all = out2->GetAttributeSet()->GetAllAttributes();
    for (IGsize i = 0; i < all->GetNumberOfElements(); ++i) {
        auto& attr = all->GetElement(i);
        if (!attr.IsNone() && attr.pointer->GetName() == "Result") ++count;
    }
    if (count != 1) {
        std::cout << "FAIL: Result should be unique (count=" << count << ")\n";
        return false;
    }

    // 值 = 第二次提取的分量（输入 vec 的第 1 分量 → 1,4,7,10）
    auto& attr = out2->GetAttributeSet()->GetScalar("Result");
    auto arr = attr.pointer;
    bool ok = (arr != nullptr);
    for (IGsize i = 0; ok && i < arr->GetNumberOfElements(); ++i) {
        ok = (arr->GetValue(i) == ExpectPointValue(i, 1));
    }
    // 触发 dataRange 懒计算（修复前此处为空数组越界 → 回归失败）
    auto range = attr.GetDataRange();
    ok = ok && (range != nullptr) && (range->GetValue(2) == ExpectPointValue(0, 1))
         && (range->GetValue(3) == ExpectPointValue(3, 1));
    return ok;
}

// 真实数据验证：值与输入分量一致、挂载跟随、dataRange 等于手动扫描的分量范围、输入不被修改
bool VerifyRealData(iGame::UnstructuredMesh::Pointer mesh, const std::string& inputName,
                    int component, const std::string& outputName) {
    const unsigned int pointsMTimeBefore = InputPointsMTime(mesh);
    auto filter = iGame::ExtractComponentFilter::New();
    filter->SetInput(mesh);
    if (!inputName.empty()) filter->SetInputArrayName(inputName);
    filter->SetOutputArrayName(outputName);
    filter->SetComponent(component);
    if (!filter->Execute()) {
        std::cout << "FAIL: Execute (" << filter->GetMessage() << ")\n";
        return false;
    }
    auto outMesh = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    if (outMesh == nullptr) {
        std::cout << "FAIL: output is not UnstructuredMesh\n";
        return false;
    }
    if (!mesh->GetAttributeSet()->GetAttribute(outputName).IsNone()) {
        std::cout << "FAIL: input should not be modified\n";
        return false;
    }
    if (InputPointsMTime(mesh) != pointsMTimeBefore) {
        std::cout << "FAIL: input geometry should not be marked modified\n";
        return false;
    }
    if (!VerifyGeometrySharing(mesh, outMesh)) { return false; }

    auto attrSet = mesh->GetAttributeSet();
    iGame::AttributeSet::Attribute inputAttr;
    if (inputName.empty()) {
        auto all = attrSet->GetAllAttributes();
        for (IGsize i = 0; i < all->GetNumberOfElements(); ++i) {
            auto& candidate = all->GetElement(i);
            if (!candidate.IsNone() && candidate.type == IG_VECTOR) {
                inputAttr = candidate;
                break;
            }
        }
    } else {
        inputAttr = attrSet->GetAttribute(inputName, IG_VECTOR);
    }
    if (inputAttr.IsNone()) {
        std::cout << "FAIL: input attribute not found\n";
        return false;
    }

    auto inArr = inputAttr.pointer;
    auto& outAttr = outMesh->GetAttributeSet()->GetScalar(outputName);
    auto outArr = outAttr.pointer;
    bool ok = (outArr != nullptr) && (outAttr.attachmentType == inputAttr.attachmentType)
              && (outArr->GetNumberOfElements() == inArr->GetNumberOfElements());

    double minV = DBL_MAX, maxV = DBL_MIN;
    for (IGsize i = 0; ok && i < inArr->GetNumberOfElements(); ++i) {
        double v = inArr->GetElementValue(i, component);
        minV = std::min(minV, v);
        maxV = std::max(maxV, v);
        ok = (outArr->GetValue(i) == v);
    }

    auto range = outAttr.GetDataRange();
    ok = ok && (range != nullptr) && (range->GetValue(2) == minV) && (range->GetValue(3) == maxV);
    return ok;
}

// VTK 官方示例 ExtractComponents.cxx 的用法：同一输入建三个 filter，分别提取 0/1/2 分量再逐路核对
bool VerifyRealDataAllComponents(iGame::UnstructuredMesh::Pointer mesh, const std::string& inputName) {
    auto* inArr = mesh->GetAttributeSet()->GetArrayPointer(IG_VECTOR, IG_POINT, inputName);
    if (inArr == nullptr) {
        std::cout << "FAIL: input vector not found: " << inputName << "\n";
        return false;
    }
    for (int comp = 0; comp < 3; ++comp) {
        const std::string outputName = "Component" + std::to_string(comp);
        auto filter = iGame::ExtractComponentFilter::New();
        filter->SetInput(mesh);
        filter->SetInputArrayName(inputName);
        filter->SetOutputArrayName(outputName);
        filter->SetComponent(comp);
        if (!filter->Execute()) {
            std::cout << "FAIL: Execute (component " << comp << ": " << filter->GetMessage() << ")\n";
            return false;
        }
        auto outMesh = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
        if (outMesh == nullptr) {
            std::cout << "FAIL: output is not UnstructuredMesh (component " << comp << ")\n";
            return false;
        }
        auto& attr = outMesh->GetAttributeSet()->GetScalar(outputName);
        auto arr = attr.pointer;
        bool ok = (arr != nullptr) && (arr->GetNumberOfElements() == inArr->GetNumberOfElements());
        for (IGsize i = 0; ok && i < inArr->GetNumberOfElements(); ++i) {
            ok = (arr->GetValue(i) == inArr->GetElementValue(i, comp));
        }
        if (!ok) {
            std::cout << "FAIL: component " << comp << " values mismatch\n";
            return false;
        }
    }
    return true;
}

// 多分量提取：SetComponents(c1) / (c1, c2) / (c1, c2, c3)，输出维度 = 分量个数（对齐 VTK）
bool VerifyMultiComponent() {
    bool allOk = true;

    // (0,1)：输出 2 维，逐元素 = 输入的第 0 / 第 1 分量
    {
        auto mesh = CreateMeshWithDimVector(3);
        auto filter = iGame::ExtractComponentFilter::New();
        filter->SetInput(mesh);
        filter->SetOutputArrayName("XY");
        filter->SetComponents(0, 1);
        if (!filter->Execute()) {
            std::cout << "FAIL: Execute (0,1) (" << filter->GetMessage() << ")\n";
            return false;
        }
        const int* comps = filter->GetComponents();
        bool ok = (filter->GetNumberOfComponents() == 2) && (comps[0] == 0) && (comps[1] == 1);
        auto outMesh = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
        auto& attr = outMesh->GetAttributeSet()->GetScalar("XY");
        auto arr = attr.pointer;
        ok = ok && (arr != nullptr) && (arr->GetDimension() == 2) && (arr->GetNumberOfElements() == 4);
        for (IGsize i = 0; ok && i < 4; ++i) {
            ok = (arr->GetElementValue(i, 0) == ExpectPointValue(i, 0)) &&
                 (arr->GetElementValue(i, 1) == ExpectPointValue(i, 1));
        }
        // dataRange 的 magnitude 段 = 各元素模长的 [min, max]
        auto range = attr.GetDataRange();
        ok = ok && (range != nullptr) && (std::fabs(range->GetValue(0) - 1.0) < 1e-6) &&
             (std::fabs(range->GetValue(1) - std::sqrt(181.0)) < 1e-6);
        if (!ok) std::cout << "FAIL: SetComponents(0, 1)\n";
        allOk = allOk && ok;
    }

    // (2,0)：分量顺序按参数保留
    {
        auto mesh = CreateMeshWithDimVector(3);
        auto filter = iGame::ExtractComponentFilter::New();
        filter->SetInput(mesh);
        filter->SetOutputArrayName("ZX");
        filter->SetComponents(2, 0);
        if (!filter->Execute()) {
            std::cout << "FAIL: Execute (2,0) (" << filter->GetMessage() << ")\n";
            return false;
        }
        auto outMesh = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
        auto arr = outMesh->GetAttributeSet()->GetScalar("ZX").pointer;
        bool ok = (arr != nullptr) && (arr->GetDimension() == 2);
        for (IGsize i = 0; ok && i < 4; ++i) {
            ok = (arr->GetElementValue(i, 0) == ExpectPointValue(i, 2)) &&
                 (arr->GetElementValue(i, 1) == ExpectPointValue(i, 0));
        }
        if (!ok) std::cout << "FAIL: SetComponents(2, 0) should keep the given order\n";
        allOk = allOk && ok;
    }

    // (0,1,2)：等价于原向量；随后 SetComponent(1) 回到单分量
    {
        auto mesh = CreateMeshWithDimVector(3);
        auto filter = iGame::ExtractComponentFilter::New();
        filter->SetInput(mesh);
        filter->SetOutputArrayName("XYZ");
        filter->SetComponents(0, 1, 2);
        if (!filter->Execute()) {
            std::cout << "FAIL: Execute (0,1,2) (" << filter->GetMessage() << ")\n";
            return false;
        }
        auto outMesh = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
        auto arr = outMesh->GetAttributeSet()->GetScalar("XYZ").pointer;
        bool ok = (arr != nullptr) && (arr->GetDimension() == 3) && (filter->GetNumberOfComponents() == 3);
        for (IGsize i = 0; ok && i < 4; ++i) {
            for (int c = 0; ok && c < 3; ++c) {
                ok = (arr->GetElementValue(i, c) == ExpectPointValue(i, c));
            }
        }
        auto single = iGame::ExtractComponentFilter::New();
        single->SetInput(mesh);
        single->SetOutputArrayName("Y");
        single->SetComponents(0, 1, 2);
        single->SetComponent(1);  // 单分量别名：分量个数回到 1
        ok = ok && (single->GetNumberOfComponents() == 1) && (single->GetComponent() == 1);
        ok = ok && single->Execute();
        if (ok) {
            auto singleOut = iGame::DynamicCast<iGame::UnstructuredMesh>(single->GetOutput());
            auto singleArr = singleOut->GetAttributeSet()->GetScalar("Y").pointer;
            ok = (singleArr != nullptr) && (singleArr->GetDimension() == 1);
            for (IGsize i = 0; ok && i < 4; ++i) {
                ok = (singleArr->GetValue(i) == ExpectPointValue(i, 1));
            }
        }
        if (!ok) std::cout << "FAIL: SetComponents(0, 1, 2) / SetComponent(1)\n";
        allOk = allOk && ok;
    }

    // 非法分量：超出维度（2 维数组取第 3 个分量）、负值 —— 都必须失败并给出消息
    {
        auto mesh2d = CreateMeshWithDimVector(2);
        auto f = iGame::ExtractComponentFilter::New();
        f->SetInput(mesh2d);
        f->SetOutputArrayName("Bad");
        f->SetComponents(0, 2);
        bool ok = !f->Execute() && !f->GetMessage().empty() && (f->GetOutput() == nullptr);
        if (!ok) std::cout << "FAIL: out-of-range component should reject\n";
        allOk = allOk && ok;

        auto mesh3d = CreateMeshWithDimVector(3);
        auto fNeg = iGame::ExtractComponentFilter::New();
        fNeg->SetInput(mesh3d);
        fNeg->SetOutputArrayName("Bad");
        fNeg->SetComponent(-1);
        bool negOk = !fNeg->Execute() && !fNeg->GetMessage().empty();
        if (!negOk) std::cout << "FAIL: negative component should reject\n";
        allOk = allOk && negOk;
    }

    return allOk;
}

// 属性集拷贝策略：默认深拷贝（结果与输入解耦）；SetShallowCopyAttributes(true) 改为只读共享
bool VerifyAttributeCopyPolicy() {
    bool allOk = true;

    // 默认深拷贝：结果里的输入属性是副本（不同数组对象 / 不同缓冲），改输入不影响结果
    {
        auto mesh = CreateMeshWithPointVector();
        auto* inVec = mesh->GetAttributeSet()->GetArrayPointer(IG_VECTOR, IG_POINT, "vec");
        auto filter = iGame::ExtractComponentFilter::New();
        filter->SetInput(mesh);
        filter->SetOutputArrayName("Result");
        filter->SetComponent(0);
        if (!filter->Execute()) {
            std::cout << "FAIL: Execute (" << filter->GetMessage() << ")\n";
            return false;
        }
        auto outMesh = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
        auto* outVec = outMesh->GetAttributeSet()->GetArrayPointer(IG_VECTOR, IG_POINT, "vec");
        bool ok = (inVec != nullptr) && (outVec != nullptr) && (outVec != inVec) &&
                  (outVec->GetArrayType() == inVec->GetArrayType()) &&
                  (outVec->GetDimension() == inVec->GetDimension()) &&
                  (outVec->GetNumberOfElements() == inVec->GetNumberOfElements());
        // 解耦校验：改输入数组后，结果里的副本保持不变（说明是各自独立的缓冲）
        const double before = (inVec != nullptr) ? inVec->GetElementValue(0, 0) : 0.0;
        if (inVec != nullptr) inVec->SetValue(0, 999.0);
        ok = ok && (outVec != nullptr) && (outVec->GetElementValue(0, 0) == before);
        if (!ok) std::cout << "FAIL: default should deep copy input attributes\n";
        allOk = allOk && ok;
    }

    // 共享模式：结果直接引用输入数组（省内存，代价是输入被改时结果同步变化）
    {
        auto mesh = CreateMeshWithPointVector();
        auto* inVec = mesh->GetAttributeSet()->GetArrayPointer(IG_VECTOR, IG_POINT, "vec");
        auto filter = iGame::ExtractComponentFilter::New();
        filter->SetInput(mesh);
        filter->SetOutputArrayName("Result");
        filter->SetComponent(0);
        filter->SetShallowCopyAttributes(true);
        if (!filter->Execute()) {
            std::cout << "FAIL: Execute (shallow) (" << filter->GetMessage() << ")\n";
            return false;
        }
        auto outMesh = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
        auto* outVec = outMesh->GetAttributeSet()->GetArrayPointer(IG_VECTOR, IG_POINT, "vec");
        bool ok = (inVec != nullptr) && (outVec == inVec) && (filter->GetShallowCopyAttributes());
        if (inVec != nullptr) inVec->SetValue(0, 777.0);
        ok = ok && (outVec != nullptr) && (outVec->GetElementValue(0, 0) == 777.0);
        if (!ok) std::cout << "FAIL: shallow copy should share input arrays\n";
        allOk = allOk && ok;
    }

    return allOk;
}

// 输出数组名为空：直接失败并给出消息，不保留输出
bool VerifyEmptyOutputName() {
    auto mesh = CreateMeshWithPointVector();
    auto filter = iGame::ExtractComponentFilter::New();
    filter->SetInput(mesh);
    filter->SetOutputArrayName("");
    filter->SetComponent(0);
    if (filter->Execute()) {
        std::cout << "FAIL: empty output name should reject\n";
        return false;
    }
    if (filter->GetMessage().empty()) {
        std::cout << "FAIL: empty output name should leave a message\n";
        return false;
    }
    if (filter->GetOutput() != nullptr) {
        std::cout << "FAIL: failed execution should not keep an output\n";
        return false;
    }
    return true;
}

// The output array keeps the same concrete type as the input (Int -> Int, LongLong -> LongLong, ...).
bool VerifyOutputTypePreserved() {
    bool allOk = true;

    // IntArray input
    auto meshInt = iGame::UnstructuredMesh::New();
    meshInt->AddPoint(iGame::Point(0.f, 0.f, 0.f));
    meshInt->AddPoint(iGame::Point(1.f, 0.f, 0.f));
    meshInt->AddPoint(iGame::Point(0.f, 1.f, 0.f));
    meshInt->AddPoint(iGame::Point(0.f, 0.f, 1.f));
    igIndex cell[4] = {0, 1, 2, 3};
    meshInt->AddCell(cell, 4, iGame::IG_TETRA);
    auto vecInt = iGame::IntArray::New();
    vecInt->SetName("vecInt");
    vecInt->SetDimension(3);
    for (int i = 0; i < 4; ++i) {
        int v[3] = {i, i + 1, i + 2};
        vecInt->AddElement(v);
    }
    meshInt->GetAttributeSet()->AddVector(IG_POINT, vecInt);

    auto fInt = iGame::ExtractComponentFilter::New();
    fInt->SetInput(meshInt);
    fInt->SetOutputArrayName("RInt");
    fInt->SetComponent(0);
    if (!fInt->Execute()) {
        std::cout << "FAIL: Int Execute (" << fInt->GetMessage() << ")\n";
        return false;
    }
    auto outInt = iGame::DynamicCast<iGame::UnstructuredMesh>(fInt->GetOutput());
    if (outInt == nullptr) return false;
    auto& attrInt = outInt->GetAttributeSet()->GetScalar("RInt");
    bool intOk = (attrInt.pointer != nullptr) && (attrInt.pointer->GetArrayType() == IG_IntArray);
    for (IGsize i = 0; intOk && i < attrInt.pointer->GetNumberOfElements(); ++i) {
        intOk = (attrInt.pointer->GetValue(i) == static_cast<double>(i));
    }
    if (!intOk) std::cout << "FAIL: IntArray output type\n";
    allOk = allOk && intOk;

    // LongLongArray input
    auto meshLL = iGame::UnstructuredMesh::New();
    meshLL->AddPoint(iGame::Point(0.f, 0.f, 0.f));
    meshLL->AddPoint(iGame::Point(1.f, 0.f, 0.f));
    meshLL->AddPoint(iGame::Point(0.f, 1.f, 0.f));
    meshLL->AddPoint(iGame::Point(0.f, 0.f, 1.f));
    meshLL->AddCell(cell, 4, iGame::IG_TETRA);
    auto vecLL = iGame::LongLongArray::New();
    vecLL->SetName("vecLL");
    vecLL->SetDimension(3);
    for (int i = 0; i < 4; ++i) {
        int64_t v[3] = {10LL + i, 20LL + i, 30LL + i};
        vecLL->AddElement(v);
    }
    meshLL->GetAttributeSet()->AddVector(IG_POINT, vecLL);

    auto fLL = iGame::ExtractComponentFilter::New();
    fLL->SetInput(meshLL);
    fLL->SetOutputArrayName("RLL");
    fLL->SetComponent(0);
    if (!fLL->Execute()) {
        std::cout << "FAIL: LongLong Execute (" << fLL->GetMessage() << ")\n";
        return false;
    }
    auto outLL = iGame::DynamicCast<iGame::UnstructuredMesh>(fLL->GetOutput());
    if (outLL == nullptr) return false;
    auto& attrLL = outLL->GetAttributeSet()->GetScalar("RLL");
    bool llOk = (attrLL.pointer != nullptr) && (attrLL.pointer->GetArrayType() == IG_LongLongArray);
    for (IGsize i = 0; llOk && i < attrLL.pointer->GetNumberOfElements(); ++i) {
        llOk = (attrLL.pointer->GetValue(i) == 10.0 + i);
    }
    if (!llOk) std::cout << "FAIL: LongLongArray output type\n";
    allOk = allOk && llOk;

    return allOk;
}

// Same-named arrays on PointData and CellData are distinguished by attachment type.
bool VerifyAttachmentSelection() {
    auto mesh = CreateMeshWithCellVector();
    // Add a same-named Point vector ("cellVec" already exists on CellData).
    auto pv = iGame::FloatArray::New();
    pv->SetName("cellVec");
    pv->SetDimension(3);
    for (int i = 0; i < 5; ++i) {
        pv->AddElement3(1.f + 3.f * i, 2.f + 3.f * i, 3.f + 3.f * i);
    }
    mesh->GetAttributeSet()->AddVector(IG_POINT, pv);

    // Restrict to IG_POINT -> point values 1, 4, 7, 10, 13
    auto fP = iGame::ExtractComponentFilter::New();
    fP->SetInput(mesh);
    fP->SetInputArrayName("cellVec");
    fP->SetInputAttachmentType(IG_POINT);
    fP->SetOutputArrayName("RP");
    fP->SetComponent(0);
    if (!fP->Execute()) {
        std::cout << "FAIL: Point Execute (" << fP->GetMessage() << ")\n";
        return false;
    }
    auto outP = iGame::DynamicCast<iGame::UnstructuredMesh>(fP->GetOutput());
    if (outP == nullptr) return false;
    auto& attrP = outP->GetAttributeSet()->GetScalar("RP");
    bool pOk = (attrP.pointer != nullptr) && (attrP.attachmentType == IG_POINT)
               && (attrP.pointer->GetNumberOfElements() == 5);
    for (IGsize i = 0; pOk && i < attrP.pointer->GetNumberOfElements(); ++i) {
        pOk = (attrP.pointer->GetValue(i) == 1.0 + 3.0 * i);
    }
    if (!pOk) std::cout << "FAIL: Point attachment selection\n";

    // Restrict to IG_CELL -> cell values 10, 40
    auto fC = iGame::ExtractComponentFilter::New();
    fC->SetInput(mesh);
    fC->SetInputArrayName("cellVec");
    fC->SetInputAttachmentType(IG_CELL);
    fC->SetOutputArrayName("RC");
    fC->SetComponent(0);
    if (!fC->Execute()) {
        std::cout << "FAIL: Cell Execute (" << fC->GetMessage() << ")\n";
        return false;
    }
    auto outC = iGame::DynamicCast<iGame::UnstructuredMesh>(fC->GetOutput());
    if (outC == nullptr) return false;
    auto& attrC = outC->GetAttributeSet()->GetScalar("RC");
    bool cOk = (attrC.pointer != nullptr) && (attrC.attachmentType == IG_CELL)
               && (attrC.pointer->GetNumberOfElements() == 2);
    if (cOk) {
        cOk = (attrC.pointer->GetValue(0) == 10.0) && (attrC.pointer->GetValue(1) == 40.0);
    }
    if (!cOk) std::cout << "FAIL: Cell attachment selection\n";

    return pOk && cOk;
}
}  // namespace

int main(int argc, char* argv[]) {
    bool allOk = true;

    // 默认使用仓库自带的 AI 测试模型，无参即可完整运行；第一个命令行参数可覆盖主模型路径
    const std::string mainModel =
            (argc > 1) ? std::string(argv[1]) : std::string("./Models/ExtractComponent_FlowPipe.vtk");

    // 真实模型（直管道螺旋流）：空名取首向量 X、显式名 V 取 Y、VTK 示例式 0/1/2 三路拆分
    auto pipeMesh = ReadModelFromFile(mainModel);
    if (pipeMesh == nullptr) return 1;

    bool pipeDefaultOk = VerifyRealData(pipeMesh, "", 0, "Result");
    std::cout << (pipeDefaultOk ? "PASS" : "FAIL") << ": real data (FlowPipe), empty input name -> first vector, X\n";
    allOk = allOk && pipeDefaultOk;

    bool pipeExplicitOk = VerifyRealData(pipeMesh, "V", 1, "ResultV");
    std::cout << (pipeExplicitOk ? "PASS" : "FAIL") << ": real data (FlowPipe), explicit input name V, Y\n";
    allOk = allOk && pipeExplicitOk;

    bool pipeSplitOk = VerifyRealDataAllComponents(pipeMesh, "V");
    std::cout << (pipeSplitOk ? "PASS" : "FAIL") << ": real data (FlowPipe), components 0/1/2 (VTK example style)\n";
    allOk = allOk && pipeSplitOk;

    // 第二个模型（90° 弯管）：显式名 V 取 Z
    auto bendMesh = ReadModelFromFile("./Models/ExtractComponent_BendPipe.vtk");
    if (bendMesh == nullptr) return 1;

    bool bendZOk = VerifyRealData(bendMesh, "V", 2, "ResultZ");
    std::cout << (bendZOk ? "PASS" : "FAIL") << ": real data (BendPipe), explicit input name V, Z\n";
    allOk = allOk && bendZOk;

    bool xOk = VerifyExtract(0, "Result", IG_POINT);
    std::cout << (xOk ? "PASS" : "FAIL") << ": extract X -> Result\n";
    allOk = allOk && xOk;

    bool yOk = VerifyExtract(1, "Result", IG_POINT);
    std::cout << (yOk ? "PASS" : "FAIL") << ": extract Y -> Result\n";
    allOk = allOk && yOk;

    bool zOk = VerifyExtract(2, "Result", IG_POINT);
    std::cout << (zOk ? "PASS" : "FAIL") << ": extract Z -> Result\n";
    allOk = allOk && zOk;

    bool customOk = VerifyExtract(0, "MyScalar", IG_POINT);
    std::cout << (customOk ? "PASS" : "FAIL") << ": custom output name MyScalar\n";
    allOk = allOk && customOk;

    bool cellOk = VerifyExtractCell();
    std::cout << (cellOk ? "PASS" : "FAIL") << ": cell data attachment (IG_CELL)\n";
    allOk = allOk && cellOk;

    bool defaultOk = VerifyInputArraySelection(false);
    std::cout << (defaultOk ? "PASS" : "FAIL") << ": empty input name -> first vector vec\n";
    allOk = allOk && defaultOk;

    bool explicitOk = VerifyInputArraySelection(true);
    std::cout << (explicitOk ? "PASS" : "FAIL") << ": explicit input name test_2\n";
    allOk = allOk && explicitOk;

    bool dupOk = VerifyOverwriteDuplicateName();
    std::cout << (dupOk ? "PASS" : "FAIL") << ": duplicate output name overwritten (unique)\n";
    allOk = allOk && dupOk;

    bool dimOk = VerifyDimensionGuard();
    std::cout << (dimOk ? "PASS" : "FAIL") << ": dimension guard (1D/2D arrays)\n";
    allOk = allOk && dimOk;

    bool multiOk = VerifyMultiComponent();
    std::cout << (multiOk ? "PASS" : "FAIL") << ": multi-component extraction + invalid component guard\n";
    allOk = allOk && multiOk;

    bool copyOk = VerifyAttributeCopyPolicy();
    std::cout << (copyOk ? "PASS" : "FAIL") << ": attribute copy policy (deep copy default / shallow switch)\n";
    allOk = allOk && copyOk;

    bool emptyNameOk = VerifyEmptyOutputName();
    std::cout << (emptyNameOk ? "PASS" : "FAIL") << ": empty output name rejected\n";
    allOk = allOk && emptyNameOk;

    bool regOk = VerifyExtractOnExtractedResult();
    std::cout << (regOk ? "PASS" : "FAIL") << ": extract on extracted result (regression)\n";
    allOk = allOk && regOk;

    bool typeOk = VerifyOutputTypePreserved();
    std::cout << (typeOk ? "PASS" : "FAIL") << ": output type preserved (Int/LongLong)\n";
    allOk = allOk && typeOk;

    bool attachOk = VerifyAttachmentSelection();
    std::cout << (attachOk ? "PASS" : "FAIL") << ": Point/Cell attachment selection\n";
    allOk = allOk && attachOk;

    return allOk ? 0 : 1;
}
