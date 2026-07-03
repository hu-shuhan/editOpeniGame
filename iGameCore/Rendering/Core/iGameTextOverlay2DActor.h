#pragma once

#include "OpenGL/GLBuffer.h"
#include "OpenGL/GLVertexArray.h"
#include "iGameObject.h"
#include <string>

IGAME_NAMESPACE_BEGIN

class Scene;

class TextOverlay2DActor : public Object {
public:
    I_OBJECT(TextOverlay2DActor);
    static Pointer New() { return new TextOverlay2DActor; }

    void SetScene(SmartPointer<Scene> scene);
    void Initialize();
    void Draw();

    void DrawText(const std::string& text, float x, float baselineY,
                  float scale, const igm::vec3& color, float viewportWidth,
                  float viewportHeight);
    float ComputeCenteredTextX(const std::string& text, float centerX,
                               float scale) const;

    void SetText(const std::string& text);
    const std::string& GetText() const;
    void SetVisible(bool visible);
    bool GetVisible() const;
    void SetPosition(float left, float top);
    void SetScale(float scale);
    void SetColor(const igm::vec3& color);
    void SetAnchorToBottomRight(bool anchorToBottomRight);
    bool GetAnchorToBottomRight() const;

protected:
    TextOverlay2DActor();
    ~TextOverlay2DActor() override;

private:
    SmartPointer<Scene> m_Scene;
    bool m_Initialized{false};
    bool m_Visible{true};
    std::string m_Text{"iGameVis WASM"};
    igm::vec2 m_Position{12.0f, 10.0f};
    igm::vec3 m_Color{1.0f, 1.0f, 1.0f};
    float m_Scale{0.13f};

    SmartPointer<GLVertexArray> m_VAO;
    SmartPointer<GLBuffer> m_PositionVBO;
    SmartPointer<GLBuffer> m_UVVBO;
    SmartPointer<GLBuffer> m_EBO;
    bool m_AnchorToBottomRight{true};

    float ComputeTextWidth(const std::wstring& characters, float scale) const;
};

IGAME_NAMESPACE_END
