#include "iGameCtxPresObjData.h"
#include <iGameThreadPool.h>
#include <random>
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
    float x = c * (1.0f - std::fabs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
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

void CtxPresObjData_Main::SetAttributes(ElementArray<AttributeSet::Attribute>::Pointer attrs) { m_Attrs = attrs; }

void CtxPresObjData_Main::SetObjectNum(int objNum) { m_ObjNum = objNum; }

int CtxPresObjData_Main::GetObjectNum() const { return m_ObjNum; }

double CtxPresObjData_Main::GetObjectData(int objId, int variableIndex) {
    if (objId < 0 || m_ObjNum <= objId) return {};
    if (variableIndex < 0 || m_VariableIndex.size() <= variableIndex) return {};
    return m_Attrs->GetElement(m_VariableIndex[variableIndex].first)
            .pointer->GetElementValue(objId, m_VariableIndex[variableIndex].second);
}

void CtxPresObjData_Main::SetVariableNum(int variableNum) { m_VariableNum = variableNum; }

int CtxPresObjData_Main::GetVariableNum() const { return m_VariableNum; }

void CtxPresObjData_Main::SetVariableName(const std::vector<std::string>& variableName) {
    m_VariableName = variableName;
}

const std::vector<std::string>& CtxPresObjData_Main::GetVariableName() { return m_VariableName; }

void CtxPresObjData_Main::SetVariableIndex(const std::vector<std::pair<int, int>>& variableIndex) {
    m_VariableIndex = variableIndex;
}

const std::vector<std::pair<int, int>>& CtxPresObjData_Main::GetVariableIndex() { return m_VariableIndex; }

void CtxPresObjData_Main::SetKeyObjectIds(const std::vector<int>& keyObjIds) { m_KeyObjIds = keyObjIds; }

const std::vector<int>& CtxPresObjData_Main::GetKeyObjectIds() const { return m_KeyObjIds; }

void CtxPresObjData_Main::SetKeyObjectIdToIndexMap(const std::map<int, int>& keyObjIdToIndex) {
    m_KeyObjIdToIndexs = keyObjIdToIndex;
}

const std::map<int, int>& CtxPresObjData_Main::GetKeyObjectIdToIndexMap() const { return m_KeyObjIdToIndexs; }

void CtxPresObjData_Main::SetChoosedObjectIds(const std::set<int>& choosedObjIds) { m_ChoosedObjIds = choosedObjIds; }

void CtxPresObjData_Main::AddChoosedObjectId(int objId) { m_ChoosedObjIds.insert(objId); }

void CtxPresObjData_Main::RemoveChoosedObjectId(int objId) { m_ChoosedObjIds.erase(objId); }

void CtxPresObjData_Main::ClearChoosedObjectIds() { m_ChoosedObjIds.clear(); }

const std::set<int>& CtxPresObjData_Main::GetChoosedObjectIds() { return m_ChoosedObjIds; }

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

std::vector<std::string>
CtxPresObjData_Main::GenerateVariableNames(ElementArray<AttributeSet::Attribute>::Pointer attrs, IGenum dataType) {
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

std::vector<std::pair<int, int>>
CtxPresObjData_Main::GenerateVariableIndex(ElementArray<AttributeSet::Attribute>::Pointer attrs, IGenum dataType) {
    std::vector<std::pair<int, int>> re;
    for (int attrIndex = 0; attrIndex < attrs->Size(); attrIndex++) {
        auto& attr = attrs->GetElement(attrIndex);
        if (attr.attachmentType != dataType) continue;
        if (attr.pointer->GetDimension() > 1) { re.push_back({attrIndex, -1}); }
        for (int dimensionIndex = 0; dimensionIndex < attr.pointer->GetDimension(); dimensionIndex++) {
            re.push_back({attrIndex, dimensionIndex});
        }
    }
    return re;
}

std::vector<int> CtxPresObjData_Main::GenerateKeyObjectIds(int objNum, int maxObjNum) {
    return GenerateRandomSample(objNum, maxObjNum);
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

std::map<int, int> CtxPresObjData_Main::GenerateKeyObjectIdToIndexs(const std::vector<int>& objectIds) {
    std::map<int, int> re;
    for (int i = 0; i < objectIds.size(); i++) { re[objectIds[i]] = i; }
    return re;
}

double CtxPresObjData_Main::GenerateObjData(int objectId, ElementArray<AttributeSet::Attribute>::Pointer attrs, const std::pair<int, int>& variableIndex_) {
    return attrs->GetElement(variableIndex_.first).pointer->GetElementValue(objectId, variableIndex_.second);
}

bool CtxPresObjData_Main::LegalAttrs(ElementArray<AttributeSet::Attribute>::Pointer attrs, IGenum dataType) {
    for (int attrIndex = 0; attrIndex < attrs->Size(); attrIndex++) {
        auto& attr = attrs->GetElement(attrIndex);
        if (attr.attachmentType != dataType) continue;
        return true;
    }
    return false;
}

int CtxPresObjData_Main::GetLegalAttrsObjNum(ElementArray<AttributeSet::Attribute>::Pointer attrs, IGenum dataType) {
    for (int attrIndex = 0; attrIndex < attrs->Size(); attrIndex++) {
        auto& attr = attrs->GetElement(attrIndex);
        if (attr.attachmentType != dataType) continue;
        return attr.pointer->GetNumberOfElements();
    }
    return -1;
}

void CtxPresObjData_LightAlpha::SetChoosedAlpha(int alpha) { m_ChoosedAlpha = alpha; }

int CtxPresObjData_LightAlpha::GetChoosedAlpha() const { return m_ChoosedAlpha; }

void CtxPresObjData_LightAlpha::SetUnChoosedAlpha(int alpha) { m_UnChoosedAlpha = alpha; }

int CtxPresObjData_LightAlpha::GetUnChoosedAlpha() const { return m_UnChoosedAlpha; }

void CtxPresObjData_LightAlpha::SetChoosedLight(int light) { m_ChoosedLight = light; }

int CtxPresObjData_LightAlpha::GetChoosedLight() const { return m_ChoosedLight; }

void CtxPresObjData_LightAlpha::SetUnChoosedLight(int light) { m_UnChoosedLight = light; }

int CtxPresObjData_LightAlpha::GetUnChoosedLight() const { return m_UnChoosedLight; }

std::vector<std::vector<int>> CtxPresObjData_Draw::GenerateObjectDrawSorts(int variableNum,
                                                                           const std::vector<int>& objIds,
                                                                           CtxPresObjData_Main* theData) {
    std::vector<std::vector<int>> re(variableNum, std::vector<int>(objIds.size(), 0));
    for (int variableIndex = 0; variableIndex < variableNum; variableIndex++) {
        re[variableIndex] = objIds;
    }
    for (int variableIndex = 0; variableIndex < variableNum; variableIndex++) {
        std::sort(re[variableIndex].begin(), re[variableIndex].end(), [&](int objIdA, int objIdB) {
            return theData->GetObjectData(objIdA, variableIndex) < theData->GetObjectData(objIdB, variableIndex);
        });
    }
    return re;
}

std::vector<std::vector<int>> CtxPresObjData_Draw::GenerateObjectDrawSorts(int variableNum, const std::set<int>& objIds,
                                                                           CtxPresObjData_Main* theData) {
    std::vector<std::vector<int>> re(variableNum, std::vector<int>(objIds.size(), 0));
    std::vector<int> objIds_V(objIds.begin(), objIds.end());
    for (int variableIndex = 0; variableIndex < variableNum; variableIndex++) { re[variableIndex] = objIds_V; }
    for (int variableIndex = 0; variableIndex < variableNum; variableIndex++) {
        std::sort(re[variableIndex].begin(), re[variableIndex].end(), [&](int objIdA, int objIdB) {
            return theData->GetObjectData(objIdA, variableIndex) < theData->GetObjectData(objIdB, variableIndex);
        });
    }
    return re;
}

std::vector<std::vector<int>> CtxPresObjData_Draw::GenerateDefaultObjectDrawSorts(int variableNum) {
    return std::vector<std::vector<int>>(variableNum);
}

std::tuple<int, int, int> CtxPresObjData_Draw::GenerateDefaultColor(int brightNess) {
    return ChangeBrightness(1.0f, 0.0f, 0.0f, brightNess);
}

std::vector<std::tuple<int, int, int>>
CtxPresObjData_Draw::GenerateObjectColors(int variableIndex, const std::vector<int>& objIds,
                                          CtxPresObjData_Main* theData, const std::vector<double>& maxValues,
                                          const std::vector<double>& minValues, int brightness,
                                          ScalarsToColors::Pointer colorMap) {
    if (variableIndex < 0 || objIds.empty() || theData->GetVariableNum() <= variableIndex) return {};
    float shift = 0 - minValues[variableIndex];
    float scale = 1.0 / (maxValues[variableIndex] - minValues[variableIndex]);
    vector<tuple<int, int, int>> re(objIds.size());
    ThreadPool::parallelFor(0, objIds.size(), [&](int st, int ed) {
        float rgb[3]{};
        for (int i = st; i < ed; i++) {
            auto objId = objIds[i];
            auto value = theData->GetObjectData(objId, variableIndex);
            colorMap->GetColor(value, rgb, shift, scale);
            re[i] = ChangeBrightness(rgb[0], rgb[1], rgb[2], brightness);
        }
    });
    return re;
}

std::map<int, std::tuple<int, int, int>>
CtxPresObjData_Draw::GenerateObjectColors(int variableIndex, const std::set<int>& objIds, CtxPresObjData_Main* theData,
                                          const std::vector<double>& maxValues, const std::vector<double>& minValues,
                                          int brightness, ScalarsToColors::Pointer colorMap) {
    std::map<int, std::tuple<int, int, int>> re;
    if (variableIndex < 0 || objIds.empty() || theData->GetVariableNum() <= variableIndex) return re;
    float shift = 0 - minValues[variableIndex];
    float scale = 1.0 / (maxValues[variableIndex] - minValues[variableIndex]);
    float rgb[3]{};
    for (auto& objId: objIds) {
        auto value = theData->GetObjectData(objId, variableIndex);
        colorMap->GetColor(value, rgb, shift, scale);
        re[objId] = ChangeBrightness(rgb[0], rgb[1], rgb[2], brightness);
    }
    return re;
}

std::tuple<int, int, int> CtxPresObjData_LightAlpha::ChangeSaturation(const std::tuple<int, int, int>& rgb,
                                                                      int _saturation) {
    float h, s, v;
    float r = (float) get<0>(rgb) / 255.0;
    float g = (float) get<1>(rgb) / 255.0;
    float b = (float) get<2>(rgb) / 255.0;
    float saturation = (float) _saturation / 255.0;
    rgbToHsb(r, g, b, h, s, v);

    if (std::fabs(saturation - s) < EPSILON) {
        int ir = r * 255;
        int ig = g * 255;
        int ib = b * 255;
        return {ir, ig, ib};
    }

    float newS = std::max(0.0f, std::min(1.0f, saturation));

    hsbToRgb(h, newS, v, r, g, b);

    int ir = r * 255;
    int ig = g * 255;
    int ib = b * 255;
    return {ir, ig, ib};
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
        if (objId < 0 || m_ObjectColor.size() <= objId) {
            return m_DefaultColor;
        }
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