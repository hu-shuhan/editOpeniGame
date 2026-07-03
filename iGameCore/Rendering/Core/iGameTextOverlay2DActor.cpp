#include "iGameTextOverlay2DActor.h"
#include "iGameScene.h"
#include <climits>
#include <cstdint>
#include <vector>

IGAME_NAMESPACE_BEGIN

namespace
{
std::wstring DecodeUtf8(const std::string& text) {
    std::wstring result;
    for (size_t i = 0; i < text.size();) {
        const auto lead = static_cast<unsigned char>(text[i]);
        uint32_t codePoint = 0;
        size_t length = 0;
        if ((lead & 0x80u) == 0u) {
            codePoint = lead;
            length = 1;
        } else if ((lead & 0xe0u) == 0xc0u) {
            codePoint = lead & 0x1fu;
            length = 2;
        } else if ((lead & 0xf0u) == 0xe0u) {
            codePoint = lead & 0x0fu;
            length = 3;
        } else if ((lead & 0xf8u) == 0xf0u) {
            codePoint = lead & 0x07u;
            length = 4;
        } else {
            result.push_back(static_cast<wchar_t>(lead));
            ++i;
            continue;
        }

        if (i + length > text.size()) {
            result.push_back(static_cast<wchar_t>(lead));
            ++i;
            continue;
        }
        bool valid = true;
        for (size_t offset = 1; offset < length; ++offset) {
            const auto next = static_cast<unsigned char>(text[i + offset]);
            if ((next & 0xc0u) != 0x80u) {
                valid = false;
                break;
            }
            codePoint = (codePoint << 6u) | (next & 0x3fu);
        }
        if (!valid) {
            result.push_back(static_cast<wchar_t>(lead));
            ++i;
            continue;
        }

#if WCHAR_MAX <= 0xffff
        if (codePoint > 0xffffu) {
            codePoint -= 0x10000u;
            result.push_back(
                    static_cast<wchar_t>(0xd800u + (codePoint >> 10u)));
            result.push_back(
                    static_cast<wchar_t>(0xdc00u + (codePoint & 0x3ffu)));
        } else
#endif
        {
            result.push_back(static_cast<wchar_t>(codePoint));
        }
        i += length;
    }
    return result;
}
} // namespace

TextOverlay2DActor::TextOverlay2DActor() = default;

TextOverlay2DActor::~TextOverlay2DActor() = default;

void TextOverlay2DActor::SetScene(SmartPointer<Scene> scene) {
    m_Scene = scene;
}

void TextOverlay2DActor::Initialize() {
    if (m_Initialized) { return; }

    m_VAO = GLVertexArray::New();
    m_VAO->Create();

    m_PositionVBO = GLBuffer::New();
    m_PositionVBO->Create();
    m_PositionVBO->Target(GL_ARRAY_BUFFER);

    m_UVVBO = GLBuffer::New();
    m_UVVBO->Create();
    m_UVVBO->Target(GL_ARRAY_BUFFER);

    m_EBO = GLBuffer::New();
    m_EBO->Create();
    m_EBO->Target(GL_ELEMENT_ARRAY_BUFFER);

    std::vector<unsigned int> indices{0, 1, 2, 2, 1, 3};
    m_EBO->Allocate(indices.size() * sizeof(unsigned int), indices.data(),
                    GL_STATIC_DRAW);

    auto shader = m_Scene->m_ShaderManager->GetShader(ShaderType::FONT);
    const auto positionAttribute = shader->GetAttribLocation("in_Position");
    const auto uvAttribute = shader->GetAttribLocation("in_UV");

    m_VAO->VertexBuffer(GL_VBO_IDX_0, m_PositionVBO, 0, 3 * sizeof(float));
    GLSetVertexAttrib(m_VAO, positionAttribute, GL_VBO_IDX_0, 3, GL_FLOAT,
                      GL_FALSE, 0);
    m_VAO->VertexBuffer(GL_VBO_IDX_1, m_UVVBO, 0, 2 * sizeof(float));
    GLSetVertexAttrib(m_VAO, uvAttribute, GL_VBO_IDX_1, 2, GL_FLOAT, GL_FALSE,
                      0);
    m_VAO->ElementBuffer(m_EBO);

    m_Initialized = true;
}

void TextOverlay2DActor::Draw() {
    if (!m_Visible || !m_Scene || m_Text.empty()) { return; }
    if (!m_Initialized) { Initialize(); }

    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    const float width = static_cast<float>(viewport[2]);
    const float height = static_cast<float>(viewport[3]);
    if (width <= 0.0f || height <= 0.0f) { return; }

    const std::wstring characters = DecodeUtf8(m_Text);
    const float textWidth = ComputeTextWidth(characters, m_Scale);
    const float anchorX =
            m_AnchorToBottomRight
                    ? std::max(0.0f, width - m_Position.x - textWidth)
                    : m_Position.x;
    const float baseline = m_AnchorToBottomRight
                                   ? m_Position.y + 128.0f * m_Scale
                                   : height - m_Position.y - 128.0f * m_Scale;
    DrawText(m_Text, anchorX, baseline, m_Scale, m_Color, width, height);
}

void TextOverlay2DActor::DrawText(const std::string& text, float x,
                                  float baselineY, float scale,
                                  const igm::vec3& color, float viewportWidth,
                                  float viewportHeight) {
    if (!m_Scene || text.empty()) { return; }
    if (!m_Initialized) { Initialize(); }

    const std::wstring characters = DecodeUtf8(text);

    auto shader = m_Scene->m_ShaderManager->GetShader(ShaderType::FONT);
    shader->Use();
    shader->SetUniformMatrix4x4("proj",
                                igm::orthoRH_OZ(0.0f, viewportWidth, 0.0f,
                                                viewportHeight, -1.0f, 1.0f));
    shader->SetUniform3f("textColor", color);
    shader->SetUniformi("fontSampler", 1);

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float cursorX = x;
    for (wchar_t character: characters) {
        auto& glyph = m_Scene->m_FontManager->GetCharacter(character);
        const float advance = static_cast<float>(glyph.Advance >> 6) * scale;
        if (glyph.Size.x == 0 || glyph.Size.y == 0) {
            cursorX += advance;
            continue;
        }
        auto texture = m_Scene->m_FontManager->GetTexture(character);
        if (!texture) {
            cursorX += advance;
            continue;
        }

        const float xpos = cursorX + glyph.Bearing.x * scale;
        const float ypos = baselineY - (glyph.Size.y - glyph.Bearing.y) * scale;
        const float width = glyph.Size.x * scale;
        const float height = glyph.Size.y * scale;

        std::vector<float> vertices{
                xpos, ypos,          0.0f, xpos + width, ypos,          0.0f,
                xpos, ypos + height, 0.0f, xpos + width, ypos + height, 0.0f,
        };
        std::vector<float> uvs{
                0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
        };

        m_PositionVBO->Allocate(vertices.size() * sizeof(float),
                                vertices.data(), GL_DYNAMIC_DRAW);
        m_UVVBO->Allocate(uvs.size() * sizeof(float), uvs.data(),
                          GL_DYNAMIC_DRAW);
        m_VAO->VertexBuffer(GL_VBO_IDX_0, m_PositionVBO, 0, 3 * sizeof(float));
        m_VAO->VertexBuffer(GL_VBO_IDX_1, m_UVVBO, 0, 2 * sizeof(float));
        texture->Active(GL_TEXTURE1);
        m_VAO->DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT);

        cursorX += advance;
    }

    glDepthMask(GL_TRUE);
}

float TextOverlay2DActor::ComputeCenteredTextX(const std::string& text,
                                               float centerX,
                                               float scale) const {
    if (!m_Scene || text.empty()) { return centerX; }

    const std::wstring characters = DecodeUtf8(text);
    float cursorX = 0.0f;
    float left = 0.0f;
    float right = 0.0f;
    bool hasVisibleGlyph = false;
    for (wchar_t character: characters) {
        auto& glyph = m_Scene->m_FontManager->GetCharacter(character);
        const float advance = static_cast<float>(glyph.Advance >> 6) * scale;
        if (glyph.Size.x != 0 && glyph.Size.y != 0) {
            const float glyphLeft = cursorX + glyph.Bearing.x * scale;
            const float glyphRight = glyphLeft + glyph.Size.x * scale;
            if (!hasVisibleGlyph) {
                left = glyphLeft;
                right = glyphRight;
                hasVisibleGlyph = true;
            } else {
                left = std::min(left, glyphLeft);
                right = std::max(right, glyphRight);
            }
        }
        cursorX += advance;
    }

    return hasVisibleGlyph ? centerX - (left + right) * 0.5f
                           : centerX - cursorX * 0.5f;
}

void TextOverlay2DActor::SetText(const std::string& text) {
    if (m_Text == text) { return; }
    m_Text = text;
    this->Modified();
}

const std::string& TextOverlay2DActor::GetText() const { return m_Text; }

void TextOverlay2DActor::SetVisible(bool visible) {
    if (m_Visible == visible) { return; }
    m_Visible = visible;
    this->Modified();
}

bool TextOverlay2DActor::GetVisible() const { return m_Visible; }

void TextOverlay2DActor::SetPosition(float left, float top) {
    m_Position = igm::vec2{left, top};
    this->Modified();
}

void TextOverlay2DActor::SetScale(float scale) {
    m_Scale = scale;
    this->Modified();
}

void TextOverlay2DActor::SetColor(const igm::vec3& color) {
    m_Color = color;
    this->Modified();
}

void TextOverlay2DActor::SetAnchorToBottomRight(bool anchorToBottomRight) {
    if (m_AnchorToBottomRight == anchorToBottomRight) { return; }
    m_AnchorToBottomRight = anchorToBottomRight;
    this->Modified();
}

bool TextOverlay2DActor::GetAnchorToBottomRight() const {
    return m_AnchorToBottomRight;
}

float TextOverlay2DActor::ComputeTextWidth(const std::wstring& characters,
                                           float scale) const {
    if (!m_Scene || characters.empty()) { return 0.0f; }

    float cursorX = 0.0f;
    float maxX = 0.0f;
    for (wchar_t character: characters) {
        auto& glyph = m_Scene->m_FontManager->GetCharacter(character);
        const float advance = static_cast<float>(glyph.Advance >> 6) * scale;
        if (glyph.Size.x == 0 || glyph.Size.y == 0) {
            cursorX += advance;
            maxX = std::max(maxX, cursorX);
            continue;
        }

        const float xpos = cursorX + glyph.Bearing.x * scale;
        const float width = glyph.Size.x * scale;
        maxX = std::max(maxX, xpos + width);
        cursorX += advance;
    }

    return std::max(maxX, cursorX);
}

IGAME_NAMESPACE_END
