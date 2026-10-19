#ifndef iGameIGVReader_h
#define iGameIGVReader_h

/**
 * @file  iGameIGVReader.h
 * @brief IGV（iGameVis 通用格式）读侧主体。
 *
 * 对应 kia/igv分阶段实现方案.md §6「读侧详细设计」与 §9 P1。
 *
 * 复用支点（§4.1 支点 2）：
 *   `FileReader::Execute()` 的顺序是 `Open() → Parsing() → Close() → CreateDataObject()`，
 *   其中只有 `Parsing()`（纯虚）与 `CreateDataObject()`（虚）是钩子，**基类零改动**。
 *
 * 重建复用支点（§4.1 支点 3）：
 *   `MeshDecodeAdapterToDataObject` 提供「原始数组 → DataObject」的完整通路，
 *   五类网格的重建、属性挂载、结构化网格隐式拓扑全部白拿。
 *   **不改动该适配器**（§4.4 第 3 条）。
 *
 * ⚠ R3 / §6.1 的关键约束：`Close()` 在 `CreateDataObject()` **之前**执行，
 *   基类此时已释放/解除映射 `FILESTART`。因此本实现**不使用**基类的文件缓冲，
 *   而是用 HDF5 自己的 fapl（core VFD / sec2）打开文件，并在 `Parsing()` 内
 *   `H5Fclose` 之前把需要的一切读进内存成员。`CreateDataObject()` 之后不再触碰 HDF5。
 *
 * HDF5 隔离（§13 第 7 条）：本头文件不含 `#include <hdf5.h>`。
 */

#include <cstdint>
#include <string>
#include <vector>

#include "iGameFileReader.h"
#include "iGameIGVFormat.h"

IGAME_NAMESPACE_BEGIN

class IGVReader : public FileReader {
public:
    I_OBJECT(IGVReader);
    static Pointer New() { return new IGVReader; }

    /// §6.1 主流程：定位 /IGV、读 attribute、枚举 dataset 名、
    /// 探测拓扑存在性、版本检查，并把**所有**需要在 CreateDataObject() 用到的
    /// 字节读进内存。
    bool Parsing() override;

    /// §6.2 主流程：经 `MeshDecodeAdapterToDataObject` 重建 DataObject。
    bool CreateDataObject() override;

    /// 最近一次失败的原因（供测试断言错误信息内容，如 V1.6 的
    /// 「根组不是 /IGV」、§6.3 的计数组交叉校验失败）。
    [[nodiscard]] const std::string& GetLastError() const { return m_LastError; }

    /// 本次 Parsing() 读到的原始元数据（供测试与 P3 交叉验证）。
    ///
    /// 返回类型写作 `igv::IGVMetadata` 而非 `IGVMetadata`：类内成员
    /// `igv::IGVMetadata m_Meta;` 会遮蔽同名的命名空间类型，无限定写法在类作用域里
    /// 会被解析成"成员"，从而报 "is not a type name"。加 `igv::` 限定即可。
    [[nodiscard]] const igv::IGVMetadata& GetParsedMetadata() const { return m_Meta; }

    /// 时序探针（仅供测试验证 R3 的时序约束）：
    /// 为 true 时 `CreateDataObject()` 会记录一条"此时 HDF5 句柄应已全部关闭"的
    /// 日志，用于证明本实现确实没有依赖"Close 之后还能读 HDF5"。
    /// 产品路径不使用。
    void SetAssertHdf5ClosedInCreateDataObject(bool v) { m_AssertHdf5Closed = v; }

protected:
    IGVReader() = default;
    ~IGVReader() override = default;

private:
    // -------------------------------------------------------------------------
    // 暂存结构：Parsing() 把 HDF5 里的东西读成这些普通内存对象，
    // CreateDataObject() 只消费它们（R3 的落实方式）。
    // -------------------------------------------------------------------------

    struct ParsedAttribute {
        std::string name;
        std::string groupName;         ///< PointData / CellData / FieldData
        std::string attachmentToken;
        std::string attributeTypeToken;
        IGenum attachmentTypeValue{IG_NONE};
        IGenum attributeTypeValue{IG_SCALAR};
        int64_t count{0};
        int components{1};
        igv::DType dtype{igv::DType::F32};
        std::vector<double> range;
        std::vector<float> f32;
        std::vector<double> f64;
        std::vector<unsigned char> u8;
    };

    igv::IGVMetadata m_Meta;
    std::string m_LastError;
    bool m_AssertHdf5Closed{false};

    // 几何 / 拓扑
    std::vector<float> m_Points;      ///< (n,3) 扁平
    std::vector<int32_t> m_Connectivity;
    std::vector<int64_t> m_Offsets;
    std::vector<uint8_t> m_Types;
    bool m_HasTopology{false};
    int64_t m_FixedCellSize{-1};

    std::vector<ParsedAttribute> m_Attributes;

    /// 解析出的 iGame 数据对象类型（由 IGVDataType 优先、Type 兜底推导）。
    IGenum m_MeshType{IG_NONE};

 void Fail(const std::string& reason);

    // -------------------------------------------------------------------------
    // 解析步骤
    // -------------------------------------------------------------------------
    //
    // 句柄参数的类型是 **`int64_t` 而不是 `hid_t`**，原因见 .cpp 顶部的说明：
    // 头文件不得 `#include <hdf5.h>`（§13 第 7 条），而 HDF5 的 `hid_t` 是
    // `typedef long long`，无法在头里前向 typedef 成 `int64_t` 而不产生
    // "两个不同的 typedef" 问题（friend 声明与定义将对不上）。
    //
    // 用 `int64_t` 传递句柄值即可完全回避：.cpp 内部做
    // `static_cast<hid_t>(...)`，并有 `static_assert(sizeof(hid_t) == sizeof(int64_t))`
    // 把宽度假设钉死在编译期。

    /// 读根组 attribute + 版本检查 + 类型推导（§6.1 步骤 3–5）。
    bool ParseRootAttributes(int64_t root);

    /// 读一个属性 dataset 的 attribute 与数据（§6.1 步骤 7）。
    bool ParseOneAttribute(int64_t group, const std::string& groupName,
                           const std::string& name);

    /// 读几何/拓扑 dataset 并做 §6.3 的全部交叉校验（§6.1 步骤 8）。
    bool ParseGeometryAndTopology(int64_t root);

    /// 属性的**元素数**必须与它依附的对象数量一致（§6.3 / P5-5）。
    ///
    /// 必须在 `ParseGeometryAndTopology()` **之后**调用 —— pointCount / cellCount
    /// 是那一步才确定的。属性自己的 dataspace 只能保证"内部自洽"，
    /// 「point 属性的长度等于点数」这条**跨对象**约束原先只有写侧在把关；
    /// P5 把它补到读侧（§6.3 明确要求读侧拒绝）。
    bool ValidateAttributeLengths();

    /// 把在 Parsing() 里暂存的原始数组交给解码适配器，重建出 DataObject（§6.2）。
    bool RebuildFromParsed();

    /// (attachmentToken / attributeTypeToken) → iGame 枚举。
    static IGenum MapAttachmentToIGenum(std::string_view token);
    static IGenum MapAttributeTypeToIGenum(std::string_view token);

    /// 由 `IGVDataType`（优先）/ `Type`（兜底）推导 iGame 的 DataObjectType。
    /// 返回 IG_NONE 表示无法推导（调用方必须拒绝，不得猜测 —— §6.3）。
    static IGenum MapIGVDataTypeToMeshType(std::string_view igvDataType,
                                          std::string_view type);
};

IGAME_NAMESPACE_END

#endif // iGameIGVReader_h
