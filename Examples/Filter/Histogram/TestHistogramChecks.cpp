// 迁移回归（待提交）：覆盖标准过滤器的算法/界面行为及源测试中的边界条件，防止仅迁移菜单占位而遗漏实现。
// ============================================================================
// Histogram 数值/逻辑一致性回归检查（无窗口，可直接进 CI）
//
// 唯一测试模型：Models/Histogram_test.vtk（20 点 / 4 三角形；点数组 v 与 vec(3 分量)，
//   单元数组 c；点与单元的 vtkGhostType 都写在 FIELD FieldData 里 —— 同时回归 legacy VTK
//   读取器对 unsigned_char 的数值解析、FIELD 数组名小写化，以及 blanking 不参与统计）。
//
// 期望值全部由 ParaView 6.2 / VTK vtkExtractHistogram 实测取得（验证记录见
//   doc/FilterUserNotice/HistogramFilter使用说明.md），硬编码在此以防实现漂移：
//   v  10 bin（点，blanked 掉两个元组）：[2,1,1,2,2,2,2,2,2,2]，总计 18，范围 [0,19]
//   v  10 bin 居中分箱：同一组计数，首/末边界 = 0 / 19
//   vec 分量 1 / 分量 3(=维数，取模长) 10 bin：计数相同，范围 [0,38] / [0,sqrt(5)*19]
//   c  3 bin / 10 bin（单元，blanked 掉两个单元）：[1,0,1] / [1,0,0,0,0,0,0,0,0,1]
//
// 失败时返回非 0，成功返回 0。运行方式（工作目录为 Examples 构建目录，Models 已拷贝到 ./Models）：
//     ./testHistogramChecks
// ============================================================================
#include <Histogram/iGameHistogramFilter.h>
#include <iGameAttributeSet.h>
#include <iGameFileIO.h>
#include <iGameHistogramData.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace iGame;

namespace {

int g_failed = 0;
int g_total = 0;

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
        if (separator != std::string::npos) {
            candidates.push_back(exePath.substr(0, separator + 1) + relative);
        }
    }
    for (const auto& candidate : candidates) {
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

bool Near(double got, double want, double tolerance = 1e-12) {
    return std::fabs(got - want) <= tolerance * std::max(1.0, std::fabs(want));
}

std::string ToText(const std::vector<long long>& values) {
    std::string text = "[";
    for (size_t i = 0; i < values.size(); ++i) {
        if (i != 0) { text += ", "; }
        text += std::to_string(values[i]);
    }
    return text + "]";
}

int FindAttribute(DataObject::Pointer input, const std::string& name, IGenum attachmentType) {
    AttributeSet* attributeSet = input ? input->GetAttributeSet() : nullptr;
    if (attributeSet == nullptr) { return -1; }
    for (IGsize i = 0; i < attributeSet->GetNumberOfAttributes(); ++i) {
        auto& attribute = attributeSet->GetAttribute(i);
        if (attribute.isDeleted || attribute.pointer == nullptr) { continue; }
        if (attribute.attachmentType != attachmentType) { continue; }
        if (attribute.pointer->GetName() == name) { return static_cast<int>(i); }
    }
    return -1;
}

struct Result {
    bool ok{false};
    std::string message;
    HistogramData::Pointer data;
};

Result Run(DataObject::Pointer input, int attributeIndex, int bins, int component, bool center = false,
           bool normalize = false, bool accumulation = false, bool averages = false, bool customRange = false,
           double rangeMin = 0.0, double rangeMax = 1.0) {
    auto filter = HistogramFilter::New();
    filter->SetInput(input);
    filter->SetAttributeIndex(attributeIndex);
    filter->SetNumberOfBins(bins);
    filter->SetComponent(component);
    filter->SetCenterBinsAroundMinAndMax(center);
    filter->SetNormalize(normalize);
    filter->SetAccumulation(accumulation);
    filter->SetCalculateAverages(averages);
    filter->SetUseCustomBinRanges(customRange);
    if (customRange) { filter->SetCustomBinRanges(rangeMin, rangeMax); }

    Result result;
    result.ok = filter->Execute();
    result.message = filter->GetMessage();
    if (result.ok) { result.data = DynamicCast<HistogramData>(filter->GetOutput(0)); }
    return result;
}

std::vector<long long> CountsOf(HistogramData::Pointer data) {
    std::vector<long long> counts;
    if (!data || !data->GetBinValues()) { return counts; }
    auto values = data->GetBinValues();
    const int bins = data->GetNumberOfBins();
    for (int i = 0; i < bins; ++i) {
        counts.push_back(static_cast<long long>(values->GetElementValue(i, 0)));
    }
    return counts;
}

std::vector<double> ColumnOf(ArrayObject::Pointer array, int component) {
    std::vector<double> values;
    if (!array) { return values; }
    const int dimension = std::max(1, array->GetDimension());
    const IGsize tuples = array->GetNumberOfValues() / dimension;
    for (IGsize i = 0; i < tuples; ++i) {
        values.push_back(array->GetElementValue(i, component));
    }
    return values;
}

void CheckCounts(const std::vector<long long>& got, const std::vector<long long>& want, const std::string& what) {
    Check(got == want, what + "  期望 " + ToText(want) + "，实际 " + ToText(got));
}

void CheckExtents(HistogramData::Pointer data, double first, double last, const std::string& what) {
    auto extents = data ? data->GetBinExtents() : nullptr;
    if (!extents) {
        Check(false, what + "  没有 bin_extents");
        return;
    }
    Check(Near(extents->GetElementValue(0, 0), first) &&
                  Near(extents->GetElementValue(extents->GetNumberOfValues() - 1, 0), last),
          what + "  期望 [" + std::to_string(first) + ", " + std::to_string(last) + "]");
}

const std::vector<long long> kGhostedCounts = {2, 1, 1, 2, 2, 2, 2, 2, 2, 2};

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

} // namespace

int main(int argc, char** argv) {
    std::cout << "=== Histogram 回归检查（Models/Histogram_test.vtk）===\n";

    const std::string modelPath = ResolveModelPath(argc > 0 ? argv[0] : nullptr, "Models/Histogram_test.vtk");
    std::cout << "模型：" << modelPath << "\n";
    DataObject::Pointer input = FileIO::ReadFile(modelPath);
    Check(input != nullptr, "读取 " + modelPath);
    if (!input) { return 1; }

    const int vIndex = FindAttribute(input, "v", IG_POINT);
    const int vecIndex = FindAttribute(input, "vec", IG_POINT);
    const int cIndex = FindAttribute(input, "c", IG_CELL);
    const int pointGhostIndex = FindAttribute(input, "vtkghosttype", IG_POINT);
    const int cellGhostIndex = FindAttribute(input, "vtkghosttype", IG_CELL);
    Check(vIndex >= 0 && vecIndex >= 0 && cIndex >= 0, "属性集含点数组 v、vec 与单元数组 c");
    Check(pointGhostIndex >= 0 && cellGhostIndex >= 0, "点/单元都有 vtkGhostType（FIELD FieldData，名字已小写化）");
    if (vIndex < 0 || vecIndex < 0 || cIndex < 0) { return 1; }

    std::cout << "\n=== 1. 点数组 v：10 bin（blanking 生效）===\n";
    Result base = Run(input, vIndex, 10, 0);
    Check(base.ok && base.data != nullptr, "执行成功并产出 HistogramData");
    if (base.data) {
        CheckCounts(CountsOf(base.data), kGhostedCounts, "bin_values 与 VTK 一致（两个 blanked 点被排除）");
        long long total = 0;
        for (long long count : CountsOf(base.data)) { total += count; }
        Check(total == 18, "计数总和 = 18（20 个点去掉 2 个 blanked）");
        Check(base.data->GetBinValues()->GetName() == "bin_values", "计数列名 bin_values");
        Check(base.data->GetBinExtents()->GetName() == "bin_extents", "边界列名 bin_extents");
        Check(base.data->GetSourceArrayName() == "v", "记录来源数组名 v");
        Check(Near(base.data->GetBinMinimum(), 0.0) && Near(base.data->GetBinMaximum(), 19.0),
              "统计范围 = [0, 19]（blanked 元组不参与求范围）");
        CheckExtents(base.data, 0.95, 18.05, "默认分箱边界 = min+i*delta+half_delta");
    }

    std::cout << "\n=== 2. 居中分箱 CenterBinsAroundMinAndMax ===\n";
    Result center = Run(input, vIndex, 10, 0, /*center=*/true);
    Check(center.ok && center.data != nullptr, "执行成功");
    if (center.data) {
        CheckCounts(CountsOf(center.data), kGhostedCounts, "计数不变");
        CheckExtents(center.data, 0.0, 19.0, "居中分箱首/末边界 = min / max");
    }

    std::cout << "\n=== 3. 分量选择与模长（vec，3 分量）===\n";
    Result comp1 = Run(input, vecIndex, 10, 1);
    Check(comp1.ok && comp1.data != nullptr, "分量 1 执行成功");
    if (comp1.data) {
        CheckCounts(CountsOf(comp1.data), kGhostedCounts, "分量 1 计数与 VTK 一致");
        CheckExtents(comp1.data, 1.9, 36.1, "分量 1 范围 [0, 38] → 边界 1.9 / 36.1");
    }
    Result magnitude = Run(input, vecIndex, 10, 3);
    Check(magnitude.ok && magnitude.data != nullptr, "分量 3 == 维数：按模长统计");
    if (magnitude.data) {
        CheckCounts(CountsOf(magnitude.data), kGhostedCounts, "模长计数与 VTK 一致");
        CheckExtents(magnitude.data, 2.1242645786248002, 40.361026993871207,
                     "模长范围 [0, sqrt(5)*19] → 边界 2.1242645786248 / 40.361026993871");
    }

    std::cout << "\n=== 4. 自定义范围 UseCustomBinRanges = [0, 19] ===\n";
    Result range = Run(input, vIndex, 10, 0, false, false, false, false, /*customRange=*/true, 0.0, 19.0);
    Check(range.ok && range.data != nullptr, "执行成功");
    if (range.data) {
        CheckCounts(CountsOf(range.data), kGhostedCounts, "计数与默认范围一致");
        CheckExtents(range.data, 0.95, 18.05, "边界与默认范围一致");
    }

    std::cout << "\n=== 5. 单元数组 c 与单元 blanking ===\n";
    Result cell3 = Run(input, cIndex, 3, 0);
    Check(cell3.ok && cell3.data != nullptr, "3 bin 执行成功");
    if (cell3.data) {
        CheckCounts(CountsOf(cell3.data), {1, 0, 1}, "3 bin 计数 = [1,0,1]（两个 blanked 单元被排除）");
        Check(Near(cell3.data->GetBinMinimum(), 1.0) && Near(cell3.data->GetBinMaximum(), 3.0),
              "统计范围 = [1, 3]");
        CheckExtents(cell3.data, 1.3333333333333333, 2.6666666666666665, "3 bin 边界 = 1.3333 / 2.6667");
    }
    Result cell10 = Run(input, cIndex, 10, 0);
    if (cell10.data) {
        CheckCounts(CountsOf(cell10.data), {1, 0, 0, 0, 0, 0, 0, 0, 0, 1}, "10 bin 计数 = [1,0,...,0,1]");
        CheckExtents(cell10.data, 1.1, 2.9, "10 bin 边界 = 1.1 / 2.9");
    }

    std::cout << "\n=== 6. 归一化 Normalize ===\n";
    Result normalized = Run(input, vIndex, 10, 0, false, /*normalize=*/true);
    Check(normalized.ok && normalized.data != nullptr, "执行成功");
    if (normalized.data) {
        Check(normalized.data->GetNormalized(), "GetNormalized() = true");
        std::vector<double> values = ColumnOf(normalized.data->GetBinValues(), 0);
        double sum = 0.0;
        bool matchesRatio = values.size() == kGhostedCounts.size();
        for (size_t i = 0; i < values.size(); ++i) {
            sum += values[i];
            if (!Near(values[i], static_cast<double>(kGhostedCounts[i]) / 18.0)) { matchesRatio = false; }
        }
        Check(Near(sum, 1.0, 1e-12), "归一化后总和 = 1");
        Check(matchesRatio, "每箱 = 计数 / 18");
    }

    std::cout << "\n=== 7. 累积 Accumulation ===\n";
    Result accum = Run(input, vIndex, 10, 0, false, false, /*accumulation=*/true);
    Check(accum.ok && accum.data != nullptr, "执行成功");
    if (accum.data) {
        auto accumulation = accum.data->GetBinAccumulation();
        Check(accumulation != nullptr, "产出 bin_accumulation");
        if (accumulation) {
            Check(accumulation->GetName() == "bin_accumulation", "列名 bin_accumulation");
            long long running = 0;
            bool prefix = true;
            for (size_t i = 0; i < kGhostedCounts.size(); ++i) {
                running += kGhostedCounts[i];
                if (static_cast<long long>(accumulation->GetElementValue(i, 0)) != running) { prefix = false; }
            }
            Check(prefix, "前缀和正确，末项 = 18");
        }
    }

    std::cout << "\n=== 8. 计算均值 CalculateAverages ===\n";
    Result averages = Run(input, vIndex, 10, 0, false, false, false, /*averages=*/true);
    Check(averages.ok && averages.data != nullptr, "执行成功");
    if (averages.data) {
        const auto& columns = averages.data->GetAverageColumns();
        // ParaView 输出表实测列：bin_extents, bin_values, vec_total, vec_average,
        // vtkGhostType_total, vtkGhostType_average —— 即「其余每个数组」一对列，多分量数组在同名列里展开
        Check(columns.size() == 2, "为其余 2 个数组（vec、vtkGhostType）各产出一对 total/average 列");
        const std::vector<long long> counts = CountsOf(averages.data);
        double vecTotal0 = 0.0;
        bool ratioOk = true;
        for (const auto& column : columns) {
            if (!column.first || !column.second) { ratioOk = false; continue; }
            for (size_t bin = 0; bin < counts.size(); ++bin) {
                const double total = column.first->GetElementValue(bin, 0);
                if (!Near(column.second->GetElementValue(bin, 0) * static_cast<double>(counts[bin]), total)) {
                    ratioOk = false;
                }
            }
            if (column.first->GetName() == "vec_total") {
                for (size_t bin = 0; bin < counts.size(); ++bin) { vecTotal0 += column.first->GetElementValue(bin, 0); }
            }
        }
        Check(ratioOk, "每箱 average * 计数 = total");
        // 各箱内被统计元组的 vec 分量 0 之和：0+1+3+5+…+19 = 184。
        // 注意 VTK 的 CalculateAverages 会产出无意义数值（其 vtkExtractHistogram 的累加有缺陷），
        // 本实现给出数学上正确的值，属已知偏差（见使用说明的偏差章节）。
        Check(Near(vecTotal0, 184.0), "vec 分量 0 的 total 合计 = 184（0+1+3+5+…+19）");
    }

    std::cout << "\n=== 9. 错误路径 ===\n";
    Result badComponent = Run(input, vIndex, 10, 2);
    Check(!badComponent.ok && !badComponent.message.empty(), "分量越界（v 只有 1 维，取 2）报错并给出提示");
    Result badIndex = Run(input, 999, 10, 0);
    Check(!badIndex.ok && !badIndex.message.empty(), "属性下标越界报错并给出提示");
    // -1 表示沿用输入对象当前属性：读取后若该下标有效则应成功，否则应明确报错
    AttributeSet* attributeSet = input->GetAttributeSet();
    const int currentIndex = input->GetAttributeIndex();
    const bool currentValid = currentIndex >= 0 &&
                              currentIndex < static_cast<int>(attributeSet->GetNumberOfAttributes());
    Result defaultIndex = Run(input, -1, 10, 0);
    Check(defaultIndex.ok == currentValid && (!defaultIndex.ok ? !defaultIndex.message.empty() : true),
          "属性下标 = -1 时沿用当前属性（当前下标 = " + std::to_string(currentIndex) + "）");
    if (currentValid && defaultIndex.data) {
        Check(defaultIndex.data->GetSourceArrayName() == attributeSet->GetAttribute(currentIndex).pointer->GetName(),
              "统计的确实是当前属性而非属性集首项");
    }

    std::cout << "\n结果：通过 " << (g_total - g_failed) << " / " << g_total << "，失败 " << g_failed << "\n";
    std::cout << (g_failed == 0 ? "[PASS] Histogram 回归检查全部通过\n" : "[FAIL] Histogram 回归检查存在失败项\n");
    PauseIfNeeded(argc, argv);
    return g_failed == 0 ? 0 : 1;
}
