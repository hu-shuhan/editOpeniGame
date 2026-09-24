// Find the integration commit: git log --diff-filter=A --format="%h %s" -- Examples/Filter/Batch2GeometryValidation.cpp
// Integration regressions (fix: feat: integrate second-batch standard filters): importing ResampleToImage without the
// structured ghost-surface dependency rendered invalid samples as a full box.
// Verify hidden, visible, non-hidden flags and malformed masks; also verify
// numeric cell sizes, line interpolation and feature-region boundaries after
// adapting the upstream implementations to this branch's data structures.
#include <CellSize/iGameCellSizeFilter.h>
#include <FeatureExtraction/iGameFeatureEdgeRegionFilter.h>
#include <FeatureExtraction/iGameFeatureEdgesFilter.h>
#include <ModelSurface/iGameModelGeometryFilter.h>
#include <MergeVectorComponents/iGameMergeVectorComponentsFilter.h>
#include <ResampleToLine/iGameResampleToLine.h>
#include <iGameStructuredMesh.h>
#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace iGame;
namespace {
void Check(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}
ArrayObject::Pointer Array(DataObject::Pointer data, const char* name) {
    auto attr = data->GetAttributeSet()->GetAttribute(name);
    Check(!attr.IsNone() && attr.pointer, "missing output attribute");
    return attr.pointer;
}
void CellSizes() {
    auto mesh = UnstructuredMesh::New();
    mesh->AddPoint(Point(0,0,0)); mesh->AddPoint(Point(2,0,0));
    mesh->AddPoint(Point(0,3,0)); mesh->AddPoint(Point(0,0,4));
    igIndex ids[] = {0,1,2,3};
    mesh->AddCell(ids,2,IG_LINE); mesh->AddCell(ids,3,IG_TRIANGLE); mesh->AddCell(ids,4,IG_TETRA);
    auto filter = CellSizeFilter::New(); filter->SetInput(mesh);
    Check(filter->Execute(), "cell_size failed");
    auto output = filter->GetOutput();
    Check(output != mesh, "cell_size modified the input object");
    auto outputMesh = DynamicCast<UnstructuredMesh>(output);
    Check(outputMesh && outputMesh->GetNumberOfPoints() == 4, "cell_size lost output geometry");
    // CellArray::DeepCopy appends offsets; the new destination must be reset
    // before cloning mixed-size cells, or its leading zero shifts every cell.
    for (int cell=0; cell<3; ++cell) {
        const igIndex* copied=nullptr;
        Check(outputMesh->GetCells()->GetCellIds(cell,copied)==cell+2,"cell_size changed cell connectivity");
        for(int i=0;i<cell+2;++i) Check(copied[i]==i,"cell_size changed point IDs");
    }
    Check(std::abs(Array(output,"Length")->GetValue(0)-2) < 1e-6, "line length");
    Check(std::abs(Array(output,"Area")->GetValue(1)-3) < 1e-6, "triangle area");
    Check(std::abs(Array(output,"Volume")->GetValue(2)-4) < 1e-6, "tetrahedron volume");
    Check(std::isnan(Array(output,"Volume")->GetValue(0)), "non-volume cell must have NaN volume");
    Check(mesh->GetAttributeSet()->GetAttribute("Length").IsNone(), "input attributes changed");
}
void LineSamples() {
    auto mesh = UnstructuredMesh::New();
    mesh->AddPoint(Point(0,0,0)); mesh->AddPoint(Point(1,0,0));
    mesh->AddPoint(Point(0,1,0)); mesh->AddPoint(Point(0,0,1));
    igIndex ids[] = {0,1,2,3}; mesh->AddCell(ids,4,IG_TETRA);
    auto field=DoubleArray::New(); field->SetName("linear"); field->SetDimension(1);
    field->AddValue(0); field->AddValue(3); field->AddValue(5); field->AddValue(7);
    mesh->GetAttributeSet()->AddScalar(IG_POINT,field);
    auto filter=ResampleToLine::New(); filter->SetInput(mesh);
    filter->SetOrigTarget(Point(.1f,.1f,.1f),Point(.3f,.1f,.1f)); filter->SetSampleNumber(5);
    Check(filter->Execute(),"line resampling failed");
    auto values=Array(filter->GetOutput(1),"linear");
    Check(filter->GetSampleValidMask().size()==5,"line sample count");
    for(int i=0;i<5;++i) {
        Check(filter->GetSampleValidMask()[i]==1,"interior sample marked invalid");
        Check(std::abs(values->GetValue(i)-(1.5+.15*i))<1e-5,"linear field interpolation");
    }
    filter->SetOrigTarget(Point(2,2,2),Point(3,3,3));
    Check(filter->Execute(),"outside resampling failed");
    for(auto mask:filter->GetSampleValidMask()) Check(mask==0,"outside sample marked valid");
}
// Mixed-size topology exposed the same append-offset issue in vector merging.
// Retain both connectivity and all three numeric components on a new object.
void MergedTopology() {
    auto mesh=UnstructuredMesh::New();
    mesh->AddPoint(Point(0,0,0)); mesh->AddPoint(Point(1,0,0));
    mesh->AddPoint(Point(1,1,0)); mesh->AddPoint(Point(0,1,0));
    igIndex ids[]={0,1,2,3}; mesh->AddCell(ids,3,IG_TRIANGLE); mesh->AddCell(ids,4,IG_QUAD);
    for(const char* name:{"x","y","z"}) {
        auto scalar=DoubleArray::New(); scalar->SetName(name); scalar->SetDimension(1);
        for(int i=0;i<4;++i) scalar->AddValue(i);
        mesh->GetAttributeSet()->AddScalar(IG_POINT,scalar);
    }
    auto filter=MergeVectorComponentsFilter::New(); filter->SetInput(mesh);
    filter->SetComponentArrayNames({"x","y","z"}); filter->SetAttachmentType(IG_POINT);
    filter->SetOutputVectorName("merged");
    Check(filter->Execute(),"vector merging failed");
    auto output=DynamicCast<UnstructuredMesh>(filter->GetOutput());
    Check(output && output!=mesh,"vector merging did not create independent output");
    for(int c=0;c<2;++c) {
        const igIndex* copied=nullptr;
        Check(output->GetCells()->GetCellIds(c,copied)==c+3,"vector merging changed cell size");
        for(int i=0;i<c+3;++i) Check(copied[i]==i,"vector merging changed connectivity");
    }
    auto vector=Array(output,"merged");
    for(int i=0;i<4;++i) for(int d=0;d<3;++d)
        Check(vector->GetElementValue(i,d)==i,"vector component order changed");
}
void FeatureRegions() {
    auto mesh=SurfaceMesh::New();
    auto points=Points::New();
    points->AddPoint(Point(0,0,0)); points->AddPoint(Point(1,0,0));
    points->AddPoint(Point(1,1,0)); points->AddPoint(Point(0,1,0));
    mesh->SetPoints(points);
    igIndex first[]={0,1,2},second[]={0,2,3};
    auto faces=CellArray::New(); faces->AddCellIds(first,3); faces->AddCellIds(second,3);
    mesh->SetFaces(faces);
    auto edges=FeatureEdgesFilter::New(); edges->SetInput(mesh);
    auto regions=FeatureEdgeRegionFilter::New(); regions->SetInput(0,mesh);
    for(bool split:{false,true}) {
        edges->SetManifoldEdges(split);
        Check(edges->Execute(),"feature edges failed");
        regions->SetInput(1,edges->GetOutput());
        Check(regions->Execute(),"feature region IDs failed");
        auto output=DynamicCast<SurfaceMesh>(regions->GetOutput());
        Check(output && output->GetNumberOfFaces()==2,"region output connectivity changed");
        auto ids=Array(output,"Region Id");
        Check((ids->GetValue(0)!=ids->GetValue(1))==split,"incorrect feature boundary grouping");
    }
    Check(mesh->GetAttributeSet()->GetAttribute("Region Id").IsNone(),"region filter changed input");
    auto malformed=UnstructuredMesh::New(); malformed->SetPoints(mesh->GetPoints());
    igIndex edge[]={0,1}; malformed->AddCell(edge,2,IG_LINE);
    regions->SetInput(1,malformed);
    Check(!regions->Execute() && !regions->GetOutput(),"missing edge IDs must fail without stale output");
}
void GhostSurface() {
    StructuredMesh::Pointer mesh=StructuredMesh::New();
    igIndex size[]={3,2,2}; mesh->SetDimensionSize(size);
    auto points=Points::New();
    for(int k=0;k<2;++k) for(int j=0;j<2;++j) for(int i=0;i<3;++i) points->AddPoint(Point(i,j,k));
    mesh->SetPoints(points); mesh->GenStructuredCellConnectivities();
    auto mask=UnsignedCharArray::New(); mask->SetName("vtkGhostType"); mask->SetDimension(1);
    mask->AddValue(0); mask->AddValue(0); mesh->GetAttributeSet()->AddScalar(IG_CELL,mask);
    auto geometry=ModelGeometryFilter::New();
    auto faces=[&]() {
        auto surface=SurfaceMesh::New();
        Check(geometry->Execute(mesh,surface),"structured surface extraction failed");
        return surface->GetNumberOfFaces();
    };
    Check(faces()==10,"two visible voxels must have ten exterior quads");
    mask->SetValue(0,32); Check(faces()==6,"hidden voxel must expose its visible neighbour");
    mask->SetValue(1,32); Check(faces()==0,"all hidden voxels must produce no surface");
    mask->SetValue(0,16); mask->SetValue(1,0); Check(faces()==10,"non-hidden flags must not blank voxels");
    mask->Resize(1); Check(faces()==10,"truncated mask must not be read out of bounds");
}
}
int main() {
    try {
        std::cerr << "Checking cell sizes...\n"; CellSizes();
        std::cerr << "Checking line samples...\n"; LineSamples();
        std::cerr << "Checking merged topology...\n"; MergedTopology();
        std::cerr << "Checking feature regions...\n"; FeatureRegions();
        std::cerr << "Checking ghost surfaces...\n"; GhostSurface();
        std::cout<<"PASS: cell sizes, line sampling, region IDs and structured ghost surfaces.\n";
        return 0;
    } catch(const std::exception& error) {
        std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1;
    }
}
