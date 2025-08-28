/*
build by revoke
code name 'Spinning Jenny'
*/

#ifndef MeshLoomCodec_h
#define MeshLoomCodec_h
#include<iGameThreadPool.h>
#include "iGameMacro.h"
#include "iGameDataObject.h"
#include "iGameMeshCodecParamSet.h"
#include "iGameMeshOptModifiedIndexBufferCodec.h"
#include "iGamePointSet.h"
#include "iGameSurfaceMesh.h"
#include "iGameVolumeMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameStructuredMesh.h"
#include "iGameFilter.h"
#include <atomic>
#include <condition_variable>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include "iGameThreadPool.h"
#include <vector>

IGAME_NAMESPACE_BEGIN
class MeshLoomCodec : public Filter {
public:
    I_OBJECT(MeshLoomCodec);

    // Filter 基类要求的空的 Execute 方法（子类实现具体逻辑）
    bool Execute() override { return true; }

protected:
    MeshLoomCodec() = default;

    using IndexBufferCodec = MeshOptModifiedIndexBufferCodec;
    CodecParameters m_codecParams;

    // void UpdateProgress(double p) {
    //     if (m_Progress) {
    //         m_Progress->UpdateProgress(p);
    //     }
    //     else {
    //         m_Progress = ProgressObserver::Instance();
    //     }
    // }

    template<typename Func>
    void ProgressParallelFor(int start, int end, float startProgress, float endProgress, Func&& process, int numThreads = ThreadPool::GetDefaultThreadCount()) {
        int range = end - start;
        int chunkSize = range / numThreads;
        if (range < numThreads) {
            numThreads = range;
            chunkSize = 1;
        }
        // std::cout << "The number of threads uesd  is " << numThreads << '\n';
        std::vector<std::future<void>> futures;
        for (int i = 0; i < numThreads; ++i) {
            int chunkStart = start + i * chunkSize;
            int chunkEnd = (i == numThreads - 1) ? end : chunkStart + chunkSize;
            if (chunkStart == chunkEnd) continue;
            // 使用线程池提交任务
            //std::cout << chunkStart << " " << chunkEnd << " " << i << std::endl;
            futures.emplace_back(ThreadPool::Instance()->Commit([=]() { process(chunkStart, chunkEnd); }));
        }
        // 等待所有任务完成
        for (size_t i = 0; i < futures.size(); ++i) {
            futures[i].get();
            float progress = startProgress +
                (static_cast<float>(i + 1) / futures.size()) * (endProgress - startProgress);
            UpdateProgress(progress);
        }
    }

    // 写入相关 代码改造自 tmc
    enum class PayloadType
    {
        kParameterSet = 0,
        kGeometryBrick = 1,
        kAttributeBrick = 2,
        kTopologyBrick = 3,
        kCompressedBrick = 4,
    };

    struct PayloadBuffer : public std::vector<char> {
        PayloadType type;

        PayloadBuffer() = default;

        PayloadBuffer(PayloadType payload_type) : type(payload_type)
        {
            reserve(4096);
        }
    };

    std::ostream&
        WriteBuf(const PayloadBuffer& buf, std::ostream& os)
    {
        uint32_t length = uint32_t(buf.size());

        os.put(char(buf.type));
        os.put(char(length >> 24));
        os.put(char(length >> 16));
        os.put(char(length >> 8));
        os.put(char(length >> 0));

        os.write(buf.data(), length);
        return os;
    }

    std::istream*
        ReadBuf(std::istream* is, PayloadBuffer* buf)
    {
        buf->resize(0);
        buf->type = PayloadType(static_cast<unsigned>(is->get()));

        uint32_t length = 0;
        length = (length << 8) | static_cast<unsigned>(is->get());
        length = (length << 8) | static_cast<unsigned>(is->get());
        length = (length << 8) | static_cast<unsigned>(is->get());
        length = (length << 8) | static_cast<unsigned>(is->get());

        if (!(*is))
            return is;

        buf->resize(length);
        is->read(buf->data(), length);
        return is;
    }

    // bool ReadBuf(MeshCodecDataObject::Pointer codecData, PayloadBuffer* buf)
    // {
    //     // 直接使用MeshCodecDataObject的成员变量，就像STL的left引用
    //     char*& currentPos = codecData->m_CurrentPos;
    //     char* fileEnd = codecData->m_FileEnd;
    //
    //     // 原版：buf->resize(0);
    //     buf->resize(0);
    //
    //     // 原版：buf->type = PayloadType(static_cast<unsigned>(is.get()));
    //     buf->type = PayloadType(static_cast<unsigned>(*currentPos++));
    //
    //     // 原版：4次 is.get() 读取 length
    //     uint32_t length = 0;
    //     length = (length << 8) | static_cast<unsigned>(*currentPos++);
    //     length = (length << 8) | static_cast<unsigned>(*currentPos++);
    //     length = (length << 8) | static_cast<unsigned>(*currentPos++);
    //     length = (length << 8) | static_cast<unsigned>(*currentPos++);
    //
    //     // 原版：if (!is) return is;
    //     if (currentPos + length > fileEnd) {
    //         return false;
    //     }
    //
    //     // 原版：buf->resize(length);
    //     buf->resize(length);
    //
    //     // 原版：is.read(buf->data(), length);
    //     std::memcpy(buf->data(), currentPos, length);
    //     currentPos += length;
    //
    //     return true;
    // }
};
IGAME_NAMESPACE_END
#endif