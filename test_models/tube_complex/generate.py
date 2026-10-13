"""Generate Tube fixtures as ASCII VTP/PVD using only Python stdlib."""
from pathlib import Path
import math
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parent
def write(name, frame=0, branches=False):
    points, lines, radius, speed, ids = [], [], [], [], []
    for curve in range(5):
        line = []
        for j in range(121):
            u = j/120
            a = 6*math.pi*u + frame*0.2
            if curve == 0:
                p = (math.cos(a), math.sin(a), 5*u)
            elif curve == 1:
                p = (4+2*u, math.sin(4*math.pi*u), 2*u+0.3*math.cos(a))
            elif curve == 2:
                p = (8+math.cos(2*math.pi*u), math.sin(2*math.pi*u), 1)
            elif curve == 3:
                p = (3*u, 4, 0 if u <= 0.5 else 4*(u-0.5))
            else:
                p = (5+3*u, 4+0.4*math.sin(a), 1+u)
            line.append(len(points))
            points.append(p)
            radius.append(0.04+0.12*(0.5+0.5*math.sin(4*math.pi*u+frame*0.5)))
            speed.append(1+4*u+frame)
            ids.append(curve)
        if curve == 2:
            # Explicitly close the ring using the first point ID.
            line = line[:-1]+[line[0]]
        lines.append(line)
    if branches:
        # Three line cells share a junction: Tube does not promise welded junctions.
        junction = len(points)
        points.append((10,4,0)); radius.append(0.1); speed.append(2); ids.append(5)
        for tip in [(12,4,0),(10,6,0),(10,4,2)]:
            tipid=len(points)
            points.append(tip); radius.append(0.1); speed.append(2); ids.append(5)
            lines.append([junction,tipid])
    root=ET.Element('VTKFile',type='PolyData',version='0.1',byte_order='LittleEndian')
    piece=ET.SubElement(ET.SubElement(root,'PolyData'),'Piece',NumberOfPoints=str(len(points)),
                        NumberOfVerts='0',NumberOfLines=str(len(lines)),NumberOfStrips='0',NumberOfPolys='0')
    def arr(parent,name,data,n=1,kind='Float64'):
        a=ET.SubElement(parent,'DataArray',type=kind,Name=name,NumberOfComponents=str(n),format='ascii')
        a.text=' '.join(str(v) for v in data)
    pd=ET.SubElement(piece,'PointData',Scalars='Radius',Vectors='Velocity')
    arr(pd,'Radius',radius)
    arr(pd,'Speed',speed)
    arr(pd,'CurveID',ids,kind='Int32')
    arr(pd,'Velocity',[v for s in speed for v in (s,0,0)],3)
    cd=ET.SubElement(piece,'CellData')
    arr(cd,'LineID',list(range(len(lines))),kind='Int32')
    arr(ET.SubElement(piece,'Points'),'Points',[v for p in points for v in p],3)
    section=ET.SubElement(piece,'Lines')
    arr(section,'connectivity',[v for line in lines for v in line],kind='Int64')
    offsets=[]; count=0
    for line in lines:
        count+=len(line); offsets.append(count)
    arr(section,'offsets',offsets,kind='Int64')
    ET.indent(root)
    ET.ElementTree(root).write(ROOT/name,encoding='utf-8',xml_declaration=True)

write('curves.vtp')
write('branches.vtp',branches=True)
collection=ET.Element('VTKFile',type='Collection',version='0.1',byte_order='LittleEndian')
entries=ET.SubElement(collection,'Collection')
for frame in range(5):
    name=f'animated_{frame:03d}.vtp'
    write(name,frame)
    ET.SubElement(entries,'DataSet',timestep=str(frame),group='',part='0',file=name)
ET.indent(collection)
ET.ElementTree(collection).write(ROOT/'animated.pvd',encoding='utf-8',xml_declaration=True)
for path in ROOT.glob('*.vtp'):
    p=ET.parse(path).find('./PolyData/Piece')
    np,nl=int(p.attrib['NumberOfPoints']),int(p.attrib['NumberOfLines'])
    for section,count in [('PointData',np),('CellData',nl),('Points',np)]:
        for a in p.find(section):
            assert len(a.text.split())==count*int(a.attrib['NumberOfComponents']),path
    conn=list(map(int,p.find("./Lines/DataArray[@Name='connectivity']").text.split()))
    offsets=list(map(int,p.find("./Lines/DataArray[@Name='offsets']").text.split()))
    assert len(offsets)==nl and offsets[-1]==len(conn)
    assert all(0<=v<np for v in conn)
    assert all(b>a for a,b in zip([0]+offsets,offsets))
    r=list(map(float,p.find("./PointData/DataArray[@Name='Radius']").text.split()))
    assert all(0.039999<=v<=0.160001 for v in r)
print('Validated 7 VTP files and one five-frame PVD series.')
