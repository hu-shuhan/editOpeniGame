#ifndef DATACODEC_RUNTIME_CACHE_DECODECACHEIDENTITY_H
#define DATACODEC_RUNTIME_CACHE_DECODECACHEIDENTITY_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

namespace datacodec {

struct DecodeSourceIdentity {
    std::string stableId;
    std::string revision;

    [[nodiscard]] bool IsStable() const noexcept { return !stableId.empty(); }

    friend bool operator==(const DecodeSourceIdentity&, const DecodeSourceIdentity&) = default;
};

struct DecodeReferenceKey {
    DecodeSourceIdentity source;
    std::uint32_t keyFrameIndex{0u};

    friend bool operator==(const DecodeReferenceKey&, const DecodeReferenceKey&) = default;
};

struct DecodedFrameKey {
    DecodeSourceIdentity source;
    std::uint32_t frameIndex{0u};
    std::string resultIdentity;

    friend bool operator==(const DecodedFrameKey&, const DecodedFrameKey&) = default;
};

inline void HashDecodeCacheValue(std::size_t& seed, const std::size_t value) noexcept {
    seed ^= value + 0x9e3779b97f4a7c15ull + (seed << 6u) + (seed >> 2u);
}

struct DecodeSourceIdentityHash {
    [[nodiscard]] std::size_t operator()(const DecodeSourceIdentity& value) const noexcept {
        std::size_t seed = std::hash<std::string>{}(value.stableId);
        HashDecodeCacheValue(seed, std::hash<std::string>{}(value.revision));
        return seed;
    }
};

struct DecodeReferenceKeyHash {
    [[nodiscard]] std::size_t operator()(const DecodeReferenceKey& value) const noexcept {
        std::size_t seed = DecodeSourceIdentityHash{}(value.source);
        HashDecodeCacheValue(seed, std::hash<std::uint32_t>{}(value.keyFrameIndex));
        return seed;
    }
};

struct DecodedFrameKeyHash {
    [[nodiscard]] std::size_t operator()(const DecodedFrameKey& value) const noexcept {
        std::size_t seed = DecodeSourceIdentityHash{}(value.source);
        HashDecodeCacheValue(seed, std::hash<std::uint32_t>{}(value.frameIndex));
        HashDecodeCacheValue(seed, std::hash<std::string>{}(value.resultIdentity));
        return seed;
    }
};

} // namespace datacodec

#endif
