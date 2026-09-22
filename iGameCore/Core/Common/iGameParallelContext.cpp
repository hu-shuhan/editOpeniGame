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

void ParallelContext::Barrier() const {
#ifdef IGAME_ENABLE_MPI
    MPI_Barrier(MPI_COMM_WORLD);
#endif
}

IGAME_NAMESPACE_END
