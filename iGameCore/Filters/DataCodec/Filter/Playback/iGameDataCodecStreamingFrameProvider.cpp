#include "DataCodec/Filter/Playback/iGameDataCodecStreamingFrameProvider.h"

#include "DataCodec/Filter/Output/iGameDataCodecOutputBinding.h"
#include "DataCodec/Filter/Adapter/iGameFramePackageDecodeAssembly.h"
#include "Log/iGameLogger.h"

#include <utility>

IGAME_NAMESPACE_BEGIN

DataCodecStreamingFrameProvider::DataCodecStreamingFrameProvider(
        std::shared_ptr<::datacodec::PlaybackSession> session,
        std::vector<std::uint32_t> playbackFrameOrder)
    : m_session(std::move(session)),
      m_playbackFrameOrder(std::move(playbackFrameOrder)) {}

std::vector<Object::Pointer> DataCodecStreamingFrameProvider::RequestFrame(
        const unsigned int ordinal) {
    if (m_session == nullptr || ordinal >= m_playbackFrameOrder.size()) { return {}; }
    const auto result = m_session->RequestFrame({
            .frameIndex = m_playbackFrameOrder[ordinal],
            .progressFrameOrdinal = ordinal,
            .progressFrameCount = static_cast<std::uint32_t>(m_playbackFrameOrder.size()),
            .runRecordSink = MakeiGameDataCodecOutputRecordSink({}, {}, true),
    });
    const auto output = DataObjectFromDecodedFrame(result.frame);
    if (result.success && output != nullptr) {
        return {output};
    }
    for (const auto& message : result.messages) {
        IGAME_CORE_ERROR(
                "DataCodec frame {} decode failed [{}]: {}",
                m_playbackFrameOrder[ordinal],
                message.origin,
                message.text);
    }
    if (result.messages.empty()) {
        IGAME_CORE_ERROR(
                "DataCodec frame {} decode failed without diagnostic messages",
                m_playbackFrameOrder[ordinal]);
    }
    return {};
}

void DataCodecStreamingFrameProvider::NotifyFramePresented(const unsigned int ordinal) {
    if (m_session == nullptr || ordinal >= m_playbackFrameOrder.size()) { return; }
    m_currentOrdinal.store(ordinal);
    m_session->NotifyFramePresented(m_playbackFrameOrder[ordinal]);
}

void DataCodecStreamingFrameProvider::ConfigureCacheCapacity(
        const unsigned int bufferedFrameCount) {
    // 动画控件的帧数仅描述iGame播放缓存，不覆盖DataCodec完整帧LRU参数
    // 完整帧LRU由解码请求中的DecodedFrameCachePolicy统一配置
    (void)bufferedFrameCount;
}

void DataCodecStreamingFrameProvider::ClearCachedFrames() {
    m_currentOrdinal.store(std::numeric_limits<unsigned int>::max());
    // iGame动画缓存的清理不失效DataCodec跨读取完整帧LRU
    // 显式调用PlaybackSession::ClearDecodedFrameCache时才按数据源清理完整帧
}

std::size_t DataCodecStreamingFrameProvider::CachedFrameCount() const {
    if (m_session == nullptr) { return 0u; }
    const auto cachedFrames = m_session->CachedDecodedFrameIndices();
    const auto currentOrdinal = m_currentOrdinal.load();
    if (currentOrdinal >= m_playbackFrameOrder.size()) { return cachedFrames.size(); }
    const auto currentFrameIndex = m_playbackFrameOrder[currentOrdinal];
    return cachedFrames.size() - static_cast<std::size_t>(
        std::find(cachedFrames.begin(), cachedFrames.end(), currentFrameIndex) != cachedFrames.end());
}

IGAME_NAMESPACE_END
