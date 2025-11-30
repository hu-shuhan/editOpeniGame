#ifndef MeshCodecZSTD_h
#define MeshCodecZSTD_h

#include "iGameMacro.h"
#include "zstd.h"
#include <vector>
#include <cstring>

IGAME_NAMESPACE_BEGIN

class MeshCodecZSTD {
public:
    static bool Compress(
        std::vector<char>& dest,
        std::vector<char>& source,
        int compressLevel,
        int numThreads)
    {
        if (source.empty()) {
            dest.resize(0);
            return false;
        }

        // 计算压缩后的最大可能大小
        size_t sourceSize = source.size();
        size_t maxCompressedSize = ZSTD_compressBound(sourceSize);

        if (ZSTD_isError(maxCompressedSize)) {
            dest.resize(0);
            return false;
        }

        // 预留空间：原始大小(8字节) + 压缩数据
        // 注意：zstd frame本身包含了解压大小，但为了与LZMA格式保持一致，也存储一份
        size_t headerSize = sizeof(size_t);
        dest.resize(headerSize + maxCompressedSize);

        // 写入原始大小到头部
        std::memcpy(dest.data(), &sourceSize, headerSize);

        // 创建压缩上下文以支持多线程
        ZSTD_CCtx* cctx = ZSTD_createCCtx();
        if (!cctx) {
            dest.resize(0);
            return false;
        }

        // 设置压缩级别
        size_t ret = ZSTD_CCtx_setParameter(cctx, ZSTD_c_compressionLevel, compressLevel);
        if (ZSTD_isError(ret)) {
            ZSTD_freeCCtx(cctx);
            dest.resize(0);
            return false;
        }

        // 设置线程数（如果 > 1 则启用多线程压缩）
        if (numThreads > 1) {
            ret = ZSTD_CCtx_setParameter(cctx, ZSTD_c_nbWorkers, numThreads);
            if (ZSTD_isError(ret)) {
                ZSTD_freeCCtx(cctx);
                dest.resize(0);
                return false;
            }
        }

        // 执行压缩
        size_t compressedSize = ZSTD_compress2(
            cctx,
            dest.data() + headerSize,
            maxCompressedSize,
            source.data(),
            sourceSize
        );

        ZSTD_freeCCtx(cctx);

        if (ZSTD_isError(compressedSize)) {
            dest.resize(0);
            return false;
        }

        // 调整到实际压缩大小
        dest.resize(headerSize + compressedSize);
        return true;
    }

    static bool Decompress(
        std::vector<char>& dest,
        std::vector<char>& source)
    {
        if (source.empty() || source.size() < 8) {
            dest.resize(0);
            return false;
        }

        // 自动检测头部大小
        // 优先尝试8字节头部（64位系统）
        size_t headerSize = 8;
        unsigned long long frameSize = ZSTD_getFrameContentSize(
            source.data() + headerSize,
            source.size() - headerSize
        );

        // 如果8字节偏移无效，尝试4字节头部（32位系统）
        if (frameSize == ZSTD_CONTENTSIZE_ERROR || frameSize == ZSTD_CONTENTSIZE_UNKNOWN) {
            headerSize = 4;
            if (source.size() < headerSize) {
                dest.resize(0);
                return false;
            }
            frameSize = ZSTD_getFrameContentSize(
                source.data() + headerSize,
                source.size() - headerSize
            );
        }

        if (frameSize == ZSTD_CONTENTSIZE_ERROR || frameSize == ZSTD_CONTENTSIZE_UNKNOWN) {
            dest.resize(0);
            return false;
        }

        size_t decompressedSize = static_cast<size_t>(frameSize);
        dest.resize(decompressedSize);

        size_t actualSize = ZSTD_decompress(
            dest.data(),
            decompressedSize,
            source.data() + headerSize,
            source.size() - headerSize
        );

        if (ZSTD_isError(actualSize)) {
            dest.resize(0);
            return false;
        }

        if (actualSize != decompressedSize) {
            dest.resize(0);
            return false;
        }

        return true;
    }

    // 获取错误信息（调试用）
    static const char* GetErrorName(size_t code) {
        return ZSTD_getErrorName(code);
    }

    // 获取版本信息
    static unsigned GetVersion() {
        return ZSTD_versionNumber();
    }
};

IGAME_NAMESPACE_END

#endif // MeshCodecZSTD_h
