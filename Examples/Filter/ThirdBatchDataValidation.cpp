// Find the integration commit: git log --diff-filter=A --format="%h %s" -- Examples/Filter/ThirdBatchDataValidation.cpp
// Regression: CellArray::DeepCopy appended to the constructor's initial zero.
// Mixed triangle/quad offsets shifted, corrupting copied geometry and centroids
// in AppendLocation, GenerateIds, CountCellFaces, MeshQuality and Shrink.
// Test exact connectivity, repeated/self copy, source independence, coordinates,
// areas and normal lengths. Homogeneous cells alone do not exercise the bug.
// Fix: feat: integrate third-batch standard filters.
#include <AppendLocationAttribute/iGameAppendLocationAttribute.h>
#include <FeatureExtraction/iGameCountCellFacesFilter.h>
#include <GenerateIds/iGameGenerateIdsFilter.h>
#include <MeshQuality/iGameMeshQualityFilter.h>
#include <Shrink/iGameShrinkFilter.h>
#include <SurfaceNormals/iGameSurfaceNormalsFilter.h>
#include <iGameSurfaceMesh.h>
#include <iGameFileIO.h>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace iGame;
namespace {
void Check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
void Near(double a, double b, const char* message) { Check(std::abs(a-b) < 1e-6, message); }
SurfaceMesh::Pointer Fixture() {
    auto mesh = SurfaceMesh::New();
    const Point points[] = {{0,0,0},{1,0,0},{1,1,0},{0,1,0},{2,0,0}};
    for (auto p : points) mesh->GetPoints()->AddPoint(p);
    auto faces = CellArray::New();
    igIndex quad[] = {0,1,2,3}, triangle[] = {1,4,2};
    faces->AddCellIds(quad,4); faces->AddCellIds(triangle,3); mesh->SetFaces(faces);
    return mesh;
}
void SameCells(CellArray* actual, CellArray* expected) {
    Check(actual->GetNumberOfCells() == expected->GetNumberOfCells(), "cell count changed");
    for (IGsize i=0;i<expected->GetNumberOfCells();++i) {
        const igIndex *a, *b;
        int n = actual->GetCellIds(i,a), m = expected->GetCellIds(i,b);
        Check(n == m, "copied cell size changed");
        for (int j=0;j<n;++j) Check(a[j] == b[j], "copied connectivity changed");
    }
}
template<class F> DataObject::Pointer Run(F filter, DataObject::Pointer input) {
    filter->SetInput(input); Check(filter->Execute(), "filter failed");
    auto output=filter->GetOutput(); Check(output && output != input, "output aliases source"); return output;
}
}
int main() {
    try {
        // VTK section reuse used to erase the zero offset and overwrite LINES
        // when POLYGONS followed it. Preserve both independent cell arrays.
        auto loaded=DynamicCast<SurfaceMesh>(FileIO::ReadFile("Models/Batch3MixedLinesAndFaces.vtk"));
        Check(loaded && loaded->GetNumberOfEdges()==1 && loaded->GetNumberOfFaces()==2,
              "VTK mixed topology or separate line section was corrupted");
        Check(loaded->GetEdges()!=loaded->GetFaces(),"VTK line/face connectivity aliases");
        Check(loaded->GetEdges()->GetCellSize(0)==2,"VTK line connectivity changed");
        Check(loaded->GetFaces()->GetCellSize(0)==4 && loaded->GetFaces()->GetCellSize(1)==3,
              "VTK mixed face offsets changed");
        auto mesh=Fixture();
        auto copy=CellArray::New();
        for (int i=0;i<2;++i) { Check(copy->DeepCopy(mesh->GetFaces()), "copy failed"); SameCells(copy,mesh->GetFaces()); }
        Check(copy->DeepCopy(copy), "self copy failed"); SameCells(copy,mesh->GetFaces());
        auto located=DynamicCast<SurfaceMesh>(Run(AppendLocationAttribute::New(),mesh));
        Check(located, "location output is not surface"); SameCells(located->GetFaces(),mesh->GetFaces());
        auto location=located->GetAttributeSet()->GetAttribute("LocationAttribute").pointer;
        auto centers=located->GetAttributeSet()->GetAttribute("CellCenter").pointer;
        Check(location && centers, "location arrays missing");
        Near(location->GetElementValue(4,0),2,"point coordinate is wrong");
        Near(centers->GetElementValue(0,0),0.5,"quad center is wrong");
        Near(centers->GetElementValue(1,0),4.0/3,"triangle center is wrong");
        auto counted=Run(CountCellFacesFilter::New(),mesh);
        SameCells(DynamicCast<SurfaceMesh>(counted)->GetFaces(),mesh->GetFaces());
        auto generated=DynamicCast<UnstructuredMesh>(Run(iGameGenerateIdsFilter::New(IG_CELL),mesh));
        Check(generated,"ID output missing"); SameCells(generated->GetCells(),mesh->GetFaces());
        auto quality=MeshQualityFilter::New();
        auto measured=DynamicCast<SurfaceMesh>(Run(quality,mesh));
        Check(measured,"quality output missing"); SameCells(measured->GetFaces(),mesh->GetFaces());
        Near(quality->GetMinimum(),0.5,"triangle area incorrect");
        Near(quality->GetMaximum(),1,"quad area incorrect");
        auto shrunk=DynamicCast<SurfaceMesh>(Run(ShrinkFilter::New(),mesh));
        Check(shrunk && shrunk->GetNumberOfPoints()==7,"shrink dropped mixed cells");
        Check(shrunk->GetFaces()->GetCellSize(0)==4 && shrunk->GetFaces()->GetCellSize(1)==3,"shrink changed cell size");
        auto normalFilter=SurfaceNormalsFilter::New(); normalFilter->SetSplitting(false);
        auto normals=Run(normalFilter,mesh)->GetAttributeSet()->GetAttribute("Normals").pointer;
        Check(normals && normals->GetDimension()==3,"normals missing");
        for (IGsize i=0;i<normals->GetNumberOfElements();++i) {
            double value[3]; normals->GetElement(i,value);
            Near(value[0]*value[0]+value[1]*value[1]+value[2]*value[2],1,"normal is not unit length");
        }
        Check(mesh->GetAttributeSet()->GetNumberOfAttributes()==0,"source attributes modified");
        Near(mesh->GetPoint(0)[0],0,"source position modified");
        // ID filters produce 64-bit integers. Shrink must duplicate them without
        // converting through double (which loses low bits beyond 2^53).
        auto ids=LongLongArray::New(); ids->SetName("ExactIds");
        const long long base=(1LL<<53)+5;
        for (int i=0;i<5;++i) ids->AddValue(base+i);
        mesh->GetAttributeSet()->AddScalar(IG_POINT,ids);
        auto withIds=Run(ShrinkFilter::New(),mesh);
        auto copiedIds=DynamicCast<LongLongArray>(withIds->GetAttributeSet()->GetAttribute("ExactIds").pointer);
        Check(copiedIds && copiedIds->GetNumberOfElements()==7,"shrink dropped integer IDs");
        const int mapping[]={0,1,2,3,1,4,2};
        for(int i=0;i<7;++i) Check(copiedIds->ValueAt(i)==base+mapping[i],"shrink rounded integer IDs");
        std::cout<<"PASS: mixed topology, repeated copying, locations, IDs, areas, shrink and normals.\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<"FAIL: "<<e.what()<<'\n'; return 1; }
}
