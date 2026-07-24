#ifndef DATACODEC_CODEC_SUBCODEC_ZSTDCODEC_H
#define DATACODEC_CODEC_SUBCODEC_ZSTDCODEC_H

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <span>
#include <string>
#include <thread>
#include <vector>

#include "DataCodec/Storage/ByteIO/ByteSource.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"
#include "zstd.h"
namespace datacodec::codec {

namespace detail {

struct ZstdCompressionContextDeleter {
    void operator()(ZSTD_CCtx* context) const noexcept {
        if (context != nullptr) {
            ZSTD_freeCCtx(context);
        }
    }
};

using ZstdCompressionContextHandle = std::unique_ptr<ZSTD_CCtx, ZstdCompressionContextDeleter>;

struct ZstdDecompressionContextDeleter {
    void operator()(ZSTD_DCtx* context) const noexcept {
        if (context != nullptr) {
            ZSTD_freeDCtx(context);
        }
    }
};

using ZstdDecompressionContextHandle = std::unique_ptr<ZSTD_DCtx, ZstdDecompressionContextDeleter>;

inline bool AssignZstdResultError(
    std::string* error,
    const char* messagePrefix,
    const std::size_t result) {
    return validation::AssignError(error, std::string(messagePrefix) + ZSTD_getErrorName(result));
}

inline ZstdCompressionContextHandle CreateCompressionContext(
    std::string* error = nullptr,
    const char* message = "zstd failed to create compression context") {
    ZstdCompressionContextHandle context(ZSTD_createCCtx());
    if (context == nullptr) {
        validation::AssignError(error, message);
    }
    return context;
}

inline ZstdDecompressionContextHandle CreateDecompressionContext(
    std::string* error = nullptr,
    const char* message = "zstd failed to create decompression context") {
    ZstdDecompressionContextHandle context(ZSTD_createDCtx());
    if (context == nullptr) {
        validation::AssignError(error, message);
    }
    return context;
}

inline bool ConfigureCompressionContext(
    ZSTD_CCtx& context,
    const int level,
    const std::size_t workerCount,
    std::string* error = nullptr) {
    const auto clampedLevel = level <= 0 ? 1 : level;
    const auto levelResult = ZSTD_CCtx_setParameter(&context, ZSTD_c_compressionLevel, clampedLevel);
    if (ZSTD_isError(levelResult)) {
        return AssignZstdResultError(error, "zstd failed to set compression level: ", levelResult);
    }
    if (workerCount <= 1u) {
        return true;
    }
    const auto workerResult =
        ZSTD_CCtx_setParameter(&context, ZSTD_c_nbWorkers, static_cast<int>(workerCount));
    if (ZSTD_isError(workerResult)) {
        return AssignZstdResultError(error, "zstd failed to set worker count: ", workerResult);
    }
    return true;
}

} // namespace detail

class ZstdCodec {
public:
    [[nodiscard]] static bool IsAvailable() { return true; }

    [[nodiscard]] static std::size_t RecommendWorkerCount(const std::size_t inputBytes) {
        constexpr std::size_t kMaxWorkerCount = 16u;
        const auto hardwareConcurrency = static_cast<std::size_t>(std::thread::hardware_concurrency());
        if (hardwareConcurrency <= 1u) {
            return 1;
        }

        std::size_t requestedWorkerCount = 1;
        if (inputBytes >= 32u * 1024u * 1024u) {
            requestedWorkerCount = 2;
        }
        if (inputBytes >= 128u * 1024u * 1024u) {
            requestedWorkerCount = 4;
        }
        if (inputBytes >= 512u * 1024u * 1024u) {
            requestedWorkerCount = 8;
        }

        if (requestedWorkerCount > kMaxWorkerCount) {
            requestedWorkerCount = kMaxWorkerCount;
        }
        return requestedWorkerCount < hardwareConcurrency ? requestedWorkerCount : hardwareConcurrency;
    }

    static bool Compress(
        const std::span<const std::uint8_t> input,
        const int level,
        std::vector<std::uint8_t>& output,
        std::string* error = nullptr) {
        return Compress(input, level, 1, output, error);
    }

    static bool Compress(
        const std::span<const std::uint8_t> input,
        const int level,
        const std::size_t workerCount,
        std::vector<std::uint8_t>& output,
        std::string* error = nullptr) {
        output.clear();
        if (input.empty()) {
            return true;
        }

        const auto bound = ZSTD_compressBound(input.size());
        output.resize(bound, 0);

        const auto clampedLevel = level <= 0 ? 1 : level;
        const auto resolvedWorkerCount = workerCount == 0 ? RecommendWorkerCount(input.size()) : workerCount;
        size_t compressedSize = 0;
        if (resolvedWorkerCount <= 1u) {
            compressedSize = ZSTD_compress(output.data(), output.size(), input.data(), input.size(), clampedLevel);
        } else {
            auto context = detail::CreateCompressionContext(error);
            if (context == nullptr) {
                output.clear();
                return false;
            }

            if (!detail::ConfigureCompressionContext(*context, clampedLevel, resolvedWorkerCount, error)) {
                output.clear();
                return false;
            }
            compressedSize = ZSTD_compress2(context.get(), output.data(), output.size(), input.data(), input.size());
        }
        if (ZSTD_isError(compressedSize)) {
            output.clear();
            return detail::AssignZstdResultError(error, "zstd compression failed: ", compressedSize);
        }

        output.resize(compressedSize);
        return true;
    }

    static bool Decompress(
        const std::span<const std::uint8_t> input,
        const std::size_t expectedSize,
        std::vector<std::uint8_t>& output,
        std::string* error = nullptr) {
        output.assign(expectedSize, 0);
        if (input.empty()) {
            if (expectedSize == 0) {
                return true;
            }
            output.clear();
            validation::AssignError(error, "zstd input buffer is empty");
            return false;
        }

        const auto decompressedSize =
            ZSTD_decompress(output.data(), output.size(), input.data(), input.size());
        if (ZSTD_isError(decompressedSize)) {
            output.clear();
            return detail::AssignZstdResultError(error, "zstd decompression failed: ", decompressedSize);
        }
        if (decompressedSize != expectedSize) {
            output.clear();
            validation::AssignError(error, "zstd decompression size does not match the encoded block");
            return false;
        }

        return true;
    }
};

class ZstdStreamingEncoder final {
public:
    ~ZstdStreamingEncoder() { Release(); }

    bool Initialize(
        const int level,
        const std::size_t workerCount,
        const std::uint64_t rawBytes,
        std::string* error = nullptr) {
        Release();
        m_context = detail::CreateCompressionContext(
            error,
            "zstd failed to create streaming compression context");
        if (m_context == nullptr) {
            return false;
        }
        m_outputBuffer.resize(ZSTD_CStreamOutSize());

        const auto knownRawBytes = bytestore::IsUnknownByteSize(rawBytes)
            ? 0u
            : static_cast<std::size_t>(std::min<std::uint64_t>(
                rawBytes,
                static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())));
        const auto resolvedWorkerCount = workerCount == 0u
            ? ZstdCodec::RecommendWorkerCount(knownRawBytes)
            : workerCount;
        if (!detail::ConfigureCompressionContext(*m_context, level, resolvedWorkerCount, error)) {
            Release();
            return false;
        }
        return true;
    }

    bool Compress(
        const std::span<const std::uint8_t> input,
        bytestore::IByteWriter& writer,
        std::string* error = nullptr) {
        return CompressInternal(input, ZSTD_e_continue, writer, error);
    }

    bool Finish(bytestore::IByteWriter& writer, std::string* error = nullptr) {
        return CompressInternal({}, ZSTD_e_end, writer, error);
    }

private:
    bool CompressInternal(
        const std::span<const std::uint8_t> input,
        const ZSTD_EndDirective directive,
        bytestore::IByteWriter& writer,
        std::string* error) {
        if (m_context == nullptr) {
            validation::AssignError(error, "zstd streaming context was not initialized");
            return false;
        }
        ZSTD_inBuffer inBuffer{
            input.data(),
            input.size(),
            0u,
        };
        if (m_outputBuffer.empty()) {
            m_outputBuffer.resize(ZSTD_CStreamOutSize());
        }
        do {
            ZSTD_outBuffer outBuffer{
                m_outputBuffer.data(),
                m_outputBuffer.size(),
                0u,
            };
            const auto remaining = ZSTD_compressStream2(m_context.get(), &outBuffer, &inBuffer, directive);
            if (ZSTD_isError(remaining)) {
                return detail::AssignZstdResultError(
                    error,
                    "zstd streaming compression failed: ",
                    remaining);
            }
            if (outBuffer.pos != 0u &&
                !writer.Write(std::span<const std::uint8_t>(m_outputBuffer.data(), outBuffer.pos), error)) {
                return false;
            }
            if (directive == ZSTD_e_end && remaining == 0u) {
                break;
            }
        } while (inBuffer.pos < inBuffer.size || directive == ZSTD_e_end);
        return true;
    }

    void Release() noexcept {
        m_context.reset();
        std::vector<std::uint8_t>().swap(m_outputBuffer);
    }

    detail::ZstdCompressionContextHandle m_context;
    std::vector<std::uint8_t> m_outputBuffer;
};

class ZstdStreamingDecoder final {
public:
    ZstdStreamingDecoder() = default;
    ZstdStreamingDecoder(const ZstdStreamingDecoder&) = delete;
    ZstdStreamingDecoder& operator=(const ZstdStreamingDecoder&) = delete;
    ZstdStreamingDecoder(ZstdStreamingDecoder&&) noexcept = default;
    ZstdStreamingDecoder& operator=(ZstdStreamingDecoder&&) noexcept = default;

    ~ZstdStreamingDecoder() { Release(); }

    bool Initialize(std::string* error = nullptr) {
        Release();
        m_context = detail::CreateDecompressionContext(
            error,
            "zstd failed to create streaming decompression context");
        return m_context != nullptr;
    }

    bool Decompress(
        const std::span<const std::uint8_t> input,
        const std::span<std::uint8_t> output,
        std::size_t& consumedBytes,
        std::size_t& producedBytes,
        bool& frameComplete,
        std::string* error = nullptr) {
        consumedBytes = 0u;
        producedBytes = 0u;
        frameComplete = false;
        if (m_context == nullptr) {
            validation::AssignError(error, "zstd streaming decompression context was not initialized");
            return false;
        }
        ZSTD_inBuffer inputBuffer{
            input.data(),
            input.size(),
            0u,
        };
        ZSTD_outBuffer outputBuffer{
            output.data(),
            output.size(),
            0u,
        };
        const auto remaining = ZSTD_decompressStream(m_context.get(), &outputBuffer, &inputBuffer);
        consumedBytes = inputBuffer.pos;
        producedBytes = outputBuffer.pos;
        if (ZSTD_isError(remaining)) {
            return detail::AssignZstdResultError(
                error,
                "zstd streaming decompression failed: ",
                remaining);
        }
        frameComplete = remaining == 0u;
        return true;
    }

    void Release() noexcept {
        m_context.reset();
    }

private:
    detail::ZstdDecompressionContextHandle m_context;
};

class ZstdInputWriter final : public bytestore::IByteWriter {
public:
    ZstdInputWriter(
        ZstdStreamingEncoder& encoder,
        bytestore::IByteWriter& outputWriter)
        : m_encoder(encoder),
          m_outputWriter(outputWriter) {}

    bool Write(const std::span<const std::uint8_t> bytes, std::string* error = nullptr) override {
        if (!m_encoder.Compress(bytes, m_outputWriter, error)) {
            return false;
        }
        if (!validation::CheckedAddU64(
                m_rawBytes,
                static_cast<std::uint64_t>(bytes.size()),
                m_rawBytes,
                "zstd streaming input size",
                error)) {
            return false;
        }
        return true;
    }

    [[nodiscard]] std::uint64_t ByteSizeHint() const noexcept override { return m_rawBytes; }

private:
    ZstdStreamingEncoder& m_encoder;
    bytestore::IByteWriter& m_outputWriter;
    std::uint64_t m_rawBytes{0u};
};

} // namespace datacodec::codec

#endif
