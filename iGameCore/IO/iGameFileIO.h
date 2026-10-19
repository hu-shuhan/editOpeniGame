#ifndef iGameFileIO_h
#define iGameFileIO_h

#include "MeshCodec/Utils/iGameMeshCodecParams.h"
#include "iGameDataObject.h"
#include "iGameObject.h"
#include <cstddef>
#include <string>

#if defined(IGV_ENABLE)
// 元数据快路径的返回结构（P3）。只在 IGV 模块开启时暴露，避免 OFF 构建引入
// IGV 的类型依赖；`igv::IGVMetadata` 是纯数据（不含 HDF5），见 §7.1。
#include "IGV/iGameIGVFormat.h"
#endif

IGAME_NAMESPACE_BEGIN
class FileIO : public Object {
public:
    I_OBJECT(FileIO);

    enum FileType {
        NONE = 0,
        VTK,
        IGC,
        OBJ,
        OFF,
        MESH,
        STL,
        PLY,
        STEP,
        IGES,
        PVD,
        VTU,
        VTP,
        VTM,
        VTS,
        VTR,
        EX2,
        CGNS,
        INP,
        ODB,
        CAS,
        CCM,
        RST,
        RTH,
        BDF,
        IGCM,
        D3PLOT,
        SPECTRAL_DAT,
        SPECTRAL_NEKTAR,
        VTI, // ImageData support required by the Median example model
        // IGV（iGameVis 通用格式，HDF5 容器）。按 kia/igv分阶段实现方案.md §9 P2
        // 要求插入到 **FILETYPE_COUNT 之前**：既有取值一个都不变（已有格式的
        // switch 分支因此零回归），只把计数加一。
        IGV,
        FILETYPE_COUNT
    };

    static DataObject::Pointer ReadFile(const std::string& file_name);
    // Explicit C/S entry; ordinary ReadFile never opts into remote rendering.
    static DataObject::Pointer ReadRemoteFile(const std::string& file_name);
    static DataObject::Pointer ReadVTKFromMemory(const void* data, size_t size);
    static DataObject::Pointer ReadVTUFromMemory(const void* data, size_t size);
    static DataObject::Pointer ReadVTPFromMemory(const void* data, size_t size);
    static DataObject::Pointer ReadIGCFromMemory(const void* data, size_t size);
    // IGV（HDF5 容器）内存路径读入。与 ReadIGCFromMemory 同构。
    // 返回 nullptr 表示失败（含 ENABLE_IGV_MODULE=OFF 的构建 —— 此时签名仍在，
    // 只是不产生对象，UI 不会因为"少一个符号"而在链接期/运行期崩溃）。
    static DataObject::Pointer ReadIGVFromMemory(const void* data, size_t size);

#if defined(IGV_ENABLE)
    /**
     * IGV **元数据快路径**（P3，§7）：不加载 mesh、不解压任何 dataset，
     * 只返回包围盒 / 计数 / 属性清单（含每个属性的 dtype、分量数、预计算 range）。
     *
     * 入口会 `IGVStats::Reset()`（§11.3：入口清零、出口不清），因此调用方可以在返回后
     * 读取 `igv::IGVStats::h5dreadCount` 断言"本次快路径没读任何数据"。
     *
     * @param error 非空时写入失败原因
     * @return 失败时 `out` 内容未定义（实现内部会先清零再解析）
     */
    static bool ReadIGVMetadata(const std::string& file_name, igv::IGVMetadata& out,
                                std::string* error = nullptr);
#endif

    static bool WriteFile(const std::string& file_name, DataObject::Pointer);
    static IGenum GetFileType(const std::string& file_name);
    static std::string GetFileTypeAsString(IGenum type);
private:
    static DataObject::Pointer ReadFileWithRenderingPolicy(const std::string& file_name, bool remoteRendering);
};

IGAME_NAMESPACE_END
#endif
