#include "iGameMeshTriangulationFilter.h"

#include <queue>

IGAME_NAMESPACE_BEGIN

class RemovePriorityQueue {
private:
    // 定义队列中元素的类型：pair<double, int>
    using ElementType = std::pair<double, int>;

    // 小根堆
    std::priority_queue<ElementType, std::vector<ElementType>, std::greater<ElementType>> main_pq;

    // 记录每个id的最新信息：<最新优先级, 是否有效>
    std::unordered_map<int, std::pair<double, bool>> latest_info;

public:
    // 插入或更新元素
    void push(double priority, int id) {
        // 更新该ID的最新信息：优先级为priority，状态为有效(true)
        latest_info[id] = {priority, true};
        // 将新元素加入主队列
        main_pq.push({priority, id});
    }

    // 标记删除指定ID的元素
    void remove(int id) {
        // 将该ID标记为无效。如果ID不存在，则插入一个无效记录。
        // 注意：这里存储的优先级值不会被用到，可以设为任意值（如0.0）
        latest_info[id] = {0.0, false};
    }

    // 获取当前最高优先级的有效元素
    ElementType top() {
        cleanup();
        if (main_pq.empty()) { throw std::runtime_error("Priority queue is empty"); }
        return main_pq.top();
    }

    // 弹出当前最高优先级的有效元素
    void pop() {
        cleanup();
        if (main_pq.empty()) { throw std::runtime_error("Priority queue is empty"); }
        main_pq.pop();
    }

    // 检查队列是否为空（指不存在有效元素）
    bool empty() {
        cleanup();
        return main_pq.empty();
    }

    // 获取有效元素的大致数量（注意：由于惰性删除，这可能不精确）
    size_t size() {
        cleanup();
        // 此大小可能包含一些尚未被cleanup处理掉的无效元素
        return main_pq.size();
    }

private:
    // 核心：清理堆顶的无效元素，直到堆顶元素是当前最新且有效的
    void cleanup() {
        while (!main_pq.empty()) {
            ElementType top_elem = main_pq.top();
            double value = top_elem.first;
            int id = top_elem.second;

            // 查找该ID的最新信息
            auto it = latest_info.find(id);

            // 判断该堆顶元素是否有效的条件：
            // 1. 在latest_info中有记录
            // 2. 该记录标记为有效 (true)
            // 3. 记录中的优先级与堆顶元素的优先级一致（确保是“最新”的版本）
            if (it != latest_info.end() && it->second.second && it->second.first == value) {
                // 堆顶元素是有效且最新的，停止清理
                break;
            }
            // 否则，这个堆顶条目是无效的或过时的，将其弹出
            main_pq.pop();
        }
    }
};

 struct LocalPolyVertex {
    int id;
    Vector3d x;
    double measure;
    LocalPolyVertex* next;
    LocalPolyVertex* previous;
};

class PolyVertexList {
public:
    PolyVertexList(IdArray::Pointer ptIds, Points::Pointer pts, double tol2, int measure);
    ~PolyVertexList();

    bool ComputeNormal();
    double ComputeMeasure(LocalPolyVertex* vtx);
    void RemoveVertex(LocalPolyVertex* vtx, IdArray::Pointer ids, RemovePriorityQueue* queue = nullptr);
    void RemoveVertex(int i, IdArray::Pointer ids, RemovePriorityQueue* queue = nullptr);
    int CanRemoveVertex(LocalPolyVertex* vtx);
    int CanRemoveVertex(int id);

    double Tol;
    double Tol2;
    int Measure;

    int NumberOfVerts;
    LocalPolyVertex* Array;
    LocalPolyVertex* Head;
    Vector3d Normal;
};

PolyVertexList::PolyVertexList(IdArray::Pointer ptIds, Points::Pointer pts, double tol2, int measure) {
    this->Tol2 = tol2;
    this->Tol = (tol2 > 0.0 ? sqrt(tol2) : 0.0);
    this->Measure = measure;

    int numVerts = ptIds->GetNumberOfIds();
    this->NumberOfVerts = numVerts;
    this->Array = new LocalPolyVertex[numVerts];
    int i;

    // Load the data into the array.
    for (i = 0; i < numVerts; i++) {
        this->Array[i].id = i;
        pts->GetPoint(i, this->Array[i].x);
        this->Array[i].next = (i == (numVerts - 1) ? this->Array : this->Array + i + 1);
        this->Array[i].previous = (i == 0 ? this->Array + numVerts - 1 : this->Array + i - 1);
    }

    // Make sure that there are no coincident vertices.
    // Beware of multiple coincident vertices.
    LocalPolyVertex *vtx, *next;
    this->Head = this->Array;

    for (vtx = this->Head, i = 0; i < numVerts; i++) {
        next = vtx->next;
        if (vtx->x.distance2(next->x) < tol2) {
            next->next->previous = vtx;
            vtx->next = next->next;
            if (next == this->Head) { this->Head = vtx; }
            this->NumberOfVerts--;
        } 
        else // can move forward
        {
            vtx = next;
        }
    }
}

PolyVertexList::~PolyVertexList() { delete[] this->Array; }

bool PolyVertexList::ComputeNormal() {
    LocalPolyVertex* vtx = this->Head;
    Vector3d v1, v2, n, anchor = vtx->x;

    this->Normal[0] = this->Normal[1] = this->Normal[2] = 0.0;
    for (vtx = vtx->next; vtx->next != this->Head; vtx = vtx->next) {
        v1 = vtx->x - anchor;
        v2 = vtx->next->x - anchor;
        n = v1.cross(v2);
        this->Normal += n;
    }
    if (this->Normal.normalize() == 0.0) {
        return false;
    } else {
        return true;
    }
}
enum EarCutMeasureTypes { 
    PERIMETER2_TO_AREA_RATIO = 0,
    DOT_PRODUCT = 1, 
    BEST_QUALITY = 2 
};

static double DistanceToLine(const Vector3d x, const Vector3d p1, const Vector3d p2) {
    int i;
    Vector3d np1, p1p2;
    double proj, den;

    np1 = x - p1;
    p1p2 = p1 - p2;

    if ((den = p1p2.norm()) != 0.0) {
        for (i = 0; i < 3; i++) { p1p2[i] /= den; }
    } else {
        return DotProduct(np1, np1);
    }

    proj = DotProduct(np1, p1p2);

    return (DotProduct(np1, np1) - proj * proj);
}

double PolyVertexList::ComputeMeasure(LocalPolyVertex* vtx) 
{ 
    double area, perimeter;

    Vector3d v1 = vtx->x - vtx->previous->x;
    Vector3d v2 = vtx->next->x - vtx->x;
    Vector3d v3 = vtx->previous->x - vtx->next->x;
    Vector3d v4 = v1.cross(v2);

    if ((area = v4.dot(this->Normal)) < 0.0) {
        return (vtx->measure = -1.0); // concave or bad triangle
    } 
    else if (area == 0.0) {
        return (vtx->measure = -2.0); // concave or bad triangle
    }

    // If here, the vertex is convex and the area of the triangle is positive.
    // Compute the specified measure.
    if (this->Measure == EarCutMeasureTypes::PERIMETER2_TO_AREA_RATIO) {
        // This measure sucks as triangles become "needle-like" but works fine
        // when the triangle is more flattened.
        perimeter = v1.norm() + v2.norm() + v3.norm();
        return (vtx->measure = perimeter * perimeter / area);
    } 
    else if (this->Measure == EarCutMeasureTypes::DOT_PRODUCT) {
        v1.normalize();
        v2.normalize();
        return (vtx->measure = (1.0 + v1.dot(v2)));
    } 
    else if (this->Measure == EarCutMeasureTypes::BEST_QUALITY) {
        // Best quality: ratio of maximum edge length to height.
        // This is a greedy triangulation algorithm, so it may
        // not produce the mesh with the best total quality. However,
        // in greedy fashion it will select the next triangle with the
        // best quality. It is an expensive operation.
        double l1 = v1.norm();
        double l2 = v2.norm();
        double l3 = v3.norm();
        int longestEdge = (l1 > l2 ? (l1 > l3 ? 1 : 3) : (l2 > l3 ? 2 : 3));
        double shortest, longest;
        if (longestEdge == 1) {
            longest = l1;
            shortest = DistanceToLine(vtx->next->x, vtx->x, vtx->previous->x);
        } else if (longestEdge == 2) {
            longest = l2;
            shortest = DistanceToLine(vtx->previous->x, vtx->x, vtx->next->x);
        } else {
            longest = l3;
            shortest = DistanceToLine(vtx->x, vtx->previous->x, vtx->next->x);
        }

        // sqrt(3)/2 = 0.866025404 comes from equilateral triangle
        return (vtx->measure = (0.866025404 - (shortest / longest)));
    } else {
        return -1.0;
    }
}

void PolyVertexList::RemoveVertex(LocalPolyVertex* vtx, IdArray::Pointer tris, RemovePriorityQueue* queue) {
    // Create triangle
    tris->AddId(vtx->id);
    tris->AddId(vtx->next->id);
    tris->AddId(vtx->previous->id);

    // remove vertex; special case if single triangle left
    if (--this->NumberOfVerts < 3) { return; }
    if (vtx == this->Head) { this->Head = vtx->next; }
    vtx->previous->next = vtx->next;
    vtx->next->previous = vtx->previous;

    // recompute measure, reinsert into queue
    // note that id may have been previously deleted (with Pop()) if we
    // are dealing with a concave polygon and vertex couldn't be split.
    if (queue) {
        queue->remove(vtx->previous->id);
        queue->remove(vtx->next->id);
        if (this->ComputeMeasure(vtx->previous) > 0.0) { 
            queue->push(vtx->previous->measure, vtx->previous->id);
        }
        if (this->ComputeMeasure(vtx->next) > 0.0) { 
            queue->push(vtx->next->measure, vtx->next->id);
        }
    }
}

void PolyVertexList::RemoveVertex(int i, IdArray::Pointer tris, RemovePriorityQueue* queue) {
    return this->RemoveVertex(this->Array + i, tris, queue);
}

static double PlaneEvaluate(const Vector3d& normal, const Vector3d& origin, const Vector3d& x) {
    return normal[0] * (x[0] - origin[0]) + normal[1] * (x[1] - origin[1]) + normal[2] * (x[2] - origin[2]);
}

// Performs intersection of the projection of two finite 3D lines onto a 2D plane.  
// An intersection is found if the projection of the two lines onto the plane 
// perpendicular to the cross product of the two lines intersect.  
// The parameters (u,v) are the parametric coordinates of the lines at the position of closest approach.
static int Intersection(const Vector3d& a1, const Vector3d& a2, const Vector3d& b1, const Vector3d& b2,
                        double& u, double& v, double tolerance) 
{
    //const double epsilon = 1e-10; // 容差值，处理浮点数精度问题
    const double epsilon = tolerance; // 容差值，处理浮点数精度问题

    // 计算两条线段的方向向量
    Vector3d d1 = a2 - a1; // 第一条线段的方向向量
    Vector3d d2 = b2 - b1; // 第二条线段的方向向量

    // 计算方向向量的叉积（法向量）
    Vector3d n = d1.cross(d2);
    double n_squared_norm = n.squaredNorm();

    // 检查线段是否平行或接近平行
    if (n_squared_norm < epsilon) {
        // 线段平行或接近平行，在投影平面上不会相交
        return 0;
    }

    // 计算向量从a1到b1
    Vector3d w0 = a1 - b1;

    // 计算参数u和v
    // 使用克莱姆法则解线性方程组
    double a = d1.dot(d1);
    double b = d1.dot(d2);
    double c = d2.dot(d2);
    double d = d1.dot(w0);
    double e = d2.dot(w0);

    double denominator = a * c - b * b;

    // 检查分母是否接近零（应该已经被n_squared_norm检查覆盖，但双重保险）
    if (std::abs(denominator) < epsilon) { return 0; }

    // 计算参数u和v
    u = (b * e - c * d) / denominator;
    v = (a * e - b * d) / denominator;

    // 检查交点是否在线段范围内
    // u应该在[0,1]范围内，表示交点在第一条线段上
    // v应该在[0,1]范围内，表示交点在第二条线段上
    if (u >= 0.0 && u <= 1.0 && v >= 0.0 && v <= 1.0) {
        return 1; // 找到有效交点
    }

    return 0; // 交点不在线段范围内
}

int PolyVertexList::CanRemoveVertex(LocalPolyVertex* currentVtx) { 

    double tolerance = this->Tol;
    int i, sign, currentSign;
    double val, s, t;
    LocalPolyVertex *previous, *next, *vtx;

    // Check for simple case
    if (this->NumberOfVerts <= 3) { return 1; }

    // Compute split plane, the point to be cut off
    // is always on the positive side of the plane.
    previous = currentVtx->previous;
    next = currentVtx->next;

    Vector3d sPt = previous->x; // point on plane
    Vector3d v = next->x - previous->x;

    Vector3d sN = CrossProduct(v, this->Normal);
    if (sN.normalize() == 0.0) {
        return 0; // bad split, indeterminant
    }

    // Traverse the other points to see if a) they are all on the
    // other side of the plane; and if not b) whether they intersect
    // the split line.
    int oneNegative = 0;
    val = PlaneEvaluate(sN, sPt, next->next->x);
    currentSign = (val > tolerance ? 1 : (val < -tolerance ? -1 : 0));
    oneNegative = (currentSign < 0 ? 1 : 0); // very important

    // Intersections are only computed when the split half-space is crossed
    for (vtx = next->next->next; vtx != previous; vtx = vtx->next) {
        val = PlaneEvaluate(sN, sPt, vtx->x);
        sign = (val > tolerance ? 1 : (val < -tolerance ? -1 : 0));
        if (sign != currentSign) {
            if (!oneNegative) {
                oneNegative = (sign < 0 ? 1 : 0); // very important
            }
            if (Intersection(sPt, next->x, vtx->x, vtx->previous->x, s, t, tolerance) != 0) {
                return 0;
            } else {
                currentSign = sign;
            }
        } // if crossing occurs
    } // for the rest of the loop

    if (!oneNegative) {
        return 0; // entire loop is on this side of plane
    } else {
        return 1;
    }
}

int PolyVertexList::CanRemoveVertex(int id) { return this->CanRemoveVertex(this->Array + id); }

bool MeshTriangulationFilter::Execute() {

    auto input = GetInput(0);
    switch (input->GetDataObjectType()) {
        case IG_SURFACE_MESH:
            mesh = DynamicCast<SurfaceMesh>(input);
            break;
        case IG_UNSTRUCTURED_MESH:
            mesh = DynamicCast<UnstructuredMesh>(input)->TransferToSurfaceMesh();
            break;
        default:
            break;
    }
    if (mesh == nullptr) { return false; }

    {
        bool isTriangle = true;
        igIndex face[IGAME_CELL_MAX_SIZE]{};
        for (int i = 0; i < mesh->GetNumberOfFaces(); i++) {
            int size = mesh->GetFacePointIds(i, face);
            if (size != 3) { 
                isTriangle = false;
                break;
            }
        }
        
        if (isTriangle) { 
            SetOutput(mesh);
            return true;
        }
    }

    auto Attrs = mesh->GetAttributeSet();
    std::vector<int> FaceIdMap;
    CellArray::Pointer Faces = CellArray::New();
    Points::Pointer Points = mesh->GetPoints();
    IdArray::Pointer Tris = IdArray::New();

    igIndex face[IGAME_CELL_MAX_SIZE]{};
    Point pts[IGAME_CELL_MAX_SIZE]{};
    for (int k = 0; k < mesh->GetNumberOfFaces(); k++) 
    {
        Face* face = mesh->GetFace(k);

        if (face->m_PointIds->GetNumberOfIds() == 3) 
        {
            Faces->AddCellId3(face->m_PointIds->GetId(0), face->m_PointIds->GetId(1), face->m_PointIds->GetId(2));
            FaceIdMap.push_back(k);
        }
        else
        {
            Tris->Reset();
            PolyVertexList poly(face->m_PointIds, face->m_Points, 1e-15, EarCutMeasureTypes::BEST_QUALITY);
            LocalPolyVertex* vtx;
            int i, j;
            
            if (!poly.ComputeNormal()) { 
                return false;
            }

            RemovePriorityQueue VertexQueue;
            for (i = 0, vtx = poly.Head; i < poly.NumberOfVerts; i++, vtx = vtx->next) {
                if (poly.ComputeMeasure(vtx) > 0.0) { 
                    VertexQueue.push(vtx->measure, vtx->id);
                }
            }

            while (poly.NumberOfVerts > 2 && !VertexQueue.empty()) {
                auto [_m, id] = VertexQueue.top();
                VertexQueue.pop();
                if (poly.CanRemoveVertex(id)) { 
                    poly.RemoveVertex(id, Tris, &VertexQueue);
                }
            } 
            
            for (i = 0; i < Tris->GetNumberOfIds() / 3; i++) {
                Faces->AddCellId3(face->m_PointIds->GetId(Tris->GetId(3 * i + 0)),
                                  face->m_PointIds->GetId(Tris->GetId(3 * i + 1)),
                                  face->m_PointIds->GetId(Tris->GetId(3 * i + 2)));
                FaceIdMap.push_back(k);
            } 
        }
    }

    SurfaceMesh::Pointer Mesh = SurfaceMesh::New();
    Mesh->SetName(mesh->GetName());
    Mesh->SetPoints(Points);
    Mesh->SetFaces(Faces);

    double cell[IGAME_CELL_MAX_SIZE]{};
    auto newAttrs = Mesh->GetAttributeSet();
    for (int i = 0; i < Attrs->GetNumberOfAttributes(); i++) { 
        auto& attr = Attrs->GetAttribute(i);
        if (attr.attachmentType == IG_POINT) { 
            FloatArray::Pointer arr = FloatArray::New();
            arr->SetName(attr.pointer->GetName());
            arr->SetDimension(attr.pointer->GetDimension());
            arr->Reserve(attr.pointer->GetNumberOfElements());

            for (IGsize j = 0; j < attr.pointer->GetNumberOfValues(); j++) { 
                arr->AddValue(attr.pointer->GetValue(j));
            }
            newAttrs->AddAttribute(attr.type, IG_POINT, arr);

        } else {

            FloatArray::Pointer arr = FloatArray::New();
            arr->SetName(attr.pointer->GetName());
            arr->SetDimension(attr.pointer->GetDimension());
            arr->Resize(Faces->GetNumberOfCells());

            for (IGsize j = 0; j < Faces->GetNumberOfCells(); j++) { 
                attr.pointer->GetElement(FaceIdMap[j], cell);
                arr->SetElement(j, cell);
            }
            newAttrs->AddAttribute(attr.type, IG_CELL, arr);
        }
    }

    SetOutput(Mesh);
    return true;
}

MeshTriangulationFilter::MeshTriangulationFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

double MeshTriangulationFilter::GetArea(Vector3d a, Vector3d b, Vector3d c) { return CrossProduct(a - b, a - c).length() / 2; }
IGAME_NAMESPACE_END


