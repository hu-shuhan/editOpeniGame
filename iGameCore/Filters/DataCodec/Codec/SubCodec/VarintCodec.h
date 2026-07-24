#ifndef DATACODEC_CODEC_SUBCODEC_VARINTCODEC_H
#define DATACODEC_CODEC_SUBCODEC_VARINTCODEC_H

#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <vector>
namespace datacodec::codec {

inline std::uint64_t ZigZagEncode64(const std::int64_t value) {
    return (static_cast<std::uint64_t>(value) << 1u) ^ static_cast<std::uint64_t>(value >> 63u);
}

inline std::int64_t ZigZagDecode64(const std::uint64_t value) {
    return static_cast<std::int64_t>(value >> 1u) ^ -static_cast<std::int64_t>(value & 1u);
}

inline std::size_t MeasureVarint64(std::uint64_t value) {
    std::size_t size = 1;
    while (value >= 0x80u) {
        value >>= 7u;
        ++size;
    }
    return size;
}

inline void EncodeVarint64(std::vector<std::uint8_t>& bytes, std::uint64_t value) {
    do {
        auto byte = static_cast<std::uint8_t>(value & 0x7fu);
        value >>= 7u;
        if (value != 0u) {
            byte |= 0x80u;
        }
        bytes.push_back(byte);
    } while (value != 0u);
}

inline void EncodeVarint64(unsigned char*& cursor, std::uint64_t value) {
    do {
        auto byte = static_cast<std::uint8_t>(value & 0x7fu);
        value >>= 7u;
        if (value != 0u) {
            byte |= 0x80u;
        }
        *cursor++ = byte;
    } while (value != 0u);
}

inline void EncodeVarint32(std::vector<std::uint8_t>& bytes, const std::uint32_t value) {
    EncodeVarint64(bytes, value);
}

inline void EncodeVarint32(unsigned char*& cursor, const std::uint32_t value) {
    EncodeVarint64(cursor, value);
}

inline bool DecodeVarint64(
    const std::span<const std::uint8_t> bytes,
    std::size_t& cursor,
    std::uint64_t& value,
    std::string* error = nullptr) {
    value = 0u;
    std::uint32_t shift = 0u;
    for (std::uint32_t group = 0u; group < 10u; ++group) {
        if (cursor >= bytes.size()) {
            return validation::AssignError(error, "unexpected end of buffer while reading uint64 varint");
        }

        const auto byte = bytes[cursor++];
        if (group == 9u && (byte & 0x7eu) != 0u) {
            return validation::AssignError(error, "uint64 varint exceeds range");
        }

        value |= static_cast<std::uint64_t>(byte & 0x7fu) << shift;
        if ((byte & 0x80u) == 0u) {
            return true;
        }
        shift += 7u;
    }

    return validation::AssignError(error, "uint64 varint is too long");
}

inline bool DecodeVarint64(
    const unsigned char*& cursor,
    const unsigned char* end,
    std::uint64_t& value,
    std::string* error = nullptr) {
    value = 0u;
    std::uint32_t shift = 0u;
    for (std::uint32_t group = 0u; group < 10u; ++group) {
        if (cursor >= end) {
            return validation::AssignError(error, "unexpected end of buffer while reading uint64 varint");
        }

        const auto byte = *cursor++;
        if (group == 9u && (byte & 0x7eu) != 0u) {
            return validation::AssignError(error, "uint64 varint exceeds range");
        }

        value |= static_cast<std::uint64_t>(byte & 0x7fu) << shift;
        if ((byte & 0x80u) == 0u) {
            return true;
        }
        shift += 7u;
    }

    return validation::AssignError(error, "uint64 varint is too long");
}

inline bool DecodeVarint32(
    const std::span<const std::uint8_t> bytes,
    std::size_t& cursor,
    std::uint32_t& value,
    std::string* error = nullptr) {
    std::uint64_t rawValue = 0u;
    if (!DecodeVarint64(bytes, cursor, rawValue, error)) {
        return false;
    }
    if (rawValue > std::numeric_limits<std::uint32_t>::max()) {
        return validation::AssignError(error, "uint32 varint exceeds range");
    }
    value = static_cast<std::uint32_t>(rawValue);
    return true;
}

inline bool DecodeVarint32(
    const unsigned char*& cursor,
    const unsigned char* end,
    std::uint32_t& value,
    std::string* error = nullptr) {
    std::uint64_t rawValue = 0u;
    if (!DecodeVarint64(cursor, end, rawValue, error)) {
        return false;
    }
    if (rawValue > std::numeric_limits<std::uint32_t>::max()) {
        return validation::AssignError(error, "uint32 varint exceeds range");
    }
    value = static_cast<std::uint32_t>(rawValue);
    return true;
}

} // namespace datacodec::codec

#endif
