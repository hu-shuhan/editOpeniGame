//
// Created by m_ky on 2024/10/17.
//

/**
 * @class   iGameSplineSurfaceReader
 * @brief   iGameSplineSurfaceReader's brief
 */
#if defined(GPSCUDA_ENABLE)
#include "iGameSplineSurfaceReader.h"

#include "GPSpline/GPSplinePatchSurface.h"
#include <GPHelperIO/GP_Surface_Convert.h>
#include <GPSpline/GPCadscene.h>

#include "iGameSurfaceMesh.h"
//#include "iGameStructuredMesh.h"
//#include "iGameUnstructuredMesh.h"

IGAME_NAMESPACE_BEGIN

bool SplineSurfaceReader::Execute() { return this->Parsing(); }


bool SplineSurfaceReader::Parsing() {
    SurfaceMesh::Pointer output = SurfaceMesh::New();
    //    UnstructuredMesh::Pointer output = iGame::UnstructuredMesh::New();
    m_Output = output;
    SetOutput(0, m_Output);

    bool isSurface = true;
    gpmesh::CadSceneGP m_scene_gp;
    gpbezier::SurfaceConvertHelper SurfaceHelper;
    SurfaceHelper.readfile(m_FilePath.c_str(), isSurface, 0);
    m_scene_gp.init_CUDA_map_mode(true);
    UpdateProgress(0.2f);
    std::vector<gpmesh::GPSplinePatchSurface>& main_patchsurfaces =
            m_scene_gp.init_scene(SurfaceHelper);
    std::vector<gpmesh::real_t*> result = m_scene_gp.m_cuda_ptr_arr;
    std::vector<gpmesh::real_t*> normal = m_scene_gp.m_cuda_normal_arr;
    std::vector<gpmesh::real_t*> scalar = m_scene_gp.m_cuda_scalar_arr;
    UpdateProgress(0.7f);
    /* Add Scalar */
    FloatArray::Pointer scalarArray = FloatArray::New();
    scalarArray->SetDimension(3);
    scalarArray->SetName("scalar3");
    output->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, scalarArray);
    CellArray::Pointer faces = CellArray::New();
    output->SetFaces(faces);
    //    int64_t surface_num = SurfaceHelper.getSurfaces().size();
    for (auto i = 0; i < m_scene_gp.get_patchsurface_num(); i++) {
        for (auto p = 0; p < 5; p++) {
            for (auto q = 0; q < 5; q++) {
                igIndex face[8][8]{};
                for (auto j = 0; j < 64; j++) {

                    //ofs1 << i << ',' << p * 5 + q << ',' << j << ":"
                    //     << result[i][(p * 5 + q) * 64 * 3 + j * 3] << ','
                    //     << result[i][(p * 5 + q) * 64 * 3 + j * 3 + 1] << ','
                    //     << result[i][(p * 5 + q) * 64 * 3 + j * 3 + 2] << ','
                    //     << endl;
                    Point x = {result[i][(p * 5 + q) * 64 * 3 + j * 3], result[i][(p * 5 + q) * 64 * 3 + j * 3 + 1],
                               result[i][(p * 5 + q) * 64 * 3 + j * 3 + 2]};
                    face[j / 8][j % 8] = output->AddPoint(x);
                    float value[3] = {scalar[i][(p * 5 + q) * 64 * 3 + j * 3],
                                      scalar[i][(p * 5 + q) * 64 * 3 + j * 3 + 1],
                                      scalar[i][(p * 5 + q) * 64 * 3 + j * 3 + 2]};
                    scalarArray->AddElement(value);
                }


                for (auto j = 0; j < 7; j++) {
                    for (auto k = 0; k < 7; k++) {
                        if (j == 0) {
                            if (k == 0) {
                                output->AddEdge(face[j][k], face[j][k + 1]);
                                output->AddEdge(face[j][k + 1], face[j + 1][k]);
                                output->AddEdge(face[j][k], face[j + 1][k]);
                                //output->AddEdge(face[j + 1][k], face[j][k + 1]);
                                output->AddEdge(face[j][k + 1], face[j + 1][k + 1]);
                                output->AddEdge(face[j + 1][k + 1], face[j + 1][k]);
                            } else {
                                output->AddEdge(face[j][k], face[j][k + 1]);
                                output->AddEdge(face[j][k + 1], face[j + 1][k]);
                                //output->AddEdge(face[j][k], face[j + 1][k]);
                                //output->AddEdge(face[j + 1][k], face[j][k + 1]);
                                output->AddEdge(face[j][k + 1], face[j + 1][k + 1]);
                                output->AddEdge(face[j + 1][k + 1], face[j + 1][k]);
                            }
                        } else {
                            if (k == 0) {
                                //output->AddEdge(face[j][k], face[j][k + 1]);
                                output->AddEdge(face[j][k + 1], face[j + 1][k]);
                                output->AddEdge(face[j][k], face[j + 1][k]);
                                //output->AddEdge(face[j + 1][k], face[j][k + 1]);
                                output->AddEdge(face[j][k + 1], face[j + 1][k + 1]);
                                output->AddEdge(face[j + 1][k + 1], face[j + 1][k]);
                            } else {
                                //output->AddEdge(face[j][k], face[j][k + 1]);
                                output->AddEdge(face[j][k + 1], face[j + 1][k]);
                                //output->AddEdge(face[j][k], face[j + 1][k]);
                                //output->AddEdge(face[j + 1][k], face[j][k + 1]);
                                output->AddEdge(face[j][k + 1], face[j + 1][k + 1]);
                                output->AddEdge(face[j + 1][k + 1], face[j + 1][k]);
                            }
                        }

                        faces->AddCellId3(face[j][k], face[j][k + 1], face[j + 1][k]);
                        faces->AddCellId3(face[j + 1][k], face[j][k + 1], face[j + 1][k + 1]);
                    }
                }
            }
        }
    }
    UpdateProgress(1.f);
    return true;
}

SplineSurfaceReader::SplineSurfaceReader() {
    SetNumberOfInputs(0);
    SetNumberOfOutputs(1);
}
IGAME_NAMESPACE_END
#endif