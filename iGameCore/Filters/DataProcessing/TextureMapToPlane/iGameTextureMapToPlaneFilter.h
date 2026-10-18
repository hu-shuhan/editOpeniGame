#pragma once

#include <iGameFilter.h>
#include <iGamePointSet.h>

#include <string>

IGAME_NAMESPACE_BEGIN

class TextureMapToPlaneFilter : public Filter {
public:
    I_OBJECT(TextureMapToPlaneFilter);
    static Pointer New() { return new TextureMapToPlaneFilter; }

    bool Execute() override;

    static const char* TextureCoordinatesArrayName() { return "Texture Coordinates"; }

    void SetAutomaticPlaneGeneration(bool automatic) { m_AutomaticPlaneGeneration = automatic; }
    bool GetAutomaticPlaneGeneration() const { return m_AutomaticPlaneGeneration; }

    void SetOrigin(const Point& origin) { m_Origin = origin; }
    void SetPoint1(const Point& point) { m_Point1 = point; }
    void SetPoint2(const Point& point) { m_Point2 = point; }
    const Point& GetOrigin() const { return m_Origin; }
    const Point& GetPoint1() const { return m_Point1; }
    const Point& GetPoint2() const { return m_Point2; }

    const std::string& GetLastError() const { return m_LastError; }
    FloatArray::Pointer GetTextureCoordinates() const { return m_TextureCoordinates; }

protected:
    TextureMapToPlaneFilter();
    ~TextureMapToPlaneFilter() override = default;

private:
    bool m_AutomaticPlaneGeneration{true};
    Point m_Origin{0.0f, 0.0f, 0.0f};
    Point m_Point1{1.0f, 0.0f, 0.0f};
    Point m_Point2{0.0f, 1.0f, 0.0f};
    FloatArray::Pointer m_TextureCoordinates{};
    std::string m_LastError{};
};

IGAME_NAMESPACE_END
