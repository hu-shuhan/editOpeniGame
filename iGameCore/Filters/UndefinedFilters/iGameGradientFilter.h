//#ifndef GradientFilter_h
//#define GradientFilter_h
//
//#include "Eigen/Dense"
//#include "Eigen/Eigenvalues"
//#include "iGameFilter.h"
//#include "iGamePointSet.h"
//#include "iGameSurfaceMesh.h"
//#include "iGameVolumeMesh.h"
//#include "iGameUnstructuredMesh.h"
//#include <cmath>
//
//
//IGAME_NAMESPACE_BEGIN
//
////现在默认取第一个数组
//class GradientFilter : public Filter {
//public:
//    I_OBJECT(GradientFilter);
//    static Pointer New() { return new GradientFilter; }
//    bool Execute() override {
//
//        auto input = GetInput(0);
//        if (input == nullptr) return false;
//
//        auto CheckType = [&]() -> bool {
//            attributeSet = input->GetAttributeSet();
//            if (!attributeSet) return false;
//            curIndex = input->GetAttributeIndex();
//            curDim = input->GetAttributeDimension();
//            if (curIndex < 0) return false;
//
//            int dim = input->GetAttributeSet()
//                              ->GetAttribute(curIndex)
//                              .pointer->GetDimension();
//            if (dim != 1) { return false; }
//            return true;
//        };
//
//        switch (input->GetDataObjectType()) {
//            case IG_SURFACE_MESH: {
//                surface_Mesh = DynamicCast<SurfaceMesh>(input);
//                if (!CheckType()) return false;
//            } break;
//            case IG_VOLUME_MESH: {
//                volume_Mesh = DynamicCast<VolumeMesh>(input);
//                if (volume_Mesh) {
//                    surface_Mesh = DynamicCast<SurfaceMesh>(
//                            volume_Mesh->GetDisplayObject());
//                    if (!surface_Mesh) return false;
//
//                    if (!CheckType()) return false;
//
//                    FloatArray::Pointer gradients = FloatArray::New();
//                    gradients->SetDimension(3);
//                    gradients->SetName("gradients");
//                    input->GetAttributeSet()->AddScalar(IG_POINT, gradients);
//                }
//            } break;
//            case IG_UNSTRUCTURED_MESH: {
//                auto mesh = DynamicCast<UnstructuredMesh>(input);
//                surface_Mesh = mesh->TransferToSurfaceMesh();
//                volume_Mesh = mesh->TransferToVolumeMesh();
//
//                if (surface_Mesh) {
//                    if (!CheckType()) return false;
//                }
//
//                if (volume_Mesh) {
//                    surface_Mesh =
//                            DynamicCast<SurfaceMesh>(mesh->GetDisplayObject());
//                    if (!surface_Mesh) return false;
//
//                    if (!CheckType()) return false;
//
//                    FloatArray::Pointer gradients = FloatArray::New();
//                    gradients->SetDimension(3);
//                    gradients->SetName("gradients");
//                    input->GetAttributeSet()->AddScalar(IG_POINT, gradients);
//                }
//            } break;
//            default:
//                return false;
//        }
//
//        // 体网格
//        if (volume_Mesh) {
//            //std::cout << "[Debug  ] "
//            //          << "gradient in volume_mesh " << '\n';
//            //attributeSet = volume_Mesh->GetAttributeSet();
//            //if (attributeSet == nullptr) return false;
//
//            //auto attachmentType =
//            //        attributeSet->GetAttribute(curIndex).attachmentType;
//
//            //int VolumeNum = volume_Mesh->GetNumberOfVolumes();
//            //int PointNum = volume_Mesh->GetNumberOfPoints();
//            //Points::Pointer Points = volume_Mesh->GetPoints();
//            //volume_Mesh->RequestEditStatus();
//            //// 附着在point
//            //if (PointNum != 0 && attachmentType == 0)
//            //    return GetPointGradient(1, Points, PointNum);
//            //// 附着在cell
//            //else if (VolumeNum != 0 && attachmentType == 1)
//            //    return GetOtherGradient(1, VolumeNum);
//
//            // 表面网格
//        } 
//        
//        if (surface_Mesh) {
//            std::cout << "[Debug  ] "
//                      << "gradient in surface_mesh " << '\n';
//
//            attributeSet = surface_Mesh->GetAttributeSet();
//            if (attributeSet == nullptr) return false;
//
//            auto attachmentType =
//                    attributeSet->GetAttribute(curIndex).attachmentType;
//
//            int FaceNum = surface_Mesh->GetNumberOfFaces();
//            int PointNum = surface_Mesh->GetNumberOfPoints();
//            Points::Pointer Points = surface_Mesh->GetPoints();
//            surface_Mesh->RequestEditStatus();
//            // 附着在point
//            if (PointNum != 0 && attachmentType == 0)
//                return GetPointGradient(0, Points, PointNum);
//            // 附着在cell
//            else if (FaceNum != 0 && attachmentType == 1)
//                return GetOtherGradient(0, FaceNum);
//        }
//        return false;
//    }
//
//    // 表面/体网格：点
//    bool GetPointGradient(int type, Points::Pointer Points, int PointNum) {
//
//        AttributeSet* attributeSet;
//        if (type == 0) attributeSet = surface_Mesh->GetAttributeSet();
//        else if (type == 1)
//            attributeSet = volume_Mesh->GetAttributeSet();
//
//        auto data = attributeSet->GetAttribute(curIndex).pointer;
//        int dimension = data->GetDimension();
//        FloatArray::Pointer gradients = FloatArray::New();
//        gradients->SetDimension(3);
//        gradients->Reserve(PointNum);
//        gradients->SetName("gradients");
//        attributeSet->AddScalar(IG_POINT, gradients);
//
//        std::vector<std::array<float, 3>> gradient(PointNum,
//                                                   {0.0f, 0.0f, 0.0f});
//        std::vector<float> sumWeights(PointNum, 0.0f);
//
//        igIndex neighborVerts[256]{};
//        // 计算点的梯度
//        for (igIndex idx = 0; idx < PointNum; ++idx) {
//            int NeighborNum;
//            // 获取邻接顶点
//            if (type == 1)
//                NeighborNum = volume_Mesh->GetPointToOneRingPoints(
//                        idx, neighborVerts);
//            else if (type == 0)
//                NeighborNum = surface_Mesh->GetPointToOneRingPoints(
//                        idx, neighborVerts);
//
//            auto v1 = Points->GetPoint(idx);
//
//            for (int m = 0; m < NeighborNum; m++) {
//                Vector<float, 3> v2;
//                if (type == 1) v2 = volume_Mesh->GetPoint(neighborVerts[m]);
//                else if (type == 0)
//                    v2 = surface_Mesh->GetPoint(neighborVerts[m]);
//
//                float x = v1[0] - v2[0];
//                float y = v1[1] - v2[1];
//                float z = v1[2] - v2[2];
//
//                // 标量计算时就算是三维数据也默认取第一维
//                double value = data->GetValue(dimension * idx) -
//                               data->GetValue(dimension * neighborVerts[m]);
//
//                float weight = 1.0f / std::sqrt(x * x + y * y + z * z);
//                sumWeights[idx] += weight;
//                gradient[idx][0] += x * weight * value;
//                gradient[idx][1] += y * weight * value;
//                gradient[idx][2] += z * weight * value;
//            }
//            if (sumWeights[idx] > 0) {
//                gradient[idx][0] /= sumWeights[idx];
//                gradient[idx][1] /= sumWeights[idx];
//                gradient[idx][2] /= sumWeights[idx];
//                gradients->AddElement3(gradient[idx][0], gradient[idx][1],
//                                       gradient[idx][2]);
//            }
//        }
//
//        return true;
//    }
//
//    std::array<float, 3> GetPosition_volume(Volume* v, int num) {
//        std::array<float, 3> position = {0.0f, 0.0f, 0.0f};
//        for (igIndex idx = 0; idx < num; idx++) {
//            position[0] += v->GetPoint(idx)[0];
//            position[1] += v->GetPoint(idx)[1];
//            position[2] += v->GetPoint(idx)[2];
//        }
//
//        for (igIndex i = 0; i < 3; i++) {
//            if (position[i] != 0) position[i] /= num;
//        }
//        return position;
//    }
//
//    std::array<float, 3> GetPosition_face(Face* f, int num) {
//        std::array<float, 3> position = {0.0f, 0.0f, 0.0f};
//        for (igIndex idx = 0; idx < num; idx++) {
//            position[0] += f->GetPoint(idx)[0];
//            position[1] += f->GetPoint(idx)[1];
//            position[2] += f->GetPoint(idx)[2];
//        }
//
//        for (igIndex i = 0; i < 3; i++) {
//            if (position[i] != 0) position[i] /= num;
//        }
//        return position;
//    }
//
//    // 表面网格：面 / 体网格：体
//    bool GetOtherGradient(int type, int Num) {
//        AttributeSet* attributeSet;
//        if (type == 0) attributeSet = surface_Mesh->GetAttributeSet();
//        else if (type == 1)
//            attributeSet = volume_Mesh->GetAttributeSet();
//
//        auto data = attributeSet->GetAttribute(curIndex).pointer;
//        int dimension = data->GetDimension();
//        FloatArray::Pointer gradients = FloatArray::New();
//        gradients->SetDimension(3);
//        gradients->Reserve(Num);
//        gradients->SetName("gradients");
//
//        attributeSet->AddScalar(IG_POINT, gradients);
//
//        std::vector<std::array<float, 3>> gradient(Num, {0.0f, 0.0f, 0.0f});
//        std::vector<float> sumWeights(Num, 0.0f);
//
//        igIndex neighborVerts[256]{};
//        // 计算点的梯度
//        for (igIndex idx = 0; idx < Num; ++idx) {
//
//            int NeighborNum;
//            // 获取邻接顶点
//            if (type == 1)
//                // neighbors:volumeIds
//                NeighborNum = volume_Mesh->GetVolumeToNeighborVolumesWithFace(
//                        idx, neighborVerts);
//
//            else if (type == 0)
//                // neighbors:faceIds
//                NeighborNum = surface_Mesh->GetFaceToNeighborFaces(
//                        idx, neighborVerts);
//
//            for (int m = 0; m < NeighborNum; m++) {
//                float x, y, z;
//                if (type == 1) {
//                    igIndex* volumeIds;
//                    auto v1 = volume_Mesh->GetVolume(idx);
//                    auto v2 = volume_Mesh->GetVolume(neighborVerts[m]);
//                    auto size_v1 = v1->GetNumberOfPoints();
//                    auto size_v2 = v2->GetNumberOfPoints();
//
//                    std::array<float, 3> v1_position =
//                            GetPosition_volume(v1, size_v1);
//                    std::array<float, 3> v2_position =
//                            GetPosition_volume(v2, size_v2);
//
//                    x = v1_position[0] - v2_position[0];
//                    y = v1_position[1] - v2_position[1];
//                    z = v1_position[2] - v2_position[2];
//
//                } else if (type == 0) {
//                    auto v1 = surface_Mesh->GetFace(idx);
//                    auto v2 = surface_Mesh->GetFace(neighborVerts[m]);
//                    auto size_v1 = v1->GetNumberOfPoints();
//                    auto size_v2 = v2->GetNumberOfPoints();
//
//                    std::array<float, 3> v1_position =
//                            GetPosition_face(v1, size_v1);
//                    std::array<float, 3> v2_position =
//                            GetPosition_face(v2, size_v2);
//
//                    x = v1_position[0] - v2_position[0];
//                    y = v1_position[1] - v2_position[1];
//                    z = v1_position[2] - v2_position[2];
//                }
//                // 标量计算时就算是三维数据也默认取第一维
//                auto value = data->GetValue(idx * dimension) -
//                             data->GetValue(neighborVerts[m] * dimension);
//                float weight = 1.0f / std::sqrt(x * x + y * y + z * z);
//
//                sumWeights[idx] += weight;
//                gradient[idx][0] += x * weight * value;
//                gradient[idx][1] += y * weight * value;
//                gradient[idx][2] += z * weight * value;
//            }
//            if (sumWeights[idx] > 0) {
//                gradient[idx][0] /= sumWeights[idx];
//                gradient[idx][1] /= sumWeights[idx];
//                gradient[idx][2] /= sumWeights[idx];
//                gradients->AddElement3(gradient[idx][0], gradient[idx][1],
//                                       gradient[idx][2]);
//            }
//        }
//        return true;
//    }
//
//protected:
//    GradientFilter()
//    //输入输出个数
//    {
//        SetNumberOfInputs(1);
//        SetNumberOfOutputs(1);
//    }
//    ~GradientFilter() override = default;
//
//    SurfaceMesh::Pointer surface_Mesh{};
//    VolumeMesh::Pointer volume_Mesh{};
//    AttributeSet* attributeSet{nullptr};
//    int curIndex{-1};
//    int curDim{-1};
//};
//
//IGAME_NAMESPACE_END
//#endif


#ifndef GradientFilter_h
#define GradientFilter_h

#include "Eigen/Dense"
#include "Eigen/Eigenvalues"
#include "iGameFilter.h"
#include "iGamePointSet.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"
#include <cmath>


IGAME_NAMESPACE_BEGIN
class GradientFilter : public Filter {
public:
    I_OBJECT(GradientFilter);
    static Pointer New() { return new GradientFilter; }


    struct Gradient {
        float gx, gy, gz;
    };

    struct VectorGrad {
        Gradient x, y, z;
    };

    bool Execute() override;

    float ComputeCellVolume(Cell* cell);

    // 三角形和四边形和多边形
    std::array<float, 3> ComputePointGradient(int type, Cell* cell, ArrayObject::Pointer data, int dim);
    std::array<float, 3> ComputePointGradient(Cell* cell, ArrayObject::Pointer data, int dim);

    std::array<float, 3> ComputeCellGradient(int type, Cell* cell, ArrayObject::Pointer data, int dim);

    bool ComputeVorticityWithSurfaceMesh(SurfaceMesh::Pointer Mesh, AttributeSet::Pointer Attributes, int Index);
    bool ComputeGradientWithSurfaceMesh(SurfaceMesh::Pointer Mesh, AttributeSet::Pointer Attributes, int Index);
    bool ComputeGradientWithVolumeMesh(VolumeMesh::Pointer Mesh, AttributeSet::Pointer Attributes, int Index);
    bool ComputeGradientWithVolumeMesh2(VolumeMesh::Pointer Mesh, AttributeSet::Pointer Attributes, int Index);

    VectorGrad ComputeVectorGradByPlane(Cell* cell, ArrayObject* data);
    Vector3f ComputeCenter(Cell* cell);

    // =========================== 表面网格 Point ===========================
    bool ComputePointVorticityForMesh(SurfaceMesh::Pointer surface_Mesh, AttributeSet* attributeSet, int curIndex);

    // =========================== 表面网格 Cell ===========================
    bool ComputeCellVorticityForMesh(SurfaceMesh::Pointer surface_Mesh, AttributeSet* attributeSet, int curIndex);

    // =========================== 体网格 Point ===========================
    bool ComputePointVorticityForVol(VolumeMesh::Pointer volume_Mesh, AttributeSet* attributeSet, int curIndex);

    // =========================== 体网格 Cell  ===========================
    bool ComputeCellVorticityForVol(VolumeMesh::Pointer volume_Mesh, AttributeSet* attributeSet, int curIndex);

    std::array<float, 3> ComputePointGradientWithSurfaceMesh(Cell* cell, ArrayObject::Pointer data, int dim);
    std::array<float, 3> ComputePointGradientWithVolumeMesh(Cell* cell, ArrayObject::Pointer data, int dim);

    // 三角形
    std::array<float, 3> ComputeTriPointGradient(Cell* cell, ArrayObject::Pointer data, int dim);

    // 四边形
    std::array<float, 3> ComputeQuadPointGradient(Cell* cell, ArrayObject::Pointer data, int dim);

    // 多边形
    std::array<float, 3> ComputePolygonPointGradient(Cell* cell, ArrayObject::Pointer data, int dim);

    // 四面体线性插值 point
    std::array<float, 3> ComputeTetPointGradient(Cell* cell, ArrayObject::Pointer data, int dim);

    // 四面体线性插值 cell
    std::array<float, 3> ComputeTetCellGradient(int type, Cell* cell, ArrayObject::Pointer data, int dim);

    // 六面体中心差分
    std::array<float, 3> ComputeHexPointGradient(Cell* cell, ArrayObject::Pointer data, int dim);

    // 六面体中心差分 cell
    std::array<float, 3> ComputeHexCellGradient(int type, Cell* cell, ArrayObject::Pointer data, int dim);

    // 多面体中心差分
    std::array<float, 3> ComputePolyPointGradient(Cell* cell, ArrayObject::Pointer data, int dim);

    // 多面体中心差分 cell
    std::array<float, 3> ComputePolyCellGradient(int type, Cell* cell, ArrayObject::Pointer data, int dim);

    ArrayObject::Pointer AttributeCell2Point(CellArray::Pointer Cell, ArrayObject::Pointer OriArray, size_t PointNum);

    float ComputeSurfaceArea(Cell* cell);
    float ComputeTriangleArea(Cell* cell);

    float ComputeVolumeAverageEdgeLength(Cell* cell);
    float ComputeAverageEdgeLength(Cell* cell);

    float ComputeTetVolume(Cell* cell);

    bool InverseMatrix4x4(const float in[4][4], float out[4][4]);

protected:
    GradientFilter();
    ~GradientFilter() override = default;

    VolumeMesh::Pointer volume_Mesh{};
    SurfaceMesh::Pointer surface_Mesh{};
    AttributeSet::Pointer attributeSet{};

    int curIndex{-1};
    int curDim{-1};
};

IGAME_NAMESPACE_END
#endif