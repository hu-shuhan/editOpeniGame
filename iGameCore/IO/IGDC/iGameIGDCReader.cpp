#include "iGameIGDCReader.h"

#include "DataCodec/Filter/Execution/iGameDataCodecThreadPoolTaskRunner.h"
#include "DataCodec/Filter/Execution/iGameRunRecordSink.h"
#include "DataCodec/Filter/Playback/iGameFrameSequenceDecodeBridge.h"
#include "DataCodec/Storage/ByteIO/ByteRange.h"
#include "DataCodec/Storage/Package/PackageBinaryHeader.h"
#include "iGameDataCodecIOSettings.h"
#include "DataCodec/Filter/Adapter/iGameFileByteRangeIO.h"
#include "iGameIGDCFrameSequence.h"
#include "Log/iGameLogger.h"

#include <chrono>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <utility>

IGAME_NAMESPACE_BEGIN

struct IGDCReader::State {
    bool hasCodecParams{false};
    ::datacodec::DecodeControlParams codecParams;
    ::datacodec::DecodeExecutionOptions executionOptions;
    ::datacodec::DataCodecDecodeConfigurationSource configurationSource;
    std::vector<::datacodec::TelemetryMessageRecord> messages;
    std::vector<std::string> selectedFramePaths;
    ::datacodec::DecodedFrameCachePolicy decodedFrameCachePolicy;
    std::shared_ptr<::datacodec::IDecodedFrameCache> decodedFrameCache;
    ::datacodec::EncodedInputCachePolicy encodedInputCachePolicy;
    std::shared_ptr<::datacodec::IEncodedInputCache> encodedInputCache;
    std::shared_ptr<::datacodec::IRunRecordSink> runRecordSink;
    AttributeDataSourcePointer attributeDataSource;
    bool loadAllAvailableAttributes{true};
};

IGDCReader::Pointer IGDCReader::New() {
    return new IGDCReader;
}

IGDCReader::IGDCReader() : m_state(std::make_unique<State>()) {
    SetDecodeOptions(DataCodecIOSettings::GetDefaultDecodeOptions());
    SetLoadAllAvailableAttributes(
        DataCodecIOSettings::GetDefaultLoadAllAvailableAttributes());
}

IGDCReader::~IGDCReader() = default;

void IGDCReader::SetCodecControlParams(const ::datacodec::DecodeControlParams& params) {
    auto& state = *m_state;
    state.hasCodecParams = true;
    state.codecParams = params;
    state.configurationSource.customControlParams = true;
}

void IGDCReader::SetDecodeControls(
    const ::datacodec::DataCodecDecodeConfigurationParams& definition) {
    auto& state = *m_state;
    state.hasCodecParams = true;
    state.codecParams = definition.controlParams;
    state.executionOptions = definition.execution;
    state.decodedFrameCachePolicy = definition.decodedFrameCachePolicy;
    state.encodedInputCachePolicy = definition.encodedInputCachePolicy;
    state.configurationSource = definition.source;
}

void IGDCReader::SetDecodeTier(const ::datacodec::DataCodecDecodeTier tier) {
    SetDecodeOptions(::datacodec::DataCodecDecodeOptions{.tier = tier});
}

void IGDCReader::SetDecodeOptions(const ::datacodec::DataCodecDecodeOptions& options) {
    auto definition = ::datacodec::MakeDecodeConfigurationParams(options);
    SetDecodeControls(definition);
}

void IGDCReader::SetSelectedFramePaths(std::vector<std::string> framePaths) {
    m_state->selectedFramePaths = std::move(framePaths);
}

void IGDCReader::SetDecodedFrameCachePolicy(
        const ::datacodec::DecodedFrameCachePolicy& policy) {
    ::datacodec::AssertValidDecodedFrameCachePolicy(policy);
    m_state->decodedFrameCachePolicy = policy;
}

void IGDCReader::SetDecodedFrameCache(
        std::shared_ptr<::datacodec::IDecodedFrameCache> frameCache) {
    m_state->decodedFrameCache = std::move(frameCache);
}

void IGDCReader::SetEncodedInputCachePolicy(
        const ::datacodec::EncodedInputCachePolicy& policy) {
    ::datacodec::AssertValidEncodedInputCachePolicy(policy);
    m_state->encodedInputCachePolicy = policy;
}

void IGDCReader::SetEncodedInputCache(
        std::shared_ptr<::datacodec::IEncodedInputCache> inputCache) {
    m_state->encodedInputCache = std::move(inputCache);
}

void IGDCReader::SetLoadAllAvailableAttributes(const bool loadAllAvailableAttributes) {
    m_state->loadAllAvailableAttributes = loadAllAvailableAttributes;
}

void IGDCReader::SetRunRecordSink(
        std::shared_ptr<::datacodec::IRunRecordSink> sink) {
    m_state->runRecordSink = std::move(sink);
}

AttributeDataSourcePointer IGDCReader::GetAttributeDataSource() const {
    return m_state != nullptr ? m_state->attributeDataSource : AttributeDataSourcePointer{};
}

const std::vector<::datacodec::TelemetryMessageRecord>& IGDCReader::GetMessages() const {
    static const std::vector<::datacodec::TelemetryMessageRecord> empty;
    return m_state != nullptr ? m_state->messages : empty;
}

bool IGDCReader::Execute() {
    auto resetProgressUI = [this]() -> void {
        m_Progress = 0.0;
        m_ProgressShift = 0.0;
        m_ProgressScale = 1.0;
        if (m_ProgressObserver != nullptr) {
            m_ProgressObserver->UpdateProgress(0.0);
            m_ProgressObserver->UpdateText("");
        }
    };

    const auto start = std::chrono::steady_clock::now();
    if (!DecodeInput()) {
        for (const auto& message : m_state->messages) {
            IGAME_CORE_ERROR("{}", message.text);
        }
        IGAME_CORE_ERROR("Parsing failure");
        resetProgressUI();
        return false;
    }
    if (!CreateDataObject()) {
        auto message = MakeiGameRunMessage(
            ::datacodec::TelemetryMessageSeverity::Error,
            "IGDCReader",
            "DataCodec decoded output is unavailable");
        SubmitiGameRunMessage(m_state->runRecordSink.get(), message);
        m_state->messages.push_back(std::move(message));
        IGAME_CORE_ERROR("Generate DataObject failure");
        resetProgressUI();
        return false;
    }
    if (m_Output != nullptr) {
        m_Output->GetProperties()->AddProperty(Variant::LongLong, "FileSize")->SetValue(static_cast<long long>(m_FileSize));
    }
    this->SetOutput(0, m_Output);

    const auto end = std::chrono::steady_clock::now();
    igDebug(
        "Read file success! The time cost: {} ms",
        std::chrono::duration<double, std::milli>(end - start).count());
    return true;
}

bool IGDCReader::Parsing() {
    return DecodeInput();
}

bool IGDCReader::DecodeInput() {
    auto& state = *m_state;
    m_DecodedOutput = nullptr;
    state.messages.clear();
    state.attributeDataSource.reset();

    auto runRecordSink = MakeiGameRunRecordSink(
        state.runRecordSink,
        m_ProgressObserver != nullptr);
    const auto addError = [&state, &runRecordSink](std::string text) {
        auto message = MakeiGameRunMessage(
            ::datacodec::TelemetryMessageSeverity::Error,
            "IGDCReader",
            std::move(text));
        SubmitiGameRunMessage(runRecordSink.get(), message);
        state.messages.push_back(std::move(message));
    };
    std::shared_ptr<::datacodec::IByteRangeReader> inputReader;
    ::datacodec::PackageInspection packageInspection;
    if (!m_UseMemoryBuffer) {
        std::vector<std::filesystem::path> selectedPaths;
        if (state.selectedFramePaths.empty()) {
            selectedPaths.emplace_back(m_FilePath);
        } else {
            selectedPaths.reserve(state.selectedFramePaths.size());
            for (const auto& path: state.selectedFramePaths) { selectedPaths.emplace_back(path); }
        }
        inputReader = std::make_shared<iGameFileByteRangeReader>(selectedPaths.front());
        std::string inspectionError;
        if (!::datacodec::InspectPackage(*inputReader, packageInspection, &inspectionError)) {
            addError(std::move(inspectionError));
            return false;
        }
        if (packageInspection.format == ::datacodec::PackageBinaryFormat::FramePackage) {
            IGDCFrameSequence sequence;
            std::string sequenceError;
            if (!ResolveIGDCFrameSelection(selectedPaths, sequence, &sequenceError)) {
                addError(sequenceError.empty()
                    ? "failed to resolve selected frame package sequence"
                    : std::move(sequenceError));
                return false;
            }
            auto sequenceResult = DecodeFrameSequence({
                .decodeSources = sequence.decodeSources,
                .selectedFrameOrder = sequence.selectedFrameIndices,
                .targetFrameIndex = m_requestedFrameIndex.value_or(sequence.entryFrameIndex),
                .sourceLabel = sequence.entryPath.string(),
                .controlParams = state.hasCodecParams ? &state.codecParams : nullptr,
                .executionOptions = &state.executionOptions,
                .configurationSource = &state.configurationSource,
                .decodedFrameCachePolicy = state.decodedFrameCachePolicy,
                .decodedFrameCache = state.decodedFrameCache,
                .encodedInputCachePolicy = state.encodedInputCachePolicy,
                .encodedInputCache = state.encodedInputCache,
                .parallelTaskRunner = DataCodecTaskRunner(),
                .loadAllAvailableAttributes = state.loadAllAvailableAttributes,
                .runRecordSink = runRecordSink,
            });
            state.messages = std::move(sequenceResult.messages);
            if (!sequenceResult.success || sequenceResult.output == nullptr) { return false; }
            m_DecodedOutput = std::move(sequenceResult.output);
            state.attributeDataSource = std::move(sequenceResult.attributeDataSource);
            m_FileSize = 0u;
            for (const auto& path: sequence.selectedFramePaths) {
                std::error_code sizeError;
                const auto bytes = std::filesystem::file_size(path, sizeError);
                if (!sizeError) { m_FileSize += static_cast<std::size_t>(bytes); }
            }
            return true;
        }
        if (state.selectedFramePaths.size() > 1u) {
            addError("multiple selected paths require frame packages");
            return false;
        }
    }
    if (m_UseMemoryBuffer) {
        if (m_MemoryBuffer == nullptr || m_MemoryBufferSize == 0u) {
            addError("DataCodec memory input is empty");
            return false;
        }
        inputReader = std::make_shared<::datacodec::MemoryByteRangeReader>(
            std::span<const std::uint8_t>(
                reinterpret_cast<const std::uint8_t*>(m_MemoryBuffer),
                m_MemoryBufferSize));
        std::string headerError;
        if (!::datacodec::InspectPackage(*inputReader, packageInspection, &headerError)) {
            addError(headerError.empty() ? "版本不符合" : std::move(headerError));
            return false;
        }
    }
    auto inputSourceIdentity = packageInspection.sourceIdentity;
    const auto decodeResult = DecodeDataCodecDataObject({
        .inputReader = std::move(inputReader),
        .inputSourceIdentity = inputSourceIdentity,
        .controlParams = state.hasCodecParams ? &state.codecParams : nullptr,
        .executionOptions = &state.executionOptions,
        .configurationSource = &state.configurationSource,
        .decodedFrameCachePolicy = state.decodedFrameCachePolicy,
        .decodedFrameCache = state.decodedFrameCache,
        .encodedInputCachePolicy = state.encodedInputCachePolicy,
        .encodedInputCache = state.encodedInputCache,
        .executionResources = MakeDataCodecExecutionResources(),
        .requestedFrameIndex = m_requestedFrameIndex,
        .loadAllAvailableAttributes = state.loadAllAvailableAttributes,
        .runRecordSink = runRecordSink,
    });
    state.messages = decodeResult.messages;
    m_FileSize = static_cast<std::size_t>(decodeResult.inputBytes);
    if (!decodeResult.success || decodeResult.output == nullptr) {
        if (state.messages.empty()) {
            addError("DataCodec decode failed");
        }
        for (const auto& message : state.messages) {
            IGAME_CORE_ERROR("IGDCReader message: {}", message.text);
        }
        return false;
    }
    m_DecodedOutput = decodeResult.output;
    return true;
}

bool IGDCReader::CreateDataObject() {
    if (m_DecodedOutput != nullptr) {
        m_Output = m_DecodedOutput;
        return true;
    }
    return false;
}

IGAME_NAMESPACE_END
