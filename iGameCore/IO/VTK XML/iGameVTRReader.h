/**
 * @class   iGameVTRReader
 * @brief   VTK XML RectilinearGrid (.vtr) 读取器。
 *
 * @details
 *  RectilinearGrid 通过 <Coordinates> 的 X/Y/Z 三组坐标数组描述非均匀规则网格，
 *  读取后按 i + j*nx + k*nx*ny 的顶点顺序展开成 StructuredMesh（与 .vts 一致），
 *  使后续体渲染/处理逻辑可直接复用。
 *
 *  二进制（inline base64 / appended）数据统一走 iGameVTKXMLDataArrayDecoder 共享解码器，
 *  正确支持 UInt32/UInt64 头、base64 填充、appended 偏移与 zlib 压缩；
 *  不再使用 iGameBase64Util 中无法正确处理 base64 '=' 填充的旧解码路径。
 */
#ifndef iGameVTRReader_h
#define iGameVTRReader_h

#include "XML/iGameXMLFileReader.h"
#include "iGameVTKXMLDataArrayDecoder.h"

#include <string>
#include <vector>

namespace tinyxml2 {
class XMLElement;
}

IGAME_NAMESPACE_BEGIN
class iGameVTRReader : public iGameXMLFileReader {
public:
    I_OBJECT(iGameVTRReader)

    static Pointer New() { return new iGameVTRReader; }

    bool Parsing() override;

    bool CreateDataObject() override;

protected:
    iGameVTRReader() = default;
    ~iGameVTRReader() = default;

    // 通过共享解码器把 binary/appended DataArray 解码为原始字节。
    bool DecodeDataArrayPayload(tinyxml2::XMLElement* element, vtkxml::ByteBuffer& output);
    // 定位 <AppendedData> 的文本头（base64）或标记 raw 模式。
    const char* GetAppendDataHead();
    void SetDataArrayDecodeError(const std::string& message);

    DoubleArray::Pointer ParseCoordinateArray(tinyxml2::XMLElement* elem, int count);
    void AddDataArray(tinyxml2::XMLElement* elem, IGenum attachmentType);

protected:
    char* m_AppendedDataHead{nullptr};
    bool m_parseRawBinaryData{false};
    bool m_DataArrayDecodeFailed{false};
    std::string m_DataArrayDecodeError;
    std::vector<char> m_DataArraySourceBuffer;
};

IGAME_NAMESPACE_END
#endif // iGameVTRReader_h
