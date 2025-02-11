#ifndef iGameModelGeometryFilter_h
#define iGameModelGeometryFilter_h

#include "iGameDataObject.h"
#include "iGameFilter.h"
#include "iGameSurfaceMesh.h"
#include "iGameVolumeMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameStructuredMesh.h"
IGAME_NAMESPACE_BEGIN

struct ExtractCellBoundaries;
class iGameModelGeometryFilter : public Filter {
public:
	I_OBJECT(iGameModelGeometryFilter);
	static iGameModelGeometryFilter::Pointer New() {
		return new iGameModelGeometryFilter;
	};
	~iGameModelGeometryFilter();
	bool Execute() override;
	bool Execute(DataObject::Pointer);
	bool Execute(DataObject::Pointer, SurfaceMesh::Pointer&);
	/**
	 * Specify a (xmin,xmax, ymin,ymax, zmin,zmax) bounding box to clip data.
	 */
	void SetExtent(double xMin, double xMax, double yMin, double yMax,
		double zMin, double zMax, bool flip = false);
	void SetExtent(double ex[6], bool flip = false);

	/**
	 * Specify a plane to clip data.
	 */
	void SetClipPlane(double ox, double oy, double oz, double nx, double ny, double nz, bool flip = false);
	void SetClipPlane(double orgin[3], double normal[3], bool flip = false);

	///@{

	void SetPointIndexExtent(igIndex _min, igIndex _max);
	void SetPointIndexMinimum(igIndex _min);
	void SetPointIndexMaximum(igIndex _max);
	void SetCellIndexExtent(igIndex _min, igIndex _max);
	void SetCellIndexMinimum(igIndex _min);
	void SetCellIndexMaximum(igIndex _max);
	/**
	 处理不同的网格
	 */
	int ExecuteWithSurfaceMesh(DataObject::Pointer input, SurfaceMesh::Pointer& output,
		SurfaceMesh::Pointer exc);
	virtual int ExecuteWithSurfaceMesh(DataObject::Pointer, SurfaceMesh::Pointer&);

	int ExecuteWithVolumeMesh(DataObject::Pointer input, SurfaceMesh::Pointer& output,
		SurfaceMesh::Pointer exc);
	virtual int ExecuteWithVolumeMesh(DataObject::Pointer input,
		SurfaceMesh::Pointer& output);
	int ExecuteWithUnstructuredGrid(DataObject::Pointer input, SurfaceMesh::Pointer& output,
		SurfaceMesh::Pointer exc);
	virtual int ExecuteWithUnstructuredGrid(DataObject::Pointer input,
		SurfaceMesh::Pointer& output);

	int ExecuteWithStructuredGrid(DataObject::Pointer input, SurfaceMesh::Pointer& output,
		SurfaceMesh::Pointer exc, bool* extractFace = nullptr);
	virtual int ExecuteWithStructuredGrid(DataObject::Pointer input,
		SurfaceMesh::Pointer& output,
		bool* extractFace = nullptr);


	void SetInput(DataObject::Pointer ip) { this->input = ip; }
	SurfaceMesh::Pointer GetExtractMesh() { return this->output; }
	DataObject::Pointer GetOutput() override { return this->output; }
	void CompositeCellAttribute(std::vector<igIndex>& f2c, AttributeSet::Pointer inAllDataArray, AttributeSet::Pointer& outAllDataArray);
	//这边直接对attributeset进行处理，不用再copy一个，因为传进去的已经是一个对cellattributeset处理过的对象
	void CompositePointAttribute(igIndex* PointMap, IGsize oldPNum, IGsize newPNum, AttributeSet::Pointer outAllDataArray);

	void SetPointClipping(bool _b) { this->PointClipping = _b; }
	void SetCellClipping(bool _b) { this->CellClipping = _b; }
	void SetExtentClipping(bool _b) { this->ExtentClipping = _b; }
	void SetPlaneClipping(bool _b) { this->PlaneClipping = _b; }
	void SetPointMergin(bool _b) { this->Merging = _b; }
	void SetMaxThreadSize(int _b) { this->MaxThreadSize = _b; }
	char* ComputeCellVisibleArray(CharArray::Pointer& CellVisibleArray, Points::Pointer inPoints, CellArray::Pointer Cells);
	void ProcessPointMergin(ExtractCellBoundaries* extract,
		Points::Pointer inPoints, Points::Pointer& outPoints, CellArray::Pointer Polygons,
		AttributeSet::Pointer outAllDataArray);
	FlatArray<igIndex>::Pointer GetPointMap() {return m_PointMap;}
protected:
	iGameModelGeometryFilter();
	//有时候在文件里会有标注表面信息，如果有则不需要这边运算，
	//只需要把attribute的信息copy一份给表面就可以，暂时没有完善这个功能.
	SurfaceMesh::Pointer excFaces;

	DataObject::Pointer input;
	SurfaceMesh::Pointer output;
	igIndex PointMaximum;
	igIndex PointMinimum;
	igIndex CellMinimum;
	igIndex CellMaximum;
	std::vector<Vector4d>CutPlanes;
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
