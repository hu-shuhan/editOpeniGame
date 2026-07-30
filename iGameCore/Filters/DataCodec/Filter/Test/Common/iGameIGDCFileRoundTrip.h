#ifndef iGameIGDCFileRoundTrip_h
#define iGameIGDCFileRoundTrip_h

#include "DataCodec/Filter/Test/Common/iGameDataCodecTestFilePath.h"
#include "DataCodec/Test/Common/DataCodecTestResult.h"
#include "DataCodec/API/Params/CodecParamFactories.h"
#include "DataCodec/Log/Telemetry/Sinks/TelemetrySessionSink.h"
#include "DataCodec/Runtime/Record/RunRecordDispatcher.h"
#include "IGDC/iGameIGDCReader.h"
#include "IGDC/iGameIGDCWriter.h"
#include "iGameDataObject.h"

#include <filesystem>
#include <algorithm>
#include <string>
#include <system_error>
#include <utility>
#include <vector>
namespace iGame::datacodec_test {
using namespace ::datacodec;
using namespace ::datacodec::test;

using iGame::DataObject;
using iGame::IGDCReader;
using iGame::IGDCWriter;

struct EncodeFileResult {
    TestResult status;
    std::filesystem::path encodedFile;
    std::vector<TelemetrySession> telemetrySessions;
};

struct DecodeFileResult {
    TestResult status;
    DataObject::Pointer decodedObject;
    std::vector<TelemetrySession> telemetrySessions;
};

inline void AppendMessageTexts(
    TestResult& result,
    const std::vector<TelemetryMessageRecord>& messages) {
    for (const auto& message : messages) {
        result.AddDiagnostic(message.text);
    }
}

[[nodiscard]] inline CodecControlParams MakeDefaultEncodeParams() {
    return CodecControlParamsFactory::MakeDefault();
}

[[nodiscard]] inline std::filesystem::path ResolveEncodedPackagePath(const std::filesystem::path& outputHint) {
    if (std::filesystem::is_regular_file(outputHint)) {
        return outputHint;
    }
    const auto outputDirectory = outputHint.parent_path();
    if (outputDirectory.empty()) {
        return outputHint;
    }

    const auto outputStem = outputHint.stem().string();
    std::vector<std::filesystem::path> candidates;
    std::error_code errorCode;
    for (const auto& entry : std::filesystem::directory_iterator(outputDirectory, errorCode)) {
        if (errorCode) {
            return outputHint;
        }
        if (!entry.is_regular_file(errorCode) || errorCode) {
            errorCode.clear();
            continue;
        }
        const auto extension = ToLowerAscii(entry.path().extension().string());
        if (extension != ".igc") {
            continue;
        }
        const auto stem = entry.path().stem().string();
        if (stem == outputStem || stem.rfind(outputStem + "_", 0u) == 0u) {
            candidates.push_back(entry.path());
        }
    }
    std::sort(candidates.begin(), candidates.end());
    if (!candidates.empty()) {
        return candidates.front();
    }
    return outputHint;
}

[[nodiscard]] inline EncodeFileResult EncodeDataObjectToFile(
    const DataObject::Pointer& sourceObject,
    std::filesystem::path outputHint,
    DataCodecEncodeConfigurationParams configuration,
    IRunRecordSink* additionalSink = nullptr) {
    EncodeFileResult result;
    result.encodedFile = outputHint;

    if (sourceObject == nullptr) {
        result.status.AddFailure("EncodeDataObjectToFile.input", "source object is null");
        return result;
    }

    auto writer = IGDCWriter::New();
    auto telemetrySink = std::make_shared<TelemetrySessionSink>(
        kRunLifecycleRecordMask |
        RunRecordKind::Message |
        RunRecordKind::StageTiming |
        RunRecordKind::ResourceUsage);
    auto recordDispatcher = std::make_shared<RunRecordDispatcher>();
    recordDispatcher->AddSink(telemetrySink);
    recordDispatcher->AddSink(additionalSink);
    writer->SetEncodeControls(configuration);
    writer->SetTelemetrySink(recordDispatcher);
    if (!writer->WriteToFile(sourceObject, PathToUtf8String(outputHint))) {
        result.telemetrySessions = telemetrySink->SnapshotCompletedSessions();
        result.status.AddFailure("EncodeDataObjectToFile.write", "writer returned failure");
        AppendMessageTexts(result.status, telemetrySink->SnapshotMessages());
        return result;
    }

    result.telemetrySessions = telemetrySink->SnapshotCompletedSessions();
    result.encodedFile = ResolveEncodedPackagePath(outputHint);
    if (!std::filesystem::is_regular_file(result.encodedFile)) {
        result.status.AddFailure("EncodeDataObjectToFile.output", "encoded file was not created");
    }
    AppendMessageTexts(result.status, telemetrySink->SnapshotMessages());
    return result;
}

[[nodiscard]] inline EncodeFileResult EncodeDataObjectToFile(
    const DataObject::Pointer& sourceObject,
    std::filesystem::path outputHint,
    CodecControlParams params = MakeDefaultEncodeParams(),
    IRunRecordSink* additionalSink = nullptr) {
    auto configuration = MakeDefaultEncodeConfigurationParams();
    configuration.controlParams = std::move(params);
    configuration.source.customControlParams = true;
    return EncodeDataObjectToFile(
        sourceObject,
        std::move(outputHint),
        std::move(configuration),
        additionalSink);
}

[[nodiscard]] inline DecodeFileResult DecodeFileToDataObject(
    const std::filesystem::path& encodedFile,
    DecodeControlParams params = {}) {
    DecodeFileResult result;

    if (!std::filesystem::is_regular_file(encodedFile)) {
        result.status.AddFailure("DecodeFileToDataObject.input", "encoded file is missing");
        return result;
    }

    auto reader = IGDCReader::New();
    auto telemetrySink = std::make_shared<TelemetrySessionSink>(
        kRunLifecycleRecordMask |
        RunRecordKind::Message |
        RunRecordKind::StageTiming |
        RunRecordKind::ResourceUsage);
    auto recordDispatcher = std::make_shared<RunRecordDispatcher>();
    recordDispatcher->AddSink(telemetrySink);
    reader->SetCodecControlParams(params);
    reader->SetTelemetrySink(recordDispatcher);
    result.decodedObject = reader->ReadFile(PathToUtf8String(encodedFile));
    result.telemetrySessions = telemetrySink->SnapshotCompletedSessions();
    if (result.decodedObject == nullptr) {
        result.status.AddFailure("DecodeFileToDataObject.read", "reader returned null object");
    }
    AppendMessageTexts(result.status, telemetrySink->SnapshotMessages());
    return result;
}

}

#endif
