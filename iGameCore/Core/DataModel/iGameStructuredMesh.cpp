#include "iGameStructuredMesh.h"
#include "iGameFaceTable.h"
#include "iGameModelSurfaceFilters/iGameModelGeometryFilter.h"
#include "iGameScene.h"
IGAME_NAMESPACE_BEGIN

StructuredMesh::~StructuredMesh() {}
void StructuredMesh::SetDimensionSize(igIndex s[3]) {
    std::copy(s, s + 3, this->size);
    if (size[2] <= 1) {
        size[2] = 1;
        this->Dimension = 2;
    } else
        this->Dimension = 3;
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
    if (size[2] <= 1) {
        size[2] = 1;
        this->Dimension = 2;
    }
    igIndex i = 0, j = 0, k = 0;
    igIndex vhs[8] = {0};
    igIndex st = 0;
    this->m_Faces = CellArray::New();
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

//void StructuredMesh::ViewCloudPicture(Scene* scene, int index, int demension) {
//    //Structured meshes come in special forms such as IJ planes
//    return this->VolumeMesh::ViewCloudPicture(scene, index, demension);
//}

IGsize StructuredMesh::GetRealMemorySize() {
    IGsize res = this->VolumeMesh::GetRealMemorySize();
    return res + sizeof(Dimension) + sizeof(extent) + sizeof(size);
}
IGAME_NAMESPACE_END
