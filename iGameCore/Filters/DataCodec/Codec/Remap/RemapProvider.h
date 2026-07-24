#ifndef DATACODEC_CODEC_REMAP_REMAPPROVIDER_H
#define DATACODEC_CODEC_REMAP_REMAPPROVIDER_H

#include "DataCodec/Common/DataCodecTypes.h"
#include "DataCodec/Storage/ByteStore/ByteStore.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <limits>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
namespace datacodec {

class IRemapProvider {
public:
    virtual ~IRemapProvider() = default;
    [[nodiscard]] virtual std::size_t Size() const noexcept = 0;
    [[nodiscard]] virtual bool IsIdentity() const noexcept = 0;
    [[nodiscard]] virtual std::uint64_t ResidentSizeHint() const noexcept = 0;
    virtual bool ReadAt(std::size_t index, IndexType& value, std::string* error) const = 0;
    virtual bool ReadRange(
        std::uint64_t offset,
        std::uint64_t count,
        std::vector<IndexType>& output,
        std::string* error) const = 0;
    virtual void Release() noexcept = 0;
};

class IWritableRemapProvider : public IRemapProvider {
public:
    virtual bool BeginWrite(std::string* error = nullptr) = 0;
    virtual bool BeginRandomWrite(std::string* error = nullptr) = 0;
    virtual bool AppendRange(std::span<const IndexType> order, std::string* error = nullptr) = 0;
    virtual bool WriteAt(std::size_t index, IndexType value, std::string* error = nullptr) = 0;
    virtual bool EndWrite(std::string* error = nullptr) = 0;
    virtual bool EndRandomWrite(std::string* error = nullptr) = 0;
};

using WritableRemapProviderFactory = std::function<std::shared_ptr<IWritableRemapProvider>(
    std::size_t size,
    bool randomWrite,
    std::string_view label,
    std::string* error)>;

class IdentityRemapProvider final : public IRemapProvider {
public:
    explicit IdentityRemapProvider(const std::size_t size)
        : m_size(size) {}

    [[nodiscard]] std::size_t Size() const noexcept override { return m_size; }
    [[nodiscard]] bool IsIdentity() const noexcept override { return true; }
    [[nodiscard]] std::uint64_t ResidentSizeHint() const noexcept override { return 0u; }

    bool ReadAt(const std::size_t index, IndexType& value, std::string* error) const override {
        if (index >= m_size) {
            return validation::AssignError(error, "remap read is out of range");
        }
        value = static_cast<IndexType>(index);
        return true;
    }

    bool ReadRange(
        const std::uint64_t offset,
        const std::uint64_t count,
        std::vector<IndexType>& output,
        std::string* error) const override {
        output.clear();
        if (offset > m_size || count > m_size - offset) {
            return validation::AssignError(error, "remap range is out of bounds");
        }
        output.resize(static_cast<std::size_t>(count));
        for (std::size_t local = 0; local < output.size(); ++local) {
            output[local] = static_cast<IndexType>(offset + local);
        }
        return true;
    }

    void Release() noexcept override { m_size = 0; }

private:
    std::size_t m_size{0};
};

class VectorRemapProvider final : public IRemapProvider {
public:
    explicit VectorRemapProvider(std::vector<IndexType> order)
        : m_order(std::move(order)) {}

    [[nodiscard]] std::size_t Size() const noexcept override { return m_order.size(); }
    [[nodiscard]] bool IsIdentity() const noexcept override { return m_order.empty(); }
    [[nodiscard]] std::uint64_t ResidentSizeHint() const noexcept override {
        return VectorCapacityBytes(m_order);
    }

    bool ReadAt(const std::size_t index, IndexType& value, std::string* error) const override {
        if (m_order.empty()) {
            value = static_cast<IndexType>(index);
            return true;
        }
        if (index >= m_order.size()) {
            return validation::AssignError(error, "remap read is out of range");
        }
        value = m_order[index];
        return true;
    }

    bool ReadRange(
        const std::uint64_t offset,
        const std::uint64_t count,
        std::vector<IndexType>& output,
        std::string* error) const override {
        output.clear();
        if (m_order.empty()) {
            output.resize(static_cast<std::size_t>(count));
            for (std::size_t local = 0; local < output.size(); ++local) {
                output[local] = static_cast<IndexType>(offset + local);
            }
            return true;
        }
        if (offset > m_order.size() || count > m_order.size() - offset) {
            return validation::AssignError(error, "remap range is out of bounds");
        }
        const auto begin = m_order.begin() + static_cast<std::ptrdiff_t>(offset);
        const auto end = begin + static_cast<std::ptrdiff_t>(count);
        output.assign(begin, end);
        return true;
    }

    void Release() noexcept override { ReleaseVectorStorage(m_order); }

private:
    std::vector<IndexType> m_order;
};

class ByteStoreRemapProvider : public IWritableRemapProvider {
public:
    using ByteStoreFactory = std::function<std::shared_ptr<bytestore::IRandomAccessByteStore>(std::string*)>;

    explicit ByteStoreRemapProvider(
        const std::size_t size,
        ByteStoreFactory sourceFactory)
        : m_sourceFactory(std::move(sourceFactory)),
          m_size(size) {}

    ~ByteStoreRemapProvider() override { Release(); }

    bool InitializeFromVector(const std::vector<IndexType>& order, std::string* error) {
        if (order.size() != m_size) {
            return validation::AssignError(error, "remap provider size mismatch");
        }

        ResetStorage(error);
        return AppendRawValues(std::span<const IndexType>(order.data(), order.size()), error);
    }

    bool BeginWrite(std::string* error = nullptr) override {
        ResetStorage(error);
        if (m_source == nullptr) {
            return validation::AssignError(error, "failed to create remap provider storage");
        }
        m_writtenSize = 0u;
        m_writing = true;
        return true;
    }

    bool BeginRandomWrite(std::string* error = nullptr) override {
        ResetStorage(error);
        if (m_source == nullptr) {
            return validation::AssignError(error, "failed to create remap provider storage");
        }
        const auto byteCount = RemapByteCount(error);
        if (byteCount == kInvalidByteCount ||
            !m_source->ResizeBytes(byteCount, error)) {
            return false;
        }
        m_randomWriting = true;
        return true;
    }

    bool AppendRange(std::span<const IndexType> order, std::string* error = nullptr) override {
        if (!m_writing || m_source == nullptr) {
            return validation::AssignError(error, "remap provider writer is not open");
        }
        if (order.size() > m_size || m_writtenSize > m_size - order.size()) {
            return validation::AssignError(error, "remap provider write exceeds expected size");
        }
        if (!AppendRawValues(order, error)) {
            return false;
        }
        m_writtenSize += order.size();
        return true;
    }

    bool WriteAt(
        const std::size_t index,
        const IndexType value,
        std::string* error = nullptr) override {
        if (!m_randomWriting || m_source == nullptr) {
            return validation::AssignError(error, "remap provider random writer is not open");
        }
        if (index >= m_size) {
            return validation::AssignError(error, "remap provider random write is out of range");
        }
        const auto byteOffset = static_cast<std::uint64_t>(index) * sizeof(IndexType);
        const auto* bytes = reinterpret_cast<const std::uint8_t*>(&value);
        return m_source->WriteBytesAt(
            byteOffset,
            std::span<const std::uint8_t>(bytes, sizeof(IndexType)),
            error);
    }

    bool EndRandomWrite(std::string* error = nullptr) override {
        if (!m_randomWriting) {
            return validation::AssignError(error, "remap provider random writer is not open");
        }
        m_randomWriting = false;
        return true;
    }

    bool EndWrite(std::string* error = nullptr) override {
        if (!m_writing) {
            return validation::AssignError(error, "remap provider writer is not open");
        }
        if (m_writtenSize != m_size) {
            return validation::AssignError(error, "remap provider written size mismatch");
        }
        m_writtenSize = 0u;
        m_writing = false;
        return true;
    }

    [[nodiscard]] std::size_t Size() const noexcept override { return m_size; }
    [[nodiscard]] bool IsIdentity() const noexcept override { return false; }
    [[nodiscard]] std::uint64_t ResidentSizeHint() const noexcept override {
        return m_source != nullptr ? m_source->ResidentSizeHint() : 0u;
    }

    bool ReadAt(const std::size_t index, IndexType& value, std::string* error) const override {
        if (index >= m_size) {
            return validation::AssignError(error, "remap read is out of range");
        }
        if (m_source == nullptr) {
            return validation::AssignError(error, "remap provider source is missing");
        }
        auto* bytes = reinterpret_cast<std::uint8_t*>(&value);
        return m_source->Read(
            static_cast<std::uint64_t>(index) * sizeof(IndexType),
            std::span<std::uint8_t>(bytes, sizeof(IndexType)),
            error);
    }

    bool ReadRange(
        const std::uint64_t offset,
        const std::uint64_t count,
        std::vector<IndexType>& output,
        std::string* error) const override {
        output.clear();
        if (offset > m_size || count > m_size - offset) {
            return validation::AssignError(error, "remap range is out of bounds");
        }
        output.resize(static_cast<std::size_t>(count));
        if (count == 0u) {
            return true;
        }

        if (m_source == nullptr) {
            return validation::AssignError(error, "remap provider source is missing");
        }
        if (!validation::CanMulU64(offset, sizeof(IndexType)) ||
            !validation::CanMulU64(count, sizeof(IndexType))) {
            validation::AssignError(error, "remap provider range exceeds addressable byte size");
            return false;
        }
        const auto byteOffset = offset * sizeof(IndexType);
        const auto byteCount = count * sizeof(IndexType);
        std::size_t localByteCount = 0u;
        if (!validation::CheckedCastSizeT(byteCount, localByteCount, "remap provider range", error)) {
            return false;
        }
        return m_source->Read(
            byteOffset,
            std::span<std::uint8_t>(
                reinterpret_cast<std::uint8_t*>(output.data()),
                localByteCount),
            error);
    }

    void Release() noexcept override {
        if (m_source != nullptr) {
            m_source->Release();
            m_source.reset();
        }
        m_size = 0;
        m_writtenSize = 0u;
        m_writing = false;
        m_randomWriting = false;
    }

private:
    static constexpr std::uint64_t kInvalidByteCount = std::numeric_limits<std::uint64_t>::max();

    std::shared_ptr<bytestore::IRandomAccessByteStore> CreateSource(std::string* error) {
        if (!m_sourceFactory) {
            validation::AssignError(error, "remap provider source factory is missing");
            return nullptr;
        }
        auto source = m_sourceFactory(error);
        if (source == nullptr && error != nullptr && error->empty()) {
            validation::AssignError(error, "remap provider source factory returned null");
        }
        return source;
    }

    void ResetStorage(std::string* error) {
        if (m_source != nullptr) {
            m_source->Release();
        }
        m_source = CreateSource(error);
        m_writtenSize = 0u;
        m_writing = false;
        m_randomWriting = false;
    }

    [[nodiscard]] std::uint64_t RemapByteCount(std::string* error) const {
        if (!validation::CanMulU64(m_size, sizeof(IndexType))) {
            validation::AssignError(error, "remap provider exceeds addressable byte size");
            return kInvalidByteCount;
        }
        return static_cast<std::uint64_t>(m_size) * sizeof(IndexType);
    }

    bool AppendRawValues(std::span<const IndexType> values, std::string* error) {
        if (m_source == nullptr) {
            return validation::AssignError(error, "remap provider source is missing");
        }
        if (!validation::CanMulU64(static_cast<std::uint64_t>(values.size()), sizeof(IndexType))) {
            validation::AssignError(error, "remap provider append exceeds addressable byte size");
            return false;
        }
        const auto byteCount = static_cast<std::uint64_t>(values.size()) * sizeof(IndexType);
        std::size_t localByteCount = 0u;
        if (!validation::CheckedCastSizeT(byteCount, localByteCount, "remap provider append", error)) {
            return false;
        }
        return m_source->AppendBytes(
            std::span<const std::uint8_t>(
                reinterpret_cast<const std::uint8_t*>(values.data()),
                localByteCount),
            error);
    }

    ByteStoreFactory m_sourceFactory;
    std::shared_ptr<bytestore::IRandomAccessByteStore> m_source;
    std::size_t m_writtenSize{0};
    std::size_t m_size{0};
    bool m_writing{false};
    bool m_randomWriting{false};
};

class RemapStoreProvider final : public ByteStoreRemapProvider {
public:
    RemapStoreProvider(
        const std::size_t size,
        bytestore::ByteStoreSession& session,
        std::string label = "remap",
        const bool useMemoryStore = false)
        : ByteStoreRemapProvider(
              size,
              [&session, label = std::move(label), useMemoryStore](std::string* error)
                  -> std::shared_ptr<bytestore::IRandomAccessByteStore> {
                  auto store = bytestore::CreateByteStore(session, label, useMemoryStore, error);
                  if (store == nullptr) {
                      return std::shared_ptr<bytestore::IRandomAccessByteStore>{};
                  }
                  return store;
              }) {}
};

inline std::shared_ptr<IWritableRemapProvider> MakeStoreBackedWritableRemapProvider(
    const std::size_t size,
    bytestore::ByteStoreSession& session,
    const bool randomWrite,
    const std::string& label,
    const bool useMemoryStore = false,
    std::string* error = nullptr) {
    auto provider = std::make_shared<RemapStoreProvider>(size, session, label, useMemoryStore);
    const auto initialized = randomWrite ? provider->BeginRandomWrite(error) : provider->BeginWrite(error);
    if (!initialized) {
        provider->Release();
        return nullptr;
    }
    return provider;
}

inline WritableRemapProviderFactory MakeStoreBackedWritableRemapProviderFactory(
    bytestore::ByteStoreSession& session,
    std::string prefix,
    const bool useMemoryStore = false) {
    return [&session, prefix = std::move(prefix), useMemoryStore](
        const std::size_t size,
        const bool randomWrite,
        const std::string_view label,
        std::string* error) {
        std::string storeLabel = prefix;
        if (!storeLabel.empty() && !label.empty()) {
            storeLabel.push_back('_');
        }
        storeLabel.append(label.data(), label.size());
        return MakeStoreBackedWritableRemapProvider(
            size,
            session,
            randomWrite,
            storeLabel.empty() ? std::string("remap") : storeLabel,
            useMemoryStore,
            error);
    };
}

inline std::shared_ptr<IRemapProvider> MakeIdentityRemapProvider(const std::size_t size) {
    return std::make_shared<IdentityRemapProvider>(size);
}

inline std::shared_ptr<IRemapProvider> MakeVectorRemapProvider(std::vector<IndexType> order) {
    return std::make_shared<VectorRemapProvider>(std::move(order));
}

inline bool ReadRemapValue(
    const IRemapProvider* provider,
    const std::size_t index,
    IndexType& value,
    std::string* error = nullptr) {
    if (provider == nullptr || provider->IsIdentity()) {
        value = static_cast<IndexType>(index);
        return true;
    }
    return provider->ReadAt(index, value, error);
}

inline std::size_t RemapSizeOrZero(const IRemapProvider* provider) noexcept {
    return provider == nullptr || provider->IsIdentity() ? 0u : provider->Size();
}

inline std::uint64_t RemapResidentBytes(const std::shared_ptr<IRemapProvider>& provider) noexcept {
    return provider == nullptr ? 0u : provider->ResidentSizeHint();
}

} // namespace datacodec

#endif
