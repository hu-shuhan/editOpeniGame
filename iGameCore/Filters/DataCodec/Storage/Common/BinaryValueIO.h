#ifndef DATACODEC_STORAGE_COMMON_BINARYVALUEIO_H
#define DATACODEC_STORAGE_COMMON_BINARYVALUEIO_H

#include "DataCodec/Common/DataCodecTypes.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <array>
#include <cstring>
#include <limits>
#include <span>
#include <string>
#include <type_traits>
#include <vector>
namespace datacodec::detail {

template<typename TValue>
inline constexpr bool IsWireScalarValue =
    std::is_integral_v<TValue> ||
    std::is_enum_v<TValue> ||
    std::is_same_v<TValue, float> ||
    std::is_same_v<TValue, double>;

template<typename TValue, bool = std::is_enum_v<TValue>>
struct WireValueType {
    using type = TValue;
};

template<typename TValue>
struct WireValueType<TValue, true> {
    using type = std::underlying_type_t<TValue>;
};

template<typename TValue, typename TValueBase = typename WireValueType<TValue>::type, typename = void>
struct WireStorageType;

template<typename TValue, typename TValueBase>
struct WireStorageType<
    TValue,
    TValueBase,
    std::enable_if_t<std::is_integral_v<TValueBase> && !std::is_same_v<TValueBase, bool>>> {
    using type = std::make_unsigned_t<TValueBase>;
};

template<typename TValue, typename TValueBase>
struct WireStorageType<TValue, TValueBase, std::enable_if_t<std::is_same_v<TValueBase, bool>>> {
    using type = std::uint8_t;
};

template<typename TValue, typename TValueBase>
struct WireStorageType<TValue, TValueBase, std::enable_if_t<std::is_same_v<TValueBase, float>>> {
    using type = std::uint32_t;
};

template<typename TValue, typename TValueBase>
struct WireStorageType<TValue, TValueBase, std::enable_if_t<std::is_same_v<TValueBase, double>>> {
    using type = std::uint64_t;
};

template<typename TValue>
using WireStorageTypeT = typename WireStorageType<TValue>::type;

template<typename TValue>
inline constexpr std::size_t WireScalarSize() {
    static_assert(IsWireScalarValue<TValue>, "wire scalar must be integral, enum, float, or double");
    return sizeof(WireStorageTypeT<TValue>);
}

template<typename TValue>
inline WireStorageTypeT<TValue> ToWireStorage(const TValue& value) {
    static_assert(IsWireScalarValue<TValue>, "wire scalar must be integral, enum, float, or double");
    using ValueBase = typename WireValueType<TValue>::type;
    using Storage = WireStorageTypeT<TValue>;

    if constexpr (std::is_enum_v<TValue>) {
        return ToWireStorage(static_cast<ValueBase>(value));
    } else if constexpr (std::is_same_v<ValueBase, bool>) {
        return value ? std::uint8_t{1} : std::uint8_t{0};
    } else {
        if constexpr (std::is_floating_point_v<ValueBase>) {
            static_assert(std::numeric_limits<ValueBase>::is_iec559, "wire float must use IEC 559 layout");
        }
        Storage storage{};
        std::memcpy(&storage, &value, sizeof(ValueBase));
        return storage;
    }
}

template<typename TValue>
inline void FromWireStorage(const WireStorageTypeT<TValue> storage, TValue& value) {
    static_assert(IsWireScalarValue<TValue>, "wire scalar must be integral, enum, float, or double");
    using ValueBase = typename WireValueType<TValue>::type;

    if constexpr (std::is_enum_v<TValue>) {
        ValueBase base{};
        FromWireStorage(storage, base);
        value = static_cast<TValue>(base);
    } else if constexpr (std::is_same_v<ValueBase, bool>) {
        value = storage != 0;
    } else {
        if constexpr (std::is_floating_point_v<ValueBase>) {
            static_assert(std::numeric_limits<ValueBase>::is_iec559, "wire float must use IEC 559 layout");
        }
        std::memcpy(&value, &storage, sizeof(ValueBase));
    }
}

inline bool HasWireBytes(
    const std::span<const std::uint8_t> input,
    const std::size_t cursor,
    const std::size_t count) {
    return cursor <= input.size() && count <= input.size() - cursor;
}

template<typename TValue>
inline void AppendScalar(std::vector<std::uint8_t>& output, const TValue& value) {
    using Storage = WireStorageTypeT<TValue>;
    auto storage = ToWireStorage(value);
    for (std::size_t byteIndex = 0; byteIndex < sizeof(Storage); ++byteIndex) {
        output.push_back(static_cast<std::uint8_t>((storage >> (byteIndex * 8u)) & Storage{0xFFu}));
    }
}

template<typename TValue>
inline void AppendArray(std::vector<std::uint8_t>& output, const TValue* values, const std::size_t count) {
    static_assert(IsWireScalarValue<TValue>, "wire array must contain scalar values");
    if (values == nullptr || count == 0) {
        return;
    }
    if constexpr (WireScalarSize<TValue>() == 1u && !std::is_same_v<typename WireValueType<TValue>::type, bool>) {
        const auto* begin = reinterpret_cast<const std::uint8_t*>(values);
        output.insert(output.end(), begin, begin + count);
    } else {
        for (std::size_t index = 0; index < count; ++index) {
            AppendScalar(output, values[index]);
        }
    }
}

inline void AppendString(std::vector<std::uint8_t>& output, const std::string& value) {
    AppendScalar(output, static_cast<std::uint64_t>(value.size()));
    output.insert(output.end(), value.begin(), value.end());
}

template<typename TValue>
inline bool ReadScalar(
    const std::span<const std::uint8_t> input,
    std::size_t& cursor,
    TValue& value,
    std::string* error = nullptr) {
    using Storage = WireStorageTypeT<TValue>;
    if (!HasWireBytes(input, cursor, sizeof(Storage))) {
        return validation::AssignError(error, "unexpected end of buffer while reading scalar");
    }
    Storage storage{};
    for (std::size_t byteIndex = 0; byteIndex < sizeof(Storage); ++byteIndex) {
        storage |= static_cast<Storage>(input[cursor + byteIndex]) << (byteIndex * 8u);
    }
    cursor += sizeof(Storage);
    FromWireStorage(storage, value);
    return true;
}

template<typename TValue>
inline bool ReadArray(
    const std::span<const std::uint8_t> input,
    std::size_t& cursor,
    std::vector<TValue>& values,
    const std::size_t count,
    std::string* error = nullptr) {
    static_assert(IsWireScalarValue<TValue>, "wire array must contain scalar values");
    constexpr auto valueSize = WireScalarSize<TValue>();
    if (!validation::CanMulSizeT(valueSize, count)) {
        return validation::AssignError(error, "unexpected end of buffer while reading array");
    }
    const auto byteCount = valueSize * count;
    if (!HasWireBytes(input, cursor, byteCount)) {
        return validation::AssignError(error, "unexpected end of buffer while reading array");
    }
    values.resize(count);
    if constexpr (valueSize == 1u && !std::is_same_v<typename WireValueType<TValue>::type, bool>) {
        if (count > 0) {
            std::memcpy(values.data(), input.data() + cursor, count);
        }
        cursor += count;
    } else {
        for (std::size_t index = 0; index < count; ++index) {
            if (!ReadScalar(input, cursor, values[index], error)) {
                values.clear();
                return false;
            }
        }
    }
    return true;
}

template<typename TValue, std::size_t TCount>
inline bool ReadArray(
    const std::span<const std::uint8_t> input,
    std::size_t& cursor,
    std::array<TValue, TCount>& values,
    const std::size_t count,
    std::string* error = nullptr) {
    static_assert(IsWireScalarValue<TValue>, "wire array must contain scalar values");
    if (count != TCount) {
        return validation::AssignError(error, "unexpected fixed-size array length");
    }
    constexpr auto valueSize = WireScalarSize<TValue>();
    if (!validation::CanMulSizeT(valueSize, count)) {
        return validation::AssignError(error, "unexpected end of buffer while reading fixed-size array");
    }
    const auto byteCount = valueSize * count;
    if (!HasWireBytes(input, cursor, byteCount)) {
        return validation::AssignError(error, "unexpected end of buffer while reading fixed-size array");
    }
    if constexpr (valueSize == 1u && !std::is_same_v<typename WireValueType<TValue>::type, bool>) {
        if (count > 0) {
            std::memcpy(values.data(), input.data() + cursor, count);
        }
        cursor += count;
    } else {
        for (std::size_t index = 0; index < count; ++index) {
            if (!ReadScalar(input, cursor, values[index], error)) {
                return false;
            }
        }
    }
    return true;
}

inline bool ReadString(
    const std::span<const std::uint8_t> input,
    std::size_t& cursor,
    std::string& value,
    std::string* error = nullptr) {
    std::uint64_t size = 0;
    if (!ReadScalar(input, cursor, size, error)) {
        return false;
    }
    std::size_t localSize = 0u;
    if (!validation::CheckedCastSizeT(size, localSize, "wire string", error)) {
        return false;
    }
    if (!HasWireBytes(input, cursor, localSize)) {
        return validation::AssignError(error, "unexpected end of buffer while reading string");
    }
    value.assign(
        reinterpret_cast<const char*>(input.data() + cursor),
        reinterpret_cast<const char*>(input.data() + cursor + localSize));
    cursor += localSize;
    return true;
}

inline bool ReadSpan(
    const std::span<const std::uint8_t> input,
    std::size_t& cursor,
    const std::size_t count,
    std::span<const std::uint8_t>& view,
    std::string* error = nullptr) {
    if (!HasWireBytes(input, cursor, count)) {
        return validation::AssignError(error, "unexpected end of buffer while reading span");
    }
    view = input.subspan(cursor, count);
    cursor += count;
    return true;
}

} // namespace datacodec::detail

#endif
