#ifndef iGameIGDCWriter_h
#define iGameIGDCWriter_h

#include "DataCodec/API/Entry/DataCodecEncodeEntry.h"
#include "DataCodec/Filter/Localization/iGameDataCodecHostMessage.h"
#include "iGameFileWriter.h"

#include <filesystem>
#include <memory>
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
        m_language = definition.language;
    }

    void SetAttributeTargets(std::vector<::datacodec::AttributeTarget> targets) {
        m_hasAttributeTargets = true;
        m_attributeTargets = std::move(targets);
    }

    void ClearAttributeTargets() {
        m_hasAttributeTargets = false;
        m_attributeTargets.clear();
    }

    void SetOutputSinks(::datacodec::DataCodecOutputSinks sinks) noexcept {
        m_outputSinks = std::move(sinks);
    }

    void SetTelemetrySink(std::shared_ptr<::datacodec::IRunRecordSink> sink) noexcept {
        m_telemetrySink = std::move(sink);
    }

    [[nodiscard]] const std::vector<std::string>& GetWrittenFilePaths() const noexcept {
        return m_writtenFilePaths;
    }

    void SetEncodeTier(const ::datacodec::DataCodecEncodeTier tier) {
        SetEncodeOptions(::datacodec::DataCodecEncodeOptions{.tier = tier});
    }

    void SetEncodeOptions(const ::datacodec::DataCodecEncodeOptions& options) {
        SetEncodeControls(::datacodec::MakeEncodeConfigurationParams(options));
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
    ::datacodec::DataCodecLanguage m_language{
        ::datacodec::DataCodecLanguage::SimplifiedChinese};
    std::vector<::datacodec::AttributeTarget> m_attributeTargets;
    ::datacodec::DataCodecOutputSinks m_outputSinks;
    std::shared_ptr<::datacodec::IRunRecordSink> m_telemetrySink;
    std::vector<std::string> m_writtenFilePaths;

    ::datacodec::EncodePackageKind ResolveEncodePackageKind(const DataObject::Pointer& rootObject) const;
    bool EncodeToFile(::datacodec::EncodePackageKind packageKind);
    bool EncodeFrameSequence(const std::filesystem::path& outputHint);
    void RecordMessage(
        iGameDataCodecHostMessageId messageId,
        std::string technicalDetail = {});
};

IGAME_NAMESPACE_END
#endif
