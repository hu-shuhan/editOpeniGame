#include "iGameStreamBase.h"
#include "iGameScene.h"
IGAME_NAMESPACE_BEGIN
iGameStreamBase::iGameStreamBase() {
    this->m_Points = Points::New();
    this->m_PositionColors = FloatArray::New();
    this->m_PositionColors->SetDimension(3);
    this->index = UnsignedIntArray::New();
    this->index->SetDimension(2);
    DynamicCast<iGame::DrawObject>(this)->AddViewStyle(IG_WIREFRAME);
    streamFilter = new iGameStreamTracer;
}
iGameStreamBase::~iGameStreamBase() {}

void iGameStreamBase::ComputeBoundingBox() {
    if (m_Bounding.isNull() || m_BoundingHelper->GetMTime() < m_Points->GetMTime()) {
        m_Bounding.reset();
        for (int i = 0; i < m_Points->GetNumberOfPoints(); i++) { m_Bounding.add(m_Points->GetPoint(i)); }
        m_BoundingHelper->Modified();
    }
}

void iGameStreamBase::ConvertToDrawableData() {
    if (!isUpdate) { return; }

    m_Points->Reset();
    m_PositionColors->Reset();
    index->Reset();
    auto m_StreamMesh = streamFilter->GetOutput();
    // 使用UnstructuredMesh格式
    if (m_StreamMesh == nullptr) {
        isUpdate = false;
        return;
    }

    // 从UnstructuredMesh中提取数据
    auto meshPoints = m_StreamMesh->GetPoints();
    auto meshCells = m_StreamMesh->GetCells();
    auto meshTypes = m_StreamMesh->GetCellTypes();
    auto attrSet = m_StreamMesh->GetAttributeSet();

    if (!meshPoints || !meshCells) {
        isUpdate = false;
        return;
    }

    // 复制所有点数据
    for (int i = 0; i < meshPoints->GetNumberOfPoints(); i++) {
        m_Points->AddPoint(meshPoints->GetPoint(i));
    }

    // 处理单元连接数据，转换为线条索引
    for (int cellId = 0; cellId < meshCells->GetNumberOfCells(); cellId++) {
        IGenum cellType = meshTypes->GetValue(cellId);

        if (cellType == IG_POLY_LINE) {
            // 处理POLYLINE单元：连接相邻的点
            // 首先使用安全方式获取点数量
            const igIndex* pointIds = nullptr;
            int numPoints = meshCells->GetCellIds(cellId, pointIds);

            if (numPoints <= 0) continue;

            // 连接相邻的点
            for (int i = 0; i < numPoints - 1; i++) {
                index->AddElement2(pointIds[i], pointIds[i + 1]);
            }
        } else if (cellType == IG_LINE) {
            // 处理LINE单元：直接使用连接关系
            const igIndex* pointIds = nullptr;
            int numPoints = meshCells->GetCellIds(cellId, pointIds);

            if (numPoints == 2) {
                index->AddElement2(pointIds[0], pointIds[1]);
            }
        }
    }

    // 提取速度属性作为颜色数据
    if (attrSet) {
        auto velocityAttr = attrSet->GetVector("Velocity");
        if (!velocityAttr.IsNone() && velocityAttr.pointer) {
            // 使用速度数据作为颜色数据
            for (int i = 0; i < meshPoints->GetNumberOfPoints(); i++) {
                float velocity[3] = {0.0f, 0.0f, 0.0f};
                velocityAttr.pointer->GetElement(i, velocity);
                m_PositionColors->AddElement3(velocity[0], velocity[1], velocity[2]);
            }
        } else {
            // 如果没有速度属性，使用默认颜色
            for (int i = 0; i < meshPoints->GetNumberOfPoints(); i++) {
                m_PositionColors->AddElement3(1.0f, 1.0f, 1.0f);  // 白色
            }
        }
    } else {
        // 如果没有属性集，使用默认颜色
        for (int i = 0; i < meshPoints->GetNumberOfPoints(); i++) {
            m_PositionColors->AddElement3(1.0f, 1.0f, 1.0f);  // 白色
        }
    }

    // 应用颜色映射和设置渲染数据
    this->m_ColorMapper->InitRange(m_PositionColors, -1);
    auto colors = this->m_ColorMapper->MapScalars(m_PositionColors, -1);

    m_Positions = m_Points->ConvertToArray();
    m_Positions->Modified();

    m_LineIndices = index;
    m_LineIndices->Modified();

    m_Colors = colors;
    m_Colors->Modified();

    if (m_Colors != nullptr) {
        m_UseColor = true;
    }

    isUpdate = false;
}

bool iGameStreamBase::IsUseSinglePassWireframeRendering() { return false; }

IGAME_NAMESPACE_END