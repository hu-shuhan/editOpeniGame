#include "iGameDrawObject.h"

#include "DataProcessing/iGameMeshSimplificationFilterPro.h"
#include "iGameScene.h"
#include "iGameSurfaceMesh.h"

#include "Meshleter/iGameSurfaceMeshMeshleter.h"

#ifdef __EMSCRIPTEN__
    #include <chrono>
    #include <iomanip>
    #include <iostream>
    #include <sstream>
#endif

#include <utility>

IGAME_NAMESPACE_BEGIN
#ifdef __EMSCRIPTEN__
namespace
{
using RenderTimingClock = std::chrono::steady_clock;
constexpr int kRenderTimingDrawObjectLogLimit = 10;
int g_renderTimingDrawObjectLogCount = 0;

std::string RenderTimingMs(RenderTimingClock::duration duration) {
    const double ms = std::chrono::duration<double, std::milli>(duration).count();
    std::ostringstream out;
    out << std::fixed << std::setprecision(2) << ms << " ms";
    return out.str();
}

bool ShouldLogRenderTimingDrawObject() {
    if (g_renderTimingDrawObjectLogCount >= kRenderTimingDrawObjectLogLimit) {
        return false;
    }
    ++g_renderTimingDrawObjectLogCount;
    return true;
}
} // namespace
#endif

DrawObject::DrawObject() {
    m_AutoUpdateDrawData = true;
    m_RenderableMesh.SurfaceMesh = nullptr;
    m_RenderableMesh.SimplifiedMesh = nullptr;
    m_RenderableMesh.mMeshleter = nullptr;

    m_PointVAO = GLVertexArray::New();
    m_LineVAO = GLVertexArray::New();
    m_TriangleVAO = GLVertexArray::New();

    m_PositionVBO = GLBuffer::New();
    m_ColorVBO = GLBuffer::New();
    m_NormalVBO = GLBuffer::New();
    m_TextureVBO = GLBuffer::New();

    m_PointEBO = GLBuffer::New();
    m_LineEBO = GLBuffer::New();
    m_TriangleEBO = GLBuffer::New();

    m_CellVAO = GLVertexArray::New();
    m_CellPositionVBO = GLBuffer::New();
    m_CellColorVBO = GLBuffer::New();

    m_Positions = FloatArray::New();
    m_Positions->SetDimension(3);
    m_Colors = FloatArray::New();
    m_Colors->SetDimension(3);
    m_Normals = FloatArray::New();
    m_Normals->SetDimension(3);
    m_Textures = FloatArray::New();
    m_Textures->SetDimension(2);

    m_PointIndices = UnsignedIntArray::New();
    m_PointIndices->SetDimension(1);
    m_LineIndices = UnsignedIntArray::New();
    m_LineIndices->SetDimension(2);
    m_TriangleIndices = UnsignedIntArray::New();
    m_TriangleIndices->SetDimension(3);

    m_UseSinglePassWireframeRendering = true;
    m_TriangleEdgeMasks = UnsignedCharArray::New();
    m_TriangleEdgeMasks->SetDimension(1);
    m_EdgeMaskBuffer = GLBuffer::New();
    m_EdgeMaskTexture = GLTextureBuffer::New();

    m_CellPositions = FloatArray::New();
    m_CellPositions->SetDimension(3);
    m_CellColors = FloatArray::New();
    m_CellColors->SetDimension(3);
    m_CellTriangleEdgeMasks = UnsignedCharArray::New();
    m_CellTriangleEdgeMasks->SetDimension(1);
    m_CellEdgeMaskBuffer = GLBuffer::New();
    m_CellEdgeMaskTexture = GLTextureBuffer::New();

    m_ViewStyle = IG_SURFACE;
    m_Visibility = true;

    m_Flag = false;
    m_UseColor = false;
    m_UseNormalSmooth = false;
    m_ColorWithCell = false;
    m_PointSize = 3.0f;
    m_LineWidth = 1.0f;
    m_CellPositionSize = 0;

    m_PolygonFactor = 0.0f;
    m_PolygonOffset = 0.0f;
    m_LineFactor = 0.0f;
    m_LineOffset = 0.0f;
    m_PointOffset = 0.0f;

    m_Transparency = 1.0f;
    m_ReConvertToDrawableData = false;

    m_Clipper = iGameClipper::New();
    m_DefaultColor = igm::vec3{0.85f, 0.85f, 0.85f};
}

void DrawObject::SetDefaultColor(const igm::vec3& color) {
    m_DefaultColor = color;
    // propagate to renderable meshes
    if (m_RenderableMesh.SurfaceMesh) { m_RenderableMesh.SurfaceMesh->SetDefaultColor(color); }
    if (m_RenderableMesh.SimplifiedMesh) { m_RenderableMesh.SimplifiedMesh->SetDefaultColor(color); }
}

igm::vec3 DrawObject::GetDefaultColor() const { return m_DefaultColor; }

void DrawObject::ConvertToDrawableData() {
    // 当多子块文件时，父节点为DrawObject，在这里处理子块
    if (this->HasSubDataObject()) { ProcessSubDataObjects(&DrawObject::ConvertToDrawableData); }
}

bool DrawObject::IsUseSinglePassWireframeRendering() {
    if (m_TriangleIndices->GetNumberOfElements() && m_TriangleEdgeMasks->GetNumberOfElements()) {
        return true;
    } else {
        return false;
    }
}

IGenum DrawObject::GetDataObjectType() const { return IG_DRAW_OBJECT; }

IGsize DrawObject::GetRealMemorySize() {
    IGsize res = this->DataObject::GetRealMemorySize();
    return res;
}

bool DrawObject::IsUseColor() { return m_UseColor; }
IGsize DrawObject::GetActiveColorBufferElementCount() {
    const auto colors = m_ColorWithCell ? m_CellColors : m_Colors;
    return colors != nullptr ? colors->GetNumberOfElements() : 0u;
}
unsigned int DrawObject::GetActiveColorBufferUpdateId() {
    const auto colors = m_ColorWithCell ? m_CellColors : m_Colors;
    return colors != nullptr ? colors->GetMTime().GetMTime() : 0u;
}
bool DrawObject::IsActiveColorBufferCellBased() const { return m_ColorWithCell; }

bool DrawObject::IsUseNormalSmooth() {
    if (m_UseNormalSmooth && m_Normals->GetNumberOfValues() == 0) {
        IGAME_RENDERING_WARN("You have enabled normal smoothing, but have not provided normals.");
    }
    return m_UseNormalSmooth;
}

void DrawObject::SetVisibility(bool f) {
    // process renderable object
    if (m_RenderableMesh.SurfaceMesh) { m_RenderableMesh.SurfaceMesh->SetVisibility(f); }
    if (m_RenderableMesh.SimplifiedMesh) { m_RenderableMesh.SimplifiedMesh->SetVisibility(f); }

    // process this object
    this->m_Visibility = f;
    if (this->HasSubDataObject()) { ProcessSubDataObjects(&DrawObject::SetVisibility, f); }
}

bool DrawObject::GetVisibility() { return m_Visibility; }

void DrawObject::SetViewStyle(IGenum mode) {
    /*
     * e.g. mode = IG_WIREFRAME | IG_SURFACE, means that the model shows the wireframe and surface.
     * */

    // process renderable object
    if (m_RenderableMesh.SurfaceMesh) { m_RenderableMesh.SurfaceMesh->SetViewStyle(mode); }
    if (m_RenderableMesh.SimplifiedMesh) { m_RenderableMesh.SimplifiedMesh->SetViewStyle(mode); }

    const auto newlyEnabled = mode & ~m_ViewStyle;

    // process this object
    m_ViewStyle = mode;
    // 延迟上传的索引在视图首次启用时重新标记
    if ((newlyEnabled & IG_POINTS) != 0u) { m_PointIndices->Modified(); }
    if ((newlyEnabled & IG_WIREFRAME) != 0u) {
        m_LineIndices->Modified();
        if (m_CanRegenerateLineIndices && m_LineIndices->GetNumberOfValues() == 0u) {
            m_ReConvertToDrawableData = true;
        }
    }
    if ((newlyEnabled & IG_SURFACE) != 0u) { m_TriangleIndices->Modified(); }
    if ((newlyEnabled & (IG_WIREFRAME | IG_SURFACE)) != 0u) {
        m_TriangleEdgeMasks->Modified();
        m_CellTriangleEdgeMasks->Modified();
    }
    if (this->HasSubDataObject()) { ProcessSubDataObjects(&DrawObject::SetViewStyle, mode); }
}

void DrawObject::AddViewStyle(IGenum mode) {
    // process renderable object
    if (m_RenderableMesh.SurfaceMesh) { m_RenderableMesh.SurfaceMesh->AddViewStyle(mode); }
    if (m_RenderableMesh.SimplifiedMesh) { m_RenderableMesh.SimplifiedMesh->AddViewStyle(mode); }

    const auto newlyEnabled = mode & ~m_ViewStyle;

    // process this object
    m_ViewStyle |= mode;
    // 延迟上传的索引在视图首次启用时重新标记
    if ((newlyEnabled & IG_POINTS) != 0u) { m_PointIndices->Modified(); }
    if ((newlyEnabled & IG_WIREFRAME) != 0u) {
        m_LineIndices->Modified();
        if (m_CanRegenerateLineIndices && m_LineIndices->GetNumberOfValues() == 0u) {
            m_ReConvertToDrawableData = true;
        }
    }
    if ((newlyEnabled & IG_SURFACE) != 0u) { m_TriangleIndices->Modified(); }
    if ((newlyEnabled & (IG_WIREFRAME | IG_SURFACE)) != 0u) {
        m_TriangleEdgeMasks->Modified();
        m_CellTriangleEdgeMasks->Modified();
    }
    if (this->HasSubDataObject()) { ProcessSubDataObjects(&DrawObject::AddViewStyle, mode); }
}

void DrawObject::RemoveViewStyle(IGenum mode) {
    // process renderable object
    if (m_RenderableMesh.SurfaceMesh) { m_RenderableMesh.SurfaceMesh->RemoveViewStyle(mode); }
    if (m_RenderableMesh.SimplifiedMesh) { m_RenderableMesh.SimplifiedMesh->RemoveViewStyle(mode); }

    // process this object
    m_ViewStyle &= ~mode;
    if (this->HasSubDataObject()) { ProcessSubDataObjects(&DrawObject::RemoveViewStyle, mode); }
}

unsigned int DrawObject::GetViewStyle() { return m_ViewStyle; }

void DrawObject::AddViewStyleOfModel(IGenum mode) {
    // process renderable object
    if (m_RenderableMesh.SurfaceMesh) { m_RenderableMesh.SurfaceMesh->AddViewStyleOfModel(mode); }
    if (m_RenderableMesh.SimplifiedMesh) { m_RenderableMesh.SimplifiedMesh->AddViewStyleOfModel(mode); }

    // process this object
    auto* parentDrawObject = DynamicCast<DrawObject>(FindParent());
    if (parentDrawObject != this) {
        parentDrawObject->AddViewStyle(mode);
    } else {
        this->AddViewStyle(mode);
    }
}

unsigned int DrawObject::GetViewStyleOfModel() {
    auto* parentDrawObject = DynamicCast<DrawObject>(FindParent());
    if (parentDrawObject != this) {
        return parentDrawObject->GetViewStyle();
    } else {
        return this->GetViewStyle();
    }
}

bool DrawObject::GetClipped() { return false; }; // Gets whether this can be clipped.

iGameClipper::Pointer DrawObject::GetClipper() { return m_Clipper; }

void DrawObject::SetPointSize(float size) {
    if (size < 0) {
        igDebug("Point size cannot be negative. Provided size: {}. Point size has been set to default value: 8.", size);
        size = 8.0f;
    }

    // process renderable object
    if (m_RenderableMesh.SurfaceMesh) { m_RenderableMesh.SurfaceMesh->SetPointSize(size); }
    if (m_RenderableMesh.SimplifiedMesh) { m_RenderableMesh.SimplifiedMesh->SetPointSize(size); }

    // process this object
    m_PointSize = size;
    if (this->HasSubDataObject()) { ProcessSubDataObjects(&DrawObject::SetPointSize, size); }
}

int DrawObject::GetPointSize() { return m_PointSize; }

void DrawObject::SetLineWidth(float size) {
    if (size < 0) {
        igDebug("Line width cannot be negative. Provided size: {}. Line width has been set to default value: 1.", size);
        size = 1.0f;
    }

    // process renderable object
    if (m_RenderableMesh.SurfaceMesh) { m_RenderableMesh.SurfaceMesh->SetLineWidth(size); }
    if (m_RenderableMesh.SimplifiedMesh) { m_RenderableMesh.SimplifiedMesh->SetLineWidth(size); }

    // process this object
    m_LineWidth = size;
    if (this->HasSubDataObject()) { ProcessSubDataObjects(&DrawObject::SetLineWidth, size); }
}

int DrawObject::GetLineWidth() { return m_LineWidth; }

void DrawObject::SetTransparency(float transparency) {
    // process renderable object
    if (m_RenderableMesh.SurfaceMesh) { m_RenderableMesh.SurfaceMesh->SetTransparency(transparency); }
    if (m_RenderableMesh.SimplifiedMesh) { m_RenderableMesh.SimplifiedMesh->SetTransparency(transparency); }

    // process this object
    if (transparency < 0.0f || transparency > 1.0f) { throw std::runtime_error("Transparency must be between 0-1"); }
    m_Transparency = transparency;

    if (this->HasSubDataObject()) { ProcessSubDataObjects(&DrawObject::SetTransparency, transparency); }
}

float DrawObject::GetTransparency() { return m_Transparency; }

bool DrawObject::ViewCloudPicture(
    Scene* scene,
    const int index,
    const int dimension,
    const bool updateScene) {
    const bool changed = ApplyCloudPicture(index, dimension);
    if (changed && updateScene && scene != nullptr) {
        scene->Update();
    }
    return changed;
}

bool DrawObject::ApplyCloudPicture(const int index, const int dimension) {
    if (index == m_AttributeIndex && dimension == m_AttributeDimension) {
        return false;
    }

    auto* attributes = GetAttributeSet();
    const auto attributeCount = attributes != nullptr
        ? attributes->GetNumberOfAttributes()
        : 0;
    if (index < -1 || index >= attributeCount) {
        std::cout << "[Warning] The specified attribute index is out of range." << std::endl;
        return false;
    }

    // Share parent's dataRange to RenderableMesh BEFORE processing
    // This ensures RenderableMesh uses the full model's calculated range
    if (index >= 0) {
        auto& parentAttr = attributes->GetAttribute(index);
        auto parentDataRange = parentAttr.GetDataRange();

        if (m_RenderableMesh.SurfaceMesh &&
            m_RenderableMesh.SurfaceMesh->GetAttributeSet()->GetNumberOfAttributes() > index) {
            m_RenderableMesh.SurfaceMesh->GetAttributeSet()->GetAttribute(index).dataRange = parentDataRange;
        }
        if (m_RenderableMesh.SimplifiedMesh &&
            m_RenderableMesh.SimplifiedMesh->GetAttributeSet()->GetNumberOfAttributes() > index) {
            m_RenderableMesh.SimplifiedMesh->GetAttributeSet()->GetAttribute(index).dataRange = parentDataRange;
        }
    }

    if (m_RenderableMesh.SurfaceMesh &&
        (index == -1 ||
         m_RenderableMesh.SurfaceMesh->GetAttributeSet()->GetNumberOfAttributes() > index)) {
        m_RenderableMesh.SurfaceMesh->ApplyCloudPicture(index, dimension);
    }
    if (m_RenderableMesh.SimplifiedMesh &&
        (index == -1 ||
         m_RenderableMesh.SimplifiedMesh->GetAttributeSet()->GetNumberOfAttributes() > index)) {
        m_RenderableMesh.SimplifiedMesh->ApplyCloudPicture(index, dimension);
    }
    if (this->HasSubDataObject()) {
        for (auto iterator = m_SubDataObjectsHelper->Begin();
             iterator != m_SubDataObjectsHelper->End();
            ++iterator) {
            if (auto child = DynamicCast<DrawObject>(iterator->second); child != nullptr) {
                child->ApplyCloudPicture(index, dimension);
            }
        }
    }

    // 记录当前激活的属性索引和维度到 m_AttributeIndex / m_AttributeDimension
    if (index == -1) {
        m_AttributeIndex = -1;
        m_AttributeDimension = -1;
        m_UseColor = false;
    } else {
        m_AttributeIndex = index;
        m_AttributeDimension = dimension;
        m_UseColor = true;
    }
    m_AttributeChanged = true;
    return true;
}

void DrawObject::ViewCloudPictureOfModel(Scene* scene, int index, int dimension) {
    auto* parent = dynamic_cast<DrawObject*>(FindParent());
    if (parent != nullptr && parent != this) {
        parent->ViewCloudPicture(scene, index, dimension);
    } else {
        this->ViewCloudPicture(scene, index, dimension);
    }
}

FloatArray::Pointer DrawObject::GetRenderPoints() {
    // return renderable object
    if (m_RenderableMesh.SurfaceMesh) { return m_RenderableMesh.SurfaceMesh->m_Positions; }

    // return this object
    return m_Positions;
}
void DrawObject::SetRenderPoints(FloatArray::Pointer points) { m_Positions = std::move(points); }

UnsignedIntArray::Pointer DrawObject::GetRenderTriangleIndices() {
    if (m_RenderableMesh.SurfaceMesh) {
        return m_RenderableMesh.SurfaceMesh->m_TriangleIndices;
    }
    return m_TriangleIndices;
}

UnsignedCharArray::Pointer DrawObject::GetRenderTriangleEdgeMasks() {
    if (m_RenderableMesh.SurfaceMesh) {
        return m_RenderableMesh.SurfaceMesh->m_TriangleEdgeMasks;
    }
    return m_TriangleEdgeMasks;
}

void DrawObject::SetSharedRenderData(
    FloatArray::Pointer positions,
    UnsignedIntArray::Pointer triangleIndices,
    UnsignedCharArray::Pointer triangleEdgeMasks) {
    m_Positions = std::move(positions);
    m_TriangleIndices = std::move(triangleIndices);
    m_TriangleEdgeMasks = std::move(triangleEdgeMasks);
    m_CanRegenerateLineIndices = false;
    m_LineIndices->Reset();
    m_LineIndices->SetDimension(2);
    m_PointIndices->Reset();
    m_PointIndices->SetDimension(1);
    if (m_Positions != nullptr) { m_Positions->Modified(); }
    if (m_TriangleIndices != nullptr) { m_TriangleIndices->Modified(); }
    if (m_TriangleEdgeMasks != nullptr) { m_TriangleEdgeMasks->Modified(); }
    m_ReConvertToDrawableData = false;
    m_ReConvertHelper->Modified();
    Modified();
}

// void DrawObject::SetPolygonOffsetParameters(float factor, float units) {
//     // process renderable object
//     if (m_RenderableMesh.SurfaceMesh) {
//         m_RenderableMesh.SurfaceMesh->SetPolygonOffsetParameters(factor, units);
//     }
//     if (m_RenderableMesh.SimplifiedMesh) {
//         m_RenderableMesh.SimplifiedMesh->SetPolygonOffsetParameters(factor, units);
//     }
//
//     // process this object
//     this->m_PolygonFactor = factor;
//     this->m_PolygonOffset = units;
//     this->Modified();
// }
//
// void DrawObject::GetPolygonOffsetParameters(float& factor, float& units) {
//     factor = this->m_PolygonFactor;
//     units = this->m_PolygonOffset;
// }
//
// void DrawObject::SetLineOffsetParameters(float factor, float units) {
//     // process renderable object
//     if (m_RenderableMesh.SurfaceMesh) {
//         m_RenderableMesh.SurfaceMesh->SetLineOffsetParameters(factor, units);
//     }
//     if (m_RenderableMesh.SimplifiedMesh) {
//         m_RenderableMesh.SimplifiedMesh->SetLineOffsetParameters(factor, units);
//     }
//
//     // process this object
//     this->m_LineFactor = factor;
//     this->m_LineOffset = units;
//     this->Modified();
// }
//
// void DrawObject::GetLineOffsetParameters(float& factor, float& units) {
//     factor = this->m_LineFactor;
//     units = this->m_LineOffset;
// }
//
// void DrawObject::SetPointOffsetParameters(float units) {
//     // process renderable object
//     if (m_RenderableMesh.SurfaceMesh) {
//         m_RenderableMesh.SurfaceMesh->SetPointOffsetParameters(units);
//     }
//     if (m_RenderableMesh.SimplifiedMesh) {
//         m_RenderableMesh.SimplifiedMesh->SetPointOffsetParameters(units)
//     }
//
//     // process this object
//     this->m_PointOffset = units;
//     this->Modified();
// }
//
// void DrawObject::GetPointOffsetParameters(float& units) { units = this->m_PointOffset; }

void DrawObject::SetRenderableObject(DataObject::Pointer dataObject) {
    if (dataObject->GetDataObjectType() != IG_SURFACE_MESH) {
        igDebug("Only SurfaceMesh type objects can be set as renderable objects.");
        return;
    }

    if (this->GetDataObjectType() == IG_SURFACE_MESH) {
        m_RenderableMesh.SurfaceMesh = nullptr;
    } else {
        m_RenderableMesh.SurfaceMesh = DynamicCast<DrawObject>(dataObject);
        SyncRenderableState(m_RenderableMesh.SurfaceMesh);
        // After the first extraction, if the "m_Positions" is not updated, the shell will be extracted repeatedly
        m_Positions->Modified();
    }

    // Build simplified mesh lazily in GetRenderableObject(true)
    m_RenderableMesh.SimplifiedMesh = nullptr;
    m_SimplifiedMeshBuildAttempted = false;

    // 设置Meshleter
    m_RenderableMesh.mMeshleter = SurfaceMeshMeshleter::New();
    m_RenderableMesh.mMeshleter->SetInput(dataObject);
}

DrawObject::Pointer DrawObject::GetRenderableObject(bool useSimplified) {
    if (!m_ShellRendering) { return this; }

    if (useSimplified && m_RenderableMesh.SimplifiedMesh == nullptr && !m_SimplifiedMeshBuildAttempted) {
        BuildSimplifiedRenderableObject();
    }

    if (useSimplified && m_RenderableMesh.SimplifiedMesh != nullptr) { return m_RenderableMesh.SimplifiedMesh; }
    if (m_RenderableMesh.SurfaceMesh != nullptr) { return m_RenderableMesh.SurfaceMesh; }
    return this;
}

void DrawObject::BuildSimplifiedRenderableObject() {
    m_SimplifiedMeshBuildAttempted = true;

    DrawObject::Pointer sourceMesh = nullptr;
    if (m_RenderableMesh.SurfaceMesh != nullptr) {
        sourceMesh = m_RenderableMesh.SurfaceMesh;
    } else if (this->GetDataObjectType() == IG_SURFACE_MESH) {
        sourceMesh = this;
    }

    if (sourceMesh == nullptr) { return; }

    auto simplifiedMesh = sourceMesh;
    MeshSimplificationFilterPro::Pointer meshSimplifier = MeshSimplificationFilterPro::New();
    meshSimplifier->SetInput(sourceMesh);
    meshSimplifier->SetPreserveBoundary(true);
    meshSimplifier->SetFreeze(false);
    meshSimplifier->SetTransformToCellData(false);
    meshSimplifier->SetTargetReduction(0.2);
    if (meshSimplifier->Execute()) {
        auto outputMesh = DynamicCast<DrawObject>(meshSimplifier->GetOutput());
        if (outputMesh != nullptr) { simplifiedMesh = outputMesh; }
    }

    m_RenderableMesh.SimplifiedMesh = simplifiedMesh;
    SyncRenderableState(m_RenderableMesh.SimplifiedMesh);
}

void DrawObject::SyncRenderableState(const DrawObject::Pointer& renderableObject) {
    if (renderableObject == nullptr) { return; }

    renderableObject->m_ViewStyle = this->m_ViewStyle;
    renderableObject->m_Visibility = this->m_Visibility;
    renderableObject->m_UseNormalSmooth = this->m_UseNormalSmooth;
    renderableObject->m_ColorWithCell = this->m_ColorWithCell;
    renderableObject->m_PointSize = this->m_PointSize;
    renderableObject->m_LineWidth = this->m_LineWidth;
    renderableObject->m_Transparency = this->m_Transparency;
    renderableObject->m_AttributeIndex = this->m_AttributeIndex;
    renderableObject->m_AttributeDimension = this->m_AttributeDimension;
    renderableObject->m_UseColor = this->m_UseColor;
    renderableObject->m_ColorMapper = m_ColorMapper;
    renderableObject->m_DefaultColor = this->m_DefaultColor;
    renderableObject->m_IsMainRenderableObject = false;
}

void DrawObject::SetAlwaysOnTop(bool enable) { m_AlwaysOnTop = enable; }

bool DrawObject::IsAlwaysOnTop() const { return m_AlwaysOnTop; }

void DrawObject::SetShellRenderingOption(bool option) {
    if (m_ShellRendering != option) {
        m_ShellRendering = option;
        m_ReConvertToDrawableData = true;
    }
}

bool DrawObject::GetShellRenderingOption() { return m_ShellRendering; }

void DrawObject::SetAccelerationOption(bool enabled) {
#ifdef IGAME_OPENGL_VERSION_330
    IGAME_RENDERING_WARN("Acceleration rendering disabled (OpenGL 3.3 detected). Requires "
                         "OpenGL 4.3+ for hardware acceleration support.");
    return;
#endif

    m_AccelerationOption = enabled;
    if (this->HasSubDataObject()) { ProcessSubDataObjects(&DrawObject::SetAccelerationOption, enabled); }
}

bool DrawObject::GetAccelerationOption() const { return m_AccelerationOption; }

void DrawObject::SetRenderWithMeshlet(bool val) {
    m_RenderableMesh.mMeshleter->SetRenderWithMeshlet(val);
    if (val) { m_ColorWithCell = true; }
}

bool DrawObject::GetRenderWithMeshlet() const { return m_RenderableMesh.mMeshleter->GetRenderWithMeshlet(); }

void DrawObject::CreateDrawBuffer() {
    if (!m_Flag) {
        m_PointVAO->Create();
        m_LineVAO->Create();
        m_TriangleVAO->Create();

        m_PositionVBO->Create();
        m_PositionVBO->Target(GL_ARRAY_BUFFER);
        m_ColorVBO->Create();
        m_ColorVBO->Target(GL_ARRAY_BUFFER);
        m_NormalVBO->Create();
        m_NormalVBO->Target(GL_ARRAY_BUFFER);
        m_TextureVBO->Create();
        m_TextureVBO->Target(GL_ARRAY_BUFFER);
        m_CellVAO->Create();
        m_CellPositionVBO->Create();
        m_CellPositionVBO->Target(GL_ARRAY_BUFFER);
        m_CellColorVBO->Create();
        m_CellColorVBO->Target(GL_ARRAY_BUFFER);

        //// set point drawing format
        //{
        //    m_PointVAO.vertexBuffer(GL_VBO_IDX_0, m_PositionVBO, 0,
        //                            3 * sizeof(float));
        //    m_PointVAO.vertexBuffer(GL_VBO_IDX_1, m_ColorVBO, 0,
        //                            3 * sizeof(float));
        //    m_PointVAO.vertexBuffer(GL_VBO_IDX_2, m_NormalVBO, 0,
        //                            3 * sizeof(float));
        //    m_PointVAO.vertexBuffer(GL_VBO_IDX_3, m_TextureVBO, 0,
        //                            2 * sizeof(float));
        //    GLSetVertexAttrib(m_PointVAO, GL_LOCATION_IDX_0, GL_VBO_IDX_0, 3,
        //                      GL_FLOAT, GL_FALSE, 0);
        //    GLSetVertexAttrib(m_PointVAO, GL_LOCATION_IDX_1, GL_VBO_IDX_1, 3,
        //                      GL_FLOAT, GL_FALSE, 0);
        //    GLSetVertexAttrib(m_PointVAO, GL_LOCATION_IDX_2, GL_VBO_IDX_2, 3,
        //                      GL_FLOAT, GL_FALSE, 0);
        //    GLSetVertexAttrib(m_PointVAO, GL_LOCATION_IDX_3, GL_VBO_IDX_3, 2,
        //                      GL_FLOAT, GL_FALSE, 0);
        //    m_PointVAO.elementBuffer(m_PointEBO);
        //}
        //
        //// set line drawing format
        //{
        //    m_LineVAO.vertexBuffer(GL_VBO_IDX_0, m_PositionVBO, 0,
        //                           3 * sizeof(float));
        //    m_LineVAO.vertexBuffer(GL_VBO_IDX_1, m_ColorVBO, 0,
        //                           3 * sizeof(float));
        //    m_LineVAO.vertexBuffer(GL_VBO_IDX_2, m_NormalVBO, 0,
        //                           3 * sizeof(float));
        //    m_LineVAO.vertexBuffer(GL_VBO_IDX_3, m_TextureVBO, 0,
        //                           2 * sizeof(float));
        //    GLSetVertexAttrib(m_LineVAO, GL_LOCATION_IDX_0, GL_VBO_IDX_0, 3,
        //                      GL_FLOAT, GL_FALSE, 0);
        //    GLSetVertexAttrib(m_LineVAO, GL_LOCATION_IDX_1, GL_VBO_IDX_1, 3,
        //                      GL_FLOAT, GL_FALSE, 0);
        //    GLSetVertexAttrib(m_LineVAO, GL_LOCATION_IDX_2, GL_VBO_IDX_2, 3,
        //                      GL_FLOAT, GL_FALSE, 0);
        //    GLSetVertexAttrib(m_LineVAO, GL_LOCATION_IDX_3, GL_VBO_IDX_3, 2,
        //                      GL_FLOAT, GL_FALSE, 0);
        //    m_LineVAO.elementBuffer(m_LineEBO);
        //}
        //
        //}
        //{
        //    m_TriangleVAO.vertexBuffer(GL_VBO_IDX_0, m_PositionVBO, 0,
        //                               3 * sizeof(float));
        //    m_TriangleVAO.vertexBuffer(GL_VBO_IDX_1, m_ColorVBO, 0,
        //                               3 * sizeof(float));
        //    m_TriangleVAO.vertexBuffer(GL_VBO_IDX_2, m_NormalVBO, 0,
        //                               3 * sizeof(float));
        //    m_TriangleVAO.vertexBuffer(GL_VBO_IDX_3, m_TextureVBO, 0,
        //                               2 * sizeof(float));
        //    GLSetVertexAttrib(m_TriangleVAO, GL_LOCATION_IDX_0, GL_VBO_IDX_0, 3,
        //                      GL_FLOAT, GL_FALSE, 0);
        //    GLSetVertexAttrib(m_TriangleVAO, GL_LOCATION_IDX_1, GL_VBO_IDX_1, 3,
        //                      GL_FLOAT, GL_FALSE, 0);
        //    GLSetVertexAttrib(m_TriangleVAO, GL_LOCATION_IDX_2, GL_VBO_IDX_2, 3,
        //                      GL_FLOAT, GL_FALSE, 0);
        //    GLSetVertexAttrib(m_TriangleVAO, GL_LOCATION_IDX_3, GL_VBO_IDX_3, 2,
        //                      GL_FLOAT, GL_FALSE, 0);
        //    m_TriangleVAO.elementBuffer(m_TriangleEBO);
        //}
        //
        //// set cell drawing format
        //{
        //    m_CellVAO.vertexBuffer(GL_VBO_IDX_0, m_CellPositionVBO, 0,
        //                           3 * sizeof(float));
        //    m_CellVAO.vertexBuffer(GL_VBO_IDX_1, m_CellColorVBO, 0,
        //                           3 * sizeof(float));
        //    GLSetVertexAttrib(m_CellVAO, GL_LOCATION_IDX_0, GL_VBO_IDX_0, 3,
        //                      GL_FLOAT, GL_FALSE, 0);
        //    GLSetVertexAttrib(m_CellVAO, GL_LOCATION_IDX_1, GL_VBO_IDX_1, 3,
        //                      GL_FLOAT, GL_FALSE, 0);
        //    m_CellVAO.elementBuffer(m_CellEBO);
        //}

        m_Flag = true;
    }

    if (m_PointEBO->Handle() == 0u) {
        m_PointEBO->Create();
        m_PointEBO->Target(GL_ELEMENT_ARRAY_BUFFER);
    }
    if (m_LineEBO->Handle() == 0u) {
        m_LineEBO->Create();
        m_LineEBO->Target(GL_ELEMENT_ARRAY_BUFFER);
    }
    if (m_TriangleEBO->Handle() == 0u) {
        m_TriangleEBO->Create();
        m_TriangleEBO->Target(GL_ELEMENT_ARRAY_BUFFER);
    }

#ifndef __EMSCRIPTEN__
    bool bindEdgeMaskTexture = false;
    if (m_EdgeMaskBuffer->Handle() == 0u) {
        m_EdgeMaskBuffer->Create();
        m_EdgeMaskBuffer->Target(GL_TEXTURE_BUFFER);
        m_EdgeMaskBuffer->Allocate(sizeof(unsigned char), nullptr, GL_STATIC_DRAW);
        bindEdgeMaskTexture = true;
    }
    if (m_EdgeMaskTexture->Handle() == 0u) {
        m_EdgeMaskTexture->Create();
        bindEdgeMaskTexture = true;
    }
    if (bindEdgeMaskTexture) {
        m_EdgeMaskTexture->Buffer(GL_R8, m_EdgeMaskBuffer);
    }

    bool bindCellEdgeMaskTexture = false;
    if (m_CellEdgeMaskBuffer->Handle() == 0u) {
        m_CellEdgeMaskBuffer->Create();
        m_CellEdgeMaskBuffer->Target(GL_TEXTURE_BUFFER);
        m_CellEdgeMaskBuffer->Allocate(sizeof(unsigned char), nullptr, GL_STATIC_DRAW);
        bindCellEdgeMaskTexture = true;
    }
    if (m_CellEdgeMaskTexture->Handle() == 0u) {
        m_CellEdgeMaskTexture->Create();
        bindCellEdgeMaskTexture = true;
    }
    if (bindCellEdgeMaskTexture) {
        m_CellEdgeMaskTexture->Buffer(GL_R8, m_CellEdgeMaskBuffer);
    }
#endif

    GLCheckError();
}

void DrawObject::SyncGpuBuffers() {
#ifdef __EMSCRIPTEN__
    const auto syncStart = RenderTimingClock::now();
#endif
    // 多子块文件只需要处理子块
    if (this->HasSubDataObject()) {
        ProcessSubDataObjects(&DrawObject::SyncGpuBuffers);
#ifdef __EMSCRIPTEN__
        const auto syncEnd = RenderTimingClock::now();
        if (syncEnd - syncStart > std::chrono::milliseconds(1) && ShouldLogRenderTimingDrawObject()) {
            std::cout << "[iGameWeb][INFO] [Render timing drawobject] object=" << this
                      << " kind=composite subObjects=true sync-total=" << RenderTimingMs(syncEnd - syncStart) << '\n';
        }
#endif
        return;
    }

#ifdef __EMSCRIPTEN__
    const bool wasDirtyBeforeConvert = m_ReConvertToDrawableData;
    auto convertStart = syncStart;
    auto convertEnd = syncStart;
#endif
    if (m_AutoUpdateDrawData) {
#ifdef __EMSCRIPTEN__
        convertStart = RenderTimingClock::now();
#endif
        ConvertToDrawableData();
#ifdef __EMSCRIPTEN__
        convertEnd = RenderTimingClock::now();
#endif
    }

    const bool needsPoints = (m_ViewStyle & IG_POINTS) != 0u;
    const bool needsSurface = (m_ViewStyle & IG_SURFACE) != 0u;
    const bool needsWireframe = (m_ViewStyle & IG_WIREFRAME) != 0u;
#ifdef __EMSCRIPTEN__
    const bool needsLineIndices = needsWireframe;
#else
    const bool needsLineIndices = needsWireframe &&
        !(needsSurface && IsUseSinglePassWireframeRendering());
#endif
    const bool needsTriangleEdgeMasks = needsSurface && needsWireframe &&
        IsUseSinglePassWireframeRendering();

    // 处理其抽壳后的表面网格
    if (m_RenderableMesh.SurfaceMesh) { m_RenderableMesh.SurfaceMesh->SyncGpuBuffers(); }
    if (m_RenderableMesh.SimplifiedMesh) { m_RenderableMesh.SimplifiedMesh->SyncGpuBuffers(); }

    // 当是表面网格时，还需要构建meshlet
    if (m_AccelerationOption) { m_RenderableMesh.mMeshleter->SyncGpuBuffers(); }

    this->CreateDrawBuffer();
    // 引用拓扑帧可以复用已上传的索引缓冲，新VAO仍需绑定共享EBO
    if (needsPoints && m_PointEBO != nullptr && m_PointEBO->Handle() != 0u) {
        m_PointVAO->ElementBuffer(m_PointEBO);
    }
    if (needsLineIndices && m_LineEBO != nullptr && m_LineEBO->Handle() != 0u) {
        m_LineVAO->ElementBuffer(m_LineEBO);
    }
    if (needsSurface && m_TriangleEBO != nullptr && m_TriangleEBO->Handle() != 0u) {
        m_TriangleVAO->ElementBuffer(m_TriangleEBO);
    }
    if (m_Positions->GetMTime() > m_PositionVBO->GetMTime()) {
        GLAllocateGLBuffer(m_PositionVBO, m_Positions->GetNumberOfValues() * sizeof(float), m_Positions->RawPointer());
        m_PositionVBO->Modified();
        SetPositionBufferToVAO(m_PointVAO, m_PositionVBO);
        SetPositionBufferToVAO(m_LineVAO, m_PositionVBO);
        SetPositionBufferToVAO(m_TriangleVAO, m_PositionVBO);
    }

    if (m_Colors->GetMTime() > m_ColorVBO->GetMTime()) {
        GLAllocateGLBuffer(m_ColorVBO, m_Colors->GetNumberOfValues() * sizeof(float), m_Colors->RawPointer());
        m_ColorVBO->Modified();
        SetColorBufferToVAO(m_PointVAO, m_ColorVBO);
        SetColorBufferToVAO(m_LineVAO, m_ColorVBO);
        SetColorBufferToVAO(m_TriangleVAO, m_ColorVBO);
    }

    if (m_Normals->GetMTime() > m_NormalVBO->GetMTime()) {
        GLAllocateGLBuffer(m_NormalVBO, m_Normals->GetNumberOfValues() * sizeof(float), m_Normals->RawPointer());
        m_NormalVBO->Modified();

        SetNormalBufferToVAO(m_PointVAO, m_NormalVBO);
        SetNormalBufferToVAO(m_LineVAO, m_NormalVBO);
        SetNormalBufferToVAO(m_TriangleVAO, m_NormalVBO);
    }

    if (m_Textures->GetMTime() > m_TextureVBO->GetMTime()) {
        GLAllocateGLBuffer(m_TextureVBO, m_Textures->GetNumberOfValues() * sizeof(float), m_Textures->RawPointer());
        m_TextureVBO->Modified();

        SetTextureBufferToVAO(m_PointVAO, m_TextureVBO);
        SetTextureBufferToVAO(m_LineVAO, m_TextureVBO);
        SetTextureBufferToVAO(m_TriangleVAO, m_TextureVBO);
    }

    // 仅上传当前视图会消费的索引缓冲
    if (needsPoints && m_PointIndices->GetMTime() > m_PointEBO->GetMTime()) {
        GLAllocateGLBuffer(m_PointEBO, m_PointIndices->GetNumberOfValues() * sizeof(igIndex),
                           m_PointIndices->RawPointer());
        m_PointEBO->Modified();

        m_PointVAO->ElementBuffer(m_PointEBO);
    }

    if (needsLineIndices && m_LineIndices->GetMTime() > m_LineEBO->GetMTime()) {
        GLAllocateGLBuffer(m_LineEBO, m_LineIndices->GetNumberOfValues() * sizeof(igIndex),
                           m_LineIndices->RawPointer());
        m_LineEBO->Modified();

        m_LineVAO->ElementBuffer(m_LineEBO);
    }

    if (needsSurface && m_TriangleIndices->GetMTime() > m_TriangleEBO->GetMTime()) {
        GLAllocateGLBuffer(m_TriangleEBO, m_TriangleIndices->GetNumberOfValues() * sizeof(igIndex),
                           m_TriangleIndices->RawPointer());
        m_TriangleEBO->Modified();

        m_TriangleVAO->ElementBuffer(m_TriangleEBO);
    }

#ifndef __EMSCRIPTEN__
    if (needsTriangleEdgeMasks &&
        m_TriangleEdgeMasks->GetMTime() > m_EdgeMaskBuffer->GetMTime()) {
        GLAllocateGLBuffer(m_EdgeMaskBuffer, m_TriangleEdgeMasks->GetNumberOfValues() * sizeof(unsigned char),
                           m_TriangleEdgeMasks->RawPointer());
        m_EdgeMaskBuffer->Modified();

        m_EdgeMaskTexture->Buffer(GL_R8, m_EdgeMaskBuffer);
    }
#endif

    if (m_CellPositions->GetMTime() > m_CellPositionVBO->GetMTime()) {
        GLAllocateGLBuffer(m_CellPositionVBO, m_CellPositions->GetNumberOfValues() * sizeof(float),
                           m_CellPositions->RawPointer());
        m_CellPositionVBO->Modified();

        SetPositionBufferToVAO(m_CellVAO, m_CellPositionVBO);
    }

    if (m_CellColors->GetMTime() > m_CellColorVBO->GetMTime()) {
        GLAllocateGLBuffer(m_CellColorVBO, m_CellColors->GetNumberOfValues() * sizeof(float),
                           m_CellColors->RawPointer());
        m_CellColorVBO->Modified();

        SetColorBufferToVAO(m_CellVAO, m_CellColorVBO);
    }

#ifndef __EMSCRIPTEN__
    if (needsTriangleEdgeMasks &&
        m_CellTriangleEdgeMasks->GetMTime() > m_CellEdgeMaskBuffer->GetMTime()) {
        GLAllocateGLBuffer(m_CellEdgeMaskBuffer, m_CellTriangleEdgeMasks->GetNumberOfValues() * sizeof(unsigned char),
                           m_CellTriangleEdgeMasks->RawPointer());
        m_CellEdgeMaskBuffer->Modified();

        m_CellEdgeMaskTexture->Buffer(GL_R8, m_CellEdgeMaskBuffer);
    }
#endif

    GLCheckError();
#ifdef __EMSCRIPTEN__
    const auto syncEnd = RenderTimingClock::now();
    if ((wasDirtyBeforeConvert || syncEnd - syncStart > std::chrono::milliseconds(1)) &&
        ShouldLogRenderTimingDrawObject()) {
        std::cout << "[iGameWeb][INFO] [Render timing drawobject] object=" << this
                  << " type=" << this->GetDataObjectType()
                  << " dirty-before=" << (wasDirtyBeforeConvert ? "true" : "false")
                  << " auto-update=" << (m_AutoUpdateDrawData ? "true" : "false")
                  << " convert=" << RenderTimingMs(convertEnd - convertStart)
                  << " sync-total=" << RenderTimingMs(syncEnd - syncStart)
                  << " positions=" << m_Positions->GetNumberOfElements()
                  << " triangles=" << m_TriangleIndices->GetNumberOfValues()
                  << " lines=" << m_LineIndices->GetNumberOfValues() << '\n';
    }
#endif
}

void DrawObject::SetPositionBufferToVAO(GLVertexArray::Pointer VAO, GLBuffer::Pointer VBO) {
    VAO->VertexBuffer(GL_VBO_IDX_0, VBO, 0, 3 * sizeof(float));
    GLSetVertexAttrib(VAO, GL_LOCATION_IDX_0, GL_VBO_IDX_0, 3, GL_FLOAT, GL_FALSE, 0);
}
void DrawObject::SetColorBufferToVAO(GLVertexArray::Pointer VAO, GLBuffer::Pointer VBO) {
    VAO->VertexBuffer(GL_VBO_IDX_1, VBO, 0, 3 * sizeof(float));
    GLSetVertexAttrib(VAO, GL_LOCATION_IDX_1, GL_VBO_IDX_1, 3, GL_FLOAT, GL_FALSE, 0);
}
void DrawObject::SetNormalBufferToVAO(GLVertexArray::Pointer VAO, GLBuffer::Pointer VBO) {
    VAO->VertexBuffer(GL_VBO_IDX_2, VBO, 0, 3 * sizeof(float));
    GLSetVertexAttrib(VAO, GL_LOCATION_IDX_2, GL_VBO_IDX_2, 3, GL_FLOAT, GL_FALSE, 0);
}
void DrawObject::SetTextureBufferToVAO(GLVertexArray::Pointer VAO, GLBuffer::Pointer VBO) {
    VAO->VertexBuffer(GL_VBO_IDX_3, VBO, 0, 2 * sizeof(float));
    GLSetVertexAttrib(VAO, GL_LOCATION_IDX_3, GL_VBO_IDX_3, 2, GL_FLOAT, GL_FALSE, 0);
}
void DrawObject::ForceReConvertToDrawableData() {
    if (m_SharedTopologyDrawBuffers) {
        m_PointEBO = GLBuffer::New();
        m_LineEBO = GLBuffer::New();
        m_TriangleEBO = GLBuffer::New();
        m_EdgeMaskBuffer = GLBuffer::New();
        m_EdgeMaskTexture = GLTextureBuffer::New();
        m_CellEdgeMaskBuffer = GLBuffer::New();
        m_CellEdgeMaskTexture = GLTextureBuffer::New();
        m_SharedTopologyDrawBuffers = false;
    }
    m_ReConvertToDrawableData = true;
    if (this->HasSubDataObject()) { ProcessSubDataObjects(&DrawObject::ForceReConvertToDrawableData); }
}

void DrawObject::ReuseTopologyDrawDataFrom(const DrawObject* source) {
    if (source == nullptr || source == this) { return; }
    m_PointIndices = source->m_PointIndices;
    m_LineIndices = source->m_LineIndices;
    m_TriangleIndices = source->m_TriangleIndices;
    m_TriangleEdgeMasks = source->m_TriangleEdgeMasks;
    m_CellTriangleEdgeMasks = source->m_CellTriangleEdgeMasks;
    m_PointEBO = source->m_PointEBO;
    m_LineEBO = source->m_LineEBO;
    m_TriangleEBO = source->m_TriangleEBO;
    m_EdgeMaskBuffer = source->m_EdgeMaskBuffer;
    m_EdgeMaskTexture = source->m_EdgeMaskTexture;
    m_CellEdgeMaskBuffer = source->m_CellEdgeMaskBuffer;
    m_CellEdgeMaskTexture = source->m_CellEdgeMaskTexture;
    m_UseSinglePassWireframeRendering = source->m_UseSinglePassWireframeRendering;
    m_CanRegenerateLineIndices = source->m_CanRegenerateLineIndices;
    m_SharedTopologyDrawBuffers = true;
    m_ReConvertToDrawableData = false;
    m_ReConvertHelper->Modified();
}

IGAME_NAMESPACE_END
