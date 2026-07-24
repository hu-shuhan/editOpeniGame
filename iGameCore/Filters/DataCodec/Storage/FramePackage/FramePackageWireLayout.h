#ifndef DATACODEC_STORAGE_FRAMEPACKAGE_FRAMEPACKAGEWIRELAYOUT_H
#define DATACODEC_STORAGE_FRAMEPACKAGE_FRAMEPACKAGEWIRELAYOUT_H

#include "DataCodec/Storage/Common/BinaryValueIO.h"
#include "DataCodec/Storage/FramePackage/FramePackageFormat.h"

#include <cstdint>
#include <string>
namespace datacodec {
namespace framepackagewire {

inline constexpr std::uint32_t kFramePackageMagic = 0x4D434749u;
inline constexpr std::uint16_t kFramePackageVersion = 1u;

[[nodiscard]] constexpr std::uint64_t IdentityByteSize() noexcept {
    return static_cast<std::uint64_t>(detail::WireScalarSize<std::uint64_t>() * 2u);
}

[[nodiscard]] constexpr std::uint64_t PreambleByteSize() noexcept {
    return static_cast<std::uint64_t>(
        detail::WireScalarSize<std::uint32_t>() +
        detail::WireScalarSize<std::uint16_t>()) +
        IdentityByteSize();
}

[[nodiscard]] constexpr std::uint64_t ScalarByteCount() noexcept {
    return static_cast<std::uint64_t>(detail::WireScalarSize<std::uint64_t>());
}

[[nodiscard]] inline std::uint64_t StringByteCount(const std::string& value) {
    return ScalarByteCount() + static_cast<std::uint64_t>(value.size());
}

[[nodiscard]] inline std::uint64_t MetadataByteSize(const FramePackage& container) {
    std::uint64_t size =
        PreambleByteSize() +
        detail::WireScalarSize<std::uint32_t>() +
        detail::WireScalarSize<float>() +
        detail::WireScalarSize<std::uint8_t>() +
        detail::WireScalarSize<std::uint32_t>() +
        detail::WireScalarSize<std::uint8_t>() +
        detail::WireScalarSize<std::uint32_t>() +
        StringByteCount(container.rootName) +
        detail::WireScalarSize<std::uint32_t>();
    for (const auto& branch : container.branches) {
        size += StringByteCount(branch.path);
        size += StringByteCount(branch.name);
    }
    size += detail::WireScalarSize<std::uint32_t>();
    for (const auto& leaf : container.leaves) {
        size += StringByteCount(leaf.path);
        size += StringByteCount(leaf.name);
        size += detail::WireScalarSize<std::uint32_t>();
        size += detail::WireScalarSize<std::uint8_t>();
        size += detail::WireScalarSize<std::uint64_t>() * 2u;
    }
    return size;
}

} // namespace framepackagewire
} // namespace datacodec

#endif
