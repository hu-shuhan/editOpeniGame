#ifndef iGameIGDCWriter_h
#define iGameIGDCWriter_h

#include "DataCodec/API/Entry/DataCodecEncodeEntry.h"
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
    }

    void SetAttributeTargets(std::vector<::datacodec::AttributeTarget> targets) {
        m_hasAttributeTargets = true;
        m_attributeTargets = std::move(targets);
    }

    void ClearAttributeTargets() {
        m_hasAttributeTargets = false;
        m_attributeTargets.clear();
    }

    void SetRunRecordSink(std::shared_ptr<::datacodec::IRunRecordSink> sink) noexcept {
        m_runRecordSink = std::move(sink);
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
    std::vector<::datacodec::AttributeTarget> m_attributeTargets;
    std::shared_ptr<::datacodec::IRunRecordSink> m_runRecordSink;
    std::vector<std::string> m_writtenFilePaths;

    ::datacodec::EncodePackageKind ResolveEncodePackageKind(const DataObject::Pointer& rootObject) const;
    bool EncodeToFile(::datacodec::EncodePackageKind packageKind);
    bool EncodeFrameSequence(const std::filesystem::path& outputHint);
    void RecordMessage(std::string text);
};

IGAME_NAMESPACE_END
#endif
