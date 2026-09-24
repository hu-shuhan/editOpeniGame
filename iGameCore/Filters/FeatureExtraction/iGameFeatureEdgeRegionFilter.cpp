#include "iGameFeatureEdgeRegionFilter.h"
#include <vector>
#include <unordered_set>
#include <utility>
#include <algorithm>
#include <map>

IGAME_NAMESPACE_BEGIN
namespace{
    class UnionFind {
    public:
        std::vector<int> parent;

        UnionFind(int numFaces) {
            parent = std::vector<int>(numFaces, -1);
            for (int faceId = 0; faceId < numFaces; faceId++) {
                parent[faceId] = faceId;
            } // Each face starts as its own parent.
        }

        void Union(int faceId1, int faceId2) {
            int root1 = FindParent(faceId1);
            int root2 = FindParent(faceId2);
            if (root1 != root2) { parent[root1] = root2; }
        }

        int FindParent(int id) {
            if (parent[id] != id) { parent[id] = FindParent(parent[id]); }
            return parent[id];
        }
    };
} // namespace


bool FeatureEdgeRegionFilter::Execute() {
    SetOutput(nullptr);
	auto obj = GetInput(0);//Get the original mesh
    if (obj == nullptr) {
        std::cerr << "Failed to get input mesh" << std::endl;
        return false;
    }
    auto inputMesh = DynamicCast<SurfaceMesh>(obj);
    if (inputMesh == nullptr) {
        std::cerr << "Failed to get input surface mesh" << std::endl;
        return false;
    }
    if (!inputMesh->GetPoints() || !inputMesh->GetFaces()) return false;
    auto mesh = SurfaceMesh::New();
    auto points = Points::New();
    points->DeepCopy(inputMesh->GetPoints());
    mesh->SetPoints(points);
    auto faces = CellArray::New();
    faces->GetOffset()->Reset(); // 清除构造时预置的 0
    if (!faces->DeepCopy(inputMesh->GetFaces())) { return false; }
    mesh->SetFaces(faces);
    auto attributeSet = AttributeSet::New();
    attributeSet->DeepCopy(inputMesh->GetAttributeSet());
    mesh->SetAttributeSet(attributeSet);
    /*mesh->SetPoints(inputMesh->GetPoints());
    mesh->SetFaces(inputMesh->GetFaces());
    mesh->SetAttributeSet(inputMesh->GetAttributeSet());*/

    mesh->BuildEdges();
    mesh->BuildEdgeLinks();
    mesh->BuildFaceEdgeLinks();

    auto featureObj = GetInput(1);
    if (featureObj == nullptr) {
        std::cerr << "Failed to get featrue obj" << std::endl;
        return false;
    }
    auto featureMesh = DynamicCast<UnstructuredMesh>(featureObj);
    if (featureMesh == nullptr) {
        std::cerr << "Failed to get featrue mesh" << std::endl;
        return false;
    }
   auto featureCells = featureMesh->GetCells();
    if (featureCells == nullptr) {
        std::cerr << "Failed to get featrue cells" << std::endl;
        return false;
    }

    //std::set<std::pair<igIndex,igIndex>> featureEdgePointPairs;
    //igIndex linePts[2]{};
    //for (int lineId = 0; lineId < featureCells->GetNumberOfCells(); ++lineId) {//traverse every edge in feature Mesh.Each cell is a line with two points
    //    int count = featureCells->GetCellIds(lineId, linePts);
    //    if (count != 2) continue;
    //    igIndex edgeId = mesh->GetEdgeIdFormPointIds(linePts[0], linePts[1]);//make sure the edge exist in origin mesh
    //    if (edgeId != -1) {
    //        igIndex pt1 = std::min(linePts[0], linePts[1]);
    //        igIndex pt2 = std::max(linePts[0], linePts[1]);
    //        featureEdgePointPairs.insert(std::pair<igIndex, igIndex>(pt1,pt2)); }
    //}

    auto edgeIdAttribute = featureMesh->GetAttributeSet()->GetAttribute("Edge Ids");

    UnsignedIntArray::Pointer edgeIdArray = UnsignedIntArray::New();

    if (!edgeIdAttribute.IsNone()) {
        edgeIdArray = DynamicCast<UnsignedIntArray>(edgeIdAttribute.pointer);
    }

    if (!edgeIdArray || edgeIdArray->GetDimension() != 1 ||
        edgeIdArray->GetNumberOfValues() != featureCells->GetNumberOfCells()) return false;

    std::unordered_set<igIndex> featureEdgeIds;
    for (int lineId = 0; lineId < featureCells->GetNumberOfCells(); lineId++) {//traverse every feature edge in feature Mesh.
        featureEdgeIds.insert(edgeIdArray->GetValue(lineId));
    }

    const int numFaces = mesh->GetNumberOfFaces();
    UnionFind myUnion(numFaces);

    const int numEdges = mesh->GetNumberOfEdges();

    for (int edgeId = 0; edgeId < numEdges; edgeId++) {
        if (featureEdgeIds.count(edgeId)) continue; //continue if the edge is feature edge
        //union if the edge is not feature edge
        igIndex faceIds[IGAME_CELL_MAX_SIZE]{};
        int faceCount = mesh->GetEdgeToNeighborFaces(edgeId, faceIds);
        if (faceCount==2)myUnion.Union(faceIds[0], faceIds[1]); //union neighbor faces
    }

    //realign the region IDs
    std::map<IGint,IGint> regionIDs;
    auto regionArray = IntArray::New(); //save region ids
    regionArray->SetName("Region Id");
    regionArray->SetDimension(1);
    regionArray->Reserve(numFaces);
    int p = 0;
    for (int i = 0; i < numFaces; i++) {
        int regionID = myUnion.FindParent(i);
        auto it = regionIDs.find(regionID);
        if (it == regionIDs.end()) {//this parent haven't be save in regionIDs
            regionIDs[regionID]=p;
            p++;
            if (p >= std::numeric_limits<IGint>::max()) {
                std::cerr << "Too many regions for IGint RegionId." << std::endl;
                return false;
            }
        }
        regionArray->AddValue(regionIDs[regionID]);
    }

    //auto attributeSet = mesh->GetAttributeSet();
    if (!attributeSet) { return false; }
    auto oldAttributeId = attributeSet->GetAttributeIndex("Region Id");
    if (oldAttributeId >= 0) {
        auto oldArray = DynamicCast<IntArray>(attributeSet->GetAttribute("Region Id").pointer);
        if (oldArray == nullptr) {
            std::cerr << "Region Id already exists, but it is not an IntArray." << std::endl;
            return false;
        }
        oldArray->SetDimension(1);
        oldArray->Resize(numFaces);
        for (int i = 0; i < numFaces; ++i) { oldArray->SetValue(i, regionArray->GetValue(i)); }
        oldArray->Modified();
        attributeSet->GetAttribute("Region Id").UpdateAllDataRange();
    } else {
        attributeSet->AddScalar(IG_CELL, regionArray);
    }
    attributeSet->ForceReConvertToDrawableData();
    mesh->SetName(inputMesh->GetName() + "_regionId");

    std::map<int, int> regionFaceCount;
    for (int i = 0; i < numFaces; i++) {
        int root = myUnion.FindParent(i);
        int regionId = regionIDs[root];
        regionFaceCount[regionId]++;
    }
    //for (auto& item: regionFaceCount) {//print id and faces number of every region
    //    std::cout << "Region " << item.first << " faces: " << item.second << std::endl;
    //}
    std::cout << "Number of regions:" << p<<std::endl;
    SetOutput(mesh);
    return true;

}
IGAME_NAMESPACE_END
