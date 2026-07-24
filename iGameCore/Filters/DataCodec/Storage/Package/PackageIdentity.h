#ifndef DATACODEC_STORAGE_PACKAGE_PACKAGEIDENTITY_H
#define DATACODEC_STORAGE_PACKAGE_PACKAGEIDENTITY_H

#include "DataCodec/Runtime/Cache/DecodeCacheIdentity.h"

#include <atomic>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <exception>
#include <functional>
#include <random>
#include <string>
#include <thread>

namespace datacodec {

struct PackageIdentity {
    std::uint64_t high{0u};
    std::uint64_t low{0u};

    [[nodiscard]] bool IsValid() const noexcept { return high != 0u || low != 0u; }

    friend bool operator==(const PackageIdentity&, const PackageIdentity&) = default;
};

namespace packageidentitydetail {

[[nodiscard]] inline std::uint64_t Mix64(std::uint64_t value) noexcept {
    value ^= value >> 30u;
    value *= 0xbf58476d1ce4e5b9ull;
    value ^= value >> 27u;
    value *= 0x94d049bb133111ebull;
    value ^= value >> 31u;
    return value;
}

[[nodiscard]] inline int HexDigitValue(const unsigned char value) noexcept {
    if (value >= '0' && value <= '9') {
        return static_cast<int>(value - '0');
    }
    const auto lower = static_cast<unsigned char>(std::tolower(value));
    if (lower >= 'a' && lower <= 'f') {
        return static_cast<int>(lower - 'a') + 10;
    }
    return -1;
}

}

inline bool GeneratePackageIdentity(
    PackageIdentity& output,
    std::string* error = nullptr) {
    output = {};
    static std::atomic<std::uint64_t> sequence{0u};
    PackageIdentity entropy;
    try {
        std::random_device random;
        entropy.high = (static_cast<std::uint64_t>(random()) << 32u) ^
            static_cast<std::uint64_t>(random());
        entropy.low = (static_cast<std::uint64_t>(random()) << 32u) ^
            static_cast<std::uint64_t>(random());
    } catch (const std::exception& exception) {
        if (error != nullptr) {
            *error = std::string("package identity random entropy generation failed: ") +
                exception.what();
        }
        return false;
    } catch (...) {
        if (error != nullptr) {
            *error = "package identity random entropy generation failed";
        }
        return false;
    }
    const auto sequenceValue = sequence.fetch_add(1u, std::memory_order_relaxed) + 1u;
    const auto systemTicks = static_cast<std::uint64_t>(
        std::chrono::system_clock::now().time_since_epoch().count());
    const auto steadyTicks = static_cast<std::uint64_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    const auto threadValue = static_cast<std::uint64_t>(
        std::hash<std::thread::id>{}(std::this_thread::get_id()));
    const auto processSalt = static_cast<std::uint64_t>(
        reinterpret_cast<std::uintptr_t>(&sequence));

    output = PackageIdentity{
        .high = packageidentitydetail::Mix64(
            systemTicks ^ entropy.high ^ processSalt ^ sequenceValue),
        .low = packageidentitydetail::Mix64(
            steadyTicks ^ entropy.low ^ threadValue ^
            (sequenceValue * 0x9e3779b97f4a7c15ull)),
    };
    if (!output.IsValid()) {
        if (error != nullptr) {
            *error = "package identity generation produced an invalid identity";
        }
        return false;
    }
    if (error != nullptr) {
        error->clear();
    }
    return true;
}

[[nodiscard]] inline std::string PackageIdentityToHex(const PackageIdentity identity) {
    constexpr char kHexDigits[] = "0123456789abcdef";
    std::string output(32u, '0');
    const auto appendHalf = [&](const std::uint64_t value, const std::size_t offset) {
        for (std::size_t index = 0u; index < 16u; ++index) {
            const auto shift = static_cast<unsigned>((15u - index) * 4u);
            output[offset + index] = kHexDigits[(value >> shift) & 0x0fu];
        }
    };
    appendHalf(identity.high, 0u);
    appendHalf(identity.low, 16u);
    return output;
}

[[nodiscard]] inline bool TryParsePackageIdentityHex(
    const std::string& text,
    PackageIdentity& identity) noexcept {
    identity = {};
    if (text.size() != 32u) {
        return false;
    }
    std::uint64_t halves[2]{0u, 0u};
    for (std::size_t halfIndex = 0u; halfIndex < 2u; ++halfIndex) {
        for (std::size_t digitIndex = 0u; digitIndex < 16u; ++digitIndex) {
            const auto value = packageidentitydetail::HexDigitValue(
                static_cast<unsigned char>(text[halfIndex * 16u + digitIndex]));
            if (value < 0) {
                return false;
            }
            halves[halfIndex] = (halves[halfIndex] << 4u) |
                static_cast<std::uint64_t>(value);
        }
    }
    identity = PackageIdentity{.high = halves[0], .low = halves[1]};
    return identity.IsValid();
}

[[nodiscard]] inline DecodeSourceIdentity MakePackageDecodeSourceIdentity(
    const PackageIdentity identity,
    const std::uint16_t version,
    const std::uint64_t byteSize) {
    if (!identity.IsValid()) {
        return {};
    }
    return DecodeSourceIdentity{
        .stableId = "igc-package:" + PackageIdentityToHex(identity),
        .revision = "igdc-v" + std::to_string(version) + ":bytes:" + std::to_string(byteSize),
    };
}

}

#endif
