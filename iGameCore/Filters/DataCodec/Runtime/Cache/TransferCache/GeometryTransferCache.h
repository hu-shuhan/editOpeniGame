#ifndef DATACODEC_RUNTIME_CACHE_TRANSFERCACHE_GEOMETRYTRANSFERCACHE_H
#define DATACODEC_RUNTIME_CACHE_TRANSFERCACHE_GEOMETRYTRANSFERCACHE_H

#include "DataCodec/Storage/ByteIO/ByteSource.h"
#include "DataCodec/API/Params/CodecStorageParams.h"

#include <memory>
#include <utility>
namespace datacodec {

class GeometryTransferCache final {
public:
    GeometryTransferCache() = default;
    GeometryTransferCache(
        GeometryStorageParams meta,
        std::shared_ptr<bytestore::IByteSource> payload)
        : m_meta(std::move(meta)),
          m_payload(std::move(payload)) {}

    GeometryTransferCache(const GeometryTransferCache&) = delete;
    GeometryTransferCache& operator=(const GeometryTransferCache&) = delete;

    GeometryTransferCache(GeometryTransferCache&& other) noexcept {
        MoveFrom(std::move(other));
    }

    GeometryTransferCache& operator=(GeometryTransferCache&& other) noexcept {
        if (this != &other) {
            Release();
            MoveFrom(std::move(other));
        }
        return *this;
    }

    ~GeometryTransferCache() { Release(); }

    [[nodiscard]] const GeometryStorageParams& Meta() const noexcept { return m_meta; }
    [[nodiscard]] std::shared_ptr<bytestore::IByteSource> Payload() const noexcept { return m_payload; }
    [[nodiscard]] std::shared_ptr<bytestore::IByteSource> TakePayload() noexcept { return std::move(m_payload); }

    void Release() noexcept {
        if (m_payload != nullptr) {
            m_payload->Release();
            m_payload.reset();
        }
    }

private:
    void MoveFrom(GeometryTransferCache&& other) noexcept {
        m_meta = std::move(other.m_meta);
        m_payload = std::move(other.m_payload);
        other.m_meta = {};
    }

    GeometryStorageParams m_meta;
    std::shared_ptr<bytestore::IByteSource> m_payload;
};

} // namespace datacodec

#endif
