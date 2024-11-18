#ifndef MeshCodecLZMA_h
#define MeshCodecLZMA_h

#include "iGameMacro.h"

#include "LzmaLib.h"
#include "LzmaEnc.h"
#include "LzmaDec.h"

#include <vector>

IGAME_NAMESPACE_BEGIN
class MeshCodecLZMA {
public:
    static bool Compress(
        std::vector<char>& dest,
        std::vector<char>& source,
        int compressLevel,
        int numThreads)
    {
        if (source.empty())
        {
            dest.resize(0);
            return false;
        }

        // 顺序
        // props destLength data
        size_t propsSize = LZMA_PROPS_SIZE;
        size_t sourceSizeSize = sizeof(size_t);
        size_t sourceSize = source.size();
        size_t destSize = source.size() + source.size() / 3 + 128;
        dest.resize(destSize);

        unsigned char* destPointer = reinterpret_cast<unsigned char*>(dest.data());
        unsigned char* sourcePointer = reinterpret_cast<unsigned char*>(source.data());

        CLzmaEncProps props;
        LzmaEncProps_Init(&props);
        props.level = compressLevel;
        props.numThreads = numThreads;

        std::memcpy(destPointer, &props, propsSize);

        ISzAlloc sz = { &LzmaAlloc ,&LzmaFree };
        ICompressProgress progressCallback = { &OnProgress };

        // 管用手法 把prop写入lzma编码的开头
        // 有一个比较强迫症的想法是把这两个也写入paramSet 不过以后看情况再说
        SRes ret =
            LzmaEncode(
                destPointer + propsSize + sourceSizeSize, &destSize,
                sourcePointer, sourceSize,
                &props, destPointer, &propsSize, props.writeEndMark,
                &progressCallback, &sz, &sz
            );

            /*LzmaEncode(
                &dest[propsSize + sourceSizeSize], &destSize,
                source.data(), source.size(),
                &props, &dest[0], &propsSize, props.writeEndMark,
                &progressCallback, &sz, &sz
            );*/

        dest.resize(destSize + propsSize + sourceSizeSize);
        std::memcpy(&dest[propsSize], &sourceSize, sourceSizeSize);
        return ret == SZ_OK;
    }

    static bool Decompress(
        std::vector<char>& dest,
        std::vector<char>& source)
    {
        if (source.empty())
        {
            dest.resize(0);
            return false;
        }

        size_t propsSize = LZMA_PROPS_SIZE;
        size_t destSizeSize = sizeof(size_t);
        size_t destSize;
        std::memcpy(&destSize, &source[propsSize], destSizeSize);
        dest.resize(destSize);

        unsigned char* destPointer = reinterpret_cast<unsigned char*>(dest.data());
        unsigned char* sourcePointer = reinterpret_cast<unsigned char*>(source.data());

        size_t sourceSize = source.size() - propsSize - destSizeSize;

        SRes ret = LzmaUncompress(
            destPointer, &destSize,
            sourcePointer + propsSize + destSizeSize, &sourceSize,
            sourcePointer, propsSize
        );

        //LzmaUncompress(
        //    destPointer, &destSize,
        //    &source[propsSize + destSizeSize], &sourceSize,
        //    &source[0], propsSize
        //);

        return ret == SZ_OK;
    }
   
private:
    // lzma2的调用涉及到诸多的回调函数
    static void* LzmaAlloc(ISzAllocPtr, size_t size)
    {
        return new uint8_t[size];
    }
    
    static void LzmaFree(ISzAllocPtr, void* address)
    {
        if (!address) {
            return;
        }
        delete[] reinterpret_cast<uint8_t*>(address);
    }

    static SRes OnProgress(ICompressProgressPtr p, UInt64 inSize, UInt64 outSize)
    {
        // 更新进度条

        return SZ_OK;
    }
};

IGAME_NAMESPACE_END
#endif