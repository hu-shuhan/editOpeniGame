#ifndef iGameIGVReader_h
#define iGameIGVReader_h

/**
 * @file  iGameIGVReader.h
 * @brief IGV (iGameVis universal format) reader.
 *
 * FileReader::Execute() runs Open() -> Parsing() -> Close() -> CreateDataObject().
 * Only Parsing() and CreateDataObject() are hooks.
 *
 * Parsing() opens the file with HDF5's own fapl (core VFD or sec2), reads everything it
 * needs into memory, and closes HDF5. CreateDataObject() must not touch HDF5 afterward.
 *
 * Mesh reconstruction uses MeshDecodeAdapterToDataObject, which handles the five mesh
 * kinds, attribute attachment, and structured-grid implicit topology.
 *
 * This header does not include hdf5.h.
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

    /// 探测拓扑存在性、版本检查，并把**所有**需要在 CreateDataObject() 用到的
    /// 字节读进内存。
    bool Parsing() override;

    bool CreateDataObject() override;

    /// 最近一次失败的原因（供测试断言错误信息内容，如 V1.6 的
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
        /// 属性值的**原始字节**（按 `dtype` 解释；读入时已由 HDF5 转成本机字节序）。
        ///
        /// 用字节而不是"每种 dtype 一个 vector"：10 类定宽 dtype 只需一条读取/重建路径，
        std::vector<unsigned char> bytes;
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

    //   m_Connectivity     = 体 → 面 的面 id（**不是**点 id）
    //   m_FaceConnectivity = 面 → 点 的点 id
    bool m_SecondaryIndex{false};
    std::vector<int32_t> m_FaceConnectivity;
    std::vector<int64_t> m_FaceOffsets;
    int64_t m_FaceCount{0};

    std::vector<ParsedAttribute> m_Attributes;

    /// 解析出的 iGame 数据对象类型（由 IGVDataType 优先、Type 兜底推导）。
    IGenum m_MeshType{IG_NONE};

 void Fail(const std::string& reason);

    // -------------------------------------------------------------------------
    // 解析步骤
    // -------------------------------------------------------------------------
    //
    // 句柄参数的类型是 **`int64_t` 而不是 `hid_t`**，原因见 .cpp 顶部的说明：
    // `typedef long long`，无法在头里前向 typedef 成 `int64_t` 而不产生
    // "两个不同的 typedef" 问题（friend 声明与定义将对不上）。
    //
    // 用 `int64_t` 传递句柄值即可完全回避：.cpp 内部做
    // `static_cast<hid_t>(...)`，并有 `static_assert(sizeof(hid_t) == sizeof(int64_t))`
    // 把宽度假设钉死在编译期。

    bool ParseRootAttributes(int64_t root);

    bool ParseOneAttribute(int64_t group, const std::string& groupName,
                           const std::string& name);

    bool ParseGeometryAndTopology(int64_t root);

    /// 多面体的面表：解析 FaceConnectivity / FaceOffsets / NumberOfFaces 并交叉校验
    bool ParsePolyhedronFaces(int64_t root, int64_t declaredCells, int64_t pointCount);

    ///
    /// 必须在 `ParseGeometryAndTopology()` **之后**调用 —— pointCount / cellCount
    /// 是那一步才确定的。属性自己的 dataspace 只能保证"内部自洽"，
    /// 「point 属性的长度等于点数」这条**跨对象**约束原先只有写侧在把关；
    bool ValidateAttributeLengths();

    bool RebuildFromParsed();

    /// (attachmentToken / attributeTypeToken) → iGame 枚举。
    static IGenum MapAttachmentToIGenum(std::string_view token);
    static IGenum MapAttributeTypeToIGenum(std::string_view token);

    /// 由 `IGVDataType`（优先）/ `Type`（兜底）推导 iGame 的 DataObjectType。
    static IGenum MapIGVDataTypeToMeshType(std::string_view igvDataType,
                                          std::string_view type);
};

IGAME_NAMESPACE_END

#endif // iGameIGVReader_h
