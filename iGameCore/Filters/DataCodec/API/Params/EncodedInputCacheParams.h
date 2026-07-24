#ifndef DATACODEC_API_PARAMS_ENCODEDINPUTCACHEPARAMS_H
#define DATACODEC_API_PARAMS_ENCODEDINPUTCACHEPARAMS_H

#include <cstddef>
#include <cstdint>

namespace datacodec {

// 编码输入LRU仅保存原始包字节，不保存解码后的几何、拓扑或属性
// Native 文件读取默认依赖 mio 和操作系统页缓存，因此默认关闭
// Wasm 或网络读取可以显式启用，避免再次取得已经读过的包字节
struct EncodedInputCachePolicy {
    bool enabled{false};
    // 0表示不限制条目数量
    std::size_t residentInputLimit{3u};
    // 0表示不设置驻留字节上限
    std::uint64_t residentLimitBytes{0u};
};

inline void AssertValidEncodedInputCachePolicy(const EncodedInputCachePolicy&) noexcept {}

} // namespace datacodec

#endif
