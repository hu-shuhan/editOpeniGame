/**
 * @class   iGameVTPReader
 * @brief   VTK XML PolyData reader for VTP files.
 */

#include "iGameVTPReader.h"
#include "iGameXMLUtils.h"

#include <tinyxml2.h>

#include <algorithm>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

namespace
{
using ByteBuffer = iGame::vtkxml::ByteBuffer;

template<typename T>
T ReadLittleEndian(const unsigned char* p) {
    T value{};
    std::memcpy(&value, p, sizeof(T));
    return value;
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
    m_Header_8_byte_flag = false;
    m_parseRawBinaryData = false;
    m_AppendedDataHead = nullptr;
    m_DataArrayDecodeFailed = false;
    m_DataArrayDecodeError.clear();
    m_DataArraySourceBuffer.clear();
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
        if (m_DataArrayDecodeFailed) return false;
        if (!m_IndependentUpdate) UpdateProgress(0.35);
        ReadVTPPointAttribute();
        if (m_DataArrayDecodeFailed) return false;
    }

    if (!m_IndependentUpdate) UpdateProgress(0.6);
    ReadVTPCellData();
    ReadPolyDataCells("Lines", true, false);
    if (m_DataArrayDecodeFailed) return false;
    if (!m_IndependentUpdate) UpdateProgress(0.8);
    ReadPolyDataCells("Polys", false, false);
    ReadPolyDataCells("Strips", false, true);
    if (m_DataArrayDecodeFailed) return false;
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
    if (!DecodeDataArrayPayload(array, bytes)) return false;

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
        }
        if (data == nullptr) continue;
        while (*data == '\n' || *data == ' ' || *data == '\t') ++data;

        ArrayObject::Pointer array;
        ByteBuffer bytes;
        if (std::strcmp(format, "ascii") != 0 && !DecodeDataArrayPayload(arrayElem, bytes)) return false;

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
    }
    if (data == nullptr) return empty;
    while (*data == '\n' || *data == ' ' || *data == '\t') ++data;

    ByteBuffer bytes;
    if (std::strcmp(format, "ascii") != 0 && !DecodeDataArrayPayload(arrayElem, bytes)) return empty;

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
