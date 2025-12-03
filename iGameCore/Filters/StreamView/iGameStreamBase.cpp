#include "iGameStreamBase.h"
#include "iGameScene.h"
IGAME_NAMESPACE_BEGIN
StreamBase::StreamBase() {
    this->m_Points = Points::New();
    this->m_PositionColors = FloatArray::New();
    this->m_PositionColors->SetDimension(3);
    this->index = UnsignedIntArray::New();
    this->index->SetDimension(2);
    DynamicCast<iGame::DrawObject>(this)->AddViewStyle(IG_WIREFRAME);
    streamFilter = new StreamTracer;
}
StreamBase::~StreamBase() {}

void StreamBase::ComputeBoundingBox() {
    if (m_Bounding.isNull() || m_BoundingHelper->GetMTime() < m_Points->GetMTime()) {
        m_Bounding.reset();
        for (int i = 0; i < m_Points->GetNumberOfPoints(); i++) { m_Bounding.add(m_Points->GetPoint(i)); }
        m_BoundingHelper->Modified();
    }
}

void StreamBase::ConvertToDrawableData() {
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
    for (int i = 0; i < meshPoints->GetNumberOfPoints(); i++) { m_Points->AddPoint(meshPoints->GetPoint(i)); }

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
            for (int i = 0; i < numPoints - 1; i++) { index->AddElement2(pointIds[i], pointIds[i + 1]); }
        } else if (cellType == IG_LINE) {
            // 处理LINE单元：直接使用连接关系
            const igIndex* pointIds = nullptr;
            int numPoints = meshCells->GetCellIds(cellId, pointIds);

            if (numPoints == 2) { index->AddElement2(pointIds[0], pointIds[1]); }
        }
    }

    // 提取速度属性作为颜色数据
    if (attrSet) {
        auto velocityAttr = attrSet->GetVector("V");
        float step1=0.0f;
        float step2=FLT_MAX;
        int count1 = 0;
        int count2= 0;
        if (!velocityAttr.IsNone() && velocityAttr.pointer) {
            // 使用速度数据作为颜色数据
            for (int i = 0; i < meshPoints->GetNumberOfPoints(); i++) {
                float velocity[3] = {0.0f, 0.0f, 0.0f};
                velocityAttr.pointer->GetElement(i, velocity);
                auto temV1= sqrt(velocity[0] * velocity[0] + velocity[1] * velocity[1] + velocity[2] * velocity[2]);
                if (temV1 > 40) { count2++;
                }
                count1++;
                step1=std::max(static_cast<double>(step1), temV1);
                step2=std::min(static_cast<double>(step2), temV1);
            }
        } else {
            // 如果没有速度属性，使用默认颜色
            for (int i = 0; i < meshPoints->GetNumberOfPoints(); i++) {
                m_PositionColors->AddElement3(1.0f, 1.0f, 1.0f); // 白色
            }
        }
    
        std::cout << "Velocity Magnitude Range: [" << step2 << "," << step1 << "]" << std::endl;
        std::cout << (float)count2/(float)count1 << std::endl;

    } else {
        // 如果没有属性集，使用默认颜色
        for (int i = 0; i < meshPoints->GetNumberOfPoints(); i++) {
            m_PositionColors->AddElement3(1.0f, 1.0f, 1.0f); // 白色
        }
    }

    // 应用颜色映射和设置渲染数据
    this->m_ColorMapper->InitRange(m_PositionColors, -1);
    this->m_ColorMapper->SetRange(streamFilter->minF, streamFilter->maxF);
    auto colors = this->m_ColorMapper->MapScalars(m_PositionColors, -1);
    m_Positions = m_Points->ConvertToArray();
    m_Positions->Modified();

    m_LineIndices = index;
    m_LineIndices->Modified();

    m_Colors = colors;
    m_Colors->Modified();

    if (m_Colors != nullptr) { m_UseColor = true; }
    auto m_Manager=iGame::SceneManager::Instance();
    auto painter = m_Manager->GetCurrentScene()->GetPainter3D();
    m_Painter->Clear();

    // 创建画笔
    Pen::Pointer pen = Pen::New();
    // 设置画笔粗细
    pen->SetWidth(10);
    // 设置画笔颜色
    pen->SetColor(Color::Green);
    // 创建画刷
    Brush::Pointer brush = Brush::New();
    // 设置画刷颜色
    brush->SetColor(Color::Red);
    // Painter设置
    m_Painter->SetPen(pen);
    m_Painter->SetBrush(brush);
    for (int i = 0; i < seeds.size(); ++i) { m_Painter->DrawSphere(seeds[i],1.0f,6,6); }
    isUpdate = false;
}

bool StreamBase::IsUseSinglePassWireframeRendering() { return false; }

IGAME_NAMESPACE_END
