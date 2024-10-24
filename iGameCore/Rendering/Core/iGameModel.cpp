#include "iGameModel.h"
#include "iGameFilter.h"
#include "iGameInteractor.h"
#include "iGameScene.h"

IGAME_NAMESPACE_BEGIN

void Model::Draw(Scene* scene) {
    auto draw = [&](const DataObject::Pointer& dataObject) {
        scene->UpdateObjectDataBlock(dataObject);
        scene->UpdateUniformBufferObjectBlock(dataObject);

        auto drawObject = DynamicCast<DrawObject>(dataObject);
        auto visibility = drawObject->m_Visibility;
        auto useColor = drawObject->m_UseColor;
        auto colorWithCell = drawObject->m_ColorWithCell;
        auto viewStyle = drawObject->m_ViewStyle;

        if (!visibility) { return; }

        if (useColor && colorWithCell) {
            scene->GetShader(Scene::BLINNPHONG)->Use();

            drawObject->m_CellVAO->Bind();
            {
                float f, u;
                drawObject->GetPolygonOffsetParameters(f, u);

                glEnable(GL_POLYGON_OFFSET_FILL);
                glPolygonOffset(f, u);
                glad_glDrawArrays(GL_TRIANGLES, 0,
                                  drawObject->m_CellPositionSize);
                glDisable(GL_POLYGON_OFFSET_FILL);
            }
            drawObject->m_CellVAO->Release();
            return;
        }
        if (viewStyle & IG_POINTS) {
            scene->GetShader(Scene::NOLIGHT)->Use();

            drawObject->m_PointVAO->Bind();
            {
                glad_glPointSize(drawObject->m_PointSize);

                float u;
                drawObject->GetPointOffsetParameters(u);

                glEnable(GL_POLYGON_OFFSET_POINT);
                glPolygonOffset(0.0f, u);
                if (drawObject->m_PointIndices->GetNumberOfValues() == 0) {
                    glad_glDrawArrays(
                            GL_POINTS, 0,
                            drawObject->m_Positions->GetNumberOfElements());
                } else {
                    glad_glDrawElements(
                            GL_POINTS,
                            drawObject->m_PointIndices->GetNumberOfValues(),
                            GL_UNSIGNED_INT, 0);
                }
                glDisable(GL_POLYGON_OFFSET_POINT);
            }
            drawObject->m_PointVAO->Release();
        }
        if (viewStyle & IG_WIREFRAME) {
            if (useColor) {
                scene->GetShader(Scene::NOLIGHT)->Use();
            } else {
                auto shader = scene->GetShader(Scene::PURECOLOR);
                shader->Use();
                shader->SetUniform(shader->GetUniformLocation("inputColor"),
                                   igm::vec3{0.0f, 0.0f, 0.0f});
            }
            drawObject->m_LineVAO->Bind();
            {
                glLineWidth(drawObject->m_LineWidth);

                float f, u;
                drawObject->GetLineOffsetParameters(f, u);

                glEnable(GL_POLYGON_OFFSET_LINE);
                glPolygonOffset(f, u);
                glad_glDrawElements(
                        GL_LINES,
                        drawObject->m_LineIndices->GetNumberOfValues(),
                        GL_UNSIGNED_INT, 0);
                glDisable(GL_POLYGON_OFFSET_LINE);
            }
            drawObject->m_LineVAO->Release();
        }
        if (viewStyle & IG_SURFACE) {
            auto shader = scene->GetShader(Scene::BLINNPHONG);
            shader->Use();

            drawObject->m_TriangleVAO->Bind();
            {
                float f, u;
                drawObject->GetPolygonOffsetParameters(f, u);

                glEnable(GL_POLYGON_OFFSET_FILL);
                glPolygonOffset(f, u);
                glad_glDrawElements(
                        GL_TRIANGLES,
                        drawObject->m_TriangleIndices->GetNumberOfValues(),
                        GL_UNSIGNED_INT, 0);
                glDisable(GL_POLYGON_OFFSET_FILL);
            }
            drawObject->m_TriangleVAO->Release();
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

    m_Painter3D->Draw(scene);
}

void Model::DrawWithTransparency(Scene* scene) {
    auto draw = [&](const DataObject::Pointer& dataObject) {
        scene->UpdateObjectDataBlock(dataObject);
        scene->UpdateUniformBufferObjectBlock(dataObject);

        auto drawObject = DynamicCast<DrawObject>(dataObject);
        auto visibility = drawObject->m_Visibility;
        auto useColor = drawObject->m_UseColor;
        auto colorWithCell = drawObject->m_ColorWithCell;
        auto viewStyle = drawObject->m_ViewStyle;

        if (!visibility) { return; }

        if (useColor && colorWithCell) {
            auto shader = scene->GetShader(Scene::TRANSPARENCYLINK);
            shader->Use();
            shader->SetUniform(shader->GetUniformLocation("colorMode"), 0);

            drawObject->m_CellVAO->Bind();
            {
                float f, u;
                drawObject->GetPolygonOffsetParameters(f, u);

                glEnable(GL_POLYGON_OFFSET_FILL);
                glPolygonOffset(f, u);
                glad_glDrawArrays(GL_TRIANGLES, 0,
                                  drawObject->m_CellPositionSize);
                glDisable(GL_POLYGON_OFFSET_FILL);
            }
            drawObject->m_CellVAO->Release();
            return;
        }

        if (viewStyle & IG_POINTS) {
            auto shader = scene->GetShader(Scene::TRANSPARENCYLINK);
            shader->Use();
            shader->SetUniform(shader->GetUniformLocation("colorMode"), 1);

            drawObject->m_PointVAO->Bind();
            {
                glad_glPointSize(drawObject->m_PointSize);

                float u;
                drawObject->GetPointOffsetParameters(u);

                glEnable(GL_POLYGON_OFFSET_POINT);
                glPolygonOffset(0.0f, u);
                glad_glDrawArrays(
                        GL_POINTS, 0,
                        drawObject->m_Positions->GetNumberOfElements());
                glDisable(GL_POLYGON_OFFSET_POINT);
            }
            drawObject->m_PointVAO->Release();
        }
        if (viewStyle & IG_POINTS) {
            scene->GetShader(Scene::NOLIGHT)->Use();

            drawObject->m_PointVAO->Bind();
            {
                glad_glPointSize(drawObject->m_PointSize);

                float u;
                drawObject->GetPointOffsetParameters(u);

                glEnable(GL_POLYGON_OFFSET_POINT);
                glPolygonOffset(0.0f, u);
                glad_glDrawArrays(
                        GL_POINTS, 0,
                        drawObject->m_Positions->GetNumberOfElements());
                glDisable(GL_POLYGON_OFFSET_POINT);
            }
            drawObject->m_PointVAO->Release();
        }

        if (viewStyle & IG_WIREFRAME) {
            if (useColor) {
                auto shader = scene->GetShader(Scene::TRANSPARENCYLINK);
                shader->Use();
                shader->SetUniform(shader->GetUniformLocation("colorMode"), 1);
            } else {
                auto shader = scene->GetShader(Scene::TRANSPARENCYLINK);
                shader->Use();
                shader->SetUniform(shader->GetUniformLocation("colorMode"), 2);
                shader->SetUniform(shader->GetUniformLocation("inputColor"),
                                   igm::vec3{0.0f, 0.0f, 0.0f});
            }

            drawObject->m_LineVAO->Bind();
            {
                glLineWidth(drawObject->m_LineWidth);

                float f, u;
                drawObject->GetLineOffsetParameters(f, u);

                glEnable(GL_POLYGON_OFFSET_LINE);
                glPolygonOffset(f, u);
                glad_glDrawElements(
                        GL_LINES,
                        drawObject->m_LineIndices->GetNumberOfValues(),
                        GL_UNSIGNED_INT, 0);
                glDisable(GL_POLYGON_OFFSET_LINE);
            }
            drawObject->m_LineVAO->Release();
        }
        if (viewStyle & IG_SURFACE) {
            auto shader = scene->GetShader(Scene::TRANSPARENCYLINK);
            shader->Use();
            shader->SetUniform(shader->GetUniformLocation("colorMode"), 0);

            drawObject->m_TriangleVAO->Bind();
            {
                float f, u;
                drawObject->GetPolygonOffsetParameters(f, u);

                glEnable(GL_POLYGON_OFFSET_FILL);
                glPolygonOffset(f, u);
                glad_glDrawElements(
                        GL_TRIANGLES,
                        drawObject->m_TriangleIndices->GetNumberOfValues(),
                        GL_UNSIGNED_INT, 0);
                glDisable(GL_POLYGON_OFFSET_FILL);
            }
            drawObject->m_TriangleVAO->Release();
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
            draw(it->second);
        }
    }

    m_Painter3D->Draw(scene);
}

void Model::DrawPhase1(Scene* scene) {
#ifdef IGAME_OPENGL_VERSION_460
    // std::cout << "Draw phase 1:" << std::endl;

    auto draw = [&](const DataObject::Pointer& dataObject) {
        scene->UpdateObjectDataBlock(dataObject);
        scene->UpdateUniformBufferObjectBlock(dataObject);

        auto drawObject = DynamicCast<DrawObject>(dataObject);
        auto visibility = drawObject->m_Visibility;
        auto useColor = drawObject->m_UseColor;
        auto colorWithCell = drawObject->m_ColorWithCell;
        auto viewStyle = drawObject->m_ViewStyle;
        auto meshlets = drawObject->m_Meshlets;

        if (!visibility) { return; }

        // draw
        if (useColor && colorWithCell) {}

        if (viewStyle & IG_POINTS) {}
        if (viewStyle & IG_WIREFRAME) {}
        if (viewStyle & IG_SURFACE) {
            scene->GetShader(Scene::BLINNPHONG)->Use();
            drawObject->m_TriangleVAO->Bind();
            unsigned int count = 0;
            meshlets->VisibleMeshletBuffer()->GetSubData(
                    0, sizeof(unsigned int), &count);
            meshlets->FinalDrawCommandBuffer()->Target(GL_DRAW_INDIRECT_BUFFER);
            meshlets->FinalDrawCommandBuffer()->Bind();
            glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr,
                                        count, 0);
            std::cout << "Draw phase 1: [render count: " << count;
            std::cout << ", meshlet count: " << meshlets->MeshletsCount() << "]"
                      << std::endl;
            drawObject->m_TriangleVAO->Release();
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
            draw(it->second);
        }
    }
#endif
}

void Model::DrawPhase2(Scene* scene) {
#ifdef IGAME_OPENGL_VERSION_460
    // std::cout << "Draw phase 2:" << std::endl;

    auto draw = [&](const DataObject::Pointer& dataObject) {
        scene->UpdateObjectDataBlock(dataObject);
        scene->UpdateUniformBufferObjectBlock(dataObject);

        auto drawObject = DynamicCast<DrawObject>(dataObject);
        auto visibility = drawObject->m_Visibility;
        auto useColor = drawObject->m_UseColor;
        auto colorWithCell = drawObject->m_ColorWithCell;
        auto viewStyle = drawObject->m_ViewStyle;
        auto meshlets = drawObject->m_Meshlets;

        if (!visibility) { return; }

        // draw
        if (useColor && colorWithCell) {}

        if (viewStyle & IG_POINTS) {}
        if (viewStyle & IG_WIREFRAME) {}
        if (viewStyle & IG_SURFACE) {
            // compute culling
            {
                auto shader = scene->GetShader(Scene::MESHLETCULL);
                shader->Use();

                GLUniform::Pointer workMode =
                        shader->GetUniformLocation("workMode");
                shader->SetUniform(workMode, 0);

                meshlets->MeshletsBuffer()->Target(GL_SHADER_STORAGE_BUFFER);
                meshlets->MeshletsBuffer()->BindBase(1);

                meshlets->DrawCommandBuffer()->Target(GL_SHADER_STORAGE_BUFFER);
                meshlets->DrawCommandBuffer()->BindBase(2);

                unsigned int data = 0;
                meshlets->VisibleMeshletBuffer()->SubData(
                        0, sizeof(unsigned int), &data);
                meshlets->VisibleMeshletBuffer()->Target(
                        GL_SHADER_STORAGE_BUFFER);
                meshlets->VisibleMeshletBuffer()->BindBase(3);

                meshlets->FinalDrawCommandBuffer()->Target(
                        GL_SHADER_STORAGE_BUFFER);
                meshlets->FinalDrawCommandBuffer()->BindBase(4);

                scene->GetDrawCullDataBuffer()->Target(GL_UNIFORM_BUFFER);
                scene->GetDrawCullDataBuffer()->BindBase(5);

                scene->DepthPyramid()->Active(GL_TEXTURE1);
                shader->SetUniform(shader->GetUniformLocation("depthPyramid"),
                                   1);

                auto count = meshlets->MeshletsCount();
                glDispatchCompute(((count + 255) / 256), 1, 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            }

            scene->GetShader(Scene::BLINNPHONG)->Use();
            drawObject->m_TriangleVAO->Bind();

            unsigned int count = 0;
            meshlets->VisibleMeshletBuffer()->GetSubData(
                    0, sizeof(unsigned int), &count);

            meshlets->FinalDrawCommandBuffer()->Target(GL_DRAW_INDIRECT_BUFFER);
            meshlets->FinalDrawCommandBuffer()->Bind();
            glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr,
                                        count, 0);

            std::cout << "Draw phase 2: [render count: " << count;
            std::cout << ", meshlet count: " << meshlets->MeshletsCount() << "]"
                      << std::endl;

            drawObject->m_TriangleVAO->Release();
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
            draw(it->second);
        }
    }
#endif
}

void Model::TestOcclusionResults(Scene* scene) {
#ifdef IGAME_OPENGL_VERSION_460
    // std::cout << "Test Occlusion:" << std::endl;

    auto draw = [&](const DataObject::Pointer& dataObject) {
        auto drawObject = DynamicCast<DrawObject>(dataObject);
        auto visibility = drawObject->m_Visibility;
        auto useColor = drawObject->m_UseColor;
        auto colorWithCell = drawObject->m_ColorWithCell;
        auto viewStyle = drawObject->m_ViewStyle;
        auto meshlets = drawObject->m_Meshlets;

        if (!visibility) { return; }

        if (useColor && colorWithCell) {}

        if (viewStyle & IG_POINTS) {}
        if (viewStyle & IG_WIREFRAME) {}
        if (viewStyle & IG_SURFACE) {
            // compute culling
            {
                auto shader = scene->GetShader(Scene::MESHLETCULL);
                shader->Use();

                GLUniform::Pointer workMode =
                        shader->GetUniformLocation("workMode");
                shader->SetUniform(workMode, 1);

                meshlets->MeshletsBuffer()->Target(GL_SHADER_STORAGE_BUFFER);
                meshlets->MeshletsBuffer()->BindBase(1);

                meshlets->DrawCommandBuffer()->Target(GL_SHADER_STORAGE_BUFFER);
                meshlets->DrawCommandBuffer()->BindBase(2);

                unsigned int data = 0;
                meshlets->VisibleMeshletBuffer()->SubData(
                        0, sizeof(unsigned int), &data);
                meshlets->VisibleMeshletBuffer()->Target(
                        GL_SHADER_STORAGE_BUFFER);
                meshlets->VisibleMeshletBuffer()->BindBase(3);

                meshlets->FinalDrawCommandBuffer()->Target(
                        GL_SHADER_STORAGE_BUFFER);
                meshlets->FinalDrawCommandBuffer()->BindBase(4);

                scene->GetDrawCullDataBuffer()->Target(GL_UNIFORM_BUFFER);
                scene->GetDrawCullDataBuffer()->BindBase(5);

                scene->DepthPyramid()->Active(GL_TEXTURE1);
                shader->SetUniform(shader->GetUniformLocation("depthPyramid"),
                                   1);

                auto count = meshlets->MeshletsCount();
                glDispatchCompute(((count + 255) / 256), 1, 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            }
        }

        unsigned int count = 0;
        meshlets->VisibleMeshletBuffer()->GetSubData(0, sizeof(unsigned int),
                                                     &count);
        // std::cout << "Test Occlusion: [render count: " << count;
        // std::cout << ", meshlet count: " << meshlets->MeshletsCount() << "]"
        //           << std::endl;

        // std::vector<DrawElementsIndirectCommand> readBackCommands(
        //         meshlets->MeshletsCount());
        // meshlets->DrawCommandBuffer().GetSubData(
        //         0, readBackCommands.size() * sizeof(DrawElementsIndirectCommand),
        //         readBackCommands.data());
        // for (const auto& cmd: readBackCommands) {
        //     //std::cout << "count: " << cmd.count << std::endl;
        //     std::cout << "primCount: " << cmd.primCount << std::endl;
        //     //std::cout << "firstIndex: " << cmd.firstIndex << std::endl;
        //     //std::cout << "baseVertex: " << cmd.baseVertex << std::endl;
        //     //std::cout << "baseInstance: " << cmd.baseInstance << std::endl;
        // }
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
            draw(it->second);
        }
    }
#endif
}

void Model::Update() {
    if (m_Scene) { m_Scene->Update(); }
}

Selection* Model::GetSelection() {
    if (m_Selection == nullptr) { m_Selection = Selection::New(); }
    return m_Selection.get();
}

void Model::RequestPointSelection(Points* p, Selection* s) {
    if (m_Scene->GetInteractor() == nullptr) return;
    s->m_Points = p;
    s->m_Model = this;
    m_Scene->GetInteractor()->RequestPointSelectionStyle(s);
}

void Model::RequestDragPoint(Points* p, Selection* s) {
    if (m_Scene->GetInteractor() == nullptr) return;
    s->m_Points = p;
    s->m_Model = this;
    m_Scene->GetInteractor()->RequestDragPointStyle(s);
}

Filter* Model::GetModelFilter() { return m_Filter; }
void Model::DeleteModelFilter() { m_Filter = nullptr; }
void Model::SetModelFilter(SmartPointer<Filter> _filter) { m_Filter = _filter; }

void Model::Show() {
    auto drawObject = DynamicCast<DrawObject>(m_DataObject);
    drawObject->SetVisibility(true);
    m_Scene->ChangeModelVisibility(this, true);
    //m_Painter3D->ShowAll();
}

void Model::Hide() {
    auto drawObject = DynamicCast<DrawObject>(m_DataObject);
    drawObject->SetVisibility(false);
    m_Scene->ChangeModelVisibility(this, false);
    //m_Painter3D->HideAll();
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
        m_Painter3D->SetBrush(Color::None);
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

Model::Model() {
    SwitchOff(ViewSwitch::BoundingBox);
    SwitchOn(ViewSwitch::PickedItem);
}
IGAME_NAMESPACE_END
