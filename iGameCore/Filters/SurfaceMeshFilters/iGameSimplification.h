#ifndef Simplification_h
#define Simplification_h

#include "iGameFilter.h"
#include "iGameSurfaceMesh.h"
#include "iGamePriorityQueue.h"
#include "iGameQuadric.h"
#include "iGameFlexArray.h"
#include <iGameUnstructuredMesh.h>

IGAME_NAMESPACE_BEGIN
class Simplification : public Filter {
public:
    I_OBJECT(Simplification);
    static Pointer New() { return new Simplification; }

    static constexpr int QEM_FASTEST = 0;
    static constexpr int QEM_NICEST = 1;

    static constexpr int QEM_INTERIOR_EDGE = 0;      // 内部边
    static constexpr int QEM_HALF_BOUNDARY_EDGE = 1; // 边界半边
    static constexpr int QEM_BOUNDARY_EDGE = 2;      // 边界边

    int TargetFaceNum = 0;         // 目标面数
    double TargetReduction = 0.8;  // 减少的百分比
    bool NormalCheck = true;       // 是否进行法线检查。
    double NormalThr = M_PI / 36.0; // 法线检查的阈值，以弧度表示。
    double CosineThr = cos(NormalThr); // 法线检查的余弦阈值。
    bool OptimalPosition = true;       // 是否使用最优位置。
    bool PreserveBoundary = false;     // 是否保持边界。
    double QuadricEpsilon = 1e-15;     // 二次型的阈值。
    bool QualityCheck = true;          // 是否进行质量检查。
    double QualityThr = 0.3;           // 用于质量检查的质量阈值。
    bool ScalarCheck = true;          // 是否进行标量检查。
    double DeltaThr = 2;         

    //std::vector<int> activedAttribIndices{1, 3, 7};
    std::vector<int> activedAttribIndices{0, 1, 2};
    std::vector<double> attribute_weights{0.3, 0.3, 0.3};

	bool Execute() override {

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
        //painter = m_Model->GetPainter();
        attrbs = mesh->GetAttributeSet();
        scalarIndex = 0;
        //auto temp = mesh;
        //mesh = SurfaceMesh::New();
        //mesh->DeepCopy(temp);
        //newAttrs = mesh->GetAttributeSet();
        //newAttrs->DeepCopy(att);

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
        optimalScalar.resize(nedges);
        mergeCount.resize(npts, 0);
        isCollapsable.resize(nedges, 1);

        {
            auto& box = mesh->GetBoundingBox();

            int attributes_count = activedAttribIndices.size();
            for (int i = 0; i < attributes_count; i++) {
                int index = activedAttribIndices[i];
                auto& pointer = attrbs->GetAttribute(index).pointer;
                double minVal = 1e27;
                double maxVal = 0;

                for (int j = 0; j < npts; j++) {
                    double val = pointer->GetValue(j) * attribute_weights[i];
                    maxVal = std::max(val, maxVal);
                    minVal = std::min(val, minVal);
                }
                double range = maxVal - minVal;
                attribute_weights[i] =
                        box.diag() / range * 10 / attributes_count;
            }
        }

        {
            int attributes_count = activedAttribIndices.size();
            attributes = FloatArray::New();
            attributes->SetDimension(attributes_count);
            attributes->Resize(npts);
            
            for (int i = 0; i < attributes_count; i++) 
            {
                int index = activedAttribIndices[i];
                auto& pointer = attrbs->GetAttribute(index).pointer;
                for (int j = 0; j < npts; j++) { 
                    double val = pointer->GetValue(j) * attribute_weights[i];
                    attributes->SetValue(
                            static_cast<IGsize>(j) * attributes_count + i, val);
                }
            }
        }
        
        {
            double T = 0;
            igIndex ids[8]{};
            int count = 0;
            for (int i = 0; i < nedges; i++) {
                int size = mesh->GetEdgeToNeighborFaces(i, ids);
                if (size == 2) { 
                    Vector3d n1 = Normal(ids[0]);
                    Vector3d n2 = Normal(ids[1]);

                    double theta = GetCosTheta(n1, n2);
                    T += theta * theta;
                    count++;
                }
            }
            T /= count;
            T = std::sqrt(T);
            double Smax = 0.9;
            double al = 1;
            double S = Smax * std::exp(-al * T);
            //S = 0.8;
            this->TargetReduction = S;
            std::cout << T << " " <<  S << std::endl;
        }

        if (this->TargetReduction != 0) {
            this->TargetFaceNum = nfaces * (1 - this->TargetReduction);
        }

        {
            //auto arr = attrbs->GetAttribute(scalarIndex).pointer;
            //int dim = arr->GetDimension();
            //scalarRange.resize(dim);
            //for (int i = 0; i < dim; i++) {
            //    scalarRange[i][0] = std::numeric_limits<double>::max();
            //    scalarRange[i][1] = -std::numeric_limits<double>::max();
            //}
            //for (int j = 0; j < npts; j++) {
            //    double val = arr->GetValue(j);
            //    scalarRange[0][0] = std::min(scalarRange[0][0], val);
            //    scalarRange[0][1] = std::max(scalarRange[0][1], val);
            //    //for (int i = 0; i < dim; i++) {
            //    //    double val = arr->GetValue(j * dim + i);
            //    //    scalarRange[i][0] = std::min(scalarRange[i][0], val);
            //    //    scalarRange[i][1] = std::max(scalarRange[i][1], val);
            //    //}
            //}
        }

        InitCategories();
        //return true;
        UpdateProgress(0.01);
        InitQuadric();
        UpdateProgress(0.05);

        ResetProgress(0.3);
        int blockNum = nedges / 100, progress = 0;
        for (int i = 0; i < nedges; i++) {
            InsertEdgeToHeap(i);
            //std::cout << i << std::endl;
            if (i >= blockNum * progress) {
                UpdateProgress(progress * 0.01);
                progress++;
            }
        }

        /*std::cout << "GarbageCollection" << std::endl;*/

        ResetProgress();
        int needEliminatedNum = nfaces - TargetFaceNum;
        blockNum = needEliminatedNum / 100;
        progress = 0;
        for (int totalEliminated = 0; totalEliminated < needEliminatedNum;) {
            heap->update();
            if (heap->empty()) { break; }
  
            igIndex edgeId = heap->top().handle;
            double pri = heap->top().priority;
            double geo_pri = heap->top().rest;
            int count = heap->top().count;

            heap->pop();

            //if (this->ScalarCheck) {
            //    if (geo_pri * (11 - count) * 0.1 > 2 * mean) {
            //        heap->push(pri * 1.1, edgeId, count + 1, geo_pri);
            //        continue;
            //    }
            //}

            igIndex e[2]{};
            mesh->GetEdgePointIds(edgeId, e);

            if (!mesh->IsCollapsable(edgeId)) continue;

            totalEliminated += mesh->GetNumberOfLinks(edgeId, SurfaceMesh::E2F);
            if (totalEliminated >= blockNum * progress) {
                UpdateProgress(0.01 * progress);
                //std::cout << progress << std::endl;
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
            mergeCount[e[1]]++;
            {
                int attributes_count = activedAttribIndices.size();
                for (int i = 0; i < attributes_count; i++) {
                    attributes->SetValue(e[1] * attributes_count + i,
                                         optimalScalar[edgeId][i]);
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
                    igIndex ids[64]{};
                    int size = mesh->GetPointToNeighborFaces(container[i], ids);
                    for (int j = 0; j < size; j++) {
                        Quadric q = QuadricFace(ids[j]);
                        quadrics[container[i]] += q;
                    }
                }

                quadrics[e[1]].setZero();
                igIndex ids[64]{};
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

        {
            int attributes_count = activedAttribIndices.size();

            for (int id = 0; id < attributes_count; id++) {
                int index = activedAttribIndices[id];
                auto& arr = attrbs->GetAttribute(index).pointer;
                
                int k = 0;
                for (int i = 0; i < npts; i++) {
                    if (mesh->IsPointDeleted(i)) continue;
                    double val = attributes->GetValue(
                            static_cast<IGsize>(i) * attributes_count + id);
                    arr->SetValue(k, val / attribute_weights[id]);
                    k++;
                }
                arr->Resize(k);
            }
        }

        mesh->GarbageCollection();
        //SetOutput(mesh);
        std::cout << "after: "
                  << " point size: " << mesh->GetNumberOfPoints()
                  << " face size: " << mesh->GetNumberOfFaces() << std::endl;

        return true;
    }

protected:
    Simplification()
	{
		SetNumberOfInputs(1);
		SetNumberOfOutputs(1);
	}
    ~Simplification() override = default;

    void InitCategories() { 
        //double mean = 0;
        //double s_delta = 0;
        //double delta = 0;
        //auto arr = attrbs->GetAttribute(scalarIndex).pointer;
        //int dim = arr->GetDimension();
        //scalarRange.resize(dim);
        //for (int i = 0; i < dim; i++) {
        //    scalarRange[i][0] = std::numeric_limits<double>::max();
        //    scalarRange[i][1] = -std::numeric_limits<double>::max();
        //}
        //for (int i = 0; i < npts; i++) {
        //    double val = arr->GetValue(i);
        //    mean += val;
        //    scalarRange[0][0] = std::min(scalarRange[0][0], val);
        //    scalarRange[0][1] = std::max(scalarRange[0][1], val);
        //    //for (int i = 0; i < dim; i++) {
        //    //    double val = arr->GetValue(j * dim + i);
        //    //    scalarRange[i][0] = std::min(scalarRange[i][0], val);
        //    //    scalarRange[i][1] = std::max(scalarRange[i][1], val);
        //    //}
        //}
        //mean /= npts;
        //for (int i = 0; i < npts; i++) {
        //    double val = arr->GetValue(i);
        //    s_delta += (val - mean) * (val - mean);
        //}
        //s_delta /= npts;
        //delta = std::sqrt(s_delta);

        //IntArray::Pointer a = IntArray::New();
        //a->SetName("a");
        //faceType.resize(nfaces, 0);
        //igIndex f[3]{};
        //for (int i = 0; i < nfaces; i++) {
        //    mesh->GetFacePointIds(i, f);
        //    double val0 = arr->GetValue(f[0]);
        //    double val1 = arr->GetValue(f[1]);
        //    double val2 = arr->GetValue(f[2]);
        //    int count = 0;
        //    if (std::abs(val0 - val1) >= DeltaThr * delta) count++;
        //    if (std::abs(val1 - val2) >= DeltaThr * delta) count++;
        //    if (std::abs(val0 - val2) >= DeltaThr * delta) count++;
        //    faceType[i] = count;
        //    if (count == 1 || count == 2) count = 1;
        //    if (count == 3) count = 2;
        //    
        //    a->AddValue(count);
        //}
        //mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_CELL, a);

        //double sum = 0;
        //double max_val = 0;
        //double min_val = 1e27; 
        //cellScalar.resize(nfaces);
        //for (int i = 0; i < nfaces; i++) { 
        //    igIndex f[3]{};
        //    mesh->GetFacePointIds(i, f);
        //    double val = (arr->GetValue(f[0]) + arr->GetValue(f[1]) +
        //                  arr->GetValue(f[2])) / 3;
        //    cellScalar[i] = val;
        //    sum += val;

        //    max_val = std::max(val, max_val);
        //    min_val = std::min(val, min_val);
        //}
        //mean = sum / npts;
        //range = max_val - min_val;

        //IntArray::Pointer b = IntArray::New();
        //b->SetName("b");
        //pointType.resize(npts);
        //igIndex ids[64]{};
        //for (int i = 0; i < npts; i++) {
        //    int size = mesh->GetPointToNeighborFaces(i, ids);
        //    //double sum = 0;
        //    double minVal = 1e27; 
        //    double maxVal = 0;
        //    for (int j = 0; j < size; j++) { 
        //        //sum += cellScalar[ids[j]];
        //        maxVal = std::max(cellScalar[ids[j]], maxVal);
        //        minVal = std::min(cellScalar[ids[j]], minVal);
        //    }
        //    if (maxVal - minVal < range * 0.03) pointType[i] = 0;
        //    else
        //        pointType[i] = 1;
        //    
        //    //double mean = sum / size;
        //    //double delta = 0;
        //    //for (int j = 0; j < size; j++) { 
        //    //    delta += (cellScalar[ids[j]] - mean) *
        //    //             (cellScalar[ids[j]] - mean);
        //    //}
        //    //delta /= size;
        //    //if (delta < range * 0.05) {

        //    //}
        //    b->AddValue(pointType[i]);
        //}

        //mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, b);
    }

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
        //if (!isCollapsable[edgeId]) return;

        int type = EvaluateEdge(edgeId);

        if ((type == QEM_BOUNDARY_EDGE || type == QEM_HALF_BOUNDARY_EDGE) &&
            PreserveBoundary) {
            return;
        }

        double geo_priority;
        double priority = ComputePriority(edgeId, geo_priority);

        if (priority != std::numeric_limits<double>::max()) {
            heap->push(priority, edgeId, 1, geo_priority);
        }
    }

    double ComputePriority(igIndex edgeId, double& geo_priority) {
        igIndex e[2]{};
        mesh->GetEdgePointIds(edgeId, e);
        Point v0 = mesh->GetPoint(e[0]);
        Point v1 = mesh->GetPoint(e[1]);

        {
            //for (int i = 0; i < 2; i++) {
            //    igIndex ids[64]{};
            //    int size = mesh->GetPointToNeighborEdges(e[i], ids);
            //    for (int j = 0; j < size; j++) {
            //        igIndex ids2[64]{};
            //        int size2 = mesh->GetEdgeToNeighborFaces(ids[j], ids2);
            //        if (size2 == 2) {
            //            Vector3d n1 = Normal(ids2[0]).normalized();
            //            Vector3d n2 = Normal(ids2[1]).normalized();
            //            if (n1 * n2 < cos(M_PI / 4)) {
            //                return std::numeric_limits<double>::max();
            //            }
            //        }
            //    }
            //}
        }

        igIndex ef[8]{};
        int size = mesh->GetEdgeToNeighborFaces(edgeId, ef);
        igIndex fid0 = size > 0 ? ef[0] : -1;
        igIndex fid1 = size > 1 ? ef[1] : -1;

        FlexArray<Vector3d, 128> origNormal;
        SurfaceMesh::ReturnContainer eorf; 
        mesh->GetEdgeToOneRingFaces(edgeId, eorf);
        //origNormal.resize(eorf.size());
        if (this->NormalCheck) {
            for (int i = 0; i < eorf.size(); ++i) {
                if (eorf[i] != fid0 && eorf[i] != fid1) {
                    origNormal.push_back(Normal(eorf[i]).normalized());
                } else {
                    origNormal.push_back(Vector3d());
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

        auto GetCellScalar = [&](igIndex faceId) -> Vector3d {
            int attributes_count = activedAttribIndices.size();
            igIndex f[3]{};
            mesh->GetFacePointIds(faceId, f);
            Vector3d val;
            for (int i = 0; i < attributes_count; i++) {
                val[i] = attributes->GetValue(f[0] * attributes_count + i);
                val[i] += attributes->GetValue(f[1] * attributes_count + i);
                val[i] += attributes->GetValue(f[2] * attributes_count + i);
                val[i] /= 3;
            }

            return val;
        };

        FlexArray<Vector3d, 128> origValue;
        if (this->ScalarCheck) {
            for (int i = 0; i < eorf.size(); ++i) {
                if (eorf[i] != fid0 && eorf[i] != fid1) {
                    origValue.push_back(GetCellScalar(eorf[i]));
                } else {
                    origValue.push_back(Vector3d());
                }
            }
        }

        //double gradient = 0;
        //if (this->ScalarCheck) { 
        //    gradient += this->GetMeanValue(e[0]);
        //    gradient += this->GetMeanValue(e[1]);
        //    
        //    //if (gradient < this->DeltaThr * mean)
        //        /*return std::numeric_limits<double>::max();*/
        //        //gradient = this->DeltaThr * mean;
        //}

        optimalPos[edgeId] = ComputePosition(edgeId);
        mesh->SetPoint(e[0], optimalPos[edgeId]);
        mesh->SetPoint(e[1], optimalPos[edgeId]);

        std::vector<Vector2d> attrbOrigin;
        {
            int attributes_count = activedAttribIndices.size();
            attrbOrigin.resize(attributes_count);

            for (int i = 0; i < attributes_count; i++) {
                attrbOrigin[i][0] = attributes->GetValue(e[0] * attributes_count + i);
                attrbOrigin[i][1] = attributes->GetValue(e[1] * attributes_count + i);
            }
        }
        
        if (this->ScalarCheck) {
            SurfaceMesh::ReturnContainer container;
            double minD = std::numeric_limits<double>::max();
            igIndex fid = -1;
            Vector3d p;
            for (int i = 0; i < 2; ++i) {
                mesh->GetPointToNeighborFaces(e[i], container);
                for (int j = 0; j < container.size(); j++) {
                    double d;
                    Vector3d proj;
                    if (Projection(optimalPos[edgeId], container[j], d, proj)) {
                        if (d < minD) {
                            minD = d;
                            fid = container[j];
                            p = proj;
                        }
                    }
                }
            }

            //auto arr = attrbs->GetAttribute(scalarIndex).pointer;
            //int dim = arr->GetDimension();
            if (fid != -1) {
                auto param = GetCentroidParam(p, fid);
                igIndex f[3]{};
                mesh->GetFacePointIds(fid, f);

                int attributes_count = activedAttribIndices.size();
                for (int i = 0; i < attributes_count; i++) {
                    double newValue =
                            param[0] * attributes->GetValue(f[0] * attributes_count + i) +
                            param[1] * attributes->GetValue(f[1] * attributes_count + i) +
                            param[2] * attributes->GetValue(f[2] * attributes_count + i);
                    attributes->SetValue(e[1] * attributes_count + i, newValue);
                    optimalScalar[edgeId][i] = newValue;
                }

            } else {
                int attributes_count = activedAttribIndices.size();
                for (int i = 0; i < attributes_count; i++) {
                    double newValue =
                            attributes->GetValue(e[0] * attributes_count + i) +
                            attributes->GetValue(e[1] * attributes_count + i);
                    newValue /= 2;
                    attributes->SetValue(e[1] * attributes_count + i, newValue);
                    optimalScalar[edgeId][i] = newValue;
                }
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

        double scalar = 0;
        if (this->ScalarCheck) {
            for (int i = 0; i < eorf.size(); ++i) {
                if (eorf[i] != fid0 && eorf[i] != fid1) {
                    int attributes_count = activedAttribIndices.size();
                    auto newScalar = GetCellScalar(eorf[i]);
                    for (int j = 0; j < attributes_count; j++) {
                        scalar += std::abs(newScalar[j] - origValue[i][j]);
                    }
                } 
            }
        }

        {
            int attributes_count = activedAttribIndices.size();

            for (int i = 0; i < attributes_count; i++) {
                attributes->SetValue(e[0] * attributes_count + i,
                                     attrbOrigin[i][0]);
                attributes->SetValue(e[1] * attributes_count + i,
                                     attrbOrigin[i][1]);
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
            //if (minCos < cos(M_PI / 4.0)) 
                //return std::numeric_limits<double>::max();
            //else if (minCos < this->CosineThr)
                //minCos *= 0.8;
            
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
        if (this->ScalarCheck) priority = priority * (1 + scalar);

        //int d = 2;
        //if (minCos < 1.0 / d) {
        //    return std::numeric_limits<double>::max();
        //}

        //geo_priority = gradient;
        //if (this->ScalarCheck) { 
        //    priority *= (1 + gradient);
        //}

        
        return priority;
    }
    
    double GetMeanValue(igIndex ptId) {
        double value = 0;
        igIndex ids[256]{};
        int size = mesh->GetPointToOneRingPoints(ptId, ids);
        double p = attrbs->GetAttribute(scalarIndex).pointer->GetValue(ptId);
        for (int j = 0; j < size; j++) {
            value += std::abs(p - attrbs->GetAttribute(scalarIndex)
                                          .pointer->GetValue(ids[j]));
        }

        value /= size;
        return value;
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

    bool IsInTriangle(const Point& p, const Point& a, const Point& b,
        const Point& c) {

        Vector3d ab = b - a;
        Vector3d bc = c - b;
        Vector3d ca = a - c;

        Vector3d ap = p - a;
        Vector3d bp = p - b;
        Vector3d cp = p - c;
  
        Vector3d cross1 = ab.cross(ap); // Cross product of (b-a) and (p-a)
        Vector3d cross2 = bc.cross(bp); // Cross product of (c-b) and (p-b)
        Vector3d cross3 = ca.cross(cp); // Cross product of (a-c) and (p-c)

        bool sameSign1 = cross1.dot(cross2) > 0;
        bool sameSign2 = cross2.dot(cross3) > 0;
        bool sameSign3 = cross3.dot(cross1) > 0;

        return sameSign1 && sameSign2 && sameSign3;
    }

    bool Projection(const Point& p, igIndex faceId, double& d, Vector3d& proj) { 
        igIndex f[3]{};
        mesh->GetFacePointIds(faceId, f);
        Point v0 = mesh->GetPoint(f[0]);
        Point v1 = mesh->GetPoint(f[1]);
        Point v2 = mesh->GetPoint(f[2]);

        Vector3d d10 = v1 - v0;
        Vector3d d20 = v2 - v0;

        Vector3d normal = CrossProduct(d10, d20);
        normal.normalize();

        d = (p - v0) * normal;
        proj = p - d * normal;

        return IsInTriangle(proj, v0, v1, v2);
    }

    Vector3d GetCentroidParam(const Point& p, igIndex faceId) {
        igIndex f[3]{};
        mesh->GetFacePointIds(faceId, f);
        Point a = mesh->GetPoint(f[0]);
        Point b = mesh->GetPoint(f[1]);
        Point c = mesh->GetPoint(f[2]);

        // 计算向量
        Vector3d ab = b - a;
        Vector3d ac = c - a;
        Vector3d ap = p - a;
        Vector3d bp = p - b;
        Vector3d cp = p - c;

        // 计算叉积
        Vector3d crossAB_AC = ab.cross(ac); 
        Vector3d crossBP_CP = bp.cross(cp); 
        Vector3d crossCP_AP = cp.cross(ap); 
        Vector3d crossAP_BP = ap.cross(bp); 

        // 计算每个重心坐标
        double lambda1 =
                crossBP_CP.dot(crossAB_AC) / crossAB_AC.squaredNorm();
        double lambda2 =
                crossCP_AP.dot(crossAB_AC) / crossAB_AC.squaredNorm();
        double lambda3 =
                crossAP_BP.dot(crossAB_AC) / crossAB_AC.squaredNorm();

        return Vector3d(lambda1, lambda2, lambda3);
        
    }

    bool GetNormalAndArea(igIndex faceId, double& area, Vector3d& n) {
        igIndex f[3]{};
        mesh->GetFacePointIds(faceId, f);
        Point v0 = mesh->GetPoint(f[0]);
        Point v1 = mesh->GetPoint(f[1]);
        Point v2 = mesh->GetPoint(f[2]);

        Vector3f d10 = v1 - v0;
        Vector3f d20 = v2 - v0;

        n = CrossProduct(d10, d20);
        if (n.squaredNorm() == 0.0) return false;
        area = n.norm() / 2;

        return true;
    }

    double GetCosTheta(Vector3d n1, Vector3d n2) {
        double dotProduct = n1.dot(n2);
        double lengths = n1.length() * n2.length();
        if (lengths == 0) return 0.0;
        double cosTheta = dotProduct / lengths;
        cosTheta = std::max(
                -1.0, std::min(1.0, cosTheta)); // 限制cosTheta在[-1, 1]范围内
        return std::acos(cosTheta); // 返回夹角，单位为弧度
    }

	SurfaceMesh::Pointer mesh{};
    PriorityQueue::Pointer heap{};
    std::vector<Quadric<double>> quadrics;
    std::vector<int> faceType;
    std::vector<int> pointType;
    std::vector<Vector3f> optimalPos;
    std::vector<Vector3d> optimalScalar;
    //FloatArray::Pointer optimalScalar{};
    IGenum mode{QEM_FASTEST};
    AttributeSet::Pointer newAttrs{}, oldAttrs{}, attrbs{};
    int scalarIndex{-1};
    std::vector<Vector2d> scalarRange;
    std::vector<int> mergeCount;
    std::vector<int> isCollapsable;
    std::vector<double> cellScalar;
    double range;
    double mean;
    Painter3D::Pointer painter{nullptr};

    FloatArray::Pointer attributes{};

    IGsize npts{}, nedges{}, nfaces{};

};
IGAME_NAMESPACE_END
#endif