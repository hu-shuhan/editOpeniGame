#include "iGameP3SAMSegmenter.h"
#include "iGameP3SAMClient.h"
#include "iGameBlockMapping.h"
#include "iGameFileIO.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"
#include "iGameDrawObject.h"
#include "DataProcessing/iGameMeshTriangulationFilter.h"
#include "DataProcessing/iGameMeshSimplificationFilterPro.h"
#include "OBJ/iGameOBJWriter.h"
#include "Log/iGameLogger.h"

IGAME_NAMESPACE_BEGIN

P3SAMSegmenter::P3SAMSegmenter(const std::string& serverHost, int serverPort)
    : m_serverHost(serverHost), m_serverPort(serverPort),
      m_simplificationRatio(0.1f), m_postProcess(false), m_timeoutMs(300000),
      m_preserveBoundary(false), m_partCount(0) {}

P3SAMSegmenter::~P3SAMSegmenter() = default;

void P3SAMSegmenter::SetInput(DataObject::Pointer input) {
    m_input = input;
}

DataObject::Pointer P3SAMSegmenter::GetInput() const {
    return m_input;
}

DataObject::Pointer P3SAMSegmenter::GetOutput() {
    return m_output;
}

void P3SAMSegmenter::SetSimplificationRatio(float ratio) {
    m_simplificationRatio = ratio;
}

void P3SAMSegmenter::SetPostProcess(bool postProcess) {
    m_postProcess = postProcess;
}

void P3SAMSegmenter::SetTimeout(int timeoutMs) {
    m_timeoutMs = timeoutMs;
}

void P3SAMSegmenter::SetPreserveBoundary(bool preserve) {
    m_preserveBoundary = preserve;
}

std::string P3SAMSegmenter::GetErrorMessage() const {
    return m_errorMessage;
}

int P3SAMSegmenter::GetPartCount() const {
    return m_partCount;
}

bool P3SAMSegmenter::Execute() {
    m_errorMessage.clear();
    m_partCount = 0;
    m_output = nullptr;

    if (!m_input) {
        m_errorMessage = "No input mesh set";
        return false;
    }

    igDebug("P3SAMSegmenter: Starting segmentation pipeline...");

    // 1. 三角化
    DataObject::Pointer triangulated = nullptr;
    if (!triangulateInput(m_input, triangulated)) {
        return false;
    }

    // 2. 简化
    DataObject::Pointer simplified = nullptr;
    if (!simplifyMesh(triangulated, simplified)) {
        return false;
    }

    // 3. 导出为OBJ
    std::vector<uint8_t> objData;
    if (!exportToOBJ(simplified, objData)) {
        return false;
    }

    // 4. 发送到服务器并接收VTK结果
    std::vector<uint8_t> vtkData;
    if (!sendToServer(objData, vtkData)) {
        return false;
    }

    // 5. 解析VTK结果
    DataObject::Pointer segmented = nullptr;
    if (!parseVTKResult(vtkData, segmented)) {
        return false;
    }

    // 6. 映射回原始网格
    if (!mapBackToOriginal(m_input, segmented)) {
        return false;
    }

    m_output = m_input;
    igDebug("P3SAMSegmenter: Segmentation complete! Parts: {}", m_partCount);
    return true;
}

bool P3SAMSegmenter::triangulateInput(DataObject::Pointer input, DataObject::Pointer& triangulated) {
    igDebug("P3SAMSegmenter: Triangulating mesh...");

    // 先将任意类型转换为 SurfaceMesh（VolumeMesh 等非 SurfaceMesh 类型走此路径）
    DataObject::Pointer toTriangulate = input;
    auto drawObj = DynamicCast<DrawObject>(input);
    if (drawObj) {
        drawObj->ConvertToDrawableData();
        SurfaceMesh::Pointer surfaceMesh = DynamicCast<SurfaceMesh>(drawObj->GetRenderableObject(false));
        if (surfaceMesh) {
            toTriangulate = surfaceMesh;
        }
    }

    MeshTriangulationFilter::Pointer filter = MeshTriangulationFilter::New();
    filter->SetInput(toTriangulate);

    if (!filter->Execute()) {
        m_errorMessage = "Triangulation failed";
        return false;
    }

    triangulated = filter->GetOutput();
    if (!triangulated) {
        m_errorMessage = "Triangulation produced no output";
        return false;
    }

    return true;
}

bool P3SAMSegmenter::simplifyMesh(DataObject::Pointer input, DataObject::Pointer& simplified) {
    igDebug("P3SAMSegmenter: Fast simplifying mesh to {}%...", m_simplificationRatio * 100.0f);

    MeshSimplificationFilterPro::Pointer filter = MeshSimplificationFilterPro::New();
    filter->SetInput(input);
    filter->SetTargetReduction(m_simplificationRatio);
    filter->SetTargetFaceCount(0);
    filter->SetPreserveBoundary(m_preserveBoundary);
    filter->SetFreeze(true);
    filter->SetTransformToCellData(true);

    if (!filter->Execute()) {
        m_errorMessage = "Mesh simplification failed";
        return false;
    }

    simplified = filter->GetOutput();
    if (!simplified) {
        m_errorMessage = "Simplification produced no output";
        return false;
    }

    return true;
}

bool P3SAMSegmenter::exportToOBJ(DataObject::Pointer mesh, std::vector<uint8_t>& objData) {
    igDebug("P3SAMSegmenter: Exporting mesh to OBJ format...");

    OBJWriter::Pointer writer = OBJWriter::New();
    if (!writer->WriteToMemory(mesh, objData)) {
        m_errorMessage = "Failed to write OBJ to memory";
        return false;
    }

    if (objData.empty()) {
        m_errorMessage = "OBJ data is empty";
        return false;
    }

    igDebug("P3SAMSegmenter: OBJ data size: {} bytes", objData.size());
    return true;
}

bool P3SAMSegmenter::sendToServer(const std::vector<uint8_t>& objData, std::vector<uint8_t>& vtkData) {
    igDebug("P3SAMSegmenter: Connecting to P3SAM server at {}:{}...", m_serverHost, m_serverPort);

    P3SAMClient client(m_serverHost, m_serverPort);
    client.setTimeout(m_timeoutMs);

    if (!client.connect()) {
        m_errorMessage = "Failed to connect to P3SAM server";
        return false;
    }

    igDebug("P3SAMSegmenter: Sending segmentation request...");

    P3SAMRequest request;
    request.objData = objData;
    request.postProcess = m_postProcess;

    P3SAMResponse response;
    if (!client.requestSegmentation(request, response)) {
        m_errorMessage = "Failed to send segmentation request";
        client.disconnect();
        return false;
    }

    client.disconnect();

    if (!response.success) {
        m_errorMessage = "Server error: " + response.errorMessage;
        return false;
    }

    vtkData = response.vtkData;
    igDebug("P3SAMSegmenter: Received VTK data: {} bytes", vtkData.size());
    return true;
}

bool P3SAMSegmenter::parseVTKResult(const std::vector<uint8_t>& vtkData, DataObject::Pointer& segmented) {
    igDebug("P3SAMSegmenter: Parsing VTK result...");

    segmented = FileIO::ReadVTKFromMemory(vtkData.data(), vtkData.size());
    if (!segmented) {
        m_errorMessage = "Failed to parse VTK data";
        return false;
    }

    return true;
}

bool P3SAMSegmenter::mapBackToOriginal(DataObject::Pointer original, DataObject::Pointer segmented) {
    igDebug("P3SAMSegmenter: Mapping segmentation back to original mesh...");

    UnstructuredMesh::Pointer unstructuredMesh = DynamicCast<UnstructuredMesh>(segmented);
    if (!unstructuredMesh) {
        m_errorMessage = "Segmented result is not an UnstructuredMesh";
        return false;
    }

    IntArray::Pointer resultArray;
    IGenum type = original->GetDataObjectType();

    switch (type) {
        case IG_SURFACE_MESH: {
            SurfaceMesh::Pointer sm = DynamicCast<SurfaceMesh>(original);
            if (!sm) { m_errorMessage = "Failed to cast to SurfaceMesh"; return false; }
            resultArray = BlockMapping::GetMappingBlockCellsArray(sm, unstructuredMesh);
            break;
        }
        case IG_UNSTRUCTURED_MESH: {
            UnstructuredMesh::Pointer um = DynamicCast<UnstructuredMesh>(original);
            if (!um) { m_errorMessage = "Failed to cast to UnstructuredMesh"; return false; }
            resultArray = BlockMapping::GetMappingBlockCellsArray(um, unstructuredMesh);
            break;
        }
        case IG_VOLUME_MESH: {
            VolumeMesh::Pointer vm = DynamicCast<VolumeMesh>(original);
            if (!vm) { m_errorMessage = "Failed to cast to VolumeMesh"; return false; }
            resultArray = BlockMapping::GetMappingBlockCellsArray(vm, unstructuredMesh);
            break;
        }
        default:
            m_errorMessage = "Unsupported mesh type for block mapping";
            return false;
    }

    if (!resultArray) {
        m_errorMessage = "Block mapping failed";
        return false;
    }

    resultArray->SetName("part_id");
    original->SetBlockMapping(resultArray);

    // 统计部件数量
    m_partCount = 0;
    for (int i = 0; i < resultArray->GetNumberOfValues(); ++i) {
        int partId = resultArray->GetValue(i);
        if (partId >= 0 && partId >= m_partCount) {
            m_partCount = partId + 1;
        }
    }

    igDebug("P3SAMSegmenter: Mapping complete. Total parts: {}", m_partCount);
    return true;
}

IGAME_NAMESPACE_END
