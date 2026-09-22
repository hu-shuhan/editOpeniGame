//
// Created for iGameVis: VTK XML RectilinearGrid (.vtr) reader.
//

#include "iGameVTRReader.h"
#include "iGameFileSystem.h"

#include <iGameStructuredMesh.h>
#include <tinyxml2.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>

IGAME_NAMESPACE_BEGIN

namespace
{
using ByteBuffer = iGame::vtkxml::ByteBuffer;

template <typename T>
T ReadLittleEndian(const unsigned char* p) {
    T value{};
    std::memcpy(&value, p, sizeof(T));
    return value;
}

const char* FormatOf(tinyxml2::XMLElement* elem) {
    const char* f = elem ? elem->Attribute("format") : nullptr;
    return (f != nullptr && *f != '\0') ? f : "ascii";
}

const char* TypeOf(tinyxml2::XMLElement* elem) {
    const char* t = elem ? elem->Attribute("type") : nullptr;
    return (t != nullptr && *t != '\0') ? t : "Float32";
}

int ComponentsOf(tinyxml2::XMLElement* elem) {
    const char* c = elem ? elem->Attribute("NumberOfComponents") : nullptr;
    if (c == nullptr || *c == '\0') { return 1; }
    const int n = std::atoi(c);
    return n > 0 ? n : 1;
}

// ASCII -> DoubleArray（坐标），count < 0 表示不限数量。
void AppendAsciiToDoubleArray(const char* text, iGame::DoubleArray::Pointer arr, int count) {
    if (text == nullptr || arr == nullptr) { return; }
    std::istringstream iss(text);
    double v = 0.0;
    int n = 0;
    while ((count < 0 || n < count) && iss >> v) {
        arr->AddValue(v);
        ++n;
    }
}

// ASCII -> FloatArray（属性）。
void AppendAsciiToFloatArray(const char* text, iGame::FloatArray::Pointer arr) {
    if (text == nullptr || arr == nullptr) { return; }
    std::istringstream iss(text);
    double v = 0.0;
    while (iss >> v) { arr->AddValue(static_cast<float>(v)); }
}

// 二进制字节 -> DoubleArray（坐标），T 为源类型。
template <typename T>
void AppendBytesToDoubleArray(const ByteBuffer& bytes, iGame::DoubleArray::Pointer arr) {
    if (arr == nullptr || bytes.empty()) { return; }
    const std::size_t count = bytes.size() / sizeof(T);
    for (std::size_t i = 0; i < count; ++i) {
        arr->AddValue(static_cast<double>(ReadLittleEndian<T>(bytes.data() + i * sizeof(T))));
    }
}

// 二进制字节 -> FloatArray（属性），T 为源类型。
template <typename T>
void AppendBytesToFloatArray(const ByteBuffer& bytes, iGame::FloatArray::Pointer arr) {
    if (arr == nullptr || bytes.empty()) { return; }
    const std::size_t count = bytes.size() / sizeof(T);
    for (std::size_t i = 0; i < count; ++i) {
        arr->AddValue(static_cast<float>(ReadLittleEndian<T>(bytes.data() + i * sizeof(T))));
    }
}
} // namespace

bool iGameVTRReader::DecodeDataArrayPayload(tinyxml2::XMLElement* element, vtkxml::ByteBuffer& output) {
    const char* format = element ? element->Attribute("format") : nullptr;
    const char* appended = nullptr;
    if (format != nullptr && std::strcmp(format, "appended") == 0) {
        appended = GetAppendDataHead();
    }

    vtkxml::DataArrayDecodeContext context;
    context.root = root;
    context.appendedData = appended;
    context.sourceData = m_MemoryBuffer;
    context.sourceSize = m_MemoryBufferSize;

    // raw appended 需要整份源文件字节；文件模式下按需读入一次。
    if (format != nullptr && std::strcmp(format, "appended") == 0 && m_parseRawBinaryData &&
        (context.sourceData == nullptr || context.sourceSize == 0)) {
        if (m_DataArraySourceBuffer.empty()) {
            std::ifstream stream(FileSystem::PathFromUtf8(m_FilePath), std::ios::binary | std::ios::ate);
            if (!stream) {
                SetDataArrayDecodeError("cannot open the source file for raw AppendedData");
                return false;
            }
            const std::streamsize size = stream.tellg();
            if (size <= 0) {
                SetDataArrayDecodeError("cannot determine the raw AppendedData source size");
                return false;
            }
            m_DataArraySourceBuffer.resize(static_cast<std::size_t>(size));
            stream.seekg(0, std::ios::beg);
            if (!stream.read(m_DataArraySourceBuffer.data(), size)) {
                m_DataArraySourceBuffer.clear();
                SetDataArrayDecodeError("cannot read the source file for raw AppendedData");
                return false;
            }
        }
        context.sourceData = m_DataArraySourceBuffer.data();
        context.sourceSize = m_DataArraySourceBuffer.size();
    }

    std::string error;
    if (!vtkxml::DecodeDataArray(element, context, output, error)) {
        const char* name = element ? element->Attribute("Name") : nullptr;
        SetDataArrayDecodeError(std::string("DataArray '") + (name ? name : "<unnamed>") + "': " + error);
        return false;
    }
    return true;
}

const char* iGameVTRReader::GetAppendDataHead() {
    static char empty = '\0';
    if (m_AppendedDataHead == nullptr) {
        tinyxml2::XMLElement* elem = FindTargetItem(root, "AppendedData");
        if (elem == nullptr) { return &empty; }
        const char* encoding = elem->Attribute("encoding");
        m_parseRawBinaryData = encoding != nullptr && std::strcmp(encoding, "raw") == 0;
        m_AppendedDataHead = const_cast<char*>(elem->GetText());
        if (m_AppendedDataHead == nullptr) { return &empty; }
        while (*m_AppendedDataHead == '\n' || *m_AppendedDataHead == '\r' ||
               *m_AppendedDataHead == ' ' || *m_AppendedDataHead == '\t') {
            ++m_AppendedDataHead;
        }
        if (*m_AppendedDataHead == '_') { ++m_AppendedDataHead; }
    }
    return m_AppendedDataHead;
}

void iGameVTRReader::SetDataArrayDecodeError(const std::string& message) {
    if (!m_DataArrayDecodeFailed) {
        std::fprintf(stderr, "[iGameVTRReader] data decode failed: %s\n", message.c_str());
    }
    m_DataArrayDecodeFailed = true;
    m_DataArrayDecodeError = message;
}

DoubleArray::Pointer iGameVTRReader::ParseCoordinateArray(tinyxml2::XMLElement* elem, int count) {
    DoubleArray::Pointer out = DoubleArray::New();
    out->SetDimension(1);
    if (elem == nullptr) { return out; }

    const char* format = FormatOf(elem);
    const char* type = TypeOf(elem);
    if (std::strcmp(format, "ascii") == 0) {
        AppendAsciiToDoubleArray(elem->GetText(), out, count);
        return out;
    }

    ByteBuffer bytes;
    if (!DecodeDataArrayPayload(elem, bytes)) { return out; }
    if (std::strcmp(type, "Float32") == 0) {
        AppendBytesToDoubleArray<float>(bytes, out);
    } else {
        // 默认按 Float64 处理（VTK 坐标仅使用 Float32/Float64）。
        AppendBytesToDoubleArray<double>(bytes, out);
    }
    return out;
}

void iGameVTRReader::AddDataArray(tinyxml2::XMLElement* elem, IGenum attachmentType) {
    if (elem == nullptr) { return; }

    const std::string name = elem->Attribute("Name") ? elem->Attribute("Name") : "";
    const int components = ComponentsOf(elem);
    const char* format = FormatOf(elem);
    const char* type = TypeOf(elem);

    FloatArray::Pointer arr = FloatArray::New();
    arr->SetDimension(components);

    if (std::strcmp(format, "ascii") == 0) {
        AppendAsciiToFloatArray(elem->GetText(), arr);
    } else {
        ByteBuffer bytes;
        if (!DecodeDataArrayPayload(elem, bytes)) { return; }
        if (std::strcmp(type, "Float32") == 0) {
            AppendBytesToFloatArray<float>(bytes, arr);
        } else if (std::strcmp(type, "Float64") == 0) {
            AppendBytesToFloatArray<double>(bytes, arr);
        } else {
            // 仅支持浮点场数据（与原读取器行为一致），未知类型跳过以避免误读。
            std::fprintf(stderr, "[iGameVTRReader] unsupported field type '%s' for array '%s'\n",
                         type, name.c_str());
            return;
        }
    }

    if (arr->GetNumberOfValues() <= 0) { return; }
    arr->SetName(name);
    if (arr->GetDimension() > 1) {
        m_Data.GetData()->AddVector(attachmentType, arr);
    } else {
        m_Data.GetData()->AddScalar(attachmentType, arr);
    }
}

bool iGameVTRReader::Parsing() {
    m_AppendedDataHead = nullptr;
    m_parseRawBinaryData = false;
    m_DataArrayDecodeFailed = false;
    m_DataArrayDecodeError.clear();
    m_DataArraySourceBuffer.clear();

    // 1. RectilinearGrid -> WholeExtent -> dims
    tinyxml2::XMLElement* grid = FindTargetItem(root, "RectilinearGrid");
    if (!grid) {
        std::fprintf(stderr, "Could not load Vtr file. Error='No RectilinearGrid Attribute'.\n");
        return false;
    }
    int extent[6] = {0, 0, 0, 0, 0, 0};
    {
        std::istringstream iss(grid->Attribute("WholeExtent") ? grid->Attribute("WholeExtent") : "");
        for (int i = 0; i < 6; ++i) { iss >> extent[i]; }
    }
    const int nx = extent[1] - extent[0] + 1;
    const int ny = extent[3] - extent[2] + 1;
    const int nz = extent[5] - extent[4] + 1;
    if (nx < 1 || ny < 1 || nz < 1) {
        std::fprintf(stderr, "Could not load Vtr file. Error='Invalid WholeExtent'.\n");
        return false;
    }
    m_Data.dimensionSize[0] = nx;
    m_Data.dimensionSize[1] = ny;
    m_Data.dimensionSize[2] = nz;

    // 2. Coordinates -> X/Y/Z
    tinyxml2::XMLElement* coordsElem = FindTargetItem(root, "Coordinates");
    if (!coordsElem) {
        std::fprintf(stderr, "Could not load Vtr file. Error='No Coordinates Attribute'.\n");
        return false;
    }

    DoubleArray::Pointer coords[3] = {nullptr, nullptr, nullptr};
    const int axisCount[3] = {nx, ny, nz};
    int axis = 0;
    for (tinyxml2::XMLElement* c = coordsElem->FirstChildElement("DataArray");
         c != nullptr && axis < 3; c = c->NextSiblingElement("DataArray"), ++axis) {
        coords[axis] = ParseCoordinateArray(c, axisCount[axis]);
        if (m_DataArrayDecodeFailed) { return false; }
    }

    if (coords[0] == nullptr || coords[1] == nullptr || coords[2] == nullptr ||
        coords[0]->GetNumberOfValues() < static_cast<IGsize>(nx) ||
        coords[1]->GetNumberOfValues() < static_cast<IGsize>(ny) ||
        coords[2]->GetNumberOfValues() < static_cast<IGsize>(nz)) {
        std::fprintf(stderr, "Could not load Vtr file. Error='Invalid Coordinates'.\n");
        return false;
    }

    // 3. 展开顶点（i + j*nx + k*nx*ny）
    Points::Pointer points = m_Data.GetPoints();
    for (int k = 0; k < nz; ++k) {
        for (int j = 0; j < ny; ++j) {
            for (int i = 0; i < nx; ++i) {
                points->AddPoint(static_cast<float>(coords[0]->GetValue(i)),
                                 static_cast<float>(coords[1]->GetValue(j)),
                                 static_cast<float>(coords[2]->GetValue(k)));
            }
        }
    }

    // 4. PointData（顶点数据，原样）
    tinyxml2::XMLElement* pd = FindTargetItem(root, "PointData");
    if (pd) {
        for (tinyxml2::XMLElement* da = pd->FirstChildElement("DataArray");
             da != nullptr; da = da->NextSiblingElement("DataArray")) {
            AddDataArray(da, IG_POINT);
        }
    }

    // 5. CellData（单元数据，原样保留为单元属性；体渲染时再转点数据）
    tinyxml2::XMLElement* cd = FindTargetItem(root, "CellData");
    if (cd) {
        for (tinyxml2::XMLElement* da = cd->FirstChildElement("DataArray");
             da != nullptr; da = da->NextSiblingElement("DataArray")) {
            AddDataArray(da, IG_CELL);
        }
    }

    return !m_DataArrayDecodeFailed;
}

bool iGameVTRReader::CreateDataObject() {
    m_Output = iGame::StructuredMesh::New();
    auto mesh = DynamicCast<iGame::StructuredMesh>(m_Output);
    mesh->SetDimensionSize(m_Data.dimensionSize);
    mesh->SetPoints(m_Data.GetPoints());
    mesh->SetAttributeSet(m_Data.GetData());
    mesh->GenStructuredCellConnectivities();
    return true;
}

IGAME_NAMESPACE_END
