// 集成回归（待提交）：原标准菜单缺少本过滤器实现；保留源仓库的数值、类型及错误输入断言，
// 用于保证迁移后结果和边界行为一致，防止仅有菜单入口而算法不可用。
// PerlinNoise 过滤器测试：与参考实现逐点对拍 + 输出形态/参数行为检查。
//
// 参考值由 out/pv_perlin_reference.py 采样得到，采样点为 21x21x21 规则网格
// （origin(-10,-10,-10)、spacing 1）上的若干点。
#include <AttributeManipulation/iGamePerlinNoiseFilter.h>
#include <iGameAttributeSet.h>
#include <iGameDataObject.h>
#include <iGameFileIO.h>
#include <iGameFlatArray.h>
#include <iGamePointSet.h>
#include <iGamePoints.h>
#include <iGameType.h>

#include <cmath>
#include <iostream>
#include <string>

namespace {

int g_failed = 0;

void Check(bool ok, const std::string& what) {
    std::cout << (ok ? "[ok]   " : "[FAIL] ") << what << "\n";
    if (!ok) ++g_failed;
}

struct Sample {
    double x, y, z;
    double value;
};

// 21x21x21 规则网格上的参考采样值
const Sample kReferenceA[] = {
        {-10.0, -10.0, -10.0, -0.970326126},
        {-9.0, -10.0, -10.0, -0.289364457},
        {-8.0, -10.0, -10.0, 0.424212992},
        {10.0, -10.0, -10.0, 0.674302220},
        {-10.0, -9.0, -10.0, -0.395240843},
        {-9.0, 9.0, -10.0, -0.727613747},
        {3.0, -5.0, -8.0, 0.364625782},
        {10.0, 10.0, 10.0, 0.547765851},
};

const Sample kReferenceB[] = { // Amplitude=2, Frequency=(2,2,2), Phase=(0.5,0,0)
        {-10.0, -10.0, -10.0, -1.683062673},
        {-9.0, -10.0, -10.0, 0.295740873},
        {-8.0, -10.0, -10.0, 0.481811970},
        {10.0, -10.0, -10.0, -1.471859574},
        {-10.0, -9.0, -10.0, 1.428662419},
        {-9.0, 9.0, -10.0, 1.544585466},
        {3.0, -5.0, -8.0, 0.451897293},
        {10.0, 10.0, 10.0, -0.844230473},
};

const Sample kReferenceC[] = { // Amplitude=0.5, Frequency=(1,3,0.5), Phase=(0,0.25,0.75)
        {-10.0, -10.0, -10.0, -0.130307361},
        {-9.0, -10.0, -10.0, 0.013902921},
        {-8.0, -10.0, -10.0, 0.005790086},
        {10.0, -10.0, -10.0, 0.101703055},
        {-10.0, -9.0, -10.0, 0.092478290},
        {-9.0, 9.0, -10.0, 0.023769541},
        {3.0, -5.0, -8.0, -0.160523459},
        {10.0, 10.0, 10.0, -0.077735461},
};

// 任意坐标（含小数与大坐标）的参考值：由 out/pv_perlin_scalar_ref.py 直接求值得到。
// 这些点的坐标不是整数，输入以 float 存储会有约 1e-4 的表示误差，故容差放宽到 1e-3。
const Sample kScalarRefA[] = { // Amplitude=1, Frequency=(1,1,1), Phase=(0,0,0)
        {0.0, 0.0, 0.0, -0.281790984},
        {0.5, 0.5, 0.5, 0.008229024},
        {1.25, -2.75, 3.125, -0.424809108},
        {-100.5, 200.25, -300.125, -0.410684934},
        {1000.0, 2000.0, -3000.0, 0.424614969},
        {12345.6789, 0.0, 0.0, -0.302110017},
        {-0.125, 0.875, -1.5, -0.354561811},
};

const Sample kScalarRefB[] = { // Amplitude=2.5, Frequency=(3,0.5,4), Phase=(0.25,0.5,0.125)
        {0.0, 0.0, 0.0, -1.067518266},
        {0.5, 0.5, 0.5, 1.630988227},
        {1.25, -2.75, 3.125, 0.716680725},
        {-100.5, 200.25, -300.125, 0.876252870},
        {1000.0, 2000.0, -3000.0, 1.084665882},
        {12345.6789, 0.0, 0.0, 0.308536927},
        {-0.125, 0.875, -1.5, 0.970200117},
};

iGame::PointSet::Pointer MakePointSet(const Sample* samples, int count) {
    using namespace iGame;
    auto ps = PointSet::New();
    for (int i = 0; i < count; ++i) {
        ps->AddPoint(Point(static_cast<float>(samples[i].x),
                           static_cast<float>(samples[i].y),
                           static_cast<float>(samples[i].z)));
    }
    return ps;
}

// 用给定参数执行 filter，并返回每个采样点的噪声值
bool RunCase(iGame::PointSet::Pointer ps, double amplitude, const double freq[3],
             const double phase[3], double* outValues, int count, std::string& message) {
    using namespace iGame;
    auto filter = PerlinNoiseFilter::New();
    filter->SetAmplitude(amplitude);
    filter->SetFrequency(freq);
    filter->SetPhase(phase);
    filter->SetInput(ps);
    if (!filter->Execute()) {
        message = "Execute() failed";
        return false;
    }
    auto out = filter->GetOutput();
    if (!out) {
        message = "output is null";
        return false;
    }
    auto outPs = DynamicCast<PointSet>(out);
    auto attrSet = out->GetAttributeSet();
    if (!outPs || !attrSet) {
        message = "output has no PointSet/AttributeSet";
        return false;
    }
    const int index = attrSet->GetAttributeIndex("PerlinNoise");
    if (index < 0) {
        message = "output has no \"PerlinNoise\" scalar attribute";
        return false;
    }
    auto array = attrSet->GetAttribute(index).pointer;
    if (!array || array->GetDimension() != 1) {
        message = "PerlinNoise attribute is not a 1-component scalar";
        return false;
    }
    for (int i = 0; i < count; ++i) { outValues[i] = array->GetValue(i); }
    return true;
}

bool CompareCase(const char* tag, iGame::PointSet::Pointer ps, double amplitude,
                 const double freq[3], const double phase[3], const Sample* reference,
                 int count, double tolerance = 1e-6) {
    double values[8] = {0};
    std::string message;
    if (!RunCase(ps, amplitude, freq, phase, values, count, message)) {
        Check(false, std::string(tag) + ": " + message);
        return false;
    }
    double maxError = 0.0;
    for (int i = 0; i < count; ++i) {
        maxError = std::max(maxError, std::fabs(values[i] - reference[i].value));
    }
    const bool ok = (maxError < tolerance);
    Check(ok, std::string(tag) + ": matches reference values (maxErr=" +
                       std::to_string(maxError) + ")");
    return ok;
}

// 用给定参数执行 filter，并返回噪声值的 min/max
bool RunCaseRange(iGame::PointSet::Pointer ps, double amplitude, const double freq[3],
                  const double phase[3], double& minValue, double& maxValue) {
    using namespace iGame;
    auto filter = PerlinNoiseFilter::New();
    filter->SetAmplitude(amplitude);
    filter->SetFrequency(freq);
    filter->SetPhase(phase);
    filter->SetInput(ps);
    if (!filter->Execute()) return false;
    auto out = filter->GetOutput();
    auto attrSet = out ? out->GetAttributeSet() : nullptr;
    const int idx = attrSet ? attrSet->GetAttributeIndex("PerlinNoise") : -1;
    if (idx < 0) return false;
    auto array = attrSet->GetAttribute(idx).pointer;
    if (!array || array->GetNumberOfElements() == 0) return false;
    minValue = maxValue = array->GetValue(0);
    for (size_t i = 1; i < array->GetNumberOfElements(); ++i) {
        const double v = array->GetValue(i);
        minValue = std::min(minValue, v);
        maxValue = std::max(maxValue, v);
    }
    return true;
}

} // namespace

int main() {
    using namespace iGame;
    const int count = 8;

    /* ---- 1) 逐点对拍（三组参数） ---- */
    auto psA = MakePointSet(kReferenceA, count);
    const double freqA[3] = {1.0, 1.0, 1.0};
    const double phaseA[3] = {0.0, 0.0, 0.0};
    CompareCase("A (Amp=1, Freq=(1,1,1), Phase=(0,0,0))", psA, 1.0, freqA, phaseA, kReferenceA, count);

    const double freqB[3] = {2.0, 2.0, 2.0};
    const double phaseB[3] = {0.5, 0.0, 0.0};
    CompareCase("B (Amp=2, Freq=(2,2,2), Phase=(0.5,0,0))", psA, 2.0, freqB, phaseB, kReferenceB, count);

    const double freqC[3] = {1.0, 3.0, 0.5};
    const double phaseC[3] = {0.0, 0.25, 0.75};
    CompareCase("C (Amp=0.5, Freq=(1,3,0.5), Phase=(0,0.25,0.75))", psA, 0.5, freqC, phaseC,
                kReferenceC, count);

    /* ---- 1b) 任意坐标（小数/大坐标）对拍：参考值由求值脚本直接给出 ---- */
    {
        const int scalarCount = 7;
        auto psScalar = MakePointSet(kScalarRefA, scalarCount);
        CompareCase("D (arbitrary coords, Amp=1, Freq=(1,1,1))", psScalar, 1.0, freqA, phaseA,
                    kScalarRefA, scalarCount, 1e-3);
        const double freqD[3] = {3.0, 0.5, 4.0};
        const double phaseD[3] = {0.25, 0.5, 0.125};
        CompareCase("E (arbitrary coords, Amp=2.5, Freq=(3,0.5,4), Phase=(0.25,0.5,0.125))",
                    psScalar, 2.5, freqD, phaseD, kScalarRefB, scalarCount, 1e-2);
    }

    /* ---- 1c) 测试模型文件（11x11x11 六面体，步长 0.5）逐点对拍 ---- */
    // 参考值由 out/pv_perlin_modelfile_ref.py 读取同一个 .vtk 文件后采样得到，
    // 点序与文件一致（x 最快）。
    {
        struct IndexedSample {
            int index;
            double value;
        };
        const IndexedSample kModelRefA[] = { // Amp=1, Freq=(1,1,1), Phase=(0,0,0)
                {0, -0.163010612},  {1, 0.011929035},  {2, 0.115580931},  {10, 0.052891217},
                {11, -0.286811441}, {100, -0.127717614}, {500, -0.178265199}, {1330, 0.272042185},
        };
        const IndexedSample kModelRefB[] = { // Amp=2, Freq=(2,2,2), Phase=(0.5,0,0)
                {0, -1.165000916},  {1, 1.114290595},  {2, -1.363125205},  {10, 1.013640285},
                {11, -1.841730118}, {100, 1.984589696}, {500, -1.706741571}, {1330, -1.638184309},
        };

        auto mesh = FileIO::ReadFile("./Models/PerlinNoise_Test.vtk");
        auto loaded = DynamicCast<PointSet>(mesh);
        Check(loaded && loaded->GetNumberOfPoints() == 1331,
              "test model loaded (11x11x11 = 1331 points)");

        auto runOnModel = [&](double amplitude, const double freq[3], const double phase[3],
                              const IndexedSample* ref, const char* tag) {
            auto filter = PerlinNoiseFilter::New();
            filter->SetAmplitude(amplitude);
            filter->SetFrequency(freq);
            filter->SetPhase(phase);
            filter->SetInput(mesh);
            const bool ok = filter->Execute();
            auto out = filter->GetOutput();
            auto attrSet = out ? out->GetAttributeSet() : nullptr;
            const int idx = attrSet ? attrSet->GetAttributeIndex("PerlinNoise") : -1;
            if (!ok || idx < 0) {
                Check(false, std::string(tag) + ": filter failed on the test model");
                return;
            }
            auto array = attrSet->GetAttribute(idx).pointer;
            const auto elements = array ? array->GetNumberOfElements() : 0;
            double maxErr = 0.0;
            for (int s = 0; s < count; ++s) {
                const size_t index = static_cast<size_t>(ref[s].index);
                if (!array || index >= elements) {
                    maxErr = 1.0;
                    break;
                }
                maxErr = std::max(maxErr, std::fabs(array->GetValue(index) - ref[s].value));
            }
            Check(maxErr < 1e-5, std::string(tag) + ": matches reference values on the test model (maxErr=" +
                                         std::to_string(maxErr) + ")");
        };

        if (loaded) {
            runOnModel(1.0, freqA, phaseA, kModelRefA, "model file / Amp=1, Freq=(1,1,1)");
            const double freqB2[3] = {2.0, 2.0, 2.0};
            const double phaseB2[3] = {0.5, 0.0, 0.0};
            runOnModel(2.0, freqB2, phaseB2, kModelRefB, "model file / Amp=2, Freq=(2,2,2), Phase=(0.5,0,0)");
        }
    }

    /* ---- 2) 几何/拓扑不变，只多一个点标量 ---- */
    {
        auto filter = PerlinNoiseFilter::New();
        filter->SetInput(psA);
        const bool executed = filter->Execute();
        auto out = filter->GetOutput();
        auto outPs = DynamicCast<PointSet>(out);
        Check(executed && outPs && outPs->GetNumberOfPoints() == psA->GetNumberOfPoints(),
              "output keeps the input geometry (same point count)");
        Check(outPs && outPs->GetPoint(0)[0] == psA->GetPoint(0)[0] &&
                      outPs->GetPoint(0)[2] == psA->GetPoint(0)[2],
              "output point coordinates are unchanged");
        // 输入模型不受影响
        Check(psA->GetAttributeSet()->GetAttributeIndex("PerlinNoise") < 0,
              "input object is not modified (no PerlinNoise attribute added)");
        // 输出数组名与默认一致
        auto attrSet = out ? out->GetAttributeSet() : nullptr;
        Check(attrSet && attrSet->GetAttributeIndex("PerlinNoise") >= 0,
              "default scalar array name is \"PerlinNoise\"");
    }

    /* ---- 3) 确定性：同参数两次执行结果完全一致 ---- */
    {
        double v1[8] = {0}, v2[8] = {0};
        std::string msg;
        const bool ok1 = RunCase(psA, 1.0, freqA, phaseA, v1, count, msg);
        const bool ok2 = RunCase(psA, 1.0, freqA, phaseA, v2, count, msg);
        bool same = ok1 && ok2;
        for (int i = 0; i < count && same; ++i) { same = (v1[i] == v2[i]); }
        Check(same, "deterministic: same parameters give identical values");
    }

    /* ---- 4) 参数生效：Amplitude 线性缩放；Frequency/Phase 改变数值 ---- */
    {
        double base[8] = {0}, doubled[8] = {0}, freqChanged[8] = {0}, phaseChanged[8] = {0};
        std::string msg;
        RunCase(psA, 1.0, freqA, phaseA, base, count, msg);
        RunCase(psA, 2.0, freqA, phaseA, doubled, count, msg);
        bool ampOk = true;
        for (int i = 0; i < count; ++i) {
            if (std::fabs(doubled[i] - 2.0 * base[i]) > 1e-9) { ampOk = false; }
        }
        Check(ampOk, "Amplitude scales the noise linearly");

        const double freq2[3] = {2.0, 2.0, 2.0};
        RunCase(psA, 1.0, freq2, phaseA, freqChanged, count, msg);
        bool freqDiffers = false;
        for (int i = 0; i < count; ++i) {
            if (std::fabs(freqChanged[i] - base[i]) > 1e-6) { freqDiffers = true; }
        }
        Check(freqDiffers, "Frequency changes the sampled values");

        const double phase2[3] = {0.3, 0.3, 0.3};
        RunCase(psA, 1.0, freqA, phase2, phaseChanged, count, msg);
        bool phaseDiffers = false;
        for (int i = 0; i < count; ++i) {
            if (std::fabs(phaseChanged[i] - base[i]) > 1e-6) { phaseDiffers = true; }
        }
        Check(phaseDiffers, "Phase changes the sampled values");
    }

    /* ---- 5) 值域与插值：注意整数格点上 hermite 的 t=0 退化，必须用非整数坐标采样 ---- */
    {
        // 半整数偏移的网格：这样才真正走插值路径（整数格点上的值就是格点随机值）
        auto psGrid = PointSet::New();
        for (int i = 0; i < 21; ++i) {
            for (int j = 0; j < 21; ++j) {
                for (int k = 0; k < 21; ++k) {
                    psGrid->AddPoint(Point(static_cast<float>(i - 10) + 0.5f,
                                           static_cast<float>(j - 10) + 0.5f,
                                           static_cast<float>(k - 10) + 0.5f));
                }
            }
        }
        double lo = 0.0, hi = 0.0;
        const bool ok = RunCaseRange(psGrid, 1.0, freqA, phaseA, lo, hi);
        Check(ok && hi > lo, "interpolation is effective (non-constant values on offset grid)");
        // hermite 插值会轻微 overshoot：参考实现扫描 2 万个点、Amplitude=2.5 时
        // min≈-2.7467 / max≈2.8338，故这里只做宽松的合理性约束
        Check(ok && lo >= -1.5 && hi <= 1.5,
              "value range stays sane on the offset grid (Amplitude=1)");

        // 整数格点上应正好等于该格点的随机值（无插值），可用于验证实现走了 hermite 分支
        auto psInt = PointSet::New();
        psInt->AddPoint(Point(0.0f, 0.0f, 0.0f));
        double loInt = 0.0, hiInt = 0.0;
        const bool okInt = RunCaseRange(psInt, 1.0, freqA, phaseA, loInt, hiInt);
        Check(okInt && std::fabs(loInt - (-0.281790984)) < 1e-6,
              "lattice point (0,0,0) equals reference value (no interpolation at integer coords)");
    }

    /* ---- 6) 自定义数组名 + 边界情况 ---- */
    {
        auto filter = PerlinNoiseFilter::New();
        filter->SetScalarArrayName("Noise");
        filter->SetInput(psA);
        const bool ok = filter->Execute();
        auto attrSet = filter->GetOutput() ? filter->GetOutput()->GetAttributeSet() : nullptr;
        Check(ok && attrSet && attrSet->GetAttributeIndex("Noise") >= 0,
              "custom scalar array name is honored");

        auto empty = PointSet::New();
        auto filter2 = PerlinNoiseFilter::New();
        filter2->SetInput(empty);
        Check(!filter2->Execute(), "empty input (0 points) is rejected");

        auto filter3 = PerlinNoiseFilter::New();
        Check(!filter3->Execute(), "missing input is rejected");

        auto filter4 = PerlinNoiseFilter::New();
        auto ps2 = MakePointSet(kReferenceA, count);
        filter4->SetFrequency(1.5, 2.5, 3.5);
        double freqBack[3] = {0};
        double phaseBack[3] = {0};
        filter4->GetFrequency(freqBack);
        filter4->GetPhase(phaseBack);
        filter4->SetInput(ps2);
        const bool ok4 = filter4->Execute();
        Check(ok4 && freqBack[0] == 1.5 && freqBack[1] == 2.5 && freqBack[2] == 3.5 &&
                      phaseBack[0] == 0.0 && phaseBack[1] == 0.0 && phaseBack[2] == 0.0,
              "getters return what setters stored (Frequency/Phase)");
        auto out4 = filter4->GetOutput();
        Check(out4 && out4->GetName() == std::string(ps2->GetName() + "_PerlinNoise"),
              "output model name is suffixed with _PerlinNoise");
    }

    if (g_failed == 0) {
        std::cout << "\nResult: PASS\n";
        return 0;
    }
    std::cout << "\nResult: FAIL (" << g_failed << " checks failed)\n";
    return 1;
}
