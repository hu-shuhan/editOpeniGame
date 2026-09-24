#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif

#include <GLFW/glfw3.h>

#include "GLVendor.h"
#include "iGameFileIO.h"
#include "iGameInteractor.h"
#include "iGameRenderWindow.h"
#include "iGameScene.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <string>
#include <vector>

namespace
{
using Clock = std::chrono::steady_clock;

class InspectableScene final : public iGame::Scene {
public:
    using Pointer = iGame::SmartPointer<InspectableScene>;
    static Pointer New() { return new InspectableScene; }

    bool WillRenderNextCall() const { return ShouldRenderThisCall(); }
    double LastGpuTimeMs() const { return m_LastGpuTimeMs; }
    double SmoothedGpuTimeMs() const { return m_SmoothedGpuTimeMs; }
    bool HasRenderedFrame() const { return m_LastRenderEndValid; }
    Clock::time_point LastRenderEnd() const { return m_LastRenderEnd; }

protected:
    InspectableScene() = default;
    ~InspectableScene() override = default;
};

struct Options {
    bool benchmark = false;
    std::string model = "./Models/Tet_Plane.vtk";
    int width = 1920;
    int height = 1080;
    int warmupFrames = 30;
    int targetFps = 30;
    double gpuLimit = 0.0;
    double durationSeconds = 5.0;
};

struct Summary {
    double mean = 0.0;
    double median = 0.0;
    double p95 = 0.0;
    double p99 = 0.0;
    double minimum = 0.0;
    double maximum = 0.0;
};

struct ModelStats {
    std::uint64_t objects = 0;
    std::uint64_t points = 0;
    std::uint64_t cells = 0;
};

struct GpuQuerySample {
    GLuint start = 0;
    GLuint end = 0;
    bool fullRender = false;
    std::uint64_t requestIndex = 0;
};

bool ParseOptions(int argc, char** argv, Options& options) {
    auto nextValue = [&](int& index, const char* name) -> const char* {
        if (index + 1 >= argc) {
            std::cerr << "[BENCH][ERROR] missing value for " << name << '\n';
            return nullptr;
        }
        return argv[++index];
    };

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--benchmark") {
            options.benchmark = true;
        } else if (arg == "--model") {
            const char* value = nextValue(i, "--model");
            if (!value) return false;
            options.model = value;
        } else if (arg == "--width") {
            const char* value = nextValue(i, "--width");
            if (!value) return false;
            options.width = std::stoi(value);
        } else if (arg == "--height") {
            const char* value = nextValue(i, "--height");
            if (!value) return false;
            options.height = std::stoi(value);
        } else if (arg == "--warmup") {
            const char* value = nextValue(i, "--warmup");
            if (!value) return false;
            options.warmupFrames = std::stoi(value);
        } else if (arg == "--target-fps") {
            const char* value = nextValue(i, "--target-fps");
            if (!value) return false;
            options.targetFps = std::stoi(value);
        } else if (arg == "--gpu-limit") {
            const char* value = nextValue(i, "--gpu-limit");
            if (!value) return false;
            options.gpuLimit = std::stod(value);
        } else if (arg == "--seconds") {
            const char* value = nextValue(i, "--seconds");
            if (!value) return false;
            options.durationSeconds = std::stod(value);
        } else if (arg == "--help") {
            std::cout
                    << "Usage: testSetRenderingPressure [--benchmark] "
                       "[--model PATH] [--target-fps N] [--gpu-limit 0..1] "
                       "[--seconds N] [--width N] [--height N] [--warmup N]\n";
            return false;
        } else {
            std::cerr << "[BENCH][ERROR] unknown argument: " << arg << '\n';
            return false;
        }
    }

    if (options.width <= 0 || options.height <= 0 ||
        options.warmupFrames < 2 || options.targetFps < 0 ||
        options.gpuLimit < 0.0 || options.gpuLimit > 1.0 ||
        options.durationSeconds <= 0.0) {
        std::cerr << "[BENCH][ERROR] invalid benchmark option\n";
        return false;
    }
    return true;
}

double Milliseconds(Clock::duration duration) {
    return std::chrono::duration<double, std::milli>(duration).count();
}

Summary Summarize(const std::vector<double>& samples) {
    Summary result;
    if (samples.empty()) return result;

    std::vector<double> sorted = samples;
    std::sort(sorted.begin(), sorted.end());
    result.mean = std::accumulate(sorted.begin(), sorted.end(), 0.0) /
                  static_cast<double>(sorted.size());
    auto percentile = [&](double p) {
        const double position = p * static_cast<double>(sorted.size() - 1);
        const auto lower = static_cast<std::size_t>(std::floor(position));
        const auto upper = static_cast<std::size_t>(std::ceil(position));
        const double fraction = position - static_cast<double>(lower);
        return sorted[lower] * (1.0 - fraction) + sorted[upper] * fraction;
    };
    result.median = percentile(0.50);
    result.p95 = percentile(0.95);
    result.p99 = percentile(0.99);
    result.minimum = sorted.front();
    result.maximum = sorted.back();
    return result;
}

void PrintSummary(const char* label, const Summary& summary,
                  std::size_t count) {
    std::cout << "[BENCH][" << label << "] samples=" << count
              << " mean_ms=" << summary.mean
              << " median_ms=" << summary.median << " p95_ms=" << summary.p95
              << " p99_ms=" << summary.p99 << " min_ms=" << summary.minimum
              << " max_ms=" << summary.maximum << '\n';
}

void AccumulateModelStats(iGame::DataObject* object, ModelStats& stats) {
    if (!object) return;
    ++stats.objects;
    if (auto points = object->GetPoints()) {
        stats.points += points->GetNumberOfPoints();
    }
    if (auto cells = object->GetCellArray()) {
        stats.cells += cells->GetNumberOfCells();
    }
    if (object->HasSubDataObject()) {
        for (auto it = object->SubDataObjectIteratorBegin();
             it != object->SubDataObjectIteratorEnd(); ++it) {
            AccumulateModelStats(it->second, stats);
        }
    }
}

int RunBenchmark(const Options& options) {
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "[BENCH][CONFIG] test=rendering_pressure model=\""
              << options.model << "\" width=" << options.width
              << " height=" << options.height
              << " target_fps=" << options.targetFps
              << " gpu_limit=" << options.gpuLimit
              << " duration_s=" << options.durationSeconds
              << " warmup_frames=" << options.warmupFrames << '\n';

    const auto loadStart = Clock::now();
    iGame::DataObject::Pointer dataObject =
            iGame::FileIO::ReadFile(options.model);
    const auto loadEnd = Clock::now();
    if (!dataObject) {
        std::cerr << "[BENCH][ERROR] failed to read model\n";
        return 2;
    }
    auto drawObject = iGame::DynamicCast<iGame::DrawObject>(dataObject);
    if (!drawObject) {
        std::cerr << "[BENCH][ERROR] model is not drawable\n";
        return 3;
    }
    drawObject->SetAccelerationOption(false);
    drawObject->SetViewStyle(IG_SURFACE);
    drawObject->SetDefaultColor(igm::vec3{1.0f, 1.0f, 1.0f});

    ModelStats modelStats;
    AccumulateModelStats(dataObject, modelStats);
    std::cout << "[BENCH][LOAD] wall_ms=" << Milliseconds(loadEnd - loadStart)
              << '\n';
    std::cout << "[BENCH][MODEL] objects=" << modelStats.objects
              << " points=" << modelStats.points << " cells="
              << modelStats.cells << " memory_mib="
              << static_cast<double>(dataObject->GetRealMemorySize()) /
                         (1024.0 * 1024.0)
              << '\n';

    auto scene = InspectableScene::New();
    scene->AddModel(dataObject);
    scene->ResetCameraView();
    scene->EnableFramePacing(false);
    scene->SetAxesVisible(false);
    scene->SetCenterAxesVisible(false);
    scene->SetColorBarVisible(false);
    scene->SetBackGround(16, 18, 24);

    auto window = iGame::RenderWindow::New();
    window->SetSize(options.width, options.height);

    const auto initializeStart = Clock::now();
    window->SetScene(scene);
    scene->ResetCameraView();
    glFinish();
    const auto initializeEnd = Clock::now();
    glfwSwapInterval(0);
    GLFWwindow* rawWindow = glfwGetCurrentContext();
    if (!rawWindow) {
        std::cerr << "[BENCH][ERROR] no current GLFW context\n";
        window->SetScene(nullptr);
        scene->Finalize();
        scene = nullptr;
        drawObject = nullptr;
        dataObject = nullptr;
        return 4;
    }
    std::cout << "[BENCH][INIT] scene_and_gpu_ms="
              << Milliseconds(initializeEnd - initializeStart) << '\n';

    for (int i = 0; i < options.warmupFrames; ++i) {
        scene->Draw();
        glfwSwapBuffers(rawWindow);
        glfwPollEvents();
    }
    glFinish();
    std::cout << "[BENCH][WARMUP] internal_last_gpu_ms="
              << scene->LastGpuTimeMs() << " internal_smoothed_gpu_ms="
              << scene->SmoothedGpuTimeMs() << '\n';

    scene->SetTargetFps(static_cast<unsigned int>(options.targetFps));
    scene->SetGpuUsageLimit(static_cast<float>(options.gpuLimit));
    scene->EnableFramePacing(options.targetFps > 0 || options.gpuLimit > 0.0);

    constexpr std::size_t maxGpuSamples = 16384;
    std::vector<GLuint> queryIds(maxGpuSamples * 2U);
    glGenQueries(static_cast<GLsizei>(queryIds.size()), queryIds.data());
    std::vector<GpuQuerySample> querySamples;
    querySamples.reserve(maxGpuSamples);
    std::vector<double> fullCpuMs;
    std::vector<double> skippedCpuMs;
    fullCpuMs.reserve(static_cast<std::size_t>(
            options.durationSeconds * std::max(options.targetFps, 60) * 2));
    skippedCpuMs.reserve(4096);

    std::uint64_t requests = 0;
    std::uint64_t fullFrames = 0;
    std::uint64_t skippedFrames = 0;
    const auto measurementStart = Clock::now();
    const auto deadline = measurementStart +
                          std::chrono::duration_cast<Clock::duration>(
                                  std::chrono::duration<double>(
                                          options.durationSeconds));
    while (Clock::now() < deadline) {
        const bool predictedFullRender = scene->WillRenderNextCall();
        const bool sampleGpu = querySamples.size() < maxGpuSamples &&
                               (predictedFullRender || requests < 512U ||
                                requests % 32U == 0U);
        const bool hadRenderedFrame = scene->HasRenderedFrame();
        const auto previousRenderEnd = scene->LastRenderEnd();
        GLuint sampleStart = 0;
        GLuint sampleEnd = 0;
        if (sampleGpu) {
            const auto queryIndex = querySamples.size() * 2U;
            sampleStart = queryIds[queryIndex];
            sampleEnd = queryIds[queryIndex + 1U];
            glQueryCounter(sampleStart, GL_TIMESTAMP);
        }

        const auto cpuStart = Clock::now();
        scene->Draw();
        const auto cpuEnd = Clock::now();
        if (sampleGpu) {
            glQueryCounter(sampleEnd, GL_TIMESTAMP);
        }
        const bool fullRender =
                !hadRenderedFrame || scene->LastRenderEnd() != previousRenderEnd;
        glfwSwapBuffers(rawWindow);
        glfwPollEvents();
        if (sampleGpu) {
            querySamples.push_back(
                    {sampleStart, sampleEnd, fullRender, requests});
        }
        const double cpuMs = Milliseconds(cpuEnd - cpuStart);
        if (fullRender) {
            ++fullFrames;
            fullCpuMs.push_back(cpuMs);
        } else {
            ++skippedFrames;
            if (skippedCpuMs.size() < 100000U) skippedCpuMs.push_back(cpuMs);
        }
        ++requests;
    }
    const auto measurementEnd = Clock::now();
    const auto drainStart = Clock::now();
    glFinish();
    const auto drainEnd = Clock::now();

    std::vector<double> fullGpuMs;
    std::vector<double> skippedGpuMs;
    std::uint64_t firstFullSampleRequest =
            std::numeric_limits<std::uint64_t>::max();
    std::uint64_t lastFullSampleRequest = 0;
    for (const auto& sample: querySamples) {
        GLuint64 startNs = 0;
        GLuint64 endNs = 0;
        glGetQueryObjectui64v(sample.start, GL_QUERY_RESULT, &startNs);
        glGetQueryObjectui64v(sample.end, GL_QUERY_RESULT, &endNs);
        if (endNs < startNs) continue;
        const double gpuMs =
                static_cast<double>(endNs - startNs) / 1000000.0;
        if (sample.fullRender) {
            fullGpuMs.push_back(gpuMs);
            firstFullSampleRequest =
                    std::min(firstFullSampleRequest, sample.requestIndex);
            lastFullSampleRequest =
                    std::max(lastFullSampleRequest, sample.requestIndex);
        } else {
            skippedGpuMs.push_back(gpuMs);
        }
    }
    glDeleteQueries(static_cast<GLsizei>(queryIds.size()), queryIds.data());

    const double elapsedSeconds =
            std::chrono::duration<double>(measurementEnd - measurementStart)
                    .count();
    const double elapsedMs = elapsedSeconds * 1000.0;
    const double renderedFps =
            static_cast<double>(fullFrames) / elapsedSeconds;
    const double requestFps = static_cast<double>(requests) / elapsedSeconds;
    const double skipRatio = requests == 0
                                     ? 0.0
                                     : 100.0 * static_cast<double>(skippedFrames) /
                                               static_cast<double>(requests);
    const Summary fullCpu = Summarize(fullCpuMs);
    const Summary skipCpu = Summarize(skippedCpuMs);
    const Summary fullGpu = Summarize(fullGpuMs);
    const Summary skipGpu = Summarize(skippedGpuMs);

    std::cout << "[BENCH][RESULT] elapsed_s=" << elapsedSeconds
              << " requests=" << requests << " full_frames=" << fullFrames
              << " skipped_frames=" << skippedFrames
              << " request_hz=" << requestFps
              << " full_render_fps=" << renderedFps
              << " skip_ratio_pct=" << skipRatio << '\n';
    std::cout << "[BENCH][DRAIN] gpu_queue_drain_ms="
              << Milliseconds(drainEnd - drainStart) << '\n';
    PrintSummary("CPU_FULL", fullCpu, fullCpuMs.size());
    PrintSummary("CPU_SKIP", skipCpu, skippedCpuMs.size());
    PrintSummary("GPU_FULL", fullGpu, fullGpuMs.size());
    PrintSummary("GPU_SKIP", skipGpu, skippedGpuMs.size());
    const double fullCoverage =
            fullFrames == 0U
                    ? 0.0
                    : 100.0 * static_cast<double>(fullGpuMs.size()) /
                              static_cast<double>(fullFrames);
    const double skipCoverage =
            skippedFrames == 0U
                    ? 0.0
                    : 100.0 * static_cast<double>(skippedGpuMs.size()) /
                              static_cast<double>(skippedFrames);
    const double temporalCoverage =
            querySamples.size() < 2U || requests < 2U
                    ? 0.0
                    : 100.0 * static_cast<double>(
                                      querySamples.back().requestIndex -
                                      querySamples.front().requestIndex) /
                              static_cast<double>(requests - 1U);
    const double fullTemporalCoverage =
            fullGpuMs.size() < 2U || requests < 2U
                    ? 0.0
                    : 100.0 * static_cast<double>(lastFullSampleRequest -
                                                  firstFullSampleRequest) /
                              static_cast<double>(requests - 1U);
    std::cout << "[BENCH][GPU_COVERAGE] full_pct=" << fullCoverage
              << " skip_pct=" << skipCoverage
              << " temporal_span_pct=" << temporalCoverage
              << " full_temporal_span_pct=" << fullTemporalCoverage << '\n';

    const double estimatedFullGpuDuty =
            elapsedMs > 0.0
                    ? fullGpu.mean * static_cast<double>(fullFrames) /
                              elapsedMs * 100.0
                    : 0.0;
    const double estimatedTotalGpuDuty =
            elapsedMs > 0.0
                    ? (fullGpu.mean * static_cast<double>(fullFrames) +
                       skipGpu.mean * static_cast<double>(skippedFrames)) /
                              elapsedMs * 100.0
                    : 0.0;
    std::cout << "[BENCH][GPU_DUTY_ESTIMATE] full_only_pct="
              << estimatedFullGpuDuty << " including_skip_blit_pct="
              << estimatedTotalGpuDuty
              << " configured_limit_pct=" << options.gpuLimit * 100.0
              << " queue_saturated="
              << (estimatedTotalGpuDuty >= 100.0 ? "true" : "false")
              << '\n';

    bool checkFailed = false;
    bool checkIndeterminate = false;
    if (options.targetFps > 0) {
        const double attainment =
                renderedFps / static_cast<double>(options.targetFps) * 100.0;
        const bool capPass = renderedFps <= options.targetFps * 1.05;
        const bool trackingPass = std::abs(attainment - 100.0) <= 10.0;
        std::cout << "[BENCH][FPS_CHECK] target=" << options.targetFps
                  << " attainment_pct=" << attainment
                  << " cap_pass=" << (capPass ? "true" : "false")
                  << " within_10pct="
                  << (trackingPass ? "true" : "false") << '\n';
        checkFailed = checkFailed || !capPass || !trackingPass;
    }
    if (options.gpuLimit > 0.0) {
        const double tolerancePct =
                std::max(3.0, options.gpuLimit * 100.0 * 0.10);
        const double timerReferenceMs = std::max(
                {scene->LastGpuTimeMs(), fullGpu.median, 0.001});
        const double timerRatio =
                scene->SmoothedGpuTimeMs() / timerReferenceMs;
        const bool timerStable = std::isfinite(timerRatio) && timerRatio <= 5.0;
        std::cout << "[BENCH][GPU_TIMER_HEALTH] last_or_external_ms="
                  << timerReferenceMs << " smoothed_to_reference="
                  << timerRatio << " stable="
                  << (timerStable ? "true" : "false") << '\n';
        if (!timerStable) {
            std::cout << "[BENCH][GPU_LIMIT_CHECK] status=indeterminate"
                      << " reason=unstable_internal_gpu_timer\n";
            checkIndeterminate = true;
        } else if (fullFrames < 100U || fullGpuMs.size() < 100U ||
                   fullTemporalCoverage < 80.0) {
            std::cout << "[BENCH][GPU_LIMIT_CHECK] status=indeterminate"
                      << " reason=insufficient_full_frames"
                      << " full_frames=" << fullFrames
                      << " gpu_samples=" << fullGpuMs.size() << '\n';
            checkIndeterminate = true;
        } else {
            const bool capPass = estimatedFullGpuDuty <=
                                 options.gpuLimit * 100.0 + tolerancePct;
            std::cout << "[BENCH][GPU_LIMIT_CHECK] status=measured"
                      << " tolerance_pct=" << tolerancePct
                      << " full_duty_cap_pass="
                      << (capPass ? "true" : "false") << '\n';
            checkFailed = checkFailed || !capPass;
        }
    }
    std::cout << "[BENCH][INTERNAL_GPU] last_ms=" << scene->LastGpuTimeMs()
              << " smoothed_ms=" << scene->SmoothedGpuTimeMs() << '\n';
    std::cout << "[BENCH][NOTE] GPU duty is a scene-work estimate, not whole-card utilization.\n";
    window->SetScene(nullptr);
    scene->Finalize();
    scene = nullptr;
    drawObject = nullptr;
    dataObject = nullptr;
    const char* finalStatus = checkFailed
                                      ? "fail"
                                      : (checkIndeterminate ? "indeterminate"
                                                            : "pass");
    std::cout << "[BENCH][DONE] status=" << finalStatus << '\n';
    return checkFailed ? 7 : (checkIndeterminate ? 8 : 0);
}
} // namespace

int main(int argc, char** argv) {
    Options options;
    try {
        if (!ParseOptions(argc, argv, options)) return 1;
        if (options.benchmark) return RunBenchmark(options);

        auto scene = iGame::Scene::New();
        auto dataObject = iGame::FileIO::ReadFile(options.model);
        auto drawObject = iGame::DynamicCast<iGame::DrawObject>(dataObject);
        if (!drawObject) return 2;
        scene->AddModel(dataObject);
        scene->ResetCameraView();
        scene->SetTargetFps(static_cast<unsigned int>(options.targetFps));
        scene->SetGpuUsageLimit(static_cast<float>(options.gpuLimit));
        drawObject->SetViewStyle(IG_SURFACE);

        auto window = iGame::RenderWindow::New();
        window->SetSize(options.width, options.height);
        window->SetScene(scene);
        auto interactor = iGame::Interactor::New();
        interactor->Initialize(scene);
        interactor->CreateDefaultStyle();
        window->SetInteractor(interactor);
        window->Show();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[BENCH][ERROR] " << error.what() << '\n';
        return 1;
    }
}
