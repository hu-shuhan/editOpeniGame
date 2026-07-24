#ifndef DATACODEC_STORAGE_BYTEIO_BYTESOURCE_H
#define DATACODEC_STORAGE_BYTEIO_BYTESOURCE_H

#include "DataCodec/Storage/ByteIO/ContiguousView.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <span>
#include <stop_token>
#include <string>
#include <utility>
#include <vector>
namespace datacodec::bytestore {

inline constexpr std::uint64_t kUnknownByteSize = std::numeric_limits<std::uint64_t>::max();

inline bool IsUnknownByteSize(const std::uint64_t byteSize) noexcept {
    return byteSize == kUnknownByteSize;
}

class IByteWriter {
public:
    virtual ~IByteWriter() = default;
    virtual bool Write(std::span<const std::uint8_t> bytes, std::string* error = nullptr) = 0;
    [[nodiscard]] virtual std::uint64_t ByteSizeHint() const noexcept { return 0u; }
    [[nodiscard]] virtual std::uint64_t ResidentSizeHint() const noexcept { return 0u; }
};

class IByteSource {
public:
    virtual ~IByteSource() = default;
    [[nodiscard]] virtual std::uint64_t ByteSizeHint() const noexcept = 0;
    [[nodiscard]] virtual std::uint64_t ResidentSizeHint() const noexcept { return ByteSizeHint(); }
    [[nodiscard]] virtual std::uint64_t MappedSizeHint() const noexcept { return 0u; }
    [[nodiscard]] virtual std::span<const std::uint8_t> ContiguousBytes() const noexcept { return {}; }
    [[nodiscard]] virtual bool PreferDirectCopy() const noexcept { return false; }
    virtual ContiguousViewStatus PrepareContiguousBytes(
        std::span<const std::uint8_t>& output,
        std::string* error = nullptr) {
        output = ContiguousBytes();
        if (!output.empty() || ByteSizeHint() == 0u) {
            if (error != nullptr) { error->clear(); }
            return ContiguousViewStatus::Ready;
        }
        if (!CanRead()) {
            validation::AssignError(error, "byte source is not readable");
            return ContiguousViewStatus::Error;
        }
        if (error != nullptr) { error->clear(); }
        return ContiguousViewStatus::Unavailable;
    }
    [[nodiscard]] virtual bool CanRead() const noexcept { return false; }
    virtual bool Read(
        std::uint64_t offset,
        std::span<std::uint8_t> output,
        std::string* error = nullptr) const {
        (void)offset;
        (void)output;
        return validation::AssignError(error, "byte source does not support ranged reads");
    }
    virtual bool CopyTo(IByteWriter& writer, std::string* error = nullptr) = 0;
    virtual void Release() noexcept = 0;
};

enum class ByteSourceConsumptionMode {
    Replayable,
    OneShot,
};

inline bool WaitUntilByteSourceFlagReady(
    const std::atomic<std::uint8_t>& ready,
    const std::stop_token& stopToken) {
    std::stop_callback stopCallback(stopToken, [&ready]() {
        const_cast<std::atomic<std::uint8_t>&>(ready).notify_all();
    });
    for (;;) {
        const auto value = ready.load(std::memory_order_acquire);
        if (value != 0u) {
            return true;
        }
        if (stopToken.stop_requested()) {
            return false;
        }
        ready.wait(0u, std::memory_order_acquire);
    }
}

class VectorByteSource final : public IByteSource {
public:
    VectorByteSource() = default;
    explicit VectorByteSource(std::vector<std::uint8_t> bytes)
        : m_bytes(std::move(bytes)) {}

    [[nodiscard]] std::uint64_t ByteSizeHint() const noexcept override {
        return static_cast<std::uint64_t>(m_bytes.size());
    }
    [[nodiscard]] std::uint64_t ResidentSizeHint() const noexcept override {
        return static_cast<std::uint64_t>(m_bytes.capacity());
    }
    [[nodiscard]] std::span<const std::uint8_t> ContiguousBytes() const noexcept override {
        return std::span<const std::uint8_t>(m_bytes.data(), m_bytes.size());
    }
    [[nodiscard]] bool CanRead() const noexcept override { return true; }

    bool Read(
        const std::uint64_t offset,
        const std::span<std::uint8_t> output,
        std::string* error = nullptr) const override {
        if (offset > m_bytes.size() ||
            output.size() > m_bytes.size() - static_cast<std::size_t>(offset)) {
            return validation::AssignError(error, "vector byte source read is outside the source range");
        }
        if (!output.empty()) {
            std::memcpy(
                output.data(),
                m_bytes.data() + static_cast<std::size_t>(offset),
                output.size());
        }
        return true;
    }

    bool CopyTo(IByteWriter& writer, std::string* error = nullptr) override {
        constexpr std::size_t kWriteWindowBytes = 16u * 1024u * 1024u;
        std::size_t offset = 0u;
        while (offset < m_bytes.size()) {
            const auto remaining = m_bytes.size() - offset;
            const auto currentBytes = remaining < kWriteWindowBytes ? remaining : kWriteWindowBytes;
            if (!writer.Write(std::span<const std::uint8_t>(m_bytes.data() + offset, currentBytes), error)) {
                return false;
            }
            offset += currentBytes;
        }
        return true;
    }

    void Release() noexcept override { std::vector<std::uint8_t>().swap(m_bytes); }

private:
    std::vector<std::uint8_t> m_bytes;
};

} // namespace datacodec::bytestore

#endif
