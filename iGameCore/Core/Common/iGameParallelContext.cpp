#include "iGameParallelContext.h"

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <thread>

#ifdef IGAME_ENABLE_MPI
#include <mpi.h>
#endif

IGAME_NAMESPACE_BEGIN

bool ParallelContext::s_Initialized = false;

namespace {
// 0 = 关闭（默认）。见头文件 PVR_MPI_WAIT_TIMEOUT_SEC 的说明。
double g_WaitTimeoutSec = 0.0;
} // namespace

void ParallelContext::SetWaitTimeoutSeconds(double seconds) {
    g_WaitTimeoutSec = (seconds > 0.0) ? seconds : 0.0;
}

double ParallelContext::GetWaitTimeoutSeconds() { return g_WaitTimeoutSec; }

void ParallelContext::Initialize(int* argc, char*** argv) {
    if (s_Initialized) { return; }

    ParallelContext* ctx = Instance().GetPointer();
#ifdef IGAME_ENABLE_MPI
    int mpiInitialized = 0;
    MPI_Initialized(&mpiInitialized);
    if (!mpiInitialized) {
        MPI_Init(argc, argv);
    }
    MPI_Comm_rank(MPI_COMM_WORLD, &ctx->m_Rank);
    MPI_Comm_size(MPI_COMM_WORLD, &ctx->m_Size);
#else
    (void) argc;
    (void) argv;
    ctx->m_Rank = 0;
    ctx->m_Size = 1;
#endif
    // 可选的 MPI 等待超时（秒）：默认关闭。打开后，点对点 Isend/Irecv 的 Wait
    // 若永久等不到完成（慢 rank/网络卡住），会在超时后 MPI_Abort，而不是让 C/S
    // 服务端悄悄卡死、客户端永远等不到 metadata/帧。注意：集合通信（AllGather/
    // Gatherv/Barrier）不经过这里，仍由 MPI 本身决定；本开关主要覆盖 binary-swap
    // 的 ⌈log₂P⌉ 轮 P2P。超算上若怀疑有慢 rank，设成 120 左右即可。
    const char* timeoutEnv = std::getenv("PVR_MPI_WAIT_TIMEOUT_SEC");
    SetWaitTimeoutSeconds(timeoutEnv ? std::atof(timeoutEnv) : 0.0);
#ifdef IGAME_ENABLE_MPI
    if (ctx->m_Rank == 0 && g_WaitTimeoutSec > 0.0) {
        std::cerr << "[mpi] PVR_MPI_WAIT_TIMEOUT_SEC = " << g_WaitTimeoutSec
                  << "s (MPI_Test poll interval 1ms)\n";
    }
#endif
    s_Initialized = true;
}

void ParallelContext::Finalize() {
#ifdef IGAME_ENABLE_MPI
    int mpiInitialized = 0;
    int mpiFinalized = 0;
    MPI_Initialized(&mpiInitialized);
    MPI_Finalized(&mpiFinalized);
    if (mpiInitialized && !mpiFinalized) {
        MPI_Finalize();
    }
#endif
    s_Initialized = false;
}

void ParallelContext::Broadcast(double* buf, int count, int root) const {
    if (!buf || count <= 0) { return; }
#ifdef IGAME_ENABLE_MPI
    MPI_Bcast(buf, count, MPI_DOUBLE, root, MPI_COMM_WORLD);
#else
    (void) buf;
    (void) root;
#endif
}

void ParallelContext::Broadcast(int* buf, int count, int root) const {
    if (!buf || count <= 0) { return; }
#ifdef IGAME_ENABLE_MPI
    MPI_Bcast(buf, count, MPI_INT, root, MPI_COMM_WORLD);
#else
    (void) buf;
    (void) root;
#endif
}

void ParallelContext::Broadcast(char* buf, int count, int root) const {
    if (!buf || count <= 0) { return; }
#ifdef IGAME_ENABLE_MPI
    MPI_Bcast(buf, count, MPI_CHAR, root, MPI_COMM_WORLD);
#else
    (void) buf;
    (void) root;
#endif
}

void ParallelContext::AllReduce(const double* in, double* out, int count,
                                ReduceOp op) const {
    if (!in || !out || count <= 0) { return; }
#ifdef IGAME_ENABLE_MPI
    MPI_Op mpiOp = MPI_SUM;
    switch (op) {
        case ReduceOp::Min: mpiOp = MPI_MIN; break;
        case ReduceOp::Max: mpiOp = MPI_MAX; break;
        case ReduceOp::Sum: mpiOp = MPI_SUM; break;
    }
    MPI_Allreduce(in, out, count, MPI_DOUBLE, mpiOp, MPI_COMM_WORLD);
#else
    (void) op;
    if (in != out) {
        std::memcpy(out, in, static_cast<size_t>(count) * sizeof(double));
    }
#endif
}

void ParallelContext::AllGather(const double* in, double* out, int count) const {
    if (!in || !out || count <= 0) { return; }
#ifdef IGAME_ENABLE_MPI
    MPI_Allgather(in, count, MPI_DOUBLE, out, count, MPI_DOUBLE, MPI_COMM_WORLD);
#else
    std::memcpy(out, in, static_cast<size_t>(count) * sizeof(double));
#endif
}

void ParallelContext::AllGatherInt(const int* in, int* out, int count) const {
    if (!in || !out || count <= 0) { return; }
#ifdef IGAME_ENABLE_MPI
    MPI_Allgather(in, count, MPI_INT, out, count, MPI_INT, MPI_COMM_WORLD);
#else
    std::memcpy(out, in, static_cast<size_t>(count) * sizeof(int));
#endif
}

void ParallelContext::Gather(const char* send, char* recv, int count,
                             int root) const {
    if (count <= 0) { return; }
#ifdef IGAME_ENABLE_MPI
    MPI_Gather(send, count, MPI_CHAR, recv, count, MPI_CHAR, root,
               MPI_COMM_WORLD);
#else
    (void) root;
    if (send && recv && send != recv) {
        std::memcpy(recv, send, static_cast<size_t>(count));
    }
#endif
}

void ParallelContext::Gather(const float* send, float* recv, int count,
                             int root) const {
    if (count <= 0) { return; }
#ifdef IGAME_ENABLE_MPI
    MPI_Gather(send, count, MPI_FLOAT, recv, count, MPI_FLOAT, root,
               MPI_COMM_WORLD);
#else
    (void) root;
    if (send && recv && send != recv) {
        std::memcpy(recv, send, static_cast<size_t>(count) * sizeof(float));
    }
#endif
}

void ParallelContext::Gatherv(const char* send, int sendCount, char* recv,
                              const int* recvCounts, const int* displs,
                              int root) const {
    if (!recvCounts || !displs) { return; }
#ifdef IGAME_ENABLE_MPI
    MPI_Gatherv(const_cast<char*>(send), sendCount, MPI_CHAR, recv, recvCounts,
                displs, MPI_CHAR, root, MPI_COMM_WORLD);
#else
    (void) send;
    (void) sendCount;
    (void) root;
    // 单进程直通：只有 rank 0，recvCounts[0] 即总数，displs[0] 为 0。
    if (send && recv && send != recv && recvCounts[0] > 0) {
        std::memcpy(recv + displs[0], send,
                    static_cast<size_t>(recvCounts[0]));
    }
#endif
}

void ParallelContext::Barrier() const {
#ifdef IGAME_ENABLE_MPI
    MPI_Barrier(MPI_COMM_WORLD);
#endif
}

int ParallelContext::StoreRequest(void* handle) const {
    // 复用已 Wait 过的空槽：否则 m_Requests 会随帧数线性增长（每帧 2*log2(P) 项），
    // 长时间交互会话里是一份只增不减的泄漏；同时也让 requestId 保持在很小的范围。
    for (size_t i = 0; i < m_Requests.size(); ++i) {
        if (m_Requests[i] == nullptr) {
            m_Requests[i] = handle;
            return static_cast<int>(i);
        }
    }
    m_Requests.push_back(handle);
    return static_cast<int>(m_Requests.size()) - 1;
}

int ParallelContext::Isend(const char* buf, int count, int dest, int tag) const {
    if (!buf || count <= 0) { return -1; }
#ifdef IGAME_ENABLE_MPI
    MPI_Request* r = new MPI_Request;
    *r = MPI_REQUEST_NULL;
    const int rc = MPI_Isend(const_cast<char*>(buf), count, MPI_CHAR, dest, tag,
                             MPI_COMM_WORLD, r);
    if (rc != MPI_SUCCESS) {
        delete r;
        std::cerr << "[ParallelContext] MPI_Isend failed (rc=" << rc
                  << ", dest=" << dest << ", tag=" << tag
                  << ", count=" << count << ")\n";
        MPI_Abort(MPI_COMM_WORLD, rc);
        return -1;
    }
    return StoreRequest(r);
#else
    (void) buf; (void) count; (void) dest; (void) tag;
    return -1;
#endif
}

int ParallelContext::Irecv(char* buf, int count, int src, int tag) const {
    if (!buf || count <= 0) { return -1; }
#ifdef IGAME_ENABLE_MPI
    MPI_Request* r = new MPI_Request;
    *r = MPI_REQUEST_NULL;
    const int rc = MPI_Irecv(buf, count, MPI_CHAR, src, tag, MPI_COMM_WORLD, r);
    if (rc != MPI_SUCCESS) {
        delete r;
        std::cerr << "[ParallelContext] MPI_Irecv failed (rc=" << rc
                  << ", src=" << src << ", tag=" << tag
                  << ", count=" << count << ")\n";
        MPI_Abort(MPI_COMM_WORLD, rc);
        return -1;
    }
    return StoreRequest(r);
#else
    (void) buf; (void) count; (void) src; (void) tag;
    return -1;
#endif
}

void ParallelContext::Wait(int requestId) const {
#ifdef IGAME_ENABLE_MPI
    if (requestId < 0 || static_cast<size_t>(requestId) >= m_Requests.size()) {
        return;
    }
    MPI_Request* r = static_cast<MPI_Request*>(m_Requests[requestId]);
    if (!r) { return; }

    if (g_WaitTimeoutSec > 0.0) {
        // 可选超时：MPI_Test + 1ms 轮询（默认关闭，不影响正常路径的 MPI_Wait）。
        // 超时后 MPI_Abort：让「rank0 永久卡在 MPI、客户端永远等不到帧」变成
        // 一次明确的作业失败，而不是让持久服务假死。
        const auto deadline = std::chrono::steady_clock::now() +
                              std::chrono::duration<double>(g_WaitTimeoutSec);
        int completed = 0;
        for (;;) {
            const int rc = MPI_Test(r, &completed, MPI_STATUS_IGNORE);
            if (rc != MPI_SUCCESS) {
                std::cerr << "[ParallelContext] MPI_Test failed (rc=" << rc
                          << ", rank=" << m_Rank << ", req=" << requestId
                          << ")\n";
                MPI_Abort(MPI_COMM_WORLD, rc);
                return;
            }
            if (completed) { break; }
            if (std::chrono::steady_clock::now() >= deadline) {
                std::cerr << "[ParallelContext] MPI wait timeout after "
                          << g_WaitTimeoutSec << "s (rank=" << m_Rank
                          << ", req=" << requestId
                          << "); aborting the job instead of hanging.\n";
                MPI_Abort(MPI_COMM_WORLD, 3);
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    } else {
        const int rc = MPI_Wait(r, MPI_STATUS_IGNORE);
        if (rc != MPI_SUCCESS) {
            std::cerr << "[ParallelContext] MPI_Wait failed (rc=" << rc
                      << ", rank=" << m_Rank << ", req=" << requestId << ")\n";
            MPI_Abort(MPI_COMM_WORLD, rc);
            return;
        }
    }
    delete r;
    m_Requests[requestId] = nullptr;
#else
    (void) requestId;
#endif
}

int ParallelContext::WaitAny(const int* requestIds, int count) const {
#ifdef IGAME_ENABLE_MPI
    if (!requestIds || count <= 0) { return -1; }
    std::vector<MPI_Request> arr;
    std::vector<int> map;
    arr.reserve(static_cast<size_t>(count));
    map.reserve(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
        const int id = requestIds[i];
        if (id < 0 || static_cast<size_t>(id) >= m_Requests.size()) { continue; }
        MPI_Request* r = static_cast<MPI_Request*>(m_Requests[id]);
        if (r) {
            arr.push_back(*r);
            map.push_back(id);
        }
    }
    if (arr.empty()) { return -1; }
    int idx = -1;
    MPI_Waitany(static_cast<int>(arr.size()), arr.data(), &idx, MPI_STATUS_IGNORE);
    if (idx < 0) { return -1; }
    const int completedId = map[static_cast<size_t>(idx)];
    MPI_Request* r = static_cast<MPI_Request*>(m_Requests[completedId]);
    delete r;
    m_Requests[completedId] = nullptr;
    return completedId;
#else
    (void) requestIds; (void) count;
    return -1;
#endif
}

IGAME_NAMESPACE_END
