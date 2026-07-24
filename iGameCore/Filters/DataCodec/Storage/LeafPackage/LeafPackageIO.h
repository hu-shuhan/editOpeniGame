#ifndef DATACODEC_STORAGE_LEAFPACKAGE_LEAFPACKAGEIO_H
#define DATACODEC_STORAGE_LEAFPACKAGE_LEAFPACKAGEIO_H

#include "DataCodec/Storage/Common/BinaryValueIO.h"
#include "DataCodec/Storage/ByteIO/ByteRange.h"
#include "DataCodec/Storage/ByteStore/SegmentedBinaryObject.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"
#include "DataCodec/Storage/Common/BinaryFieldWriter.h"
#include "DataCodec/Storage/LeafPackage/LeafPackage.h"
#include "DataCodec/Storage/LeafPackage/LeafPackageWireLayout.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>
namespace datacodec {

namespace leafpackagewire {

// [DC防护:字段] leaf package 字节边界校验字段大小、偏移顺序和本地地址空间
inline bool CheckAddressableRawFieldBytes(
    const std::uint64_t rawFieldBytes,
    std::string* error) {
    std::size_t localBytes = 0u;
    return validation::CheckedCastSizeT(
        rawFieldBytes,
        localBytes,
        "leaf package raw field bytes",
        error);
}

inline bool CheckAddressableFieldSizes(
    const std::uint64_t fieldRawSize,
    const std::uint64_t fieldByteSize,
    std::string* error) {
    std::size_t localFieldRawSize = 0u;
    std::size_t localFieldByteSize = 0u;
    return validation::CheckedCastSizeT(
            fieldRawSize,
            localFieldRawSize,
            "leaf package field raw size",
            error) &&
        validation::CheckedCastSizeT(
            fieldByteSize,
            localFieldByteSize,
            "leaf package field byte size",
            error);
}

inline bool CheckSequentialFieldOffset(
    const std::uint64_t fieldByteOffset,
    const std::uint64_t expectedFieldByteOffset,
    std::string* error) {
    if (fieldByteOffset != expectedFieldByteOffset) {
        validation::AssignError(error, "leaf package field descriptor offset is not sequential");
        return false;
    }
    return true;
}

inline bool AdvanceSequentialFieldOffset(
    const std::uint64_t fieldByteSize,
    std::uint64_t& expectedFieldByteOffset,
    std::string* error) {
    return validation::CheckedAddU64(
        expectedFieldByteOffset,
        fieldByteSize,
        expectedFieldByteOffset,
        "leaf package field byte offset",
        error);
}

inline bool CheckFieldSourcePresence(
    const std::uint64_t fieldRawSize,
    const bytestore::IByteSource* source,
    std::string* error) {
    if (fieldRawSize > 0u && source == nullptr) {
        validation::AssignError(error, "field is missing its source");
        return false;
    }
    return true;
}

inline bool CheckRawAndEncodedSizeConsistency(
    const EncodedFieldCompressionType compressionType,
    const std::uint64_t fieldRawSize,
    const std::uint64_t fieldByteSize,
    std::string* error) {
    if (compressionType == EncodedFieldCompressionType::None &&
        fieldRawSize != fieldByteSize) {
        validation::AssignError(error, "uncompressed leaf package field raw size does not match byte size");
        return false;
    }
    return true;
}

inline bool AccumulateLeafPackageRawPayloadBytes(
    const LeafPackage& leafPackage,
    std::uint64_t& rawPayloadBytes,
    std::string* error = nullptr) {
    rawPayloadBytes = 0u;
    for (const auto& field : leafPackage.fields) {
        if (!validation::CanAddU64(rawPayloadBytes, field.rawSize)) {
            validation::AssignError(error, "leaf package raw field bytes overflow");
            return false;
        }
        rawPayloadBytes += static_cast<std::uint64_t>(field.rawSize);
    }
    return true;
}

inline bool CheckLeafPackageRawFieldBytes(
    const LeafPackage& leafPackage,
    std::string* error = nullptr) {
    std::uint64_t rawPayloadBytes = 0u;
    if (!AccumulateLeafPackageRawPayloadBytes(leafPackage, rawPayloadBytes, error)) {
        return false;
    }
    if (rawPayloadBytes > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        validation::AssignError(error, "leaf package raw field bytes exceed the local address space");
        return false;
    }
    const auto expectedRawFieldBytes = leafpackagewire::ComputeRawLeafPackageSize(
        leafPackage.fields.size(),
        static_cast<std::size_t>(rawPayloadBytes));
    if (leafPackage.rawFieldBytes != expectedRawFieldBytes) {
        validation::AssignError(error, "leaf package raw field bytes do not match field descriptors");
        return false;
    }
    return true;
}

} // namespace leafpackagewire

class LeafPackageIO {
private:
    class ByteRangeByteSource final : public bytestore::IByteSource {
    public:
        ByteRangeByteSource(
            std::shared_ptr<IByteRangeReader> reader,
            const std::uint64_t byteOffset,
            const std::uint64_t byteSize)
            : m_reader(std::move(reader)),
              m_byteOffset(byteOffset),
              m_byteSize(byteSize) {}

        [[nodiscard]] std::uint64_t ByteSizeHint() const noexcept override { return m_byteSize; }
        [[nodiscard]] std::uint64_t ResidentSizeHint() const noexcept override { return 0u; }
        [[nodiscard]] std::uint64_t MappedSizeHint() const noexcept override {
            return ContiguousBytes().empty() ? 0u : m_byteSize;
        }
        [[nodiscard]] std::span<const std::uint8_t> ContiguousBytes() const noexcept override {
            return m_reader != nullptr
                ? m_reader->ContiguousRange(m_byteOffset, m_byteSize)
                : std::span<const std::uint8_t>{};
        }
        ContiguousViewStatus PrepareContiguousBytes(
            std::span<const std::uint8_t>& output,
            std::string* error = nullptr) override {
            output = {};
            if (m_reader == nullptr) {
                validation::AssignError(error, "byte range source reader is missing");
                return ContiguousViewStatus::Error;
            }
            return m_reader->PrepareContiguousRange(
                m_byteOffset,
                m_byteSize,
                output,
                error);
        }
        [[nodiscard]] bool PreferDirectCopy() const noexcept override {
            return !ContiguousBytes().empty();
        }
        [[nodiscard]] bool CanRead() const noexcept override { return m_reader != nullptr; }

        bool Read(
            const std::uint64_t offset,
            const std::span<std::uint8_t> output,
            std::string* error = nullptr) const override {
            if (m_reader == nullptr) {
                return validation::AssignError(error, "byte range source reader is missing");
            }
            if (offset > m_byteSize || output.size() > m_byteSize - offset) {
                return validation::AssignError(error, "byte range source read is outside the source range");
            }
            std::uint64_t absoluteOffset = 0u;
            if (!validation::CheckedAddU64(
                    m_byteOffset,
                    offset,
                    absoluteOffset,
                    "byte range source absolute offset",
                    error)) {
                return false;
            }
            return m_reader->ReadAt(absoluteOffset, output, error);
        }

        bool CopyTo(bytestore::IByteWriter& writer, std::string* error = nullptr) override {
            if (m_reader == nullptr) {
                return validation::AssignError(error, "byte range source reader is missing");
            }
            constexpr std::size_t kWriteWindowBytes = 16u * 1024u * 1024u;
            std::vector<std::uint8_t> buffer(kWriteWindowBytes, 0u);
            std::uint64_t offset = 0u;
            while (offset < m_byteSize) {
                const auto currentBytes = static_cast<std::size_t>(
                    std::min<std::uint64_t>(
                        m_byteSize - offset,
                        static_cast<std::uint64_t>(kWriteWindowBytes)));
                auto window = std::span<std::uint8_t>(buffer.data(), currentBytes);
                if (!Read(offset, window, error) ||
                    !writer.Write(std::span<const std::uint8_t>(window.data(), window.size()), error)) {
                    return false;
                }
                offset += static_cast<std::uint64_t>(currentBytes);
            }
            return true;
        }

        void Release() noexcept override {
            m_reader.reset();
            m_byteOffset = 0u;
            m_byteSize = 0u;
        }

    private:
        std::shared_ptr<IByteRangeReader> m_reader;
        std::uint64_t m_byteOffset{0u};
        std::uint64_t m_byteSize{0u};
    };

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
                    "leaf package byte range output byte count",
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

public:
    static bool ComputeLeafPackageByteSize(
        const LeafPackage& leafPackage,
        std::uint64_t& byteSize,
        std::string* error = nullptr) {
        byteSize = 0u;

        if (leafPackage.fields.size() > std::numeric_limits<std::uint32_t>::max()) {
            validation::AssignError(error, "leaf package has too many fields to write");
            return false;
        }
        if (!leafpackagewire::CheckLeafPackageRawFieldBytes(leafPackage, error)) {
            return false;
        }

        constexpr auto headerByteCount = static_cast<std::uint64_t>(leafpackagewire::HeaderByteCount());
        constexpr auto fieldDescriptorByteCount =
            static_cast<std::uint64_t>(leafpackagewire::FieldDescriptorByteCount());
        const auto fieldCount = static_cast<std::uint64_t>(leafPackage.fields.size());
        if (!validation::CanMulU64(fieldDescriptorByteCount, fieldCount) ||
            !validation::CanAddU64(headerByteCount, fieldDescriptorByteCount * fieldCount)) {
            validation::AssignError(error, "leaf package descriptor table is too large to write");
            return false;
        }

        std::uint64_t totalBytes = headerByteCount + fieldDescriptorByteCount * fieldCount;
        for (const auto& field : leafPackage.fields) {
            const auto fieldByteSize = static_cast<std::uint64_t>(field.ByteSizeHint());
            if (!leafpackagewire::CheckFieldSourcePresence(field.rawSize, field.source.get(), error) ||
                !leafpackagewire::CheckRawAndEncodedSizeConsistency(
                    field.compressionType,
                    field.rawSize,
                    fieldByteSize,
                    error)) {
                return false;
            }
            if (!validation::CheckedAddU64(
                    totalBytes,
                    fieldByteSize,
                    totalBytes,
                    "leaf package byte size",
                    error)) {
                return false;
            }
        }

        byteSize = totalBytes;
        return true;
    }

    static bool WriteLeafPackage(
        const LeafPackage& leafPackage,
        bytestore::IByteWriter& writer,
        std::string* error = nullptr) {
        using namespace detail;

        std::uint64_t leafPackageByteSize = 0u;
        if (!ComputeLeafPackageByteSize(leafPackage, leafPackageByteSize, error)) {
            return false;
        }
        PackageIdentity identity;
        if (!GeneratePackageIdentity(identity, error)) {
            return false;
        }

        if (!WriteScalarToWriter(writer, leafpackagewire::kLeafPackageMagic, error) ||
            !WriteScalarToWriter(writer, leafpackagewire::kLeafPackageVersion, error) ||
            !WriteScalarToWriter(writer, identity.high, error) ||
            !WriteScalarToWriter(writer, identity.low, error) ||
            !WriteScalarToWriter(writer, static_cast<std::uint32_t>(leafPackage.fields.size()), error) ||
            !WriteScalarToWriter(writer, static_cast<std::uint64_t>(leafPackage.rawFieldBytes), error)) {
            return false;
        }

        std::uint64_t fieldByteOffset = static_cast<std::uint64_t>(
            leafpackagewire::HeaderByteCount() + leafpackagewire::FieldDescriptorByteCount() * leafPackage.fields.size());
        for (const auto& field : leafPackage.fields) {
            if (!WriteScalarToWriter(writer, static_cast<std::uint16_t>(field.type), error) ||
                !WriteScalarToWriter(writer, static_cast<std::uint32_t>(field.compressionType), error) ||
                !WriteScalarToWriter(writer, static_cast<std::uint64_t>(field.rawSize), error) ||
                !WriteScalarToWriter(writer, fieldByteOffset, error) ||
                !WriteScalarToWriter(writer, static_cast<std::uint64_t>(field.ByteSizeHint()), error)) {
                return false;
            }
            if (!validation::CheckedAddU64(
                    fieldByteOffset,
                    static_cast<std::uint64_t>(field.ByteSizeHint()),
                    fieldByteOffset,
                    "leaf package field byte offset",
                    error)) {
                return false;
            }
        }
        if (fieldByteOffset != leafPackageByteSize) {
            validation::AssignError(error, "leaf package byte size changed while writing descriptors");
            return false;
        }

        for (const auto& field : leafPackage.fields) {
            if (field.source != nullptr) {
                if (!field.source->CopyTo(writer, error)) {
                    return false;
                }
            }
        }

        return true;
    }

    static std::shared_ptr<bytestore::SegmentedBinaryObject> BuildSegmentedLeafPackage(
        const LeafPackage& leafPackage,
        std::string* error = nullptr,
        const bytestore::ByteSourceConsumptionMode replayMode = bytestore::ByteSourceConsumptionMode::OneShot) {
        using namespace detail;

        std::uint64_t leafPackageByteSize = 0u;
        if (!ComputeLeafPackageByteSize(leafPackage, leafPackageByteSize, error)) {
            return nullptr;
        }
        PackageIdentity identity;
        if (!GeneratePackageIdentity(identity, error)) {
            return nullptr;
        }

        std::vector<std::uint8_t> headerAndDescriptors;
        headerAndDescriptors.reserve(
            leafpackagewire::HeaderByteCount() +
            leafpackagewire::FieldDescriptorByteCount() * leafPackage.fields.size());
        AppendScalar(headerAndDescriptors, leafpackagewire::kLeafPackageMagic);
        AppendScalar(headerAndDescriptors, leafpackagewire::kLeafPackageVersion);
        AppendScalar(headerAndDescriptors, identity.high);
        AppendScalar(headerAndDescriptors, identity.low);
        AppendScalar(headerAndDescriptors, static_cast<std::uint32_t>(leafPackage.fields.size()));
        AppendScalar(headerAndDescriptors, static_cast<std::uint64_t>(leafPackage.rawFieldBytes));

        std::uint64_t fieldByteOffset = static_cast<std::uint64_t>(
            leafpackagewire::HeaderByteCount() +
            leafpackagewire::FieldDescriptorByteCount() * leafPackage.fields.size());
        for (const auto& field : leafPackage.fields) {
            AppendScalar(headerAndDescriptors, static_cast<std::uint16_t>(field.type));
            AppendScalar(headerAndDescriptors, static_cast<std::uint32_t>(field.compressionType));
            AppendScalar(headerAndDescriptors, static_cast<std::uint64_t>(field.rawSize));
            AppendScalar(headerAndDescriptors, fieldByteOffset);
            AppendScalar(headerAndDescriptors, static_cast<std::uint64_t>(field.ByteSizeHint()));
            if (!validation::CheckedAddU64(
                    fieldByteOffset,
                    static_cast<std::uint64_t>(field.ByteSizeHint()),
                    fieldByteOffset,
                    "leaf package segmented field byte offset",
                    error)) {
                return nullptr;
            }
        }
        if (fieldByteOffset != leafPackageByteSize) {
            validation::AssignError(error, "leaf package byte size changed while building segmented object");
            return nullptr;
        }

        auto object = std::make_shared<bytestore::SegmentedBinaryObject>(
            std::vector<bytestore::SegmentedBinaryObject::Segment>{},
            replayMode);
        if (!object->AddSegment(
                std::make_shared<bytestore::VectorByteSource>(std::move(headerAndDescriptors)),
                error)) {
            return nullptr;
        }
        for (const auto& field : leafPackage.fields) {
            if (field.source != nullptr && !object->AddSegment(field.source, error)) {
                return nullptr;
            }
        }
        return object;
    }

    static bool ReadFromMemory(
        const std::span<const std::uint8_t> bytes,
        LeafPackage& leafPackage,
        std::string* error = nullptr) {
        using namespace detail;

        leafPackage = {};
        if (bytes.size() < leafpackagewire::HeaderByteCount()) {
            validation::AssignError(error, "buffer is too small for an leaf package header");
            return false;
        }

        std::size_t cursor = 0;
        std::uint32_t magic = 0;
        std::uint16_t version = 0;
        PackageIdentity identity;
        std::uint32_t fieldCount = 0;
        std::uint64_t rawFieldBytes = 0;
        if (!ReadScalar(bytes, cursor, magic, error) ||
            !ReadScalar(bytes, cursor, version, error) ||
            !ReadScalar(bytes, cursor, identity.high, error) ||
            !ReadScalar(bytes, cursor, identity.low, error) ||
            !ReadScalar(bytes, cursor, fieldCount, error) ||
            !ReadScalar(bytes, cursor, rawFieldBytes, error)) {
            leafPackage = {};
            return false;
        }
        if (magic != leafpackagewire::kLeafPackageMagic) {
            validation::AssignError(error, "leaf package magic does not match");
            leafPackage = {};
            return false;
        }
        if (version != leafpackagewire::kLeafPackageVersion) {
            validation::AssignError(error, "版本不符合");
            leafPackage = {};
            return false;
        }
        if (!identity.IsValid()) {
            validation::AssignError(error, "leaf package identity is invalid");
            leafPackage = {};
            return false;
        }

        const auto descBytes =
            static_cast<std::uint64_t>(leafpackagewire::FieldDescriptorByteCount()) * static_cast<std::uint64_t>(fieldCount);
        std::size_t localDescBytes = 0u;
        std::size_t fieldTableEnd = 0u;
        if (!validation::CheckedCastSizeT(descBytes, localDescBytes, "leaf package field table", error) ||
            !validation::CheckedAddSizeT(cursor, localDescBytes, fieldTableEnd, "leaf package field table", error) ||
            fieldTableEnd > bytes.size()) {
            validation::AssignError(error, "leaf package field table exceeds the input buffer");
            leafPackage = {};
            return false;
        }

        if (!leafpackagewire::CheckAddressableRawFieldBytes(rawFieldBytes, error)) {
            leafPackage = {};
            return false;
        }

        leafPackage.identity = identity;
        leafPackage.rawFieldBytes = static_cast<std::size_t>(rawFieldBytes);
        leafPackage.fields.reserve(fieldCount);
        std::uint64_t expectedFieldByteOffset =
            static_cast<std::uint64_t>(leafpackagewire::HeaderByteCount()) + descBytes;
        for (std::uint32_t fieldIndex = 0; fieldIndex < fieldCount; ++fieldIndex) {
            std::uint16_t typeValue = 0;
            std::uint32_t compressionType = 0;
            std::uint64_t fieldRawSize = 0;
            std::uint64_t fieldByteOffset = 0;
            std::uint64_t fieldByteSize = 0;
            if (!ReadScalar(bytes, cursor, typeValue, error) ||
                !ReadScalar(bytes, cursor, compressionType, error) ||
                !ReadScalar(bytes, cursor, fieldRawSize, error) ||
                !ReadScalar(bytes, cursor, fieldByteOffset, error) ||
                !ReadScalar(bytes, cursor, fieldByteSize, error)) {
                leafPackage = {};
                return false;
            }

            if (compressionType != static_cast<std::uint32_t>(EncodedFieldCompressionType::None) &&
                compressionType != static_cast<std::uint32_t>(EncodedFieldCompressionType::ZSTD)) {
                validation::AssignError(error, "unsupported encoded field compression type");
                leafPackage = {};
                return false;
            }
            std::size_t localFieldByteOffset = 0u;
            if (!leafpackagewire::CheckAddressableFieldSizes(fieldRawSize, fieldByteSize, error) ||
                !validation::CheckedCastSizeT(
                    fieldByteOffset,
                    localFieldByteOffset,
                    "leaf package field byte offset",
                    error)) {
                leafPackage = {};
                return false;
            }
            const auto resolvedCompressionType = static_cast<EncodedFieldCompressionType>(compressionType);
            if (!leafpackagewire::CheckRawAndEncodedSizeConsistency(
                    resolvedCompressionType,
                    fieldRawSize,
                    fieldByteSize,
                    error)) {
                leafPackage = {};
                return false;
            }
            if (fieldByteOffset > static_cast<std::uint64_t>(bytes.size()) ||
                fieldByteSize > static_cast<std::uint64_t>(bytes.size()) - fieldByteOffset) {
                validation::AssignError(error, "leaf package field bytes range is out of bounds");
                leafPackage = {};
                return false;
            }
            if (!leafpackagewire::CheckSequentialFieldOffset(
                    fieldByteOffset,
                    expectedFieldByteOffset,
                    error) ||
                !leafpackagewire::AdvanceSequentialFieldOffset(
                    fieldByteSize,
                    expectedFieldByteOffset,
                    error)) {
                leafPackage = {};
                return false;
            }

            LeafPackage::Field field;
            field.type = static_cast<FieldType>(typeValue);
            field.compressionType = resolvedCompressionType;
            field.rawSize = static_cast<std::size_t>(fieldRawSize);
            std::vector<std::uint8_t> fieldBytes(
                bytes.begin() + static_cast<std::ptrdiff_t>(localFieldByteOffset),
                bytes.begin() + static_cast<std::ptrdiff_t>(fieldByteOffset + fieldByteSize));
            field.source = std::make_shared<bytestore::VectorByteSource>(std::move(fieldBytes));
            leafPackage.fields.push_back(std::move(field));
        }
        if (expectedFieldByteOffset != static_cast<std::uint64_t>(bytes.size())) {
            validation::AssignError(error, "leaf package field descriptors do not cover the input buffer");
            leafPackage = {};
            return false;
        }
        if (!leafpackagewire::CheckLeafPackageRawFieldBytes(leafPackage, error)) {
            leafPackage = {};
            return false;
        }
        return true;
    }

    static bool ReadFromByteRange(
        std::shared_ptr<IByteRangeReader> reader,
        const std::uint64_t leafPackageOffset,
        const std::uint64_t leafPackageSize,
        LeafPackage& leafPackage,
        std::string* error = nullptr) {
        using namespace detail;

        leafPackage = {};
        if (reader == nullptr) {
            validation::AssignError(error, "leaf package byte range reader is missing");
            return false;
        }
        if (leafPackageSize == 0u) {
            return true;
        }
        if (leafPackageSize < leafpackagewire::HeaderByteCount()) {
            validation::AssignError(error, "byte range is too small for an leaf package header");
            return false;
        }
        if (leafPackageOffset > reader->ByteSize() ||
            leafPackageSize > reader->ByteSize() - leafPackageOffset) {
            validation::AssignError(error, "leaf package range exceeds the byte range reader");
            return false;
        }

        auto readRange = [&](
            const std::uint64_t offset,
            const std::span<std::uint8_t> output) -> bool {
            if (offset > leafPackageSize ||
                output.size() > leafPackageSize - offset ||
                !validation::CanAddU64(leafPackageOffset, offset)) {
                validation::AssignError(error, "leaf package descriptor range is outside the leaf package");
                return false;
            }
            return reader->ReadAt(leafPackageOffset + offset, output, error);
        };

        std::vector<std::uint8_t> headerBytes(leafpackagewire::HeaderByteCount(), 0u);
        if (!readRange(0u, std::span<std::uint8_t>(headerBytes.data(), headerBytes.size()))) {
            return false;
        }

        std::size_t cursor = 0;
        std::uint32_t magic = 0;
        std::uint16_t version = 0;
        PackageIdentity identity;
        std::uint32_t fieldCount = 0;
        std::uint64_t rawFieldBytes = 0;
        if (!ReadScalar(headerBytes, cursor, magic, error) ||
            !ReadScalar(headerBytes, cursor, version, error) ||
            !ReadScalar(headerBytes, cursor, identity.high, error) ||
            !ReadScalar(headerBytes, cursor, identity.low, error) ||
            !ReadScalar(headerBytes, cursor, fieldCount, error) ||
            !ReadScalar(headerBytes, cursor, rawFieldBytes, error)) {
            leafPackage = {};
            return false;
        }
        if (magic != leafpackagewire::kLeafPackageMagic) {
            validation::AssignError(error, "leaf package magic does not match");
            leafPackage = {};
            return false;
        }
        if (version != leafpackagewire::kLeafPackageVersion) {
            validation::AssignError(error, "版本不符合");
            leafPackage = {};
            return false;
        }
        if (!identity.IsValid()) {
            validation::AssignError(error, "leaf package identity is invalid");
            leafPackage = {};
            return false;
        }

        const auto descBytes =
            static_cast<std::uint64_t>(leafpackagewire::FieldDescriptorByteCount()) * static_cast<std::uint64_t>(fieldCount);
        std::size_t localDescBytes = 0u;
        if (!validation::CheckedCastSizeT(descBytes, localDescBytes, "leaf package field table", error) ||
            leafpackagewire::HeaderByteCount() > leafPackageSize ||
            descBytes > leafPackageSize - leafpackagewire::HeaderByteCount()) {
            validation::AssignError(error, "leaf package field table exceeds the leaf package range");
            leafPackage = {};
            return false;
        }
        if (!leafpackagewire::CheckAddressableRawFieldBytes(rawFieldBytes, error)) {
            leafPackage = {};
            return false;
        }

        std::vector<std::uint8_t> tableBytes(localDescBytes, 0u);
        if (!tableBytes.empty() &&
            !readRange(
                leafpackagewire::HeaderByteCount(),
                std::span<std::uint8_t>(tableBytes.data(), tableBytes.size()))) {
            leafPackage = {};
            return false;
        }

        leafPackage.identity = identity;
        leafPackage.rawFieldBytes = static_cast<std::size_t>(rawFieldBytes);
        leafPackage.fields.reserve(fieldCount);
        std::size_t tableCursor = 0u;
        std::uint64_t expectedFieldByteOffset =
            static_cast<std::uint64_t>(leafpackagewire::HeaderByteCount()) + descBytes;
        for (std::uint32_t fieldIndex = 0; fieldIndex < fieldCount; ++fieldIndex) {
            std::uint16_t typeValue = 0;
            std::uint32_t compressionType = 0;
            std::uint64_t fieldRawSize = 0;
            std::uint64_t fieldByteOffset = 0;
            std::uint64_t fieldByteSize = 0;
            if (!ReadScalar(tableBytes, tableCursor, typeValue, error) ||
                !ReadScalar(tableBytes, tableCursor, compressionType, error) ||
                !ReadScalar(tableBytes, tableCursor, fieldRawSize, error) ||
                !ReadScalar(tableBytes, tableCursor, fieldByteOffset, error) ||
                !ReadScalar(tableBytes, tableCursor, fieldByteSize, error)) {
                leafPackage = {};
                return false;
            }
            if (compressionType != static_cast<std::uint32_t>(EncodedFieldCompressionType::None) &&
                compressionType != static_cast<std::uint32_t>(EncodedFieldCompressionType::ZSTD)) {
                validation::AssignError(error, "unsupported encoded field compression type");
                leafPackage = {};
                return false;
            }
            if (!leafpackagewire::CheckAddressableFieldSizes(fieldRawSize, fieldByteSize, error)) {
                leafPackage = {};
                return false;
            }
            const auto resolvedCompressionType = static_cast<EncodedFieldCompressionType>(compressionType);
            if (!leafpackagewire::CheckRawAndEncodedSizeConsistency(
                    resolvedCompressionType,
                    fieldRawSize,
                    fieldByteSize,
                    error)) {
                leafPackage = {};
                return false;
            }
            if (fieldByteOffset > leafPackageSize || fieldByteSize > leafPackageSize - fieldByteOffset) {
                validation::AssignError(error, "leaf package field bytes range is out of bounds");
                leafPackage = {};
                return false;
            }
            if (!validation::CanAddU64(leafPackageOffset, fieldByteOffset)) {
                validation::AssignError(error, "leaf package field absolute byte offset overflows");
                leafPackage = {};
                return false;
            }
            if (!leafpackagewire::CheckSequentialFieldOffset(
                    fieldByteOffset,
                    expectedFieldByteOffset,
                    error) ||
                !leafpackagewire::AdvanceSequentialFieldOffset(
                    fieldByteSize,
                    expectedFieldByteOffset,
                    error)) {
                leafPackage = {};
                return false;
            }

            LeafPackage::Field field;
            field.type = static_cast<FieldType>(typeValue);
            field.compressionType = resolvedCompressionType;
            field.rawSize = static_cast<std::size_t>(fieldRawSize);
            field.source = std::make_shared<ByteRangeByteSource>(
                reader,
                leafPackageOffset + fieldByteOffset,
                fieldByteSize);
            leafPackage.fields.push_back(std::move(field));
        }
        if (expectedFieldByteOffset != leafPackageSize) {
            validation::AssignError(error, "leaf package field descriptors do not cover the leaf package range");
            leafPackage = {};
            return false;
        }
        if (!leafpackagewire::CheckLeafPackageRawFieldBytes(leafPackage, error)) {
            leafPackage = {};
            return false;
        }
        return true;
    }
};

} // namespace datacodec

#endif
