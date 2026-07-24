#ifndef DATACODEC_STORAGE_PACKAGE_PACKAGEBINARYHEADER_H
#define DATACODEC_STORAGE_PACKAGE_PACKAGEBINARYHEADER_H

#include "DataCodec/Storage/ByteIO/ByteRange.h"
#include "DataCodec/Storage/Common/BinaryValueIO.h"
#include "DataCodec/Storage/FramePackage/FramePackageWireLayout.h"
#include "DataCodec/Storage/LeafPackage/LeafPackageWireLayout.h"
#include "DataCodec/Storage/Package/PackageIdentity.h"

#include <array>
#include <cstdint>
#include <span>
#include <string>

namespace datacodec {

enum class PackageBinaryFormat {
    Unknown,
    LeafPackage,
    FramePackage,
};

struct PackageInspection {
    PackageBinaryFormat format{PackageBinaryFormat::Unknown};
    std::uint32_t magic{0u};
    std::uint16_t version{0u};
    PackageIdentity identity;
    std::uint64_t byteSize{0u};
    DecodeSourceIdentity sourceIdentity;
};

inline bool InspectPackage(
    IByteRangeReader& reader,
    PackageInspection& inspection,
    std::string* error = nullptr) {
    constexpr std::size_t kFixedHeaderByteCount =
        sizeof(std::uint32_t) + sizeof(std::uint16_t) + sizeof(std::uint64_t) * 2u;
    inspection = {};
    inspection.byteSize = reader.ByteSize();
    if (inspection.byteSize < kFixedHeaderByteCount) {
        if (error != nullptr) {
            *error = "文件头不完整";
        }
        return false;
    }

    std::array<std::uint8_t, kFixedHeaderByteCount> bytes{};
    if (!reader.ReadAt(
            0u,
            std::span<std::uint8_t>(bytes),
            error)) {
        return false;
    }
    std::size_t cursor = 0u;
    if (!detail::ReadScalar(
            std::span<const std::uint8_t>(bytes),
            cursor,
            inspection.magic,
            error) ||
        !detail::ReadScalar(
            std::span<const std::uint8_t>(bytes),
            cursor,
            inspection.version,
            error) ||
        !detail::ReadScalar(
            std::span<const std::uint8_t>(bytes),
            cursor,
            inspection.identity.high,
            error) ||
        !detail::ReadScalar(
            std::span<const std::uint8_t>(bytes),
            cursor,
            inspection.identity.low,
            error)) {
        inspection = {};
        if (error != nullptr) {
            *error = "文件头不完整";
        }
        return false;
    }

    if (inspection.magic == leafpackagewire::kLeafPackageMagic) {
        inspection.format = PackageBinaryFormat::LeafPackage;
    } else if (inspection.magic == framepackagewire::kFramePackageMagic) {
        inspection.format = PackageBinaryFormat::FramePackage;
    } else {
        if (error != nullptr) {
            *error = "格式错误";
        }
        return false;
    }
    const auto expectedVersion = inspection.format == PackageBinaryFormat::LeafPackage
        ? leafpackagewire::kLeafPackageVersion
        : framepackagewire::kFramePackageVersion;
    if (inspection.version != expectedVersion) {
        if (error != nullptr) {
            *error = "版本不符合";
        }
        return false;
    }
    if (!inspection.identity.IsValid()) {
        if (error != nullptr) {
            *error = "package identity 错误";
        }
        return false;
    }
    inspection.sourceIdentity = MakePackageDecodeSourceIdentity(
        inspection.identity,
        inspection.version,
        inspection.byteSize);
    if (!inspection.sourceIdentity.IsStable()) {
        if (error != nullptr) {
            *error = "package identity 错误";
        }
        return false;
    }
    if (error != nullptr) {
        error->clear();
    }
    return true;
}

} // namespace datacodec

#endif
