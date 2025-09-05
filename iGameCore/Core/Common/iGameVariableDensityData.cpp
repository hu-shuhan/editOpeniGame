#include "iGameVariableDensityData.h"
#include <cmath>
#include <algorithm>
#include <iGameThreadPool.h>
#include <mutex>
using namespace std;
IGAME_NAMESPACE_BEGIN

static std::pair<double, double> CalculateCopyRange(int copyNum, int copyIndex, double minValue, double maxValue) {
    if (maxValue == minValue) { return {minValue, minValue}; }

    double binWidth = (maxValue - minValue) / copyNum;
    double start = minValue + copyIndex * binWidth;
    double end = (copyIndex == copyNum - 1) ? maxValue : (start + binWidth);

    return {start, end};
}

const float EPSILON = 1e-6f;

static void rgbToHsb(float r, float g, float b, float& h, float& s, float& bVal) {
    float max_c = std::max({r, g, b});
    float min_c = std::min({r, g, b});
    float delta = max_c - min_c;

    bVal = max_c;

    if (max_c != 0.0f) {
        s = delta / max_c;
    } else {
        s = 0.0f;
        h = 0.0f;
        return;
    }

    if (delta == 0.0f) {
        h = 0.0f;
        return;
    }

    if (max_c == r) {
        h = 60.0f * std::fmod((g - b) / delta, 6.0f);
    } else if (max_c == g) {
        h = 60.0f * ((b - r) / delta + 2.0f);
    } else { // max_c == b
        h = 60.0f * ((r - g) / delta + 4.0f);
    }
}

static void hsbToRgb(float h, float s, float bVal, float& r, float& g, float& b) {
    float c = bVal * s;
    float x = c * (1.0f - std::fabs(std::fmodf(h / 60.0f, 2.0f) - 1.0f));
    float m = bVal - c;

    float r1, g1, b1;

    if (h >= 0.0f - EPSILON && h < 60.0f + EPSILON) {
        r1 = c + m;
        g1 = x + m;
        b1 = 0.0f + m;
    } else if (h >= 60.0f - EPSILON && h < 120.0f + EPSILON) {
        r1 = x + m;
        g1 = c + m;
        b1 = 0.0f + m;
    } else if (h >= 120.0f - EPSILON && h < 180.0f + EPSILON) {
        r1 = 0.0f + m;
        g1 = c + m;
        b1 = x + m;
    } else if (h >= 180.0f - EPSILON && h < 240.0f + EPSILON) {
        r1 = 0.0f + m;
        g1 = x + m;
        b1 = c + m;
    } else if (h >= 240.0f - EPSILON && h < 300.0f + EPSILON) {
        r1 = x + m;
        g1 = 0.0f + m;
        b1 = c + m;
    } else {
        r1 = c + m;
        g1 = 0.0f + m;
        b1 = x + m;
    }

    r = r1;
    g = g1;
    b = b1;
}

static void ChangeRgbBrightness(float& r, float& g, float& b, float targetBrightness) {
    float h, s, currentB;

    rgbToHsb(r, g, b, h, s, currentB);

    if (std::fabs(currentB - targetBrightness) < EPSILON) { return; }

    float newB = std::max(0.0f, std::min(1.0f, targetBrightness));

    hsbToRgb(h, s, newB, r, g, b);
}

static tuple<int, int, int> ChangeBrightness(float r, float g, float b, int brightness) {
    float targetBrightness = (float) brightness / 255.0;
    ChangeRgbBrightness(r, g, b, targetBrightness);
    int ir = r * 255;
    int ig = g * 255;
    int ib = b * 255;
    return {ir, ig, ib};
}

void VariableDensityData::SetCopyNum(int copyNum) { m_CopyNum = copyNum; }

int VariableDensityData::GetCopyNum() const { return m_CopyNum; }

void VariableDensityData::SetChoosedObjectIndexs(const std::set<int>& objIds) { m_ChoosedObjIndexs = objIds; }

void VariableDensityData::AddChoosedObjectIndex(int objId) { m_ChoosedObjIndexs.insert(objId); }

void VariableDensityData::RemoveChoosedObjectIndex(int objId) { m_ChoosedObjIndexs.erase(objId); }

void VariableDensityData::ClearChoosedObjectIndex() { m_ChoosedObjIndexs.clear(); }

const std::set<int>& VariableDensityData::GetChoosedObjectIndexs() { return m_ChoosedObjIndexs; }

void VariableDensityData::SetDensity(const std::vector<std::vector<int>>& density) { m_Density = density; }

const std::vector<std::vector<int>>& VariableDensityData::GetDensity() { return m_Density; }

void VariableDensityData::SetChoosedDensity(const std::vector<std::vector<int>>& density) {
    m_ChoosedDensity = density;
}

const std::vector<std::vector<int>>& VariableDensityData::GetChoosedDensity() { return m_ChoosedDensity; }

void VariableDensityData::SetDensityColor(
        const std::vector<std::pair<std::tuple<int, int, int>, std::tuple<int, int, int>>>& densityColor) {
    m_DensityColor = densityColor;
}

const std::vector<std::pair<std::tuple<int, int, int>, std::tuple<int, int, int>>>&
VariableDensityData::GetDensityColor() {
    return m_DensityColor;
}

void VariableDensityData::SetChoosedDensityColor(
        const std::vector<std::pair<std::tuple<int, int, int>, std::tuple<int, int, int>>>& densityColor) {
    m_ChoosedDensityColor = densityColor;
}

const std::vector<std::pair<std::tuple<int, int, int>, std::tuple<int, int, int>>>&
VariableDensityData::GetChoosedDensityColor() {
    return m_ChoosedDensityColor;
}

int VariableDensityData::CalculateCopyIndexByValue(int copyNum, double value, double maxValue, double minValue) {
    if (maxValue == minValue) { return copyNum / 2; }
    double binWidth = (maxValue - minValue) / copyNum;

    int index = static_cast<int>((value - minValue) / binWidth);

    if (index < 0) return 0;
    if (index >= copyNum) return copyNum - 1;

    return index;
}

std::vector<std::vector<int>>
VariableDensityData::GenerateDensity(int variableNum, int copyNum, const std::vector<double>& maxValueInVariables,
                                          const std::vector<double>& minValueInVariables,
                                          const std::vector<std::vector<double>>& objDatas) {

    if (variableNum <= 0 || copyNum <= 0 || maxValueInVariables.size() < static_cast<size_t>(variableNum) ||
        minValueInVariables.size() < static_cast<size_t>(variableNum)) {
        return {};
    }

    std::vector<std::vector<int>> counts(variableNum, std::vector<int>(copyNum, 0));
    size_t objCount = objDatas.size();
    if (objCount == 0) return counts;

    for (size_t objIdx = 0; objIdx < objCount; ++objIdx) {
        const std::vector<double>& objData = objDatas[objIdx];
        for (int v = 0; v < variableNum; ++v) {
            int binIdx = CalculateCopyIndexByValue(copyNum, objData[v], maxValueInVariables[v], minValueInVariables[v]);
            ++counts[v][binIdx];
        }
    }
    return counts;
}

std::vector<std::vector<int>> VariableDensityData::GenerateDensity(int variableNum, int copyNum,
                                                                   const std::vector<double>& maxValueInVariables,
                                                                   const std::vector<double>& minValueInVariables,
                                                                   ElementArray<AttributeSet::Attribute>::Pointer attrs,
                                                                   IGenum dataType, int objCount) {

    if (variableNum <= 0 || copyNum <= 0 || maxValueInVariables.size() < static_cast<size_t>(variableNum) ||
        minValueInVariables.size() < static_cast<size_t>(variableNum)) {
        return {};
    }

    std::vector<std::vector<int>> counts(variableNum, std::vector<int>(copyNum, 0));
    if (objCount == 0) return counts;

    
    static mutex CountMutex;
    ThreadPool::parallelFor(
            0, objCount,
            [&](int st, int ed) {
                std::vector<std::vector<int>> tempCounts(variableNum, std::vector<int>(copyNum, 0));
                for (int objIdx = st; objIdx < ed; ++objIdx) {
                    int v{};
                    for (int attrIndex = 0; attrIndex < attrs->Size(); attrIndex++) {
                        auto& attr = attrs->GetElement(attrIndex);
                        if (attr.attachmentType != dataType) continue;
                        if (attr.pointer->GetDimension() > 1) {
                            int binIdx = CalculateCopyIndexByValue(copyNum, attr.pointer->GetElementValue(objIdx, -1),
                                                                   maxValueInVariables[v], minValueInVariables[v]);
                            ++tempCounts[v][binIdx];
                            ++v;
                        }
                        for (int dimensionIndex = 0; dimensionIndex < attr.pointer->GetDimension(); dimensionIndex++) {
                            int binIdx = CalculateCopyIndexByValue(
                                    copyNum, attr.pointer->GetElementValue(objIdx, dimensionIndex),
                                    maxValueInVariables[v], minValueInVariables[v]);
                            ++tempCounts[v][binIdx];
                            ++v;
                        }
                    }
                }
                //add to counts
                std::lock_guard lg(CountMutex);
                for (int variableIndex = 0; variableIndex < variableNum; variableIndex++) {
                    for (int copyIndex = 0; copyIndex < copyNum; copyIndex++) {
                        counts[variableIndex][copyIndex] += tempCounts[variableIndex][copyIndex];
                    }
                }
            },
            std::max<int>(1, pow(objCount, 0.25)));
    return counts;
    //for (int objIdx = 0; objIdx < objCount; ++objIdx) {
    //    int v{};
    //    for (int attrIndex = 0; attrIndex < attrs->Size(); attrIndex++) {
    //        auto& attr = attrs->GetElement(attrIndex);
    //        if (attr.attachmentType != dataType) continue;
    //        if (attr.pointer->GetDimension() > 1) {
    //            int binIdx = CalculateCopyIndexByValue(copyNum, attr.pointer->GetElementValue(objIdx, -1),
    //                                                   maxValueInVariables[v], minValueInVariables[v]);
    //            ++counts[v][binIdx];
    //            ++v;
    //        }
    //        for (int dimensionIndex = 0; dimensionIndex < attr.pointer->GetDimension(); dimensionIndex++) {
    //            int binIdx = CalculateCopyIndexByValue(copyNum, attr.pointer->GetElementValue(objIdx, dimensionIndex),
    //                                                   maxValueInVariables[v], minValueInVariables[v]);
    //            ++counts[v][binIdx];
    //            ++v;
    //        }
    //    }
    //}
    //return counts;
}

std::vector<std::vector<int>>
VariableDensityData::GenerateDensity(int variableNum, int copyNum, const std::vector<double>& maxValueInVariables,
                                          const std::vector<double>& minValueInVariables,
                                          const std::map<int, std::vector<double>>& objDatas) {
    if (variableNum <= 0 || copyNum <= 0 || maxValueInVariables.size() < static_cast<size_t>(variableNum) ||
        minValueInVariables.size() < static_cast<size_t>(variableNum)) {
        return {};
    }

    std::vector<std::vector<int>> counts(variableNum, std::vector<int>(copyNum, 0));
    if (objDatas.empty()) return counts;

    for (const auto& entry: objDatas) {
        const std::vector<double>& objData = entry.second;
        for (int v = 0; v < variableNum; ++v) {
            int binIdx = CalculateCopyIndexByValue(copyNum, objData[v], maxValueInVariables[v], minValueInVariables[v]);
            ++counts[v][binIdx];
        }
    }
    return counts;
}

std::vector<std::vector<int>> VariableDensityData::GenerateDensity(int variableNum, int copyNum,
                                                                   const std::vector<double>& maxValueInVariables,
                                                                   const std::vector<double>& minValueInVariables,
                                                                   ElementArray<AttributeSet::Attribute>::Pointer attrs,
                                                                   IGenum dataType, const std::set<int>& objIndexs) {

    if (variableNum <= 0 || copyNum <= 0 || maxValueInVariables.size() < static_cast<size_t>(variableNum) ||
        minValueInVariables.size() < static_cast<size_t>(variableNum)) {
        return {};
    }

    std::vector<std::vector<int>> counts(variableNum, std::vector<int>(copyNum, 0));
    if (objIndexs.size() == 0) return counts;

    for (auto& objIdx: objIndexs) {
        int v{};
        for (int attrIndex = 0; attrIndex < attrs->Size(); attrIndex++) {
            auto& attr = attrs->GetElement(attrIndex);
            if (attr.attachmentType != dataType) continue;
            if (attr.pointer->GetDimension() > 1) {
                int binIdx = CalculateCopyIndexByValue(copyNum, attr.pointer->GetElementValue(objIdx, -1),
                                                       maxValueInVariables[v], minValueInVariables[v]);
                ++counts[v][binIdx];
                ++v;
            }
            for (int dimensionIndex = 0; dimensionIndex < attr.pointer->GetDimension(); dimensionIndex++) {
                int binIdx = CalculateCopyIndexByValue(copyNum, attr.pointer->GetElementValue(objIdx, dimensionIndex),
                                                       maxValueInVariables[v], minValueInVariables[v]);
                ++counts[v][binIdx];
                ++v;
            }
        }
    }
    return counts;
}

std::vector<std::vector<int>> VariableDensityData::GenerateDefaultDensity(int variableNum, int copyNum) {
    return std::vector<std::vector<int>>(variableNum, std::vector<int>(copyNum, 0));
}

std::vector<std::pair<std::tuple<int, int, int>, std::tuple<int, int, int>>>
VariableDensityData::GenerateDensityColor(int copyNum, int brightness, ScalarsToColors::Pointer colorMap) {
    std::vector<std::pair<std::tuple<int, int, int>, std::tuple<int, int, int>>> re(copyNum);
    static constexpr double minValue = 0.0;
    static constexpr double maxValue = 1.0;
    float shift = 0 - minValue;
    float scale = 1.0 / (maxValue - minValue);
    float rgbSt[3]{};
    float rgbEd[3]{};
    for (int copyIndex = 0; copyIndex < copyNum; copyIndex++) {
        auto [st, ed] = CalculateCopyRange(copyNum, copyIndex, minValue, maxValue);
        colorMap->GetColor(st, rgbSt, shift, scale);
        colorMap->GetColor(ed, rgbEd, shift, scale);
        re[copyIndex] = {ChangeBrightness(rgbSt[0], rgbSt[1], rgbSt[2], brightness),
                         ChangeBrightness(rgbEd[0], rgbEd[1], rgbEd[2], brightness)};
    }
    return re;
}

std::vector<std::pair<std::tuple<int, int, int>, std::tuple<int, int, int>>>
VariableDensityData::GenerateDefaultDensityColor(int copyNum) {
    return std::vector<std::pair<std::tuple<int, int, int>, std::tuple<int, int, int>>>(
            copyNum, std::pair<std::tuple<int, int, int>, std::tuple<int, int, int>>(
                             std::tuple<int, int, int>(0, 0, 0), std::tuple<int, int, int>(0, 0, 0)));
}

std::set<int> VariableDensityData::GenerateChoosedObjectIndexs(
        const std::map<Selection::Event::Type, std::map<igIndex, Selection::Event>>& selectedItems, IGenum dataType) {
    std::set<int> re;
    switch (dataType) {
        case IG_POINT: {
            if (selectedItems.count(Selection::Event::Type::PickPoint) == 0) break;
            auto& selectedPoints = selectedItems.at(Selection::Event::Type::PickPoint);
            for (auto& point: selectedPoints) {
                auto pointId = point.first;
                re.insert(pointId);
            }
            break;
        }
        case IG_CELL: {
            if (selectedItems.count(Selection::Event::Type::PickFace) == 0) break;
            auto& selectedCells = selectedItems.at(Selection::Event::Type::PickFace);
            for (auto& cell: selectedCells) {
                auto cellId = cell.first;
                re.insert(cellId);
            }
            break;
        }
        default:
            break;
    }
    return re;
}

VariableDensityData::Pointer
VariableDensityData::New(ElementArray<AttributeSet::Attribute>::Pointer attrs, IGenum dataType,
                         const std::map<Selection::Event::Type, std::map<igIndex, Selection::Event>>& selectedItems,
                         int objNum, int boxNum, ScalarsToColors::Pointer colorMap) {
    auto variableNames = VariableDensityData::GenerateVariableNames(attrs, dataType);
    int variableNum = variableNames.size();
    if (variableNum == 0) return VariableDensityData::Pointer();
    auto Data = VariableDensityData::New();
    Data->SetCopyNum(boxNum);
    Data->SetVariableNum(variableNum);
    Data->SetVariableName(variableNames);
    auto variableIndex = VariableDensityData::GenerateVariableIndex(attrs, dataType);
    Data->SetVariableIndex(variableIndex);
    auto choosedObjIds = VariableDensityData::GenerateChoosedObjectIndexs(selectedItems, dataType);
    Data->SetChoosedObjectIndexs(choosedObjIds);
    auto [minValue, maxValue] = VariableDensityData::GenerateMinMaxData(attrs, dataType);
    Data->SetMinValueInVariables(minValue);
    Data->SetMaxValueInVariables(maxValue);
    auto density =
            VariableDensityData::GenerateDensity(variableNum, boxNum, maxValue, minValue, attrs, dataType, objNum);
    Data->SetDensity(density);
    auto choosedDensity = VariableDensityData::GenerateDensity(variableNum, boxNum, maxValue, minValue, attrs, dataType,
                                                               choosedObjIds);
    Data->SetChoosedDensity(choosedDensity);
    Data->SetDensityColor(VariableDensityData::GenerateDensityColor(boxNum, Data->GetUnChoosedLight(), colorMap));
    Data->SetChoosedDensityColor(VariableDensityData::GenerateDensityColor(boxNum, Data->GetChoosedLight(), colorMap));
    Data->SetDataType(dataType);
    Data->SetDataTypeName(VariableDensityData::GenerateDataTypeName(dataType));
    Data->SetUnChoosedAlpha(255);
    return Data;
}

VariableDensityData::Pointer VariableDensityData::New(ElementArray<AttributeSet::Attribute>::Pointer attrs,
                                                      IGenum dataType, int boxNum) {
    auto variableNames = VariableDensityData::GenerateVariableNames(attrs, dataType);
    int variableNum = variableNames.size();
    if (variableNum == 0) return VariableDensityData::Pointer();
    int objNum = VariableDensityData::GetLegalAttrsObjNum(attrs, dataType);
    auto Data = VariableDensityData::New();
    Data->SetCopyNum(boxNum);
    Data->SetVariableNum(variableNum);
    Data->SetVariableName(variableNames);
    auto variableIndex = VariableDensityData::GenerateVariableIndex(attrs, dataType);
    Data->SetVariableIndex(variableIndex);
    auto [minValue, maxValue] = VariableDensityData::GenerateMinMaxData(attrs, dataType);
    Data->SetMinValueInVariables(minValue);
    Data->SetMaxValueInVariables(maxValue);
    auto density =
            VariableDensityData::GenerateDensity(variableNum, boxNum, maxValue, minValue, attrs, dataType, objNum);
    Data->SetDensity(density);
    auto choosedDensity = VariableDensityData::GenerateDefaultDensity(variableNum, boxNum);
    Data->SetChoosedDensity(choosedDensity);
    Data->SetDensityColor(VariableDensityData::GenerateDefaultDensityColor(boxNum));
    Data->SetChoosedDensityColor(VariableDensityData::GenerateDefaultDensityColor(boxNum));
    Data->SetDataType(dataType);
    Data->SetDataTypeName(VariableDensityData::GenerateDataTypeName(dataType));
    Data->SetUnChoosedAlpha(255);
    return Data;
}

std::vector<igIndex> VariableDensityData::FiltInRangeIds(int _variableIndex, double variableMinValue,
                                                         double variableMaxValue,
                                                         ElementArray<AttributeSet::Attribute>::Pointer attrs,
                                                         int objNum) {
    std::vector<igIndex> ids;
    static mutex IDS_MUTEX;
    ThreadPool::parallelFor(0, objNum, [&](int st, int ed) {
        std::vector<igIndex> tempIds;
        for (int objId = st; objId < ed; objId++) {
            auto& variableIndex = this->GetVariableIndex()[_variableIndex];
            auto value = attrs->GetElement(variableIndex.first).pointer->GetElementValue(objId, variableIndex.second);
            if (variableMinValue <= value && value <= variableMaxValue) {
                tempIds.push_back(objId);
                continue;
            }
        }
        lock_guard lg(IDS_MUTEX);
        ids.insert(ids.end(), tempIds.begin(), tempIds.end());
    });
    return ids;
}
IGAME_NAMESPACE_END