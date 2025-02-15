#include "iGameSimplification.h"
#include "iGamePointFinder.h"
IGAME_NAMESPACE_BEGIN

bool Simplification::Execute() {

    mesh = DynamicCast<SurfaceMesh>(GetInput(0));
    if (mesh == nullptr) {
        if (DynamicCast<UnstructuredMesh>(GetInput(0))) {
            mesh = DynamicCast<UnstructuredMesh>(GetInput(0))->TransferToSurfaceMesh();
        }
    }
    if (mesh == nullptr) { return false; }

    if (TargetReduction < 0 || TargetReduction > 1) { return false; }
    
    {
        igIndex face[16]{};
        for (int i = 0; i < mesh->GetNumberOfFaces(); i++) { 
            int size = mesh->GetFacePointIds(i, face);
            if (size != 3) { 
                return false;
            }
        }
    }

    if (this->IsAllScalarCheck) {
        activedAttribIndices.clear();
        activedAttribIndices.resize(mesh->GetAttributeSet()->GetNumberOfAttributes());
        for (int i = 0; i < activedAttribIndices.size(); i++) { activedAttribIndices[i] = i; }
    }

    auto newMesh = SurfaceMesh::New();
    newMesh->DeepCopy(mesh);
    newMesh->SetName(mesh->GetName() + "_new");
    auto oldMesh = mesh;
    mesh = newMesh;
    SetOutput(newMesh);

    auto oldPoints = oldMesh->GetPoints();
    PointFinder::Pointer oldPicker = PointFinder::New();
    PointFinder::Pointer newPicker = PointFinder::New();

    oldPicker->SetPoints(oldPoints);
    oldPicker->Initialize();
    double w1 = 0.0, w2 = 0.0;
    {
        // 计算原始网格的表面积
        igIndex face[16]{};
        for (int i = 0; i < oldMesh->GetNumberOfFaces(); i++) { 
            w1 += Normal(i).norm() / 2.0;
        }
    }

    // 初始化
    Initialize();
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
        double geo_pri = heap->top().rest;
        int count = heap->top().count;

        heap->pop();

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
                for (int j = 0; j < container.size(); j++) { heap->remove(container[j]); }
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
            for (int attrId = 0; attrId < attributes_count; attrId++) {
                auto& info = attributes[attrId];
                if (info.type == IG_SCALAR) {
                    info.ptr->SetValue(size_t(e[1]) * info.dimension + info.offset,
                                       info.optimalAttrib->GetValue(edgeId));
                } else {
                    double element[16]{};
                    info.optimalAttrib->GetElement(edgeId, element);
                    info.ptr->SetElement(e[1], element);
                }
            }
        }

        if (mode == QEM_FASTEST) {
            mesh->GetPointToNeighborEdges(e[1], container);
            for (int i = 0; i < container.size(); i++) { this->InsertEdgeToHeap(container[i]); }
        } else if (mode == QEM_NICEST) {
            FlexArray<igIndex> edgeIds;
            mesh->GetPointToNeighborEdges(e[1], container);
            for (int i = 0; i < container.size(); i++) { edgeIds.push_back(container[i]); }
            mesh->GetPointToNeighborFaces(e[1], container);
            for (int i = 0; i < container.size(); i++) { edgeIds.push_back(GetOppEdge(e[1], container[i])); }

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

            for (int i = 0; i < edgeIds.size(); i++) { this->InsertEdgeToHeap(edgeIds[i]); }
        }
    }

    std::cout << "Before: \n"
              << "The Number of Point: " << mesh->GetNumberOfPoints() << " ;The Number of Face: " << mesh->GetNumberOfFaces()
              << std::endl;

    
    AttributeSet::Pointer newAttrs = AttributeSet::New();
    if (this->ScalarCheck) {
        for (int attrId = 0; attrId < attributes_count; attrId++) {
            auto& info = attributes[attrId];
            double element[16]{};
            int k = 0;
            for (int i = 0; i < info.size; i++) {
                if ((info.attach == IG_POINT && mesh->IsPointDeleted(i)) ||
                    (info.attach == IG_CELL && mesh->IsFaceDeleted(i)))
                    continue;

                if (info.type == IG_SCALAR) {
                    double val = info.ptr->GetValue(size_t(i) * info.dimension + info.offset);

                    info.ptr->SetValue(size_t(k) * info.dimension + info.offset, val);

                } else {
                    info.ptr->GetElement(i, element);
                    info.ptr->SetElement(k, element);
                }
                k++;
            }
            if (info.hasNext == false) { 
                info.ptr->Resize(k);
                newAttrs->AddAttribute(info.type, info.attach, info.ptr);
            }
        }
    }

    mesh->SetAttributeSet(newAttrs);
    mesh->GarbageCollection();
    std::cout << "After: \n"
              << "The Number of Point: " << mesh->GetNumberOfPoints() << " ;The Number of Face: " << mesh->GetNumberOfFaces()
              << std::endl;

    auto newPoints = mesh->GetPoints();
    newPicker->SetPoints(newPoints);
    newPicker->Initialize();

    if(false){
        double d1 = 0.0, d2 = 0.0;
        double d3 = 0.0, d4 = 0.0;
        for (int i = 0; i < mesh->GetNumberOfFaces(); i++) { 
            w2 += Normal(i).norm() / 2.0; 
        }

        // 计算平均平方距离
        for (int i = 0; i < oldPoints->GetNumberOfPoints(); i++) { 
            auto p = oldPoints->GetPoint(i);

            igIndex id = newPicker->FindClosestPoint(p);
            if (id != -1) { 
                Point cp = newPoints->GetPoint(id);
                d1 += (p - cp).squaredNorm();
                d3 += (p - cp).norm();
            }
        }

        for (int i = 0; i < newPoints->GetNumberOfPoints(); i++) {
            auto p = newPoints->GetPoint(i);

            igIndex id = oldPicker->FindClosestPoint(p);
            if (id != -1) {
                Point cp = oldPoints->GetPoint(id);
                d2 += (p - cp).squaredNorm();
                d4 += (p - cp).norm();
            }
        }

        double d = 1.0 / w1 * d1 + 1.0 / w2 * d2;
        double dd = 1.0 / oldPoints->GetNumberOfPoints() * d3 + 1.0 / newPoints->GetNumberOfPoints() * d4;
        std::cout << "Squared Mean Distance: " << d << "\n" 
                  << "Mean Distance: " << dd
                  << std::endl;
    }

    return true;
}

Simplification::Simplification() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

void Simplification::Initialize() {

    mesh->RequestEditStatus();
    npts = mesh->GetNumberOfPoints();
    nedges = mesh->GetNumberOfEdges();
    nfaces = mesh->GetNumberOfFaces();

    // 初始化算法所需的内存空间
    InitMemory();

    InitAttributes();

    //{
    //    double T = 0;
    //    igIndex ids[8]{};
    //    int count = 0;
    //    for (int i = 0; i < nedges; i++) {
    //        int size = mesh->GetEdgeToNeighborFaces(i, ids);
    //        if (size == 2) {
    //            Vector3d n1 = Normal(ids[0]);
    //            Vector3d n2 = Normal(ids[1]);

    //            double theta = GetCosTheta(n1, n2);
    //            T += theta * theta;
    //            count++;
    //        }
    //    }
    //    T /= count;
    //    T = std::sqrt(T);
    //    double Smax = 0.9;
    //    double al = 1;
    //    double S = Smax * std::exp(-al * T);
    //    //S = 0.1;
    //    this->TargetReduction = S;
    //    //std::cout << T << " " << S << std::endl;
    //}

    if (this->TargetReduction != 0) { this->TargetFaceNum = nfaces * (1 - this->TargetReduction); }
}

void Simplification::InitMemory() {
    heap = PriorityQueue::New();
    optimalPos.resize(nedges);
    optimalScalar.resize(nedges);
    origValue = DoubleArray::New();
}

void Simplification::InitAttributes() {

    attrbs = mesh->GetAttributeSet();
    if (attrbs == nullptr) {
        this->ScalarCheck = false;
        return;
    }

    auto& box = mesh->GetBoundingBox();

    for (int i = 0; i < activedAttribIndices.size(); i++) {
        int index = activedAttribIndices[i];
        auto& attrb = attrbs->GetAttribute(index);
        if (attrb.isDeleted || attrb.IsNone()) continue;

        if (attrb.attachmentType == IG_CELL) continue;

        if (attrb.type == IG_SCALAR) {
            for (int d = 0; d < attrb.pointer->GetDimension(); d++) {
                AttribInfo info;
                info.mapId = index;
                info.type = attrb.type;
                info.attach = attrb.attachmentType;
                info.ptr = attrb.pointer;
                info.dimension = info.ptr->GetDimension();
                info.offset = d;

                double minVal = 1e27;
                double maxVal = 0;

                if (attrb.attachmentType == IG_POINT) info.size = npts;
                else if (attrb.attachmentType == IG_CELL)
                    info.size = nfaces;

                for (int j = 0; j < info.size; j++) {
                    double val = info.ptr->GetValue(size_t(j) * info.dimension + info.offset);

                    maxVal = std::max(val, maxVal);
                    minVal = std::min(val, minVal);
                }

                info.magMin = minVal;
                info.magMax = maxVal;
                double range = maxVal - minVal;
                if (range == 0.0) continue;
                info.weights = box.diag() / range * 10;
                info.optimalAttrib = FloatArray::New();
                info.optimalAttrib->Resize(nedges);
                if (d == info.dimension - 1) info.hasNext = false;
                else
                    info.hasNext = true;
                attributes.push_back(info);
            }
        } else {
            AttribInfo info;
            info.mapId = index;
            info.type = attrb.type;
            info.attach = attrb.attachmentType;
            info.ptr = attrb.pointer;
            info.dimension = info.ptr->GetDimension();
            info.offset = -1;

            if (attrb.attachmentType == IG_POINT) info.size = npts;
            else if (attrb.attachmentType == IG_CELL)
                info.size = nfaces;

            double minVal = 1e27;
            double maxVal = 0;

            double element[16]{};
            for (int j = 0; j < info.size; j++) {
                info.ptr->GetElement(j, element);
                double magVal = 0;
                for (int k = 0; k < info.dimension; k++) { magVal += element[k] * element[k]; }
                magVal = std::sqrt(magVal);

                maxVal = std::max(magVal, maxVal);
                minVal = std::min(magVal, minVal);
            }

            info.magMin = minVal;
            info.magMax = maxVal;
            double range = maxVal - minVal;
            if (range == 0.0) continue;

            info.weights = box.diag() / range * 10;
            info.optimalAttrib = FloatArray::New();
            info.optimalAttrib->SetDimension(info.dimension);
            info.optimalAttrib->Resize(nedges);
            info.hasNext = false;
            attributes.push_back(info);
        }
    }
    attributes_count = attributes.size();
    if (attributes_count == 0) {
        this->ScalarCheck = false;
        return;
    }
    for (int i = 0; i < attributes_count; i++) {
        attributes[i].weights /= attributes_count;
        //std::cout << attributes[i].weights << std::endl;
    }
    origValue->SetDimension(attributes_count);
    origValue->Resize(256);
}

void Simplification::InitQuadric() {
    quadrics.resize(npts);
    for (int i = 0; i < npts; i++) { quadrics[i].setZero(); }

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

int Simplification::EvaluateEdge(igIndex edgeId) {
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

void Simplification::InsertEdgeToHeap(igIndex edgeId) {
    //if (!isCollapsable[edgeId]) return;

    int type = EvaluateEdge(edgeId);

    if ((type == QEM_BOUNDARY_EDGE || type == QEM_HALF_BOUNDARY_EDGE) && PreserveBoundary) { return; }

    double geo_priority;
    double priority = ComputePriority(edgeId, geo_priority);

    if (priority != std::numeric_limits<double>::max()) { heap->push(priority, edgeId, 1, geo_priority); }
}

double Simplification::ComputePriority(igIndex edgeId, double& geo_priority) {
    igIndex e[2]{};
    mesh->GetEdgePointIds(edgeId, e);
    Point v0 = mesh->GetPoint(e[0]);
    Point v1 = mesh->GetPoint(e[1]);

    igIndex ef[8]{};
    int size = mesh->GetEdgeToNeighborFaces(edgeId, ef);
    igIndex fid0 = size > 0 ? ef[0] : -1;
    igIndex fid1 = size > 1 ? ef[1] : -1;

    FlexArray<Vector3d, 256> origNormal;
    SurfaceMesh::ReturnContainer eorf;
    mesh->GetEdgeToOneRingFaces(edgeId, eorf);
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
            if (eorf[i] != fid0 && eorf[i] != fid1) { origQuality = std::min(origQuality, QualityFace(eorf[i])); }
        }
    }

    auto GetCellScalar = [&](igIndex faceId, double val[]) -> void {
        igIndex f[3]{};
        mesh->GetFacePointIds(faceId, f);

        double element[16]{};
        for (int attrId = 0; attrId < attributes_count; attrId++) {
            auto& info = attributes[attrId];

            if (info.attach == IG_POINT) {
                if (info.type == IG_SCALAR) {
                    val[attrId] = info.ptr->GetValue(size_t(f[0]) * info.dimension + info.offset) * info.weights;
                    val[attrId] += info.ptr->GetValue(size_t(f[1]) * info.dimension + info.offset) * info.weights;
                    val[attrId] += info.ptr->GetValue(size_t(f[2]) * info.dimension + info.offset) * info.weights;
                    val[attrId] /= 3;
                } else {
                    val[attrId] = 0;
                    double sum[16]{0.0};
                    info.ptr->GetElement(f[0], sum);
                    info.ptr->GetElement(f[1], element);
                    for (int k = 0; k < info.dimension; k++) sum[k] += element[k] * info.weights;
                    info.ptr->GetElement(f[2], element);
                    for (int k = 0; k < info.dimension; k++) {
                        sum[k] += element[k] * info.weights;
                        sum[k] /= 3;
                        val[attrId] += sum[k] * sum[k];
                    }
                    val[attrId] = std::sqrt(val[attrId]);
                }
            } else if (info.attach == IG_CELL) {
                if (info.type == IG_SCALAR) {
                    val[attrId] = info.ptr->GetValue(faceId) * info.weights;
                } else {
                    val[attrId] = 0;
                    double sum[16]{0.0};
                    info.ptr->GetElement(faceId, element);
                    for (int k = 0; k < info.dimension; k++) {
                        val[attrId] += element[k] * element[k] * info.weights * info.weights;
                    }
                    val[attrId] = std::sqrt(val[attrId]);
                }
            }
        }
    };

    if (this->ScalarCheck) {
        double val[64]{};
        if (eorf.size() > 256) std::cout << eorf.size() << std::endl;
        for (int i = 0; i < eorf.size(); ++i) {
            if (eorf[i] != fid0 && eorf[i] != fid1) {
                GetCellScalar(eorf[i], val);
                origValue->SetElement(i, val);
            }
        }
    }

    optimalPos[edgeId] = ComputePosition(edgeId);
    mesh->SetPoint(e[0], optimalPos[edgeId]);
    mesh->SetPoint(e[1], optimalPos[edgeId]);

    if (this->ScalarCheck) {
        for (int attrId = 0; attrId < attributes_count; attrId++) {
            auto& info = attributes[attrId];

            if (info.attach == IG_POINT) {
                if (info.type == IG_SCALAR) {
                    info.tempOriValue1[0] = info.ptr->GetValue(size_t(e[0]) * info.dimension + info.offset);

                    info.tempOriValue2[0] = info.ptr->GetValue(size_t(e[1]) * info.dimension + info.offset);
                } else {
                    double element[16]{};
                    info.ptr->GetElement(e[0], element);
                    for (int k = 0; k < info.dimension; k++) info.tempOriValue1[k] = element[k];

                    info.ptr->GetElement(e[1], element);
                    for (int k = 0; k < info.dimension; k++) info.tempOriValue2[k] = element[k];
                }
            }
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

        if (fid != -1) {
            auto param = GetCentroidParam(p, fid);
            igIndex f[3]{};
            mesh->GetFacePointIds(fid, f);

            for (int attrId = 0; attrId < attributes_count; attrId++) {
                auto& info = attributes[attrId];
                if (info.attach == IG_POINT) {
                    if (info.type == IG_SCALAR) {
                        double newValue = param[0] * info.ptr->GetValue(size_t(f[0]) * info.dimension + info.offset) +
                                          param[1] * info.ptr->GetValue(size_t(f[1]) * info.dimension + info.offset) +
                                          param[2] * info.ptr->GetValue(size_t(f[2]) * info.dimension + info.offset);
                        info.ptr->SetValue(size_t(e[1]) * info.dimension + info.offset, newValue);
                        info.optimalAttrib->SetValue(edgeId, newValue);
                    } else {
                        double sum[16]{0.0};
                        double element[16]{};
                        info.ptr->GetElement(f[0], element);
                        for (int k = 0; k < info.dimension; k++) sum[k] += param[0] * element[k];

                        info.ptr->GetElement(f[1], element);
                        for (int k = 0; k < info.dimension; k++) sum[k] += param[1] * element[k];

                        info.ptr->GetElement(f[2], element);
                        for (int k = 0; k < info.dimension; k++) sum[k] += param[2] * element[k];

                        info.ptr->SetElement(e[1], sum);
                        info.optimalAttrib->SetElement(edgeId, sum);
                    }
                }
            }

        } else {
            for (int attrId = 0; attrId < attributes_count; attrId++) {
                auto& info = attributes[attrId];

                if (info.attach == IG_POINT) {
                    if (info.type == IG_SCALAR) {
                        double newValue = info.ptr->GetValue(size_t(e[0]) * info.dimension + info.offset) +
                                          info.ptr->GetValue(size_t(e[1]) * info.dimension + info.offset);
                        newValue /= 2;
                        info.ptr->SetValue(size_t(e[1]) * info.dimension + info.offset, newValue);
                        info.optimalAttrib->SetValue(edgeId, newValue);
                    } else {
                        double sum[16]{0.0};
                        double element[16]{};
                        info.ptr->GetElement(e[0], element);
                        for (int k = 0; k < info.dimension; k++) sum[k] += element[k] * 0.5;

                        info.ptr->GetElement(e[1], element);
                        for (int k = 0; k < info.dimension; k++) sum[k] += element[k] * 0.5;

                        info.ptr->SetElement(e[1], sum);
                        info.optimalAttrib->SetElement(edgeId, sum);
                    }
                }
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
            if (eorf[i] != fid0 && eorf[i] != fid1) { newQuality = std::min(newQuality, QualityFace(eorf[i])); }
        }
    }

    double scalar = 0;
    if (this->ScalarCheck) {
        double newVal[64]{};
        for (int i = 0; i < eorf.size(); ++i) {
            if (eorf[i] != fid0 && eorf[i] != fid1) {
                GetCellScalar(eorf[i], newVal);
                for (int j = 0; j < attributes_count; j++) { scalar += std::abs(newVal[j] - origValue->GetValue(j)); }
            }
        }
    }

    if (this->ScalarCheck) {
        for (int attrId = 0; attrId < attributes_count; attrId++) {
            auto& info = attributes[attrId];

            if (info.attach == IG_POINT) {
                if (info.type == IG_SCALAR) {
                    info.ptr->SetValue(size_t(e[0]) * info.dimension + info.offset, info.tempOriValue1[0]);
                    info.ptr->SetValue(size_t(e[1]) * info.dimension + info.offset, info.tempOriValue2[0]);

                } else {
                    double element[16]{};
                    for (int k = 0; k < info.dimension; k++) element[k] = info.tempOriValue1[k];
                    info.ptr->SetElement(e[0], element);

                    for (int k = 0; k < info.dimension; k++) element[k] = info.tempOriValue2[k];
                    info.ptr->SetElement(e[1], element);
                }
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
    if (QualityCheck && newQuality < 0.1 && newQuality < origQuality) { return priority; }

    if (!this->QualityCheck && !this->NormalCheck) priority = error;
    if (this->QualityCheck && !this->NormalCheck) priority = error / newQuality;
    if (!this->QualityCheck && this->NormalCheck) priority = error / minCos;
    if (this->QualityCheck && this->NormalCheck) priority = error / (newQuality * minCos);
    if (this->ScalarCheck) priority = priority * (1 + scalar);

    return priority;
}

Vector3f Simplification::ComputePosition(igIndex edgeId) {
    igIndex e[2]{};
    mesh->GetEdgePointIds(edgeId, e);
    Vector3f newPos = (mesh->GetPoint(e[0]) + mesh->GetPoint(e[1])) / 2.0;
    if (this->OptimalPosition) {
        Quadric q = quadrics[e[0]];
        q += quadrics[e[1]];
        if (q.apply(newPos) > this->QuadricEpsilon) { q.findMinimum(newPos); }
    }
    return newPos;
}

Vector3f Simplification::Normal(igIndex faceId) {
    igIndex f[3]{};
    mesh->GetFacePointIds(faceId, f);
    Point v0 = mesh->GetPoint(f[0]);
    Point v1 = mesh->GetPoint(f[1]);
    Point v2 = mesh->GetPoint(f[2]);

    Vector3f d10 = v1 - v0;
    Vector3f d20 = v2 - v0;

    return CrossProduct(d10, d20);
}

Quadric<double> Simplification::QuadricFace(igIndex faceId) {
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

double Simplification::QualityFace(igIndex faceId) {
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
    double b = std::max(d10.squaredNorm(), std::max(d20.squaredNorm(), d12.squaredNorm()));
    if (b == 0) return 0;
    return a / b;
}

igIndex Simplification::GetOppEdge(igIndex ptId, igIndex faceId) {
    igIndex f[3]{}, fe[3]{};
    mesh->GetFacePointIds(faceId, f);
    mesh->GetFaceEdgeIds(faceId, fe);
    int i = 0;
    while (f[i] != ptId) i++;
    return fe[(i + 1) % 3];
}

void Simplification::GetEdgeToOneRingPoints(igIndex edgeId, SurfaceMesh::ReturnContainer& ptIds) {

    igIndex e[2]{};
    mesh->GetEdgePointIds(edgeId, e);
    std::set<igIndex> st;
    for (int i = 0; i < 2; i++) {
        mesh->GetPointToOneRingPoints(e[i], ptIds);
        for (int j = 0; j < ptIds.size(); j++) {
            if (ptIds[j] != e[1 - i]) { st.insert(ptIds[j]); }
        }
    }
    ptIds.reset();
    for (auto id: st) { ptIds.push_back(id); }
}

bool Simplification::IsInTriangle(const Point& p, const Point& a, const Point& b, const Point& c) {

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

bool Simplification::Projection(const Point& p, igIndex faceId, double& d, Vector3d& proj) {
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

Vector3d Simplification::GetCentroidParam(const Point& p, igIndex faceId) {
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
    double lambda1 = crossBP_CP.dot(crossAB_AC) / crossAB_AC.squaredNorm();
    double lambda2 = crossCP_AP.dot(crossAB_AC) / crossAB_AC.squaredNorm();
    double lambda3 = crossAP_BP.dot(crossAB_AC) / crossAB_AC.squaredNorm();

    return Vector3d(lambda1, lambda2, lambda3);
}

bool Simplification::GetNormalAndArea(igIndex faceId, double& area, Vector3d& n) {
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

double Simplification::GetCosTheta(Vector3d n1, Vector3d n2) {
    double dotProduct = n1.dot(n2);
    double lengths = n1.length() * n2.length();
    if (lengths == 0) return 0.0;
    double cosTheta = dotProduct / lengths;
    cosTheta = std::max(-1.0, std::min(1.0, cosTheta)); // 限制cosTheta在[-1, 1]范围内
    return std::acos(cosTheta);                         // 返回夹角，单位为弧度
}

IGAME_NAMESPACE_END