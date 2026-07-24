#ifndef iGameDataCodecScenarioRunner_h
#define iGameDataCodecScenarioRunner_h

#include "DataCodec/Filter/Test/Common/iGameDataCodecTestFilePath.h"
#include "DataCodec/Test/Common/DataCodecTestResult.h"
#include "DataCodec/Filter/Test/Comparison/iGameDataCodecComparator.h"
#include "DataCodec/Filter/Test/Comparison/iGameDataCodecRoundTripOracle.h"
#include "DataCodec/Log/Capture/RemapOrderCapture.h"
#include "DataCodec/Filter/Test/Data/iGameDataCodecDataGenerator.h"
#include "DataCodec/Filter/Test/Data/iGameDataCodecRealDatasetLoader.h"
#include "DataCodec/Filter/Test/Common/iGameIGDCFileRoundTrip.h"
#include "DataCodec/Test/Assertions/TelemetryArtifactAssertions.h"
#include "iGameFileIO.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>
namespace iGame::datacodec_test {
using namespace ::datacodec;
using namespace ::datacodec::test;

using iGame::DataObject;
using iGame::DrawObject;
using iGame::FileIO;

enum class DataCodecScenarioKind {
    SingleBlockSingleFrame,
    MultiBlockSingleFrame,
    SingleBlockMultiFrame,
    MultiBlockMultiFrame,
};

struct DataCodecScenarioOptions {
    std::filesystem::path outputRoot;
    bool enableTelemetry{true};
    SyntheticDataShape singleBlockShape{SyntheticDataShape::Surface};
    DataObjectSignatureOptions signatureOptions;
};

struct DataCodecScenarioInput {
    std::string name;
    DataObject::Pointer encodeSource;
    DataObject::Pointer expectedDecodedEntry;
    std::size_t expectedFrameCount{1u};
    bool expectsMultiFrame{false};
    bool expectsMultiBlock{false};
};

[[nodiscard]] inline const char* DataCodecScenarioName(const DataCodecScenarioKind kind) {
    switch (kind) {
        case DataCodecScenarioKind::SingleBlockSingleFrame:
            return "scenario_single-block_single-frame";
        case DataCodecScenarioKind::MultiBlockSingleFrame:
            return "scenario_multi-block_single-frame";
        case DataCodecScenarioKind::SingleBlockMultiFrame:
            return "scenario_single-block_multi-frame";
        case DataCodecScenarioKind::MultiBlockMultiFrame:
            return "scenario_multi-block_multi-frame";
    }
    return "scenario_unknown";
}

inline void AppendStatus(TestResult& target, const TestResult& source) {
    if (!source.passed) {
        target.passed = false;
    }
    target.failures.insert(target.failures.end(), source.failures.begin(), source.failures.end());
    target.diagnostics.insert(target.diagnostics.end(), source.diagnostics.begin(), source.diagnostics.end());
}

[[nodiscard]] inline CodecControlParams MakeScenarioCodecParams() {
    auto params = MakeDefaultEncodeParams();
    params.attrReference.enabled = false;
    params.geometryReference.enabled = false;
    params.topologyReference.enabled = false;
    return params;
}

[[nodiscard]] inline std::filesystem::path DefaultScenarioOutputRoot() {
    const auto salt = std::chrono::steady_clock::now().time_since_epoch().count();
    return std::filesystem::temp_directory_path() /
        ("igame_datacodec_scenario_" + std::to_string(salt));
}

inline bool PrepareScenarioDirectory(const std::filesystem::path& directory, TestResult& result) {
    std::error_code errorCode;
    std::filesystem::remove_all(directory, errorCode);
    errorCode.clear();
    std::filesystem::create_directories(directory, errorCode);
    if (errorCode) {
        result.AddFailure("scenario.outputDirectory", "failed to create scenario directory");
        return false;
    }
    return true;
}

inline bool WriteScenarioVtk(
    const std::filesystem::path& path,
    const SyntheticSurfaceOptions& options,
    TestResult& result) {
    std::string error;
    if (WriteSyntheticSurfaceVtkFile(path, options, &error)) {
        return true;
    }
    result.AddFailure("scenario.fixture.vtk", error.empty() ? "failed to write synthetic vtk" : error);
    return false;
}

[[nodiscard]] inline DataObject::Pointer ReadScenarioObject(
    const std::filesystem::path& path,
    TestResult& result) {
    auto object = FileIO::ReadFile(PathToUtf8String(path));
    if (object == nullptr) {
        result.AddFailure("scenario.fixture.read", "failed to read synthetic vtk " + path.filename().string());
    }
    return object;
}

[[nodiscard]] inline DataObject::Pointer BuildExpectedMultiBlockFrame(
    const std::string& name,
    const std::vector<std::filesystem::path>& files,
    TestResult& result) {
    auto root = DrawObject::New();
    root->SetName(name);
    for (const auto& file : files) {
        auto part = ReadScenarioObject(file, result);
        if (part == nullptr) {
            return nullptr;
        }
        root->AddSubDataObject(part);
    }
    return root;
}

[[nodiscard]] inline DataCodecScenarioInput BuildScenarioInput(
    const DataCodecScenarioKind kind,
    const std::filesystem::path& inputDirectory,
    const DataCodecScenarioOptions& options,
    TestResult& result) {
    DataCodecScenarioInput input;
    input.name = DataCodecScenarioName(kind);

    switch (kind) {
        case DataCodecScenarioKind::SingleBlockSingleFrame:
            input.encodeSource = BuildSyntheticSingleBlockSmokeObject(options.singleBlockShape);
            input.expectedDecodedEntry = input.encodeSource;
            return input;

        case DataCodecScenarioKind::MultiBlockSingleFrame:
            input.encodeSource = BuildSyntheticMultiBlockSmokeObject();
            input.expectedDecodedEntry = input.encodeSource;
            input.expectsMultiBlock = true;
            return input;

        case DataCodecScenarioKind::SingleBlockMultiFrame: {
            const auto frame0 = inputDirectory / "single_block_frame_0000.vtk";
            const auto frame1 = inputDirectory / "single_block_frame_0001.vtk";
            if (!WriteScenarioVtk(frame0, SyntheticSurfaceOptions{
                    .name = "scenario_single_block_frame_0",
                    .zOffset = 0.0f,
                    .pointScalarOffset = 0.0f,
                    .cellScalarOffset = 0.0f,
                }, result) ||
                !WriteScenarioVtk(frame1, SyntheticSurfaceOptions{
                    .name = "scenario_single_block_frame_1",
                    .zOffset = 0.125f,
                    .pointScalarOffset = 1.0f,
                    .cellScalarOffset = 2.0f,
                }, result)) {
                return input;
            }
            input.encodeSource = BuildAnimationSourceObject(input.name, std::vector<std::filesystem::path>{frame0, frame1});
            input.expectedDecodedEntry = ReadScenarioObject(frame0, result);
            input.expectedFrameCount = 2u;
            input.expectsMultiFrame = true;
            return input;
        }

        case DataCodecScenarioKind::MultiBlockMultiFrame: {
            const auto frame0Part0 = inputDirectory / "multi_block_frame_0000_part_0.vtk";
            const auto frame0Part1 = inputDirectory / "multi_block_frame_0000_part_1.vtk";
            const auto frame1Part0 = inputDirectory / "multi_block_frame_0001_part_0.vtk";
            const auto frame1Part1 = inputDirectory / "multi_block_frame_0001_part_1.vtk";
            if (!WriteScenarioVtk(frame0Part0, SyntheticSurfaceOptions{
                    .name = "scenario_multi_block_frame_0_part_0",
                    .zOffset = 0.0f,
                    .pointScalarOffset = 0.0f,
                    .cellScalarOffset = 0.0f,
                }, result) ||
                !WriteScenarioVtk(frame0Part1, SyntheticSurfaceOptions{
                    .name = "scenario_multi_block_frame_0_part_1",
                    .zOffset = 0.5f,
                    .pointScalarOffset = 10.0f,
                    .cellScalarOffset = 100.0f,
                }, result) ||
                !WriteScenarioVtk(frame1Part0, SyntheticSurfaceOptions{
                    .name = "scenario_multi_block_frame_1_part_0",
                    .zOffset = 0.125f,
                    .pointScalarOffset = 1.0f,
                    .cellScalarOffset = 2.0f,
                }, result) ||
                !WriteScenarioVtk(frame1Part1, SyntheticSurfaceOptions{
                    .name = "scenario_multi_block_frame_1_part_1",
                    .zOffset = 0.625f,
                    .pointScalarOffset = 11.0f,
                    .cellScalarOffset = 102.0f,
                }, result)) {
                return input;
            }
            input.encodeSource = BuildAnimationSourceObject(
                input.name,
                std::vector<std::vector<std::filesystem::path>>{
                    {frame0Part0, frame0Part1},
                    {frame1Part0, frame1Part1},
                });
            input.expectedDecodedEntry = BuildExpectedMultiBlockFrame(
                "scenario_multi_block_frame_0",
                {frame0Part0, frame0Part1},
                result);
            input.expectedFrameCount = 2u;
            input.expectsMultiFrame = true;
            input.expectsMultiBlock = true;
            return input;
        }
    }

    result.AddFailure("scenario.kind", "unsupported scenario kind");
    return input;
}

[[nodiscard]] inline std::size_t CountEncodedScenarioFiles(
    const std::filesystem::path& directory,
    const std::string& stem,
    const std::string& extension) {
    std::size_t count = 0u;
    std::error_code errorCode;
    for (const auto& entry : std::filesystem::directory_iterator(directory, errorCode)) {
        if (errorCode) {
            return count;
        }
        if (!entry.is_regular_file(errorCode) || errorCode) {
            errorCode.clear();
            continue;
        }
        if (ToLowerAscii(entry.path().extension().string()) != extension) {
            continue;
        }
        const auto entryStem = entry.path().stem().string();
        if (entryStem == stem || entryStem.rfind(stem + "_", 0u) == 0u) {
            ++count;
        }
    }
    return count;
}

inline void ValidateScenarioOutputShape(
    TestResult& result,
    const DataCodecScenarioInput& input,
    const std::filesystem::path& packageDirectory,
    const std::filesystem::path& entryFile) {
    const auto extension = ToLowerAscii(entryFile.extension().string());
    const std::string expectedExtension = ".igc";
    Require(
        result,
        extension == expectedExtension,
        "scenario.output.extension",
        "encoded entry extension mismatch");
    if (input.expectsMultiFrame) {
        const auto count = CountEncodedScenarioFiles(packageDirectory, input.name, expectedExtension);
        Require(result, count == input.expectedFrameCount, "scenario.output.frameCount", "encoded frame count mismatch");
    } else {
        const auto count = CountEncodedScenarioFiles(packageDirectory, input.name, expectedExtension);
        Require(result, count == 1u, "scenario.output.singleFrameCount", "single-frame scenario should emit one frame");
    }
}

inline void ValidateScenarioDecodedShape(
    TestResult& result,
    const DataCodecScenarioInput& input,
    const DataObjectCompareResult& compareResult) {
    if (!compareResult.status.passed) {
        return;
    }
    const auto actualLeafCount = compareResult.actualSignature.leaves.size();
    Require(
        result,
        actualLeafCount == compareResult.expectedSignature.leaves.size(),
        "scenario.decoded.leafCount",
        "decoded leaf count mismatch");
    if (input.expectsMultiBlock) {
        Require(result, actualLeafCount > 1u, "scenario.decoded.multiBlock", "multi-block scenario should decode multiple leaves");
    } else {
        Require(result, actualLeafCount == 1u, "scenario.decoded.singleBlock", "single-block scenario should decode one leaf");
    }
}

[[nodiscard]] inline TestResult RunDataCodecScenario(
    const DataCodecScenarioKind kind,
    DataCodecScenarioOptions options = {}) {
    TestResult result;
    if (options.outputRoot.empty()) {
        options.outputRoot = DefaultScenarioOutputRoot();
    }

    const auto scenarioName = std::string(DataCodecScenarioName(kind));
    const auto scenarioDirectory = options.outputRoot / scenarioName;
    const auto inputDirectory = scenarioDirectory / "inputs";
    const auto packageDirectory = scenarioDirectory / "packages";
    if (!PrepareScenarioDirectory(inputDirectory, result) ||
        !PrepareScenarioDirectory(packageDirectory, result)) {
        return result;
    }

    auto input = BuildScenarioInput(kind, inputDirectory, options, result);
    if (!result.passed) {
        return result;
    }
    Require(result, input.encodeSource != nullptr, "scenario.input.encodeSource", "encode source is null");
    Require(
        result,
        input.expectedDecodedEntry != nullptr,
        "scenario.input.expectedDecodedEntry",
        "expected decoded entry is null");
    if (!result.passed) {
        return result;
    }

    auto params = MakeScenarioCodecParams();

    const auto outputHint = packageDirectory / (scenarioName + ".igc");
    log::RemapOrderCapture remapOrders;
    const auto encodeResult = EncodeDataObjectToFile(input.encodeSource, outputHint, params, &remapOrders);
    AppendStatus(result, encodeResult.status);
    if (!result.passed) {
        return result;
    }
    Require(
        result,
        std::filesystem::is_regular_file(encodeResult.encodedFile),
        "scenario.output.entry",
        "encoded entry file is missing");
    ValidateScenarioOutputShape(result, input, packageDirectory, encodeResult.encodedFile);
    if (options.enableTelemetry) {
        Require(
            result,
            HasSerializableTelemetryJson(encodeResult.telemetrySessions),
            "scenario.telemetry",
            "telemetry sessions could not be serialized to json");
    }
    if (!result.passed) {
        return result;
    }

    auto decodeResult = DecodeFileToDataObject(encodeResult.encodedFile, params);
    AppendStatus(result, decodeResult.status);
    if (!result.passed) {
        return result;
    }

    CodecRoundTripOracle oracle(options.signatureOptions);
    auto orderSnapshot = remapOrders.TakeSnapshot();
    oracle.SetPointOrders(std::move(orderSnapshot.pointOrders));
    oracle.SetCellOrders(std::move(orderSnapshot.cellOrders));
    const auto compareResult = oracle.Compare(input.expectedDecodedEntry, decodeResult.decodedObject);
    AppendStatus(result, compareResult.status);
    ValidateScenarioDecodedShape(result, input, compareResult);
    return result;
}

}

#endif
