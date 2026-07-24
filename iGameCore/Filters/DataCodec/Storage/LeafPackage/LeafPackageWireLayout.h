#ifndef DATACODEC_STORAGE_LEAFPACKAGE_LEAFPACKAGEWIRELAYOUT_H
#define DATACODEC_STORAGE_LEAFPACKAGE_LEAFPACKAGEWIRELAYOUT_H

#include <cstddef>
#include <cstdint>
namespace datacodec {
namespace leafpackagewire {

inline constexpr std::uint32_t kLeafPackageMagic = 0x4C434749u;
inline constexpr std::uint16_t kLeafPackageVersion = 1u;

[[nodiscard]] constexpr std::size_t IdentityByteCount() noexcept {
    return sizeof(std::uint64_t) * 2u;
}

[[nodiscard]] constexpr std::size_t PreambleByteCount() noexcept {
    return sizeof(std::uint32_t) + sizeof(std::uint16_t) + IdentityByteCount();
}

[[nodiscard]] constexpr std::size_t HeaderByteCount() noexcept {
    return PreambleByteCount() + sizeof(std::uint32_t) + sizeof(std::uint64_t);
}

[[nodiscard]] constexpr std::size_t FieldDescriptorByteCount() noexcept {
    return sizeof(std::uint16_t) + sizeof(std::uint32_t) + sizeof(std::uint64_t) * 3u;
}

[[nodiscard]] constexpr std::size_t ComputeRawLeafPackageSize(
    const std::size_t fieldCount,
    const std::size_t rawFieldBytes) noexcept {
    return HeaderByteCount() + FieldDescriptorByteCount() * fieldCount + rawFieldBytes;
}

} // namespace leafpackagewire
} // namespace datacodec

#endif
