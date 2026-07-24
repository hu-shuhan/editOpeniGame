#ifndef iGameDataCodecStreamingFrameProvider_h
#define iGameDataCodecStreamingFrameProvider_h

#include "DataCodec/Workflow/Session/PlaybackSession.h"
#include "iGameStreamingData.h"

#include <atomic>
#include <cstdint>
#include <limits>
#include <memory>
#include <vector>

IGAME_NAMESPACE_BEGIN

class DataCodecStreamingFrameProvider final : public IStreamingFrameProvider {
public:
    DataCodecStreamingFrameProvider(
            std::shared_ptr<::datacodec::PlaybackSession> session,
            std::vector<std::uint32_t> playbackFrameOrder);

    [[nodiscard]] std::vector<Object::Pointer> RequestFrame(unsigned int ordinal) override;
    void NotifyFramePresented(unsigned int ordinal) override;
    void ConfigureCacheCapacity(unsigned int bufferedFrameCount) override;
    void ClearCachedFrames() override;
    [[nodiscard]] std::size_t CachedFrameCount() const override;

private:
    std::shared_ptr<::datacodec::PlaybackSession> m_session;
    std::vector<std::uint32_t> m_playbackFrameOrder;
    std::atomic_uint m_currentOrdinal{std::numeric_limits<unsigned int>::max()};
};

IGAME_NAMESPACE_END

#endif
