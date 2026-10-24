#ifndef iGameIGVWriter_h
#define iGameIGVWriter_h

/**
 * @file  iGameIGVWriter.h
 * @brief IGV (iGameVis universal format) writer.
 *
 * GenerateBuffers() builds a complete HDF5 file image with the core VFD and stores it in
 * m_Buffers; the FileWriter base class then saves it with the existing platform code.
 *
 * Mesh data is read through MeshEncodeAdapterFromDataObject, the same traversal used by
 * the IGC encoder, so point/cell ordering has one definition.
 *
 * This header does not include hdf5.h; all HDF5 handles are held as void*.
 */

#include <string>
#include <vector>

#include "iGameFileWriter.h"
#include "iGameIGVFormat.h"

IGAME_NAMESPACE_BEGIN

class IGVWriter : public FileWriter {
public:
    I_OBJECT(IGVWriter);
    static Pointer New() { return new IGVWriter; }

    // -------------------------------------------------------------------------
    // 私有成员放在最前
    // -------------------------------------------------------------------------
    //
    // **为什么成员声明放在 public 方法之前**：C++ 的**类作用域**名字查找覆盖
    // 整个类体，与声明顺序无关。若把 `igv::IGVWriteOptions m_Options;` 写在
    // `SetWriteOptions(const igv::IGVWriteOptions&)` **之后**的某个访问区里，
    // 那么 `IGVWriteOptions` 这个名字在类作用域内会优先解析成那个**成员**
    // （`IGVWriteOptions m_Options;` 的声明本身也会失败），编译器报
    // "m_Options: unknown override specifier"。
    //
    // 这一点在本文件上真实发生过（首次编译即报 3 条 C3646/C2059）。
    // 修法有两种：把成员提前，或所有类型引用都加命名空间限定。这里两者都做了
    // —— 提前声明是根治，限定是冗余保险。
private:
    igv::IGVWriteOptions m_Options{};
    std::string m_LastError;
    std::vector<char> m_MemoryImage;

public:
    ///
    /// 不得退化为 assert（A9）。P1 的 V1.7 逐个覆盖这些拒绝用例。
    bool GenerateBuffers() override;

    /// 写侧选项（压缩策略 / 写盘方式 / Generator 串）。
    void SetWriteOptions(const igv::IGVWriteOptions& options) { m_Options = options; }
    [[nodiscard]] const igv::IGVWriteOptions& GetWriteOptions() const { return m_Options; }

    /// 最近一次失败的原因（供测试断言错误信息内容，如 V1.7 第 1 项的
    /// "Points must be float32"）。成功时为上一次的残留值，测试应配合返回值使用。
    [[nodiscard]] const std::string& GetLastError() const { return m_LastError; }

    /**
     * 从内存镜像中取回最近一次写出的 .igv 字节。
     *
     * 仅 `IGVWriteMode::MemoryImage` 下有内容。存在的目的有两个：
     *   2. P2 的 `FileIO::WriteFile` 之外还需要"写出到内存"的能力。
     *
     * 注意：`FileWriter::SaveBufferDataToFile()` 会 `m_Buffers.clear()`，
     * 因此调用方必须在 `Execute()` **之前**取走，或用
     * `GenerateBuffers()` + `GetMemoryImage()` 的组合自行落盘。
     */
    [[nodiscard]] const std::vector<char>& GetMemoryImage() const { return m_MemoryImage; }

    ///
    /// 与 `WriteToFile(obj, path)` 的区别：后者走 MemoryImage（内存镜像再落盘），
    /// 本函数让 HDF5 直接写目标路径，适用于 >2GB 场景（R2 / P7-4）。
    bool WriteToFilePathDirect(DataObject::Pointer dataObject, const std::string& filePath);

protected:
    IGVWriter() = default;
    ~IGVWriter() override = default;

private:
    bool ValidateDataObject();

    /// @param outImage 产出字节；失败时不修改
    bool BuildFileImage(std::vector<char>& outImage);

    bool BuildFileAtPath(const std::string& path);

    /// 写盘的公共实现（两种 writeMode 共用）。
    /// @param memoryImage 非空时表示 MemoryImage 模式
    bool BuildHdf5(const std::string& pathForDirectWrite, std::vector<char>* memoryImage);

 void Fail(const std::string& reason);
};

IGAME_NAMESPACE_END

#endif // iGameIGVWriter_h
