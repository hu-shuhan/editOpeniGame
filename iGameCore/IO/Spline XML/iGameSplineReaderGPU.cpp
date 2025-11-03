#if defined(GPSCUDA_ENABLE)
#include <MeshKernel/Mesh.h>

#include "iGameSplineReaderGPU.h"

#include "GPSpline/iGameGPSplinePatchSurface.h"
#include <GPHelperIO/iGameGP_Surface_Convert.h>
#include <GPSpline/iGameGPCadscene.h>

#include <cmath>
#include <iGameFileReader.h>
#include <regex>
#include <tinyxml2.h>

#include "iGameSurfaceMesh.h"
#include "iGameVolumeMesh.h"

IGAME_NAMESPACE_BEGIN
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
    // 调用GPU离散成表面网格
    DataObject::Pointer output = nullptr;

    if (m_SurfaceRenderForVolume) {
        SurfaceMesh::Pointer surfaceMesh = SurfaceMesh::New();
        output = surfaceMesh;

        CSFile* csf;
        bool isSurface = true;
        gpmesh::CadSceneGP m_scene_gp;
        gpbezier::SurfaceConvertHelper SurfaceHelper;
        SurfaceHelper.readfile(m_FilePath.c_str(), isSurface, 0);
        m_scene_gp.init_CUDA_map_mode(true);
        UpdateProgress(0.2f);
        std::vector<gpmesh::GPSplinePatchSurface>& main_patchsurfaces = m_scene_gp.init_scene(SurfaceHelper);
        std::vector<gpmesh::real_t*> result = m_scene_gp.m_cuda_ptr_arr;
        std::vector<gpmesh::real_t*> normal = m_scene_gp.m_cuda_normal_arr;
        std::vector<gpmesh::real_t*> scalar = m_scene_gp.m_cuda_scalar_arr;
        std::vector<double> controlPoints;
        UpdateProgress(0.6f);
        FloatArray::Pointer scalarArray = FloatArray::New();
        scalarArray->SetDimension(3);
        scalarArray->SetName("scalar3");
        surfaceMesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, scalarArray);
        CellArray::Pointer faces = CellArray::New();
        surfaceMesh->SetFaces(faces);

        for (auto i = 0; i < m_scene_gp.get_patchsurface_num(); i++) {
            for (auto p = 0; p < 5; p++) {
                for (auto q = 0; q < 5; q++) {
                    igIndex face[8][8]{};
                    for (auto j = 0; j < 64; j++) {
                        Point x = {result[i][(p * 5 + q) * 64 * 3 + j * 3], result[i][(p * 5 + q) * 64 * 3 + j * 3 + 1],
                                   result[i][(p * 5 + q) * 64 * 3 + j * 3 + 2]};
                        face[j / 8][j % 8] = surfaceMesh->AddPoint(x);
                        float value[3] = {scalar[i][(p * 5 + q) * 64 * 3 + j * 3],
                                          scalar[i][(p * 5 + q) * 64 * 3 + j * 3 + 1],
                                          scalar[i][(p * 5 + q) * 64 * 3 + j * 3 + 2]};
                        scalarArray->AddElement(value);
                    }

                    for (auto j = 0; j < 7; j++) {
                        for (auto k = 0; k < 7; k++) {
                            if (j == 0) {
                                if (k == 0) {
                                    surfaceMesh->AddEdge(face[j][k], face[j][k + 1]);
                                    surfaceMesh->AddEdge(face[j][k + 1], face[j + 1][k]);
                                    surfaceMesh->AddEdge(face[j][k], face[j + 1][k]);
                                    surfaceMesh->AddEdge(face[j][k + 1], face[j + 1][k + 1]);
                                    surfaceMesh->AddEdge(face[j + 1][k + 1], face[j + 1][k]);
                                } else {
                                    surfaceMesh->AddEdge(face[j][k], face[j][k + 1]);
                                    surfaceMesh->AddEdge(face[j][k + 1], face[j + 1][k]);
                                    surfaceMesh->AddEdge(face[j][k + 1], face[j + 1][k + 1]);
                                    surfaceMesh->AddEdge(face[j + 1][k + 1], face[j + 1][k]);
                                }
                            } else {
                                if (k == 0) {
                                    surfaceMesh->AddEdge(face[j][k + 1], face[j + 1][k]);
                                    surfaceMesh->AddEdge(face[j][k], face[j + 1][k]);
                                    surfaceMesh->AddEdge(face[j][k + 1], face[j + 1][k + 1]);
                                    surfaceMesh->AddEdge(face[j + 1][k + 1], face[j + 1][k]);
                                } else {
                                    surfaceMesh->AddEdge(face[j][k + 1], face[j + 1][k]);
                                    surfaceMesh->AddEdge(face[j][k + 1], face[j + 1][k + 1]);
                                    surfaceMesh->AddEdge(face[j + 1][k + 1], face[j + 1][k]);
                                }
                            }

                            faces->AddCellId3(face[j][k], face[j][k + 1], face[j + 1][k]);
                            faces->AddCellId3(face[j + 1][k], face[j][k + 1], face[j + 1][k + 1]);
                        }
                    }
                }
            }
        }
    } else {
        VolumeMesh::Pointer volumeMesh = VolumeMesh::New();
        output = volumeMesh;

        CSFile* csf;
        bool isSurface = false;
        int isoNum = 5;
        gpmesh::CadSceneGP m_scene_gp;
        gpbezier::SurfaceConvertHelper SurfaceHelper;
        SurfaceHelper.readfile(m_FilePath.c_str(), isSurface, isoNum);

        m_scene_gp.init_CUDA_map_mode(true);
        UpdateProgress(0.2f);
        std::vector<gpmesh::GPSplinePatchSurface>& main_patchsurfaces = m_scene_gp.init_scene(SurfaceHelper);
        UpdateProgress(0.7f);

        std::vector<gpmesh::real_t*> result = m_scene_gp.m_cuda_ptr_arr;
        std::vector<gpmesh::real_t*> normal = m_scene_gp.m_cuda_normal_arr;
        std::vector<gpmesh::real_t*> scalar = m_scene_gp.m_cuda_scalar_arr;
        std::vector<gpmesh::GPSplinePatchSurface> surfacepatch = m_scene_gp.host_patchsurfaces;
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
                ids[k].resize(5 * 5 * 64);
                for (auto p = 0; p < 5; p++) {
                    for (auto q = 0; q < 5; q++) {
                        for (auto u = 0; u < 8; u++) {
                            for (auto v = 0; v < 8; v++) {
                                Point x1 = {result[i * isoNum + k][(p * 5 + q) * 64 * 3 + (u * 8 + v) * 3],
                                            result[i * isoNum + k][(p * 5 + q) * 64 * 3 + (u * 8 + v) * 3 + 1],
                                            result[i * isoNum + k][(p * 5 + q) * 64 * 3 + (u * 8 + v) * 3 + 2]};

                                ids[k][(p * 5 + q) * 64 + (u * 8 + v)] = Points->AddPoint(x1);
                                float value[3] = {scalar[i * isoNum + k][(p * 5 + q) * 64 * 3 + (u * 8 + v) * 3],
                                                  scalar[i * isoNum + k][(p * 5 + q) * 64 * 3 + (u * 8 + v) * 3 + 1],
                                                  scalar[i * isoNum + k][(p * 5 + q) * 64 * 3 + (u * 8 + v) * 3 + 2]};
                                scalarArray->AddElement(value);
                            }
                        }
                    }
                }
            }

            igIndex cell[8]{};
            for (auto k = 0; k < isoNum - 1; k++) {
                for (auto p = 0; p < 5; p++) {
                    for (auto q = 0; q < 5; q++) {
                        for (auto u = 0; u < 7; u++) {
                            for (auto v = 0; v < 7; v++) {
                                cell[0] = ids[k][(p * 5 + q) * 64 + (u * 8 + v)];
                                cell[1] = ids[k][(p * 5 + q) * 64 + (u * 8 + v + 1)];
                                cell[2] = ids[k][((p * 5 + q) * 64 + ((u + 1) * 8 + v + 1))];
                                cell[3] = ids[k][(p * 5 + q) * 64 + ((u + 1) * 8 + v)];

                                cell[4] = ids[k + 1][(p * 5 + q) * 64 + (u * 8 + v)];
                                cell[5] = ids[k + 1][(p * 5 + q) * 64 + (u * 8 + v + 1)];
                                cell[6] = ids[k + 1][((p * 5 + q) * 64 + ((u + 1) * 8 + v + 1))];
                                cell[7] = ids[k + 1][(p * 5 + q) * 64 + ((u + 1) * 8 + v)];

                                Volume->AddCellIds(cell, 8);
                            }
                        }
                    }
                }
            }
        }
    }

    m_Output = output;
    SetOutput(0, m_Output);
    UpdateProgress(1.0f);

    return true;
}

IGAME_NAMESPACE_END
#endif
