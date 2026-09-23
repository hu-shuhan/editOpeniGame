#include "iGameDrawObject.h"
#include "iGamePointSet.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include <algorithm>
#include <functional>
#include <limits>
#include <unordered_set>

IGAME_NAMESPACE_BEGIN

void DrawObject::ReleaseGpuResourcesKeepCpuData() {
    if (!m_RemoteRenderingEnabled) return;
    std::unordered_set<DataObject*> visited;
    auto destroy = [](const auto& resource) { if (resource) resource->Destroy(); };
    std::function<void(DataObject*)> release = [&](DataObject* node) {
        if (!node || !visited.insert(node).second) return;
        if (auto* draw = dynamic_cast<DrawObject*>(node)) {
            release(draw->m_RenderableMesh.SurfaceMesh.get());
            release(draw->m_RenderableMesh.SimplifiedMesh.get());
            if (draw->m_RenderableMesh.mMeshleter) {
                draw->m_RenderableMesh.mMeshleter->ReleaseGpuBuffers();
            }
            destroy(draw->m_PointVAO); destroy(draw->m_LineVAO); destroy(draw->m_TriangleVAO);
            destroy(draw->m_CellVAO); destroy(draw->m_EdgeMaskTexture); destroy(draw->m_CellEdgeMaskTexture);
            destroy(draw->m_PositionVBO); destroy(draw->m_ColorVBO); destroy(draw->m_NormalVBO);
            destroy(draw->m_TextureVBO); destroy(draw->m_PointEBO); destroy(draw->m_LineEBO);
            destroy(draw->m_TriangleEBO); destroy(draw->m_CellPositionVBO); destroy(draw->m_CellColorVBO);
            destroy(draw->m_EdgeMaskBuffer); destroy(draw->m_CellEdgeMaskBuffer);
            draw->m_Flag = false;
            draw->m_ForceGpuBufferUpload = true;
            // Do not dirty geometry/scalars or replace CPU arrays/helpers.
            // The C/S caller uses UploadPreparedCpuData before reattachment;
            // ordinary forced SyncGpuBuffers also enables empty attributes.
        }
        if (node->HasSubDataObject()) {
            for (auto it = node->SubDataObjectIteratorBegin(); it != node->SubDataObjectIteratorEnd(); ++it)
                release(it->second.get());
        }
    };
    release(this);
}

bool DrawObject::UploadPreparedCpuData() {
    if (!m_RemoteRenderingEnabled || HasGpuResources() || !InspectCpuDisplayCache().ready) return false;
    std::unordered_set<DataObject*> visited;
    std::function<void(DataObject*)> upload = [&](DataObject* node) {
        if (!node || !visited.insert(node).second) return;
        if (node->HasSubDataObject()) {
            for (auto it = node->SubDataObjectIteratorBegin(); it != node->SubDataObjectIteratorEnd(); ++it)
                upload(it->second.get());
            return;
        }
        auto* draw = dynamic_cast<DrawObject*>(node);
        if (!draw) return;
        upload(draw->m_RenderableMesh.SurfaceMesh.get());
        upload(draw->m_RenderableMesh.SimplifiedMesh.get());
        draw->CreateDrawBuffer();
        auto buffer = [&](const auto& data, const GLBuffer::Pointer& gpu) {
            if (!data->GetNumberOfValues()) {
                // Mark even an intentionally absent attribute synchronized so
                // the next ordinary SyncGpuBuffers does not enable it again.
                gpu->Modified();
                return false;
            }
            GLAllocateGLBuffer(gpu, data->GetNumberOfValues() * sizeof(*data->RawPointer()), data->RawPointer());
            gpu->Modified();
            return true;
        };
        if (buffer(draw->m_Positions, draw->m_PositionVBO)) {
            draw->SetPositionBufferToVAO(draw->m_PointVAO, draw->m_PositionVBO);
            draw->SetPositionBufferToVAO(draw->m_LineVAO, draw->m_PositionVBO);
            draw->SetPositionBufferToVAO(draw->m_TriangleVAO, draw->m_PositionVBO);
        }
        if (buffer(draw->m_Colors, draw->m_ColorVBO)) {
            draw->SetColorBufferToVAO(draw->m_PointVAO, draw->m_ColorVBO);
            draw->SetColorBufferToVAO(draw->m_LineVAO, draw->m_ColorVBO);
            draw->SetColorBufferToVAO(draw->m_TriangleVAO, draw->m_ColorVBO);
        }
        // A freshly-created VAO has these optional attributes disabled. An
        // empty normal/UV array must not become an enabled zero-byte VBO.
        if (buffer(draw->m_Normals, draw->m_NormalVBO)) {
            draw->SetNormalBufferToVAO(draw->m_PointVAO, draw->m_NormalVBO);
            draw->SetNormalBufferToVAO(draw->m_LineVAO, draw->m_NormalVBO);
            draw->SetNormalBufferToVAO(draw->m_TriangleVAO, draw->m_NormalVBO);
        }
        if (buffer(draw->m_Textures, draw->m_TextureVBO)) {
            draw->SetTextureBufferToVAO(draw->m_PointVAO, draw->m_TextureVBO);
            draw->SetTextureBufferToVAO(draw->m_LineVAO, draw->m_TextureVBO);
            draw->SetTextureBufferToVAO(draw->m_TriangleVAO, draw->m_TextureVBO);
        }
        if (buffer(draw->m_PointIndices, draw->m_PointEBO)) draw->m_PointVAO->ElementBuffer(draw->m_PointEBO);
        if (buffer(draw->m_LineIndices, draw->m_LineEBO)) draw->m_LineVAO->ElementBuffer(draw->m_LineEBO);
        if (buffer(draw->m_TriangleIndices, draw->m_TriangleEBO)) draw->m_TriangleVAO->ElementBuffer(draw->m_TriangleEBO);
        if (buffer(draw->m_CellPositions, draw->m_CellPositionVBO)) draw->SetPositionBufferToVAO(draw->m_CellVAO, draw->m_CellPositionVBO);
        if (buffer(draw->m_CellColors, draw->m_CellColorVBO)) draw->SetColorBufferToVAO(draw->m_CellVAO, draw->m_CellColorVBO);
#ifndef __EMSCRIPTEN__
        auto edgeMask = [&](const UnsignedCharArray::Pointer& masks, const GLBuffer::Pointer& gpu,
                            const GLTextureBuffer::Pointer& texture, int& constantMask, bool& available) {
            const IGsize count = masks->GetNumberOfValues();
            constantMask = -1;
            available = false;
            if (!count) {
                gpu->Modified();
                return;
            }
            const auto* data = masks->RawPointer();
            if (std::all_of(data + 1, data + count, [data](unsigned char value) { return value == data[0]; })) {
                constantMask = data[0];
                available = true;
                GLAllocateGLBuffer(gpu, 0, nullptr);
                gpu->Modified();
                return;
            }
            GLint maxTexels = 0;
            glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &maxTexels);
            if (maxTexels > 0 && count <= static_cast<IGsize>(maxTexels)) {
                GLAllocateGLBuffer(gpu, count * sizeof(unsigned char), data);
                gpu->Modified();
                texture->Buffer(GL_R8, gpu);
                available = true;
            } else {
                GLAllocateGLBuffer(gpu, 0, nullptr);
                gpu->Modified();
            }
        };
        edgeMask(draw->m_TriangleEdgeMasks, draw->m_EdgeMaskBuffer, draw->m_EdgeMaskTexture,
                 draw->m_ConstantEdgeMask, draw->m_EdgeMaskAvailable);
        edgeMask(draw->m_CellTriangleEdgeMasks, draw->m_CellEdgeMaskBuffer, draw->m_CellEdgeMaskTexture,
                 draw->m_ConstantCellEdgeMask, draw->m_CellEdgeMaskAvailable);
#endif
        draw->m_ForceGpuBufferUpload = false;
    };
    upload(this);
    return glGetError() == GL_NO_ERROR;
}

DrawObject::CpuDisplayCacheState DrawObject::InspectCpuDisplayCache() {
    CpuDisplayCacheState result;
    std::unordered_set<DataObject*> visited;
    std::unordered_set<const Object*> countedArrays;
    auto addBytes = [&](std::uint64_t bytes) {
        const auto maximum = std::numeric_limits<std::uint64_t>::max();
        result.estimatedBytes = bytes > maximum - result.estimatedBytes
            ? maximum : result.estimatedBytes + bytes;
    };
    auto identity = [&](const void* value) {
        result.signature.push_back(static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(value)));
    };
    auto stamp = [&](Object* value) {
        identity(value);
        result.signature.push_back(value ? value->GetMTime().GetMTime() : 0);
    };
    auto array = [&](ArrayObject* value) {
        stamp(value);
        result.signature.push_back(value ? value->GetNumberOfValues() : 0);
        result.signature.push_back(value ? value->GetDimension() : 0);
    };
    auto drawArray = [&](const auto& value) {
        array(value.get());
        if (value && countedArrays.insert(value.get()).second) addBytes(value->GetRealMemorySize());
    };
    // Legacy estimates recursively include ordinary children, but not derived
    // render meshes. Count each extra derived root once. Shared input geometry
    // in those estimates is deliberately not subtracted (conservative budget).
    addBytes(GetRealMemorySize());
    std::unordered_set<DataObject*> ordinary;
    std::function<void(DataObject*)> mark = [&](DataObject* node) {
        if (!node || !ordinary.insert(node).second) return;
        if (node->HasSubDataObject()) {
            for (auto it = node->SubDataObjectIteratorBegin(); it != node->SubDataObjectIteratorEnd(); ++it)
                mark(it->second.get());
        }
    };
    mark(this);
    std::function<void(DataObject*)> inspect = [&](DataObject* node) {
        identity(node);
        if (!node || !visited.insert(node).second) return;
        if (!ordinary.count(node)) addBytes(node->GetRealMemorySize());
        auto points = node->GetPoints();
        stamp(points.get());
        if (points) array(points->ConvertToArray().get());
        auto cells = node->GetCellArray();
        stamp(cells.get());
        if (cells) {
            auto ids = cells->GetCellIdArray();
            stamp(ids.get());
            result.signature.push_back(cells->GetNumberOfCells());
            if (ids) {
                identity(ids->RawPointer());
                result.signature.push_back(ids->GetNumberOfIds());
                // Legacy IdArray estimates size, not retained vector capacity.
                if (countedArrays.insert(ids.get()).second)
                    addBytes(ids->GetAllocatedMemorySize() - ids->GetRealMemorySize());
            }
            array(cells->GetOffset().get());
        }
        if (auto* pointSet = dynamic_cast<PointSet*>(node)) drawArray(pointSet->GetPointMap());
        auto* attributes = node->GetAttributeSet();
        stamp(attributes);
        if (attributes) {
            result.signature.push_back(attributes->GetNumberOfAttributes());
            for (std::size_t i = 0; i < attributes->GetNumberOfAttributes(); ++i) {
                auto& attribute = attributes->GetAttribute(i);
                array(attribute.pointer.get());
                result.signature.push_back(attribute.isDeleted);
                result.signature.push_back(attribute.attachmentType);
                result.signature.push_back(attribute.type);
            }
        }
        if (auto* draw = dynamic_cast<DrawObject*>(node)) {
            auto notReady = [&](const char* reason) {
                result.ready = false;
                if (result.notReadyReason.empty()) result.notReadyReason =
                    node->GetName() + ":" + reason;
            };
            stamp(draw->m_Clipper.get());
            stamp(draw->m_AttributeHelper.get());
            stamp(draw->m_ColorMapper.get());
            result.signature.insert(result.signature.end(), {
                static_cast<std::uint64_t>(draw->m_AttributeIndex),
                static_cast<std::uint64_t>(draw->m_AttributeDimension),
                draw->m_ShellRendering, draw->m_AccelerationOption, draw->m_UseColor,
                draw->m_ColorWithCell, draw->m_UseNormalSmooth, draw->m_AutoUpdateDrawData,
                draw->m_ReConvertToDrawableData, draw->m_AttributeChanged, draw->m_RemoteRenderingEnabled});
            drawArray(draw->m_Positions); drawArray(draw->m_Colors);
            drawArray(draw->m_Normals); drawArray(draw->m_Textures);
            drawArray(draw->m_PointIndices); drawArray(draw->m_LineIndices);
            drawArray(draw->m_TriangleIndices); drawArray(draw->m_TriangleEdgeMasks);
            drawArray(draw->m_CellPositions); drawArray(draw->m_CellColors);
            drawArray(draw->m_CellTriangleEdgeMasks);
            if (draw->m_RenderableMesh.mMeshleter)
                addBytes(draw->m_RenderableMesh.mMeshleter->GetRetainedCpuMemorySize());
            // Meshlet Build currently mixes CPU work with GPU upload. It is
            // intentionally outside this new upload-only cache contract.
            if (draw->m_AccelerationOption) notReady("meshlet-path-not-supported");
            if (!node->HasSubDataObject()) {
                const bool shell = draw->m_ShellRendering && draw->m_RenderableMesh.SurfaceMesh;
                if (draw->m_ReConvertToDrawableData) notReady("geometry-dirty");
                if (!shell && draw->m_AttributeChanged) notReady("scalar-dirty");
                if (points && points->GetMTime() > draw->m_ReConvertHelper->GetMTime()) notReady("points-dirty");
                if (draw->m_Clipper->GetMTime() > draw->m_ReConvertHelper->GetMTime()) notReady("clipper-dirty");
                if (draw->m_AttributeHelper->GetMTime() > draw->m_ReConvertHelper->GetMTime()) notReady("selection-dirty");
                if (!shell && draw->m_ColorMapper->GetMTime() > draw->m_ReConvertHelper->GetMTime()) notReady("color-mapper-dirty");
                if (cells && cells->GetNumberOfCells() && !draw->m_RenderableMesh.SurfaceMesh &&
                    (draw->m_Positions->GetNumberOfValues() == 0 ||
                     draw->m_TriangleIndices->GetNumberOfValues() == 0)) notReady("drawing-arrays-missing");
            }
            inspect(draw->m_RenderableMesh.SurfaceMesh.get());
            inspect(draw->m_RenderableMesh.SimplifiedMesh.get());
        }
        if (node->HasSubDataObject()) {
            for (auto it = node->SubDataObjectIteratorBegin(); it != node->SubDataObjectIteratorEnd(); ++it)
                inspect(it->second.get());
        }
    };
    inspect(this);
    return result;
}

IGAME_NAMESPACE_END
