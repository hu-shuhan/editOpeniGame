#ifndef DATACODEC_STORAGE_PACKAGE_PACKAGEIDENTITY_H
#define DATACODEC_STORAGE_PACKAGE_PACKAGEIDENTITY_H

#include "DataCodec/Runtime/Cache/DecodeCacheIdentity.h"

#include <algorithm>
#include <bit>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

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

[[nodiscard]] inline PackageIdentity MakeSampleDigest(
    const std::span<const std::uint8_t> bytes) noexcept;

}

class PackageIdentityBuilder final {
public:
    explicit PackageIdentityBuilder(const std::string_view domain = {}) noexcept {
        if (!domain.empty()) {
            AddString(domain);
        }
    }

    void AddUnsigned(const std::uint64_t value) noexcept {
        ++m_tokenCount;
        const auto tokenSalt = m_tokenCount * 0x9e3779b97f4a7c15ull;
        m_high = packageidentitydetail::Mix64(m_high ^ value ^ tokenSalt);
        m_low = packageidentitydetail::Mix64(
            m_low + std::rotl(value ^ tokenSalt, static_cast<int>(m_tokenCount & 63u)));
    }

    void AddIdentity(const PackageIdentity identity) noexcept {
        AddUnsigned(identity.high);
        AddUnsigned(identity.low);
    }

    void AddBytes(const std::span<const std::uint8_t> bytes) noexcept {
        AddUnsigned(static_cast<std::uint64_t>(bytes.size()));
        std::size_t offset = 0u;
        while (offset < bytes.size()) {
            std::uint64_t word = 0u;
            const auto currentBytes = std::min<std::size_t>(sizeof(word), bytes.size() - offset);
            for (std::size_t byteIndex = 0u; byteIndex < currentBytes; ++byteIndex) {
                word |= static_cast<std::uint64_t>(bytes[offset + byteIndex]) << (byteIndex * 8u);
            }
            AddUnsigned(word ^ packageidentitydetail::Mix64(static_cast<std::uint64_t>(offset)));
            offset += currentBytes;
        }
    }

    void AddString(const std::string_view text) noexcept {
        AddBytes(std::span<const std::uint8_t>(
            reinterpret_cast<const std::uint8_t*>(text.data()),
            text.size()));
    }

    [[nodiscard]] PackageIdentity Finish() const noexcept {
        auto output = PackageIdentity{
            .high = packageidentitydetail::Mix64(
                m_high ^ (m_tokenCount * 0xd6e8feb86659fd93ull)),
            .low = packageidentitydetail::Mix64(
                m_low ^ std::rotl(m_high, 29) ^ (m_tokenCount * 0xa0761d6478bd642full)),
        };
        if (!output.IsValid()) {
            output.low = 0xe7037ed1a0b428dbull;
        }
        return output;
    }

private:
    std::uint64_t m_high{0x243f6a8885a308d3ull};
    std::uint64_t m_low{0x13198a2e03707344ull};
    std::uint64_t m_tokenCount{0u};
};

inline constexpr std::size_t kPackageIdentitySampleWindowBytes = 4u * 1024u;
inline constexpr std::uint64_t kPackageIdentitySampleStrideBytes = 256ull * 1024ull * 1024ull;

struct PackageIdentitySampleRange {
    std::uint64_t offset{0u};
    std::size_t byteCount{0u};

    friend bool operator==(const PackageIdentitySampleRange&, const PackageIdentitySampleRange&) = default;
};

[[nodiscard]] inline std::vector<PackageIdentitySampleRange>
BuildPackageIdentitySampleRanges(const std::uint64_t byteSize) {
    std::vector<PackageIdentitySampleRange> ranges;
    if (byteSize == 0u) {
        return ranges;
    }
    const auto sampleBytes = static_cast<std::size_t>(std::min<std::uint64_t>(
        byteSize,
        static_cast<std::uint64_t>(kPackageIdentitySampleWindowBytes)));
    ranges.push_back({.offset = 0u, .byteCount = sampleBytes});
    for (std::uint64_t offset = kPackageIdentitySampleStrideBytes;
         offset < byteSize;) {
        ranges.push_back({
            .offset = offset,
            .byteCount = static_cast<std::size_t>(std::min<std::uint64_t>(
                byteSize - offset,
                static_cast<std::uint64_t>(kPackageIdentitySampleWindowBytes))),
        });
        if (offset > std::numeric_limits<std::uint64_t>::max() - kPackageIdentitySampleStrideBytes) {
            break;
        }
        offset += kPackageIdentitySampleStrideBytes;
    }
    if (byteSize > static_cast<std::uint64_t>(sampleBytes)) {
        ranges.push_back({
            .offset = byteSize - static_cast<std::uint64_t>(sampleBytes),
            .byteCount = sampleBytes,
        });
    }
    std::sort(
        ranges.begin(),
        ranges.end(),
        [](const auto& left, const auto& right) { return left.offset < right.offset; });
    ranges.erase(
        std::unique(
            ranges.begin(),
            ranges.end(),
            [](const auto& left, const auto& right) {
                return left.offset == right.offset && left.byteCount == right.byteCount;
            }),
        ranges.end());
    return ranges;
}

template<typename ReadRange>
inline bool ComputeSparsePackageContentIdentity(
    const std::uint64_t byteSize,
    ReadRange&& readRange,
    PackageIdentity& output,
    std::string* error = nullptr) {
    PackageIdentityBuilder builder("igdc.sparse-content.v1");
    builder.AddUnsigned(byteSize);
    for (const auto& range : BuildPackageIdentitySampleRanges(byteSize)) {
        std::vector<std::uint8_t> bytes(range.byteCount, 0u);
        if (!readRange(
                range.offset,
                std::span<std::uint8_t>(bytes.data(), bytes.size()),
                error)) {
            output = {};
            return false;
        }
        builder.AddUnsigned(range.offset);
        builder.AddUnsigned(static_cast<std::uint64_t>(range.byteCount));
        builder.AddIdentity(packageidentitydetail::MakeSampleDigest(bytes));
    }
    output = builder.Finish();
    if (error != nullptr) {
        error->clear();
    }
    return true;
}

class StreamingPackageContentSampler final {
public:
    void Append(const std::span<const std::uint8_t> bytes) {
        if (bytes.empty()) {
            return;
        }
        const auto rangeBegin = m_byteSize;
        const auto rangeEnd = rangeBegin + static_cast<std::uint64_t>(bytes.size());
        CapturePrefix(rangeBegin, bytes);
        CaptureInterior(rangeBegin, rangeEnd, bytes);
        CaptureTail(bytes);
        m_byteSize = rangeEnd;
    }

    [[nodiscard]] std::uint64_t ByteSize() const noexcept { return m_byteSize; }

    [[nodiscard]] PackageIdentity Finish() const {
        auto samples = m_samples;
        if (!m_prefix.empty()) {
            samples.push_back(MakeSample(0u, m_prefix));
        }
        if (!m_activeInterior.empty()) {
            samples.push_back(MakeSample(m_nextInteriorOffset, m_activeInterior));
        }
        if (!m_tail.empty()) {
            samples.push_back(MakeSample(
                m_byteSize - static_cast<std::uint64_t>(m_tail.size()),
                m_tail));
        }
        std::sort(
            samples.begin(),
            samples.end(),
            [](const auto& left, const auto& right) { return left.offset < right.offset; });
        samples.erase(
            std::unique(
                samples.begin(),
                samples.end(),
                [](const auto& left, const auto& right) {
                    return left.offset == right.offset && left.byteCount == right.byteCount;
                }),
            samples.end());

        PackageIdentityBuilder builder("igdc.sparse-content.v1");
        builder.AddUnsigned(m_byteSize);
        for (const auto& sample : samples) {
            builder.AddUnsigned(sample.offset);
            builder.AddUnsigned(static_cast<std::uint64_t>(sample.byteCount));
            builder.AddIdentity(sample.digest);
        }
        return builder.Finish();
    }

private:
    struct Sample {
        std::uint64_t offset{0u};
        std::size_t byteCount{0u};
        PackageIdentity digest;
    };

    [[nodiscard]] static Sample MakeSample(
        const std::uint64_t offset,
        const std::vector<std::uint8_t>& bytes) {
        return Sample{
            .offset = offset,
            .byteCount = bytes.size(),
            .digest = packageidentitydetail::MakeSampleDigest(bytes),
        };
    }

    void CapturePrefix(
        const std::uint64_t rangeBegin,
        const std::span<const std::uint8_t> bytes) {
        if (rangeBegin >= kPackageIdentitySampleWindowBytes ||
            m_prefix.size() >= kPackageIdentitySampleWindowBytes) {
            return;
        }
        const auto available = kPackageIdentitySampleWindowBytes - m_prefix.size();
        const auto currentBytes = std::min<std::size_t>(available, bytes.size());
        m_prefix.insert(m_prefix.end(), bytes.begin(), bytes.begin() + currentBytes);
    }

    void CaptureInterior(
        const std::uint64_t rangeBegin,
        const std::uint64_t rangeEnd,
        const std::span<const std::uint8_t> bytes) {
        while (m_nextInteriorOffset < rangeEnd) {
            const auto sampleEnd = m_nextInteriorOffset +
                static_cast<std::uint64_t>(kPackageIdentitySampleWindowBytes);
            if (rangeEnd <= m_nextInteriorOffset) {
                return;
            }
            if (rangeBegin >= sampleEnd) {
                m_nextInteriorOffset += kPackageIdentitySampleStrideBytes;
                m_activeInterior.clear();
                continue;
            }
            const auto overlapBegin = std::max(rangeBegin, m_nextInteriorOffset);
            const auto overlapEnd = std::min(rangeEnd, sampleEnd);
            if (overlapBegin < overlapEnd) {
                const auto inputOffset = static_cast<std::size_t>(overlapBegin - rangeBegin);
                const auto currentBytes = static_cast<std::size_t>(overlapEnd - overlapBegin);
                m_activeInterior.insert(
                    m_activeInterior.end(),
                    bytes.begin() + inputOffset,
                    bytes.begin() + inputOffset + currentBytes);
            }
            if (m_activeInterior.size() < kPackageIdentitySampleWindowBytes) {
                return;
            }
            m_samples.push_back(MakeSample(m_nextInteriorOffset, m_activeInterior));
            m_activeInterior.clear();
            m_nextInteriorOffset += kPackageIdentitySampleStrideBytes;
        }
    }

    void CaptureTail(const std::span<const std::uint8_t> bytes) {
        if (bytes.size() >= kPackageIdentitySampleWindowBytes) {
            m_tail.assign(
                bytes.end() - static_cast<std::ptrdiff_t>(kPackageIdentitySampleWindowBytes),
                bytes.end());
            return;
        }
        const auto combinedBytes = m_tail.size() + bytes.size();
        if (combinedBytes > kPackageIdentitySampleWindowBytes) {
            m_tail.erase(
                m_tail.begin(),
                m_tail.begin() + static_cast<std::ptrdiff_t>(
                    combinedBytes - kPackageIdentitySampleWindowBytes));
        }
        m_tail.insert(m_tail.end(), bytes.begin(), bytes.end());
    }

    std::vector<std::uint8_t> m_prefix;
    std::vector<std::uint8_t> m_tail;
    std::vector<std::uint8_t> m_activeInterior;
    std::vector<Sample> m_samples;
    std::uint64_t m_nextInteriorOffset{kPackageIdentitySampleStrideBytes};
    std::uint64_t m_byteSize{0u};
};

namespace packageidentitydetail {

[[nodiscard]] inline PackageIdentity MakeSampleDigest(
    const std::span<const std::uint8_t> bytes) noexcept {
    PackageIdentityBuilder builder("igdc.sample-window.v1");
    builder.AddBytes(bytes);
    return builder.Finish();
}

}

[[nodiscard]] inline bool TryReadPackageIdentityPrefix(
    const std::span<const std::uint8_t> bytes,
    PackageIdentity& identity) noexcept {
    constexpr std::size_t kIdentityOffset = sizeof(std::uint32_t) + sizeof(std::uint16_t);
    constexpr std::size_t kIdentityBytes = sizeof(std::uint64_t) * 2u;
    identity = {};
    if (bytes.size() < kIdentityOffset + kIdentityBytes) {
        return false;
    }
    const auto readHalf = [&](const std::size_t offset) {
        std::uint64_t value = 0u;
        for (std::size_t byteIndex = 0u; byteIndex < sizeof(value); ++byteIndex) {
            value |= static_cast<std::uint64_t>(bytes[offset + byteIndex]) << (byteIndex * 8u);
        }
        return value;
    };
    identity.high = readHalf(kIdentityOffset);
    identity.low = readHalf(kIdentityOffset + sizeof(std::uint64_t));
    return identity.IsValid();
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
