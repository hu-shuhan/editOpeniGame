#include "iGameMeshReportGenerator.h"
#include "iGameMeshReportClient.h"
#include "iGameSurfaceMesh.h"
#include "iGameDrawObject.h"
#include "DataProcessing/iGameMeshTriangulationFilter.h"
#include "DataProcessing/iGameMeshSimplificationFilterPro.h"
#include "VTK/iGameVTKWriter.h"
#include "Log/iGameLogger.h"
#include <filesystem>
#include <chrono>
#include <cstdio>

IGAME_NAMESPACE_BEGIN

MeshReportGenerator::MeshReportGenerator(const std::string& reportSavePath,
                                          const std::string& serverHost,
                                          int serverPort)
    : m_reportSavePath(reportSavePath), m_serverHost(serverHost), m_serverPort(serverPort),
      m_simplificationRatio(0.1f), m_timeoutMs(300000), m_preserveBoundary(false) {}

MeshReportGenerator::~MeshReportGenerator() = default;

void MeshReportGenerator::SetInput(DataObject::Pointer input) {
    m_input = input;
}

DataObject::Pointer MeshReportGenerator::GetInput() const {
    return m_input;
}

void MeshReportGenerator::SetSimplificationRatio(float ratio) {
    m_simplificationRatio = ratio;
}

void MeshReportGenerator::SetTimeout(int timeoutMs) {
    m_timeoutMs = timeoutMs;
}

void MeshReportGenerator::SetPreserveBoundary(bool preserve) {
    m_preserveBoundary = preserve;
}

std::string MeshReportGenerator::GetErrorMessage() const {
    return m_errorMessage;
}

std::string MeshReportGenerator::GetReportSavePath() const {
    return m_reportSavePath;
}

bool MeshReportGenerator::Execute() {
    m_errorMessage.clear();

    if (!m_input) {
        m_errorMessage = "No input mesh set";
        return false;
    }

    igDebug("MeshReportGenerator: Starting report generation pipeline...");

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

    // 3. 导出为VTK临时文件并读取为二进制内容
    std::string tempVtkPath;
    std::vector<uint8_t> vtkData;
    if (!exportToVTKFile(simplified, tempVtkPath, vtkData)) {
        return false;
    }

    // 4. 发送到服务器并接收报告文件
    std::vector<uint8_t> reportData;
    bool sendOk = sendToServer(vtkData, reportData);

    // 无论发送是否成功，都清理临时VTK文件
    if (!tempVtkPath.empty()) {
        std::error_code ec;
        std::filesystem::remove(tempVtkPath, ec);
    }

    if (!sendOk) {
        return false;
    }

    // 5. 保存报告文件
    if (!saveReport(reportData)) {
        return false;
    }

    igDebug("MeshReportGenerator: Report generation complete! Saved to: {}", m_reportSavePath);
    return true;
}

bool MeshReportGenerator::triangulateInput(DataObject::Pointer input, DataObject::Pointer& triangulated) {
    igDebug("MeshReportGenerator: Triangulating mesh...");

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

bool MeshReportGenerator::simplifyMesh(DataObject::Pointer input, DataObject::Pointer& simplified) {
    igDebug("MeshReportGenerator: Simplifying mesh to {}%...", m_simplificationRatio * 100.0f);

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

bool MeshReportGenerator::exportToVTKFile(DataObject::Pointer mesh, std::string& tempVtkPath, std::vector<uint8_t>& vtkData) {
    igDebug("MeshReportGenerator: Exporting mesh to temp VTK file...");

    std::error_code ec;
    std::filesystem::path tempDir = std::filesystem::temp_directory_path(ec);
    if (ec) {
        m_errorMessage = "Failed to resolve system temp directory";
        return false;
    }

    std::string fileName = "igame_meshreport_" + std::to_string(reinterpret_cast<uintptr_t>(this))
                            + "_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".vtk";
    tempVtkPath = (tempDir / fileName).string();

    VTKWriter::Pointer writer = VTKWriter::New();
    if (!writer->WriteToFile(mesh, tempVtkPath)) {
        m_errorMessage = "Failed to write temp VTK file";
        return false;
    }

    FILE* f = fopen(tempVtkPath.c_str(), "rb");
    if (!f) {
        m_errorMessage = "Failed to reopen temp VTK file for reading";
        return false;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    vtkData.resize(size > 0 ? static_cast<size_t>(size) : 0);
    if (size > 0) {
        fread(vtkData.data(), 1, static_cast<size_t>(size), f);
    }
    fclose(f);

    if (vtkData.empty()) {
        m_errorMessage = "Temp VTK file is empty";
        return false;
    }

    igDebug("MeshReportGenerator: VTK data size: {} bytes", vtkData.size());
    return true;
}

bool MeshReportGenerator::sendToServer(const std::vector<uint8_t>& vtkData, std::vector<uint8_t>& reportData) {
    igDebug("MeshReportGenerator: Connecting to MeshReport server at {}:{}...", m_serverHost, m_serverPort);

    MeshReportClient client(m_serverHost, m_serverPort);
    client.setTimeout(m_timeoutMs);

    if (!client.connect()) {
        m_errorMessage = "Failed to connect to MeshReport server";
        return false;
    }

    igDebug("MeshReportGenerator: Sending report generation request...");

    MeshReportRequest request;
    request.vtkData = vtkData;

    MeshReportResponse response;
    if (!client.requestReport(request, response)) {
        m_errorMessage = "Failed to send report generation request";
        client.disconnect();
        return false;
    }

    client.disconnect();

    if (!response.success) {
        m_errorMessage = "Server error: " + response.errorMessage;
        return false;
    }

    reportData = response.reportData;
    igDebug("MeshReportGenerator: Received report data: {} bytes", reportData.size());
    return true;
}

bool MeshReportGenerator::saveReport(const std::vector<uint8_t>& reportData) {
    igDebug("MeshReportGenerator: Saving report to {}...", m_reportSavePath);

    FILE* f = fopen(m_reportSavePath.c_str(), "wb");
    if (!f) {
        m_errorMessage = "Failed to open report save path: " + m_reportSavePath;
        return false;
    }
    fwrite(reportData.data(), 1, reportData.size(), f);
    fclose(f);

    return true;
}

IGAME_NAMESPACE_END
