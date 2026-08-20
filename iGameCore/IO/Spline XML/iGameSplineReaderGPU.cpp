#if defined(GPSCUDA_ENABLE)
#include <MeshKernel/Mesh.h>
#include <cmath>

#include "iGameSplineReaderGPU.h"

#include "GPSpline/iGameGPSplinePatchSurface.h"
#include <GPHelperIO/iGameGP_Surface_Convert.h>
#include <GPSpline/iGameGPCadscene.h>

#include <algorithm>
#include <array>
#include <iGameFileReader.h>
#include <limits>
#include <regex>
#include <sstream>
#include <tinyxml2.h>

#include "iGameSurfaceMesh.h"
#include "iGameVolumeMesh.h"
#include <chrono>
#include <fstream>
IGAME_NAMESPACE_BEGIN
int maxpq = 3;
int isoNum = maxpq * 8 - 2;

namespace
{
struct KnotVectorInfo {
    std::vector<double> values;
    int degree = -1;
    int direction = -1;
};

struct ControlNetData {
    std::vector<Vector3d> points;
    std::vector<std::array<igIndex, 2>> edges;
    std::vector<std::array<double, 3>> scalars;
};

struct ControlDrawData {
    UnsignedIntArray::Pointer pointIndices;
    UnsignedIntArray::Pointer lineIndices;
    CellArray::Pointer edges;
    IGsize linePointOffset = 0;
    std::vector<Vector3d> linePoints;
};

bool PadRenderOnlyVertexArray(FloatArray::Pointer& array, IGsize basePointCount, IGsize totalPointCount) {
    if (array == nullptr || array->GetNumberOfElements() == 0 || array->GetNumberOfElements() == totalPointCount) {
        return true;
    }
    if (array->GetNumberOfElements() != basePointCount) { return false; }

    array->Resize(totalPointCount);
    array->Modified();
    return true;
}

bool PrepareBlackControlLines(FloatArray::Pointer& positions, FloatArray::Pointer& colors, FloatArray::Pointer& normals,
                              FloatArray::Pointer& textures, bool useColor, const ControlDrawData& drawData) {
    if (positions == nullptr || drawData.linePoints.empty()) { return false; }

    const IGsize linePointCount = static_cast<IGsize>(drawData.linePoints.size());
    const IGsize totalPointCount = drawData.linePointOffset + linePointCount;
    const IGsize positionCount = positions->GetNumberOfElements();

    if (positionCount == drawData.linePointOffset) {
        // Points::ConvertToArray() returns the mesh's own buffer. Make a
        // render-only copy so the extra line vertices do not alter the mesh.
        FloatArray::Pointer renderPositions = FloatArray::New();
        renderPositions->DeepCopy(positions);
        renderPositions->Resize(totalPointCount);
        for (IGsize pointId = 0; pointId < linePointCount; ++pointId) {
            const auto& point = drawData.linePoints[pointId];
            float value[3] = {static_cast<float>(point[0]), static_cast<float>(point[1]), static_cast<float>(point[2])};
            renderPositions->SetElement(drawData.linePointOffset + pointId, value);
        }
        renderPositions->Modified();
        positions = renderPositions;
    } else if (positionCount != totalPointCount) {
        IGAME_CORE_ERROR("[SplineReaderGPU]: Unexpected drawable point count while preparing black control lines: {} "
                         "(expected {} or {}).",
                         positionCount, drawData.linePointOffset, totalPointCount);
        return false;
    }

    // Point-scalar rendering uses the vertex-color buffer for explicit lines.
    // Only the render-only line vertices are black; surface and point colors stay unchanged.
    if (useColor) {
        if (colors == nullptr ||
            (colors->GetNumberOfElements() != 0 && colors->GetNumberOfElements() != drawData.linePointOffset &&
             colors->GetNumberOfElements() != totalPointCount)) {
            IGAME_CORE_ERROR("[SplineReaderGPU]: Unexpected drawable color count while preparing control lines.");
            return false;
        }

        if (colors->GetNumberOfElements() != totalPointCount) {
            colors->Resize(totalPointCount);
            std::vector<float> black(static_cast<size_t>(colors->GetDimension()), 0.0f);
            if (black.size() > 3) { black[3] = 1.0f; }
            for (IGsize pointId = 0; pointId < linePointCount; ++pointId) {
                colors->SetElement(drawData.linePointOffset + pointId, black.data());
            }
            colors->Modified();
        }
    }

    // Keep every enabled vertex buffer long enough for the render-only indices.
    // The line shaders do not use normal/UV values, so zero-filled tails are sufficient.
    if (!PadRenderOnlyVertexArray(normals, drawData.linePointOffset, totalPointCount) ||
        !PadRenderOnlyVertexArray(textures, drawData.linePointOffset, totalPointCount)) {
        IGAME_CORE_ERROR("[SplineReaderGPU]: Unexpected vertex attribute count while preparing control lines.");
        return false;
    }

    return true;
}

bool ParseNumberList(const char* text, std::vector<double>& values) {
    if (text == nullptr) { return false; }

    std::istringstream stream(text);
    double value = 0.0;
    while (stream >> value) { values.push_back(value); }
    return stream.eof() && !values.empty();
}

bool CollectKnotVectors(tinyxml2::XMLElement* element, std::vector<KnotVectorInfo>& knotVectors,
                        int inheritedDirection = -1) {
    int direction = inheritedDirection;
    element->QueryIntAttribute("index", &direction);

    for (auto* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        if (std::string(child->Name()) == "KnotVector") {
            KnotVectorInfo info;
            child->QueryIntAttribute("degree", &info.degree);
            info.direction = direction;
            child->QueryIntAttribute("index", &info.direction);
            if (!ParseNumberList(child->GetText(), info.values)) { return false; }
            knotVectors.emplace_back(std::move(info));
        } else if (!CollectKnotVectors(child, knotVectors, direction)) {
            return false;
        }
    }
    return true;
}

bool ReadControlNet(tinyxml2::XMLElement* xmlRoot, ControlNetData& controlNet) {
    if (xmlRoot == nullptr) {
        IGAME_CORE_ERROR("[SplineReaderGPU]: XML root is null while reading the control net.");
        return false;
    }

    int geometryCount = 0;
    for (auto* geometry = xmlRoot->FirstChildElement("Geometry"); geometry;
         geometry = geometry->NextSiblingElement("Geometry"), ++geometryCount) {
        auto* basis = geometry->FirstChildElement("Basis");
        if (basis == nullptr) {
            IGAME_CORE_ERROR("[SplineReaderGPU]: <Basis> not found in Geometry {}.", geometryCount);
            return false;
        }

        std::vector<KnotVectorInfo> knotVectors;
        if (!CollectKnotVectors(basis, knotVectors) || knotVectors.size() != 3) {
            IGAME_CORE_ERROR("[SplineReaderGPU]: Geometry {} must contain three valid knot vectors.", geometryCount);
            return false;
        }

        std::array<KnotVectorInfo*, 3> orderedKnotVectors{};
        const bool hasDirections = std::any_of(knotVectors.begin(), knotVectors.end(),
                                               [](const KnotVectorInfo& info) { return info.direction >= 0; });
        if (hasDirections) {
            for (auto& knotVector: knotVectors) {
                if (knotVector.direction < 0 || knotVector.direction >= 3 ||
                    orderedKnotVectors[knotVector.direction] != nullptr) {
                    IGAME_CORE_ERROR("[SplineReaderGPU]: Invalid or duplicate Basis index in Geometry {}.",
                                     geometryCount);
                    return false;
                }
                orderedKnotVectors[knotVector.direction] = &knotVector;
            }
        } else {
            for (size_t direction = 0; direction < orderedKnotVectors.size(); ++direction) {
                orderedKnotVectors[direction] = &knotVectors[direction];
            }
        }

        std::array<size_t, 3> controlCount{};
        for (size_t direction = 0; direction < controlCount.size(); ++direction) {
            auto& knotVector = *orderedKnotVectors[direction];
            if (knotVector.degree < 0) {
                size_t startMultiplicity = 1;
                while (startMultiplicity < knotVector.values.size() &&
                       std::fabs(knotVector.values[startMultiplicity] - knotVector.values.front()) < 1e-9) {
                    ++startMultiplicity;
                }
                knotVector.degree = static_cast<int>(startMultiplicity) - 1;
            }

            if (knotVector.degree < 0 || knotVector.values.size() <= static_cast<size_t>(knotVector.degree + 1)) {
                IGAME_CORE_ERROR("[SplineReaderGPU]: Invalid knot vector in Geometry {}, direction {}.", geometryCount,
                                 direction);
                return false;
            }
            controlCount[direction] = knotVector.values.size() - static_cast<size_t>(knotVector.degree) - 1;
        }

        auto* coefs = geometry->FirstChildElement("coefs");
        if (coefs == nullptr) { coefs = geometry->FirstChildElement("Coefs"); }
        std::vector<double> coefficients;
        if (coefs == nullptr || !ParseNumberList(coefs->GetText(), coefficients)) {
            IGAME_CORE_ERROR("[SplineReaderGPU]: Invalid <coefs> in Geometry {}.", geometryCount);
            return false;
        }

        size_t expectedPointCount = 1;
        for (const size_t count: controlCount) {
            if (count == 0 || expectedPointCount > std::numeric_limits<size_t>::max() / count) {
                IGAME_CORE_ERROR("[SplineReaderGPU]: Control point count overflow in Geometry {}.", geometryCount);
                return false;
            }
            expectedPointCount *= count;
        }
        if (expectedPointCount > std::numeric_limits<size_t>::max() / 3 ||
            coefficients.size() != expectedPointCount * 3) {
            IGAME_CORE_ERROR("[SplineReaderGPU]: Geometry {} has {} control-point values, but {} are required.",
                             geometryCount, coefficients.size(), expectedPointCount * 3);
            return false;
        }

        const size_t maximumPointCount = static_cast<size_t>(std::numeric_limits<igIndex>::max());
        if (expectedPointCount > maximumPointCount ||
            controlNet.points.size() > maximumPointCount - expectedPointCount) {
            IGAME_CORE_ERROR("[SplineReaderGPU]: Too many control points in the input file.");
            return false;
        }

        std::vector<double> scalarValues;
        bool hasValidScalars = false;
        if (auto* scalarElement = geometry->FirstChildElement("scalars")) {
            int dimension = 3;
            scalarElement->QueryIntAttribute("dim", &dimension);
            hasValidScalars = dimension == 3 && ParseNumberList(scalarElement->GetText(), scalarValues) &&
                              scalarValues.size() == expectedPointCount * 3;
            if (!hasValidScalars) {
                IGAME_CORE_WARN("[SplineReaderGPU]: Geometry {} has invalid control-point scalars; zeros will be used.",
                                geometryCount);
            }
        }

        const igIndex pointOffset = static_cast<igIndex>(controlNet.points.size());
        for (size_t pointId = 0; pointId < expectedPointCount; ++pointId) {
            controlNet.points.emplace_back(coefficients[pointId * 3], coefficients[pointId * 3 + 1],
                                           coefficients[pointId * 3 + 2]);
            if (hasValidScalars) {
                controlNet.scalars.push_back(
                        {scalarValues[pointId * 3], scalarValues[pointId * 3 + 1], scalarValues[pointId * 3 + 2]});
            } else {
                controlNet.scalars.push_back({0.0, 0.0, 0.0});
            }
        }

        const size_t nu = controlCount[0];
        const size_t nv = controlCount[1];
        const size_t nw = controlCount[2];
        auto pointId = [=](size_t u, size_t v, size_t w) {
            return pointOffset + static_cast<igIndex>(w * nu * nv + v * nu + u);
        };

        for (size_t w = 0; w < nw; ++w) {
            for (size_t v = 0; v < nv; ++v) {
                for (size_t u = 0; u + 1 < nu; ++u) {
                    controlNet.edges.push_back({pointId(u, v, w), pointId(u + 1, v, w)});
                }
            }
        }
        for (size_t w = 0; w < nw; ++w) {
            for (size_t v = 0; v + 1 < nv; ++v) {
                for (size_t u = 0; u < nu; ++u) {
                    controlNet.edges.push_back({pointId(u, v, w), pointId(u, v + 1, w)});
                }
            }
        }
        for (size_t w = 0; w + 1 < nw; ++w) {
            for (size_t v = 0; v < nv; ++v) {
                for (size_t u = 0; u < nu; ++u) {
                    controlNet.edges.push_back({pointId(u, v, w), pointId(u, v, w + 1)});
                }
            }
        }
    }

    if (geometryCount == 0 || controlNet.points.empty()) {
        IGAME_CORE_ERROR("[SplineReaderGPU]: No control points were read from the file.");
        return false;
    }

    return true;
}

bool AppendControlNet(SurfaceMesh* mesh, const ControlNetData& controlNet, bool replaceMeshEdges,
                      ControlDrawData& drawData) {
    if (mesh == nullptr || mesh->GetPoints() == nullptr) { return false; }

    const IGsize pointOffset = mesh->GetNumberOfPoints();
    const IGsize maximumPointCount = static_cast<IGsize>(std::numeric_limits<igIndex>::max());
    const IGsize controlPointCount = static_cast<IGsize>(controlNet.points.size());
    if (pointOffset > maximumPointCount || controlPointCount > (maximumPointCount - pointOffset) / 2) {
        IGAME_CORE_ERROR("[SplineReaderGPU]: The combined GPU and control-point mesh is too large to render.");
        return false;
    }

    drawData.pointIndices = UnsignedIntArray::New();
    drawData.pointIndices->SetDimension(1);
    drawData.lineIndices = UnsignedIntArray::New();
    drawData.lineIndices->SetDimension(2);
    drawData.edges = CellArray::New();

    for (size_t pointId = 0; pointId < controlNet.points.size(); ++pointId) {
        mesh->GetPoints()->AddPoint(controlNet.points[pointId]);
        drawData.pointIndices->AddValue(static_cast<iguIndex>(pointOffset + pointId));
    }

    drawData.linePointOffset = mesh->GetNumberOfPoints();
    drawData.linePoints = controlNet.points;

    for (const auto& edge: controlNet.edges) {
        const igIndex first = static_cast<igIndex>(pointOffset) + edge[0];
        const igIndex second = static_cast<igIndex>(pointOffset) + edge[1];
        drawData.edges->AddCellId2(first, second);
        drawData.lineIndices->AddElement2(static_cast<iguIndex>(drawData.linePointOffset + edge[0]),
                                          static_cast<iguIndex>(drawData.linePointOffset + edge[1]));
    }
    drawData.pointIndices->Modified();
    drawData.lineIndices->Modified();
    if (replaceMeshEdges) { mesh->SetEdges(drawData.edges); }

    const IGsize combinedPointCount = mesh->GetNumberOfPoints();
    auto attributes = mesh->GetAttributeSet();
    for (IGsize attributeId = 0; attributes && attributeId < attributes->GetNumberOfAttributes(); ++attributeId) {
        auto& attribute = attributes->GetAttribute(attributeId);
        if (attribute.isDeleted || attribute.attachmentType != IG_POINT || attribute.pointer == nullptr) { continue; }

        DoubleArray::Pointer originalDataRange = DoubleArray::New();
        originalDataRange->DeepCopy(attribute.GetDataRange());
        attribute.pointer->Resize(combinedPointCount);
        if (attribute.pointer->GetDimension() == 3 && attribute.pointer->GetName() == "scalar3" &&
            controlNet.scalars.size() == controlNet.points.size()) {
            for (size_t pointId = 0; pointId < controlNet.scalars.size(); ++pointId) {
                double value[3] = {controlNet.scalars[pointId][0], controlNet.scalars[pointId][1],
                                   controlNet.scalars[pointId][2]};
                attribute.pointer->SetElement(pointOffset + pointId, value);
            }
        }
        attribute.pointer->Modified();
        attribute.SetDataRange(originalDataRange);
    }

    return true;
}

class ControlNetSurfaceMesh final : public SurfaceMesh {
public:
    using Pointer = SmartPointer<ControlNetSurfaceMesh>;
    static Pointer New() { return new ControlNetSurfaceMesh; }

    bool IsUseSinglePassWireframeRendering() override { return false; }

    void SetControlDrawData(const ControlDrawData& drawData) {
        m_ControlDrawData = drawData;
        m_PointIndices = m_ControlDrawData.pointIndices;
        m_LineIndices = m_ControlDrawData.lineIndices;
    }

    void ConvertToDrawableData() override {
        // Keep the indices rebuilt from logical mesh edges separate from the
        // render-only line indices that reference black vertex copies.
        m_LineIndices = m_SurfaceLineIndices;
        const bool shellRendering = m_ShellRendering;
        m_ShellRendering = false;
        SurfaceMesh::ConvertToDrawableData();
        m_ShellRendering = shellRendering;
        m_RenderableMesh.SimplifiedMesh = nullptr;
        if (m_ControlDrawData.pointIndices != nullptr) { m_PointIndices = m_ControlDrawData.pointIndices; }
        if (m_ControlDrawData.lineIndices != nullptr &&
            PrepareBlackControlLines(m_Positions, m_Colors, m_Normals, m_Textures, m_UseColor, m_ControlDrawData)) {
            m_LineIndices = m_ControlDrawData.lineIndices;
        }
    }

protected:
    ControlNetSurfaceMesh() {
        m_SurfaceLineIndices = UnsignedIntArray::New();
        m_SurfaceLineIndices->SetDimension(2);
    }
    ~ControlNetSurfaceMesh() override = default;

private:
    ControlDrawData m_ControlDrawData;
    UnsignedIntArray::Pointer m_SurfaceLineIndices;
};

class ControlNetVolumeMesh final : public VolumeMesh {
public:
    using Pointer = SmartPointer<ControlNetVolumeMesh>;
    static Pointer New() { return new ControlNetVolumeMesh; }

    bool IsUseSinglePassWireframeRendering() override { return false; }

    void SetControlNet(const ControlNetData& controlNet, const ControlDrawData& drawData) {
        m_ControlNet = controlNet;
        m_ControlDrawData = drawData;
        m_HasControlNet = true;
    }

    void ConvertToDrawableData() override {
        VolumeMesh::ConvertToDrawableData();
        if (!m_HasControlNet) { return; }

        if (!m_ShellRendering) {
            if (m_PointIndices.GetPointer() != m_ControlDrawData.pointIndices.GetPointer()) {
                m_PointIndices = m_ControlDrawData.pointIndices;
                m_PointIndices->Modified();
            }
            if (PrepareBlackControlLines(m_Positions, m_Colors, m_Normals, m_Textures, m_UseColor, m_ControlDrawData) &&
                m_LineIndices.GetPointer() != m_ControlDrawData.lineIndices.GetPointer()) {
                m_LineIndices = m_ControlDrawData.lineIndices;
                m_LineIndices->Modified();
            }
            return;
        }

        if (DynamicCast<ControlNetSurfaceMesh>(m_RenderableMesh.SurfaceMesh) != nullptr) { return; }
        auto extractedSurface = DynamicCast<SurfaceMesh>(m_RenderableMesh.SurfaceMesh);
        if (extractedSurface == nullptr) { return; }

        ControlNetSurfaceMesh::Pointer renderSurface = ControlNetSurfaceMesh::New();
        Points::Pointer renderPoints = Points::New();
        renderPoints->DeepCopy(extractedSurface->GetPoints());
        renderSurface->SetPoints(renderPoints);

        CellArray::Pointer renderFaces = extractedSurface->GetFaces();
        if (renderFaces == nullptr) { renderFaces = CellArray::New(); }
        renderSurface->SetFaces(renderFaces);

        AttributeSet::Pointer renderAttributes = AttributeSet::New();
        renderAttributes->DeepCopy(extractedSurface->GetAttributeSet());
        renderSurface->SetAttributeSet(renderAttributes);

        ControlDrawData renderDrawData;
        if (!AppendControlNet(renderSurface, m_ControlNet, true, renderDrawData)) { return; }
        renderSurface->SetControlDrawData(renderDrawData);

        const bool shellRendering = m_ShellRendering;
        m_ShellRendering = false;
        SetRenderableObject(renderSurface);
        m_ShellRendering = shellRendering;
        m_RenderableMesh.SimplifiedMesh = nullptr;
    }

protected:
    ControlNetVolumeMesh() = default;
    ~ControlNetVolumeMesh() override = default;

private:
    ControlNetData m_ControlNet;
    ControlDrawData m_ControlDrawData;
    bool m_HasControlNet = false;
};
} // namespace

SplineReaderGPU::SplineReaderGPU() {
    SetNumberOfInputs(0);
    SetNumberOfOutputs(1);
}

bool SplineReaderGPU::Parsing() {
    // 判断是否是体数据
    auto* geometry = root->FirstChildElement("Geometry");
    {
        const char* delimiters = " \n\t\r";

        if (!geometry) { return false; }

        // Basis may be nested; support both 'Basis' and 'basis'
        auto* basisRoot = geometry->FirstChildElement("Basis");
        if (!basisRoot) {
            IGAME_CORE_ERROR("[SplineReaderCGU]: <basis> not found under <geometry>.");
            return false;
        }
        // collect knot vectors from nested basis nodes
        std::vector<std::vector<double>> knots;
        auto parseKnotVectorText = [&](const char* text) {
            std::vector<double> kv;
            if (!text) return kv;
            std::string value = text;
            char* data_p = const_cast<char*>(value.data());
            while (*data_p == '\n' || *data_p == ' ' || *data_p == '\t' || *data_p == '\r') data_p++;
            char* token = strtok(const_cast<char*>(value.c_str()), delimiters);
            while (token != nullptr) {
                float f = mAtof(token);
                token = strtok(nullptr, delimiters);
                kv.push_back(static_cast<double>(f));
            }
            return kv;
        };

        tinyxml2::XMLElement* container = nullptr;
        if (auto* b1 = basisRoot->FirstChildElement("Basis")) {
            container = b1->FirstChildElement("Basis") ? b1 : basisRoot;
        }
        if (container) {
            for (auto* b = container->FirstChildElement("Basis"); b; b = b->NextSiblingElement("Basis")) {
                auto* kvElem = b->FirstChildElement("KnotVector");
                if (!kvElem) { kvElem = b->FirstChildElement("KnotVector"); }
                if (!kvElem) { kvElem = b->FirstChildElement("KnotVector"); }
                if (kvElem) {
                    auto kv = parseKnotVectorText(kvElem->GetText());
                    if (!kv.empty()) { knots.emplace_back(std::move(kv)); }
                }
            }
        } else {
            if (auto* kvElem = basisRoot->FirstChildElement("KnotVector"); kvElem) {
                auto kv = parseKnotVectorText(kvElem->GetText());
                if (!kv.empty()) { knots.emplace_back(std::move(kv)); }
            }
        }

        const int num = static_cast<int>(knots.size());
        if (num <= 0 || num > 3) {
            IGAME_CORE_ERROR("[SplineReaderGPU]: Unsupported number of parametric directions: {}", num);
            return false;
        }

        if (num != 3) {
            igDebug("[SplineReaderCGU]: can not process non-volume data.");
            return false;
        }
    }

    return true;
}

bool SplineReaderGPU::CreateDataObject() {
    ControlNetData controlNet;
    if (!ReadControlNet(root, controlNet)) { return false; }

    // 调用GPU离散成表面网格
    DataObject::Pointer output = nullptr;
    if (m_SurfaceRenderForVolume) {
        gpmesh::CadSceneGP m_scene_gp;
        ControlNetSurfaceMesh::Pointer surfaceMesh = ControlNetSurfaceMesh::New();
        output = surfaceMesh;

        CSFile* csf;
        bool isSurface = true;
        gpbezier::SurfaceConvertHelper SurfaceHelper;

        SurfaceHelper.readfile(m_FilePath.c_str(), isSurface, 0);


        m_scene_gp.init_CUDA_map_mode(true);
        UpdateProgress(0.2f);
        //m_scene_gp.release();

        // 开始计时
        auto start_time = std::chrono::high_resolution_clock::now();
        m_scene_gp.init_scene(SurfaceHelper, maxpq);
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double>(end_time - start_time);
        igDebug("GPU Processing Time: {}s", duration.count());

        //std::vector<gpmesh::real_t*> result = m_scene_gp.m_cuda_ptr_arr;
        //std::vector<gpmesh::real_t*> normal = m_scene_gp.m_cuda_normal_arr;
        //std::vector<gpmesh::real_t*> scalar = m_scene_gp.m_cuda_scalar_arr;
        auto& result = m_scene_gp.m_cuda_ptr_arr;
        auto& normal = m_scene_gp.m_cuda_normal_arr;
        auto& scalar = m_scene_gp.m_cuda_scalar_arr;
        UpdateProgress(0.6f);
        FloatArray::Pointer scalarArray = FloatArray::New();
        scalarArray->SetDimension(3);
        scalarArray->SetName("scalar3");
        surfaceMesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, scalarArray);
        CellArray::Pointer faces = CellArray::New();
        surfaceMesh->SetFaces(faces);

        //for (auto i = 0; i < m_scene_gp.get_patchsurface_num(); i++) {
        //    for (auto p = 0; p < maxpq; p++) {
        //        for (auto q = 0; q < maxpq; q++) {
        //            igIndex face[8][8]{};
        //            for (auto j = 0; j < 64; j++) {
        //                Point x = {result[i][(p * maxpq + q) * 64 * 3 + j * 3],
        //                           result[i][(p * maxpq + q) * 64 * 3 + j * 3 + 1],
        //                           result[i][(p * maxpq + q) * 64 * 3 + j * 3 + 2]};
        //                face[j / 8][j % 8] = surfaceMesh->AddPoint(x);
        //                float value[3] = {scalar[i][(p * maxpq + q) * 64 * 3 + j * 3],
        //                                  scalar[i][(p * maxpq + q) * 64 * 3 + j * 3 + 1],
        //                                  scalar[i][(p * maxpq + q) * 64 * 3 + j * 3 + 2]};
        //                scalarArray->AddElement(value);
        //            }

        //            for (auto j = 0; j < 7; j++) {
        //                for (auto k = 0; k < 7; k++) {
        //                    if (j == 0) {
        //                        if (k == 0) {
        //                            surfaceMesh->AddEdge(face[j][k], face[j][k + 1]);
        //                            surfaceMesh->AddEdge(face[j][k + 1], face[j + 1][k]);
        //                            surfaceMesh->AddEdge(face[j][k], face[j + 1][k]);
        //                            surfaceMesh->AddEdge(face[j][k + 1], face[j + 1][k + 1]);
        //                            surfaceMesh->AddEdge(face[j + 1][k + 1], face[j + 1][k]);
        //                        } else {
        //                            surfaceMesh->AddEdge(face[j][k], face[j][k + 1]);
        //                            surfaceMesh->AddEdge(face[j][k + 1], face[j + 1][k]);
        //                            surfaceMesh->AddEdge(face[j][k + 1], face[j + 1][k + 1]);
        //                            surfaceMesh->AddEdge(face[j + 1][k + 1], face[j + 1][k]);
        //                        }
        //                    } else {
        //                        if (k == 0) {
        //                            surfaceMesh->AddEdge(face[j][k + 1], face[j + 1][k]);
        //                            surfaceMesh->AddEdge(face[j][k], face[j + 1][k]);
        //                            surfaceMesh->AddEdge(face[j][k + 1], face[j + 1][k + 1]);
        //                            surfaceMesh->AddEdge(face[j + 1][k + 1], face[j + 1][k]);
        //                        } else {
        //                            surfaceMesh->AddEdge(face[j][k + 1], face[j + 1][k]);
        //                            surfaceMesh->AddEdge(face[j][k + 1], face[j + 1][k + 1]);
        //                            surfaceMesh->AddEdge(face[j + 1][k + 1], face[j + 1][k]);
        //                        }
        //                    }

        //                    faces->AddCellId3(face[j][k], face[j][k + 1], face[j + 1][k]);
        //                    faces->AddCellId3(face[j + 1][k], face[j][k + 1], face[j + 1][k + 1]);
        //                }
        //            }
        //        }
        //    }
        //}
        for (auto i = 0; i < m_scene_gp.get_patchsurface_num(); i++) {
            for (auto p = 0; p < maxpq; p++) {
                for (auto q = 0; q < maxpq; q++) {

                    igIndex face[8][8]{};
                    for (auto j = 0; j < 64; j++) {
                        Point x = {result[i][(p * maxpq + q) * 64 * 3 + j * 3],
                                   result[i][(p * maxpq + q) * 64 * 3 + j * 3 + 1],
                                   result[i][(p * maxpq + q) * 64 * 3 + j * 3 + 2]};

                        face[j / 8][j % 8] = surfaceMesh->AddPoint(x);

                        float value[3] = {scalar[i][(p * maxpq + q) * 64 * 3 + j * 3],
                                          scalar[i][(p * maxpq + q) * 64 * 3 + j * 3 + 1],
                                          scalar[i][(p * maxpq + q) * 64 * 3 + j * 3 + 2]};
                        scalarArray->AddElement(value);
                    }
                    for (int j = 0; j < 7; j++) {
                        for (int k = 0; k < 7; k++) {
                            faces->AddCellId4(face[j][k], face[j][k + 1], face[j + 1][k + 1], face[j + 1][k]);
                        }
                    }
                }
            }
        }
    } else {
        ControlNetVolumeMesh::Pointer volumeMesh = ControlNetVolumeMesh::New();
        output = volumeMesh;

        CSFile* csf;
        bool isSurface = false;
        gpmesh::CadSceneGP m_scene_gp;
        gpbezier::SurfaceConvertHelper SurfaceHelper;
        SurfaceHelper.readfile(m_FilePath.c_str(), isSurface, isoNum);

        m_scene_gp.init_CUDA_map_mode(true);
        UpdateProgress(0.2f);

        // 开始计时
        auto start_time = std::chrono::high_resolution_clock::now();
        m_scene_gp.init_scene(SurfaceHelper, maxpq);
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double>(end_time - start_time);
        igDebug("GPU Processing Time: {}s", duration.count());

        UpdateProgress(0.7f);
        //std::vector<gpmesh::real_t*> result = m_scene_gp.m_cuda_ptr_arr;
        //std::vector<gpmesh::real_t*> normal = m_scene_gp.m_cuda_normal_arr;
        //std::vector<gpmesh::real_t*> scalar = m_scene_gp.m_cuda_scalar_arr;
        auto& result = m_scene_gp.m_cuda_ptr_arr;
        auto& normal = m_scene_gp.m_cuda_normal_arr;
        auto& scalar = m_scene_gp.m_cuda_scalar_arr;
        //std::vector<gpmesh::GPSplinePatchSurface> surfacepatch = m_scene_gp.host_patchsurfaces;
        auto& surfacepatch = m_scene_gp.host_patchsurfaces;
        FloatArray::Pointer scalarArray = FloatArray::New();
        scalarArray->SetDimension(3);
        scalarArray->SetName("scalar3");

        int64_t surface_num = SurfaceHelper.getSurfaces().size();
        Points::Pointer Points = Points::New();
        CellArray::Pointer Volume = CellArray::New();

        volumeMesh->SetPoints(Points);
        volumeMesh->SetVolumes(Volume);
        volumeMesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, scalarArray);

        for (auto i = 0; i < surface_num / isoNum; i++) {
            std::vector<std::vector<igIndex>> ids(isoNum);
            for (auto k = 0; k < isoNum; k++) {
                ids[k].resize(maxpq * maxpq * 64);
                for (auto p = 0; p < maxpq; p++) {
                    for (auto q = 0; q < maxpq; q++) {
                        for (auto u = 0; u < 8; u++) {
                            for (auto v = 0; v < 8; v++) {
                                Point x1 = {result[i * isoNum + k][(p * maxpq + q) * 64 * 3 + (u * 8 + v) * 3],
                                            result[i * isoNum + k][(p * maxpq + q) * 64 * 3 + (u * 8 + v) * 3 + 1],
                                            result[i * isoNum + k][(p * maxpq + q) * 64 * 3 + (u * 8 + v) * 3 + 2]};

                                ids[k][(p * maxpq + q) * 64 + (u * 8 + v)] = Points->AddPoint(x1);
                                float value[3] = {
                                        scalar[i * isoNum + k][(p * maxpq + q) * 64 * 3 + (u * 8 + v) * 3],
                                        scalar[i * isoNum + k][(p * maxpq + q) * 64 * 3 + (u * 8 + v) * 3 + 1],
                                        scalar[i * isoNum + k][(p * maxpq + q) * 64 * 3 + (u * 8 + v) * 3 + 2]};
                                scalarArray->AddElement(value);
                            }
                        }
                    }
                }
            }

            igIndex cell[8]{};
            for (auto k = 0; k < isoNum - 1; k++) {
                for (auto p = 0; p < maxpq; p++) {
                    for (auto q = 0; q < maxpq; q++) {
                        for (auto u = 0; u < 7; u++) {
                            for (auto v = 0; v < 7; v++) {
                                cell[0] = ids[k][(p * maxpq + q) * 64 + (u * 8 + v)];
                                cell[1] = ids[k][(p * maxpq + q) * 64 + (u * 8 + v + 1)];
                                cell[2] = ids[k][((p * maxpq + q) * 64 + ((u + 1) * 8 + v + 1))];
                                cell[3] = ids[k][(p * maxpq + q) * 64 + ((u + 1) * 8 + v)];

                                cell[4] = ids[k + 1][(p * maxpq + q) * 64 + (u * 8 + v)];
                                cell[5] = ids[k + 1][(p * maxpq + q) * 64 + (u * 8 + v + 1)];
                                cell[6] = ids[k + 1][((p * maxpq + q) * 64 + ((u + 1) * 8 + v + 1))];
                                cell[7] = ids[k + 1][(p * maxpq + q) * 64 + ((u + 1) * 8 + v)];

                                Volume->AddCellIds(cell, 8);
                            }
                        }
                    }
                }
            }
        }
    }

    auto mesh = DynamicCast<SurfaceMesh>(output);
    ControlDrawData controlDrawData;
    if (!AppendControlNet(mesh, controlNet, m_SurfaceRenderForVolume, controlDrawData)) { return false; }
    mesh->SetLineColor(igm::vec3{0.0f, 0.0f, 0.0f});
    mesh->SetLineWidth(1.0f);

    if (m_SurfaceRenderForVolume) {
        auto surfaceMesh = DynamicCast<ControlNetSurfaceMesh>(output);
        surfaceMesh->SetControlDrawData(controlDrawData);
    } else {
        auto volumeMesh = DynamicCast<ControlNetVolumeMesh>(output);
        volumeMesh->SetControlNet(controlNet, controlDrawData);
    }

    m_Output = output;
    SetOutput(0, m_Output);
    UpdateProgress(1.0f);
    return true;
}

IGAME_NAMESPACE_END
#endif
