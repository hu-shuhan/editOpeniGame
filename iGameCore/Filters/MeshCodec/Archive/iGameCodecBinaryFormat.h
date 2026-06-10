#ifndef IGAMEVIS_IGAMECODECBINARYFORMAT_H
#define IGAMEVIS_IGAMECODECBINARYFORMAT_H

#include "iGameMacro.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

class CodecBinaryFormatWriter {
public:
    explicit CodecBinaryFormatWriter(std::vector<uint8_t>& data) : m_Data(data) {}

    void WriteBytes(const uint8_t* data, size_t size) {
        if (size == 0) { return; }
        if (!data) { throw std::runtime_error("Null codec binary data"); }
        m_Data.insert(m_Data.end(), data, data + size);
    }

    void WriteUnsigned(uint64_t value, size_t byteCount) {
        for (size_t i = 0; i < byteCount; ++i) {
            m_Data.push_back(static_cast<uint8_t>((value >> (i * 8)) & 0xFFu));
        }
    }

    void WriteSigned(int64_t value, size_t byteCount) {
        WriteUnsigned(static_cast<uint64_t>(value), byteCount);
    }

    void WriteUnsignedChecked(uint64_t value, size_t byteCount) {
        const uint64_t maxValue = (byteCount == 8) ? std::numeric_limits<uint64_t>::max()
                                                   : ((uint64_t{1} << (byteCount * 8)) - 1);
        if (value > maxValue) {
            throw std::runtime_error("Unsigned integer value is out of range for codec archive");
        }
        WriteUnsigned(value, byteCount);
    }

    void WriteSignedChecked(int64_t value, size_t byteCount) {
        if (byteCount < 8) {
            const int64_t minValue = -(int64_t{1} << (byteCount * 8 - 1));
            const int64_t maxValue = (int64_t{1} << (byteCount * 8 - 1)) - 1;
            if (value < minValue || value > maxValue) {
                throw std::runtime_error("Signed integer value is out of range for codec archive");
            }
        }
        WriteSigned(value, byteCount);
    }

    [[nodiscard]] size_t Size() const { return m_Data.size(); }

private:
    std::vector<uint8_t>& m_Data;
};

class CodecBinaryFormatReader {
public:
    explicit CodecBinaryFormatReader(const std::vector<uint8_t>& data) : m_Data(data) {}

    void ReadBytes(uint8_t* target, size_t size) {
        if (size == 0) { return; }
        if (!target) { throw std::runtime_error("Null codec binary target"); }
        EnsureAvailable(size);
        std::memcpy(target, m_Data.data() + m_Cursor, size);
        m_Cursor += size;
    }

    uint64_t ReadUnsigned(size_t byteCount) {
        EnsureAvailable(byteCount);
        uint64_t value = 0;
        for (size_t i = 0; i < byteCount; ++i) {
            value |= static_cast<uint64_t>(m_Data[m_Cursor + i]) << (i * 8);
        }
        m_Cursor += byteCount;
        return value;
    }

    int64_t ReadSigned(size_t byteCount) {
        const uint64_t raw = ReadUnsigned(byteCount);
        if (byteCount == 8) { return static_cast<int64_t>(raw); }

        const uint64_t signBit = uint64_t{1} << (byteCount * 8 - 1);
        const uint64_t mask = (~uint64_t{0}) << (byteCount * 8);
        return static_cast<int64_t>((raw & signBit) ? (raw | mask) : raw);
    }

    void EnsureAvailable(size_t byteCount) const {
        if (byteCount > m_Data.size() || m_Cursor > m_Data.size() - byteCount) {
            throw std::runtime_error("Unexpected end of codec archive");
        }
    }

    [[nodiscard]] size_t Cursor() const { return m_Cursor; }
    void SetCursor(size_t cursor) {
        if (cursor > m_Data.size()) { throw std::runtime_error("Codec archive cursor is out of range"); }
        m_Cursor = cursor;
    }

private:
    const std::vector<uint8_t>& m_Data;
    size_t m_Cursor = 0;
};

IGAME_NAMESPACE_END

#endif
