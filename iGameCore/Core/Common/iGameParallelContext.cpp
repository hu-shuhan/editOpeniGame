#include "iGameParallelContext.h"

#include <cstring>

#ifdef IGAME_ENABLE_MPI
#include <mpi.h>
#endif

IGAME_NAMESPACE_BEGIN

bool ParallelContext::s_Initialized = false;

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

int ParallelContext::Isend(const char* buf, int count, int dest, int tag) const {
    if (!buf || count <= 0) { return -1; }
#ifdef IGAME_ENABLE_MPI
    MPI_Request* r = new MPI_Request;
    MPI_Isend(const_cast<char*>(buf), count, MPI_CHAR, dest, tag, MPI_COMM_WORLD,
              r);
    m_Requests.push_back(r);
    return static_cast<int>(m_Requests.size()) - 1;
#else
    (void) buf; (void) count; (void) dest; (void) tag;
    return -1;
#endif
}

int ParallelContext::Irecv(char* buf, int count, int src, int tag) const {
    if (!buf || count <= 0) { return -1; }
#ifdef IGAME_ENABLE_MPI
    MPI_Request* r = new MPI_Request;
    MPI_Irecv(buf, count, MPI_CHAR, src, tag, MPI_COMM_WORLD, r);
    m_Requests.push_back(r);
    return static_cast<int>(m_Requests.size()) - 1;
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
    if (r) {
        MPI_Wait(r, MPI_STATUS_IGNORE);
        delete r;
        m_Requests[requestId] = nullptr;
    }
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
