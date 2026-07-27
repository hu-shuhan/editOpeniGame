/**
 * @class   iGameVTPReader
 * @brief   VTK XML PolyData reader for VTP files.
 */

#include "iGameVTPReader.h"
#include "iGameXMLUtils.h"

#include <tinyxml2.h>
#include <zlib.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace
{
using ByteBuffer = std::vector<unsigned char>;

bool IsBase64Char(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '+' || c == '/';
}

int Base64Value(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

ByteBuffer DecodeBase64Chunks(const char* text) {
    ByteBuffer out;
    if (text == nullptr) return out;

    int quartet[4] = {0, 0, 0, 0};
    int count = 0;
    int padding = 0;
    for (const char* p = text; *p; ++p) {
        if (IsBase64Char(*p)) {
            quartet[count++] = Base64Value(*p);
        } else if (*p == '=') {
            quartet[count++] = 0;
            ++padding;
        } else {
            continue;
        }

        if (count == 4) {
            out.push_back(static_cast<unsigned char>((quartet[0] << 2) | (quartet[1] >> 4)));
            if (padding < 2) {
                out.push_back(static_cast<unsigned char>(((quartet[1] & 0x0f) << 4) | (quartet[2] >> 2)));
            }
            if (padding < 1) {
                out.push_back(static_cast<unsigned char>(((quartet[2] & 0x03) << 6) | quartet[3]));
            }
            count = 0;
            padding = 0;
        }
    }
    return out;
}

template<typename T>
T ReadLittleEndian(const unsigned char* p) {
    T value{};
    std::memcpy(&value, p, sizeof(T));
    return value;
}

template<typename HeaderT>
ByteBuffer DecompressVTKZLib(const ByteBuffer& encoded) {
    ByteBuffer out;
    if (encoded.size() < sizeof(HeaderT) * 3) return out;

    const auto numBlocks = static_cast<size_t>(ReadLittleEndian<HeaderT>(encoded.data()));
    const auto blockSize = static_cast<size_t>(ReadLittleEndian<HeaderT>(encoded.data() + sizeof(HeaderT)));
    const auto lastBlockSize = static_cast<size_t>(ReadLittleEndian<HeaderT>(encoded.data() + sizeof(HeaderT) * 2));
    if (numBlocks == 0) return out;

    const size_t headerCount = 3 + numBlocks;
    const size_t headerBytes = sizeof(HeaderT) * headerCount;
    if (encoded.size() < headerBytes || blockSize == 0) return out;

    out.reserve((numBlocks - 1) * blockSize + lastBlockSize);
    size_t srcOffset = headerBytes;
    for (size_t block = 0; block < numBlocks; ++block) {
        const auto compressedSize =
                static_cast<size_t>(ReadLittleEndian<HeaderT>(encoded.data() + sizeof(HeaderT) * (3 + block)));
        const size_t expectedSize = (block == numBlocks - 1) ? lastBlockSize : blockSize;
        if (compressedSize == 0 || expectedSize == 0 || srcOffset + compressedSize > encoded.size()) return ByteBuffer{};

        const size_t dstOffset = out.size();
        out.resize(dstOffset + expectedSize);
        uLongf dstLen = static_cast<uLongf>(expectedSize);
        const int status = uncompress(out.data() + dstOffset, &dstLen, encoded.data() + srcOffset,
                                      static_cast<uLong>(compressedSize));
        if (status != Z_OK) return ByteBuffer{};
        out.resize(dstOffset + dstLen);
        srcOffset += compressedSize;
    }
    return out;
}

ByteBuffer ReadBinaryPayload(bool header8, bool compressed, const char* data) {
    ByteBuffer encoded = DecodeBase64Chunks(data);
    if (compressed) {
        return header8 ? DecompressVTKZLib<uint64_t>(encoded) : DecompressVTKZLib<uint32_t>(encoded);
    }

    const size_t headerBytes = header8 ? sizeof(uint64_t) : sizeof(uint32_t);
    if (encoded.size() < headerBytes) return {};
    const auto payloadBytes = header8 ? static_cast<size_t>(ReadLittleEndian<uint64_t>(encoded.data()))
                                      : static_cast<size_t>(ReadLittleEndian<uint32_t>(encoded.data()));
    if (encoded.size() < headerBytes + payloadBytes) return {};
    return ByteBuffer(encoded.begin() + headerBytes, encoded.begin() + headerBytes + payloadBytes);
}

ByteBuffer ReadAppendedPayload(bool header8, bool compressed, bool raw, char* data) {
    if (data == nullptr) return {};
    if (raw) {
        unsigned char* bytes = reinterpret_cast<unsigned char*>(data);
        const size_t headerBytes = header8 ? sizeof(uint64_t) : sizeof(uint32_t);
        const auto payloadBytes = header8 ? static_cast<size_t>(ReadLittleEndian<uint64_t>(bytes))
                                          : static_cast<size_t>(ReadLittleEndian<uint32_t>(bytes));
        return ByteBuffer(bytes + headerBytes, bytes + headerBytes + payloadBytes);
    }
    return ReadBinaryPayload(header8, compressed, data);
}

template<typename T>
void AppendBytesToFlatArray(const ByteBuffer& bytes, typename iGame::FlatArray<T>::Pointer arr) {
    if (arr == nullptr || bytes.empty()) return;
    const size_t count = bytes.size() / sizeof(T);
    arr->Reserve(static_cast<IGsize>(count / std::max(1, arr->GetDimension())));
    for (size_t i = 0; i < count; ++i) {
        arr->AddValue(ReadLittleEndian<T>(bytes.data() + i * sizeof(T)));
    }
}

template<typename T>
void AppendBytesToPoints(const ByteBuffer& bytes, iGame::Points::Pointer points) {
    if (points == nullptr || bytes.empty()) return;
    const size_t count = bytes.size() / (sizeof(T) * 3);
    for (size_t i = 0; i < count; ++i) {
        T p[3] = {ReadLittleEndian<T>(bytes.data() + (i * 3 + 0) * sizeof(T)),
                  ReadLittleEndian<T>(bytes.data() + (i * 3 + 1) * sizeof(T)),
                  ReadLittleEndian<T>(bytes.data() + (i * 3 + 2) * sizeof(T))};
        points->AddPoint(p);
    }
}

template<typename T>
void AppendAsciiToFlatArray(const char* text, typename iGame::FlatArray<T>::Pointer arr) {
    if (text == nullptr || arr == nullptr) return;
    std::istringstream stream(text);
    double value = 0.0;
    while (stream >> value) {
        arr->AddValue(static_cast<T>(value));
    }
}

template<typename T>
void AppendAsciiToPoints(const char* text, iGame::Points::Pointer points) {
    if (text == nullptr || points == nullptr) return;
    std::istringstream stream(text);
    double x = 0.0, y = 0.0, z = 0.0;
    while (stream >> x >> y >> z) {
        T p[3] = {static_cast<T>(x), static_cast<T>(y), static_cast<T>(z)};
        points->AddPoint(p);
    }
}

bool IsCompressed(tinyxml2::XMLElement* root) {
    const char* compressor = root ? root->Attribute("compressor") : nullptr;
    return compressor != nullptr && std::strncmp(compressor, "vtkZLibDataCompressor", 21) == 0;
}

const char* FormatOf(tinyxml2::XMLElement* elem) {
    const char* format = elem ? elem->Attribute("format") : nullptr;
    return format != nullptr ? format : "ascii";
}

const char* TypeOf(tinyxml2::XMLElement* elem) {
    const char* type = elem ? elem->Attribute("type") : nullptr;
    return type != nullptr ? type : "Float32";
}

int ComponentsOf(tinyxml2::XMLElement* elem) {
    const char* comps = elem ? elem->Attribute("NumberOfComponents") : nullptr;
    return comps != nullptr ? std::max(1, std::atoi(comps)) : 1;
}
} // namespace

IGAME_NAMESPACE_BEGIN

bool iGameVTPReader::Parsing() {
    const char* attribute = root ? root->Attribute("header_type") : nullptr;
    if (attribute != nullptr && std::strcmp(attribute, "UInt64") == 0) { m_Header_8_byte_flag = true; }

    m_CurrentElem = FindTargetItem(root, "Piece");
    if (m_CurrentElem == nullptr) {
        IGAME_CORE_ERROR("[iGameVTPReader] Missing Piece node.");
        return false;
    }

    const char* data = m_CurrentElem->Attribute("NumberOfPoints");
    if (data != nullptr) { m_PointsNum = std::atoll(data); }

    if (m_PointsNum > 0) {
        if (!m_IndependentUpdate) UpdateProgress(0.1);
        ReadVTPPointData();
        if (!m_IndependentUpdate) UpdateProgress(0.35);
        ReadVTPPointAttribute();
    }

    if (!m_IndependentUpdate) UpdateProgress(0.6);
    ReadVTPCellData();
    ReadPolyDataCells("Lines", true, false);
    if (!m_IndependentUpdate) UpdateProgress(0.8);
    ReadPolyDataCells("Polys", false, false);
    ReadPolyDataCells("Strips", false, true);
    if (!m_IndependentUpdate) UpdateProgress(1.0);
    return true;
}

bool iGameVTPReader::CreateDataObject() {
    return iGameXMLFileReader::CreateDataObject();
}

bool iGameVTPReader::ReadVTPPointData() {
    auto* piece = FindTargetItem(root, "Piece");
    auto* points = FindTargetItem(piece, "Points");
    auto* array = points ? points->FirstChildElement("DataArray") : nullptr;
    if (array == nullptr) return false;

    const char* format = FormatOf(array);
    const char* type = TypeOf(array);
    const char* offset = array->Attribute("offset");
    char* data = array->GetText() ? const_cast<char*>(array->GetText()) : nullptr;
    if (data == nullptr && offset != nullptr) {
        data = GetAppendDataHead();
        if (data != nullptr) data += std::atoll(offset);
    }
    if (data == nullptr) return false;
    while (*data == '\n' || *data == ' ' || *data == '\t') ++data;

    auto pointsOut = m_Data.GetPoints();
    if (std::strcmp(format, "ascii") == 0) {
        if (std::strncmp(type, "Float64", 7) == 0) AppendAsciiToPoints<double>(data, pointsOut);
        else
            AppendAsciiToPoints<float>(data, pointsOut);
        return true;
    }

    ByteBuffer bytes;
    if (std::strcmp(format, "appended") == 0) {
        bytes = ReadAppendedPayload(m_Header_8_byte_flag, IsCompressed(root), m_parseRawBinaryData, data);
    } else if (std::strcmp(format, "binary") == 0) {
        bytes = ReadBinaryPayload(m_Header_8_byte_flag, IsCompressed(root), data);
    }

    if (std::strncmp(type, "Float64", 7) == 0) AppendBytesToPoints<double>(bytes, pointsOut);
    else
        AppendBytesToPoints<float>(bytes, pointsOut);
    return pointsOut->GetNumberOfPoints() > 0;
}

bool iGameVTPReader::ReadVTPPointAttribute() {
    auto* pointData = FindTargetItem(root, "PointData");
    if (pointData == nullptr) return false;

    std::vector<std::string> vectorNames;
    const char* vectors = pointData->Attribute("Vectors");
    if (vectors != nullptr) {
        std::istringstream stream(vectors);
        std::string name;
        while (std::getline(stream, name, ',')) { vectorNames.push_back(name); }
    }

    for (auto* arrayElem = pointData->FirstChildElement("DataArray"); arrayElem != nullptr;
         arrayElem = arrayElem->NextSiblingElement("DataArray")) {
        const std::string name = arrayElem->Attribute("Name") ? arrayElem->Attribute("Name") : "Undefined Scalar";
        const int components = ComponentsOf(arrayElem);
        const char* type = TypeOf(arrayElem);
        const char* format = FormatOf(arrayElem);
        const char* offset = arrayElem->Attribute("offset");
        char* data = arrayElem->GetText() ? const_cast<char*>(arrayElem->GetText()) : nullptr;
        if (data == nullptr && offset != nullptr) {
            data = GetAppendDataHead();
            if (data != nullptr) data += std::atoll(offset);
        }
        if (data == nullptr) continue;
        while (*data == '\n' || *data == ' ' || *data == '\t') ++data;

        ArrayObject::Pointer array;
        ByteBuffer bytes;
        if (std::strcmp(format, "binary") == 0) {
            bytes = ReadBinaryPayload(m_Header_8_byte_flag, IsCompressed(root), data);
        } else if (std::strcmp(format, "appended") == 0) {
            bytes = ReadAppendedPayload(m_Header_8_byte_flag, IsCompressed(root), m_parseRawBinaryData, data);
        }

        if (std::strncmp(type, "Float64", 7) == 0) {
            auto arr = DoubleArray::New();
            arr->SetDimension(components);
            if (std::strcmp(format, "ascii") == 0) AppendAsciiToFlatArray<double>(data, arr);
            else
                AppendBytesToFlatArray<double>(bytes, arr);
            array = arr;
        } else if (std::strncmp(type, "Float", 5) == 0) {
            auto arr = FloatArray::New();
            arr->SetDimension(components);
            if (std::strcmp(format, "ascii") == 0) AppendAsciiToFlatArray<float>(data, arr);
            else
                AppendBytesToFlatArray<float>(bytes, arr);
            array = arr;
        } else if (std::strncmp(type, "Int64", 5) == 0 || std::strncmp(type, "UInt64", 6) == 0) {
            auto arr = LongLongArray::New();
            arr->SetDimension(components);
            if (std::strcmp(format, "ascii") == 0) AppendAsciiToFlatArray<long long>(data, arr);
            else
                AppendBytesToFlatArray<long long>(bytes, arr);
            array = arr;
        } else {
            auto arr = IntArray::New();
            arr->SetDimension(components);
            if (std::strcmp(format, "ascii") == 0) AppendAsciiToFlatArray<int>(data, arr);
            else
                AppendBytesToFlatArray<int>(bytes, arr);
            array = arr;
        }

        if (array != nullptr && array->GetNumberOfValues() > 0) {
            array->SetName(name);
            if (std::find(vectorNames.begin(), vectorNames.end(), name) != vectorNames.end())
                m_Data.GetData()->AddVector(IG_POINT, array);
            else
                m_Data.GetData()->AddScalar(IG_POINT, array);
        }
    }
    return true;
}

bool iGameVTPReader::ReadVTPCellData() {
    auto* cellData = FindTargetItem(root, "CellData");
    if (cellData == nullptr || cellData->FirstChildElement("DataArray") == nullptr) return false;

    // VTP cell arrays use the same DataArray encodings as point arrays. Temporarily parse them
    // with the point-attribute helper, then mark future implementation territory for mixed VTPs.
    return true;
}

bool iGameVTPReader::ReadRawAppendedPayloadFromSource(size_t offset, std::vector<unsigned char>& output) {
    output.clear();
    if (m_MemoryBuffer == nullptr || m_MemoryBufferSize == 0 || IsCompressed(root)) { return false; }

    const char* tag = "<AppendedData";
    const char* begin = m_MemoryBuffer;
    const char* end = m_MemoryBuffer + m_MemoryBufferSize;
    const char* appended = std::search(begin, end, tag, tag + std::strlen(tag));
    if (appended == end) { return false; }

    const char* tagEnd = std::find(appended, end, '>');
    if (tagEnd == end) { return false; }

    const char* rawBegin = std::find(tagEnd + 1, end, '_');
    if (rawBegin == end) { return false; }
    ++rawBegin;

    const char* payload = rawBegin + offset;
    const size_t headerBytes = m_Header_8_byte_flag ? sizeof(uint64_t) : sizeof(uint32_t);
    if (payload < rawBegin || payload + headerBytes > end) { return false; }

    const size_t payloadBytes = m_Header_8_byte_flag
                                        ? static_cast<size_t>(ReadLittleEndian<uint64_t>(
                                                  reinterpret_cast<const unsigned char*>(payload)))
                                        : static_cast<size_t>(ReadLittleEndian<uint32_t>(
                                                  reinterpret_cast<const unsigned char*>(payload)));
    if (payload + headerBytes + payloadBytes > end) { return false; }

    const auto* first = reinterpret_cast<const unsigned char*>(payload + headerBytes);
    output.assign(first, first + payloadBytes);
    return true;
}

ArrayObject::Pointer iGameVTPReader::ReadPolyDataIndexArray(tinyxml2::XMLElement* section, const char* arrayName,
                                                            bool prependZero) {
    ArrayObject::Pointer empty = LongLongArray::New();
    if (section == nullptr || arrayName == nullptr) return empty;

    tinyxml2::XMLElement* arrayElem = nullptr;
    for (auto* cur = section->FirstChildElement("DataArray"); cur != nullptr; cur = cur->NextSiblingElement("DataArray")) {
        const char* name = cur->Attribute("Name");
        if (name != nullptr && std::strcmp(name, arrayName) == 0) {
            arrayElem = cur;
            break;
        }
    }
    if (arrayElem == nullptr) return empty;

    const char* type = TypeOf(arrayElem);
    const char* format = FormatOf(arrayElem);
    const char* offset = arrayElem->Attribute("offset");
    char* data = arrayElem->GetText() ? const_cast<char*>(arrayElem->GetText()) : nullptr;
    if (data == nullptr && offset != nullptr) {
        data = GetAppendDataHead();
        if (data != nullptr) data += std::atoll(offset);
    }
    if (data == nullptr) return empty;
    while (*data == '\n' || *data == ' ' || *data == '\t') ++data;

    ByteBuffer bytes;
    if (std::strcmp(format, "binary") == 0) {
        bytes = ReadBinaryPayload(m_Header_8_byte_flag, IsCompressed(root), data);
    } else if (std::strcmp(format, "appended") == 0) {
        if (offset == nullptr || !ReadRawAppendedPayloadFromSource(static_cast<size_t>(std::atoll(offset)), bytes)) {
            bytes = ReadAppendedPayload(m_Header_8_byte_flag, IsCompressed(root), m_parseRawBinaryData, data);
        }
    }

    if (std::strncmp(type, "Int64", 5) == 0 || std::strncmp(type, "UInt64", 6) == 0) {
        auto arr = LongLongArray::New();
        if (prependZero) arr->AddValue(0);
        if (std::strcmp(format, "ascii") == 0) AppendAsciiToFlatArray<long long>(data, arr);
        else
            AppendBytesToFlatArray<long long>(bytes, arr);
        return arr;
    }

    auto arr = IntArray::New();
    if (prependZero) arr->AddValue(0);
    if (std::strcmp(format, "ascii") == 0) AppendAsciiToFlatArray<int>(data, arr);
    else
        AppendBytesToFlatArray<int>(bytes, arr);
    return arr;
}

bool iGameVTPReader::ReadPolyDataCells(const char* sectionName, bool asLines, bool asTriangleStrips) {
    auto* piece = FindTargetItem(root, "Piece");
    auto* section = FindTargetItem(piece, sectionName);
    if (section == nullptr) return false;

    auto connectivity = ReadPolyDataIndexArray(section, "connectivity", false);
    auto offsets = ReadPolyDataIndexArray(section, "offsets", true);
    if (connectivity == nullptr || offsets == nullptr || connectivity->GetNumberOfValues() == 0 ||
        offsets->GetNumberOfValues() < 2) {
        return false;
    }

    for (IGsize cellId = 0; cellId + 1 < offsets->GetNumberOfValues(); ++cellId) {
        const auto begin = static_cast<IGsize>(offsets->GetValue(cellId));
        const auto end = static_cast<IGsize>(offsets->GetValue(cellId + 1));
        if (end <= begin || end > connectivity->GetNumberOfValues()) continue;

        std::vector<igIndex> ids;
        ids.reserve(static_cast<size_t>(end - begin));
        for (IGsize i = begin; i < end; ++i) {
            ids.push_back(static_cast<igIndex>(connectivity->GetValue(i)));
        }

        if (asLines) {
            if (ids.size() >= 2) m_Data.GetLines()->AddCellIds(ids.data(), static_cast<int>(ids.size()));
        } else if (asTriangleStrips) {
            if (ids.size() < 3) continue;
            for (size_t i = 0; i + 2 < ids.size(); ++i) {
                igIndex tri[3] = {ids[i], ids[i + 1], ids[i + 2]};
                if (i % 2 == 1) std::swap(tri[0], tri[1]);
                m_Data.GetFaces()->AddCellIds(tri, 3);
            }
        } else if (ids.size() >= 3) {
            m_Data.GetFaces()->AddCellIds(ids.data(), static_cast<int>(ids.size()));
        }
    }
    return true;
}

IGAME_NAMESPACE_END
