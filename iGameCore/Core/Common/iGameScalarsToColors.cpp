#include "iGameScalarsToColors.h"
#include "iGameThreadPool.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>
#include <time.h>

IGAME_NAMESPACE_BEGIN

ScalarsToColors::ScalarsToColors() {
    this->Alpha = 1.0;
    this->VectorComponent = 0;
    this->VectorSize = -1;
    this->VectorMode = ScalarsToColors::COMPONENT;
    this->m_stable = false;
    this->m_AutoRangeMode = ScalarsToColors::EXACT_AUTO_RANGE;

    // only used in this class, not used in subclasses
    this->InputRange[0] = 0.0;
    this->InputRange[1] = 255.0;
    this->RGB[0] = 0;
    this->RGB[1] = 0;
    this->RGB[2] = 0;
    this->RGBABytes[0] = 0;
    this->RGBABytes[1] = 0;
    this->RGBABytes[2] = 0;
    this->RGBABytes[3] = 0;
}


ScalarsToColors::~ScalarsToColors() {}


void ScalarsToColors::SetVectorModeToComponent() { this->SetVectorMode(ScalarsToColors::COMPONENT); }


void ScalarsToColors::SetVectorModeToMagnitude() { this->SetVectorMode(ScalarsToColors::MAGNITUDE); }


void ScalarsToColors::SetVectorModeToRGBColors() { this->SetVectorMode(ScalarsToColors::RGBCOLORS); }


// do not use SetMacro() because we do not want the table to rebuild.
void ScalarsToColors::SetAlpha(float alpha) { this->Alpha = (alpha < 0.0 ? 0.0 : (alpha > 1.0 ? 1.0 : alpha)); }

void ScalarsToColors::InitRange(ArrayObject::Pointer input) {
    if (this->VectorMode == RGBCOLORS) return;
    this->SetVectorModeToMagnitude();
    InitRange(input, 0, -1);
}

void ScalarsToColors::InitRange(ArrayObject::Pointer input, int component) {
    if (this->VectorMode == RGBCOLORS) return;
    if (component < 0) { return InitRange(input); }
    this->SetVectorModeToComponent();
    InitRange(input, component, 1);
}

void ScalarsToColors::InitRangeRobust(ArrayObject::Pointer input) {
    if (this->VectorMode == RGBCOLORS) return;
    this->SetVectorModeToMagnitude();
    InitRangeRobust(input, 0, -1);
}

void ScalarsToColors::InitRangeRobust(ArrayObject::Pointer input, int component) {
    if (this->VectorMode == RGBCOLORS) return;
    if (component < 0) { return InitRangeRobust(input); }
    this->SetVectorModeToComponent();
    InitRangeRobust(input, component, 1);
}

void ScalarsToColors::InitRangeRobust(ArrayObject::Pointer input, int component, int size) {
    if (this->VectorMode == RGBCOLORS || input == nullptr || input->GetNumberOfElements() <= 0) return;

    int vectorMode = this->GetVectorMode();
    const int inComponent = input->GetDimension();
    if (inComponent <= 0) return;

    if (vectorMode == COMPONENT) {
        if (component == -1) { component = this->GetVectorComponent(); }
        if (component < 0) { component = 0; }
        if (component >= inComponent) { component = inComponent - 1; }
    }
    if (vectorMode == MAGNITUDE) {
        if (size == -1) { size = this->GetVectorSize(); }
        if (size <= 0) {
            component = 0;
            size = inComponent;
        } else {
            if (component < 0) { component = 0; }
            if (component >= inComponent) { component = inComponent - 1; }
            if (component + size > inComponent) { size = inComponent - component; }
        }
        if (size == 1) { vectorMode = COMPONENT; }
    }

    constexpr std::size_t sampleLimit = 65536u;
    constexpr double lowerQuantile = 0.01;
    constexpr double upperQuantile = 0.99;
    const auto elementCount = static_cast<std::size_t>(input->GetNumberOfElements());
    const auto sampleCount = std::min(elementCount, sampleLimit);
    std::vector<float> samples;
    samples.reserve(sampleCount);

    std::vector<float> data(static_cast<std::size_t>(inComponent));
    const auto readValue = [&](const std::size_t elementIndex, double& value) {
        input->GetElement(static_cast<IGsize>(elementIndex), data.data());
        if (vectorMode == COMPONENT) {
            value = data[component];
        } else if (vectorMode == MAGNITUDE) {
            double squaredMagnitude = 0.0;
            for (int valueIndex = component; valueIndex < component + size; ++valueIndex) {
                const double currentValue = data[valueIndex];
                squaredMagnitude += currentValue * currentValue;
            }
            value = std::sqrt(squaredMagnitude);
        } else {
            return false;
        }
        return std::isfinite(value);
    };
    for (std::size_t sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex) {
        const auto elementIndex = sampleCount <= 1u
            ? 0u
            : sampleIndex * (elementCount - 1u) / (sampleCount - 1u);
        double value = 0.0;
        if (readValue(elementIndex, value)) { samples.push_back(static_cast<float>(value)); }
    }

    if (samples.empty()) return;
    std::sort(samples.begin(), samples.end());
    const auto lastIndex = samples.size() - 1u;
    const auto lowerIndex = static_cast<std::size_t>(std::floor(lowerQuantile * lastIndex));
    const auto upperIndex = static_cast<std::size_t>(std::ceil(upperQuantile * lastIndex));
    double minValue = samples[lowerIndex];
    double maxValue = samples[upperIndex];

    if (!(minValue < maxValue)) {
        double actualMin = std::numeric_limits<double>::infinity();
        double actualMax = -std::numeric_limits<double>::infinity();
        std::size_t nonZeroCount = 0u;
        for (std::size_t elementIndex = 0u; elementIndex < elementCount; ++elementIndex) {
            double value = 0.0;
            if (!readValue(elementIndex, value)) { continue; }
            actualMin = std::min(actualMin, value);
            actualMax = std::max(actualMax, value);
            if (value != 0.0) { ++nonZeroCount; }
        }

        if (actualMin < actualMax) {
            minValue = actualMin;
            maxValue = actualMax;
            if (nonZeroCount > 1u) {
                const auto nonZeroSampleCount = std::min(nonZeroCount, sampleLimit);
                std::vector<float> nonZeroSamples;
                nonZeroSamples.reserve(nonZeroSampleCount);
                std::size_t nonZeroOrdinal = 0u;
                std::size_t targetSample = 0u;
                for (std::size_t elementIndex = 0u;
                     elementIndex < elementCount && targetSample < nonZeroSampleCount;
                     ++elementIndex) {
                    double value = 0.0;
                    if (!readValue(elementIndex, value) || value == 0.0) { continue; }
                    const auto targetOrdinal = nonZeroSampleCount <= 1u
                        ? 0u
                        : targetSample * (nonZeroCount - 1u) / (nonZeroSampleCount - 1u);
                    if (nonZeroOrdinal == targetOrdinal) {
                        nonZeroSamples.push_back(static_cast<float>(value));
                        ++targetSample;
                    }
                    ++nonZeroOrdinal;
                }
                std::sort(nonZeroSamples.begin(), nonZeroSamples.end());
                if (nonZeroSamples.size() > 1u) {
                    const auto nonZeroLastIndex = nonZeroSamples.size() - 1u;
                    const auto nonZeroLowerIndex = static_cast<std::size_t>(
                        std::floor(lowerQuantile * nonZeroLastIndex));
                    const auto nonZeroUpperIndex = static_cast<std::size_t>(
                        std::ceil(upperQuantile * nonZeroLastIndex));
                    const double nonZeroMin = nonZeroSamples[nonZeroLowerIndex];
                    const double nonZeroMax = nonZeroSamples[nonZeroUpperIndex];
                    if (nonZeroMin < nonZeroMax) {
                        minValue = nonZeroMin;
                        maxValue = nonZeroMax;
                    }
                }
            }
        } else if (std::isfinite(actualMin)) {
            const double padding = std::max(std::abs(actualMin) * 1e-6, 1e-6);
            minValue = actualMin - padding;
            maxValue = actualMin + padding;
        }
    }
    this->SetRange(minValue, maxValue);
}

void ScalarsToColors::SetRange(double minval, double maxval) {
    if (this->InputRange[0] != minval || this->InputRange[1] != maxval) {
        this->InputRange[0] = minval;
        this->InputRange[1] = maxval;
        this->Modified();
    }
}

double* ScalarsToColors::GetRange() { return this->InputRange; }

void ScalarsToColors::ComputeShiftScale(float& shift, float& scale) {
    constexpr float minscale = -1e17;
    constexpr float maxscale = 1e17;
    const double* range = this->GetRange();
    shift = -range[0];
    scale = range[1] - range[0];
    if (scale * scale > 1e-30) {
        scale = 1.0 / scale;
    } else {
        scale = (scale < 0.0 ? minscale : maxscale);
    }
}

void ScalarsToColors::GetColor(float v, float rgb[3], float& shift, float& scale) {
    constexpr float minval = 0.0;
    constexpr float maxval = 0.999999;
    float val = (v + shift) * scale;
    val = (val > minval ? val : minval);
    val = (val < maxval ? val : maxval);
    MapColor(val, rgb);
}


const unsigned char* ScalarsToColors::MapValue(float v, float& shift, float& scale) {
    float rgb[3];
    this->GetColor(v, rgb, shift, scale);
    float alpha = this->GetOpacity(v);
    this->RGBABytes[0] = ColorToUChar(rgb[0]);
    this->RGBABytes[1] = ColorToUChar(rgb[1]);
    this->RGBABytes[2] = ColorToUChar(rgb[2]);
    this->RGBABytes[3] = ColorToUChar(alpha);
    return this->RGBABytes;
}
const float* ScalarsToColors::MapValueToRGB(float v, float& shift, float& scale) {
    this->GetColor(v, this->RGB, shift, scale);
    return this->RGB;
}

void ScalarsToColors::MapValueToRGB(float v, float* rgb, float& shift, float& scale) {
    this->GetColor(v, rgb, shift, scale);
}


void ScalarsToColors::InitRange(ArrayObject::Pointer input, int component, int size) {
    if (this->VectorMode == RGBCOLORS) return;
    float minv = FLT_MAX;
    float maxv = -FLT_MAX;
    int vectorMode = this->GetVectorMode();
    int inComponent = input->GetDimension();
    if (vectorMode == COMPONENT) {
        if (component == -1) { component = this->GetVectorComponent(); }
        if (component < 0) { component = 0; }
        if (component >= inComponent) { component = inComponent - 1; }
    }
    if (vectorMode == MAGNITUDE) {
        if (size == -1) { size = this->GetVectorSize(); }
        if (size <= 0) {
            component = 0;
            size = inComponent;
        } else {
            if (component < 0) { component = 0; }
            if (component >= inComponent) { component = inComponent - 1; }
            if (component + size > inComponent) { size = inComponent - component; }
        }
        if (size == 1) { vectorMode = COMPONENT; }
    }
    switch (vectorMode) {
        case COMPONENT: {
            float data[128];
            for (int i = 0; i < input->GetNumberOfElements(); i++) {
                input->GetElement(i, data);
                minv = std::min(minv, data[component]);
                maxv = std::max(maxv, data[component]);
            }
        } break;
        case MAGNITUDE: {
            float data[128];
            for (int i = 0; i < input->GetNumberOfElements(); i++) {
                input->GetElement(i, data);
                float sum = 0.0;
                for (int j = component; j < component + size; j++) { sum += data[j] * data[j]; }
                sum = sqrt(sum);
                minv = std::min(minv, sum);
                maxv = std::max(maxv, sum);
            }
        } break;
        default:
            break;
    }
    //std::cout << minv << " " << maxv << std::endl;
    this->SetRange(minv, maxv);
}
FloatArray::Pointer ScalarsToColors::MapScalars(ArrayObject::Pointer scalars, int component, int outputFormat) {
    //component::渲染第几个维度
    int numberOfComponents = scalars->GetDimension();
    FloatArray::Pointer newColors = FloatArray::New();
    newColors->SetDimension(outputFormat);
    newColors->Resize(scalars->GetNumberOfElements());
    if (this->VectorMode == RGBCOLORS) {
        this->MapVectorsToColors(scalars, newColors, outputFormat);
        return newColors;
    }
    if (component < 0 && numberOfComponents > 1) {
        this->SetVectorModeToMagnitude();
        this->MapVectorsToColors(scalars, newColors, outputFormat);
    } else {
        if (component < 0) { component = 0; }
        if (component >= numberOfComponents) { component = numberOfComponents - 1; }
        // Map the scalars to colors
        this->MapVectorsToColors(scalars, newColors, outputFormat, component, 1);
    }
    return newColors;
}

// Map a set of vector values through the table
void ScalarsToColors::MapVectorsToColors(ArrayObject::Pointer input, FloatArray::Pointer output, int outputFormat,
                                         int vectorComponent, int vectorSize) {
    clock_t time1 = clock();
    int inComponents = input->GetDimension();
    int vectorMode = this->GetVectorMode();
    if (vectorMode == COMPONENT) {
        if (vectorComponent == -1) { vectorComponent = this->GetVectorComponent(); }
        if (vectorComponent < 0) { vectorComponent = 0; }
        if (vectorComponent >= inComponents) { vectorComponent = inComponents - 1; }
    } else {
        if (vectorSize == -1) { vectorSize = this->GetVectorSize(); }
        if (vectorSize <= 0) {
            vectorComponent = 0;
            vectorSize = inComponents;
        } else {
            if (vectorComponent < 0) { vectorComponent = 0; }
            if (vectorComponent >= inComponents) { vectorComponent = inComponents - 1; }
            if (vectorComponent + vectorSize > inComponents) { vectorSize = inComponents - vectorComponent; }
        }
        if (vectorMode == MAGNITUDE && (inComponents == 1 || vectorSize == 1)) { vectorMode = COMPONENT; }
    }
    // map according to the current vector mode
    switch (vectorMode) {
        case ScalarsToColors::COMPONENT: {
            int index = vectorComponent;
            float shift, scale;
            ComputeShiftScale(shift, scale);
            auto func = [&](igIndex start, igIndex end) -> void {
                float data[128];
                float rgb[3];
                for (int i = start; i < end; i++) {
                    input->GetElement(i, data);
                    MapValueToRGB(data[index], rgb, shift, scale);
                    //const unsigned char* rgb = MapValue(data[index], shift, scale);
                    //std::array<unsigned char, 3>tmp = { rgb[0], rgb[1], rgb[2] };
                    output->SetElement(i, rgb);
                }
            };
            ThreadPool::parallelFor(0, input->GetNumberOfElements(), func);
        } break;
        case ScalarsToColors::MAGNITUDE: {
            int index = vectorComponent;
            float shift, scale;
            ComputeShiftScale(shift, scale);
            auto func = [&](igIndex start, igIndex end) -> void {
                float data[128];
                float rgb[3];
                for (int i = start; i < end; i++) {
                    input->GetElement(i, data);
                    float value = 0.0;
                    for (int j = index; j < index + vectorSize; j++) { value += data[j] * data[j]; }
                    value = sqrt(value);
                    MapValueToRGB(value, rgb, shift, scale);
                    output->SetElement(i, rgb);
                }
            };
            ThreadPool::parallelFor(0, input->GetNumberOfElements(), func);

        } break;
        case ScalarsToColors::RGBCOLORS: {
            if (inComponents < 3) return;
            //std::array<unsigned char, 3> rgb;
            auto func = [&](igIndex start, igIndex end) -> void {
                float data[128];
                for (int i = start; i < end; i++) {
                    input->GetElement(i, data);
                    output->SetElement(i, data);
                }
            };
            ThreadPool::parallelFor(0, input->GetNumberOfElements(), func);
        } break;
    }
    //clock_t time2 = clock();
    //std::cout << "map cost " << time2 - time1 << "ms\n";
}

bool ScalarsToColors::DeepCopy(ScalarsToColors::Pointer other) {
    if (!ColorMap::DeepCopy(other)) { return false; }
    if (other == nullptr) { return false; }

    // Copy simple scalar fields
    this->Alpha = other->Alpha;
    this->VectorMode = other->VectorMode;
    this->VectorComponent = other->VectorComponent;
    this->VectorSize = other->VectorSize;
    this->m_stable = other->m_stable;
    this->m_AutoRangeMode = other->m_AutoRangeMode;

    // Copy range and last RGB/RGBA buffers
    this->InputRange[0] = other->InputRange[0];
    this->InputRange[1] = other->InputRange[1];
    this->RGB[0] = other->RGB[0];
    this->RGB[1] = other->RGB[1];
    this->RGB[2] = other->RGB[2];
    this->RGBABytes[0] = other->RGBABytes[0];
    this->RGBABytes[1] = other->RGBABytes[1];
    this->RGBABytes[2] = other->RGBABytes[2];
    this->RGBABytes[3] = other->RGBABytes[3];

    // Copy name if needed (Object base class holds m_Name)
    this->SetName(other->GetName());

    this->Modified();
    return true;
}

IGAME_NAMESPACE_END
