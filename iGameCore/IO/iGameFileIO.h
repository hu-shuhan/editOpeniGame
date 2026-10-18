#ifndef iGameFileIO_h
#define iGameFileIO_h

#include "MeshCodec/Utils/iGameMeshCodecParams.h"
#include "iGameDataObject.h"
#include "iGameObject.h"
#include <cstddef>

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
    static bool WriteFile(const std::string& file_name, DataObject::Pointer);
    static IGenum GetFileType(const std::string& file_name);
    static std::string GetFileTypeAsString(IGenum type);
private:
    static DataObject::Pointer ReadFileWithRenderingPolicy(const std::string& file_name, bool remoteRendering);
};

IGAME_NAMESPACE_END
#endif
