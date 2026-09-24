"""Convert Nastran BDF/OP2 files to VTK using pyNastran's GUI pipeline."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import sys
import tempfile

from pyNastran.bdf.bdf import read_bdf
from pyNastran.converters.nastran.nastran_to_vtk import NastranGUI, save_nastran_results
from vtkmodules.vtkIOCore import vtkLZMADataCompressor
from vtkmodules.vtkIOLegacy import vtkUnstructuredGridReader, vtkUnstructuredGridWriter
from vtkmodules.vtkIOXML import vtkXMLUnstructuredGridReader, vtkXMLUnstructuredGridWriter


BDF_EXTENSIONS = {".bdf", ".dat", ".nas"}
OUTPUT_EXTENSIONS = {".vtk", ".vtu"}


def compression_level(value: str) -> int:
    try:
        level = int(value)
    except ValueError as error:
        raise argparse.ArgumentTypeError("compression level must be an integer from 0 to 9") from error
    if not 0 <= level <= 9:
        raise argparse.ArgumentTypeError("compression level must be from 0 to 9")
    return level


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Convert Nastran BDF/OP2 data to VTK.")
    parser.add_argument("-b", "--bdf", required=True, metavar="FILE", help="input BDF file")
    parser.add_argument("-o", "--output", required=True, metavar="FILE", help="output .vtu or .vtk file")
    parser.add_argument("-p", "--op2", metavar="FILE", help="optional OP2 results file")
    parser.add_argument(
        "-l", "--log-level", choices=("debug", "info", "warning", "error"),
        default="info", help="logging level (default: info)")
    parser.add_argument(
        "-c", "--compression", type=compression_level, default=0, metavar="LEVEL",
        help="LZMA compression level 0-9; 0 disables compression (default: 0)")
    parser.add_argument("--force", action="store_true", help="overwrite an existing output file")
    parser.add_argument("--validate", action="store_true", help="validate the BDF and generated VTK file")
    return parser


def checked_input(path_text: str, extensions: set[str], label: str) -> Path:
    path = Path(path_text).expanduser().resolve()
    if path.suffix.lower() not in extensions:
        expected = ", ".join(sorted(extensions))
        raise ValueError(f"{label} must use one of these extensions: {expected}")
    if not path.is_file():
        raise FileNotFoundError(f"{label} file does not exist: {path}")
    return path


def checked_output(path_text: str, force: bool) -> Path:
    path = Path(path_text).expanduser().resolve()
    if path.suffix.lower() not in OUTPUT_EXTENSIONS:
        raise ValueError("output must use the .vtu or .vtk extension")
    if not path.parent.is_dir():
        raise FileNotFoundError(f"output directory does not exist: {path.parent}")
    if path.exists() and not force:
        raise FileExistsError(f"output already exists (use --force): {path}")
    if path.exists() and not path.is_file():
        raise ValueError(f"output path is not a regular file: {path}")
    return path


def load_grid(bdf_path: Path, op2_path: Path | None, log_level: str):
    gui = NastranGUI()
    gui.create_secondary_actors = False
    gui.log.level = log_level
    gui.load_nastran_geometry(os.fspath(bdf_path))
    grid = gui.grid
    if grid is None:
        raise RuntimeError("pyNastran did not create an unstructured grid")
    if op2_path is not None:
        gui.load_nastran_results(os.fspath(op2_path))
    save_nastran_results(gui, grid)
    return grid


def write_grid(grid, output_path: Path, compression: int) -> None:
    if output_path.suffix.lower() == ".vtu":
        writer = vtkXMLUnstructuredGridWriter()
        writer.SetDataModeToAppended()
        writer.EncodeAppendedDataOn()
        writer.SetHeaderTypeToUInt32()
        if compression == 0:
            writer.SetCompressor(None)
        else:
            compressor = vtkLZMADataCompressor()
            compressor.SetCompressionLevel(compression)
            writer.SetCompressor(compressor)
    else:
        if compression:
            raise ValueError("compression is supported only for .vtu output")
        writer = vtkUnstructuredGridWriter()
        writer.SetFileTypeToBinary()
    writer.SetFileName(os.fspath(output_path))
    writer.SetInputData(grid)
    if writer.Write() != 1:
        raise RuntimeError(f"VTK writer failed to create: {output_path}")


def validate_output(path: Path) -> None:
    reader = vtkXMLUnstructuredGridReader() if path.suffix.lower() == ".vtu" else vtkUnstructuredGridReader()
    reader.SetFileName(os.fspath(path))
    reader.Update()
    grid = reader.GetOutput()
    if grid is None or grid.GetNumberOfPoints() == 0:
        raise RuntimeError(f"generated VTK file contains no points: {path}")
    if grid.GetNumberOfCells() == 0:
        raise RuntimeError(f"generated VTK file contains no cells: {path}")


def convert(args: argparse.Namespace) -> Path:
    bdf_path = checked_input(args.bdf, BDF_EXTENSIONS, "BDF")
    op2_path = checked_input(args.op2, {".op2"}, "OP2") if args.op2 else None
    output_path = checked_output(args.output, args.force)

    if args.validate:
        read_bdf(os.fspath(bdf_path), validate=True, xref=False, debug=False)

    temporary_name: str | None = None
    try:
        with tempfile.NamedTemporaryFile(
                prefix=f".{output_path.stem}.", suffix=output_path.suffix,
                dir=output_path.parent, delete=False) as temporary:
            temporary_name = temporary.name
        temporary_path = Path(temporary_name)
        grid = load_grid(bdf_path, op2_path, args.log_level)
        write_grid(grid, temporary_path, args.compression)
        if not temporary_path.is_file() or temporary_path.stat().st_size == 0:
            raise RuntimeError("conversion produced an empty output file")
        if args.validate:
            validate_output(temporary_path)
        os.replace(temporary_path, output_path)
        temporary_name = None
        return output_path
    finally:
        if temporary_name is not None:
            try:
                Path(temporary_name).unlink(missing_ok=True)
            except OSError:
                pass


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    try:
        output_path = convert(args)
    except (OSError, RuntimeError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    print(f"Wrote {output_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
