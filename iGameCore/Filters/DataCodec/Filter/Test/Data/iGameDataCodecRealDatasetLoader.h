#ifndef iGameDataCodecTestingRealDatasetLoader_h
#define iGameDataCodecTestingRealDatasetLoader_h

#include "DataCodec/Filter/Test/Common/iGameDataCodecTestFilePath.h"
#include "iGameDrawObject.h"
#include "iGameFileIO.h"
#include "iGameStreamingData.h"
#include "iGameStringArray.h"

#include <algorithm>
#include <cstddef>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
namespace iGame::datacodec_test {
using namespace ::datacodec;
using namespace ::datacodec::test;

using iGame::DataObject;
using iGame::DrawObject;
using iGame::FileIO;
using iGame::StreamingData;
using iGame::StringArray;
using ::StreamingType;

enum class RealDatasetKind {
    SingleFrame,
    Animation,
};

struct RealDatasetCase {
    std::string name;
    std::filesystem::path sourcePath;
    RealDatasetKind kind{RealDatasetKind::SingleFrame};
};

[[nodiscard]] inline bool IsRawDataFile(const std::filesystem::path& path) {
    const auto extension = ToLowerAscii(path.extension().string());
    return extension == ".cgns" || extension == ".vtk" || extension == ".vtu" || extension == ".vtp";
}

[[nodiscard]] inline std::optional<std::uint64_t> ExtractTrailingNumber(const std::string& text) {
    std::size_t end = text.size();
    while (end > 0u && !std::isdigit(static_cast<unsigned char>(text[end - 1u]))) {
        --end;
    }
    if (end == 0u) {
        return std::nullopt;
    }
    std::size_t begin = end;
    while (begin > 0u && std::isdigit(static_cast<unsigned char>(text[begin - 1u]))) {
        --begin;
    }
    std::uint64_t value = 0u;
    for (std::size_t index = begin; index < end; ++index) {
        value = value * 10u + static_cast<std::uint64_t>(text[index] - '0');
    }
    return value;
}

[[nodiscard]] inline std::vector<std::filesystem::path> CollectAnimationFiles(
    const std::filesystem::path& sourceDirectory) {
    std::vector<std::filesystem::path> files;
    std::error_code errorCode;
    if (!std::filesystem::is_directory(sourceDirectory, errorCode) || errorCode) {
        return files;
    }
    for (const auto& entry : std::filesystem::directory_iterator(sourceDirectory, errorCode)) {
        if (errorCode) {
            files.clear();
            return files;
        }
        if (!entry.is_regular_file(errorCode) || errorCode) {
            errorCode.clear();
            continue;
        }
        if (IsRawDataFile(entry.path())) {
            files.push_back(entry.path());
        }
    }
    std::sort(
        files.begin(),
        files.end(),
        [](const std::filesystem::path& lhs, const std::filesystem::path& rhs) {
            const auto lhsNumber = ExtractTrailingNumber(lhs.stem().string());
            const auto rhsNumber = ExtractTrailingNumber(rhs.stem().string());
            if (lhsNumber.has_value() && rhsNumber.has_value() && lhsNumber != rhsNumber) {
                return *lhsNumber < *rhsNumber;
            }
            return lhs.filename().string() < rhs.filename().string();
        });
    return files;
}

[[nodiscard]] inline DataObject::Pointer BuildAnimationSourceObject(
    const std::string& objectName,
    const std::vector<std::vector<std::filesystem::path>>& frameFileSets) {
    auto rootObject = DrawObject::New();
    rootObject->SetName(objectName);
    auto timeFrames = StreamingData::New();
    for (std::size_t frameIndex = 0u; frameIndex < frameFileSets.size(); ++frameIndex) {
        auto files = StringArray::New();
        for (const auto& frameFile : frameFileSets[frameIndex]) {
            files->AddElement(PathToUtf8String(frameFile));
        }
        timeFrames->AddTimeStep(static_cast<float>(frameIndex), files, StreamingType::MultiSubFiles);
    }
    rootObject->SetTimeFrames(timeFrames);
    return rootObject;
}

[[nodiscard]] inline DataObject::Pointer BuildAnimationSourceObject(
    const std::string& objectName,
    const std::vector<std::filesystem::path>& frameFiles) {
    std::vector<std::vector<std::filesystem::path>> frameFileSets;
    frameFileSets.reserve(frameFiles.size());
    for (const auto& frameFile : frameFiles) {
        frameFileSets.push_back({frameFile});
    }
    return BuildAnimationSourceObject(objectName, frameFileSets);
}

[[nodiscard]] inline std::filesystem::path ResolveExecutableDirectory() {
#if defined(_WIN32)
    std::wstring pathBuffer(260, L'\0');
    for (;;) {
        const auto length = GetModuleFileNameW(nullptr, pathBuffer.data(), static_cast<DWORD>(pathBuffer.size()));
        if (length == 0) {
            return {};
        }
        if (length < pathBuffer.size() - 1u) {
            pathBuffer.resize(length);
            return std::filesystem::path(pathBuffer).parent_path();
        }
        pathBuffer.resize(pathBuffer.size() * 2u);
    }
#else
    std::error_code errorCode;
    return std::filesystem::current_path(errorCode);
#endif
}

[[nodiscard]] inline bool IsRepoRoot(const std::filesystem::path& path) {
    std::error_code errorCode;
    return std::filesystem::exists(path / "CMakeLists.txt", errorCode) &&
        std::filesystem::is_directory(path / "Examples", errorCode) &&
        std::filesystem::is_directory(path / "iGameCore", errorCode);
}

[[nodiscard]] inline std::filesystem::path FindRepoRootFrom(std::filesystem::path startPath) {
    std::error_code errorCode;
    if (startPath.empty()) {
        return {};
    }

    startPath = std::filesystem::weakly_canonical(startPath, errorCode);
    if (errorCode) {
        startPath = startPath.lexically_normal();
    }

    for (auto current = startPath; !current.empty(); current = current.parent_path()) {
        if (IsRepoRoot(current)) {
            return current;
        }
        if (current == current.root_path()) {
            break;
        }
    }
    return {};
}

[[nodiscard]] inline std::filesystem::path ResolveRepoRoot() {
    if (const auto repoRoot = FindRepoRootFrom(ResolveExecutableDirectory()); !repoRoot.empty()) {
        return repoRoot;
    }

    std::error_code errorCode;
    return FindRepoRootFrom(std::filesystem::current_path(errorCode));
}

[[nodiscard]] inline std::filesystem::path ResolveDataRoot(const std::filesystem::path& explicitDataRoot = {}) {
    if (!explicitDataRoot.empty()) {
        return explicitDataRoot;
    }
    const auto repoRoot = ResolveRepoRoot();
    return repoRoot.empty() ? std::filesystem::path{} : repoRoot / "Datas";
}

[[nodiscard]] inline std::filesystem::path MakeDataCodecOutputRoot(
    const std::filesystem::path& dataRoot,
    const std::string& label) {
    return dataRoot / label;
}

[[nodiscard]] inline std::vector<RealDatasetCase> CollectRealDatasetCases(const std::filesystem::path& dataRoot) {
    std::vector<RealDatasetCase> cases;
    std::error_code errorCode;
    if (!std::filesystem::is_directory(dataRoot, errorCode) || errorCode) {
        return cases;
    }

    for (const auto& entry : std::filesystem::directory_iterator(dataRoot, errorCode)) {
        if (errorCode) {
            cases.clear();
            return cases;
        }
        if (entry.is_regular_file(errorCode) && IsRawDataFile(entry.path())) {
            cases.push_back(RealDatasetCase{
                .name = entry.path().stem().string(),
                .sourcePath = entry.path(),
                .kind = RealDatasetKind::SingleFrame,
            });
            continue;
        }
        errorCode.clear();
        if (!entry.is_directory(errorCode) || errorCode) {
            errorCode.clear();
            continue;
        }
        const auto directoryName = entry.path().filename().string();
        if (directoryName.rfind("datacodec-", 0u) == 0u) {
            continue;
        }
        if (!CollectAnimationFiles(entry.path()).empty()) {
            cases.push_back(RealDatasetCase{
                .name = directoryName,
                .sourcePath = entry.path(),
                .kind = RealDatasetKind::Animation,
            });
        }
    }

    std::sort(cases.begin(), cases.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.name < rhs.name;
    });
    return cases;
}

[[nodiscard]] inline DataObject::Pointer LoadSingleFrameSourceObject(const std::filesystem::path& sourceFile) {
    return FileIO::ReadFile(PathToUtf8String(sourceFile));
}

[[nodiscard]] inline DataObject::Pointer LoadAnimationSourceObject(
    const std::string& objectName,
    const std::filesystem::path& sourceDirectory) {
    const auto frameFiles = CollectAnimationFiles(sourceDirectory);
    if (frameFiles.empty()) {
        return nullptr;
    }
    return BuildAnimationSourceObject(objectName, frameFiles);
}

[[nodiscard]] inline DataObject::Pointer LoadRepeatedSingleFileAnimationSourceObject(
    const std::string& objectName,
    const std::filesystem::path& sourceFile,
    const std::size_t frameCount) {
    if (frameCount == 0u) {
        return nullptr;
    }
    std::vector<std::filesystem::path> frameFiles(frameCount, sourceFile);
    return BuildAnimationSourceObject(objectName, frameFiles);
}

[[nodiscard]] inline DataObject::Pointer LoadRealDatasetSourceObject(const RealDatasetCase& testCase) {
    if (testCase.kind == RealDatasetKind::Animation) {
        return LoadAnimationSourceObject(testCase.name, testCase.sourcePath);
    }
    return LoadSingleFrameSourceObject(testCase.sourcePath);
}

} // namespace iGame::datacodec_test

#endif
