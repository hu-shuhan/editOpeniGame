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
void ColorMap::InitColorBarWithFastType() {
    static const float positions[9] = {0.0000f, 0.1716f, 0.2985f, 0.4321f, 0.5000f,
                                       0.5882f, 0.7061f, 0.8476f, 1.0000f};
    static const float colors[9][3] = {{0.0564f, 0.0564f, 0.4700f},
                                       {0.2430f, 0.4604f, 0.8100f},
                                       {0.3568f, 0.7450f, 0.9544f},
                                       {0.6882f, 0.9300f, 0.9179f},
                                       {0.8995f, 0.9446f, 0.7687f},
                                       {0.9571f, 0.8338f, 0.5089f},
                                       {0.9275f, 0.6214f, 0.3154f},
                                       {0.8000f, 0.3520f, 0.1600f},
                                       {0.5900f, 0.0767f, 0.1195f}};

    m_ColorBar->Reset();
    m_ColorBar->SetDimension(3);
    m_ColorBar->Reserve(9);
    m_ColorRange->Reset();
    m_ColorRange->Reserve(9);
    for (int i = 0; i < 9; ++i) {
        m_ColorBar->AddElement3(colors[i][0], colors[i][1], colors[i][2]);
        m_ColorRange->AddValue(positions[i]);
    }
}

void ColorMap::InitColorBarWithTurboSoftType() {
    // Turbo colormap (based on turbo.json), sampled to 17 control points.
    // The green/yellow middle section is desaturated (up to 40%) so it looks
    // softer and less vivid, while the blue/red ends keep their saturation.
    // Control-point positions remapped from the uniform Turbo spacing:
    // the cyan and orange bands are widened, while the blue/violet and red
    // ends are compressed, matching the reference color bar proportions.
    static const float positions[17] = {
            0.0000f, 0.0400f, 0.0900f, 0.1500f, 0.2200f, 0.3100f,
            0.4000f, 0.4700f, 0.5400f, 0.6100f, 0.6800f, 0.7600f,
            0.8400f, 0.9100f, 0.9500f, 0.9800f, 1.0000f};
    static const float colors[17][3] = {
            {0.1900f, 0.0718f, 0.2322f}, // violet blue
            {0.2511f, 0.2524f, 0.6337f}, // blue
            {0.2763f, 0.4212f, 0.8912f}, // blue
            {0.2586f, 0.5796f, 0.9988f}, // sky blue
            {0.1854f, 0.7421f, 0.9231f}, // cyan
            {0.2731f, 0.8655f, 0.7864f}, // soft cyan green
            {0.4223f, 0.9490f, 0.7010f}, // soft green
            {0.6260f, 0.9942f, 0.5987f}, // soft yellow green
            {0.7822f, 0.9900f, 0.5361f}, // soft yellow
            {0.8425f, 0.9303f, 0.4581f}, // soft yellow orange
            {0.9267f, 0.8522f, 0.4360f}, // soft orange
            {0.9916f, 0.7194f, 0.2971f}, // orange
            {0.9859f, 0.5091f, 0.1414f}, // orange red
            {0.9262f, 0.3247f, 0.0584f}, // red
            {0.8240f, 0.1918f, 0.0197f}, // dark red
            {0.6754f, 0.0898f, 0.0045f}, // dark red
            {0.4796f, 0.0158f, 0.0106f}  // darkest red
    };

    m_ColorBar->Reset();
    m_ColorBar->SetDimension(3);
    m_ColorBar->Reserve(17);
    m_ColorRange->Reset();
    m_ColorRange->Reserve(17);
    for (int i = 0; i < 17; ++i) {
        m_ColorBar->AddElement3(colors[i][0], colors[i][1], colors[i][2]);
        m_ColorRange->AddValue(positions[i]);
    }
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
