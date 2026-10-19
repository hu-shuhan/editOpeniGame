#ifndef iGameIGVStats_h
#define iGameIGVStats_h

/**
 * @file  iGameIGVStats.h
 * @brief IGV 的 HDF5 调用计数器（P3 的核心验收手段）。
 *
 * 为什么需要它（`kia/igv分阶段实现方案.md` §11.3）：
 *   "元数据快路径不解压任何 dataset" 如果只是口头约定，第一次有人为了省事在快路径里
 *   读一下 `Points` 就会悄悄破功。把约束编码成**可执行断言**——调用次数计数 + 测试期断言
 *   ——才能长期守住。
 *
 * 本头文件**不含 `<hdf5.h>`**：它只被 IGV 的 `.cpp` 与测试 include，任何对外头文件都不该
 * 引入 HDF5 类型（§4.2）。抽成独立文件而不是塞在 `iGameIGVHdf5Util.h` 里，是为了让
 * "只想看计数"的调用方（如 `FileIO`、验收程序）不必依赖 HDF5 的编译环境。
 *
 * 计数语义：
 *   - `h5dreadCount`      —— `H5Dread` 次数。**快路径必须为 0**（这才是"零解压"的物理含义：
 *                            dataset 数据一旦被读，gzip chunk 就必须解压）。
 *   - `datasetOpenCount`  —— `H5Dopen` 次数。快路径**不要求为 0**：取点数/单元数/分量数
 *                            必须看 dataset 的 dataspace，而 dataspace 在对象头里、不触发解压。
 *                            详见 §7.2 的 P3 修正说明。
 *   - `h5areadCount`      —— `H5Aread` 次数。attribute 从不压缩（§3.4），读它不产生解压成本。
 */

#include <atomic>
#include <cstdint>

IGAME_NAMESPACE_BEGIN

namespace igv {

/** §11.3 要求的可注入计数器。 */
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
