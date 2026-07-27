#pragma once

#include "OpenGL/GLBuffer.h"
#include "OpenGL/GLVertexArray.h"
#include "iGameObject.h"
#include "iGameScalarsToColors.h"
#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

class Scene;

class ColorBar2DActor : public Object {
public:
    I_OBJECT(ColorBar2DActor);
    static Pointer New() { return new ColorBar2DActor; }

    enum class CoordinateMode { NormalizedViewport = 0, Pixel = 1 };
    enum class Orientation { Vertical = 0, Horizontal = 1 };

    void SetScene(SmartPointer<Scene> scene);
    SmartPointer<Scene> GetScene() const;

    void Initialize();
    void Draw();

    void SetColorMapper(ScalarsToColors::Pointer mapper);
    ScalarsToColors::Pointer GetColorMapper() const;

    void SetVisible(bool visible);
    bool GetVisible() const;

    void SetCoordinateMode(CoordinateMode mode);
    CoordinateMode GetCoordinateMode() const;

    void SetPosition(float x, float y);
    void SetSize(float width, float height);
    void SetOrientation(Orientation orientation);
    Orientation GetOrientation() const;

    void SetNumberOfLabels(int numberOfLabels);
    int GetNumberOfLabels() const;

    void SetMaximumNumberOfColors(int maximumNumberOfColors);
    int GetMaximumNumberOfColors() const;

    void SetTitle(const std::string& title);
    const std::string& GetTitle() const;

protected:
    ColorBar2DActor();
    ~ColorBar2DActor() override;

private:
    struct Layout {
        float x{0.0f};
        float y{0.0f};
        float width{0.0f};
        float height{0.0f};
    };

    ScalarsToColors::Pointer ResolveColorMapper() const;
    Layout ResolveLayout(float viewportWidth, float viewportHeight) const;
    void RebuildGeometry(const Layout& layout, ScalarsToColors::Pointer mapper);
    void DrawGeometry(float viewportWidth, float viewportHeight);
    void DrawLabels(const Layout& layout, ScalarsToColors::Pointer mapper,
                    float viewportWidth, float viewportHeight);
    void DrawText(const std::string& text, float x, float y, float scale,
                  const igm::vec3& color, float viewportWidth,
                  float viewportHeight);
    std::string FormatNumber(double value) const;

    SmartPointer<Scene> m_Scene;
    ScalarsToColors::Pointer m_ColorMapper;

    bool m_Initialized{false};
    bool m_Visible{false};
    CoordinateMode m_CoordinateMode{CoordinateMode::NormalizedViewport};
    Orientation m_Orientation{Orientation::Horizontal};
    igm::vec2 m_Position{0.18f, 0.10f};
    igm::vec2 m_Size{0.62f, 0.05f};
    int m_NumberOfLabels{3};
    int m_MaximumNumberOfColors{128};
    std::string m_Title;

    unsigned int m_LastMapperMTime{0};
    unsigned int m_LastActorMTime{0};
    Layout m_LastLayout;

    SmartPointer<GLVertexArray> m_VAO;
    SmartPointer<GLBuffer> m_PositionVBO;
    SmartPointer<GLBuffer> m_ColorVBO;
    SmartPointer<GLBuffer> m_LineEBO;
    SmartPointer<GLBuffer> m_TriangleEBO;
    IGsize m_LineIndexCount{0};
    IGsize m_TriangleIndexCount{0};
};

IGAME_NAMESPACE_END
