#ifndef DATACODEC_STORAGE_LEAFPACKAGE_LEAFPACKAGEBYTEWRITER_H
#define DATACODEC_STORAGE_LEAFPACKAGE_LEAFPACKAGEBYTEWRITER_H

#include "DataCodec/Storage/Common/BinaryValueIO.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"
#include "DataCodec/Storage/LeafPackage/LeafPackageWireLayout.h"
#include "DataCodec/Storage/Package/PackageIdentity.h"
#include "DataCodec/Storage/ByteIO/ByteRange.h"
#include "DataCodec/Runtime/Cache/TransferCache/Common/EncodeTransferCache.h"

#include <algorithm>
#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>
namespace datacodec {

class LeafPackageByteWriter final {
public:
    ~LeafPackageByteWriter() = default;

    bool BeginPackageToSink(
        const std::size_t descriptorCapacity,
        const std::size_t initialStreamCount,
        IByteRangeOutput& sink,
        std::string* error = nullptr) {
        if (initialStreamCount > descriptorCapacity) {
            return validation::AssignError(
                error,
                "leaf package byte writer initial stream count exceeds descriptor capacity");
        }
        Reset();
        m_outputSink = &sink;
        m_descriptors.clear();
        m_descriptors.resize(descriptorCapacity);
        m_descriptorCount = initialStreamCount;
        m_rawFieldBytes = 0u;
        if (!GeneratePackageIdentity(m_identity, error)) {
            Reset();
            return false;
        }
        m_cursor = static_cast<std::uint64_t>(
            leafpackagewire::HeaderByteCount() +
            leafpackagewire::FieldDescriptorByteCount() * m_descriptors.size());
        return WriteHeaderToSink(error);
    }

    bool BeginStream(
        const std::size_t streamIndex,
        EncodeTransferCacheSchedule schedule,
        std::string* error = nullptr) {
        if (streamIndex >= m_descriptors.size()) {
            return validation::AssignError(error, "leaf package byte writer stream index is out of range");
        }
        if (m_streamOpen) {
            return validation::AssignError(error, "leaf package byte writer stream is already open");
        }
        auto& descriptor = m_descriptors[streamIndex];
        descriptor.schedule = std::move(schedule);
        descriptor.compressionType = descriptor.schedule.compressionType;
        descriptor.rawSize = descriptor.schedule.rawSize;
        descriptor.offset = m_cursor;
        descriptor.byteSize = 0u;
        m_descriptorCount = std::max(m_descriptorCount, streamIndex + 1u);
        m_currentStreamIndex = streamIndex;
        m_streamOpen = true;
        return true;
    }

    bool WriteStreamBytes(const std::span<const std::uint8_t> bytes, std::string* error = nullptr) {
        if (!m_streamOpen) {
            return validation::AssignError(
                error,
                "leaf package byte writer received bytes without an open stream");
        }
        if (bytes.empty()) {
            return true;
        }
        auto& descriptor = m_descriptors[m_currentStreamIndex];
        if (m_outputSink == nullptr) {
            return validation::AssignError(error, "leaf package byte writer sink is missing");
        }
        if (!m_outputSink->WriteAt(m_cursor, bytes, error)) {
            return false;
        }
        const auto byteCount = static_cast<std::uint64_t>(bytes.size());
        std::uint64_t nextDescriptorByteSize = 0u;
        std::uint64_t nextCursor = 0u;
        if (!validation::CheckedAddU64(
                descriptor.byteSize,
                byteCount,
                nextDescriptorByteSize,
                "leaf package byte writer stream byte size",
                error) ||
            !validation::CheckedAddU64(
                m_cursor,
                byteCount,
                nextCursor,
                "leaf package byte writer cursor",
                error)) {
            return false;
        }
        descriptor.byteSize = nextDescriptorByteSize;
        m_cursor = nextCursor;
        return true;
    }

    bool EndStream(const EncodeTransferCacheSchedule& finalSchedule, std::string* error = nullptr) {
        if (!m_streamOpen) {
            return validation::AssignError(
                error,
                "leaf package byte writer received stream end without stream begin");
        }
        if (bytestore::IsUnknownByteSize(finalSchedule.rawSize)) {
            return validation::AssignError(error, "leaf package byte writer stream ended with unknown raw size");
        }
        auto& descriptor = m_descriptors[m_currentStreamIndex];
        descriptor.schedule.rawSize = finalSchedule.rawSize;
        descriptor.rawSize = finalSchedule.rawSize;
        descriptor.schedule.compressionType = finalSchedule.compressionType;
        descriptor.compressionType = finalSchedule.compressionType;
        m_streamOpen = false;
        return true;
    }

    bool EndPackage(std::string* error = nullptr) {
        if (m_streamOpen) {
            return validation::AssignError(
                error,
                "leaf package byte writer cannot end package while stream is open");
        }
        std::uint64_t rawFieldBytes = 0u;
        for (std::size_t descriptorIndex = 0; descriptorIndex < m_descriptorCount; ++descriptorIndex) {
            if (!validation::CheckedAddU64(
                    rawFieldBytes,
                    m_descriptors[descriptorIndex].rawSize,
                    rawFieldBytes,
                    "leaf package byte writer raw field bytes",
                    error)) {
                return false;
            }
        }
        std::size_t localRawFieldBytes = 0u;
        if (!validation::CheckedCastSizeT(
                rawFieldBytes,
                localRawFieldBytes,
                "leaf package byte writer raw field bytes",
                error)) {
            return false;
        }
        m_rawFieldBytes = leafpackagewire::ComputeRawLeafPackageSize(
            m_descriptorCount,
            localRawFieldBytes);
        if (m_descriptorCount == 0u) {
            m_cursor = leafpackagewire::HeaderByteCount();
        }
        if (!WriteHeaderToSink(error)) {
            return false;
        }
        if (!WriteDescriptorsToSink(error)) {
            return false;
        }
        if (m_outputSink == nullptr || !m_outputSink->Finalize(m_cursor, error)) {
            return false;
        }
        m_committedByteCount = m_cursor;
        return true;
    }

    [[nodiscard]] std::uint64_t ByteCount() const noexcept { return m_committedByteCount; }

private:
    struct StreamDescriptor {
        EncodeTransferCacheSchedule schedule;
        EncodedFieldCompressionType compressionType{EncodedFieldCompressionType::None};
        std::uint64_t rawSize{0u};
        std::uint64_t offset{0u};
        std::uint64_t byteSize{0u};
    };

    template<typename TValue>
    static void AppendScalar(std::vector<std::uint8_t>& bytes, const TValue value) {
        using Storage = detail::WireStorageTypeT<TValue>;
        const auto storage = detail::ToWireStorage(value);
        for (std::size_t byteIndex = 0; byteIndex < sizeof(Storage); ++byteIndex) {
            bytes.push_back(static_cast<std::uint8_t>((storage >> (byteIndex * 8u)) & Storage{0xFFu}));
        }
    }

    static void AppendDescriptor(std::vector<std::uint8_t>& bytes, const StreamDescriptor& descriptor) {
        AppendScalar(bytes, static_cast<std::uint16_t>(descriptor.schedule.fieldType));
        AppendScalar(bytes, static_cast<std::uint32_t>(descriptor.compressionType));
        AppendScalar(bytes, descriptor.rawSize);
        AppendScalar(bytes, descriptor.offset);
        AppendScalar(bytes, descriptor.byteSize);
    }

    bool WriteHeaderToSink(std::string* error) {
        if (m_outputSink == nullptr) {
            return validation::AssignError(error, "leaf package byte writer sink is missing");
        }
        std::vector<std::uint8_t> header;
        AppendHeader(header);
        return m_outputSink->WriteAt(0u, std::span<const std::uint8_t>(header.data(), header.size()), error);
    }

    void AppendHeader(std::vector<std::uint8_t>& header) const {
        AppendScalar(header, leafpackagewire::kLeafPackageMagic);
        AppendScalar(header, leafpackagewire::kLeafPackageVersion);
        AppendScalar(header, m_identity.high);
        AppendScalar(header, m_identity.low);
        AppendScalar(header, static_cast<std::uint32_t>(m_descriptorCount));
        AppendScalar(header, static_cast<std::uint64_t>(m_rawFieldBytes));
    }

    bool WriteDescriptorsToSink(std::string* error) {
        if (m_outputSink == nullptr) {
            return validation::AssignError(error, "leaf package byte writer sink is missing");
        }
        std::vector<std::uint8_t> descriptors;
        descriptors.reserve(leafpackagewire::FieldDescriptorByteCount() * m_descriptorCount);
        for (std::size_t descriptorIndex = 0; descriptorIndex < m_descriptorCount; ++descriptorIndex) {
            AppendDescriptor(descriptors, m_descriptors[descriptorIndex]);
        }
        return m_outputSink->WriteAt(
            leafpackagewire::HeaderByteCount(),
            std::span<const std::uint8_t>(descriptors.data(), descriptors.size()),
            error);
    }

    void Reset() {
        m_descriptors.clear();
        m_descriptorCount = 0u;
        m_rawFieldBytes = 0u;
        m_identity = {};
        m_cursor = 0u;
        m_committedByteCount = 0u;
        m_streamOpen = false;
        m_currentStreamIndex = 0u;
        m_outputSink = nullptr;
    }

    std::vector<StreamDescriptor> m_descriptors;
    IByteRangeOutput* m_outputSink{nullptr};
    std::size_t m_descriptorCount{0u};
    std::uint64_t m_rawFieldBytes{0u};
    PackageIdentity m_identity;
    std::uint64_t m_cursor{0u};
    std::uint64_t m_committedByteCount{0u};
    std::size_t m_currentStreamIndex{0u};
    bool m_streamOpen{false};
};

} // namespace datacodec

#endif
