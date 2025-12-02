#ifndef MeshCodecZSTD_h
#define MeshCodecZSTD_h

#include "iGameMacro.h"
#include "zstd.h"
#include <vector>

IGAME_NAMESPACE_BEGIN

class MeshCodecZSTD {
public:
    explicit MeshCodecZSTD(int compressLevel = 3, int numThreads = 1)
        : m_CompressLevel(compressLevel), m_NumThreads(numThreads) {}

    bool Compress(std::vector<char>& dest, const std::vector<char>& source) const {
        if (source.empty()) {
            dest.clear();
            return false;
        }

        const size_t maxSize = ZSTD_compressBound(source.size());
        dest.resize(maxSize);

        ZSTD_CCtx* cctx = ZSTD_createCCtx();
        if (!cctx) {
            dest.clear();
            return false;
        }

        ZSTD_CCtx_setParameter(cctx, ZSTD_c_compressionLevel, m_CompressLevel);
        if (m_NumThreads > 1) {
            ZSTD_CCtx_setParameter(cctx, ZSTD_c_nbWorkers, m_NumThreads);
        }

        size_t compressedSize = ZSTD_compress2(
            cctx, dest.data(), maxSize, source.data(), source.size());
        ZSTD_freeCCtx(cctx);

        if (ZSTD_isError(compressedSize)) {
            dest.clear();
            return false;
        }

        dest.resize(compressedSize);
        return true;
    }

    bool Decompress(std::vector<char>& dest, const std::vector<char>& source) const
    {
        if (source.empty()) {
            dest.clear();
            return false;
        }

        const unsigned long long frameSize = ZSTD_getFrameContentSize(source.data(), source.size());
        if (frameSize == ZSTD_CONTENTSIZE_ERROR || frameSize == ZSTD_CONTENTSIZE_UNKNOWN) {
            dest.clear();
            return false;
        }

        dest.resize(static_cast<size_t>(frameSize));

        const size_t actualSize = ZSTD_decompress(
                dest.data(),
                dest.size(),
                source.data(),
                source.size());

        if (ZSTD_isError(actualSize)) {
            dest.clear();
            return false;
        }

        return true;
    }

    static const char* GetErrorName(size_t code) { return ZSTD_getErrorName(code); }
    static unsigned GetVersion() { return ZSTD_versionNumber(); }

private:
    int m_CompressLevel;
    int m_NumThreads;
};

IGAME_NAMESPACE_END

#endif
