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

class LaplacianFilter : public Filter {
public:
    I_OBJECT(LaplacianFilter);
    static Pointer New() { return new LaplacianFilter; }

    void SetAttributeByIndex(int index) { curIndex = index; }
    void SetAttributeByName(const std::string& name) { this->name = name; }

    bool Execute() override {
        auto input = GetInput(0);
        if (input == nullptr) return false;

        auto CheckType = [&]() -> bool {
            attributeSet = input->GetAttributeSet();
            if (attributeSet == nullptr) return false;
            if (curIndex == -1 && name == "") return false;
            if (curIndex == -1) curIndex = attributeSet->GetAttributeIndex(name);
            if (curIndex < 0 || curIndex >= attributeSet->GetNumberOfAttributes()) return false;

            int dim = input->GetAttributeSet()->GetAttribute(curIndex).pointer->GetDimension();
            if (dim != 1) { return false; }
            return true;
        };

        SetOutput(input);

        switch (input->GetDataObjectType()) {
            case IG_SURFACE_MESH: {
                surface_Mesh = DynamicCast<SurfaceMesh>(input);
                if (!CheckType()) return false;
            } break;
            case IG_VOLUME_MESH: {
                return false;
                //volume_Mesh = DynamicCast<VolumeMesh>(input);
                //if (volume_Mesh) {
                //    surface_Mesh = DynamicCast<SurfaceMesh>(
                //            volume_Mesh->GetDisplayObject());
                //    if (!surface_Mesh) return false;

                //    if (!CheckType()) return false;

                //    FloatArray::Pointer Laplacians = FloatArray::New();
                //    Laplacians->SetDimension(1);
                //    Laplacians->SetName("laplacians");
                //    input->GetAttributeSet()->AddScalar(IG_POINT, Laplacians);
                //}
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
                    //surface_Mesh =
                    //        DynamicCast<SurfaceMesh>(mesh->GetDisplayObject());
                    //if (!surface_Mesh) return false;

                    //if (!CheckType()) return false;

                    //FloatArray::Pointer Laplacians = FloatArray::New();
                    //Laplacians->SetDimension(1);
                    //Laplacians->SetName("laplacians");
                    //input->GetAttributeSet()->AddScalar(IG_POINT, Laplacians);
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
            //// 附着在point
            //if (PointNum != 0 && attachmentType == 0)
            //    return GetPointLaplacian(1, Points, PointNum);
            //// 附着在cell
            //else if (VolumeNum != 0 && attachmentType == 1)
            //    return GetOtherLaplacian(1, VolumeNum);

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
                return GetPointLaplacian(0, Points, PointNum,surface_Mesh);
            // 附着在cell
            else if (FaceNum != 0 && attachmentType == 1)
                return GetOtherLaplacian(0, FaceNum);
        }
        return false;
    }
    
private:
    // 表面/体网格：点
    //bool GetPointLaplacian(int type, Points::Pointer Points, int PointNum) {

    //    AttributeSet* attributeSet;
    //    if (type == 0) attributeSet = surface_Mesh->GetAttributeSet();
    //    else if (type == 1)
    //        attributeSet = volume_Mesh->GetAttributeSet();

    //    auto data = attributeSet->GetAttribute(curIndex).pointer;

    //    int dimension = data->GetDimension();
    //    FloatArray::Pointer Laplacians = FloatArray::New();
    //    Laplacians->SetDimension(1);
    //    Laplacians->Reserve(PointNum);
    //    Laplacians->SetName("laplacians");
    //    attributeSet->AddScalar(IG_POINT, Laplacians);

    //    std::vector<float> laplacian(PointNum, 0.0f);

    //    igIndex neighborVerts[256]{};
    //    int hundred = PointNum / 100;
    //    // 计算点的梯度
    //    int progress = 0;
    //    int block = PointNum / 100;
    //    for (igIndex idx = 0; idx < PointNum; ++idx) {
    //        if(idx % hundred == 0) UpdateProgress((double)idx / PointNum);

    //        int NeighborNum;

    //        float temp = 0.0;
    //        float weightSum = 0.0;
    //        // 获取邻接顶点
    //        if (type == 1)
    //            NeighborNum = volume_Mesh->GetPointToOneRingPoints(
    //                    idx, neighborVerts);
    //        else if (type == 0)
    //            NeighborNum = surface_Mesh->GetPointToOneRingPoints(
    //                    idx, neighborVerts);

    //        auto v1 = Points->GetPoint(idx);

    //        for (int m = 0; m < NeighborNum; m++) {
    //            Vector<float, 3> v2;
    //            if (type == 1) v2 = volume_Mesh->GetPoint(neighborVerts[m]);
    //            else if (type == 0)
    //                v2 = surface_Mesh->GetPoint(neighborVerts[m]);

    //            float x = v1[0] - v2[0];
    //            float y = v1[1] - v2[1];
    //            float z = v1[2] - v2[2];

    //            // 标量计算时就算是三维数据也默认取第一维
    //            double value = data->GetValue(dimension * idx) -
    //                           data->GetValue(dimension * neighborVerts[m]);

    //            float weight = 1.0f / std::sqrt(x * x + y * y + z * z);
    //            weightSum += weight;
    //            temp += weight * value;
    //        }
    //        if (weightSum > 0) {
    //            laplacian[idx] = temp / weightSum;
    //            Laplacians->AddValue(laplacian[idx]);
    //        }
    //    }
    //    UpdateProgress(1.0f);
    //    return true;
    //}
    bool GetPointLaplacian(int type, Points::Pointer points, int pointNum, SurfaceMesh::Pointer surface_Mesh) {

        AttributeSet* attrSet = surface_Mesh->GetAttributeSet();
        if (!attrSet) return false;

        auto data = attrSet->GetAttribute(curIndex).pointer;
        const int dim = data->GetDimension();

        FloatArray::Pointer Laplacians = FloatArray::New();
        Laplacians->SetDimension(1);
        Laplacians->Reserve(pointNum);
        Laplacians->SetName("laplacians");
        attrSet->AddScalar(IG_POINT, Laplacians);

        igIndex neighborBuf[256];
        constexpr double kEps = 1e-12;

        const int step = std::max(1, pointNum / 100);
        for (igIndex i = 0; i < pointNum; ++i) {
            if (i % step == 0) UpdateProgress(double(i) / std::max(1, pointNum));

            int nb = surface_Mesh->GetPointToOneRingPoints(i, neighborBuf);
            igIndex* nbr = neighborBuf;
            std::vector<igIndex> dyn;
            if (nb > 256) {
                dyn.resize(nb);
                nb = surface_Mesh->GetPointToOneRingPoints(i, dyn.data());
                nbr = dyn.data();
            }

            const auto pi = points->GetPoint(i);
            const double fi = data->GetValue(dim * i);

            double num = 0.0;
            double wsum = 0.0;
            for (int k = 0; k < nb; ++k) {
                const igIndex j = nbr[k];
                if (j == i) continue;

                const auto pj = surface_Mesh->GetPoint(j);
                const double dx = pi[0] - pj[0];
                const double dy = pi[1] - pj[1];
                const double dz = pi[2] - pj[2];

                const double w = 1.0 / std::sqrt(dx * dx + dy * dy + dz * dz + kEps);
                const double fj = data->GetValue(dim * j);
                num += w * fj;
                wsum += w;
            }

            const double lap = (wsum > 0.0) ? (num / wsum - fi) : 0.0;
            Laplacians->AddValue(static_cast<float>(lap));
        }

        UpdateProgress(1.0);
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

        auto data = attributeSet->GetAttribute(curIndex).pointer;
        int dimension = data->GetDimension();

        FloatArray::Pointer laplacians = FloatArray::New();
        laplacians->SetDimension(1);
        laplacians->Resize(Num);
        laplacians->SetName("laplacians");
        attributeSet->AddScalar(IG_POINT, laplacians);


        igIndex neighborVerts[256]{};
        std::vector<float> laplacian(Num, 0.0f);
        int hundred = Num / 100;
        for (igIndex idx = 0; idx < Num; ++idx) {
            if(idx % hundred == 0) UpdateProgress((double)idx / Num);
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
                float temp = 0.0;
                float weightSum = 0.0;
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
                auto value = data->GetValue(idx * dimension) -
                             data->GetValue(neighborVerts[m] * dimension);
                float weight = 1.0f / std::sqrt(x * x + y * y + z * z);

                weightSum += weight;
                temp += weight * value;

                if (weightSum > 0) {
                    laplacian[idx] = temp / weightSum;
                    laplacians->AddValue(laplacian[idx]);
                }
            }
        }
        UpdateProgress(1.0f);
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
    AttributeSet* attributeSet{nullptr};

    int curIndex{-1};
    std::string name;
};

IGAME_NAMESPACE_END
#endif
