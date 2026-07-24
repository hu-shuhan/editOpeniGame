#ifndef DATACODEC_STORAGE_BYTESTORE_BYTESTORE_H
#define DATACODEC_STORAGE_BYTESTORE_BYTESTORE_H

#include "DataCodec/Storage/ByteIO/ByteSource.h"
#include "DataCodec/Storage/ByteIO/ByteBudget.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <mutex>
#include <new>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>
#if !defined(__EMSCRIPTEN__)
#include <mio/mmap.hpp>
#endif
namespace datacodec {
namespace bytestore {

inline std::uint64_t NextByteStoreSessionId() noexcept {
    static std::atomic<std::uint64_t> nextId{0u};
    return nextId.fetch_add(1u, std::memory_order_relaxed) + 1u;
}

class IAppendableByteStore : public IByteSource {
public:
    virtual bool AppendBytes(std::span<const std::uint8_t> bytes, std::string* error = nullptr) = 0;
    virtual bool Seal(std::string* error = nullptr) = 0;

    bool AppendBytes(std::vector<std::uint8_t> bytes, std::string* error = nullptr) {
        const auto ok = AppendBytes(std::span<const std::uint8_t>(bytes.data(), bytes.size()), error);
        std::vector<std::uint8_t>().swap(bytes);
        return ok;
    }
};

class IRandomAccessByteStore : public IAppendableByteStore {
public:
    virtual bool ResizeBytes(std::uint64_t byteSize, std::string* error = nullptr) = 0;
    virtual bool WriteBytesAt(
        std::uint64_t offset,
        std::span<const std::uint8_t> bytes,
        std::string* error = nullptr) = 0;
};

class AppendableByteStoreWriter final : public IByteWriter {
public:
    explicit AppendableByteStoreWriter(std::shared_ptr<IAppendableByteStore> store)
        : m_store(std::move(store)) {}

    bool Write(const std::span<const std::uint8_t> bytes, std::string* error = nullptr) override {
        if (m_store == nullptr) {
            return validation::AssignError(error, "byte store writer is missing its backing store");
        }
        return m_store->AppendBytes(bytes, error);
    }

    [[nodiscard]] std::uint64_t ByteSizeHint() const noexcept override {
        return m_store != nullptr ? m_store->ByteSizeHint() : 0u;
    }

    [[nodiscard]] std::uint64_t ResidentSizeHint() const noexcept override {
        return m_store != nullptr ? m_store->ResidentSizeHint() : 0u;
    }

private:
    std::shared_ptr<IAppendableByteStore> m_store;
};

class IByteStore : public IRandomAccessByteStore {
public:
    virtual bool Append(std::span<const std::uint8_t> bytes, std::string* error = nullptr) = 0;
    virtual bool Resize(std::uint64_t byteSize, std::string* error = nullptr) = 0;
    virtual bool WriteAt(
        std::uint64_t offset,
        std::span<const std::uint8_t> bytes,
        std::string* error = nullptr) = 0;
    bool AppendBytes(std::span<const std::uint8_t> bytes, std::string* error = nullptr) override {
        return Append(bytes, error);
    }
    bool ResizeBytes(std::uint64_t byteSize, std::string* error = nullptr) override {
        return Resize(byteSize, error);
    }
    bool WriteBytesAt(
        std::uint64_t offset,
        std::span<const std::uint8_t> bytes,
        std::string* error = nullptr) override {
        return WriteAt(offset, bytes, error);
    }
};

class MemoryStore final : public IByteStore {
public:
    explicit MemoryStore(
        std::shared_ptr<resource::ResidentByteBudget> residentBudget = nullptr,
        const bool requireSealBeforeRead = false) noexcept
        : m_residentBudget(std::move(residentBudget)),
          m_requireSealBeforeRead(requireSealBeforeRead) {}
    ~MemoryStore() override { Release(); }

    bool Append(const std::span<const std::uint8_t> bytes, std::string* error = nullptr) override {
        if (m_released) {
            return validation::AssignError(error, "memory store was already released");
        }
        if (m_requireSealBeforeRead && m_sealed) {
            return validation::AssignError(error, "memory store was already sealed");
        }
        if (bytes.empty()) {
            return true;
        }
        const auto oldSize = m_size;
        std::size_t newSize = 0u;
        if (!validation::CheckedAddSizeT(oldSize, bytes.size(), newSize, "memory store", error)) {
            return false;
        }
        if (!ReserveCapacity(newSize, error)) {
            return false;
        }
        std::memcpy(m_bytes.get() + oldSize, bytes.data(), bytes.size());
        m_size = newSize;
        m_sealed = false;
        return true;
    }

    bool Resize(const std::uint64_t byteSize, std::string* error = nullptr) override {
        if (m_released) {
            return validation::AssignError(error, "memory store was already released");
        }
        if (m_requireSealBeforeRead && m_sealed) {
            return validation::AssignError(error, "memory store was already sealed");
        }
        std::size_t localByteSize = 0u;
        if (!validation::CheckedCastSizeT(byteSize, localByteSize, "memory store resize", error)) {
            return false;
        }
        if (localByteSize > m_capacity) {
            if (!ReserveCapacity(localByteSize, error)) {
                return false;
            }
        } else if (localByteSize < m_capacity) {
            std::unique_ptr<std::uint8_t[]> resized;
            if (localByteSize != 0u) {
                resized.reset(new (std::nothrow) std::uint8_t[localByteSize]);
                if (resized == nullptr) {
                    return validation::AssignError(
                        error,
                        "memory store shrink failed because memory is exhausted");
                }
                if (m_bytes != nullptr && m_size != 0u) {
                    std::memcpy(resized.get(), m_bytes.get(), std::min(m_size, localByteSize));
                }
            }
            const auto releasedBytes = static_cast<std::uint64_t>(m_capacity - localByteSize);
            m_bytes = std::move(resized);
            m_capacity = localByteSize;
            if (m_residentBudget != nullptr) {
                m_residentBudget->Release(releasedBytes);
            }
        }
        m_size = localByteSize;
        m_released = false;
        m_sealed = false;
        return true;
    }

    bool WriteAt(
        const std::uint64_t offset,
        const std::span<const std::uint8_t> bytes,
        std::string* error = nullptr) override {
        if (m_released ||
            (m_requireSealBeforeRead && m_sealed) ||
            offset > m_size ||
            bytes.size() > m_size - static_cast<std::size_t>(offset)) {
            return validation::AssignError(error, "memory store write is outside the store range");
        }
        if (!bytes.empty()) {
            std::memcpy(m_bytes.get() + static_cast<std::size_t>(offset), bytes.data(), bytes.size());
        }
        m_sealed = false;
        return true;
    }

    bool Seal(std::string* error = nullptr) override {
        if (m_released) {
            return validation::AssignError(error, "memory store was already released");
        }
        m_sealed = true;
        return true;
    }

    [[nodiscard]] std::uint64_t ByteSizeHint() const noexcept override {
        return static_cast<std::uint64_t>(m_size);
    }
    [[nodiscard]] std::uint64_t ResidentSizeHint() const noexcept override {
        return static_cast<std::uint64_t>(m_capacity);
    }
    [[nodiscard]] std::span<const std::uint8_t> ContiguousBytes() const noexcept override {
        return CanRead()
            ? std::span<const std::uint8_t>(m_bytes.get(), m_size)
            : std::span<const std::uint8_t>{};
    }
    [[nodiscard]] bool CanRead() const noexcept override {
        return !m_released && (!m_requireSealBeforeRead || m_sealed);
    }

    bool Read(
        const std::uint64_t offset,
        const std::span<std::uint8_t> output,
        std::string* error = nullptr) const override {
        if (!CanRead()) {
            return validation::AssignError(error, "memory store is not sealed for reading");
        }
        if (offset > m_size || output.size() > m_size - static_cast<std::size_t>(offset)) {
            return validation::AssignError(error, "memory store read is outside the store range");
        }
        if (!output.empty()) {
            std::memcpy(output.data(), m_bytes.get() + static_cast<std::size_t>(offset), output.size());
        }
        return true;
    }

    bool CopyTo(IByteWriter& writer, std::string* error = nullptr) override {
        if (!CanRead()) {
            return validation::AssignError(error, "memory store is not sealed for copying");
        }
        constexpr std::size_t kWindowBytes = 16u * 1024u * 1024u;
        std::size_t offset = 0u;
        while (offset < m_size) {
            const auto currentBytes = std::min<std::size_t>(m_size - offset, kWindowBytes);
            if (!writer.Write(std::span<const std::uint8_t>(m_bytes.get() + offset, currentBytes), error)) {
                return false;
            }
            offset += currentBytes;
        }
        return true;
    }

    void Release() noexcept override {
        if (m_residentBudget != nullptr) {
            m_residentBudget->Release(static_cast<std::uint64_t>(m_capacity));
        }
        m_bytes.reset();
        m_size = 0u;
        m_capacity = 0u;
        m_sealed = false;
        m_released = true;
    }

private:
    bool ReserveCapacity(const std::size_t requiredBytes, std::string* error) {
        if (requiredBytes <= m_capacity) {
            return true;
        }
        auto targetCapacity = requiredBytes;
        if (m_capacity != 0u) {
            const auto growth = std::max<std::size_t>(m_capacity / 2u, 1u);
            if (growth <= std::numeric_limits<std::size_t>::max() - m_capacity) {
                targetCapacity = std::max(requiredBytes, m_capacity + growth);
            }
        }
        auto reservedBytes = static_cast<std::uint64_t>(targetCapacity - m_capacity);
        bool reserved = m_residentBudget == nullptr || m_residentBudget->TryReserve(reservedBytes);
        if (!reserved && targetCapacity != requiredBytes) {
            targetCapacity = requiredBytes;
            reservedBytes = static_cast<std::uint64_t>(targetCapacity - m_capacity);
            reserved = m_residentBudget->TryReserve(reservedBytes);
        }
        if (!reserved) {
            return validation::AssignError(error, "memory store resident limit was exceeded");
        }
        std::unique_ptr<std::uint8_t[]> expanded(
            new (std::nothrow) std::uint8_t[targetCapacity]);
        if (expanded == nullptr) {
            if (m_residentBudget != nullptr) {
                m_residentBudget->Release(reservedBytes);
            }
            return validation::AssignError(error, "memory store allocation failed because memory is exhausted");
        }
        if (m_bytes != nullptr && m_size != 0u) {
            std::memcpy(expanded.get(), m_bytes.get(), m_size);
        }
        m_bytes = std::move(expanded);
        m_capacity = targetCapacity;
        return true;
    }

    std::shared_ptr<resource::ResidentByteBudget> m_residentBudget;
    std::unique_ptr<std::uint8_t[]> m_bytes;
    std::size_t m_size{0u};
    std::size_t m_capacity{0u};
    bool m_requireSealBeforeRead{false};
    bool m_sealed{false};
    bool m_released{false};
};

class FileBackedStreamStore final : public IByteStore {
public:
    explicit FileBackedStreamStore(
        std::filesystem::path path,
        const bool requireSealBeforeRead = false)
        : m_path(std::move(path)),
          m_requireSealBeforeRead(requireSealBeforeRead) {}

    ~FileBackedStreamStore() override { Release(); }

    bool Append(const std::span<const std::uint8_t> bytes, std::string* error = nullptr) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_released) {
            return validation::AssignError(error, "file-backed stream store was already released");
        }
        if (m_requireSealBeforeRead && m_sealed) {
            return validation::AssignError(error, "file-backed stream store was already sealed");
        }
        std::uint64_t nextSize = 0u;
        if (!validation::CheckedAddU64(
                m_byteSize,
                static_cast<std::uint64_t>(bytes.size()),
                nextSize,
                "file-backed stream store append",
                error)) {
            return false;
        }
        if (!bytes.empty() && !WriteFileRangeUnlocked(m_byteSize, bytes, error)) {
            return false;
        }
        m_byteSize = nextSize;
        m_sealed = false;
        return true;
    }

    bool Resize(const std::uint64_t byteSize, std::string* error = nullptr) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_released) {
            return validation::AssignError(error, "file-backed stream store was already released");
        }
        if (m_requireSealBeforeRead && m_sealed) {
            return validation::AssignError(error, "file-backed stream store was already sealed");
        }
        if (m_file.is_open()) {
            m_file.close();
        }
        m_writeOffsetKnown = false;
        std::error_code resizeError;
        std::filesystem::resize_file(m_path, byteSize, resizeError);
        if (resizeError) {
            return validation::AssignError(
                error,
                "failed to resize file-backed stream store: " + resizeError.message());
        }
        m_byteSize = byteSize;
        m_sealed = false;
        return true;
    }

    bool WriteAt(
        const std::uint64_t offset,
        const std::span<const std::uint8_t> bytes,
        std::string* error = nullptr) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_released ||
            (m_requireSealBeforeRead && m_sealed) ||
            offset > m_byteSize ||
            bytes.size() > m_byteSize - offset) {
            return validation::AssignError(error, "file-backed stream store write is outside the store range");
        }
        if (!bytes.empty() && !WriteFileRangeUnlocked(offset, bytes, error)) {
            return false;
        }
        m_sealed = false;
        return true;
    }

    bool Seal(std::string* error = nullptr) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_released) {
            return validation::AssignError(error, "file-backed stream store was already released");
        }
        if (!EnsureFileOpenUnlocked(error)) {
            return false;
        }
        m_file.flush();
        if (!m_file) {
            return validation::AssignError(error, "failed to flush file-backed stream store");
        }
        CloseFileUnlocked();
        m_sealed = true;
        return true;
    }

    [[nodiscard]] std::uint64_t ByteSizeHint() const noexcept override { return m_byteSize; }
    [[nodiscard]] std::uint64_t ResidentSizeHint() const noexcept override { return 0u; }
    [[nodiscard]] std::uint64_t MappedSizeHint() const noexcept override { return 0u; }
    [[nodiscard]] std::span<const std::uint8_t> ContiguousBytes() const noexcept override { return {}; }
    [[nodiscard]] bool PreferDirectCopy() const noexcept override { return false; }
    [[nodiscard]] bool CanRead() const noexcept override {
        return !m_released && (!m_requireSealBeforeRead || m_sealed);
    }

    bool Read(
        const std::uint64_t offset,
        const std::span<std::uint8_t> output,
        std::string* error = nullptr) const override {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!CanRead()) {
            return validation::AssignError(error, "file-backed stream store is not sealed for reading");
        }
        if (m_released || offset > m_byteSize || output.size() > m_byteSize - offset) {
            return validation::AssignError(error, "file-backed stream store read is outside the store range");
        }
        return ReadFileRangeUnlocked(offset, output, error);
    }

    bool CopyTo(IByteWriter& writer, std::string* error = nullptr) override {
        constexpr std::size_t kWindowBytes = 16u * 1024u * 1024u;
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!CanRead()) {
            return validation::AssignError(error, "file-backed stream store is not sealed for copying");
        }
        std::vector<std::uint8_t> window(kWindowBytes);
        std::uint64_t offset = 0u;
        while (offset < m_byteSize) {
            const auto currentBytes = static_cast<std::size_t>(
                std::min<std::uint64_t>(m_byteSize - offset, window.size()));
            const auto output = std::span<std::uint8_t>(window.data(), currentBytes);
            if (!ReadFileRangeUnlocked(offset, output, error) ||
                !writer.Write(std::span<const std::uint8_t>(output.data(), output.size()), error)) {
                CloseFileUnlocked();
                return false;
            }
            offset += currentBytes;
        }
        CloseFileUnlocked();
        return true;
    }

    void Release() noexcept override {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_released) {
            return;
        }
        if (m_file.is_open()) {
            m_file.close();
        }
        std::error_code removeError;
        std::filesystem::remove(m_path, removeError);
        m_byteSize = 0u;
        m_sealed = false;
        m_released = true;
    }

private:
    void CloseFileUnlocked() const noexcept {
        if (m_file.is_open()) {
            m_file.close();
        }
        m_writeOffsetKnown = false;
    }

    bool EnsureFileOpenUnlocked(std::string* error) const {
        if (m_file.is_open()) {
            return true;
        }
        m_file.open(m_path, std::ios::binary | std::ios::in | std::ios::out);
        if (!m_file) {
            return validation::AssignError(error, "failed to open file-backed stream store");
        }
        m_writeOffsetKnown = false;
        return true;
    }

    bool WriteFileRangeUnlocked(
        const std::uint64_t offset,
        const std::span<const std::uint8_t> bytes,
        std::string* error) const {
        if (!EnsureFileOpenUnlocked(error)) {
            return false;
        }
        std::uint64_t nextOffset = 0u;
        if (!validation::CheckedAddU64(
                offset,
                static_cast<std::uint64_t>(bytes.size()),
                nextOffset,
                "file-backed stream store write position",
                error)) {
            return false;
        }
        if (!m_writeOffsetKnown || m_writeOffset != offset) {
            m_file.clear();
            m_file.seekp(static_cast<std::streamoff>(offset), std::ios::beg);
            if (!m_file) {
                return validation::AssignError(error, "failed to seek file-backed stream store");
            }
        }
        m_file.write(
            reinterpret_cast<const char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
        if (!m_file) {
            return validation::AssignError(error, "failed to write file-backed stream store");
        }
        m_writeOffset = nextOffset;
        m_writeOffsetKnown = true;
        return true;
    }

    bool ReadFileRangeUnlocked(
        const std::uint64_t offset,
        const std::span<std::uint8_t> output,
        std::string* error) const {
        if (output.empty()) {
            return true;
        }
        if (!EnsureFileOpenUnlocked(error)) {
            return false;
        }
        m_file.flush();
        m_file.clear();
        m_file.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
        m_writeOffsetKnown = false;
        m_file.read(
            reinterpret_cast<char*>(output.data()),
            static_cast<std::streamsize>(output.size()));
        if (!m_file) {
            return validation::AssignError(error, "failed to read file-backed stream store");
        }
        return true;
    }

    std::filesystem::path m_path;
    mutable std::fstream m_file;
    std::uint64_t m_byteSize{0u};
    bool m_requireSealBeforeRead{false};
    bool m_sealed{false};
    bool m_released{false};
    mutable std::uint64_t m_writeOffset{0u};
    mutable bool m_writeOffsetKnown{false};
    mutable std::mutex m_mutex;
};
#if !defined(__EMSCRIPTEN__)
class FileBackedMMapStore final : public IByteStore {
public:
    explicit FileBackedMMapStore(std::filesystem::path path)
        : m_path(std::move(path)) {}

    ~FileBackedMMapStore() override { Release(); }

    bool Append(const std::span<const std::uint8_t> bytes, std::string* error = nullptr) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_released) {
            return validation::AssignError(error, "file-backed mmap store was already released");
        }
        std::uint64_t nextSize = 0u;
        if (!validation::CheckedAddU64(
                m_byteSize,
                static_cast<std::uint64_t>(bytes.size()),
                nextSize,
                "file-backed mmap store append",
                error) ||
            !ResizeUnlocked(nextSize, error)) {
            return false;
        }
        if (!bytes.empty()) {
            std::memcpy(m_mapping.data() + static_cast<std::size_t>(m_byteSize - bytes.size()), bytes.data(), bytes.size());
        }
        m_sealed = false;
        return true;
    }

    bool Resize(const std::uint64_t byteSize, std::string* error = nullptr) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_released) {
            return validation::AssignError(error, "file-backed mmap store was already released");
        }
        return ResizeUnlocked(byteSize, error);
    }

    bool WriteAt(
        const std::uint64_t offset,
        const std::span<const std::uint8_t> bytes,
        std::string* error = nullptr) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_released || offset > m_byteSize || bytes.size() > m_byteSize - offset) {
            return validation::AssignError(error, "file-backed mmap store write is outside the store range");
        }
        if (!bytes.empty()) {
            std::memcpy(m_mapping.data() + static_cast<std::size_t>(offset), bytes.data(), bytes.size());
        }
        m_sealed = false;
        return true;
    }

    bool Seal(std::string* error = nullptr) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_released) {
            return validation::AssignError(error, "file-backed mmap store was already released");
        }
        if (m_mapping.is_mapped()) {
            std::error_code mappingError;
            m_mapping.sync(mappingError);
            if (mappingError) {
                return validation::AssignError(error, "failed to flush file-backed mmap store: " + mappingError.message());
            }
        }
        m_sealed = true;
        return true;
    }

    [[nodiscard]] std::uint64_t ByteSizeHint() const noexcept override { return m_byteSize; }
    [[nodiscard]] std::uint64_t ResidentSizeHint() const noexcept override { return 0u; }
    [[nodiscard]] std::uint64_t MappedSizeHint() const noexcept override {
        return m_mapping.is_mapped() ? m_byteSize : 0u;
    }
    [[nodiscard]] std::span<const std::uint8_t> ContiguousBytes() const noexcept override {
        if (m_released || m_byteSize == 0u) {
            return {};
        }
        return std::span<const std::uint8_t>(
            reinterpret_cast<const std::uint8_t*>(m_mapping.data()),
            static_cast<std::size_t>(m_byteSize));
    }
    [[nodiscard]] bool PreferDirectCopy() const noexcept override { return true; }
    [[nodiscard]] bool CanRead() const noexcept override { return !m_released; }

    bool Read(
        const std::uint64_t offset,
        const std::span<std::uint8_t> output,
        std::string* error = nullptr) const override {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_released || offset > m_byteSize || output.size() > m_byteSize - offset) {
            return validation::AssignError(error, "file-backed mmap store read is outside the store range");
        }
        if (!output.empty()) {
            std::memcpy(output.data(), m_mapping.data() + static_cast<std::size_t>(offset), output.size());
        }
        return true;
    }

    bool CopyTo(IByteWriter& writer, std::string* error = nullptr) override {
        constexpr std::size_t kWindowBytes = 16u * 1024u * 1024u;
        std::lock_guard<std::mutex> lock(m_mutex);
        std::uint64_t offset = 0u;
        while (offset < m_byteSize) {
            const auto currentBytes = static_cast<std::size_t>(
                std::min<std::uint64_t>(m_byteSize - offset, kWindowBytes));
            if (!writer.Write(
                    std::span<const std::uint8_t>(
                        reinterpret_cast<const std::uint8_t*>(m_mapping.data()) + static_cast<std::size_t>(offset),
                        currentBytes),
                    error)) {
                return false;
            }
            offset += currentBytes;
        }
        return true;
    }

    void Release() noexcept override {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_released) {
            return;
        }
        if (m_mapping.is_mapped()) {
            m_mapping.unmap();
        }
        std::error_code removeError;
        std::filesystem::remove(m_path, removeError);
        m_byteSize = 0u;
        m_sealed = false;
        m_released = true;
    }

private:
    bool ResizeUnlocked(const std::uint64_t byteSize, std::string* error) {
        if (byteSize > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
            return validation::AssignError(error, "file-backed mmap store exceeds local address space");
        }
        if (m_mapping.is_mapped()) {
            m_mapping.unmap();
        }
        std::error_code resizeError;
        std::filesystem::resize_file(m_path, byteSize, resizeError);
        if (resizeError) {
            return validation::AssignError(error, "failed to resize file-backed mmap store: " + resizeError.message());
        }
        m_byteSize = byteSize;
        if (byteSize != 0u) {
            std::error_code mappingError;
            m_mapping.map(m_path.string(), 0u, mio::map_entire_file, mappingError);
            if (mappingError) {
                return validation::AssignError(error, "failed to map file-backed byte store: " + mappingError.message());
            }
        }
        m_sealed = false;
        return true;
    }

    std::filesystem::path m_path;
    mio::mmap_sink m_mapping;
    std::uint64_t m_byteSize{0u};
    bool m_sealed{false};
    bool m_released{false};
    mutable std::mutex m_mutex;
};
#endif

struct ByteStoreSessionStats {
    std::uint64_t logicalBytes{0u};
    std::uint64_t residentBytes{0u};
    std::uint64_t mappedBytes{0u};
    std::uint64_t managedFileBytes{0u};
    std::uint64_t peakResidentBytes{0u};
    std::uint64_t residentLimitBytes{0u};
    std::size_t storeCount{0u};
};

class ByteStoreSession final {
public:
    ByteStoreSession() = default;
    ~ByteStoreSession() { ReleaseAll(); }
    ByteStoreSession(const ByteStoreSession&) = delete;
    ByteStoreSession& operator=(const ByteStoreSession&) = delete;

    ByteStoreSession(ByteStoreSession&& other) {
        MoveFrom(std::move(other));
    }

    ByteStoreSession& operator=(ByteStoreSession&& other) {
        if (this != &other) {
            ReleaseAll();
            MoveFrom(std::move(other));
        }
        return *this;
    }

    void Reset() {
        ReleaseAll();
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_residentBudget->Reset(m_residentLimitBytes);
            m_sequence = 0u;
            m_sessionId = NextByteStoreSessionId();
            m_diagnostics.clear();
            m_released = false;
        }
    }

    void ConfigureResidentLimit(const std::uint64_t limitBytes) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_stores.empty()) {
            if (m_residentLimitBytes == std::max<std::uint64_t>(limitBytes, 1u)) {
                return;
            }
            throw std::logic_error("byte store resident limit must be configured before stores are created");
        }
        m_residentLimitBytes = std::max<std::uint64_t>(limitBytes, 1u);
        m_residentBudget->Reset(m_residentLimitBytes);
    }

    [[nodiscard]] std::shared_ptr<MemoryStore> CreateMemoryStore(
        const bool requireSealBeforeRead = false) {
        auto store = std::make_shared<MemoryStore>(m_residentBudget, requireSealBeforeRead);
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_released) {
                return nullptr;
            }
            m_stores.push_back(store);
        }
        return store;
    }

    [[nodiscard]] std::shared_ptr<IByteStore> CreateManagedByteStore(
        const std::string& label = "store",
        std::string* error = nullptr) {
        const auto path = CreateManagedStorePath(label, error);
        if (!path.has_value()) {
            return nullptr;
        }
#if defined(__EMSCRIPTEN__)
        auto store = std::make_shared<FileBackedStreamStore>(*path, false);
#else
        auto store = std::make_shared<FileBackedMMapStore>(*path);
#endif
        if (!RegisterStore(store, error)) {
            return nullptr;
        }
        return store;
    }

    [[nodiscard]] std::shared_ptr<IAppendableByteStore> CreateAppendableManagedByteStore(
        const std::string& label = "store",
        std::string* error = nullptr) {
        const auto path = CreateManagedStorePath(label, error);
        if (!path.has_value()) {
            return nullptr;
        }
        auto store = std::make_shared<FileBackedStreamStore>(*path, true);
        if (!RegisterStore(store, error)) {
            return nullptr;
        }
        return store;
    }

    [[nodiscard]] ByteStoreSessionStats SnapshotStats() const noexcept {
        std::lock_guard<std::mutex> lock(m_mutex);
        ByteStoreSessionStats stats;
        stats.storeCount = m_stores.size();
        const auto residentStats = m_residentBudget->SnapshotStats();
        stats.peakResidentBytes = residentStats.peakResidentBytes;
        stats.residentLimitBytes = residentStats.limitBytes;
        for (const auto& store : m_stores) {
            if (store == nullptr) {
                continue;
            }
            stats.logicalBytes = validation::SaturatingAddU64(
                stats.logicalBytes,
                store->ByteSizeHint());
            stats.residentBytes = validation::SaturatingAddU64(
                stats.residentBytes,
                store->ResidentSizeHint());
            stats.mappedBytes = validation::SaturatingAddU64(
                stats.mappedBytes,
                store->MappedSizeHint());
            if (dynamic_cast<const MemoryStore*>(store.get()) == nullptr) {
                stats.managedFileBytes = validation::SaturatingAddU64(
                    stats.managedFileBytes,
                    store->ByteSizeHint());
            }
        }
        return stats;
    }

    void Register(std::shared_ptr<IByteSource> source) {
        if (source != nullptr) {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_released) {
                source->Release();
                return;
            }
            m_stores.push_back(std::move(source));
        }
    }

    void ReleaseAll() noexcept {
        std::vector<std::shared_ptr<IByteSource>> stores;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_released = true;
            stores.swap(m_stores);
        }
        for (auto& store : stores) {
            if (store != nullptr) {
                store->Release();
            }
        }
    }

    [[nodiscard]] std::vector<std::string> TakeDiagnostics() {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto diagnostics = std::move(m_diagnostics);
        m_diagnostics.clear();
        return diagnostics;
    }

private:
    [[nodiscard]] std::optional<std::filesystem::path> CreateManagedStorePath(
        const std::string& label,
        std::string* error) {
        std::uint64_t sequence = 0u;
        std::uint64_t sessionId = 0u;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_released) {
                validation::AssignError(error, "failed to create byte store in a released session");
                return std::nullopt;
            }
            sequence = ++m_sequence;
            sessionId = m_sessionId;
        }
        std::string safeLabel;
        safeLabel.reserve(label.size());
        for (const auto character : label) {
            safeLabel.push_back(
                (character >= '0' && character <= '9') ||
                    (character >= 'A' && character <= 'Z') ||
                    (character >= 'a' && character <= 'z') ||
                    character == '_'
                ? character
                : '_');
        }
        if (safeLabel.empty()) {
            safeLabel = "store";
        }
        std::filesystem::path tempDirectory;
#if defined(__EMSCRIPTEN__)
        tempDirectory = "/tmp";
#else
        std::error_code pathError;
        tempDirectory = std::filesystem::temp_directory_path(pathError);
        if (pathError) {
            validation::AssignError(error, "failed to locate byte store temporary directory: " + pathError.message());
            return std::nullopt;
        }
#endif
        const auto path = tempDirectory /
            ("datacodec_" + std::to_string(sessionId) + "_" +
             std::to_string(sequence) + "_" + safeLabel + ".cache");
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        if (!file) {
            validation::AssignError(error, "failed to create managed byte store");
            return std::nullopt;
        }
        return path;
    }

    template<typename TStore>
    bool RegisterStore(const std::shared_ptr<TStore>& store, std::string* error) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_released) {
            store->Release();
            validation::AssignError(error, "byte store session was released while creating a store");
            return false;
        }
        m_stores.push_back(store);
        return true;
    }

    void MoveFrom(ByteStoreSession&& other) {
        std::scoped_lock lock(m_mutex, other.m_mutex);
        m_stores = std::move(other.m_stores);
        m_residentBudget = std::move(other.m_residentBudget);
        m_residentLimitBytes = other.m_residentLimitBytes;
        m_sequence = other.m_sequence;
        m_sessionId = other.m_sessionId;
        m_diagnostics = std::move(other.m_diagnostics);
        m_released = other.m_released;
        other.m_sequence = 0u;
        other.m_sessionId = NextByteStoreSessionId();
        other.m_residentBudget = std::make_shared<resource::ResidentByteBudget>();
        other.m_residentLimitBytes = std::numeric_limits<std::uint64_t>::max();
        other.m_diagnostics.clear();
        other.m_released = true;
    }

    std::vector<std::shared_ptr<IByteSource>> m_stores;
    std::shared_ptr<resource::ResidentByteBudget> m_residentBudget{
        std::make_shared<resource::ResidentByteBudget>()};
    std::uint64_t m_residentLimitBytes{std::numeric_limits<std::uint64_t>::max()};
    std::vector<std::string> m_diagnostics;
    std::uint64_t m_sequence{0u};
    std::uint64_t m_sessionId{NextByteStoreSessionId()};
    bool m_released{false};
    mutable std::mutex m_mutex;
};

inline std::shared_ptr<IByteStore> CreateByteStore(
    ByteStoreSession& session,
    const std::string& label,
    const bool useMemoryStore,
    std::string* error = nullptr) {
    if (useMemoryStore) {
        auto store = session.CreateMemoryStore();
        if (store == nullptr && error != nullptr) {
            validation::AssignError(error, "failed to create memory byte store");
        }
        return store;
    }
    return session.CreateManagedByteStore(label, error);
}

inline std::shared_ptr<IAppendableByteStore> CreateAppendableManagedByteStore(
    ByteStoreSession& session,
    const std::string& label,
    std::string* error = nullptr) {
    return session.CreateAppendableManagedByteStore(label, error);
}

inline std::shared_ptr<IAppendableByteStore> CreateAppendableByteStore(
    ByteStoreSession& session,
    const std::string& label,
    const bool useMemoryStore,
    std::string* error = nullptr) {
    if (useMemoryStore) {
        auto store = session.CreateMemoryStore(true);
        if (store == nullptr && error != nullptr) {
            validation::AssignError(error, "failed to create appendable memory byte store");
        }
        return store;
    }
    return session.CreateAppendableManagedByteStore(label, error);
}

} // namespace bytestore
} // namespace datacodec

#endif
