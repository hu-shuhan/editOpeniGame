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
#include <chrono>
#include <fstream>
IGAME_NAMESPACE_BEGIN
SplineReaderGPU::SplineReaderGPU() {
    SetNumberOfInputs(0);
    SetNumberOfOutputs(1);
}
int maxpq = 3;
int isoNum = maxpq * 8 - 2;

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
        gpmesh::CadSceneGP m_scene_gp;
        SurfaceMesh::Pointer surfaceMesh = SurfaceMesh::New();
        output = surfaceMesh;

        CSFile* csf;
        bool isSurface = true;
        gpbezier::SurfaceConvertHelper SurfaceHelper;

        SurfaceHelper.readfile(m_FilePath.c_str(), isSurface, 0);


        ////////////////////////////////////////////////////////////////////////////////////////////
        //获取控制点
        vector<CBSplineSurface> allSurfaces = SurfaceHelper.getSurfaces();
        int num_suf = allSurfaces.size();
        vector<vector<vector<double>>> allSurfacesControlPointsX; // 所有曲面的X坐标
        vector<vector<vector<double>>> allSurfacesControlPointsY; // 所有曲面的Y坐标
        vector<vector<vector<double>>> allSurfacesControlPointsZ; // 所有曲面的Z坐标
        // 遍历每个曲面
        for (int i = 0; i < num_suf; i++) {
            // 获取当前曲面的控制点网格 (二维数组)
            vector<vector<CPoint>> controlPointsGrid = allSurfaces[i].getControlPoints();

            // 为当前曲面创建存储坐标的容器
            vector<vector<double>> surfacePointsX; // 当前曲面所有点的X坐标
            vector<vector<double>> surfacePointsY; // 当前曲面所有点的Y坐标
            vector<vector<double>> surfacePointsZ; // 当前曲面所有点的Z坐标

            // 遍历控制点网格的每一行
            for (const auto& row: controlPointsGrid) {
                // 为当前行创建坐标向量
                vector<double> rowX;
                vector<double> rowY;
                vector<double> rowZ;

                // 遍历当前行的每个控制点，提取坐标
                for (const auto& point: row) {
                    // 假设CPoint有getX(), getY(), getZ()方法
                    rowX.push_back(point.getX());
                    rowY.push_back(point.getY());
                    rowZ.push_back(point.getZ());
                }

                // 将当前行的坐标添加到曲面容器中
                surfacePointsX.push_back(rowX);
                surfacePointsY.push_back(rowY);
                surfacePointsZ.push_back(rowZ);
            }

            // 将当前曲面的坐标容器添加到总容器中
            allSurfacesControlPointsX.push_back(surfacePointsX);
            allSurfacesControlPointsY.push_back(surfacePointsY);
            allSurfacesControlPointsZ.push_back(surfacePointsZ);
        }
        //////////////////////////////////////////////////////////////////////////


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
        std::vector<double> controlPoints;
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
                    for (int jj = 0; jj < 8; jj++) {
                        for (int kk = 0; kk < 7; kk++) { surfaceMesh->AddEdge(face[jj][kk], face[jj][kk + 1]); }
                    }
                    for (int jj = 0; jj < 7; jj++) {
                        for (int kk = 0; kk < 8; kk++) { surfaceMesh->AddEdge(face[jj][kk], face[jj + 1][kk]); }
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
        VolumeMesh::Pointer volumeMesh = VolumeMesh::New();
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

    m_Output = output;
    SetOutput(0, m_Output);
    UpdateProgress(1.0f);
    return true;
}

IGAME_NAMESPACE_END
#endif
