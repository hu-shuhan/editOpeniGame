#include "iGameStreamTracer.h"
#include <algorithm>
#include <atomic>
#include <future>
#include <iGameThreadPool.h>
#include <shared_mutex>
#include <unordered_set>

// Initialize static thread_local members
IGAME_NAMESPACE_BEGIN
thread_local StreamTracer::TrigCache StreamTracer::trigCache;
thread_local std::vector<float> StreamTracer::reusableWeights;
thread_local std::vector<Vector3f> StreamTracer::reusableVectors;
thread_local std::vector<double> StreamTracer::reusableDoubles;
float A[5] = {1.0 / 5.0, 3.0 / 10.0, 3.0 / 5.0, 1.0, 7.0 / 8.0};
float B[5][5] = {{1.0 / 5.0, 0, 0, 0, 0},
                 {3.0 / 40.0, 9.0 / 40.0, 0, 0, 0},
                 {3.0 / 10.0, -9.0 / 10.0, 6.0 / 5.0, 0, 0},
                 {-11.0 / 54.0, 5.0 / 2.0, -70.0 / 27.0, 35.0 / 27.0, 0},
                 {1631.0 / 55296.0, 175.0 / 512.0, 575.0 / 13824.0, 44275.0 / 110592.0, 253.0 / 4096.0}};
float C[6] = {37.0 / 378.0, 0, 250.0 / 621.0, 125.0 / 594.0, 0, 512.0 / 1771.0};
float DC[6] = {37.0 / 378.0 - 2825.0 / 27648.0,
               0,
               250.0 / 621.0 - 18575.0 / 48384.0,
               125.0 / 594.0 - 13525.0 / 55296.0,
               -277.0 / 14336.0,
               512.0 / 1771.0 - 1.0 / 4.0};
void StreamTracer::initStreamTracer(DataObject::Pointer obj) {
    auto newModel = Model::New();
    newModel->SetDataObject(obj);
    initStreamTracer(newModel);
}
void StreamTracer::initStreamTracer(Model::Pointer _model) {
    model = _model;
    if (meshId == model->GetDataObject()->GetDataObjectId()) {

    } else if (DynamicCast<UnstructuredMesh>(model->GetDataObject())) {
        std::cout << "is UnstructuredMesh" << std::endl;
        ptFinder.clear();
        SetMesh(DynamicCast<UnstructuredMesh>(model->GetDataObject())->TransferToVolumeMesh());
        auto numOfCells = mesh->GetNumberOfVolumes();

        auto temPtFinder = PointFinder::New();
        temPtFinder->SetPoints(mesh->GetPoints());
        temPtFinder->Initialize();
        AddPtFinder(temPtFinder);
        std::cout << "PF over" << std::endl;
        if (!mesh->GetIsPolyhedronType()) {
            InitAdjacent(mesh->GetCells(), mesh->GetNumberOfPoints());
            this->UpdateProgress(0.2);
            mesh->RequestEditStatus();
            this->UpdateProgress(1);
        } else {
            clock_t startTime = clock();
            InitAdjacent(mesh->GetCells(), mesh->GetNumberOfPoints());
            std::cout<< "Init Adjacent Time: "
					  << static_cast<double>(clock() - startTime) / CLOCKS_PER_SEC << " seconds." << std::endl;
           // mesh->SetShouldBuildEageLinks(false);
           // mesh->SetShouldBuildFaceLinks(false);
           // mesh->SetShouldBuildFaceEageLinks(false);
           // mesh->SetShouldBuildVolumeFaceLinks(false);
           // mesh->SetShouldBuildVolumeEageLinks(false);
            mesh->InitPolyhedronVertices([this](double p) { this->UpdateProgress(p); });
            this->UpdateProgress(1);
        }

    } else if (DynamicCast<VolumeMesh>(model->GetDataObject())) {
        std::cout << "is VolumeMesh" << std::endl;
        ptFinder.clear();
        SetMesh(DynamicCast<VolumeMesh>(model->GetDataObject()));
        auto numOfCells = mesh->GetNumberOfVolumes();

        auto temPtFinder = PointFinder::New();
        temPtFinder->SetPoints(mesh->GetPoints());
        temPtFinder->Initialize();
        AddPtFinder(temPtFinder);
        std::cout << "PF over" << std::endl;
        if (!mesh->GetIsPolyhedronType()) {
            InitAdjacent(mesh->GetCells(), mesh->GetNumberOfPoints());
            mesh->ClearAllLinks();
            this->UpdateProgress(0.2);
            mesh->RequestEditStatus(); // Establishing Adjacency
            this->UpdateProgress(1);

        } else if (!mesh->HasSubDataObject()) {
            clock_t startTime = clock();
            InitAdjacent(mesh->GetCells(), mesh->GetNumberOfPoints());
            std::cout<< "Init Adjacent Time: "<< static_cast<double>(clock() - startTime) / CLOCKS_PER_SEC << " seconds." << std::endl;
            //mesh->SetShouldBuildEageLinks(false);
            //mesh->SetShouldBuildFaceLinks(false);
            //mesh->SetShouldBuildFaceEageLinks(false);
            //mesh->SetShouldBuildVolumeFaceLinks(false);
            //mesh->SetShouldBuildVolumeEageLinks(false);
            mesh->InitPolyhedronVertices([this](double p) { this->UpdateProgress(p); });
            this->UpdateProgress(1);
        }

    } else {
        std::cout << "is subdata" << std::endl;
        auto temData = model->GetDataObject();
        if (temData->HasSubDataObject()) {
            isSubModel = true;
            ptFinder.clear();
            auto it = temData->SubDataObjectIteratorBegin();
            for (; it != temData->SubDataObjectIteratorEnd(); it++) {
                auto temPtFinder = PointFinder::New();
                auto vol = DynamicCast<VolumeMesh>(it->second);
                temPtFinder->SetPoints(vol->GetPoints());
                temPtFinder->Initialize();
                AddPtFinder(temPtFinder);
            }
            initSubmodelLinks();
        } else {
            std::cout << "model data error" << std::endl;
            return;
        }
    }
    processCount = 0;
    totalProcess = 0;
    isChange = true;

    // Precompute trigonometric values for better performance
    precomputeTrigValues();
    meshId = model->GetDataObject()->GetDataObjectId();
    return;
}

Vector3f StreamTracer::SampleVector(const Vector3f& coord, bool& inside, igIndex& VolumeId,
                                    const std::string& vectorName, float terminalSpeed) {
    return interpolationVector(coord, inside, VolumeId, vectorName, terminalSpeed);
}
std::vector<Vector3f> StreamTracer::computeSubBlockCenters(const Vector3f& minCorner, const Vector3f& maxCorner,
                                                           int splitCount) {
    std::vector<Vector3f> centers;

    Vector3f boxSize;
    for (int i = 0; i < 3; ++i) { boxSize[i] = maxCorner[i] - minCorner[i]; }

    Vector3f subBlockSize;
    for (int i = 0; i < 3; ++i) { subBlockSize[i] = boxSize[i] / splitCount; }

    Vector3f halfSubBlockSize;
    for (int i = 0; i < 3; ++i) { halfSubBlockSize[i] = subBlockSize[i] / 2.0f; }

    for (int i = 0; i < splitCount; ++i) {
        for (int j = 0; j < splitCount; ++j) {
            for (int k = 0; k < splitCount; ++k) {
                Vector3f currentMin;
                currentMin[0] = minCorner[0] + i * subBlockSize[0];
                currentMin[1] = minCorner[1] + j * subBlockSize[1];
                currentMin[2] = minCorner[2] + k * subBlockSize[2];

                Vector3f center;
                center[0] = currentMin[0] + halfSubBlockSize[0];
                center[1] = currentMin[1] + halfSubBlockSize[1];
                center[2] = currentMin[2] + halfSubBlockSize[2];

                centers.push_back(center);
            }
        }
    }

    return centers;
}
std::vector<Vector3f> StreamTracer::getAllSubBlockCenters(const Vector3f& boxMax,   // 包围盒最大值
                                                          const Vector3f& boxMin,   // 包围盒最小值
                                                          const Vector3f& focusMax, // 重点观察区域最大值
                                                          const Vector3f& focusMin, // 重点观察区域最小值
                                                          int boxSplitCount,        // 包围盒分割数量（e×e×e）
                                                          int focusSplitCount       // 重点观察区域分割数量（f×f×f）
) {
    std::vector<Vector3f> allCenters;

    // 计算包围盒的所有子块中心
   // auto boxCenters = computeSubBlockCenters(boxMin, boxMax, boxSplitCount);
   // allCenters.insert(allCenters.end(), boxCenters.begin(), boxCenters.end());

    // 计算重点观察区域的所有子块中心
    auto focusCenters = computeSubBlockCenters(focusMin, focusMax, focusSplitCount);
    allCenters.insert(allCenters.end(), focusCenters.begin(), focusCenters.end());

    return allCenters;
}
std::vector<Vector3f> StreamTracer::getModelSelect() {
    auto& selectedPoints = model->GetSelection()->GetSelectedItems(IG_POINT);
    auto& selectedCells = model->GetSelection()->GetSelectedItems(IG_CELL);
    float minX = 0, minY = 0, minZ = 0;
    float maxX = 0, maxY = 0, maxZ = 0;
    if (!selectedPoints.empty()) {
        auto& point = selectedPoints;
        bool start = true;
        for (auto it = point.begin(); it != point.end(); it++) {
            Point p = mesh->GetPoint(*it);
            if (start) {
                start = false;
                minX = p[0];
                minY = p[1];
                minZ = p[2];
                maxX = p[0];
                maxY = p[1];
                maxZ = p[2];
            } else {
                minX = std::min(minX, p[0]);
                minY = std::min(minY, p[1]);
                minZ = std::min(minZ, p[2]);
                maxX = std::max(maxX, p[0]);
                maxY = std::max(maxY, p[1]);
                maxZ = std::max(maxZ, p[2]);
            }
        }
    } else if (!selectedCells.empty()) {
        auto& cell = selectedCells;
        bool flag = true;
        for (auto it = cell.begin(); it != cell.end(); it++) {
            igIndex pVolume[32]{};
            int psize = mesh->GetVolumePointIds(*it, pVolume);
            for (int i = 0; i < psize; i++) {
                Point p = mesh->GetPoint(pVolume[i]);
                if (flag) {
                    flag = false;
                    minX = p[0];
                    minY = p[1];
                    minZ = p[2];
                    maxX = p[0];
                    maxY = p[1];
                    maxZ = p[2];
                } else {
                    minX = std::min(minX, p[0]);
                    minY = std::min(minY, p[1]);
                    minZ = std::min(minZ, p[2]);
                    maxX = std::max(maxX, p[0]);
                    maxY = std::max(maxY, p[1]);
                    maxZ = std::max(maxZ, p[2]);
                }
            }
        }
    } else {
        std::cout << "no selete" << std::endl;
    }
    return getAllSubBlockCenters(mesh->GetBoundingBox().max, mesh->GetBoundingBox().min, Vector3f(maxX, maxY, maxZ),
                                 Vector3f(minX, minY, minZ), 4, 4);
}
std::vector<Vector3f> StreamTracer::GetUnifiedVectorField(std::string vectorName) {
    std::vector<Vector3f> result;


    auto attr = mesh->GetAttributeSet();
    iGame::AttributeSet::Attribute& vec = attr->GetAttribute(vectorName);


    int numPoints = mesh->GetNumberOfPoints();
    int numVolumes = mesh->GetNumberOfVolumes();

    result.resize(numPoints, Vector3f(0, 0, 0));

    if (vec.attachmentType == IG_CELL) {
        std::cout << "vec is cell" << std::endl;
        for (int cellId = 0; cellId < numVolumes; cellId++) {
            igIndex pts[32]{};
            int n = mesh->GetVolumePointIds(cellId, pts);
            double cv[4] = {0};
            vec.pointer->GetElement(cellId, cv);
            Vector3f V(cv[0], cv[1], cv[2]);

            for (int i = 0; i < n; i++) {
                igIndex pid = pts[i];
                result[pid] = V;
            }
        }
    } else {
        std::cout << "vec is point" << std::endl;
        for (int i = 0; i < numPoints; i++) {
            double pv[4] = {0};
            vec.pointer->GetElement(i, pv);
            result[i] = Vector3f(pv[0], pv[1], pv[2]);
        }
    }

    return result;
}
bool StreamTracer::Execute() {
    // 检查参数是否已设置
    if (m_SeedPoints.empty()) {
        igError("Seed points not set. Please call setinput() first.");
        m_ResultMesh = nullptr;
        return false;
    }

    if (m_VectorName.empty()) {
        igError("Vector name not set. Please call setinput() first.");
        m_ResultMesh = nullptr;
        return false;
    }
    if (!mesh) {
        std::cout << "No mesh" << std::endl;
        return false;
    }
    auto attr = mesh->GetAttributeSet();
    
    if (!attr) { 
        std::cout << "No attr" << std::endl;
        return false; 
    }
    std::cout << "VectorName: " << m_VectorName << std::endl;
    iGame::AttributeSet::Attribute& vec = attr->GetAttribute(m_VectorName);
    if (vec.IsNone()) {
        std::cout << "Vetor is none " << std::endl;
        return false; 
    }
    if (vec.pointer->GetDimension() != 3) {
        std::cout << "Vetor is not 3D " << std::endl;
        return false; 
    }

    int numPoints = mesh->GetNumberOfPoints();
  //  std::cout << "Point Num" << numPoints << std::endl;
    int numVolumes = mesh->GetNumberOfVolumes();
  //  std::cout << "Volume Num" << numVolumes << std::endl;
    int numAttr = vec.pointer->GetNumberOfElements();
  //  std::cout << "Attr Num" << numAttr << std::endl;
    if (vec.attachmentType == IG_POINT) { 
        if (numAttr != numPoints) { 
            std::cout << "numAttr != numPoints" << std::endl;
            return false; 
        }
    } else {
        if (numAttr != numVolumes) {
            std::cout << "numAttr != numVolumes" << std::endl;
            return false;
        }
    }

  //  std::cout << "1111111111111" << std::endl;

    currentV = std::move(GetUnifiedVectorField(m_VectorName));

    // 使用内部存储的参数调用原始计算逻辑
    std::vector<std::vector<float>> streamColor;
    auto streamlines = showStreamLineMix(m_SeedPoints, m_VectorName, streamColor, m_LengthOfStreamLine, m_LengthOfStep,
                                         m_TerminalSpeed, m_MaxSteps);
    //std::cout << "7777777777777" << std::endl;
    // 检查数据有效性
    if (streamlines.empty() || streamColor.empty()) {
        igError("Streamline calculation failed or returned empty data");
        m_ResultMesh = nullptr;
        return false;
    }
   // std::cout << "88888888888888888" << std::endl;
    // 创建 UnstructuredMesh 对象
    UnstructuredMesh::Pointer streamMesh = UnstructuredMesh::New();
    streamMesh->SetShellRenderingOption(false);
    Points::Pointer points = Points::New();
    CellArray::Pointer cells = CellArray::New();
    UnsignedIntArray::Pointer types = UnsignedIntArray::New();
    AttributeSet::Pointer attrSet = AttributeSet::New();
    FloatArray::Pointer velocityArray = FloatArray::New();
    velocityArray->SetDimension(3);
    velocityArray->SetName("Velocity");
    igIndex globalPointIndex = 0;
 //   std::cout << "99999999999999999" << std::endl;
    // 处理每条流线
    for (int streamlineIdx = 0; streamlineIdx < streamlines.size(); streamlineIdx++) {
        auto& streamline = streamlines[streamlineIdx];
        auto& streamlinecolor = streamColor[streamlineIdx];
        if (streamline.size() <= 6) continue; // 至少需要两个点（6个float值）

        igIndex lineStartIndex = globalPointIndex;
        int numPoints = streamline.size() / 6;

        // 添加流线的所有点
        for (int i = 0; i < numPoints; i++) {
            Point p1(streamline[i * 6], streamline[i * 6 + 1], streamline[i * 6 + 2]);
            Point p2(streamline[i * 6 + 3], streamline[i * 6 + 4], streamline[i * 6 + 5]);
            points->AddPoint(p1);
            points->AddPoint(p2);
            velocityArray->AddElement3(streamlinecolor[i * 6], streamlinecolor[i * 6 + 1], streamlinecolor[i * 6 + 2]);
            velocityArray->AddElement3(streamlinecolor[i * 6 + 3], streamlinecolor[i * 6 + 4],
                                       streamlinecolor[i * 6 + 5]);
            int tem[2]{globalPointIndex, globalPointIndex + 1};
            cells->AddCellIds(tem, 2);
            types->AddValue(IG_LINE);
            globalPointIndex+= 2;
        }

    }
 //   std::cout << "1111111111111111111" << std::endl;
    // 设置点数据和单元数据
    streamMesh->SetPoints(points);
    streamMesh->SetCells(cells, types);

    // 创建速度属性数组


    //// 添加速度数据到属性数组
    //for (const auto& velocityData: streamColor) {
    //    for (int i = 0; i < velocityData.size(); i += 3) {
    //        if (i + 2 < velocityData.size()) {
    //            velocityArray->AddElement3(velocityData[i], velocityData[i + 1], velocityData[i + 2]);
    //        }
    //    }
    //}
   // std::cout << "22222222222222222222" << std::endl;
    // 添加速度属性
    attrSet->AddAttribute(IG_VECTOR, IG_POINT, velocityArray);
   // std::cout << "33333333333333333333" << std::endl;

    // 设置属性集到网格
    streamMesh->SetAttributeSet(attrSet);

    // 设置网格名称
    streamMesh->SetName("StreamlinesUnstructuredMesh");
    // 将结果赋值给成员变量
    m_ResultMesh = streamMesh;
    return true;
}

std::vector<Vector3f> StreamTracer::getModelSelectMax(std::string VectorName,int numOfSeeds) {
    auto& selectedPoints = model->GetSelection()->GetSelectedItems(IG_POINT);
    auto& selectedCells = model->GetSelection()->GetSelectedItems(IG_CELL);
    std::vector<Vector3f> localMaxPoints;
    std::map<float, Vector3f, std::greater<>> seedsMap;
    currentV = std::move(GetUnifiedVectorField(VectorName));
    // 收集所有选中的点ID
    std::unordered_set<igIndex> allSelectedPoints;

    if (!selectedPoints.empty()) {
        // 如果选中的是点，直接使用
        for (auto pointId: selectedPoints) { allSelectedPoints.insert(pointId); }
    } else if (!selectedCells.empty()) {
        // 如果选中的是单元，提取单元中的所有点
        for (auto cellId: selectedCells) {
            igIndex pVolume[32]{};
            int psize = mesh->GetVolumePointIds(cellId, pVolume);
            for (int i = 0; i < psize; i++) { allSelectedPoints.insert(pVolume[i]); }
        }
    } else {
        std::cout << "no select" << std::endl;
        return localMaxPoints;
    }

    // 获取向量数据
    auto VectorData = mesh->GetAttributeSet();
    auto Vector = VectorData->GetAttribute(VectorName);
    if (Vector.pointer == nullptr) {
        std::cout << "Vector " << VectorName << " not found!" << std::endl;
        return localMaxPoints;
    }

    // 对每个选中的点，检查是否为局部最大值
    for (igIndex pointId: allSelectedPoints) {
        // 获取该点的向量值并计算其模长
        //double currentValue[4] = {0.0};
        //Vector.pointer->GetElement(pointId, currentValue);
        if (pointId < 0 || pointId >= currentV.size()) {
            std::cout << "over index:"<< pointId << std::endl;
            continue;
        } 
       const Vector3f& currentValue = currentV[pointId];
        double currentMagnitude = std::sqrt(currentValue[0] * currentValue[0] + currentValue[1] * currentValue[1] +
                                            currentValue[2] * currentValue[2]);

        // 收集邻接点
        std::unordered_set<igIndex> neighborPoints;

        // 通过该点的邻接单元获取邻接点
        if (pointId < vetex_link.offset.size() - 1) {
            for (long long k = vetex_link.offset[pointId]; k < vetex_link.offset[pointId + 1]; k++) {
                if (k < vetex_link.data.size()) {
                    igIndex cellId = vetex_link.data[k];
                    igIndex cellPoints[32]{};
                    int pointCount = mesh->GetVolumePointIds(cellId, cellPoints);
                    for (int i = 0; i < pointCount; i++) {
                        if (cellPoints[i] != pointId) { neighborPoints.insert(cellPoints[i]); }
                    }
                }
            }
        }

        // 检查是否为局部最大值
        bool isLocalMax = true;
        for (igIndex neighborId: neighborPoints) {
            //double neighborValue[4] = {0.0};
            //Vector.pointer->GetElement(neighborId, neighborValue);
            Vector3f neighborValue = currentV[neighborId];
            double neighborMagnitude =
                    std::sqrt(neighborValue[0] * neighborValue[0] + neighborValue[1] * neighborValue[1] +
                              neighborValue[2] * neighborValue[2]);

            if (neighborMagnitude >= currentMagnitude) {
                isLocalMax = false;
                break;
            } else if (neighborMagnitude == currentMagnitude) {
                std::cout << "equal" << std::endl;
            }
        }

        // 如果是局部最大值，添加该点的坐标到结果
        if (isLocalMax && !neighborPoints.empty()) {
            //float V[4] = {0.0};
            //Vector.pointer->GetElement(pointId, V);
            Vector3f V = currentV[pointId];
            Point p = mesh->GetPoint(pointId);
           // std::cout << pointId << ":" << mesh->IsBoundaryPoint(pointId) << std::endl;
            //seedsMap.emplace(V[0] * V[0] + V[1] * V[1] + V[2] * V[2], Vector3f(p[0] + 0.001f, p[1] + 0.001f, p[2] + 0.001f));
            seedsMap.emplace(V[0] * V[0] + V[1] * V[1] + V[2] * V[2], p+V*0.00001);
           // std::cout << p + V * 0.001 << std::endl;
        }
        //bool isLocalMin = true;
        //for (igIndex neighborId: neighborPoints) {
        //    double neighborValue[4] = {0.0};
        //    Vector.pointer->GetElement(neighborId, neighborValue);
        //    double neighborMagnitude =
        //            std::sqrt(neighborValue[0] * neighborValue[0] + neighborValue[1] * neighborValue[1] +
        //                      neighborValue[2] * neighborValue[2]);

        //    if (neighborMagnitude < currentMagnitude) {
        //        isLocalMin = false;
        //        break;
        //    } else if (neighborMagnitude == currentMagnitude) {
        //        std::cout << "equal" << std::endl;
        //    }
        //}

        //// 如果是局部最小值，添加该点的坐标到结果
        //if (isLocalMin && !neighborPoints.empty()) {
        //    Point p = mesh->GetPoint(pointId);
        //    localMaxPoints.emplace_back(Vector3f(p[0]-0.01f, p[1]-0.001f, p[2]-0.001f));
        //}
    }
    int numOfPoints = std::min((int)seedsMap.size(), numOfSeeds);
    auto it = seedsMap.begin();
    for (int i = 0; i < numOfPoints; ++i) { 
        localMaxPoints.emplace_back(it->second);
        it++;
    }
    std::cout << "Found " << localMaxPoints.size() << " local  points for vector field: " << VectorName << std::endl;
    return localMaxPoints;
}
void StreamTracer::initSubmodelLinks() {
    auto temData = model->GetDataObject();
    auto it = temData->SubDataObjectIteratorBegin();
    for (; it != temData->SubDataObjectIteratorEnd(); ++it) {
        auto vol = DynamicCast<VolumeMesh>(it->second);
        // Initialize adjacency for each sub-model
        SetMesh(vol);
        InitAdjacent(vol->GetCells(), vol->GetNumberOfPoints());
        vol->BuildVolumeLinks();
    }
}

std::vector<Vector3f> StreamTracer::subdataSeedGenerate(int numOfSeed) {
    std::vector<Vector3f> tem;
    auto temData = model->GetDataObject();
    auto box = temData->GetBoundingBox();
    if (temData->HasSubDataObject()) {

        Vector3f V = (box.max - box.min) / numOfSeed;
        for (int i = 0; i < numOfSeed; i++) { tem.emplace_back(box.min + V * i); }

    } else {
        std::cout << "model data error" << std::endl;
    }
    return tem;
}
std::vector<Vector3f> StreamTracer::seedLineGenerate(int numOfseed) {
    std::vector<Vector3f> tem;
    auto boundBox = this->mesh->GetBoundingBox();
    Vector3f maxPosition(boundBox.max);
    Vector3f minPosition(boundBox.min);
    Vector3f V = (maxPosition - minPosition) / numOfseed;
    for (int i = 0; i < numOfseed; i++) { tem.emplace_back(minPosition + V * i); }
    return tem;
}
std::vector<Vector3f> StreamTracer::seedPidGenerate(int numOfseed, igIndex pId1, igIndex pId2) {
    this->mesh = DynamicCast<VolumeMesh>(this->mesh);
    auto allPoints = mesh->GetPoints();
    int numOfPoints = mesh->GetNumberOfPoints();
    std::vector<Vector3f> tem;
    if (mesh == nullptr) { return tem; }
    Point a = allPoints->GetPoint(igIndex(pId1));
    Point b = allPoints->GetPoint(igIndex(pId2));
    auto step = (b - a) / numOfseed;
    for (int i = 0; i < numOfseed; i++) { tem.emplace_back(a + step * i); }
    return tem;
}
std::vector<Vector3f> StreamTracer::seedPCoordGenerate(int numOfseed, Vector3f p1, Vector3f p2) {
    this->mesh = DynamicCast<VolumeMesh>(this->mesh);
    auto allPoints = mesh->GetPoints();
    int numOfPoints = mesh->GetNumberOfPoints();
    std::vector<Vector3f> tem;
    if (mesh == nullptr) { return tem; }
    auto step = (p2 - p1) / numOfseed;
    for (int i = 0; i < numOfseed; i++) { tem.emplace_back(p1 + step * i); }
    return tem;
}
std::vector<Vector3f> StreamTracer::seedDataGenerate(int control, float proportion, int numOfseed, igIndex pId1,
                                                     igIndex pId2) {
    std::vector<Vector3f> seeds;
    switch (streamMode) {
        case Diagonal: {
            seedLineGenerate(numOfseed);
            break;
        }
        case PointId: {
            seedPidGenerate(numOfseed, pId1, pId2);
        }
        default:
            break;
    }
    return seeds;
}
std::vector<Vector3f> StreamTracer::seedGenerate(int control, float proportion,
                                                 int numOfseed) { // face
    this->mesh = DynamicCast<VolumeMesh>(this->mesh);
    auto allPoints = mesh->GetPoints();
    int numOfPoints = mesh->GetNumberOfPoints();
    std::vector<Vector3f> tem;
    if (mesh == nullptr) { return tem; }
    Point first = allPoints->GetPoint(igIndex(0));
    // Point first = HexMesh->GetPoint(igIndex(0));
    float maxPosition[3] = {first[0], first[1], first[2]};
    float minPosition[3] = {first[0], first[1], first[2]};
    for (int i = 1; i < numOfPoints; i++) {
        // Point a = HexMesh->GetPoint(igIndex(i));
        Point a = allPoints->GetPoint(igIndex(i));

        for (int j = 0; j < 3; j++) {
            maxPosition[j] = (maxPosition[j] > a[j]) ? maxPosition[j] : a[j];
            minPosition[j] = (minPosition[j] < a[j]) ? minPosition[j] : a[j];
        }
    }
    // std::cout << "max is" << maxPosition[0] << " "<<maxPosition[1] << " " <<
    // maxPosition[2] << std::endl; std::cout << "min is" << minPosition[0] << " "
    // << minPosition[1] << " " << minPosition[2] << std::endl;
    float lengthX = maxPosition[0] - minPosition[0];
    float lengthY = maxPosition[1] - minPosition[1];
    float lengthZ = maxPosition[2] - minPosition[2];
    float num = sqrt(numOfseed) - 1;
    for (int i = 0; i < num; i++) {
        for (int j = 0; j < num; j++) {
            float step = 1 / num;
            switch (control) {
                case 0: {
                    tem.emplace_back(Vector3f(minPosition[0] + proportion * lengthX,
                                              minPosition[1] + i * step * lengthY,
                                              minPosition[2] + j * step * lengthZ));
                    break;
                }
                case 1: {
                    tem.emplace_back(Vector3f(minPosition[0] + i * step * lengthX, minPosition[1] + j * step * lengthY,
                                              minPosition[2] + proportion * lengthZ));
                    break;
                }
                case 2: {
                    tem.emplace_back(Vector3f(minPosition[0] + i * step * lengthX,
                                              minPosition[1] + proportion * lengthY,
                                              minPosition[2] + j * step * lengthZ));
                    break;
                }
                default:
                    break;
            }
        }
    }
    return tem;
}
float StreamTracer::AccuracyCul(std::vector<std::vector<float>> streamline, float threshold, int Nth) {
    int NumOfP = 0;
    int TrueOfP = 0;
    for (auto line: streamline) {
        auto tem = line.size() / 3;
        NumOfP += tem;
        int t = Nth * 3;
        for (int i = t; i < line.size(); i += t) {
            int T;
            if (i + t + 2 > line.size()) {
                T = (line.size() - 2 - i) / 3;
            } else {
                T = Nth;
            }
            Vector3f start(line[i - t], line[i - t + 1], line[i - t + 2]);
            Vector3f end(line[i], line[i + 1], line[i + 2]);
            auto step = (end - start) / T;
            for (int j = 0; j < T; j++) {
                Vector3f real(line[i + 3 * j], line[i + 3 * j + 1], line[i + 3 * j + 2]);
                Vector3f temV = real - start - step * j;
                float len = temV.length();
                if (len < threshold) {
                    TrueOfP++;
                } else {
                    //std:: cout << len <<std:: endl;
                }
            }
        }
    }
    float ans = ((float) TrueOfP) / NumOfP;
    return ans;
    ;
}

void StreamTracer::SetInput(std::vector<Vector3f> seeds, std::string vectorName, float lengthOfStreamLine,
                            float lengthOfStep, float terminalSpeed, int maxSteps) {
    m_SeedPoints = seeds;
    m_VectorName = vectorName;
    m_LengthOfStreamLine = lengthOfStreamLine;
    m_LengthOfStep = lengthOfStep;
    m_TerminalSpeed = terminalSpeed;
    m_MaxSteps = maxSteps;
}

std::vector<Vector3f> StreamTracer::streamSeedGenerate(int control, float proportion,
                                                       int numOfseed) { // line
    // auto HexMesh =
    // iGameHexMesh::SafeDownCast(model->RenderMeshes[0]);//Subsequent update
    // options

    this->mesh = DynamicCast<VolumeMesh>(this->mesh);
    auto allPoints = mesh->GetPoints();
    int numOfPoints = mesh->GetNumberOfPoints();
    //mesh->RequestEditStatus(); // Establishing Adjacency
    std::vector<Vector3f> tem;
    // Point first = HexMesh->GetPoint(igIndex(0));
    Point first = allPoints->GetPoint(igIndex(0));
    float maxPosition[3] = {first[0], first[1], first[2]};
    float minPosition[3] = {first[0], first[1], first[2]};
    for (int i = 1; i < numOfPoints; i++) {
        // Point a = HexMesh->GetPoint(igIndex(i));
        Point a = allPoints->GetPoint(igIndex(i));
        for (int j = 0; j < 3; j++) {
            maxPosition[j] = (maxPosition[j] > a[j]) ? maxPosition[j] : a[j];
            minPosition[j] = (minPosition[j] < a[j]) ? minPosition[j] : a[j];
        }
    }
    // std::cout << "max is" << maxPosition[0] << " "<<maxPosition[1] << " " <<
    // maxPosition[2] << std::endl; std::cout << "min is" << minPosition[0] << " "
    // << minPosition[1] << " " << minPosition[2] << std::endl;
    float lengthX = maxPosition[0] - minPosition[0];
    float lengthY = maxPosition[1] - minPosition[1];
    float lengthZ = maxPosition[2] - minPosition[2];
    float num = numOfseed;
    for (int i = 0; i < num; i++) {

        float step = 1 / num;
        switch (control) {
            case 0: {
                tem.emplace_back(Vector3f(minPosition[0] + proportion * lengthX, minPosition[1] + i * step * lengthY,
                                          minPosition[2] + 0.5 * lengthZ));
                break;
            }
            case 1: {
                tem.emplace_back(Vector3f(minPosition[0] + i * step * lengthX, minPosition[1] + 0.5 * lengthY,
                                          minPosition[2] + proportion * lengthZ));
                break;
            }
            case 2: {
                tem.emplace_back(Vector3f(minPosition[0] + i * step * lengthX, minPosition[1] + proportion * lengthY,
                                          minPosition[2] + 0.5 * lengthZ));
                break;
            }
            default:
                break;
        }
    }
    return tem;
}
std::vector<Vector3f> StreamTracer::streamBoundSeedGenerate(int numOfseed) { // line
    // auto HexMesh =
    // iGameHexMesh::SafeDownCast(model->RenderMeshes[0]);//Subsequent update
    // options

    this->mesh = DynamicCast<VolumeMesh>(this->mesh);
    auto allPoints = mesh->GetPoints();
    int numOfPoints = mesh->GetNumberOfPoints();
    //   mesh->RequestEditStatus(); // Establishing Adjacency
    std::vector<Vector3f> tem;
    // Point first = HexMesh->GetPoint(igIndex(0));
    Point first = allPoints->GetPoint(igIndex(0));
    auto bound = mesh->GetBoundingBox();
    float num = numOfseed;
    auto step = (bound.max - bound.min) / num;
    // std::cout << "max is" << maxPosition[0] << " "<<maxPosition[1] << " " <<
    // maxPosition[2] << std::endl; std::cout << "min is" << minPosition[0] << " "
    // << minPosition[1] << " " << minPosition[2] << std::endl;
    for (int i = 0; i < num; i++) { tem.emplace_back(bound.min + step * i); }
    return tem;
}

std::vector<std::vector<float>> StreamTracer::showStreamLineMix(std::vector<Vector3f> seed, std::string vectorName,
                                                                std::vector<std::vector<float>>& streamColor,
                                                                float lengOfStreamLine, float lengthOfStep,
                                                                float terminalSpeed, int maxSteps) {
   // std::cout << "2222222222222" << std::endl;
    streamColor.clear();
    streamColor.resize(seed.size());
    std::vector<std::vector<float>> tem(seed.size());
    if (model == nullptr && mesh == nullptr) return tem;
    if (!isSubModel) {
        if (isChange) {
            cellBoundLength.clear();
            int numOfPoints = mesh->GetNumberOfPoints();
            clock_t time1 = clock();
            auto PointData = mesh->GetAttributeSet();
            auto Vector = PointData->GetAttribute(vectorName);
            std::cout << "Vector type is:" << Vector.attachmentType << std::endl;
            for (int i = 0; i < Vector.pointer->GetNumberOfElements(); ++i) { 
                float v[4] = {0.0f};
                Vector.pointer->GetElement(i, v);
                maxF = std::max(static_cast<float>(maxF),
                                static_cast<float>(sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2])));
                minF = std::min(static_cast<float>(minF),
                                static_cast<float>(sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2])));
            }
            int component = Vector.pointer->GetDimension();
            isChange = false;
        }
    } else {
        if (isChange) { isChange = false; }
    }
 //   std::cout << "333333333333333" << std::endl;

    auto func = [&](int i) -> void {
        // std::cout << i << " start\n";
        // auto time1 = clock();
        int steps = maxSteps;
        auto& _coord = seed[i];
        tem[i].emplace_back(_coord[0]);
        tem[i].emplace_back(_coord[1]);
        tem[i].emplace_back(_coord[2]);
        bool inside = true;
        bool flag = true;
        igIndex flag1 = -1;
        float length = 0;
        while (flag && length < lengOfStreamLine && steps-- > 0) {
            inside = false;
            flag = false;
            bool check = false;
            Vector3f k[7];
            auto temV = interpolationVector(_coord, inside, flag1, vectorName, terminalSpeed);
            k[1] = lengthOfStep * temV.normalized();
            streamColor[i].emplace_back(temV[0]);
            streamColor[i].emplace_back(temV[1]);
            streamColor[i].emplace_back(temV[2]);
            if (inside) {
                flag = true;
                inside = false;
            }
            k[2] = lengthOfStep *
                   interpolationVector(_coord + k[1] * B[0][0], inside, flag1, vectorName, terminalSpeed).normalized();
            if (inside) {
                flag = true;
                inside = false;
            }

            k[3] = lengthOfStep * interpolationVector(_coord + k[1] * B[1][0] + k[2] * B[1][1], inside, flag1,
                                                      vectorName, terminalSpeed)
                                          .normalized();
            if (inside) {
                flag = true;
                inside = false;
            }

            k[4] = lengthOfStep * interpolationVector(_coord + k[1] * B[2][0] + k[2] * B[2][1] + k[3] * B[2][2], inside,
                                                      flag1, vectorName, terminalSpeed)
                                          .normalized();
            if (inside) {
                flag = true;
                inside = false;
            }

            k[5] = lengthOfStep *
                   interpolationVector(_coord + k[1] * B[3][0] + k[2] * B[3][1] + k[3] * B[3][2] + k[4] * B[3][3],
                                       inside, flag1, vectorName, terminalSpeed)
                           .normalized();
            if (inside) {
                flag = true;
                inside = false;
            }

            k[6] = lengthOfStep * interpolationVector(_coord + k[1] * B[4][0] + k[2] * B[4][1] + k[3] * B[4][2] +
                                                              k[4] * B[4][3] + k[5] * B[4][4],
                                                      inside, flag1, vectorName, terminalSpeed)
                                          .normalized();
            if (inside) {
                flag = true;
                inside = false;
            }

            if (flag) {
                Vector3f temC(0, 0, 0);
                for (int i = 0; i < 6; i++) { temC += k[i + 1] * C[i]; }
                length += temC.length();
                _coord += temC;
            }
            tem[i].emplace_back(_coord[0]);
            tem[i].emplace_back(_coord[1]);
            tem[i].emplace_back(_coord[2]);
            temV = interpolationVector(_coord, inside, flag1, vectorName, terminalSpeed);
            streamColor[i].emplace_back(temV[0]);
            streamColor[i].emplace_back(temV[1]);
            streamColor[i].emplace_back(temV[2]);
            if (flag && length < lengOfStreamLine && steps != 0) {
                tem[i].emplace_back(_coord[0]);
                tem[i].emplace_back(_coord[1]);
                tem[i].emplace_back(_coord[2]);
            }
        }
    };
    processCount = 0;
    totalProcess = seed.size();

  //  std::cout << "555555555555555" << std::endl;
    if (m_IsSingleThread) {
        for (int i = 0; i < seed.size(); i++) {
            func(i);
            processCount++;
            this->UpdateProgress((double) processCount / seed.size());
        }
    } else {
        ThreadPool::Pointer tp = ThreadPool::Instance();
        std::vector<std::future<void>> result(seed.size());
        for (int i = 0; i < seed.size(); i++) { result[i] = tp->Commit(func, i); }

        for (int i = 0; i < seed.size(); i++) {
            result[i].wait();
            processCount++;
            this->UpdateProgress((double) processCount / seed.size());
        }
    }

   // std::cout << "6666666666666666" << std::endl;
    return tem;


    //	std::cout << "time: " << clock() - time1 << std::endl;
}
std::vector<std::vector<float>> StreamTracer::showStreamLineHex(std::vector<Vector3f> seed, std::string vectorName,
                                                                std::vector<std::vector<float>>& streamColor,
                                                                float lengOfStreamLine, float lengthOfStep,
                                                                float terminalSpeed, int maxSteps) {
    // this->mesh = DynamicCast<VolumeMesh>(this->mesh);
    streamColor.clear();
    streamColor.resize(seed.size());
    std::vector<std::vector<float>> tem(seed.size());
    if (mesh == nullptr) return tem;
    std::vector<Vector3f> _vector;
    int numOfPoints = mesh->GetNumberOfPoints();
    clock_t time1 = clock();
    auto PointData = mesh->GetAttributeSet();
    auto Points = mesh->GetPoints();
    auto Vector = PointData->GetAttribute(vectorName);
    int component = Vector.pointer->GetDimension();
    float MAX_STEP = 0.001, MIN_STEP = 0.0001, ERR = 0.000001;

    for (int i = 0; i < numOfPoints; i++) {
        float v[8] = {0.0f};
        Vector.pointer->GetElement(i, v);
        _vector.emplace_back(Vector3f(v[0], v[1], v[2]));
    }
    // std::cout << clock() - time1 << std::endl;
    // clock_t time1 = clock();
    ThreadPool::Pointer tp = ThreadPool::Instance();

    std::vector<std::future<void>> result(seed.size());

    auto func = [&](int i) -> void {
        // std::cout << i << " start\n";
        // auto time1 = clock();
        int steps = maxSteps;
        auto& _coord = seed[i];
        tem[i].emplace_back(_coord[0]);
        tem[i].emplace_back(_coord[1]);
        tem[i].emplace_back(_coord[2]);
        bool inside = true;
        bool flag = true;
        igIndex flag1 = -1;
        float length = 0;
        while (flag && length < lengOfStreamLine && steps-- > 0) {
            inside = false;
            flag = false;
            bool check = false;
            Vector3f k[7];
            auto temV = interpolationVectorHexWithNatural(_coord, inside, flag1, vectorName, terminalSpeed);
            k[1] = lengthOfStep * temV;
            streamColor[i].emplace_back(temV[0]);
            streamColor[i].emplace_back(temV[1]);
            streamColor[i].emplace_back(temV[2]);
            if (inside) {
                flag = true;
                inside = false;
            }
            k[2] = lengthOfStep *
                   interpolationVectorHexWithNatural(_coord + k[1] * B[0][0], inside, flag1, vectorName, terminalSpeed);
            if (inside) {
                flag = true;
                inside = false;
            }

            k[3] = lengthOfStep * interpolationVectorHexWithNatural(_coord + k[1] * B[1][0] + k[2] * B[1][1], inside,
                                                                    flag1, vectorName, terminalSpeed);
            if (inside) {
                flag = true;
                inside = false;
            }

            k[4] = lengthOfStep *
                   interpolationVectorHexWithNatural(_coord + k[1] * B[2][0] + k[2] * B[2][1] + k[3] * B[2][2], inside,
                                                     flag1, vectorName, terminalSpeed);
            if (inside) {
                flag = true;
                inside = false;
            }

            k[5] = lengthOfStep * interpolationVectorHexWithNatural(_coord + k[1] * B[3][0] + k[2] * B[3][1] +
                                                                            k[3] * B[3][2] + k[4] * B[3][3],
                                                                    inside, flag1, vectorName, terminalSpeed);
            if (inside) {
                flag = true;
                inside = false;
            }

            k[6] = lengthOfStep *
                   interpolationVectorHexWithNatural(_coord + k[1] * B[4][0] + k[2] * B[4][1] + k[3] * B[4][2] +
                                                             k[4] * B[4][3] + k[5] * B[4][4],
                                                     inside, flag1, vectorName, terminalSpeed);
            if (inside) {
                flag = true;
                inside = false;
            }

            if (flag) {
                Vector3f temC(0, 0, 0);
                for (int i = 0; i < 6; i++) { temC += k[i + 1] * C[i]; }
                length += temC.length();
                _coord += temC;
            }
            tem[i].emplace_back(_coord[0]);
            tem[i].emplace_back(_coord[1]);
            tem[i].emplace_back(_coord[2]);
            temV = interpolationVectorHexWithNatural(_coord, inside, flag1, vectorName, terminalSpeed);
            streamColor[i].emplace_back(temV[0]);
            streamColor[i].emplace_back(temV[1]);
            streamColor[i].emplace_back(temV[2]);
            if (flag && length < lengOfStreamLine && steps != 0) {
                tem[i].emplace_back(_coord[0]);
                tem[i].emplace_back(_coord[1]);
                tem[i].emplace_back(_coord[2]);
            }
        }
        // std::cout << "1" << std::endl;
        // std::cout << i << "end" <<clock()-time1<< std::endl;
    };

    for (int i = 0; i < seed.size(); i++) { result[i] = tp->Commit(func, i); }
    for (int i = 0; i < seed.size(); i++) { result[i].wait(); }

    // result[2] = tp->Commit(func, 2);
    // result[2].wait();

    //	std::cout << "time: " << clock() - time1 << std::endl;
    return tem;
}

std::vector<std::vector<std::vector<float>>>
StreamTracer::showStreamFace(std::vector<Vector3f> seed, std::string vectorName,
                             std::vector<std::vector<std::vector<float>>>& streamColor, float lengOfStreamLine,
                             float lengthOfStep, float terminalSpeed, int maxSteps) {
    this->mesh = DynamicCast<VolumeMesh>(this->mesh);
    streamColor.clear();
    std::vector<std::vector<std::vector<float>>> tem;
    std::vector<Vector3f> seedTem = seed;
    std::cout << "temsize" << seedTem.size() << std::endl;
    if (mesh == nullptr) return tem;
    std::vector<Vector3f> _vector;
    int numOfPoints = mesh->GetNumberOfPoints();
    clock_t time1 = clock();
    auto PointData = mesh->GetAttributeSet();
    auto Vector = PointData->GetAttribute(vectorName);
    int component = Vector.pointer->GetDimension();
    float MAX_STEP = 0.001, MIN_STEP = 0.0001, ERR = 0.000001;
    int initial = 0;
    float* Length = new float[seedTem.size()];
    int MAX_SEED = seed.size() * 2;
    std::vector<int> tem_strip;
    for (int i = 0; i < seedTem.size(); i++) {
        Length[i] = 0;
        if (i != seedTem.size() - 1) { tem_strip.emplace_back(i); }
    }
    // mesh->streamFaceStrip.emplace_back(tem_strip);
    for (int i = 0; i < numOfPoints; i++) {
        float v[8] = {0.0f};
        Vector.pointer->GetElement(i, v);
        _vector.emplace_back(Vector3f(v[0], v[1], v[2]));
    }
    // std::cout << clock() - time1 << std::endl;
    // clock_t time1 = clock();
    ThreadPool* tp = ThreadPool::Instance();
    std::vector<std::vector<float>> temFace;
    std::vector<std::vector<float>> temColor;
    auto func = [&](int i, float* length, int index) -> void {
        // std::cout << i << " start\n";
        // auto time1 = clock();
        int steps;
        if (maxSteps >= 100) {
            steps = 100;
        } else {
            steps = maxSteps;
        }
        auto& _coord = seedTem[i];
        temFace[i].emplace_back(_coord[0]);
        temFace[i].emplace_back(_coord[1]);
        temFace[i].emplace_back(_coord[2]);
        bool inside = true;
        bool flag = true;
        igIndex flag1 = -1;
        while (flag && steps-- > 0) {
            inside = false;
            flag = false;
            bool check = false;
            Vector3f k[7];
            auto temV = interpolationVectorHexWithNatural(_coord, inside, flag1, vectorName, terminalSpeed);
            k[1] = lengthOfStep * temV;
            temColor[i].emplace_back(temV[0]);
            temColor[i].emplace_back(temV[1]);
            temColor[i].emplace_back(temV[2]);
            if (inside) {
                flag = true;
                inside = false;
            }
            k[2] = lengthOfStep *
                   interpolationVectorHexWithNatural(_coord + k[1] * B[0][0], inside, flag1, vectorName, terminalSpeed);
            if (inside) {
                flag = true;
                inside = false;
            }

            k[3] = lengthOfStep * interpolationVectorHexWithNatural(_coord + k[1] * B[1][0] + k[2] * B[1][1], inside,
                                                                    flag1, vectorName, terminalSpeed);
            if (inside) {
                flag = true;
                inside = false;
            }

            k[4] = lengthOfStep *
                   interpolationVectorHexWithNatural(_coord + k[1] * B[2][0] + k[2] * B[2][1] + k[3] * B[2][2], inside,
                                                     flag1, vectorName, terminalSpeed);
            if (inside) {
                flag = true;
                inside = false;
            }

            k[5] = lengthOfStep * interpolationVectorHexWithNatural(_coord + k[1] * B[3][0] + k[2] * B[3][1] +
                                                                            k[3] * B[3][2] + k[4] * B[3][3],
                                                                    inside, flag1, vectorName, terminalSpeed);
            if (inside) {
                flag = true;
                inside = false;
            }

            k[6] = lengthOfStep *
                   interpolationVectorHexWithNatural(_coord + k[1] * B[4][0] + k[2] * B[4][1] + k[3] * B[4][2] +
                                                             k[4] * B[4][3] + k[5] * B[4][4],
                                                     inside, flag1, vectorName, terminalSpeed);
            if (inside) {
                flag = true;
                inside = false;
            }

            if (flag) {
                Vector3f temC(0, 0, 0);
                for (int i = 0; i < 6; i++) { temC += k[i + 1] * C[i]; }
                length[i] += temC.length();
                _coord += temC;
            }
            temFace[i].emplace_back(_coord[0]);
            temFace[i].emplace_back(_coord[1]);
            temFace[i].emplace_back(_coord[2]);
            temV = interpolationVectorHexWithNatural(_coord, inside, flag1, vectorName, terminalSpeed);
            temColor[i].emplace_back(temV[0]);
            temColor[i].emplace_back(temV[1]);
            temColor[i].emplace_back(temV[2]);
            if (flag && steps != 0) {
                temFace[i].emplace_back(_coord[0]);
                temFace[i].emplace_back(_coord[1]);
                temFace[i].emplace_back(_coord[2]);
            }
        }
        // std::cout << "1" << std::endl;
        // std::cout << i << "end" <<clock()-time1<< std::endl;
    };
    float* temLength = nullptr;
    temLength = new float[1];
    temLength[0] = 0;
    while (maxSteps > 0 && !seedTem.empty()) {
        int nOfSeed = seedTem.size();
        std::vector<std::future<void>> result(nOfSeed);
        temFace.clear();
        temColor.clear();
        tem_strip.clear();
        temFace.resize(nOfSeed);
        temColor.resize(nOfSeed);
        int lineNum = 0;
        for (int i = 0; i < nOfSeed; i++) { result[i] = tp->Commit(func, i, Length, initial); }
        for (int i = 0; i < nOfSeed; i++) { result[i].wait(); }
        tem.emplace_back(temFace);
        streamColor.emplace_back(temColor);
        seedTem.clear();
        delete[] temLength;
        temLength = new float[nOfSeed * 2];
        for (int i = 0; i < nOfSeed * 2; i++) { temLength[i] = 0; }
        bool* seedIsEnd = new bool[nOfSeed];
        for (int i = 0; i < nOfSeed; i++) { seedIsEnd[i] = true; }
        for (int i = 0; i < nOfSeed - 1; i++) {
            int num1 = tem[initial][i].size(), num2 = tem[initial][i + 1].size();
            // if (num1 == 600 && num2 == 600) {
            if (true) {
                Vector3f v00(tem[initial][i][num1 - 6], tem[initial][i][num1 - 5], tem[initial][i][num1 - 4]);
                Vector3f v10(tem[initial][i + 1][num2 - 6], tem[initial][i + 1][num2 - 5],
                             tem[initial][i + 1][num2 - 4]);
                Vector3f v01(tem[initial][i][num1 - 3], tem[initial][i][num1 - 2], tem[initial][i][num1 - 1]);
                Vector3f v11(tem[initial][i + 1][num2 - 3], tem[initial][i + 1][num2 - 2],
                             tem[initial][i + 1][num2 - 1]);
                float H = std::min(distance2Line(v01, v00, v10), distance2Line(v11, v00, v10));
                if ((v01 - v11).length() > 2 * H && (lineNum < MAX_SEED)) {
                    tem_strip.emplace_back(lineNum);
                    if (seedIsEnd[i]) {
                        seedTem.emplace_back(v01);
                        temLength[lineNum++] = Length[i];
                    }
                    seedTem.emplace_back((v01 + v11) / 2);
                    tem_strip.emplace_back(lineNum);
                    temLength[lineNum++] = (Length[i + 1] + Length[i]) / 2;
                    seedTem.emplace_back(v11);
                    temLength[lineNum++] = Length[i + 1];
                    seedIsEnd[i + 1] = false;
                } else {
                    tem_strip.emplace_back(lineNum);
                    if (seedIsEnd[i]) {
                        seedTem.emplace_back(v01);
                        temLength[lineNum++] = Length[i];
                    }
                }
            }
        }

        maxSteps -= 100;
        delete[] seedIsEnd;
        initial++;
        delete[] Length;
        Length = temLength;
        float checkLength = 0;
        std::cout << "temsize" << seedTem.size() << std::endl;
        for (int i = 0; i < seedTem.size(); i++) { checkLength += Length[i]; }
        if (checkLength > (lengOfStreamLine * seedTem.size())) { seedTem.clear(); }
        //	mesh->streamFaceStrip.emplace_back(tem_strip);
    }
    delete[] Length;
    // result[41] = tp->Commit(func, 41);
    // result[41].wait();

    //	std::cout << "time: " << clock() - time1 << std::endl;
    return tem;
}
bool StreamTracer::CellData2PointData(std::string vectorName) {
    this->mesh = DynamicCast<VolumeMesh>(this->mesh);
    auto allPolyhedrons = mesh->GetVolumes();
    auto allPoints = mesh->GetPoints();
    auto numOfPoints = mesh->GetNumberOfPoints();
    auto numOfCells = mesh->GetNumberOfVolumes();
    auto Vec = mesh->GetAttributeSet();
    auto Scalars = Vec->GetAllAttributes();
    int size = Scalars->GetNumberOfElements();
    for (int i = 0; i < size; i++) {
        auto scalarDataArray = Scalars->GetElement(i);
        if (scalarDataArray.type == IG_VECTOR) std::cout << "type is a" << scalarDataArray.attachmentType << std::endl;
    }
    auto vec = Vec->GetAttribute(vectorName);
    // how much D
    if (vec.attachmentType != IG_CELL) {
        std::cout << "error! type is:" << vec.attachmentType << std::endl;
        return false;
    }
    auto vecV = vec.pointer->GetDimension();
    std::vector<Vector3f> pointVector(numOfPoints);
    auto chec1 = vec.pointer->GetNumberOfElements();
    std::cout << chec1 << std::endl;
    std::cout << numOfPoints << std::endl;
    std::vector<int> pointVectorNUM(numOfPoints); // The number of volumes to which the point belongs
    igIndex vhs[256];
    igIndex fhs[256];
    igIndex vcnt, fcnt = 0;
    for (int i = 0; i < numOfCells; i++) {
        float VV[3] = {0.0f};
        vec.pointer->GetElement(i, VV);
        Vector3f temVec(VV[0], VV[1], VV[2]);
        auto volume = mesh->GetVolume(i);
        int numOfCellPoints = volume->GetCellSize(); // The number of point
        for (int j = 0; j < numOfCellPoints; j++) {
            auto pointId = volume->GetPointId(j);
            pointVector[pointId] = pointVector[pointId] + temVec;
            pointVectorNUM[pointId]++;
        }
    }
    FloatArray::Pointer VectorData = FloatArray::New();
    VectorData->SetDimension(3);
    VectorData->SetName(vectorName + "_Point");
    for (int i = 0; i < numOfPoints; i++) {
        for (int j = 0; j < vecV; j++) { VectorData->AddValue(pointVector[i][j] / pointVectorNUM[i]); }
    }
    return true;
}
float StreamTracer::distance2Line(Vector3f point, Vector3f lineP1, Vector3f lineP2) {
    Vector3f pP1 = point - lineP1;
    Vector3f line = lineP2 - lineP1;
    return abs((pP1 * line) / line.length());
}
Vector3f StreamTracer::interpolationVector(const Vector3f& coord, bool& inside, igIndex& VolumeId,
                                           std::string vectorName, float terminalSpeed) {
    Vector3f finnal = Vector3f(0, 0, 0);
    auto temData = model->GetDataObject();
    if (!temData->HasSubDataObject()) {
        std::vector<igIndex> tem{};
        if (VolumeId == -1) {
            // if (true) {
            int numOfPoints = mesh->GetNumberOfPoints();
            int numOfVolumes = mesh->GetNumberOfVolumes();
            if (ptFinder[0]) {
                igIndex temPointId = ptFinder[0]->FindClosestPoint(coord);
                // Use vetex_link adjacency data instead of mesh method
                for (long long k = vetex_link.offset[temPointId]; k < vetex_link.offset[temPointId + 1]; k++) {
                    tem.emplace_back(vetex_link.data[k]);
                }
            }

        } else {
            // Use cell_link adjacency data instead of mesh method
            for (long long k = cell_link.offset[VolumeId]; k < cell_link.offset[VolumeId + 1]; k++) {
                tem.emplace_back(cell_link.data[k]);
            }
            tem.emplace_back(VolumeId);
        }
        VolumeId = -1;
        if (tem.empty()) { std::cout << "no tem Volume" << std::endl; }
        for (auto& c: tem) {
            if (mesh == nullptr) { return finnal; }
            int contactPointNum = 0;
            // Hexahedron *volume = dynamic_cast<Hexahedron*>(mesh->GetVolume(c));
            igIndex pVolume[32]{};
            int psize = mesh->GetVolumePointIds(c, pVolume);
            BoundingBox culL;
            for (int i = 0; i < psize; i++) { culL.add(mesh->GetPoint(pVolume[i])); }
            if (!culL.isIn(coord)) { continue; }

            igIndex volume[32]{};
            igIndex f[32]{};
            int size = mesh->GetVolumeFaceIds(c, volume);
            for (int i = 0; i < size; i++) {
                int fsize = mesh->GetFacePointIds(volume[i], f);
                for (int j = 1; j < fsize - 1; j++) {
                    auto& v0 = mesh->GetPoint(f[0]);
                    auto& v1 = mesh->GetPoint(f[j]);
                    auto& v2 = mesh->GetPoint(f[(j + 1)]);

                    if (checkContact(coord, v0, v1, v2)) contactPointNum++;
                }
            }
            if (contactPointNum % 2 == 1) {
                inside = true;
                VolumeId = c;
                std::unique_lock<std::shared_mutex> lock(rwMutex);
                cellBoundLength[VolumeId] = culL.diag();
                break;
            }
        }
        if (!inside) { return finnal; }
        igIndex volume[32]{};
        int size = mesh->GetVolumePointIds(VolumeId, volume);
        auto it = [&]() {
            std::shared_lock<std::shared_mutex> lock(rwMutex);
            return cellBoundLength.find(VolumeId);
        }();

        float longest;
        if (it != cellBoundLength.end()) {
            longest = it->second;
        } else {
            BoundingBox culL;
            for (int i = 0; i < size; i++) { culL.add(mesh->GetPoint(volume[i])); }
            longest = culL.diag();
            // use write lock
            std::unique_lock<std::shared_mutex> lock(rwMutex);
            cellBoundLength[VolumeId] = longest;
        }
        auto CellData = mesh->GetAttributeSet();
        auto Vector = CellData->GetAttribute(vectorName);
        //if (Vector.attachmentType == IG_CELL) {
        //    float v[4] = {0.0f};
        //    Vector.pointer->GetElement(VolumeId, v);
        //    finnal = Vector3f(v[0], v[1], v[2]);
        //}
        //else 
        if (mesh->GetIsPolyhedronType()) {
                finnal = interpolationVectorMixWithMeanV(coord, inside, VolumeId, vectorName, terminalSpeed);
        } 
        else  {
            if (size == 4) { finnal = interpolationVectorTri(coord, inside, VolumeId, vectorName, terminalSpeed); }
            else if (size == 8) {
                finnal = interpolationVectorHexWithNatural(coord, inside, VolumeId, vectorName, terminalSpeed);
            }
            else { finnal = interpolationVectorMixWithMeanV(coord, inside, VolumeId, vectorName, terminalSpeed); }
        }

    } 
    else {
        std::vector<igIndex> tem;
        int temIndex = 0;
        double minD = DBL_MAX;
        auto it = temData->SubDataObjectIteratorBegin();
        for (; it != temData->SubDataObjectIteratorEnd(); ++it) {
            if (ptFinder[temIndex]) {
                double temMinD = 0;
                igIndex temPointId = ptFinder[temIndex]->FindClosestPoint(coord, temMinD);
                minD = std::min(minD, temMinD);
            } else {
                std::cout << "no ptFinder!" << std::endl;
            }
            temIndex++;
        }
        it = temData->SubDataObjectIteratorBegin();
        temIndex = 0;
        for (; it != temData->SubDataObjectIteratorEnd(); ++it) {
            tem.clear();
            auto vol = DynamicCast<VolumeMesh>(it->second);
            int numOfPoints = vol->GetNumberOfPoints();
            if (ptFinder[temIndex]) {
                double temMinD = 0;
                igIndex temPointId = ptFinder[temIndex]->FindClosestPoint(coord, temMinD);
                if (temMinD == minD) {
                    igIndex nearVolume[128];
                    int find = vol->GetPointToNeighborVolumes(temPointId, nearVolume);
                    for (int i = 0; i < find; i++) { tem.emplace_back(nearVolume[i]); }
                    VolumeId = -1;
                    for (auto& c: tem) {
                        if (vol == nullptr) { return finnal; }
                        int contactPointNum = 0;
                        // Hexahedron *volume = dynamic_cast<Hexahedron*>(mesh->GetVolume(c));
                        igIndex volume[32]{};
                        igIndex f[32]{};
                        int size = vol->GetVolumeFaceIds(c, volume);
                        for (int i = 0; i < size; i++) {
                            int fsize = vol->GetFacePointIds(volume[i], f);
                            for (int j = 1; j < fsize - 1; j++) {
                                auto& v0 = vol->GetPoint(f[0]);
                                auto& v1 = vol->GetPoint(f[j]);
                                auto& v2 = vol->GetPoint(f[(j + 1)]);

                                if (checkContact(coord, v0, v1, v2)) contactPointNum++;
                            }
                        }
                        if (contactPointNum % 2 == 1) {
                            inside = true;
                            VolumeId = c;
                            mesh = vol;
                            igIndex volume[32]{};
                            int size = mesh->GetVolumePointIds(VolumeId, volume);
                            if (mesh->GetIsPolyhedronType()) {
                                finnal = interpolationVectorMixWithMeanV(coord, inside, VolumeId, vectorName,
                                                                         terminalSpeed);
                            } else if (size == 4) {
                                finnal = interpolationVectorTri(coord, inside, VolumeId, vectorName, terminalSpeed);
                            } else if (size == 8) {
                                finnal = interpolationVectorHexWithNatural(coord, inside, VolumeId, vectorName,
                                                                           terminalSpeed);
                            } else {
                                finnal = interpolationVectorMixWithMeanV(coord, inside, VolumeId, vectorName,
                                                                         terminalSpeed);
                            }
                            break;
                        }
                    }
                } else {
                    temIndex++;
                    continue;
                }
            } else {
                std::cout << "no ptFinder!" << std::endl;
            }
            temIndex++;
        }
        if (!inside) { return finnal; }
    }
    return finnal;
}
Vector3f StreamTracer::interpolationVectorTri(const Vector3f& coord, bool& inside, igIndex& VolumeId,
                                              std::string vectorName,
                                              float terminalSpeed) { // Interpolation
    Vector3f finnal(0, 0, 0);
    std::vector<Vector3f> v(4);
    igIndex volume[4];
    double def;
    auto size = mesh->GetVolumePointIds(VolumeId, volume);

    // Verify tetrahedron has exactly 4 vertices
    if (size != 4) {
        std::cout << "Error: Expected 4 vertices for tetrahedron, got " << size << std::endl;
        inside = false;
        return finnal;
    }

    for (int i = 0; i < 4; i++) { v[i] = mesh->GetPoint(volume[i]); }
    double weights[4];
    Vector3f rhs = coord - v[0];
    Vector3f p10 = v[1] - v[0];
    Vector3f p20 = v[2] - v[0];
    Vector3f p30 = v[3] - v[0];
    double d_rhs[3] = {rhs[0], rhs[1], rhs[2]};
    double pt[3][3] = {
            {p10[0], p10[1], p10[2]},
            {p20[0], p20[1], p20[2]},
            {p30[0], p30[1], p30[2]},
    };
    if ((def = Determinant3x3(pt[0], pt[1], pt[2])) == 0.0) {
        inside = false;
        return finnal;
    }
    weights[1] = Determinant3x3(d_rhs, pt[1], pt[2]) / def;
    weights[2] = Determinant3x3(pt[0], d_rhs, pt[2]) / def;
    weights[3] = Determinant3x3(pt[0], pt[1], d_rhs) / def;
    weights[0] = 1 - weights[1] - weights[2] - weights[3];
    auto VectorData = mesh->GetAttributeSet();
    auto Vector = VectorData->GetAttribute(vectorName);
    for (int i = 0; i < 4; ++i) {
        double _v[4] = {1.0f};
        Vector.pointer->GetElement(volume[i], _v);
        Vector3f V(_v[0], _v[1], _v[2]);
        finnal += V * weights[i];
    }
    if (finnal.length() < terminalSpeed) { inside = false; }
    return finnal;
}
Vector3f StreamTracer::interpolationVectorHexWithNatural(const Vector3f& coord, bool& inside, igIndex& VolumeId,
                                                         std::string vectorName,
                                                         float terminalSpeed) { // Interpolation
    Vector3f finnal(0.0f, 0.0f, 0.0f);
    inside = false;

    if (!mesh) { return finnal; }

    igIndex volume[32]{};
    int numPts = mesh->GetVolumePointIds(VolumeId, volume);
    if (numPts != 8) {
        std::cout << "Error: Expected 8 vertices for hexahedron, got " << numPts << std::endl;
        return finnal;
    }

    Point v[8];
    for (int i = 0; i < 8; ++i) { v[i] = mesh->GetPoint(volume[i]); }

    double pcoords[3] = {0.5, 0.5, 0.5};
    double lastPCoords[3] = {0.5, 0.5, 0.5};
    double derivs[24];
    double weights[8];

    int diagonals[4][2] = {{0, 6}, {1, 7}, {2, 4}, {3, 5}};
    double longestDiagonal = 0.0;
    for (int i = 0; i < 4; ++i) {
        double d2 = (v[diagonals[i][0]] - v[diagonals[i][1]]).length();
        if (longestDiagonal < d2) { longestDiagonal = d2; }
    }
    double volumeBound = longestDiagonal * std::sqrt(longestDiagonal);
    double determinantTolerance = std::max(1e-20, 1e-5 * volumeBound);

    const int maxIter = 10;
    const double tol = 1e-5;

    bool converged = false;
    for (int iteration = 0; iteration < maxIter && !converged; ++iteration) {
        InterpolationFunctions(pcoords, weights);
        InterpolationDerivs(pcoords, derivs);

        double fcol[3] = {0.0, 0.0, 0.0};
        double rcol[3] = {0.0, 0.0, 0.0}; // d x / d r
        double scol[3] = {0.0, 0.0, 0.0}; // d x / d s
        double tcol[3] = {0.0, 0.0, 0.0}; // d x / d t

        for (int i = 0; i < 8; ++i) {
            for (int j = 0; j < 3; ++j) {
                double xv = v[i][j];
                fcol[j] += xv * weights[i];
                rcol[j] += xv * derivs[i];      // dN_i/dr
                scol[j] += xv * derivs[i + 8];  // dN_i/ds
                tcol[j] += xv * derivs[i + 16]; // dN_i/dt
            }
        }

        fcol[0] -= coord[0];
        fcol[1] -= coord[1];
        fcol[2] -= coord[2];

        double detJ = Determinant3x3(rcol, scol, tcol);
        if (std::fabs(detJ) < determinantTolerance) {
            inside = false;
            return finnal;
        }

        double rhs[3] = {-fcol[0], -fcol[1], -fcol[2]};

        double c1[3], c2[3], c3[3];

        c1[0] = rhs[0];
        c1[1] = rhs[1];
        c1[2] = rhs[2];
        c2[0] = scol[0];
        c2[1] = scol[1];
        c2[2] = scol[2];
        c3[0] = tcol[0];
        c3[1] = tcol[1];
        c3[2] = tcol[2];
        double dp0 = Determinant3x3(c1, c2, c3) / detJ;

        c1[0] = rcol[0];
        c1[1] = rcol[1];
        c1[2] = rcol[2];
        c2[0] = rhs[0];
        c2[1] = rhs[1];
        c2[2] = rhs[2];
        c3[0] = tcol[0];
        c3[1] = tcol[1];
        c3[2] = tcol[2];
        double dp1 = Determinant3x3(c1, c2, c3) / detJ;

        c1[0] = rcol[0];
        c1[1] = rcol[1];
        c1[2] = rcol[2];
        c2[0] = scol[0];
        c2[1] = scol[1];
        c2[2] = scol[2];
        c3[0] = rhs[0];
        c3[1] = rhs[1];
        c3[2] = rhs[2];
        double dp2 = Determinant3x3(c1, c2, c3) / detJ;

        double newPCoords[3];
        newPCoords[0] = pcoords[0] + dp0;
        newPCoords[1] = pcoords[1] + dp1;
        newPCoords[2] = pcoords[2] + dp2;

        for (int k = 0; k < 3; ++k) {
            if (newPCoords[k] < -0.01) newPCoords[k] = -0.01;
            if (newPCoords[k] > 1.01) newPCoords[k] = 1.01;
        }

        double diff0 = std::fabs(newPCoords[0] - pcoords[0]);
        double diff1 = std::fabs(newPCoords[1] - pcoords[1]);
        double diff2 = std::fabs(newPCoords[2] - pcoords[2]);
        if (diff0 < tol && diff1 < tol && diff2 < tol) {
            pcoords[0] = newPCoords[0];
            pcoords[1] = newPCoords[1];
            pcoords[2] = newPCoords[2];
            converged = true;
            break;
        }

        lastPCoords[0] = pcoords[0];
        lastPCoords[1] = pcoords[1];
        lastPCoords[2] = pcoords[2];
        pcoords[0] = newPCoords[0];
        pcoords[1] = newPCoords[1];
        pcoords[2] = newPCoords[2];

        if (std::fabs(pcoords[0]) > 1e4 || std::fabs(pcoords[1]) > 1e4 || std::fabs(pcoords[2]) > 1e4) {
            inside = false;
            return finnal;
        }
    }

    if (!converged) {
        inside = false;
        return finnal;
    }

    const double eps = 1e-4;
    if (pcoords[0] < -eps || pcoords[0] > 1.0 + eps || pcoords[1] < -eps || pcoords[1] > 1.0 + eps ||
        pcoords[2] < -eps || pcoords[2] > 1.0 + eps) {
        inside = false;
        return finnal;
    }

    inside = true;

    InterpolationFunctions(pcoords, weights);
    auto VectorData = mesh->GetAttributeSet();
    auto Vector = VectorData->GetAttribute(vectorName);
    for (int i = 0; i < 8; ++i) {
        double _v[4] = {0.0};
        Vector.pointer->GetElement(volume[i], _v);
        Vector3f V(static_cast<float>(_v[0]), static_cast<float>(_v[1]), static_cast<float>(_v[2]));
        finnal += V * static_cast<float>(weights[i]);
    }

    if (finnal.length() < terminalSpeed) { inside = false; }

    return finnal;
}
Vector3f StreamTracer::interpolationVectorMixWithMeanV(const Vector3f& coord, bool& inside, igIndex& VolumeId,
                                                       std::string vectorName,
                                                       float terminalSpeed) { // Interpolation
    Vector3f finnal = Vector3f(0, 0, 0);
    igIndex volume[32]{};
    igIndex face[32]{};
    igIndex p[32]{};
    int size = mesh->GetVolumePointIds(VolumeId, volume);
    int fsize = mesh->GetVolumeFaceIds(VolumeId, face);

    // Check array bounds safety
    if (size > 32) {
        std::cout << "Error: Volume has too many points (" << size << "), max supported is 32" << std::endl;
        inside = false;
        return finnal;
    }
    if (fsize > 32) {
        std::cout << "Error: Volume has too many faces (" << fsize << "), max supported is 32" << std::endl;
        inside = false;
        return finnal;
    }

    // Early exit for degenerate cases
    if (size < 3 || fsize < 3) {
        inside = false;
        return finnal;
    }
    // Use memory pool for weights to avoid frequent allocation
    if (reusableWeights.size() < size) {
        reusableWeights.resize(size * 2); // Pre-allocate extra space
    }
    std::fill_n(reusableWeights.data(), size, 0.0f);
    float* weights = reusableWeights.data(); // Use raw pointer for performance

    int MaxPolygonSize = 0;
    for (int i = 0; i < fsize; i++) {
        int fpsize = mesh->GetFacePointIds(face[i], p);
        MaxPolygonSize = std::max(fpsize, MaxPolygonSize);
    }
    ComputeWeightsForPolygonMesh(volume, coord, face, MaxPolygonSize, size, fsize, weights);
    auto VectorData = mesh->GetAttributeSet();
    auto Vector = VectorData->GetAttribute(vectorName);
    for (int i = 0; i < size; ++i) {
        //double _v[4] = {0.0f};
        //Vector.pointer->GetElement(volume[i], _v);
        //Vector3f V(_v[0], _v[1], _v[2]);
        auto V = currentV[volume[i]];
        finnal += V * weights[i];
    }
    if (finnal.length() < terminalSpeed) { inside = false; }
    // return finnal * longestDiagonal;
    return finnal;
}
void StreamTracer::ComputeWeightsForPolygonMesh(igIndex* PointIds, const Vector3f& coord, igIndex* FaceIds,
                                                int MaxPolygonSize, int psize, int fsize, float* weights) {
    // weights array is already initialized to 0 by caller
    // 点到点源的长度
    std::vector<double> dist(psize);
    // 点到点源的单位矢量
    std::vector<Vector3f> uVec(psize);
    static constexpr double eps = 0.00000001;

    // Early exit conditions
    if (psize <= 0 || fsize <= 0) return;

    for (int pid = 0; pid < psize; ++pid) {

        auto pt = mesh->GetPoint(PointIds[pid]);
        uVec[pid] = Vector3f(pt[0] - coord[0], pt[1] - coord[1], pt[2] - coord[2]);
        // distance
        dist[pid] = uVec[pid].length();

        if (dist[pid] < eps) {
            weights[pid] = 1.0;
            return;
        }

        uVec[pid] /= dist[pid];
    }

    //点到多面体内点源的单位矢量
    std::vector<Vector3f> u(MaxPolygonSize);
    //点在体单元内的局部id
    std::vector<int> uIdInVolume(MaxPolygonSize);
    std::vector<double> alpha(MaxPolygonSize);
    std::vector<double> theta(MaxPolygonSize);

    int poly = 0;
    while (poly < fsize) {
        igIndex fp[32];
        int fpsize = mesh->GetFacePointIds(FaceIds[poly], fp);
        if (fpsize == 0) {
            poly++;
            continue;
        }

        // Check face point array bounds
        if (fpsize > 32) {
            std::cout << "Error: Face has too many points (" << fpsize << "), max supported is 32" << std::endl;
            poly++;
            continue;
        }

        for (int j = 0; j < fpsize; j++) {
            uIdInVolume[j] = -1; // Initialize to invalid index
            for (int i = 0; i < psize; i++) {
                int temid = fp[j];
                if (PointIds[i] == temid) {
                    u[j] = uVec[i];
                    uIdInVolume[j] = i;
                    break; // Found match, break inner loop
                }
            }
        }

        // unit vector v.
        Vector3f v(0, 0, 0);
        double l;
        double angle;
        // Pre-allocate vectors to avoid repeated creation
        if (reusableVectors.size() < fpsize) { reusableVectors.resize(fpsize); }

        Vector3f temp;
        for (int j = 0; j < fpsize - 1; j++) {
            reusableVectors[j] = u[j + 1] - u[j]; // Cache difference vector
            double diffLength = reusableVectors[j].length();

            temp = u[j].cross(u[j + 1]);
            double crossLength = temp.length();
            if (crossLength > 0) {
                temp /= crossLength; // Normalize
            }

            angle = 2.0 * fastAsin(diffLength / 2.0); // Use cached asin

            v[0] += 0.5 * angle * temp[0];
            v[1] += 0.5 * angle * temp[1];
            v[2] += 0.5 * angle * temp[2];
        }

        // Handle last->first connection
        reusableVectors[fpsize - 1] = u[0] - u[fpsize - 1];
        l = reusableVectors[fpsize - 1].length();
        angle = 2.0 * fastAsin(l / 2.0);
        temp = u[fpsize - 1].cross(u[0]);
        double crossLength = temp.length();
        if (crossLength > 0) { temp /= crossLength; }
        v[0] += 0.5 * angle * temp[0];
        v[1] += 0.5 * angle * temp[1];
        v[2] += 0.5 * angle * temp[2];
        double vNorm = v.length();
        v.normalize();
        if (v.dot(u[0]) < 0) {
            v[0] = -v[0];
            v[1] = -v[1];
            v[2] = -v[2];
        }

        // Use memory pool for double arrays
        if (reusableDoubles.size() < fpsize * 4) { reusableDoubles.resize(fpsize * 4); }

        Vector3f n0, n1;
        for (int j = 0; j < fpsize - 1; j++) {
            n0 = u[j].cross(v);
            double n0Length = n0.length();
            if (n0Length > 0) n0 /= n0Length;

            n1 = u[j + 1].cross(v);
            double n1Length = n1.length();
            if (n1Length > 0) n1 /= n1Length;

            l = (n0 - n1).length();
            alpha[j] = 2.0 * fastAsin(l / 2.0); // Use cached asin
            temp = n0.cross(n1);
            if (temp.dot(v) < 0) { alpha[j] = -alpha[j]; }

            // Cache this calculation
            reusableVectors[j] = u[j] - v;
            l = reusableVectors[j].length();
            theta[j] = 2.0 * fastAsin(l / 2.0); // Use cached asin
        }

        n0 = u[fpsize - 1].cross(v);
        double n0Length = n0.length();
        if (n0Length > 0) n0 /= n0Length;

        n1 = u[0].cross(v);
        double n1Length = n1.length();
        if (n1Length > 0) n1 /= n1Length;

        l = (n0 - n1).length();
        alpha[fpsize - 1] = 2.0 * fastAsin(l / 2.0); // Use cached asin
        temp = n0.cross(n1);
        if (temp.dot(v) < 0) { alpha[fpsize - 1] = -alpha[fpsize - 1]; }

        // Use previously cached vector if possible
        reusableVectors[fpsize - 1] = u[fpsize - 1] - v;
        l = reusableVectors[fpsize - 1].length();
        theta[fpsize - 1] = 2.0 * fastAsin(l / 2.0); // Use cached asin

        bool outlierFlag = false;
        for (int j = 0; j < fpsize; j++) {
            if (fabs(theta[j]) < eps) {
                outlierFlag = true;
                if (uIdInVolume[j] >= 0 && uIdInVolume[j] < psize) {
                    weights[uIdInVolume[j]] += vNorm / dist[uIdInVolume[j]];
                }
                break;
            }
        }

        if (outlierFlag) {
            poly++;
            continue;
        }

        double sum = 0.0;
        sum += 1.0 / fastTan(theta[0]) * (fastTan(alpha[0] / 2.0) + fastTan(alpha[fpsize - 1] / 2.0));
        for (int j = 1; j < fpsize; j++) {
            sum += 1.0 / fastTan(theta[j]) * (fastTan(alpha[j] / 2.0) + fastTan(alpha[j - 1] / 2.0));
        }

        if (fabs(sum) < eps) {
            // Clear weights array
            for (int k = 0; k < psize; k++) weights[k] = 0.0;

            for (int j = 0; j < fpsize - 1; j++) {
                l = reusableVectors[j].length();    // Reuse cached vector
                theta[j] = 2.0 * fastAsin(l / 2.0); // Use cached asin
            }
            l = reusableVectors[fpsize - 1].length();    // Reuse cached vector
            theta[fpsize - 1] = 2.0 * fastAsin(l / 2.0); // Use cached asin

            double sumWeight;
            if (uIdInVolume[0] >= 0 && uIdInVolume[0] < psize) {
                weights[uIdInVolume[0]] =
                        1.0 / dist[uIdInVolume[0]] * (fastTan(theta[fpsize - 1] / 2.0) + fastTan(theta[0] / 2.0));
                sumWeight = weights[uIdInVolume[0]];
            } else {
                sumWeight = 0.0;
            }

            for (int j = 1; j < fpsize; j++) {
                if (uIdInVolume[j] >= 0 && uIdInVolume[j] < psize) {
                    weights[uIdInVolume[j]] =
                            1.0 / dist[uIdInVolume[j]] * (fastTan(theta[j - 1] / 2.0) + fastTan(theta[j] / 2.0));
                    sumWeight = sumWeight + weights[uIdInVolume[j]];
                }
            }

            if (sumWeight < eps) { return; }

            for (int j = 0; j < fpsize; j++) {
                if (uIdInVolume[j] >= 0 && uIdInVolume[j] < psize) { weights[uIdInVolume[j]] /= sumWeight; }
            }

            return;
        }

        // weight
        if (uIdInVolume[0] >= 0 && uIdInVolume[0] < psize) {
            weights[uIdInVolume[0]] += vNorm / sum / dist[uIdInVolume[0]] / fastSin(theta[0]) *
                                       (fastTan(alpha[0] / 2.0) + fastTan(alpha[fpsize - 1] / 2.0));
        }
        for (int j = 1; j < fpsize; j++) {
            if (uIdInVolume[j] >= 0 && uIdInVolume[j] < psize) {
                weights[uIdInVolume[j]] += vNorm / sum / dist[uIdInVolume[j]] / fastSin(theta[j]) *
                                           (fastTan(alpha[j] / 2.0) + fastTan(alpha[j - 1] / 2.0));
            }
        }

        poly++;
    }

    float sumWeight = 0;
    for (int i = 0; i < psize; i++) { sumWeight += weights[i]; }

    if (fabs(sumWeight) < eps) { return; }
    for (int i = 0; i < psize; i++) { weights[i] /= sumWeight; }
}
bool StreamTracer::isInside(Vector3f coord, Vector3f v0, Vector3f v1, Vector3f v2, Vector3f v3,
                            std::vector<float>& dis) {
    int contact_num = 0;
    dis.clear();
    if (checkContact(coord, v0, v1, v3)) contact_num++;
    if (checkContact(coord, v0, v2, v3)) contact_num++;
    if (checkContact(coord, v0, v1, v2)) contact_num++;
    if (checkContact(coord, v2, v1, v3)) contact_num++;
    if (contact_num % 2 == 1) {
        dis.emplace_back(pointToFaceDis(coord, v1, v2, v3) / pointToFaceDis(Vector3f(v0[0], v0[1], v0[2]), v1, v2, v3));
        dis.emplace_back(pointToFaceDis(coord, v0, v2, v3) / pointToFaceDis(Vector3f(v1[0], v1[1], v1[2]), v0, v2, v3));
        dis.emplace_back(pointToFaceDis(coord, v1, v0, v3) / pointToFaceDis(Vector3f(v2[0], v2[1], v2[2]), v1, v0, v3));
        dis.emplace_back(pointToFaceDis(coord, v1, v2, v0) / pointToFaceDis(Vector3f(v3[0], v3[1], v3[2]), v1, v2, v0));
        return true;
    }
    return false;
}
void StreamTracer::InterpolationFunctions(const double pcoords[3], double sf[8]) {
    double rm, sm, tm;

    rm = 1. - pcoords[0];
    sm = 1. - pcoords[1];
    tm = 1. - pcoords[2];

    const auto rmXsm = rm * sm;
    const auto p0Xsm = pcoords[0] * sm;
    const auto p0Xp1 = pcoords[0] * pcoords[1];
    const auto rmXp1 = rm * pcoords[1];

    sf[0] = rmXsm * tm;
    sf[1] = p0Xsm * tm;
    sf[2] = p0Xp1 * tm;
    sf[3] = rmXp1 * tm;
    sf[4] = rmXsm * pcoords[2];
    sf[5] = p0Xsm * pcoords[2];
    sf[6] = p0Xp1 * pcoords[2];
    sf[7] = rmXp1 * pcoords[2];
}
void StreamTracer::InterpolationDerivs(const double pcoords[3], double derivs[24]) {
    double rm, sm, tm;

    rm = 1. - pcoords[0];
    sm = 1. - pcoords[1];
    tm = 1. - pcoords[2];

    // r-derivatives
    derivs[0] = -sm * tm;
    derivs[1] = -derivs[0];
    derivs[2] = pcoords[1] * tm;
    derivs[3] = -derivs[2];
    derivs[4] = -sm * pcoords[2];
    derivs[5] = -derivs[4];
    derivs[6] = pcoords[1] * pcoords[2];
    derivs[7] = -derivs[6];

    // s-derivatives
    derivs[8] = -rm * tm;
    derivs[9] = -pcoords[0] * tm;
    derivs[10] = -derivs[9];
    derivs[11] = -derivs[8];
    derivs[12] = -rm * pcoords[2];
    derivs[13] = -pcoords[0] * pcoords[2];
    derivs[14] = -derivs[13];
    derivs[15] = -derivs[12];

    // t-derivatives
    derivs[16] = -rm * sm;
    derivs[17] = -pcoords[0] * sm;
    derivs[18] = -pcoords[0] * pcoords[1];
    derivs[19] = -rm * pcoords[1];
    derivs[20] = -derivs[16];
    derivs[21] = -derivs[17];
    derivs[22] = -derivs[18];
    derivs[23] = -derivs[19];
}
double StreamTracer::Determinant3x3(const double c1[3], const double c2[3], const double c3[3]) {
    return c1[0] * c2[1] * c3[2] + c2[0] * c3[1] * c1[2] + c3[0] * c1[1] * c2[2] - c1[0] * c3[1] * c2[2] -
           c2[0] * c1[1] * c3[2] - c3[0] * c2[1] * c1[2];
}
float StreamTracer::pointToFaceDis(Vector3f coord, Vector3f v0, Vector3f v1, Vector3f v2) {
    Vector3f a(v0[0], v0[1], v0[2]);
    Vector3f b(v1[0], v1[1], v1[2]);
    Vector3f c(v2[0], v2[1], v2[2]);
    Vector3f ab = b - a;
    Vector3f ac = c - a;
    Vector3f normal = ab.cross(ac);
    Vector3f pa = coord - a;
    return abs((pa.dot(normal)));
}
double StreamTracer::pointToFaceDis(Vector3d coord, Vector3d v0, Vector3d v1, Vector3d v2) {
    Vector3f a(v0[0], v0[1], v0[2]);
    Vector3f b(v1[0], v1[1], v1[2]);
    Vector3f c(v2[0], v2[1], v2[2]);
    Vector3f ab = b - a;
    Vector3f ac = c - a;
    Vector3f normal = ab.cross(ac);
    Vector3f pa = coord - a;
    return abs((pa.dot(normal)));
}
bool StreamTracer::checkContact(const Vector3f& coord, const Vector3f& v0, const Vector3f& v1, const Vector3f& v2) {
    Vector3f d(1.0f, 1.0f, 1.0f);
    d += Vector3f(1e-6f, -2e-6f, 3e-6f); // 防止平行

    const float EPSILON = 1e-7f;

    Vector3f e1 = v1 - v0;
    Vector3f e2 = v2 - v0;

    Vector3f pvec = d.cross(e2);
    float det = e1.dot(pvec);

    if (fabs(det) < EPSILON) return false;

    float invDet = 1.0f / det;

    Vector3f tvec = coord - v0;
    float u = tvec.dot(pvec) * invDet;

    if (u < -EPSILON || u > 1.0f + EPSILON) return false;

    Vector3f qvec = tvec.cross(e1);
    float v = d.dot(qvec) * invDet;

    if (v < -EPSILON || u + v > 1.0f + EPSILON) return false;

    float t = e2.dot(qvec) * invDet;

    return (t > EPSILON);
}
//bool StreamTracer::checkContact(Vector3f coord, Vector3f v0, Vector3f v1, Vector3f v2) {
//    Vector3f p0(v0[0], v0[1], v0[2]);
//    Vector3f p1(v1[0], v1[1], v1[2]);
//    Vector3f p2(v2[0], v2[1], v2[2]);
//    Vector3f d(1, 1, 1);
//    Vector3f e1 = p1 - p0;
//    Vector3f e2 = p2 - p0;
//    Vector3f s = coord - p0;
//    Vector3f s1 = d.cross(e2);
//    Vector3f s2 = s.cross(e1);
//    float t = s2.dot(e2) / (s1.dot(e1));
//    float b1 = s1.dot(s) / (s1.dot(e1));
//    float b2 = s2.dot(d) / (s1.dot(e1));
//    if (t > 0 && b1 > 0 && b2 > 0 && (1 - b1 - b2) > 0) return true;
//    return false;
//}

// Performance optimization implementations
inline double StreamTracer::fastSin(double x) {
    // Convert to integer key for caching (precision to 0.001)
    int key = static_cast<int>(x * 1000);
    auto it = trigCache.sinCache.find(key);
    if (it != trigCache.sinCache.end()) { return it->second; }
    double result = std::sin(x);
    trigCache.sinCache[key] = result;
    return result;
}

inline double StreamTracer::fastTan(double x) {
    int key = static_cast<int>(x * 1000);
    auto it = trigCache.tanCache.find(key);
    if (it != trigCache.tanCache.end()) { return it->second; }
    double result = std::tan(x);
    trigCache.tanCache[key] = result;
    return result;
}

inline double StreamTracer::fastAsin(double x) {
    int key = static_cast<int>(x * 1000);
    auto it = trigCache.asinCache.find(key);
    if (it != trigCache.asinCache.end()) { return it->second; }
    double result = std::asin(x);
    trigCache.asinCache[key] = result;
    return result;
}

inline double StreamTracer::fastSqrt(double x) {
    // Fast inverse square root approximation + Newton iteration
    if (x <= 0.0) return 0.0;

    // Use standard sqrt for now, can optimize with SIMD later
    return std::sqrt(x);
}

void StreamTracer::precomputeTrigValues() {
    // Precompute common trigonometric values
    for (int i = 0; i <= 3142; ++i) { // 0 to π with 0.001 precision
        double angle = i * 0.001;
        trigCache.sinCache[i] = std::sin(angle);
        trigCache.tanCache[i] = std::tan(angle);
        if (angle <= 1.0) { trigCache.asinCache[i] = std::asin(angle); }
    }
}

void StreamTracer::InitAdjacent(iGame::CellArray::Pointer cellData, int vetexNum) {
    igIndex cell[128];
    long long cellNum = cellData->GetNumberOfCells();

    // 清空数据结构
    vetex_link.data.clear();
    vetex_link.offset.clear();
    vetex_link.offset.resize(vetexNum + 1, 0);

    std::cout << "Building adjacency for " << cellNum << " cells and " << vetexNum << " vertices..." << std::endl;

    // === 第一步：分块计算顶点度数，避免一次性分配大内存 ===
    const size_t CHUNK_SIZE = 100000; // 每次处理10万个单元

    for (size_t chunk_start = 0; chunk_start < cellNum; chunk_start += CHUNK_SIZE) {
        size_t chunk_end = std::min(chunk_start + CHUNK_SIZE, (size_t) cellNum);

        for (size_t i = chunk_start; i < chunk_end; i++) {
            int vetex_size = cellData->GetCellIds(i, cell);
            if (i == 0) {
                for (int j = 0; j < vetex_size; ++j) { 
                    std::cout << "cell id::::" << cell[j] << std::endl;
                }
                std::cout << "vetex" << vetex_size << std::endl;
            }
            for (int j = 0; j < vetex_size; j++) {
                if (cell[j] < vetexNum) { vetex_link.offset[cell[j] + 1]++; }
            }
        }

        if ((chunk_start / CHUNK_SIZE) % 10 == 0) {
            std::cout << "Counting progress: " << (chunk_start * 100 / cellNum) << "%" << std::endl;
        }
    }

    // === 第二步：计算前缀和 ===
    for (int i = 1; i <= vetexNum; i++) { vetex_link.offset[i] += vetex_link.offset[i - 1]; }

    size_t totalSize = vetex_link.offset[vetexNum];
    std::cout << "Total adjacency entries needed: " << totalSize << " (approximately "
              << (totalSize * sizeof(igIndex) / 1024 / 1024) << " MB)" << std::endl;

    // === 第三步：使用内存映射或分段分配策略 ===
    try {
        // 尝试分段分配，每段最大256MB
        const size_t MAX_SEGMENT_SIZE = 64 * 1024 * 1024; // 64M entries = ~256MB
        const size_t segmentCount = (totalSize + MAX_SEGMENT_SIZE - 1) / MAX_SEGMENT_SIZE;

        if (segmentCount > 1) {
            std::cout << "Using segmented allocation: " << segmentCount << " segments" << std::endl;
        }

        // 使用reserve来减少重新分配
        vetex_link.data.reserve(totalSize);
        vetex_link.data.resize(totalSize);

        std::cout << "Memory allocation successful!" << std::endl;

    } catch (const std::bad_alloc& e) {
        std::cout << "Critical Error: Cannot allocate memory for adjacency data!" << std::endl;
        std::cout << "Required: " << (totalSize * sizeof(igIndex) / 1024 / 1024) << " MB" << std::endl;
        std::cout << "This model is too large for available system memory." << std::endl;

        // 清理并返回空的邻接结构
        vetex_link.data.clear();
        vetex_link.offset.assign(vetexNum + 1, 0);
        cell_link.offset.assign(cellNum + 1, 0);
        cell_link.data.clear();
        throw std::runtime_error("Insufficient memory for adjacency data");
    }

    // === 第四步：重置偏移量并分块填充数据 ===
    std::vector<long long> fill_offset = vetex_link.offset; // 复制用于填充

    for (size_t chunk_start = 0; chunk_start < cellNum; chunk_start += CHUNK_SIZE) {
        size_t chunk_end = std::min(chunk_start + CHUNK_SIZE, (size_t) cellNum);

        for (size_t i = chunk_start; i < chunk_end; i++) {
            int vetex_size = cellData->GetCellIds(i, cell);
            for (int j = 0; j < vetex_size; j++) {
                if (cell[j] < vetexNum) {
                    size_t pos = fill_offset[cell[j]]++;
                    if (pos < vetex_link.data.size()) { vetex_link.data[pos] = i; }
                }
            }
        }

        if ((chunk_start / CHUNK_SIZE) % 10 == 0) {
            std::cout << "Filling progress: " << (chunk_start * 100 / cellNum) << "%" << std::endl;
            if ((chunk_start / cellNum)<0.2)
            this->UpdateProgress((chunk_start/ cellNum));
        }
    }

    // === 第五步：构建 cell_link（内存高效版本）===
    cell_link.offset.resize(cellNum + 1, 0);
    cell_link.data.clear();

    // 估算cell_link大小并预分配
    const size_t estimatedCellNeighbors = cellNum * 6; // 平均每个单元6个邻居
    cell_link.data.reserve(std::min(estimatedCellNeighbors, totalSize / 2));

    // 分块处理避免内存峰值
    std::unordered_set<igIndex> neighborSet;
    neighborSet.reserve(50); // 预估邻居数

    for (size_t chunk_start = 0; chunk_start < cellNum; chunk_start += CHUNK_SIZE) {
        size_t chunk_end = std::min(chunk_start + CHUNK_SIZE, (size_t) cellNum);

        for (size_t i = chunk_start; i < chunk_end; i++) {
            cell_link.offset[i] = cell_link.data.size();

            int vetex_size = cellData->GetCellIds(i, cell);
            neighborSet.clear();

            for (int j = 0; j < vetex_size; j++) {
                if (cell[j] < vetexNum) {
                    for (size_t k = vetex_link.offset[cell[j]]; k < vetex_link.offset[cell[j] + 1]; k++) {
                        if (k < vetex_link.data.size()) {
                            igIndex neighborCell = vetex_link.data[k];
                            if (neighborCell != i && neighborCell < cellNum) { neighborSet.insert(neighborCell); }
                        }
                    }
                }
            }

            // 添加邻居到全局数据
            for (igIndex neighbor: neighborSet) { cell_link.data.push_back(neighbor); }
        }

        if ((chunk_start / CHUNK_SIZE) % 50 == 0) {
            std::cout << "Cell link progress: " << (chunk_start * 100 / cellNum) << "%" << std::endl;
        }
    }

    cell_link.offset[cellNum] = cell_link.data.size();

    std::cout << "Adjacency building completed successfully!" << std::endl;
    std::cout << "Vertex links: " << vetex_link.data.size() << " entries" << std::endl;
    std::cout << "Cell links: " << cell_link.data.size() << " entries" << std::endl;
    std::cout << "Total memory used: "
              << ((vetex_link.data.size() + cell_link.data.size()) * sizeof(igIndex) / 1024 / 1024) << " MB"
              << std::endl;
}
IGAME_NAMESPACE_END
