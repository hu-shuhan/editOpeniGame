#ifndef DATACODEC_STORAGE_FRAMEPACKAGE_FRAMEPACKAGEIO_H
#define DATACODEC_STORAGE_FRAMEPACKAGE_FRAMEPACKAGEIO_H

#include "DataCodec/Storage/Common/BinaryValueIO.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"
#include "DataCodec/Storage/LeafPackage/LeafPackageIO.h"
#include "DataCodec/Storage/Common/BinaryFieldWriter.h"
#include "DataCodec/Storage/ByteIO/ByteRange.h"
#include "DataCodec/Storage/FramePackage/FramePackageFormat.h"
#include "DataCodec/Storage/FramePackage/FramePackageWireLayout.h"

#include <functional>
#include <array>
#include <bit>
#include <istream>
#include <limits>
#include <memory>
#include <span>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
namespace datacodec {

class FramePackageIO {
public:
    using WriteProgressCallback = std::function<void(std::uint64_t, std::uint64_t)>;

    struct LeafPackageWriter {
        std::uint64_t leafPackageByteSize{0};
        PackageIdentity identity;
        std::function<bool(bytestore::IByteWriter&, std::string*)> appendLeafPackage;
    };

    static bool WriteToSink(
        const FramePackage& container,
        const std::span<const LeafPackageWriter> leafPackageWriters,
        IByteRangeOutput& sink,
        std::uint64_t* writtenBytes = nullptr,
        std::string* error = nullptr,
        WriteProgressCallback progressCallback = {}) {
        std::uint64_t totalByteCount = 0u;
        if (!ComputeFramePackageByteSize(
                container,
                leafPackageWriters,
                totalByteCount,
                error)) {
            return false;
        }
        ByteRangeOutputWriter writer(sink);
        CountingByteWriter progressWriter(
            writer,
            totalByteCount,
            std::move(progressCallback));
        progressWriter.ReportCurrentProgress();
        if (!WriteToWriter(container, leafPackageWriters, progressWriter, error)) {
            return false;
        }
        const auto byteCount = writer.ByteSizeHint();
        if (!sink.Finalize(byteCount, error)) {
            return false;
        }
        if (writtenBytes != nullptr) {
            *writtenBytes = byteCount;
        }
        progressWriter.ReportCompletedProgress();
        return true;
    }

    static bool WriteToWriter(
        const FramePackage& container,
        const std::span<const LeafPackageWriter> leafPackageWriters,
        bytestore::IByteWriter& output,
        std::string* error = nullptr) {
        using namespace detail;

        if (leafPackageWriters.size() != container.leaves.size()) {
            return validation::AssignError(error, "frame package leaf package writer count mismatch");
        }

        if (container.branches.size() > std::numeric_limits<std::uint32_t>::max() ||
            container.leaves.size() > std::numeric_limits<std::uint32_t>::max()) {
            return validation::AssignError(error, "frame package has too many records");
        }
        std::uint64_t totalByteCount = 0u;
        if (!ComputeFramePackageByteSize(
                container,
                leafPackageWriters,
                totalByteCount,
                error)) {
            return false;
        }
        const auto identity = ComputeFramePackageIdentity(
            container,
            leafPackageWriters,
            totalByteCount);

        if (!WriteScalarToWriter(output, framepackagewire::kFramePackageMagic, error) ||
            !WriteScalarToWriter(output, framepackagewire::kFramePackageVersion, error) ||
            !WriteScalarToWriter(output, identity.high, error) ||
            !WriteScalarToWriter(output, identity.low, error) ||
            !WriteScalarToWriter(output, container.frameIndex, error) ||
            !WriteScalarToWriter(output, container.timeValue, error) ||
            !WriteScalarToWriter(output, static_cast<std::uint8_t>(container.geometryTemporalRole), error) ||
            !WriteScalarToWriter(output, container.geometryKeyFrameIndex, error) ||
            !WriteScalarToWriter(output, static_cast<std::uint8_t>(container.attributeTemporalRole), error) ||
            !WriteScalarToWriter(output, container.attributeKeyFrameIndex, error) ||
            !WriteStringToWriter(output, container.rootName, error) ||
            !WriteScalarToWriter(output, static_cast<std::uint32_t>(container.branches.size()), error)) {
            return false;
        }
        for (const auto& branch : container.branches) {
            if (!WriteStringToWriter(output, branch.path, error) ||
                !WriteStringToWriter(output, branch.name, error)) {
                return false;
            }
        }
        if (!WriteScalarToWriter(output, static_cast<std::uint32_t>(container.leaves.size()), error)) {
            return false;
        }

        auto leafPackageOffset = framepackagewire::MetadataByteSize(container);
        for (std::size_t leafIndex = 0; leafIndex < container.leaves.size(); ++leafIndex) {
            const auto& leaf = container.leaves[leafIndex];
            const auto& writer = leafPackageWriters[leafIndex];
            const auto leafOffset = leafPackageOffset;
            if (!validation::CheckedAddU64(
                    leafPackageOffset,
                    writer.leafPackageByteSize,
                    leafPackageOffset,
                    "frame package leaf package offset",
                    error)) {
                return false;
            }
            if (!WriteStringToWriter(output, leaf.path, error) ||
                !WriteStringToWriter(output, leaf.name, error) ||
                !WriteScalarToWriter(output, leaf.ownerFrameIndex, error) ||
                !WriteScalarToWriter(output, static_cast<std::uint8_t>(leaf.topologyMode), error) ||
                !WriteScalarToWriter(output, writer.leafPackageByteSize, error) ||
                !WriteScalarToWriter(output, leafOffset, error)) {
                return false;
            }
        }

        for (const auto& writer : leafPackageWriters) {
            if (writer.appendLeafPackage != nullptr) {
                CountingByteWriter leafPackageOutput(output);
                if (!writer.appendLeafPackage(leafPackageOutput, error)) {
                    return false;
                }
                if (leafPackageOutput.ByteSizeHint() != writer.leafPackageByteSize) {
                    return validation::AssignError(error, "frame leaf leaf package size mismatch");
                }
            } else if (writer.leafPackageByteSize != 0u) {
                return validation::AssignError(error, "frame leaf leaf package writer is missing");
            }
        }

        return true;
    }

    static bool ReadMetadata(
        std::istream& stream,
        FramePackage& container,
        std::string* error = nullptr) {
        container = {};

        const auto currentPosition = stream.tellg();
        stream.clear();
        stream.seekg(0, std::ios::end);
        const auto streamByteSize = stream.tellg();
        stream.clear();
        stream.seekg(0, std::ios::beg);
        if (streamByteSize < 0) {
            return validation::AssignError(error, "failed to query frame package stream byte size");
        }

        auto restoreAndFail = [&]() {
            container = {};
            stream.clear();
            if (currentPosition >= 0) {
                stream.seekg(currentPosition, std::ios::beg);
            }
            return false;
        };

        std::uint32_t magic = 0;
        if (!ReadStreamScalar(stream, magic, error)) {
            return restoreAndFail();
        }
        if (magic != framepackagewire::kFramePackageMagic) {
            validation::AssignError(error, "frame package magic does not match");
            return restoreAndFail();
        }

        std::uint16_t version = 0;
        if (!ReadStreamScalar(stream, version, error)) {
            return restoreAndFail();
        }
        if (version != framepackagewire::kFramePackageVersion) {
            validation::AssignError(error, "版本不符合");
            return restoreAndFail();
        }
        if (!ReadStreamScalar(stream, container.identity.high, error) ||
            !ReadStreamScalar(stream, container.identity.low, error)) {
            return restoreAndFail();
        }
        if (!container.identity.IsValid()) {
            validation::AssignError(error, "frame package identity is invalid");
            return restoreAndFail();
        }

        if (!ReadHeader(stream, container, error)) {
            return restoreAndFail();
        }

        std::uint32_t branchCount = 0;
        if (!ReadStreamScalar(stream, branchCount, error)) {
            return restoreAndFail();
        }

        constexpr std::uint64_t minBranchRecordBytes =
            detail::WireScalarSize<std::uint64_t>() * 2u;
        if (!ValidateRecordCountFitsRemaining(
                stream,
                branchCount,
                minBranchRecordBytes,
                "frame package branch table",
                error)) {
            return restoreAndFail();
        }
        container.branches.reserve(branchCount);
        for (std::uint32_t index = 0; index < branchCount; ++index) {
            FramePackageBranchRecord branch;
            if (!ReadStreamString(stream, branch.path, error) ||
                !ReadStreamString(stream, branch.name, error)) {
                return restoreAndFail();
            }
            container.branches.push_back(std::move(branch));
        }

        std::uint32_t leafCount = 0;
        if (!ReadStreamScalar(stream, leafCount, error)) {
            return restoreAndFail();
        }

        constexpr std::uint64_t minLeafRecordBytes =
            detail::WireScalarSize<std::uint64_t>() * 4u +
            detail::WireScalarSize<std::uint32_t>() +
            detail::WireScalarSize<std::uint8_t>();
        if (!ValidateRecordCountFitsRemaining(
                stream,
                leafCount,
                minLeafRecordBytes,
                "frame package leaf table",
                error)) {
            return restoreAndFail();
        }
        container.leaves.reserve(leafCount);
        for (std::uint32_t index = 0; index < leafCount; ++index) {
            FramePackageLeafRecord leaf;
            std::uint8_t topologyModeValue = 0;
            if (!ReadStreamString(stream, leaf.path, error) ||
                !ReadStreamString(stream, leaf.name, error) ||
                !ReadStreamScalar(stream, leaf.ownerFrameIndex, error) ||
                !ReadStreamScalar(stream, topologyModeValue, error) ||
                !ReadStreamScalar(stream, leaf.leafPackageByteSize, error) ||
                !ReadStreamScalar(stream, leaf.leafPackageByteOffset, error) ||
                !ParseTopologyMode(topologyModeValue, leaf.topologyMode, error) ||
                !ValidateLeafPackageRange(
                    static_cast<std::uint64_t>(streamByteSize),
                    leaf.leafPackageByteOffset,
                    leaf.leafPackageByteSize,
                    "frame leaf leaf package",
                    error)) {
                return restoreAndFail();
            }
            container.leaves.push_back(std::move(leaf));
        }

        stream.clear();
        if (currentPosition >= 0) {
            stream.seekg(currentPosition, std::ios::beg);
        }
        return true;
    }

    static bool ReadMetadata(
        IByteRangeReader& reader,
        FramePackage& container,
        std::string* error = nullptr) {
        container = {};
        std::uint64_t offset = 0u;
        std::uint32_t magic = 0u;
        std::uint16_t version = 0u;
        std::uint8_t geometryTemporalRoleValue = 0u;
        std::uint8_t attributeTemporalRoleValue = 0u;
        if (!ReadRangeScalar(reader, offset, magic, error) ||
            magic != framepackagewire::kFramePackageMagic ||
            !ReadRangeScalar(reader, offset, version, error) ||
            version != framepackagewire::kFramePackageVersion ||
            !ReadRangeScalar(reader, offset, container.identity.high, error) ||
            !ReadRangeScalar(reader, offset, container.identity.low, error) ||
            !container.identity.IsValid() ||
            !ReadRangeScalar(reader, offset, container.frameIndex, error) ||
            !ReadRangeScalar(reader, offset, container.timeValue, error) ||
            !ReadRangeScalar(reader, offset, geometryTemporalRoleValue, error) ||
            !ReadRangeScalar(reader, offset, container.geometryKeyFrameIndex, error) ||
            !ReadRangeScalar(reader, offset, attributeTemporalRoleValue, error) ||
            !ReadRangeScalar(reader, offset, container.attributeKeyFrameIndex, error) ||
            !ReadRangeString(reader, offset, container.rootName, error)) {
            if (error != nullptr && error->empty()) {
                if (magic != framepackagewire::kFramePackageMagic) {
                    *error = "frame package magic does not match";
                } else if (version != framepackagewire::kFramePackageVersion) {
                    *error = "版本不符合";
                } else {
                    *error = "frame package identity is invalid";
                }
            }
            container = {};
            return false;
        }
        if (!ParseTemporalFieldRole(
                geometryTemporalRoleValue,
                container.geometryTemporalRole,
                error) ||
            !ParseTemporalFieldRole(
                attributeTemporalRoleValue,
                container.attributeTemporalRole,
                error)) {
            container = {};
            return false;
        }

        std::uint32_t branchCount = 0u;
        constexpr std::uint64_t minBranchRecordBytes = detail::WireScalarSize<std::uint64_t>() * 2u;
        if (!ReadRangeScalar(reader, offset, branchCount, error) ||
            !ValidateRangeRecordCount(
                reader.ByteSize(),
                offset,
                branchCount,
                minBranchRecordBytes,
                "frame package branch table",
                error)) {
            container = {};
            return false;
        }
        container.branches.reserve(branchCount);
        for (std::uint32_t index = 0u; index < branchCount; ++index) {
            FramePackageBranchRecord branch;
            if (!ReadRangeString(reader, offset, branch.path, error) ||
                !ReadRangeString(reader, offset, branch.name, error)) {
                container = {};
                return false;
            }
            container.branches.push_back(std::move(branch));
        }

        std::uint32_t leafCount = 0u;
        constexpr std::uint64_t minLeafRecordBytes =
            detail::WireScalarSize<std::uint64_t>() * 4u +
            detail::WireScalarSize<std::uint32_t>() +
            detail::WireScalarSize<std::uint8_t>();
        if (!ReadRangeScalar(reader, offset, leafCount, error) ||
            !ValidateRangeRecordCount(
                reader.ByteSize(),
                offset,
                leafCount,
                minLeafRecordBytes,
                "frame package leaf table",
                error)) {
            container = {};
            return false;
        }
        container.leaves.reserve(leafCount);
        for (std::uint32_t index = 0u; index < leafCount; ++index) {
            FramePackageLeafRecord leaf;
            std::uint8_t topologyModeValue = 0u;
            if (!ReadRangeString(reader, offset, leaf.path, error) ||
                !ReadRangeString(reader, offset, leaf.name, error) ||
                !ReadRangeScalar(reader, offset, leaf.ownerFrameIndex, error) ||
                !ReadRangeScalar(reader, offset, topologyModeValue, error) ||
                !ReadRangeScalar(reader, offset, leaf.leafPackageByteSize, error) ||
                !ReadRangeScalar(reader, offset, leaf.leafPackageByteOffset, error) ||
                !ParseTopologyMode(topologyModeValue, leaf.topologyMode, error) ||
                !ValidateLeafPackageRange(
                    reader.ByteSize(),
                    leaf.leafPackageByteOffset,
                    leaf.leafPackageByteSize,
                    "frame leaf leaf package",
                    error)) {
                container = {};
                return false;
            }
            container.leaves.push_back(std::move(leaf));
        }
        return true;
    }

public:
    static bool MakeFrameLeafPackageWriter(
        std::shared_ptr<const LeafPackage> leafPackage,
        LeafPackageWriter& writer,
        std::string* error = nullptr) {
        if (leafPackage == nullptr) {
            return validation::AssignError(error, "leaf package is missing");
        }

        std::uint64_t leafPackageByteSize = 0u;
        if (!LeafPackageIO::ComputeLeafPackageByteSize(*leafPackage, leafPackageByteSize, error)) {
            return false;
        }
        PackageIdentity identity;
        auto segmentedLeafPackage = LeafPackageIO::BuildSegmentedLeafPackage(
            *leafPackage,
            error,
            bytestore::ByteSourceConsumptionMode::OneShot,
            &identity);
        if (segmentedLeafPackage == nullptr) {
            return false;
        }

        writer = LeafPackageWriter{
            .leafPackageByteSize = leafPackageByteSize,
            .identity = identity,
            .appendLeafPackage = [segmentedLeafPackage](bytestore::IByteWriter& output, std::string* writeError) {
                return segmentedLeafPackage->CopyTo(output, writeError);
            },
        };
        return true;
    }

    static bool MakeFrameLeafPackageWriter(
        const LeafPackage& leafPackage,
        LeafPackageWriter& writer,
        std::string* error = nullptr) {
        return MakeFrameLeafPackageWriter(std::make_shared<LeafPackage>(leafPackage), writer, error);
    }

    static bool MakeFrameLeafPackageBytesWriter(
        std::vector<std::uint8_t> leafPackageBytes,
        LeafPackageWriter& writer,
        std::string* error = nullptr) {
        if (leafPackageBytes.empty()) {
            return validation::AssignError(error, "encoded data codec output bytes are empty");
        }

        PackageIdentity identity;
        if (!TryReadPackageIdentityPrefix(
                std::span<const std::uint8_t>(leafPackageBytes.data(), leafPackageBytes.size()),
                identity)) {
            return validation::AssignError(error, "encoded leaf package identity is invalid");
        }

        auto leafPackageBytesStorage = std::make_shared<std::vector<std::uint8_t>>(std::move(leafPackageBytes));
        writer = LeafPackageWriter{
            .leafPackageByteSize = static_cast<std::uint64_t>(leafPackageBytesStorage->size()),
            .identity = identity,
            .appendLeafPackage = [leafPackageBytesStorage](bytestore::IByteWriter& output, std::string* writeError) {
                return output.Write(
                    std::span<const std::uint8_t>(leafPackageBytesStorage->data(), leafPackageBytesStorage->size()),
                    writeError);
            },
        };
        return true;
    }

    static bool MakeFrameLeafPackageByteRangeWriter(
        std::shared_ptr<IByteRangeReader> leafPackageReader,
        LeafPackageWriter& writer,
        const std::size_t windowBytes,
        std::string* error = nullptr) {
        if (leafPackageReader == nullptr) {
            return validation::AssignError(error, "leaf package byte range reader is missing");
        }
        const auto leafPackageByteSize = leafPackageReader->ByteSize();
        if (leafPackageByteSize == 0u) {
            return validation::AssignError(error, "leaf package byte range reader is empty");
        }
        constexpr std::size_t kPackageIdentityPrefixBytes =
            sizeof(std::uint32_t) + sizeof(std::uint16_t) + sizeof(std::uint64_t) * 2u;
        std::array<std::uint8_t, kPackageIdentityPrefixBytes> prefix{};
        PackageIdentity identity;
        if (!leafPackageReader->ReadAt(
                0u,
                std::span<std::uint8_t>(prefix.data(), prefix.size()),
                error) ||
            !TryReadPackageIdentityPrefix(prefix, identity)) {
            return validation::AssignError(error, "leaf package byte range identity is invalid");
        }
        writer = LeafPackageWriter{
            .leafPackageByteSize = leafPackageByteSize,
            .identity = identity,
            .appendLeafPackage = [leafPackageReader, windowBytes](
                bytestore::IByteWriter& output,
                std::string* writeError) {
                const auto resolvedWindowBytes = std::max<std::size_t>(windowBytes, 1u);
                std::vector<std::uint8_t> buffer(resolvedWindowBytes, 0u);
                std::uint64_t offset = 0u;
                while (offset < leafPackageReader->ByteSize()) {
                    const auto currentBytes = static_cast<std::size_t>(
                        std::min<std::uint64_t>(
                            leafPackageReader->ByteSize() - offset,
                            static_cast<std::uint64_t>(resolvedWindowBytes)));
                    auto window = std::span<std::uint8_t>(buffer.data(), currentBytes);
                    if (!leafPackageReader->ReadAt(offset, window, writeError) ||
                        !output.Write(
                            std::span<const std::uint8_t>(window.data(), window.size()),
                            writeError)) {
                        return false;
                    }
                    offset += static_cast<std::uint64_t>(currentBytes);
                }
                return true;
            },
        };
        return true;
    }

private:
    static bool ComputeFramePackageByteSize(
        const FramePackage& container,
        const std::span<const LeafPackageWriter> leafPackageWriters,
        std::uint64_t& byteSize,
        std::string* error) {
        byteSize = framepackagewire::MetadataByteSize(container);
        for (const auto& leafPackageWriter : leafPackageWriters) {
            if (!validation::CheckedAddU64(
                    byteSize,
                    leafPackageWriter.leafPackageByteSize,
                    byteSize,
                    "frame package output byte count",
                    error)) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] static PackageIdentity ComputeFramePackageIdentity(
        const FramePackage& container,
        const std::span<const LeafPackageWriter> leafPackageWriters,
        const std::uint64_t byteSize) {
        PackageIdentityBuilder builder("igdc.frame-package.v1");
        builder.AddString(container.rootName);
        builder.AddUnsigned(byteSize);
        builder.AddUnsigned(container.frameIndex);
        builder.AddUnsigned(std::bit_cast<std::uint32_t>(container.timeValue));
        builder.AddUnsigned(static_cast<std::uint64_t>(container.geometryTemporalRole));
        builder.AddUnsigned(container.geometryKeyFrameIndex);
        builder.AddUnsigned(static_cast<std::uint64_t>(container.attributeTemporalRole));
        builder.AddUnsigned(container.attributeKeyFrameIndex);
        builder.AddUnsigned(static_cast<std::uint64_t>(container.branches.size()));
        for (const auto& branch : container.branches) {
            builder.AddString(branch.path);
            builder.AddString(branch.name);
        }
        builder.AddUnsigned(static_cast<std::uint64_t>(container.leaves.size()));
        for (std::size_t leafIndex = 0u; leafIndex < container.leaves.size(); ++leafIndex) {
            const auto& leaf = container.leaves[leafIndex];
            builder.AddUnsigned(static_cast<std::uint64_t>(leafIndex));
            builder.AddString(leaf.path);
            builder.AddString(leaf.name);
            builder.AddUnsigned(leaf.ownerFrameIndex);
            builder.AddUnsigned(static_cast<std::uint64_t>(leaf.topologyMode));
            if (leafIndex < leafPackageWriters.size()) {
                builder.AddUnsigned(leafPackageWriters[leafIndex].leafPackageByteSize);
                builder.AddIdentity(leafPackageWriters[leafIndex].identity);
            }
        }
        return builder.Finish();
    }

    template<typename TValue>
    static bool ReadRangeScalar(
        IByteRangeReader& reader,
        std::uint64_t& offset,
        TValue& value,
        std::string* error = nullptr) {
        static_assert(detail::IsWireScalarValue<TValue>);
        using Storage = detail::WireStorageTypeT<TValue>;
        if (offset > reader.ByteSize() || sizeof(Storage) > reader.ByteSize() - offset) {
            return validation::AssignError(error, "unexpected end of frame package range while reading scalar");
        }
        std::array<std::uint8_t, sizeof(Storage)> bytes{};
        if (!reader.ReadAt(offset, std::span<std::uint8_t>(bytes.data(), bytes.size()), error)) {
            return false;
        }
        Storage storage{};
        for (std::size_t byteIndex = 0u; byteIndex < sizeof(Storage); ++byteIndex) {
            storage |= static_cast<Storage>(bytes[byteIndex]) << (byteIndex * 8u);
        }
        detail::FromWireStorage(storage, value);
        offset += sizeof(Storage);
        return true;
    }

    static bool ReadRangeString(
        IByteRangeReader& reader,
        std::uint64_t& offset,
        std::string& value,
        std::string* error = nullptr) {
        std::uint64_t byteCount = 0u;
        if (!ReadRangeScalar(reader, offset, byteCount, error)) {
            return false;
        }
        std::size_t localByteCount = 0u;
        if (!validation::CheckedCastSizeT(byteCount, localByteCount, "frame package string", error) ||
            offset > reader.ByteSize() || byteCount > reader.ByteSize() - offset) {
            return validation::AssignError(error, "frame package string exceeds input range");
        }
        value.resize(localByteCount);
        if (localByteCount != 0u &&
            !reader.ReadAt(
                offset,
                std::span<std::uint8_t>(
                    reinterpret_cast<std::uint8_t*>(value.data()),
                    value.size()),
                error)) {
            value.clear();
            return false;
        }
        offset += byteCount;
        return true;
    }

    static bool ValidateRangeRecordCount(
        const std::uint64_t byteSize,
        const std::uint64_t offset,
        const std::uint32_t count,
        const std::uint64_t minRecordBytes,
        const char* label,
        std::string* error = nullptr) {
        if (offset > byteSize ||
            (count != 0u &&
             (minRecordBytes == 0u || static_cast<std::uint64_t>(count) > (byteSize - offset) / minRecordBytes))) {
            return validation::AssignError(error, std::string(label) + " exceeds input range");
        }
        return true;
    }

    class ByteRangeOutputWriter final : public bytestore::IByteWriter {
    public:
        explicit ByteRangeOutputWriter(IByteRangeOutput& sink) : m_sink(sink) {}

        [[nodiscard]] std::uint64_t ByteSizeHint() const noexcept override { return m_byteCount; }

        bool Write(const std::span<const std::uint8_t> bytes, std::string* error = nullptr) override {
            std::uint64_t nextByteCount = 0u;
            if (!validation::CheckedAddU64(
                    m_byteCount,
                    static_cast<std::uint64_t>(bytes.size()),
                    nextByteCount,
                    "frame package byte range output byte count",
                    error)) {
                return false;
            }
            if (!m_sink.WriteAt(m_byteCount, bytes, error)) {
                return false;
            }
            m_byteCount = nextByteCount;
            return true;
        }

    private:
        IByteRangeOutput& m_sink;
        std::uint64_t m_byteCount{0u};
    };

    class CountingByteWriter final : public bytestore::IByteWriter {
    public:
        explicit CountingByteWriter(
            bytestore::IByteWriter& target,
            const std::uint64_t totalByteCount = 0u,
            WriteProgressCallback progressCallback = {})
            : m_target(target),
              m_totalByteCount(totalByteCount),
              m_progressCallback(std::move(progressCallback)) {}

        [[nodiscard]] std::uint64_t ByteSizeHint() const noexcept override { return m_byteCount; }

        bool Write(const std::span<const std::uint8_t> bytes, std::string* error = nullptr) override {
            std::uint64_t nextByteCount = 0u;
            if (!validation::CheckedAddU64(
                    m_byteCount,
                    static_cast<std::uint64_t>(bytes.size()),
                    nextByteCount,
                    "frame package counting writer byte count",
                    error)) {
                return false;
            }
            if (!m_target.Write(bytes, error)) {
                return false;
            }
            m_byteCount = nextByteCount;
            ReportCurrentProgress();
            return true;
        }

        void ReportCurrentProgress() const {
            if (m_progressCallback) {
                m_progressCallback(m_byteCount, m_totalByteCount);
            }
        }

        void ReportCompletedProgress() const {
            if (m_progressCallback) {
                m_progressCallback(m_totalByteCount, m_totalByteCount);
            }
        }

    private:
        bytestore::IByteWriter& m_target;
        std::uint64_t m_byteCount{0u};
        std::uint64_t m_totalByteCount{0u};
        WriteProgressCallback m_progressCallback;
    };

    static bool ParseTopologyMode(
        const std::uint8_t value,
        TopologyOwnershipMode& mode,
        std::string* error = nullptr) {
        if (value > static_cast<std::uint8_t>(TopologyOwnershipMode::Reused)) {
            return validation::AssignError(error, "invalid topology ownership mode");
        }
        mode = static_cast<TopologyOwnershipMode>(value);
        return true;
    }

    static bool ParseTemporalFieldRole(
        const std::uint8_t value,
        TemporalFieldRole& role,
        std::string* error = nullptr) {
        if (value > static_cast<std::uint8_t>(TemporalFieldRole::PredFrame)) {
            return validation::AssignError(error, "invalid temporal field role");
        }
        role = static_cast<TemporalFieldRole>(value);
        return true;
    }

    static bool ValidateLeafPackageRange(
        const std::uint64_t bufferSize,
        const std::uint64_t offset,
        const std::uint64_t size,
        const char* label,
        std::string* error = nullptr) {
        if (size == 0) {
            return true;
        }
        if (offset > bufferSize || size > bufferSize - offset) {
            return validation::AssignError(error, std::string(label) + " exceeds input buffer");
        }
        return true;
    }

    static bool RemainingStreamBytes(
        std::istream& stream,
        std::uint64_t& remaining,
        std::string* error = nullptr) {
        remaining = 0u;
        const auto currentPosition = stream.tellg();
        if (currentPosition < 0) {
            return validation::AssignError(error, "failed to query frame package stream position");
        }
        stream.clear();
        stream.seekg(0, std::ios::end);
        const auto endPosition = stream.tellg();
        stream.clear();
        stream.seekg(currentPosition, std::ios::beg);
        if (endPosition < currentPosition || endPosition < 0) {
            return validation::AssignError(error, "failed to query frame package stream size");
        }
        remaining = static_cast<std::uint64_t>(endPosition - currentPosition);
        return true;
    }

    static bool ValidateRecordCountFitsRemaining(
        std::istream& stream,
        const std::uint32_t count,
        const std::uint64_t minRecordBytes,
        const char* label,
        std::string* error = nullptr) {
        if (count == 0u) {
            return true;
        }
        std::uint64_t remaining = 0u;
        if (!RemainingStreamBytes(stream, remaining, error)) {
            return false;
        }
        if (minRecordBytes == 0u ||
            static_cast<std::uint64_t>(count) > remaining / minRecordBytes) {
            return validation::AssignError(error, std::string(label) + " exceeds input buffer");
        }
        return true;
    }

    static bool ReadHeader(
        std::istream& stream,
        FramePackage& container,
        std::string* error = nullptr) {
        std::uint8_t geometryTemporalRoleValue = 0;
        std::uint8_t attributeTemporalRoleValue = 0;
        if (!ReadStreamScalar(stream, container.frameIndex, error) ||
            !ReadStreamScalar(stream, container.timeValue, error) ||
            !ReadStreamScalar(stream, geometryTemporalRoleValue, error) ||
            !ReadStreamScalar(stream, container.geometryKeyFrameIndex, error) ||
            !ReadStreamScalar(stream, attributeTemporalRoleValue, error) ||
            !ReadStreamScalar(stream, container.attributeKeyFrameIndex, error) ||
            !ReadStreamString(stream, container.rootName, error)) {
            return false;
        }
        return ParseTemporalFieldRole(geometryTemporalRoleValue, container.geometryTemporalRole, error) &&
            ParseTemporalFieldRole(attributeTemporalRoleValue, container.attributeTemporalRole, error);
    }

    template<typename TValue>
    static bool ReadStreamScalar(
        std::istream& stream,
        TValue& value,
        std::string* error = nullptr) {
        static_assert(detail::IsWireScalarValue<TValue>);
        using Storage = detail::WireStorageTypeT<TValue>;
        std::uint8_t bytes[sizeof(Storage)]{};
        if (!stream.read(reinterpret_cast<char*>(bytes), static_cast<std::streamsize>(sizeof(Storage)))) {
            return validation::AssignError(
                error,
                "unexpected end of frame package stream while reading scalar");
        }
        Storage storage{};
        for (std::size_t byteIndex = 0; byteIndex < sizeof(Storage); ++byteIndex) {
            storage |= static_cast<Storage>(bytes[byteIndex]) << (byteIndex * 8u);
        }
        detail::FromWireStorage(storage, value);
        return true;
    }

    static bool ReadStreamString(
        std::istream& stream,
        std::string& value,
        std::string* error = nullptr) {
        std::uint64_t size = 0;
        if (!ReadStreamScalar(stream, size, error)) {
            return false;
        }
        std::size_t localSize = 0u;
        if (!validation::CheckedCastSizeT(size, localSize, "frame package string", error)) {
            return false;
        }
        std::uint64_t remaining = 0u;
        if (!RemainingStreamBytes(stream, remaining, error)) {
            return false;
        }
        if (size > remaining) {
            return validation::AssignError(error, "frame package string exceeds input buffer");
        }
        if (size > static_cast<std::uint64_t>(std::numeric_limits<std::streamsize>::max())) {
            return validation::AssignError(error, "frame package string exceeds stream read size");
        }
        value.resize(localSize);
        if (size == 0) {
            return true;
        }
        if (!stream.read(value.data(), static_cast<std::streamsize>(localSize))) {
            validation::AssignError(error, "unexpected end of frame package stream while reading string");
            value.clear();
            return false;
        }
        return true;
    }
};

inline bool MakeFrameLeafPackageWriter(
    std::shared_ptr<const LeafPackage> leafPackage,
    FramePackageIO::LeafPackageWriter& writer,
    std::string* error = nullptr) {
    return FramePackageIO::MakeFrameLeafPackageWriter(std::move(leafPackage), writer, error);
}

inline bool MakeFrameLeafPackageWriter(
    const LeafPackage& leafPackage,
    FramePackageIO::LeafPackageWriter& writer,
    std::string* error = nullptr) {
    return FramePackageIO::MakeFrameLeafPackageWriter(leafPackage, writer, error);
}

inline bool MakeFrameLeafPackageBytesWriter(
    std::vector<std::uint8_t> leafPackageBytes,
    FramePackageIO::LeafPackageWriter& writer,
    std::string* error = nullptr) {
    return FramePackageIO::MakeFrameLeafPackageBytesWriter(std::move(leafPackageBytes), writer, error);
}

} // namespace datacodec

#endif
