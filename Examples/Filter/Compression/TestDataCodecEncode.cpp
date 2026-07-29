#include <IGDC/iGameIGDCWriter.h>
#include <iGameFileIO.h>
#include <DataCodec/Codec/SubCodec/ZstdCodec.h>
#include <DataCodec/Codec/Topology/Connectivity/ConnectivityTopologyCodec.h>
#include <DataCodec/Filter/Adapter/iGameFileByteRangeIO.h>
#include <DataCodec/Filter/Execution/iGameRunRecordSink.h>
#include <DataCodec/Storage/LeafPackage/LeafPackageIO.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace {

constexpr const char* kSourceFile = "./Models/Tet_Plane.vtk";
constexpr const char* kEncodedFile = "./Models/DataCodec/Tet_Plane.igc";

std::optional<::datacodec::DataCodecEncodeTier> ParseTier(const std::string& value) {
    if (value == "time") return ::datacodec::DataCodecEncodeTier::TimePriority;
    if (value == "balanced") return ::datacodec::DataCodecEncodeTier::Balanced;
    if (value == "memory") return ::datacodec::DataCodecEncodeTier::MemoryPriority;
    return std::nullopt;
}

void PrintMessages(const std::vector<::datacodec::TelemetryMessageRecord>& messages) {
    for (const auto& message : messages) {
        std::cerr << message.text << '\n';
    }
}

class CountingByteWriter final : public ::datacodec::bytestore::IByteWriter {
public:
    bool Write(
        const std::span<const std::uint8_t> bytes,
        std::string* error = nullptr) override {
        if (bytes.size() > std::numeric_limits<std::uint64_t>::max() - m_byteCount) {
            if (error != nullptr) {
                *error = "zstd audit output size overflow";
            }
            return false;
        }
        m_byteCount += static_cast<std::uint64_t>(bytes.size());
        return true;
    }

    [[nodiscard]] std::uint64_t ByteSizeHint() const noexcept override {
        return m_byteCount;
    }

private:
    std::uint64_t m_byteCount{0u};
};

struct AttributeZstdAuditProfile {
    const char* name;
    int level;
    std::size_t workerCount;
};

bool AuditAttributeZstd(const std::filesystem::path& encodedFile) {
    auto reader = std::make_shared<iGame::iGameFileByteRangeReader>(encodedFile);
    ::datacodec::LeafPackage leafPackage;
    std::string error;
    if (!::datacodec::LeafPackageIO::ReadFromByteRange(
            reader,
            0u,
            reader->ByteSize(),
            leafPackage,
            &error)) {
        std::cerr << "failed to read raw leaf package for attribute zstd audit: " << error << '\n';
        return false;
    }

    const auto fieldIt = std::find_if(
        leafPackage.fields.begin(),
        leafPackage.fields.end(),
        [](const auto& field) {
            return field.type == ::datacodec::FieldType::Attribute;
        });
    if (fieldIt == leafPackage.fields.end() || fieldIt->source == nullptr) {
        std::cerr << "raw leaf package has no attribute field for zstd audit\n";
        return false;
    }
    if (fieldIt->compressionType != ::datacodec::EncodedFieldCompressionType::None) {
        std::cerr << "attribute zstd audit requires an uncompressed package field\n";
        return false;
    }

    const auto rawBytes = fieldIt->source->ByteSizeHint();
    if (::datacodec::bytestore::IsUnknownByteSize(rawBytes)) {
        std::cerr << "attribute zstd audit source size is unknown\n";
        return false;
    }
    if (rawBytes == 0u) {
        std::cout << "DataCodec attr zstd audit: raw_bytes=0\n";
        return true;
    }

    constexpr std::size_t kWindowBytes = 8u * 1024u * 1024u;
    constexpr std::size_t kProbeBytes = 1u * 1024u * 1024u;
    constexpr std::array<AttributeZstdAuditProfile, 3u> kProfiles{{
        {"TimePriority", 1, 8u},
        {"Balanced", 3, 4u},
        {"MemoryPriority", 1, 1u},
    }};
    constexpr std::array<std::uint64_t, 5u> kProbeNumerators{{0u, 1u, 2u, 3u, 4u}};

    std::vector<std::uint8_t> window(kWindowBytes, 0u);
    for (const auto& profile : kProfiles) {
        ::datacodec::codec::ZstdStreamingEncoder encoder;
        CountingByteWriter output;
        if (!encoder.Initialize(profile.level, profile.workerCount, rawBytes, &error)) {
            std::cerr << "failed to initialize attribute zstd audit: " << error << '\n';
            return false;
        }

        const auto begin = std::chrono::steady_clock::now();
        std::uint64_t offset = 0u;
        while (offset < rawBytes) {
            const auto chunkBytes = static_cast<std::size_t>(std::min<std::uint64_t>(
                static_cast<std::uint64_t>(window.size()),
                rawBytes - offset));
            const auto chunk = std::span<std::uint8_t>(window.data(), chunkBytes);
            if (!fieldIt->source->Read(offset, chunk, &error) ||
                !encoder.Compress(
                    std::span<const std::uint8_t>(chunk.data(), chunk.size()),
                    output,
                    &error)) {
                std::cerr << "failed to stream attribute zstd audit: " << error << '\n';
                return false;
            }
            offset += static_cast<std::uint64_t>(chunkBytes);
        }
        if (!encoder.Finish(output, &error)) {
            std::cerr << "failed to finish attribute zstd audit: " << error << '\n';
            return false;
        }
        const auto end = std::chrono::steady_clock::now();
        const auto elapsedMs = std::chrono::duration<double, std::milli>(end - begin).count();
        const auto savingPercent = 100.0 *
            (1.0 - static_cast<double>(output.ByteSizeHint()) / static_cast<double>(rawBytes));
        std::cout << std::fixed << std::setprecision(4)
                  << "DataCodec attr zstd full: profile=" << profile.name
                  << "; level=" << profile.level
                  << "; workers=" << profile.workerCount
                  << "; raw_bytes=" << rawBytes
                  << "; encoded_bytes=" << output.ByteSizeHint()
                  << "; saving_percent=" << savingPercent
                  << "; elapsed_ms=" << elapsedMs << '\n';

        const auto probeBytes = static_cast<std::size_t>(std::min<std::uint64_t>(
            rawBytes,
            static_cast<std::uint64_t>(kProbeBytes)));
        std::vector<std::uint8_t> probe(probeBytes, 0u);
        std::vector<std::uint8_t> compressedProbe;
        const auto maxProbeOffset = rawBytes - static_cast<std::uint64_t>(probeBytes);
        for (const auto numerator : kProbeNumerators) {
            const auto probeOffset = maxProbeOffset * numerator / 4u;
            if (!fieldIt->source->Read(
                    probeOffset,
                    std::span<std::uint8_t>(probe.data(), probe.size()),
                    &error) ||
                !::datacodec::codec::ZstdCodec::Compress(
                    std::span<const std::uint8_t>(probe.data(), probe.size()),
                    profile.level,
                    1u,
                    compressedProbe,
                    &error)) {
                std::cerr << "failed to run attribute zstd probe audit: " << error << '\n';
                return false;
            }
            const auto probeSavingPercent = 100.0 *
                (1.0 - static_cast<double>(compressedProbe.size()) /
                    static_cast<double>(probeBytes));
            std::cout << std::fixed << std::setprecision(4)
                      << "DataCodec attr zstd probe: profile=" << profile.name
                      << "; position_quarter=" << numerator
                      << "; raw_bytes=" << probeBytes
                      << "; encoded_bytes=" << compressedProbe.size()
                      << "; saving_percent=" << probeSavingPercent << '\n';
        }
    }
    return true;
}

} // 匿名命名空间

int main(const int argc, char** argv) {
    if (argc > 9) {
        std::cerr << "usage: testDataCodecEncode [source] [output] [time|balanced|memory] [compression-enhancement] [monitor] [raw-package] [audit-attr-zstd] [cell-remap]\n";
        return 2;
    }
    const std::filesystem::path sourceFile = argc >= 2 ? argv[1] : kSourceFile;
    const std::filesystem::path encodedFile = argc >= 3 ? argv[2] : kEncodedFile;
    const auto tier = argc >= 4
        ? ParseTier(argv[3])
        : std::optional<::datacodec::DataCodecEncodeTier>{
            ::datacodec::DataCodecEncodeTier::Balanced};
    if (!tier.has_value()) {
        std::cerr << "unknown DataCodec encode tier: " << argv[3] << '\n';
        return 2;
    }
    bool monitor = false;
    bool compressionEnhancement = false;
    bool rawPackage = false;
    bool auditAttributeZstd = false;
    bool forceCellRemap = false;
    for (int index = 4; index < argc; ++index) {
        const std::string option = argv[index];
        if (option == "monitor") {
            monitor = true;
        } else if (option == "compression-enhancement") {
            compressionEnhancement = true;
        } else if (option == "raw-package") {
            rawPackage = true;
        } else if (option == "audit-attr-zstd") {
            auditAttributeZstd = true;
            rawPackage = true;
        } else if (option == "cell-remap") {
            forceCellRemap = true;
        } else {
            std::cerr << "unknown DataCodec encode option: " << option << '\n';
            return 2;
        }
    }

    const auto loadStart = std::chrono::steady_clock::now();
    auto object = iGame::FileIO::ReadFile(sourceFile.string());
    const auto loadEnd = std::chrono::steady_clock::now();
    if (object == nullptr) {
        std::cerr << "failed to read DataCodec source file: " << sourceFile.string() << '\n';
        return 1;
    }

    auto currentEncodedFile = encodedFile;
        if (!currentEncodedFile.parent_path().empty()) {
            std::error_code errorCode;
            std::filesystem::create_directories(
                currentEncodedFile.parent_path(),
                errorCode);
            if (errorCode) {
                std::cerr << "failed to create DataCodec output directory: "
                          << errorCode.message() << '\n';
                return 1;
            }
        }

        auto writer = iGame::IGDCWriter::New();
        iGame::iGameRunRecordSinkSet recordSinks;
        recordSinks.CaptureMessages();
        if (monitor) {
            recordSinks.CaptureTelemetry(
                ::datacodec::kRunLifecycleRecordMask |
                ::datacodec::RunRecordKind::Message |
                ::datacodec::RunRecordKind::StageTiming |
                ::datacodec::RunRecordKind::ResourceUsage);
        }
        writer->SetRunRecordSink(recordSinks.Sink());
        auto configuration = ::datacodec::MakeEncodeConfigurationParams(
            ::datacodec::DataCodecEncodeOptions{
            .tier = *tier,
            .enableCompressionEnhancement = compressionEnhancement,
        });
        if (rawPackage) {
            configuration.pipelineControl.packageFields.mode =
                ::datacodec::PackageFieldEncodingMode::Raw;
        }
        if (forceCellRemap) {
            configuration.pipelineControl.cellOrder =
                ::datacodec::EncodeCellOrderMode::Morton;
            configuration.controlParams.resourceBudget.SetRemapMortonLeafMiB(512u);
            configuration.controlParams.resourceBudget.SetRemapMortonRunBufferMiB(64u);
            configuration.controlParams.resourceBudget.SetRemapScratchQuotaMiB(1024u);
        }
        writer->SetEncodeControls(configuration);
        ::datacodec::topocodec::blockcodec::ResetTopologyEncodeStats();
        const auto encodeStart = std::chrono::steady_clock::now();
        if (!writer->WriteToFile(object, currentEncodedFile.string())) {
            std::cerr << "failed to encode DataCodec package: " << currentEncodedFile.string() << '\n';
            PrintMessages(recordSinks.TakeMessages());
            return 1;
        }
        const auto encodeEnd = std::chrono::steady_clock::now();

        std::cout << "DataCodec source file: " << sourceFile.string() << '\n';
        std::cout << "DataCodec encoded file: " << currentEncodedFile.string() << '\n';
        std::cout << "DataCodec connectivity codec: topology-grammar\n";
        std::cout << "DataCodec encode tier: " << ::datacodec::DataCodecEncodeTierName(*tier) << '\n';
        std::cout << "DataCodec compression enhancement: "
                  << (compressionEnhancement ? "enabled" : "disabled") << '\n';
        std::cout << "DataCodec load elapsed ms: "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(loadEnd - loadStart).count()
                  << '\n';
        std::cout << "DataCodec encode elapsed ms: "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(encodeEnd - encodeStart).count()
                  << '\n';
        std::cout << "DataCodec encoded bytes: " << std::filesystem::file_size(currentEncodedFile) << '\n';
        std::cout << "DataCodec package field mode: " << (rawPackage ? "Raw" : "Preset") << '\n';
        std::cout << "DataCodec cell order: " << (forceCellRemap ? "MortonOverride" : "Preset") << '\n';
        const auto topologyStats =
            ::datacodec::topocodec::blockcodec::SnapshotTopologyEncodeStats();
        std::cout << "Topology blocks: " << topologyStats.blockCount << '\n';
        std::cout << "Topology cells: " << topologyStats.cellCount << '\n';
        std::cout << "Topology indices: " << topologyStats.connectivityCount << '\n';
        std::cout << "Topology seed edges: hit=" << topologyStats.seedEdgeHitCount
                  << "; miss=" << topologyStats.seedEdgeMissCount << '\n';
        std::cout << "Topology lanes bytes: main=" << topologyStats.mainStreamBytes
                  << "; seed_lead=" << topologyStats.seedLeadStreamBytes
                  << "; seed_order=" << topologyStats.seedOrderBytes
                  << "; residual=" << topologyStats.residualBytes
                  << "; seed_aux_symbols=" << topologyStats.seedAuxSymbolCount
                  << "; seed_aux_stream=" << topologyStats.seedAuxStreamBytes
                  << "; payload=" << topologyStats.payloadBytes << '\n';
        if (monitor) {
            const auto sessions = recordSinks.SnapshotCompletedTelemetrySessions();
            double elapsedMs = 0.0;
            std::string meshType;
            std::vector<::datacodec::TelemetryStageRecord> stages;
            for (const auto& session : sessions) {
                if (session.parentRunId == 0u) {
                    elapsedMs = std::max(elapsedMs, session.elapsedMs);
                }
                if (meshType.empty() && !session.meshType.empty()) {
                    meshType = session.meshType;
                }
                stages.insert(stages.end(), session.stages.begin(), session.stages.end());
            }
            std::cout << "DataCodec telemetry elapsed ms: " << elapsedMs << '\n';
            std::cout << "DataCodec telemetry mesh type: " << meshType << '\n';
            std::cout << "DataCodec telemetry stage count: " << stages.size() << '\n';
            std::sort(stages.begin(), stages.end(), [](const auto& lhs, const auto& rhs) {
                return lhs.elapsedMs > rhs.elapsedMs;
            });
            const auto reportedStageCount = std::min<std::size_t>(stages.size(), 20u);
            for (std::size_t index = 0u; index < reportedStageCount; ++index) {
                std::cout << "DataCodec stage " << index + 1u << ": "
                          << stages[index].name << " " << stages[index].elapsedMs << " ms\n";
            }
        }
        if (auditAttributeZstd && !AuditAttributeZstd(currentEncodedFile)) {
            return 1;
        }
    return 0;
}
