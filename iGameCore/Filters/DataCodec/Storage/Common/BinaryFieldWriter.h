#ifndef DATACODEC_STORAGE_COMMON_BINARYFIELDWRITER_H
#define DATACODEC_STORAGE_COMMON_BINARYFIELDWRITER_H

#include "DataCodec/Storage/ByteIO/ByteSource.h"
#include "DataCodec/Storage/Common/BinaryValueIO.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <array>
#include <cstdint>
#include <span>
#include <string>
namespace datacodec {
namespace detail {

inline bool WriteBytesToWriter(
    bytestore::IByteWriter& writer,
    const std::uint8_t* bytes,
    const std::size_t byteCount,
    std::string* error = nullptr) {
    if (byteCount == 0u) {
        return true;
    }
    if (bytes == nullptr) {
        return validation::AssignError(error, "cannot write null byte range");
    }
    return writer.Write(std::span<const std::uint8_t>(bytes, byteCount), error);
}

template<typename TValue>
inline bool WriteScalarToWriter(
    bytestore::IByteWriter& writer,
    const TValue& value,
    std::string* error = nullptr) {
    using Storage = WireStorageTypeT<TValue>;
    auto storage = ToWireStorage(value);
    std::array<std::uint8_t, sizeof(Storage)> bytes{};
    for (std::size_t byteIndex = 0; byteIndex < sizeof(Storage); ++byteIndex) {
        bytes[byteIndex] =
            static_cast<std::uint8_t>((storage >> (byteIndex * 8u)) & Storage{0xFFu});
    }
    return writer.Write(std::span<const std::uint8_t>(bytes.data(), bytes.size()), error);
}

inline bool WriteStringToWriter(
    bytestore::IByteWriter& writer,
    const std::string& value,
    std::string* error = nullptr) {
    if (!WriteScalarToWriter(writer, static_cast<std::uint64_t>(value.size()), error)) {
        return false;
    }
    return WriteBytesToWriter(
        writer,
        reinterpret_cast<const std::uint8_t*>(value.data()),
        value.size(),
        error);
}

} // namespace detail
} // namespace datacodec

#endif
