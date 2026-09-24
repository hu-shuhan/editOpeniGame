#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif

#include <GLFW/glfw3.h>

#include "GLVendor.h"
#include "iGameFileIO.h"
#include "iGameInteractor.h"
#include "iGameRenderWindow.h"
#include "iGameScene.h"
#include "iGameSurfaceMesh.h"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
using Clock = std::chrono::steady_clock;

struct Options {
    bool benchmark = false;
    bool meshlet = true;
    std::string model = "./Models/Tet_Plane.vtk";
    int width = 1920;
    int height = 1080;
    int warmupFrames = 30;
    int measuredFrames = 240;
    std::string capturePath;
    std::string referencePath;
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

struct TriangleObjCounts {
    std::uint64_t vertices = 0;
    std::uint64_t faces = 0;
};

bool HasExtension(const std::string& path, const std::string& extension) {
    if (path.size() < extension.size()) return false;
    return std::equal(extension.rbegin(), extension.rend(), path.rbegin(),
                      [](char lhs, char rhs) {
                          return std::tolower(static_cast<unsigned char>(lhs)) ==
                                 std::tolower(static_cast<unsigned char>(rhs));
                      });
}

TriangleObjCounts CountTriangleObj(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("cannot open OBJ model: " + path);
    }

    TriangleObjCounts counts;
    std::string line;
    while (std::getline(input, line)) {
        if (line.size() >= 2U && line[0] == 'v' && line[1] == ' ') {
            ++counts.vertices;
        } else if (line.size() >= 2U && line[0] == 'f' && line[1] == ' ') {
            ++counts.faces;
        }
    }
    if (!input.eof()) {
        throw std::runtime_error("failed while scanning OBJ model: " + path);
    }
    if (counts.vertices == 0U || counts.faces == 0U ||
        counts.vertices > static_cast<std::uint64_t>(
                                  std::numeric_limits<igIndex>::max()) ||
        counts.faces >
                std::numeric_limits<std::uint64_t>::max() / 3ULL) {
        throw std::runtime_error("OBJ dimensions exceed supported limits");
    }
    return counts;
}

float ParseObjCoordinate(const char*& cursor, std::uint64_t lineNumber) {
    while (*cursor == ' ' || *cursor == '\t') ++cursor;
    errno = 0;
    char* end = nullptr;
    const float value = std::strtof(cursor, &end);
    if (end == cursor || errno == ERANGE || !std::isfinite(value)) {
        throw std::runtime_error("invalid OBJ coordinate at line " +
                                 std::to_string(lineNumber));
    }
    cursor = end;
    return value;
}

igIndex ParseObjVertexIndex(const char*& cursor,
                            std::uint64_t parsedVertices,
                            std::uint64_t lineNumber) {
    while (*cursor == ' ' || *cursor == '\t') ++cursor;
    errno = 0;
    char* end = nullptr;
    const long long rawIndex = std::strtoll(cursor, &end, 10);
    if (end == cursor || errno == ERANGE || rawIndex == 0) {
        throw std::runtime_error("invalid OBJ face index at line " +
                                 std::to_string(lineNumber));
    }

    const long long resolved =
            rawIndex > 0
                    ? rawIndex - 1
                    : static_cast<long long>(parsedVertices) + rawIndex;
    if (resolved < 0 ||
        static_cast<std::uint64_t>(resolved) >= parsedVertices ||
        resolved > std::numeric_limits<igIndex>::max()) {
        throw std::runtime_error("OBJ face index is out of range at line " +
                                 std::to_string(lineNumber));
    }

    cursor = end;
    while (*cursor != '\0' && *cursor != ' ' && *cursor != '\t') ++cursor;
    return static_cast<igIndex>(resolved);
}

iGame::SurfaceMesh::Pointer ReadTriangleObj(const std::string& path) {
    const TriangleObjCounts counts = CountTriangleObj(path);
    std::cout << "[BENCH][OBJ_READER] mode=test_relative_index vertices="
              << counts.vertices << " triangle_faces=" << counts.faces
              << '\n';

    auto points = iGame::Points::New();
    points->SetNumberOfPoints(static_cast<IGsize>(counts.vertices));
    float* coordinates = points->RawPointer();

    auto faceIds = iGame::IdArray::New();
    faceIds->SetNumberOfIds(
            static_cast<IGsize>(counts.faces * 3ULL));
    igIndex* indices = faceIds->RawPointer();

    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("cannot reopen OBJ model: " + path);
    }

    std::uint64_t parsedVertices = 0;
    std::uint64_t parsedFaces = 0;
    std::uint64_t lineNumber = 0;
    std::string line;
    while (std::getline(input, line)) {
        ++lineNumber;
        if (line.size() >= 2U && line[0] == 'v' && line[1] == ' ') {
            const char* cursor = line.c_str() + 2;
            const std::uint64_t offset = parsedVertices * 3ULL;
            coordinates[offset] = ParseObjCoordinate(cursor, lineNumber);
            coordinates[offset + 1ULL] =
                    ParseObjCoordinate(cursor, lineNumber);
            coordinates[offset + 2ULL] =
                    ParseObjCoordinate(cursor, lineNumber);
            ++parsedVertices;
        } else if (line.size() >= 2U && line[0] == 'f' && line[1] == ' ') {
            const char* cursor = line.c_str() + 2;
            const std::uint64_t offset = parsedFaces * 3ULL;
            for (std::uint64_t corner = 0; corner < 3ULL; ++corner) {
                indices[offset + corner] = ParseObjVertexIndex(
                        cursor, parsedVertices, lineNumber);
            }
            while (*cursor == ' ' || *cursor == '\t' || *cursor == '\r') {
                ++cursor;
            }
            if (*cursor != '\0' && *cursor != '#') {
                throw std::runtime_error(
                        "test OBJ reader requires triangular faces; line " +
                        std::to_string(lineNumber));
            }
            ++parsedFaces;
        }
    }
    if (!input.eof() || parsedVertices != counts.vertices ||
        parsedFaces != counts.faces) {
        throw std::runtime_error("OBJ contents changed or were truncated");
    }

    auto faces = iGame::CellArray::New();
    faces->SetData(faceIds, 3);
    auto mesh = iGame::SurfaceMesh::New();
    mesh->SetName("Triangle OBJ benchmark model");
    mesh->SetPoints(points);
    mesh->SetFaces(faces);
    return mesh;
}

iGame::DataObject::Pointer ReadModel(const std::string& path) {
    if (HasExtension(path, ".obj")) return ReadTriangleObj(path);
    return iGame::FileIO::ReadFile(path);
}

bool ParseBool(const std::string& value, bool& result) {
    if (value == "1" || value == "on" || value == "true") {
        result = true;
        return true;
    }
    if (value == "0" || value == "off" || value == "false") {
        result = false;
        return true;
    }
    return false;
}

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
        } else if (arg == "--meshlet") {
            const char* value = nextValue(i, "--meshlet");
            if (!value || !ParseBool(value, options.meshlet)) {
                std::cerr << "[BENCH][ERROR] --meshlet expects on/off or 1/0\n";
                return false;
            }
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
        } else if (arg == "--frames") {
            const char* value = nextValue(i, "--frames");
            if (!value) return false;
            options.measuredFrames = std::stoi(value);
        } else if (arg == "--capture") {
            const char* value = nextValue(i, "--capture");
            if (!value) return false;
            options.capturePath = value;
        } else if (arg == "--reference") {
            const char* value = nextValue(i, "--reference");
            if (!value) return false;
            options.referencePath = value;
        } else if (arg == "--help") {
            std::cout
                    << "Usage: testMeshletRendering [--benchmark] [--model PATH] "
                       "[--meshlet on|off] [--width N] [--height N] "
                       "[--warmup N] [--frames N] [--capture RGB_FILE] "
                       "[--reference RGB_FILE]\n";
            return false;
        } else {
            std::cerr << "[BENCH][ERROR] unknown argument: " << arg << '\n';
            return false;
        }
    }

    if (options.width <= 0 || options.height <= 0 ||
        options.warmupFrames < 1 || options.measuredFrames < 1) {
        std::cerr << "[BENCH][ERROR] invalid dimensions or frame counts\n";
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

std::uint64_t HashPixels(const std::vector<unsigned char>& pixels) {
    std::uint64_t hash = 1469598103934665603ULL;
    for (unsigned char value: pixels) {
        hash ^= value;
        hash *= 1099511628211ULL;
    }
    return hash;
}

bool WritePixels(const std::string& path,
                 const std::vector<unsigned char>& pixels) {
    std::ofstream output(path, std::ios::binary);
    if (!output) return false;
    output.write(reinterpret_cast<const char*>(pixels.data()),
                 static_cast<std::streamsize>(pixels.size()));
    return output.good();
}

bool ReadPixels(const std::string& path, std::vector<unsigned char>& pixels) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) return false;
    const auto size = input.tellg();
    if (size < 0) return false;
    pixels.resize(static_cast<std::size_t>(size));
    input.seekg(0, std::ios::beg);
    input.read(reinterpret_cast<char*>(pixels.data()), size);
    return input.good();
}

bool ComparePixels(const std::vector<unsigned char>& reference,
                   const std::vector<unsigned char>& actual) {
    if (reference.size() != actual.size() || actual.size() % 3U != 0U) {
        std::cout << "[BENCH][IMAGE_DIFF] status=size_mismatch reference_bytes="
                  << reference.size() << " actual_bytes=" << actual.size()
                  << '\n';
        return false;
    }

    double absoluteSum = 0.0;
    double squaredSum = 0.0;
    unsigned int maximum = 0;
    std::size_t badPixels = 0;
    std::size_t referenceForeground = 0;
    std::size_t missingPixels = 0;
    std::size_t extraPixels = 0;
    constexpr unsigned char background[3] = {16U, 18U, 24U};
    const std::size_t pixelCount = actual.size() / 3U;
    for (std::size_t pixel = 0; pixel < pixelCount; ++pixel) {
        bool bad = false;
        bool referenceIsForeground = false;
        bool actualIsForeground = false;
        for (std::size_t channel = 0; channel < 3U; ++channel) {
            const auto index = pixel * 3U + channel;
            const unsigned int difference = static_cast<unsigned int>(
                    std::abs(static_cast<int>(actual[index]) -
                             static_cast<int>(reference[index])));
            absoluteSum += static_cast<double>(difference);
            squaredSum += static_cast<double>(difference * difference);
            maximum = std::max(maximum, difference);
            bad = bad || difference > 8U;
            referenceIsForeground =
                    referenceIsForeground ||
                    std::abs(static_cast<int>(reference[index]) -
                             static_cast<int>(background[channel])) > 8;
            actualIsForeground =
                    actualIsForeground ||
                    std::abs(static_cast<int>(actual[index]) -
                             static_cast<int>(background[channel])) > 8;
        }
        if (bad) ++badPixels;
        if (referenceIsForeground) ++referenceForeground;
        if (referenceIsForeground && !actualIsForeground) ++missingPixels;
        if (!referenceIsForeground && actualIsForeground) ++extraPixels;
    }

    const double channelCount = static_cast<double>(actual.size());
    const double badPixelPct =
            100.0 * static_cast<double>(badPixels) /
            static_cast<double>(pixelCount);
    const double missingForegroundPct =
            referenceForeground == 0U
                    ? 0.0
                    : 100.0 * static_cast<double>(missingPixels) /
                              static_cast<double>(referenceForeground);
    const double extraImagePct =
            100.0 * static_cast<double>(extraPixels) /
            static_cast<double>(pixelCount);
    const bool visualMatch = badPixelPct <= 0.1 &&
                             missingForegroundPct <= 0.01 &&
                             extraImagePct <= 0.01;
    std::cout << "[BENCH][IMAGE_DIFF] status=compared mae="
              << absoluteSum / channelCount << " rmse="
              << std::sqrt(squaredSum / channelCount) << " max=" << maximum
              << " bad_pixel_pct=" << badPixelPct
              << " missing_foreground_pct=" << missingForegroundPct
              << " extra_image_pct=" << extraImagePct
              << " threshold=8 visual_match="
              << (visualMatch ? "true" : "false") << '\n';
    return visualMatch;
}

void PrintSummary(const char* label, const Summary& summary) {
    std::cout << "[BENCH][" << label << "] mean_ms=" << summary.mean
              << " median_ms=" << summary.median << " p95_ms=" << summary.p95
              << " p99_ms=" << summary.p99 << " min_ms=" << summary.minimum
              << " max_ms=" << summary.maximum << '\n';
}

int RunBenchmark(const Options& options) {
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "[BENCH][CONFIG] test=meshlet mode="
              << (options.meshlet ? "on" : "off") << " model=\""
              << options.model << "\" width=" << options.width
              << " height=" << options.height
              << " warmup_frames=" << options.warmupFrames
              << " measured_frames=" << options.measuredFrames << '\n';

    const auto loadStart = Clock::now();
    iGame::DataObject::Pointer dataObject = ReadModel(options.model);
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

    drawObject->SetAccelerationOption(options.meshlet);
    drawObject->SetViewStyle(IG_SURFACE);
    drawObject->SetDefaultColor(igm::vec3{1.0f, 1.0f, 1.0f});

    auto scene = iGame::Scene::New();
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
    std::cout << "[BENCH][INIT] scene_and_gpu_ms="
              << Milliseconds(initializeEnd - initializeStart) << '\n';

    const auto firstFrameStart = Clock::now();
    window->RenderOneFrame();
    glFinish();
    const auto firstFrameEnd = Clock::now();
    std::cout << "[BENCH][FIRST_FRAME] cpu_to_gpu_complete_ms="
              << Milliseconds(firstFrameEnd - firstFrameStart) << '\n';
    std::cout << "[BENCH][MODE] requested="
              << (options.meshlet ? "on" : "off") << " effective="
              << (drawObject->GetAccelerationOption() ? "on" : "off")
              << '\n';

    for (int i = 1; i < options.warmupFrames; ++i) {
        window->RenderOneFrame();
    }
    glFinish();

    std::vector<GLuint> timestampQueries(
            static_cast<std::size_t>(options.measuredFrames) * 2U);
    glGenQueries(static_cast<GLsizei>(timestampQueries.size()),
                 timestampQueries.data());

    std::vector<double> cpuFrameMs;
    cpuFrameMs.reserve(options.measuredFrames);
    const auto measurementStart = Clock::now();
    for (int frame = 0; frame < options.measuredFrames; ++frame) {
        const auto queryIndex = static_cast<std::size_t>(frame) * 2U;
        glQueryCounter(timestampQueries[queryIndex], GL_TIMESTAMP);
        const auto cpuStart = Clock::now();
        window->RenderOneFrame();
        const auto cpuEnd = Clock::now();
        glQueryCounter(timestampQueries[queryIndex + 1U], GL_TIMESTAMP);
        cpuFrameMs.push_back(Milliseconds(cpuEnd - cpuStart));
    }
    glFinish();
    const auto measurementEnd = Clock::now();

    std::vector<double> gpuFrameMs;
    gpuFrameMs.reserve(options.measuredFrames);
    for (int frame = 0; frame < options.measuredFrames; ++frame) {
        const auto queryIndex = static_cast<std::size_t>(frame) * 2U;
        GLuint64 startNs = 0;
        GLuint64 endNs = 0;
        glGetQueryObjectui64v(timestampQueries[queryIndex], GL_QUERY_RESULT,
                              &startNs);
        glGetQueryObjectui64v(timestampQueries[queryIndex + 1U],
                              GL_QUERY_RESULT, &endNs);
        if (endNs >= startNs) {
            gpuFrameMs.push_back(
                    static_cast<double>(endNs - startNs) / 1000000.0);
        }
    }
    glDeleteQueries(static_cast<GLsizei>(timestampQueries.size()),
                    timestampQueries.data());

    const double measuredSeconds =
            std::chrono::duration<double>(measurementEnd - measurementStart)
                    .count();
    const double throughputFps =
            static_cast<double>(options.measuredFrames) / measuredSeconds;
    PrintSummary("CPU_FRAME", Summarize(cpuFrameMs));
    PrintSummary("GPU_FRAME", Summarize(gpuFrameMs));
    std::cout << "[BENCH][THROUGHPUT] elapsed_s=" << measuredSeconds
              << " completed_fps=" << throughputFps << '\n';

    GLint viewport[4] = {0, 0, options.width, options.height};
    GLint previousPackAlignment = 4;
    glGetIntegerv(GL_VIEWPORT, viewport);
    glGetIntegerv(GL_PACK_ALIGNMENT, &previousPackAlignment);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    const auto pixels = scene->CaptureScreen(
            viewport[0], viewport[1], viewport[2], viewport[3],
            iGame::GLFramebuffer::Type::RGB, false);
    glPixelStorei(GL_PACK_ALIGNMENT, previousPackAlignment);
    std::cout << "[BENCH][IMAGE] width=" << viewport[2]
              << " height=" << viewport[3] << " bytes=" << pixels.size()
              << " fnv1a64=0x" << std::hex << HashPixels(pixels) << std::dec
              << '\n';
    bool visualMatch = true;
    if (!options.capturePath.empty()) {
        if (!WritePixels(options.capturePath, pixels)) {
            std::cerr << "[BENCH][ERROR] failed to write capture: "
                      << options.capturePath << '\n';
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            window->SetScene(nullptr);
            scene->Finalize();
            scene = nullptr;
            drawObject = nullptr;
            dataObject = nullptr;
            return 5;
        }
        std::cout << "[BENCH][CAPTURE] path=\"" << options.capturePath
                  << "\" bytes=" << pixels.size() << '\n';
    }
    if (!options.referencePath.empty()) {
        std::vector<unsigned char> reference;
        if (!ReadPixels(options.referencePath, reference)) {
            std::cerr << "[BENCH][ERROR] failed to read reference: "
                      << options.referencePath << '\n';
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            window->SetScene(nullptr);
            scene->Finalize();
            scene = nullptr;
            drawObject = nullptr;
            dataObject = nullptr;
            return 6;
        }
        visualMatch = ComparePixels(reference, pixels);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    window->SetScene(nullptr);
    scene->Finalize();
    scene = nullptr;
    drawObject = nullptr;
    dataObject = nullptr;
    std::cout << "[BENCH][DONE] status="
              << (visualMatch ? "pass" : "fail") << '\n';
    return visualMatch ? 0 : 9;
}
} // namespace

int main(int argc, char** argv) {
    Options options;
    try {
        if (!ParseOptions(argc, argv, options)) return 1;
        if (options.benchmark) return RunBenchmark(options);

        auto scene = iGame::Scene::New();
        auto dataObject = ReadModel(options.model);
        auto drawObject = iGame::DynamicCast<iGame::DrawObject>(dataObject);
        if (!drawObject) return 2;
        drawObject->SetAccelerationOption(options.meshlet);
        scene->AddModel(dataObject);
        scene->ResetCameraView();

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
