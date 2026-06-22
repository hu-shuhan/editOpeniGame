#ifndef IGAMEVIS_IGAMECODECBINARYTYPETRAITS_H
#define IGAMEVIS_IGAMECODECBINARYTYPETRAITS_H

#include "MeshCodec/Archive/iGameCodecBinaryFormat.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <type_traits>

IGAME_NAMESPACE_BEGIN

template<typename T, typename = void>
struct CodecBinaryTypeTraits {
    static constexpr bool Supported = false;
};

template<>
struct CodecBinaryTypeTraits<bool> {
    static constexpr bool Supported = true;

    static void Write(CodecBinaryFormatWriter& writer, bool value) {
        writer.WriteUnsigned(value ? uint8_t{1} : uint8_t{0}, 1);
    }

    static void Read(CodecBinaryFormatReader& reader, bool& value) {
        value = reader.ReadUnsigned(1) != 0;
    }
};

template<>
struct CodecBinaryTypeTraits<std::size_t> {
    static constexpr bool Supported = true;

    static void Write(CodecBinaryFormatWriter& writer, std::size_t value) {
        writer.WriteUnsignedChecked(static_cast<uint64_t>(value), 8);
    }

    static void Read(CodecBinaryFormatReader& reader, std::size_t& value) {
        const uint64_t storedValue = reader.ReadUnsigned(8);
        if (storedValue > static_cast<uint64_t>(std::numeric_limits<std::size_t>::max())) {
            throw std::runtime_error("size_t value is out of range for this platform");
        }
        value = static_cast<std::size_t>(storedValue);
    }
};

template<typename T>
struct CodecBinaryTypeTraits<T, std::enable_if_t<std::is_enum_v<T>>> {
    static constexpr bool Supported = true;

    static void Write(CodecBinaryFormatWriter& writer, T value) {
        using Underlying = std::underlying_type_t<T>;
        if constexpr (std::is_signed_v<Underlying>) {
            writer.WriteSignedChecked(static_cast<int64_t>(value), 4);
        } else {
            writer.WriteUnsignedChecked(static_cast<uint64_t>(value), 4);
        }
    }

    static void Read(CodecBinaryFormatReader& reader, T& value) {
        value = static_cast<T>(reader.ReadSigned(4));
    }
};

template<typename T>
struct CodecBinaryTypeTraits<T, std::enable_if_t<
    std::is_integral_v<T> &&
    !std::is_same_v<T, bool> &&
    !std::is_same_v<T, std::size_t> &&
    std::is_signed_v<T>>> {
    static constexpr bool Supported = true;

    static void Write(CodecBinaryFormatWriter& writer, T value) {
        writer.WriteSignedChecked(static_cast<int64_t>(value), StoredByteCount());
    }

    static void Read(CodecBinaryFormatReader& reader, T& value) {
        const int64_t storedValue = reader.ReadSigned(StoredByteCount());
        if (storedValue < static_cast<int64_t>(std::numeric_limits<T>::min()) ||
            storedValue > static_cast<int64_t>(std::numeric_limits<T>::max())) {
            throw std::runtime_error("Signed integer value is out of range for this platform");
        }
        value = static_cast<T>(storedValue);
    }

private:
    static constexpr size_t StoredByteCount() {
        if constexpr (sizeof(T) == 1) {
            return 1;
        } else if constexpr (sizeof(T) == 2) {
            return 2;
        } else if constexpr (sizeof(T) <= 4 || std::is_same_v<T, long>) {
            return 4;
        } else {
            return 8;
        }
    }
};

template<typename T>
struct CodecBinaryTypeTraits<T, std::enable_if_t<
    std::is_integral_v<T> &&
    !std::is_same_v<T, bool> &&
    !std::is_same_v<T, std::size_t> &&
    std::is_unsigned_v<T>>> {
    static constexpr bool Supported = true;

    static void Write(CodecBinaryFormatWriter& writer, T value) {
        writer.WriteUnsignedChecked(static_cast<uint64_t>(value), StoredByteCount());
    }

    static void Read(CodecBinaryFormatReader& reader, T& value) {
        const uint64_t storedValue = reader.ReadUnsigned(StoredByteCount());
        if (storedValue > static_cast<uint64_t>(std::numeric_limits<T>::max())) {
            throw std::runtime_error("Unsigned integer value is out of range for this platform");
        }
        value = static_cast<T>(storedValue);
    }

private:
    static constexpr size_t StoredByteCount() {
        if constexpr (sizeof(T) == 1) {
            return 1;
        } else if constexpr (sizeof(T) == 2) {
            return 2;
        } else if constexpr (sizeof(T) <= 4 || std::is_same_v<T, unsigned long>) {
            return 4;
        } else {
            return 8;
        }
    }
};

template<typename T>
struct CodecBinaryTypeTraits<T, std::enable_if_t<std::is_floating_point_v<T>>> {
    static constexpr bool Supported = true;

    static void Write(CodecBinaryFormatWriter& writer, T value) {
        if constexpr (sizeof(T) == sizeof(uint32_t)) {
            uint32_t bits = 0;
            std::memcpy(&bits, &value, sizeof(bits));
            writer.WriteUnsigned(bits, 4);
        } else {
            static_assert(sizeof(T) == sizeof(uint64_t), "Unsupported floating point width");
            uint64_t bits = 0;
            std::memcpy(&bits, &value, sizeof(bits));
            writer.WriteUnsigned(bits, 8);
        }
    }

    static void Read(CodecBinaryFormatReader& reader, T& value) {
        if constexpr (sizeof(T) == sizeof(uint32_t)) {
            uint32_t bits = static_cast<uint32_t>(reader.ReadUnsigned(4));
            std::memcpy(&value, &bits, sizeof(bits));
        } else {
            static_assert(sizeof(T) == sizeof(uint64_t), "Unsupported floating point width");
            uint64_t bits = reader.ReadUnsigned(8);
            std::memcpy(&value, &bits, sizeof(bits));
        }
    }
};

IGAME_NAMESPACE_END

#endif
