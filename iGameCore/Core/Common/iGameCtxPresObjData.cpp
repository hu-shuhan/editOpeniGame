#include "iGameCtxPresObjData.h"
#include "iGameCtxPresObjData.h"
#include <random>
#include <iGameThreadPool.h>
using namespace std;
IGAME_NAMESPACE_BEGIN

static std::vector<int> GenerateRandomSample(int maxNum, int getNum) {
    if (getNum <= 0 || maxNum <= 0) { return std::vector<int>(); }

    if (getNum >= maxNum) {
        std::vector<int> res;
        res.reserve(maxNum);
        for (int i = 0; i < maxNum; ++i) { res.push_back(i); }
        return res;
    }

    std::unordered_set<int> sample;
    sample.reserve(getNum);

    std::random_device rd;
    std::mt19937 gen(rd());

    for (int j = maxNum - getNum; j < maxNum; ++j) {
        std::uniform_int_distribution<int> distrib(0, j);
        int r = distrib(gen);
        if (sample.find(r) != sample.end()) {
            sample.insert(j);
        } else {
            sample.insert(r);
        }
    }
    std::vector<int> res(sample.begin(), sample.end());
    std::sort(res.begin(), res.end());
    return res;
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

void CtxPresObjData_Main::SetVariableNum(int variableNum) { m_VariableNum = variableNum; }

int CtxPresObjData_Main::GetVariableNum() const { return m_VariableNum; }

void CtxPresObjData_Main::SetVariableName(const std::vector<std::string>& variableName) { m_VariableName = variableName; }

const std::vector<std::string>& CtxPresObjData_Main::GetVariableName() { return m_VariableName; }

void CtxPresObjData_Main::SetObjectDatas(const std::vector<std::vector<double>>& objectDatas) {
    m_ObjectDatas = objectDatas;
}

const std::vector<std::vector<double>>& CtxPresObjData_Main::GetObjectDatas() { return m_ObjectDatas; }

void CtxPresObjData_Main::SetChoosedObjectDatas(const std::map<int, std::vector<double>>& choosedObjectDatas) {
    m_ChoosedObjectDatas = choosedObjectDatas;
}

void CtxPresObjData_Main::AddChoosedObjectData(int objId, const std::vector<double>& objData) {
    m_ChoosedObjectDatas[objId] = objData;
}

void CtxPresObjData_Main::RemoveChoosedObjectData(int objId) { m_ChoosedObjectDatas.erase(objId); }

void CtxPresObjData_Main::ClearChoosedObjectData() { m_ChoosedObjectDatas.clear(); }

const std::map<int, std::vector<double>>& CtxPresObjData_Main::GetChoosedObjectData() { return m_ChoosedObjectDatas; }

void CtxPresObjData_Main::SetMaxValueInVariables(const std::vector<double>& maxValueInVariables) {
    m_MaxValueInVariables = maxValueInVariables;
}

const std::vector<double>& CtxPresObjData_Main::GetMaxValueInVariables() { return m_MaxValueInVariables; }

void CtxPresObjData_Main::SetMinValueInVariables(const std::vector<double>& minValueInVariables) {
    m_MinValueInVariables = minValueInVariables;
}

const std::vector<double>& CtxPresObjData_Main::GetMinValueInVariables() { return m_MinValueInVariables; }

void CtxPresObjData_Main::SetDataTypeName(const std::string& name) { m_DataTypeName = name; }

const std::string& CtxPresObjData_Main::GetDataTypeName() { return m_DataTypeName; }

void CtxPresObjData_Main::SetDataType(IGenum dataType) { m_DataType = dataType; }

IGenum CtxPresObjData_Main::GetDataType() const { return m_DataType; }

std::vector<std::string> CtxPresObjData_Main::GenerateVariableNames(ElementArray<AttributeSet::Attribute>::Pointer attrs,
                                                               IGenum dataType) {
    vector<string> variableNames;
    for (int i = 0; i < attrs->Size(); i++) {
        auto& attr = attrs->GetElement(i);
        if (attr.attachmentType != dataType) continue;
        if (attr.pointer->GetDimension() == 1) {
            variableNames.push_back(attr.pointer->GetName());
            continue;
        }
        variableNames.push_back(attr.pointer->GetName() + "_magnitude");
        for (int j = 1; j <= attr.pointer->GetDimension(); j++) {
            variableNames.push_back(attr.pointer->GetName() + "_" + std::to_string(j));
        }
    }
    return variableNames;
}

std::vector<double> CtxPresObjData_Main::GenerateObjectData(ElementArray<AttributeSet::Attribute>::Pointer attrs, IGenum dataType,
                                                  int objId) {
    std::vector<double> objData;
    for (int attrIndex = 0; attrIndex < attrs->Size(); attrIndex++) {
        auto& attr = attrs->GetElement(attrIndex);
        if (attr.attachmentType != dataType) continue;
        if (attr.pointer->GetDimension() > 1) { objData.push_back(attr.pointer->GetElementValue(objId, -1)); }
        for (int dimensionIndex = 0; dimensionIndex < attr.pointer->GetDimension(); dimensionIndex++) {
            objData.push_back(attr.pointer->GetElementValue(objId, dimensionIndex));
        }
    }
    return objData;
}

std::vector<std::vector<double>>
CtxPresObjData_Main::GenerateObjectDatas(ElementArray<AttributeSet::Attribute>::Pointer attrs, IGenum dataType, int objNum,
                                    int maxObjNum) {
    std::vector<std::vector<double>> objDatas;
    auto randomObjIds = GenerateRandomSample(objNum, maxObjNum);
    for (auto& objIndex: randomObjIds) { objDatas.push_back(GenerateObjectData(attrs, dataType, objIndex)); }
    return objDatas;
}

std::map<int, std::vector<double>> CtxPresObjData_Main::GenerateChoosedObjectDatas(
        const std::map<Selection::Event::Type, std::map<igIndex, Selection::Event>>& selectedItems,
        ElementArray<AttributeSet::Attribute>::Pointer attrs, IGenum dataType) {
    std::map<int, std::vector<double>> re;
    switch (dataType) {
        case IG_POINT: {
            if (selectedItems.count(Selection::Event::Type::PickPoint) == 0) break;
            auto& selectedPoints = selectedItems.at(Selection::Event::Type::PickPoint);
            for (auto& point: selectedPoints) {
                auto pointId = point.first;
                re[pointId] = GenerateObjectData(attrs, dataType, pointId);
            }
            break;
        }
        case IG_CELL: {
            if (selectedItems.count(Selection::Event::Type::PickFace) == 0) break;
            auto& selectedCells = selectedItems.at(Selection::Event::Type::PickFace);
            for (auto& cell: selectedCells) {
                auto cellId = cell.first;
                re[cellId] = GenerateObjectData(attrs, dataType, cellId);
            }
            break;
        }
        default:
            break;
    }
    return re;
}

std::pair<std::vector<double>, std::vector<double>>
CtxPresObjData_Main::GenerateMinMaxData(ElementArray<AttributeSet::Attribute>::Pointer attrs, IGenum dataType) {
    vector<double> minData;
    vector<double> maxData;
    for (int attrIndex = 0; attrIndex < attrs->Size(); attrIndex++) {
        auto& attr = attrs->GetElement(attrIndex);
        if (attr.attachmentType != dataType) continue;
        auto dataRange = attr.GetDataRange();
        for (int valueIndex = dataRange->GetNumberOfValues() >= 6 ? 0 : 2; valueIndex < dataRange->GetNumberOfValues();
             valueIndex += 2) {
            minData.push_back(dataRange->GetValue(valueIndex));
            maxData.push_back(dataRange->GetValue(valueIndex + 1));
        }
    }
    return {minData, maxData};
}

std::string CtxPresObjData_Main::GenerateDataTypeName(IGenum dataType) {
    switch (dataType) {
        case IG_POINT:
            return "Point";
            break;
        case IG_CELL:
            return "Cell";
            break;
        default:
            return "";
            break;
    }
}

void CtxPresObjData_Draw::SetChoosedAlpha(int alpha) { m_ChoosedAlpha = alpha; }

int CtxPresObjData_Draw::GetChoosedAlpha() const { return m_ChoosedAlpha; }

void CtxPresObjData_Draw::SetUnChoosedAlpha(int alpha) { m_UnChoosedAlpha = alpha; }

int CtxPresObjData_Draw::GetUnChoosedAlpha() const { return m_UnChoosedAlpha; }

void CtxPresObjData_Draw::SetChoosedLight(int light) { m_ChoosedLight = light; }

int CtxPresObjData_Draw::GetChoosedLight() const { return m_ChoosedLight; }

void CtxPresObjData_Draw::SetUnChoosedLight(int light) { m_UnChoosedLight = light; }

int CtxPresObjData_Draw::GetUnChoosedLight() const { return m_UnChoosedLight; }

std::vector<std::vector<int>>
CtxPresObjData_Draw::GenerateObjectDrawSorts(int variableNum, const std::vector<std::vector<double>>& objcetValues) {
    std::vector<std::vector<int>> re(variableNum, std::vector<int>(objcetValues.size(), 0));
    for (int variableIndex = 0; variableIndex < variableNum; variableIndex++) {
        for (int objIndex = 0; objIndex < objcetValues.size(); objIndex++) { re[variableIndex][objIndex] = objIndex; }
    }
    for (int variableIndex = 0; variableIndex < variableNum; variableIndex++) {
        std::sort(re[variableIndex].begin(), re[variableIndex].end(), [&](int objIdA, int objIdB) {
            return objcetValues[objIdA][variableIndex] < objcetValues[objIdB][variableIndex];
        });
    }
    return re;
}

std::vector<std::vector<int>>
CtxPresObjData_Draw::GenerateObjectDrawSorts(int variableNum, const std::map<int, std::vector<double>>& objcetValues) {
    std::vector<std::vector<int>> re(variableNum);
    std::vector<int> objIds;
    for (auto& obj: objcetValues) { objIds.push_back(obj.first); }
    for (int variableIndex = 0; variableIndex < variableNum; variableIndex++) { re[variableIndex] = objIds; }
    for (int variableIndex = 0; variableIndex < variableNum; variableIndex++) {
        std::sort(re[variableIndex].begin(), re[variableIndex].end(), [&](int objIdA, int objIdB) {
            return objcetValues.at(objIdA)[variableIndex] < objcetValues.at(objIdB)[variableIndex];
        });
    }
    return re;
}

std::tuple<int, int, int> CtxPresObjData_Draw::GenerateDefaultColor(int brightNess) {
    return ChangeBrightness(1.0f, 0.0f, 0.0f, brightNess);
}

std::vector<std::tuple<int, int, int>>
CtxPresObjData_Draw::GenerateObjectColors(int variableIndex, const std::vector<std::vector<double>>& objDatas,
                                          const std::vector<double>& maxValues, const std::vector<double>& minValues,
                                          int brightness, ScalarsToColors::Pointer colorMap) {
    if (variableIndex < 0 || objDatas.empty() || objDatas.front().size() <= variableIndex) return {};
    float shift = 0 - minValues[variableIndex];
    float scale = 1.0 / (maxValues[variableIndex] - minValues[variableIndex]);
    vector<tuple<int, int, int>> re(objDatas.size());
    ThreadPool::parallelFor(0, objDatas.size(), [&](int st, int ed) {
        float rgb[3]{};
        for (int i = st; i < ed; i++) {
            auto& value = objDatas[i][variableIndex];
            colorMap->GetColor(value, rgb, shift, scale);
            re[i] = ChangeBrightness(rgb[0], rgb[1], rgb[2], brightness);
        }
    });
    return re;

    /*vector<tuple<int, int, int>> re;
    float rgb[3]{};
    for (auto& objData: objDatas) {
        auto& value = objData[variableIndex];
        colorMap->GetColor(value, rgb, shift, scale);
        re.push_back(ChangeBrightness(rgb[0], rgb[1], rgb[2], brightness));
    }
    return re;*/
}

std::map<int, std::tuple<int, int, int>>
CtxPresObjData_Draw::GenerateObjectColors(int variableIndex, const std::map<int, std::vector<double>>& objDatas,
                                          const std::vector<double>& maxValues, const std::vector<double>& minValues,
                                          int brightness, ScalarsToColors::Pointer colorMap) {
    std::map<int, std::tuple<int, int, int>> re;
    if (variableIndex < 0 || objDatas.empty() || objDatas.begin()->second.size() <= variableIndex) return re;
    float shift = 0 - minValues[variableIndex];
    float scale = 1.0 / (maxValues[variableIndex] - minValues[variableIndex]);
    float rgb[3]{};
    for (auto& objData: objDatas) {
        auto& value = objData.second[variableIndex];
        colorMap->GetColor(value, rgb, shift, scale);
        re[objData.first] = ChangeBrightness(rgb[0], rgb[1], rgb[2], brightness);
    }
    return re;
}


void CtxPresObjData_Draw::SetChoosedObjectColor(const std::map<int, std::tuple<int, int, int>>& objColor) {
    m_ChoosedObjectColor = objColor;
}

const std::map<int, std::tuple<int, int, int>>& CtxPresObjData_Draw::GetChoosedObjectColor() {
    return m_ChoosedObjectColor;
}

void CtxPresObjData_Draw::SetObjectColor(const std::vector<std::tuple<int, int, int>>& objColor) {
    m_ObjectColor = objColor;
}

const std::vector<std::tuple<int, int, int>>& CtxPresObjData_Draw::GetObjectColor() { return m_ObjectColor; }

void CtxPresObjData_Draw::SetChoosedDefaultColor(const std::tuple<int, int, int>& color) {
    m_ChoosedDefaultColor = color;
}

const std::tuple<int, int, int>& CtxPresObjData_Draw::GetChoosedDefaultColor() { return m_ChoosedDefaultColor; }

void CtxPresObjData_Draw::SetDefaultColor(const std::tuple<int, int, int>& color) { m_DefaultColor = color; }

const std::tuple<int, int, int>& CtxPresObjData_Draw::GetDefaultColor() { return m_DefaultColor; }

const std::tuple<int, int, int>& CtxPresObjData_Draw::GetObjectColor(bool choosed, int objId) {
    if (choosed) {
        if (m_ChoosedObjectColor.count(objId) == 0) return m_ChoosedDefaultColor;
        return m_ChoosedObjectColor.at(objId);
    } else {
        if (objId < 0 || m_ObjectColor.size() <= objId) return m_DefaultColor;
        return m_ObjectColor[objId];
    }
}

void CtxPresObjData_Draw::SetObjectDrawSorts(const std::vector<std::vector<int>>& drawSorts) {
    m_ObjDrawSortInVariables = drawSorts;
}

const std::vector<std::vector<int>>& CtxPresObjData_Draw::GetObjectDrawSorts() { return m_ObjDrawSortInVariables; }

void CtxPresObjData_Draw::SetChoosedObjectDrawSorts(const std::vector<std::vector<int>>& drawSorts) {
    m_ChoosedObjDrawSortInVariables = drawSorts;
}

const std::vector<std::vector<int>>& CtxPresObjData_Draw::GetChoosedObjDrawSorts() {
    return m_ChoosedObjDrawSortInVariables;
}

IGAME_NAMESPACE_END