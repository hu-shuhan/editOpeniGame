#ifndef DATACODEC_STORAGE_BYTEIO_BYTERANGE_H
#define DATACODEC_STORAGE_BYTEIO_BYTERANGE_H

#include "DataCodec/Storage/ByteIO/ContiguousView.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

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

enum class ByteRangePrefetchStatus : std::uint8_t {
    Accepted = 0u,
    RejectedByPolicy = 1u,
    Unavailable = 2u,
    Error = 3u,
};

struct ByteRangePrefetchResult {
    ByteRangePrefetchStatus status{ByteRangePrefetchStatus::Unavailable};
    std::string error;

    [[nodiscard]] bool IsAccepted() const noexcept {
        return status == ByteRangePrefetchStatus::Accepted;
    }
    [[nodiscard]] bool IsRejectedByPolicy() const noexcept {
        return status == ByteRangePrefetchStatus::RejectedByPolicy;
    }
    [[nodiscard]] bool IsUnavailable() const noexcept {
        return status == ByteRangePrefetchStatus::Unavailable;
    }
    [[nodiscard]] bool IsError() const noexcept {
        return status == ByteRangePrefetchStatus::Error;
    }
};

class IByteRangeReader {
public:
    virtual ~IByteRangeReader() = default;
    [[nodiscard]] virtual std::uint64_t ByteSize() const noexcept = 0;
    // 可选的连续只读区间能力
    // 返回值的生命周期由 reader 自身保证，空 span 表示当前 reader 不提供连续视图
    [[nodiscard]] virtual std::span<const std::uint8_t> ContiguousRange(
        std::uint64_t offset,
        std::uint64_t byteSize) const noexcept {
        (void)offset;
        (void)byteSize;
        return {};
    }
    virtual ContiguousViewStatus PrepareContiguousRange(
        const std::uint64_t offset,
        const std::uint64_t byteSize,
        std::span<const std::uint8_t>& output,
        std::string* error = nullptr) const {
        output = {};
        if (offset > ByteSize() || byteSize > ByteSize() - offset) {
            validation::AssignError(error, "contiguous byte range is outside the reader");
            return ContiguousViewStatus::Error;
        }
        output = ContiguousRange(offset, byteSize);
        if (byteSize == 0u || output.size() == byteSize) {
            if (error != nullptr) { error->clear(); }
            return ContiguousViewStatus::Ready;
        }
        if (!output.empty()) {
            output = {};
            validation::AssignError(error, "contiguous byte range has an invalid size");
            return ContiguousViewStatus::Error;
        }
        if (error != nullptr) { error->clear(); }
        return ContiguousViewStatus::Unavailable;
    }
    // 可选的完整输入共享所有权能力
    // 返回非空值时调用方可以直接复用既有字节，不需要重新复制整份输入
    [[nodiscard]] virtual std::shared_ptr<const std::vector<std::uint8_t>> RetainAllBytes() const noexcept {
        return {};
    }
    // 可选的输入预取能力
    // 默认实现不改变读取语义，文件桥接可据此提前准备映射页面
    [[nodiscard]] virtual ByteRangePrefetchResult PrefetchRange(
        std::uint64_t offset,
        std::uint64_t byteSize) const {
        (void)offset;
        (void)byteSize;
        return {.status = ByteRangePrefetchStatus::Unavailable};
    }
    virtual bool ReadAt(
        std::uint64_t offset,
        std::span<std::uint8_t> output,
        std::string* error = nullptr) = 0;
};

class IByteRangeOutput {
public:
    virtual ~IByteRangeOutput() = default;
    virtual bool WriteAt(
        std::uint64_t offset,
        std::span<const std::uint8_t> bytes,
        std::string* error = nullptr) = 0;
    virtual bool Finalize(std::uint64_t logicalSize, std::string* error = nullptr) = 0;
};

class MemoryByteRangeReader final : public IByteRangeReader {
public:
    explicit MemoryByteRangeReader(std::span<const std::uint8_t> bytes)
        : m_bytes(std::make_shared<const std::vector<std::uint8_t>>(bytes.begin(), bytes.end())) {}

    explicit MemoryByteRangeReader(std::vector<std::uint8_t> bytes)
        : m_bytes(std::make_shared<const std::vector<std::uint8_t>>(std::move(bytes))) {}

    explicit MemoryByteRangeReader(std::shared_ptr<const std::vector<std::uint8_t>> bytes)
        : m_bytes(std::move(bytes)) {}

    [[nodiscard]] std::uint64_t ByteSize() const noexcept override {
        return m_bytes == nullptr ? 0u : static_cast<std::uint64_t>(m_bytes->size());
    }

    [[nodiscard]] std::shared_ptr<const std::vector<std::uint8_t>> RetainAllBytes() const noexcept override {
        return m_bytes;
    }

    [[nodiscard]] std::span<const std::uint8_t> ContiguousRange(
        const std::uint64_t offset,
        const std::uint64_t byteSize) const noexcept override {
        if (m_bytes == nullptr ||
            offset > m_bytes->size() ||
            byteSize > m_bytes->size() - static_cast<std::size_t>(offset) ||
            byteSize > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
            return {};
        }
        return std::span<const std::uint8_t>(
            m_bytes->data() + static_cast<std::size_t>(offset),
            static_cast<std::size_t>(byteSize));
    }

    bool ReadAt(
        const std::uint64_t offset,
        const std::span<std::uint8_t> output,
        std::string* error = nullptr) override {
        if (m_bytes == nullptr ||
            offset > m_bytes->size() ||
            output.size() > m_bytes->size() - static_cast<std::size_t>(offset)) {
            return validation::AssignError(error, "memory byte range reader read range is outside the buffer");
        }
        if (!output.empty()) {
            std::memcpy(output.data(), m_bytes->data() + static_cast<std::size_t>(offset), output.size());
        }
        return true;
    }

private:
    std::shared_ptr<const std::vector<std::uint8_t>> m_bytes;
};

class SubrangeByteRangeReader final : public IByteRangeReader {
public:
    SubrangeByteRangeReader(
        std::shared_ptr<IByteRangeReader> source,
        const std::uint64_t offset,
        const std::uint64_t byteSize)
        : m_source(std::move(source)), m_offset(offset), m_byteSize(byteSize) {}

    [[nodiscard]] std::uint64_t ByteSize() const noexcept override { return m_byteSize; }

    [[nodiscard]] std::span<const std::uint8_t> ContiguousRange(
        const std::uint64_t offset,
        const std::uint64_t byteSize) const noexcept override {
        if (m_source == nullptr || offset > m_byteSize || byteSize > m_byteSize - offset) {
            return {};
        }
        std::uint64_t sourceOffset = 0u;
        if (!validation::CanAddU64(m_offset, offset)) {
            return {};
        }
        sourceOffset = m_offset + offset;
        return m_source->ContiguousRange(sourceOffset, byteSize);
    }

    ContiguousViewStatus PrepareContiguousRange(
        const std::uint64_t offset,
        const std::uint64_t byteSize,
        std::span<const std::uint8_t>& output,
        std::string* error = nullptr) const override {
        output = {};
        if (m_source == nullptr || offset > m_byteSize || byteSize > m_byteSize - offset) {
            validation::AssignError(error, "subrange contiguous view is outside the selected range");
            return ContiguousViewStatus::Error;
        }
        std::uint64_t sourceOffset = 0u;
        if (!validation::CheckedAddU64(
                m_offset,
                offset,
                sourceOffset,
                "subrange contiguous view offset",
                error)) {
            return ContiguousViewStatus::Error;
        }
        return m_source->PrepareContiguousRange(sourceOffset, byteSize, output, error);
    }

    [[nodiscard]] ByteRangePrefetchResult PrefetchRange(
        const std::uint64_t offset,
        const std::uint64_t byteSize) const override {
        if (m_source == nullptr || offset > m_byteSize || byteSize > m_byteSize - offset ||
            !validation::CanAddU64(m_offset, offset)) {
            return {
                .status = ByteRangePrefetchStatus::Error,
                .error = "subrange prefetch range is invalid",
            };
        }
        return m_source->PrefetchRange(m_offset + offset, byteSize);
    }

    bool ReadAt(
        const std::uint64_t offset,
        const std::span<std::uint8_t> output,
        std::string* error = nullptr) override {
        if (m_source == nullptr || offset > m_byteSize || output.size() > m_byteSize - offset) {
            return validation::AssignError(error, "subrange byte reader read is outside the selected range");
        }
        std::uint64_t sourceOffset = 0u;
        if (!validation::CheckedAddU64(m_offset, offset, sourceOffset, "subrange byte reader offset", error)) {
            return false;
        }
        return m_source->ReadAt(sourceOffset, output, error);
    }

private:
    std::shared_ptr<IByteRangeReader> m_source;
    std::uint64_t m_offset{0u};
    std::uint64_t m_byteSize{0u};
};

class MemoryByteRangeOutput final : public IByteRangeOutput {
public:
    bool WriteAt(
        const std::uint64_t offset,
        const std::span<const std::uint8_t> bytes,
        std::string* error = nullptr) override {
        std::size_t localOffset = 0u;
        std::size_t requiredBytes = 0u;
        if (!validation::CheckedCastSizeT(offset, localOffset, "memory byte range output range", error) ||
            !validation::CheckedAddSizeT(localOffset, bytes.size(), requiredBytes, "memory byte range output range", error)) {
            return false;
        }
        if (m_bytes.size() < requiredBytes) {
            m_bytes.resize(requiredBytes, 0u);
        }
        if (!bytes.empty()) {
            std::memcpy(m_bytes.data() + localOffset, bytes.data(), bytes.size());
        }
        return true;
    }

    bool Finalize(const std::uint64_t logicalSize, std::string* error = nullptr) override {
        if (logicalSize > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
            return validation::AssignError(
                error,
                "memory byte range output logical size exceeds local address space");
        }
        m_bytes.resize(static_cast<std::size_t>(logicalSize), 0u);
        return true;
    }

    [[nodiscard]] const std::vector<std::uint8_t>& Bytes() const noexcept { return m_bytes; }
    [[nodiscard]] std::vector<std::uint8_t> TakeBytes() noexcept { return std::move(m_bytes); }

private:
    std::vector<std::uint8_t> m_bytes;
};

inline bool CopyByteRangeReaderToOutput(
    IByteRangeReader& source,
    IByteRangeOutput& sink,
    const std::size_t windowBytes,
    std::string* error = nullptr) {
    const auto byteSize = source.ByteSize();
    const auto resolvedWindowBytes = std::max<std::size_t>(windowBytes, 1u);
    std::vector<std::uint8_t> buffer(resolvedWindowBytes, 0u);
    std::uint64_t offset = 0u;
    while (offset < byteSize) {
        const auto currentBytes = static_cast<std::size_t>(
            std::min<std::uint64_t>(byteSize - offset, static_cast<std::uint64_t>(resolvedWindowBytes)));
        auto window = std::span<std::uint8_t>(buffer.data(), currentBytes);
        if (!source.ReadAt(offset, window, error) ||
            !sink.WriteAt(offset, std::span<const std::uint8_t>(window.data(), window.size()), error)) {
            return false;
        }
        offset += static_cast<std::uint64_t>(currentBytes);
    }
    return sink.Finalize(byteSize, error);
}

} // namespace datacodec

#endif
