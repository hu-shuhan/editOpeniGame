#ifndef CellSizeFilter_h
#define CellSizeFilter_h

#include "iGameCell.h"
#include "iGameFilter.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"
#include <vector>

IGAME_NAMESPACE_BEGIN

// DIME Filter: cell_size
// Compute geometric size of each cell: 1D line -> length, 2D face -> area, 3D volume -> volume
// Outputs IG_CELL scalar attributes "Length"/"Area"/"Volume" on an INDEPENDENT output node
// (deep copy of the input; the input model is never modified).
// Semantics: each cell fills only the attribute matching its dimension; non-matching
// dimensions are written as NaN to distinguish "not applicable" from a real 0.
class CellSizeFilter : public Filter {
public:
    I_OBJECT(CellSizeFilter);
    static Pointer New() { return new CellSizeFilter; }

    bool Execute() override;
    std::string GetMessage() const { return m_Message; }
    // Number of cells processed by the last successful Execute()
    int GetComputedCellCount() const { return m_ComputedCellCount; }

protected:
    CellSizeFilter() {
        SetNumberOfInputs(1);
        SetNumberOfOutputs(1);
    }
    ~CellSizeFilter() override = default;

    // Dispatch by cellType if available, otherwise by dim+vNum
    static double ComputeCellSize(IGenum cellType, int dim, const std::vector<Point>& points);

    // 1D: line length
    static double ComputeLineLength(const std::vector<Point>& points);

    // 2D: area
    static double ComputeTriangleArea(const std::vector<Point>& points);
    static double ComputeQuadArea(const std::vector<Point>& points);
    static double ComputePolygonArea(const std::vector<Point>& points);

    // 3D: volume
    static double ComputeTetVolume(const std::vector<Point>& points);
    static double ComputeHexVolume(const std::vector<Point>& points);
    static double ComputePyramidVolume(const std::vector<Point>& points);
    static double ComputePrismVolume(const std::vector<Point>& points);
    // Signed volume of a tetrahedron
    static double ComputeTetSignedVolume(const Point& p0, const Point& p1,
                                         const Point& p2, const Point& p3);

    CellArray::Pointer m_Cells = nullptr;
    Points::Pointer m_Points = nullptr;
    std::string m_Message;
    int m_ComputedCellCount = 0;
};

IGAME_NAMESPACE_END
#endif
