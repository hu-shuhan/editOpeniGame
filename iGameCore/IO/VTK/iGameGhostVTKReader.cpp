#include "iGameGhostVTKReader.h"

#include "iGameAttributeSet.h"
#include "iGameFlatArray.h"
#include "iGameVTKReader.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

namespace
{

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::string ToUpper(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return value;
}

bool IsGhostName(const std::string& name) { return ToLower(name) == "vtkghosttype"; }

bool IsUnsignedCharType(const std::string& type) {
    const std::string lower = ToLower(type);
    return lower == "unsigned_char" || lower == "unsigned char";
}


// 读取Legacy ASCII VTK中的Cell vtkGhostType
bool ReadGhostArray(const std::string& filePath, std::vector<unsigned char>& values, int& numberOfComponents) {

    std::ifstream file(filePath);

    if (!file.is_open()) { return false; }


    // 读取Legacy VTK文件头
    std::string header;
    std::string title;
    std::string format;

    if (!std::getline(file, header)) { return false; }

    if (!std::getline(file, title)) { return false; }

    if (!std::getline(file, format)) { return false; }


    if (!format.empty() && format.back() == '\r') { format.pop_back(); }


    // Binary VTK继续使用原Reader
    if (ToUpper(format) != "ASCII") { return false; }


    std::string token;

    int currentTupleCount = 0;
    IGenum currentAttachment = IG_NONE;


    while (file >> token) {

        const std::string lowerToken = ToLower(token);


        if (lowerToken == "point_data") {

            if (!(file >> currentTupleCount)) { return false; }

            currentAttachment = IG_POINT;

            continue;
        }


        if (lowerToken == "cell_data") {

            if (!(file >> currentTupleCount)) { return false; }

            currentAttachment = IG_CELL;

            continue;
        }


        if (lowerToken != "scalars") { continue; }


        std::string name;
        std::string type;

        if (!(file >> name >> type)) { return false; }


        std::string nextToken;

        if (!(file >> nextToken)) { return false; }


        int numComp = 1;


        // SCALARS name type [numComp]
        if (ToLower(nextToken) != "lookup_table") {

            try {
                numComp = std::stoi(nextToken);
            } catch (...) { return false; }


            if (!(file >> nextToken)) { return false; }
        }


        if (ToLower(nextToken) != "lookup_table") { return false; }


        std::string tableName;

        if (!(file >> tableName)) { return false; }


        // 只处理Cell上的vtkGhostType
        if (currentAttachment != IG_CELL) { continue; }


        if (!IsGhostName(name) || !IsUnsignedCharType(type)) { continue; }


        if (currentTupleCount <= 0 || numComp <= 0) { return false; }


        const int valueCount = currentTupleCount * numComp;

        values.resize(static_cast<size_t>(valueCount));


        for (int i = 0; i < valueCount; ++i) {

            int value = 0;

            if (!(file >> value)) { return false; }


            if (value < 0 || value > 255) { return false; }


            values[static_cast<size_t>(i)] = static_cast<unsigned char>(value);
        }


        numberOfComponents = numComp;

        return true;
    }


    return false;
}


// 替换原Reader中的Cell vtkGhostType
bool ReplaceGhostArray(DataObject::Pointer dataObject, const std::vector<unsigned char>& values,
                       int numberOfComponents) {

    if (dataObject.IsNull()) { return false; }


    if (numberOfComponents <= 0 || values.empty()) { return false; }


    if (values.size() % static_cast<size_t>(numberOfComponents) != 0) { return false; }


    auto attributeSet = dataObject->GetAttributeSet();

    if (attributeSet == nullptr) { return false; }


    auto ghostArray = UnsignedCharArray::New();

    ghostArray->SetName("vtkGhostType");
    ghostArray->SetDimension(numberOfComponents);


    const IGsize tupleCount = static_cast<IGsize>(values.size() / static_cast<size_t>(numberOfComponents));


    ghostArray->Resize(tupleCount);


    for (IGsize i = 0; i < static_cast<IGsize>(values.size()); ++i) {

        ghostArray->SetValue(i, values[static_cast<size_t>(i)]);
    }


    // 查找并替换原Reader产生的Cell vtkGhostType
    const IGsize attributeCount = static_cast<IGsize>(attributeSet->GetNumberOfAttributes());


    for (IGsize i = 0; i < attributeCount; ++i) {

        auto& attr = attributeSet->GetAttribute(i);


        if (attr.isDeleted || attr.pointer == nullptr) { continue; }


        if (attr.attachmentType != IG_CELL) { continue; }


        if (!IsGhostName(attr.pointer->GetName())) { continue; }


        attr.SetPointer(ghostArray);

        return true;
    }


    // 原Reader未生成Ghost属性时补充Cell Ghost属性
    attributeSet->AddAttribute(IG_SCALAR, IG_CELL, ghostArray);


    return true;
}

} // namespace


DataObject::Pointer GhostVTKReader::ReadFile(const std::string& filePath) {

    // 其他VTK数据仍由原VTKReader读取
    auto reader = VTKReader::New();

    auto output = reader->ReadFile(filePath);


    if (output.IsNull()) { return nullptr; }


    std::vector<unsigned char> ghostValues;

    int numberOfComponents = 1;


    // 非ASCII或没有Cell vtkGhostType时直接返回原Reader结果
    if (!ReadGhostArray(filePath, ghostValues, numberOfComponents)) { return output; }


    ReplaceGhostArray(output, ghostValues, numberOfComponents);


    return output;
}

IGAME_NAMESPACE_END