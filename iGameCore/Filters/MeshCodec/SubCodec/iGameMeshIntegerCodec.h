#ifndef MeshCodecIntegerCodec_h
#define MeshCodecIntegerCodec_h

#include "iGameMacro.h"
#include "iGameType.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>
#include <vector>

IGAME_NAMESPACE_BEGIN

class MeshIntegerCodec {
public:
    template<typename T>
    static bool Encode(std::vector<unsigned char>& dest, const std::vector<T>& source,
                       IGsize elementCount, int dimension) {
        static_assert(std::is_integral_v<T> && !std::is_same_v<T, bool>,
                      "MeshIntegerCodec requires an integer type");

        if (dimension <= 0) { return false; }
        if (elementCount > static_cast<IGsize>(std::numeric_limits<size_t>::max())) { return false; }
        size_t valueCount = 0;
        if (!ValueCount(elementCount, dimension, valueCount)) { return false; }
        if (source.size() != valueCount) { return false; }

        dest.clear();
        for (int comp = 0; comp < dimension; ++comp) {
            if (!EncodeComponent<T>(dest, source, static_cast<size_t>(elementCount), dimension, comp)) {
                return false;
            }
        }
        return true;
    }

    template<typename T>
    static bool Decode(std::vector<T>& dest, const std::vector<unsigned char>& source,
                       IGsize elementCount, int dimension) {
        static_assert(std::is_integral_v<T> && !std::is_same_v<T, bool>,
                      "MeshIntegerCodec requires an integer type");

        if (dimension <= 0) { return false; }
        if (elementCount > static_cast<IGsize>(std::numeric_limits<size_t>::max())) { return false; }

        const size_t elementCountSize = static_cast<size_t>(elementCount);
        size_t valueCount = 0;
        if (!ValueCount(elementCount, dimension, valueCount)) { return false; }
        dest.assign(valueCount, T{});

        size_t cursor = 0;
        for (int comp = 0; comp < dimension; ++comp) {
            if (!DecodeComponent<T>(dest, source, cursor, elementCountSize, dimension, comp)) {
                return false;
            }
        }

        return cursor == source.size();
    }

private:
    static constexpr uint64_t kLiteralToken = 0;
    static constexpr uint64_t kRunToken = 1;
    static constexpr size_t kRunThreshold = 3;

    static bool ValueCount(IGsize elementCount, int dimension, size_t& valueCount) {
        if (dimension <= 0 || elementCount > static_cast<IGsize>(std::numeric_limits<size_t>::max())) {
            return false;
        }
        const auto elementCountSize = static_cast<size_t>(elementCount);
        const auto dimensionSize = static_cast<size_t>(dimension);
        if (dimensionSize != 0 && elementCountSize > std::numeric_limits<size_t>::max() / dimensionSize) {
            return false;
        }
        valueCount = elementCountSize * dimensionSize;
        return true;
    }

    static void EncodeVarint(std::vector<unsigned char>& dest, uint64_t value) {
        do {
            unsigned char byte = static_cast<unsigned char>(value & 0x7fu);
            value >>= 7u;
            if (value != 0) { byte = static_cast<unsigned char>(byte | 0x80u); }
            dest.push_back(byte);
        } while (value != 0);
    }

    static bool DecodeVarint(const std::vector<unsigned char>& source, size_t& cursor, uint64_t& value) {
        value = 0;
        int shift = 0;
        while (shift <= 63) {
            if (cursor >= source.size()) { return false; }
            const uint64_t byte = source[cursor++];
            value |= (byte & 0x7fu) << shift;
            if ((byte & 0x80u) == 0) { return true; }
            shift += 7;
        }
        return false;
    }

    template<typename T>
    static uint64_t ToBits(T value) {
        using U = std::make_unsigned_t<T>;
        U bits{};
        std::memcpy(&bits, &value, sizeof(T));
        return static_cast<uint64_t>(bits);
    }

    template<typename T>
    static T FromBits(uint64_t value) {
        using U = std::make_unsigned_t<T>;
        const U bits = static_cast<U>(value);
        T out{};
        std::memcpy(&out, &bits, sizeof(T));
        return out;
    }

    template<typename T>
    static uint64_t ValueMask() {
        if constexpr (sizeof(T) >= sizeof(uint64_t)) {
            return std::numeric_limits<uint64_t>::max();
        } else {
            return (uint64_t{1} << (sizeof(T) * 8u)) - 1u;
        }
    }

    template<typename T>
    static uint64_t ZigZagEncode(uint64_t deltaBits) {
        constexpr unsigned bitCount = static_cast<unsigned>(sizeof(T) * 8u);
        const uint64_t mask = ValueMask<T>();
        const uint64_t sign = (deltaBits >> (bitCount - 1u)) & 1u;
        return ((deltaBits << 1u) ^ (uint64_t{0} - sign)) & mask;
    }

    template<typename T>
    static uint64_t ZigZagDecode(uint64_t encoded) {
        const uint64_t mask = ValueMask<T>();
        const uint64_t signMask = uint64_t{0} - (encoded & 1u);
        return ((encoded >> 1u) ^ signMask) & mask;
    }

    template<typename T>
    static bool EncodeComponent(std::vector<unsigned char>& dest, const std::vector<T>& source,
                                size_t elementCount, int dimension, int comp) {
        std::vector<uint64_t> deltas;
        deltas.reserve(elementCount);

        const uint64_t mask = ValueMask<T>();
        uint64_t previous = 0;
        for (size_t i = 0; i < elementCount; ++i) {
            const uint64_t current = ToBits(source[i * static_cast<size_t>(dimension) + static_cast<size_t>(comp)]);
            const uint64_t delta = (current - previous) & mask;
            deltas.push_back(ZigZagEncode<T>(delta));
            previous = current;
        }

        size_t cursor = 0;
        while (cursor < deltas.size()) {
            size_t runLength = 1;
            while (cursor + runLength < deltas.size() && deltas[cursor + runLength] == deltas[cursor]) {
                ++runLength;
            }

            if (runLength >= kRunThreshold) {
                EncodeVarint(dest, (static_cast<uint64_t>(runLength) << 1u) | kRunToken);
                EncodeVarint(dest, deltas[cursor]);
                cursor += runLength;
                continue;
            }

            const size_t literalStart = cursor;
            cursor += runLength;
            while (cursor < deltas.size()) {
                size_t nextRun = 1;
                while (cursor + nextRun < deltas.size() && deltas[cursor + nextRun] == deltas[cursor]) {
                    ++nextRun;
                }
                if (nextRun >= kRunThreshold) { break; }
                cursor += nextRun;
            }

            const size_t literalLength = cursor - literalStart;
            EncodeVarint(dest, (static_cast<uint64_t>(literalLength) << 1u) | kLiteralToken);
            for (size_t i = literalStart; i < cursor; ++i) {
                EncodeVarint(dest, deltas[i]);
            }
        }

        return true;
    }

    template<typename T>
    static bool DecodeComponent(std::vector<T>& dest, const std::vector<unsigned char>& source,
                                size_t& cursor, size_t elementCount, int dimension, int comp) {
        const uint64_t mask = ValueMask<T>();
        uint64_t previous = 0;
        size_t written = 0;

        while (written < elementCount) {
            uint64_t header = 0;
            if (!DecodeVarint(source, cursor, header)) { return false; }

            const uint64_t tag = header & 1u;
            const uint64_t length64 = header >> 1u;
            if (length64 == 0 || length64 > static_cast<uint64_t>(elementCount - written)) { return false; }
            const size_t length = static_cast<size_t>(length64);

            if (tag == kRunToken) {
                uint64_t encodedDelta = 0;
                if (!DecodeVarint(source, cursor, encodedDelta)) { return false; }
                for (size_t i = 0; i < length; ++i) {
                    const uint64_t delta = ZigZagDecode<T>(encodedDelta);
                    const uint64_t current = (previous + delta) & mask;
                    dest[(written + i) * static_cast<size_t>(dimension) + static_cast<size_t>(comp)] = FromBits<T>(current);
                    previous = current;
                }
            } else if (tag == kLiteralToken) {
                for (size_t i = 0; i < length; ++i) {
                    uint64_t encodedDelta = 0;
                    if (!DecodeVarint(source, cursor, encodedDelta)) { return false; }
                    const uint64_t delta = ZigZagDecode<T>(encodedDelta);
                    const uint64_t current = (previous + delta) & mask;
                    dest[(written + i) * static_cast<size_t>(dimension) + static_cast<size_t>(comp)] = FromBits<T>(current);
                    previous = current;
                }
            } else {
                return false;
            }

            written += length;
        }

        return true;
    }
};

IGAME_NAMESPACE_END

#endif
