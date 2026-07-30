#include "iGameIGDCWriter.h"
#include "DataCodec/Filter/Execution/iGameDataCodecThreadPoolTaskRunner.h"
#include "DataCodec/Filter/Output/iGameDataCodecOutputBinding.h"
#include "DataCodec/Runtime/Record/RunRecordSubmit.h"

#include "DataCodec/Filter/Adapter/iGameBlockTreeAdapter.h"
#include "DataCodec/Filter/Adapter/iGameDataCodecAttributeCatalog.h"
#include "DataCodec/Filter/Adapter/iGameEncodeAdapter.h"
#include "DataCodec/Workflow/FrameSequence/FrameSequenceEncodeExecutor.h"
#include "DataCodec/Filter/Adapter/iGameFileByteRangeIO.h"
#include "iGameIGDCFrameSequence.h"
#include "iGameStreamingData.h"

#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <exception>
#include <new>
#include <filesystem>
#include <memory>
#include <set>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

IGAME_NAMESPACE_BEGIN

namespace {

std::uint64_t FileSizeOrZero(const std::filesystem::path& path) {
    std::error_code errorCode;
    const auto size = std::filesystem::file_size(path, errorCode);
    return errorCode ? 0u : static_cast<std::uint64_t>(size);
}

std::vector<std::filesystem::path> FindExistingFrameSequenceFiles(
        const std::filesystem::path& outputHint) {
    std::vector<std::filesystem::path> paths;
    auto directory = outputHint.parent_path();
    if (directory.empty()) { directory = std::filesystem::current_path(); }
    const auto seriesStem = ResolveIGDCFrameSequenceStem(outputHint);
    std::error_code directoryError;
    for (const auto& entry: std::filesystem::directory_iterator(directory, directoryError)) {
        if (directoryError || !entry.is_regular_file()) { continue; }
        ::datacodec::FramePackagePathInfo candidate;
        if (!::datacodec::ParseFramePackagePath(entry.path(), candidate, nullptr) ||
            !candidate.hasFrameIndex ||
            candidate.seriesStem != seriesStem) {
            continue;
        }
        paths.push_back(entry.path());
    }
    return paths;
}

class ScopedStreamingDataCachePause {
public:
    explicit ScopedStreamingDataCachePause(StreamingData* timeFrames)
        : m_timeFrames(timeFrames) {
        if (m_timeFrames == nullptr) {
            return;
        }
        m_wasCacheEnabled = m_timeFrames->IsCacheEnabled();
        m_maxCacheSize = m_timeFrames->GetMaxCacheSize();
        if (m_wasCacheEnabled) {
            // 多帧压缩逐帧读取源数据，不复用播放器的完整帧缓存
            m_timeFrames->DisableCache();
        }
    }

    ~ScopedStreamingDataCachePause() {
        if (m_timeFrames == nullptr || !m_wasCacheEnabled) {
            return;
        }
        m_timeFrames->EnableCache(m_maxCacheSize);
    }

    ScopedStreamingDataCachePause(const ScopedStreamingDataCachePause&) = delete;
    ScopedStreamingDataCachePause& operator=(const ScopedStreamingDataCachePause&) = delete;

private:
    StreamingData* m_timeFrames{nullptr};
    bool m_wasCacheEnabled{false};
    unsigned int m_maxCacheSize{0};
};

class IGDCFrameSequenceEncodeSource final : public ::datacodec::IFrameSequenceEncodeSource {
public:
    IGDCFrameSequenceEncodeSource(
        DataObject::Pointer rootObject,
        StreamingData::Pointer timeFrames,
        const bool hasAttributeTargets,
        std::vector<::datacodec::AttributeTarget> attributeTargets)
        : m_rootObject(std::move(rootObject)),
          m_timeFrames(std::move(timeFrames)),
          m_hasAttributeTargets(hasAttributeTargets),
          m_attributeTargets(std::move(attributeTargets)) {}

    [[nodiscard]] std::size_t FrameCount() const noexcept override {
        return m_timeFrames != nullptr ? m_timeFrames->GetTimeNum() : 0u;
    }

    bool LoadFrame(
        const std::size_t frameOrdinal,
        ::datacodec::FrameSequenceEncodeFrame& frame,
        std::string* error) override {
        frame = {};
        auto frameRoot = BuildDataCodecTimeFrameRoot(m_rootObject, frameOrdinal);
        if (frameRoot == nullptr ||
            (!CanCreateiGameEncodeAdapter(frameRoot) && !frameRoot->HasSubDataObject())) {
            return AssignError(error, "failed to load frame sequence input");
        }

        auto frameAdapter = std::make_unique<iGameBlockTreeAdapter>(frameRoot);
        std::vector<DataCodecEncodeAttributeDescriptor> descriptors;
        if (!CollectDataCodecEncodeAttributeDescriptors(
                nullptr,
                frameAdapter.get(),
                static_cast<std::uint32_t>(frameOrdinal),
                descriptors)) {
            return AssignError(error, "failed to enumerate frame sequence attributes");
        }
        if (!ResolveAttributeSelection(descriptors, error)) {
            return false;
        }

        frame.rootName = frameRoot->GetName();
        frame.frameIndex = static_cast<std::uint32_t>(frameOrdinal);
        frame.timeValue = m_timeFrames->GetTargetTimeValue(
            static_cast<unsigned int>(frameOrdinal));
        for (const auto& descriptor : descriptors) {
            if (!m_hasAttributeTargets || m_selectedAttributeNames.contains(descriptor.name)) {
                frame.attributeTargets.push_back(descriptor.target);
            }
        }
        frame.blockTreeAdapter = std::move(frameAdapter);
        return true;
    }

private:
    bool ResolveAttributeSelection(
        const std::vector<DataCodecEncodeAttributeDescriptor>& frameDescriptors,
        std::string* error) {
        if (m_selectionResolved || !m_hasAttributeTargets) {
            return true;
        }
        m_selectionResolved = true;
        if (m_attributeTargets.empty()) {
            return true;
        }

        std::vector<DataCodecEncodeAttributeDescriptor> representativeDescriptors;
        if (CollectDataCodecEncodeRepresentativeAttributeCatalog(
                m_rootObject,
                representativeDescriptors)) {
            for (const auto& descriptor : representativeDescriptors) {
                const bool selected = std::any_of(
                    m_attributeTargets.begin(),
                    m_attributeTargets.end(),
                    [&](const auto& target) {
                        return target.frameIndex == descriptor.target.frameIndex &&
                            target.blockPath == descriptor.target.blockPath &&
                            target.attrIndex == descriptor.target.attrIndex;
                    });
                if (selected) {
                    m_selectedAttributeNames.insert(descriptor.name);
                }
            }
        }
        if (m_selectedAttributeNames.empty()) {
            for (const auto& descriptor : frameDescriptors) {
                const bool selected = std::any_of(
                    m_attributeTargets.begin(),
                    m_attributeTargets.end(),
                    [&](const auto& target) {
                        return target.blockPath == descriptor.target.blockPath &&
                            target.attrIndex == descriptor.target.attrIndex;
                    });
                if (selected) {
                    m_selectedAttributeNames.insert(descriptor.name);
                }
            }
        }
        return !m_selectedAttributeNames.empty() ||
            AssignError(error, "failed to resolve selected frame sequence attributes");
    }

    static bool AssignError(std::string* error, std::string text) {
        if (error != nullptr) {
            *error = std::move(text);
        }
        return false;
    }

    DataObject::Pointer m_rootObject;
    StreamingData::Pointer m_timeFrames;
    bool m_hasAttributeTargets{false};
    bool m_selectionResolved{false};
    std::vector<::datacodec::AttributeTarget> m_attributeTargets;
    std::set<std::string> m_selectedAttributeNames;
};

class IGDCFrameSequenceOutputSink final : public ::datacodec::IFrameSequenceOutputSink {
public:
    explicit IGDCFrameSequenceOutputSink(std::filesystem::path outputHint)
        : m_outputHint(std::move(outputHint)),
          m_existingPaths(FindExistingFrameSequenceFiles(m_outputHint)) {}

    [[nodiscard]] std::unique_ptr<::datacodec::IByteRangeOutput> OpenFrame(
        const std::size_t frameOrdinal,
        const std::uint32_t frameIndex,
        std::string*) override {
        (void)frameIndex;
        const auto path = BuildIGDCFrameSequencePath(
            m_outputHint,
            static_cast<std::uint32_t>(frameOrdinal));
        m_attemptedPaths.push_back(path);
        return std::make_unique<iGameFileByteRangeOutput>(path);
    }

    bool CommitFrame(
        const std::size_t frameOrdinal,
        const std::uint32_t,
        const std::uint64_t encodedByteCount,
        std::string* error) override {
        if (frameOrdinal >= m_attemptedPaths.size() || encodedByteCount == 0u) {
            if (error != nullptr) {
                *error = "frame sequence output is empty or unavailable";
            }
            return false;
        }
        m_writtenPaths.push_back(m_attemptedPaths[frameOrdinal]);
        m_totalBytes += FileSizeOrZero(m_attemptedPaths[frameOrdinal]);
        return true;
    }

    void AbortSequence() noexcept override {
        std::error_code errorCode;
        for (const auto& path : m_attemptedPaths) {
            std::filesystem::remove(path, errorCode);
        }
        m_writtenPaths.clear();
        m_totalBytes = 0u;
    }

    void Complete() {
        std::error_code errorCode;
        for (const auto& oldPath : m_existingPaths) {
            const bool retained = std::any_of(
                m_writtenPaths.begin(),
                m_writtenPaths.end(),
                [&](const auto& writtenPath) {
                    return writtenPath.filename() == oldPath.filename();
                });
            if (!retained) {
                std::filesystem::remove(oldPath, errorCode);
            }
        }
    }

    [[nodiscard]] const std::vector<std::filesystem::path>& WrittenPaths() const noexcept {
        return m_writtenPaths;
    }

    [[nodiscard]] std::uint64_t TotalBytes() const noexcept {
        return m_totalBytes;
    }

private:
    std::filesystem::path m_outputHint;
    std::vector<std::filesystem::path> m_existingPaths;
    std::vector<std::filesystem::path> m_attemptedPaths;
    std::vector<std::filesystem::path> m_writtenPaths;
    std::uint64_t m_totalBytes{0u};
};

} // namespace

bool IGDCWriter::Execute()
{
    m_writtenFilePaths.clear();
    this->m_DataObject = this->m_Inputs->GetElement(0);
    if (!m_DataObject) {
        igDebug("could not write nullptr object!");
        RecordMessage(iGameDataCodecHostMessageId::EncodeRequiresInputObject);
        return false;
    }

    return EncodeToFile(ResolveEncodePackageKind(m_DataObject));
}

bool IGDCWriter::GenerateBuffers()
{
    m_writtenFilePaths.clear();
    if (!m_DataObject) {
        RecordMessage(iGameDataCodecHostMessageId::EncodeRequiresInputObject);
        return false;
    }

    return EncodeToFile(ResolveEncodePackageKind(m_DataObject));
}

::datacodec::EncodePackageKind IGDCWriter::ResolveEncodePackageKind(const DataObject::Pointer& rootObject) const
{
    return ResolveDataCodecEncodePackageKind(rootObject);
}

bool IGDCWriter::EncodeToFile(const ::datacodec::EncodePackageKind packageKind)
{
    const auto outputSinks = ResolveiGameDataCodecOutputSinks(
        m_outputSinks,
        m_ProgressObserver != nullptr);
    auto outputPath = std::filesystem::path(m_FilePath);
    outputPath.replace_extension(".igc");
    const auto timeFrames = m_DataObject != nullptr ? m_DataObject->PeekTimeFrames() : nullptr;
    if (timeFrames != nullptr && timeFrames->GetTimeNum() > 1u) {
        return EncodeFrameSequence(outputPath);
    }

    std::unique_ptr<iGameEncodeAdapter> leafAdapter;
    std::unique_ptr<iGameBlockTreeAdapter> blockTreeAdapter;
    ::datacodec::EncodeInput encodeInput;
    if (packageKind == ::datacodec::EncodePackageKind::LeafPackage) {
        if (!CanCreateiGameEncodeAdapter(m_DataObject)) {
            RecordMessage(iGameDataCodecHostMessageId::EncodeRequiresSupportedLeafObject);
            return false;
        }
        leafAdapter = std::make_unique<iGameEncodeAdapter>(m_DataObject);
        encodeInput = ::datacodec::EncodeInput::LeafAdapter(leafAdapter.get());
    } else if (packageKind == ::datacodec::EncodePackageKind::FramePackage) {
        blockTreeAdapter = std::make_unique<iGameBlockTreeAdapter>(m_DataObject);
        encodeInput = ::datacodec::EncodeInput::BlockTreeAdapter(
            blockTreeAdapter.get(),
            m_DataObject != nullptr ? m_DataObject->GetName() : std::string{});
    }

    iGameFileByteRangeOutput outputSink(outputPath);
    auto controlParams = m_hasCodecParams
        ? m_CodecParams
        : ::datacodec::MakeDefaultEncodeControlParams();
    std::vector<::datacodec::AttributeTarget> attributeTargets = m_hasAttributeTargets
        ? m_attributeTargets
        : std::vector<::datacodec::AttributeTarget>{};
    ::datacodec::DataCodecEncodeConfigurationParams configuration{
        .controlParams = std::move(controlParams),
        .pipelineControl = m_pipelineControl,
        .execution = m_execution,
        .source = m_configurationSource,
        .language = m_language,
    };
    auto encodeResult = ::datacodec::Encode({
        .input = std::move(encodeInput),
        .output = ::datacodec::EncodeOutput::ByteRange(outputSink, packageKind),
        .attributeSelection = m_hasAttributeTargets
            ? ::datacodec::AttributeSelectionMode::Explicit
            : ::datacodec::AttributeSelectionMode::AllAvailable,
        .attributeTargets = std::move(attributeTargets),
        .configuration = std::move(configuration),
        .outputSinks = outputSinks,
        .runRecordSink = m_telemetrySink,
        .executionResources = MakeDataCodecExecutionResources(),
    });

    const bool ok = encodeResult.success && encodeResult.hasEncodedOutput;
    if (!ok) {
        RecordMessage(iGameDataCodecHostMessageId::EncodeFailed);
        std::error_code errorCode;
        std::filesystem::remove(outputPath, errorCode);
        return false;
    }

    m_FilePath = outputPath.string();
    m_writtenFilePaths = {m_FilePath};
    m_FileSize = static_cast<std::size_t>(FileSizeOrZero(outputPath));
    m_Buffers.clear();
    if (m_FileSize == 0u) {
        RecordMessage(iGameDataCodecHostMessageId::EncodeEmptyOutput);
        return false;
    }
    return true;
}

bool IGDCWriter::EncodeFrameSequence(const std::filesystem::path& outputHint)
{
    iGameDataCodecOutputBinding outputBinding(
        m_outputSinks,
        m_telemetrySink,
        m_ProgressObserver != nullptr);
    const auto timeFrames = m_DataObject != nullptr ? m_DataObject->PeekTimeFrames() : nullptr;
    if (timeFrames == nullptr || timeFrames->GetTimeNum() <= 1u) {
        RecordMessage(iGameDataCodecHostMessageId::FrameSequenceRequiresMultipleFrames);
        return false;
    }

    auto controlParams = m_hasCodecParams
        ? m_CodecParams
        : ::datacodec::MakeDefaultEncodeControlParams();
    ScopedStreamingDataCachePause streamingCachePause(timeFrames.get());
    IGDCFrameSequenceEncodeSource source(
        m_DataObject,
        timeFrames,
        m_hasAttributeTargets,
        m_attributeTargets);
    IGDCFrameSequenceOutputSink outputSink(outputHint);

    auto result = ::datacodec::FrameSequenceEncodeExecutor::Execute({
        .source = &source,
        .outputSink = &outputSink,
        .controlParams = &controlParams,
        .pipelineControl = m_pipelineControl,
        .configurationSource = m_configurationSource,
        .language = m_language,
        .runRecordSink = outputBinding.RecordSink().get(),
        .enableParallelStages = m_execution.enableParallelStages,
        .parallelTaskRunner = DataCodecTaskRunner().get(),
    });
    if (!result.success) {
        RecordMessage(iGameDataCodecHostMessageId::FrameSequenceEncodeFailed);
        return false;
    }

    outputSink.Complete();
    m_writtenFilePaths.clear();
    for (const auto& path : outputSink.WrittenPaths()) {
        m_writtenFilePaths.push_back(path.string());
    }
    if (m_writtenFilePaths.empty()) {
        RecordMessage(iGameDataCodecHostMessageId::FrameSequenceNoOutputFiles);
        return false;
    }
    m_FilePath = m_writtenFilePaths.front();
    m_FileSize = static_cast<std::size_t>(outputSink.TotalBytes());
    m_Buffers.clear();
    if (m_FileSize == 0u) {
        RecordMessage(iGameDataCodecHostMessageId::FrameSequenceEmptyOutputFiles);
        return false;
    }
    return true;
}

void IGDCWriter::RecordMessage(
    const iGameDataCodecHostMessageId messageId,
    std::string technicalDetail)
{
    const auto text = std::string(iGameDataCodecHostMessage(m_language, messageId));
    if (text.empty()) {
        return;
    }
    auto outputSinks = ResolveiGameDataCodecOutputSinks(
        m_outputSinks,
        false);
    const ::datacodec::DataCodecStatusRecord status{
        .severity = ::datacodec::DataCodecStatusSeverity::Error,
        .language = m_language,
        .text = text,
        .technicalDetail = technicalDetail,
    };
    if (outputSinks.ui != nullptr) {
        outputSinks.ui->SubmitUiStatus(status);
    }
    if (outputSinks.console != nullptr) {
        outputSinks.console->SubmitConsoleStatus(status);
    }
    ::datacodec::SubmitRunMessage(
        m_telemetrySink.get(),
        ::datacodec::TelemetryMessageRecord{
            .severity = ::datacodec::TelemetryMessageSeverity::Error,
            .origin = "IGDCWriter",
            .language = m_language,
            .text = text,
            .technicalDetail = std::move(technicalDetail),
        });
}

IGAME_NAMESPACE_END
