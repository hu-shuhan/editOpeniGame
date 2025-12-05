#ifndef iGameModelGeometryFilter_h
#define iGameModelGeometryFilter_h

#include "iGameDataObject.h"
#include "iGameFilter.h"
#include "iGameStructuredMesh.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"
IGAME_NAMESPACE_BEGIN

struct ExtractCellBoundaries;
class ModelGeometryFilter : public Filter {
public:
    I_OBJECT(ModelGeometryFilter);
    static ModelGeometryFilter::Pointer New() { return new ModelGeometryFilter; };
    ~ModelGeometryFilter();
    bool Execute() override;
    bool Execute(DataObject::Pointer);
    bool Execute(DataObject::Pointer, SurfaceMesh::Pointer&);
    /**
     * Specify a (xmin,xmax, ymin,ymax, zmin,zmax) bounding box to clip data.
     */
    void SetExtent(double xMin, double xMax, double yMin, double yMax, double zMin, double zMax, bool flip = false);
    void SetExtent(double ex[6], bool flip = false);

    /**
     * Specify a plane to clip data.
     */
    void SetClipPlane(double ox, double oy, double oz, double nx, double ny, double nz, bool flip = false);
    void SetClipPlane(double orgin[3], double normal[3], bool flip = false);

    ///@{ 设置点区间或者cell区间
    void SetPointIndexExtent(igIndex _min, igIndex _max);
    void SetPointIndexMinimum(igIndex _min);
    void SetPointIndexMaximum(igIndex _max);
    void SetCellIndexExtent(igIndex _min, igIndex _max);
    void SetCellIndexMinimum(igIndex _min);
    void SetCellIndexMaximum(igIndex _max);
    ///@}

    /**
    * 对不同的网格进行对应的抽壳算法，
    * 由于用户可能需要特定的输出网格指针，因此该算法需要输入输出网格的指针，不可为空指针
    */
    int ExecuteWithSurfaceMesh(DataObject::Pointer input, SurfaceMesh::Pointer& output, SurfaceMesh::Pointer exc);
    virtual int ExecuteWithSurfaceMesh(DataObject::Pointer, SurfaceMesh::Pointer&);
    int ExecuteWithVolumeMesh(DataObject::Pointer input, SurfaceMesh::Pointer& output, SurfaceMesh::Pointer exc);
    virtual int ExecuteWithVolumeMesh(DataObject::Pointer input, SurfaceMesh::Pointer& output);
    int ExecuteWithUnstructuredMesh(DataObject::Pointer input, SurfaceMesh::Pointer& output, SurfaceMesh::Pointer exc);
    virtual int ExecuteWithUnstructuredMesh(DataObject::Pointer input, SurfaceMesh::Pointer& output);
    int ExecuteWithStructuredMesh(DataObject::Pointer input, SurfaceMesh::Pointer& output, SurfaceMesh::Pointer exc,
                                  bool* extractFace = nullptr);
    virtual int ExecuteWithStructuredMesh(DataObject::Pointer input, SurfaceMesh::Pointer& output,
                                          bool* extractFace = nullptr);


    void SetInput(DataObject::Pointer ip) { this->input = ip; }
    //返回转化后的抽壳网格，为表面网格
    SurfaceMesh::Pointer GetExtractMesh() { return this->output; }
    //返回抽壳网格的基类指针
    DataObject::Pointer GetOutput() override { return this->output; }
    //处理cell的数据场，f2c表示的是face对应的cell的index
    void CompositeCellAttribute(std::vector<igIndex>& f2c, AttributeSet::Pointer inAllDataArray,
                                AttributeSet::Pointer& outAllDataArray);
    //这边直接对attributeset进行处理，不用再copy一个，因为传进去的已经是一个对cellattributeset处理过的对象
    void CompositePointAttribute(igIndex* PointMap, IGsize oldPNum, IGsize newPNum,
                                 AttributeSet::Pointer outAllDataArray);

    void SetPointClipping(bool _b) { this->PointClipping = _b; }
    void SetCellClipping(bool _b) { this->CellClipping = _b; }
    void SetExtentClipping(bool _b) { this->ExtentClipping = _b; }
    void SetPlaneClipping(bool _b) { this->PlaneClipping = _b; }
    void SetPointMerging(bool _b) { this->Merging = _b; }
    void SetMaxThreadSize(int _b) { this->MaxThreadSize = _b; }

    FlatArray<igIndex>::Pointer GetPointMap() { return m_PointMap; }

private:
    char* ComputeCellVisibleArray(CharArray::Pointer& CellVisibleArray, Points::Pointer inPoints,
                                  CellArray::Pointer Cells, UnsignedIntArray::Pointer Types = nullptr);
    void ProcessPointMergin(ExtractCellBoundaries* extract, Points::Pointer inPoints, Points::Pointer& outPoints,
                            CellArray::Pointer Polygons, AttributeSet::Pointer outAllDataArray);

protected:
    ModelGeometryFilter();
    //有时候在文件里会有标注表面信息，如果有则不需要这边运算，
    //只需要把attribute的信息copy一份给表面就可以，暂时没有完善这个功能.
    SurfaceMesh::Pointer excFaces;

    DataObject::Pointer input;
    SurfaceMesh::Pointer output;
    igIndex PointMaximum;
    igIndex PointMinimum;
    igIndex CellMinimum;
    igIndex CellMaximum;
    std::vector<Vector4d> CutPlanes;
    double Extent[6];
    double PlaneOrigin[3], PlaneNormal[3];
    bool PointClipping;
    bool CellClipping;
    bool ExtentClipping;
    bool PlaneClipping;

    //maybe remain inside or outside, temporary not used.
    bool ExtentClippingFlip;
    bool PlaneClippingFlip;

    //maybe exist ghost data
    bool RemoveGhostInterfaces;

    int MaxThreadSize = 1024;

public:
    //Point merging
    bool Merging;

    //m_PointMap->GetValue(i)表示的是第i个new point对应的origin point id
    FlatArray<igIndex>::Pointer m_PointMap = nullptr;

private:
};
IGAME_NAMESPACE_END
#endif
