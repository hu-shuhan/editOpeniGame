#ifndef DATACODEC_VALIDATION_COMMON_DATACODECVALIDATION_H
#define DATACODEC_VALIDATION_COMMON_DATACODECVALIDATION_H

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
namespace datacodec::validation {

// [DC防护:通用] 统一承载无业务语义的整数和字节范围防护
inline bool AssignError(std::string* error, std::string message) {
    if (error != nullptr) {
        *error = std::move(message);
    }
    return false;
}

[[nodiscard]] inline bool CanAddU64(
    const std::uint64_t left,
    const std::uint64_t right) noexcept {
    return right <= std::numeric_limits<std::uint64_t>::max() - left;
}

[[nodiscard]] inline bool CanMulU64(
    const std::uint64_t left,
    const std::uint64_t right) noexcept {
    return left == 0u || right <= std::numeric_limits<std::uint64_t>::max() / left;
}

[[nodiscard]] inline bool CanAddSizeT(
    const std::size_t left,
    const std::size_t right) noexcept {
    return right <= std::numeric_limits<std::size_t>::max() - left;
}

[[nodiscard]] inline bool CanMulSizeT(
    const std::size_t left,
    const std::size_t right) noexcept {
    return left == 0u || right <= std::numeric_limits<std::size_t>::max() / left;
}

inline bool CheckedAddU64(
    const std::uint64_t left,
    const std::uint64_t right,
    std::uint64_t& output,
    const std::string_view label,
    std::string* error = nullptr) {
    if (!CanAddU64(left, right)) {
        return AssignError(error, std::string(label) + " overflows uint64");
    }
    output = left + right;
    return true;
}

inline bool CheckedMulU64(
    const std::uint64_t left,
    const std::uint64_t right,
    std::uint64_t& output,
    const std::string_view label,
    std::string* error = nullptr) {
    if (!CanMulU64(left, right)) {
        return AssignError(error, std::string(label) + " overflows uint64");
    }
    output = left * right;
    return true;
}

inline bool CheckedAddSizeT(
    const std::size_t left,
    const std::size_t right,
    std::size_t& output,
    const std::string_view label,
    std::string* error = nullptr) {
    if (!CanAddSizeT(left, right)) {
        return AssignError(error, std::string(label) + " overflows the local address space");
    }
    output = left + right;
    return true;
}

inline bool CheckedMulSizeT(
    const std::size_t left,
    const std::size_t right,
    std::size_t& output,
    const std::string_view label,
    std::string* error = nullptr) {
    if (!CanMulSizeT(left, right)) {
        return AssignError(error, std::string(label) + " overflows the local address space");
    }
    output = left * right;
    return true;
}

[[nodiscard]] inline std::uint64_t SaturatingAddU64(
    const std::uint64_t left,
    const std::uint64_t right) noexcept {
    if (!CanAddU64(left, right)) {
        return std::numeric_limits<std::uint64_t>::max();
    }
    return left + right;
}

[[nodiscard]] inline std::size_t SaturatingAddSizeT(
    const std::size_t left,
    const std::size_t right) noexcept {
    if (!CanAddSizeT(left, right)) {
        return std::numeric_limits<std::size_t>::max();
    }
    return left + right;
}

[[nodiscard]] inline std::uint64_t SaturatingMulU64(
    const std::uint64_t left,
    const std::uint64_t right) noexcept {
    if (!CanMulU64(left, right)) {
        return std::numeric_limits<std::uint64_t>::max();
    }
    return left * right;
}

[[nodiscard]] inline std::size_t SaturatingMulSizeT(
    const std::size_t left,
    const std::size_t right) noexcept {
    if (!CanMulSizeT(left, right)) {
        return std::numeric_limits<std::size_t>::max();
    }
    return left * right;
}

inline bool CheckedCastSizeT(
    const std::uint64_t value,
    std::size_t& output,
    const std::string_view label,
    std::string* error = nullptr) {
    if (value > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        return AssignError(error, std::string(label) + " exceeds the local address space");
    }
    output = static_cast<std::size_t>(value);
    return true;
}

inline bool ValidateRangeWithin(
    const std::uint64_t extent,
    const std::uint64_t offset,
    const std::uint64_t byteCount,
    const std::string_view label,
    std::string* error = nullptr) {
    if (offset > extent || byteCount > extent - offset) {
        return AssignError(error, std::string(label) + " range is outside the source");
    }
    return true;
}

inline bool ValidateExactConsumed(
    const std::uint64_t position,
    const std::uint64_t expected,
    const std::string_view label,
    std::string* error = nullptr) {
    if (position != expected) {
        return AssignError(error, std::string(label) + " bytes were not consumed exactly");
    }
    return true;
}

} // namespace datacodec::validation

#endif
