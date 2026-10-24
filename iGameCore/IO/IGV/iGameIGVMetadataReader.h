#ifndef iGameIGVMetadataReader_h
#define iGameIGVMetadataReader_h

/**
 * @file  iGameIGVMetadataReader.h
 * @brief IGV metadata fast path: bounds, array descriptors, and counts without loading data.
 *
 * The fast path is implemented independently of IGVReader and shares only low-level HDF5
 * helpers, so it cannot grow into a full mesh read.
 *
 * It reads attributes and dataset dataspace/dtype only. The Version dataset is the only
 * H5Dread performed by this class. The test suite asserts IGVStats::h5dreadCount == 1;
 * reading Points here makes that assertion fail immediately.
 *
 * Tests that call this class directly must reset IGVStats themselves.
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
