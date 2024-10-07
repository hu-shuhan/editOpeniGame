#pragma once

#include "OpenGL/GLBuffer.h"
#include "OpenGL/GLShader.h"
#include "OpenGL/GLTexture2d.h"
#include "OpenGL/GLVertexArray.h"
#include "iGameFontSet.h"

IGAME_NAMESPACE_BEGIN
class Axes : public Object {
public:
    I_OBJECT(Axes);
    static Pointer New() { return new Axes; }

protected:
    Axes();
    ~Axes() override;

public:
    void DrawAxes();
    void DrawXYZ(const GLShaderProgram* shader, const GLUniform texture,
                 const GLUniform color);

    static igm::vec3 CameraPos();
    static igm::mat4 ViewMatrix();
    static igm::mat4 ProjMatrix();

    void Update(const igm::mat4& _mvp, const igm::ivec4& viewPort);

private:
    void initialize();

    void requestData(std::vector<igm::vec3>& vertices,
                     std::vector<igm::vec3>& colors);

    void displayCoordToWorldCoord(igm::vec4& dc, igm::vec4& wc);

    void worldCoordToDisplayCoord(igm::vec4& wc, igm::vec4& dc);

private:
    GLVertexArray m_TriangleVAO;
    GLBuffer m_PositionVBO, m_ColorVBO;
    GLBuffer m_TriangleEBO;

    GLVertexArray m_FontVAO;
    GLBuffer m_TextureCoordVBO, m_WorldCoordVBO, m_FontTextureEBO;
    double Viewport[4];
    igm::mat4 mvp{1.0f};
    igm::mat4 inv_mvp{1.0f};

    const float m_ShaftLength = 1.0f;
    const float m_ShaftSize = 0.03f;
    const float m_ArrowSize = 0.06f;
    const float m_OriginSize = 0.05f;
};

IGAME_NAMESPACE_END