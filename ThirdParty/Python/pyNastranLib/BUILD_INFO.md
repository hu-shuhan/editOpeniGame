# Build and release record

Release date: 2026-08-27  
Target: Windows x64, single-file console executable

## Toolchain

- CPython 3.12.10 x64
- PyInstaller 6.22.2
- pyNastran 1.4.1
- VTK 9.3.1
- NumPy 1.26.4
- SciPy 1.12.0
- Matplotlib 3.8.4
- PyQt5 5.15.11 / Qt 5.15.2
- QtPy 2.4.3

All direct and transitive Python package versions are in `requirements-build.txt`.

The official CPython installer used to create the build environment was `python-3.12.10-amd64.exe`, SHA-256:

```text
67b5635e80ea51072b87941312d00ec8927c4db9ba18938f7ad2d27b328b95fb
```

## Build command

From this directory:

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\build_exe.ps1
```

Equivalent PyInstaller invocation after installing the lock file:

```powershell
python -m PyInstaller --clean --noconfirm --distpath dist --workpath build nastran_to_vtk_cli.spec
```

## Release checksums

```text
759e870c52c56344c7786054852124b75f80afb1df0f68332ea20ca282471979  nastran_to_vtk_cli.legacy-759e870c.exe
9ad0475a44b3029b062f6991bae9a8a5e3097eb09014a4b453c5ff50d803f63e  nastran_to_vtk_cli.exe
```

The legacy executable was backed up only after the candidate passed source, packaged-EXE, real-model, Unicode-path, atomic-output, `iGameVTUReader`, and `testNastranReader` acceptance tests.
