#ifndef iGameIGVMetadataReader_h
#define iGameIGVMetadataReader_h

/**
 * @file  iGameIGVMetadataReader.h
 * @brief IGV 的**元数据快路径**（P3，`kia/igv分阶段实现方案.md` §7 / §9 P3）。
 *
 * 存在意义：不加载 mesh、不解压任何 dataset，就拿到「包围盒 + 属性清单 + 计数」。
 * 典型用途：文件浏览器列表、色标初始化、打开前的相机 fit、C/S 端"只列清单不传数据"。
 *
 * ## 为什么它独立于 `IGVReader`（不共享流程编排）
 *
 * 若让「只读元数据」复用 `IGVReader` 的全量读代码，实现迟早会被「全量读」的需求污染，
 * 最终退化成「把 Reader 打开一遍、只是不用它的数据」——那样快路径就失去意义了。
 * 因此本类**独立实现**，只共享 `iGameIGVHdf5Util.h` 里的**底层小工具**
 * （attribute 读写、dtype 映射、RAII 句柄），流程编排完全独立。
 *
 * ## 与全量读的边界（可断言事实，见 §11.3）
 *
 * 快路径**只碰两类东西**：
 *   1. **attribute**（根组 attribute + 每个属性 dataset 上的 attribute）—— attribute 从不压缩，
 *      读取不产生任何解压成本（§3.4）；
 *   2. **dataset 的 dataspace / dtype**（为了拿 count / components / valueSize）——
 *      dataspace 与 dtype 都在对象头里，**不触发 chunk 解压**。
 *
 * **唯一的例外**是 `Version` 数据集（i64[2]，写侧 `compress=false`，永远不压缩）：
 * 版本号必须以 dataset 形式存储（§3.1 / `kDatasetVersion` 的说明），所以快路径会读它。
 * 除此之外**没有任何数据读取**；验收程序断言 `IGVStats::h5dreadCount == 1`，
 * 一旦有人为了省事在快路径里读了 `Points`，该断言立刻变红。
 *
 * `IGVStats` 的 `Reset()` 由 `FileIO::ReadIGVMetadata()` 入口负责（§11.3：入口清零、出口不清），
 * 直接调用本类的测试需要自行 `Reset()`。
 */

#include <cstddef>
#include <string>

#include "iGameIGVFormat.h"

IGAME_NAMESPACE_BEGIN

class IGVMetadataReader {
public:
    IGVMetadataReader() = delete;

    /**
     * 从文件读元数据。
     *
     * @param path   文件路径
     * @param out    成功时填充；失败时不保证内容
     * @param error  非空时写入失败原因（中文，可直接展示）
     * @return 成功 true；失败 false（错误已清栈，不透传 HDF5 错误栈，§5.4）
     */
    static bool ReadFromFile(const std::string& path, igv::IGVMetadata& out,
                             std::string* error = nullptr);

    /**
     * 从内存镜像读元数据（对应 `FileReader::SetMemoryBuffer` 的"调用方拥有缓冲"语义）。
     *
     * @param data  文件字节；调用方负责生命周期（本函数返回前不会超出）
     * @param size  字节数
     * @return 同上
     */
    static bool ReadFromMemory(const void* data, size_t size, igv::IGVMetadata& out,
                               std::string* error = nullptr);
};

IGAME_NAMESPACE_END

#endif // iGameIGVMetadataReader_h
