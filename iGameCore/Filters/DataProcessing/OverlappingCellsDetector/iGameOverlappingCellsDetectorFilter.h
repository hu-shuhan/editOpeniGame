#pragma once

#include <iGameBoundingBox.h>
#include <iGameFilter.h>
#include <iGameUnstructuredMesh.h>
#include <iGameVolumeMesh.h>

#include <string>
#include <utility>
#include <vector>

IGAME_NAMESPACE_BEGIN

class OverlappingCellsDetectorFilter : public Filter {
public:
    I_OBJECT(OverlappingCellsDetectorFilter);
    static Pointer New() { return new OverlappingCellsDetectorFilter; }

    bool Execute() override;

    /// 与 vtkOverlappingCellsDetector 一致的输出单元标量名称。
    static const char* NumberOfOverlapsPerCellArrayName() { return "NumberOfOverlapsPerCell"; }

    /// 非负公差。非零值要求候选单元具有更明确的共同体积，可降低近接触的误判。
    void SetTolerance(double tolerance) { m_Tolerance = tolerance < 0.0 ? 0.0 : tolerance; }
    double GetTolerance() const { return m_Tolerance; }

    /// 支持 SurfaceMesh / UnstructuredMesh 的三角形、平面凸四边形，或线性体单元
    /// (Tetra、Hexahedron、Prism、Pyramid)。三维 StructuredMesh 在已具备体单元连接关系时
    /// 也可通过 VolumeMesh 路径处理。其他数据对象或单元类型会安全失败。
    const std::string& GetLastError() const { return m_LastError; }

    /// 每个重叠对只记录一次，first < second，均为同一输入网格的单元编号。
    using CellPair = std::pair<igIndex, igIndex>;

    const std::vector<CellPair>& GetOverlappingCellPairs() const {
        return m_OverlappingCellPairs;
    }

    /// 与输出单元标量逐项对应；统计面单元的内部相交或体单元的真实体积重叠。
    const std::vector<igIndex>& GetNumberOfOverlapsPerCell() const {
        return m_NumberOfOverlapsPerCell;
    }

protected:
    OverlappingCellsDetectorFilter();
    ~OverlappingCellsDetectorFilter() override = default;

private:
    UnstructuredMesh::Pointer m_UnstructuredMesh{};
    VolumeMesh::Pointer m_VolumeMesh{};
    double m_Tolerance{0.0};
    std::vector<BoundingBox> m_CellBounds{};
    std::vector<CellPair> m_CandidateCellPairs{};
    std::vector<CellPair> m_OverlappingCellPairs{};
    std::vector<igIndex> m_NumberOfOverlapsPerCell{};
    std::string m_LastError{};
};

IGAME_NAMESPACE_END
