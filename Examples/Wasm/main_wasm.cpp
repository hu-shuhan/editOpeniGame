#include "Clip/iGameClipFilter.h"
#include "iGameDrawObject.h"
#include "iGameFileIO.h"
#include "iGameInteractor.h"
#include "iGamePainter3D.h"
#include "iGamePointSet.h"
#include "iGameRenderWindow.h"
#include "iGameScene.h"
#include "iGameSelection.h"
#include "iGameSelectionParameter.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"
#include "Slice/iGameSliceFilter.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <emscripten/bind.h>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <zlib.h>

using namespace emscripten;

namespace
{
iGame::Scene::Pointer g_scene;
iGame::RenderWindow::Pointer g_window;
iGame::Interactor::Pointer g_interactor;
IGuint g_activeModelId = 0;
bool g_debugEnabled = true;
int g_selectionMode = 0;
bool g_slicingActive = false;
int g_sliceSourceModelId = 0;
int g_sliceResultModelId = 0;
int g_sliceOperationMode = 0; // 0 = slice, 1 = clip
bool g_sliceCrinkle = false;
bool g_sliceInvert = true;
iGame::ClipSelection::Pointer g_clipSelection;

struct WebErrorState {
    int code = 0;
    std::string func;
    std::string detail;
    long long timestamp = 0;
};

WebErrorState g_lastError;

struct WebModelMeta {
    IGuint id = 0;
    std::string name;
    bool visible = true;
    std::string sourceType;
    long long loadTime = 0;
    bool solidEnabled = false;
    igm::vec3 solidColor{0.85f, 0.85f, 0.85f};
};

std::map<IGuint, WebModelMeta> g_modelRegistry;

// Guards for JS->WASM boundary to avoid single-shot huge allocations and 32-bit typed_memory_view limits
constexpr size_t MAX_SAFE_INPUT_LENGTH = (1ull << 30); // max elements for array-like reads (about 1G)
constexpr size_t MAX_SAFE_INPUT_BYTES = (1ull << 30);  // max bytes for single buffer upload (about 1GiB)
// When performing chunked copies, use a chunk small enough to fit into 32-bit typed_memory_view.
constexpr size_t CHUNK_COPY_SIZE = 16 * 1024 * 1024; // 16MiB

std::string EscapeJsonString(const std::string& in);
long long GetUnixTimestampSeconds();
void ForEachDrawObjectInTree(const iGame::DataObject::Pointer& root,
                             const std::function<void(iGame::DrawObject::Pointer)>& fn);
bool RebindCurrentSelectionMode(const char* funcName);
void ExitSlicingModeInternal(bool restoreBasicStyle);
bool BindSlicingMode(int modelId, const char* funcName);
int ExecuteSliceOperation(const char* funcName);

igm::vec3 ClampColor01(float r, float g, float b) {
    if (!std::isfinite(r)) r = 0.0f;
    if (!std::isfinite(g)) g = 0.0f;
    if (!std::isfinite(b)) b = 0.0f;
    return igm::vec3{std::max(0.0f, std::min(1.0f, r)), std::max(0.0f, std::min(1.0f, g)),
                     std::max(0.0f, std::min(1.0f, b))};
}

WebModelMeta* FindModelMeta(IGuint modelId) {
    auto it = g_modelRegistry.find(modelId);
    return it != g_modelRegistry.end() ? &it->second : nullptr;
}

const WebModelMeta* FindModelMetaConst(IGuint modelId) {
    auto it = g_modelRegistry.find(modelId);
    return it != g_modelRegistry.end() ? &it->second : nullptr;
}

void ForEachDrawObjectInTree(const iGame::DataObject::Pointer& root,
                             const std::function<void(iGame::DrawObject::Pointer)>& fn) {
    if (root == nullptr) return;
    auto draw = iGame::DynamicCast<iGame::DrawObject>(root);
    if (draw != nullptr) { fn(draw); }

    if (!root->HasSubDataObject()) return;
    for (auto it = root->SubDataObjectIteratorBegin(); it != root->SubDataObjectIteratorEnd(); ++it) {
        ForEachDrawObjectInTree(it->second, fn);
    }
}

void ConfigureManualRange(iGame::DrawObject::Pointer drawObj, double minValue, double maxValue) {
    if (drawObj == nullptr) return;
    auto mapper = drawObj->GetColorMapper();
    if (mapper != nullptr) {
        mapper->SetRangeStable(true);
        mapper->SetRange(minValue, maxValue);
        mapper->Modified();
    }

    auto renderObj = drawObj->GetRenderableObject(false);
    if (renderObj != nullptr && renderObj != drawObj) {
        if (mapper != nullptr) { renderObj->SetColorMapper(mapper); }
        renderObj->ForceReConvertToDrawableData();
    }
    drawObj->ForceReConvertToDrawableData();
}

void ResetAutoRange(iGame::DrawObject::Pointer drawObj, const std::string& scalarName, int dimension) {
    if (drawObj == nullptr) return;
    auto mapper = drawObj->GetColorMapper();
    if (mapper == nullptr) return;

    mapper->SetRangeStable(false);
    auto attrSet = drawObj->GetAttributeSet();
    if (attrSet != nullptr) {
        const int attrIndex = attrSet->GetAttributeIndex(scalarName);
        if (attrIndex >= 0) {
            auto attrs = attrSet->GetAllAttributes();
            if (attrs != nullptr && attrIndex < static_cast<int>(attrs->GetNumberOfElements())) {
                auto attr = attrs->GetElement(attrIndex);
                if (!attr.isDeleted && attr.pointer != nullptr) {
                    if (dimension == -1) {
                        mapper->InitRange(attr.pointer);
                    } else {
                        mapper->InitRange(attr.pointer, dimension);
                    }
                }
            }
        }
    }

    auto renderObj = drawObj->GetRenderableObject(false);
    if (renderObj != nullptr && renderObj != drawObj) {
        renderObj->SetColorMapper(mapper);
        renderObj->ForceReConvertToDrawableData();
    }
    drawObj->ForceReConvertToDrawableData();
}

void ResetAutoRange(iGame::DrawObject::Pointer drawObj, int attrIndex, int dimension) {
    if (drawObj == nullptr) return;
    auto mapper = drawObj->GetColorMapper();
    if (mapper == nullptr) return;

    mapper->SetRangeStable(false);
    auto attrSet = drawObj->GetAttributeSet();
    if (attrSet != nullptr && attrIndex >= 0) {
        auto attrs = attrSet->GetAllAttributes();
        if (attrs != nullptr && attrIndex < static_cast<int>(attrs->GetNumberOfElements())) {
            const auto& attr = attrs->GetElement(attrIndex);
            if (!attr.isDeleted && attr.pointer != nullptr) {
                if (dimension == -1) {
                    mapper->InitRange(attr.pointer);
                } else {
                    mapper->InitRange(attr.pointer, dimension);
                }
            }
        }
    }

    auto renderObj = drawObj->GetRenderableObject(false);
    if (renderObj != nullptr && renderObj != drawObj) {
        renderObj->SetColorMapper(mapper);
        renderObj->ForceReConvertToDrawableData();
    }
    drawObj->ForceReConvertToDrawableData();
}

void SetModelSolidMode(IGuint modelId, const igm::vec3& color, bool enabled) {
    auto* meta = FindModelMeta(modelId);
    if (meta == nullptr) {
        WebModelMeta newMeta;
        newMeta.id = modelId;
        newMeta.name = "model_" + std::to_string(modelId);
        newMeta.visible = true;
        newMeta.sourceType = "unknown";
        newMeta.loadTime = GetUnixTimestampSeconds();
        newMeta.solidColor = color;
        newMeta.solidEnabled = enabled;
        g_modelRegistry[modelId] = newMeta;
        return;
    }
    meta->solidColor = color;
    meta->solidEnabled = enabled;
}

void DisableModelSolidMode(IGuint modelId) {
    auto* meta = FindModelMeta(modelId);
    if (meta == nullptr) return;
    meta->solidEnabled = false;
}

bool IsModelSolidModeEnabled(IGuint modelId) {
    const auto* meta = FindModelMetaConst(modelId);
    return meta != nullptr && meta->solidEnabled;
}

igm::vec3 GetModelSolidColor(IGuint modelId, const iGame::DrawObject::Pointer& drawObj) {
    const auto* meta = FindModelMetaConst(modelId);
    if (meta != nullptr) return meta->solidColor;
    if (drawObj != nullptr) return drawObj->GetDefaultColor();
    return igm::vec3{0.85f, 0.85f, 0.85f};
}

#include <iostream>

void DebugLog(const char* level, const std::string& msg) {
    if (!g_debugEnabled) return;
    std::cout << "[iGameWeb][" << level << "] " << msg << '\n';
}

void SetLastError(int code, const char* func, const std::string& detail) {
    g_lastError.code = code;
    g_lastError.func = (func != nullptr) ? func : "unknown";
    g_lastError.detail = detail;
    g_lastError.timestamp = static_cast<long long>(std::time(nullptr));
    DebugLog("ERROR", g_lastError.func + " code=" + std::to_string(code) + " detail=" + detail);
}

void ClearLastError() {
    g_lastError.code = 0;
    g_lastError.func.clear();
    g_lastError.detail.clear();
    g_lastError.timestamp = 0;
}

std::string GetLastErrorJson() {
    std::string json = "{";
    json += "\"code\":" + std::to_string(g_lastError.code) + ",";
    json += "\"func\":\"" + EscapeJsonString(g_lastError.func) + "\",";
    json += "\"detail\":\"" + EscapeJsonString(g_lastError.detail) + "\",";
    json += "\"timestamp\":" + std::to_string(g_lastError.timestamp);
    json += "}";
    return json;
}

int FailWithError(int code, const char* func, const std::string& detail) {
    SetLastError(code, func, detail);
    return code;
}

std::string FormatVec3(const iGame::Vector3d& v) {
    return "(" + std::to_string(v[0]) + ", " + std::to_string(v[1]) + ", " + std::to_string(v[2]) + ")";
}

std::string DescribeDataObjectType(IGenum type) {
    switch (type) {
        case IG_DATA_OBJECT:
            return "DataObject";
        case IG_POINT_SET:
            return "PointSet";
        case IG_SURFACE_MESH:
            return "SurfaceMesh";
        case IG_UNSTRUCTURED_MESH:
            return "UnstructuredMesh";
        default:
            return "Unknown(" + std::to_string(static_cast<int>(type)) + ")";
    }
}

void LogVtuSummary(const iGame::DataObject::Pointer& dataObj, const std::string& sourceName) {
    if (dataObj == nullptr) {
        DebugLog("INFO", "VTU summary skipped: data object is null");
        return;
    }

    const auto typeName = DescribeDataObjectType(dataObj->GetDataObjectType());
    const auto pointSet = iGame::DynamicCast<iGame::PointSet>(dataObj);
    const auto unstructuredMesh = iGame::DynamicCast<iGame::UnstructuredMesh>(dataObj);

    const IGsize pointCount =
            (pointSet != nullptr && pointSet->GetPoints() != nullptr) ? pointSet->GetPoints()->GetNumberOfPoints() : 0;
    const IGsize cellCount = unstructuredMesh != nullptr ? unstructuredMesh->GetNumberOfCells() : 0;

    size_t totalAttrCount = 0;
    size_t pointAttrCount = 0;
    size_t cellAttrCount = 0;
    auto attributeSet = dataObj->GetAttributeSet();
    if (attributeSet != nullptr) {
        totalAttrCount = attributeSet->GetNumberOfAttributes();
        auto pointAttrs = attributeSet->GetAllPointAttributes();
        auto cellAttrs = attributeSet->GetAllCellAttributes();
        pointAttrCount = pointAttrs ? pointAttrs->GetNumberOfElements() : 0;
        cellAttrCount = cellAttrs ? cellAttrs->GetNumberOfElements() : 0;
    }

    const auto& bbox = dataObj->GetBoundingBox();
    DebugLog("INFO", "VTU summary sourceName=" + sourceName + " type=" + typeName +
                             " points=" + std::to_string(pointCount) + " cells=" + std::to_string(cellCount) +
                             " attrs(total=" + std::to_string(totalAttrCount) +
                             ", point=" + std::to_string(pointAttrCount) + ", cell=" + std::to_string(cellAttrCount) +
                             ")" + " bbox[min=" + FormatVec3(bbox.min) + ", max=" + FormatVec3(bbox.max) + "]");
}

struct ZipEntryInfo {
    std::string name;
    uint16_t compressionMethod = 0;
    uint32_t crcValue = 0;
    uint64_t compressedSize = 0;
    uint64_t uncompressedSize = 0;
    uint64_t localHeaderOffset = 0;
};

template<typename T>
T ReadLittleEndian(const unsigned char* ptr) {
    T value{};
    std::memcpy(&value, ptr, sizeof(T));
    return value;
}

uint16_t ReadU16(const unsigned char* ptr) { return ReadLittleEndian<uint16_t>(ptr); }
uint32_t ReadU32(const unsigned char* ptr) { return ReadLittleEndian<uint32_t>(ptr); }

std::string ToLowerAscii(std::string value) {
    for (char& c: value) {
        if (c >= 'A' && c <= 'Z') { c = static_cast<char>(c - 'A' + 'a'); }
    }
    return value;
}

std::string NormalizeEntryFileName(const std::string& name) {
    std::string normalized = name;
    for (char& c: normalized) {
        if (c == '\\') { c = '/'; }
    }
    const auto pos = normalized.find_last_of('/');
    if (pos != std::string::npos && pos + 1 < normalized.size()) { normalized = normalized.substr(pos + 1); }
    return normalized;
}

bool EndsWithCaseInsensitive(const std::string& value, const std::string& suffix) {
    if (value.size() < suffix.size()) return false;
    return ToLowerAscii(value.substr(value.size() - suffix.size())) == ToLowerAscii(suffix);
}

bool IsSupportedModelEntry(const std::string& name) {
    return EndsWithCaseInsensitive(name, ".vtk") || EndsWithCaseInsensitive(name, ".vtu") ||
           EndsWithCaseInsensitive(name, ".vtp") || EndsWithCaseInsensitive(name, ".igc") ||
           EndsWithCaseInsensitive(name, ".igcm");
}

bool IsZipDirectoryEntry(const std::string& name) {
    return !name.empty() && (name.back() == '/' || name.back() == '\\');
}

bool NormalizeZipRelativePath(const std::string& name, std::string& normalized) {
    normalized = name;
    for (char& c: normalized) {
        if (c == '\\') { c = '/'; }
    }

    if (normalized.empty() || normalized.front() == '/') { return false; }
    if (normalized.size() >= 2 && normalized[1] == ':') { return false; }

    std::filesystem::path path(normalized);
    if (path.is_absolute() || path.has_root_name() || path.has_root_directory()) { return false; }
    for (const auto& part: path) {
        if (part == "..") { return false; }
    }

    return true;
}

std::string JoinZipMountPath(const std::string& mountRoot, const std::string& relativePath) {
    return (std::filesystem::path(mountRoot) / std::filesystem::path(relativePath)).generic_string();
}

bool WriteFileBytes(const std::string& path, const std::vector<unsigned char>& bytes, std::string& errorDetail) {
    try {
        const auto parent = std::filesystem::path(path).parent_path();
        if (!parent.empty()) { std::filesystem::create_directories(parent); }

        std::ofstream out(path, std::ios::binary);
        if (!out.is_open()) {
            errorDetail = "failed to open MEMFS file for write: " + path;
            return false;
        }
        if (!bytes.empty()) {
            out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        }
        if (!out.good()) {
            errorDetail = "failed to write MEMFS file: " + path;
            return false;
        }
    } catch (const std::exception& ex) {
        errorDetail = std::string("failed to write MEMFS file: ") + ex.what();
        return false;
    }
    return true;
}

std::string DescribeZipEntry(const std::string& archiveName, const std::string& entryName) {
    if (archiveName.empty()) return entryName;
    return archiveName + "::" + entryName;
}

bool InflateRawDeflate(const unsigned char* input, size_t inputSize, size_t expectedSize,
                       std::vector<unsigned char>& output, std::string& errorDetail) {
    output.clear();
    if (expectedSize == 0) {
        errorDetail = "zip entry has zero uncompressed size";
        return false;
    }

    z_stream stream{};
    stream.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(input));
    stream.avail_in = static_cast<uInt>(inputSize);

    const int initStatus = inflateInit2(&stream, -MAX_WBITS);
    if (initStatus != Z_OK) {
        errorDetail = "inflateInit2 failed with status " + std::to_string(initStatus);
        return false;
    }

    output.resize(expectedSize);
    stream.next_out = reinterpret_cast<Bytef*>(output.data());
    stream.avail_out = static_cast<uInt>(output.size());

    const int inflateStatus = inflate(&stream, Z_FINISH);
    const bool success = (inflateStatus == Z_STREAM_END && stream.total_out == expectedSize);
    inflateEnd(&stream);

    if (!success) {
        errorDetail = "inflate failed with status " + std::to_string(inflateStatus) +
                      " total_out=" + std::to_string(static_cast<unsigned long long>(stream.total_out)) +
                      " expected=" + std::to_string(expectedSize);
        output.clear();
        return false;
    }

    return true;
}

bool ExtractZipEntry(const std::string& archiveBytes, const ZipEntryInfo& entry, std::vector<unsigned char>& output,
                     std::string& errorDetail) {
    if (entry.localHeaderOffset > archiveBytes.size()) {
        errorDetail = "zip entry local header offset is out of range";
        return false;
    }

    const auto* archiveData = reinterpret_cast<const unsigned char*>(archiveBytes.data());
    const size_t archiveSize = archiveBytes.size();
    const size_t localOffset = static_cast<size_t>(entry.localHeaderOffset);
    if (archiveSize < localOffset + 30) {
        errorDetail = "zip local header is truncated";
        return false;
    }

    if (ReadU32(archiveData + localOffset) != 0x04034b50u) {
        errorDetail = "zip local header signature mismatch";
        return false;
    }

    const uint16_t fileNameLength = ReadU16(archiveData + localOffset + 26);
    const uint16_t extraFieldLength = ReadU16(archiveData + localOffset + 28);
    const size_t dataOffset =
            localOffset + 30 + static_cast<size_t>(fileNameLength) + static_cast<size_t>(extraFieldLength);
    if (dataOffset > archiveSize) {
        errorDetail = "zip entry data offset is out of range";
        return false;
    }

    if (entry.compressedSize > archiveSize - dataOffset) {
        errorDetail = "zip entry compressed data is truncated";
        return false;
    }

    const unsigned char* compressedData = archiveData + dataOffset;
    if (entry.compressionMethod == 0) {
        output.assign(compressedData, compressedData + static_cast<size_t>(entry.compressedSize));
    } else if (entry.compressionMethod == 8) {
        if (!InflateRawDeflate(compressedData, static_cast<size_t>(entry.compressedSize),
                               static_cast<size_t>(entry.uncompressedSize), output, errorDetail)) {
            return false;
        }
    } else {
        errorDetail = "unsupported zip compression method " + std::to_string(entry.compressionMethod);
        return false;
    }

    const uLong initialCrc = ::crc32(0L, Z_NULL, 0);
    const uLong actualCrc =
            ::crc32(initialCrc, reinterpret_cast<const Bytef*>(output.data()), static_cast<uInt>(output.size()));
    if (actualCrc != entry.crcValue) {
        errorDetail = "zip crc mismatch for entry " + entry.name;
        return false;
    }

    return true;
}

bool ParseZipEntries(const std::string& archiveBytes, std::vector<ZipEntryInfo>& entries, std::string& errorDetail) {
    entries.clear();
    if (archiveBytes.size() < 22) {
        errorDetail = "zip archive is too small";
        return false;
    }

    const auto* data = reinterpret_cast<const unsigned char*>(archiveBytes.data());
    const size_t size = archiveBytes.size();
    const size_t searchStart = size > (22 + 0xFFFFu) ? size - (22 + 0xFFFFu) : 0;

    size_t eocdOffset = size;
    for (size_t pos = size - 22;; --pos) {
        if (pos < searchStart) { break; }
        if (ReadU32(data + pos) == 0x06054b50u) {
            eocdOffset = pos;
            break;
        }
        if (pos == 0) { break; }
    }

    if (eocdOffset == size) {
        errorDetail = "zip end of central directory not found";
        return false;
    }

    const uint16_t diskNumber = ReadU16(data + eocdOffset + 4);
    const uint16_t centralDirectoryStartDisk = ReadU16(data + eocdOffset + 6);
    const uint16_t entryCountOnThisDisk = ReadU16(data + eocdOffset + 8);
    const uint16_t totalEntryCount = ReadU16(data + eocdOffset + 10);
    const uint32_t centralDirectorySize = ReadU32(data + eocdOffset + 12);
    const uint32_t centralDirectoryOffset = ReadU32(data + eocdOffset + 16);
    const uint16_t commentLength = ReadU16(data + eocdOffset + 20);

    if (diskNumber != 0 || centralDirectoryStartDisk != 0 || entryCountOnThisDisk != totalEntryCount) {
        errorDetail = "multi-disk zip archives are not supported";
        return false;
    }

    if (centralDirectoryOffset >= size || static_cast<uint64_t>(centralDirectoryOffset) + centralDirectorySize > size) {
        errorDetail = "central directory is out of range";
        return false;
    }

    if (eocdOffset + 22 + commentLength > size) {
        errorDetail = "zip comment is truncated";
        return false;
    }

    size_t cursor = centralDirectoryOffset;
    entries.reserve(totalEntryCount);
    for (uint16_t index = 0; index < totalEntryCount; ++index) {
        if (cursor + 46 > size) {
            errorDetail = "central directory entry is truncated";
            return false;
        }
        if (ReadU32(data + cursor) != 0x02014b50u) {
            errorDetail = "central directory signature mismatch";
            return false;
        }

        const uint16_t compressionMethod = ReadU16(data + cursor + 10);
        const uint32_t crc = ReadU32(data + cursor + 16);
        const uint32_t compressedSize32 = ReadU32(data + cursor + 20);
        const uint32_t uncompressedSize32 = ReadU32(data + cursor + 24);
        const uint16_t fileNameLength = ReadU16(data + cursor + 28);
        const uint16_t extraFieldLength = ReadU16(data + cursor + 30);
        const uint16_t commentFieldLength = ReadU16(data + cursor + 32);
        const uint32_t localHeaderOffset32 = ReadU32(data + cursor + 42);

        if (compressedSize32 == 0xFFFFFFFFu || uncompressedSize32 == 0xFFFFFFFFu ||
            localHeaderOffset32 == 0xFFFFFFFFu) {
            errorDetail = "zip64 archives are not supported";
            return false;
        }

        const size_t headerSize = 46 + static_cast<size_t>(fileNameLength) + static_cast<size_t>(extraFieldLength) +
                                  static_cast<size_t>(commentFieldLength);
        if (cursor + headerSize > size) {
            errorDetail = "central directory entry fields are truncated";
            return false;
        }

        std::string entryName(reinterpret_cast<const char*>(data + cursor + 46), fileNameLength);
        ZipEntryInfo entry;
        entry.name = entryName;
        entry.compressionMethod = compressionMethod;
        entry.crcValue = crc;
        entry.compressedSize = compressedSize32;
        entry.uncompressedSize = uncompressedSize32;
        entry.localHeaderOffset = localHeaderOffset32;
        entries.push_back(std::move(entry));

        cursor += headerSize;
    }

    return true;
}

std::string EscapeJsonString(const std::string& in) {
    std::string out;
    out.reserve(in.size() + 8);
    for (char c: in) {
        switch (c) {
            case '"':
                out += "\\\"";
                break;
            case '\\':
                out += "\\\\";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                out += c;
                break;
        }
    }
    return out;
}

void EnsureScene() {
    if (g_scene != nullptr && g_window != nullptr && g_interactor != nullptr) {
        DebugLog("INFO", "EnsureScene skip: scene/window/interactor already ready");
        return;
    }

    DebugLog("INFO", "EnsureScene begin");

    if (g_scene == nullptr) {
        g_scene = iGame::Scene::New();
        if (g_scene == nullptr) {
            SetLastError(0, "EnsureScene", "Scene::New() returned null");
            return;
        }
        DebugLog("INFO", "EnsureScene scene created");
    } else {
        DebugLog("INFO", "EnsureScene reuse existing scene");
    }

    if (g_window == nullptr) {
        g_window = iGame::RenderWindow::New();
        if (g_window == nullptr) {
            SetLastError(0, "EnsureScene", "RenderWindow::New() returned null");
            return;
        }
        DebugLog("INFO", "EnsureScene window created");
    } else {
        DebugLog("INFO", "EnsureScene reuse existing window");
    }

    g_window->SetScene(g_scene);
    if (g_window->GetScene() == nullptr) {
        SetLastError(0, "EnsureScene", "window scene binding failed (GetScene is null)");
        return;
    }
    DebugLog("INFO", "EnsureScene window scene binding ok");

    if (g_interactor == nullptr) {
        g_interactor = iGame::Interactor::New();
        if (g_interactor == nullptr) {
            SetLastError(0, "EnsureScene", "Interactor::New() returned null");
            return;
        }
        DebugLog("INFO", "EnsureScene interactor created");
    } else {
        DebugLog("INFO", "EnsureScene reuse existing interactor");
    }

    g_interactor->Initialize(g_scene);
    g_window->SetInteractor(g_interactor);
    g_scene->SetInteractor(g_interactor);

    if (g_window->GetRawWindowPtr() == nullptr) {
        SetLastError(0, "EnsureScene", "window raw pointer is null after initialize");
        return;
    }

    ClearLastError();
    DebugLog("INFO", "EnsureScene success");
}

long long GetUnixTimestampSeconds() { return static_cast<long long>(std::time(nullptr)); }

void SyncActiveModelIdFromScene() {
    if (g_scene == nullptr) {
        g_activeModelId = 0;
        return;
    }
    auto current = g_scene->GetCurrentModelID();
    if (current != 0) {
        g_activeModelId = current;
        return;
    }
    g_activeModelId = 0;
}

void RemoveAllUserModels() {
    if (g_scene == nullptr) return;
    std::vector<IGuint> ids;
    ids.reserve(g_modelRegistry.size());
    for (const auto& kv: g_modelRegistry) { ids.push_back(kv.first); }
    for (IGuint id: ids) {
        g_scene->RemoveModel(id);
        g_modelRegistry.erase(id);
    }
    SyncActiveModelIdFromScene();
}

int AddModelFromDataObject(iGame::SmartPointer<iGame::DataObject> dataObj, const char* sourceName, bool replaceExisting,
                           const char* sourceType) {
    if (dataObj == nullptr || g_scene == nullptr) {
        return FailWithError(0, "AddModelFromDataObject", "dataObj or scene is null");
    }

    if (replaceExisting) { RemoveAllUserModels(); }

    const IGuint modelId = g_scene->AddModel(dataObj);
    if (modelId == 0) { return FailWithError(0, "AddModelFromDataObject", "scene->AddModel returned 0"); }

    WebModelMeta meta;
    meta.id = modelId;
    meta.name = (sourceName != nullptr && sourceName[0] != '\0') ? sourceName
                                                                 : std::string("model_") + std::to_string(modelId);
    meta.visible = true;
    meta.sourceType = (sourceType != nullptr && sourceType[0] != '\0') ? sourceType : "memory-vtk";
    meta.loadTime = GetUnixTimestampSeconds();
    auto drawObj = iGame::DynamicCast<iGame::DrawObject>(dataObj);
    meta.solidColor = drawObj != nullptr ? drawObj->GetDefaultColor() : igm::vec3{0.85f, 0.85f, 0.85f};
    meta.solidEnabled = false;
    g_modelRegistry[modelId] = meta;

    g_activeModelId = modelId;
    g_scene->SetCurrentModel(static_cast<int>(modelId));
    RebindCurrentSelectionMode("AddModelFromDataObject");
    g_scene->ResetCameraView();
    ClearLastError();
    DebugLog("INFO", "AddModelFromDataObject success modelId=" + std::to_string(modelId));
    return static_cast<int>(modelId);
}

iGame::Model::Pointer GetActiveModel() {
    if (g_scene == nullptr) return nullptr;
    if (g_activeModelId != 0) {
        auto model = g_scene->GetModelById(static_cast<int>(g_activeModelId));
        if (model != nullptr) { return model; }
    }
    return g_scene->GetCurrentModel();
}

iGame::ClipSelection::Pointer EnsureClipSelection() {
    if (g_clipSelection == nullptr) {
        g_clipSelection = iGame::ClipSelection::New();
        g_clipSelection->Preview = false;
        g_clipSelection->SetSelectionCallBackEvent(
                [](IGenum itemType, const std::vector<igIndex>&, iGame::Selection::Operate) {
                    if (itemType != IG_CHANGE || !g_slicingActive) { return; }
                    if (g_clipSelection != nullptr && g_clipSelection->Preview) {
                        ExecuteSliceOperation("ClipSelectionPreview");
                    }
                },
                std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    }
    return g_clipSelection;
}

void ExitSlicingModeInternal(bool restoreBasicStyle) {
    if (!g_slicingActive) { return; }
    g_slicingActive = false;
    g_sliceSourceModelId = 0;
    if (restoreBasicStyle && g_interactor != nullptr) {
        iGame::SelectionParameter::Instance().SetInSelection(false);
        g_interactor->RequestBasicStyle();
    }
}

bool BindSlicingMode(int modelId, const char* funcName) {
    EnsureScene();
    if (g_interactor == nullptr || g_scene == nullptr) {
        FailWithError(0, funcName, "scene or interactor is null");
        return false;
    }

    auto model = g_scene->GetModelById(modelId);
    if (model == nullptr) {
        return FailWithError(0, funcName, "model not found id=" + std::to_string(modelId));
    }

    auto obj = model->GetDataObject();
    if (obj == nullptr) {
        return FailWithError(0, funcName, "model data object is null id=" + std::to_string(modelId));
    }

    g_selectionMode = 0;
    iGame::SelectionParameter::Instance().SetInSelection(false);

    auto clipSelection = EnsureClipSelection();
    auto painter = model->GetPainter3D();
    if (painter == nullptr) {
        return FailWithError(0, funcName, "model painter is null id=" + std::to_string(modelId));
    }
    painter->SetTotallyHide(false);
    g_interactor->SetDataObject(obj);
    g_interactor->SetPainter3D(painter);
    g_interactor->RequestSlicingStyle(clipSelection);

    g_slicingActive = true;
    g_sliceSourceModelId = modelId;
    g_activeModelId = static_cast<IGuint>(modelId);
    g_scene->SetCurrentModel(modelId);
    if (g_interactor != nullptr && !g_interactor->IsBasicStyle()) {
        g_scene->Update();
    } else {
        return FailWithError(0, funcName, "RequestSlicingStyle did not activate slicing interactor");
    }
    ClearLastError();
    return true;
}

std::string GetClipPlaneJson() {
    if (g_clipSelection == nullptr) {
        return "{\"origin\":[0,0,0],\"normal\":[1,0,0]}";
    }
    std::ostringstream out;
    out << std::setprecision(9);
    out << "{\"origin\":[" << g_clipSelection->PlanePoint[0] << "," << g_clipSelection->PlanePoint[1] << ","
        << g_clipSelection->PlanePoint[2] << "],\"normal\":[" << g_clipSelection->PlaneNormal[0] << ","
        << g_clipSelection->PlaneNormal[1] << "," << g_clipSelection->PlaneNormal[2] << "]}";
    return out.str();
}

int SetClipPlane(float ox, float oy, float oz, float nx, float ny, float nz) {
    if (!g_slicingActive) {
        return FailWithError(0, "SetClipPlane", "slicing mode is not active");
    }
    if (!std::isfinite(ox) || !std::isfinite(oy) || !std::isfinite(oz) || !std::isfinite(nx) ||
        !std::isfinite(ny) || !std::isfinite(nz)) {
        return FailWithError(0, "SetClipPlane", "plane values must be finite");
    }
    const float length2 = nx * nx + ny * ny + nz * nz;
    if (length2 <= 1e-12f) {
        return FailWithError(0, "SetClipPlane", "plane normal must be non-zero");
    }

    auto clipSelection = EnsureClipSelection();
    clipSelection->PlanePoint = iGame::Vector3d{ox, oy, oz};
    clipSelection->PlaneNormal = iGame::Vector3d{nx, ny, nz};
    clipSelection->UpdatePlane();
    if (g_scene != nullptr) { g_scene->Update(); }
    ClearLastError();
    return 1;
}

int SetSlicingPreview(bool enabled) {
    auto clipSelection = EnsureClipSelection();
    clipSelection->Preview = enabled;
    ClearLastError();
    return 1;
}

int SetSliceOperationMode(int mode) {
    if (mode < 0 || mode > 1) {
        return FailWithError(0, "SetSliceOperationMode", "mode must be 0(slice) or 1(clip)");
    }
    g_sliceOperationMode = mode;
    ClearLastError();
    return 1;
}

int SetSliceCrinkle(bool enabled) {
    g_sliceCrinkle = enabled;
    ClearLastError();
    return 1;
}

int SetSliceInvert(bool enabled) {
    g_sliceInvert = enabled;
    ClearLastError();
    return 1;
}

static bool ApplySliceOrClipToInput(iGame::DataObject::Pointer input, iGame::UnstructuredMesh::Pointer result,
                                    double origin[3], double normal[3]) {
    if (input == nullptr || result == nullptr) { return false; }

    if (g_sliceOperationMode == 0) {
        auto slicer = iGame::SliceFilter::New();
        slicer->SetInput(input);
        slicer->SetPlane(origin, normal);
        slicer->SetCrinkle(g_sliceCrinkle);
        if (!slicer->Execute()) { return false; }
        auto out = slicer->GetSliceMesh();
        if (out == nullptr) { return false; }
        result->SetPoints(out->GetPoints());
        result->SetCells(out->GetCells(), out->GetCellTypes());
        result->SetAttributeSet(out->GetAttributeSet());
        return true;
    }

    auto clipper = iGame::ClipFilter::New();
    clipper->SetInput(input);
    clipper->SetPlane(origin, normal);
    clipper->SetCrinkle(g_sliceCrinkle);
    clipper->SetInvert(g_sliceInvert);
    if (!clipper->Execute()) { return false; }
    auto out = clipper->GetClipMesh();
    if (out == nullptr) { return false; }
    result->SetPoints(out->GetPoints());
    result->SetCells(out->GetCells(), out->GetCellTypes());
    result->SetAttributeSet(out->GetAttributeSet());
    return true;
}

int ExecuteSliceOperation(const char* funcName) {
    EnsureScene();
    if (!g_slicingActive || g_sliceSourceModelId <= 0) {
        return FailWithError(0, funcName, "slicing mode is not active");
    }
    if (g_clipSelection == nullptr) {
        return FailWithError(0, funcName, "clip selection is null");
    }

    auto sourceModel = g_scene->GetModelById(g_sliceSourceModelId);
    if (sourceModel == nullptr) {
        return FailWithError(0, funcName, "source model not found id=" + std::to_string(g_sliceSourceModelId));
    }
    auto sourceObject = sourceModel->GetDataObject();
    if (sourceObject == nullptr) {
        return FailWithError(0, funcName, "source data object is null");
    }

    double origin[3] = {g_clipSelection->PlanePoint[0], g_clipSelection->PlanePoint[1],
                        g_clipSelection->PlanePoint[2]};
    double normal[3] = {g_clipSelection->PlaneNormal[0], g_clipSelection->PlaneNormal[1],
                        g_clipSelection->PlaneNormal[2]};

    auto resultMesh = iGame::UnstructuredMesh::New();
    const std::string suffix = g_sliceOperationMode == 0 ? "_Slice" : "_Clip";
    const WebModelMeta* sourceMeta = FindModelMetaConst(static_cast<IGuint>(g_sliceSourceModelId));
    const std::string sourceName = sourceMeta != nullptr ? sourceMeta->name : ("model_" + std::to_string(g_sliceSourceModelId));
    resultMesh->SetName(sourceName + suffix);
    if (sourceObject->GetAttributeSet() != nullptr) {
        resultMesh->SetAttributeSet(sourceObject->GetAttributeSet());
    }

    bool ok = false;
    if (sourceObject->HasSubDataObject()) {
        resultMesh->ClearSubDataObject();
        for (auto it = sourceObject->SubDataObjectIteratorBegin(); it != sourceObject->SubDataObjectIteratorEnd(); ++it) {
            auto childObject = it->second;
            if (childObject == nullptr) { continue; }
            auto childResult = iGame::UnstructuredMesh::New();
            if (!ApplySliceOrClipToInput(childObject, childResult, origin, normal)) { continue; }
            resultMesh->AddSubDataObject(childResult);
        }
        ok = resultMesh->GetNumberOfSubDataObjects() > 0;
    } else {
        ok = ApplySliceOrClipToInput(sourceObject, resultMesh, origin, normal);
    }

    if (!ok) {
        return FailWithError(0, funcName, "slice/clip filter produced no output");
    }

    auto sourceDraw = iGame::DynamicCast<iGame::DrawObject>(sourceObject);
    resultMesh->SetViewStyle(sourceDraw != nullptr ? sourceDraw->GetViewStyle() : 4u);
    resultMesh->ConvertToDrawableData();
    if (sourceDraw != nullptr && sourceDraw->GetColorMapper() != nullptr) {
        resultMesh->SetColorMapper(sourceDraw->GetColorMapper());
    }

    if (g_sliceResultModelId > 0) {
        g_scene->RemoveModel(static_cast<IGuint>(g_sliceResultModelId));
        g_modelRegistry.erase(static_cast<IGuint>(g_sliceResultModelId));
        g_sliceResultModelId = 0;
    }

    const IGuint resultModelId = g_scene->AddModel(resultMesh);
    if (resultModelId == 0) {
        return FailWithError(0, funcName, "failed to add slice result model");
    }

    WebModelMeta meta;
    meta.id = resultModelId;
    meta.name = resultMesh->GetName();
    meta.visible = true;
    meta.sourceType = g_sliceOperationMode == 0 ? "slice-result" : "clip-result";
    meta.loadTime = GetUnixTimestampSeconds();
    auto drawObj = iGame::DynamicCast<iGame::DrawObject>(resultMesh);
    meta.solidColor = drawObj != nullptr ? drawObj->GetDefaultColor() : igm::vec3{0.85f, 0.85f, 0.85f};
    meta.solidEnabled = false;
    g_modelRegistry[resultModelId] = meta;
    g_sliceResultModelId = static_cast<int>(resultModelId);

    g_scene->Update();
    ClearLastError();
    return static_cast<int>(resultModelId);
}

int EnterSlicingMode(int modelId) {
    EnsureScene();
    if (modelId <= 0) {
        const auto active = GetActiveModel();
        if (active == nullptr) {
            return FailWithError(0, "EnterSlicingMode", "no active model and modelId is invalid");
        }
        modelId = static_cast<int>(g_activeModelId);
    }
    if (!BindSlicingMode(modelId, "EnterSlicingMode")) { return 0; }
    return 1;
}

int ExitSlicingMode() {
    ExitSlicingModeInternal(true);
    if (g_scene != nullptr) { g_scene->Update(); }
    ClearLastError();
    return 1;
}

int IsSlicingMode() { return g_slicingActive ? 1 : 0; }

int GetSliceSourceModelId() { return g_sliceSourceModelId; }

int GetSliceResultModelId() { return g_sliceResultModelId; }

bool BindSelectionMode(int mode, const char* funcName, bool allowMissingModel) {
    if (g_slicingActive && mode != 0) {
        ExitSlicingModeInternal(false);
    }
    if (g_interactor == nullptr || g_scene == nullptr) {
        FailWithError(0, funcName, "scene or interactor is null");
        return false;
    }

    if (mode == 0) {
        iGame::SelectionParameter::Instance().SetInSelection(false);
        g_interactor->RequestBasicStyle();
        return true;
    }

    auto model = GetActiveModel();
    if (model == nullptr) {
        iGame::SelectionParameter::Instance().SetInSelection(false);
        g_interactor->RequestBasicStyle();
        if (allowMissingModel) return true;
        FailWithError(0, funcName, "active model is null");
        return false;
    }

    auto obj = model->GetDataObject();
    auto pointSet = iGame::DynamicCast<iGame::PointSet>(obj);
    auto selection = model->GetSelection();
    if (obj == nullptr || pointSet == nullptr || selection == nullptr) {
        FailWithError(0, funcName, "active model does not support selection");
        return false;
    }

    selection->SetPoints(pointSet->GetPoints());
    selection->SetModel(model);
    g_interactor->SetDataObject(obj);
    g_interactor->SetPainter3D(model->GetPainter3D());

    if (mode == 1) {
        g_interactor->RequestPointSelectionStyle(selection);
    } else if (mode == 2) {
        iGame::CellArray::Pointer cells;
        if (auto mesh = iGame::DynamicCast<iGame::VolumeMesh>(obj)) {
            cells = mesh->GetFaces();
        } else if (auto mesh = iGame::DynamicCast<iGame::UnstructuredMesh>(obj)) {
            cells = mesh->GetCells();
        } else if (auto mesh = iGame::DynamicCast<iGame::SurfaceMesh>(obj)) {
            cells = mesh->GetFaces();
        }
        if (cells == nullptr) {
            FailWithError(0, funcName, "active model does not support face selection");
            return false;
        }
        selection->SetCells(cells);
        g_interactor->RequestFaceSelectionStyle(selection);
    } else {
        FailWithError(0, funcName, "unknown selection mode=" + std::to_string(mode));
        return false;
    }

    auto& params = iGame::SelectionParameter::Instance();
    params.SetSelectionRadius(0.0);
    params.SetSelectMode(iGame::SelectionParameter::SelectMode::RADIUS_MODE);
    params.SetSelectOrUnSelect(true);
    params.SetInSelection(true);
    return true;
}

bool RebindCurrentSelectionMode(const char* funcName) {
    return BindSelectionMode(g_selectionMode, funcName, true);
}

bool ReadFloatVectorFromJsArrayLike(const val& input, std::vector<float>& output, const char* funcName) {
    try {
        const size_t length = input["length"].as<size_t>();
        if (length > output.max_size() || length > MAX_SAFE_INPUT_LENGTH) {
            FailWithError(0, funcName, "input array is too large to fit in wasm memory");
            return false;
        }
        output.clear();
        output.reserve(length);
        for (size_t i = 0; i < length; ++i) { output.push_back(input[i].as<float>()); }
        return true;
    } catch (...) {
        FailWithError(0, funcName, "failed to convert JS array-like value to float vector");
        return false;
    }
}

bool ReadBytesFromJsValue(const val& input, std::string& output, const char* funcName) {
    try {
        if (input.typeOf().as<std::string>() == "string") {
            output = input.as<std::string>();
            return true;
        }

        val byteSource = input;
        if (input["buffer"].isUndefined()) { byteSource = val::global("Uint8Array").new_(input); }

        const size_t byteLength = byteSource["byteLength"].as<size_t>();
        if (byteLength > output.max_size() || byteLength > MAX_SAFE_INPUT_BYTES) {
            FailWithError(0, funcName, "input byte buffer is too large to fit in wasm memory");
            return false;
        }

        // If the buffer fits in a single 32-bit typed_memory_view, do a single set.
        if (byteLength <= static_cast<size_t>(UINT32_MAX)) {
            output.assign(byteLength, '\0');
            if (byteLength == 0) { return true; }

            val target = val(typed_memory_view(static_cast<size_t>(byteLength),
                                               reinterpret_cast<unsigned char*>(output.data())));
            target.call<void>("set", byteSource);
            return true;
        }

        // Otherwise perform a chunked copy to avoid 32-bit typed_memory_view limits.
        size_t remaining = byteLength;
        output.assign(byteLength, '\0');
        if (byteLength == 0) { return true; }

        size_t offset = 0;
        while (remaining > 0) {
            const size_t thisChunk = std::min(CHUNK_COPY_SIZE, remaining);

            // JS typed arrays support subarray(start, end)
            val sub = byteSource.call<val>("subarray", static_cast<uint32_t>(offset),
                                           static_cast<uint32_t>(offset + thisChunk));

            val target = val(typed_memory_view(static_cast<size_t>(thisChunk),
                                               reinterpret_cast<unsigned char*>(output.data() + offset)));
            target.call<void>("set", sub);

            offset += thisChunk;
            remaining -= thisChunk;
        }
        return true;
    } catch (...) {
        FailWithError(0, funcName, "failed to convert JS bytes to wasm byte buffer");
        output.clear();
        return false;
    }
}

} // namespace

namespace iGameWeb
{
void RenderFrame();

struct API {
    static int init();
    static int destroy();
    static std::string stressLifecycleVtu(const val& bytes, int iterations);
    static void setDebugEnabled(bool enabled);
    static int raiseTestError(const std::string& detail, int code);
    static std::string getLastErrorJson();
    static void clearLastError();
    static int setSize(int width, int height);
    static int loadVtkFromMem(const val& bytes);
    static int loadVtuFromMem(const val& bytes);
    static int loadVtpFromMem(const val& bytes);
    static int loadIgcFromMem(const val& bytes);
    static int loadIgcLegacy32FromMem(const val& bytes);
    static int loadIgcLegacy64FromMem(const val& bytes);
    static int loadVtkFromMemEx(const val& bytes, const std::string& sourceName, bool replaceExisting);
    static int loadVtuFromMemEx(const val& bytes, const std::string& sourceName, bool replaceExisting);
    static int loadVtpFromMemEx(const val& bytes, const std::string& sourceName, bool replaceExisting);
    static int loadIgcFromMemEx(const val& bytes, const std::string& sourceName, bool replaceExisting);
    static int loadIgcLegacy32FromMemEx(const val& bytes, const std::string& sourceName, bool replaceExisting);
    static int loadIgcLegacy64FromMemEx(const val& bytes, const std::string& sourceName, bool replaceExisting);
    static int loadZipFromMemEx(const val& bytes, const std::string& sourceName, bool replaceExisting);
    static std::string getModelListJson();
    static int setActiveModel(int modelId);
    static int setModelVisibility(int modelId, bool visible);
    static int removeModel(int modelId);
    static int removeAllModels();
    static int clearViewer();
    static int clear();
    static int setSelectionMode(int mode);
    static int getSelectionMode();
    static int enterSlicingMode(int modelId);
    static int exitSlicingMode();
    static int isSlicingMode();
    static int getSliceSourceModelId();
    static int getSliceResultModelId();
    static std::string getClipPlaneJson();
    static int setClipPlane(float ox, float oy, float oz, float nx, float ny, float nz);
    static int setSlicingPreview(bool enabled);
    static int setSliceOperationMode(int mode);
    static int setSliceCrinkle(bool enabled);
    static int setSliceInvert(bool enabled);
    static int executeSlice();
    static std::string getSelectionJson(int modelId);
    static int clearSelection(int modelId);
    static int setViewStyle(int styleMask);
    static int setViewStyle(int modelId, int styleMask);
    static int getViewStyle(int modelId);
    static int setActorOpacity(int modelId, float opacity);
    static float getActorOpacity(int modelId);
    static int setScalarField(int attributeIndex, int dimension, int dataLocation);
    static int setSurfaceShadingMode(int mode);
    static int setBackgroundColor(float r, float g, float b);
    static int setSceneBackgroundColor(float r, float g, float b);
    static int setSceneBackgroundGradientColor(float r1, float g1, float b1, float r2, float g2, float b2, int mode);
    static int setColorMap(const val& colors_flat, const val& ranges);
    // Note: `dataLocation` removed from JS API; Web example ignores it.
    static int setColorMapByName(int modelId, const std::string& scalarName, int dimension, const val& colors_flat,
                                 const val& ranges);
    static int setColorMapByNameRange(int modelId, const std::string& scalarName, int dimension, double minValue,
                                      double maxValue);
    static std::string debugColorMapRangeSequence(int modelId, const std::string& scalarName, int dimension,
                                                  double firstMin, double firstMax, double secondMin, double secondMax,
                                                  double thirdMin, double thirdMax);
    static int setModelDefaultColor(int modelId, float r, float g, float b);
    static std::string getModelDefaultColor(int modelId);
    static int setSolidColor(int modelId, float r, float g, float b);
    static std::string getSolidColor(int modelId);
    static int setHelperPen(int modelId, bool enabled, float r, float g, float b, float width);
    static int setHelperBrush(int modelId, bool enabled, float r, float g, float b);
    static int drawHelperSphere(int modelId, float cx, float cy, float cz, float radius, int stackCount,
                                int sectorCount);
    static int drawHelperCylinder(int modelId, float cx, float cy, float cz, float nx, float ny, float nz, float height,
                                  float radius, int resolution);
    static int drawHelperCube(int modelId, float x1, float y1, float z1, float x2, float y2, float z2);
    static int drawHelperCircle(int modelId, float cx, float cy, float cz, float nx, float ny, float nz, float radius,
                                int resolution);
    static int showHelper(int modelId, int handle);
    static int hideHelper(int modelId, int handle);
    static int deleteHelper(int modelId, int handle);
    static int clearHelpers(int modelId);
    static std::string getColorMapJson();
    static std::string getAttributeListJson();
    static void renderFrame();
    static void resetCamera();
    static int setCameraType(int type);
    static int viewXPlus();
    static int viewXMinus();
    static int viewYPlus();
    static int viewYMinus();
    static int viewZPlus();
    static int viewZMinus();
    static int viewIsometric();
    static int toggleAxes(bool visible);
    static int toggleColorBar(bool visible);
    static int setColorBarLayout(float x, float y, float width, float height, int coordinateMode);
    static int setColorBarOptions(int orientation, int numberOfLabels, int maximumNumberOfColors);
    static int setCornerAnnotationText(const std::string& text);
    static int setCornerAnnotationPosition(float left, float top);
    static int setCornerAnnotationAnchorToBottomRight(bool anchorToBottomRight);
    static int setCornerAnnotationVisible(bool visible);
    static int zoom(float factor);
    static void sendMouseEvent(int type, int button, float x, float y, double delta);
};

int Init() {
    DebugLog("INFO", "Init called");
    DebugLog("INFO", "Init pre-check scene=" + std::string(g_scene != nullptr ? "ready" : "null") +
                             " window=" + std::string(g_window != nullptr ? "ready" : "null") +
                             " interactor=" + std::string(g_interactor != nullptr ? "ready" : "null"));

    EnsureScene();

    DebugLog("INFO", "Init post-check scene=" + std::string(g_scene != nullptr ? "ready" : "null") +
                             " window=" + std::string(g_window != nullptr ? "ready" : "null") +
                             " interactor=" + std::string(g_interactor != nullptr ? "ready" : "null"));

    if (g_window == nullptr) { return FailWithError(0, "Init", "window is null after EnsureScene"); }

    if (g_window->GetRawWindowPtr() == nullptr) {
        return FailWithError(0, "Init", "raw window pointer is null after EnsureScene");
    }

    if (g_scene != nullptr && g_window != nullptr && g_window->GetRawWindowPtr() != nullptr) {
        ClearLastError();
        DebugLog("INFO", "Init success");
        return 1;
    }
    return FailWithError(0, "Init", "scene/window/raw window pointer init failed");
}

void SetDebugEnabled(bool enabled) {
    g_debugEnabled = enabled;
    DebugLog("INFO", std::string("SetDebugEnabled=") + (enabled ? "true" : "false"));
}

int RaiseTestError(const std::string& detail, int code) {
    const int finalCode = (code == 0) ? -999 : code;
    const std::string finalDetail = detail.empty() ? "manual wasm test error from html" : detail;
    return FailWithError(finalCode, "RaiseTestError", finalDetail);
}

int SetSize(int width, int height) {
    DebugLog("INFO", "SetSize called width=" + std::to_string(width) + " height=" + std::to_string(height));

    EnsureScene();
    if (g_window == nullptr) { return FailWithError(0, "SetSize", "window is null"); }

    if (width <= 0 || height <= 0) {
        return FailWithError(0, "SetSize",
                             "invalid size width=" + std::to_string(width) + " height=" + std::to_string(height));
    }

    g_window->SetSize(width, height);
    ClearLastError();
    return 1;
}

int LoadVtkFromMem(const std::string& bytes) {
    DebugLog("INFO", "LoadVtkFromMem called bytes=" + std::to_string(bytes.size()));
    if (bytes.empty()) return FailWithError(0, "LoadVtkFromMem", "empty input bytes");

    EnsureScene();

    auto dataObj = iGame::FileIO::ReadVTKFromMemory(bytes.data(), static_cast<size_t>(bytes.size()));
    if (dataObj == nullptr) return FailWithError(0, "LoadVtkFromMem", "ReadVTKFromMemory returned null");

    return AddModelFromDataObject(dataObj, "Imported VTK", false, "memory-vtk");
}

int LoadVtuFromMem(const std::string& bytes) {
    DebugLog("INFO", "LoadVtuFromMem called bytes=" + std::to_string(bytes.size()));
    if (bytes.empty()) return FailWithError(0, "LoadVtuFromMem", "empty input bytes");

    EnsureScene();

    auto dataObj = iGame::FileIO::ReadVTUFromMemory(bytes.data(), static_cast<size_t>(bytes.size()));
    if (dataObj == nullptr) return FailWithError(0, "LoadVtuFromMem", "ReadVTUFromMemory returned null");

    LogVtuSummary(dataObj, "memory-vtu");

    return AddModelFromDataObject(dataObj, "Imported VTU", false, "memory-vtu");
}

int LoadVtpFromMem(const std::string& bytes) {
    DebugLog("INFO", "LoadVtpFromMem called bytes=" + std::to_string(bytes.size()));
    if (bytes.empty()) return FailWithError(0, "LoadVtpFromMem", "empty input bytes");

    EnsureScene();

    auto dataObj = iGame::FileIO::ReadVTPFromMemory(bytes.data(), static_cast<size_t>(bytes.size()));
    if (dataObj == nullptr) return FailWithError(0, "LoadVtpFromMem", "ReadVTPFromMemory returned null");

    return AddModelFromDataObject(dataObj, "Imported VTP", false, "memory-vtp");
}

int LoadIgcFromMem(const std::string& bytes) {
    DebugLog("INFO", "LoadIgcFromMem called bytes=" + std::to_string(bytes.size()));
    if (bytes.empty()) return FailWithError(0, "LoadIgcFromMem", "empty input bytes");

    EnsureScene();

    auto dataObj =
            iGame::FileIO::ReadIGCFromMemory(bytes.data(), static_cast<size_t>(bytes.size()), iGame::IGCLayout::Native);
    if (dataObj == nullptr) return FailWithError(0, "LoadIgcFromMem", "ReadIGCFromMemory returned null");

    return AddModelFromDataObject(dataObj, "Imported IGC", false, "memory-igc");
}

int LoadIgcLegacy32FromMem(const std::string& bytes) {
    DebugLog("INFO", "LoadIgcLegacy32FromMem called bytes=" + std::to_string(bytes.size()));
    if (bytes.empty()) return FailWithError(0, "LoadIgcLegacy32FromMem", "empty input bytes");

    EnsureScene();

    auto dataObj = iGame::FileIO::ReadIGCFromMemory(bytes.data(), static_cast<size_t>(bytes.size()),
                                                    iGame::IGCLayout::Legacy32);
    if (dataObj == nullptr) return FailWithError(0, "LoadIgcLegacy32FromMem", "ReadIGCFromMemory returned null");

    return AddModelFromDataObject(dataObj, "Imported IGC", false, "memory-igc-legacy32");
}

int LoadIgcLegacy64FromMem(const std::string& bytes) {
    DebugLog("INFO", "LoadIgcLegacy64FromMem called bytes=" + std::to_string(bytes.size()));
    if (bytes.empty()) return FailWithError(0, "LoadIgcLegacy64FromMem", "empty input bytes");

    EnsureScene();

    auto dataObj = iGame::FileIO::ReadIGCFromMemory(bytes.data(), static_cast<size_t>(bytes.size()),
                                                    iGame::IGCLayout::Legacy64);
    if (dataObj == nullptr) return FailWithError(0, "LoadIgcLegacy64FromMem", "ReadIGCFromMemory returned null");

    return AddModelFromDataObject(dataObj, "Imported IGC", false, "memory-igc-legacy64");
}

int LoadVtkFromMemEx(const std::string& bytes, const std::string& sourceName, bool replaceExisting) {
    DebugLog("INFO", "LoadVtkFromMemEx called bytes=" + std::to_string(bytes.size()) + " sourceName=" + sourceName +
                             " replaceExisting=" + (replaceExisting ? "true" : "false"));
    if (bytes.empty()) return FailWithError(0, "LoadVtkFromMemEx", "empty input bytes");

    EnsureScene();

    auto dataObj = iGame::FileIO::ReadVTKFromMemory(bytes.data(), static_cast<size_t>(bytes.size()));
    if (dataObj == nullptr) return FailWithError(0, "LoadVtkFromMemEx", "ReadVTKFromMemory returned null");

    return AddModelFromDataObject(dataObj, sourceName.c_str(), replaceExisting, "memory-vtk");
}

int LoadVtuFromMemEx(const std::string& bytes, const std::string& sourceName, bool replaceExisting) {
    DebugLog("INFO", "LoadVtuFromMemEx called bytes=" + std::to_string(bytes.size()) + " sourceName=" + sourceName +
                             " replaceExisting=" + (replaceExisting ? "true" : "false"));
    if (bytes.empty()) return FailWithError(0, "LoadVtuFromMemEx", "empty input bytes");

    EnsureScene();

    auto dataObj = iGame::FileIO::ReadVTUFromMemory(bytes.data(), static_cast<size_t>(bytes.size()));
    if (dataObj == nullptr) return FailWithError(0, "LoadVtuFromMemEx", "ReadVTUFromMemory returned null");

    LogVtuSummary(dataObj, sourceName);

    return AddModelFromDataObject(dataObj, sourceName.c_str(), replaceExisting, "memory-vtu");
}

int LoadVtpFromMemEx(const std::string& bytes, const std::string& sourceName, bool replaceExisting) {
    DebugLog("INFO", "LoadVtpFromMemEx called bytes=" + std::to_string(bytes.size()) + " sourceName=" + sourceName +
                             " replaceExisting=" + (replaceExisting ? "true" : "false"));
    if (bytes.empty()) return FailWithError(0, "LoadVtpFromMemEx", "empty input bytes");

    EnsureScene();

    auto dataObj = iGame::FileIO::ReadVTPFromMemory(bytes.data(), static_cast<size_t>(bytes.size()));
    if (dataObj == nullptr) return FailWithError(0, "LoadVtpFromMemEx", "ReadVTPFromMemory returned null");

    return AddModelFromDataObject(dataObj, sourceName.c_str(), replaceExisting, "memory-vtp");
}

int LoadIgcFromMemEx(const std::string& bytes, const std::string& sourceName, bool replaceExisting) {
    DebugLog("INFO", "LoadIgcFromMemEx called bytes=" + std::to_string(bytes.size()) + " sourceName=" + sourceName +
                             " replaceExisting=" + (replaceExisting ? "true" : "false"));
    if (bytes.empty()) return FailWithError(0, "LoadIgcFromMemEx", "empty input bytes");

    EnsureScene();

    auto dataObj =
            iGame::FileIO::ReadIGCFromMemory(bytes.data(), static_cast<size_t>(bytes.size()), iGame::IGCLayout::Native);
    if (dataObj == nullptr) return FailWithError(0, "LoadIgcFromMemEx", "ReadIGCFromMemory returned null");

    return AddModelFromDataObject(dataObj, sourceName.c_str(), replaceExisting, "memory-igc");
}

int LoadIgcLegacy32FromMemEx(const std::string& bytes, const std::string& sourceName, bool replaceExisting) {
    DebugLog("INFO", "LoadIgcLegacy32FromMemEx called bytes=" + std::to_string(bytes.size()) +
                             " sourceName=" + sourceName + " replaceExisting=" + (replaceExisting ? "true" : "false"));
    if (bytes.empty()) return FailWithError(0, "LoadIgcLegacy32FromMemEx", "empty input bytes");

    EnsureScene();

    auto dataObj = iGame::FileIO::ReadIGCFromMemory(bytes.data(), static_cast<size_t>(bytes.size()),
                                                    iGame::IGCLayout::Legacy32);
    if (dataObj == nullptr) return FailWithError(0, "LoadIgcLegacy32FromMemEx", "ReadIGCFromMemory returned null");

    return AddModelFromDataObject(dataObj, sourceName.c_str(), replaceExisting, "memory-igc-legacy32");
}

int LoadIgcLegacy64FromMemEx(const std::string& bytes, const std::string& sourceName, bool replaceExisting) {
    DebugLog("INFO", "LoadIgcLegacy64FromMemEx called bytes=" + std::to_string(bytes.size()) +
                             " sourceName=" + sourceName + " replaceExisting=" + (replaceExisting ? "true" : "false"));
    if (bytes.empty()) return FailWithError(0, "LoadIgcLegacy64FromMemEx", "empty input bytes");

    EnsureScene();

    auto dataObj = iGame::FileIO::ReadIGCFromMemory(bytes.data(), static_cast<size_t>(bytes.size()),
                                                    iGame::IGCLayout::Legacy64);
    if (dataObj == nullptr) return FailWithError(0, "LoadIgcLegacy64FromMemEx", "ReadIGCFromMemory returned null");

    return AddModelFromDataObject(dataObj, sourceName.c_str(), replaceExisting, "memory-igc-legacy64");
}

int LoadIgcmFromFileEx(const std::string& filePath, const std::string& sourceName, bool replaceExisting) {
    DebugLog("INFO", "LoadIgcmFromFileEx called filePath=" + filePath + " sourceName=" + sourceName +
                             " replaceExisting=" + (replaceExisting ? "true" : "false"));
    if (filePath.empty()) return FailWithError(0, "LoadIgcmFromFileEx", "empty manifest path");

    EnsureScene();

    auto dataObj = iGame::FileIO::ReadFile(filePath);
    if (dataObj == nullptr) return FailWithError(0, "LoadIgcmFromFileEx", "ReadFile returned null for IGCM");

    return AddModelFromDataObject(dataObj, sourceName.c_str(), replaceExisting, "memory-igcm");
}

int LoadZipFromMemEx(const std::string& bytes, const std::string& sourceName, bool replaceExisting) {
    DebugLog("INFO", "LoadZipFromMemEx called bytes=" + std::to_string(bytes.size()) + " sourceName=" + sourceName +
                             " replaceExisting=" + (replaceExisting ? "true" : "false"));
    if (bytes.empty()) return FailWithError(0, "LoadZipFromMemEx", "empty input bytes");

    EnsureScene();

    std::vector<ZipEntryInfo> entries;
    std::string parseError;
    if (!ParseZipEntries(bytes, entries, parseError)) { return FailWithError(0, "LoadZipFromMemEx", parseError); }

    std::vector<std::string> manifestEntryNames;
    for (const auto& entry: entries) {
        if (IsZipDirectoryEntry(entry.name)) { continue; }
        std::string relativePath;
        if (!NormalizeZipRelativePath(entry.name, relativePath)) { continue; }
        if (EndsWithCaseInsensitive(relativePath, ".igcm")) { manifestEntryNames.emplace_back(relativePath); }
    }

    if (!manifestEntryNames.empty()) {
        static int zipMountCounter = 0;
        const std::string mountRoot = "/tmp/igame_zip_" + std::to_string(++zipMountCounter);
        std::vector<unsigned char> mountedBytes;
        for (const auto& entry: entries) {
            if (IsZipDirectoryEntry(entry.name)) { continue; }

            std::string relativePath;
            if (!NormalizeZipRelativePath(entry.name, relativePath)) {
                DebugLog("WARN", "skipping unsafe zip entry path: " + entry.name);
                continue;
            }

            std::string extractError;
            if (!ExtractZipEntry(bytes, entry, mountedBytes, extractError)) {
                return FailWithError(0, "LoadZipFromMemEx", "entry " + entry.name + ": " + extractError);
            }

            std::string writeError;
            const std::string mountedPath = JoinZipMountPath(mountRoot, relativePath);
            if (!WriteFileBytes(mountedPath, mountedBytes, writeError)) {
                return FailWithError(0, "LoadZipFromMemEx", writeError);
            }
        }

        int loadedManifestCount = 0;
        bool shouldReplaceManifest = replaceExisting;
        for (const auto& manifestName: manifestEntryNames) {
            const std::string manifestPath = JoinZipMountPath(mountRoot, manifestName);
            const std::string entrySourceName = DescribeZipEntry(sourceName, manifestName);
            const int ret = LoadIgcmFromFileEx(manifestPath, entrySourceName, shouldReplaceManifest);
            if (ret <= 0) {
                return FailWithError(0, "LoadZipFromMemEx",
                                     "failed to load IGCM manifest " + manifestName + ": " + g_lastError.detail);
            }
            ++loadedManifestCount;
            shouldReplaceManifest = false;
        }

        ClearLastError();
        DebugLog("INFO", "LoadZipFromMemEx success loaded IGCM manifests=" + std::to_string(loadedManifestCount));
        return loadedManifestCount;
    }

    int loadedCount = 0;
    bool shouldReplace = replaceExisting;
    std::vector<unsigned char> extractedBytes;
    for (const auto& entry: entries) {
        const std::string entryFileName = NormalizeEntryFileName(entry.name);
        if (!IsSupportedModelEntry(entryFileName)) { continue; }

        std::string extractError;
        if (!ExtractZipEntry(bytes, entry, extractedBytes, extractError)) {
            return FailWithError(0, "LoadZipFromMemEx", "entry " + entry.name + ": " + extractError);
        }

        const std::string entrySourceName = DescribeZipEntry(sourceName, entry.name);
        const std::string extracted(reinterpret_cast<const char*>(extractedBytes.data()), extractedBytes.size());
        int ret = 0;
        if (EndsWithCaseInsensitive(entryFileName, ".vtp")) {
            ret = LoadVtpFromMemEx(extracted, entrySourceName, shouldReplace);
        } else if (EndsWithCaseInsensitive(entryFileName, ".vtu")) {
            ret = LoadVtuFromMemEx(extracted, entrySourceName, shouldReplace);
        } else if (EndsWithCaseInsensitive(entryFileName, ".igc")) {
            ret = LoadIgcFromMemEx(extracted, entrySourceName, shouldReplace);
        } else if (EndsWithCaseInsensitive(entryFileName, ".igcm")) {
            return FailWithError(0, "LoadZipFromMemEx",
                                 "IGCM manifests must be loaded with their referenced IGC files in the same zip");
        } else {
            ret = LoadVtkFromMemEx(extracted, entrySourceName, shouldReplace);
        }

        if (ret <= 0) {
            return FailWithError(0, "LoadZipFromMemEx",
                                 "failed to load entry " + entry.name + ": " + g_lastError.detail);
        }

        ++loadedCount;
        shouldReplace = false;
    }

    if (loadedCount == 0) {
        return FailWithError(0, "LoadZipFromMemEx", "no supported .vtk/.vtu/.vtp/.igc/.igcm entries found");
    }

    ClearLastError();
    DebugLog("INFO", "LoadZipFromMemEx success loadedCount=" + std::to_string(loadedCount));
    return loadedCount;
}

std::string GetModelListJson() {
    static std::string json;
    EnsureScene();

    if (g_scene == nullptr) {
        FailWithError(0, "GetModelListJson", "scene is null");
        return "[]";
    }

    json = "[";
    bool first = true;
    for (auto it = g_modelRegistry.begin(); it != g_modelRegistry.end(); ++it) {
        const auto modelId = it->first;
        auto& meta = it->second;

        auto model = g_scene->GetModelById(static_cast<int>(modelId));
        if (model == nullptr) { continue; }
        meta.visible = model->GetVisibility();

        if (!first) { json += ","; }
        first = false;

        json += "{";
        json += "\"modelId\":" + std::to_string(modelId) + ",";
        json += "\"name\":\"" + EscapeJsonString(meta.name) + "\",";
        json += "\"visible\":" + std::string(meta.visible ? "true" : "false") + ",";
        json += "\"isActive\":" + std::string(modelId == g_activeModelId ? "true" : "false") + ",";
        json += "\"sourceType\":\"" + EscapeJsonString(meta.sourceType) + "\",";
        json += "\"loadTime\":" + std::to_string(meta.loadTime);
        json += "}";
    }
    json += "]";
    ClearLastError();
    return json;
}

int SetActiveModel(int modelId) {
    EnsureScene();
    if (g_scene == nullptr) return FailWithError(0, "SetActiveModel", "scene is null");

    auto model = g_scene->GetModelById(modelId);
    if (model == nullptr) return FailWithError(0, "SetActiveModel", "model not found id=" + std::to_string(modelId));

    g_scene->SetCurrentModel(modelId);
    g_activeModelId = static_cast<IGuint>(modelId);
    if (!RebindCurrentSelectionMode("SetActiveModel")) return 0;
    g_scene->Update();
    ClearLastError();
    return 1;
}

int SetModelVisibility(int modelId, bool visible) {
    EnsureScene();
    if (g_scene == nullptr) return FailWithError(0, "SetModelVisibility", "scene is null");

    auto model = g_scene->GetModelById(modelId);
    if (model == nullptr) {
        return FailWithError(0, "SetModelVisibility", "model not found id=" + std::to_string(modelId));
    }

    g_scene->ChangeModelVisibility(modelId, visible);
    auto it = g_modelRegistry.find(static_cast<IGuint>(modelId));
    if (it != g_modelRegistry.end()) { it->second.visible = visible; }
    g_scene->Update();
    ClearLastError();
    return 1;
}

int RemoveModel(int modelId) {
    EnsureScene();
    if (g_scene == nullptr) return FailWithError(0, "RemoveModel", "scene is null");

    auto model = g_scene->GetModelById(modelId);
    if (model == nullptr) return FailWithError(0, "RemoveModel", "model not found id=" + std::to_string(modelId));

    g_scene->RemoveModel(static_cast<IGuint>(modelId));
    g_modelRegistry.erase(static_cast<IGuint>(modelId));
    SyncActiveModelIdFromScene();
    RebindCurrentSelectionMode("RemoveModel");
    g_scene->Update();
    ClearLastError();
    return 1;
}

int RemoveAllModels() {
    EnsureScene();
    if (g_scene == nullptr) return FailWithError(0, "RemoveAllModels", "scene is null");

    RemoveAllUserModels();
    RebindCurrentSelectionMode("RemoveAllModels");
    g_scene->Update();
    ClearLastError();
    return 1;
}

int ClearViewer() { return RemoveAllModels(); }

int SetSelectionMode(int mode) {
    EnsureScene();
    if (mode < 0 || mode > 2) {
        return FailWithError(0, "SetSelectionMode", "unknown selection mode=" + std::to_string(mode));
    }
    if (mode != 0 && g_slicingActive) {
        ExitSlicingModeInternal(false);
    }
    if (!BindSelectionMode(mode, "SetSelectionMode", false)) return 0;
    g_selectionMode = mode;
    g_scene->Update();
    ClearLastError();
    return 1;
}

int GetSelectionMode() { return g_selectionMode; }

std::string GetSelectionJson(int modelId) {
    EnsureScene();
    if (g_scene == nullptr) {
        FailWithError(0, "GetSelectionJson", "scene is null");
        return "{}";
    }

    auto model = g_scene->GetModelById(modelId);
    if (model == nullptr) {
        FailWithError(0, "GetSelectionJson", "model not found id=" + std::to_string(modelId));
        return "{}";
    }

    auto selection = model->GetSelection();
    if (selection == nullptr) {
        FailWithError(0, "GetSelectionJson", "model does not support selection id=" + std::to_string(modelId));
        return "{}";
    }

    auto appendIds = [](std::string& json, const std::set<igIndex>& ids) {
        bool first = true;
        for (const auto id: ids) {
            if (!first) json += ",";
            first = false;
            json += std::to_string(id);
        }
    };

    std::string json = "{\"modelId\":" + std::to_string(modelId) + ",\"pointIds\":[";
    appendIds(json, selection->GetSelectedPoints());
    json += "],\"cellIds\":[";
    appendIds(json, selection->GetSelectedCells());
    json += "]}";
    ClearLastError();
    return json;
}

int ClearSelection(int modelId) {
    EnsureScene();
    if (g_scene == nullptr) return FailWithError(0, "ClearSelection", "scene is null");

    auto model = g_scene->GetModelById(modelId);
    if (model == nullptr) {
        return FailWithError(0, "ClearSelection", "model not found id=" + std::to_string(modelId));
    }
    auto selection = model->GetSelection();
    if (selection == nullptr) {
        return FailWithError(0, "ClearSelection", "model does not support selection id=" + std::to_string(modelId));
    }

    selection->ClearSelections();
    g_scene->Update();
    ClearLastError();
    return 1;
}

int SetViewStyle(int styleMask) {
    EnsureScene();
    auto model = GetActiveModel();
    if (model == nullptr) return FailWithError(0, "SetViewStyle", "active model is null");

    auto drawObj = iGame::DynamicCast<iGame::DrawObject>(model->GetDataObject());
    if (drawObj == nullptr) return FailWithError(0, "SetViewStyle", "draw object is null");

    drawObj->SetViewStyle(static_cast<IGenum>(styleMask));
    g_scene->Update();
    ClearLastError();
    return 1;
}

int SetViewStyle(int modelId, int styleMask) {
    EnsureScene();
    if (g_scene == nullptr) return FailWithError(0, "SetViewStyle", "scene is null");

    auto model = g_scene->GetModelById(modelId);
    if (model == nullptr) return FailWithError(0, "SetViewStyle", "model not found id=" + std::to_string(modelId));

    auto drawObj = iGame::DynamicCast<iGame::DrawObject>(model->GetDataObject());
    if (drawObj == nullptr) return FailWithError(0, "SetViewStyle", "draw object is null");

    drawObj->SetViewStyle(static_cast<IGenum>(styleMask));
    g_scene->Update();
    ClearLastError();
    return 1;
}

int GetViewStyle(int modelId) {
    EnsureScene();
    if (g_scene == nullptr) { return FailWithError(-1, "GetViewStyle", "scene is null"); }
    auto model = g_scene->GetModelById(modelId);
    if (model == nullptr) return FailWithError(-1, "GetViewStyle", "model not found id=" + std::to_string(modelId));
    auto drawObj = iGame::DynamicCast<iGame::DrawObject>(model->GetDataObject());
    if (drawObj == nullptr) return FailWithError(-2, "GetViewStyle", "draw object is null");
    const int styleMask = static_cast<int>(drawObj->GetViewStyle());
    ClearLastError();
    return styleMask;
}

int SetActorOpacity(int modelId, float opacity) {
    EnsureScene();
    if (g_scene == nullptr) return FailWithError(0, "SetActorOpacity", "scene is null");

    if (opacity < 0.0f || opacity > 1.0f || !std::isfinite(opacity)) {
        return FailWithError(0, "SetActorOpacity", "opacity must be a finite value between 0 and 1");
    }

    auto model = g_scene->GetModelById(modelId);
    if (model == nullptr) return FailWithError(0, "SetActorOpacity", "model not found id=" + std::to_string(modelId));

    auto drawObj = iGame::DynamicCast<iGame::DrawObject>(model->GetDataObject());
    if (drawObj == nullptr) return FailWithError(0, "SetActorOpacity", "draw object is null");

    drawObj->SetTransparency(opacity);
    drawObj->Modified();
    g_scene->Update();
    ClearLastError();
    return 1;
}

float GetActorOpacity(int modelId) {
    EnsureScene();
    if (g_scene == nullptr) {
        FailWithError(0, "GetActorOpacity", "scene is null");
        return -1.0f;
    }

    auto model = g_scene->GetModelById(modelId);
    if (model == nullptr) {
        FailWithError(0, "GetActorOpacity", "model not found id=" + std::to_string(modelId));
        return -1.0f;
    }

    auto drawObj = iGame::DynamicCast<iGame::DrawObject>(model->GetDataObject());
    if (drawObj == nullptr) {
        FailWithError(0, "GetActorOpacity", "draw object is null");
        return -1.0f;
    }

    ClearLastError();
    return drawObj->GetTransparency();
}

int SetScalarField(int attributeIndex, int dimension, int dataLocation) {
    EnsureScene();
    auto model = GetActiveModel();
    if (model == nullptr) return FailWithError(-1, "SetScalarField", "active model is null");

    auto drawObj = iGame::DynamicCast<iGame::DrawObject>(model->GetDataObject());
    if (drawObj == nullptr) return FailWithError(-2, "SetScalarField", "draw object is null");

    if (attributeIndex < 0) {
        ForEachDrawObjectInTree(model->GetDataObject(), [](iGame::DrawObject::Pointer obj) {
            auto mapper = obj->GetColorMapper();
            if (mapper != nullptr) { mapper->SetRangeStable(false); }
            obj->ForceReConvertToDrawableData();
        });
        model->ViewCloudPicture(-1, -1);
        ClearLastError();
        return 1;
    }

    auto attrs = drawObj->GetAttributeSet()->GetAllAttributes();
    if (attrs == nullptr || attributeIndex >= static_cast<int>(attrs->GetNumberOfElements())) {
        return FailWithError(-3, "SetScalarField",
                             "attribute index out of range index=" + std::to_string(attributeIndex));
    }

    auto attr = attrs->GetElement(attributeIndex);
    if (attr.isDeleted || attr.pointer == nullptr) {
        return FailWithError(-3, "SetScalarField",
                             "attribute is deleted or null index=" + std::to_string(attributeIndex));
    }

    const int attrDim = attr.pointer->GetDimension();
    if (dimension < -1 || (attrDim > 0 && dimension >= attrDim)) {
        return FailWithError(-5, "SetScalarField",
                             "invalid dimension dim=" + std::to_string(dimension) +
                                     " attrDim=" + std::to_string(attrDim));
    }

    if (dataLocation == IG_POINT || dataLocation == IG_CELL) {
        if (attr.attachmentType != dataLocation) {
            return FailWithError(-4, "SetScalarField",
                                 "attachment mismatch request=" + std::to_string(dataLocation) +
                                         " actual=" + std::to_string(attr.attachmentType));
        }
    }

    ForEachDrawObjectInTree(model->GetDataObject(),
                            [&](iGame::DrawObject::Pointer obj) { ResetAutoRange(obj, attributeIndex, dimension); });
    model->ViewCloudPicture(attributeIndex, dimension);
    DisableModelSolidMode(static_cast<IGuint>(g_activeModelId));
    if (g_scene != nullptr) {
        auto colorBar = g_scene->GetColorBar2DActor();
        if (colorBar != nullptr) {
            colorBar->SetColorMapper(drawObj->GetColorMapper());
            g_scene->SetColorBarVisible(true);
        }
    }
    ClearLastError();
    return 1;
}

int SetSurfaceShadingMode(int mode) {
    EnsureScene();
    if (g_scene == nullptr) return FailWithError(0, "SetSurfaceShadingMode", "scene is null");
    g_scene->SetSurfaceShadingMode(mode);
    g_scene->Update();
    ClearLastError();
    return 1;
}

int SetBackgroundColor(float r, float g, float b) {
    EnsureScene();
    if (g_scene == nullptr) return FailWithError(0, "SetBackgroundColor", "scene is null");

    if (!std::isfinite(r)) r = 0.0f;
    if (!std::isfinite(g)) g = 0.0f;
    if (!std::isfinite(b)) b = 0.0f;
    r = std::max(0.0f, std::min(1.0f, r));
    g = std::max(0.0f, std::min(1.0f, g));
    b = std::max(0.0f, std::min(1.0f, b));

    g_scene->SetBackGround(static_cast<int>(std::round(r * 255.0f)), static_cast<int>(std::round(g * 255.0f)),
                           static_cast<int>(std::round(b * 255.0f)));
    g_scene->Update();
    ClearLastError();
    return 1;
}

int SetSceneBackgroundColor(float r, float g, float b) { return SetBackgroundColor(r, g, b); }

int SetSceneBackgroundGradientColor(float r1, float g1, float b1, float r2, float g2, float b2, int mode) {
    EnsureScene();
    if (g_scene == nullptr) { return FailWithError(0, "SetSceneBackgroundGradientColor", "scene is null"); }

    if (!std::isfinite(r1)) r1 = 0.0f;
    if (!std::isfinite(g1)) g1 = 0.0f;
    if (!std::isfinite(b1)) b1 = 0.0f;
    if (!std::isfinite(r2)) r2 = 0.0f;
    if (!std::isfinite(g2)) g2 = 0.0f;
    if (!std::isfinite(b2)) b2 = 0.0f;
    r1 = std::max(0.0f, std::min(1.0f, r1));
    g1 = std::max(0.0f, std::min(1.0f, g1));
    b1 = std::max(0.0f, std::min(1.0f, b1));
    r2 = std::max(0.0f, std::min(1.0f, r2));
    g2 = std::max(0.0f, std::min(1.0f, g2));
    b2 = std::max(0.0f, std::min(1.0f, b2));
    mode = std::max(0, std::min(2, mode));

    g_scene->SetBackGroundGradient(static_cast<int>(std::round(r1 * 255.0f)), static_cast<int>(std::round(g1 * 255.0f)),
                                   static_cast<int>(std::round(b1 * 255.0f)), static_cast<int>(std::round(r2 * 255.0f)),
                                   static_cast<int>(std::round(g2 * 255.0f)), static_cast<int>(std::round(b2 * 255.0f)),
                                   mode);
    g_scene->Update();
    ClearLastError();
    return 1;
}

std::string GetAttributeListJson() {
    static std::string json;

    EnsureScene();
    auto model = GetActiveModel();
    if (model == nullptr) {
        FailWithError(0, "GetAttributeListJson", "active model is null");
        return "[]";
    }

    auto drawObj = iGame::DynamicCast<iGame::DrawObject>(model->GetDataObject());
    if (drawObj == nullptr) {
        FailWithError(0, "GetAttributeListJson", "draw object is null");
        return "[]";
    }

    auto attrs = drawObj->GetAttributeSet()->GetAllAttributes();
    if (attrs == nullptr) {
        FailWithError(0, "GetAttributeListJson", "attributes set is null");
        return "[]";
    }

    json = "[";
    bool first = true;
    const auto n = static_cast<int>(attrs->GetNumberOfElements());
    for (int i = 0; i < n; ++i) {
        const auto& attr = attrs->GetElement(i);
        if (attr.isDeleted || attr.pointer == nullptr) { continue; }

        if (!first) { json += ","; }
        first = false;

        const std::string name = EscapeJsonString(attr.pointer->GetName());
        const int dim = attr.pointer->GetDimension();

        json += "{";
        json += "\"index\":" + std::to_string(i) + ",";
        json += "\"name\":\"" + name + "\",";
        json += "\"type\":" + std::to_string(static_cast<int>(attr.type)) + ",";
        json += "\"attachment\":" + std::to_string(static_cast<int>(attr.attachmentType)) + ",";
        json += "\"dimension\":" + std::to_string(dim);
        json += "}";
    }
    json += "]";
    ClearLastError();
    return json;
}

std::string DescribeColorBarState() {
    if (g_scene == nullptr) { return "scene=null"; }

    std::string state = "scene=ready";
    auto colorBar = g_scene->GetColorBar2DActor();
    state += " colorBar=" + std::string(colorBar != nullptr ? "ready" : "null");
    if (colorBar != nullptr) {
        state += " visible=" + std::string(colorBar->GetVisible() ? "true" : "false");
        state += " coordinateMode=" + std::to_string(static_cast<int>(colorBar->GetCoordinateMode()));
        state += " orientation=" + std::to_string(static_cast<int>(colorBar->GetOrientation()));
        state += " labels=" + std::to_string(colorBar->GetNumberOfLabels());
        state += " maxColors=" + std::to_string(colorBar->GetMaximumNumberOfColors());
        state += " title=" + colorBar->GetTitle();
        state += " actorMapper=" + std::string(colorBar->GetColorMapper() != nullptr ? "set" : "null");
    }

    auto model = g_scene->GetCurrentModel();
    state += " currentModel=" + std::string(model != nullptr ? "ready" : "null");
    if (model != nullptr && model->GetDataObject() != nullptr) {
        auto dataObject = model->GetDataObject();
        state += " attrIndex=" + std::to_string(dataObject->GetAttributeIndex());
        state += " dataMapper=" + std::string(dataObject->GetColorMapper() != nullptr ? "set" : "null");
    }

    return state;
}

int SetColorMapFromArrays(const std::vector<float>& colors_flat, const std::vector<float>& ranges) {
    EnsureScene();
    auto model = GetActiveModel();
    if (model == nullptr) return FailWithError(0, "SetColorMapFromArrays", "active model is null");

    if (g_activeModelId != 0) { DisableModelSolidMode(g_activeModelId); }

    auto drawObj = iGame::DynamicCast<iGame::DrawObject>(model->GetDataObject());
    if (drawObj == nullptr) return FailWithError(0, "SetColorMapFromArrays", "draw object is null");

    if (ranges.empty() || colors_flat.empty()) return FailWithError(0, "SetColorMapFromArrays", "empty input arrays");
    const size_t nodes = ranges.size();
    if (colors_flat.size() != nodes * 3)
        return FailWithError(0, "SetColorMapFromArrays", "colors_flat size mismatch (expect nodes*3)");

    auto mapper = drawObj->GetColorMapper();
    if (mapper == nullptr) return FailWithError(0, "SetColorMapFromArrays", "color mapper is null");

    auto Colors = iGame::FloatArray::New();
    Colors->SetDimension(3);
    Colors->Resize(static_cast<IGsize>(nodes));
    for (size_t i = 0; i < nodes; ++i) {
        float cell[3] = {colors_flat[i * 3 + 0], colors_flat[i * 3 + 1], colors_flat[i * 3 + 2]};
        Colors->SetElement(static_cast<IGsize>(i), cell);
    }

    auto ColorRange = iGame::FloatArray::New();
    ColorRange->SetDimension(1);
    ColorRange->Resize(static_cast<IGsize>(nodes));
    for (size_t i = 0; i < nodes; ++i) { ColorRange->SetValue(static_cast<IGsize>(i), ranges[i]); }

    mapper->SetColorMap(Colors, ColorRange);
    mapper->Modified();
    if (g_scene) g_scene->Update();
    ClearLastError();
    DebugLog("INFO",
             "SetColorMapFromArrays success nodes=" + std::to_string(nodes) + " state=" + DescribeColorBarState());
    return 1;
}

int SetColorMapByName(int modelId, const std::string& scalarName, int dimension, int dataLocation,
                      const std::vector<float>& colors_flat, const std::vector<float>& ranges) {
    EnsureScene();
    auto model = g_scene->GetModelById(modelId);
    if (model == nullptr) return FailWithError(0, "SetColorMapByName", "model not found id=" + std::to_string(modelId));

    auto drawObj = iGame::DynamicCast<iGame::DrawObject>(model->GetDataObject());
    if (drawObj == nullptr) return FailWithError(0, "SetColorMapByName", "draw object is null");

    if (scalarName == "Solid") {
        const auto solidColor = GetModelSolidColor(static_cast<IGuint>(modelId), drawObj);
        SetModelSolidMode(static_cast<IGuint>(modelId), solidColor, true);
        g_activeModelId = static_cast<IGuint>(modelId);
        g_scene->SetCurrentModel(modelId);
        model->ViewCloudPicture(-1, -1);
        auto colorBar = g_scene->GetColorBar2DActor();
        if (colorBar != nullptr) { g_scene->SetColorBarVisible(false); }
        g_scene->Update();
        ClearLastError();
        DebugLog("INFO", "SetColorMapByName solid mode enabled modelId=" + std::to_string(modelId) + " color=(" +
                                 std::to_string(solidColor[0]) + ", " + std::to_string(solidColor[1]) + ", " +
                                 std::to_string(solidColor[2]) + ")");
        return 1;
    }

    auto attrSet = drawObj->GetAttributeSet();
    if (attrSet == nullptr) return FailWithError(0, "SetColorMapByName", "attribute set is null");

    const int attrIndex = attrSet->GetAttributeIndex(scalarName);
    if (attrIndex < 0) { return FailWithError(0, "SetColorMapByName", "attribute not found name=" + scalarName); }

    auto attrs = attrSet->GetAllAttributes();
    if (attrs == nullptr || attrIndex >= static_cast<int>(attrs->GetNumberOfElements())) {
        return FailWithError(0, "SetColorMapByName", "attribute index out of range index=" + std::to_string(attrIndex));
    }

    const auto& attr = attrs->GetElement(attrIndex);
    if (attr.isDeleted || attr.pointer == nullptr) {
        return FailWithError(0, "SetColorMapByName", "attribute is deleted or null index=" + std::to_string(attrIndex));
    }

    const int attrDim = attr.pointer->GetDimension();
    if (dimension < -1 || (attrDim > 0 && dimension >= attrDim)) {
        return FailWithError(0, "SetColorMapByName",
                             "invalid dimension dim=" + std::to_string(dimension) +
                                     " attrDim=" + std::to_string(attrDim));
    }

    // dataLocation 参数在 Web 示例中被忽略，直接使用找到的属性。

    auto mapper = drawObj->GetColorMapper();
    if (mapper == nullptr) return FailWithError(0, "SetColorMapByName", "color mapper is null");
    ForEachDrawObjectInTree(model->GetDataObject(),
                            [&](iGame::DrawObject::Pointer obj) { ResetAutoRange(obj, scalarName, dimension); });

    // Apply scalar selection on this model
    model->ViewCloudPicture(attrIndex, dimension);
    DisableModelSolidMode(static_cast<IGuint>(modelId));

    if (ranges.empty() || colors_flat.empty()) return FailWithError(0, "SetColorMapByName", "empty input arrays");
    const size_t nodes = ranges.size();
    if (colors_flat.size() != nodes * 3)
        return FailWithError(0, "SetColorMapByName", "colors_flat size mismatch (expect nodes*3)");

    auto Colors = iGame::FloatArray::New();
    Colors->SetDimension(3);
    Colors->Resize(static_cast<IGsize>(nodes));
    for (size_t i = 0; i < nodes; ++i) {
        float cell[3] = {colors_flat[i * 3 + 0], colors_flat[i * 3 + 1], colors_flat[i * 3 + 2]};
        Colors->SetElement(static_cast<IGsize>(i), cell);
    }

    auto ColorRange = iGame::FloatArray::New();
    ColorRange->SetDimension(1);
    ColorRange->Resize(static_cast<IGsize>(nodes));
    for (size_t i = 0; i < nodes; ++i) { ColorRange->SetValue(static_cast<IGsize>(i), ranges[i]); }

    mapper->SetColorMap(Colors, ColorRange);
    mapper->Modified();
    if (g_scene) {
        auto colorBar = g_scene->GetColorBar2DActor();
        if (colorBar != nullptr) {
            colorBar->SetColorMapper(mapper);
            g_scene->SetColorBarVisible(true);
        }
        g_scene->Update();
    }

    ClearLastError();
    DebugLog("INFO", "SetColorMapByName success modelId=" + std::to_string(modelId) + " attr=" + scalarName +
                             " index=" + std::to_string(attrIndex));
    return 1;
}

int SetColorMapByNameRange(int modelId, const std::string& scalarName, int dimension, double minValue,
                           double maxValue) {
    EnsureScene();
    if (g_scene == nullptr) return FailWithError(0, "SetColorMapByNameRange", "scene is null");

    auto model = g_scene->GetModelById(modelId);
    if (model == nullptr) {
        return FailWithError(0, "SetColorMapByNameRange", "model not found id=" + std::to_string(modelId));
    }

    auto drawObj = iGame::DynamicCast<iGame::DrawObject>(model->GetDataObject());
    if (drawObj == nullptr) return FailWithError(0, "SetColorMapByNameRange", "draw object is null");

    if (scalarName == "Solid") {
        const auto solidColor = GetModelSolidColor(static_cast<IGuint>(modelId), drawObj);
        SetModelSolidMode(static_cast<IGuint>(modelId), solidColor, true);
        g_activeModelId = static_cast<IGuint>(modelId);
        g_scene->SetCurrentModel(modelId);
        model->ViewCloudPicture(-1, -1);
        auto colorBar = g_scene->GetColorBar2DActor();
        if (colorBar != nullptr) { g_scene->SetColorBarVisible(false); }
        g_scene->Update();
        ClearLastError();
        DebugLog("INFO", "SetColorMapByNameRange solid mode enabled modelId=" + std::to_string(modelId));
        return 1;
    }

    if (!std::isfinite(minValue) || !std::isfinite(maxValue)) {
        return FailWithError(0, "SetColorMapByNameRange", "range values must be finite");
    }
    if (minValue > maxValue) { std::swap(minValue, maxValue); }

    auto attrSet = drawObj->GetAttributeSet();
    if (attrSet == nullptr) return FailWithError(0, "SetColorMapByNameRange", "attribute set is null");

    const int attrIndex = attrSet->GetAttributeIndex(scalarName);
    if (attrIndex < 0) { return FailWithError(0, "SetColorMapByNameRange", "attribute not found name=" + scalarName); }

    auto attrs = attrSet->GetAllAttributes();
    if (attrs == nullptr || attrIndex >= static_cast<int>(attrs->GetNumberOfElements())) {
        return FailWithError(0, "SetColorMapByNameRange",
                             "attribute index out of range index=" + std::to_string(attrIndex));
    }

    const auto& attr = attrs->GetElement(attrIndex);
    if (attr.isDeleted || attr.pointer == nullptr) {
        return FailWithError(0, "SetColorMapByNameRange",
                             "attribute is deleted or null index=" + std::to_string(attrIndex));
    }

    const int attrDim = attr.pointer->GetDimension();
    if (dimension < -1 || (attrDim > 0 && dimension >= attrDim)) {
        return FailWithError(0, "SetColorMapByNameRange",
                             "invalid dimension dim=" + std::to_string(dimension) +
                                     " attrDim=" + std::to_string(attrDim));
    }

    auto mapper = drawObj->GetColorMapper();
    if (mapper == nullptr) return FailWithError(0, "SetColorMapByNameRange", "color mapper is null");

    g_activeModelId = static_cast<IGuint>(modelId);
    g_scene->SetCurrentModel(modelId);
    model->ViewCloudPicture(attrIndex, dimension);
    DisableModelSolidMode(static_cast<IGuint>(modelId));

    ForEachDrawObjectInTree(model->GetDataObject(),
                            [&](iGame::DrawObject::Pointer obj) { ConfigureManualRange(obj, minValue, maxValue); });

    if (g_scene != nullptr) {
        auto colorBar = g_scene->GetColorBar2DActor();
        if (colorBar != nullptr) {
            colorBar->SetColorMapper(mapper);
            g_scene->SetColorBarVisible(true);
        }
        g_scene->Update();
    }

    ClearLastError();
    DebugLog("INFO", "SetColorMapByNameRange success modelId=" + std::to_string(modelId) + " attr=" + scalarName +
                             " index=" + std::to_string(attrIndex) + " range=[" + std::to_string(minValue) + ", " +
                             std::to_string(maxValue) + "]");
    return 1;
}

int SetModelDefaultColor(int modelId, float r, float g, float b) {
    if (g_scene == nullptr) return FailWithError(0, "SetModelDefaultColor", "scene is null");
    auto model = g_scene->GetModelById(modelId);
    if (model == nullptr)
        return FailWithError(0, "SetModelDefaultColor", "model not found id=" + std::to_string(modelId));
    auto drawObj = iGame::DynamicCast<iGame::DrawObject>(model->GetDataObject());
    if (drawObj == nullptr) return FailWithError(0, "SetModelDefaultColor", "draw object is null");

    if (!std::isfinite(r)) r = 0.0f;
    if (!std::isfinite(g)) g = 0.0f;
    if (!std::isfinite(b)) b = 0.0f;
    r = std::max(0.0f, std::min(1.0f, r));
    g = std::max(0.0f, std::min(1.0f, g));
    b = std::max(0.0f, std::min(1.0f, b));

    igm::vec3 color{r, g, b};
    drawObj->SetDefaultColor(color);
    drawObj->Modified();
    g_scene->Update();
    ClearLastError();
    DebugLog("INFO", "SetModelDefaultColor success modelId=" + std::to_string(modelId));
    return 1;
}

int SetSolidColor(int modelId, float r, float g, float b) {
    EnsureScene();
    if (g_scene == nullptr) return FailWithError(0, "SetSolidColor", "scene is null");

    auto model = g_scene->GetModelById(modelId);
    if (model == nullptr) return FailWithError(0, "SetSolidColor", "model not found id=" + std::to_string(modelId));

    auto drawObj = iGame::DynamicCast<iGame::DrawObject>(model->GetDataObject());
    if (drawObj == nullptr) return FailWithError(0, "SetSolidColor", "draw object is null");

    const auto color = ClampColor01(r, g, b);
    auto* meta = FindModelMeta(static_cast<IGuint>(modelId));
    if (meta == nullptr) {
        WebModelMeta newMeta;
        newMeta.id = static_cast<IGuint>(modelId);
        newMeta.name = "model_" + std::to_string(modelId);
        newMeta.visible = true;
        newMeta.sourceType = "unknown";
        newMeta.loadTime = GetUnixTimestampSeconds();
        newMeta.solidColor = color;
        newMeta.solidEnabled = false;
        g_modelRegistry[static_cast<IGuint>(modelId)] = newMeta;
    } else {
        meta->solidColor = color;
    }

    g_scene->Update();

    ClearLastError();
    DebugLog("INFO", "SetSolidColor success modelId=" + std::to_string(modelId));
    return 1;
}

std::string GetSolidColorJson(int modelId) {
    EnsureScene();
    if (g_scene == nullptr) {
        FailWithError(0, "GetSolidColorJson", "scene is null");
        return "{}";
    }

    auto model = g_scene->GetModelById(modelId);
    if (model == nullptr) {
        FailWithError(0, "GetSolidColorJson", "model not found id=" + std::to_string(modelId));
        return "{}";
    }

    auto drawObj = iGame::DynamicCast<iGame::DrawObject>(model->GetDataObject());
    if (drawObj == nullptr) {
        FailWithError(0, "GetSolidColorJson", "draw object is null");
        return "{}";
    }

    const auto color = GetModelSolidColor(static_cast<IGuint>(modelId), drawObj);
    std::ostringstream ss;
    ss.setf(std::ios::fixed);
    ss << std::setprecision(6);

    std::string json = "{";
    json += "\"modelId\":" + std::to_string(modelId) + ",";
    json += "\"enabled\":" + std::string(IsModelSolidModeEnabled(static_cast<IGuint>(modelId)) ? "true" : "false") +
            ",";
    ss.str("");
    ss.clear();
    ss << color[0];
    json += "\"r\":" + ss.str() + ",";
    ss.str("");
    ss.clear();
    ss << color[1];
    json += "\"g\":" + ss.str() + ",";
    ss.str("");
    ss.clear();
    ss << color[2];
    json += "\"b\":" + ss.str();
    json += "}";

    ClearLastError();
    return json;
}

iGame::Painter3D::Pointer GetModelHelperPainter(int modelId, const char* funcName) {
    EnsureScene();
    if (g_scene == nullptr) {
        FailWithError(0, funcName, "scene is null");
        return nullptr;
    }

    auto model = g_scene->GetModelById(modelId);
    if (model == nullptr) {
        FailWithError(0, funcName, "model not found id=" + std::to_string(modelId));
        return nullptr;
    }

    auto painter = model->GetPainter3D();
    if (painter == nullptr) {
        FailWithError(0, funcName, "model painter is null id=" + std::to_string(modelId));
        return nullptr;
    }
    return painter;
}

bool IsFinite3(float x, float y, float z) { return std::isfinite(x) && std::isfinite(y) && std::isfinite(z); }

bool ValidateHelperColor(float r, float g, float b, const char* funcName) {
    if (!IsFinite3(r, g, b)) {
        FailWithError(0, funcName, "color values must be finite");
        return false;
    }
    return true;
}

bool ValidateHelperPoint(float x, float y, float z, const char* funcName, const char* name) {
    if (!IsFinite3(x, y, z)) {
        FailWithError(0, funcName, std::string(name) + " values must be finite");
        return false;
    }
    return true;
}

bool ValidateHelperNormal(float nx, float ny, float nz, const char* funcName) {
    if (!IsFinite3(nx, ny, nz)) {
        FailWithError(0, funcName, "normal values must be finite");
        return false;
    }
    const float length2 = nx * nx + ny * ny + nz * nz;
    if (length2 <= 1e-12f) {
        FailWithError(0, funcName, "normal length must be greater than 0");
        return false;
    }
    return true;
}

bool ValidatePositiveFinite(float value, const char* funcName, const char* name) {
    if (!std::isfinite(value) || value <= 0.0f) {
        FailWithError(0, funcName, std::string(name) + " must be a finite value greater than 0");
        return false;
    }
    return true;
}

bool ValidateMinimumCount(int value, int minimum, const char* funcName, const char* name) {
    if (value < minimum) {
        FailWithError(0, funcName, std::string(name) + " must be greater than or equal to " + std::to_string(minimum));
        return false;
    }
    return true;
}

int SetHelperPen(int modelId, bool enabled, float r, float g, float b, float width) {
    auto painter = GetModelHelperPainter(modelId, "SetHelperPen");
    if (painter == nullptr) { return 0; }

    if (!enabled) {
        painter->SetPen(iGame::Pen::Style::NoPen);
        g_scene->Update();
        ClearLastError();
        return 1;
    }

    if (!ValidateHelperColor(r, g, b, "SetHelperPen") || !ValidatePositiveFinite(width, "SetHelperPen", "width")) {
        return 0;
    }

    const auto color = ClampColor01(r, g, b);
    painter->SetPen(iGame::Pen::Style::SolidLine);
    painter->SetPen(color[0], color[1], color[2]);
    painter->SetPen(width);
    g_scene->Update();
    ClearLastError();
    return 1;
}

int SetHelperBrush(int modelId, bool enabled, float r, float g, float b) {
    auto painter = GetModelHelperPainter(modelId, "SetHelperBrush");
    if (painter == nullptr) { return 0; }

    if (!enabled) {
        painter->SetBrush(iGame::Brush::Style::NoBrush);
        g_scene->Update();
        ClearLastError();
        return 1;
    }

    if (!ValidateHelperColor(r, g, b, "SetHelperBrush")) { return 0; }

    const auto color = ClampColor01(r, g, b);
    painter->SetBrush(iGame::Brush::Style::SolidPattern);
    painter->SetBrush(color[0], color[1], color[2]);
    g_scene->Update();
    ClearLastError();
    return 1;
}

int DrawHelperSphere(int modelId, float cx, float cy, float cz, float radius, int stackCount, int sectorCount) {
    auto painter = GetModelHelperPainter(modelId, "DrawHelperSphere");
    if (painter == nullptr) { return 0; }
    if (!ValidateHelperPoint(cx, cy, cz, "DrawHelperSphere", "center") ||
        !ValidatePositiveFinite(radius, "DrawHelperSphere", "radius") ||
        !ValidateMinimumCount(stackCount, 2, "DrawHelperSphere", "stackCount") ||
        !ValidateMinimumCount(sectorCount, 3, "DrawHelperSphere", "sectorCount")) {
        return 0;
    }

    const IGuint handle = painter->DrawSphere(iGame::Point{cx, cy, cz}, radius, static_cast<unsigned int>(stackCount),
                                              static_cast<unsigned int>(sectorCount));
    g_scene->Update();
    ClearLastError();
    return static_cast<int>(handle);
}

int DrawHelperCylinder(int modelId, float cx, float cy, float cz, float nx, float ny, float nz, float height,
                       float radius, int resolution) {
    auto painter = GetModelHelperPainter(modelId, "DrawHelperCylinder");
    if (painter == nullptr) { return 0; }
    if (!ValidateHelperPoint(cx, cy, cz, "DrawHelperCylinder", "center") ||
        !ValidateHelperNormal(nx, ny, nz, "DrawHelperCylinder") ||
        !ValidatePositiveFinite(height, "DrawHelperCylinder", "height") ||
        !ValidatePositiveFinite(radius, "DrawHelperCylinder", "radius") ||
        !ValidateMinimumCount(resolution, 3, "DrawHelperCylinder", "resolution")) {
        return 0;
    }

    const IGuint handle = painter->DrawCylinder(iGame::Point{cx, cy, cz}, iGame::Vector3f{nx, ny, nz}, height, radius,
                                                static_cast<unsigned int>(resolution));
    g_scene->Update();
    ClearLastError();
    return static_cast<int>(handle);
}

int DrawHelperCube(int modelId, float x1, float y1, float z1, float x2, float y2, float z2) {
    auto painter = GetModelHelperPainter(modelId, "DrawHelperCube");
    if (painter == nullptr) { return 0; }
    if (!ValidateHelperPoint(x1, y1, z1, "DrawHelperCube", "p1") ||
        !ValidateHelperPoint(x2, y2, z2, "DrawHelperCube", "p2")) {
        return 0;
    }

    const IGuint handle = painter->DrawCube(iGame::Point{x1, y1, z1}, iGame::Point{x2, y2, z2});
    g_scene->Update();
    ClearLastError();
    return static_cast<int>(handle);
}

int DrawHelperCircle(int modelId, float cx, float cy, float cz, float nx, float ny, float nz, float radius,
                     int resolution) {
    auto painter = GetModelHelperPainter(modelId, "DrawHelperCircle");
    if (painter == nullptr) { return 0; }
    if (!ValidateHelperPoint(cx, cy, cz, "DrawHelperCircle", "center") ||
        !ValidateHelperNormal(nx, ny, nz, "DrawHelperCircle") ||
        !ValidatePositiveFinite(radius, "DrawHelperCircle", "radius") ||
        !ValidateMinimumCount(resolution, 3, "DrawHelperCircle", "resolution")) {
        return 0;
    }

    const IGuint handle =
            painter->DrawCircle(iGame::Point{cx, cy, cz}, iGame::Vector3f{nx, ny, nz}, radius, resolution);
    g_scene->Update();
    ClearLastError();
    return static_cast<int>(handle);
}

int ShowHelper(int modelId, int handle) {
    auto painter = GetModelHelperPainter(modelId, "ShowHelper");
    if (painter == nullptr) { return 0; }
    if (handle <= 0) { return FailWithError(0, "ShowHelper", "handle must be greater than 0"); }

    painter->Show(static_cast<IGuint>(handle));
    g_scene->Update();
    ClearLastError();
    return 1;
}

int HideHelper(int modelId, int handle) {
    auto painter = GetModelHelperPainter(modelId, "HideHelper");
    if (painter == nullptr) { return 0; }
    if (handle <= 0) { return FailWithError(0, "HideHelper", "handle must be greater than 0"); }

    painter->Hide(static_cast<IGuint>(handle));
    g_scene->Update();
    ClearLastError();
    return 1;
}

int DeleteHelper(int modelId, int handle) {
    auto painter = GetModelHelperPainter(modelId, "DeleteHelper");
    if (painter == nullptr) { return 0; }
    if (handle <= 0) { return FailWithError(0, "DeleteHelper", "handle must be greater than 0"); }

    painter->Delete(static_cast<IGuint>(handle));
    g_scene->Update();
    ClearLastError();
    return 1;
}

int ClearHelpers(int modelId) {
    auto painter = GetModelHelperPainter(modelId, "ClearHelpers");
    if (painter == nullptr) { return 0; }

    painter->Clear();
    g_scene->Update();
    ClearLastError();
    return 1;
}

std::string GetModelDefaultColor(int modelId) {
    EnsureScene();
    if (g_scene == nullptr) {
        FailWithError(0, "GetModelDefaultColor", "scene is null");
        return "{}";
    }

    auto model = g_scene->GetModelById(modelId);
    if (model == nullptr) {
        FailWithError(0, "GetModelDefaultColor", "model not found id=" + std::to_string(modelId));
        return "{}";
    }

    auto drawObj = iGame::DynamicCast<iGame::DrawObject>(model->GetDataObject());
    if (drawObj == nullptr) {
        FailWithError(0, "GetModelDefaultColor", "draw object is null");
        return "{}";
    }

    const auto color = drawObj->GetDefaultColor();
    const auto clampToUnit = [](float value) -> float { return std::max(0.0f, std::min(1.0f, value)); };

    const float r = clampToUnit(color[0]);
    const float g = clampToUnit(color[1]);
    const float b = clampToUnit(color[2]);

    std::ostringstream ss;
    ss.setf(std::ios::fixed);
    ss << std::setprecision(6);

    std::string json = "{";
    json += "\"modelId\":" + std::to_string(modelId) + ",";
    ss.str("");
    ss << r;
    json += "\"r\":" + ss.str() + ",";
    ss.str("");
    ss.clear();
    ss << g;
    json += "\"g\":" + ss.str() + ",";
    ss.str("");
    ss.clear();
    ss << b;
    json += "\"b\":" + ss.str();
    json += "}";

    ClearLastError();
    return json;
}

std::string GetColorMapJson() {
    EnsureScene();
    auto model = GetActiveModel();
    if (model == nullptr) {
        FailWithError(0, "GetColorMapJson", "active model is null");
        return "{}";
    }

    auto drawObj = iGame::DynamicCast<iGame::DrawObject>(model->GetDataObject());
    if (drawObj == nullptr) {
        FailWithError(0, "GetColorMapJson", "draw object is null");
        return "{}";
    }

    auto mapper = drawObj->GetColorMapper();
    if (mapper == nullptr) {
        FailWithError(0, "GetColorMapJson", "color mapper is null");
        return "{}";
    }

    auto colorBar = mapper->GetColorBar();
    auto colorRange = mapper->GetColorRange();
    if (colorBar == nullptr || colorRange == nullptr) {
        FailWithError(0, "GetColorMapJson", "color bar or color range is null");
        return "{}";
    }

    std::string json = "{";
    const int n = static_cast<int>(colorBar->GetNumberOfElements());
    json += "\"size\":" + std::to_string(n) + ",";
    json += "\"colors\":";
    json += "[";
    for (int i = 0; i < n; ++i) {
        auto elem = colorBar->GetElement(i);
        if (i) json += ",";
        json += "[" + std::to_string(elem[0]) + "," + std::to_string(elem[1]) + "," + std::to_string(elem[2]) + "]";
    }
    json += "]";
    json += ",\"ranges\":";
    json += "[";
    for (int i = 0; i < n; ++i) {
        if (i) json += ",";
        json += std::to_string(colorRange->GetValue(i));
    }
    json += "]";
    json += "}";
    ClearLastError();
    return json;
}

void AppendMapperSnapshot(std::ostringstream& out, iGame::DrawObject::Pointer drawObj, const std::string& scalarName,
                          int dimension, int index, bool& first) {
    if (drawObj == nullptr) return;
    auto mapper = drawObj->GetColorMapper();
    if (mapper == nullptr) return;

    if (!first) out << ",";
    first = false;

    const double* range = mapper->GetRange();
    float rgbMin[3]{};
    float rgbMid[3]{};
    float rgbMax[3]{};
    const auto midValue = static_cast<float>((range[0] + range[1]) * 0.5);
    mapper->GetColor(static_cast<float>(range[0]), rgbMin);
    mapper->GetColor(midValue, rgbMid);
    mapper->GetColor(static_cast<float>(range[1]), rgbMax);

    out << "{";
    out << "\"index\":" << index << ",";
    out << "\"name\":\"" << EscapeJsonString(drawObj->GetName()) << "\",";
    out << "\"objectType\":" << drawObj->GetDataObjectType() << ",";
    out << "\"mapper\":\"0x" << std::hex << reinterpret_cast<uintptr_t>(mapper.get()) << std::dec << "\",";
    out << "\"stable\":" << (mapper->GetStable() ? "true" : "false") << ",";
    out << "\"mtime\":" << static_cast<unsigned int>(mapper->GetMTime()) << ",";
    out << "\"range\":[" << range[0] << "," << range[1] << "],";
    out << "\"sampleColors\":{";
    out << "\"min\":[" << rgbMin[0] << "," << rgbMin[1] << "," << rgbMin[2] << "],";
    out << "\"mid\":[" << rgbMid[0] << "," << rgbMid[1] << "," << rgbMid[2] << "],";
    out << "\"max\":[" << rgbMax[0] << "," << rgbMax[1] << "," << rgbMax[2] << "]";
    out << "}";

    auto attrSet = drawObj->GetAttributeSet();
    if (attrSet != nullptr) {
        const int attrIndex = attrSet->GetAttributeIndex(scalarName);
        out << ",\"attrIndex\":" << attrIndex;
        if (attrIndex >= 0) {
            auto attrs = attrSet->GetAllAttributes();
            if (attrs != nullptr && attrIndex < static_cast<int>(attrs->GetNumberOfElements())) {
                auto attr = attrs->GetElement(attrIndex);
                out << ",\"attrDeleted\":" << (attr.isDeleted ? "true" : "false");
                if (attr.pointer != nullptr) {
                    out << ",\"attrDim\":" << attr.pointer->GetDimension();
                    out << ",\"attrElements\":" << attr.pointer->GetNumberOfElements();
                }
                auto dataRange = attr.GetDataRange();
                if (dataRange != nullptr && dataRange->GetNumberOfElements() >= 2) {
                    out << ",\"dataRange\":[";
                    const int count = static_cast<int>(dataRange->GetNumberOfElements());
                    for (int i = 0; i < count; ++i) {
                        if (i) out << ",";
                        out << dataRange->GetValue(i);
                    }
                    out << "]";
                    if (dimension >= 0 && 2 + dimension * 2 + 1 < count) {
                        out << ",\"componentDataRange\":[" << dataRange->GetValue(2 + dimension * 2) << ","
                            << dataRange->GetValue(2 + dimension * 2 + 1) << "]";
                    } else {
                        out << ",\"magnitudeDataRange\":[" << dataRange->GetValue(0) << "," << dataRange->GetValue(1)
                            << "]";
                    }
                }
            }
        }
    }
    out << "}";
}

std::string DebugColorMapRangeSnapshotJson(int modelId, const std::string& scalarName, int dimension,
                                           const std::string& label) {
    std::ostringstream out;
    out << "{";
    out << "\"label\":\"" << EscapeJsonString(label) << "\",";
    out << "\"modelId\":" << modelId << ",";
    out << "\"scalarName\":\"" << EscapeJsonString(scalarName) << "\",";
    out << "\"dimension\":" << dimension << ",";
    out << "\"objects\":[";

    auto model = g_scene != nullptr ? g_scene->GetModelById(modelId) : nullptr;
    bool first = true;
    int index = 0;
    if (model != nullptr) {
        ForEachDrawObjectInTree(model->GetDataObject(), [&](iGame::DrawObject::Pointer drawObj) {
            AppendMapperSnapshot(out, drawObj, scalarName, dimension, index++, first);
            auto renderObj = drawObj->GetRenderableObject(false);
            if (renderObj != nullptr && renderObj != drawObj) {
                AppendMapperSnapshot(out, renderObj, scalarName, dimension, index++, first);
            }
        });
    }

    out << "]}";
    return out.str();
}

std::string DebugColorMapRangeSequence(int modelId, const std::string& scalarName, int dimension, double firstMin,
                                       double firstMax, double secondMin, double secondMax, double thirdMin,
                                       double thirdMax) {
    EnsureScene();
    if (g_scene == nullptr) {
        FailWithError(0, "DebugColorMapRangeSequence", "scene is null");
        return "{}";
    }

    std::ostringstream out;
    out << "{\"steps\":[";

    struct Step {
        const char* label;
        double minValue;
        double maxValue;
    };
    const Step steps[3] = {
            {"first", firstMin, firstMax},
            {"second", secondMin, secondMax},
            {"third", thirdMin, thirdMax},
    };

    for (int i = 0; i < 3; ++i) {
        const int ret = SetColorMapByNameRange(modelId, scalarName, dimension, steps[i].minValue, steps[i].maxValue);
        if (g_window != nullptr) { RenderFrame(); }
        const auto snapshot = DebugColorMapRangeSnapshotJson(modelId, scalarName, dimension,
                                                             std::string(steps[i].label) + " range=[" +
                                                                     std::to_string(steps[i].minValue) + "," +
                                                                     std::to_string(steps[i].maxValue) + "]");
        DebugLog("INFO", "DebugColorMapRangeSequence " + snapshot);
        if (i) out << ",";
        out << "{\"label\":\"" << steps[i].label << "\",\"status\":" << ret << ",\"snapshot\":" << snapshot << "}";
        if (ret != 1) break;
    }

    out << "]}";
    ClearLastError();
    return out.str();
}

void RenderFrame() {
    if (g_window == nullptr) return;

    struct SolidColorRestore {
        std::vector<std::pair<iGame::DrawObject::Pointer, igm::vec3>> overrides;
        ~SolidColorRestore() {
            for (auto& item: overrides) {
                if (item.first != nullptr) { item.first->SetDefaultColor(item.second); }
            }
        }
    } restore;

    if (g_scene != nullptr) {
        for (const auto& kv: g_modelRegistry) {
            if (!kv.second.solidEnabled) continue;
            auto model = g_scene->GetModelById(static_cast<int>(kv.first));
            if (model == nullptr) continue;
            auto drawObj = iGame::DynamicCast<iGame::DrawObject>(model->GetDataObject());
            if (drawObj == nullptr) continue;
            restore.overrides.emplace_back(drawObj, drawObj->GetDefaultColor());
            drawObj->SetDefaultColor(kv.second.solidColor);
        }
    }

    g_window->RenderOneFrame();
}

void ResetCamera() {
    if (g_scene == nullptr) {
        FailWithError(0, "ResetCamera", "scene is null");
        return;
    }
    g_scene->ResetCameraView();
    g_scene->Update();
    ClearLastError();
}

int SetCameraType(int type) {
    EnsureScene();
    if (g_scene == nullptr) { return FailWithError(0, "SetCameraType", "scene is null"); }
    if (type == 0) {
        g_scene->ChangeCameraType(iGame::Camera::Type::PERSPECTIVE);
    } else if (type == 1) {
        g_scene->ChangeCameraType(iGame::Camera::Type::ORTHOGRAPHIC);
    } else {
        return FailWithError(0, "SetCameraType", "unknown camera type=" + std::to_string(type));
    }
    g_scene->Update();
    ClearLastError();
    return 1;
}

int SetCameraPreset(int preset) {
    EnsureScene();
    if (g_scene == nullptr) { return FailWithError(0, "SetCameraPreset", "scene is null"); }

    switch (preset) {
        case 0:
            g_scene->ResetCameraViewToPositiveX();
            break;
        case 1:
            g_scene->ResetCameraViewToNegativeX();
            break;
        case 2:
            g_scene->ResetCameraViewToPositiveY();
            break;
        case 3:
            g_scene->ResetCameraViewToNegativeY();
            break;
        case 4:
            g_scene->ResetCameraViewToPositiveZ();
            break;
        case 5:
            g_scene->ResetCameraViewToNegativeZ();
            break;
        case 6:
            g_scene->ResetCameraViewToIsometric();
            break;
        default:
            return FailWithError(0, "SetCameraPreset", "unknown preset=" + std::to_string(preset));
    }

    g_scene->Update();
    ClearLastError();
    return 1;
}

int ToggleAxes(bool visible) {
    EnsureScene();
    if (g_scene == nullptr) { return FailWithError(0, "ToggleAxes", "scene is null"); }
    g_scene->SetAxesVisible(visible);
    g_scene->Update();
    ClearLastError();
    return 1;
}

int ToggleColorBar(bool visible) {
    EnsureScene();
    if (g_scene == nullptr) { return FailWithError(0, "ToggleColorBar", "scene is null"); }
    g_scene->SetColorBarVisible(visible);
    g_scene->Update();
    ClearLastError();
    return 1;
}

int SetColorBarLayout(float x, float y, float width, float height, int coordinateMode) {
    EnsureScene();
    if (g_scene == nullptr) { return FailWithError(0, "SetColorBarLayout", "scene is null"); }
    auto colorBar = g_scene->GetColorBar2DActor();
    if (colorBar == nullptr) { return FailWithError(0, "SetColorBarLayout", "color bar actor is null"); }
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(width) || !std::isfinite(height) || width <= 0.0f ||
        height <= 0.0f) {
        return FailWithError(0, "SetColorBarLayout", "layout values must be finite and size must be positive");
    }

    colorBar->SetPosition(x, y);
    colorBar->SetSize(width, height);
    colorBar->SetCoordinateMode(coordinateMode == 1 ? iGame::ColorBar2DActor::CoordinateMode::Pixel
                                                    : iGame::ColorBar2DActor::CoordinateMode::NormalizedViewport);
    g_scene->Update();
    ClearLastError();
    return 1;
}

int SetColorBarOptions(int orientation, int numberOfLabels, int maximumNumberOfColors) {
    EnsureScene();
    if (g_scene == nullptr) { return FailWithError(0, "SetColorBarOptions", "scene is null"); }
    auto colorBar = g_scene->GetColorBar2DActor();
    if (colorBar == nullptr) { return FailWithError(0, "SetColorBarOptions", "color bar actor is null"); }

    colorBar->SetOrientation(orientation == 1 ? iGame::ColorBar2DActor::Orientation::Horizontal
                                              : iGame::ColorBar2DActor::Orientation::Vertical);
    colorBar->SetNumberOfLabels(numberOfLabels);
    colorBar->SetMaximumNumberOfColors(maximumNumberOfColors);
    g_scene->Update();
    ClearLastError();
    return 1;
}

int SetCornerAnnotationText(const std::string& text) {
    EnsureScene();
    if (g_scene == nullptr) { return FailWithError(0, "SetCornerAnnotationText", "scene is null"); }
    g_scene->SetCornerAnnotationText(text);
    g_scene->SetCornerAnnotationVisible(!text.empty());
    g_scene->Update();
    ClearLastError();
    return 1;
}

int SetCornerAnnotationPosition(float left, float top) {
    EnsureScene();
    if (g_scene == nullptr) { return FailWithError(0, "SetCornerAnnotationPosition", "scene is null"); }
    if (!std::isfinite(left) || !std::isfinite(top) || left < 0.0f || top < 0.0f) {
        return FailWithError(0, "SetCornerAnnotationPosition", "position values must be finite and non-negative");
    }
    g_scene->SetCornerAnnotationPosition(left, top);
    g_scene->Update();
    ClearLastError();
    return 1;
}

int SetCornerAnnotationAnchorToBottomRight(bool anchorToBottomRight) {
    EnsureScene();
    if (g_scene == nullptr) { return FailWithError(0, "SetCornerAnnotationAnchorToBottomRight", "scene is null"); }
    g_scene->SetCornerAnnotationAnchorToBottomRight(anchorToBottomRight);
    g_scene->Update();
    ClearLastError();
    return 1;
}

int SetCornerAnnotationVisible(bool visible) {
    EnsureScene();
    if (g_scene == nullptr) { return FailWithError(0, "SetCornerAnnotationVisible", "scene is null"); }
    g_scene->SetCornerAnnotationVisible(visible);
    g_scene->Update();
    ClearLastError();
    return 1;
}

int Zoom(float factor) {
    EnsureScene();
    if (g_scene == nullptr) { return FailWithError(0, "Zoom", "scene is null"); }
    if (!std::isfinite(factor) || factor <= 0.0f) {
        return FailWithError(0, "Zoom", "factor must be a finite value greater than 0");
    }

    auto camera = g_scene->GetCamera();
    if (camera == nullptr) { return FailWithError(0, "Zoom", "camera is null"); }

    const auto focal = camera->GetFocal();
    const auto position = camera->GetPosition();
    auto offset = position - focal;
    const float distance = offset.length();
    if (distance <= 1e-6f) { return FailWithError(0, "Zoom", "camera distance is too small"); }

    const float nextDistance = std::max(1e-4f, distance / factor);
    camera->SetPosition(focal + offset.normalized() * nextDistance);
    g_scene->Update();
    ClearLastError();
    return 1;
}

void SendMouseEvent(int type, int button, float x, float y, double delta) {
    if (g_interactor == nullptr) {
        FailWithError(0, "SendMouseEvent", "interactor is null");
        return;
    }

    iGame::IEvent event;
    switch (type) {
        case 0:
            event.type = iGame::IEvent::MousePress;
            break;
        case 1:
            event.type = iGame::IEvent::MouseMove;
            break;
        case 2:
            event.type = iGame::IEvent::MouseRelease;
            break;
        case 3:
            event.type = iGame::IEvent::Wheel;
            break;
        default:
            FailWithError(0, "SendMouseEvent", "unknown event type=" + std::to_string(type));
            return;
    }

    switch (button) {
        case 1:
            event.button = iGame::MouseButton::LeftButton;
            break;
        case 2:
            event.button = iGame::MouseButton::RightButton;
            break;
        case 4:
            event.button = iGame::MouseButton::MiddleButton;
            break;
        default:
            event.button = iGame::MouseButton::NoButton;
            break;
    }

    event.pos.x = x;
    event.pos.y = y;
    event.delta = delta;

    g_interactor->FilterEvent(event);
    ClearLastError();
}
} // namespace iGameWeb

int iGameWeb::API::init() {
    DebugLog("INFO", "API.init called");
    int ret = iGameWeb::Init();
    if (ret > 0) {
        DebugLog("INFO", "API.init success");
    } else {
        DebugLog("ERROR", "API.init failed code=" + std::to_string(g_lastError.code) + " detail=" + g_lastError.detail);
    }
    return ret;
}

int iGameWeb::API::destroy() {
    DebugLog("INFO", "API.destroy called");
    if (g_scene != nullptr) {
        RemoveAllUserModels();
    }

    g_modelRegistry.clear();
    g_activeModelId = 0;
    g_selectionMode = 0;
    g_slicingActive = false;
    g_sliceSourceModelId = 0;
    g_sliceResultModelId = 0;
    g_clipSelection = nullptr;
    if (g_window != nullptr) {
        g_window->SetInteractor(nullptr);
        g_window->SetScene(nullptr);
    }
    if (g_scene != nullptr) {
        g_scene->SetInteractor(nullptr);
        g_scene->Finalize();
    }
    if (g_interactor != nullptr) {
        g_interactor->Finalize();
    }
    g_interactor = nullptr;
    g_window = nullptr;
    g_scene = nullptr;
    ClearLastError();
    DebugLog("INFO", "API.destroy success");
    return 1;
}

std::string iGameWeb::API::stressLifecycleVtu(const val& bytes, int iterations) {
    DebugLog("INFO", "API.stressLifecycleVtu called iterations=" + std::to_string(iterations));
    std::string byteBuffer;
    if (!ReadBytesFromJsValue(bytes, byteBuffer, "API.stressLifecycleVtu")) {
        return "{\"requested\":0,\"completed\":0,\"failedIteration\":0,\"phase\":\"readBytes\",\"lastError\":" +
               GetLastErrorJson() + "}";
    }

    const int requested = std::max(1, std::min(iterations <= 0 ? 50 : iterations, 100));
    int completed = 0;
    int failedIteration = -1;
    std::string phase = "ok";
    std::string errorJson = "{}";

    for (int i = 0; i < requested; ++i) {
        DebugLog("INFO", "API.stressLifecycleVtu iteration begin " + std::to_string(i + 1) + "/" +
                                 std::to_string(requested));

        if (iGameWeb::Init() <= 0) {
            failedIteration = i + 1;
            phase = "init";
            errorJson = GetLastErrorJson();
            iGameWeb::API::destroy();
            break;
        }

        const std::string sourceName = "stress_lifecycle_" + std::to_string(i + 1) + ".vtu";
        if (iGameWeb::LoadVtuFromMemEx(byteBuffer, sourceName, true) <= 0) {
            failedIteration = i + 1;
            phase = "load";
            errorJson = GetLastErrorJson();
            iGameWeb::API::destroy();
            break;
        }

        iGameWeb::RenderFrame();
        if (iGameWeb::API::destroy() <= 0) {
            failedIteration = i + 1;
            phase = "destroy";
            errorJson = GetLastErrorJson();
            break;
        }
        ++completed;
    }

    std::string json = "{";
    json += "\"requested\":" + std::to_string(requested) + ",";
    json += "\"completed\":" + std::to_string(completed) + ",";
    json += "\"failedIteration\":" + std::to_string(failedIteration) + ",";
    json += "\"phase\":\"" + EscapeJsonString(phase) + "\",";
    json += "\"lastError\":" + errorJson;
    json += "}";
    DebugLog("INFO", "API.stressLifecycleVtu result " + json);
    return json;
}

void iGameWeb::API::setDebugEnabled(bool enabled) {
    DebugLog("INFO", std::string("API.setDebugEnabled called enabled=") + (enabled ? "true" : "false"));
    iGameWeb::SetDebugEnabled(enabled);
    DebugLog("INFO", "API.setDebugEnabled done");
}

int iGameWeb::API::raiseTestError(const std::string& detail, int code) {
    DebugLog("INFO", "API.raiseTestError called");
    int ret = iGameWeb::RaiseTestError(detail, code);
    DebugLog("INFO", "API.raiseTestError returned code=" + std::to_string(ret));
    return ret;
}

std::string iGameWeb::API::getLastErrorJson() {
    DebugLog("INFO", "API.getLastErrorJson called");
    return GetLastErrorJson();
}

void iGameWeb::API::clearLastError() {
    DebugLog("INFO", "API.clearLastError called");
    ClearLastError();
}

int iGameWeb::API::setSize(int width, int height) {
    DebugLog("INFO", "API.setSize called width=" + std::to_string(width) + " height=" + std::to_string(height));
    int ret = iGameWeb::SetSize(width, height);
    if (ret > 0) DebugLog("INFO", "API.setSize success");
    else
        DebugLog("ERROR",
                 "API.setSize failed code=" + std::to_string(g_lastError.code) + " detail=" + g_lastError.detail);
    return ret;
}

int iGameWeb::API::loadVtkFromMem(const val& bytes) {
    DebugLog("INFO", "API.loadVtkFromMem called");
    std::string byteBuffer;
    if (!ReadBytesFromJsValue(bytes, byteBuffer, "API.loadVtkFromMem")) { return 0; }
    int ret = iGameWeb::LoadVtkFromMem(byteBuffer);
    if (ret > 0) DebugLog("INFO", "API.loadVtkFromMem success");
    else
        DebugLog("ERROR", "API.loadVtkFromMem failed code=" + std::to_string(g_lastError.code) +
                                  " detail=" + g_lastError.detail);
    return ret;
}

int iGameWeb::API::loadVtuFromMem(const val& bytes) {
    DebugLog("INFO", "API.loadVtuFromMem called");
    std::string byteBuffer;
    if (!ReadBytesFromJsValue(bytes, byteBuffer, "API.loadVtuFromMem")) { return 0; }
    int ret = iGameWeb::LoadVtuFromMem(byteBuffer);
    if (ret > 0) DebugLog("INFO", "API.loadVtuFromMem success");
    else
        DebugLog("ERROR", "API.loadVtuFromMem failed code=" + std::to_string(g_lastError.code) +
                                  " detail=" + g_lastError.detail);
    return ret;
}

int iGameWeb::API::loadVtpFromMem(const val& bytes) {
    DebugLog("INFO", "API.loadVtpFromMem called");
    std::string byteBuffer;
    if (!ReadBytesFromJsValue(bytes, byteBuffer, "API.loadVtpFromMem")) { return 0; }
    int ret = iGameWeb::LoadVtpFromMem(byteBuffer);
    if (ret > 0) DebugLog("INFO", "API.loadVtpFromMem success");
    else
        DebugLog("ERROR", "API.loadVtpFromMem failed code=" + std::to_string(g_lastError.code) +
                                  " detail=" + g_lastError.detail);
    return ret;
}

int iGameWeb::API::loadIgcFromMem(const val& bytes) {
    DebugLog("INFO", "API.loadIgcFromMem called");
    std::string byteBuffer;
    if (!ReadBytesFromJsValue(bytes, byteBuffer, "API.loadIgcFromMem")) { return 0; }
    int ret = iGameWeb::LoadIgcFromMem(byteBuffer);
    if (ret > 0) DebugLog("INFO", "API.loadIgcFromMem success");
    else
        DebugLog("ERROR", "API.loadIgcFromMem failed code=" + std::to_string(g_lastError.code) +
                                  " detail=" + g_lastError.detail);
    return ret;
}

int iGameWeb::API::loadIgcLegacy32FromMem(const val& bytes) {
    DebugLog("INFO", "API.loadIgcLegacy32FromMem called");
    std::string byteBuffer;
    if (!ReadBytesFromJsValue(bytes, byteBuffer, "API.loadIgcLegacy32FromMem")) { return 0; }
    int ret = iGameWeb::LoadIgcLegacy32FromMem(byteBuffer);
    if (ret > 0) DebugLog("INFO", "API.loadIgcLegacy32FromMem success");
    else
        DebugLog("ERROR", "API.loadIgcLegacy32FromMem failed code=" + std::to_string(g_lastError.code) +
                                  " detail=" + g_lastError.detail);
    return ret;
}

int iGameWeb::API::loadIgcLegacy64FromMem(const val& bytes) {
    DebugLog("INFO", "API.loadIgcLegacy64FromMem called");
    std::string byteBuffer;
    if (!ReadBytesFromJsValue(bytes, byteBuffer, "API.loadIgcLegacy64FromMem")) { return 0; }
    int ret = iGameWeb::LoadIgcLegacy64FromMem(byteBuffer);
    if (ret > 0) DebugLog("INFO", "API.loadIgcLegacy64FromMem success");
    else
        DebugLog("ERROR", "API.loadIgcLegacy64FromMem failed code=" + std::to_string(g_lastError.code) +
                                  " detail=" + g_lastError.detail);
    return ret;
}

int iGameWeb::API::loadVtkFromMemEx(const val& bytes, const std::string& sourceName, bool replaceExisting) {
    DebugLog("INFO", "API.loadVtkFromMemEx called sourceName=" + sourceName);
    std::string byteBuffer;
    if (!ReadBytesFromJsValue(bytes, byteBuffer, "API.loadVtkFromMemEx")) { return 0; }
    int ret = iGameWeb::LoadVtkFromMemEx(byteBuffer, sourceName, replaceExisting);
    if (ret > 0) DebugLog("INFO", "API.loadVtkFromMemEx success");
    else
        DebugLog("ERROR", "API.loadVtkFromMemEx failed code=" + std::to_string(g_lastError.code) +
                                  " detail=" + g_lastError.detail);
    return ret;
}

int iGameWeb::API::loadVtuFromMemEx(const val& bytes, const std::string& sourceName, bool replaceExisting) {
    DebugLog("INFO", "API.loadVtuFromMemEx called sourceName=" + sourceName);
    std::string byteBuffer;
    if (!ReadBytesFromJsValue(bytes, byteBuffer, "API.loadVtuFromMemEx")) { return 0; }
    int ret = iGameWeb::LoadVtuFromMemEx(byteBuffer, sourceName, replaceExisting);
    if (ret > 0) DebugLog("INFO", "API.loadVtuFromMemEx success");
    else
        DebugLog("ERROR", "API.loadVtuFromMemEx failed code=" + std::to_string(g_lastError.code) +
                                  " detail=" + g_lastError.detail);
    return ret;
}

int iGameWeb::API::loadVtpFromMemEx(const val& bytes, const std::string& sourceName, bool replaceExisting) {
    DebugLog("INFO", "API.loadVtpFromMemEx called sourceName=" + sourceName);
    std::string byteBuffer;
    if (!ReadBytesFromJsValue(bytes, byteBuffer, "API.loadVtpFromMemEx")) { return 0; }
    int ret = iGameWeb::LoadVtpFromMemEx(byteBuffer, sourceName, replaceExisting);
    if (ret > 0) DebugLog("INFO", "API.loadVtpFromMemEx success");
    else
        DebugLog("ERROR", "API.loadVtpFromMemEx failed code=" + std::to_string(g_lastError.code) +
                                  " detail=" + g_lastError.detail);
    return ret;
}

int iGameWeb::API::loadIgcFromMemEx(const val& bytes, const std::string& sourceName, bool replaceExisting) {
    DebugLog("INFO", "API.loadIgcFromMemEx called sourceName=" + sourceName);
    std::string byteBuffer;
    if (!ReadBytesFromJsValue(bytes, byteBuffer, "API.loadIgcFromMemEx")) { return 0; }
    int ret = iGameWeb::LoadIgcFromMemEx(byteBuffer, sourceName, replaceExisting);
    if (ret > 0) DebugLog("INFO", "API.loadIgcFromMemEx success");
    else
        DebugLog("ERROR", "API.loadIgcFromMemEx failed code=" + std::to_string(g_lastError.code) +
                                  " detail=" + g_lastError.detail);
    return ret;
}

int iGameWeb::API::loadIgcLegacy32FromMemEx(const val& bytes, const std::string& sourceName, bool replaceExisting) {
    DebugLog("INFO", "API.loadIgcLegacy32FromMemEx called sourceName=" + sourceName);
    std::string byteBuffer;
    if (!ReadBytesFromJsValue(bytes, byteBuffer, "API.loadIgcLegacy32FromMemEx")) { return 0; }
    int ret = iGameWeb::LoadIgcLegacy32FromMemEx(byteBuffer, sourceName, replaceExisting);
    if (ret > 0) DebugLog("INFO", "API.loadIgcLegacy32FromMemEx success");
    else
        DebugLog("ERROR", "API.loadIgcLegacy32FromMemEx failed code=" + std::to_string(g_lastError.code) +
                                  " detail=" + g_lastError.detail);
    return ret;
}

int iGameWeb::API::loadIgcLegacy64FromMemEx(const val& bytes, const std::string& sourceName, bool replaceExisting) {
    DebugLog("INFO", "API.loadIgcLegacy64FromMemEx called sourceName=" + sourceName);
    std::string byteBuffer;
    if (!ReadBytesFromJsValue(bytes, byteBuffer, "API.loadIgcLegacy64FromMemEx")) { return 0; }
    int ret = iGameWeb::LoadIgcLegacy64FromMemEx(byteBuffer, sourceName, replaceExisting);
    if (ret > 0) DebugLog("INFO", "API.loadIgcLegacy64FromMemEx success");
    else
        DebugLog("ERROR", "API.loadIgcLegacy64FromMemEx failed code=" + std::to_string(g_lastError.code) +
                                  " detail=" + g_lastError.detail);
    return ret;
}

int iGameWeb::API::loadZipFromMemEx(const val& bytes, const std::string& sourceName, bool replaceExisting) {
    DebugLog("INFO", "API.loadZipFromMemEx called sourceName=" + sourceName);
    std::string byteBuffer;
    if (!ReadBytesFromJsValue(bytes, byteBuffer, "API.loadZipFromMemEx")) { return 0; }
    int ret = iGameWeb::LoadZipFromMemEx(byteBuffer, sourceName, replaceExisting);
    if (ret > 0) DebugLog("INFO", "API.loadZipFromMemEx success loadedCount=" + std::to_string(ret));
    else
        DebugLog("ERROR", "API.loadZipFromMemEx failed code=" + std::to_string(g_lastError.code) +
                                  " detail=" + g_lastError.detail);
    return ret;
}

std::string iGameWeb::API::getModelListJson() {
    DebugLog("INFO", "API.getModelListJson called");
    std::string s = iGameWeb::GetModelListJson();
    DebugLog("INFO", "API.getModelListJson returned length=" + std::to_string(s.size()));
    return s;
}

int iGameWeb::API::setActiveModel(int modelId) {
    DebugLog("INFO", "API.setActiveModel called modelId=" + std::to_string(modelId));
    int ret = iGameWeb::SetActiveModel(modelId);
    if (ret > 0) DebugLog("INFO", "API.setActiveModel success");
    else
        DebugLog("ERROR", "API.setActiveModel failed code=" + std::to_string(g_lastError.code) +
                                  " detail=" + g_lastError.detail);
    return ret;
}

int iGameWeb::API::setModelVisibility(int modelId, bool visible) {
    DebugLog("INFO", "API.setModelVisibility called modelId=" + std::to_string(modelId));
    int ret = iGameWeb::SetModelVisibility(modelId, visible);
    if (ret > 0) DebugLog("INFO", "API.setModelVisibility success");
    else
        DebugLog("ERROR", "API.setModelVisibility failed code=" + std::to_string(g_lastError.code) +
                                  " detail=" + g_lastError.detail);
    return ret;
}

int iGameWeb::API::removeModel(int modelId) {
    DebugLog("INFO", "API.removeModel called modelId=" + std::to_string(modelId));
    int ret = iGameWeb::RemoveModel(modelId);
    if (ret > 0) DebugLog("INFO", "API.removeModel success");
    else
        DebugLog("ERROR",
                 "API.removeModel failed code=" + std::to_string(g_lastError.code) + " detail=" + g_lastError.detail);
    return ret;
}

int iGameWeb::API::removeAllModels() {
    DebugLog("INFO", "API.removeAllModels called");
    int ret = iGameWeb::RemoveAllModels();
    if (ret > 0) DebugLog("INFO", "API.removeAllModels success");
    else
        DebugLog("ERROR", "API.removeAllModels failed code=" + std::to_string(g_lastError.code) +
                                  " detail=" + g_lastError.detail);
    return ret;
}

int iGameWeb::API::clearViewer() {
    DebugLog("INFO", "API.clearViewer called");
    return iGameWeb::ClearViewer();
}

int iGameWeb::API::clear() {
    DebugLog("INFO", "API.clear called");
    return iGameWeb::ClearViewer();
}

int iGameWeb::API::setSelectionMode(int mode) {
    DebugLog("INFO", "API.setSelectionMode called mode=" + std::to_string(mode));
    return iGameWeb::SetSelectionMode(mode);
}

int iGameWeb::API::getSelectionMode() { return iGameWeb::GetSelectionMode(); }

int iGameWeb::API::enterSlicingMode(int modelId) { return EnterSlicingMode(modelId); }

int iGameWeb::API::exitSlicingMode() { return ExitSlicingMode(); }

int iGameWeb::API::isSlicingMode() { return IsSlicingMode(); }

int iGameWeb::API::getSliceSourceModelId() { return GetSliceSourceModelId(); }

int iGameWeb::API::getSliceResultModelId() { return GetSliceResultModelId(); }

std::string iGameWeb::API::getClipPlaneJson() { return GetClipPlaneJson(); }

int iGameWeb::API::setClipPlane(float ox, float oy, float oz, float nx, float ny, float nz) {
    return SetClipPlane(ox, oy, oz, nx, ny, nz);
}

int iGameWeb::API::setSlicingPreview(bool enabled) { return SetSlicingPreview(enabled); }

int iGameWeb::API::setSliceOperationMode(int mode) { return SetSliceOperationMode(mode); }

int iGameWeb::API::setSliceCrinkle(bool enabled) { return SetSliceCrinkle(enabled); }

int iGameWeb::API::setSliceInvert(bool enabled) { return SetSliceInvert(enabled); }

int iGameWeb::API::executeSlice() {
    const int ret = ExecuteSliceOperation("API.executeSlice");
    if (ret > 0) { RenderFrame(); }
    return ret;
}

std::string iGameWeb::API::getSelectionJson(int modelId) {
    DebugLog("INFO", "API.getSelectionJson called modelId=" + std::to_string(modelId));
    return iGameWeb::GetSelectionJson(modelId);
}

int iGameWeb::API::clearSelection(int modelId) {
    DebugLog("INFO", "API.clearSelection called modelId=" + std::to_string(modelId));
    return iGameWeb::ClearSelection(modelId);
}

int iGameWeb::API::setViewStyle(int styleMask) {
    DebugLog("INFO", "API.setViewStyle called styleMask=" + std::to_string(styleMask));
    int ret = iGameWeb::SetViewStyle(styleMask);
    if (ret > 0) DebugLog("INFO", "API.setViewStyle success");
    else
        DebugLog("ERROR",
                 "API.setViewStyle failed code=" + std::to_string(g_lastError.code) + " detail=" + g_lastError.detail);
    return ret;
}

int iGameWeb::API::setViewStyle(int modelId, int styleMask) {
    DebugLog("INFO",
             "API.setViewStyle called modelId=" + std::to_string(modelId) + " styleMask=" + std::to_string(styleMask));
    int ret = iGameWeb::SetViewStyle(modelId, styleMask);
    if (ret > 0) DebugLog("INFO", "API.setViewStyle success");
    else
        DebugLog("ERROR",
                 "API.setViewStyle failed code=" + std::to_string(g_lastError.code) + " detail=" + g_lastError.detail);
    return ret;
}

int iGameWeb::API::getViewStyle(int modelId) {
    DebugLog("INFO", "API.getViewStyle called modelId=" + std::to_string(modelId));
    int ret = iGameWeb::GetViewStyle(modelId);
    if (ret >= 0) DebugLog("INFO", "API.getViewStyle success viewStyle=" + std::to_string(ret));
    else
        DebugLog("ERROR",
                 "API.getViewStyle failed code=" + std::to_string(g_lastError.code) + " detail=" + g_lastError.detail);
    return ret;
}

int iGameWeb::API::setActorOpacity(int modelId, float opacity) {
    DebugLog("INFO", "API.setActorOpacity called modelId=" + std::to_string(modelId));
    int ret = iGameWeb::SetActorOpacity(modelId, opacity);
    if (ret > 0) DebugLog("INFO", "API.setActorOpacity success");
    else
        DebugLog("ERROR", "API.setActorOpacity failed code=" + std::to_string(g_lastError.code) +
                                  " detail=" + g_lastError.detail);
    return ret;
}

float iGameWeb::API::getActorOpacity(int modelId) {
    DebugLog("INFO", "API.getActorOpacity called modelId=" + std::to_string(modelId));
    return iGameWeb::GetActorOpacity(modelId);
}

int iGameWeb::API::setScalarField(int attributeIndex, int dimension, int dataLocation) {
    DebugLog("INFO", "API.setScalarField called attributeIndex=" + std::to_string(attributeIndex));
    int ret = iGameWeb::SetScalarField(attributeIndex, dimension, dataLocation);
    if (ret > 0) DebugLog("INFO", "API.setScalarField success");
    else
        DebugLog("ERROR", "API.setScalarField failed code=" + std::to_string(g_lastError.code) +
                                  " detail=" + g_lastError.detail);
    return ret;
}

int iGameWeb::API::setSurfaceShadingMode(int mode) {
    DebugLog("INFO", "API.setSurfaceShadingMode called mode=" + std::to_string(mode));
    int ret = iGameWeb::SetSurfaceShadingMode(mode);
    if (ret > 0) DebugLog("INFO", "API.setSurfaceShadingMode success");
    else
        DebugLog("ERROR", "API.setSurfaceShadingMode failed code=" + std::to_string(g_lastError.code) +
                                  " detail=" + g_lastError.detail);
    return ret;
}

int iGameWeb::API::setBackgroundColor(float r, float g, float b) {
    DebugLog("INFO", "API.setBackgroundColor called r=" + std::to_string(r) + " g=" + std::to_string(g) +
                             " b=" + std::to_string(b));
    int ret = iGameWeb::SetBackgroundColor(r, g, b);
    if (ret > 0) DebugLog("INFO", "API.setBackgroundColor success");
    else
        DebugLog("ERROR", "API.setBackgroundColor failed code=" + std::to_string(g_lastError.code) +
                                  " detail=" + g_lastError.detail);
    return ret;
}

int iGameWeb::API::setSceneBackgroundColor(float r, float g, float b) {
    DebugLog("INFO", "API.setSceneBackgroundColor called");
    int ret = iGameWeb::SetSceneBackgroundColor(r, g, b);
    if (ret > 0) DebugLog("INFO", "API.setSceneBackgroundColor success");
    else
        DebugLog("ERROR", "API.setSceneBackgroundColor failed code=" + std::to_string(g_lastError.code) +
                                  " detail=" + g_lastError.detail);
    return ret;
}

int iGameWeb::API::setSceneBackgroundGradientColor(float r1, float g1, float b1, float r2, float g2, float b2,
                                                   int mode) {
    DebugLog("INFO", "API.setSceneBackgroundGradientColor called mode=" + std::to_string(mode));
    int ret = iGameWeb::SetSceneBackgroundGradientColor(r1, g1, b1, r2, g2, b2, mode);
    if (ret > 0) {
        DebugLog("INFO", "API.setSceneBackgroundGradientColor success");
    } else {
        DebugLog("ERROR", "API.setSceneBackgroundGradientColor failed code=" + std::to_string(g_lastError.code) +
                                  " detail=" + g_lastError.detail);
    }
    return ret;
}

int iGameWeb::API::setColorMap(const val& colors_flat, const val& ranges) {
    std::vector<float> colorsVector;
    std::vector<float> rangesVector;

    if (!ReadFloatVectorFromJsArrayLike(colors_flat, colorsVector, "setColorMap")) {
        return g_lastError.code == 0 ? -1 : g_lastError.code;
    }
    if (!ReadFloatVectorFromJsArrayLike(ranges, rangesVector, "setColorMap")) {
        return g_lastError.code == 0 ? -1 : g_lastError.code;
    }

    DebugLog("INFO", "API.setColorMap called colors_flat_len=" + std::to_string(colorsVector.size()) +
                             " ranges_len=" + std::to_string(rangesVector.size()));
    int ret = SetColorMapFromArrays(colorsVector, rangesVector);
    if (ret > 0) DebugLog("INFO", "API.setColorMap success");
    else
        DebugLog("ERROR",
                 "API.setColorMap failed code=" + std::to_string(g_lastError.code) + " detail=" + g_lastError.detail);
    return ret;
}

int iGameWeb::API::setColorMapByName(int modelId, const std::string& scalarName, int dimension, const val& colors_flat,
                                     const val& ranges) {
    std::vector<float> colorsVector;
    std::vector<float> rangesVector;

    if (!ReadFloatVectorFromJsArrayLike(colors_flat, colorsVector, "setColorMapByName")) {
        return g_lastError.code == 0 ? -1 : g_lastError.code;
    }
    if (!ReadFloatVectorFromJsArrayLike(ranges, rangesVector, "setColorMapByName")) {
        return g_lastError.code == 0 ? -1 : g_lastError.code;
    }

    DebugLog("INFO", "API.setColorMapByName called modelId=" + std::to_string(modelId) + " name=" + scalarName +
                             " colors_flat_len=" + std::to_string(colorsVector.size()) +
                             " ranges_len=" + std::to_string(rangesVector.size()));
    // dataLocation 参数在 Web API 中被移除；向底层传入 -1 表示忽略
    int ret = SetColorMapByName(modelId, scalarName, dimension, -1, colorsVector, rangesVector);
    if (ret > 0) DebugLog("INFO", "API.setColorMapByName success");
    else
        DebugLog("ERROR", "API.setColorMapByName failed code=" + std::to_string(g_lastError.code) +
                                  " detail=" + g_lastError.detail);
    return ret;
}

int iGameWeb::API::setColorMapByNameRange(int modelId, const std::string& scalarName, int dimension, double minValue,
                                          double maxValue) {
    DebugLog("INFO", "API.setColorMapByNameRange called modelId=" + std::to_string(modelId) + " name=" + scalarName +
                             " component=" + std::to_string(dimension) + " min=" + std::to_string(minValue) +
                             " max=" + std::to_string(maxValue));
    int ret = SetColorMapByNameRange(modelId, scalarName, dimension, minValue, maxValue);
    if (ret > 0) {
    } else {
        DebugLog("ERROR", "API.setColorMapByNameRange failed code=" + std::to_string(g_lastError.code) +
                                  " detail=" + g_lastError.detail);
    }
    return ret;
}

std::string iGameWeb::API::debugColorMapRangeSequence(int modelId, const std::string& scalarName, int dimension,
                                                      double firstMin, double firstMax, double secondMin,
                                                      double secondMax, double thirdMin, double thirdMax) {
    DebugLog("INFO", "API.debugColorMapRangeSequence called modelId=" + std::to_string(modelId) +
                             " name=" + scalarName + " component=" + std::to_string(dimension));
    return DebugColorMapRangeSequence(modelId, scalarName, dimension, firstMin, firstMax, secondMin, secondMax,
                                      thirdMin, thirdMax);
}

int iGameWeb::API::setModelDefaultColor(int modelId, float r, float g, float b) {
    DebugLog("INFO", "API.setModelDefaultColor called modelId=" + std::to_string(modelId));
    int ret = SetModelDefaultColor(modelId, r, g, b);
    if (ret > 0) DebugLog("INFO", "API.setModelDefaultColor success");
    else
        DebugLog("ERROR", "API.setModelDefaultColor failed code=" + std::to_string(g_lastError.code) +
                                  " detail=" + g_lastError.detail);
    return ret;
}

int iGameWeb::API::setSolidColor(int modelId, float r, float g, float b) {
    DebugLog("INFO", "API.setSolidColor called modelId=" + std::to_string(modelId));
    int ret = SetSolidColor(modelId, r, g, b);
    if (ret > 0) DebugLog("INFO", "API.setSolidColor success");
    else
        DebugLog("ERROR",
                 "API.setSolidColor failed code=" + std::to_string(g_lastError.code) + " detail=" + g_lastError.detail);
    return ret;
}

std::string iGameWeb::API::getModelDefaultColor(int modelId) {
    DebugLog("INFO", "API.getModelDefaultColor called modelId=" + std::to_string(modelId));
    std::string s = iGameWeb::GetModelDefaultColor(modelId);
    DebugLog("INFO", "API.getModelDefaultColor returned length=" + std::to_string(s.size()));
    return s;
}

std::string iGameWeb::API::getSolidColor(int modelId) {
    DebugLog("INFO", "API.getSolidColor called modelId=" + std::to_string(modelId));
    std::string s = GetSolidColorJson(modelId);
    DebugLog("INFO", "API.getSolidColor returned length=" + std::to_string(s.size()));
    return s;
}

int iGameWeb::API::setHelperPen(int modelId, bool enabled, float r, float g, float b, float width) {
    DebugLog("INFO", "API.setHelperPen called modelId=" + std::to_string(modelId));
    int ret = SetHelperPen(modelId, enabled, r, g, b, width);
    if (ret > 0) DebugLog("INFO", "API.setHelperPen success");
    else
        DebugLog("ERROR",
                 "API.setHelperPen failed code=" + std::to_string(g_lastError.code) + " detail=" + g_lastError.detail);
    return ret;
}

int iGameWeb::API::setHelperBrush(int modelId, bool enabled, float r, float g, float b) {
    DebugLog("INFO", "API.setHelperBrush called modelId=" + std::to_string(modelId));
    int ret = SetHelperBrush(modelId, enabled, r, g, b);
    if (ret > 0) DebugLog("INFO", "API.setHelperBrush success");
    else
        DebugLog("ERROR", "API.setHelperBrush failed code=" + std::to_string(g_lastError.code) +
                                  " detail=" + g_lastError.detail);
    return ret;
}

int iGameWeb::API::drawHelperSphere(int modelId, float cx, float cy, float cz, float radius, int stackCount,
                                    int sectorCount) {
    DebugLog("INFO", "API.drawHelperSphere called modelId=" + std::to_string(modelId));
    int ret = DrawHelperSphere(modelId, cx, cy, cz, radius, stackCount, sectorCount);
    if (ret > 0) DebugLog("INFO", "API.drawHelperSphere success handle=" + std::to_string(ret));
    else if (g_lastError.code != 0)
        DebugLog("ERROR", "API.drawHelperSphere failed code=" + std::to_string(g_lastError.code) +
                                  " detail=" + g_lastError.detail);
    return ret;
}

int iGameWeb::API::drawHelperCylinder(int modelId, float cx, float cy, float cz, float nx, float ny, float nz,
                                      float height, float radius, int resolution) {
    DebugLog("INFO", "API.drawHelperCylinder called modelId=" + std::to_string(modelId));
    int ret = DrawHelperCylinder(modelId, cx, cy, cz, nx, ny, nz, height, radius, resolution);
    if (ret > 0) DebugLog("INFO", "API.drawHelperCylinder success handle=" + std::to_string(ret));
    else if (g_lastError.code != 0)
        DebugLog("ERROR", "API.drawHelperCylinder failed code=" + std::to_string(g_lastError.code) +
                                  " detail=" + g_lastError.detail);
    return ret;
}

int iGameWeb::API::drawHelperCube(int modelId, float x1, float y1, float z1, float x2, float y2, float z2) {
    DebugLog("INFO", "API.drawHelperCube called modelId=" + std::to_string(modelId));
    int ret = DrawHelperCube(modelId, x1, y1, z1, x2, y2, z2);
    if (ret > 0) DebugLog("INFO", "API.drawHelperCube success handle=" + std::to_string(ret));
    else if (g_lastError.code != 0)
        DebugLog("ERROR", "API.drawHelperCube failed code=" + std::to_string(g_lastError.code) +
                                  " detail=" + g_lastError.detail);
    return ret;
}

int iGameWeb::API::drawHelperCircle(int modelId, float cx, float cy, float cz, float nx, float ny, float nz,
                                    float radius, int resolution) {
    DebugLog("INFO", "API.drawHelperCircle called modelId=" + std::to_string(modelId));
    int ret = DrawHelperCircle(modelId, cx, cy, cz, nx, ny, nz, radius, resolution);
    if (ret > 0) DebugLog("INFO", "API.drawHelperCircle success handle=" + std::to_string(ret));
    else if (g_lastError.code != 0)
        DebugLog("ERROR", "API.drawHelperCircle failed code=" + std::to_string(g_lastError.code) +
                                  " detail=" + g_lastError.detail);
    return ret;
}

int iGameWeb::API::showHelper(int modelId, int handle) {
    DebugLog("INFO", "API.showHelper called modelId=" + std::to_string(modelId));
    return ShowHelper(modelId, handle);
}

int iGameWeb::API::hideHelper(int modelId, int handle) {
    DebugLog("INFO", "API.hideHelper called modelId=" + std::to_string(modelId));
    return HideHelper(modelId, handle);
}

int iGameWeb::API::deleteHelper(int modelId, int handle) {
    DebugLog("INFO", "API.deleteHelper called modelId=" + std::to_string(modelId));
    return DeleteHelper(modelId, handle);
}

int iGameWeb::API::clearHelpers(int modelId) {
    DebugLog("INFO", "API.clearHelpers called modelId=" + std::to_string(modelId));
    return ClearHelpers(modelId);
}

std::string iGameWeb::API::getColorMapJson() {
    DebugLog("INFO", "API.getColorMapJson called");
    std::string s = GetColorMapJson();
    DebugLog("INFO", "API.getColorMapJson returned length=" + std::to_string(s.size()));
    return s;
}

std::string iGameWeb::API::getAttributeListJson() {
    DebugLog("INFO", "API.getAttributeListJson called");
    std::string s = iGameWeb::GetAttributeListJson();
    DebugLog("INFO", "API.getAttributeListJson returned length=" + std::to_string(s.size()));
    return s;
}

void iGameWeb::API::renderFrame() {
    iGameWeb::RenderFrame();
    if (g_lastError.code != 0) {
        DebugLog("ERROR",
                 "API.renderFrame failed code=" + std::to_string(g_lastError.code) + " detail=" + g_lastError.detail);
    }
}

void iGameWeb::API::resetCamera() {
    DebugLog("INFO", "API.resetCamera called");
    iGameWeb::ResetCamera();
    if (g_lastError.code != 0) {
        DebugLog("ERROR",
                 "API.resetCamera failed code=" + std::to_string(g_lastError.code) + " detail=" + g_lastError.detail);
    }
}

int iGameWeb::API::setCameraType(int type) {
    DebugLog("INFO", "API.setCameraType called type=" + std::to_string(type));
    int result = iGameWeb::SetCameraType(type);
    if (g_lastError.code != 0) {
        DebugLog("ERROR",
                 "API.setCameraType failed code=" + std::to_string(g_lastError.code) + " detail=" + g_lastError.detail);
    }
    return result;
}

int iGameWeb::API::viewXPlus() { return iGameWeb::SetCameraPreset(0); }
int iGameWeb::API::viewXMinus() { return iGameWeb::SetCameraPreset(1); }
int iGameWeb::API::viewYPlus() { return iGameWeb::SetCameraPreset(2); }
int iGameWeb::API::viewYMinus() { return iGameWeb::SetCameraPreset(3); }
int iGameWeb::API::viewZPlus() { return iGameWeb::SetCameraPreset(4); }
int iGameWeb::API::viewZMinus() { return iGameWeb::SetCameraPreset(5); }
int iGameWeb::API::viewIsometric() { return iGameWeb::SetCameraPreset(6); }

int iGameWeb::API::toggleAxes(bool visible) {
    DebugLog("INFO", std::string("API.toggleAxes called visible=") + (visible ? "true" : "false"));
    return iGameWeb::ToggleAxes(visible);
}

int iGameWeb::API::toggleColorBar(bool visible) {
    DebugLog("INFO", std::string("API.toggleColorBar called visible=") + (visible ? "true" : "false"));
    int ret = iGameWeb::ToggleColorBar(visible);
    if (ret > 0) { DebugLog("INFO", "API.toggleColorBar success state=" + DescribeColorBarState()); }
    return ret;
}

int iGameWeb::API::setColorBarLayout(float x, float y, float width, float height, int coordinateMode) {
    DebugLog("INFO", "API.setColorBarLayout called");
    int ret = iGameWeb::SetColorBarLayout(x, y, width, height, coordinateMode);
    if (ret > 0) { DebugLog("INFO", "API.setColorBarLayout success state=" + DescribeColorBarState()); }
    return ret;
}

int iGameWeb::API::setColorBarOptions(int orientation, int numberOfLabels, int maximumNumberOfColors) {
    DebugLog("INFO", "API.setColorBarOptions called");
    int ret = iGameWeb::SetColorBarOptions(orientation, numberOfLabels, maximumNumberOfColors);
    if (ret > 0) { DebugLog("INFO", "API.setColorBarOptions success state=" + DescribeColorBarState()); }
    return ret;
}

int iGameWeb::API::setCornerAnnotationText(const std::string& text) {
    DebugLog("INFO", "API.setCornerAnnotationText called");
    return iGameWeb::SetCornerAnnotationText(text);
}

int iGameWeb::API::setCornerAnnotationPosition(float left, float top) {
    DebugLog("INFO", "API.setCornerAnnotationPosition called");
    return iGameWeb::SetCornerAnnotationPosition(left, top);
}

int iGameWeb::API::setCornerAnnotationAnchorToBottomRight(bool anchorToBottomRight) {
    DebugLog("INFO", std::string("API.setCornerAnnotationAnchorToBottomRight called anchorToBottomRight=") +
                             (anchorToBottomRight ? "true" : "false"));
    return iGameWeb::SetCornerAnnotationAnchorToBottomRight(anchorToBottomRight);
}

int iGameWeb::API::setCornerAnnotationVisible(bool visible) {
    DebugLog("INFO", std::string("API.setCornerAnnotationVisible called visible=") + (visible ? "true" : "false"));
    return iGameWeb::SetCornerAnnotationVisible(visible);
}

int iGameWeb::API::zoom(float factor) {
    DebugLog("INFO", "API.zoom called factor=" + std::to_string(factor));
    return iGameWeb::Zoom(factor);
}

void iGameWeb::API::sendMouseEvent(int type, int button, float x, float y, double delta) {
    iGameWeb::SendMouseEvent(type, button, x, y, delta);
    if (g_lastError.code != 0) {
        DebugLog("ERROR", "API.sendMouseEvent failed code=" + std::to_string(g_lastError.code) +
                                  " detail=" + g_lastError.detail);
    }
}

EMSCRIPTEN_BINDINGS(iGameWeb_bindings) {
    class_<iGameWeb::API>("iGameWeb")
            .class_function("init", &iGameWeb::API::init)
            .class_function("destroy", &iGameWeb::API::destroy)
            .class_function("stressLifecycleVtu", &iGameWeb::API::stressLifecycleVtu)
            .class_function("setDebugEnabled", &iGameWeb::API::setDebugEnabled)
            .class_function("raiseTestError", &iGameWeb::API::raiseTestError)
            .class_function("getLastErrorJson", &iGameWeb::API::getLastErrorJson)
            .class_function("clearLastError", &iGameWeb::API::clearLastError)
            .class_function("setSize", &iGameWeb::API::setSize)
            .class_function("loadVtkFromMem", &iGameWeb::API::loadVtkFromMem)
            .class_function("loadVtuFromMem", &iGameWeb::API::loadVtuFromMem)
            .class_function("loadVtpFromMem", &iGameWeb::API::loadVtpFromMem)
            .class_function("loadIgcFromMem", &iGameWeb::API::loadIgcFromMem)
            .class_function("loadIgcLegacy32FromMem", &iGameWeb::API::loadIgcLegacy32FromMem)
            .class_function("loadIgcLegacy64FromMem", &iGameWeb::API::loadIgcLegacy64FromMem)
            .class_function("loadVtkFromMemEx", &iGameWeb::API::loadVtkFromMemEx)
            .class_function("loadVtuFromMemEx", &iGameWeb::API::loadVtuFromMemEx)
            .class_function("loadVtpFromMemEx", &iGameWeb::API::loadVtpFromMemEx)
            .class_function("loadIgcFromMemEx", &iGameWeb::API::loadIgcFromMemEx)
            .class_function("loadIgcLegacy32FromMemEx", &iGameWeb::API::loadIgcLegacy32FromMemEx)
            .class_function("loadIgcLegacy64FromMemEx", &iGameWeb::API::loadIgcLegacy64FromMemEx)
            .class_function("loadZipFromMemEx", &iGameWeb::API::loadZipFromMemEx)
            .class_function("getModelListJson", &iGameWeb::API::getModelListJson)
            .class_function("setActiveModel", &iGameWeb::API::setActiveModel)
            .class_function("setModelVisibility", &iGameWeb::API::setModelVisibility)
            .class_function("removeModel", &iGameWeb::API::removeModel)
            .class_function("removeAllModels", &iGameWeb::API::removeAllModels)
            .class_function("clearViewer", &iGameWeb::API::clearViewer)
            .class_function("clear", &iGameWeb::API::clear)
            .class_function("setSelectionMode", &iGameWeb::API::setSelectionMode)
            .class_function("getSelectionMode", &iGameWeb::API::getSelectionMode)
            .class_function("enterSlicingMode", &iGameWeb::API::enterSlicingMode)
            .class_function("exitSlicingMode", &iGameWeb::API::exitSlicingMode)
            .class_function("isSlicingMode", &iGameWeb::API::isSlicingMode)
            .class_function("getSliceSourceModelId", &iGameWeb::API::getSliceSourceModelId)
            .class_function("getSliceResultModelId", &iGameWeb::API::getSliceResultModelId)
            .class_function("getClipPlaneJson", &iGameWeb::API::getClipPlaneJson)
            .class_function("setClipPlane", &iGameWeb::API::setClipPlane)
            .class_function("setSlicingPreview", &iGameWeb::API::setSlicingPreview)
            .class_function("setSliceOperationMode", &iGameWeb::API::setSliceOperationMode)
            .class_function("setSliceCrinkle", &iGameWeb::API::setSliceCrinkle)
            .class_function("setSliceInvert", &iGameWeb::API::setSliceInvert)
            .class_function("executeSlice", &iGameWeb::API::executeSlice)
            .class_function("getSelectionJson", &iGameWeb::API::getSelectionJson)
            .class_function("clearSelection", &iGameWeb::API::clearSelection)
            .class_function("setViewStyle", static_cast<int (*)(int)>(&iGameWeb::API::setViewStyle))
            .class_function("setViewStyle", static_cast<int (*)(int, int)>(&iGameWeb::API::setViewStyle))
            .class_function("getViewStyle", &iGameWeb::API::getViewStyle)
            .class_function("setActorOpacity", &iGameWeb::API::setActorOpacity)
            .class_function("getActorOpacity", &iGameWeb::API::getActorOpacity)
            .class_function("setScalarField", &iGameWeb::API::setScalarField)
            .class_function("setSurfaceShadingMode", &iGameWeb::API::setSurfaceShadingMode)
            .class_function("setBackgroundColor", &iGameWeb::API::setBackgroundColor)
            .class_function("setSceneBackgroundColor", &iGameWeb::API::setSceneBackgroundColor)
            .class_function("setSceneBackgroundGradientColor", &iGameWeb::API::setSceneBackgroundGradientColor)
            .class_function("setColorMap", &iGameWeb::API::setColorMap)
            .class_function("setColorMapByName", &iGameWeb::API::setColorMapByName)
            .class_function("setColorMapByNameRange", &iGameWeb::API::setColorMapByNameRange)
            .class_function("debugColorMapRangeSequence", &iGameWeb::API::debugColorMapRangeSequence)
            .class_function("getModelDefaultColor", &iGameWeb::API::getModelDefaultColor)
            .class_function("setSolidColor", &iGameWeb::API::setSolidColor)
            .class_function("getSolidColor", &iGameWeb::API::getSolidColor)
            .class_function("setHelperPen", &iGameWeb::API::setHelperPen)
            .class_function("setHelperBrush", &iGameWeb::API::setHelperBrush)
            .class_function("drawHelperSphere", &iGameWeb::API::drawHelperSphere)
            .class_function("drawHelperCylinder", &iGameWeb::API::drawHelperCylinder)
            .class_function("drawHelperCube", &iGameWeb::API::drawHelperCube)
            .class_function("drawHelperCircle", &iGameWeb::API::drawHelperCircle)
            .class_function("showHelper", &iGameWeb::API::showHelper)
            .class_function("hideHelper", &iGameWeb::API::hideHelper)
            .class_function("deleteHelper", &iGameWeb::API::deleteHelper)
            .class_function("clearHelpers", &iGameWeb::API::clearHelpers)
            .class_function("getColorMapJson", &iGameWeb::API::getColorMapJson)
            .class_function("getAttributeListJson", &iGameWeb::API::getAttributeListJson)
            .class_function("renderFrame", &iGameWeb::API::renderFrame)
            .class_function("resetCamera", &iGameWeb::API::resetCamera)
            .class_function("setCameraType", &iGameWeb::API::setCameraType)
            .class_function("viewXPlus", &iGameWeb::API::viewXPlus)
            .class_function("viewXMinus", &iGameWeb::API::viewXMinus)
            .class_function("viewYPlus", &iGameWeb::API::viewYPlus)
            .class_function("viewYMinus", &iGameWeb::API::viewYMinus)
            .class_function("viewZPlus", &iGameWeb::API::viewZPlus)
            .class_function("viewZMinus", &iGameWeb::API::viewZMinus)
            .class_function("viewIsometric", &iGameWeb::API::viewIsometric)
            .class_function("toggleAxes", &iGameWeb::API::toggleAxes)
            .class_function("toggleColorBar", &iGameWeb::API::toggleColorBar)
            .class_function("setColorBarLayout", &iGameWeb::API::setColorBarLayout)
            .class_function("setColorBarOptions", &iGameWeb::API::setColorBarOptions)
            .class_function("setCornerAnnotationText", &iGameWeb::API::setCornerAnnotationText)
            .class_function("setCornerAnnotationPosition", &iGameWeb::API::setCornerAnnotationPosition)
            .class_function("setCornerAnnotationAnchorToBottomRight",
                            &iGameWeb::API::setCornerAnnotationAnchorToBottomRight)
            .class_function("setCornerAnnotationVisible", &iGameWeb::API::setCornerAnnotationVisible)
            .class_function("zoom", &iGameWeb::API::zoom)
            .class_function("setModelDefaultColor", &iGameWeb::API::setModelDefaultColor)
            .class_function("sendMouseEvent", &iGameWeb::API::sendMouseEvent);
}
