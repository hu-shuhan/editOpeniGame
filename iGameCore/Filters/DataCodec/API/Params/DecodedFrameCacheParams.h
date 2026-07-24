#ifndef DATACODEC_API_PARAMS_DECODEDFRAMECACHEPARAMS_H
#define DATACODEC_API_PARAMS_DECODEDFRAMECACHEPARAMS_H

#include <cstddef>
#include <cstdint>

namespace datacodec
{

// DataCodec默认完整帧LRU和播放预取参数
// 外部完整帧缓存实现可以使用自己的容量和淘汰策略
struct DecodedFrameCachePolicy {
    // false 时DataCodec默认完整帧LRU不参与读取
    // 调用方显式注入外部完整帧缓存时仍由该后端决定是否驻留
    bool enabled{true};
    // 完整解码帧LRU的条目上限 包含当前正在使用的帧
    // 0表示不限制条目数量
    std::size_t residentFrameLimit{3u};
    // 0表示默认完整帧LRU不设置驻留字节上限
    std::uint64_t residentLimitBytes{0u};
    // 该参数只控制播放预取候选 不参与LRU淘汰
    std::size_t prefetchFrameCount{1u};
};

inline void AssertValidDecodedFrameCachePolicy(const DecodedFrameCachePolicy&) noexcept {}

} // datacodec命名空间

#endif
