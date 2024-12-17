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

    // need opengl context
    void Initialize();

    void DrawAxes();

    void DrawXYZ(GLShaderProgram::Pointer shader);

    void Update(const igm::mat4& _mvp, const igm::ivec4& viewPort);

    static igm::vec3 CameraPos();
    static igm::mat4 ViewMatrix();
    static igm::mat4 ProjMatrix();

protected:
    Axes();
    ~Axes() override;

    void RequestData(std::vector<igm::vec3>& vertices,
                     std::vector<igm::vec3>& colors);

    void DisplayCoordToWorldCoord(igm::vec4& dc, igm::vec4& wc);

    void WorldCoordToDisplayCoord(igm::vec4& wc, igm::vec4& dc);

    GLVertexArray::Pointer m_TriangleVAO;
    GLBuffer::Pointer m_PositionVBO, m_ColorVBO;
    GLBuffer::Pointer m_TriangleEBO;

    GLVertexArray::Pointer m_FontVAO;
    GLBuffer::Pointer m_TextureCoordVBO, m_WorldCoordVBO, m_FontTextureEBO;

    double Viewport[4];
    igm::mat4 m_Mvp;
    igm::mat4 m_MvpInv;

    float m_ShaftLength;
    float m_ShaftSize;
    float m_ArrowSize;
    float m_OriginSize;
};

IGAME_NAMESPACE_END