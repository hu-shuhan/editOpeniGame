#include "iGameColorBar2DActor.h"
#include "iGameScene.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>

IGAME_NAMESPACE_BEGIN

ColorBar2DActor::ColorBar2DActor() = default;

ColorBar2DActor::~ColorBar2DActor() = default;

void ColorBar2DActor::SetScene(SmartPointer<Scene> scene) { m_Scene = scene; }

SmartPointer<Scene> ColorBar2DActor::GetScene() const { return m_Scene; }

void ColorBar2DActor::Initialize() {
    if (m_Initialized) { return; }

    m_VAO = GLVertexArray::New();
    m_VAO->Create();

    m_PositionVBO = GLBuffer::New();
    m_PositionVBO->Create();
    m_PositionVBO->Target(GL_ARRAY_BUFFER);

    m_ColorVBO = GLBuffer::New();
    m_ColorVBO->Create();
    m_ColorVBO->Target(GL_ARRAY_BUFFER);

    m_LineEBO = GLBuffer::New();
    m_LineEBO->Create();
    m_LineEBO->Target(GL_ELEMENT_ARRAY_BUFFER);

    m_TriangleEBO = GLBuffer::New();
    m_TriangleEBO->Create();
    m_TriangleEBO->Target(GL_ELEMENT_ARRAY_BUFFER);

    m_VAO->VertexBuffer(GL_VBO_IDX_0, m_PositionVBO, 0, 3 * sizeof(float));
    GLSetVertexAttrib(m_VAO, GL_LOCATION_IDX_0, GL_VBO_IDX_0, 3, GL_FLOAT,
                      GL_FALSE, 0);
    m_VAO->VertexBuffer(GL_VBO_IDX_1, m_ColorVBO, 0, 3 * sizeof(float));
    GLSetVertexAttrib(m_VAO, GL_LOCATION_IDX_1, GL_VBO_IDX_1, 3, GL_FLOAT,
                      GL_FALSE, 0);

    m_Initialized = true;
}

void ColorBar2DActor::SetColorMapper(ScalarsToColors::Pointer mapper) {
    if (m_ColorMapper == mapper) { return; }
    m_ColorMapper = mapper;
    this->Modified();
}

ScalarsToColors::Pointer ColorBar2DActor::GetColorMapper() const {
    return m_ColorMapper;
}

void ColorBar2DActor::SetVisible(bool visible) {
    if (m_Visible == visible) { return; }
    m_Visible = visible;
    this->Modified();
}

bool ColorBar2DActor::GetVisible() const { return m_Visible; }

void ColorBar2DActor::SetCoordinateMode(CoordinateMode mode) {
    if (m_CoordinateMode == mode) { return; }
    m_CoordinateMode = mode;
    this->Modified();
}

ColorBar2DActor::CoordinateMode ColorBar2DActor::GetCoordinateMode() const {
    return m_CoordinateMode;
}

void ColorBar2DActor::SetPosition(float x, float y) {
    if (m_Position.x == x && m_Position.y == y) { return; }
    m_Position = igm::vec2{x, y};
    this->Modified();
}

void ColorBar2DActor::SetSize(float width, float height) {
    if (m_Size.x == width && m_Size.y == height) { return; }
    m_Size = igm::vec2{width, height};
    this->Modified();
}

void ColorBar2DActor::SetOrientation(Orientation orientation) {
    if (m_Orientation == orientation) { return; }
    m_Orientation = orientation;
    this->Modified();
}

ColorBar2DActor::Orientation ColorBar2DActor::GetOrientation() const {
    return m_Orientation;
}

void ColorBar2DActor::SetNumberOfLabels(int numberOfLabels) {
    numberOfLabels = std::max(2, numberOfLabels);
    if (m_NumberOfLabels == numberOfLabels) { return; }
    m_NumberOfLabels = numberOfLabels;
    this->Modified();
}

int ColorBar2DActor::GetNumberOfLabels() const { return m_NumberOfLabels; }

void ColorBar2DActor::SetMaximumNumberOfColors(int maximumNumberOfColors) {
    maximumNumberOfColors = std::max(2, maximumNumberOfColors);
    if (m_MaximumNumberOfColors == maximumNumberOfColors) { return; }
    m_MaximumNumberOfColors = maximumNumberOfColors;
    this->Modified();
}

int ColorBar2DActor::GetMaximumNumberOfColors() const {
    return m_MaximumNumberOfColors;
}

void ColorBar2DActor::SetTitle(const std::string& title) {
    if (m_Title == title) { return; }
    m_Title = title;
    this->Modified();
}

const std::string& ColorBar2DActor::GetTitle() const { return m_Title; }

ScalarsToColors::Pointer ColorBar2DActor::ResolveColorMapper() const {
    if (m_ColorMapper) { return m_ColorMapper; }
    if (!m_Scene) {
        std::cout
                << "[ColorBar2DActor] ResolveColorMapper skipped: scene is null"
                << std::endl;
        return nullptr;
    }

    auto model = m_Scene->GetCurrentModel();
    if (!model) {
        std::cout << "[ColorBar2DActor] ResolveColorMapper skipped: current "
                     "model is null"
                  << std::endl;
        return nullptr;
    }

    auto dataObject = model->GetDataObject();
    if (!dataObject) {
        std::cout << "[ColorBar2DActor] ResolveColorMapper skipped: data "
                     "object is null"
                  << std::endl;
        return nullptr;
    }

    if (dataObject->GetAttributeIndex() < 0) {
        std::cout << "[ColorBar2DActor] ResolveColorMapper skipped: attribute "
                     "index < 0"
                  << std::endl;
        return nullptr;
    }

    auto mapper = dataObject->GetColorMapper();
    if (!mapper) {
        std::cout << "[ColorBar2DActor] ResolveColorMapper skipped: data "
                     "object color mapper is null"
                  << std::endl;
        return nullptr;
    }

    return mapper;
}

ColorBar2DActor::Layout
ColorBar2DActor::ResolveLayout(float viewportWidth,
                               float viewportHeight) const {
    Layout layout;
    if (m_CoordinateMode == CoordinateMode::NormalizedViewport) {
        layout.x = m_Position.x * viewportWidth;
        layout.y = m_Position.y * viewportHeight;
        layout.width = m_Size.x * viewportWidth;
        layout.height = m_Size.y * viewportHeight;
    } else {
        layout.x = m_Position.x;
        layout.y = m_Position.y;
        layout.width = m_Size.x;
        layout.height = m_Size.y;
    }

    layout.width = std::max(8.0f, layout.width);
    layout.height = std::max(8.0f, layout.height);
    layout.x = std::clamp(layout.x, 0.0f,
                          std::max(0.0f, viewportWidth - layout.width));
    layout.y = std::clamp(layout.y, 0.0f,
                          std::max(0.0f, viewportHeight - layout.height));
    return layout;
}

void ColorBar2DActor::RebuildGeometry(const Layout& layout,
                                      ScalarsToColors::Pointer mapper) {
    std::vector<igm::vec3> positions;
    std::vector<igm::vec3> colors;
    std::vector<unsigned int> lineIndices;
    std::vector<unsigned int> triangleIndices;

    const int samples = std::max(2, m_MaximumNumberOfColors);
    positions.reserve(samples * 4 + 8 + m_NumberOfLabels * 2);
    colors.reserve(samples * 4 + 8 + m_NumberOfLabels * 2);
    triangleIndices.reserve((samples - 1) * 6);

    auto addVertex = [&](float x, float y, const igm::vec3& color) {
        positions.emplace_back(x, y, 0.0f);
        colors.push_back(color);
        return static_cast<unsigned int>(positions.size() - 1);
    };

    auto addLine = [&](float x0, float y0, float x1, float y1,
                       const igm::vec3& color) {
        unsigned int a = addVertex(x0, y0, color);
        unsigned int b = addVertex(x1, y1, color);
        lineIndices.push_back(a);
        lineIndices.push_back(b);
    };

    float rgb[3]{};
    const float x0 = layout.x;
    const float y0 = layout.y;
    const float x1 = layout.x + layout.width;
    const float y1 = layout.y + layout.height;

    for (int i = 0; i < samples - 1; ++i) {
        const float t0 =
                static_cast<float>(i) / static_cast<float>(samples - 1);
        const float t1 =
                static_cast<float>(i + 1) / static_cast<float>(samples - 1);
        mapper->MapColor(t0, rgb);
        igm::vec3 c0{rgb[0], rgb[1], rgb[2]};
        mapper->MapColor(t1, rgb);
        igm::vec3 c1{rgb[0], rgb[1], rgb[2]};

        unsigned int a, b, c, d;
        if (m_Orientation == Orientation::Vertical) {
            const float ya = y0 + t0 * layout.height;
            const float yb = y0 + t1 * layout.height;
            a = addVertex(x0, ya, c0);
            b = addVertex(x1, ya, c0);
            c = addVertex(x0, yb, c1);
            d = addVertex(x1, yb, c1);
        } else {
            const float xa = x0 + t0 * layout.width;
            const float xb = x0 + t1 * layout.width;
            a = addVertex(xa, y0, c0);
            b = addVertex(xb, y0, c1);
            c = addVertex(xa, y1, c0);
            d = addVertex(xb, y1, c1);
        }

        triangleIndices.insert(triangleIndices.end(), {a, b, c, c, b, d});
    }

    const igm::vec3 white{1.0f, 1.0f, 1.0f};
    addLine(x0, y0, x1, y0, white);
    addLine(x1, y0, x1, y1, white);
    addLine(x1, y1, x0, y1, white);
    addLine(x0, y1, x0, y0, white);

    for (int i = 0; i < m_NumberOfLabels; ++i) {
        const float t = static_cast<float>(i) /
                        static_cast<float>(std::max(1, m_NumberOfLabels - 1));
        if (m_Orientation == Orientation::Vertical) {
            float y = y0 + t * layout.height;
            addLine(x1, y, x1 + std::min(8.0f, layout.width * 0.35f), y, white);
        } else {
            float x = x0 + t * layout.width;
            // place ticks above the color bar to match labels positioned above
            // (use y1 as the top edge and extend upward)
            addLine(x, y1, x, y1 + std::min(8.0f, layout.height * 0.35f),
                    white);
        }
    }

    m_PositionVBO->Allocate(positions.size() * sizeof(igm::vec3),
                            positions.data(), GL_DYNAMIC_DRAW);
    m_ColorVBO->Allocate(colors.size() * sizeof(igm::vec3), colors.data(),
                         GL_DYNAMIC_DRAW);
    m_LineEBO->Allocate(lineIndices.size() * sizeof(unsigned int),
                        lineIndices.data(), GL_DYNAMIC_DRAW);
    m_TriangleEBO->Allocate(triangleIndices.size() * sizeof(unsigned int),
                            triangleIndices.data(), GL_DYNAMIC_DRAW);

    m_LineIndexCount = static_cast<IGsize>(lineIndices.size());
    m_TriangleIndexCount = static_cast<IGsize>(triangleIndices.size());
    m_LastMapperMTime = mapper->GetMTime().GetMTime();
    m_LastActorMTime = this->GetMTime().GetMTime();
    m_LastLayout = layout;

}

void ColorBar2DActor::DrawGeometry(float viewportWidth, float viewportHeight) {

    if (m_LastLayout.width < 40.0f || m_LastLayout.height < 40.0f) {

    }
    if (m_LastLayout.x + m_LastLayout.width > viewportWidth ||
        m_LastLayout.y + m_LastLayout.height > viewportHeight) {
    }

    igm::mat4 model = igm::mat4{1.0f};
    igm::mat4 view = igm::mat4{1.0f};
    igm::mat4 proj = igm::orthoRH_OZ(0.0f, viewportWidth, 0.0f, viewportHeight,
                                     -1.0f, 1.0f);

    auto shaderManager = m_Scene->m_ShaderManager;
    shaderManager->UpdateCameraBlock(
            {igm::vec3{0.0f}, 1,
             igm::vec4{0.0f, viewportWidth, 0.0f, viewportHeight}, -1.0f, 1.0f,
             view, proj, proj * view});
    shaderManager->UpdateObjectBlock(
            {1.0f, model, model.invert().transpose(), igm::vec4{}});
    shaderManager->UpdateUBOBlock({1, 0});

    auto shader = shaderManager->GetShader(ShaderType::NOLIGHT);
    shader->Use();
#ifdef __EMSCRIPTEN__
    shaderManager->ApplyWebFallbackUniforms(shader);
#endif

    m_VAO->VertexBuffer(GL_VBO_IDX_0, m_PositionVBO, 0, 3 * sizeof(float));
    m_VAO->VertexBuffer(GL_VBO_IDX_1, m_ColorVBO, 0, 3 * sizeof(float));

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_VAO->ElementBuffer(m_TriangleEBO);
    m_VAO->DrawElements(GL_TRIANGLES, static_cast<int>(m_TriangleIndexCount),
                        GL_UNSIGNED_INT);
    glLineWidth(1.0f);
    m_VAO->ElementBuffer(m_LineEBO);
    m_VAO->DrawElements(GL_LINES, static_cast<int>(m_LineIndexCount),
                        GL_UNSIGNED_INT);
    glDepthMask(GL_TRUE);

    GLenum drawErr = GL_NO_ERROR;
    while ((drawErr = glGetError()) != GL_NO_ERROR) {
        std::clog << "[ColorBar2DActor] DrawGeometry GL error=" << drawErr
                  << std::endl;
    }

}

void ColorBar2DActor::DrawLabels(const Layout& layout,
                                 ScalarsToColors::Pointer mapper,
                                 float viewportWidth, float viewportHeight) {
    if (!m_Scene || !m_Scene->GetTextOverlay2DActor() || !mapper) { return; }

    const double* range = mapper->GetRange();
    if (range == nullptr) { return; }

    const igm::vec3 textColor{1.0f, 1.0f, 1.0f};
    const std::string minText = FormatNumber(range[0]);
    const std::string midText = FormatNumber((range[0] + range[1]) * 0.5);
    const std::string maxText = FormatNumber(range[1]);

    const float labelScale =
            std::max(0.06f, std::min(0.14f, layout.height / 90.0f));
    const float labelGap = std::max(12.0f, 18.0f * labelScale);

    if (m_Orientation == Orientation::Horizontal) {
        // Force labels to be placed above the color bar by default. This keeps
        // the layout simple and avoids early-frame overlap with corner
        // annotations (watermark).
        const float estimatedTextHeight = 128.0f * labelScale;
        const float tickLength = std::min(8.0f, layout.height * 0.35f);
        const float labelClearance = std::max(6.0f, labelScale * 28.0f);
        const bool placeBelow = false;
        const float labelBaselineY = layout.y + layout.height + labelClearance;
        DrawText(minText, layout.x, labelBaselineY, labelScale, textColor,
                 viewportWidth, viewportHeight);
        DrawText(midText, layout.x + layout.width * 0.5f, labelBaselineY,
                 labelScale, textColor, viewportWidth, viewportHeight);
        DrawText(maxText, layout.x + layout.width, labelBaselineY, labelScale,
                 textColor, viewportWidth, viewportHeight);

        if (!m_Title.empty()) {
            DrawText(m_Title, layout.x,
                     placeBelow ? layout.y + layout.height + labelGap * 0.7f
                                : labelBaselineY + estimatedTextHeight,
                     labelScale, textColor, viewportWidth, viewportHeight);
        }
        return;
    }

    const float labelX = layout.x + layout.width + labelGap * 0.35f;
    DrawText(minText, labelX, layout.y, labelScale, textColor, viewportWidth,
             viewportHeight);
    DrawText(midText, labelX, layout.y + layout.height * 0.5f, labelScale,
             textColor, viewportWidth, viewportHeight);
    DrawText(maxText, labelX, layout.y + layout.height, labelScale, textColor,
             viewportWidth, viewportHeight);

    if (!m_Title.empty()) {
        DrawText(m_Title, layout.x, layout.y + layout.height + labelGap * 0.7f,
                 labelScale, textColor, viewportWidth, viewportHeight);
    }
}

void ColorBar2DActor::DrawText(const std::string& text, float x, float y,
                               float scale, const igm::vec3& color,
                               float viewportWidth, float viewportHeight) {
    if (!m_Scene || !m_Scene->GetTextOverlay2DActor()) { return; }
    m_Scene->GetTextOverlay2DActor()->DrawText(text, x, y, scale, color,
                                               viewportWidth, viewportHeight);
}

std::string ColorBar2DActor::FormatNumber(double value) const {
    std::ostringstream stream;
    const double absValue = std::abs(value);
    if ((absValue > 0.0 && absValue < 0.001) || absValue >= 10000.0) {
        stream << std::scientific << std::setprecision(2) << value;
    } else {
        stream << std::setprecision(4) << value;
    }
    return stream.str();
}

void ColorBar2DActor::Draw() {

    if (!m_Visible || !m_Scene) {
        return;
    }
    if (!m_Initialized) { Initialize(); }

    auto mapper = ResolveColorMapper();
    if (!mapper) {
        std::cout << "[ColorBar2DActor] Draw skipped: color mapper is null"
                  << std::endl;
        return;
    }


    int vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    const float viewportWidth = static_cast<float>(vp[2]);
    const float viewportHeight = static_cast<float>(vp[3]);
    if (viewportWidth <= 0.0f || viewportHeight <= 0.0f) {
        std::cout << "[ColorBar2DActor] Draw skipped: invalid viewport "
                  << viewportWidth << "x" << viewportHeight << std::endl;
        return;
    }

    Layout layout = ResolveLayout(viewportWidth, viewportHeight);

    const bool layoutChanged = layout.x != m_LastLayout.x ||
                               layout.y != m_LastLayout.y ||
                               layout.width != m_LastLayout.width ||
                               layout.height != m_LastLayout.height;

    if (layoutChanged || mapper->GetMTime().GetMTime() != m_LastMapperMTime ||
        this->GetMTime().GetMTime() != m_LastActorMTime) {
        RebuildGeometry(layout, mapper);
    } else {

    }

    DrawGeometry(viewportWidth, viewportHeight);
    DrawLabels(layout, mapper, viewportWidth, viewportHeight);

}

IGAME_NAMESPACE_END
