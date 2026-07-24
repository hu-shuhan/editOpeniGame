#ifndef DATACODEC_STORAGE_BYTEIO_WINDOW_WINDOWRUNTIMEPARAMS_H
#define DATACODEC_STORAGE_BYTEIO_WINDOW_WINDOWRUNTIMEPARAMS_H

#include <cstddef>
#include <cstdint>
namespace datacodec {

// 1 MiB 对应的字节数
inline constexpr std::size_t kBytesPerMiB = 1024u * 1024u;
// encode 侧 window 字段视图的默认访问窗口大小
inline constexpr std::size_t kDefaultEncodeAccessWindowBytes = 64u * kBytesPerMiB;
// encode 侧允许同时触达的默认窗口总量
inline constexpr std::uint64_t kDefaultEncodeActiveWindowBytes = 256u * kBytesPerMiB;
// decode 侧读取 zstd 输入的内部窗口大小
inline constexpr std::size_t kDecodeInputWindowBytes = 16u * kBytesPerMiB;
// decode 侧吐出原始数据的内部窗口大小
inline constexpr std::size_t kDecodeOutputWindowBytes = 16u * kBytesPerMiB;
// decode 侧访问 decoded cache 的默认窗口大小
inline constexpr std::size_t kDefaultDecodeAccessWindowBytes = kDecodeOutputWindowBytes;
// decode 侧允许同时触达的默认窗口总量
inline constexpr std::uint64_t kDefaultDecodeActiveWindowBytes = 256u * kBytesPerMiB;

} // namespace datacodec

#endif
