#include "iGameStructuredMesh.h"
#include "ModelSurface/iGameModelGeometryFilter.h"
#include "iGameFaceTable.h"
#include "iGameScene.h"
#include <array>
#include <vector>
IGAME_NAMESPACE_BEGIN

StructuredMesh::~StructuredMesh() {}
void StructuredMesh::SetDimensionSize(igIndex s[3]){
    if (s[2] <= 1) s[2] = 1;
    if (s[0] == size[0] && s[1] == size[1] && s[2] == size[2]) { 
        return ;
    }
    std::copy(s, s + 3, this->size);
    if (size[2] <= 1) {
        size[2] = 1;
        this->Dimension = 2;
    } else
        this->Dimension = 3;
    this->m_BuildStructuredConnectivty = false;
}
IGsize StructuredMesh::GetNumberOfCells() {
    if (size[2] > 1) return GetNumberOfVolumes();
    else
        return GetNumberOfFaces();
}
void StructuredMesh::BuildStructuredFaces() {
    igIndex i = 0, j = 0, k = 0;
    igIndex vhs[4] = {0};
    igIndex st = 0;
    igIndex tmpvhs[4] = {0, 1, 1 + size[0] * size[1], size[0] * size[1]};
    this->m_Faces->Resize(size[2] * (size[1] - 1) * (size[0] - 1) +
                          size[0] * (size[1] - 1) * (size[2] - 1) +
                          size[1] * (size[2] - 1) * (size[0] - 1));
    // ij面的定义
    tmpvhs[1] = 1;
    tmpvhs[2] = 1 + size[0];
    tmpvhs[3] = size[0];
    for (k = 0; k < size[2]; ++k) {
        for (j = 0; j < size[1] - 1; ++j) {
            st = j * size[0] + k * size[0] * size[1];
            for (i = 0; i < size[0] - 1; ++i) {
                for (int it = 0; it < 4; it++) { vhs[it] = st + tmpvhs[it]; }
                st++;
                m_Faces->AddCellIds(vhs, 4);
            }
        }
    }
    // ik方向面的定义
    tmpvhs[1] = 1;
    tmpvhs[2] = 1 + size[0] * size[1];
    tmpvhs[3] = size[0] * size[1];
    for (j = 0; j < size[1]; j++) {
        for (k = 0; k < size[2] - 1; k++) {
            st = j * size[0] + k * size[0] * size[1];
            for (i = 0; i < size[0] - 1; i++) {
                for (int it = 0; it < 4; it++) { vhs[it] = st + tmpvhs[it]; }
                st++;
                m_Faces->AddCellIds(vhs, 4);
            }
        }
    }

    // jk方向面的定义
    tmpvhs[1] = size[0];
    tmpvhs[2] = size[0] + size[0] * size[1];
    tmpvhs[3] = size[0] * size[1];
    for (i = 0; i < size[0]; i++) {
        for (k = 0; k < size[2] - 1; k++) {
            st = i + k * size[0] * size[1];
            for (j = 0; j < size[1] - 1; j++) {
                for (int it = 0; it < 4; it++) { vhs[it] = st + tmpvhs[it]; }
                st += size[0];
                m_Faces->AddCellIds(vhs, 4);
            }
        }
    }
}
void StructuredMesh::GenStructuredCellConnectivities() {
    if (m_BuildStructuredConnectivty) return;
    if (size[2] <= 1) {
        size[2] = 1;
        this->Dimension = 2;
    }
    igIndex i = 0, j = 0, k = 0;
    igIndex vhs[8] = {0};
    igIndex st = 0;
    if (this->Dimension == 3) {
        this->m_Volumes = CellArray::New();
        this->m_Volumes->Resize((size[0] - 1) * (size[1] - 1) * (size[2] - 1));
        igIndex tmpvhs[8] = {0,
                             1,
                             1 + size[0] * size[1],
                             size[0] * size[1],
                             size[0],
                             1 + size[0],
                             1 + size[0] + size[0] * size[1],
                             size[0] + size[0] * size[1]};
        for (k = 0; k < size[2] - 1; ++k) {
            for (j = 0; j < size[1] - 1; ++j) {
                st = j * size[0] + k * size[0] * size[1];
                for (i = 0; i < size[0] - 1; ++i) {
                    for (int it = 0; it < 8; it++) {
                        vhs[it] = st + tmpvhs[it];
                    }
                    m_Volumes->AddCellIds(vhs, 8);
                    st++;
                }
            }
        }

    } else {
        this->m_Faces = CellArray::New();
        this->m_Faces->Resize((size[0] - 1) * (size[1] - 1));
        igIndex tmpvhs[4] = {0, 1, size[0] + 1, size[0]};
        for (j = 0; j < size[1] - 1; ++j) {
            st = j * size[0];
            for (i = 0; i < size[0] - 1; ++i) {
                for (int it = 0; it < 4; it++) { vhs[it] = st + tmpvhs[it]; }
                st++;
                m_Faces->AddCellIds(vhs, 4);
            }
        }
    }
    m_BuildStructuredConnectivty = true;
}
igIndex StructuredMesh::GetPointIndex(igIndex i, igIndex j, igIndex k) {
    return i + j * size[0] + k * size[0] * size[1];
}
igIndex StructuredMesh::GetVolumeIndex(igIndex i, igIndex j, igIndex k) {
    return i + j * (size[0] - 1) + k * (size[0] - 1) * (size[1] - 1);
}

void StructuredMesh::SetAttributeWithCellData(ArrayObject::Pointer attr, DoubleArray::Pointer attrRange,
                                              igIndex dimension) {
    // 结构化连接关系是单元几何的唯一来源，这里先确保它已经生成（同时确定 Dimension 是 2 还是 3）
    this->GenStructuredCellConnectivities();
    const IGsize numberOfCells = this->GetNumberOfCells();
    if (attr == nullptr || m_Points == nullptr || numberOfCells <= 0) {
        // 没有单元（或没有属性）时不能留下过期的单元几何，否则 m_ColorWithCell 与实际几何不匹配
        m_CellPositionSize = 0;
        // 同理不能保留上一个属性生成的逐点颜色，否则点样式会显示过期颜色
        m_Colors = FloatArray::New();
        m_Colors->SetDimension(4);
        m_Colors->Modified();
        return;
    }

    /* 与 VolumeMesh 保持一致：只有色带范围被用户手工固定（SetRangeStable）时才保留当前范围，
       否则每次切换属性都按该属性自身的数据范围重设色带，避免沿用上一个属性的范围。 */
    if (m_IsMainRenderableObject && (!attrRange || m_ColorMapper->GetMTime() <= attrRange->GetMTime()) &&
        !m_ColorMapper->GetStable()) {
        double minimal_val = attrRange ? attrRange->GetValue(2 + dimension * 2 + 0) : 0.0;
        double maximal_val = attrRange ? attrRange->GetValue(2 + dimension * 2 + 1) : 0.0;
        if (attrRange && minimal_val < maximal_val) {
            m_ColorMapper->SetRange(minimal_val, maximal_val);
        } else {
            m_ColorMapper->InitRange(attr, dimension);
        }
    }

    FloatArray::Pointer colors = m_ColorMapper->MapScalars(attr, dimension, 4);
    if (colors == nullptr) {
        m_CellPositionSize = 0;
        m_Colors = FloatArray::New();
        m_Colors->SetDimension(4);
        m_Colors->Modified();
        return;
    }
    const IGsize numberOfColors = colors->GetNumberOfElements();

    FloatArray::Pointer newPositions = FloatArray::New();
    FloatArray::Pointer newColors = FloatArray::New();
    UnsignedCharArray::Pointer newEdgeMasks = UnsignedCharArray::New();
    newPositions->SetDimension(3);
    newColors->SetDimension(4);
    newEdgeMasks->SetDimension(3);

    float color[4]{};
    // 点样式（IG_POINTS）绘制的是 m_Positions / m_Colors，单元属性的颜色却只在 m_CellColors 里，
    // 渲染侧过去只好把点画成纯白。这里同时生成逐点颜色（cell->point 取入射单元颜色平均）。
    CellToPointColorBuilder pointColors;
    pointColors.Initialize(this->GetNumberOfPoints());
    // 一个四边形面按扇形剖分为 2 个三角形，与 UnstructuredMesh/SurfaceMesh/VolumeMesh 的
    // 单元着色完全一致（第一个三角形 mask=3，第二个 mask=6）
    auto appendQuad = [&](const igIndex* quad, const float* rgb) {
        for (int k = 1; k < 4 - 1; ++k) {
            const auto& p0 = this->GetPoint(quad[0]);
            const auto& p1 = this->GetPoint(quad[k]);
            const auto& p2 = this->GetPoint(quad[k + 1]);

            newPositions->AddElement3(p0[0], p0[1], p0[2]);
            newPositions->AddElement3(p1[0], p1[1], p1[2]);
            newPositions->AddElement3(p2[0], p2[1], p2[2]);

            newColors->AddElement4(rgb[0], rgb[1], rgb[2], rgb[3]);
            newColors->AddElement4(rgb[0], rgb[1], rgb[2], rgb[3]);
            newColors->AddElement4(rgb[0], rgb[1], rgb[2], rgb[3]);

            newEdgeMasks->AddValue(k == 1 ? 3 : 6);
        }
    };

    if (this->Dimension == 3) {
        // 六面体单元：6 个四边形面，局部点序与 Hexahedron::faces 的定义一致
        const IGsize numberOfVolumes = this->GetNumberOfVolumes();
        for (IGsize cid = 0; cid < numberOfVolumes && cid < numberOfColors; ++cid) {
            const igIndex* cell = nullptr;
            if (m_Volumes->GetCellIds(cid, cell) != Hexahedron::NumberOfPoints) { continue; }
            colors->GetElement(cid, color);
            pointColors.AddCell(cell, Hexahedron::NumberOfPoints, color);
            for (int f = 0; f < Hexahedron::NumberOfFaces; ++f) {
                const int* face = Hexahedron::faces[f];
                const igIndex quad[4] = {cell[face[0]], cell[face[1]], cell[face[2]], cell[face[3]]};
                appendQuad(quad, color);
            }
        }
    } else {
        // 2D 结构化网格（size[2]==1）：单元本身就是四边形（m_Faces）
        const IGsize numberOfFaces = this->GetNumberOfFaces();
        for (IGsize cid = 0; cid < numberOfFaces && cid < numberOfColors; ++cid) {
            const igIndex* cell = nullptr;
            if (m_Faces->GetCellIds(cid, cell) != 4) { continue; }
            colors->GetElement(cid, color);
            pointColors.AddCell(cell, 4, color);
            appendQuad(cell, color);
        }
    }

    m_CellPositionSize = newPositions->GetNumberOfElements();

    m_CellPositions = newPositions;
    m_CellPositions->Modified();

    m_CellColors = newColors;
    m_CellColors->Modified();

    m_CellTriangleEdgeMasks = newEdgeMasks;
    m_CellTriangleEdgeMasks->Modified();

    m_Colors = pointColors.Build(this->GetDefaultColor());
    m_Colors->Modified();
}

//void StructuredMesh::ViewCloudPicture(Scene* scene, int index, int demension) {
//    //Structured meshes come in special forms such as IJ planes
//    return this->VolumeMesh::ViewCloudPicture(scene, index, demension);
//}

IGsize StructuredMesh::GetRealMemorySize() {
    IGsize res = this->VolumeMesh::GetRealMemorySize();
    return res + sizeof(Dimension) + sizeof(extent) + sizeof(size);
}
IGAME_NAMESPACE_END
