#include "iGameIGDCReader.h"

#include "DataCodec/Filter/Execution/iGameDataCodecThreadPoolTaskRunner.h"
#include "DataCodec/Filter/Output/iGameDataCodecOutputBinding.h"
#include "DataCodec/Runtime/Record/RunRecordSubmit.h"
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
    ::datacodec::DataCodecOutputSinks outputSinks;
    std::shared_ptr<::datacodec::IRunRecordSink> telemetrySink;
    ::datacodec::DataCodecLanguage language{
        ::datacodec::DataCodecLanguage::SimplifiedChinese};
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
    state.language = definition.language;
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

void IGDCReader::SetOutputSinks(::datacodec::DataCodecOutputSinks sinks) {
    m_state->outputSinks = std::move(sinks);
}

void IGDCReader::SetTelemetrySink(
        std::shared_ptr<::datacodec::IRunRecordSink> sink) {
    m_state->telemetrySink = std::move(sink);
}

void IGDCReader::SetLanguage(const ::datacodec::DataCodecLanguage language) {
    m_state->language = language;
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
        resetProgressUI();
        return false;
    }
    if (!CreateDataObject()) {
        RecordMessage(iGameDataCodecHostMessageId::DecodedOutputUnavailable);
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

    iGameDataCodecOutputBinding outputBinding(
        state.outputSinks,
        state.telemetrySink,
        m_ProgressObserver != nullptr);
    const auto runRecordSink = outputBinding.RecordSink();
    const auto addStatus = [&state, &runRecordSink](
        ::datacodec::TelemetryMessageRecord message) {
        ::datacodec::SubmitRunMessage(runRecordSink.get(), message);
        state.messages.push_back(std::move(message));
    };
    const auto addHostError = [&addStatus, &state](
        const iGameDataCodecHostMessageId messageId,
        std::string technicalDetail) {
        ::datacodec::TelemetryMessageRecord message{
            .severity = ::datacodec::TelemetryMessageSeverity::Error,
            .origin = "IGDCReader",
            .language = state.language,
            .text = std::string(iGameDataCodecHostMessage(state.language, messageId)),
            .technicalDetail = std::move(technicalDetail),
        };
        addStatus(std::move(message));
    };
    const auto addCodecError = [&addStatus, &state](std::string technicalDetail) {
        if (technicalDetail.empty()) {
            technicalDetail = "DataCodec package inspection failed";
        }
        const auto localized = ::datacodec::LocalizeDataCodecMessage(
            state.language,
            ::datacodec::DataCodecMessageId::DecodeFailed,
            {},
            std::move(technicalDetail));
        addStatus(::datacodec::TelemetryMessageRecord{
            .severity = ::datacodec::TelemetryMessageSeverity::Error,
            .origin = "DataCodec",
            .language = localized.language,
            .messageId = localized.id,
            .messageArguments = localized.arguments,
            .text = localized.text,
            .technicalDetail = localized.technicalDetail,
        });
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
            addCodecError(std::move(inspectionError));
            return false;
        }
        if (packageInspection.format == ::datacodec::PackageBinaryFormat::FramePackage) {
            IGDCFrameSequence sequence;
            std::string sequenceError;
            if (!ResolveIGDCFrameSelection(selectedPaths, sequence, &sequenceError)) {
                addHostError(
                    iGameDataCodecHostMessageId::ResolveFrameSequenceFailed,
                    sequenceError.empty()
                        ? std::string{}
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
                .language = state.language,
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
            addHostError(
                iGameDataCodecHostMessageId::MultiplePathsRequireFramePackages,
                {});
            return false;
        }
    }
    if (m_UseMemoryBuffer) {
        if (m_MemoryBuffer == nullptr || m_MemoryBufferSize == 0u) {
            addHostError(iGameDataCodecHostMessageId::MemoryInputEmpty, {});
            return false;
        }
        inputReader = std::make_shared<::datacodec::MemoryByteRangeReader>(
            std::span<const std::uint8_t>(
                reinterpret_cast<const std::uint8_t*>(m_MemoryBuffer),
                m_MemoryBufferSize));
        std::string headerError;
        if (!::datacodec::InspectPackage(*inputReader, packageInspection, &headerError)) {
            addCodecError(headerError.empty() ? "package header validation failed" : std::move(headerError));
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
        .language = state.language,
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
            addHostError(iGameDataCodecHostMessageId::DecodeFailed, {});
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

void IGDCReader::RecordMessage(
    const iGameDataCodecHostMessageId messageId,
    std::string technicalDetail) {
    const auto text = std::string(iGameDataCodecHostMessage(
        m_state->language,
        messageId));
    if (text.empty()) {
        return;
    }
    auto outputBinding = iGameDataCodecOutputBinding(
        m_state->outputSinks,
        m_state->telemetrySink,
        false);
    ::datacodec::TelemetryMessageRecord message{
        .severity = ::datacodec::TelemetryMessageSeverity::Error,
        .origin = "IGDCReader",
        .language = m_state->language,
        .text = text,
        .technicalDetail = std::move(technicalDetail),
    };
    ::datacodec::SubmitRunMessage(outputBinding.RecordSink().get(), message);
    m_state->messages.push_back(std::move(message));
}

IGAME_NAMESPACE_END
