#include "iGameColorMap.h"
IGAME_NAMESPACE_BEGIN
ColorMap::ColorMap() {
    this->m_ColorBar = FloatArray::New();
    this->m_ColorRange = FloatArray::New();
    InitColorBarWithBlueWhiteRedType();
    this->m_MapType = IG_MAPPER_RGB_LINER;
}
ColorMap::~ColorMap() {
    m_ColorBar = nullptr;
    m_ColorRange = nullptr;
}
void ColorMap::InitColorBarWithGrayScaleType() {
    m_ColorBar->Reset();
    m_ColorBar->SetDimension(3);
    m_ColorBar->Reserve(2);
    m_ColorBar->AddElement3(0.0, 0.0, 0.0);
    m_ColorBar->AddElement3(1.0, 1.0, 1.0);

    m_ColorRange->Reset();
    m_ColorRange->Reserve(2);
    m_ColorRange->AddValue(0.0);
    m_ColorRange->AddValue(1.0);
}
void ColorMap::InitColorBarWithBlueWhiteRedType() {
    m_ColorBar->Reset();
    m_ColorBar->SetDimension(3);
    m_ColorBar->Reserve(3);
    m_ColorBar->AddElement3(0.2, 0.25, 0.75);
    m_ColorBar->AddElement3(0.85, 0.85, 0.85);
    m_ColorBar->AddElement3(0.9, 0.15, 0.1);

    m_ColorRange->Reset();
    m_ColorRange->Reserve(3);
    m_ColorRange->AddValue(0.0);
    m_ColorRange->AddValue(0.5);
    m_ColorRange->AddValue(1.0);
}
void ColorMap::InitColorBarWithBlueCyanGreenYellowRedType() {
    m_ColorBar->Reset();
    m_ColorBar->SetDimension(3);
    m_ColorBar->Reserve(5);
    m_ColorBar->AddElement3(0.0, 0.0, 1.0);
    m_ColorBar->AddElement3(0.0, 1.0, 1.0);
    m_ColorBar->AddElement3(0.0, 1.0, 0.0);
    m_ColorBar->AddElement3(1.0, 1.0, 0.0);
    m_ColorBar->AddElement3(1.0, 0.0, 0.0);

    m_ColorRange->Reset();
    m_ColorRange->Reserve(5);
    m_ColorRange->AddValue(0.0);
    m_ColorRange->AddValue(0.25);
    m_ColorRange->AddValue(0.5);
    m_ColorRange->AddValue(0.75);
    m_ColorRange->AddValue(1.0);
}
void ColorMap::InitColorBarWithBlueCyanGreenYellowRedMagentaType() {
    m_ColorBar->Reset();
    m_ColorBar->SetDimension(3);
    m_ColorBar->Reserve(6);
    m_ColorBar->AddElement3(0.0, 0.0, 1.0);
    m_ColorBar->AddElement3(0.0, 1.0, 1.0);
    m_ColorBar->AddElement3(0.0, 1.0, 0.0);
    m_ColorBar->AddElement3(1.0, 1.0, 0.0);
    m_ColorBar->AddElement3(1.0, 0.0, 0.0);
    m_ColorBar->AddElement3(1.0, 0.0, 1.0);

    m_ColorRange->Reset();
    m_ColorRange->Reserve(6);
    m_ColorRange->AddValue(0.0);
    m_ColorRange->AddValue(0.2);
    m_ColorRange->AddValue(0.4);
    m_ColorRange->AddValue(0.6);
    m_ColorRange->AddValue(0.8);
    m_ColorRange->AddValue(1.0);
}
void ColorMap::InitColorBarWithTurboType() {
    m_ColorBar->Reset();
    m_ColorBar->SetDimension(3);
    m_ColorBar->Reserve(11);
    m_ColorBar->AddElement3(0.188, 0.071, 0.231);
    m_ColorBar->AddElement3(0.255, 0.271, 0.671);
    m_ColorBar->AddElement3(0.275, 0.459, 0.929);
    m_ColorBar->AddElement3(0.224, 0.635, 0.988);
    m_ColorBar->AddElement3(0.106, 0.812, 0.831);
    m_ColorBar->AddElement3(0.141, 0.925, 0.651);
    m_ColorBar->AddElement3(0.380, 0.988, 0.424);
    m_ColorBar->AddElement3(0.635, 0.988, 0.235);
    m_ColorBar->AddElement3(0.855, 0.839, 0.141);
    m_ColorBar->AddElement3(0.957, 0.467, 0.129);
    m_ColorBar->AddElement3(0.478, 0.016, 0.012);

    m_ColorRange->Reset();
    m_ColorRange->Reserve(11);
    for (int i = 0; i < 11; ++i) { m_ColorRange->AddValue(i / 10.0); }
}
void ColorMap::InitColorBarWithViridisType() {
    m_ColorBar->Reset();
    m_ColorBar->SetDimension(3);
    m_ColorBar->Reserve(10);
    m_ColorBar->AddElement3(0.267, 0.005, 0.329);
    m_ColorBar->AddElement3(0.282, 0.157, 0.471);
    m_ColorBar->AddElement3(0.243, 0.290, 0.537);
    m_ColorBar->AddElement3(0.192, 0.408, 0.557);
    m_ColorBar->AddElement3(0.149, 0.510, 0.557);
    m_ColorBar->AddElement3(0.122, 0.620, 0.537);
    m_ColorBar->AddElement3(0.208, 0.718, 0.475);
    m_ColorBar->AddElement3(0.427, 0.804, 0.349);
    m_ColorBar->AddElement3(0.706, 0.871, 0.173);
    m_ColorBar->AddElement3(0.992, 0.906, 0.145);

    m_ColorRange->Reset();
    m_ColorRange->Reserve(10);
    for (int i = 0; i < 10; ++i) { m_ColorRange->AddValue(i / 9.0); }
}
void ColorMap::InitColorBarWithCoolWarmType() {
    m_ColorBar->Reset();
    m_ColorBar->SetDimension(3);
    m_ColorBar->Reserve(8);
    m_ColorBar->AddElement3(0.231, 0.298, 0.753);
    m_ColorBar->AddElement3(0.404, 0.533, 0.933);
    m_ColorBar->AddElement3(0.604, 0.733, 1.000);
    m_ColorBar->AddElement3(0.788, 0.843, 0.941);
    m_ColorBar->AddElement3(0.929, 0.820, 0.761);
    m_ColorBar->AddElement3(0.969, 0.659, 0.537);
    m_ColorBar->AddElement3(0.886, 0.412, 0.322);
    m_ColorBar->AddElement3(0.706, 0.016, 0.149);

    m_ColorRange->Reset();
    m_ColorRange->Reserve(8);
    for (int i = 0; i < 8; ++i) { m_ColorRange->AddValue(i / 7.0); }
}
void ColorMap::SetIndexColor(int index, float r, float g, float b) {
    float rgb[3]{r, g, b};
    m_ColorBar->SetElement(index, rgb);
}
void ColorMap::SetIndexRange(int index, float& x) { this->m_ColorRange->SetValue(index, x); }

void ColorMap::DeleteIndexColor(int index) {
    FloatArray::Pointer retColor = FloatArray::New();
    FloatArray::Pointer retRange = FloatArray::New();
    retColor->SetDimension(3);
    float color[3] = {0, 0, 0};
    int ColorBarSize = this->GetColorBarSize();
    for (int i = 0; i <= ColorBarSize; i++) {
        if (i == index) continue;
        this->m_ColorBar->GetElement(i, color);
        retColor->AddElement(color);
        retRange->AddValue(m_ColorRange->GetValue(i));
    }
    m_ColorBar = retColor;
    m_ColorRange = retRange;
}
void ColorMap::InsertIndexColor(int index, float r, float g, float b) {
    FloatArray::Pointer ret = FloatArray::New();
    ret->SetDimension(3);
    for (int i = 0; i < index; i++) {
        float color[3];
        m_ColorBar->GetElement(i, color);
        ret->AddElement(color);
    }
    ret->AddElement3(r, g, b);
    int ColorBarSize = this->GetColorBarSize();
    for (int i = index; i <= ColorBarSize; i++) {
        float color[3];
        m_ColorBar->GetElement(i, color);
        ret->AddElement(color);
    }
    m_ColorBar = ret;
}
void ColorMap::InsertIndexRange(int index, float x) {
    FloatArray::Pointer ret = FloatArray::New();
    for (int i = 0; i < index; i++) { ret->AddValue(m_ColorRange->GetValue(i)); }
    ret->AddValue(x);
    int rangeSize = m_ColorRange->GetNumberOfElements();
    for (int i = index; i < rangeSize; i++) { ret->AddValue(m_ColorRange->GetValue(i)); }
    m_ColorRange = ret;
}

void ColorMap::MapColor(float value, float rgb[3]) {
    int idx = 0;
    float st_v, fi_v, local_v;
    int ColorBarSize = this->GetColorBarSize();
    if (ColorBarSize < 0) {
        rgb[0] = 0.0f;
        rgb[1] = 0.0f;
        rgb[2] = 0.0f;
        return;
    }
    if (value <= m_ColorRange->GetValue(0)) {
        m_ColorBar->GetElement(0, rgb);
        return;
    }
    if (value >= m_ColorRange->GetValue(ColorBarSize)) {
        m_ColorBar->GetElement(ColorBarSize, rgb);
        return;
    }
    for (idx = 0; idx <= ColorBarSize; idx++) {
        if (value < m_ColorRange->GetValue(idx)) { break; }
    }

    float startRGB[3];
    float finalRGB[3];
    //std::cout << idx << std::endl;
    if (idx == 0) {
        m_ColorBar->GetElement(idx, startRGB);
        st_v = 0.0;
    } else {
        m_ColorBar->GetElement(idx - 1, startRGB);
        st_v = m_ColorRange->GetValue(idx - 1);
    }
    if (this->m_MapType == IG_MAPPER_RGB_STEP) {
        rgb[0] = startRGB[0];
        rgb[1] = startRGB[1];
        rgb[2] = startRGB[2];
        return;
    }
    if (idx == ColorBarSize + 1) {
        m_ColorBar->GetElement(ColorBarSize, finalRGB);
        fi_v = 1.0;
    } else {
        m_ColorBar->GetElement(idx, finalRGB);
        fi_v = m_ColorRange->GetValue(idx);
    }
    if (fi_v == st_v) {
        rgb[0] = finalRGB[0];
        rgb[1] = finalRGB[1];
        rgb[2] = finalRGB[2];
        return;
    }
    local_v = (value - st_v) / (fi_v - st_v);

    //std::cout << st_v << " " << fi_v << " " << local_v << std::endl;
    //std::cout << startRGB[0] << " " << startRGB[1] << " " << startRGB[2] << "\n";
    //std::cout << finalRGB[0] << " " << finalRGB[1] << " " << finalRGB[2] << "\n";
    if (this->m_MapType == IG_MAPPER_RGB_LINER) {
        //double s = local_v;
        //double sharpness = 0.0;
        //if (s < .5){
        //	s = 0.5 * pow(s * 2, 1.0 + 10 * sharpness);
        //}
        //else if (s > .5){
        //	s = 1.0 - 0.5 * pow((1.0 - s) * 2, 1 + 10 * sharpness);
        //}
        //// Compute some coefficients we will need for the hermite curve
        //double ss = s * s;
        //double sss = ss * s;
        //double h1 = 2 * sss - 3 * ss + 1;
        //double h2 = -2 * sss + 3 * ss;
        //double h3 = sss - 2 * ss + s;
        //double h4 = sss - ss;
        //double slope;
        //double t;
        //int j = 0;
        //for (j = 0; j < 3; j++){
        //	// Use one slope for both end points
        //	slope = finalRGB[j] - startRGB[j];
        //	t = (1.0 - sharpness) * slope;
        //	// Compute the value
        //	rgb[j] = h1 * startRGB[j] + h2 * finalRGB[j] + h3 * t + h4 * t;
        //}
        rgb[0] = local_v * finalRGB[0] + (1 - local_v) * startRGB[0];
        rgb[1] = local_v * finalRGB[1] + (1 - local_v) * startRGB[1];
        rgb[2] = local_v * finalRGB[2] + (1 - local_v) * startRGB[2];
        return;
    }
    //std::cout << rgb[0] << " " << rgb[1] << " " << rgb[2] << "\n";
}
bool ColorMap::DeepCopy(const ColorMap::Pointer other) {
    if (other == nullptr) { return false; }

    this->m_ColorBar->DeepCopy(other->m_ColorBar);
    this->m_ColorRange->DeepCopy(other->m_ColorRange);
    this->m_MapType = other->m_MapType;
    return true;
}

void ColorMap::MapOpacity(float value, float& opacity) {
    value = value < 0.0 ? 0.0 : (value > 0.999999 ? 0.999999 : value);
    opacity = 0.1 + value * 0.9;
}

IGAME_NAMESPACE_END
