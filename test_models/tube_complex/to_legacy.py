"""Convert generated static VTP fixtures to legacy ASCII VTK POLYDATA."""
from pathlib import Path
import xml.etree.ElementTree as ET

root = Path(__file__).resolve().parent
for name in ('curves', 'branches'):
    piece = ET.parse(root / (name + '.vtp')).find('./PolyData/Piece')
    np = int(piece.attrib['NumberOfPoints'])
    nl = int(piece.attrib['NumberOfLines'])
    coords = piece.find('./Points/DataArray').text.split()
    conn = piece.find("./Lines/DataArray[@Name='connectivity']").text.split()
    offsets = list(map(int, piece.find("./Lines/DataArray[@Name='offsets']").text.split()))
    lines = ['# vtk DataFile Version 3.0', 'Complex static Tube fixture', 'ASCII',
             'DATASET POLYDATA', f'POINTS {np} double']
    lines.extend(' '.join(coords[i:i+3]) for i in range(0,len(coords),3))
    lines.append(f'LINES {nl} {len(conn)+nl}')
    start = 0
    for end in offsets:
        lines.append(str(end-start) + ' ' + ' '.join(conn[start:end]))
        start = end
    for section, count, header in [('PointData',np,'POINT_DATA'),('CellData',nl,'CELL_DATA')]:
        lines.append(f'{header} {count}')
        for arr in piece.find(section):
            components = int(arr.attrib['NumberOfComponents'])
            values = arr.text.split()
            assert len(values) == count*components
            kind = 'int' if arr.attrib['type'].startswith('Int') else 'double'
            if components == 3:
                lines.append(f"VECTORS {arr.attrib['Name']} {kind}")
            else:
                lines.extend([f"SCALARS {arr.attrib['Name']} {kind} 1", 'LOOKUP_TABLE default'])
            lines.extend(' '.join(values[i:i+components]) for i in range(0,len(values),components))
    target = root / (name+'.vtk')
    target.write_text('\n'.join(lines)+'\n', encoding='ascii')
    assert target.read_text(encoding='ascii').startswith('# vtk DataFile Version 3.0\n')
    print(f'{target.name}: {np} points, {nl} polylines, attributes preserved')
