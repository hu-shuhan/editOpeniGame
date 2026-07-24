#ifndef DATACODEC_STORAGE_LEAFPACKAGE_LEAFPACKAGEFIELDDECODESTREAM_H
#define DATACODEC_STORAGE_LEAFPACKAGE_LEAFPACKAGEFIELDDECODESTREAM_H

#include "DataCodec/Runtime/Cache/CacheResources.h"
#include "DataCodec/Storage/ByteIO/ByteSource.h"
#include "DataCodec/Storage/ByteIO/ScratchByteBuffer.h"
#include "DataCodec/Storage/ByteIO/Window/WindowBudget.h"
#include "DataCodec/Storage/ByteIO/Window/WindowRuntimeParams.h"
#include "DataCodec/Codec/SubCodec/ZstdCodec.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"
#include "DataCodec/Storage/LeafPackage/LeafPackage.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>
namespace datacodec {
namespace decodefield {

struct FieldInputSegment {
    std::uint64_t encodedOffset{0};
    std::span<const std::uint8_t> bytes;
};

struct FieldOutputSegment {
    std::uint64_t rawOffset{0};
    std::span<const std::uint8_t> bytes;
};

class FieldDecodeStreamReader {
public:
    FieldDecodeStreamReader() = default;
    FieldDecodeStreamReader(const FieldDecodeStreamReader&) = delete;
    FieldDecodeStreamReader& operator=(const FieldDecodeStreamReader&) = delete;

    FieldDecodeStreamReader(FieldDecodeStreamReader&& other) noexcept { MoveFrom(std::move(other)); }
    FieldDecodeStreamReader& operator=(FieldDecodeStreamReader&& other) noexcept {
        if (this != &other) {
            Release();
            MoveFrom(std::move(other));
        }
        return *this;
    }

    ~FieldDecodeStreamReader() { Release(); }

    bool Open(
        std::shared_ptr<bytestore::IByteSource> source,
        const std::uint64_t rawSize,
        const CacheResources& runtime,
        std::string* error = nullptr) {
        Release();
        if (source == nullptr) {
            validation::AssignError(error, "zstd decode segment reader received a null byte source");
            return false;
        }
        const auto sourceByteSize = source->ByteSizeHint();
        if (sourceByteSize > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
            validation::AssignError(error, "zstd source bytes exceed local address space");
            return false;
        }
        if (rawSize > 0u && sourceByteSize == 0u) {
            validation::AssignError(error, "zstd source bytes is empty while raw size is non-zero");
            return false;
        }
        if (!source->CanRead()) {
            validation::AssignError(error, "zstd decode byte source must support ranged reads");
            return false;
        }
        m_source = std::move(source);
        m_sourceByteSize = static_cast<std::size_t>(sourceByteSize);
        m_rawSize = rawSize;
        m_scratchBytePool = &runtime.scratchBytePool;
        m_windowBudget = &runtime.windowBudget;
        m_rawMode = false;
        if (m_rawSize == 0u && sourceByteSize == 0u) {
            m_finished = true;
            return true;
        }

        if (!m_zstdDecoder.Initialize(error)) {
            Release();
            return false;
        }
        return true;
    }

    bool OpenRaw(
        std::shared_ptr<bytestore::IByteSource> source,
        const std::uint64_t rawSize,
        const CacheResources& runtime,
        std::string* error = nullptr) {
        Release();
        if (source == nullptr) {
            validation::AssignError(error, "raw decode segment reader received a null byte source");
            return false;
        }
        const auto sourceByteSize = source->ByteSizeHint();
        if (sourceByteSize != rawSize) {
            validation::AssignError(error, "raw decode source size does not match recorded raw size");
            return false;
        }
        if (sourceByteSize > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
            validation::AssignError(error, "raw decode source bytes exceed local address space");
            return false;
        }
        if (!source->CanRead()) {
            validation::AssignError(error, "raw decode byte source must support ranged reads");
            return false;
        }
        m_source = std::move(source);
        m_sourceByteSize = static_cast<std::size_t>(sourceByteSize);
        m_rawSize = rawSize;
        m_scratchBytePool = &runtime.scratchBytePool;
        m_windowBudget = &runtime.windowBudget;
        m_rawMode = true;
        if (m_rawSize == 0u) {
            m_finished = true;
        }
        return true;
    }

    bool ReadNext(
        FieldOutputSegment& segment,
        bool& hasSegment,
        std::string* error = nullptr) {
        segment = {};
        hasSegment = false;
        if (m_source == nullptr) {
            validation::AssignError(error, "zstd decode segment reader is not open");
            return false;
        }
        if (m_rawMode) {
            return ReadRawNext(segment, hasSegment, error);
        }
        if (m_finished) {
            return true;
        }

        while (!m_finished) {
            if (m_inputCursor == m_inputLimit && !FillInput(error)) {
                return false;
            }
            if (m_inputCursor == m_inputLimit && m_inputOffset >= m_sourceByteSize) {
                validation::AssignError(error, "zstd stream ended before the frame was complete");
                return false;
            }

            if (m_scratchBytePool == nullptr || m_windowBudget == nullptr) {
                validation::AssignError(error, "zstd decode window resources are missing");
                return false;
            }
            m_outputScratchBuffer.Release();
            m_outputWindowLease.Release();
            m_outputWindowLease = m_windowBudget->Acquire(kDecodeOutputWindowBytes);
            m_outputScratchBuffer = m_scratchBytePool->Acquire(kDecodeOutputWindowBytes);
            auto outputBuffer = m_outputScratchBuffer.Span();

            std::size_t consumedBytes = 0u;
            std::size_t producedBytes = 0u;
            bool frameComplete = false;
            if (!m_zstdDecoder.Decompress(
                    std::span<const std::uint8_t>(
                        m_currentInputSegment.bytes.data() + m_inputCursor,
                        m_inputLimit - m_inputCursor),
                    outputBuffer,
                    consumedBytes,
                    producedBytes,
                    frameComplete,
                    error)) {
                return false;
            }
            m_inputCursor += consumedBytes;

            if (producedBytes > 0u) {
                // 用减法比较，避免累计输出尺寸溢出
                if (m_outputOffset > m_rawSize ||
                    producedBytes > m_rawSize - m_outputOffset) {
                    validation::AssignError(error, "zstd stream decompressed beyond recorded raw size");
                    return false;
                }
                segment.rawOffset = m_outputOffset;
                segment.bytes = std::span<const std::uint8_t>(outputBuffer.data(), producedBytes);
                m_outputOffset += producedBytes;
                if (frameComplete) {
                    if (!ValidateZstdFrameConsumed(error)) {
                        return false;
                    }
                    m_finished = true;
                    if (m_outputOffset != m_rawSize) {
                        validation::AssignError(error, "zstd stream raw size does not match decoded bytes");
                        return false;
                    }
                }
                hasSegment = true;
                return true;
            }

            if (frameComplete) {
                if (!ValidateZstdFrameConsumed(error)) {
                    return false;
                }
                m_finished = true;
                if (m_outputOffset != m_rawSize) {
                    validation::AssignError(error, "zstd stream raw size does not match decoded bytes");
                    return false;
                }
                return true;
            }

            if (m_inputCursor == m_inputLimit && m_inputOffset >= m_sourceByteSize) {
                validation::AssignError(error, "zstd stream ended before the frame was complete");
                return false;
            }
        }

        return true;
    }

private:
    bool ValidateZstdFrameConsumed(std::string* error) const {
        // field 内只接受一个完整 ZSTD frame，拒绝 frame 后的尾随字节
        if (m_inputCursor != m_inputLimit || m_inputOffset < m_sourceByteSize) {
            validation::AssignError(error, "zstd stream contains trailing encoded bytes");
            return false;
        }
        return true;
    }

    bool ReadRawNext(
        FieldOutputSegment& segment,
        bool& hasSegment,
        std::string* error) {
        if (m_finished) {
            return true;
        }
        if (m_scratchBytePool == nullptr || m_windowBudget == nullptr) {
            validation::AssignError(error, "raw decode window resources are missing");
            return false;
        }
        const auto remainingBytes = m_rawSize - m_outputOffset;
        const auto currentBytes = static_cast<std::size_t>(std::min<std::uint64_t>(
            remainingBytes,
            static_cast<std::uint64_t>(kDecodeInputWindowBytes)));
        if (currentBytes == 0u) {
            m_finished = true;
            return true;
        }

        m_outputScratchBuffer.Release();
        m_outputWindowLease.Release();
        m_outputWindowLease = m_windowBudget->Acquire(currentBytes);
        m_outputScratchBuffer = m_scratchBytePool->Acquire(currentBytes);
        auto outputBuffer = m_outputScratchBuffer.Span();
        if (!m_source->Read(m_outputOffset, outputBuffer, error)) {
            return false;
        }
        segment.rawOffset = m_outputOffset;
        segment.bytes = std::span<const std::uint8_t>(outputBuffer.data(), currentBytes);
        m_outputOffset += currentBytes;
        if (m_outputOffset == m_rawSize) {
            m_finished = true;
        }
        hasSegment = true;
        return true;
    }

    bool FillInput(std::string* error) {
        if (m_inputOffset >= m_sourceByteSize) {
            m_currentInputSegment = {};
            m_inputCursor = 0u;
            m_inputLimit = 0u;
            return true;
        }

        const auto currentBytes = std::min<std::size_t>(
            kDecodeInputWindowBytes,
            m_sourceByteSize - m_inputOffset);
        if (currentBytes == 0u) {
            m_currentInputSegment = {};
            m_inputCursor = 0u;
            m_inputLimit = 0u;
            return true;
        }

        if (m_scratchBytePool == nullptr || m_windowBudget == nullptr) {
            validation::AssignError(error, "zstd decode window resources are missing");
            return false;
        }
        m_inputScratchBuffer.Release();
        m_inputWindowLease.Release();
        m_inputWindowLease = m_windowBudget->Acquire(currentBytes);
        m_inputScratchBuffer = m_scratchBytePool->Acquire(currentBytes);
        auto inputBuffer = m_inputScratchBuffer.Span();
        if (!m_source->Read(
                static_cast<std::uint64_t>(m_inputOffset),
                inputBuffer,
                error)) {
            return false;
        }

        m_currentInputSegment.encodedOffset = static_cast<std::uint64_t>(m_inputOffset);
        m_currentInputSegment.bytes = std::span<const std::uint8_t>(inputBuffer.data(), currentBytes);
        m_inputOffset += currentBytes;
        m_inputCursor = 0u;
        m_inputLimit = currentBytes;
        return true;
    }

    void Release() noexcept {
        m_zstdDecoder.Release();
        m_source.reset();
        m_sourceByteSize = 0u;
        m_rawSize = 0u;
        m_inputOffset = 0u;
        m_inputCursor = 0u;
        m_inputLimit = 0u;
        m_currentInputSegment = {};
        m_outputOffset = 0u;
        m_finished = false;
        m_inputScratchBuffer.Release();
        m_outputScratchBuffer.Release();
        m_inputWindowLease.Release();
        m_outputWindowLease.Release();
        m_scratchBytePool = nullptr;
        m_windowBudget = nullptr;
        m_rawMode = false;
    }

    void MoveFrom(FieldDecodeStreamReader&& other) noexcept {
        m_source = std::move(other.m_source);
        m_zstdDecoder = std::move(other.m_zstdDecoder);
        m_inputScratchBuffer = std::move(other.m_inputScratchBuffer);
        m_outputScratchBuffer = std::move(other.m_outputScratchBuffer);
        m_inputWindowLease = std::move(other.m_inputWindowLease);
        m_outputWindowLease = std::move(other.m_outputWindowLease);
        const auto currentInputBytes = other.m_currentInputSegment.bytes.size();
        m_sourceByteSize = other.m_sourceByteSize;
        m_rawSize = other.m_rawSize;
        m_inputOffset = other.m_inputOffset;
        m_inputCursor = other.m_inputCursor;
        m_inputLimit = other.m_inputLimit;
        m_currentInputSegment = FieldInputSegment{
            other.m_currentInputSegment.encodedOffset,
            std::span<const std::uint8_t>(m_inputScratchBuffer.Bytes().data(), currentInputBytes),
        };
        m_outputOffset = other.m_outputOffset;
        m_finished = other.m_finished;
        m_scratchBytePool = other.m_scratchBytePool;
        m_windowBudget = other.m_windowBudget;
        m_rawMode = other.m_rawMode;
        other.m_source.reset();
        other.m_sourceByteSize = 0u;
        other.m_rawSize = 0u;
        other.m_inputOffset = 0u;
        other.m_inputCursor = 0u;
        other.m_inputLimit = 0u;
        other.m_currentInputSegment = {};
        other.m_outputOffset = 0u;
        other.m_finished = false;
        other.m_scratchBytePool = nullptr;
        other.m_windowBudget = nullptr;
        other.m_rawMode = false;
    }

    std::shared_ptr<bytestore::IByteSource> m_source;
    codec::ZstdStreamingDecoder m_zstdDecoder;
    ScratchByteBuffer m_inputScratchBuffer;
    ScratchByteBuffer m_outputScratchBuffer;
    window::WindowBudget::Lease m_inputWindowLease;
    window::WindowBudget::Lease m_outputWindowLease;
    ScratchByteBufferPool* m_scratchBytePool{nullptr};
    window::WindowBudget* m_windowBudget{nullptr};
    std::size_t m_sourceByteSize{0u};
    std::uint64_t m_rawSize{0u};
    std::size_t m_inputOffset{0};
    std::size_t m_inputCursor{0u};
    std::size_t m_inputLimit{0u};
    FieldInputSegment m_currentInputSegment;
    std::uint64_t m_outputOffset{0};
    bool m_finished{false};
    bool m_rawMode{false};
};

inline bool OpenLeafPackageFieldDecodeStream(
    const LeafPackageField& field,
    const CacheResources& runtime,
    FieldDecodeStreamReader& reader,
    std::string* error = nullptr) {
    if (field.source == nullptr) {
        validation::AssignError(error, "leaf package field is missing its byte source");
        return false;
    }
    if (field.compressionType == EncodedFieldCompressionType::ZSTD) {
        return reader.Open(field.source, field.rawSize, runtime, error);
    }
    if (field.compressionType == EncodedFieldCompressionType::None) {
        return reader.OpenRaw(field.source, field.rawSize, runtime, error);
    }
    validation::AssignError(error, "leaf package field uses an unsupported compression type");
    return false;
}

class FieldDecodeByteStream {
public:
    explicit FieldDecodeByteStream(FieldDecodeStreamReader& reader) : m_reader(reader) {}

    [[nodiscard]] std::uint64_t Position() const noexcept { return m_position; }

    bool ReadBytes(void* target, const std::size_t byteCount, std::string* error = nullptr) {
        auto* output = static_cast<std::uint8_t*>(target);
        std::size_t copied = 0u;
        while (copied < byteCount) {
            if (!EnsureSegment(error)) {
                return false;
            }
            if (m_available.empty()) {
                validation::AssignError(error, "unexpected end of decoded field stream");
                return false;
            }

            const auto currentBytes = std::min(byteCount - copied, m_available.size());
            std::memcpy(output + copied, m_available.data(), currentBytes);
            m_available = m_available.subspan(currentBytes);
            copied += currentBytes;
            m_position += currentBytes;
        }
        return true;
    }

    bool ReadVector(std::vector<std::uint8_t>& output, const std::size_t byteCount, std::string* error = nullptr) {
        output.assign(byteCount, 0u);
        if (byteCount == 0u) {
            return true;
        }
        return ReadBytes(output.data(), output.size(), error);
    }

    template<typename T>
    bool ReadScalar(T& value, std::string* error = nullptr) {
        value = {};
        return ReadBytes(&value, sizeof(T), error);
    }

    bool Skip(const std::uint64_t byteCount, std::string* error = nullptr) {
        constexpr std::size_t kSkipBufferBytes = 64u * 1024u;
        std::vector<std::uint8_t> scratch(kSkipBufferBytes);
        std::uint64_t skipped = 0u;
        while (skipped < byteCount) {
            const auto currentBytes = static_cast<std::size_t>(
                std::min<std::uint64_t>(byteCount - skipped, scratch.size()));
            if (!ReadBytes(scratch.data(), currentBytes, error)) {
                return false;
            }
            skipped += currentBytes;
        }
        return true;
    }

private:
    bool EnsureSegment(std::string* error) {
        while (m_available.empty() && !m_eof) {
            FieldOutputSegment segment;
            bool hasSegment = false;
            if (!m_reader.ReadNext(segment, hasSegment, error)) {
                return false;
            }
            if (!hasSegment) {
                m_eof = true;
                return true;
            }
            m_available = segment.bytes;
        }
        return true;
    }

    FieldDecodeStreamReader& m_reader;
    std::span<const std::uint8_t> m_available;
    std::uint64_t m_position{0};
    bool m_eof{false};
};

} // namespace decodefield
} // namespace datacodec

#endif
