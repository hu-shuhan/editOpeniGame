#ifndef DATACODEC_CODEC_NUMERICARRAY_NUMERICARRAYCODEC_H
#define DATACODEC_CODEC_NUMERICARRAY_NUMERICARRAYCODEC_H

#include "DataCodec/Codec/NumericArray/NumericArrayBuffer.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <limits>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <vector>

#ifdef DATACODEC_ENABLE_LIBPRESSIO
#include <libpressio.h>
#endif
namespace datacodec::numericarray {

class NumericArrayEncode {
public:
    [[nodiscard]] static bool IsAvailable();
    [[nodiscard]] static const char* BackendSummary();

    static bool Compress(
        const NumericArrayBufferView& input,
        const CompressorConfig& compressor,
        NumericArrayEncodedBytes& output,
        std::string* error = nullptr);

    static bool CompressSegments(
        const NumericArrayBufferView& input,
        const std::vector<NumericArraySegmentSpec>& segments,
        std::vector<NumericArrayCompressedSegment>& output,
        std::string* error = nullptr);
};

class NumericArrayDecode {
public:
    [[nodiscard]] static bool IsAvailable();
    [[nodiscard]] static const char* BackendSummary();

    static bool Decompress(
        std::span<const std::uint8_t> input,
        const NumericArrayBufferLayout& layout,
        const CompressorConfig& compressor,
        const MutableNumericArrayBufferView& output,
        std::string* error = nullptr);

    static bool DecompressSegments(
        const std::vector<NumericArrayCompressedSegment>& input,
        const MutableNumericArrayBufferView& output,
        std::string* error = nullptr);
};

namespace detail::numericarraycodec {

// [DC防护:领域] numeric array codec 在压缩解压前校验 layout、view 和 segment 契约
inline bool ValidateLayout(const NumericArrayBufferLayout& layout, std::string* error) {
    if (layout.shape.empty() && layout.componentCount == 0u) {
        validation::AssignError(error, "value codec requires dimension > 0");
        return false;
    }
    for (const auto extent : layout.shape) {
        if (extent == 0u) {
            validation::AssignError(error, "value codec explicit shape contains zero extent");
            return false;
        }
    }
    const auto expectedValueSize = DataTypeSize(layout.dataType);
    if (expectedValueSize == 0u || layout.valueSize != expectedValueSize) {
        validation::AssignError(error, "value codec data type does not match value size");
        return false;
    }
    if (layout.ByteCount() == std::numeric_limits<std::size_t>::max()) {
        validation::AssignError(error, "value codec buffer layout exceeds size_t range");
        return false;
    }
    return true;
}

inline bool ValidateInputView(const NumericArrayBufferView& input, std::string* error) {
    if (!ValidateLayout(input.layout, error)) {
        return false;
    }
    if (input.layout.ByteCount() == 0) {
        return true;
    }
    if (input.data == nullptr) {
        validation::AssignError(error, "value codec input buffer is null");
        return false;
    }
    return true;
}

inline bool ValidateOutputView(const MutableNumericArrayBufferView& output, std::string* error) {
    if (!ValidateLayout(output.layout, error)) {
        return false;
    }
    if (output.layout.ByteCount() == 0) {
        return true;
    }
    if (output.data == nullptr) {
        validation::AssignError(error, "value codec output buffer is null");
        return false;
    }
    return true;
}

inline bool ValidateSegmentRange(
    const NumericArrayBufferLayout& layout,
    const std::size_t elementOffset,
    const std::size_t elementCount,
    std::string* error) {
    if (elementOffset > layout.elementCount) {
        validation::AssignError(error, "value codec segment offset is out of range");
        return false;
    }
    if (elementCount > layout.elementCount - elementOffset) {
        validation::AssignError(error, "value codec segment length is out of range");
        return false;
    }
    return true;
}

inline NumericArrayBufferView SliceBufferView(
    const NumericArrayBufferView& input,
    const std::size_t elementOffset,
    const std::size_t elementCount) {
    const auto valuesPerElement = input.layout.componentCount;
    const auto byteOffset = elementOffset * valuesPerElement * input.layout.valueSize;

    NumericArrayBufferView slice;
    slice.data = static_cast<const std::uint8_t*>(input.data) + byteOffset;
    slice.layout = input.layout;
    slice.layout.elementCount = elementCount;
    if (!slice.layout.shape.empty()) {
        slice.layout.shape.front() = elementCount;
    }
    return slice;
}

inline MutableNumericArrayBufferView SliceBufferView(
    const MutableNumericArrayBufferView& output,
    const std::size_t elementOffset,
    const std::size_t elementCount) {
    const auto valuesPerElement = output.layout.componentCount;
    const auto byteOffset = elementOffset * valuesPerElement * output.layout.valueSize;

    MutableNumericArrayBufferView slice;
    slice.data = static_cast<std::uint8_t*>(output.data) + byteOffset;
    slice.layout = output.layout;
    slice.layout.elementCount = elementCount;
    if (!slice.layout.shape.empty()) {
        slice.layout.shape.front() = elementCount;
    }
    return slice;
}

#ifdef DATACODEC_ENABLE_LIBPRESSIO

struct PressioLibraryDeleter {
    void operator()(pressio* library) const {
        if (library != nullptr) {
            pressio_release(library);
        }
    }
};

struct PressioCompressorDeleter {
    void operator()(pressio_compressor* compressor) const {
        if (compressor != nullptr) {
            pressio_compressor_release(compressor);
        }
    }
};

struct PressioOptionsDeleter {
    void operator()(pressio_options* options) const {
        if (options != nullptr) {
            pressio_options_free(options);
        }
    }
};

using LibraryHandle = std::unique_ptr<pressio, PressioLibraryDeleter>;
using CompressorHandle = std::unique_ptr<pressio_compressor, PressioCompressorDeleter>;
using OptionsHandle = std::unique_ptr<pressio_options, PressioOptionsDeleter>;
using DataHandle = PressioDataHandle;

inline pressio_dtype ToPressioDType(const DataType dataType) {
    switch (dataType) {
        case DataType::Float32:
            return pressio_float_dtype;
        case DataType::Float64:
            return pressio_double_dtype;
        case DataType::Int8:
            return pressio_int8_dtype;
        case DataType::UInt8:
            return pressio_uint8_dtype;
        case DataType::Int16:
            return pressio_int16_dtype;
        case DataType::UInt16:
            return pressio_uint16_dtype;
        case DataType::Int32:
            return pressio_int32_dtype;
        case DataType::UInt32:
            return pressio_uint32_dtype;
        case DataType::Int64:
            return pressio_int64_dtype;
        case DataType::UInt64:
            return pressio_uint64_dtype;
        default:
            return pressio_byte_dtype;
    }
}

inline std::vector<std::size_t> BuildDims(const NumericArrayBufferLayout& layout) {
    return layout.BuildShape();
}

inline bool ConfigureCompressor(
    pressio_compressor* compressor,
    const CompressorConfig& config,
    std::string* error) {
    OptionsHandle options(pressio_options_new());
    if (!options) {
        validation::AssignError(error, "libpressio failed to allocate options");
        return false;
    }

    for (const auto& option : config.options) {
        pressio_options_set_double(options.get(), option.first.c_str(), option.second);
    }

    const auto rc = pressio_compressor_set_options(compressor, options.get());
    if (rc > 0) {
        validation::AssignError(error, "libpressio failed to set compressor options");
        return false;
    }
    return true;
}

inline void AppendDoubleBitsToKey(std::string& key, const double value) {
    std::uint64_t bits = 0u;
    static_assert(sizeof(bits) == sizeof(value));
    std::memcpy(&bits, &value, sizeof(bits));
    for (std::size_t byteIndex = 0u; byteIndex < sizeof(bits); ++byteIndex) {
        key.push_back(static_cast<char>((bits >> (byteIndex * 8u)) & 0xFFu));
    }
}

inline std::string MakeCompressorCacheKey(const CompressorConfig& config) {
    std::string key;
    for (const auto& option : config.options) {
        key.append(option.first);
        key.push_back('\0');
        AppendDoubleBitsToKey(key, option.second);
    }
    return key;
}

class ThreadLocalCompressorCache final {
public:
    pressio_compressor* Resolve(
        const CompressorConfig& compressorConfig,
        std::string* error) {
        if (!m_library) {
            m_library.reset(pressio_instance());
            if (!m_library) {
                validation::AssignError(error, "libpressio failed to create a library instance");
                return nullptr;
            }
        }

        const auto key = MakeCompressorCacheKey(compressorConfig);
        const auto cached = m_compressors.find(key);
        if (cached != m_compressors.end()) {
            return cached->second.get();
        }

        CompressorHandle compressor(
            pressio_get_compressor(m_library.get(), kNumericArrayCompressorId));
        if (!compressor) {
            validation::AssignError(error, "libpressio could not find compressor sz3");
            return nullptr;
        }
        if (!ConfigureCompressor(compressor.get(), compressorConfig, error)) {
            return nullptr;
        }
        auto* compressorPtr = compressor.get();
        m_compressors.emplace(key, std::move(compressor));
        return compressorPtr;
    }

private:
    LibraryHandle m_library;
    std::map<std::string, CompressorHandle> m_compressors;
};

inline pressio_compressor* ResolveThreadLocalCompressor(
    const CompressorConfig& compressorConfig,
    std::string* error) {
#if defined(_WIN32) && defined(__clang__)
    // Windows Clang 的线程退出阶段无法安全析构包含 libpressio 句柄的 TLS 对象
    // 缓存随工作线程保留到进程结束以避免 TLS 析构访问已释放内存
    thread_local auto* cache = new ThreadLocalCompressorCache();
    return cache->Resolve(compressorConfig, error);
#else
    thread_local ThreadLocalCompressorCache cache;
    return cache.Resolve(compressorConfig, error);
#endif
}

inline bool CompressWithLibPressio(
    const NumericArrayBufferView& input,
    const CompressorConfig& compressorConfig,
    NumericArrayEncodedBytes& bytes,
    std::string* error) {
    auto* compressor = ResolveThreadLocalCompressor(compressorConfig, error);
    if (!compressor) {
        return false;
    }

    const auto dims = BuildDims(input.layout);
    DataHandle inputData(pressio_data_new_nonowning(
        ToPressioDType(input.layout.dataType),
        const_cast<void*>(input.data),
        dims.size(),
        dims.data()));
    if (!inputData) {
        validation::AssignError(error, "libpressio failed to wrap the input buffer");
        return false;
    }

    DataHandle outputData(pressio_data_new_empty(pressio_byte_dtype, 0, nullptr));
    if (!outputData) {
        validation::AssignError(error, "libpressio failed to allocate the compressed buffer");
        return false;
    }

    int rc = 0;
    try {
        rc = pressio_compressor_compress(compressor, inputData.get(), outputData.get());
    } catch (const std::exception& exception) {
        validation::AssignError(
            error,
            "libpressio compression threw an exception: " + std::string(exception.what()));
        return false;
    } catch (...) {
        validation::AssignError(error, "libpressio compression threw an unknown exception");
        return false;
    }
    if (rc > 0) {
        const char* message = pressio_compressor_error_msg(compressor);
        validation::AssignError(error, message != nullptr ? message : "libpressio compression failed");
        return false;
    }

    std::size_t byteCount = 0;
    const auto* ptr = static_cast<const std::uint8_t*>(pressio_data_ptr(outputData.get(), &byteCount));
    if (byteCount > 0 && ptr == nullptr) {
        validation::AssignError(error, "libpressio returned an invalid compressed buffer");
        return false;
    }

    bytes.TakePressioData(
        std::move(outputData),
        std::span<const std::uint8_t>(ptr, byteCount));
    return true;
}

inline bool DecompressWithLibPressio(
    const std::span<const std::uint8_t> input,
    const NumericArrayBufferLayout& layout,
    const CompressorConfig& compressorConfig,
    void* output,
    std::string* error) {
    LibraryHandle library(pressio_instance());
    if (!library) {
        validation::AssignError(error, "libpressio failed to create a library instance");
        return false;
    }

    CompressorHandle compressor(pressio_get_compressor(library.get(), kNumericArrayCompressorId));
    if (!compressor) {
        validation::AssignError(error, "libpressio could not find compressor sz3");
        return false;
    }

    if (!ConfigureCompressor(compressor.get(), compressorConfig, error)) {
        return false;
    }

    const std::size_t compressedDims[1]{input.size()};
    DataHandle inputData(pressio_data_new_nonowning(
        pressio_byte_dtype,
        const_cast<std::uint8_t*>(input.data()),
        1,
        compressedDims));
    if (!inputData) {
        validation::AssignError(error, "libpressio failed to wrap the compressed buffer");
        return false;
    }

    const auto dims = BuildDims(layout);
    DataHandle outputData(pressio_data_new_nonowning(
        ToPressioDType(layout.dataType),
        output,
        dims.size(),
        dims.data()));
    if (!outputData) {
        validation::AssignError(error, "libpressio failed to wrap the decompression buffer");
        return false;
    }

    int rc = 0;
    try {
        rc = pressio_compressor_decompress(compressor.get(), inputData.get(), outputData.get());
    } catch (const std::exception& exception) {
        validation::AssignError(
            error,
            "libpressio decompression threw an exception: " + std::string(exception.what()));
        return false;
    } catch (...) {
        validation::AssignError(error, "libpressio decompression threw an unknown exception");
        return false;
    }
    if (rc > 0) {
        const char* message = pressio_compressor_error_msg(compressor.get());
        validation::AssignError(error, message != nullptr ? message : "libpressio decompression failed");
        return false;
    }

    return true;
}

#endif

} // namespace detail::numericarraycodec

inline bool NumericArrayEncode::IsAvailable() {
#ifdef DATACODEC_ENABLE_LIBPRESSIO
    return true;
#else
    return false;
#endif
}

inline const char* NumericArrayEncode::BackendSummary() {
#ifdef DATACODEC_ENABLE_LIBPRESSIO
    return "libpressio";
#else
    return "disabled";
#endif
}

inline bool NumericArrayEncode::Compress(
    const NumericArrayBufferView& input,
    const CompressorConfig& compressor,
    NumericArrayEncodedBytes& output,
    std::string* error) {
    output.Reset();
    if (!detail::numericarraycodec::ValidateInputView(input, error)) {
        return false;
    }

    if (input.layout.ByteCount() == 0) {
        return true;
    }

#ifdef DATACODEC_ENABLE_LIBPRESSIO
    return detail::numericarraycodec::CompressWithLibPressio(input, compressor, output, error);
#else
    validation::AssignError(error, "libpressio support is disabled");
    return false;
#endif
}

inline bool NumericArrayDecode::IsAvailable() {
    return NumericArrayEncode::IsAvailable();
}

inline const char* NumericArrayDecode::BackendSummary() {
    return NumericArrayEncode::BackendSummary();
}

inline bool NumericArrayDecode::Decompress(
    const std::span<const std::uint8_t> input,
    const NumericArrayBufferLayout& layout,
    const CompressorConfig& compressor,
    const MutableNumericArrayBufferView& output,
    std::string* error) {
    if (!detail::numericarraycodec::ValidateLayout(layout, error)) {
        return false;
    }
    if (!detail::numericarraycodec::ValidateOutputView(output, error)) {
        return false;
    }
    if (layout.dataType != output.layout.dataType ||
        layout.valueSize != output.layout.valueSize ||
        layout.elementCount != output.layout.elementCount ||
        layout.componentCount != output.layout.componentCount ||
        layout.shape != output.layout.shape) {
        validation::AssignError(error, "value codec output layout does not match the compressed payload layout");
        return false;
    }
    if (input.empty()) {
        return output.layout.ByteCount() == 0;
    }

#ifdef DATACODEC_ENABLE_LIBPRESSIO
    return detail::numericarraycodec::DecompressWithLibPressio(input, layout, compressor, output.data, error);
#else
    validation::AssignError(error, "libpressio support is disabled");
    return false;
#endif
}

inline bool NumericArrayEncode::CompressSegments(
    const NumericArrayBufferView& input,
    const std::vector<NumericArraySegmentSpec>& segments,
    std::vector<NumericArrayCompressedSegment>& output,
    std::string* error) {
    output.clear();
    if (!detail::numericarraycodec::ValidateInputView(input, error)) {
        return false;
    }

    output.reserve(segments.size());
    for (const auto& segment : segments) {
        if (!detail::numericarraycodec::ValidateSegmentRange(input.layout, segment.elementOffset, segment.elementCount, error)) {
            output.clear();
            return false;
        }

        const auto segmentView =
            detail::numericarraycodec::SliceBufferView(input, segment.elementOffset, segment.elementCount);
        NumericArrayEncodedBytes compressed;
        if (!Compress(
                segmentView,
                segment.compressor,
                compressed,
                error)) {
            output.clear();
            return false;
        }

        NumericArrayCompressedSegment entry;
        entry.elementOffset = segment.elementOffset;
        entry.layout = segmentView.layout;
        entry.compressor = segment.compressor;
        entry.bytes = compressed.MaterializeVector();
        output.push_back(std::move(entry));
    }
    return true;
}

inline bool NumericArrayDecode::DecompressSegments(
    const std::vector<NumericArrayCompressedSegment>& input,
    const MutableNumericArrayBufferView& output,
    std::string* error) {
    if (!detail::numericarraycodec::ValidateOutputView(output, error)) {
        return false;
    }

    for (const auto& segment : input) {
        if (!detail::numericarraycodec::ValidateSegmentRange(output.layout, segment.elementOffset, segment.layout.elementCount, error)) {
            return false;
        }

        if (!Decompress(
                std::span<const std::uint8_t>(segment.bytes.data(), segment.bytes.size()),
                segment.layout,
                segment.compressor,
                detail::numericarraycodec::SliceBufferView(output, segment.elementOffset, segment.layout.elementCount),
                error)) {
            return false;
        }
    }
    return true;
}

} // namespace datacodec::numericarray

#endif
