/**
 * @class   iGameVTIReader
 * @brief   VTK XML ImageData(.vti) 读取器。
 *
 *          ImageData 的点坐标是隐含的（Origin + Spacing ×(i,j,k)），因此本读取器直接构造
 *          结构化网格 StructuredMesh：WholeExtent/Piece Extent → 维度，Origin/Spacing → 点坐标，
 *          PointData / CellData 中的每个 <DataArray> → 一条属性（IG_TENSOR / IG_VECTOR /
 *          IG_NORMAL / IG_SCALAR）。
 *
 *          数据编码（ascii / binary / appended+base64 / appended+raw / vtkZLibDataCompressor）
 *          统一交给共享解码器 iGameVTKXMLDataArrayDecoder 处理，与 iGameVTUReader /
 *          iGameVTPReader 保持一致。
 */
#ifndef iGameVTIReader_h
#define iGameVTIReader_h

#include "XML/iGameXMLFileReader.h"
#include "iGameVTKXMLDataArrayDecoder.h"

#include <string>
#include <vector>

namespace tinyxml2 {
    class XMLElement;
}

IGAME_NAMESPACE_BEGIN

class iGameVTIReader : public iGameXMLFileReader {
public:
    I_OBJECT(iGameVTIReader)

    static Pointer New() { return new iGameVTIReader; }

    bool Parsing() override;

    bool CreateDataObject() override;

protected:
    iGameVTIReader() = default;
    ~iGameVTIReader() override = default;

private:
    /* 读取 <ImageData> 的 WholeExtent/Origin/Spacing 与 <Piece> 的 Extent。 */
    bool ReadImageDataInfo();

    /* 由 Origin/Spacing/Extent 生成隐含点坐标。 */
    bool BuildPoints();

    /* 读取一个 <PointData> 或 <CellData> 段下的所有 <DataArray>。 */
    bool ReadAttributes(tinyxml2::XMLElement* section, IGenum attachmentType, IGsize tupleNum);

    /* 读取单个 <DataArray>，按名字判定属性类型后加入 AttributeSet。 */
    bool ReadOneArray(tinyxml2::XMLElement* elem, IGenum attachmentType, IGsize tupleNum);

    /* <AppendedData> 中数据区起始指针（跳过 '_'）。 */
    const char* GetAppendedDataHead();

    /* 用共享解码器把 binary / appended DataArray 解成未压缩字节。 */
    bool DecodeDataArrayPayload(tinyxml2::XMLElement* element, vtkxml::ByteBuffer& output);

private:
    igIndex m_PointSize[3]{1, 1, 1};         // 各方向点数（Piece/WholeExtent）
    igIndex m_Extent[6]{0, 0, 0, 0, 0, 0};   // Piece Extent，用于隐含坐标的起点偏移
    double m_Origin[3]{0.0, 0.0, 0.0};
    double m_Spacing[3]{1.0, 1.0, 1.0};
    IGsize m_PointNum{0};
    IGsize m_CellNum{0};

    bool m_RawAppendedData{false};      // <AppendedData encoding="raw">
    const char* m_AppendedDataHead{nullptr};
    std::vector<char> m_DataArraySourceBuffer;  // raw AppendedData 的整文件字节（懒加载）

    // <PointData>/<CellData> 上的 Tensors/Normals/Vectors 名字列表（决定属性类型）
    std::vector<std::string> m_TensorNames;
    std::vector<std::string> m_VectorNames;
    std::vector<std::string> m_NormalNames;
};

IGAME_NAMESPACE_END
#endif
