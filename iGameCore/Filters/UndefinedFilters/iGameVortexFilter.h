#ifndef VortexFilter_h
#define VortexFilter_h

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
class VortexFilter : public Filter {
public:
    I_OBJECT(VortexFilter);
    static Pointer New() { return new VortexFilter; }
    bool Execute() override {

        auto input = GetInput(0);
        switch (input->GetDataObjectType()) {
            case IG_SURFACE_MESH:
                surface_Mesh = DynamicCast<SurfaceMesh>(input);
                break;
            case IG_VOLUME_MESH:
                volume_Mesh = DynamicCast<VolumeMesh>(input);
                break;
            case IG_UNSTRUCTURED_MESH:
            {
                auto mesh = DynamicCast<UnstructuredMesh>(input);
                surface_Mesh = mesh->TransferToSurfaceMesh();
                volume_Mesh = mesh->TransferToVolumeMesh();

                if (volume_Mesh) {
                    surface_Mesh = DynamicCast<SurfaceMesh>(mesh->GetDisplayObject());
                    if (!surface_Mesh) return false;

                    FloatArray::Pointer vorticities = FloatArray::New();
                    vorticities->SetDimension(3);
                    vorticities->SetName("vorticities");
                    mesh->GetAttributeSet()->AddScalar(IG_POINT, vorticities);
                }
            } break;
            default:
                return false;
        }

        

        if (volume_Mesh) {
            //surface_Mesh =
            //        DynamicCast<SurfaceMesh>(volume_Mesh->GetDisplayObject());
            //if (!surface_Mesh) return false;

            //FloatArray::Pointer vorticities = FloatArray::New();
            //vorticities->SetDimension(3);
            //vorticities->SetName("vorticities");
            //volume_Mesh->GetAttributeSet()->AddScalar(IG_POINT, vorticities);

            //// 测试时默认取第一个数组
            //auto attachmentType = attributeSet->GetAttribute(0).attachmentType;

            //int VolumeNum = volume_Mesh->GetNumberOfVolumes();
            //int PointNum = volume_Mesh->GetNumberOfPoints();
            //Points::Pointer Points = volume_Mesh->GetPoints();
            //volume_Mesh->RequestEditStatus();
            //if (PointNum != 0 && attachmentType == 0) 
            //    GetPointVortex(1, Points, PointNum);
            //else if (VolumeNum != 0 && attachmentType == 0)
            //    GetOtherVortex(1, VolumeNum);
        } 

        if (surface_Mesh) {
            attributeSet = surface_Mesh->GetAttributeSet();
            auto attachmentType = attributeSet->GetAttribute(0).attachmentType;

            int FaceNum = surface_Mesh->GetNumberOfFaces();
            int PointNum = surface_Mesh->GetNumberOfPoints();
            Points::Pointer Points = surface_Mesh->GetPoints();
            surface_Mesh->RequestEditStatus();
            if (PointNum != 0 && attachmentType == 0) {
                GetPointVortex(0, Points, PointNum);
            } else if (FaceNum != 0 && attachmentType == 1)
                GetOtherVortex(0, FaceNum);
        }

        return true;
    }

    bool GetPointVortex(int type, Points::Pointer Points, int PointNum) {

        auto data = attributeSet->GetAttribute(0).pointer;

        int dimension = data->GetDimension();
        // 必须为三维向量
        if (dimension != 3) return false;

        std::cout << "[Debug  ] "
                  << "compute vortex" << '\n';

        FloatArray::Pointer vorticities = FloatArray::New();
        vorticities->SetDimension(3);
        vorticities->Reserve(PointNum);
        vorticities->SetName("vorticities");
        attributeSet->AddScalar(IG_POINT, vorticities);

        // 分别获取三个维度的梯度
        std::vector<std::array<float, 3>> gradient_1 =
                GetPointGradient(type, Points, PointNum, 0);
        std::vector<std::array<float, 3>> gradient_2 =
                GetPointGradient(type, Points, PointNum, 1);
        std::vector<std::array<float, 3>> gradient_3 =
                GetPointGradient(type, Points, PointNum, 2);

        // 计算涡旋
        for (igIndex idx = 0; idx < PointNum; ++idx) {
            const auto& grad_x = gradient_1[idx];
            const auto& grad_y = gradient_2[idx];
            const auto& grad_z = gradient_3[idx];

            //float omega_x = grad_x[0];
            float omega_x = grad_z[1] - grad_y[2]; // ∂vz/∂y - ∂vy/∂z
            float omega_y = grad_x[2] - grad_z[0]; // ∂vx/∂z - ∂vz/∂x
            float omega_z = grad_y[0] - grad_x[1]; // ∂vy/∂x - ∂vx/∂y

            //auto scalar = sqrt(omega_x * omega_x + omega_y * omega_y +
            //                   omega_z * omega_z);
            float scalar = 1;
            if (scalar > 1e-6) {
                vorticities->AddElement3(omega_x / scalar, omega_y / scalar,
                                         omega_z / scalar);
            } else
                vorticities->AddElement3(0, 0, 0);
        }

        return true;
    }

    bool GetOtherVortex(int type, int Num) {

        auto data = attributeSet->GetAttribute(0).pointer;
        int dimension = data->GetDimension();
        // 必须为三维向量
        if (dimension != 3) return false;

        std::cout << "[Debug  ] "
                  << "compute vortex" << '\n';

        FloatArray::Pointer vorticities = FloatArray::New();
        vorticities->SetDimension(3);
        vorticities->Reserve(Num);
        vorticities->SetName("vorticities");
        attributeSet->AddScalar(IG_POINT, vorticities);

        // 分别获取三个维度的梯度
        std::vector<std::array<float, 3>> gradient_1 =
                GetOtherGradient(type, Num, 0);
        std::vector<std::array<float, 3>> gradient_2 =
                GetOtherGradient(type, Num, 1);
        std::vector<std::array<float, 3>> gradient_3 =
                GetOtherGradient(type, Num, 2);

        // 计算涡旋
        for (igIndex idx = 0; idx < Num; ++idx) {
            const auto& grad_x = gradient_1[idx];
            const auto& grad_y = gradient_2[idx];
            const auto& grad_z = gradient_3[idx];

            float omega_x = grad_x[1] - grad_y[2]; // ∂vz/∂y - ∂vy/∂z
            float omega_y = grad_x[2] - grad_z[0]; // ∂vx/∂z - ∂vz/∂x
            float omega_z = grad_y[0] - grad_x[1]; // ∂vy/∂x - ∂vx/∂y

            auto scalar = sqrt(omega_x * omega_x + omega_y * omega_y +
                               omega_z * omega_z);

            //            vorticities->AddElement3(omega_x, omega_y, omega_z);
            if (scalar > 1e-6) {
                vorticities->AddElement3(omega_x / scalar, omega_y / scalar,
                                         omega_z / scalar);
            } else
                vorticities->AddElement3(0, 0, 0);
            //if (type == 0) {
            //    auto arr = surface_Mesh->GetMetadata()->GetStringArray(
            //            ATTRIBUTE_NAME_ARRAY);
            //    arr->AddElement("vorticities");
            //} else if (type == 1) {
            //    auto arr = volume_Mesh->GetMetadata()->GetStringArray(
            //            ATTRIBUTE_NAME_ARRAY);
            //    arr->AddElement("vorticities");
            //}
            return true;
        }

        //    bool GetPointVortex_ivd(int type, Points::Pointer Points, int PointNum){
        //
        //        PropertySet* m_PropertySet;
        //        if (type == 0 )
        //            m_PropertySet = surface_Mesh->GetPropertySet();
        //        else if ( type == 1 )
        //            m_PropertySet = volume_Mesh->GetPropertySet();
        //
        //        auto data = m_PropertySet->GetProperty(1).pointer;
        //        int dimension = data->GetElementSize();
        //        if(dimension != 3)
        //            return false;
        //
        //        std::cout << "[Debug  ] " << "compute vortex" << '\n';
        //
        //        FloatArray::Pointer vorticities_ivd = FloatArray::New();
        //        vorticities_ivd->SetElementSize(3);
        //        vorticities_ivd->Reserve(PointNum);
        //        m_PropertySet->AddScalar(IG_POINT,vorticities_ivd);
        //
        //        std::vector<std::array<float, 3>> vorticity(PointNum, {0.0f, 0.0f, 0.0f});
        //        std::vector scalar(PointNum,0.0f);
        //
        //        // 计算涡旋
        //        for (igIndex idx = 0; idx < PointNum; ++idx) {
        //            auto v1 = Points->GetPoint(idx);
        //            vorticity[idx][0] = data->GetValue(dimension * idx + 1) - data->GetValue(dimension * idx + 2);
        //            vorticity[idx][1] = data->GetValue(dimension * idx + 2) - data->GetValue(dimension * idx);
        //            vorticity[idx][0] = data->GetValue(dimension * idx) - data->GetValue(dimension * idx + 1);
        //            scalar[idx] = sqrt(vorticity[idx][0] * vorticity[idx][0] + vorticity[idx][1] * vorticity[idx][1]+vorticity[idx][2]*vorticity[idx][2]);
        //        }
        //
        //        // ivd
        //        float max_scalar = *std::max_element(scalar.begin(), scalar.end());
        //        float min_scalar = *std::min_element(scalar.begin(), scalar.end());
        //        std::vector<float> normalized_scalar(scalar.size());
        //        float total_ivd = 0.0f, ivd = 0.0f;
        //        for (size_t i = 0; i < scalar.size(); ++i) {
        //            if (max_scalar - min_scalar != 0) {
        //                normalized_scalar[i] = (scalar[i] - min_scalar) / (max_scalar - min_scalar);
        //                total_ivd+= normalized_scalar[i];
        //            } else {
        //                normalized_scalar[i] = 0.0;
        //            }
        //        }
        //        if(total_ivd>0)
        //            ivd = total_ivd / scalar.size();
        //
        //        for (igIndex idx = 0; idx < PointNum; idx++) {
        //            if ( (abs(normalized_scalar[idx]) - ivd) >= 0.00 )
        //                vorticities_ivd->AddElement3(vorticity[idx][0],vorticity[idx][1],vorticity[idx][2]);
        //            else vorticities_ivd->AddElement3(0,0,0);
        //        }
        //
        //        if (type == 0 ) {
        //            auto arr = surface_Mesh->GetMetadata()->GetStringArray(ATTRIBUTE_NAME_ARRAY);
        //            arr->AddElement("vorticities_ivd");
        //        } else if ( type == 1 )
        //        {
        //            auto arr = volume_Mesh->GetMetadata()->GetStringArray(ATTRIBUTE_NAME_ARRAY);
        //            arr->AddElement("vorticities_ivd");
        //        }
        //
        //        return true;
        //    }
    }
    std::vector<std::array<float, 3>> GetPointGradient(
            int type, Points::Pointer Points, int PointNum, int dim) {

        auto data = attributeSet->GetAttribute(0).pointer;
        int dimension = data->GetDimension();

        std::vector<std::array<float, 3>> gradient(PointNum,
                                                    {0.0f, 0.0f, 0.0f});
        std::vector<float> sumWeights(PointNum, 0.0f);

        igIndex neighborVerts[64]{};
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

                // 先默认读第一维
                float value =
                        data->GetValue(dimension * idx + dim) -
                        data->GetValue(dimension * neighborVerts[m] + dim);

                float weight = 1.0f / std::sqrt(x * x + y * y + z * z);
                sumWeights[idx] += weight;
                gradient[idx][0] += x * weight * value;
                gradient[idx][1] += y * weight * value;
                gradient[idx][2] += z * weight * value;

                //gradient[idx][0] += value;
                //gradient[idx][1] += value;
                //gradient[idx][2] += value;
            }
            if (sumWeights[idx] > 0) {
                gradient[idx][0] /= sumWeights[idx];
                gradient[idx][1] /= sumWeights[idx];
                gradient[idx][2] /= sumWeights[idx];
            }
        }
        return gradient;
    }

    std::array<float, 3> GetPosition_volume(Volume * v, int num) {
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
    std::array<float, 3> GetPosition_face(Face * f, int num) {
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

    std::vector<std::array<float, 3>> GetOtherGradient(int type, int Num,
                                                        int dim) {

        auto data = attributeSet->GetAttribute(0).pointer;
        int dimension = data->GetDimension();

        std::vector<std::array<float, 3>> gradient(Num, {0.0f, 0.0f, 0.0f});
        std::vector<float> sumWeights(Num, 0.0f);

        // 计算点的梯度
        for (igIndex idx = 0; idx < Num; ++idx) {

            igIndex* neighbors;
            int NeighborNum;
            // 获取邻接顶点
            if (type == 1)
                // neighbors:volumeIds
                NeighborNum =
                        volume_Mesh->GetVolumeToNeighborVolumesWithFace(
                                idx, neighbors);

            else if (type == 0)
                // neighbors:faceIds
                NeighborNum = surface_Mesh->GetFaceToNeighborFaces(
                        idx, neighbors);

            for (int m = 0; m < NeighborNum; m++) {
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
                double value =
                        data->GetValue(idx * dimension + dim) -
                        data->GetValue(neighbors[m] * dimension + dim);

                float weight = 1.0f / std::sqrt(x * x + y * y + z * z);
                sumWeights[idx] += weight;
                gradient[idx][0] += x * weight * value;
                gradient[idx][1] += y * weight * value;
                gradient[idx][2] += z * weight * value;
            }
            //            if (sumWeights[idx] > 0) {
            //                gradient[idx][0] /= sumWeights[idx];
            //                gradient[idx][1] /= sumWeights[idx];
            //                gradient[idx][2] /= sumWeights[idx];
            //            }
        }

        return gradient;
    }


protected:
    VortexFilter()
    //输入输出个数
    {
        SetNumberOfInputs(1);
        SetNumberOfOutputs(1);
    }
    ~VortexFilter() override = default;

    VolumeMesh::Pointer volume_Mesh{};
    SurfaceMesh::Pointer surface_Mesh{};
    AttributeSet* attributeSet{nullptr};
};

IGAME_NAMESPACE_END
#endif
