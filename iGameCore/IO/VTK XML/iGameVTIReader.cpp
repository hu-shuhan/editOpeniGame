//
// VTK XML ImageData(.vti) 读取器。
//

/**
 * @class   iGameVTIReader
 * @brief   VTK XML ImageData(.vti) 读取器。
 */

#include "iGameVTIReader.h"
#include "iGameFileSystem.h"

#include <tinyxml2.h>

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include <iGameStructuredMesh.h>
#include <iGameType.h>

#undef max
#undef min

IGAME_NAMESPACE_BEGIN

namespace {
using ByteBuffer = vtkxml::ByteBuffer;

/* 把二进制数据按 T 追加到属性数组，最多追加 valueCount 个值（T 既是源元素也是目标元素类型）。
   按本机（LittleEndian）字节序直接解释，与 iGameVTUReader/iGameVTPReader 一致。 */
template<typename T>
void AppendBytesToFlatArray(const ByteBuffer& bytes, IGsize valueCount, typename FlatArray<T>::Pointer arr) {
    if (arr == nullptr || bytes.empty()) { return; }
    IGsize count = bytes.size() / sizeof(T);
    if (valueCount != 0 && count > valueCount) { count = valueCount; }
    arr->Reserve(count / static_cast<IGsize>(std::max(1, arr->GetDimension())));
    for (IGsize i = 0; i < count; ++i) {
        T value{};
        std::memcpy(&value, bytes.data() + i * sizeof(T), sizeof(T));
        arr->AddValue(value);
    }
}

/**
 * @brief 按「源元素宽度」读取整数并追加到目标整数数组。
 *
 * 取值的宽度由 VTK 的 type 决定（Int8 = 1 字节/元素），目标容器按该 type 对应的
 * 精确数组类型选择（Int8 → CharArray、UInt8 → UnsignedCharArray、……，与
 * iGameVTUReader::ReadIntegerDataArray 一致），不能按容器宽度去读：Int8 的 1 若按
 * 4 字节读会变成 0x01010101 = 16843009，且元素个数只剩应有的 1/4。
 */
template<typename TDst>
void AppendRawIntegers(const ByteBuffer& bytes, IGsize valueCount, IGsize srcSize, bool srcUnsigned,
                       typename FlatArray<TDst>::Pointer arr) {
    if (arr == nullptr || srcSize == 0 || bytes.empty()) { return; }
    IGsize count = bytes.size() / srcSize;
    if (valueCount != 0 && count > valueCount) { count = valueCount; }
    arr->Reserve(count / static_cast<IGsize>(std::max(1, arr->GetDimension())));
    for (IGsize i = 0; i < count; ++i) {
        const unsigned char* p = bytes.data() + i * srcSize;
        long long value = 0;
        switch (srcSize) {
            case 1:
                value = srcUnsigned ? static_cast<long long>(p[0])
                                    : static_cast<long long>(static_cast<int8_t>(p[0]));
                break;
            case 2: {
                uint16_t raw{};
                std::memcpy(&raw, p, sizeof(raw));
                value = srcUnsigned ? static_cast<long long>(raw)
                                    : static_cast<long long>(static_cast<int16_t>(raw));
                break;
            }
            case 4: {
                uint32_t raw{};
                std::memcpy(&raw, p, sizeof(raw));
                value = srcUnsigned ? static_cast<long long>(raw)
                                    : static_cast<long long>(static_cast<int32_t>(raw));
                break;
            }
            default: {
                uint64_t raw{};
                std::memcpy(&raw, p, sizeof(raw));
                value = static_cast<long long>(raw);
                break;
            }
        }
        arr->AddValue(static_cast<TDst>(value));
    }
}

/* 把 ascii 文本按浮点类型 T 追加到属性数组，最多追加 valueCount 个值。 */
template<typename T>
void AppendAsciiToFlatArray(const char* text, IGsize valueCount, typename FlatArray<T>::Pointer arr) {
    if (text == nullptr || arr == nullptr) { return; }
    std::istringstream stream(text);
    double value = 0.0;
    while ((valueCount == 0 || arr->GetNumberOfValues() < valueCount) && (stream >> value)) {
        arr->AddValue(static_cast<T>(value));
    }
}

/**
 * @brief 把 ascii 文本按整数类型直接解析后追加到属性数组，最多 valueCount 个值。
 *
 * 【不能复用读 double 的版本】double 只有 53 位尾数，ASCII 里的 64 位整数先读成 double
 * 再转回整数会丢精度。这里按 srcUnsigned 选择 strtoll / strtoull 直接解析成 64 位整数，
 * 再窄化到目标类型。
 */
template<typename T>
void AppendAsciiIntegersToFlatArray(const char* text, IGsize valueCount, bool srcUnsigned,
                                    typename FlatArray<T>::Pointer arr) {
    if (text == nullptr || arr == nullptr) { return; }
    std::istringstream stream(text);
    std::string token;
    while ((valueCount == 0 || arr->GetNumberOfValues() < valueCount) && (stream >> token)) {
        char* end = nullptr;
        long long value = 0;
        errno = 0;
        if (srcUnsigned) {
            const unsigned long long parsed = std::strtoull(token.c_str(), &end, 10);
            if (end == token.c_str() || errno == ERANGE) { return; }
            value = static_cast<long long>(parsed);
        } else {
            value = std::strtoll(token.c_str(), &end, 10);
            if (end == token.c_str() || errno == ERANGE) { return; }
        }
        if (*end != '\0') { return; } // 不是纯整数记号：停止解析，由长度校验判为不完整
        arr->AddValue(static_cast<T>(value));
    }
}

/* VTK DataArray/@type 的字节宽度（同时兼容 XML 里的旧名字，如 char/short/unsigned_int/double）。 */
IGsize SizeOfVtkType(const char* type) {
    if (type == nullptr) { return 4; }
    if (std::strncmp(type, "Float64", 7) == 0 || std::strcmp(type, "double") == 0 ||
        std::strncmp(type, "Int64", 5) == 0 || std::strncmp(type, "UInt64", 6) == 0 ||
        std::strncmp(type, "long_long", 9) == 0 || std::strncmp(type, "unsigned_long_long", 18) == 0) {
        return 8;
    }
    if (std::strncmp(type, "Int16", 5) == 0 || std::strncmp(type, "UInt16", 6) == 0 ||
        std::strcmp(type, "short") == 0 || std::strcmp(type, "unsigned_short") == 0) {
        return 2;
    }
    if (std::strncmp(type, "Int8", 4) == 0 || std::strncmp(type, "UInt8", 5) == 0 ||
        std::strcmp(type, "char") == 0 || std::strcmp(type, "signed_char") == 0 ||
        std::strcmp(type, "unsigned_char") == 0) {
        return 1;
    }
    return 4; // Float32 / float / Int32 / UInt32 / int / unsigned_int
}

bool IsUnsignedVtkType(const char* type) {
    if (type == nullptr) { return false; }
    return std::strncmp(type, "UInt", 4) == 0 || std::strncmp(type, "unsigned", 8) == 0;
}

bool IsFloat64VtkType(const char* type) {
    if (type == nullptr) { return false; }
    return std::strncmp(type, "Float64", 7) == 0 || std::strcmp(type, "double") == 0;
}

bool IsFloatVtkType(const char* type) {
    if (type == nullptr) { return true; } // 缺省按 Float32 处理
    return std::strncmp(type, "Float", 5) == 0 || std::strcmp(type, "float") == 0;
}

/* 解析 "a,b,c" 形式的属性名列表。 */
void SplitNameList(const char* text, std::vector<std::string>& names) {
    if (text == nullptr) { return; }
    std::istringstream stream(text);
    std::string name;
    while (std::getline(stream, name, ',')) {
        const auto begin = name.find_first_not_of(" \t\n\r");
        if (begin == std::string::npos) { continue; }
        const auto end = name.find_last_not_of(" \t\n\r");
        names.push_back(name.substr(begin, end - begin + 1));
    }
}

bool ContainsName(const std::vector<std::string>& names, const std::string& name) {
    return std::find(names.begin(), names.end(), name) != names.end();
}

/* 解析 6 个整数的范围字符串（WholeExtent / Extent）。 */
bool ParseExtent(const char* text, igIndex* extent) {
    if (text == nullptr) { return false; }
    std::istringstream stream(text);
    for (int i = 0; i < 6; ++i) {
        if (!(stream >> extent[i])) { return false; }
    }
    return true;
}

/* 解析 3 个浮点数的字符串（Origin / Spacing）。 */
bool ParseTriple(const char* text, double* values) {
    if (text == nullptr) { return false; }
    std::istringstream stream(text);
    for (int i = 0; i < 3; ++i) {
        if (!(stream >> values[i])) { return false; }
    }
    return true;
}
} // namespace

const char* iGameVTIReader::GetAppendedDataHead() {
    if (m_AppendedDataHead != nullptr) { return m_AppendedDataHead; }

    auto* elem = FindTargetItem(root, "AppendedData");
    if (elem == nullptr) { return nullptr; }

    const char* encoding = elem->Attribute("encoding");
    if (encoding != nullptr && std::strncmp(encoding, "raw", 3) == 0) { m_RawAppendedData = true; }

    const char* data = elem->GetText();
    if (data == nullptr) { return nullptr; }
    while (*data == '\n' || *data == ' ' || *data == '\t' || *data == '\r') { ++data; }
    if (*data == '_') { ++data; } // VTK 在数据区起始处写一个下划线占位

    m_AppendedDataHead = data;
    return m_AppendedDataHead;
}

bool iGameVTIReader::DecodeDataArrayPayload(tinyxml2::XMLElement* element, vtkxml::ByteBuffer& output) {
    const char* format = element != nullptr ? element->Attribute("format") : nullptr;
    const char* appended = nullptr;
    if (format != nullptr && std::strcmp(format, "appended") == 0) { appended = GetAppendedDataHead(); }

    vtkxml::DataArrayDecodeContext context;
    context.root = root;
    context.appendedData = appended;
    context.sourceData = m_MemoryBuffer;
    context.sourceSize = m_MemoryBufferSize;
    // raw AppendedData 需要整文件字节（按 offset 定位 '_' 之后的数据），与 iGameVTUReader 一致。
    if (format != nullptr && std::strcmp(format, "appended") == 0 && m_RawAppendedData &&
        (context.sourceData == nullptr || context.sourceSize == 0)) {
        if (m_DataArraySourceBuffer.empty()) {
            std::ifstream stream(FileSystem::PathFromUtf8(m_FilePath), std::ios::binary | std::ios::ate);
            if (!stream) {
                IGAME_CORE_ERROR("[iGameVTIReader] cannot open the source file for raw AppendedData.");
                return false;
            }
            const std::streamsize size = stream.tellg();
            if (size <= 0) {
                IGAME_CORE_ERROR("[iGameVTIReader] cannot determine the raw AppendedData source size.");
                return false;
            }
            m_DataArraySourceBuffer.resize(static_cast<std::size_t>(size));
            stream.seekg(0, std::ios::beg);
            if (!stream.read(m_DataArraySourceBuffer.data(), size)) {
                m_DataArraySourceBuffer.clear();
                IGAME_CORE_ERROR("[iGameVTIReader] cannot read the source file for raw AppendedData.");
                return false;
            }
        }
        context.sourceData = m_DataArraySourceBuffer.data();
        context.sourceSize = m_DataArraySourceBuffer.size();
    }

    std::string error;
    if (!vtkxml::DecodeDataArray(element, context, output, error)) {
        const char* name = element != nullptr ? element->Attribute("Name") : nullptr;
        IGAME_CORE_ERROR("[iGameVTIReader] DataArray '{}': {}.", name != nullptr ? name : "<unnamed>", error);
        return false;
    }
    return true;
}

bool iGameVTIReader::ReadImageDataInfo() {
    auto* elem = FindTargetItem(root, "ImageData");
    if (elem == nullptr) {
        IGAME_CORE_ERROR("[iGameVTIReader] Missing ImageData node.");
        return false;
    }

    igIndex wholeExtent[6]{0, 0, 0, 0, 0, 0};
    if (!ParseExtent(elem->Attribute("WholeExtent"), wholeExtent)) {
        IGAME_CORE_ERROR("[iGameVTIReader] Missing or invalid WholeExtent attribute.");
        return false;
    }
    // Origin / Spacing 缺省值：原点 0、间距 1（VTK 默认）
    if (!ParseTriple(elem->Attribute("Origin"), m_Origin)) { m_Origin[0] = m_Origin[1] = m_Origin[2] = 0.0; }
    if (!ParseTriple(elem->Attribute("Spacing"), m_Spacing)) { m_Spacing[0] = m_Spacing[1] = m_Spacing[2] = 1.0; }

    // 只读取第一个 <Piece>；没有 Piece/Extent 时退化为 WholeExtent（单块文件）
    std::copy(wholeExtent, wholeExtent + 6, m_Extent);
    if (auto* piece = FindTargetItem(root, "Piece")) {
        igIndex pieceExtent[6]{0, 0, 0, 0, 0, 0};
        if (ParseExtent(piece->Attribute("Extent"), pieceExtent)) {
            std::copy(pieceExtent, pieceExtent + 6, m_Extent);
        }
    }

    for (int axis = 0; axis < 3; ++axis) {
        m_PointSize[axis] = m_Extent[2 * axis + 1] - m_Extent[2 * axis] + 1;
        if (m_PointSize[axis] < 1) { m_PointSize[axis] = 1; }
    }
    m_PointNum = static_cast<IGsize>(m_PointSize[0]) * static_cast<IGsize>(m_PointSize[1]) *
                 static_cast<IGsize>(m_PointSize[2]);

    // 单元数：2D（size[2] == 1）时是四边形面数，3D 时是六面体数
    const IGsize cellSize[3]{static_cast<IGsize>(std::max(0, m_PointSize[0] - 1)),
                             static_cast<IGsize>(std::max(0, m_PointSize[1] - 1)),
                             static_cast<IGsize>(std::max(0, m_PointSize[2] - 1))};
    m_CellNum = cellSize[0] * cellSize[1] * (m_PointSize[2] > 1 ? cellSize[2] : 1);

    m_Data.dimensionSize[0] = m_PointSize[0];
    m_Data.dimensionSize[1] = m_PointSize[1];
    m_Data.dimensionSize[2] = m_PointSize[2];
    return true;
}

bool iGameVTIReader::BuildPoints() {
    Points::Pointer points = m_Data.GetPoints();
    if (points == nullptr) { return false; }

    points->Reserve(m_PointNum);
    // 隐含坐标：coordinate = Origin + (Extent 起点 + i) * Spacing
    const double start[3]{m_Origin[0] + static_cast<double>(m_Extent[0]) * m_Spacing[0],
                          m_Origin[1] + static_cast<double>(m_Extent[2]) * m_Spacing[1],
                          m_Origin[2] + static_cast<double>(m_Extent[4]) * m_Spacing[2]};
    for (igIndex k = 0; k < m_PointSize[2]; ++k) {
        const double z = start[2] + static_cast<double>(k) * m_Spacing[2];
        for (igIndex j = 0; j < m_PointSize[1]; ++j) {
            const double y = start[1] + static_cast<double>(j) * m_Spacing[1];
            for (igIndex i = 0; i < m_PointSize[0]; ++i) {
                const double x = start[0] + static_cast<double>(i) * m_Spacing[0];
                points->AddPoint(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
            }
        }
    }
    return points->GetNumberOfPoints() == m_PointNum;
}

bool iGameVTIReader::ReadOneArray(tinyxml2::XMLElement* elem, IGenum attachmentType, IGsize tupleNum) {
    const char* data = elem->Attribute("Name");
    const std::string name = data != nullptr ? data : "Undefined Scalar";
    const char* type = elem->Attribute("type");
    const char* format = elem->Attribute("format");
    data = elem->Attribute("NumberOfComponents");
    const int components = data != nullptr ? std::max(1, std::atoi(data)) : 1;

    const bool ascii = (format == nullptr) || (std::strcmp(format, "ascii") == 0);

    // 该段没有元组时（例如无单元的网格同样带 <CellData> 节点）没有任何数据需要读取
    if (tupleNum == 0) { return true; }

    // 元组数 × 分量数 = 数组应有的值个数。先做溢出检查，避免 NumberOfComponents 异常时
    // 乘积回绕，把后面的长度校验绕过去。
    const IGsize maxSize = std::numeric_limits<IGsize>::max();
    if (static_cast<IGsize>(components) > maxSize / tupleNum) {
        IGAME_CORE_ERROR("[iGameVTIReader] Array '{}' declares too many components ({}).", name, components);
        return false;
    }
    const IGsize valueNum = tupleNum * static_cast<IGsize>(components);

    // 源元素宽度/符号性由 VTK 的 type 决定（Int8 = 1 字节/元素）；容器类型另按 type
    // 对应的精确数组类型选择，两者不能混为一谈。
    const IGsize srcSize = SizeOfVtkType(type);
    const bool srcUnsigned = IsUnsignedVtkType(type);

    if (valueNum > maxSize / srcSize) {
        IGAME_CORE_ERROR("[iGameVTIReader] Array '{}' is too large to decode.", name);
        return false;
    }
    const IGsize byteNum = valueNum * srcSize;

    ByteBuffer bytes;
    if (!ascii) {
        if (!DecodeDataArrayPayload(elem, bytes)) { return false; }
        // 解码出的字节数必须与 DataArray 声明的元组数完全一致：字节数偏少时数组不完整，
        // 若只告警继续，后续滤波会按点索引读到不存在的元素。
        if (bytes.size() != byteNum) {
            IGAME_CORE_ERROR("[iGameVTIReader] Array '{}' decoded {} byte(s), expected {}; the data is incomplete.",
                             name, bytes.size(), byteNum);
            return false;
        }
    }

    // 容器类型按 VTK type 选择（与 iGameVTUReader::ReadIntegerDataArray 一致）：
    // Int8 → CharArray、UInt8 → UnsignedCharArray、Int16 → ShortArray、UInt16 → UnsignedShortArray、
    // Int32 → IntArray、UInt32 → UnsignedIntArray、Int64 → LongLongArray、UInt64 → UnsignedLongLongArray。
    // 取值时统一按上面的 srcSize/srcUnsigned 逐个元素读取。
    ArrayObject::Pointer array;
    if (IsFloat64VtkType(type)) {
        DoubleArray::Pointer typed = DoubleArray::New();
        typed->SetDimension(components);
        ascii ? AppendAsciiToFlatArray<double>(elem->GetText(), valueNum, typed)
              : AppendBytesToFlatArray<double>(bytes, valueNum, typed);
        array = typed;
    } else if (IsFloatVtkType(type)) {
        FloatArray::Pointer typed = FloatArray::New();
        typed->SetDimension(components);
        ascii ? AppendAsciiToFlatArray<float>(elem->GetText(), valueNum, typed)
              : AppendBytesToFlatArray<float>(bytes, valueNum, typed);
        array = typed;
    } else if (srcSize == 1) {
        if (srcUnsigned) {
            UnsignedCharArray::Pointer typed = UnsignedCharArray::New();
            typed->SetDimension(components);
            ascii ? AppendAsciiIntegersToFlatArray<unsigned char>(elem->GetText(), valueNum, srcUnsigned, typed)
                  : AppendRawIntegers<unsigned char>(bytes, valueNum, srcSize, srcUnsigned, typed);
            array = typed;
        } else {
            CharArray::Pointer typed = CharArray::New();
            typed->SetDimension(components);
            ascii ? AppendAsciiIntegersToFlatArray<char>(elem->GetText(), valueNum, srcUnsigned, typed)
                  : AppendRawIntegers<char>(bytes, valueNum, srcSize, srcUnsigned, typed);
            array = typed;
        }
    } else if (srcSize == 2) {
        if (srcUnsigned) {
            UnsignedShortArray::Pointer typed = UnsignedShortArray::New();
            typed->SetDimension(components);
            ascii ? AppendAsciiIntegersToFlatArray<unsigned short>(elem->GetText(), valueNum, srcUnsigned, typed)
                  : AppendRawIntegers<unsigned short>(bytes, valueNum, srcSize, srcUnsigned, typed);
            array = typed;
        } else {
            ShortArray::Pointer typed = ShortArray::New();
            typed->SetDimension(components);
            ascii ? AppendAsciiIntegersToFlatArray<short>(elem->GetText(), valueNum, srcUnsigned, typed)
                  : AppendRawIntegers<short>(bytes, valueNum, srcSize, srcUnsigned, typed);
            array = typed;
        }
    } else if (srcSize == 4) {
        if (srcUnsigned) {
            UnsignedIntArray::Pointer typed = UnsignedIntArray::New();
            typed->SetDimension(components);
            ascii ? AppendAsciiIntegersToFlatArray<unsigned int>(elem->GetText(), valueNum, srcUnsigned, typed)
                  : AppendRawIntegers<unsigned int>(bytes, valueNum, srcSize, srcUnsigned, typed);
            array = typed;
        } else {
            IntArray::Pointer typed = IntArray::New();
            typed->SetDimension(components);
            ascii ? AppendAsciiIntegersToFlatArray<int>(elem->GetText(), valueNum, srcUnsigned, typed)
                  : AppendRawIntegers<int>(bytes, valueNum, srcSize, srcUnsigned, typed);
            array = typed;
        }
    } else {
        if (srcUnsigned) {
            UnsignedLongLongArray::Pointer typed = UnsignedLongLongArray::New();
            typed->SetDimension(components);
            ascii ? AppendAsciiIntegersToFlatArray<unsigned long long>(elem->GetText(), valueNum, srcUnsigned, typed)
                  : AppendRawIntegers<unsigned long long>(bytes, valueNum, srcSize, srcUnsigned, typed);
            array = typed;
        } else {
            LongLongArray::Pointer typed = LongLongArray::New();
            typed->SetDimension(components);
            ascii ? AppendAsciiIntegersToFlatArray<long long>(elem->GetText(), valueNum, srcUnsigned, typed)
                  : AppendRawIntegers<long long>(bytes, valueNum, srcSize, srcUnsigned, typed);
            array = typed;
        }
    }

    // 长度校验：值个数必须与元组数 × 分量数完全一致，不完整的数据一律拒绝，
    // 不允许“告警后仍然读取成功”。
    if (array == nullptr || array->GetNumberOfValues() != valueNum) {
        IGAME_CORE_ERROR("[iGameVTIReader] Array '{}' has {} value(s), expected {}; the data is incomplete.", name,
                         array != nullptr ? array->GetNumberOfValues() : 0, valueNum);
        return false;
    }

    array->SetName(name);
    // PointData/CellData 上的 Tensors/Vectors/Normals 决定属性类型，其余按标量处理
    IGenum attributeType = IG_SCALAR;
    if (ContainsName(m_TensorNames, name)) {
        attributeType = IG_TENSOR;
    } else if (ContainsName(m_VectorNames, name)) {
        attributeType = IG_VECTOR;
    } else if (ContainsName(m_NormalNames, name)) {
        attributeType = IG_NORMAL;
    }
    // 不传 dataRange：AttributeSet 内部按需懒计算（与 iGameVTPReader/iGameVTUReader 一致）
    m_Data.GetData()->AddAttribute(attributeType, attachmentType, array);
    return true;
}

bool iGameVTIReader::ReadAttributes(tinyxml2::XMLElement* section, IGenum attachmentType, IGsize tupleNum) {
    if (section == nullptr) { return true; }

    m_TensorNames.clear();
    m_VectorNames.clear();
    m_NormalNames.clear();
    SplitNameList(section->Attribute("Tensors"), m_TensorNames);
    SplitNameList(section->Attribute("Vectors"), m_VectorNames);
    SplitNameList(section->Attribute("Normals"), m_NormalNames);

    for (auto* elem = section->FirstChildElement("DataArray"); elem != nullptr;
         elem = elem->NextSiblingElement("DataArray")) {
        // 解码失败 / 长度不足必须向上传递：静默跳过会让“读取成功但缺数据”的文件混过去
        if (!ReadOneArray(elem, attachmentType, tupleNum)) { return false; }
    }
    return true;
}

bool iGameVTIReader::Parsing() {
    if (!ReadImageDataInfo()) { return false; }
    UpdateProgress(0.1);

    if (!BuildPoints()) {
        IGAME_CORE_ERROR("[iGameVTIReader] Failed to build implicit ImageData points.");
        return false;
    }
    UpdateProgress(0.3);

    auto* piece = FindTargetItem(root, "Piece");
    if (piece == nullptr) {
        IGAME_CORE_ERROR("[iGameVTIReader] Missing Piece node.");
        return false;
    }
    if (!ReadAttributes(FindTargetItem(piece, "PointData"), IG_POINT, m_PointNum)) { return false; }
    UpdateProgress(0.7);
    if (!ReadAttributes(FindTargetItem(piece, "CellData"), IG_CELL, m_CellNum)) { return false; }
    UpdateProgress(1.0);
    return true;
}

bool iGameVTIReader::CreateDataObject() {
    // ImageData 是规则网格：用 StructuredMesh 承载（隐含坐标已生成为 Points）
    m_Output = StructuredMesh::New();
    auto structuredMesh = DynamicCast<StructuredMesh>(m_Output);
    if (structuredMesh == nullptr) {
        IGAME_CORE_ERROR("[iGameVTIReader] Failed to create StructuredMesh.");
        return false;
    }

    structuredMesh->SetExtent(m_Extent);
    structuredMesh->SetDimensionSize(m_Data.dimensionSize);
    structuredMesh->SetPoints(m_Data.GetPoints());
    structuredMesh->SetAttributeSet(m_Data.GetData());
    structuredMesh->GenStructuredCellConnectivities();
    return true;
}

IGAME_NAMESPACE_END
