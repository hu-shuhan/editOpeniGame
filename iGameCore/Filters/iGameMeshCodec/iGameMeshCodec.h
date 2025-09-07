/*
build by revoke
code name 'Spinning Jenny'
*/

#ifndef MeshLoomCodec_h
#define MeshLoomCodec_h
#include<iGameThreadPool.h>
#include "iGameMacro.h"
#include "iGameMeshCodecParamSet.h"
#include "iGameMeshOptModifiedIndexBufferCodec.h"
#include "iGameFilter.h"
#include <atomic>
#include <future>
#include <queue>
#include <vector>

IGAME_NAMESPACE_BEGIN
class MeshCodec : public Filter {
public:
    I_OBJECT(MeshCodec);

    // Filter 基类要求的空的 Execute 方法（子类实现具体逻辑）
    bool Execute() override { return true; }

protected:
    MeshCodec() = default;

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
};
IGAME_NAMESPACE_END
#endif