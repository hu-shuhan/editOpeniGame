#ifndef CurvatureFilter_h
#define CurvatureFilter_h

#include "Eigen/Dense"
#include "Eigen/Eigenvalues"
#include "iGameFilter.h"
#include "iGamePointSet.h"
#include "iGameSurfaceMesh.h"
#include "iGameVolumeMesh.h"
#include "iGameUnstructuredMesh.h"
#include <cmath>


IGAME_NAMESPACE_BEGIN

//返回二维数据
//第一维：(k1 + k2) / 2.0;
//第二维：k1 * k2;
//现在默认取第一个数组：m_AttributeSet->GetAttribute(0)
class CurvatureFilter : public Filter {
public:
    I_OBJECT(CurvatureFilter);
    static Pointer New() { return new CurvatureFilter; }

    bool Execute() override {

        auto input = GetInput(0);
        if (input == nullptr) return false;

        auto CheckType = [&]() -> bool {
            attributeSet = surface_Mesh->GetAttributeSet();
            if (attributeSet == nullptr) return false;

            curIndex = input->GetAttributeIndex();
            curDim = input->GetAttributeDimension();
            if (curIndex < 0) return false;

            int dim = input->GetAttributeSet()
                              ->GetAttribute(curIndex)
                              .pointer->GetDimension();
            if (dim != 1) { return false; }
            return true;
        };

        SetOutput(input);

        switch (input->GetDataObjectType()) {
            case IG_SURFACE_MESH:
            {
                surface_Mesh = DynamicCast<SurfaceMesh>(input);

                if (!CheckType()) return false;
            }break;
            case IG_VOLUME_MESH: {
                return false;
                volume_Mesh = DynamicCast<VolumeMesh>(input);
                if (volume_Mesh) {
                    surface_Mesh = DynamicCast<SurfaceMesh>(
                            volume_Mesh->GetDisplayObject());
                    if (!surface_Mesh) return false;

                    if (!CheckType()) return false;

                    FloatArray::Pointer curvatures = FloatArray::New();
                    curvatures->SetDimension(2);
                    curvatures->SetName("curvatures");
                    volume_Mesh->GetAttributeSet()->AddScalar(IG_POINT,
                                                              curvatures);
                }
            } break;
            case IG_UNSTRUCTURED_MESH: {
                auto mesh = DynamicCast<UnstructuredMesh>(input);
                surface_Mesh = mesh->TransferToSurfaceMesh();
                volume_Mesh = mesh->TransferToVolumeMesh();

                if (surface_Mesh) {
                    if (!CheckType()) return false;
                }

                if (volume_Mesh) {
                    return false;
                    surface_Mesh =
                            DynamicCast<SurfaceMesh>(mesh->GetDisplayObject());
                    if (!surface_Mesh) return false;

                    if (!CheckType()) return false;

                    FloatArray::Pointer curvatures = FloatArray::New();
                    curvatures->SetDimension(2);
                    curvatures->SetName("curvatures");
                    mesh->GetAttributeSet()->AddScalar(IG_POINT, curvatures);
                }
            } break;
            default:
                return false;
        }

        if (volume_Mesh) {

            //attributeSet = volume_Mesh->GetAttributeSet();
            //if (attributeSet == nullptr) return false;

            //auto attachmentType =
            //        attributeSet->GetAttribute(curIndex).attachmentType;

            //int VolumeNum = volume_Mesh->GetNumberOfVolumes();
            //int PointNum = volume_Mesh->GetNumberOfPoints();
            //Points::Pointer Points = volume_Mesh->GetPoints();

            //volume_Mesh->RequestEditStatus();
            //if (PointNum != 0 && attachmentType == 0)
            //    return GetPointCurvature(1, Points, PointNum);

            //else if (VolumeNum != 0 && attachmentType == 1)
            //    return GetOtherCurvature(1, VolumeNum);

        }
        if (surface_Mesh) {
            attributeSet = surface_Mesh->GetAttributeSet();
            if (attributeSet == nullptr) return false;

            auto attachmentType =
                    attributeSet->GetAttribute(curIndex).attachmentType;

            int FaceNum = surface_Mesh->GetNumberOfFaces();
            int PointNum = surface_Mesh->GetNumberOfPoints();
            Points::Pointer Points = surface_Mesh->GetPoints();
            surface_Mesh->RequestEditStatus();
            // 附着在point
            if (PointNum != 0 && attachmentType == 0)
                return GetPointCurvature(0, Points, PointNum);
            // 附着在cell
            else if (FaceNum != 0 && attachmentType == 1)
                return GetOtherCurvature(0, FaceNum);
        }

        return false;
    }

    bool GetPointCurvature(int type, Points::Pointer Points, int PointNum) {

        AttributeSet* attributeSet;
        if (type == 0) attributeSet = surface_Mesh->GetAttributeSet();
        else if (type == 1)
            attributeSet = volume_Mesh->GetAttributeSet();

        auto data = attributeSet->GetAttribute(curIndex).pointer;
        //        int dimension = data->GetElementSize();
        FloatArray::Pointer curvatures = FloatArray::New();
        curvatures->SetDimension(2);
        curvatures->Reserve(PointNum);
        curvatures->SetName("curvatures");
        attributeSet->AddScalar(IG_POINT, curvatures);

        std::vector<std::array<float, 3>> gradient =
                GetPointGradient(type, Points, PointNum);
        std::vector<std::array<float, 4>> curvature(PointNum,
                                                    {0.0f, 0.0f, 0.0f, 0.0f});
        std::vector<float> sumWeights(PointNum, 0.0f);

        igIndex neighborVerts[256]{};
        int hundred = PointNum / 100;
        for (igIndex idx = 0; idx < PointNum; ++idx) {
            if(idx % hundred == 0) UpdateProgress((double)idx / PointNum);
            auto v1 = Points->GetPoint(idx);

            Eigen::Vector3d gp(0.0, 0.0, 0.0);
            Eigen::Matrix3d hessian = Eigen::Matrix3d::Zero();
            gp(0) = gradient[idx][0];
            gp(1) = gradient[idx][1];
            gp(2) = gradient[idx][2];

            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    float value = 0.0;
                    float weightSum = 0.0;

                    int NeighborNum;
                    // 获取邻接顶点
                    if (type == 1)
                        NeighborNum = volume_Mesh->GetPointToOneRingPoints(
                                idx, neighborVerts);
                    else if (type == 0)
                        NeighborNum = surface_Mesh->GetPointToOneRingPoints(
                                idx, neighborVerts);

                    for (int m = 0; m < NeighborNum; m++) {
                        Vector<float, 3> v2;
                        if (type == 1)
                            v2 = volume_Mesh->GetPoint(neighborVerts[m]);
                        else if (type == 0)
                            v2 = surface_Mesh->GetPoint(neighborVerts[m]);

                        float x = v1[0] - v2[0];
                        float y = v1[1] - v2[1];
                        float z = v1[2] - v2[2];
                        //                auto data = m_PropertySet->GetProperty(0).pointer;
                        //                auto type = m_PropertySet->GetProperty(0).attachmentType;

                        //                        double value = data->GetValue(dimension * idx) - data->GetValue(dimension * neighborVerts[m]);
                        //                double value2 = data->GetValue(dimension * idx + 1) - data->GetValue(dimension * neighborVerts[m] + 1);
                        //                double value3 = data->GetValue(dimension * idx + 2) - data->GetValue(dimension * neighborVerts[m] + 2);
                        //                double value = std::sqrt(value1 * value1 + value2 * value2 + value3 * value3);
                        //                double value = v1[0];

                        float temp = gradient[idx][i] -
                                     gradient[neighborVerts[m]]
                                             [i]; //i=0:x i=1:y i=2:z
                        float weight = 1.0 / std::sqrt(std::pow(x, 2) +
                                                       std::pow(y, 2) +
                                                       std::pow(z, 2));
                        value += temp * weight;
                        weightSum += weight;
                    }
                    if (weightSum > 0) hessian(i, j) = value / weightSum;
                    if (i != j) hessian(j, i) = hessian(i, j);
                }
            }

            Eigen::Vector3d n = -gp.normalized();
            Eigen::Matrix3d I = Eigen::Matrix3d::Identity();
            Eigen::Matrix3d B = (I - n * n.transpose()) * hessian;

            Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> es(B);
            Eigen::Vector3d eigenvalues = es.eigenvalues();
            float k2 = eigenvalues(0);
            float k1 = eigenvalues(1);

            curvature[idx][0] = k1;
            curvature[idx][1] = k2;
            curvature[idx][2] = (k1 + k2) / 2.0;
            curvature[idx][3] = k1 * k2;

            curvatures->AddElement2(curvature[idx][2], curvature[idx][3]);
        }
        UpdateProgress(1.0f);
        return true;
    }

    std::array<float, 3> GetPosition_volume(Volume* v, int num) {
        std::array<float, 3> position = {0.0f, 0.0f, 0.0f};
        for (igIndex idx = 0; idx < num; idx++) {
            position[0] += v->GetPoint(idx)[0];
            position[1] += v->GetPoint(idx)[1];
            position[2] += v->GetPoint(idx)[2];
        }

        for (igIndex i = 0; i < 3; i++) {
            if (position[i] != 0) position[i] /= num;
        }
        return position;
    }
    std::array<float, 3> GetPosition_face(Face* f, int num) {
        std::array<float, 3> position = {0.0f, 0.0f, 0.0f};
        for (igIndex idx = 0; idx < num; idx++) {
            position[0] += f->GetPoint(idx)[0];
            position[1] += f->GetPoint(idx)[1];
            position[2] += f->GetPoint(idx)[2];
        }

        for (igIndex i = 0; i < 3; i++) {
            if (position[i] != 0) position[i] /= num;
        }
        return position;
    }

    bool GetOtherCurvature(int type, int Num) {

        AttributeSet* attributeSet;
        if (type == 0) attributeSet = surface_Mesh->GetAttributeSet();
        else if (type == 1)
            attributeSet = volume_Mesh->GetAttributeSet();

        FloatArray::Pointer curvatures = FloatArray::New();
        curvatures->SetDimension(2);
        curvatures->Reserve(Num);
        curvatures->SetName("curvatures");
        attributeSet->AddScalar(IG_POINT, curvatures);

        std::vector<std::array<float, 3>> gradient =
                GetOtherGradient(type, Num);
        std::vector<std::array<float, 4>> curvature(Num,
                                                    {0.0f, 0.0f, 0.0f, 0.0f});
        std::vector<float> sumWeights(Num, 0.0f);

        igIndex neighborVerts[256]{};
        int hundred = Num / 100;
        for (igIndex idx = 0; idx < Num; ++idx) {
            if(idx % hundred == 0) UpdateProgress((double)idx / Num);
            Eigen::Vector3d gp(0.0, 0.0, 0.0);
            Eigen::Matrix3d hessian = Eigen::Matrix3d::Zero();
            gp(0) = gradient[idx][0];
            gp(1) = gradient[idx][1];
            gp(2) = gradient[idx][2];

            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    float value = 0.0;
                    float weightSum = 0.0;

                    int NeighborNum;
                    // 获取邻接顶点
                    if (type == 1)
                        // neighbors:volumeIds
                        NeighborNum =
                                volume_Mesh->GetVolumeToNeighborVolumesWithFace(
                                        idx, neighborVerts);
                    else if (type == 0)
                        // neighbors:faceIds
                        NeighborNum = surface_Mesh->GetFaceToNeighborFaces(
                                idx, neighborVerts);

                    for (int m = 0; m < NeighborNum; m++) {

                        float x, y, z;
                        if (type == 1) {
                            auto v1 = volume_Mesh->GetVolume(idx);
                            auto v2 = volume_Mesh->GetVolume(neighborVerts[m]);
                            auto size_v1 = v1->GetNumberOfPoints();
                            auto size_v2 = v2->GetNumberOfPoints();

                            std::array<float, 3> v1_position =
                                    GetPosition_volume(v1, size_v1);
                            std::array<float, 3> v2_position =
                                    GetPosition_volume(v2, size_v2);

                            x = v1_position[0] - v2_position[0];
                            y = v1_position[1] - v2_position[1];
                            z = v1_position[2] - v2_position[2];
                        }

                        else if (type == 0) {
                            auto v1 = surface_Mesh->GetFace(idx);
                            auto v2 = surface_Mesh->GetFace(neighborVerts[m]);
                            auto size_v1 = v1->GetNumberOfPoints();
                            auto size_v2 = v2->GetNumberOfPoints();

                            std::array<float, 3> v1_position =
                                    GetPosition_face(v1, size_v1);
                            std::array<float, 3> v2_position =
                                    GetPosition_face(v2, size_v2);

                            x = v1_position[0] - v2_position[0];
                            y = v1_position[1] - v2_position[1];
                            z = v1_position[2] - v2_position[2];
                        }


                        float temp = gradient[idx][i] -
                                     gradient[neighborVerts[m]]
                                             [i]; //i=0:x i=1:y i=2:z
                        float weight = 1.0 / std::sqrt(std::pow(x, 2) +
                                                       std::pow(y, 2) +
                                                       std::pow(z, 2));
                        value += temp * weight;
                        weightSum += weight;
                    }
                    if (weightSum > 0) hessian(i, j) = value / weightSum;
                    if (i != j) hessian(j, i) = hessian(i, j);
                }
            }

            Eigen::Vector3d n = -gp.normalized();
            Eigen::Matrix3d I = Eigen::Matrix3d::Identity();
            Eigen::Matrix3d B = (I - n * n.transpose()) * hessian;

            Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> es(B);
            Eigen::Vector3d eigenvalues = es.eigenvalues();
            float k2 = eigenvalues(0);
            float k1 = eigenvalues(1);

            curvature[idx][0] = k1;
            curvature[idx][1] = k2;
            curvature[idx][2] = (k1 + k2) / 2.0;
            curvature[idx][3] = k1 * k2;

            curvatures->AddElement2(curvature[idx][2], curvature[idx][3]);
        }
        UpdateProgress(1.0f);
        return true;
    }

    std::vector<std::array<float, 3>>
    GetPointGradient(int type, Points::Pointer Points, int PointNum) {

        AttributeSet* attributeSet;
        if (type == 0) attributeSet = surface_Mesh->GetAttributeSet();
        else if (type == 1)
            attributeSet = volume_Mesh->GetAttributeSet();

        auto data = attributeSet->GetAttribute(curIndex).pointer;
        int dimension = data->GetDimension();

        std::vector<std::array<float, 3>> gradient(PointNum,
                                                   {0.0f, 0.0f, 0.0f});
        std::vector<float> sumWeights(PointNum, 0.0f);

        igIndex neighborVerts[256]{};
        // 计算点的梯度
        for (igIndex idx = 0; idx < PointNum; ++idx) {
            int NeighborNum;
            // 获取邻接顶点
            if (type == 1)
                NeighborNum = volume_Mesh->GetPointToOneRingPoints(
                        idx, neighborVerts);
            else if (type == 0)
                NeighborNum = surface_Mesh->GetPointToOneRingPoints(
                        idx, neighborVerts);

            auto v1 = Points->GetPoint(idx);

            for (int m = 0; m < NeighborNum; m++) {
                Vector<float, 3> v2;
                if (type == 1) v2 = volume_Mesh->GetPoint(neighborVerts[m]);
                else if (type == 0)
                    v2 = surface_Mesh->GetPoint(neighborVerts[m]);

                float x = v1[0] - v2[0];
                float y = v1[1] - v2[1];
                float z = v1[2] - v2[2];

                double value = data->GetValue(dimension * idx) -
                               data->GetValue(dimension * neighborVerts[m]);

                float weight = 1.0f / std::sqrt(x * x + y * y + z * z);
                sumWeights[idx] += weight;
                gradient[idx][0] += x * weight * value;
                gradient[idx][1] += y * weight * value;
                gradient[idx][2] += z * weight * value;
            }
            if (sumWeights[idx] > 0) {
                gradient[idx][0] /= sumWeights[idx];
                gradient[idx][1] /= sumWeights[idx];
                gradient[idx][2] /= sumWeights[idx];
            }
        }

        return gradient;
    }

    std::vector<std::array<float, 3>> GetOtherGradient(int type, int Num) {
        AttributeSet* attributeSet;
        if (type == 0) attributeSet = surface_Mesh->GetAttributeSet();
        else if (type == 1)
            attributeSet = volume_Mesh->GetAttributeSet();

        auto data = attributeSet->GetAttribute(curIndex).pointer;
        int dimension = data->GetDimension();

        std::vector<std::array<float, 3>> gradient(Num, {0.0f, 0.0f, 0.0f});
        std::vector<float> sumWeights(Num, 0.0f);

        igIndex neighborVerts[256]{};
        // 计算点的梯度
        for (igIndex idx = 0; idx < Num; ++idx) {
            int NeighborNum;
            // 获取邻接顶点
            if (type == 1)
                // neighbors:volumeIds
                NeighborNum = volume_Mesh->GetVolumeToNeighborVolumesWithFace(
                        idx, neighborVerts);

            else if (type == 0)
                // neighbors:faceIds
                NeighborNum = surface_Mesh->GetFaceToNeighborFaces(
                        idx, neighborVerts);

            for (int m = 0; m < NeighborNum; m++) {
                float x, y, z;
                if (type == 1) {
                    igIndex* volumeIds;
                    auto v1 = volume_Mesh->GetVolume(idx);
                    auto v2 = volume_Mesh->GetVolume(neighborVerts[m]);
                    auto size_v1 = v1->GetNumberOfPoints();
                    auto size_v2 = v2->GetNumberOfPoints();

                    std::array<float, 3> v1_position =
                            GetPosition_volume(v1, size_v1);
                    std::array<float, 3> v2_position =
                            GetPosition_volume(v2, size_v2);

                    x = v1_position[0] - v2_position[0];
                    y = v1_position[1] - v2_position[1];
                    z = v1_position[2] - v2_position[2];

                } else if (type == 0) {
                    auto v1 = surface_Mesh->GetFace(idx);
                    auto v2 = surface_Mesh->GetFace(neighborVerts[m]);
                    auto size_v1 = v1->GetNumberOfPoints();
                    auto size_v2 = v2->GetNumberOfPoints();

                    std::array<float, 3> v1_position =
                            GetPosition_face(v1, size_v1);
                    std::array<float, 3> v2_position =
                            GetPosition_face(v2, size_v2);

                    x = v1_position[0] - v2_position[0];
                    y = v1_position[1] - v2_position[1];
                    z = v1_position[2] - v2_position[2];
                }
                // 标量计算时就算是三维数据也默认取第一维
                double value = data->GetValue(idx * dimension) -
                               data->GetValue(neighborVerts[m] * dimension);

                float weight = 1.0f / std::sqrt(x * x + y * y + z * z);
                sumWeights[idx] += weight;
                gradient[idx][0] += x * weight * value;
                gradient[idx][1] += y * weight * value;
                gradient[idx][2] += z * weight * value;
            }
            if (sumWeights[idx] > 0) {
                gradient[idx][0] /= sumWeights[idx];
                gradient[idx][1] /= sumWeights[idx];
                gradient[idx][2] /= sumWeights[idx];
            }
        }

        return gradient;
    }

protected:
    CurvatureFilter()
    //输入输出个数
    {
        SetNumberOfInputs(1);
        SetNumberOfOutputs(1);
    }
    ~CurvatureFilter() override = default;

    SurfaceMesh::Pointer surface_Mesh{};
    VolumeMesh::Pointer volume_Mesh{};
    AttributeSet* attributeSet{nullptr};

    int curIndex{-1};
    int curDim{-1};
};

IGAME_NAMESPACE_END
#endif