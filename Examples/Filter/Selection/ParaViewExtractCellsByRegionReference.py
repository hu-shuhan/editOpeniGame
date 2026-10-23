"""Reproduce the Box/Sphere inside-extraction baseline in ParaView 6.2.

Regression trigger: the old example used Tet_Plane and did not exercise a cell
crossing the region boundary. The correct behavior is verified against the
actual ParaView filter for both values of Extractintersected, not inferred from
the iGameVis implementation. Each fixture cell owns four private points, so
the first point uniquely identifies its original cell ID.

Fix and test first land together in the commit titled
"feat: reproduce ParaView region extraction workflow". Find its real SHA with:
git log --diff-filter=A --format="%h %s" -- Examples/Filter/Selection/ParaViewExtractCellsByRegionReference.py
"""

from pathlib import Path

from paraview import servermanager
from paraview.simple import ExtractCellsByRegion, LegacyVTKReader


MODELS = Path(__file__).resolve().parents[2] / "Models"
CASES = (
    ("ExtractCellsByRegion_BoxCases.vtk", "Box", 0, [0]),
    ("ExtractCellsByRegion_BoxCases.vtk", "Box", 1, [0, 1, 3]),
    ("ExtractCellsByRegion_BoxCases.vtk", "Sphere", 0, [0]),
    ("ExtractCellsByRegion_BoxCases.vtk", "Sphere", 1, [0, 1, 3]),
    ("ExtractCellsByRegion_SphereCases.vtk", "Box", 0, [0, 4]),
    ("ExtractCellsByRegion_SphereCases.vtk", "Box", 1, [0, 1, 3, 4]),
    ("ExtractCellsByRegion_SphereCases.vtk", "Sphere", 0, [0]),
    ("ExtractCellsByRegion_SphereCases.vtk", "Sphere", 1, [0, 1, 3]),
)


def first_point(cell):
    return tuple(round(value, 5) for value in cell.GetPoints().GetPoint(0))


for filename, shape, include_intersected, expected in CASES:
    reader = LegacyVTKReader(FileNames=[str(MODELS / filename)])
    source = servermanager.Fetch(reader)
    cell_by_first_point = {
        first_point(source.GetCell(index)): index
        for index in range(source.GetNumberOfCells())
    }
    region = ExtractCellsByRegion(Input=reader)
    region.IntersectWith = shape
    region.ExtractionSide = "inside"
    region.Extractintersected = include_intersected
    if shape == "Box":
        region.IntersectWith.Position = [-1, -1, -1]
        region.IntersectWith.Length = [2, 2, 2]
    else:
        region.IntersectWith.Center = [0, 0, 0]
        region.IntersectWith.Radius = 1
    region.UpdatePipeline()
    output = servermanager.Fetch(region)
    actual = [
        cell_by_first_point[first_point(output.GetCell(index))]
        for index in range(output.GetNumberOfCells())
    ]
    assert actual == expected, (filename, shape, include_intersected, actual, expected)
    assert output.GetNumberOfPoints() == 4 * len(expected)
    point_values = output.GetPointData().GetArray("PreviewPointValue")
    cell_values = output.GetCellData().GetArray("MaterialId")
    assert point_values is not None and cell_values is not None
    assert point_values.GetNumberOfTuples() == 4 * len(expected)
    assert cell_values.GetNumberOfTuples() == len(expected)
    for output_cell, input_cell in enumerate(expected):
        assert cell_values.GetValue(output_cell) == 1000 + input_cell
        for point in range(4):
            assert point_values.GetValue(4 * output_cell + point) == 100 + 4 * input_cell + point
    print(f"PASS {filename} {shape} intersected={include_intersected}: {actual}")

print("ParaView reference: all cases passed")
