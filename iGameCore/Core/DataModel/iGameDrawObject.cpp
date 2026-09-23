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

#include <algorithm>
#include <iostream>
#include <utility>
#include <functional>
#include <unordered_set>

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
    m_Colors->SetDimension(4);
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
    m_CellColors->SetDimension(4);
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
    m_LineColor = igm::vec3{0.0f, 0.0f, 0.0f};
}

void DrawObject::SetDefaultColor(const igm::vec3& color) {
    m_DefaultColor = color;
    // propagate to renderable meshes
    if (m_RenderableMesh.SurfaceMesh) { m_RenderableMesh.SurfaceMesh->SetDefaultColor(color); }
    if (m_RenderableMesh.SimplifiedMesh) { m_RenderableMesh.SimplifiedMesh->SetDefaultColor(color); }
}

igm::vec3 DrawObject::GetDefaultColor() const { return m_DefaultColor; }

void DrawObject::SetLineColor(const igm::vec3& color) {
    m_LineColor = color;
    if (m_RenderableMesh.SurfaceMesh) { m_RenderableMesh.SurfaceMesh->SetLineColor(color); }
    if (m_RenderableMesh.SimplifiedMesh) { m_RenderableMesh.SimplifiedMesh->SetLineColor(color); }
    if (this->HasSubDataObject()) { ProcessSubDataObjects(&DrawObject::SetLineColor, color); }
}

igm::vec3 DrawObject::GetLineColor() const { return m_LineColor; }

void DrawObject::ConvertToDrawableData() {
    // 当多子块文件时，父节点为DrawObject，在这里处理子块
    if (this->HasSubDataObject()) { ProcessSubDataObjects(&DrawObject::ConvertToDrawableData); }
}

bool DrawObject::IsUseSinglePassWireframeRendering() {
    if (m_TriangleIndices->GetNumberOfElements() && m_TriangleEdgeMasks->GetNumberOfElements() &&
        (!m_RemoteRenderingEnabled || (m_ColorWithCell ? m_CellEdgeMaskAvailable : m_EdgeMaskAvailable))) {
        return true;
    } else {
        return false;
    }
}

void DrawObject::SetRemoteRenderingEnabled(bool enabled) {
    std::unordered_set<DataObject*> visited;
    std::function<void(DataObject*)> apply = [&](DataObject* node) {
        if (!node || !visited.insert(node).second) return;
        if (auto* draw = dynamic_cast<DrawObject*>(node)) {
            if (draw->m_RemoteRenderingEnabled != enabled) {
                draw->m_RemoteRenderingEnabled = enabled;
                // Rebuild existing CPU geometry when switching policies; no GL work here.
                draw->m_ReConvertToDrawableData = true;
                draw->m_TriangleEdgeMasks->Modified();
                draw->m_CellTriangleEdgeMasks->Modified();
                draw->m_ConstantEdgeMask = draw->m_ConstantCellEdgeMask = -1;
                draw->m_EdgeMaskAvailable = draw->m_CellEdgeMaskAvailable = false;
            }
            apply(draw->m_RenderableMesh.SurfaceMesh.get());
            apply(draw->m_RenderableMesh.SimplifiedMesh.get());
        }
        if (node->HasSubDataObject()) {
            for (auto it = node->SubDataObjectIteratorBegin(); it != node->SubDataObjectIteratorEnd(); ++it)
                apply(it->second.get());
        }
    };
    apply(this);
}

bool DrawObject::NeedsExplicitWireframeGeometry(IGenum viewStyle) {
    if (!m_RemoteRenderingEnabled) return true; // main eagerly prepares explicit edges.
    if ((viewStyle & IG_WIREFRAME) == 0) { return false; }
#ifdef __EMSCRIPTEN__
    return true;
#else
    // Opaque desktop Surface+Wireframe uses triangle edge masks. Pure
    // wireframe and alternate rendering paths still require a line EBO.
    return (viewStyle & IG_SURFACE) == 0 || m_Transparency < 1.0f || m_AccelerationOption ||
           m_ForceExplicitWireframeGeometry || GetOpacityMappingEnabled();
#endif
}

void DrawObject::MarkWireframeGeometryDirtyIfNeeded(IGenum viewStyle) {
    if (!m_RemoteRenderingEnabled) return;
    // Composite parents only forward state, and shell-rendered sources forward
    // drawing to their SurfaceMesh. Mark only the leaf owning the draw arrays.
    if (this->HasSubDataObject() || m_RenderableMesh.SurfaceMesh) { return; }
    if (NeedsExplicitWireframeGeometry(viewStyle) && m_LineIndices->GetNumberOfElements() == 0) {
        m_ReConvertToDrawableData = true;
    }
}

IGenum DrawObject::GetDataObjectType() const { return IG_DRAW_OBJECT; }

bool DrawObject::HasGpuResources() const {
    std::unordered_set<const DataObject*> visited;
    auto live = [](const auto& object) { return object && object->Handle() != 0; };
    std::function<bool(DataObject*)> inspect = [&](DataObject* node) {
        if (!node || !visited.insert(node).second) { return false; }
        if (auto* draw = dynamic_cast<DrawObject*>(node)) {
            if (live(draw->m_PointVAO) || live(draw->m_LineVAO) || live(draw->m_TriangleVAO) ||
                live(draw->m_CellVAO) || live(draw->m_PositionVBO) || live(draw->m_ColorVBO) ||
                live(draw->m_NormalVBO) || live(draw->m_TextureVBO) || live(draw->m_PointEBO) ||
                live(draw->m_LineEBO) || live(draw->m_TriangleEBO) ||
                live(draw->m_CellPositionVBO) || live(draw->m_CellColorVBO) ||
                live(draw->m_EdgeMaskBuffer) || live(draw->m_EdgeMaskTexture) ||
                live(draw->m_CellEdgeMaskBuffer) || live(draw->m_CellEdgeMaskTexture) ||
                (draw->m_RenderableMesh.mMeshleter && draw->m_RenderableMesh.mMeshleter->HasGpuResources())) {
                return true;
            }
            if (inspect(draw->m_RenderableMesh.SurfaceMesh.get()) ||
                inspect(draw->m_RenderableMesh.SimplifiedMesh.get())) { return true; }
        }
        if (node->HasSubDataObject()) {
            for (auto it = node->SubDataObjectIteratorBegin(); it != node->SubDataObjectIteratorEnd(); ++it) {
                if (inspect(it->second.get())) { return true; }
            }
        }
        return false;
    };
    // Existing hierarchy accessors are not const-qualified; inspection itself
    // reads only identities and GL handle metadata, never arrays or GL state.
    return inspect(const_cast<DrawObject*>(this));
}

void DrawObject::ReleaseDrawableResources() {
    std::unordered_set<const DataObject*> visited;
    auto destroy = [](const auto& object) { if (object) { object->Destroy(); } };
    auto floatArray = [](int dimension) {
        auto result = FloatArray::New(); result->SetDimension(dimension); return result;
    };
    auto indexArray = [](int dimension) {
        auto result = UnsignedIntArray::New(); result->SetDimension(dimension); return result;
    };
    auto masksArray = []() {
        auto result = UnsignedCharArray::New(); result->SetDimension(1); return result;
    };
    std::function<void(DataObject*)> release = [&](DataObject* node) {
        if (!node || !visited.insert(node).second) { return; }
        // Breaking Surface -> Meshleter -> Surface or fallback-LOD self cycles
        // must not destroy the object while this routine still operates on it.
        DataObject::Pointer keepAlive = node;
        if (auto* draw = dynamic_cast<DrawObject*>(node)) {
            auto surface = draw->m_RenderableMesh.SurfaceMesh;
            auto simplified = draw->m_RenderableMesh.SimplifiedMesh;
            auto meshleter = draw->m_RenderableMesh.mMeshleter;
            if (meshleter) {
                draw->m_RestoreMeshletColoring = meshleter->GetRenderWithMeshlet();
                meshleter->ReleaseGpuBuffers();
                meshleter->SetInput(nullptr);
            }
            draw->m_RenderableMesh = {};
            release(surface.get());
            release(simplified.get());

            // Remove all VAO/texture references before deleting buffer handles.
            // Destroy is a no-op for zero handles, including CPU-only preloads.
            destroy(draw->m_PointVAO); destroy(draw->m_LineVAO); destroy(draw->m_TriangleVAO);
            destroy(draw->m_CellVAO); destroy(draw->m_EdgeMaskTexture); destroy(draw->m_CellEdgeMaskTexture);
            destroy(draw->m_PositionVBO); destroy(draw->m_ColorVBO); destroy(draw->m_NormalVBO);
            destroy(draw->m_TextureVBO); destroy(draw->m_PointEBO); destroy(draw->m_LineEBO);
            destroy(draw->m_TriangleEBO); destroy(draw->m_CellPositionVBO); destroy(draw->m_CellColorVBO);
            destroy(draw->m_EdgeMaskBuffer); destroy(draw->m_CellEdgeMaskBuffer);
            draw->m_Flag = false;
            draw->m_CellPositionSize = 0;

            // NEVER Reset/Initialize these old arrays: positions can alias the
            // original Points backing array. Drop references, preserving raw
            // coordinates, connectivity, scalar values and their timestamps.
            draw->m_Positions = floatArray(3);
            draw->m_Colors = floatArray(4);
            draw->m_Normals = floatArray(3);
            draw->m_Textures = floatArray(2);
            draw->m_PointIndices = indexArray(1);
            draw->m_LineIndices = indexArray(2);
            draw->m_TriangleIndices = indexArray(3);
            draw->m_TriangleEdgeMasks = masksArray();
            draw->m_ConstantEdgeMask = -1;
            draw->m_EdgeMaskAvailable = false;
            draw->m_CellPositions = floatArray(3);
            draw->m_CellColors = floatArray(4);
            draw->m_CellTriangleEdgeMasks = masksArray();
            draw->m_ConstantCellEdgeMask = -1;
            draw->m_CellEdgeMaskAvailable = false;
            draw->m_ReConvertToDrawableData = true;
            draw->m_AttributeChanged = true;
            draw->m_ForceGpuBufferUpload = true;
        }
        if (node->HasSubDataObject()) {
            for (auto it = node->SubDataObjectIteratorBegin(); it != node->SubDataObjectIteratorEnd(); ++it) {
                release(it->second.get());
            }
        }
    };
    release(this);
}

IGsize DrawObject::GetRealMemorySize() {
    IGsize res = this->DataObject::GetRealMemorySize();
    return res;
}

bool DrawObject::IsUseColor() { return m_UseColor; }

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

    // process this object
    m_ViewStyle = mode;
    MarkWireframeGeometryDirtyIfNeeded(m_ViewStyle);
    if (this->HasSubDataObject()) { ProcessSubDataObjects(&DrawObject::SetViewStyle, mode); }
}

void DrawObject::AddViewStyle(IGenum mode) {
    // process renderable object
    if (m_RenderableMesh.SurfaceMesh) { m_RenderableMesh.SurfaceMesh->AddViewStyle(mode); }
    if (m_RenderableMesh.SimplifiedMesh) { m_RenderableMesh.SimplifiedMesh->AddViewStyle(mode); }

    // process this object
    m_ViewStyle |= mode;
    MarkWireframeGeometryDirtyIfNeeded(m_ViewStyle);
    if (this->HasSubDataObject()) { ProcessSubDataObjects(&DrawObject::AddViewStyle, mode); }
}

void DrawObject::RemoveViewStyle(IGenum mode) {
    // process renderable object
    if (m_RenderableMesh.SurfaceMesh) { m_RenderableMesh.SurfaceMesh->RemoveViewStyle(mode); }
    if (m_RenderableMesh.SimplifiedMesh) { m_RenderableMesh.SimplifiedMesh->RemoveViewStyle(mode); }

    // process this object
    m_ViewStyle &= ~mode;
    MarkWireframeGeometryDirtyIfNeeded(m_ViewStyle);
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
    MarkWireframeGeometryDirtyIfNeeded(m_ViewStyle);

    if (this->HasSubDataObject()) { ProcessSubDataObjects(&DrawObject::SetTransparency, transparency); }
}

float DrawObject::GetTransparency() { return m_Transparency; }

void DrawObject::ViewCloudPicture(Scene* scene, int index, int dimension) {
    if (index == m_AttributeIndex && dimension == m_AttributeDimension) {
        return; // no change
    }

    // Share parent's dataRange to RenderableMesh BEFORE processing
    // This ensures RenderableMesh uses the full model's calculated range
    //
    if (index >= 0 && this->GetAttributeSet() &&
        static_cast<int>(this->GetAttributeSet()->GetNumberOfAttributes()) > index) {
        auto& parentAttr = this->GetAttributeSet()->GetAttribute(index);
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

    // process renderable object
    if (m_RenderableMesh.SurfaceMesh) { m_RenderableMesh.SurfaceMesh->ViewCloudPicture(scene, index, dimension); }
    if (m_RenderableMesh.SimplifiedMesh) { m_RenderableMesh.SimplifiedMesh->ViewCloudPicture(scene, index, dimension); }

    // process this object
    if (this->HasSubDataObject()) { ProcessSubDataObjects(&DrawObject::ViewCloudPicture, scene, index, dimension); }

    // 记录当前激活的属性索引和维度到 m_AttributeIndex / m_AttributeDimension
    if (index == -1) {
        m_AttributeIndex = -1;
        m_AttributeDimension = -1;
        m_UseColor = false;
    } else if (GetAttributeSet()->GetNumberOfAttributes() > index) {
        m_AttributeIndex = index;
        m_AttributeDimension = dimension;
        m_UseColor = true;

        auto& curAttr = GetAttributeSet()->GetAttribute(index);
        if (curAttr.pointer) { m_ColorWithCell = (curAttr.attachmentType == IG_CELL); }
    }

    m_AttributeChanged = true;

    scene->Update();
}

void DrawObject::ViewCloudPictureOfModel(Scene* scene, int index, int dimension) {
    // process renderable object
    if (m_RenderableMesh.SurfaceMesh) {
        m_RenderableMesh.SurfaceMesh->ViewCloudPictureOfModel(scene, index, dimension);
    }
    if (m_RenderableMesh.SimplifiedMesh) {
        m_RenderableMesh.SimplifiedMesh->ViewCloudPictureOfModel(scene, index, dimension);
    }

    // process this object
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

    m_RenderableMesh.SimplifiedMesh = nullptr;
#ifndef __EMSCRIPTEN__
    constexpr IGsize maxAutomaticLodFaces = 20000000;
    auto surfaceMesh = DynamicCast<SurfaceMesh>(dataObject);
    if (m_AutoBuildInteractionLod && m_ShellRendering && surfaceMesh != nullptr &&
        surfaceMesh->GetNumberOfFaces() > 1000000) {
        if (surfaceMesh->GetNumberOfFaces() <= maxAutomaticLodFaces) {
            BuildSimplifiedRenderableObject();
        } else {
            IGAME_RENDERING_INFO("{}: automatic interaction LOD skipped for {} faces (limit {}); interaction retains the full-resolution mesh.",
                                surfaceMesh->GetName(), surfaceMesh->GetNumberOfFaces(), maxAutomaticLodFaces);
        }
    }
#endif

    // 设置Meshleter
    m_RenderableMesh.mMeshleter = SurfaceMeshMeshleter::New();
    m_RenderableMesh.mMeshleter->SetInput(dataObject);
    m_RenderableMesh.mMeshleter->SetRenderWithMeshlet(m_RestoreMeshletColoring);
}

DrawObject::Pointer DrawObject::GetRenderableObject(bool useSimplified) {
    if (!m_ShellRendering) { return this; }

    if (useSimplified && m_RenderableMesh.SimplifiedMesh != nullptr &&
        m_RenderableMesh.SimplifiedMesh->m_Positions->GetNumberOfElements() >
                0 &&
        m_RenderableMesh.SimplifiedMesh->m_TriangleIndices
                        ->GetNumberOfElements() >
                0) {
        return m_RenderableMesh.SimplifiedMesh;
    }
    if (m_RenderableMesh.SurfaceMesh != nullptr) { return m_RenderableMesh.SurfaceMesh; }
    return this;
}

void DrawObject::SetAutoBuildInteractionLod(bool enabled) { m_AutoBuildInteractionLod = enabled; }

bool DrawObject::GetAutoBuildInteractionLod() const { return m_AutoBuildInteractionLod; }

void DrawObject::BuildSimplifiedRenderableObject() {
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

    renderableObject->m_RemoteRenderingEnabled = m_RemoteRenderingEnabled;
    renderableObject->m_ViewStyle = this->m_ViewStyle;
    renderableObject->m_ForceExplicitWireframeGeometry = this->m_AccelerationOption;
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
    renderableObject->m_LineColor = this->m_LineColor;
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

void DrawObject::SetOpacityMappingEnabled(bool enabled) {
    if (m_RemoteRenderingEnabled) {
        if (m_RenderableMesh.SurfaceMesh) { m_RenderableMesh.SurfaceMesh->SetOpacityMappingEnabled(enabled); }
        if (m_RenderableMesh.SimplifiedMesh) { m_RenderableMesh.SimplifiedMesh->SetOpacityMappingEnabled(enabled); }
    }

    auto mapper = this->GetColorMapper();
    int attrIdx = this->GetAttributeIndex();
    auto attrSet = this->GetAttributeSet();
    if (mapper && attrIdx >= 0 && attrSet &&
        attrIdx < attrSet->GetNumberOfAttributes()) {
        auto& attr = attrSet->GetAttribute(attrIdx);
        if (attr.pointer) { mapper->SetOpacityMappingEnabled(enabled); }
    }
    MarkWireframeGeometryDirtyIfNeeded(m_ViewStyle);
}

void DrawObject::SetAccelerationOption(bool enabled) {
#ifdef IGAME_OPENGL_VERSION_330
    IGAME_RENDERING_WARN("Acceleration rendering disabled (OpenGL 3.3 detected). Requires "
                         "OpenGL 4.3+ for hardware acceleration support.");
    return;
#endif

    if (m_RenderableMesh.SurfaceMesh) {
        m_RenderableMesh.SurfaceMesh->m_ForceExplicitWireframeGeometry = enabled;
        m_RenderableMesh.SurfaceMesh->MarkWireframeGeometryDirtyIfNeeded(m_RenderableMesh.SurfaceMesh->m_ViewStyle);
    }
    if (m_RenderableMesh.SimplifiedMesh) {
        m_RenderableMesh.SimplifiedMesh->m_ForceExplicitWireframeGeometry = enabled;
        m_RenderableMesh.SimplifiedMesh->MarkWireframeGeometryDirtyIfNeeded(
                m_RenderableMesh.SimplifiedMesh->m_ViewStyle);
    }

    m_AccelerationOption = enabled;
    MarkWireframeGeometryDirtyIfNeeded(m_ViewStyle);
    if (this->HasSubDataObject()) { ProcessSubDataObjects(&DrawObject::SetAccelerationOption, enabled); }
}

bool DrawObject::GetAccelerationOption() const { return m_AccelerationOption; }

void DrawObject::SetRenderWithMeshlet(bool val) {
    m_RestoreMeshletColoring = val;
    if (m_RenderableMesh.mMeshleter) { m_RenderableMesh.mMeshleter->SetRenderWithMeshlet(val); }
    if (val) { m_ColorWithCell = true; }
}

bool DrawObject::GetRenderWithMeshlet() const {
    return m_RenderableMesh.mMeshleter ? m_RenderableMesh.mMeshleter->GetRenderWithMeshlet() : m_RestoreMeshletColoring;
}

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
        m_PointEBO->Create();
        m_PointEBO->Target(GL_ELEMENT_ARRAY_BUFFER);
        m_LineEBO->Create();
        m_LineEBO->Target(GL_ELEMENT_ARRAY_BUFFER);
        m_TriangleEBO->Create();
        m_TriangleEBO->Target(GL_ELEMENT_ARRAY_BUFFER);

        m_CellVAO->Create();
        m_CellPositionVBO->Create();
        m_CellPositionVBO->Target(GL_ARRAY_BUFFER);
        m_CellColorVBO->Create();
        m_CellColorVBO->Target(GL_ARRAY_BUFFER);

#ifndef __EMSCRIPTEN__
        m_EdgeMaskBuffer->Create();
        m_EdgeMaskBuffer->Target(GL_TEXTURE_BUFFER);
        // Allocate a minimal buffer with a single byte of data.
        // In OpenGL 3.3, binding a GL_TEXTURE_BUFFER requires the buffer to have a non-zero size.
        // If the buffer size is zero, glTexBuffer will trigger an INVALID_OPERATION error.
        // Allocating sizeof(unsigned char) (1 byte) ensures the buffer meets the size requirement,
        // even if the actual data is not yet provided.
        m_EdgeMaskBuffer->Allocate(sizeof(unsigned char), nullptr, GL_STATIC_DRAW);

        m_EdgeMaskTexture->Create();
        m_EdgeMaskTexture->Buffer(GL_R8, m_EdgeMaskBuffer);

        m_CellEdgeMaskBuffer->Create();
        m_CellEdgeMaskBuffer->Target(GL_TEXTURE_BUFFER);
        m_CellEdgeMaskBuffer->Allocate(sizeof(unsigned char), nullptr, GL_STATIC_DRAW);

        m_CellEdgeMaskTexture->Create();
        m_CellEdgeMaskTexture->Buffer(GL_R8, m_CellEdgeMaskBuffer);
#endif

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

    GLCheckError();
}

void DrawObject::SyncGpuBuffers() {
#ifdef __EMSCRIPTEN__
    const auto syncStart = RenderTimingClock::now();
#endif
    // 多子块文件只需要处理子块
    if (this->HasSubDataObject()) {
        if (m_RemoteRenderingEnabled) {
            // New time-frame/child objects inherit only their own remote parent's policy.
            SetRemoteRenderingEnabled(true);
        }
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

    // 处理其抽壳后的表面网格
    if (m_RenderableMesh.SurfaceMesh) { m_RenderableMesh.SurfaceMesh->SyncGpuBuffers(); }
    if (m_RenderableMesh.SimplifiedMesh) { m_RenderableMesh.SimplifiedMesh->SyncGpuBuffers(); }

    // 当是表面网格时，还需要构建meshlet
    if (m_AccelerationOption) { m_RenderableMesh.mMeshleter->SyncGpuBuffers(); }

    this->CreateDrawBuffer();
    const bool forceUpload = m_RemoteRenderingEnabled && m_ForceGpuBufferUpload;
    if (forceUpload || m_Positions->GetMTime() > m_PositionVBO->GetMTime()) {
        GLAllocateGLBuffer(m_PositionVBO, m_Positions->GetNumberOfValues() * sizeof(float), m_Positions->RawPointer());
        m_PositionVBO->Modified();
        SetPositionBufferToVAO(m_PointVAO, m_PositionVBO);
        SetPositionBufferToVAO(m_LineVAO, m_PositionVBO);
        SetPositionBufferToVAO(m_TriangleVAO, m_PositionVBO);
    }

    if (forceUpload || m_Colors->GetMTime() > m_ColorVBO->GetMTime()) {
        GLAllocateGLBuffer(m_ColorVBO, m_Colors->GetNumberOfValues() * sizeof(float), m_Colors->RawPointer());
        m_ColorVBO->Modified();
        SetColorBufferToVAO(m_PointVAO, m_ColorVBO);
        SetColorBufferToVAO(m_LineVAO, m_ColorVBO);
        SetColorBufferToVAO(m_TriangleVAO, m_ColorVBO);
    }

    if (forceUpload || m_Normals->GetMTime() > m_NormalVBO->GetMTime()) {
        GLAllocateGLBuffer(m_NormalVBO, m_Normals->GetNumberOfValues() * sizeof(float), m_Normals->RawPointer());
        m_NormalVBO->Modified();

        if (!m_RemoteRenderingEnabled || m_Normals->GetNumberOfValues() != 0) {
            SetNormalBufferToVAO(m_PointVAO, m_NormalVBO);
            SetNormalBufferToVAO(m_LineVAO, m_NormalVBO);
            SetNormalBufferToVAO(m_TriangleVAO, m_NormalVBO);
        }
    }

    if (forceUpload || m_Textures->GetMTime() > m_TextureVBO->GetMTime()) {
        GLAllocateGLBuffer(m_TextureVBO, m_Textures->GetNumberOfValues() * sizeof(float), m_Textures->RawPointer());
        m_TextureVBO->Modified();

        if (!m_RemoteRenderingEnabled || m_Textures->GetNumberOfValues() != 0) {
            SetTextureBufferToVAO(m_PointVAO, m_TextureVBO);
            SetTextureBufferToVAO(m_LineVAO, m_TextureVBO);
            SetTextureBufferToVAO(m_TriangleVAO, m_TextureVBO);
        }
    }

    if (forceUpload || m_PointIndices->GetMTime() > m_PointEBO->GetMTime()) {
        GLAllocateGLBuffer(m_PointEBO, m_PointIndices->GetNumberOfValues() * sizeof(igIndex),
                           m_PointIndices->RawPointer());
        m_PointEBO->Modified();

        m_PointVAO->ElementBuffer(m_PointEBO);
    }

    if (forceUpload || m_LineIndices->GetMTime() > m_LineEBO->GetMTime()) {
        GLAllocateGLBuffer(m_LineEBO, m_LineIndices->GetNumberOfValues() * sizeof(igIndex),
                           m_LineIndices->RawPointer());
        m_LineEBO->Modified();

        m_LineVAO->ElementBuffer(m_LineEBO);
    }

    if (forceUpload || m_TriangleIndices->GetMTime() > m_TriangleEBO->GetMTime()) {
        GLAllocateGLBuffer(m_TriangleEBO, m_TriangleIndices->GetNumberOfValues() * sizeof(igIndex),
                           m_TriangleIndices->RawPointer());
        m_TriangleEBO->Modified();

        m_TriangleVAO->ElementBuffer(m_TriangleEBO);
    }

#ifndef __EMSCRIPTEN__
    auto syncEdgeMasks = [this](UnsignedCharArray::Pointer masks, GLBuffer::Pointer buffer,
                                GLTextureBuffer::Pointer texture, int& constantMask, bool& available) {
        if (!m_RemoteRenderingEnabled) {
            // Preserve main's texture-buffer upload exactly for ordinary models.
            if (masks->GetMTime() > buffer->GetMTime()) {
                GLAllocateGLBuffer(buffer, masks->GetNumberOfValues() * sizeof(unsigned char), masks->RawPointer());
                buffer->Modified();
                texture->Buffer(GL_R8, buffer);
            }
            return;
        }
        if (!m_ForceGpuBufferUpload && masks->GetMTime() <= buffer->GetMTime()) { return; }
        const IGsize count = masks->GetNumberOfValues();
        constantMask = -1;
        available = false;
        if (count) {
            const auto* data = masks->RawPointer();
            if (std::all_of(data + 1, data + count, [data](unsigned char value) { return value == data[0]; })) {
                constantMask = data[0];
                available = true;
                GLAllocateGLBuffer(buffer, 0, nullptr);
                if (count > 1000000) {
                    IGAME_RENDERING_INFO("{} uses constant edge mask {} for {} triangles; no edge-mask texture allocation.",
                                         GetName(), constantMask, count);
                }
            } else {
                GLint maxTexels = 0;
                glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &maxTexels);
                if (maxTexels > 0 && count <= static_cast<IGsize>(maxTexels)) {
                    GLAllocateGLBuffer(buffer, count * sizeof(unsigned char), data);
                    texture->Buffer(GL_R8, buffer);
                    available = true;
                } else {
                    GLAllocateGLBuffer(buffer, 0, nullptr);
                    IGAME_RENDERING_WARN("{} has {} nonconstant edge masks, exceeding GL_MAX_TEXTURE_BUFFER_SIZE={}; single-pass wireframe is unavailable for this mesh.",
                                         GetName(), count, maxTexels);
                }
            }
        }
        buffer->Modified();
    };
    syncEdgeMasks(m_TriangleEdgeMasks, m_EdgeMaskBuffer, m_EdgeMaskTexture,
                  m_ConstantEdgeMask, m_EdgeMaskAvailable);
#endif

    if (forceUpload || m_CellPositions->GetMTime() > m_CellPositionVBO->GetMTime()) {
        GLAllocateGLBuffer(m_CellPositionVBO, m_CellPositions->GetNumberOfValues() * sizeof(float),
                           m_CellPositions->RawPointer());
        m_CellPositionVBO->Modified();

        SetPositionBufferToVAO(m_CellVAO, m_CellPositionVBO);
    }

    if (forceUpload || m_CellColors->GetMTime() > m_CellColorVBO->GetMTime()) {
        GLAllocateGLBuffer(m_CellColorVBO, m_CellColors->GetNumberOfValues() * sizeof(float),
                           m_CellColors->RawPointer());
        m_CellColorVBO->Modified();

        SetColorBufferToVAO(m_CellVAO, m_CellColorVBO);
    }

#ifndef __EMSCRIPTEN__
    syncEdgeMasks(m_CellTriangleEdgeMasks, m_CellEdgeMaskBuffer, m_CellEdgeMaskTexture,
                  m_ConstantCellEdgeMask, m_CellEdgeMaskAvailable);
#endif

    m_ForceGpuBufferUpload = false;

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
    VAO->VertexBuffer(GL_VBO_IDX_1, VBO, 0, 4 * sizeof(float));
    GLSetVertexAttrib(VAO, GL_LOCATION_IDX_1, GL_VBO_IDX_1, 4, GL_FLOAT, GL_FALSE, 0);
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
    m_ReConvertToDrawableData = true;
    if (this->HasSubDataObject()) { ProcessSubDataObjects(&DrawObject::ForceReConvertToDrawableData); }
}

IGAME_NAMESPACE_END
