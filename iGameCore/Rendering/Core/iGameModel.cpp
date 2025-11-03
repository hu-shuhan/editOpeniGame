#include "iGameModel.h"
#include "iGameFilter.h"
#include "iGameInteractor.h"
#include "iGamePointSet.h"
#include "iGameRenderingLogger.h"
#include "iGameScene.h"
#include <functional>

IGAME_NAMESPACE_BEGIN

Model::Model() {
    SwitchOff(ViewSwitch::BoundingBox);
    SwitchOn(ViewSwitch::PickedItem);

    m_DataObject = DataObject::New();
    m_Filter = Filter::New();

    m_FilePath = "";
    m_Scene = nullptr;

    m_BboxHandle = 0;
    m_Switch = 0ull;
}

Model::~Model() {}

void Model::SetScene(SmartPointer<Scene> scene) { m_Scene = scene; }

SmartPointer<Scene> Model::GetScene() const { return m_Scene; }

SmartPointer<DataObject> Model::GetDataObject() { return m_DataObject; }

void Model::SetVisibility(bool visibility) { m_Visibility = visibility; }

bool Model::GetVisibility() const { return m_Visibility; }

SmartPointer<Filter> Model::GetModelFilter() { return m_Filter; }

SmartPointer<Painter3D> Model::GetPainter3D(Painter3D::Usage usage) {
    if (m_Painter3Ds.count(usage) == 0) {
        m_Painter3Ds[usage] = Painter3D::New();
        m_Painter3Ds[usage]->SetScene(m_Scene);
    }
    return m_Painter3Ds[usage];
}

const std::map<Painter3D::Usage, SmartPointer<Painter3D>>&
Model::GetAllPainter3Ds() {
    return m_Painter3Ds;
}

void Model::SetModelFilter(SmartPointer<Filter> filter) { m_Filter = filter; }

void Model::DeleteModelFilter() { m_Filter = nullptr; }

void Model::SetDataObject(SmartPointer<DataObject> dataObject) {
    m_DataObject = dataObject;
}

void Model::Modified() {
    IGAME_RENDERING_ERROR(
            "[Model::Modified] not sure what this function does.");
    m_DataObject->Modified();
}

void Model::Update() {
    if (m_Scene) { m_Scene->Update(); }
}

void Model::ViewCloudPicture(int index, int dimension) {
    auto drawObject = DynamicCast<DrawObject>(GetDataObject());
    if (drawObject != nullptr)
        drawObject->ViewCloudPicture(m_Scene, index, dimension);
}

void Model::SetFilePath(std::string filePath) { m_FilePath = filePath; }

std::string Model::GetFilePath() { return this->m_FilePath; }

SmartPointer<Selection> Model::GetSelection() {
    return DynamicCast<PointSet>(GetDataObject())->GetSelection(this);
}

void Model::RequestPointSelection(SmartPointer<Points> p,
                                  SmartPointer<Selection> s) {
    if (m_Scene->GetInteractor() == nullptr) return;
    s->m_Points = p;
    s->m_Model = this;
    m_Scene->GetInteractor()->RequestPointSelectionStyle(s);
}

void Model::RequestDragPoint(SmartPointer<Points> p,
                             SmartPointer<Selection> s) {
    if (m_Scene->GetInteractor() == nullptr) return;
    s->m_Points = p;
    s->m_Model = this;
    m_Scene->GetInteractor()->RequestDragPointStyle(s);
}

void Model::Show() {
    auto drawObject = DynamicCast<DrawObject>(GetDataObject());
    drawObject->SetVisibility(true);
    m_Scene->ChangeModelVisibility(this, true);
}

void Model::Hide() {
    auto drawObject = DynamicCast<DrawObject>(GetDataObject());
    drawObject->SetVisibility(false);
    m_Scene->ChangeModelVisibility(this, false);
}

void Model::SetBoundingBoxSwitch(bool action) {
    auto drawObject = DynamicCast<DrawObject>(GetDataObject());
    auto painter3D = GetPainter3D(Painter3D::Usage::BoundingBox);
    if (action) {
        SwitchOn(ViewSwitch::BoundingBox);

        auto& bbox = drawObject->GetBoundingBox();
        Vector3d p1 = bbox.min;
        Vector3d p7 = bbox.max;

        if (m_BboxHandle != 0) { painter3D->Delete(m_BboxHandle); }
        painter3D->SetPen(5);
        painter3D->SetPen(Color::LightBlue);
        painter3D->SetBrush(Brush::Style::NoBrush);
        m_BboxHandle = painter3D->DrawCube(p1, p7);
    } else {
        SwitchOff(ViewSwitch::BoundingBox);
        painter3D->Hide(m_BboxHandle);
    }
}

void Model::SetPickedItemSwitch(bool action) {
    auto drawObject = DynamicCast<DrawObject>(GetDataObject());
    if (action) {
        SwitchOn(ViewSwitch::PickedItem);
        if (drawObject->GetVisibility()) {
            for (auto& painter3D_: m_Painter3Ds) {
                painter3D_.second->SetTotallyHide(false);
            }
            //m_Painter3D->ShowAll();
        }
    } else {
        SwitchOff(ViewSwitch::PickedItem);
        for (auto& painter3D_: m_Painter3Ds) {
            painter3D_.second->SetTotallyHide(true);
        }
        //m_Painter3D->HideAll();
    }
}

// Helper: apply a functor to every DrawObject in the model's DataObject tree (root included)
namespace
{
static void
ForEachDrawObject(DataObject::Pointer root,
                  const std::function<void(DrawObject::Pointer)>& fn) {
    if (!root) return;
    // apply on this node if drawable
    if (auto draw = DynamicCast<DrawObject>(root)) { fn(draw); }
    // recurse children safely (even if non-draw DataObject exists)
    if (root->HasSubDataObject()) {
        for (auto it = root->SubDataObjectIteratorBegin();
             it != root->SubDataObjectIteratorEnd(); ++it) {
            ForEachDrawObject(it->second, fn);
        }
    }
}
} // namespace

void Model::SetViewPointsSwitch(bool action) {
    auto root = GetDataObject();
    ForEachDrawObject(root, [&](DrawObject::Pointer draw) {
        if (action) {
            draw->AddViewStyle(IG_POINTS);
        } else {
            draw->RemoveViewStyle(IG_POINTS);
        }
    });
}

void Model::SetViewWireframeSwitch(bool action) {
    auto root = GetDataObject();
    ForEachDrawObject(root, [&](DrawObject::Pointer draw) {
        if (action) {
            draw->AddViewStyle(IG_WIREFRAME);
        } else {
            draw->RemoveViewStyle(IG_WIREFRAME);
        }
    });
}

void Model::SetViewFillSwitch(bool action) {
    auto root = GetDataObject();
    ForEachDrawObject(root, [&](DrawObject::Pointer draw) {
        if (action) {
            draw->AddViewStyle(IG_SURFACE);
        } else {
            draw->RemoveViewStyle(IG_SURFACE);
        }
    });
}

void Model::SwitchOn(ViewSwitch type) { m_Switch |= (1ull << type); }

void Model::SwitchOff(ViewSwitch type) { m_Switch &= ~(1ull << type); }

bool Model::GetSwitch(ViewSwitch type) { return m_Switch & (1ull << type); }

void Model::SyncGpuBuffers() {
    // convert to drawable data
    auto drawObject = DynamicCast<DrawObject>(GetDataObject());
    drawObject->SyncGpuBuffers();
}

void Model::Draw() {
    if (!this->GetVisibility()) { return; }
    bool useSimplified = m_Scene->m_IsInteracting;

    auto draw = [&](const SmartPointer<DataObject>& dataObject) {
        auto drawObject = DynamicCast<DrawObject>(dataObject);
        if (!drawObject->GetVisibility()) { return; }

        bool hasTransparency = drawObject->GetTransparency() < 1.0f;
        bool hasAcceleration = drawObject->GetAccelerationOption();
        if (hasTransparency || hasAcceleration) { return; }

        // Render
        m_Scene->UpdateObjectDataBlock(dataObject);
        m_Scene->UpdateUniformBufferObjectBlock(dataObject);

        auto renderableObject = drawObject->GetRenderableObject(useSimplified);
        auto useColor = renderableObject->IsUseColor();
        auto colorWithCell = renderableObject->m_ColorWithCell;
        auto viewStyle = renderableObject->GetViewStyle();

        if (viewStyle & IG_POINTS) {
            auto shader = m_Scene->GetShader(ShaderType::NOLIGHT);
            shader->Use();

            // 如果是样条对象，强制用红色绘制控制点
            if (m_DataObject->GetDataObjectType() == IG_SPLINE_GEOMETRY) {
                auto shader = m_Scene->GetShader(ShaderType::PURECOLOR);
                shader->Use();
                shader->SetUniform3f("inputColor", igm::vec3{1.0f, 0.0f, 0.0f});
            }

            glPointSize(renderableObject->m_PointSize);

            if (renderableObject->m_PointIndices->GetNumberOfValues() == 0) {
                renderableObject->m_PointVAO->DrawArrays(
                        GL_POINTS, 0,
                        renderableObject->m_Positions->GetNumberOfElements());
            } else {
                renderableObject->m_PointVAO->DrawRangeElements(
                        GL_POINTS, 0,
                        renderableObject->m_Positions->GetNumberOfElements() -
                                1,
                        renderableObject->m_PointIndices->GetNumberOfValues(),
                        GL_UNSIGNED_INT);
            }
        }

        // whether to use single-pass wireframe rendering
        if (viewStyle & IG_WIREFRAME && viewStyle & IG_SURFACE &&
            renderableObject->IsUseSinglePassWireframeRendering()) {
            auto shader = m_Scene->GetShader(ShaderType::SINGLEPASSWIREFRAME);
            shader->Use();

            shader->SetUniformf("lineWidth", renderableObject->GetLineWidth());

            auto edgeMaskTexture =
                    colorWithCell ? renderableObject->m_CellEdgeMaskTexture
                                  : renderableObject->m_EdgeMaskTexture;
            edgeMaskTexture->Active(GL_TEXTURE1);
            shader->SetUniformi("edgeMasks", 1);

            if (useColor && !colorWithCell) {
                shader->SetUniformi("edgeColorMode", 0);
            } else {
                shader->SetUniformi("edgeColorMode", 1);
                shader->SetUniform3f("edgeColor", igm::vec3{0.0f, 0.0f, 0.0f});
            }

            int vp[4];
            glGetIntegerv(GL_VIEWPORT, vp);
            igm::vec4 dims{(float) vp[0], (float) vp[1], (float) vp[2],
                           (float) vp[3]};
            shader->SetUniform4f("vpDims", dims);

            if (colorWithCell) {
                renderableObject->m_CellVAO->DrawArrays(
                        GL_TRIANGLES, 0, renderableObject->m_CellPositionSize);
            } else {
                renderableObject->m_TriangleVAO->DrawRangeElements(
                        GL_TRIANGLES, 0,
                        renderableObject->m_Positions->GetNumberOfElements() -
                                1,
                        renderableObject->m_TriangleIndices
                                ->GetNumberOfValues(),
                        GL_UNSIGNED_INT);
            }
        } else {
            if (viewStyle & IG_SURFACE) {
                auto shader = m_Scene->GetShader(ShaderType::BLINNPHONG);
                shader->Use();

                if (colorWithCell) {
                    renderableObject->m_CellVAO->DrawArrays(
                            GL_TRIANGLES, 0,
                            renderableObject->m_CellPositionSize);
                } else {
                    renderableObject->m_TriangleVAO->DrawRangeElements(
                            GL_TRIANGLES, 0,
                            renderableObject->m_Positions
                                            ->GetNumberOfElements() -
                                    1,
                            renderableObject->m_TriangleIndices
                                    ->GetNumberOfValues(),
                            GL_UNSIGNED_INT);
                }
            }

            if (viewStyle & IG_WIREFRAME) {
                if (useColor && !colorWithCell) {
                    m_Scene->GetShader(ShaderType::NOLIGHT)->Use();
                } else {
                    auto shader = m_Scene->GetShader(ShaderType::PURECOLOR);
                    shader->Use();
                    shader->SetUniform3f("inputColor",
                                         igm::vec3{0.0f, 0.0f, 0.0f});
                }

                // 如果是样条对象，强制用黑色绘制控制线
                if (m_DataObject->GetDataObjectType() == IG_SPLINE_GEOMETRY) {
                    auto shader = m_Scene->GetShader(ShaderType::PURECOLOR);
                    shader->Use();
                    shader->SetUniform3f("inputColor",
                                         igm::vec3{0.0f, 0.0f, 0.0f});
                }

                glLineWidth(renderableObject->m_LineWidth);

                renderableObject->m_LineVAO->DrawRangeElements(
                        GL_LINES, 0,
                        renderableObject->m_Positions->GetNumberOfElements() -
                                1,
                        renderableObject->m_LineIndices->GetNumberOfValues(),
                        GL_UNSIGNED_INT);
            }
        }
    };

    auto drawObject = DynamicCast<DrawObject>(m_DataObject);
    if (!drawObject->HasSubDataObject()) {
        draw(drawObject);
    } else {
        for (auto it = drawObject->SubDataObjectIteratorBegin();
             it != drawObject->SubDataObjectIteratorEnd(); it++) {
            auto subDataObj = it->second;
            auto subDrawObj = DynamicCast<DrawObject>(subDataObj);
            draw(subDrawObj);
        }
    }
}

void Model::DrawWithTransparency() {
    if (!this->GetVisibility()) { return; }
    bool useSimplified = m_Scene->m_IsInteracting;

    auto draw = [&](const SmartPointer<DataObject>& dataObject) {
        auto drawObject = DynamicCast<DrawObject>(dataObject);
        if (!drawObject->GetVisibility()) { return; }

        bool hasTransparency = drawObject->GetTransparency() < 1.0f;
        if (!hasTransparency) { return; }

        // Render
        m_Scene->UpdateObjectDataBlock(dataObject);
        m_Scene->UpdateUniformBufferObjectBlock(dataObject);

        auto renderableObject = drawObject->GetRenderableObject(useSimplified);
        auto useColor = renderableObject->IsUseColor();
        auto colorWithCell = renderableObject->m_ColorWithCell;
        auto viewStyle = renderableObject->GetViewStyle();

        if (viewStyle & IG_POINTS) {
            auto shader = m_Scene->GetShader(ShaderType::TRANSPARENCYLINK);
            shader->Use();
            shader->SetUniformi("colorMode", 1);

            glPointSize(renderableObject->m_PointSize);

            if (renderableObject->m_PointIndices->GetNumberOfValues() == 0) {
                renderableObject->m_PointVAO->DrawArrays(
                        GL_POINTS, 0,
                        renderableObject->m_Positions->GetNumberOfElements());
            } else {
                renderableObject->m_PointVAO->DrawRangeElements(
                        GL_POINTS, 0,
                        renderableObject->m_Positions->GetNumberOfElements() -
                                1,
                        renderableObject->m_PointIndices->GetNumberOfValues(),
                        GL_UNSIGNED_INT);
            }
        }

        if (viewStyle & IG_WIREFRAME) {
            if (useColor && !colorWithCell) {
                auto shader = m_Scene->GetShader(ShaderType::TRANSPARENCYLINK);
                shader->Use();
                shader->SetUniformi("colorMode", 1);
            } else {
                auto shader = m_Scene->GetShader(ShaderType::TRANSPARENCYLINK);
                shader->Use();
                shader->SetUniformi("colorMode", 2);
                shader->SetUniform3f("inputColor", igm::vec3{0.0f, 0.0f, 0.0f});
            }

            glLineWidth(renderableObject->m_LineWidth);

            renderableObject->m_LineVAO->DrawRangeElements(
                    GL_LINES, 0,
                    renderableObject->m_Positions->GetNumberOfElements() - 1,
                    renderableObject->m_LineIndices->GetNumberOfValues(),
                    GL_UNSIGNED_INT);
        }

        if (viewStyle & IG_SURFACE) {
            auto shader = m_Scene->GetShader(ShaderType::TRANSPARENCYLINK);
            shader->Use();
            shader->SetUniformi("colorMode", 0);

            if (colorWithCell) {
                renderableObject->m_CellVAO->DrawArrays(
                        GL_TRIANGLES, 0, renderableObject->m_CellPositionSize);
            } else {
                renderableObject->m_TriangleVAO->DrawRangeElements(
                        GL_TRIANGLES, 0,
                        renderableObject->m_Positions->GetNumberOfElements() -
                                1,
                        renderableObject->m_TriangleIndices
                                ->GetNumberOfValues(),
                        GL_UNSIGNED_INT);
            }
        }
    };

    auto drawObject = DynamicCast<DrawObject>(m_DataObject);
    if (!drawObject->HasSubDataObject()) {
        draw(drawObject);
    } else {
        for (auto it = drawObject->SubDataObjectIteratorBegin();
             it != drawObject->SubDataObjectIteratorEnd(); it++) {
            auto subDataObj = it->second;
            auto subDrawObj = DynamicCast<DrawObject>(subDataObj);
            draw(subDrawObj);
        }
    }
}

void Model::DrawWithVolume() {
    if (!this->GetVisibility()) { return; }

    auto draw = [&](const SmartPointer<DataObject>& dataObject) {
        auto drawObject = DynamicCast<DrawObject>(dataObject);
        if (!drawObject->GetVisibility()) { return; }

        // Render
        m_Scene->UpdateObjectDataBlock(dataObject);
        m_Scene->UpdateUniformBufferObjectBlock(dataObject);

        auto renderableObject = drawObject; //体绘制用原始体进行渲染
        auto colorWithCell = renderableObject->m_ColorWithCell;
        auto viewStyle = renderableObject->GetViewStyle();

        if (viewStyle & IG_POINTS) {}
        if (viewStyle & IG_WIREFRAME) {}
        if (viewStyle & IG_SURFACE) {
            auto shader = m_Scene->GetShader(ShaderType::VOLUMERENDERINGLINK);
            shader->Use();

            if (colorWithCell) {
                renderableObject->m_CellVAO->DrawArrays(
                        GL_TRIANGLES, 0, renderableObject->m_CellPositionSize);
            } else {
                renderableObject->m_TriangleVAO->DrawRangeElements(
                        GL_TRIANGLES, 0,
                        drawObject->m_Positions->GetNumberOfElements() - 1,
                        drawObject->m_TriangleIndices->GetNumberOfValues(),
                        GL_UNSIGNED_INT);
            }
        }
    };

    auto drawObject = DynamicCast<DrawObject>(m_DataObject);
    if (!drawObject->HasSubDataObject()) {
        draw(drawObject);
    } else {
        for (auto it = drawObject->SubDataObjectIteratorBegin();
             it != drawObject->SubDataObjectIteratorEnd(); it++) {
            auto subDataObj = it->second;
            auto subDrawObj = DynamicCast<DrawObject>(subDataObj);
            draw(subDrawObj);
        }
    }
}

void Model::DrawPhase1() {
#ifdef IGAME_OPENGL_VERSION_460
    if (!this->GetVisibility()) { return; }

    #ifdef GL_SUPPORTS_MESH_SHADER
    auto draw = [&](const SmartPointer<DataObject>& dataObject) {
        auto drawObject = DynamicCast<DrawObject>(dataObject);
        if (!drawObject->GetVisibility()) { return; }

        bool hasTransparency = drawObject->GetTransparency() < 1.0f;
        bool hasAcceleration = drawObject->GetAccelerationOption();
        if (hasTransparency || !hasAcceleration) { return; }

        // Render
        m_Scene->UpdateObjectDataBlock(dataObject);
        m_Scene->UpdateUniformBufferObjectBlock(dataObject);

        auto surfaceObject = drawObject->m_RenderableMesh.SurfaceMesh;
        auto meshleter = drawObject->m_RenderableMesh.Meshleter;

        auto useColor = drawObject->IsUseColor();
        auto colorWithCell = drawObject->m_ColorWithCell;
        auto viewStyle = drawObject->GetViewStyle();

        // draw
        if (useColor && colorWithCell) {}

        if (viewStyle & IG_POINTS) {
            auto shader = m_Scene->GetShader(ShaderType::NOLIGHT);
            shader->Use();

            // 如果是样条对象，强制用红色绘制控制点
            if (m_DataObject->GetDataObjectType() == IG_SPLINE_GEOMETRY) {
                auto shader = m_Scene->GetShader(ShaderType::PURECOLOR);
                shader->Use();
                shader->SetUniform3f("inputColor", igm::vec3{1.0f, 0.0f, 0.0f});
            }

            glPointSize(surfaceObject->m_PointSize);

            float u;
            surfaceObject->GetPointOffsetParameters(u);

            if (surfaceObject->m_PointIndices->GetNumberOfValues() == 0) {
                surfaceObject->m_PointVAO->DrawArrays(
                        GL_POINTS, 0,
                        surfaceObject->m_Positions->GetNumberOfElements());
            } else {
                surfaceObject->m_PointVAO->DrawRangeElements(
                        GL_POINTS, 0,
                        surfaceObject->m_Positions->GetNumberOfElements() - 1,
                        surfaceObject->m_PointIndices->GetNumberOfValues(),
                        GL_UNSIGNED_INT);
            }
        }

        if (viewStyle & IG_WIREFRAME) {
            if (useColor) {
                m_Scene->GetShader(ShaderType::NOLIGHT)->Use();
            } else {
                auto shader = m_Scene->GetShader(ShaderType::PURECOLOR);
                shader->Use();
                shader->SetUniform3f("inputColor", igm::vec3{0.0f, 0.0f, 0.0f});
            }

            // 如果是样条对象，强制用黑色绘制控制线
            if (m_DataObject->GetDataObjectType() == IG_SPLINE_GEOMETRY) {
                auto shader = m_Scene->GetShader(ShaderType::PURECOLOR);
                shader->Use();
                shader->SetUniform3f("inputColor", igm::vec3{0.0f, 0.0f, 0.0f});
            }

            glLineWidth(surfaceObject->m_LineWidth);

            float f, u;
            surfaceObject->GetLineOffsetParameters(f, u);

            surfaceObject->m_LineVAO->DrawRangeElements(
                    GL_LINES, 0,
                    surfaceObject->m_Positions->GetNumberOfElements() - 1,
                    surfaceObject->m_LineIndices->GetNumberOfValues(),
                    GL_UNSIGNED_INT);
        }

        if (viewStyle & IG_SURFACE) {
            auto shader = m_Scene->GetShader(ShaderType::CULLINGPHASE1);
            shader->Use();

            unsigned int meshletCount = meshleter->m_MeshletCount;
            shader->SetUniformui("meshletCount", meshletCount);

            m_Scene->m_HzbTexture->Active(GL_TEXTURE1);
            shader->SetUniformi("hzbSampler", 1);

            meshleter->m_MeshletBuffer->BindBase(3);
            meshleter->m_MeshletVertexBuffer->BindBase(4);
            meshleter->m_MeshletTriangleBuffer->BindBase(5);
            meshleter->m_PositionBuffer->BindBase(6);
            meshleter->m_MeshletDescriptorBuffer->BindBase(10);

            auto cullDataBuffer = m_Scene->m_ShaderManager->GetCullDataBuffer();
            cullDataBuffer->BindBase(11);

            unsigned int data = 0;
            meshleter->m_InvisibleMeshletBuffer->SubData(
                    0, sizeof(unsigned int), &data);
            meshleter->m_InvisibleMeshletBuffer->BindBase(12);

            const int meshletPerTask = 32;

            unsigned int offset = 0;
            unsigned int count =
                    (meshletCount + meshletPerTask - 1) / meshletPerTask;
            glDrawMeshTasksNV(offset, count);

            // add barrier when read SSBO
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            unsigned int invisibleMeshletCount = 0;
            meshleter->m_InvisibleMeshletBuffer->GetSubData(
                    0, sizeof(unsigned int), &invisibleMeshletCount);

        #ifdef ENABLE_CULLING_DEBUGINFO
            IGAME_RENDERING_DEBUG("{}, draw phase 1 [visiable count:{}, "
                                  "meshlet count:{}]",
                                  meshleter->GetName(),
                                  meshletCount - invisibleMeshletCount,
                                  meshletCount);
        #endif
        }
    };
    #else
    auto draw = [&](const SmartPointer<DataObject>& dataObject) {
        auto drawObject = DynamicCast<DrawObject>(dataObject);
        if (!drawObject->GetVisibility()) { return; }

        bool hasTransparency = drawObject->GetTransparency() < 1.0f;
        bool hasAcceleration = drawObject->GetAccelerationOption();
        if (hasTransparency || !hasAcceleration) { return; }

        // Render
        m_Scene->UpdateObjectDataBlock(dataObject);
        m_Scene->UpdateUniformBufferObjectBlock(dataObject);

        auto surfaceObject = drawObject->m_RenderableMesh.SurfaceMesh;
        auto meshleter = drawObject->m_RenderableMesh.Meshleter;

        auto useColor = drawObject->IsUseColor();
        auto colorWithCell = drawObject->m_ColorWithCell;
        auto viewStyle = drawObject->GetViewStyle();

        // draw
        if (viewStyle & IG_POINTS) {
            auto shader = m_Scene->GetShader(ShaderType::NOLIGHT);
            shader->Use();

            // 如果是样条对象，强制用红色绘制控制点
            if (m_DataObject->GetDataObjectType() == IG_SPLINE_GEOMETRY) {
                auto shader = m_Scene->GetShader(ShaderType::PURECOLOR);
                shader->Use();
                shader->SetUniform3f("inputColor", igm::vec3{1.0f, 0.0f, 0.0f});
            }

            glPointSize(surfaceObject->m_PointSize);
            if (surfaceObject->m_PointIndices->GetNumberOfValues() == 0) {
                surfaceObject->m_PointVAO->DrawArrays(
                        GL_POINTS, 0,
                        surfaceObject->m_Positions->GetNumberOfElements());
            } else {
                surfaceObject->m_PointVAO->DrawRangeElements(
                        GL_POINTS, 0,
                        surfaceObject->m_Positions->GetNumberOfElements() - 1,
                        surfaceObject->m_PointIndices->GetNumberOfValues(),
                        GL_UNSIGNED_INT);
            }
        }

        if (viewStyle & IG_WIREFRAME) {
            if (useColor && !colorWithCell) {
                m_Scene->GetShader(ShaderType::NOLIGHT)->Use();
            } else {
                auto shader = m_Scene->GetShader(ShaderType::PURECOLOR);
                shader->Use();
                shader->SetUniform3f("inputColor", igm::vec3{0.0f, 0.0f, 0.0f});
            }

            // 如果是样条对象，强制用黑色绘制控制线
            if (m_DataObject->GetDataObjectType() == IG_SPLINE_GEOMETRY) {
                auto shader = m_Scene->GetShader(ShaderType::PURECOLOR);
                shader->Use();
                shader->SetUniform3f("inputColor", igm::vec3{0.0f, 0.0f, 0.0f});
            }

            glLineWidth(surfaceObject->m_LineWidth);

            surfaceObject->m_LineVAO->DrawRangeElements(
                    GL_LINES, 0,
                    surfaceObject->m_Positions->GetNumberOfElements() - 1,
                    surfaceObject->m_LineIndices->GetNumberOfValues(),
                    GL_UNSIGNED_INT);
        }

        if (viewStyle & IG_SURFACE) {
            m_Scene->GetShader(ShaderType::BLINNPHONG)->Use();

            meshleter->m_TriangleVAO->Bind();
            unsigned int visibleMeshletCount = 0;
            meshleter->m_VisibleMeshletBuffer->GetSubData(
                    0, sizeof(unsigned int), &visibleMeshletCount);
            meshleter->m_FinalDrawCommandBuffer->Target(
                    GL_DRAW_INDIRECT_BUFFER);
            meshleter->m_FinalDrawCommandBuffer->Bind();
            glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr,
                                        visibleMeshletCount, 0);
            meshleter->m_TriangleVAO->Release();

        #ifdef ENABLE_CULLING_DEBUGINFO
            IGAME_RENDERING_DEBUG("{}, draw phase 1 [visiable count:{}, "
                                  "meshlet count:{}]",
                                  meshleter->GetName(), visibleMeshletCount,
                                  meshleter->m_MeshletCount);
        #endif
        }
    };
    #endif

    auto drawObject = DynamicCast<DrawObject>(m_DataObject);
    if (!drawObject->HasSubDataObject()) {
        draw(drawObject);
    } else {
        for (auto it = drawObject->SubDataObjectIteratorBegin();
             it != drawObject->SubDataObjectIteratorEnd(); it++) {
            auto subDataObj = it->second;
            auto subDrawObj = DynamicCast<DrawObject>(subDataObj);
            draw(subDrawObj);
        }
    }
#endif
}

void Model::DrawPhase2() {
#ifdef IGAME_OPENGL_VERSION_460
    if (!this->GetVisibility()) { return; }

    #ifdef GL_SUPPORTS_MESH_SHADER
    auto draw = [&](const SmartPointer<DataObject>& dataObject) {
        auto drawObject = DynamicCast<DrawObject>(dataObject);
        if (!drawObject->GetVisibility()) { return; }

        bool hasTransparency = drawObject->GetTransparency() < 1.0f;
        bool hasAcceleration = drawObject->GetAccelerationOption();
        if (hasTransparency || !hasAcceleration) { return; }

        // Render
        m_Scene->UpdateObjectDataBlock(dataObject);
        m_Scene->UpdateUniformBufferObjectBlock(dataObject);

        auto surfaceObject = drawObject->m_RenderableMesh.SurfaceMesh;
        auto meshleter = drawObject->m_RenderableMesh.Meshleter;

        auto useColor = drawObject->IsUseColor();
        auto colorWithCell = drawObject->m_ColorWithCell;
        auto viewStyle = drawObject->GetViewStyle();

        // draw
        if (useColor && colorWithCell) {}

        if (viewStyle & IG_POINTS) {
            auto shader = m_Scene->GetShader(ShaderType::NOLIGHT);
            shader->Use();

            // 如果是样条对象，强制用红色绘制控制点
            if (m_DataObject->GetDataObjectType() == IG_SPLINE_GEOMETRY) {
                auto shader = m_Scene->GetShader(ShaderType::PURECOLOR);
                shader->Use();
                shader->SetUniform3f("inputColor", igm::vec3{1.0f, 0.0f, 0.0f});
            }

            glPointSize(surfaceObject->m_PointSize);

            float u;
            surfaceObject->GetPointOffsetParameters(u);

            if (surfaceObject->m_PointIndices->GetNumberOfValues() == 0) {
                surfaceObject->m_PointVAO->DrawArrays(
                        GL_POINTS, 0,
                        surfaceObject->m_Positions->GetNumberOfElements());
            } else {
                surfaceObject->m_PointVAO->DrawRangeElements(
                        GL_POINTS, 0,
                        surfaceObject->m_Positions->GetNumberOfElements() - 1,
                        surfaceObject->m_PointIndices->GetNumberOfValues(),
                        GL_UNSIGNED_INT);
            }
        }

        if (viewStyle & IG_WIREFRAME) {
            if (useColor) {
                m_Scene->GetShader(ShaderType::NOLIGHT)->Use();
            } else {
                auto shader = m_Scene->GetShader(ShaderType::PURECOLOR);
                shader->Use();
                shader->SetUniform3f("inputColor", igm::vec3{0.0f, 0.0f, 0.0f});
            }

            // 如果是样条对象，强制用黑色绘制控制线
            if (m_DataObject->GetDataObjectType() == IG_SPLINE_GEOMETRY) {
                auto shader = m_Scene->GetShader(ShaderType::PURECOLOR);
                shader->Use();
                shader->SetUniform3f("inputColor", igm::vec3{0.0f, 0.0f, 0.0f});
            }

            glLineWidth(surfaceObject->m_LineWidth);

            float f, u;
            surfaceObject->GetLineOffsetParameters(f, u);

            surfaceObject->m_LineVAO->DrawRangeElements(
                    GL_LINES, 0,
                    surfaceObject->m_Positions->GetNumberOfElements() - 1,
                    surfaceObject->m_LineIndices->GetNumberOfValues(),
                    GL_UNSIGNED_INT);
        }

        if (viewStyle & IG_SURFACE) {
            auto shader = m_Scene->GetShader(ShaderType::CULLINGPHASE2);
            shader->Use();

            // add barrier when read SSBO
            unsigned int invisibleMeshletCount = 0;
            meshleter->m_InvisibleMeshletBuffer->GetSubData(
                    0, sizeof(unsigned int), &invisibleMeshletCount);

            shader->SetUniformui("invisibleMeshletCount",
                                 invisibleMeshletCount);

            m_Scene->m_HzbTexture->Active(GL_TEXTURE1);
            shader->SetUniformi("hzbSampler", 1);

            meshleter->m_MeshletBuffer->BindBase(3);
            meshleter->m_MeshletVertexBuffer->BindBase(4);
            meshleter->m_MeshletTriangleBuffer->BindBase(5);
            meshleter->m_PositionBuffer->BindBase(6);
            meshleter->m_MeshletDescriptorBuffer->BindBase(10);

            auto cullDataBuffer = m_Scene->m_ShaderManager->GetCullDataBuffer();
            cullDataBuffer->BindBase(11);

            unsigned int data = 0;
            meshleter->m_InvisibleMeshletBuffer->SubData(
                    0, sizeof(unsigned int), &data);
            meshleter->m_InvisibleMeshletBuffer->BindBase(12);

            const int meshletPerTask = 32;

            unsigned int offset = 0;
            unsigned int count = (invisibleMeshletCount + meshletPerTask - 1) /
                                 meshletPerTask;
            glDrawMeshTasksNV(offset, count);

            // add barrier when read SSBO
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            unsigned int c = 0;
            meshleter->m_InvisibleMeshletBuffer->GetSubData(
                    0, sizeof(unsigned int), &c);

        #ifdef ENABLE_CULLING_DEBUGINFO
            IGAME_RENDERING_DEBUG("{}, draw phase 2 [visiable count:{}, "
                                  "meshlet count:{}]",
                                  meshleter->GetName(),
                                  invisibleMeshletCount - c,
                                  invisibleMeshletCount);
        #endif
        }
    };
    #else
    auto draw = [&](const SmartPointer<DataObject>& dataObject) {
        auto drawObject = DynamicCast<DrawObject>(dataObject);
        if (!drawObject->GetVisibility()) { return; }

        bool hasTransparency = drawObject->GetTransparency() < 1.0f;
        bool hasAcceleration = drawObject->GetAccelerationOption();
        if (hasTransparency || !hasAcceleration) { return; }

        // Render
        m_Scene->UpdateObjectDataBlock(dataObject);
        m_Scene->UpdateUniformBufferObjectBlock(dataObject);

        auto surfaceObject = drawObject->m_RenderableMesh.SurfaceMesh;
        auto meshleter = drawObject->m_RenderableMesh.Meshleter;

        auto useColor = drawObject->IsUseColor();
        auto colorWithCell = drawObject->m_ColorWithCell;
        auto viewStyle = drawObject->GetViewStyle();

        // draw
        if (viewStyle & IG_POINTS) {
            auto shader = m_Scene->GetShader(ShaderType::NOLIGHT);
            shader->Use();

            // 如果是样条对象，强制用红色绘制控制点
            if (m_DataObject->GetDataObjectType() == IG_SPLINE_GEOMETRY) {
                auto shader = m_Scene->GetShader(ShaderType::PURECOLOR);
                shader->Use();
                shader->SetUniform3f("inputColor", igm::vec3{1.0f, 0.0f, 0.0f});
            }

            glPointSize(surfaceObject->m_PointSize);
            if (surfaceObject->m_PointIndices->GetNumberOfValues() == 0) {
                surfaceObject->m_PointVAO->DrawArrays(
                        GL_POINTS, 0,
                        surfaceObject->m_Positions->GetNumberOfElements());
            } else {
                surfaceObject->m_PointVAO->DrawRangeElements(
                        GL_POINTS, 0,
                        surfaceObject->m_Positions->GetNumberOfElements() - 1,
                        surfaceObject->m_PointIndices->GetNumberOfValues(),
                        GL_UNSIGNED_INT);
            }
        }

        if (viewStyle & IG_WIREFRAME) {
            if (useColor && !colorWithCell) {
                m_Scene->GetShader(ShaderType::NOLIGHT)->Use();
            } else {
                auto shader = m_Scene->GetShader(ShaderType::PURECOLOR);
                shader->Use();
                shader->SetUniform3f("inputColor", igm::vec3{0.0f, 0.0f, 0.0f});
            }

            // 如果是样条对象，强制用黑色绘制控制线
            if (m_DataObject->GetDataObjectType() == IG_SPLINE_GEOMETRY) {
                auto shader = m_Scene->GetShader(ShaderType::PURECOLOR);
                shader->Use();
                shader->SetUniform3f("inputColor", igm::vec3{0.0f, 0.0f, 0.0f});
            }

            glLineWidth(surfaceObject->m_LineWidth);
            surfaceObject->m_LineVAO->DrawRangeElements(
                    GL_LINES, 0,
                    surfaceObject->m_Positions->GetNumberOfElements() - 1,
                    surfaceObject->m_LineIndices->GetNumberOfValues(),
                    GL_UNSIGNED_INT);
        }

        if (viewStyle & IG_SURFACE) {
            unsigned int lastVisibleMeshletCount = 0;
            meshleter->m_VisibleMeshletBuffer->GetSubData(
                    0, sizeof(unsigned int), &lastVisibleMeshletCount);
            // compute culling
            {
                auto shader = m_Scene->GetShader(ShaderType::MESHLETCULL);
                shader->Use();

                shader->SetUniformi("workMode", 0);

                meshleter->m_MeshletDescriptorBuffer->BindBase(1);
                meshleter->m_DrawCommandBuffer->BindBase(2);

                unsigned int data = 0;
                meshleter->m_VisibleMeshletBuffer->SubData(
                        0, sizeof(unsigned int), &data);
                meshleter->m_VisibleMeshletBuffer->BindBase(3);

                // need switch to the GL_SHADER_STORAGE_BUFFER target
                meshleter->m_FinalDrawCommandBuffer->Target(
                        GL_SHADER_STORAGE_BUFFER);
                meshleter->m_FinalDrawCommandBuffer->BindBase(4);

                auto cullDataBuffer =
                        m_Scene->m_ShaderManager->GetCullDataBuffer();
                cullDataBuffer->Target(GL_UNIFORM_BUFFER);
                cullDataBuffer->BindBase(5);

                m_Scene->m_HzbTexture->Active(GL_TEXTURE1);
                shader->SetUniformi("hzbSampler", 1);

                auto count = meshleter->m_MeshletCount;
                glDispatchCompute(((count + 255) / 256), 1, 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            }

            m_Scene->GetShader(ShaderType::BLINNPHONG)->Use();

            meshleter->m_TriangleVAO->Bind();
            unsigned int count = 0;
            meshleter->m_VisibleMeshletBuffer->GetSubData(
                    0, sizeof(unsigned int), &count);
            meshleter->m_FinalDrawCommandBuffer->Target(
                    GL_DRAW_INDIRECT_BUFFER);
            meshleter->m_FinalDrawCommandBuffer->Bind();
            glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr,
                                        count, 0);
            meshleter->m_TriangleVAO->Release();

        #ifdef ENABLE_CULLING_DEBUGINFO
            IGAME_RENDERING_DEBUG("{}, draw phase 2 [visiable count:{}, "
                                  "meshlet count:{}]",
                                  meshleter->GetName(), count,
                                  meshleter->m_MeshletCount -
                                          lastVisibleMeshletCount);
        #endif
        }
    };
    #endif

    auto drawObject = DynamicCast<DrawObject>(m_DataObject);
    if (!drawObject->HasSubDataObject()) {
        draw(drawObject);
    } else {
        for (auto it = drawObject->SubDataObjectIteratorBegin();
             it != drawObject->SubDataObjectIteratorEnd(); it++) {
            auto subDataObj = it->second;
            auto subDrawObj = DynamicCast<DrawObject>(subDataObj);
            draw(subDrawObj);
        }
    }
#endif
}

void Model::TestOcclusionResults() {
    // 只有计算着色器需要额外计算一次可见性
#if defined(IGAME_OPENGL_VERSION_460) && !defined(GL_SUPPORTS_MESH_SHADER)
    if (!this->GetVisibility()) { return; }

    auto draw = [&](const SmartPointer<DataObject>& dataObject) {
        auto drawObject = DynamicCast<DrawObject>(dataObject);
        if (!drawObject->GetVisibility()) { return; }

        bool hasTransparency = drawObject->GetTransparency() < 1.0f;
        bool hasAcceleration = drawObject->GetAccelerationOption();
        if (hasTransparency || !hasAcceleration) { return; }

        // compute
        auto meshleter = drawObject->m_RenderableMesh.Meshleter;
        auto viewStyle = drawObject->GetViewStyle();

        // test
        if (viewStyle & IG_SURFACE) {
            // compute culling
            {
                auto shader = m_Scene->GetShader(ShaderType::MESHLETCULL);
                shader->Use();

                shader->SetUniformi("workMode", 1);

                meshleter->m_MeshletDescriptorBuffer->BindBase(1);
                meshleter->m_DrawCommandBuffer->BindBase(2);

                unsigned int data = 0;
                meshleter->m_VisibleMeshletBuffer->SubData(
                        0, sizeof(unsigned int), &data);
                meshleter->m_VisibleMeshletBuffer->BindBase(3);

                // need switch to the GL_SHADER_STORAGE_BUFFER target
                meshleter->m_FinalDrawCommandBuffer->Target(
                        GL_SHADER_STORAGE_BUFFER);
                meshleter->m_FinalDrawCommandBuffer->BindBase(4);

                auto cullDataBuffer =
                        m_Scene->m_ShaderManager->GetCullDataBuffer();
                cullDataBuffer->BindBase(5);

                m_Scene->m_HzbTexture->Active(GL_TEXTURE1);
                shader->SetUniformi("hzbSampler", 1);

                size_t count = meshleter->m_MeshletCount;
                glDispatchCompute(static_cast<GLuint>((count + 255) / 256), 1,
                                  1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            }
        }

        // unsigned int count = 0;
        // meshleter->m_VisibleMeshletBuffer->GetSubData(0, sizeof(unsigned int),
        //                                               &count);
        // std::cout << "Test Occlusion: [render count: " << count;
        // std::cout << ", meshlet count: " << meshleter->m_MeshletCount << "]"
        //           << std::endl;
        //
        // std::vector<DrawElementsIndirectCommand> readBackCommands(
        //         meshleter->m_MeshletCount);
        // meshleter->m_DrawCommandBuffer->GetSubData(
        //         0,
        //         readBackCommands.size() * sizeof(DrawElementsIndirectCommand),
        //         readBackCommands.data());
        // for (const auto& cmd: readBackCommands) {
        //     //std::cout << "count: " << cmd.count << std::endl;
        //     std::cout << "primCount: " << cmd.primCount << std::endl;
        //     std::cout << "firstIndex: " << cmd.firstIndex << std::endl;
        //     //std::cout << "baseVertex: " << cmd.baseVertex << std::endl;
        //     //std::cout << "baseInstance: " << cmd.baseInstance << std::endl;
        // }
    };

    auto drawObject = DynamicCast<DrawObject>(m_DataObject);
    if (!drawObject->HasSubDataObject()) {
        draw(drawObject);
    } else {
        for (auto it = drawObject->SubDataObjectIteratorBegin();
             it != drawObject->SubDataObjectIteratorEnd(); it++) {
            auto subDataObj = it->second;
            auto subDrawObj = DynamicCast<DrawObject>(subDataObj);
            draw(subDrawObj);
        }
    }
#endif
}

IGAME_NAMESPACE_END
