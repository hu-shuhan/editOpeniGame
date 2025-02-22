#include "iGameModel.h"
#include "iGameFilter.h"
#include "iGameInteractor.h"
#include "iGameRenderingLogger.h"
#include "iGameScene.h"
#include <format>

IGAME_NAMESPACE_BEGIN

Model::Model() {
    SwitchOff(ViewSwitch::BoundingBox);
    SwitchOn(ViewSwitch::PickedItem);

    m_Selection = Selection::New();
    m_DataObject = DataObject::New();
    m_Scene = nullptr;

    m_Painter3D = Painter3D::New();
    m_BboxHandle = 0;
    m_Switch = 0ull;
}

Model::~Model() {}

void Model::Draw(SmartPointer<Scene> scene) {
    auto draw = [&](const SmartPointer<DataObject>& dataObject) {
        scene->UpdateObjectDataBlock(dataObject);
        scene->UpdateUniformBufferObjectBlock(dataObject);

        auto drawObject = DynamicCast<DrawObject>(dataObject);
        auto visibility = drawObject->m_Visibility;
        auto useColor = drawObject->m_UseColor;
        auto colorWithCell = drawObject->m_ColorWithCell;
        auto viewStyle = drawObject->m_ViewStyle;

        if (!visibility) { return; }

        if (useColor && colorWithCell) {
            scene->GetShader(ShaderType::BLINNPHONG)->Use();

            float f, u;
            drawObject->GetPolygonOffsetParameters(f, u);

            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(f, u);
            drawObject->m_CellVAO->DrawArrays(GL_TRIANGLES, 0,
                                              drawObject->m_CellPositionSize);
            glDisable(GL_POLYGON_OFFSET_FILL);

            return;
        }

        if (viewStyle & IG_POINTS) {
            auto shader = scene->GetShader(ShaderType::NOLIGHT);
            shader->Use();

            glad_glPointSize(drawObject->m_PointSize);

            float u;
            drawObject->GetPointOffsetParameters(u);

            if (drawObject->m_PointIndices->GetNumberOfValues() == 0) {
                drawObject->m_PointVAO->DrawArrays(
                        GL_POINTS, 0,
                        drawObject->m_Positions->GetNumberOfElements());
            } else {
                drawObject->m_PointVAO->DrawRangeElements(
                        GL_POINTS, 0,
                        drawObject->m_Positions->GetNumberOfElements() - 1,
                        drawObject->m_PointIndices->GetNumberOfValues(),
                        GL_UNSIGNED_INT);
            }
        }

        // whether to use single-pass wireframe rendering
        if (viewStyle & IG_WIREFRAME && viewStyle & IG_SURFACE &&
            drawObject->IsUseSinglePassWireframeRendering()) {
            auto shader = scene->GetShader(ShaderType::SINGLEPASSWIREFRAME);
            shader->Use();

            shader->SetUniformf("lineWidth", drawObject->GetLineWidth());

            drawObject->m_EdgeMaskTexture->Active(GL_TEXTURE1);
            shader->SetUniformi("edgeMasks", 1);

            if (useColor) {
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

            drawObject->m_TriangleVAO->DrawRangeElements(
                    GL_TRIANGLES, 0,
                    drawObject->m_Positions->GetNumberOfElements() - 1,
                    drawObject->m_TriangleIndices->GetNumberOfValues(),
                    GL_UNSIGNED_INT);
        } else {
            if (viewStyle & IG_SURFACE) {
                auto shader = scene->GetShader(ShaderType::BLINNPHONG);
                shader->Use();

                float f, u;
                drawObject->GetPolygonOffsetParameters(f, u);

                glEnable(GL_POLYGON_OFFSET_FILL);
                glPolygonOffset(f, u);
                drawObject->m_TriangleVAO->DrawRangeElements(
                        GL_TRIANGLES, 0,
                        drawObject->m_Positions->GetNumberOfElements() - 1,
                        drawObject->m_TriangleIndices->GetNumberOfValues(),
                        GL_UNSIGNED_INT);
                glDisable(GL_POLYGON_OFFSET_FILL);
            }

            if (viewStyle & IG_WIREFRAME) {
                if (useColor) {
                    scene->GetShader(ShaderType::NOLIGHT)->Use();
                } else {
                    auto shader = scene->GetShader(ShaderType::PURECOLOR);
                    shader->Use();
                    shader->SetUniform3f("inputColor",
                                         igm::vec3{0.0f, 0.0f, 0.0f});
                }

                glLineWidth(drawObject->m_LineWidth);

                float f, u;
                drawObject->GetLineOffsetParameters(f, u);

                drawObject->m_LineVAO->DrawRangeElements(
                        GL_LINES, 0,
                        drawObject->m_Positions->GetNumberOfElements() - 1,
                        drawObject->m_LineIndices->GetNumberOfValues(),
                        GL_UNSIGNED_INT);
            }
        }
    };

    auto dataObject = m_DataObject;
    auto drawObject = DynamicCast<DrawObject>(dataObject);

    if (drawObject->m_DisplayObject != nullptr) {
        dataObject = DynamicCast<DataObject>(drawObject->m_DisplayObject);
        drawObject = DynamicCast<DrawObject>(dataObject);
    }

    if (!drawObject->m_Visibility) { return; }

    if (!dataObject->HasSubDataObject()) {
        draw(dataObject);
    } else {
        for (auto it = dataObject->SubDataObjectIteratorBegin();
             it != dataObject->SubDataObjectIteratorEnd(); it++) {
            auto subDataObj = it->second;
            auto subDrawObj = DynamicCast<DrawObject>(subDataObj);
            if (subDrawObj->m_DisplayObject == nullptr) {
                draw(subDataObj);
            } else {
                draw(subDrawObj->m_DisplayObject);
            }
        }
    }
}

void Model::DrawWithTransparency(SmartPointer<Scene> scene) {
    auto draw = [&](const SmartPointer<DataObject>& dataObject) {
        scene->UpdateObjectDataBlock(dataObject);
        scene->UpdateUniformBufferObjectBlock(dataObject);

        auto drawObject = DynamicCast<DrawObject>(dataObject);
        auto visibility = drawObject->m_Visibility;
        auto useColor = drawObject->m_UseColor;
        auto colorWithCell = drawObject->m_ColorWithCell;
        auto viewStyle = drawObject->m_ViewStyle;

        if (!visibility) { return; }

        if (useColor && colorWithCell) {
            auto shader = scene->GetShader(ShaderType::TRANSPARENCYLINK);
            shader->Use();
            shader->SetUniformi("colorMode", 0);

            float f, u;
            drawObject->GetPolygonOffsetParameters(f, u);

            //glEnable(GL_POLYGON_OFFSET_FILL);
            //glPolygonOffset(f, u);
            drawObject->m_CellVAO->DrawArrays(GL_TRIANGLES, 0,
                                              drawObject->m_CellPositionSize);
            //glDisable(GL_POLYGON_OFFSET_FILL);

            return;
        }

        if (viewStyle & IG_POINTS) {
            auto shader = scene->GetShader(ShaderType::TRANSPARENCYLINK);
            shader->Use();
            shader->SetUniformi("colorMode", 1);

            glad_glPointSize(drawObject->m_PointSize);

            float u;
            drawObject->GetPointOffsetParameters(u);

            //glEnable(GL_POLYGON_OFFSET_POINT);
            //glPolygonOffset(0.0f, u);
            if (drawObject->m_PointIndices->GetNumberOfValues() == 0) {
                drawObject->m_PointVAO->DrawArrays(
                        GL_POINTS, 0,
                        drawObject->m_Positions->GetNumberOfElements());
            } else {
                drawObject->m_PointVAO->DrawRangeElements(
                        GL_POINTS, 0,
                        drawObject->m_Positions->GetNumberOfElements() - 1,
                        drawObject->m_PointIndices->GetNumberOfValues(),
                        GL_UNSIGNED_INT);
            }
            //glDisable(GL_POLYGON_OFFSET_POINT);
        }

        if (viewStyle & IG_WIREFRAME) {
            if (useColor) {
                auto shader = scene->GetShader(ShaderType::TRANSPARENCYLINK);
                shader->Use();
                shader->SetUniformi("colorMode", 1);
            } else {
                auto shader = scene->GetShader(ShaderType::TRANSPARENCYLINK);
                shader->Use();
                shader->SetUniformi("colorMode", 2);
                shader->SetUniform3f("inputColor", igm::vec3{0.0f, 0.0f, 0.0f});
            }

            glLineWidth(drawObject->m_LineWidth);

            float f, u;
            drawObject->GetLineOffsetParameters(f, u);

            //glEnable(GL_POLYGON_OFFSET_LINE);
            //glPolygonOffset(f, u);
            drawObject->m_LineVAO->DrawRangeElements(
                    GL_LINES, 0,
                    drawObject->m_Positions->GetNumberOfElements() - 1,
                    drawObject->m_LineIndices->GetNumberOfValues(),
                    GL_UNSIGNED_INT);
            //glDisable(GL_POLYGON_OFFSET_LINE);
        }
        if (viewStyle & IG_SURFACE) {
            auto shader = scene->GetShader(ShaderType::TRANSPARENCYLINK);
            shader->Use();
            shader->SetUniformi("colorMode", 0);

            float f, u;
            drawObject->GetPolygonOffsetParameters(f, u);

            //glEnable(GL_POLYGON_OFFSET_FILL);
            //glPolygonOffset(f, u);
            drawObject->m_TriangleVAO->DrawRangeElements(
                    GL_TRIANGLES, 0,
                    drawObject->m_Positions->GetNumberOfElements() - 1,
                    drawObject->m_TriangleIndices->GetNumberOfValues(),
                    GL_UNSIGNED_INT);
            //glDisable(GL_POLYGON_OFFSET_FILL);
        }
    };

    auto dataObject = m_DataObject;
    auto drawObject = DynamicCast<DrawObject>(dataObject);

    if (drawObject->m_DisplayObject != nullptr) {
        dataObject = DynamicCast<DataObject>(drawObject->m_DisplayObject);
        drawObject = DynamicCast<DrawObject>(dataObject);
    }

    if (!drawObject->m_Visibility) { return; }

    if (!dataObject->HasSubDataObject()) {
        draw(dataObject);
    } else {
        for (auto it = dataObject->SubDataObjectIteratorBegin();
             it != dataObject->SubDataObjectIteratorEnd(); it++) {
            auto subDataObj = it->second;
            auto subDrawObj = DynamicCast<DrawObject>(subDataObj);
            if (subDrawObj->m_DisplayObject == nullptr) {
                draw(subDataObj);
            } else {
                draw(subDrawObj->m_DisplayObject);
            }
        }
    }
}

void Model::DrawWithVolume(SmartPointer<Scene> scene) {
    auto draw = [&](const SmartPointer<DataObject>& dataObject) {
        scene->UpdateObjectDataBlock(dataObject);
        scene->UpdateUniformBufferObjectBlock(dataObject);

        auto drawObject = DynamicCast<DrawObject>(dataObject);
        auto visibility = drawObject->m_Visibility;
        auto useColor = drawObject->m_UseColor;
        auto colorWithCell = drawObject->m_ColorWithCell;
        auto viewStyle = drawObject->m_ViewStyle;

        if (!visibility) { return; }

        if (useColor && colorWithCell) {
            auto shader = scene->GetShader(ShaderType::VOLUMERENDERINGLINK);
            shader->Use();

            float f, u;
            drawObject->GetPolygonOffsetParameters(f, u);

            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(f, u);
            drawObject->m_CellVAO->DrawArrays(GL_TRIANGLES, 0,
                                              drawObject->m_CellPositionSize);
            glDisable(GL_POLYGON_OFFSET_FILL);

            return;
        }

        if (viewStyle & IG_POINTS) {}

        if (viewStyle & IG_WIREFRAME) {}

        if (viewStyle & IG_SURFACE) {
            auto shader = scene->GetShader(ShaderType::VOLUMERENDERINGLINK);
            shader->Use();

            float f, u;
            drawObject->GetPolygonOffsetParameters(f, u);

            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(f, u);
            drawObject->m_TriangleVAO->DrawRangeElements(
                    GL_TRIANGLES, 0,
                    drawObject->m_Positions->GetNumberOfElements() - 1,
                    drawObject->m_TriangleIndices->GetNumberOfValues(),
                    GL_UNSIGNED_INT);
            glDisable(GL_POLYGON_OFFSET_FILL);
        }
    };

    auto dataObject = m_DataObject;
    auto drawObject = DynamicCast<DrawObject>(dataObject);

    if (drawObject->m_DisplayObject != nullptr) {
        dataObject = DynamicCast<DataObject>(drawObject->m_DisplayObject);
        drawObject = DynamicCast<DrawObject>(dataObject);
    }

    if (!drawObject->m_Visibility) { return; }

    if (!dataObject->HasSubDataObject()) {
        draw(dataObject);
    } else {
        for (auto it = dataObject->SubDataObjectIteratorBegin();
             it != dataObject->SubDataObjectIteratorEnd(); it++) {
            auto subDataObj = it->second;
            auto subDrawObj = DynamicCast<DrawObject>(subDataObj);
            if (subDrawObj->m_DisplayObject == nullptr) {
                draw(subDataObj);
            } else {
                draw(subDrawObj->m_DisplayObject);
            }
        }
    }
}

void Model::DrawPhase1(SmartPointer<Scene> scene) {
#ifdef IGAME_OPENGL_VERSION_460
    auto dataObject = m_DataObject;
    auto drawObject = DynamicCast<DrawObject>(dataObject);

    if (drawObject->m_DisplayObject != nullptr) {
        dataObject = DynamicCast<DataObject>(drawObject->m_DisplayObject);
        drawObject = DynamicCast<DrawObject>(dataObject);
    }

    if (!drawObject->m_Visibility) { return; }

    // Update GPU data
    m_Meshleter->Update();

#ifdef GL_SUPPORTS_MESH_SHADER
    auto draw = [&](const SmartPointer<DataObject>& dataObject) {
        scene->UpdateObjectDataBlock(dataObject);
        scene->UpdateUniformBufferObjectBlock(dataObject);

        auto drawObject = DynamicCast<DrawObject>(dataObject);
        auto visibility = drawObject->m_Visibility;
        auto useColor = drawObject->m_UseColor;
        auto colorWithCell = drawObject->m_ColorWithCell;
        auto viewStyle = drawObject->m_ViewStyle;

        if (!visibility) { return; }

        // draw
        if (useColor && colorWithCell) {}

        if (viewStyle & IG_POINTS) {}
        if (viewStyle & IG_WIREFRAME) {}
        if (viewStyle & IG_SURFACE) {
            auto shader = scene->GetShader(ShaderType::CULLINGPHASE1);
            shader->Use();

            unsigned int meshletCount = m_Meshleter->m_MeshletCount;
            shader->SetUniformui("meshletCount", meshletCount);

            scene->m_HzbTexture->Active(GL_TEXTURE1);
            shader->SetUniformi("hzbSampler", 1);

            m_Meshleter->m_MeshletBuffer->BindBase(3);
            m_Meshleter->m_MeshletVertexBuffer->BindBase(4);
            m_Meshleter->m_MeshletTriangleBuffer->BindBase(5);
            m_Meshleter->m_PositionBuffer->BindBase(6);
            m_Meshleter->m_MeshletDescriptorBuffer->BindBase(10);

            auto cullDataBuffer = scene->m_ShaderManager->GetCullDataBuffer();
            cullDataBuffer->BindBase(11);

            unsigned int data = 0;
            m_Meshleter->m_InvisibleMeshletBuffer->SubData(
                    0, sizeof(unsigned int), &data);
            m_Meshleter->m_InvisibleMeshletBuffer->BindBase(12);

            const int meshletPerTask = 32;

            unsigned int offset = 0;
            unsigned int count =
                    (meshletCount + meshletPerTask - 1) / meshletPerTask;
            glDrawMeshTasksNV(offset, count);

            // add barrier when read SSBO
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            unsigned int invisibleMeshletCount = 0;
            m_Meshleter->m_InvisibleMeshletBuffer->GetSubData(
                    0, sizeof(unsigned int), &invisibleMeshletCount);

            Logger::LogDebug(
                    "{}, draw phase 1 [visiable count:{}, meshlet count:{}]",
                    m_Meshleter->GetName(),
                    meshletCount - invisibleMeshletCount, meshletCount);
        }
    };
#else
    auto draw = [&](const SmartPointer<DataObject>& dataObject) {
        scene->UpdateObjectDataBlock(dataObject);
        scene->UpdateUniformBufferObjectBlock(dataObject);

        auto drawObject = DynamicCast<DrawObject>(dataObject);
        auto visibility = drawObject->m_Visibility;
        auto useColor = drawObject->m_UseColor;
        auto colorWithCell = drawObject->m_ColorWithCell;
        auto viewStyle = drawObject->m_ViewStyle;

        if (!visibility) { return; }

        // draw
        if (useColor && colorWithCell) {}

        if (viewStyle & IG_POINTS) {}
        if (viewStyle & IG_WIREFRAME) {}
        if (viewStyle & IG_SURFACE) {
            scene->GetShader(ShaderType::BLINNPHONG)->Use();

            m_Meshleter->m_TriangleVAO->Bind();
            unsigned int visibleMeshletCount = 0;
            m_Meshleter->m_VisibleMeshletBuffer->GetSubData(
                    0, sizeof(unsigned int), &visibleMeshletCount);
            m_Meshleter->m_FinalDrawCommandBuffer->Target(
                    GL_DRAW_INDIRECT_BUFFER);
            m_Meshleter->m_FinalDrawCommandBuffer->Bind();
            glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr,
                                        visibleMeshletCount, 0);
            m_Meshleter->m_TriangleVAO->Release();

            Logger::LogDebug(
                    "{}, draw phase 1 [visiable count:{}, meshlet count:{}]",
                    m_Meshleter->GetName(), visibleMeshletCount,
                    m_Meshleter->m_MeshletCount);
        }
    };
#endif

    if (!dataObject->HasSubDataObject()) {
        draw(dataObject);
    } else {
        for (auto it = dataObject->SubDataObjectIteratorBegin();
             it != dataObject->SubDataObjectIteratorEnd(); it++) {
            auto subDataObj = it->second;
            auto subDrawObj = DynamicCast<DrawObject>(subDataObj);
            if (subDrawObj->m_DisplayObject == nullptr) {
                draw(subDataObj);
            } else {
                draw(subDrawObj->m_DisplayObject);
            }
        }
    }
#endif
}

void Model::DrawPhase2(SmartPointer<Scene> scene) {
#ifdef IGAME_OPENGL_VERSION_460
    auto dataObject = m_DataObject;
    auto drawObject = DynamicCast<DrawObject>(dataObject);

    if (drawObject->m_DisplayObject != nullptr) {
        dataObject = DynamicCast<DataObject>(drawObject->m_DisplayObject);
        drawObject = DynamicCast<DrawObject>(dataObject);
    }

    if (!drawObject->m_Visibility) { return; }

#ifdef GL_SUPPORTS_MESH_SHADER
    auto draw = [&](const SmartPointer<DataObject>& dataObject) {
        scene->UpdateObjectDataBlock(dataObject);
        scene->UpdateUniformBufferObjectBlock(dataObject);

        auto drawObject = DynamicCast<DrawObject>(dataObject);
        auto visibility = drawObject->m_Visibility;
        auto useColor = drawObject->m_UseColor;
        auto colorWithCell = drawObject->m_ColorWithCell;
        auto viewStyle = drawObject->m_ViewStyle;

        if (!visibility) { return; }

        // draw
        if (useColor && colorWithCell) {}

        if (viewStyle & IG_POINTS) {}
        if (viewStyle & IG_WIREFRAME) {}
        if (viewStyle & IG_SURFACE) {
            auto shader = scene->GetShader(ShaderType::CULLINGPHASE2);
            shader->Use();

            // add barrier when read SSBO
            unsigned int invisibleMeshletCount = 0;
            m_Meshleter->m_InvisibleMeshletBuffer->GetSubData(
                    0, sizeof(unsigned int), &invisibleMeshletCount);

            shader->SetUniformui("invisibleMeshletCount",
                                 invisibleMeshletCount);

            scene->m_HzbTexture->Active(GL_TEXTURE1);
            shader->SetUniformi("hzbSampler", 1);

            m_Meshleter->m_MeshletBuffer->BindBase(3);
            m_Meshleter->m_MeshletVertexBuffer->BindBase(4);
            m_Meshleter->m_MeshletTriangleBuffer->BindBase(5);
            m_Meshleter->m_PositionBuffer->BindBase(6);
            m_Meshleter->m_MeshletDescriptorBuffer->BindBase(10);

            auto cullDataBuffer = scene->m_ShaderManager->GetCullDataBuffer();
            cullDataBuffer->BindBase(11);

            unsigned int data = 0;
            m_Meshleter->m_InvisibleMeshletBuffer->SubData(
                    0, sizeof(unsigned int), &data);
            m_Meshleter->m_InvisibleMeshletBuffer->BindBase(12);

            const int meshletPerTask = 32;

            unsigned int offset = 0;
            unsigned int count = (invisibleMeshletCount + meshletPerTask - 1) /
                                 meshletPerTask;
            glDrawMeshTasksNV(offset, count);

            // add barrier when read SSBO
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            unsigned int c = 0;
            m_Meshleter->m_InvisibleMeshletBuffer->GetSubData(
                    0, sizeof(unsigned int), &c);

            Logger::LogDebug(
                    "{}, draw phase 2 [visiable count:{}, meshlet count:{}]",
                    m_Meshleter->GetName(), invisibleMeshletCount - c,
                    invisibleMeshletCount);
        }
    };
#else
    auto draw = [&](const SmartPointer<DataObject>& dataObject) {
        scene->UpdateObjectDataBlock(dataObject);
        scene->UpdateUniformBufferObjectBlock(dataObject);

        auto drawObject = DynamicCast<DrawObject>(dataObject);
        auto visibility = drawObject->m_Visibility;
        auto useColor = drawObject->m_UseColor;
        auto colorWithCell = drawObject->m_ColorWithCell;
        auto viewStyle = drawObject->m_ViewStyle;

        if (!visibility) { return; }

        // draw
        if (useColor && colorWithCell) {}

        if (viewStyle & IG_POINTS) {}
        if (viewStyle & IG_WIREFRAME) {}
        if (viewStyle & IG_SURFACE) {
            unsigned int lastVisibleMeshletCount = 0;
            m_Meshleter->m_VisibleMeshletBuffer->GetSubData(
                    0, sizeof(unsigned int), &lastVisibleMeshletCount);
            // compute culling
            {
                auto shader = scene->GetShader(ShaderType::MESHLETCULL);
                shader->Use();

                shader->SetUniformi("workMode", 0);

                m_Meshleter->m_MeshletDescriptorBuffer->BindBase(1);
                m_Meshleter->m_DrawCommandBuffer->BindBase(2);

                unsigned int data = 0;
                m_Meshleter->m_VisibleMeshletBuffer->SubData(
                        0, sizeof(unsigned int), &data);
                m_Meshleter->m_VisibleMeshletBuffer->BindBase(3);

                // need switch to the GL_SHADER_STORAGE_BUFFER target
                m_Meshleter->m_FinalDrawCommandBuffer->Target(
                        GL_SHADER_STORAGE_BUFFER);
                m_Meshleter->m_FinalDrawCommandBuffer->BindBase(4);

                auto cullDataBuffer =
                        scene->m_ShaderManager->GetCullDataBuffer();
                cullDataBuffer->Target(GL_UNIFORM_BUFFER);
                cullDataBuffer->BindBase(5);

                scene->m_HzbTexture->Active(GL_TEXTURE1);
                shader->SetUniformi("hzbSampler", 1);

                auto count = m_Meshleter->m_MeshletCount;
                glDispatchCompute(((count + 255) / 256), 1, 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            }

            scene->GetShader(ShaderType::BLINNPHONG)->Use();

            m_Meshleter->m_TriangleVAO->Bind();
            unsigned int count = 0;
            m_Meshleter->m_VisibleMeshletBuffer->GetSubData(
                    0, sizeof(unsigned int), &count);
            m_Meshleter->m_FinalDrawCommandBuffer->Target(
                    GL_DRAW_INDIRECT_BUFFER);
            m_Meshleter->m_FinalDrawCommandBuffer->Bind();
            glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr,
                                        count, 0);
            m_Meshleter->m_TriangleVAO->Release();

            Logger::LogDebug(
                    "{}, draw phase 2 [visiable count:{}, meshlet count:{}]",
                    m_Meshleter->GetName(), count,
                    m_Meshleter->m_MeshletCount - lastVisibleMeshletCount);
        }
    };
#endif

    if (!dataObject->HasSubDataObject()) {
        draw(dataObject);
    } else {
        for (auto it = dataObject->SubDataObjectIteratorBegin();
             it != dataObject->SubDataObjectIteratorEnd(); it++) {
            auto subDataObj = it->second;
            auto subDrawObj = DynamicCast<DrawObject>(subDataObj);
            if (subDrawObj->m_DisplayObject == nullptr) {
                draw(subDataObj);
            } else {
                draw(subDrawObj->m_DisplayObject);
            }
        }
    }
#endif
}

void Model::TestOcclusionResults(SmartPointer<Scene> scene) {
#ifdef IGAME_OPENGL_VERSION_460
#ifndef GL_SUPPORTS_MESH_SHADER
    // Update GPU data
    m_Meshleter->Update();

    auto draw = [&](const SmartPointer<DataObject>& dataObject) {
        auto drawObject = DynamicCast<DrawObject>(dataObject);
        auto visibility = drawObject->m_Visibility;
        auto useColor = drawObject->m_UseColor;
        auto colorWithCell = drawObject->m_ColorWithCell;
        auto viewStyle = drawObject->m_ViewStyle;

        if (!visibility) { return; }

        if (useColor && colorWithCell) {}

        if (viewStyle & IG_POINTS) {}
        if (viewStyle & IG_WIREFRAME) {}
        if (viewStyle & IG_SURFACE) {
            // compute culling
            {
                auto shader = scene->GetShader(ShaderType::MESHLETCULL);
                shader->Use();

                shader->SetUniformi("workMode", 1);

                m_Meshleter->m_MeshletDescriptorBuffer->BindBase(1);
                m_Meshleter->m_DrawCommandBuffer->BindBase(2);

                unsigned int data = 0;
                m_Meshleter->m_VisibleMeshletBuffer->SubData(
                        0, sizeof(unsigned int), &data);
                m_Meshleter->m_VisibleMeshletBuffer->BindBase(3);

                // need switch to the GL_SHADER_STORAGE_BUFFER target
                m_Meshleter->m_FinalDrawCommandBuffer->Target(
                        GL_SHADER_STORAGE_BUFFER);
                m_Meshleter->m_FinalDrawCommandBuffer->BindBase(4);

                auto cullDataBuffer =
                        scene->m_ShaderManager->GetCullDataBuffer();
                cullDataBuffer->BindBase(5);

                scene->m_HzbTexture->Active(GL_TEXTURE1);
                shader->SetUniformi("hzbSampler", 1);

                size_t count = m_Meshleter->m_MeshletCount;
                glDispatchCompute(static_cast<GLuint>((count + 255) / 256), 1,
                                  1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            }
        }
        /*
        unsigned int count = 0;
        m_Meshleter->m_VisibleMeshletBuffer->GetSubData(0, sizeof(unsigned int),
                                                        &count);
        std::cout << "Test Occlusion: [render count: " << count;
        std::cout << ", meshlet count: " << m_Meshleter->m_MeshletCount << "]"
                  << std::endl;

        std::vector<DrawElementsIndirectCommand> readBackCommands(
                m_Meshleter->m_MeshletCount);
        m_Meshleter->m_DrawCommandBuffer->GetSubData(
                0,
                readBackCommands.size() * sizeof(DrawElementsIndirectCommand),
                readBackCommands.data());
        for (const auto& cmd: readBackCommands) {
            //std::cout << "count: " << cmd.count << std::endl;
            std::cout << "primCount: " << cmd.primCount << std::endl;
            std::cout << "firstIndex: " << cmd.firstIndex << std::endl;
            //std::cout << "baseVertex: " << cmd.baseVertex << std::endl;
            //std::cout << "baseInstance: " << cmd.baseInstance << std::endl;
        }*/
    };

    auto dataObject = m_DataObject;
    auto drawObject = DynamicCast<DrawObject>(dataObject);

    if (drawObject->m_DisplayObject != nullptr) {
        dataObject = DynamicCast<DataObject>(drawObject->m_DisplayObject);
        drawObject = DynamicCast<DrawObject>(dataObject);
    }

    if (!drawObject->m_Visibility) { return; }

    if (!dataObject->HasSubDataObject()) {
        draw(dataObject);
    } else {
        for (auto it = dataObject->SubDataObjectIteratorBegin();
             it != dataObject->SubDataObjectIteratorEnd(); it++) {
            auto subDataObj = it->second;
            auto subDrawObj = DynamicCast<DrawObject>(subDataObj);
            if (subDrawObj->m_DisplayObject == nullptr) {
                draw(subDataObj);
            } else {
                draw(subDrawObj->m_DisplayObject);
            }
        }
    }
#endif
#endif
}

SmartPointer<DataObject> Model::GetDataObject() { return m_DataObject; }

bool Model::GetVisibility() {
    auto drawObject = DynamicCast<DrawObject>(m_DataObject);
    return drawObject->GetVisibility();
}

SmartPointer<Filter> Model::GetModelFilter() { return m_Filter; }

SmartPointer<Painter3D> Model::GetPainter3D() { return m_Painter3D; }

void Model::SetModelFilter(SmartPointer<Filter> filter) { m_Filter = filter; }

void Model::DeleteModelFilter() { m_Filter = nullptr; }

void Model::SetDataObject(SmartPointer<DataObject> dataObject) {
    m_DataObject = dataObject;

#ifdef GL_DEBUG_CULLING
    m_Meshleter = SurfaceMeshMeshleter::New();
    m_Meshleter->SetInput(dataObject);
#endif
}

void Model::Modified() {
    Logger::LogFatal("[Model::Modified] not sure what this function does.");
    m_DataObject->Modified();
}

void Model::Update() {
    if (m_Scene) { m_Scene->Update(); }
}

void Model::ViewCloudPicture(int index, int dimension) {
    auto drawObject = DynamicCast<DrawObject>(m_DataObject);
    if (drawObject != nullptr)
        drawObject->ViewCloudPicture(m_Scene, index, dimension);
}

void Model::SetFilePath(std::string filePath) { m_FilePath = filePath; }

std::string Model::GetFilePath() { return this->m_FilePath; }

SmartPointer<Selection> Model::GetSelection() {
    if (m_Selection == nullptr) { m_Selection = Selection::New(); }
    return m_Selection.get();
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

void Model::SetMeshleter(SmartPointer<Meshleter> meshleter) {
    m_Meshleter = meshleter;
}

void Model::Show() {
    auto drawObject = DynamicCast<DrawObject>(m_DataObject);
    drawObject->SetVisibility(true);
    m_Scene->ChangeModelVisibility(this, true);
}

void Model::Hide() {
    auto drawObject = DynamicCast<DrawObject>(m_DataObject);
    drawObject->SetVisibility(false);
    m_Scene->ChangeModelVisibility(this, false);
}

void Model::SetBoundingBoxSwitch(bool action) {
    auto drawObject = DynamicCast<DrawObject>(m_DataObject);
    if (action) {
        SwitchOn(ViewSwitch::BoundingBox);

        auto& bbox = drawObject->GetBoundingBox();
        Vector3d p1 = bbox.min;
        Vector3d p7 = bbox.max;

        if (m_BboxHandle != 0) { m_Painter3D->Delete(m_BboxHandle); }
        m_Painter3D->SetPen(5);
        m_Painter3D->SetPen(Color::LightBlue);
        m_Painter3D->SetBrush(Brush::Style::NoBrush);
        m_BboxHandle = m_Painter3D->DrawCube(p1, p7);
    } else {
        SwitchOff(ViewSwitch::BoundingBox);
        m_Painter3D->Hide(m_BboxHandle);
    }
}

void Model::SetPickedItemSwitch(bool action) {
    auto drawObject = DynamicCast<DrawObject>(m_DataObject);
    if (action) {
        SwitchOn(ViewSwitch::PickedItem);
        if (drawObject->GetVisibility()) { m_Painter3D->ShowAll(); }
    } else {
        SwitchOff(ViewSwitch::PickedItem);
        m_Painter3D->HideAll();
    }
}

void Model::SetViewPointsSwitch(bool action) {
    auto drawObject = DynamicCast<DrawObject>(m_DataObject);
    if (action) {
        drawObject->AddViewStyle(IG_POINTS);
    } else {
        drawObject->RemoveViewStyle(IG_POINTS);
    }
}

void Model::SetViewWireframeSwitch(bool action) {
    auto drawObject = DynamicCast<DrawObject>(m_DataObject);
    if (action) {
        drawObject->AddViewStyle(IG_WIREFRAME);
    } else {
        drawObject->RemoveViewStyle(IG_WIREFRAME);
    }
}

void Model::SetViewFillSwitch(bool action) {
    auto drawObject = DynamicCast<DrawObject>(m_DataObject);
    if (action) {
        drawObject->AddViewStyle(IG_SURFACE);
    } else {
        drawObject->RemoveViewStyle(IG_SURFACE);
    }
}

void Model::SwitchOn(ViewSwitch type) { m_Switch |= (1ull << type); }

void Model::SwitchOff(ViewSwitch type) { m_Switch &= ~(1ull << type); }

bool Model::GetSwitch(ViewSwitch type) { return m_Switch & (1ull << type); }

IGAME_NAMESPACE_END
