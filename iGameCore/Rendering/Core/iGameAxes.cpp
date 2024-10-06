#include "iGameAxes.h"

IGAME_NAMESPACE_BEGIN

Axes::Axes() {
    m_TriangleVAO.create();
    m_PositionVBO.create();
    m_PositionVBO.target(GL_ARRAY_BUFFER);
    m_ColorVBO.create();
    m_ColorVBO.target(GL_ARRAY_BUFFER);
    m_TriangleEBO.create();
    m_TriangleEBO.target(GL_ELEMENT_ARRAY_BUFFER);

    m_FontVAO.create();
    m_TextureCoordVBO.create();
    m_TextureCoordVBO.target(GL_ARRAY_BUFFER);
    m_WorldCoordVBO.create();
    m_WorldCoordVBO.target(GL_ARRAY_BUFFER);
    m_FontTextureEBO.create();
    m_FontTextureEBO.target(GL_ELEMENT_ARRAY_BUFFER);

    initialize();
}

Axes::~Axes() {
    m_TriangleVAO.destroy();
    m_PositionVBO.destroy();
    m_ColorVBO.destroy();
    m_TriangleEBO.destroy();

    m_FontVAO.destroy();
    m_TextureCoordVBO.destroy();
    m_WorldCoordVBO.destroy();
    m_FontTextureEBO.destroy();
};

void Axes::DrawAxes() {
    // draw axes
    m_TriangleVAO.bind();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDrawElements(GL_TRIANGLES, 207, GL_UNSIGNED_INT, 0);
    m_TriangleVAO.release();
}
void Axes::DrawXYZ(const GLShaderProgram* shader, const GLUniform texture,
                   const GLUniform color) {
    // draw xyz
    m_FontVAO.bind();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    igm::vec3 red = igm::vec3{1.0f, 0.0f, 0.0f};
    igm::vec3 green = igm::vec3{0.0f, 1.0f, 0.0f};
    igm::vec3 blue = igm::vec3{0.0f, 0.0f, 1.0f};

    auto& textureX = FontSet::Instance().GetTexture(L'X');
    textureX.active(GL_TEXTURE1);
    auto& textureY = FontSet::Instance().GetTexture(L'Y');
    textureY.active(GL_TEXTURE2);
    auto& textureZ = FontSet::Instance().GetTexture(L'Z');
    textureZ.active(GL_TEXTURE3);

    shader->setUniform(texture, 1);
    shader->setUniform(color, red);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    shader->setUniform(texture, 2);
    shader->setUniform(color, green);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT,
                   (void*) (6 * sizeof(GLuint)));

    shader->setUniform(texture, 3);
    shader->setUniform(color, blue);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT,
                   (void*) (12 * sizeof(GLuint)));

    m_FontVAO.release();
}

void Axes::Update(const igm::mat4& _mvp, const igm::ivec4& viewPort) {
    mvp = _mvp;
    inv_mvp = _mvp.invert();

    Viewport[0] = viewPort[0];
    Viewport[1] = viewPort[1];
    Viewport[2] = viewPort[2];
    Viewport[3] = viewPort[3];
    /*  Vertices sequence.
             *  2---3
             *  |\  |
             *  | \ |
             *  |  \|
             *  0---1
             * */

    std::vector<float> fontVertices;

    std::vector<igm::vec4> texts = {igm::vec4{1.1, 0.1, -0.1, 1},
                                    igm::vec4{0.0, 1.3, 0.0, 1},
                                    igm::vec4{0.1, 0.0, 1.1, 1}};

    fontVertices.reserve(36);
    for (auto& text: texts) {
        igm::vec4 dc;
        worldCoordToDisplayCoord(text, dc);

        igm::vec4 tmpWc;

        //  first point
        displayCoordToWorldCoord(dc, tmpWc);
        fontVertices.push_back(tmpWc.x), fontVertices.push_back(tmpWc.y),
                fontVertices.push_back(tmpWc.z);

        // second point
        dc.x = dc.x + 20;
        displayCoordToWorldCoord(dc, tmpWc);
        fontVertices.push_back(tmpWc.x), fontVertices.push_back(tmpWc.y),
                fontVertices.push_back(tmpWc.z);

        // third point
        dc.x = dc.x - 20;
        dc.y = dc.y + 20;
        displayCoordToWorldCoord(dc, tmpWc);
        fontVertices.push_back(tmpWc.x), fontVertices.push_back(tmpWc.y),
                fontVertices.push_back(tmpWc.z);

        // fourth point
        dc.x = dc.x + 20;
        displayCoordToWorldCoord(dc, tmpWc);
        fontVertices.push_back(tmpWc.x), fontVertices.push_back(tmpWc.y),
                fontVertices.push_back(tmpWc.z);
    }

    m_WorldCoordVBO.allocate(fontVertices.size() * sizeof(float),
                             fontVertices.data(), GL_STATIC_DRAW);
    m_FontVAO.vertexBuffer(0, m_WorldCoordVBO, 0, 3 * sizeof(float));
    GLSetVertexAttrib(m_FontVAO, GL_LOCATION_IDX_0, 0, 3, GL_FLOAT, GL_FALSE,
                      0);
}

igm::vec3 Axes::CameraPos() { return igm::vec3{0.0f, 0.0f, 3.5f}; }
igm::mat4 Axes::ViewMatrix() {
    return igm::lookAtRH(CameraPos(), igm::vec3{0.0f, 0.0f, 0.0f},
                         igm::vec3{0.0f, 1.0f, 0.0f});
}
igm::mat4 Axes::ProjMatrix() {
    // reversed-z buffer
    return igm::perspectiveRH_OZ(45.0f, 1.0f, 0.01f);
}

void Axes::initialize() {
    // generate axis VBO data
    std::vector<igm::vec3> vertices;
    std::vector<igm::vec3> colors;
    std::vector<uint32_t> triangleIndices{
            8,  9,  12, 9,  10, 12, 10, 11, 12, 11, 8,  12, 21, 22, 25, 22, 23,
            25, 23, 24, 25, 24, 21, 25, 34, 35, 38, 35, 36, 38, 36, 37, 38, 37,
            34, 38, 0,  1,  5,  0,  5,  4,  1,  2,  6,  1,  6,  5,  2,  3,  7,
            2,  7,  6,  3,  0,  4,  3,  4,  7,  8,  9,  10, 8,  10, 11, 13, 14,
            18, 13, 18, 17, 14, 15, 19, 14, 19, 18, 15, 16, 20, 15, 20, 19, 16,
            13, 17, 16, 17, 20, 21, 22, 23, 21, 23, 24, 26, 27, 31, 26, 31, 30,
            27, 28, 32, 27, 32, 31, 28, 29, 33, 28, 33, 32, 29, 26, 30, 29, 30,
            33, 34, 35, 36, 34, 36, 37, 39, 40, 41, 39, 41, 42, 41, 42, 46, 41,
            46, 45, 45, 46, 43, 45, 43, 44, 43, 44, 40, 43, 40, 39, 39, 42, 46,
            39, 46, 43, 40, 41, 45, 40, 45, 44,
    };
    requestData(vertices, colors);
    m_PositionVBO.allocate(vertices.size() * sizeof(igm::vec3), vertices.data(),
                           GL_STATIC_DRAW);
    m_ColorVBO.allocate(colors.size() * sizeof(igm::vec3), colors.data(),
                        GL_STATIC_DRAW);
    m_TriangleEBO.allocate(triangleIndices.size() * sizeof(uint32_t),
                           triangleIndices.data(), GL_STATIC_DRAW);

    // bind vertex attribute pointer to VAO
    m_TriangleVAO.vertexBuffer(GL_VBO_IDX_0, m_PositionVBO, 0,
                               3 * sizeof(float));
    GLSetVertexAttrib(m_TriangleVAO, GL_LOCATION_IDX_0, GL_VBO_IDX_0, 3,
                      GL_FLOAT, GL_FALSE, 0);
    m_TriangleVAO.vertexBuffer(GL_VBO_IDX_1, m_ColorVBO, 0, 3 * sizeof(float));
    GLSetVertexAttrib(m_TriangleVAO, GL_LOCATION_IDX_1, GL_VBO_IDX_1, 3,
                      GL_FLOAT, GL_FALSE, 0);
    m_TriangleVAO.elementBuffer(m_TriangleEBO);

    // billboard
    Viewport[0] = 0;
    Viewport[1] = 0;
    Viewport[2] = 200;
    Viewport[3] = 200;

    std::vector<float> fontVertices{0, 0,    0, 0.24, 0,    0,
                                    0, 0.24, 0, 0.24, 0.24, 0};
    std::vector<float> textureCoords{
            0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1,
            0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0,
    };
    std::vector<uint32_t> fontIndices{0, 1, 2, 2, 1, 3,  4,  5, 6,
                                      6, 5, 7, 8, 9, 10, 10, 9, 11};

    // Bind Axes font texture VAO, VBO, EBO
    m_WorldCoordVBO.allocate(fontVertices.size() * sizeof(float),
                             fontVertices.data(), GL_STATIC_DRAW);
    m_TextureCoordVBO.allocate(textureCoords.size() * sizeof(float),
                               textureCoords.data(), GL_STATIC_DRAW);
    m_FontTextureEBO.allocate(fontIndices.size() * sizeof(uint32_t),
                              fontIndices.data(), GL_STATIC_DRAW);

    m_FontVAO.vertexBuffer(GL_VBO_IDX_0, m_WorldCoordVBO, 0, 3 * sizeof(float));
    GLSetVertexAttrib(m_FontVAO, GL_LOCATION_IDX_0, GL_VBO_IDX_0, 3, GL_FLOAT,
                      GL_FALSE, 0);
    m_FontVAO.vertexBuffer(GL_VBO_IDX_1, m_TextureCoordVBO, 0,
                           2 * sizeof(float));
    GLSetVertexAttrib(m_FontVAO, GL_LOCATION_IDX_3, GL_VBO_IDX_1, 2, GL_FLOAT,
                      GL_FALSE, 0);
    m_FontVAO.elementBuffer(m_FontTextureEBO);
}

void Axes::requestData(std::vector<igm::vec3>& vertices,
                       std::vector<igm::vec3>& colors) {
    vertices.resize(47);
    colors.resize(47);
    // x-axis
    vertices[0] = igm::vec3{0.0f, -m_ShaftSize, m_ShaftSize};
    vertices[1] = igm::vec3{0.0f, -m_ShaftSize, -m_ShaftSize};
    vertices[2] = igm::vec3{0.0f, m_ShaftSize, -m_ShaftSize};
    vertices[3] = igm::vec3{0.0f, m_ShaftSize, m_ShaftSize};
    vertices[4] = igm::vec3{m_ShaftLength, -m_ShaftSize, m_ShaftSize};
    vertices[5] = igm::vec3{m_ShaftLength, -m_ShaftSize, -m_ShaftSize};
    vertices[6] = igm::vec3{m_ShaftLength, m_ShaftSize, -m_ShaftSize};
    vertices[7] = igm::vec3{m_ShaftLength, m_ShaftSize, m_ShaftSize};
    vertices[8] = igm::vec3{m_ShaftLength, -m_ArrowSize, m_ArrowSize};
    vertices[9] = igm::vec3{m_ShaftLength, -m_ArrowSize, -m_ArrowSize};
    vertices[10] = igm::vec3{m_ShaftLength, m_ArrowSize, -m_ArrowSize};
    vertices[11] = igm::vec3{m_ShaftLength, m_ArrowSize, m_ArrowSize};
    vertices[12] = igm::vec3{m_ShaftLength + m_ArrowSize * 2, 0.0f, 0.0f};
    for (int i = 0; i <= 12; ++i) { colors[i] = igm::vec3{1.0f, 0.0f, 0.0f}; }
    // y-axis
    vertices[13] = igm::vec3{-m_ShaftSize, 0.0f, m_ShaftSize};
    vertices[14] = igm::vec3{m_ShaftSize, 0.0f, m_ShaftSize};
    vertices[15] = igm::vec3{m_ShaftSize, 0.0f, -m_ShaftSize};
    vertices[16] = igm::vec3{-m_ShaftSize, 0.0f, -m_ShaftSize};
    vertices[17] = igm::vec3{-m_ShaftSize, m_ShaftLength, m_ShaftSize};
    vertices[18] = igm::vec3{m_ShaftSize, m_ShaftLength, m_ShaftSize};
    vertices[19] = igm::vec3{m_ShaftSize, m_ShaftLength, -m_ShaftSize};
    vertices[20] = igm::vec3{-m_ShaftSize, m_ShaftLength, -m_ShaftSize};
    vertices[21] = igm::vec3{-m_ArrowSize, m_ShaftLength, m_ArrowSize};
    vertices[22] = igm::vec3{m_ArrowSize, m_ShaftLength, m_ArrowSize};
    vertices[23] = igm::vec3{m_ArrowSize, m_ShaftLength, -m_ArrowSize};
    vertices[24] = igm::vec3{-m_ArrowSize, m_ShaftLength, -m_ArrowSize};
    vertices[25] = igm::vec3{0.0f, m_ShaftLength + m_ArrowSize * 2, 0.0f};
    for (int i = 13; i <= 25; ++i) { colors[i] = igm::vec3{0.0f, 1.0f, 0.0f}; }
    // z-axis
    vertices[26] = igm::vec3{-m_ShaftSize, m_ShaftSize, 0.0f};
    vertices[27] = igm::vec3{m_ShaftSize, m_ShaftSize, 0.0f};
    vertices[28] = igm::vec3{m_ShaftSize, -m_ShaftSize, 0.0f};
    vertices[29] = igm::vec3{-m_ShaftSize, -m_ShaftSize, 0.0f};
    vertices[30] = igm::vec3{-m_ShaftSize, m_ShaftSize, m_ShaftLength};
    vertices[31] = igm::vec3{m_ShaftSize, m_ShaftSize, m_ShaftLength};
    vertices[32] = igm::vec3{m_ShaftSize, -m_ShaftSize, m_ShaftLength};
    vertices[33] = igm::vec3{-m_ShaftSize, -m_ShaftSize, m_ShaftLength};
    vertices[34] = igm::vec3{-m_ArrowSize, m_ArrowSize, m_ShaftLength};
    vertices[35] = igm::vec3{m_ArrowSize, m_ArrowSize, m_ShaftLength};
    vertices[36] = igm::vec3{m_ArrowSize, -m_ArrowSize, m_ShaftLength};
    vertices[37] = igm::vec3{-m_ArrowSize, -m_ArrowSize, m_ShaftLength};
    vertices[38] = igm::vec3{0.0f, 0.0f, m_ShaftLength + m_ArrowSize * 2};
    for (int i = 26; i <= 38; ++i) { colors[i] = igm::vec3{0.0f, 0.0f, 1.0f}; }
    // origin
    vertices[39] = igm::vec3{-m_OriginSize, -m_OriginSize, m_OriginSize};
    vertices[40] = igm::vec3{m_OriginSize, -m_OriginSize, m_OriginSize};
    vertices[41] = igm::vec3{m_OriginSize, m_OriginSize, m_OriginSize};
    vertices[42] = igm::vec3{-m_OriginSize, m_OriginSize, m_OriginSize};
    vertices[43] = igm::vec3{-m_OriginSize, -m_OriginSize, -m_OriginSize};
    vertices[44] = igm::vec3{m_OriginSize, -m_OriginSize, -m_OriginSize};
    vertices[45] = igm::vec3{m_OriginSize, m_OriginSize, -m_OriginSize};
    vertices[46] = igm::vec3{-m_OriginSize, m_OriginSize, -m_OriginSize};
    for (int i = 39; i <= 46; ++i) { colors[i] = igm::vec3{1.0f, 1.0f, 1.0f}; }
}

void Axes::displayCoordToWorldCoord(igm::vec4& dc, igm::vec4& wc) {
    float t[4] = {dc.x, dc.y, dc.z, 1};

    //  reverse of viewPort transport
    t[0] = (t[0] - Viewport[0]) / (0.5 * Viewport[2]) - 1;
    t[1] = 1.f - (t[1] - Viewport[1]) / (0.5 * Viewport[3]);
    wc = inv_mvp * igm::vec4{t[0], t[1], t[2], t[3]};
    wc /= wc.w;
}

void Axes::worldCoordToDisplayCoord(igm::vec4& wc, igm::vec4& dc) {
    dc = mvp * wc;
    dc /= dc.w;

    //  x = (x + 1) / 2 * width + bottom_left_x
    dc.x = (dc.x + 1) * 0.5 * Viewport[2] + Viewport[0];
    //  y = (1 - (y + 1) / 2) * height + bottom_left_y
    dc.y = (1 - (dc.y + 1) * 0.5) * Viewport[3] + Viewport[1];
}

IGAME_NAMESPACE_END