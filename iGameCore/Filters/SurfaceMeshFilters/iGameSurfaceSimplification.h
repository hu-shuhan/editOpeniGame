#ifndef SurfaceSimplification_h
#define SurfaceSimplification_h

#include "iGameFilter.h"
#include "iGameSurfaceMesh.h"
#include "iGamePriorityQueue.h"
#include "iGameQuadric.h"
#include "iGameFlexArray.h"
#include <iGameUnstructuredMesh.h>

IGAME_NAMESPACE_BEGIN
class SurfaceSimplification : public Filter {
public:
    I_OBJECT(SurfaceSimplification);
    static Pointer New() { return new SurfaceSimplification; }

    static constexpr int QEM_FASTEST = 0;
    static constexpr int QEM_NICEST = 1;

    static constexpr int QEM_INTERIOR_EDGE = 0;      // 内部边
    static constexpr int QEM_HALF_BOUNDARY_EDGE = 1; // 边界半边
    static constexpr int QEM_BOUNDARY_EDGE = 2;      // 边界边

    int TargetFaceNum = 0;         // 目标面数
    double TargetReduction = 0.5;  // 减少的百分比
    bool NormalCheck = true;       // 是否进行法线检查。
    double NormalThr = M_PI / 4.0; // 法线检查的阈值，以弧度表示。
    double CosineThr = cos(NormalThr); // 法线检查的余弦阈值。
    bool OptimalPosition = true;       // 是否使用最优位置。
    bool PreserveBoundary = false;     // 是否保持边界。
    double QuadricEpsilon = 1e-15;     // 二次型的阈值。
    bool QualityCheck = true;          // 是否进行质量检查。
    double QualityThr = 0.3;           // 用于质量检查的质量阈值。
    bool ScalarCheck = true;          // 是否进行标量检查。

	bool Execute() override {

        AttributeSet* att;
        mesh = DynamicCast<SurfaceMesh>(GetInput(0));
        if (mesh == nullptr) { 
            if (DynamicCast<UnstructuredMesh>(GetInput(0))) {
                mesh = DynamicCast<UnstructuredMesh>(GetInput(0))
                               ->TransferToSurfaceMesh();
                if (mesh == nullptr) { 
                    return false;
                }
            }
        }

        att = mesh->GetAttributeSet();

        auto temp = mesh;
        mesh = SurfaceMesh::New();
        mesh->DeepCopy(temp);
        newAttrs = mesh->GetAttributeSet();
        newAttrs->DeepCopy(att);

        //optimalScalar = FloatArray::New();
        //auto p = newAttrs->GetAttribute(0).pointer;
        //optimalScalar->SetDimension(p->GetDimension());
        //optimalScalar->Resize(p->GetDimension() * );
        mesh->RequestEditStatus();
        npts = mesh->GetNumberOfPoints();
        nedges = mesh->GetNumberOfEdges();
        nfaces = mesh->GetNumberOfFaces();

        heap = PriorityQueue::New();
        optimalPos.resize(nedges);
        if (this->TargetReduction != 0) {
            this->TargetFaceNum = nfaces * (1 - this->TargetReduction);
        }

        UpdateProgress(0.01);
        InitQuadric();
        UpdateProgress(0.05);

        ResetProgress(0.3);
        int blockNum = nedges / 100, progress = 0;
        for (int i = 0; i < nedges; i++) {
            InsertEdgeToHeap(i);
            if (i >= blockNum * progress) {
                UpdateProgress(progress * 0.01);
                progress++;
            }
        }

        ResetProgress();
        int needEliminatedNum = nfaces - TargetFaceNum;
        blockNum = needEliminatedNum / 100;
        progress = 0;
        for (int totalEliminated = 0; totalEliminated < needEliminatedNum;) {
            heap->update();
            if (heap->empty()) { break; }

            igIndex edgeId = heap->top().handle;
            double pri = heap->top().priority;
            heap->pop();

            igIndex e[2]{};
            mesh->GetEdgePointIds(edgeId, e);

            if (!mesh->IsCollapsable(edgeId)) continue;

            totalEliminated += mesh->GetNumberOfLinks(edgeId, SurfaceMesh::E2F);
            if (totalEliminated >= blockNum * progress) {
                UpdateProgress(0.01 * progress);
                progress++;
            }

            SurfaceMesh::ReturnContainer container;
            if (mode == QEM_FASTEST) {
                for (int i = 0; i < 2; ++i) {
                    mesh->GetPointToNeighborEdges(e[i], container);
                    for (int j = 0; j < container.size(); j++) {
                        heap->remove(container[j]);
                    }
                }
                quadrics[e[1]] += quadrics[e[0]];
            } else if (mode == QEM_NICEST) {
                for (int i = 0; i < 2; ++i) {
                    mesh->GetPointToNeighborFaces(e[i], container);
                    for (int j = 0; j < container.size(); j++) {
                        igIndex ids[3]{};
                        mesh->GetFaceEdgeIds(container[j], ids);
                        heap->remove(ids[0]);
                        heap->remove(ids[1]);
                        heap->remove(ids[2]);
                    }
                }
            }

            mesh->CollapseEdge(edgeId);
            mesh->SetPoint(e[1], optimalPos[edgeId]);

            if (this->ScalarCheck) {
                auto arr = newAttrs->GetAttribute(0).pointer;
                int dim = arr->GetDimension();
                for (int i = 0; i < dim; i++) {
                    double newScalar = arr->GetValue(e[0] * dim + i) +
                                       arr->GetValue(e[1] * dim + i);
                    newScalar /= 2;
                    arr->SetValue(e[1] * dim + i, newScalar);
                }
            }

            if (mode == QEM_FASTEST) {
                mesh->GetPointToNeighborEdges(e[1], container);
                for (int i = 0; i < container.size(); i++) {
                    this->InsertEdgeToHeap(container[i]);
                }
            } 
            else if (mode == QEM_NICEST) {
                FlexArray<igIndex> edgeIds;
                mesh->GetPointToNeighborEdges(e[1], container);
                for (int i = 0; i < container.size(); i++) {
                    edgeIds.push_back(container[i]);
                }
                mesh->GetPointToNeighborFaces(e[1], container);
                for (int i = 0; i < container.size(); i++) {
                    edgeIds.push_back(GetOppEdge(e[1], container[i]));
                }


                mesh->GetPointToOneRingPoints(e[1], container);
                for (int i = 0; i < container.size(); i++) {
                    quadrics[container[i]].setZero();
                    igIndex ids[32]{};
                    int size = mesh->GetPointToNeighborFaces(container[i], ids);
                    for (int j = 0; j < size; j++) {
                        Quadric q = QuadricFace(ids[j]);
                        quadrics[container[i]] += q;
                    }
                }

                quadrics[e[1]].setZero();
                igIndex ids[32]{};
                int size = mesh->GetPointToNeighborFaces(e[1], ids);
                for (int j = 0; j < size; j++) {
                    Quadric q = QuadricFace(ids[j]);
                    quadrics[e[1]] += q;
                }

                for (int i = 0; i < edgeIds.size(); i++) {
                    this->InsertEdgeToHeap(edgeIds[i]);
                }
            }
        }

        std::cout << "before: "
            << " point size: " << mesh->GetNumberOfPoints() 
            << " face size: " << mesh->GetNumberOfFaces() 
            << std::endl;

        auto arr = newAttrs->GetAttribute(0).pointer;
        int dim = arr->GetDimension();
        int k = 0;
        for (int d = 0; d < arr->GetDimension(); d++) {
            k = 0;
            for (int i = 0; i < npts; i++) {
                if (mesh->IsPointDeleted(i)) continue;
                arr->SetValue(k * dim + d, arr->GetValue(i * dim + d));
                k++;
            }
        }
        std::cout << k << std::endl;
        DynamicCast<FloatArray>(arr)->Resize(k);

        mesh->GarbageCollection();
        SetOutput(mesh);
        std::cout << "after: "
                  << " point size: " << mesh->GetNumberOfPoints()
                  << " face size: " << mesh->GetNumberOfFaces() << std::endl;

        return true;
    }

protected:
    SurfaceSimplification()
	{
		SetNumberOfInputs(1);
		SetNumberOfOutputs(1);
	}
    ~SurfaceSimplification() override = default;

    void InitQuadric() {
        quadrics.resize(npts);
        for (int i = 0; i < npts; i++) {
            quadrics[i].setZero();
        }

        igIndex f[3]{};
        for (int i = 0; i < nfaces; i++) {
            mesh->GetFacePointIds(i, f);
            Point v0 = mesh->GetPoint(f[0]);
            Point v1 = mesh->GetPoint(f[1]);
            Point v2 = mesh->GetPoint(f[2]);

            Vector3d d10 = v1 - v0;
            Vector3d d20 = v2 - v0;

            Vector3d normal = d10.cross(d20);

            double d = normal.dot(v0);
            Quadric<double> q;
            q.byPlane(normal, -d);

            quadrics[f[0]] += q;
            quadrics[f[1]] += q;
            quadrics[f[2]] += q;
        }
    }

    int EvaluateEdge(igIndex edgeId) {
        int type;
        igIndex e[2];
        mesh->GetEdgePointIds(edgeId, e);
        bool flag0 = mesh->IsBoundaryPoint(e[0]);
        bool flag1 = mesh->IsBoundaryPoint(e[1]);
        if (!flag0 && !flag1) {
            type = QEM_INTERIOR_EDGE;
        } else if (flag0 || flag1) {
            type = QEM_HALF_BOUNDARY_EDGE;
        } else {
            type = QEM_BOUNDARY_EDGE;
        }
        return type;
    }

    void InsertEdgeToHeap(igIndex edgeId) {
        int type = EvaluateEdge(edgeId);

        if ((type == QEM_BOUNDARY_EDGE || type == QEM_HALF_BOUNDARY_EDGE) &&
            PreserveBoundary) {
            return;
        }

        double priority = ComputePriority(edgeId);

        if (priority != std::numeric_limits<double>::max()) {
            heap->push(priority, edgeId);
        }
    }

    double ComputePriority(igIndex edgeId) {
        igIndex e[2]{};
        mesh->GetEdgePointIds(edgeId, e);
        Point v0 = mesh->GetPoint(e[0]);
        Point v1 = mesh->GetPoint(e[1]);

        igIndex ef[8]{};
        int size = mesh->GetEdgeToNeighborFaces(edgeId, ef);
        igIndex fid0 = size > 0 ? ef[0] : -1;
        igIndex fid1 = size > 1 ? ef[1] : -1;

        FlexArray<Vector3d, 32> origNormal;
        SurfaceMesh::ReturnContainer eorf;
        mesh->GetEdgeToOneRingFaces(edgeId, eorf);
        if (this->NormalCheck) {
            for (int i = 0; i < eorf.size(); ++i) {
                if (eorf[i] != fid0 && eorf[i] != fid1) {
                    origNormal[i] = Normal(eorf[i]);
                    origNormal[i].normalize();
                }
            }
        }

        double origQuality = std::numeric_limits<double>::max();
        if (this->QualityCheck) {
            for (int i = 0; i < eorf.size(); ++i) {
                if (eorf[i] != fid0 && eorf[i] != fid1) {
                    origQuality = std::min(origQuality, QualityFace(eorf[i]));
                }
            }
        }

        optimalPos[edgeId] = ComputePosition(edgeId);
        mesh->SetPoint(e[0], optimalPos[edgeId]);
        mesh->SetPoint(e[1], optimalPos[edgeId]);

        if (this->ScalarCheck) {
            auto arr = newAttrs->GetAttribute(0).pointer;
            int dim = arr->GetDimension();
            bool flag = true;
            SurfaceMesh::ReturnContainer container;
            for (int i = 0; i < dim; i++) {
                double newScalar = arr->GetValue(e[0] * dim + i) +
                                   arr->GetValue(e[1] * dim + i);
                newScalar /= 2;
                
                GetEdgeToOneRingPoints(edgeId, container);
                double sumScalar = 0;
                for (int j = 0; j < container.size(); j++) {
                    sumScalar += arr->GetValue(container[j] * dim + i);
                }
                sumScalar /= container.size();
                if (std::abs(newScalar - sumScalar) > 1e-1) { 
                    flag = false;
                    break;
                }
            }
            if (!flag) {
                return std::numeric_limits<double>::max();
            } 
        }

        double minCos = std::numeric_limits<double>::max();
        if (this->NormalCheck) {
            for (int i = 0; i < eorf.size(); ++i) {
                if (eorf[i] != fid0 && eorf[i] != fid1) {
                    Vector3f n = Normal(eorf[i]).normalized();
                    double cos = n.dot(origNormal[i]);
                    minCos = std::min(minCos, cos);
                }
            }
        }

        double newQuality = std::numeric_limits<double>::max();
        if (this->QualityCheck) {
            for (int i = 0; i < eorf.size(); ++i) {
                if (eorf[i] != fid0 && eorf[i] != fid1) {
                    newQuality = std::min(newQuality, QualityFace(eorf[i]));
                }
            }
        }

        mesh->SetPoint(e[0], v0);
        mesh->SetPoint(e[1], v1);

        Quadric<double> q = quadrics[e[0]];
        q += quadrics[e[1]];
        double error = q.apply(optimalPos[edgeId]);

        if (newQuality > this->QualityThr) newQuality = this->QualityThr;

        if (this->NormalCheck) {
            if (minCos > this->CosineThr) minCos = this->CosineThr;
            minCos = fabs((minCos + 1.0) / 2.0);
        }

        error = std::max(error, this->QuadricEpsilon);
        if (error <= this->QuadricEpsilon) { error *= (v0 - v1).norm(); }

        double priority = std::numeric_limits<double>::max();
        if (QualityCheck && newQuality < 0.1 && newQuality < origQuality) {
            return priority;
        }

        if (!this->QualityCheck && !this->NormalCheck) priority = error;
        if (this->QualityCheck && !this->NormalCheck)
            priority = error / newQuality;
        if (!this->QualityCheck && this->NormalCheck) priority = error / minCos;
        if (this->QualityCheck && this->NormalCheck)
            priority = error / (newQuality * minCos);
        return priority;
    }

    Vector3f ComputePosition(igIndex edgeId) {
        igIndex e[2]{};
        mesh->GetEdgePointIds(edgeId, e);
        Vector3f newPos = (mesh->GetPoint(e[0]) + mesh->GetPoint(e[1])) / 2.0;
        if (this->OptimalPosition) {
            Quadric q = quadrics[e[0]];
            q += quadrics[e[1]];
            if (q.apply(newPos) > this->QuadricEpsilon) {
                q.findMinimum(newPos);
            }
        }
        return newPos;
    }

    Vector3f Normal(igIndex faceId) {
        igIndex f[3]{};
        mesh->GetFacePointIds(faceId, f);
        Point v0 = mesh->GetPoint(f[0]);
        Point v1 = mesh->GetPoint(f[1]);
        Point v2 = mesh->GetPoint(f[2]);

        Vector3f d10 = v1 - v0;
        Vector3f d20 = v2 - v0;

        return CrossProduct(d10, d20);
    }

    Quadric<double> QuadricFace(igIndex faceId) {
        igIndex f[3]{};
        mesh->GetFacePointIds(faceId, f);
        Point v0 = mesh->GetPoint(f[0]);
        Point v1 = mesh->GetPoint(f[1]);
        Point v2 = mesh->GetPoint(f[2]);

        Vector3f d10 = v1 - v0;
        Vector3f d20 = v2 - v0;

        Vector3d normal = CrossProduct(d10, d20);
        double d = normal.dot(v0);
        Quadric<double> q;
        q.byPlane(normal, -d);
        return q;
    }

    double QualityFace(igIndex faceId) {
        igIndex f[3]{};
        mesh->GetFacePointIds(faceId, f);
        Point v0 = mesh->GetPoint(f[0]);
        Point v1 = mesh->GetPoint(f[1]);
        Point v2 = mesh->GetPoint(f[2]);

        Vector3f d10 = v1 - v0;
        Vector3f d20 = v2 - v0;
        Vector3f d12 = v1 - v2;

        Vector3f normal = CrossProduct(d10, d20);

        double a = normal.norm();
        if (a == 0) return 0;
        double b = std::max(d10.squaredNorm(),
                            std::max(d20.squaredNorm(), d12.squaredNorm()));
        if (b == 0) return 0;
        return a / b;
    }

    igIndex GetOppEdge(igIndex ptId, igIndex faceId) {
        igIndex f[3]{}, fe[3]{};
        mesh->GetFacePointIds(faceId, f);
        mesh->GetFaceEdgeIds(faceId, fe);
        int i = 0;
        while (f[i] != ptId) i++;
        return fe[(i + 1) % 3];
    }

    void GetEdgeToOneRingPoints(igIndex edgeId,
        SurfaceMesh::ReturnContainer& ptIds) {
        
        igIndex e[2]{};
        mesh->GetEdgePointIds(edgeId, e);
        std::set<igIndex> st;
        for (int i = 0; i < 2; i++) {
            mesh->GetPointToOneRingPoints(e[i], ptIds);
            for (int j = 0; j < ptIds.size(); j++) {
                if (ptIds[j] != e[1 - i]) { 
                    st.insert(ptIds[j]);
                }
            }
        }
        ptIds.reset();
        for (auto id: st) { 
            ptIds.push_back(id);
        }
    }

	SurfaceMesh::Pointer mesh{};
    PriorityQueue::Pointer heap{};
    std::vector<Quadric<double>> quadrics;
    std::vector<Vector3f> optimalPos;
    FloatArray::Pointer optimalScalar{};
    IGenum mode{QEM_NICEST};
    AttributeSet::Pointer newAttrs{}, oldAttrs{};

    IGsize npts{}, nedges{}, nfaces{};
};
IGAME_NAMESPACE_END
#endif