#pragma once

#include <iGameFilter.h>
#include <iGameUnstructuredMesh.h>

#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

/// Extracts the first linear volume cell that contains a requested spatial location,
/// matching ParaView's "Extract Cell At Location" lookup semantics.
/// The output uses the existing UnstructuredMesh data model and stores
/// vtkOriginalPointIds/vtkOriginalCellIds in signed 64-bit arrays for
/// traceability to the input mesh.
class ExtractLocationFilter : public Filter {
public:
    I_OBJECT(ExtractLocationFilter);
    static Pointer New() { return new ExtractLocationFilter; }

    bool Execute() override;

    void SetLocation(double x, double y, double z) { m_Location = Point(x, y, z); }
    void SetLocation(const Point& location) { m_Location = location; }
    const Point& GetLocation() const { return m_Location; }

    static const char* OriginalCellIdsArrayName() { return "vtkOriginalCellIds"; }
    static const char* OriginalPointIdsArrayName() { return "vtkOriginalPointIds"; }

    /// Contains zero or one cell id. Empty is valid when the location is outside every cell.
    const std::vector<igIndex>& GetExtractedCellIds() const { return m_ExtractedCellIds; }
    const std::string& GetLastError() const { return m_LastError; }

protected:
    ExtractLocationFilter();
    ~ExtractLocationFilter() override = default;

private:
    UnstructuredMesh::Pointer m_InputMesh{};
    UnstructuredMesh::Pointer m_OutputMesh{};
    Point m_Location{0.0, 0.0, 0.0};
    std::vector<igIndex> m_ExtractedCellIds{};
    std::string m_LastError{};
};

IGAME_NAMESPACE_END
