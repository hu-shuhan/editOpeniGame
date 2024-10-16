#ifndef iGameModelGeometryFilter_h
#define iGameModelGeometryFilter_h

#include "iGameDataObject.h"
#include "iGameFilter.h"
#include "iGameSurfaceMesh.h"
#include "iGameVolumeMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameStructuredMesh.h"
IGAME_NAMESPACE_BEGIN


class iGameModelGeometryFilter : public Filter {
public:
	I_OBJECT(iGameModelGeometryFilter);
	///@{
	/**
	 * Standard methods for instantiation, type information, and printing.
	 */
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
	 * Direct access methods so that this class can be used as an
	 * algorithm without using it as a filter (i.e., no pipeline updates).
	 * Also some internal methods with additional options.
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

	///@}
	void SetInput(DataObject::Pointer ip) { this->input = ip; }
	SurfaceMesh::Pointer GetExtractMesh() { return this->output; }

	void CompositeCellAttribute(std::vector<igIndex>& f2c, AttributeSet::Pointer inAllDataArray, AttributeSet::Pointer& outAllDataArray);
    void CompositePointAttribute(igIndex* PointMap,IGsize oldPNum,IGsize newPNum,AttributeSet::Pointer inAllDataArray);

	void SetPointClipping(bool _in) { this->PointClipping = _in; }
	void SetCellClipping(bool _in) { this->CellClipping = _in; }
	void SetExtentClipping(bool _in) { this->ExtentClipping = _in; }
	void SetPlaneClipping(bool _in) { this->PlaneClipping = _in; }


	char* ComputeCellVisibleArray(CharArray::Pointer& CellVisibleArray, Points::Pointer inPoints, CellArray::Pointer Cells);
protected:
	iGameModelGeometryFilter();
	//有时候在文件里会有标注表面信息，如果有则不需要这边运算，
	//只需要把attribute的信息copy一份给表面就可以
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
	bool ExtentClippingFlip;
	bool PlaneClippingFlip;

	int OutputPointsPrecision;

	bool RemoveGhostInterfaces;
	//Point merging
public:
	bool Merging;

	bool Delegation;


private:
};
IGAME_NAMESPACE_END
#endif
