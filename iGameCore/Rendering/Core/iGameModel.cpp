#include "iGameModel.h"
#include "iGameFilter.h"
#include "iGameInteractor.h"
#include "iGameScene.h"

IGAME_NAMESPACE_BEGIN

void Model::Draw(Scene* scene) {
    auto drawObject = DynamicCast<DrawObject>(m_DataObject);
    drawObject->Draw(scene);
    m_Painter->Draw(scene);
}


void Model::DrawWithTransparency(Scene* scene) {
    auto drawObject = DynamicCast<DrawObject>(m_DataObject);
    auto visibility = drawObject->m_Visibility;
    auto useColor = drawObject->m_UseColor;
    auto colorWithCell = drawObject->m_ColorWithCell;
    auto viewStyle = drawObject->m_ViewStyle;

    if (!visibility) { return; }

    if (useColor && colorWithCell) {
        auto shader = scene->GetShader(Scene::TRANSPARENCYLINK);
        shader->use();
        shader->setUniform(shader->getUniformLocation("colorMode"), 0);

        drawObject->m_CellVAO.bind();
        glad_glDrawArrays(GL_TRIANGLES, 0, drawObject->m_CellPositionSize);
        drawObject->m_CellVAO.release();
        return;
    }

    if (viewStyle & IG_POINTS) {
        auto shader = scene->GetShader(Scene::TRANSPARENCYLINK);
        shader->use();
        shader->setUniform(shader->getUniformLocation("colorMode"), 1);

        drawObject->m_PointVAO.bind();
        glad_glPointSize(8);
        glad_glDepthRange(0.000001, 1);
        glad_glDrawArrays(GL_POINTS, 0,
                          drawObject->m_Positions->GetNumberOfValues() / 3);
        glad_glDepthRange(0, 1);
        drawObject->m_PointVAO.release();
    }

    if (viewStyle & IG_WIREFRAME) {
        if (useColor) {
            auto shader = scene->GetShader(Scene::TRANSPARENCYLINK);
            shader->use();
            shader->setUniform(shader->getUniformLocation("colorMode"), 1);
        } else {
            auto shader = scene->GetShader(Scene::TRANSPARENCYLINK);
            shader->use();
            shader->setUniform(shader->getUniformLocation("colorMode"), 2);
            shader->setUniform(shader->getUniformLocation("inputColor"),
                               igm::vec3{0.0f, 0.0f, 0.0f});
        }

        drawObject->m_LineVAO.bind();
        glLineWidth(drawObject->m_LineWidth);
        glad_glDrawElements(GL_LINES,
                            drawObject->m_LineIndices->GetNumberOfIds(),
                            GL_UNSIGNED_INT, 0);
        drawObject->m_LineVAO.release();
    }

    if (viewStyle & IG_SURFACE) {
        auto shader = scene->GetShader(Scene::TRANSPARENCYLINK);
        shader->use();
        shader->setUniform(shader->getUniformLocation("colorMode"), 0);

        drawObject->m_TriangleVAO.bind();
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-0.5f, -0.5f);
        glad_glDrawElements(GL_TRIANGLES,
                            drawObject->m_TriangleIndices->GetNumberOfIds(),
                            GL_UNSIGNED_INT, 0);
        glDisable(GL_POLYGON_OFFSET_FILL);
        drawObject->m_TriangleVAO.release();
    }

    m_Painter->Draw(scene);
}

void Model::DrawPhase1(Scene* scene) {
#ifdef IGAME_OPENGL_VERSION_460
    // std::cout << "Draw phase 1:" << std::endl;

    auto drawObject = DynamicCast<DrawObject>(m_DataObject);
    auto visibility = drawObject->m_Visibility;
    auto useColor = drawObject->m_UseColor;
    auto colorWithCell = drawObject->m_ColorWithCell;
    auto viewStyle = drawObject->m_ViewStyle;
    auto meshlets = drawObject->m_Meshlets;

    if (!visibility) { return; }

    // update uniform buffer
    {
        if (useColor) {
            scene->UBO().useColor = true;
        } else {
            scene->UBO().useColor = false;
        }
        scene->UpdateUniformBuffer();
    }

    // draw
    if (useColor && colorWithCell) {}

    if (viewStyle & IG_POINTS) {}
    if (viewStyle & IG_WIREFRAME) {}
    if (viewStyle & IG_SURFACE) {
        scene->GetShader(Scene::BLINNPHONG)->use();
        drawObject->m_TriangleVAO.bind();
        unsigned int count = 0;
        meshlets->VisibleMeshletBuffer().getSubData(0, sizeof(unsigned int),
                                                    &count);
        meshlets->FinalDrawCommandBuffer().target(GL_DRAW_INDIRECT_BUFFER);
        meshlets->FinalDrawCommandBuffer().bind();
        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr,
                                    count, 0);
        std::cout << "Draw phase 1: [render count: " << count;
        std::cout << ", meshlet count: " << meshlets->MeshletsCount() << "]"
                  << std::endl;
        drawObject->m_TriangleVAO.release();
    }
#endif
}

void Model::DrawPhase2(Scene* scene) {
#ifdef IGAME_OPENGL_VERSION_460
    // std::cout << "Draw phase 2:" << std::endl;
    auto drawObject = DynamicCast<DrawObject>(m_DataObject);
    auto visibility = drawObject->m_Visibility;
    auto useColor = drawObject->m_UseColor;
    auto colorWithCell = drawObject->m_ColorWithCell;
    auto viewStyle = drawObject->m_ViewStyle;
    auto meshlets = drawObject->m_Meshlets;

    if (!visibility) { return; }

    // update uniform buffer
    {
        if (useColor) {
            scene->UBO().useColor = true;
        } else {
            scene->UBO().useColor = false;
        }
        scene->UpdateUniformBuffer();
    }

    // draw
    if (useColor && colorWithCell) {}

    if (viewStyle & IG_POINTS) {}
    if (viewStyle & IG_WIREFRAME) {}
    if (viewStyle & IG_SURFACE) {
        // compute culling
        {
            auto shader = scene->GetShader(Scene::MESHLETCULL);
            shader->use();

            GLUniform workMode = shader->getUniformLocation("workMode");
            shader->setUniform(workMode, 0);

            meshlets->MeshletsBuffer().target(GL_SHADER_STORAGE_BUFFER);
            meshlets->MeshletsBuffer().bindBase(1);

            meshlets->DrawCommandBuffer().target(GL_SHADER_STORAGE_BUFFER);
            meshlets->DrawCommandBuffer().bindBase(2);

            unsigned int data = 0;
            meshlets->VisibleMeshletBuffer().subData(0, sizeof(unsigned int),
                                                     &data);
            meshlets->VisibleMeshletBuffer().target(GL_SHADER_STORAGE_BUFFER);
            meshlets->VisibleMeshletBuffer().bindBase(3);

            meshlets->FinalDrawCommandBuffer().target(GL_SHADER_STORAGE_BUFFER);
            meshlets->FinalDrawCommandBuffer().bindBase(4);

            scene->GetDrawCullDataBuffer().target(GL_UNIFORM_BUFFER);
            scene->GetDrawCullDataBuffer().bindBase(5);

            scene->DepthPyramid().active(GL_TEXTURE1);
            shader->setUniform(shader->getUniformLocation("depthPyramid"), 1);

            auto count = meshlets->MeshletsCount();
            glDispatchCompute(((count + 255) / 256), 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }

        scene->GetShader(Scene::BLINNPHONG)->use();
        drawObject->m_TriangleVAO.bind();

        unsigned int count = 0;
        meshlets->VisibleMeshletBuffer().getSubData(0, sizeof(unsigned int),
                                                    &count);

        meshlets->FinalDrawCommandBuffer().target(GL_DRAW_INDIRECT_BUFFER);
        meshlets->FinalDrawCommandBuffer().bind();
        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr,
                                    count, 0);

        std::cout << "Draw phase 2: [render count: " << count;
        std::cout << ", meshlet count: " << meshlets->MeshletsCount() << "]"
                  << std::endl;

        drawObject->m_TriangleVAO.release();
    }

#endif
}

void Model::TestOcclusionResults(Scene* scene) {
#ifdef IGAME_OPENGL_VERSION_460
    // std::cout << "Test Occlusion:" << std::endl;
    auto drawObject = DynamicCast<DrawObject>(m_DataObject);
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
            shader->use();

            GLUniform workMode = shader->getUniformLocation("workMode");
            shader->setUniform(workMode, 1);

            meshlets->MeshletsBuffer().target(GL_SHADER_STORAGE_BUFFER);
            meshlets->MeshletsBuffer().bindBase(1);

            meshlets->DrawCommandBuffer().target(GL_SHADER_STORAGE_BUFFER);
            meshlets->DrawCommandBuffer().bindBase(2);

            unsigned int data = 0;
            meshlets->VisibleMeshletBuffer().subData(0, sizeof(unsigned int),
                                                     &data);
            meshlets->VisibleMeshletBuffer().target(GL_SHADER_STORAGE_BUFFER);
            meshlets->VisibleMeshletBuffer().bindBase(3);

            meshlets->FinalDrawCommandBuffer().target(GL_SHADER_STORAGE_BUFFER);
            meshlets->FinalDrawCommandBuffer().bindBase(4);

            scene->GetDrawCullDataBuffer().target(GL_UNIFORM_BUFFER);
            scene->GetDrawCullDataBuffer().bindBase(5);

            scene->DepthPyramid().active(GL_TEXTURE1);
            shader->setUniform(shader->getUniformLocation("depthPyramid"), 1);

            auto count = meshlets->MeshletsCount();
            glDispatchCompute(((count + 255) / 256), 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }
    }

    unsigned int count = 0;
    meshlets->VisibleMeshletBuffer().getSubData(0, sizeof(unsigned int),
                                                &count);
    // std::cout << "Test Occlusion: [render count: " << count;
    // std::cout << ", meshlet count: " << meshlets->MeshletsCount() << "]"
    //           << std::endl;

    // std::vector<DrawElementsIndirectCommand> readBackCommands(
    //         meshlets->MeshletsCount());
    // meshlets->DrawCommandBuffer().getSubData(
    //         0, readBackCommands.size() * sizeof(DrawElementsIndirectCommand),
    //         readBackCommands.data());
    // for (const auto& cmd: readBackCommands) {
    //     //std::cout << "count: " << cmd.count << std::endl;
    //     std::cout << "primCount: " << cmd.primCount << std::endl;
    //     //std::cout << "firstIndex: " << cmd.firstIndex << std::endl;
    //     //std::cout << "baseVertex: " << cmd.baseVertex << std::endl;
    //     //std::cout << "baseInstance: " << cmd.baseInstance << std::endl;
    // }
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
    m_Painter->ShowAll();
}

void Model::Hide() {
    auto drawObject = DynamicCast<DrawObject>(m_DataObject);
    drawObject->SetVisibility(false);
    m_Scene->ChangeModelVisibility(this, false);
    m_Painter->HideAll();
}

void Model::SetBoundingBoxSwitch(bool action) {
    auto drawObject = DynamicCast<DrawObject>(m_DataObject);
    if (action) {
        SwitchOn(ViewSwitch::BoundingBox);

        auto& bbox = drawObject->GetBoundingBox();
        Vector3d p1 = bbox.min;
        Vector3d p7 = bbox.max;

        if (m_BboxHandle != 0) { m_Painter->Delete(m_BboxHandle); }
        m_Painter->SetPen(5);
        m_Painter->SetPen(Color::LightBlue);
        m_Painter->SetBrush(Color::None);
        m_BboxHandle = m_Painter->DrawCube(p1, p7);
    } else {
        SwitchOff(ViewSwitch::BoundingBox);
        m_Painter->Hide(m_BboxHandle);
    }
}

void Model::SetPickedItemSwitch(bool action) {
    auto drawObject = DynamicCast<DrawObject>(m_DataObject);
    if (action) {
        SwitchOn(ViewSwitch::PickedItem);
        if (drawObject->GetVisibility()) { m_Painter->ShowAll(); }
    } else {
        SwitchOff(ViewSwitch::PickedItem);
        m_Painter->HideAll();
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
    m_Painter = Painter::New();

    SwitchOff(ViewSwitch::BoundingBox);
    SwitchOn(ViewSwitch::PickedItem);
}
IGAME_NAMESPACE_END
