"""Acceptance tests shared by the Python source and packaged executable."""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

from vtkmodules.vtkIOLegacy import vtkUnstructuredGridReader
from vtkmodules.vtkIOXML import vtkXMLUnstructuredGridReader


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("command", help="nastran_to_vtk_cli.py or nastran_to_vtk_cli.exe")
    parser.add_argument("--models", default=r"D:\Models\Models\Nastran")
    parser.add_argument("--full", action="store_true", help="also test the wingbox model")
    return parser.parse_args()


def command_prefix(command: Path) -> list[str]:
    return [sys.executable, str(command)] if command.suffix.lower() == ".py" else [str(command)]


def run(prefix: list[str], arguments: list[str], expected: int = 0) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(prefix + arguments, text=True, capture_output=True)
    if result.returncode != expected:
        raise AssertionError(
            f"expected exit {expected}, got {result.returncode}\n"
            f"command: {prefix + arguments}\nstdout:\n{result.stdout}\nstderr:\n{result.stderr}")
    return result


def inspect_vtu(path: Path, points: int, cells: int, point_names: list[str], cell_names: list[str]) -> None:
    raw = path.read_bytes()
    assert b'header_type="UInt32"' in raw[:1000]
    assert b"compressor=" not in raw[:1000]
    assert b'<AppendedData encoding="base64">' in raw
    reader = vtkXMLUnstructuredGridReader()
    reader.SetFileName(str(path))
    reader.Update()
    grid = reader.GetOutput()
    assert grid.GetNumberOfPoints() == points, grid.GetNumberOfPoints()
    assert grid.GetNumberOfCells() == cells, grid.GetNumberOfCells()
    actual_points = [grid.GetPointData().GetArrayName(i) for i in range(grid.GetPointData().GetNumberOfArrays())]
    actual_cells = [grid.GetCellData().GetArrayName(i) for i in range(grid.GetCellData().GetNumberOfArrays())]
    for expected in point_names:
        assert any(expected in name for name in actual_points), (expected, actual_points)
    for expected in cell_names:
        assert any(expected in name for name in actual_cells), (expected, actual_cells)


def inspect_legacy_vtk(path: Path, points: int, cells: int) -> None:
    reader = vtkUnstructuredGridReader()
    reader.SetFileName(str(path))
    reader.Update()
    grid = reader.GetOutput()
    assert grid.GetNumberOfPoints() == points, grid.GetNumberOfPoints()
    assert grid.GetNumberOfCells() == cells, grid.GetNumberOfCells()


def main() -> int:
    args = parse_args()
    command = Path(args.command).resolve()
    models = Path(args.models).resolve()
    prefix = command_prefix(command)
    if not command.is_file():
        raise FileNotFoundError(command)

    ogs_bdf = models / "ogs.bdf"
    ogs_op2 = models / "ogs.op2"
    wing_bdf = models / "wingbox_stitched_together-000.bdf"
    wing_op2 = models / "wingbox_stitched_together-000.op2"

    with tempfile.TemporaryDirectory(prefix="nastran-cli-验收 with spaces-") as directory:
        root = Path(directory)
        geometry = root / "ogs-geometry.vtu"
        run(prefix, ["-b", str(ogs_bdf), "-o", str(geometry), "--validate"])
        inspect_vtu(geometry, 33, 20, ["NodeID"], ["ElementID"])

        legacy = root / "ogs-geometry.vtk"
        run(prefix, ["-b", str(ogs_bdf), "-o", str(legacy), "--validate", "-c", "0"])
        inspect_legacy_vtk(legacy, 33, 20)

        compressed = root / "ogs-compressed.vtu"
        run(prefix, ["-b", str(ogs_bdf), "-o", str(compressed), "--validate", "-c", "1"])
        assert b'compressor="vtkLZMADataCompressor"' in compressed.read_bytes()[:1000]

        combined = root / "ogs-results.vtu"
        run(prefix, ["-b", str(ogs_bdf), "-p", str(ogs_op2), "-o", str(combined), "--validate"])
        inspect_vtu(combined, 33, 20,
                    ["NodeID", "Displacement T_XYZ", "SPC Forces F_XYZ"],
                    ["ElementID", "Stress vonMises"])

        run(prefix, ["-b", str(ogs_bdf), "-o", str(combined)], expected=1)
        run(prefix, ["-b", str(root / "missing.bdf"), "-o", str(root / "missing.vtu")], expected=1)
        run(prefix, ["-b", str(ogs_bdf), "-o", str(root / "bad.ext")], expected=1)
        run(prefix, ["-b", str(ogs_bdf), "-o", str(root / "bad-compression.vtu"), "-c", "10"], expected=2)

        stale_hash = hashlib.sha256(combined.read_bytes()).digest()
        run(prefix, ["-b", str(ogs_bdf), "-p", str(root / "missing.op2"),
                     "-o", str(combined), "--force"], expected=1)
        assert hashlib.sha256(combined.read_bytes()).digest() == stale_hash

        unicode_root = root / "中文 模型"
        unicode_root.mkdir()
        unicode_bdf = unicode_root / "模型 文件.bdf"
        unicode_op2 = unicode_root / "结果 文件.op2"
        unicode_output = unicode_root / "转换 结果.vtu"
        shutil.copy2(ogs_bdf, unicode_bdf)
        shutil.copy2(ogs_op2, unicode_op2)
        run(prefix, ["-b", str(unicode_bdf), "-p", str(unicode_op2), "-o", str(unicode_output)])
        inspect_vtu(unicode_output, 33, 20, ["Displacement T_XYZ"], ["Stress vonMises"])

        if args.full:
            wing = root / "wingbox.vtu"
            run(prefix, ["-b", str(wing_bdf), "-p", str(wing_op2), "-o", str(wing), "--validate"])
            inspect_vtu(
                wing, 2675, 2657,
                ["NodeID", "Displacement T_XYZ", "SPC Forces F_XYZ",
                 "MPC Forces F_XYZ", "LoadVectors F_XYZ"],
                ["ElementID", "Stress vonMises", "Strain vonMises", "Plate Force: Fx"])

    print(f"All acceptance tests passed: {command}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
