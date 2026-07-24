#ifndef DATACODEC_TEST_DATA_DATACODECMALFORMEDMUTATOR_H
#define DATACODEC_TEST_DATA_DATACODECMALFORMEDMUTATOR_H

#include "DataCodec/Storage/Common/BinaryValueIO.h"
#include "DataCodec/Storage/LeafPackage/LeafPackage.h"
#include "DataCodec/Storage/LeafPackage/LeafPackageWireLayout.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>
namespace datacodec::test {

enum class LeafPackageMutationKind {
    Empty,
    TruncatedDescriptor,
    UnsupportedCompression,
    NonSequentialOffset,
    UncoveredPayload,
    OutOfBoundsPayload,
    MissingSource,
    RawSizeMismatch,
};

[[nodiscard]] inline const char* LeafPackageMutationKindName(
    const LeafPackageMutationKind mutationKind) noexcept {
    switch (mutationKind) {
        case LeafPackageMutationKind::Empty:
            return "Empty";
        case LeafPackageMutationKind::TruncatedDescriptor:
            return "TruncatedDescriptor";
        case LeafPackageMutationKind::UnsupportedCompression:
            return "UnsupportedCompression";
        case LeafPackageMutationKind::NonSequentialOffset:
            return "NonSequentialOffset";
        case LeafPackageMutationKind::UncoveredPayload:
            return "UncoveredPayload";
        case LeafPackageMutationKind::OutOfBoundsPayload:
            return "OutOfBoundsPayload";
        case LeafPackageMutationKind::MissingSource:
            return "MissingSource";
        case LeafPackageMutationKind::RawSizeMismatch:
            return "RawSizeMismatch";
    }
    return "Unknown";
}

inline void AppendMalformedHeader(
    std::vector<std::uint8_t>& bytes,
    const std::uint32_t fieldCount,
    const std::uint64_t rawFieldBytes) {
    detail::AppendScalar(bytes, leafpackagewire::kLeafPackageMagic);
    detail::AppendScalar(bytes, leafpackagewire::kLeafPackageVersion);
    detail::AppendScalar(bytes, std::uint64_t{0x1122334455667788ull});
    detail::AppendScalar(bytes, std::uint64_t{0x8877665544332211ull});
    detail::AppendScalar(bytes, fieldCount);
    detail::AppendScalar(bytes, rawFieldBytes);
}

inline void AppendMalformedDescriptor(
    std::vector<std::uint8_t>& bytes,
    const FieldType type,
    const std::uint32_t compressionType,
    const std::uint64_t rawSize,
    const std::uint64_t byteOffset,
    const std::uint64_t byteSize) {
    detail::AppendScalar(bytes, static_cast<std::uint16_t>(type));
    detail::AppendScalar(bytes, compressionType);
    detail::AppendScalar(bytes, rawSize);
    detail::AppendScalar(bytes, byteOffset);
    detail::AppendScalar(bytes, byteSize);
}

[[nodiscard]] inline std::uint64_t ComputeMalformedHeaderRawFieldBytes(
    const std::uint32_t fieldCount,
    const std::uint64_t rawPayloadBytes) {
    return static_cast<std::uint64_t>(
        leafpackagewire::ComputeRawLeafPackageSize(
            static_cast<std::size_t>(fieldCount),
            static_cast<std::size_t>(rawPayloadBytes)));
}

[[nodiscard]] inline std::vector<std::uint8_t> BuildMalformedLeafPackageBytes(
    const FieldType fieldType,
    const LeafPackageMutationKind mutationKind) {
    const auto headerBytes = static_cast<std::uint64_t>(leafpackagewire::HeaderByteCount());
    const auto descriptorBytes = static_cast<std::uint64_t>(leafpackagewire::FieldDescriptorByteCount());
    const auto payloadOffset = headerBytes + descriptorBytes;

    if (mutationKind == LeafPackageMutationKind::Empty) {
        return {};
    }

    if (mutationKind == LeafPackageMutationKind::TruncatedDescriptor) {
        std::vector<std::uint8_t> bytes;
        AppendMalformedHeader(bytes, 1u, ComputeMalformedHeaderRawFieldBytes(1u, 0u));
        return bytes;
    }

    std::uint32_t compressionType = static_cast<std::uint32_t>(EncodedFieldCompressionType::None);
    std::uint64_t byteOffset = payloadOffset;
    std::uint64_t rawSize = 1u;
    std::uint64_t byteSize = 1u;
    std::vector<std::uint8_t> payload{0x11u};

    switch (mutationKind) {
        case LeafPackageMutationKind::UnsupportedCompression:
            compressionType = 999u;
            rawSize = 0u;
            byteSize = 0u;
            payload.clear();
            break;
        case LeafPackageMutationKind::NonSequentialOffset:
            byteOffset = payloadOffset + 1u;
            payload = {0x11u, 0x22u};
            break;
        case LeafPackageMutationKind::UncoveredPayload:
            payload = {0x11u, 0x22u};
            break;
        case LeafPackageMutationKind::OutOfBoundsPayload:
            rawSize = 8u;
            byteSize = 8u;
            payload = {0x11u};
            break;
        case LeafPackageMutationKind::RawSizeMismatch:
            payload = {0x11u};
            break;
        case LeafPackageMutationKind::Empty:
        case LeafPackageMutationKind::TruncatedDescriptor:
        case LeafPackageMutationKind::MissingSource:
            break;
    }

    std::vector<std::uint8_t> bytes;
    const auto validHeaderRawFieldBytes = ComputeMalformedHeaderRawFieldBytes(1u, rawSize);
    const auto headerRawFieldBytes = mutationKind == LeafPackageMutationKind::RawSizeMismatch
        ? rawSize
        : validHeaderRawFieldBytes;
    AppendMalformedHeader(bytes, 1u, headerRawFieldBytes);
    AppendMalformedDescriptor(bytes, fieldType, compressionType, rawSize, byteOffset, byteSize);
    bytes.insert(bytes.end(), payload.begin(), payload.end());
    return bytes;
}

[[nodiscard]] inline LeafPackage BuildMissingSourceLeafPackage(const FieldType fieldType) {
    LeafPackage package;
    package.path = "/0";
    package.rawFieldBytes = leafpackagewire::ComputeRawLeafPackageSize(1u, 4u);
    package.fields.push_back(LeafPackage::Field{
        .type = fieldType,
        .compressionType = EncodedFieldCompressionType::None,
        .rawSize = 4u,
        .source = nullptr,
    });
    return package;
}

} // namespace datacodec::test

#endif
