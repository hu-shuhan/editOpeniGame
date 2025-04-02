//
// Created by m_ky on 2024/10/17.
//

/**
 * @class   iGameSplineVolumeReader
 * @brief   iGameSplineVolumeReader's brief
 */

#if defined(GPSCUDA_ENABLE)

#include "iGameSplineVolumeReader.h"
#include "iGameVolumeMesh.h"

#include <GPHelperIO/GP_Surface_Convert.h>
#include <GPSpline/GPCadscene.h>
#include "GPSpline/GPSplinePatchSurface.h"

IGAME_NAMESPACE_BEGIN

bool SplineVolumeReader::Execute() {
    return this->Parsing();
}


bool SplineVolumeReader::Parsing() {
    SetNumberOfOutputs(1);
    VolumeMesh::Pointer output = VolumeMesh::New();
    m_Output = output;
    SetOutput(0, m_Output);

    CSFile* csf;
    bool isSurface = false;
    int isoNum = 5;
    gpmesh::CadSceneGP m_scene_gp;
    //sculpt.xml,/pinion，lever_arm，mechanical02，beam1_sub1_2500
    gpbezier::SurfaceConvertHelper SurfaceHelper;
    SurfaceHelper.readfile(m_FilePath.c_str(), isSurface, isoNum);

    m_scene_gp.init_CUDA_map_mode(true);
    UpdateProgress(0.2f);
    std::vector<gpmesh::GPSplinePatchSurface>& main_patchsurfaces =
            m_scene_gp.init_scene(SurfaceHelper);
    std::vector<gpmesh::real_t*> result = m_scene_gp.m_cuda_ptr_arr;
    std::vector<gpmesh::real_t*> normal = m_scene_gp.m_cuda_normal_arr;
    std::vector<gpmesh::real_t*> scalar = m_scene_gp.m_cuda_scalar_arr;
    UpdateProgress(0.7f);
    FloatArray::Pointer scalarArray = FloatArray::New();
    scalarArray->SetDimension(3);
    scalarArray->SetName("scalar3");

    int64_t surface_num = SurfaceHelper.getSurfaces().size();
    Points::Pointer Points = Points::New();
    CellArray::Pointer Volume = CellArray::New();

    output->SetPoints(Points);
    output->SetVolumes(Volume);
    output->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, scalarArray);

    for (auto i = 0; i < surface_num / isoNum; i++) {
        std::vector<std::vector<igIndex>> ids(isoNum);
        for (auto k = 0; k < isoNum; k++)
        {
            ids[k].resize(5 * 5 * 64);
            for (auto p = 0; p < 5; p++) {
                for (auto q = 0; q < 5; q++) {
                    for (auto u = 0; u < 8; u++) {
                        for (auto v = 0; v < 8; v++) {
                            //[i*50+k][k*1600]
                            Point x1 = { result[i * isoNum + k][(p * 5 + q) * 64 * 3 + (u * 8 + v) * 3],
                                         result[i * isoNum + k][(p * 5 + q) * 64 * 3 + (u * 8 + v) * 3 + 1],
                                         result[i * isoNum + k][(p * 5 + q) * 64 * 3 + (u * 8 + v) * 3 + 2] };

                            ids[k][(p * 5 + q) * 64 + (u * 8 + v)] = Points->AddPoint(x1);
                            float value[3] = { scalar[i * isoNum + k][(p * 5 + q) * 64 * 3 + (u * 8 + v) * 3],
                                               scalar[i * isoNum + k][(p * 5 + q) * 64 * 3 + (u * 8 + v) * 3 + 1],
                                               scalar[i * isoNum + k][(p * 5 + q) * 64 * 3 + (u * 8 + v) * 3 + 2] };
                            scalarArray->AddElement(value);
                        }
                    }
                }
            }

        }

        igIndex cell[8]{};
        for (auto k = 0; k < isoNum-1; k++)
        {
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

    UpdateProgress(1.0f);
    return true;
}

IGAME_NAMESPACE_END

#endif