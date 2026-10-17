#ifndef iGameIGVWriter_h
#define iGameIGVWriter_h

/**
 * @file  iGameIGVWriter.h
 * @brief IGV（iGameVis 通用格式）写侧主体。
 *
 * 对应 kia/igv分阶段实现方案.md §5「写侧详细设计」与 §9 P1。
 *
 * 复用支点（§4.1 支点 1）：
 *   `FileWriter::GenerateBuffers()` 是唯一的纯虚钩子，`Execute()` 在它之后统一走
 *   `SaveBufferDataToFile()`。因此 IGV 只需在 `GenerateBuffers()` 里用 HDF5 的
 *   **core VFD** 在内存中生成完整 file image，再塞进 `m_Buffers[0]`，
 *   **基类零改动**即可复用其 Windows/Linux/macOS 三套落盘实现。
 *
 * 取数复用支点（§4.1 支点：不重复实现点/单元遍历）：
 *   经由 `MeshEncodeAdapterFromDataObject` 取统一视图 —— 与 IGC 编码器同一套
 *   已验证的取数逻辑，避免出现"写出遍历"与"编码遍历"两套语义。
 *
 * HDF5 隔离（§4.4 第 7 条 / §13 第 7 条）：
 *   本头文件**不含** `#include <hdf5.h>`，全部 HDF5 句柄以 `void*` 持有。
 *
 * 与既有格式的隔离（§4.4）：
 *   不修改 `FileWriter` 基类、不修改 `MeshCodec` 任何文件、不进入 `FileIO` 分支
 *   （P1 阶段测试直接实例化本类；接入 FileIO 是 P2）。
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
    /// 写侧主流程（§5.1 步骤 1–12）。
    ///
    /// 失败语义见 §5.4：**每一个"拒绝"都是 return false + IGAME_CORE_ERROR**，
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
     *   1. 测试可直接校验"写出的字节"而不必落盘（§11 的 L1/L2 层）；
     *   2. P2 的 `FileIO::WriteFile` 之外还需要"写出到内存"的能力。
     *
     * 注意：`FileWriter::SaveBufferDataToFile()` 会 `m_Buffers.clear()`，
     * 因此调用方必须在 `Execute()` **之前**取走，或用
     * `GenerateBuffers()` + `GetMemoryImage()` 的组合自行落盘。
     */
    [[nodiscard]] const std::vector<char>& GetMemoryImage() const { return m_MemoryImage; }

    /// 独立于 FileWriter 落盘流程的直写入口（§5.1 步骤 5 的 DirectPath 分支）。
    ///
    /// 与 `WriteToFile(obj, path)` 的区别：后者走 MemoryImage（内存镜像再落盘），
    /// 本函数让 HDF5 直接写目标路径，适用于 >2GB 场景（R2 / P7-4）。
    bool WriteToFilePathDirect(DataObject::Pointer dataObject, const std::string& filePath);

protected:
    IGVWriter() = default;
    ~IGVWriter() override = default;

private:
    /// 前置校验（§5.1 步骤 1）。任一失败写 m_LastError 并返回 false。
    bool ValidateDataObject();

    /// §5.1 步骤 5–10：在内存里建出完整 HDF5 file image。
    /// @param outImage 产出字节；失败时不修改
    bool BuildFileImage(std::vector<char>& outImage);

    /// §5.1 步骤 5 的 DirectPath 分支：H5Fcreate(path) 直接写。
    bool BuildFileAtPath(const std::string& path);

    /// 写盘的公共实现（两种 writeMode 共用）。
    /// @param memoryImage 非空时表示 MemoryImage 模式
    bool BuildHdf5(const std::string& pathForDirectWrite, std::vector<char>* memoryImage);

 void Fail(const std::string& reason);
};

IGAME_NAMESPACE_END

#endif // iGameIGVWriter_h
