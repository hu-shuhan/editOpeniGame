#ifndef LaplacianFilter_h
#define LaplacianFilter_h

#include "Eigen/Dense"
#include "Eigen/Eigenvalues"
#include "iGameFilter.h"
#include "iGamePointSet.h"
#include "iGameSurfaceMesh.h"
#include "iGameVolumeMesh.h"
#include "iGameUnstructuredMesh.h"
#include <cmath>


IGAME_NAMESPACE_BEGIN

//现在默认取第一个数组
class LaplacianFilter : public Filter {

public:
    I_OBJECT(LaplacianFilter);
    static Pointer New() { return new LaplacianFilter; }
    bool Execute() override {

        surface_Mesh = DynamicCast<SurfaceMesh>(GetInput(0));
        volume_Mesh = DynamicCast<VolumeMesh>(GetInput(0));

        if (surface_Mesh == nullptr && volume_Mesh == nullptr) {
            auto mesh = DynamicCast<UnstructuredMesh>(GetInput(0));
            surface_Mesh = mesh->TransferToSurfaceMesh();
            volume_Mesh = mesh->TransferToVolumeMesh();

            if (surface_Mesh == nullptr && volume_Mesh == nullptr) {
                return false;
            }
        }

        AttributeSet* attributeSet;

        if (volume_Mesh) {
            attributeSet = volume_Mesh->GetAttributeSet();
            if (attributeSet == nullptr) return false;
            // 测试时默认取第一个数组
            auto attachmentType = attributeSet->GetAttribute(0).attachmentType;

            int VolumeNum = volume_Mesh->GetNumberOfVolumes();
            int PointNum = volume_Mesh->GetNumberOfPoints();
            Points::Pointer Points = volume_Mesh->GetPoints();
            volume_Mesh->RequestEditStatus();
            // 附着在point
            if (PointNum != 0 && attachmentType == 0)
                GetPointLaplacian(1, Points, PointNum);
            // 附着在cell
            else if (VolumeNum != 0 && attachmentType == 1)
                GetOtherLaplacian(1, VolumeNum);

        } else if (surface_Mesh) {
            attributeSet = surface_Mesh->GetAttributeSet();
            if (attributeSet == nullptr) return false;
            // 测试时默认取第一个数组
            auto attachmentType = attributeSet->GetAttribute(0).attachmentType;

            int FaceNum = surface_Mesh->GetNumberOfFaces();
            int PointNum = surface_Mesh->GetNumberOfPoints();
            Points::Pointer Points = surface_Mesh->GetPoints();
            surface_Mesh->RequestEditStatus();
            // 附着在point
            if (PointNum != 0 && attachmentType == 0)
                GetPointLaplacian(0, Points, PointNum);
            // 附着在cell
            else if (FaceNum != 0 && attachmentType == 1)
                GetOtherLaplacian(0, FaceNum);
        }
        return true;
    }
    // 表面/体网格：点
    bool GetPointLaplacian(int type, Points::Pointer Points, int PointNum) {

        AttributeSet* attributeSet;
        if (type == 0) attributeSet = surface_Mesh->GetAttributeSet();
        else if (type == 1)
            attributeSet = volume_Mesh->GetAttributeSet();

        auto data = attributeSet->GetAttribute(0).pointer;

        int dimension = data->GetDimension();
        FloatArray::Pointer Laplacians = FloatArray::New();
        Laplacians->SetDimension(1);
        Laplacians->Reserve(PointNum);
        Laplacians->SetName("laplacians");
        attributeSet->AddScalar(IG_POINT, Laplacians);

        std::vector<float> laplacian(PointNum, 0.0f);

        igIndex neighborVerts[64]{};
        // 计算点的梯度
        for (igIndex idx = 0; idx < PointNum; ++idx) {
            int NeighborNum;

            float temp = 0.0;
            float weightSum = 0.0;
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

                // 标量计算时就算是三维数据也默认取第一维
                double value = data->GetValue(dimension * idx) -
                               data->GetValue(dimension * neighborVerts[m]);

                float weight = 1.0f / std::sqrt(x * x + y * y + z * z);
                weightSum += weight;
                temp += weight * value;
            }
            if (weightSum > 0) {
                laplacian[idx] = temp / weightSum;
                Laplacians->AddValue(laplacian[idx]);
            }
        }

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

    bool GetOtherLaplacian(int type, int Num) {

        AttributeSet* attributeSet;
        if (type == 0) attributeSet = surface_Mesh->GetAttributeSet();
        else if (type == 1)
            attributeSet = volume_Mesh->GetAttributeSet();

        auto data = attributeSet->GetAttribute(0).pointer;
        int dimension = data->GetDimension();

        FloatArray::Pointer laplacians = FloatArray::New();
        laplacians->SetDimension(1);
        laplacians->Resize(Num);
        laplacians->SetName("laplacians");
        attributeSet->AddScalar(IG_POINT, laplacians);

        std::vector<float> laplacian(Num, 0.0f);
        for (igIndex idx = 0; idx < Num; ++idx) {

            igIndex* neighbors;
            int NeighborNum;
            // 获取邻接顶点
            if (type == 1)
                // neighbors:volumeIds
                NeighborNum = volume_Mesh->GetVolumeToNeighborVolumesWithFace(
                        idx, neighbors);
            else if (type == 0)
                // neighbors:faceIds
                NeighborNum =
                        surface_Mesh->GetFaceToNeighborFaces(idx, neighbors);

            for (int m = 0; m < NeighborNum; m++) {
                float temp = 0.0;
                float weightSum = 0.0;
                float x, y, z;
                if (type == 1) {
                    igIndex* volumeIds;
                    auto v1 = volume_Mesh->GetVolume(idx);
                    auto v2 = volume_Mesh->GetVolume(neighbors[m]);
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
                    auto v2 = surface_Mesh->GetFace(neighbors[m]);
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
                auto value = data->GetValue(idx * dimension) -
                             data->GetValue(neighbors[m] * dimension);
                float weight = 1.0f / std::sqrt(x * x + y * y + z * z);

                weightSum += weight;
                temp += weight * value;

                if (weightSum > 0) {
                    laplacian[idx] = temp / weightSum;
                    laplacians->AddValue(laplacian[idx]);
                }
            }
        }
        return true;
    }

protected:
    LaplacianFilter()
    //输入输出个数
    {
        SetNumberOfInputs(1);
        SetNumberOfOutputs(1);
    }
    ~LaplacianFilter() override = default;

    SurfaceMesh::Pointer surface_Mesh{};
    VolumeMesh::Pointer volume_Mesh{};
};

IGAME_NAMESPACE_END
#endif
