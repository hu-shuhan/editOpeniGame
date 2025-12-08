#include "iGameLaplacianFilter.h"

//#include "torch/csrc/jit/ir/constants.h"

IGAME_NAMESPACE_BEGIN


void LaplacianFilter::SetAttributeByIndex(int index) { curIndex = index; }
void LaplacianFilter::SetAttributeByName(const std::string& name) { this->name = name; }

bool LaplacianFilter::Execute() {
    auto input = GetInput(0);
    if (input == nullptr) return false;

    auto CheckType = [&]() -> bool {
        attributeSet = input->GetAttributeSet();
        if (attributeSet == nullptr) {
            m_Message = "please choose a attribute";
            return false;
        }
        if (curIndex == -1 && name == "") {
            m_Message = "please choose a attribute";
            return false;
        }
        if (curIndex == -1) curIndex = attributeSet->GetAttributeIndex(name);
        if (curIndex < 0 || curIndex >= attributeSet->GetNumberOfAttributes()) {
            m_Message = "please choose a attribute";
            return false;
        }

        dim = input->GetAttributeSet()->GetAttribute(curIndex).pointer->GetDimension();
        m_currentAttributeDimension = input->GetCurrentAttributeDimension();
        if (dim != 1  && m_currentAttributeDimension == -1) {
            m_Message = "please choose a component";
            return false;
        }
        if (dim == 1)
            m_currentAttributeDimension=0;
        return true;
    };

    // SetOutput(input);

    switch (input->GetDataObjectType()) {
        case IG_SURFACE_MESH: {
            surface_Mesh = DynamicCast<SurfaceMesh>(input);
            if (!CheckType()) return false;

            // auto attachmentType =
            //         attributeSet->GetAttribute(curIndex).attachmentType;
            //
            // int FaceNum = surface_Mesh->GetNumberOfFaces();
            int PointNum = surface_Mesh->GetNumberOfPoints();
            Points::Pointer Points = surface_Mesh->GetPoints();
            surface_Mesh->RequestEditStatus();
            // // 附着在point
            // if (PointNum != 0 && attachmentType == 0)
            return GetPointLaplacian(0, Points, PointNum,surface_Mesh);
            // // 附着在cell
            // else if (FaceNum != 0 && attachmentType == 1)
            //     return GetOtherLaplacian(0, FaceNum);
        } break;
        case IG_VOLUME_MESH: {
            return false;
        } break;
        case IG_UNSTRUCTURED_MESH: {
            auto mesh = DynamicCast<UnstructuredMesh>(input);
            surface_Mesh = mesh->TransferToSurfaceMesh();
            volume_Mesh = mesh->TransferToVolumeMesh();

            if (surface_Mesh) {
                if (!CheckType()) return false;
                int PointNum = surface_Mesh->GetNumberOfPoints();
                Points::Pointer Points = surface_Mesh->GetPoints();
                surface_Mesh->RequestEditStatus();
                return GetPointLaplacian(0, Points, PointNum,surface_Mesh);
            }

            if (volume_Mesh) {
                return false;

            }
        } break;
        default:
            return false;
    }
    return false;
}

ArrayObject::Pointer LaplacianFilter::AttributeCell2Point(CellArray::Pointer Cell, ArrayObject::Pointer OriArray,
                                                         size_t PointNum) {
    int dim = OriArray->GetDimension();

    auto NewArray = FloatArray::New();
    NewArray->SetName(OriArray->GetName());
    NewArray->SetDimension(dim);
    NewArray->Reserve(PointNum);

    float scalar[16]{0}, temp[16]{0};
    for (int i = 0; i < PointNum; ++i) { NewArray->AddElement(scalar); }

    std::vector<int> PointAdjNum(PointNum, 0);

    igIndex cell[IGAME_CELL_MAX_SIZE];

    for (int i = 0; i < Cell->GetNumberOfCells(); ++i) {
        int size = Cell->GetCellIds(i, cell);
        OriArray->GetElement(i, scalar);
        for (int j = 0; j < size; ++j) {
            PointAdjNum[cell[j]]++;
            NewArray->GetElement(cell[j], temp);
            for (int d = 0; d < dim; ++d) temp[d] += scalar[d];
            NewArray->SetElement(cell[j], temp);
        }
    }

    for (int i = 0; i < PointNum; ++i) {
        NewArray->GetElement(i, temp);
        for (int d = 0; d < dim; ++d) temp[d] /= PointAdjNum[i];
        NewArray->SetElement(i, temp);
    }

    return NewArray;
}
    

bool LaplacianFilter::GetPointLaplacian(int type, Points::Pointer points, int pointNum,
                                        SurfaceMesh::Pointer surface_Mesh) {

    AttributeSet* attrSet = surface_Mesh->GetAttributeSet();
    int NumPoints = surface_Mesh->GetNumberOfPoints();
    // int NumCells = surface_Mesh->GetNumberOfFaces();
    ArrayObject::Pointer Data = attrSet->GetAttribute(curIndex).pointer;

    if (attrSet->GetAttribute(curIndex).attachmentType == IG_CELL) {
        Data = AttributeCell2Point(surface_Mesh->GetFaces(), Data, NumPoints);
    }

    FloatArray::Pointer Laplacians = FloatArray::New();
    Laplacians->SetDimension(1);
    Laplacians->Reserve(pointNum);
    Laplacians->SetName("laplacians");
    // attrSet->AddScalar(IG_POINT, Laplacians);

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
        const double fi = Data->GetValue(dim * i+ m_currentAttributeDimension);

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
            const double fj = Data->GetValue(dim * j + m_currentAttributeDimension);
            num += w * fj;
            wsum += w;
        }

        const double lap = (wsum > 0.0) ? (num / wsum - fi) : 0.0;
        Laplacians->AddValue(static_cast<float>(lap));
    }

    auto newAttrs = surface_Mesh->GetAttributeSet();
    newAttrs->AddScalar(IG_POINT, Laplacians);
    newAttrs->ForceReConvertToDrawableData();
    SetOutput(surface_Mesh);

    UpdateProgress(1.0);
    return true;
}

std::array<float, 3> LaplacianFilter::GetPosition_volume(Volume* v, int num) {
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
std::array<float, 3> LaplacianFilter::GetPosition_face(Face* f, int num) {
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

bool LaplacianFilter::GetOtherLaplacian(int type, int Num) {

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


LaplacianFilter::LaplacianFilter()
//输入输出个数
{
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}


IGAME_NAMESPACE_END
