#ifndef iGameIGVStats_h
#define iGameIGVStats_h

/**
 * @file  iGameIGVStats.h
 * @brief Counters for HDF5 calls made by IGV, used to enforce the metadata fast-path contract.
 *
 * The fast path must not read mesh data. Encoding that rule as counters plus test
 * assertions prevents a later change from quietly reading Points and decompressing chunks.
 *
 * This header does not include hdf5.h, so FileIO and test programs can use the counters
 * without an HDF5 compile dependency.
 *
 * Counter semantics:
 *   - h5dreadCount: H5Dread calls. The fast path keeps this at 1 (the Version dataset);
 *     reading any data dataset decompresses chunks and violates the contract.
 *   - datasetOpenCount: H5Dopen calls. The fast path may open datasets to read dataspace
 *     and dtype; object headers do not decompress chunks.
 *   - h5areadCount: H5Aread calls. Attributes are never compressed.
 */

#include <atomic>
#include <cstdint>

IGAME_NAMESPACE_BEGIN

namespace igv {

struct IGVStats {
    static std::atomic<uint64_t> h5dreadCount;      ///< H5Dread 调用次数
    static std::atomic<uint64_t> datasetOpenCount;  ///< H5Dopen 调用次数
    static std::atomic<uint64_t> h5areadCount;      ///< H5Aread 调用次数

    static void Reset() {
        h5dreadCount.store(0, std::memory_order_relaxed);
        datasetOpenCount.store(0, std::memory_order_relaxed);
        h5areadCount.store(0, std::memory_order_relaxed);
    }
};

inline std::atomic<uint64_t> IGVStats::h5dreadCount{0};
inline std::atomic<uint64_t> IGVStats::datasetOpenCount{0};
inline std::atomic<uint64_t> IGVStats::h5areadCount{0};

} // namespace igv

IGAME_NAMESPACE_END

#endif // iGameIGVStats_h
