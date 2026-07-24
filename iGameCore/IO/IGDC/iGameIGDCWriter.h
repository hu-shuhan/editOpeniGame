#ifndef iGameIGDCWriter_h
#define iGameIGDCWriter_h

#include "DataCodec/API/Entry/DataCodecEncodeEntry.h"
#include "iGameFileWriter.h"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

IGAME_NAMESPACE_BEGIN

class IGDCWriter : public FileWriter {
public:
    I_OBJECT(IGDCWriter);
    static Pointer New() { return new IGDCWriter; }

    bool Execute() override;
    bool GenerateBuffers() override;

    void SetEncodeControls(const ::datacodec::DataCodecEncodeConfigurationParams& definition) {
        m_hasCodecParams = true;
        m_CodecParams = definition.controlParams;
        m_pipelineControl = definition.pipelineControl;
        m_execution = definition.execution;
        m_configurationSource = definition.source;
    }

    void SetAttributeTargets(std::vector<::datacodec::AttributeTarget> targets) {
        m_hasAttributeTargets = true;
        m_attributeTargets = std::move(targets);
    }

    void ClearAttributeTargets() {
        m_hasAttributeTargets = false;
        m_attributeTargets.clear();
    }

    void SetProgressReporter(std::shared_ptr<::datacodec::IProgressReporter> reporter) noexcept {
        m_progressReporter = std::move(reporter);
    }

    [[nodiscard]] const std::vector<std::string>& GetWrittenFilePaths() const noexcept {
        return m_writtenFilePaths;
    }

    void SetEncodeLogSink(::datacodec::IEncodeLogSink* sink) noexcept {
        m_encodeLogSink = sink;
    }

    void SetLogSink(std::shared_ptr<::datacodec::ILogSink> sink) noexcept {
        m_logSink = std::move(sink);
    }

    void SetEncodeTier(const ::datacodec::DataCodecEncodeTier tier) {
        SetEncodeOptions(::datacodec::DataCodecEncodeOptions{.tier = tier});
    }

    void SetEncodeOptions(const ::datacodec::DataCodecEncodeOptions& options) {
        SetEncodeControls(::datacodec::MakeEncodeConfigurationParams(options));
    }

    const std::vector<::datacodec::TelemetryMessageRecord>& GetMessages() const {
        return m_messages;
    }

    const std::optional<::datacodec::TelemetrySession>& GetTelemetrySession() const {
        return m_telemetrySession;
    }

    const std::optional<::datacodec::UiTelemetrySnapshot>& GetUiTelemetrySnapshot() const {
        return m_uiTelemetrySnapshot;
    }

protected:
    IGDCWriter() = default;
    ~IGDCWriter() override = default;

private:
    bool m_hasCodecParams = false;
    bool m_hasAttributeTargets{false};
    ::datacodec::CodecControlParams m_CodecParams;
    ::datacodec::EncodePipelineControlParams m_pipelineControl;
    ::datacodec::EncodeExecutionOptions m_execution{::datacodec::MakeDefaultEncodeExecutionOptions()};
    ::datacodec::DataCodecEncodeConfigurationSource m_configurationSource;
    std::vector<::datacodec::AttributeTarget> m_attributeTargets;
    std::shared_ptr<::datacodec::IProgressReporter> m_progressReporter;
    ::datacodec::IEncodeLogSink* m_encodeLogSink{nullptr};
    std::shared_ptr<::datacodec::ILogSink> m_logSink;

    std::vector<::datacodec::TelemetryMessageRecord> m_messages;
    std::optional<::datacodec::TelemetrySession> m_telemetrySession;
    std::optional<::datacodec::UiTelemetrySnapshot> m_uiTelemetrySnapshot;
    std::vector<std::string> m_writtenFilePaths;

    ::datacodec::EncodePackageKind ResolveEncodePackageKind(const DataObject::Pointer& rootObject) const;
    bool EncodeToFile(::datacodec::EncodePackageKind packageKind);
    bool EncodeFrameSequence(const std::filesystem::path& outputHint);
    void CaptureEncodeResult(::datacodec::EncodeResult&& result);
    void RecordMessage(std::string text);
};

IGAME_NAMESPACE_END
#endif
