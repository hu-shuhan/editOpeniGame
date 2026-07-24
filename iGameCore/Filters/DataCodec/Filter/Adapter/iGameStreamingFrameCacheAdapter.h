#ifndef iGameStreamingFrameCacheAdapter_h
#define iGameStreamingFrameCacheAdapter_h

#include "DataCodec/API/Adapter/IDecodedFrameCache.h"
#include "iGameStreamingData.h"

#include <cstdint>
#include <mutex>
#include <unordered_map>

IGAME_NAMESPACE_BEGIN

class iGameStreamingFrameCacheAdapter final : public ::datacodec::IDecodedFrameCache {
public:
    iGameStreamingFrameCacheAdapter(
        StreamingData* streamingData,
        std::unordered_map<std::uint32_t, unsigned int> frameOrdinals,
        std::unordered_map<std::uint32_t, ::datacodec::DecodeSourceIdentity> frameIdentities);

    [[nodiscard]] ::datacodec::DecodedFrameCacheLookupResult Find(
        const ::datacodec::DecodedFrameKey& key,
        ::datacodec::DecodedFrameAccessKind accessKind) override;
    [[nodiscard]] ::datacodec::CacheStoreResult Store(
        const ::datacodec::DecodedFrameKey& key,
        ::datacodec::DecodedFrameLease::Pointer frame,
        ::datacodec::DecodedFrameAccessKind accessKind) override;
    void InvalidateSource(const ::datacodec::DecodeSourceIdentity& source) override;
    [[nodiscard]] std::vector<std::uint32_t> ResidentFrameIndices(
        const ::datacodec::DecodeSourceIdentity& source) const override;
    [[nodiscard]] ::datacodec::DecodedFrameCacheStats Statistics() const override;

private:
    [[nodiscard]] bool Matches(const ::datacodec::DecodedFrameKey& key) const;
    void RefreshResidentStatsLocked() const;

    StreamingData* m_streamingData{nullptr};
    std::unordered_map<std::uint32_t, unsigned int> m_frameOrdinals;
    std::unordered_map<unsigned int, std::uint32_t> m_ordinalFrames;
    std::unordered_map<std::uint32_t, ::datacodec::DecodeSourceIdentity> m_frameIdentities;
    mutable std::mutex m_statsMutex;
    mutable ::datacodec::DecodedFrameCacheStats m_stats;
};

IGAME_NAMESPACE_END

#endif
