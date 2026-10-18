// 集成回归（待提交）：原标准菜单缺少本过滤器实现；保留源仓库的数值、类型及错误输入断言，
// 用于保证迁移后结果和边界行为一致，防止仅有菜单入口而算法不可用。
/**
 * @brief MedianFilter（结构化网格点标量中值滤波）回归自检，控制台程序、无渲染窗口。
 *
 * 覆盖四组场景：
 *   1) 合成 5×1×1 网格（IntArray）：确定性中值、边界收缩窗口、上中位数、整型数组类型保留；
 *   2) 合成 3×3×3 网格（FloatArray）：单点正/负尖刺被 3×3×3 中值完全抹平（输出全 0）；
 *   3) 真实模型 3DScalar.vti（21×21×21 点标量，DoubleArray）：
 *      - 输出与原模型的属性字段逐项一致（同名 / 同 IG_SCALAR / 同 IG_POINT / 同分量数 / 同数组类型）；
 *      - 采样点与独立参考实现交叉校验（3×3×3 与 5×5×5）；
 *      - 平滑性（总变差下降）、输出落在输入值域内、输入模型未被改写；
 *      - 输出可再次作为输入（串接），验证数组类型可被滤波器识别（回归“未知类型”）。
 *   4) 失败路径：非结构化网格 / 偶数核 / 无效属性名 / 矢量属性 / 多分量标量；
 *      以及同一实例“先失败再成功”时 GetMessage() 必须被清空（无残留文案）。
 *
 * 用法：testMedianFilter [3DScalar.vti]
 *       默认依次尝试 ./Models/3DScalar.vti、./Examples/Models/3DScalar.vti 等相对路径
 *       （CMake 的 iGameCopyExampleAssets 会把 Examples/Models 拷到构建目录）。
 */
#include <Median/iGameMedianFilter.h>

#include <iGameAttributeSet.h>
#include <iGameDataObject.h>
#include <iGameFileIO.h>
#include <iGameFlatArray.h>
#include <iGamePointSet.h>
#include <iGamePoints.h>
#include <iGameStructuredMesh.h>
#include <iGameType.h>
#include <iGameUnstructuredMesh.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

int g_checks = 0;
int g_failures = 0;

bool Check(bool ok, const std::string& what) {
    ++g_checks;
    if (!ok) { ++g_failures; }
    std::cout << "  [" << (ok ? "PASS" : "FAIL") << "] " << what << std::endl;
    return ok;
}

void Step(const std::string& name) { std::cout << "  >> " << name << std::endl; }

bool FileExists(const std::string& path) {
    std::ifstream f(path);
    return f.good();
}

// 依次尝试常见运行目录，找到模型后返回相对路径（与 TestShrinkModel.cpp 同款）
std::string FindModel(const std::string& name) {
    const std::string candidates[]{
        "./Models/" + name, "./Examples/Models/" + name, "../Examples/Models/" + name, "../../Examples/Models/" + name,
    };
    for (const auto& candidate: candidates) {
        if (FileExists(candidate)) { return candidate; }
    }
    return "";
}

// 结构化网格点索引约定：i + j*nx + k*nx*ny（与 StructuredMesh::GetPointIndex 一致）
IGsize PointIndex(igIndex nx, igIndex ny, igIndex i, igIndex j, igIndex k) {
    return static_cast<IGsize>(i) + static_cast<IGsize>(j) * nx + static_cast<IGsize>(k) * nx * ny;
}

/**
 * @brief 构造 nx×ny×nz 结构化网格（坐标即格点索引），并挂一个单分量点标量。
 * @param asInt true 用 IntArray（覆盖整型数组类型保留），false 用 FloatArray。
 */
iGame::StructuredMesh::Pointer MakeGrid(igIndex nx, igIndex ny, igIndex nz, const std::vector<double>& values,
                                       const std::string& scalarName, bool asInt) {
    auto mesh = iGame::StructuredMesh::New();
    mesh->SetName("MedianSynthetic");

    auto points = iGame::Points::New();
    for (igIndex k = 0; k < nz; ++k) {
        for (igIndex j = 0; j < ny; ++j) {
            for (igIndex i = 0; i < nx; ++i) {
                points->AddPoint(static_cast<float>(i), static_cast<float>(j), static_cast<float>(k));
            }
        }
    }
    mesh->SetPoints(points);

    igIndex dims[3]{nx, ny, nz};
    mesh->SetDimensionSize(dims);

    auto attributes = iGame::AttributeSet::New();
    if (asInt) {
        auto array = iGame::IntArray::New();
        array->SetName(scalarName);
        array->SetDimension(1);
        array->Resize(static_cast<IGsize>(values.size()));
        for (size_t i = 0; i < values.size(); ++i) { array->RawPointer()[i] = static_cast<int>(values[i]); }
        attributes->AddAttribute(IG_SCALAR, IG_POINT, array);
    } else {
        auto array = iGame::FloatArray::New();
        array->SetName(scalarName);
        array->SetDimension(1);
        array->Resize(static_cast<IGsize>(values.size()));
        for (size_t i = 0; i < values.size(); ++i) { array->RawPointer()[i] = static_cast<float>(values[i]); }
        attributes->AddAttribute(IG_SCALAR, IG_POINT, array);
    }
    mesh->SetAttributeSet(attributes);
    return mesh;
}

// 按单分量读回整个数组
std::vector<double> ReadValues(const iGame::ArrayObject::Pointer& array) {
    std::vector<double> values(static_cast<size_t>(array->GetNumberOfElements()), 0.0);
    for (IGsize i = 0; i < array->GetNumberOfElements(); ++i) { values[static_cast<size_t>(i)] = array->GetElementValue(i, 0); }
    return values;
}

/**
 * @brief 独立参考实现：核窗口内有效邻点（越界跳过）排序后取 count/2（上中位数）。
 *
 * 与滤波器分开书写，用于交叉校验索引约定、边界处理与中位数取法；
 * 中值只做选择不做算术，double 值可逐位精确比较。
 */
double ReferenceMedian(const iGame::ArrayObject::Pointer& array, const igIndex dims[3], igIndex i, igIndex j, igIndex k,
                       int kx, int ky, int kz) {
    std::vector<double> window;
    window.reserve(static_cast<size_t>(kx) * static_cast<size_t>(ky) * static_cast<size_t>(kz));
    for (int dk = -kz / 2; dk <= kz / 2; ++dk) {
        const igIndex kk = k + dk;
        if (kk < 0 || kk >= dims[2]) { continue; }
        for (int dj = -ky / 2; dj <= ky / 2; ++dj) {
            const igIndex jj = j + dj;
            if (jj < 0 || jj >= dims[1]) { continue; }
            for (int di = -kx / 2; di <= kx / 2; ++di) {
                const igIndex ii = i + di;
                if (ii < 0 || ii >= dims[0]) { continue; }
                window.push_back(array->GetElementValue(PointIndex(dims[0], dims[1], ii, jj, kk), 0));
            }
        }
    }
    std::sort(window.begin(), window.end());
    return window[window.size() / 2];
}

// 总变差（相邻点一阶差分绝对值之和）：中值滤波后应下降，用来度量“更平滑”
double TotalVariation(const iGame::ArrayObject::Pointer& array, const igIndex dims[3]) {
    const igIndex nx = dims[0];
    const igIndex ny = dims[1];
    const igIndex nz = dims[2] > 1 ? dims[2] : 1;
    double sum = 0.0;
    for (igIndex k = 0; k < nz; ++k) {
        for (igIndex j = 0; j < ny; ++j) {
            for (igIndex i = 0; i < nx; ++i) {
                const double v = array->GetElementValue(PointIndex(nx, ny, i, j, k), 0);
                if (i + 1 < nx) { sum += std::abs(array->GetElementValue(PointIndex(nx, ny, i + 1, j, k), 0) - v); }
                if (j + 1 < ny) { sum += std::abs(array->GetElementValue(PointIndex(nx, ny, i, j + 1, k), 0) - v); }
                if (k + 1 < nz) { sum += std::abs(array->GetElementValue(PointIndex(nx, ny, i, j, k + 1), 0) - v); }
            }
        }
    }
    return sum;
}

struct MedianRun {
    bool ok{false};
    std::string message;
    iGame::StructuredMesh::Pointer mesh{nullptr};
    iGame::ArrayObject::Pointer array{nullptr};
};

// 跑一次中值滤波并取回输出网格与滤波后的数组
MedianRun RunMedian(const iGame::DataObject::Pointer& input, const std::string& attributeName, int kx, int ky, int kz) {
    MedianRun result;
    auto filter = iGame::MedianFilter::New();
    filter->SetInput(input);
    filter->SetAttributeByName(attributeName);
    filter->SetKernelSize(kx, ky, kz);
    const bool executed = filter->Execute();
    result.message = filter->GetMessage(); // 成功时为空字符串（Execute() 开头会清空），失败时为具体原因
    if (!executed) { return result; }
    result.mesh = iGame::DynamicCast<iGame::StructuredMesh>(filter->GetOutput());
    if (result.mesh.IsNull()) {
        result.message = "output is not a StructuredMesh";
        return result;
    }
    auto attributes = result.mesh->GetAttributeSet();
    const int index = attributes->GetAttributeIndex(attributeName);
    if (index >= 0) { result.array = attributes->GetAttribute(index).pointer; }
    result.ok = result.array.IsNotNull();
    if (!result.ok) { result.message = "filtered array '" + attributeName + "' not found in output"; }
    return result;
}

// 采样点是否都与独立参考实现一致
bool MatchesReference(const iGame::ArrayObject::Pointer& input, const iGame::ArrayObject::Pointer& output,
                      const igIndex dims[3], const std::vector<igIndex>& samples, int kx, int ky, int kz) {
    bool allMatch = true;
    for (size_t s = 0; s + 2 < samples.size(); s += 3) {
        const igIndex i = samples[s], j = samples[s + 1], k = samples[s + 2];
        const double expected = ReferenceMedian(input, dims, i, j, k, kx, ky, kz);
        const double got = output->GetElementValue(PointIndex(dims[0], dims[1], i, j, k), 0);
        if (got != expected) {
            allMatch = false;
            std::cout << "      mismatch at (" << i << "," << j << "," << k << "): got " << got << ", expected "
                      << expected << std::endl;
        }
    }
    return allMatch;
}

// ---------------------------------------------------------------------------
// 场景一：合成 5×1×1 网格上的确定性中值
// ---------------------------------------------------------------------------
bool TestSyntheticLine() {
    std::cout << "\n== Test 1: synthetic 5x1x1 line, P = {1, 2, 100, 4, 5} (IntArray) ==" << std::endl;

    Step("build 5x1x1 structured grid with an IntArray point scalar");
    auto mesh = MakeGrid(5, 1, 1, {1, 2, 100, 4, 5}, "P", true);
    auto inAttrs = mesh->GetAttributeSet();
    const int inIndex = inAttrs->GetAttributeIndex("P");
    if (!Check(inIndex >= 0 && inAttrs->GetAttribute(inIndex).pointer->GetArrayType() == IG_IntArray,
               "input scalar 'P' is an IntArray")) {
        return false;
    }
    const std::vector<double> inputBefore = ReadValues(inAttrs->GetAttribute(inIndex).pointer);

    Step("run median, kernel 3x1x1");
    MedianRun run = RunMedian(mesh, "P", 3, 1, 1);
    if (!Check(run.ok, "Execute() succeeded")) {
        std::cout << "      message: " << run.message << std::endl;
        return false;
    }
    Check(run.message.empty(), "GetMessage() is empty after a successful Execute()");

    Check(run.mesh->GetName() == mesh->GetName() + "_Median", "output name == <input name>_Median");
    const igIndex* dims = run.mesh->GetDimensionSize();
    Check(dims[0] == 5 && dims[1] == 1 && dims[2] == 1, "output dimensions == 5x1x1 (unchanged)");
    Check(run.mesh->GetNumberOfPoints() == 5, "output point count == 5 (unchanged)");

    auto outAttrs = run.mesh->GetAttributeSet();
    const int outIndex = outAttrs->GetAttributeIndex("P");
    if (!Check(outIndex >= 0, "output keeps an attribute named 'P' (same name)")) { return false; }
    auto& outAttr = outAttrs->GetAttribute(outIndex);
    auto& inAttr = inAttrs->GetAttribute(inIndex);
    Check(outAttrs->GetNumberOfAttributes() == inAttrs->GetNumberOfAttributes(),
          "output attribute count == input attribute count");
    Check(outAttr.pointer->GetName() == inAttr.pointer->GetName(), "output array name == input array name");
    Check(outAttr.type == inAttr.type && outAttr.type == IG_SCALAR, "output attribute type == input (IG_SCALAR)");
    Check(outAttr.attachmentType == inAttr.attachmentType && outAttr.attachmentType == IG_POINT,
          "output attachment type == input (IG_POINT)");
    Check(outAttr.pointer->GetDimension() == 1, "output component count == 1");
    // 核心回归：必须是具体数组类，GetArrayType() 与输入一致（否则属性面板显示“未知类型”，也无法再次滤波）
    Check(outAttr.pointer->GetArrayType() == IG_IntArray, "output array type == IG_IntArray (same as input)");
    Check(outAttr.pointer->GetArrayType() == inAttr.pointer->GetArrayType(), "output GetArrayType() == input GetArrayType()");
    Check(outAttr.pointer->GetNumberOfElements() == 5, "output has one value per point (5)");

    // 期望：i=2 的尖刺 100 被邻域中值 4 取代；i=0 窗口收缩为 {1,2} 取上中位数 2；i=4 为 {4,5} 取 5
    const std::vector<double> expected{2, 2, 4, 5, 5};
    const std::vector<double> got = ReadValues(outAttr.pointer);
    Check(got == expected, "filtered values == {2, 2, 4, 5, 5} (upper median + shrunken boundary window)");

    Step("check input model is not modified");
    Check(ReadValues(inAttr.pointer) == inputBefore, "input array keeps its original values (1, 2, 100, 4, 5)");

    Step("run again with kernel 3x3x3 (ny=1: out-of-range neighbours in y are skipped)");
    MedianRun run333 = RunMedian(mesh, "P", 3, 3, 3);
    if (Check(run333.ok, "Execute() with 3x3x3 succeeded")) {
        Check(ReadValues(run333.array) == expected, "3x3x3 on a 5x1x1 grid gives the same result as 3x1x1");
    }
    return true;
}

// ---------------------------------------------------------------------------
// 场景二：合成 3×3×3 网格上的正/负尖刺
// ---------------------------------------------------------------------------
bool TestSyntheticSpike3D() {
    std::cout << "\n== Test 2: synthetic 3x3x3 grid with a positive/negative spike (FloatArray) ==" << std::endl;

    Step("build 3x3x3 grid, P[0] = 100, P[13] = -100, others 0");
    std::vector<double> values(27, 0.0);
    values[0] = 100.0;   // (i,j,k) = (0,0,0)
    values[13] = -100.0; // (i,j,k) = (1,1,1)
    auto mesh = MakeGrid(3, 3, 3, values, "P", false);

    Step("run median, kernel 3x3x3");
    MedianRun run = RunMedian(mesh, "P", 3, 3, 3);
    if (!Check(run.ok, "Execute() succeeded")) {
        std::cout << "      message: " << run.message << std::endl;
        return false;
    }

    Check(run.array->GetArrayType() == IG_FloatArray, "output array type == IG_FloatArray (same as input)");
    const std::vector<double> outValues = ReadValues(run.array);
    const bool allZero = std::all_of(outValues.begin(), outValues.end(), [](double v) { return v == 0.0; });
    Check(allZero, "both spikes are removed: all 27 filtered values == 0");

    auto inAttrs = mesh->GetAttributeSet();
    const std::vector<double> inValues = ReadValues(inAttrs->GetAttribute(inAttrs->GetAttributeIndex("P")).pointer);
    Check(inValues[0] == 100.0 && inValues[13] == -100.0, "input spikes are still there (100 at 0, -100 at 13)");
    return true;
}

// ---------------------------------------------------------------------------
// 场景三：真实模型 3DScalar.vti（21×21×21，DoubleArray 点标量）
// ---------------------------------------------------------------------------
bool TestScalarVolumeModel(const std::string& path) {
    std::cout << "\n== Test 3: real model (" << path << ") ==" << std::endl;

    Step("read model");
    auto input = iGame::FileIO::ReadFile(path);
    if (!Check(input.IsNotNull(), "FileIO::ReadFile returns a data object")) { return false; }
    auto inMesh = iGame::DynamicCast<iGame::StructuredMesh>(input);
    if (!Check(inMesh.IsNotNull(), "input is a StructuredMesh (ImageData -> StructuredMesh)")) { return false; }

    const IGsize expectedPoints = 21ull * 21ull * 21ull; // WholeExtent="0 20 0 20 0 20"
    const igIndex* inDims = inMesh->GetDimensionSize();
    std::cout << "      dimensions=" << inDims[0] << "x" << inDims[1] << "x" << inDims[2]
              << " points=" << inMesh->GetNumberOfPoints() << std::endl;
    Check(inDims[0] == 21 && inDims[1] == 21 && inDims[2] == 21, "input dimensions == 21x21x21");
    Check(inMesh->GetNumberOfPoints() == expectedPoints, "input point count == 9261");

    auto inAttrs = inMesh->GetAttributeSet();
    const int valueIdx = inAttrs->GetAttributeIndex("values");
    if (!Check(valueIdx >= 0, "input has a point scalar named 'values'")) { return false; }
    auto& inAttr = inAttrs->GetAttribute(valueIdx);
    Check(inAttr.type == IG_SCALAR, "input 'values' attribute type == IG_SCALAR");
    Check(inAttr.attachmentType == IG_POINT, "input 'values' attachment type == IG_POINT");
    Check(inAttr.pointer->GetDimension() == 1, "input 'values' component count == 1");
    Check(inAttr.pointer->GetArrayType() == IG_DoubleArray, "input 'values' array type == IG_DoubleArray (Float64)");
    Check(inAttr.pointer->GetNumberOfElements() == expectedPoints, "input 'values' has 9261 values");
    const std::vector<double> inputBefore = ReadValues(inAttr.pointer);

    const int maskIdx = inAttrs->GetAttributeIndex("vtkValidPointMask");
    if (maskIdx >= 0) {
        std::cout << "      note: input also carries 'vtkValidPointMask' (must survive untouched)" << std::endl;
    }

    // ---- 3x3x3 ----
    Step("run median, kernel 3x3x3 on 'values'");
    MedianRun run3 = RunMedian(input, "values", 3, 3, 3);
    if (!Check(run3.ok, "Execute() succeeded")) {
        std::cout << "      message: " << run3.message << std::endl;
        return false;
    }

    Check(run3.mesh->GetName() == input->GetName() + "_Median", "output name == <input name>_Median");
    const igIndex* outDims = run3.mesh->GetDimensionSize();
    Check(outDims[0] == inDims[0] && outDims[1] == inDims[1] && outDims[2] == inDims[2],
          "output dimensions == input dimensions");
    Check(run3.mesh->GetNumberOfPoints() == inMesh->GetNumberOfPoints(), "output point count == input point count");
    {
        const float* inPts = inMesh->GetPoints()->RawPointer();
        const float* outPts = run3.mesh->GetPoints()->RawPointer();
        bool same = true;
        const IGsize sampleIds[]{0ull, 13ull, 441ull, expectedPoints / 2, expectedPoints - 1};
        for (IGsize id: sampleIds) {
            for (int c = 0; c < 3; ++c) {
                if (inPts[id * 3 + c] != outPts[id * 3 + c]) { same = false; }
            }
        }
        Check(same, "output point coordinates == input coordinates (geometry unchanged)");
    }

    Step("compare attribute fields with the input model");
    auto outAttrs = run3.mesh->GetAttributeSet();
    const int outIdx = outAttrs->GetAttributeIndex("values");
    if (!Check(outIdx >= 0, "output has an attribute named 'values'")) { return false; }
    auto& outAttr = outAttrs->GetAttribute(outIdx);
    Check(outAttrs->GetNumberOfAttributes() == inAttrs->GetNumberOfAttributes(),
          "output attribute count == input attribute count (all attributes kept)");
    Check(outAttr.pointer->GetName() == inAttr.pointer->GetName(), "array name identical ('values')");
    Check(outAttr.type == inAttr.type, "attribute type identical (IG_SCALAR)");
    Check(outAttr.attachmentType == inAttr.attachmentType, "attachment type identical (IG_POINT)");
    Check(outAttr.pointer->GetDimension() == inAttr.pointer->GetDimension(),
          "component count identical (" + std::to_string(inAttr.pointer->GetDimension()) + ")");
    // 核心回归：输出数组类型必须与输入一致，否则属性面板显示“未知类型”，且无法再次滤波
    Check(outAttr.pointer->GetArrayType() == inAttr.pointer->GetArrayType(),
          "array type identical to input (GetArrayType() == IG_DoubleArray)");
    Check(outAttr.pointer->GetArrayType() == IG_DoubleArray, "output array is a concrete class (not IG_ARRAY_OBJECT)");
    Check(outAttr.pointer->GetNumberOfElements() == expectedPoints, "output has one value per input point (9261)");

    if (maskIdx >= 0) {
        const int outMaskIdx = outAttrs->GetAttributeIndex("vtkValidPointMask");
        Check(outMaskIdx >= 0, "untouched attribute 'vtkValidPointMask' kept in output");
        if (outMaskIdx >= 0) {
            auto& outMask = outAttrs->GetAttribute(outMaskIdx);
            bool allOnes = outMask.pointer->GetNumberOfElements() == expectedPoints;
            for (IGsize i = 0; allOnes && i < outMask.pointer->GetNumberOfElements(); ++i) {
                allOnes = outMask.pointer->GetElementValue(i, 0) == 1.0;
            }
            Check(allOnes, "untouched attribute values preserved (all ones)");
        }
    }

    Step("check filtered values");
    const std::vector<double> outValues = ReadValues(outAttr.pointer);
    const double inMin = *std::min_element(inputBefore.begin(), inputBefore.end());
    const double inMax = *std::max_element(inputBefore.begin(), inputBefore.end());
    const double outMin = *std::min_element(outValues.begin(), outValues.end());
    const double outMax = *std::max_element(outValues.begin(), outValues.end());
    size_t changed = 0;
    for (size_t i = 0; i < outValues.size(); ++i) {
        if (outValues[i] != inputBefore[i]) { ++changed; }
    }
    const double tvIn = TotalVariation(inAttr.pointer, inDims);
    const double tvOut = TotalVariation(outAttr.pointer, outDims);
    std::cout << "      input  range=[" << inMin << ", " << inMax << "] TV=" << tvIn << std::endl;
    std::cout << "      3x3x3  range=[" << outMin << ", " << outMax << "] TV=" << tvOut << " changed=" << changed << "/"
              << outValues.size() << std::endl;
    Check(inMin <= outMin && outMax <= inMax, "all filtered values stay inside the input value range");
    Check(changed > 0, "median filtering actually changed values");
    Check(tvOut < tvIn, "total variation decreased (field is smoother)");

    Step("cross-check sampled points against an independent window-median reference");
    const std::vector<igIndex> samples{
        0,  0,  0,  20, 0,  0,  0,  20, 0,  0,  0,  20, 20, 20, 20, 10, 10, 10, 0,  10, 10, 20, 10, 10,
        10, 0,  10, 10, 20, 10, 10, 10, 0,  10, 10, 20, 1,  1,  1,  19, 19, 19, 5,  17, 3,  13, 2,  18,
    };
    Check(MatchesReference(inAttr.pointer, outAttr.pointer, inDims, samples, 3, 3, 3),
          "16 sampled points match the independent reference (3x3x3)");

    Step("run median again on the output (chained filtering)");
    MedianRun chain = RunMedian(run3.mesh, "values", 3, 3, 3);
    Check(chain.ok, "output can be used as input again (array type recognised by the filter)");
    if (chain.ok) {
        Check(chain.array->GetArrayType() == IG_DoubleArray, "second pass keeps IG_DoubleArray");
        Check(chain.mesh->GetNumberOfPoints() == expectedPoints, "second pass keeps the point count");
    }

    // ---- 5x5x5 ----
    Step("run median, kernel 5x5x5");
    MedianRun run5 = RunMedian(input, "values", 5, 5, 5);
    if (Check(run5.ok, "Execute() with 5x5x5 succeeded")) {
        const std::vector<double> out5 = ReadValues(run5.array);
        const double tv5 = TotalVariation(run5.array, run5.mesh->GetDimensionSize());
        size_t changed5 = 0;
        for (size_t i = 0; i < out5.size(); ++i) {
            if (out5[i] != inputBefore[i]) { ++changed5; }
        }
        std::cout << "      5x5x5  range=[" << *std::min_element(out5.begin(), out5.end()) << ", "
                  << *std::max_element(out5.begin(), out5.end()) << "] TV=" << tv5 << " changed=" << changed5 << "/"
                  << out5.size() << std::endl;
        Check(run5.array->GetArrayType() == IG_DoubleArray, "5x5x5 output array type == IG_DoubleArray");
        Check(tv5 < tvOut, "larger kernel is smoother (TV(5x5x5) < TV(3x3x3))");
        Check(out5 != outValues, "5x5x5 result differs from 3x3x3 result");
        Check(MatchesReference(inAttr.pointer, run5.array, inDims, samples, 5, 5, 5),
              "16 sampled points match the independent reference (5x5x5)");
    }

    Step("check input model is not modified by any of the runs");
    Check(ReadValues(inAttr.pointer) == inputBefore, "input 'values' keeps its original 9261 values");
    return true;
}

// ---------------------------------------------------------------------------
// 场景四：失败路径
// ---------------------------------------------------------------------------
bool TestFailurePaths() {
    std::cout << "\n== Test 4: failure paths ==" << std::endl;

    // 非结构化网格：直接拒绝
    Step("non-structured input");
    auto unstructured = iGame::UnstructuredMesh::New();
    {
        auto filter = iGame::MedianFilter::New();
        filter->SetInput(unstructured);
        filter->SetAttributeByName("P");
        const bool executed = filter->Execute();
        const std::string message = filter->GetMessage();
        std::cout << "      message: " << message << std::endl;
        Check(!executed, "Execute() rejects a non-structured input");
        Check(message.find("structured mesh") != std::string::npos,
              "message points at the structured-mesh requirement");
    }

    // 合成网格：偶数核 / 无效属性名 / 矢量属性 / 多分量标量
    auto mesh = MakeGrid(5, 1, 1, {1, 2, 3, 4, 5}, "P", false);
    auto attributes = mesh->GetAttributeSet();
    {
        auto vector = iGame::FloatArray::New();
        vector->SetName("V3");
        vector->SetDimension(3);
        vector->Resize(5);
        for (IGsize i = 0; i < 5; ++i) {
            vector->RawPointer()[i * 3 + 0] = static_cast<float>(i);
            vector->RawPointer()[i * 3 + 1] = static_cast<float>(i);
            vector->RawPointer()[i * 3 + 2] = static_cast<float>(i);
        }
        attributes->AddAttribute(IG_VECTOR, IG_POINT, vector);

        auto multi = iGame::FloatArray::New();
        multi->SetName("S2");
        multi->SetDimension(2);
        multi->Resize(5);
        for (IGsize i = 0; i < 10; ++i) { multi->RawPointer()[i] = static_cast<float>(i); }
        attributes->AddAttribute(IG_SCALAR, IG_POINT, multi);
    }

    Step("even kernel size");
    {
        auto filter = iGame::MedianFilter::New();
        filter->SetInput(mesh);
        filter->SetAttributeByName("P");
        filter->SetKernelSize(4, 4, 4);
        const bool executed = filter->Execute();
        const std::string message = filter->GetMessage();
        std::cout << "      message: " << message << std::endl;
        Check(!executed, "Execute() rejects an even kernel size");
        Check(message.find("odd") != std::string::npos, "message points at the odd-kernel requirement");
    }

    Step("unknown attribute name");
    {
        auto filter = iGame::MedianFilter::New();
        filter->SetInput(mesh);
        filter->SetAttributeByName("NoSuchArray");
        const bool executed = filter->Execute();
        const std::string message = filter->GetMessage();
        std::cout << "      message: " << message << std::endl;
        Check(!executed, "Execute() rejects an unknown attribute name");
        Check(message.find("valid scalar attribute") != std::string::npos, "message asks for a valid scalar attribute");
    }

    Step("vector attribute");
    {
        auto filter = iGame::MedianFilter::New();
        filter->SetInput(mesh);
        filter->SetAttributeByName("V3");
        const bool executed = filter->Execute();
        const std::string message = filter->GetMessage();
        std::cout << "      message: " << message << std::endl;
        Check(!executed, "Execute() rejects a vector attribute");
        Check(message.find("only processes scalar attributes") != std::string::npos,
              "message says only scalars are supported");
    }

    Step("multi-component scalar");
    {
        auto filter = iGame::MedianFilter::New();
        filter->SetInput(mesh);
        filter->SetAttributeByName("S2");
        const bool executed = filter->Execute();
        const std::string message = filter->GetMessage();
        std::cout << "      message: " << message << std::endl;
        Check(!executed, "Execute() rejects a 2-component scalar");
        Check(message.find("single-component scalar") != std::string::npos,
              "message asks for a single-component scalar");
    }

    // 回归用例（评审 P1：长度不足时按点索引越界读取）：
    // 触发条件：网格 4 个点，所选标量数组只提供 2 个值（截断的 .vti / 手写数组）。
    // 错误表现（修复前）：Execute() 返回成功，随后用 in[idx] 读到第 3、4 个不存在的元素。
    // 期望行为：Execute() 失败并给出原因，不产生输出。
    // 该修复与用例同属提交「add: median filter script and docs」；查询命令：
    //   git log --diff-filter=A --format="%h %s" -- Examples/Filter/Median/TestMedianFilter.cpp
    Step("incomplete scalar array (fewer values than mesh points)");
    {
        auto shortMesh = MakeGrid(4, 1, 1, {1.0, 2.0}, "P", false); // 网格 4 点，数组只给 2 个值
        auto filter = iGame::MedianFilter::New();
        filter->SetInput(shortMesh);
        filter->SetAttributeByName("P");
        const bool executed = filter->Execute();
        const std::string message = filter->GetMessage();
        std::cout << "      message: " << message << std::endl;
        Check(!executed, "Execute() rejects an array shorter than the mesh point count");
        Check(message.find("incomplete") != std::string::npos, "message reports incomplete data");
    }

    Step("reuse one filter instance: fail first, then succeed (the stale message must be cleared)");
    {
        auto filter = iGame::MedianFilter::New();
        filter->SetInput(mesh);
        filter->SetAttributeByName("P");
        filter->SetKernelSize(4, 4, 4); // 偶数核 -> 失败
        const bool firstFailed = !filter->Execute();
        const std::string firstMessage = filter->GetMessage();
        filter->SetKernelSize(3, 3, 3); // 奇数核 -> 成功
        const bool secondSucceeded = filter->Execute();
        const std::string secondMessage = filter->GetMessage();
        Check(firstFailed && !firstMessage.empty(), "even kernel fails and GetMessage() reports a reason");
        Check(secondSucceeded && secondMessage.empty(),
              "the same instance then succeeds and GetMessage() is cleared (no stale failure text)");
    }
    return true;
}

} // namespace

int main(int argc, char** argv) {
    std::cout << "MedianFilter self check" << std::endl;

    bool ok = true;
    ok &= TestSyntheticLine();
    ok &= TestSyntheticSpike3D();

    std::string modelPath;
    if (argc > 1) {
        modelPath = argv[1];
    } else {
        modelPath = FindModel("3DScalar.vti");
    }
    if (modelPath.empty()) {
        std::cout << "\nMODEL NOT FOUND: 3DScalar.vti (pass the file path as argv[1])" << std::endl;
        ok = false;
    } else {
        ok &= TestScalarVolumeModel(modelPath);
    }

    ok &= TestFailurePaths();

    std::cout << "\n" << (g_checks - g_failures) << "/" << g_checks << " checks passed" << std::endl;
    if (ok && g_failures == 0) {
        std::cout << "ALL TESTS PASSED" << std::endl;
        return 0;
    }
    std::cout << "SOME TESTS FAILED" << std::endl;
    return 1;
}
