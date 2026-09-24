#ifndef iGameParallelContext_h
#define iGameParallelContext_h

#include "iGameObject.h"

#include <vector>

IGAME_NAMESPACE_BEGIN

/**
 * @class ParallelContext
 * @brief 进程上下文 + 集合通信后端抽象（阶段 2）。
 *
 * @details
 *  上层代码（数据分发、相机/传输函数域同步、分布式合成）只依赖本类，不直接
 *  调用 MPI。默认（未定义 IGAME_ENABLE_MPI）退化为单进程直通：Rank()==0、Size()==1，
 *  集合通信均为拷贝/空操作。定义 IGAME_ENABLE_MPI 后编译 MPI 后端，由 mpiexec/srun
 *  拉起多进程时通过 MPI_Init 获取真实 rank/size。
 */
class ParallelContext : public Object {
public:
    I_OBJECT(ParallelContext);
    static Pointer Instance() {
        static Pointer ins = new ParallelContext;
        return ins;
    }

    enum class ReduceOp { Min, Max, Sum };

    /** 进程上下文初始化；MPI 后端等价 MPI_Init（幂等，仅首次生效）。 */
    static void Initialize(int* argc = nullptr, char*** argv = nullptr);
    /** 进程上下文清理；MPI 后端等价 MPI_Finalize。 */
    static void Finalize();
    static bool IsInitialized() { return s_Initialized; }

    /** 当前进程号（rank）。 */
    int Rank() const { return m_Rank; }
    /** 进程总数（size）。 */
    int Size() const { return m_Size; }
    /** 是否多进程并行（Size > 1）。 */
    bool IsParallel() const { return m_Size > 1; }

    /** 广播：root 的 buf 广播给所有 rank（单进程直通为空操作）。 */
    void Broadcast(double* buf, int count, int root = 0) const;
    void Broadcast(int* buf, int count, int root = 0) const;
    void Broadcast(char* buf, int count, int root = 0) const;

    /** 规约：各 rank 的 in 按 op 归约，结果写入 out（所有 rank 一致）。 */
    void AllReduce(const double* in, double* out, int count, ReduceOp op) const;

    /** 收集：把每个 rank 的 count 个 double 拼接到 out（out 长度 = Size*count）。 */
    void AllGather(const double* in, double* out, int count) const;

    /**
     * 收集（int 版）：把每个 rank 的 count 个 int 拼接到 out（out 长度 = Size*count）。
     * 用于并行合成时交换各 rank 的「有效像素 ROI 矩形」等小整数元数据（避免走 double
     * 中转再截断，语义更清晰）。
     */
    void AllGatherInt(const int* in, int* out, int count) const;

    /**
     * 收集（到 root）：把每个 rank 的 count 个字节拼接到 root 的 recv。
     * recv 仅 root 有效（长度 = Size*count），非 root 可传 nullptr。
     * 用于 iGameCompositePass 把各 rank 的 RGBA 图像汇聚到 rank 0（阶段 3）。
     */
    void Gather(const char* send, char* recv, int count, int root = 0) const;

    /**
     * 收集（到 root）：把每个 rank 的 count 个 float 拼接到 root 的 recv。
     * recv 仅 root 有效（长度 = Size*count），非 root 可传 nullptr。
     * 用于 iGameCompositePass 把各 rank 的深度图汇聚到 rank 0（阶段 3）。
     */
    void Gather(const float* send, float* recv, int count, int root = 0) const;

    /**
     * 变长收集（到 root，对标 MPI_Gatherv）：把每个 rank 的 recvCounts[r] 个字节按
     * displs[r] 的偏移拼进 root 的 recv。所有 rank 的 recvCounts/displs 必须一致
     * （通常由一次 AllGatherInt 得到）。
     *
     * 用于 iGameCompositePass 的「稀疏 ROI 合成」：每个 rank 只把它图上非空像素的
     * 外接矩形发给 rank 0（1024² 全图 4MB → 单块通常只有几百字节），避免 O(P) 张全图
     * 汇聚（对标 IceT 的 valid_pixels_viewport）。
     *
     * sendCount == 0 时 send 可为 nullptr（MPI 忽略该缓冲区）。
     */
    void Gatherv(const char* send, int sendCount, char* recv,
                 const int* recvCounts, const int* displs, int root = 0) const;

    /** 栅栏同步（单进程直通为空操作）。 */
    void Barrier() const;

    // ---- 非阻塞点对点（阶段 4：并行合成用）----
    // requestId 由本类自增分配；发送/接收缓冲区在对应 Wait/WaitAny 完成前必须保持有效。
    int Isend(const char* buf, int count, int dest, int tag) const;
    int Irecv(char* buf, int count, int src, int tag) const;
    void Wait(int requestId) const;
    /** 返回最先完成的 requestId；无可等待请求时返回 -1。 */
    int WaitAny(const int* requestIds, int count) const;

protected:
    ParallelContext() = default;
    ~ParallelContext() override = default;

private:
    int m_Rank{0};
    int m_Size{1};
    // 非阻塞请求存储（void* 指向 MPI 后端分配的请求句柄；单进程直通不使用）。
    mutable std::vector<void*> m_Requests;
    static bool s_Initialized;
};

IGAME_NAMESPACE_END

#endif // iGameParallelContext_h
