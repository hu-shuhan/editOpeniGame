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
#include <unordered_set>
#include <algorithm>


IGAME_NAMESPACE_BEGIN

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
                // volume_Mesh = DynamicCast<VolumeMesh>(input);
                // if (volume_Mesh) {
                //     surface_Mesh = DynamicCast<SurfaceMesh>(
                //             volume_Mesh->GetDisplayObject());
                //     if (!surface_Mesh) return false;
                //
                //     if (!CheckType()) return false;
                //
                //     FloatArray::Pointer curvatures = FloatArray::New();
                //     curvatures->SetDimension(2);
                //     curvatures->SetName("curvatures");
                //     volume_Mesh->GetAttributeSet()->AddScalar(IG_POINT,
                //                                               curvatures);
                // }
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
                    // surface_Mesh =
                    //         DynamicCast<SurfaceMesh>(mesh->GetDisplayObject());
                    // if (!surface_Mesh) return false;
                    //
                    // if (!CheckType()) return false;
                    //
                    // FloatArray::Pointer curvatures = FloatArray::New();
                    // curvatures->SetDimension(2);
                    // curvatures->SetName("curvatures");
                    // mesh->GetAttributeSet()->AddScalar(IG_POINT, curvatures);
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
            if (PointNum != 0 && attachmentType == 0) {
                // return GetPointCurvature(0, Points, PointNum);
                surface_Mesh = TriangulateSurfaceMesh(surface_Mesh);
                return ComputeSurfaceCurvatureCotangent(surface_Mesh, attributeSet, curIndex);
            }
            // 附着在cell
            else if (FaceNum != 0 && attachmentType == 1)
                return GetOtherCurvature(0, FaceNum);
        }

        return false;
    }

bool ComputeSurfaceCurvatureCotangent(SurfaceMesh::Pointer surface_Mesh,
                                      AttributeSet::Pointer Attributes,
                                      int Index)
{
    const int nV = surface_Mesh->GetNumberOfPoints();
    const int nF = surface_Mesh->GetNumberOfFaces();
    if (nV <= 0 || nF <= 0) return false;

    auto attrSet = surface_Mesh->GetAttributeSet();

    FloatArray::Pointer curv_mean = FloatArray::New();
    curv_mean->SetDimension(1);
    curv_mean->Resize(nV);
    curv_mean->SetName("cur_mean");
    attrSet->AddScalar(IG_POINT, curv_mean);

    FloatArray::Pointer curv_gaussian = FloatArray::New();
    curv_gaussian->SetDimension(1);
    curv_gaussian->Resize(nV);
    curv_gaussian->SetName("cur_gaussian");
    attrSet->AddScalar(IG_POINT, curv_gaussian);

    auto toEigen = [](const Vector<float,3>& p)->Eigen::Vector3d {
        return {double(p[0]), double(p[1]), double(p[2])};
    };
    auto triArea = [](const Eigen::Vector3d& a,
                      const Eigen::Vector3d& b,
                      const Eigen::Vector3d& c)->double {
        return 0.5 * ((b-a).cross(c-a)).norm();
    };
    auto angleABC = [](const Eigen::Vector3d& A,
                       const Eigen::Vector3d& B,
                       const Eigen::Vector3d& C)->double {
        Eigen::Vector3d u = (A - B).normalized();
        Eigen::Vector3d v = (C - B).normalized();
        double cosv = std::clamp(u.dot(v), -1.0, 1.0);
        return std::acos(cosv);
    };
    auto cot = [](double ang)->double {
        const double eps = 1e-8;  // 稳定些
        double s = std::sin(ang);
        double c = std::cos(ang);
        if (std::abs(s) < eps) return (c >= 0 ? 1.0/eps : -1.0/eps);
        return c / s;
    };

    std::vector<Eigen::Vector3d> Hn(nV, Eigen::Vector3d::Zero());
    std::vector<double> mixArea(nV, 0.0);
    std::vector<double> angleSum(nV, 0.0);

    struct EdgeKey {
        int a, b;
        bool operator==(const EdgeKey& o) const { return a==o.a && b==o.b; }
    };
    struct EdgeKeyHash {
        size_t operator()(const EdgeKey& k) const {
            return (size_t(k.a) << 32) ^ size_t(k.b);
        }
    };
    std::unordered_map<EdgeKey,int,EdgeKeyHash> edgeUse;
    auto add_edge = [&](int u, int v){
        if (u>v) std::swap(u,v);
        edgeUse[{u,v}]++;
    };

    int progress = 0;
    int block = std::max(1, nF / 100);

    for (int f = 0; f < nF; ++f) {
        if (f >= block * progress) { UpdateProgress(progress * 0.01); ++progress; }

        auto face = surface_Mesh->GetFace(f);
        int m = face->GetNumberOfPoints();
        if (m < 3) continue;

        std::vector<int> vids(m);
        for (int k = 0; k < m; ++k) vids[k] = int(face->GetPointId(k));

        for (int k = 0; k < m; ++k) add_edge(vids[k], vids[(k+1)%m]);

        for (int k = 1; k + 1 < m; ++k) {
            int i0 = vids[0], i1 = vids[k], i2 = vids[k+1];
            Eigen::Vector3d p0 = toEigen(surface_Mesh->GetPoint(i0));
            Eigen::Vector3d p1 = toEigen(surface_Mesh->GetPoint(i1));
            Eigen::Vector3d p2 = toEigen(surface_Mesh->GetPoint(i2));

            double A = triArea(p0,p1,p2);
            if (A <= 1e-20) continue;

            double a0 = angleABC(p1, p0, p2); // at p0
            double a1 = angleABC(p0, p1, p2); // at p1
            double a2 = angleABC(p0, p2, p1); // at p2

            angleSum[i0] += a0;
            angleSum[i1] += a1;
            angleSum[i2] += a2;

            auto accumulateVoronoi = [&](int ivtx, double opp1, double opp2,
                                         const Eigen::Vector3d& Pi,
                                         const Eigen::Vector3d& Pj,
                                         const Eigen::Vector3d& Pk) {
                bool obtuse = (a0 > M_PI/2) || (a1 > M_PI/2) || (a2 > M_PI/2);
                if (!obtuse) {
                    double lij = (Pj - Pi).squaredNorm();
                    double lik = (Pk - Pi).squaredNorm();
                    double v = (cot(opp1) * lij + cot(opp2) * lik) * 0.125;
                    mixArea[ivtx] += v;
                } else {
                    int obtId = (a0 > M_PI/2) ? i0 : ((a1 > M_PI/2) ? i1 : i2);
                    if (ivtx == obtId) mixArea[ivtx] += 0.5 * A;
                    else               mixArea[ivtx] += 0.25 * A;
                }
            };
            accumulateVoronoi(i0, a1, a2, p0, p1, p2);
            accumulateVoronoi(i1, a0, a2, p1, p0, p2);
            accumulateVoronoi(i2, a0, a1, p2, p0, p1);

            double cot0 = cot(a2);
            double cot1 = cot(a0);
            double cot2 = cot(a1);
            auto addEdgeContrib = [&](int ia, int ib, double cotOpp) {
                Eigen::Vector3d xa = toEigen(surface_Mesh->GetPoint(ia));
                Eigen::Vector3d xb = toEigen(surface_Mesh->GetPoint(ib));
                Eigen::Vector3d e = xa - xb;
                Hn[ia] += 0.5 * cotOpp * e;
                Hn[ib] -= 0.5 * cotOpp * e;
            };
            addEdgeContrib(i0,i1,cot2);
            addEdgeContrib(i1,i2,cot0);
            addEdgeContrib(i2,i0,cot1);
        }
    }

    std::vector<double> areas = mixArea;
    std::vector<double> areasPos;
    areasPos.reserve(nV);
    for (double a : areas) if (a > 0) areasPos.push_back(a);
    double areaFloor = 1e-10;
    if (!areasPos.empty()) {
        std::nth_element(areasPos.begin(),
                         areasPos.begin() + areasPos.size()/2,
                         areasPos.end());
        double med = areasPos[areasPos.size()/2];
        areaFloor = std::max(1e-12, med * 1e-4);
    }
    for (int i=0;i<nV;++i) if (mixArea[i] <= areaFloor) mixArea[i] = areaFloor;


    std::vector<bool> isBoundary(nV,false);
    for (auto& kv : edgeUse) {
        if (kv.second == 1) {
            isBoundary[kv.first.a] = true;
            isBoundary[kv.first.b] = true;
        }
    }

    std::vector<double> H_values(nV), K_values(nV);
    const double twoPi = 2.0 * M_PI;
    const double onePi = M_PI;

    for (int i=0;i<nV;++i) {
        double H = Hn[i].norm() / (2.0 * mixArea[i]);
        double K = (isBoundary[i] ? (onePi - angleSum[i])
                                  : (twoPi - angleSum[i])) / mixArea[i];
        H_values[i] = H;
        K_values[i] = K;
    }

    auto clamp_by_quantile = [](std::vector<double>& v, double qlo, double qhi){
        if (v.empty()) return;
        std::vector<double> tmp = v;
        auto nth_q = [&](double q){
            size_t idx = size_t(std::clamp(q, 0.0, 1.0) * (tmp.size()-1));
            std::nth_element(tmp.begin(), tmp.begin()+idx, tmp.end());
            return tmp[idx];
        };
        double lo = nth_q(qlo);
        double hi = nth_q(qhi);
        if (lo > hi) std::swap(lo,hi);
        for (double& x : v) {
            if (x < lo) x = lo;
            else if (x > hi) x = hi;
        }
    };
    clamp_by_quantile(H_values, 0.02, 0.98);
    clamp_by_quantile(K_values, 0.02, 0.98);

    for (int i=0;i<nV;++i){
        curv_mean->SetValue(i, H_values[i]);
        curv_gaussian->SetValue(i, K_values[i]);
    }

    UpdateProgress(1.0);
    return true;
}

double GetArea(Vector3d a, Vector3d b, Vector3d c) {
    return CrossProduct(a - b, a - c).length() / 2;
}

 SurfaceMesh::Pointer TriangulateSurfaceMesh(SurfaceMesh::Pointer mesh) {
    {
        bool f = true;
        igIndex face[16]{};
        for (int i = 0; i < mesh->GetNumberOfFaces(); i++) {
            int size = mesh->GetFacePointIds(i, face);
            if (size != 3) {
                f = false;
                break;
            }
        }

        if (f) {
            return mesh;
        }
    }
    auto attrbs = mesh->GetAttributeSet();

    CellArray::Pointer Faces = CellArray::New();
    Points::Pointer Points = mesh->GetPoints();

    igIndex face[16]{}, tri[3]{};
    for (int i = 0; i < mesh->GetNumberOfFaces(); i++) {
        int size = mesh->GetFacePointIds(i, face);

        if (size == 3) {
            Faces->AddCellId3(face[0], face[1], face[2]);
        } else if (size == 4) {
            Point p0 = mesh->GetPoint(face[0]);
            Point p1 = mesh->GetPoint(face[1]);
            Point p2 = mesh->GetPoint(face[2]);
            Point p3 = mesh->GetPoint(face[3]);
            double area01 = GetArea(p0, p1, p3);
            double area02 = GetArea(p1, p2, p3);

            double area11 = GetArea(p0, p1, p2);
            double area12 = GetArea(p2, p3, p0);

            double r0 = area01 / area02;
            double r1 = area11 / area12;

            if (r0 < 1) r0 = 1 / r0;
            if (r1 < 1) r1 = 1 / r1;
            if (r0 < r1) {
                Faces->AddCellId3(face[0], face[1], face[3]);
                Faces->AddCellId3(face[1], face[2], face[3]);
            } else {
                Faces->AddCellId3(face[0], face[1], face[2]);
                Faces->AddCellId3(face[2], face[3], face[0]);
            }

        } else {
            Point center(0, 0, 0);
            for (int j = 0; j < size; j++) { center += Points->GetPoint(face[j]); }
            center /= size;
            igIndex newPtId = Points->AddPoint(center);

            for (int j = 0; j < attrbs->GetNumberOfAttributes(); j++) {
                auto& attrb = attrbs->GetAttribute(j);
                if (attrb.isDeleted) continue;
                if (attrb.attachmentType == IG_POINT) {
                    double val[8]{0}, sum[8]{0};
                    int dim = attrb.pointer->GetDimension();
                    for (int k = 0; k < size; k++) {
                        attrb.pointer->GetElement(face[k], val);
                        for (int d = 0; d < dim; d++) { sum[d] += val[d]; }
                    }
                    for (int d = 0; d < dim; d++) { sum[d] /= size; }
                    attrb.pointer->AddElement(sum);
                }
            }

            for (int j = 0; j < size; j++) { Faces->AddCellId3(newPtId, face[j], face[(j + 1) % size]); }
        }
    }

    SurfaceMesh::Pointer Mesh = SurfaceMesh::New();
    Mesh->SetName(mesh->GetName());
    Mesh->SetPoints(Points);
    Mesh->SetFaces(Faces);
    Mesh->SetAttributeSet(mesh->GetAttributeSet());

    return Mesh;
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

        // FloatArray::Pointer curvatures = FloatArray::New();
        // curvatures->SetDimension(2);
        // curvatures->Reserve(Num);
        // curvatures->SetName("curvatures");
        // attributeSet->AddScalar(IG_POINT, curvatures);
        FloatArray::Pointer curv_mean = FloatArray::New();
        curv_mean->SetDimension(1);
        curv_mean->Reserve(Num);
        curv_mean->SetName("cur_mean");
        attributeSet->AddScalar(IG_POINT, curv_mean);

        FloatArray::Pointer curv_gaussian = FloatArray::New();
        curv_gaussian->SetDimension(1);
        curv_gaussian->Reserve(Num);
        curv_gaussian->SetName("cur_gaussian");
        attributeSet->AddScalar(IG_POINT, curv_gaussian);

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
            curv_mean->AddValue(curvature[idx][2]);
            curv_gaussian->AddValue(curvature[idx][3]);
            // curvatures->AddElement2(curvature[idx][2], curvature[idx][3]);
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