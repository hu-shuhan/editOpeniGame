"""Static legacy VTK tangent fixtures; no external dependencies."""
from pathlib import Path
import math

ROOT = Path(__file__).resolve().parent

def write(name, points, triangles, normals, uv, expected=None):
    assert len(points)==len(normals)==len(uv)
    assert all(len(t)==3 and all(0<=i<len(points) for i in t) for t in triangles)
    out=['# vtk DataFile Version 3.0','Surface tangent test','ASCII','DATASET POLYDATA',
         f'POINTS {len(points)} double']
    out += [' '.join(map(str,p)) for p in points]
    out += [f'POLYGONS {len(triangles)} {4*len(triangles)}']
    out += ['3 '+' '.join(map(str,t)) for t in triangles]
    out += [f'POINT_DATA {len(points)}','NORMALS Normals double']
    out += [' '.join(map(str,n)) for n in normals]
    out += ['TEXTURE_COORDINATES TCoords 2 double']
    out += [' '.join(map(str,v)) for v in uv]
    if expected:
        out += ['VECTORS ExpectedTangents double']
        out += [' '.join(map(str,v)) for v in expected]
    out += ['SCALARS PointID int 1','LOOKUP_TABLE default']
    out += list(map(str,range(len(points))))
    (ROOT/name).write_text('\n'.join(out)+'\n',encoding='ascii')

# Four disconnected patches exercise +X, +Y, mirrored -X and oblique tangents.
points=[]; triangles=[]; normals=[]; uv=[]; expected=[]
directions=[(1,0,0),(0,1,0),(-1,0,0),(0.6,0.8,0)]
for patch,tangent in enumerate(directions):
    tx,ty,_=tangent
    by=(-ty,tx,0)
    for j in range(9):
        for i in range(13):
            u,v=i/12,j/8
            points.append((3*patch+2*u*tx+v*by[0],2*u*ty+v*by[1],0))
            uv.append((u,v)); normals.append((0,0,1)); expected.append(tangent)
    base=patch*117
    for j in range(8):
        for i in range(12):
            a=base+j*13+i; b=a+1; c=a+14; d=a+13
            triangles.extend([(a,b,c),(a,c,d)])
write('01_planar_reference.vtk',points,triangles,normals,uv,expected)

# Verify every planar triangle analytically: normalized dP/du equals expected tangent.
for a,b,c in triangles:
    du1,dv1=uv[b][0]-uv[a][0],uv[b][1]-uv[a][1]
    du2,dv2=uv[c][0]-uv[a][0],uv[c][1]-uv[a][1]
    det=du1*dv2-du2*dv1
    assert abs(det)>1e-12
    t=[((points[b][k]-points[a][k])*dv2-(points[c][k]-points[a][k])*dv1)/det for k in range(3)]
    length=math.sqrt(sum(v*v for v in t))
    assert max(abs(t[k]/length-expected[a][k]) for k in range(3))<1e-12

# Cylinder side only: seam points duplicated with u=0 and u=1; no end caps.
p=[]; n=[]; tex=[]; faces=[]
for j in range(17):
    for i in range(65):
        u,v=i/64,j/16; theta=2*math.pi*u
        p.append((math.cos(theta),math.sin(theta),4*v))
        n.append((math.cos(theta),math.sin(theta),0)); tex.append((u,v))
for j in range(16):
    for i in range(64):
        a=j*65+i; faces.extend([(a,a+1,a+66),(a,a+66,a+65)])
write('02_cylinder_valid_uv.vtk',p,faces,n,tex)

# Deliberately collapsed UV on the second planar patch. Geometry and normals unchanged.
bad_uv=list(uv)
for i in range(117,234):
    bad_uv[i]=(uv[i][0],0)
write('03_degenerate_uv.vtk',points,triangles,normals,bad_uv)
print('Generated 3 static VTK fixtures; 768 planar triangles analytically verified.')
