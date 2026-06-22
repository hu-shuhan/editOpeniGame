#include <IGC/iGameIGCWriter.h>
#include <iGameAttributeSet.h>
#include <iGameDataObject.h>
#include <iGameFileIO.h>
#include <iGameFlatArray.h>
#include <iGamePointSet.h>
#include <iGameStructuredMesh.h>
#include <iGameUnstructuredMesh.h>

#if defined(CGNS_ENABLE)
#include <cgnslib.h>
#endif

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

// 导览
// 重排序捕获入口：WriteLosslessIgc 通过 MeshEncodeTrace 接收编码阶段的排序信息
// 重排序恢复入口：BuildOrderMaps 和 InvertEncodeOrderMap 把 original -> decoded 反转为 decoded -> original
// field 收集入口：CollectFloatingFields 只收集 FloatArray 和 DoubleArray
// 二进制比较入口：CompareFloatingFieldsByEncodedOrder 调用 CompareArrayBytesWithDecodedToOriginalMap
// CGNS 导出入口：ExportSupportedObjectAsSourceFormat 按源格式分发到 WriteSimpleStructuredCgns 或 WriteSimpleUnstructuredCgns
// CGNS 拓扑写出：WriteUnstructuredMixedElements 将 UnstructuredMesh 普通单元写成 MIXED section
// CGNS field 写出：WriteCgnsField 保留 FloatArray/DoubleArray 精度，三分量 vector 拆成 X/Y/Z scalar field
// CGNS vector 回读比对：FindSplitVectorComponents 找回 X/Y/Z，CompareVectorArrayBytesFromScalarComponents 按原 vector tuple 做字节比较
// 三条验证路径：CheckDecodeRoundTripFields、CheckCodecExportRoundTripFields、CheckDirectExportRoundTripFields
// 路径1 decode_roundtrip：源文件 -> 无损 IGC -> 回读 IGC，比较源 field 与解码 field
// 路径2 codec_export_roundtrip：源文件 -> 无损 IGC -> 回读 IGC -> 导出源格式 -> 再读回，比较源 field 与导出 field
// 路径3 direct_export_roundtrip：源文件 -> 直接导出源格式 -> 再读回，比较源 field 与导出 field

namespace {

// region 检查结果和运行开关

struct CompareStats {
    size_t checkedArrays = 0;
    size_t checkedValues = 0;
    size_t skippedArrays = 0;
    size_t mismatchArrays = 0;
    size_t mismatchValues = 0;
    std::vector<std::string> messages;

    // 判断当前统计是否没有任何差异
    bool Passed() const {
        return mismatchArrays == 0 && mismatchValues == 0;
    }

    // 保存有限数量的差异详情，避免大数据输出过长
    void AddMessage(const std::string& message) {
        if (messages.size() < 32) {
            messages.push_back(message);
        }
    }
};

struct CaseResult {
    bool failed = false;
    bool skipped = false;
};

constexpr const char* LosslessModeName = "legacy";
constexpr bool RunDecodeRoundTrip = true;
constexpr bool RunCodecExportRoundTrip = true;
constexpr bool RunDirectExportRoundTrip = true;

// endregion

// region 失败日志格式化

// 把 float 或 double 的位模式转为整数，便于失败日志展示
template<typename T>
uint64_t BitPattern(T value) {
    uint64_t bits = 0;
    std::memcpy(&bits, &value, sizeof(T));
    return bits;
}

// 把浮点值按可回读精度转成文本，只用于失败日志
template<typename T>
std::string ValueText(T value) {
    std::ostringstream oss;
    oss << std::setprecision(std::numeric_limits<T>::max_digits10) << value;
    return oss.str();
}

// 直接拷贝一个 tuple 的原始字节，比较逻辑使用这个结果而非数值误差
template<typename T>
std::vector<unsigned char> MakeTupleBytes(const T* values, int dimension) {
    std::vector<unsigned char> bytes(sizeof(T) * static_cast<size_t>(dimension));
    std::memcpy(bytes.data(), values, bytes.size());
    return bytes;
}

// endregion

// region 编码排序映射恢复

// 构造恒等映射，用于编码未重排的对象
bool MakeIdentityOrderMap(size_t count, std::vector<size_t>& decodedToOriginal) {
    decodedToOriginal.resize(count);
    for (size_t i = 0; i < count; ++i) {
        decodedToOriginal[i] = i;
    }
    return true;
}

// 把编码记录的 original -> decoded 映射反转为检查时使用的 decoded -> original
bool InvertEncodeOrderMap(const std::vector<unsigned int>& originalToDecoded, size_t elementCount,
                          std::vector<size_t>& decodedToOriginal, std::string& reason) {
    // 编码阶段记录 original -> decoded，这里反转成 decoded -> original
    if (originalToDecoded.empty()) {
        return MakeIdentityOrderMap(elementCount, decodedToOriginal);
    }

    if (originalToDecoded.size() != elementCount) {
        reason = "encode order map size mismatch";
        return false;
    }

    decodedToOriginal.assign(elementCount, 0);
    std::vector<unsigned char> visited(elementCount, static_cast<unsigned char>(0));
    for (size_t originalIndex = 0; originalIndex < elementCount; ++originalIndex) {
        const auto decodedIndex = static_cast<size_t>(originalToDecoded[originalIndex]);
        // 映射必须是一一对应，否则不能作为合法的排序恢复依据
        if (decodedIndex >= elementCount) {
            reason = "encode order map contains an out-of-range decoded index";
            return false;
        }
        if (visited[decodedIndex]) {
            reason = "encode order map contains duplicate decoded indices";
            return false;
        }
        visited[decodedIndex] = static_cast<unsigned char>(1);
        decodedToOriginal[decodedIndex] = originalIndex;
    }
    return true;
}

// 获取点数量，点 field 使用这个数量校验排序映射长度
size_t GetPointCount(const iGame::DataObject::Pointer& obj) {
    const auto points = obj ? obj->GetPoints() : nullptr;
    return points ? static_cast<size_t>(points->GetNumberOfPoints()) : 0u;
}

// 获取单元数量，单元 field 使用这个数量校验排序映射长度
size_t GetCellCount(const iGame::DataObject::Pointer& obj) {
    auto structured = iGame::DynamicCast<iGame::StructuredMesh>(obj);
    if (structured) {
        return static_cast<size_t>(structured->GetNumberOfCells());
    }

    const auto cells = obj ? obj->GetCellArray() : nullptr;
    return cells ? static_cast<size_t>(cells->GetNumberOfCells()) : 0u;
}

// 保存点 field 和单元 field 各自使用的 decoded -> original 映射
struct OrderMaps {
    std::vector<size_t> decodedPointToOriginal;
    std::vector<size_t> decodedCellToOriginal;
    std::string pointReason;
    std::string cellReason;
    bool hasPointOrder = false;
    bool hasCellOrder = false;
};

// 同时构造点排序和单元排序，后续按 field attachment 选择其中一个
OrderMaps BuildOrderMaps(const iGame::DataObject::Pointer& expectedObj, const iGame::DataObject::Pointer& actualObj,
                         const iGame::EncodeOrderRemaps& encodeOrderRemaps) {
    OrderMaps maps;

    const size_t expectedPointCount = GetPointCount(expectedObj);
    const size_t actualPointCount = GetPointCount(actualObj);
    if (expectedPointCount != actualPointCount) {
        maps.pointReason = "point count mismatch for encode order restoration";
    } else {
        maps.hasPointOrder = InvertEncodeOrderMap(encodeOrderRemaps.pointIdRemap, expectedPointCount,
                                                  maps.decodedPointToOriginal, maps.pointReason);
    }

    const size_t expectedCellCount = GetCellCount(expectedObj);
    const size_t actualCellCount = GetCellCount(actualObj);
    if (expectedCellCount != actualCellCount) {
        maps.cellReason = "cell count mismatch for encode order restoration";
    } else {
        maps.hasCellOrder = InvertEncodeOrderMap(encodeOrderRemaps.topCellIdsRemap, expectedCellCount,
                                                 maps.decodedCellToOriginal, maps.cellReason);
    }

    return maps;
}

// endregion

// region field 二进制比较

// 核心检查逻辑：只比较 float/double field
// 编码 trace 是唯一允许使用的排序还原依据
// 浮点值直接比较原始字节，不使用 epsilon

// 把失败 tuple 转成可读数值文本，只用于定位首个差异
template<typename T>
std::string TupleText(const std::vector<unsigned char>& tuple, int dimension) {
    std::ostringstream oss;
    oss << "{";
    for (int i = 0; i < dimension; ++i) {
        T value{};
        std::memcpy(&value, tuple.data() + sizeof(T) * static_cast<size_t>(i), sizeof(T));
        if (i > 0) {
            oss << ", ";
        }
        oss << ValueText(value);
    }
    oss << "}";
    return oss.str();
}

// 把失败 tuple 转成位模式文本，只用于定位首个差异
template<typename T>
std::string TupleBits(const std::vector<unsigned char>& tuple, int dimension) {
    std::ostringstream oss;
    oss << "{";
    for (int i = 0; i < dimension; ++i) {
        T value{};
        std::memcpy(&value, tuple.data() + sizeof(T) * static_cast<size_t>(i), sizeof(T));
        if (i > 0) {
            oss << ", ";
        }
        oss << "0x" << std::hex << BitPattern(value) << std::dec;
    }
    oss << "}";
    return oss.str();
}

// 按 decoded -> original 映射恢复原顺序后逐 tuple 做原始字节比较
template<typename T>
bool CompareArrayBytesWithDecodedToOriginalMap(const std::string& label, const T* expected, const T* actual,
                                               size_t elementCount, int dimension,
                                               const std::vector<size_t>& decodedToOriginal, CompareStats& stats) {
    stats.checkedArrays++;
    stats.checkedValues += elementCount * static_cast<size_t>(dimension);

    if (decodedToOriginal.size() != elementCount) {
        stats.mismatchArrays++;
        stats.AddMessage(label + " order map size mismatch");
        return false;
    }

    size_t mismatches = 0;
    size_t firstMismatch = elementCount;
    size_t firstDecodedIndex = elementCount;
    size_t firstOriginalIndex = elementCount;
    std::vector<unsigned char> firstExpectedTuple;
    std::vector<unsigned char> firstActualTuple;
    for (size_t decodedIndex = 0; decodedIndex < elementCount; ++decodedIndex) {
        const size_t originalIndex = decodedToOriginal[decodedIndex];
        if (originalIndex >= elementCount) {
            stats.mismatchArrays++;
            stats.AddMessage(label + " order map contains invalid original index");
            return false;
        }

        // expected 使用原始顺序索引，actual 使用解码后顺序索引
        auto expectedTuple = MakeTupleBytes(expected + originalIndex * static_cast<size_t>(dimension), dimension);
        auto actualTuple = MakeTupleBytes(actual + decodedIndex * static_cast<size_t>(dimension), dimension);
        // 这里是无损判定本体，直接比较 float/double 的内存字节
        if (expectedTuple != actualTuple) {
            if (firstMismatch == elementCount) {
                firstMismatch = originalIndex;
                firstDecodedIndex = decodedIndex;
                firstOriginalIndex = originalIndex;
                firstExpectedTuple = std::move(expectedTuple);
                firstActualTuple = std::move(actualTuple);
            }
            mismatches++;
        }
    }

    if (mismatches == 0) {
        return true;
    }

    stats.mismatchArrays++;
    stats.mismatchValues += mismatches * static_cast<size_t>(dimension);

    std::ostringstream oss;
    oss << label << " mismatch tuples=" << mismatches << "/" << elementCount
        << " firstOriginalIndex=" << firstOriginalIndex << " firstDecodedIndex=" << firstDecodedIndex
        << " expected=" << TupleText<T>(firstExpectedTuple, dimension)
        << " actual=" << TupleText<T>(firstActualTuple, dimension)
        << " expectedBits=" << TupleBits<T>(firstExpectedTuple, dimension)
        << " actualBits=" << TupleBits<T>(firstActualTuple, dimension);
    stats.AddMessage(oss.str());
    return false;
}

// 校验数组类型和形状，再进入按排序恢复的字节比较
template<typename TArray, typename TValue>
bool CompareFlatArrayBytes(const std::string& label, const iGame::ArrayObject::Pointer& expectedArray,
                           const iGame::ArrayObject::Pointer& actualArray,
                           const std::vector<size_t>& decodedToOriginal, CompareStats& stats) {
    auto expected = iGame::DynamicCast<TArray>(expectedArray);
    auto actual = iGame::DynamicCast<TArray>(actualArray);
    if (!expected || !actual) {
        stats.mismatchArrays++;
        stats.AddMessage(label + " type mismatch");
        return false;
    }

    if (expected->GetDimension() != actual->GetDimension()) {
        stats.mismatchArrays++;
        stats.AddMessage(label + " dimension mismatch");
        return false;
    }

    if (expected->GetNumberOfElements() != actual->GetNumberOfElements()) {
        stats.mismatchArrays++;
        stats.AddMessage(label + " element count mismatch");
        return false;
    }

    if (expected->GetNumberOfValues() != actual->GetNumberOfValues()) {
        stats.mismatchArrays++;
        stats.AddMessage(label + " value count mismatch");
        return false;
    }

    const size_t elementCount = static_cast<size_t>(expected->GetNumberOfElements());
    return CompareArrayBytesWithDecodedToOriginalMap<TValue>(label, expected->RawPointer(), actual->RawPointer(),
                                                             elementCount, expected->GetDimension(),
                                                             decodedToOriginal, stats);
}

// 比较一个 vector field 和 CGNS 回读后的 X/Y/Z 三个 scalar field
template<typename TArray, typename TValue>
bool CompareVectorArrayBytesFromScalarComponents(const std::string& label,
                                                 const iGame::ArrayObject::Pointer& expectedArray,
                                                 const std::vector<iGame::ArrayObject::Pointer>& actualComponents,
                                                 const std::vector<size_t>& decodedToOriginal, CompareStats& stats) {
    auto expected = iGame::DynamicCast<TArray>(expectedArray);
    if (!expected || expected->GetDimension() != 3 || actualComponents.size() != 3) {
        stats.mismatchArrays++;
        stats.AddMessage(label + " vector component shape mismatch");
        return false;
    }

    std::vector<typename TArray::Pointer> actualArrays;
    actualArrays.reserve(3);
    for (const auto& component: actualComponents) {
        auto actual = iGame::DynamicCast<TArray>(component);
        if (!actual || actual->GetDimension() != 1) {
            stats.mismatchArrays++;
            stats.AddMessage(label + " vector component type mismatch");
            return false;
        }
        if (expected->GetNumberOfElements() != actual->GetNumberOfElements()) {
            stats.mismatchArrays++;
            stats.AddMessage(label + " vector component element count mismatch");
            return false;
        }
        actualArrays.push_back(actual);
    }

    const size_t elementCount = static_cast<size_t>(expected->GetNumberOfElements());
    if (decodedToOriginal.size() != elementCount) {
        stats.mismatchArrays++;
        stats.AddMessage(label + " order map size mismatch");
        return false;
    }

    stats.checkedArrays++;
    stats.checkedValues += elementCount * 3u;

    size_t mismatches = 0;
    size_t firstOriginalIndex = elementCount;
    size_t firstDecodedIndex = elementCount;
    std::vector<unsigned char> firstExpectedTuple;
    std::vector<unsigned char> firstActualTuple;
    for (size_t decodedIndex = 0; decodedIndex < elementCount; ++decodedIndex) {
        const size_t originalIndex = decodedToOriginal[decodedIndex];
        if (originalIndex >= elementCount) {
            stats.mismatchArrays++;
            stats.AddMessage(label + " order map contains invalid original index");
            return false;
        }

        auto expectedTuple = MakeTupleBytes(expected->RawPointer() + originalIndex * 3u, 3);
        std::vector<unsigned char> actualTuple(sizeof(TValue) * 3u);
        for (int componentIndex = 0; componentIndex < 3; ++componentIndex) {
            const TValue* actualValue = actualArrays[componentIndex]->RawPointer() + decodedIndex;
            std::memcpy(actualTuple.data() + sizeof(TValue) * static_cast<size_t>(componentIndex), actualValue,
                        sizeof(TValue));
        }

        if (expectedTuple != actualTuple) {
            if (firstOriginalIndex == elementCount) {
                firstOriginalIndex = originalIndex;
                firstDecodedIndex = decodedIndex;
                firstExpectedTuple = std::move(expectedTuple);
                firstActualTuple = std::move(actualTuple);
            }
            mismatches++;
        }
    }

    if (mismatches == 0) {
        return true;
    }

    stats.mismatchArrays++;
    stats.mismatchValues += mismatches * 3u;
    std::ostringstream oss;
    oss << label << " mismatch tuples=" << mismatches << "/" << elementCount
        << " firstOriginalIndex=" << firstOriginalIndex << " firstDecodedIndex=" << firstDecodedIndex
        << " expected=" << TupleText<TValue>(firstExpectedTuple, 3)
        << " actual=" << TupleText<TValue>(firstActualTuple, 3)
        << " expectedBits=" << TupleBits<TValue>(firstExpectedTuple, 3)
        << " actualBits=" << TupleBits<TValue>(firstActualTuple, 3);
    stats.AddMessage(oss.str());
    return false;
}

// 保存一个参与检查的浮点 field 及其原始 attribute index
struct FloatingField {
    iGame::AttributeSet::Attribute attr;
    size_t sourceIndex = 0;
    bool isFloat = false;
    bool isDouble = false;
};

// 生成日志中使用的 field 标签
std::string FieldLabel(const std::string& objectLabel, const FloatingField& field, size_t floatingIndex) {
    std::ostringstream oss;
    oss << objectLabel << "/attr[" << floatingIndex << "]";
    if (field.attr.pointer) {
        oss << ":" << field.attr.pointer->GetName();
    }
    oss << "#sourceIndex=" << field.sourceIndex;
    return oss.str();
}

// 获取 field 名称
std::string FieldName(const FloatingField& field) {
    return field.attr.pointer ? field.attr.pointer->GetName() : std::string{};
}

// 判断两个 field 是否是同一个浮点 field
bool SameFloatingFieldIdentity(const FloatingField& expected, const FloatingField& actual) {
    if (expected.isFloat != actual.isFloat || expected.isDouble != actual.isDouble) {
        return false;
    }

    if (expected.attr.type != actual.attr.type || expected.attr.attachmentType != actual.attr.attachmentType) {
        return false;
    }

    if (FieldName(expected) != FieldName(actual)) {
        return false;
    }

    if (!expected.attr.pointer || !actual.attr.pointer) {
        return false;
    }

    return expected.attr.pointer->GetDimension() == actual.attr.pointer->GetDimension();
}

// 按 CGNS vector 写出规则查找 X/Y/Z 三个 scalar 分量
bool FindSplitVectorComponents(const FloatingField& expected, const std::vector<FloatingField>& actualFields,
                               const std::vector<bool>& matchedActual, size_t componentIndices[3]) {
    if (expected.attr.type != IG_VECTOR || !expected.attr.pointer || expected.attr.pointer->GetDimension() != 3) {
        return false;
    }

    const char suffixes[3] = {'X', 'Y', 'Z'};
    for (int componentIndex = 0; componentIndex < 3; ++componentIndex) {
        componentIndices[componentIndex] = actualFields.size();
        const auto componentName = FieldName(expected) + suffixes[componentIndex];
        for (size_t actualIndex = 0; actualIndex < actualFields.size(); ++actualIndex) {
            const auto& actual = actualFields[actualIndex];
            if (matchedActual[actualIndex]) {
                continue;
            }
            if (expected.isFloat != actual.isFloat || expected.isDouble != actual.isDouble) {
                continue;
            }
            if (actual.attr.type != IG_SCALAR || actual.attr.attachmentType != expected.attr.attachmentType) {
                continue;
            }
            if (!actual.attr.pointer || actual.attr.pointer->GetDimension() != 1) {
                continue;
            }
            if (FieldName(actual) == componentName) {
                componentIndices[componentIndex] = actualIndex;
                break;
            }
        }
        if (componentIndices[componentIndex] == actualFields.size()) {
            return false;
        }
    }

    return componentIndices[0] != componentIndices[1] && componentIndices[0] != componentIndices[2] &&
           componentIndices[1] != componentIndices[2];
}

// 根据 field 的 attachment 选择点排序或单元排序
const std::vector<size_t>* SelectDecodedToOriginalMap(const OrderMaps& maps, IGenum attachmentType,
                                                      std::string& reason) {
    if (attachmentType == IG_POINT) {
        if (!maps.hasPointOrder) {
            reason = maps.pointReason;
            return nullptr;
        }
        return &maps.decodedPointToOriginal;
    }

    if (attachmentType == IG_CELL) {
        if (!maps.hasCellOrder) {
            reason = maps.cellReason;
            return nullptr;
        }
        return &maps.decodedCellToOriginal;
    }

    reason = "unsupported attachment type for order-aware field comparison";
    return nullptr;
}

// 从 attribute set 中收集 float 和 double field
std::vector<FloatingField> CollectFloatingFields(iGame::AttributeSet* attrSet, CompareStats& stats, bool countSkipped) {
    std::vector<FloatingField> floatingFields;
    const auto attrs = attrSet ? attrSet->GetAllAttributes() : nullptr;
    const int attrCount = attrs ? attrs->GetNumberOfElements() : 0;
    for (int i = 0; i < attrCount; ++i) {
        auto rawAttr = attrSet->GetAttribute(i);
        if (rawAttr.IsNone() || !rawAttr.pointer) {
            continue;
        }

        // 保留原始类型，double field 后续必须按 DoubleArray 比较
        FloatingField field;
        field.attr = rawAttr;
        field.sourceIndex = static_cast<size_t>(i);
        field.isFloat = iGame::DynamicCast<iGame::FloatArray>(rawAttr.pointer) != nullptr;
        field.isDouble = iGame::DynamicCast<iGame::DoubleArray>(rawAttr.pointer) != nullptr;
        if (!field.isFloat && !field.isDouble) {
            if (countSkipped) {
                stats.skippedArrays++;
            }
            continue;
        }

        floatingFields.push_back(field);
    }
    return floatingFields;
}

// 用编码 trace 还原排序后比较所有浮点 field
bool CompareFloatingFieldsByEncodedOrder(const std::string& label, const iGame::DataObject::Pointer& expectedObj,
                                         const iGame::DataObject::Pointer& actualObj,
                                         const iGame::EncodeOrderRemaps& encodeOrderRemaps, CompareStats& stats) {
    if (!expectedObj || !actualObj) {
        stats.mismatchArrays++;
        stats.AddMessage(label + " object presence mismatch");
        return false;
    }

    auto* expectedAttrSet = expectedObj ? expectedObj->GetAttributeSet() : nullptr;
    auto* actualAttrSet = actualObj ? actualObj->GetAttributeSet() : nullptr;
    // 先按原始数据和回读数据分别收集浮点 field
    auto expectedFields = CollectFloatingFields(expectedAttrSet, stats, true);
    auto actualFields = CollectFloatingFields(actualAttrSet, stats, false);
    // 排序映射来自编码 trace，不在检查阶段重新排序或猜测匹配
    const auto orderMaps = BuildOrderMaps(expectedObj, actualObj, encodeOrderRemaps);
    std::vector<bool> matchedActual(actualFields.size(), false);

    bool ok = true;
    for (size_t i = 0; i < expectedFields.size(); ++i) {
        const auto& expected = expectedFields[i];
        const auto attrLabel = FieldLabel(label, expected, i);

        size_t actualIndex = actualFields.size();
        // field 匹配只用类型、attachment、名称和维度，不按数值内容找匹配
        for (size_t j = 0; j < actualFields.size(); ++j) {
            if (!matchedActual[j] && SameFloatingFieldIdentity(expected, actualFields[j])) {
                actualIndex = j;
                break;
            }
        }

        size_t componentIndices[3] = {actualFields.size(), actualFields.size(), actualFields.size()};
        const bool splitVector = actualIndex == actualFields.size() &&
                                 FindSplitVectorComponents(expected, actualFields, matchedActual, componentIndices);

        if (actualIndex == actualFields.size() && !splitVector) {
            stats.mismatchArrays++;
            stats.AddMessage(attrLabel + " missing decoded attribute");
            ok = false;
            continue;
        }

        std::string orderReason;
        // 点 field 用点排序，单元 field 用单元排序
        const auto* orderMap = SelectDecodedToOriginalMap(orderMaps, expected.attr.attachmentType, orderReason);
        if (!orderMap) {
            stats.mismatchArrays++;
            stats.AddMessage(attrLabel + " cannot restore order: " + orderReason);
            ok = false;
            continue;
        }

        if (splitVector) {
            // 只按写出时的 X/Y/Z 命名规则取回分量，不用数值内容寻找匹配
            std::vector<iGame::ArrayObject::Pointer> components = {
                    actualFields[componentIndices[0]].attr.pointer,
                    actualFields[componentIndices[1]].attr.pointer,
                    actualFields[componentIndices[2]].attr.pointer};
            matchedActual[componentIndices[0]] = true;
            matchedActual[componentIndices[1]] = true;
            matchedActual[componentIndices[2]] = true;
            if (expected.isFloat) {
                ok = CompareVectorArrayBytesFromScalarComponents<iGame::FloatArray, float>(
                             attrLabel, expected.attr.pointer, components, *orderMap, stats) &&
                     ok;
            } else if (expected.isDouble) {
                ok = CompareVectorArrayBytesFromScalarComponents<iGame::DoubleArray, double>(
                             attrLabel, expected.attr.pointer, components, *orderMap, stats) &&
                     ok;
            }
            continue;
        }

        matchedActual[actualIndex] = true;
        const auto& actual = actualFields[actualIndex];
        if (expected.isFloat) {
            ok = CompareFlatArrayBytes<iGame::FloatArray, float>(attrLabel, expected.attr.pointer,
                                                                 actual.attr.pointer, *orderMap, stats) && ok;
        } else if (expected.isDouble) {
            ok = CompareFlatArrayBytes<iGame::DoubleArray, double>(attrLabel, expected.attr.pointer,
                                                                   actual.attr.pointer, *orderMap, stats) && ok;
        }
    }

    for (size_t i = 0; i < actualFields.size(); ++i) {
        if (!matchedActual[i]) {
            stats.mismatchArrays++;
            stats.AddMessage(label + " extra decoded floating attribute: " + FieldName(actualFields[i]));
            ok = false;
        }
    }

    return ok;
}

// endregion

// region 无损编码配置

// 把几何和所有 field 的 codec 参数切到无量化
void ForceLossless(iGame::CodecControlParams& params) {
    params.geomControl.errorMode = iGame::QuantizeMode::None;
    params.geomControl.globalQuantizeLevel = 0;
    params.geomControl.criticalQuantizeLevel = 0;
    params.geomControl.normalQuantizeLevel = 0;

    for (auto& attrControl: params.attrControl) {
        attrControl.errorMode = iGame::QuantizeMode::None;
        attrControl.globalQuantizeLevel = 0;
        attrControl.criticalQuantizeLevel = 0;
        attrControl.normalQuantizeLevel = 0;
    }
}

// 判断当前 example 是否处理这个对象
bool IsSupportedSingleBlockPointSet(const iGame::DataObject::Pointer& obj, std::string& reason) {
    if (!obj) {
        reason = "null data object";
        return false;
    }

    if (obj->HasSubDataObject()) {
        reason = "multi-block data object";
        return false;
    }

    if (!iGame::DynamicCast<iGame::PointSet>(obj)) {
        reason = "not a PointSet";
        return false;
    }

    const IGenum meshType = obj->GetDataObjectType();
    const bool supported = meshType == IG_SURFACE_MESH || meshType == IG_VOLUME_MESH ||
                           meshType == IG_STRUCTURED_MESH || meshType == IG_UNSTRUCTURED_MESH ||
                           meshType == IG_POINT_SET;
    if (!supported) {
        reason = "unsupported mesh type";
        return false;
    }

    if (meshType == IG_STRUCTURED_MESH && !iGame::DynamicCast<iGame::StructuredMesh>(obj)) {
        reason = "invalid structured mesh";
        return false;
    }

    return true;
}

// endregion

// region 输出路径生成

// 获取小写扩展名
std::string LowerExtension(const std::string& fileName) {
    std::filesystem::path path(fileName);
    auto ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return ext;
}

// 在源文件同目录生成输出路径
std::string MakeSiblingPath(const std::string& sourceFile, const std::string& suffix,
                            const std::string& extension) {
    std::filesystem::path sourcePath(sourceFile);
    auto parent = sourcePath.parent_path();
    if (parent.empty()) {
        parent = ".";
    }

    const auto stem = sourcePath.stem().string();
    return (parent / (stem + suffix + extension)).string();
}

// endregion

// region 源格式导出

#if defined(CGNS_ENABLE)
// 写入一个 CGNS field，scalar 原名写出，三分量 vector 拆成 X/Y/Z
bool WriteCgnsField(int fileIndex, int baseIndex, int zoneIndex, int solutionIndex,
                    const FloatingField& attr, std::string& reason) {
    if (attr.attr.type != IG_SCALAR && attr.attr.type != IG_VECTOR) {
        reason = "CGNS export supports scalar or vector fields only";
        return false;
    }

    if (!attr.attr.pointer) {
        reason = "CGNS export got null field";
        return false;
    }

    const int dimension = attr.attr.pointer->GetDimension();
    if ((attr.attr.type == IG_SCALAR && dimension != 1) || (attr.attr.type == IG_VECTOR && dimension != 3)) {
        reason = "CGNS export supports scalar one-component fields or three-component vector fields only";
        return false;
    }

    auto writeOneField = [&](const std::string& name, void* data, DataType_t dataType) {
        int fieldIndex = 0;
        return cg_field_write(fileIndex, baseIndex, zoneIndex, solutionIndex, dataType, name.c_str(), data,
                              &fieldIndex) == CG_OK;
    };

    const auto baseName = FieldName(attr);
    if (dimension == 1) {
        if (attr.isFloat) {
            auto array = iGame::DynamicCast<iGame::FloatArray>(attr.attr.pointer);
            if (!array || !writeOneField(baseName, array->RawPointer(), RealSingle)) {
                reason = cg_get_error();
                return false;
            }
            return true;
        }

        if (attr.isDouble) {
            auto array = iGame::DynamicCast<iGame::DoubleArray>(attr.attr.pointer);
            if (!array || !writeOneField(baseName, array->RawPointer(), RealDouble)) {
                reason = cg_get_error();
                return false;
            }
            return true;
        }
    }

    const char suffixes[3] = {'X', 'Y', 'Z'};
    const auto valueCount = static_cast<size_t>(attr.attr.pointer->GetNumberOfElements());
    if (attr.isFloat) {
        auto array = iGame::DynamicCast<iGame::FloatArray>(attr.attr.pointer);
        if (!array) {
            reason = "float vector field type mismatch";
            return false;
        }
        std::vector<float> component(valueCount);
        for (int componentIndex = 0; componentIndex < 3; ++componentIndex) {
            // CGNS reader 将每个 field 当作 scalar 回读，vector 在写出时拆成同精度分量
            const float* raw = array->RawPointer() + componentIndex;
            for (size_t i = 0; i < valueCount; ++i) {
                component[i] = raw[i * 3];
            }
            if (!writeOneField(baseName + suffixes[componentIndex], component.data(), RealSingle)) {
                reason = cg_get_error();
                return false;
            }
        }
        return true;
    }

    if (attr.isDouble) {
        auto array = iGame::DynamicCast<iGame::DoubleArray>(attr.attr.pointer);
        if (!array) {
            reason = "double vector field type mismatch";
            return false;
        }
        std::vector<double> component(valueCount);
        for (int componentIndex = 0; componentIndex < 3; ++componentIndex) {
            // double vector 的每个分量仍按 RealDouble 写出
            const double* raw = array->RawPointer() + componentIndex;
            for (size_t i = 0; i < valueCount; ++i) {
                component[i] = raw[i * 3];
            }
            if (!writeOneField(baseName + suffixes[componentIndex], component.data(), RealDouble)) {
                reason = cg_get_error();
                return false;
            }
        }
        return true;
    }

    reason = "CGNS export supports float and double fields only";
    return false;
}

// 这里只覆盖 example 需要的 simple StructuredMesh CGNS 导出，不代表完整 CGNS writer
// 写出 simple StructuredMesh 的坐标和浮点 field
bool WriteSimpleStructuredCgns(const iGame::DataObject::Pointer& obj, const std::string& fileName,
                               std::string& reason) {
    auto mesh = iGame::DynamicCast<iGame::StructuredMesh>(obj);
    auto pointSet = iGame::DynamicCast<iGame::PointSet>(obj);
    if (!mesh || !pointSet || !pointSet->GetPoints()) {
        reason = "CGNS export supports simple StructuredMesh objects only";
        return false;
    }

    // 先校验结构网格尺寸和 field attachment，避免写出不完整 CGNS
    const auto* dims = mesh->GetDimensionSize();
    if (!dims || dims[0] <= 0 || dims[1] <= 0 || dims[2] <= 0) {
        reason = "invalid structured mesh dimensions";
        return false;
    }

    const size_t pointCount = static_cast<size_t>(dims[0]) * static_cast<size_t>(dims[1]) *
                              static_cast<size_t>(dims[2]);
    if (pointSet->GetPoints()->GetNumberOfPoints() != static_cast<IGsize>(pointCount)) {
        reason = "structured dimension and point count mismatch";
        return false;
    }

    const size_t cellCount = static_cast<size_t>(std::max<igIndex>(dims[0] - 1, 0)) *
                             static_cast<size_t>(std::max<igIndex>(dims[1] - 1, 0)) *
                             static_cast<size_t>(std::max<igIndex>(dims[2] - 1, 0));

    CompareStats collectStats;
    auto attrs = CollectFloatingFields(obj->GetAttributeSet(), collectStats, false);
    if (attrs.empty()) {
        reason = "no floating fields to export";
        return false;
    }

    for (const auto& attr: attrs) {
        const auto valueCount = attr.attr.pointer ? attr.attr.pointer->GetNumberOfElements() : 0;
        if (attr.attr.attachmentType == IG_POINT && valueCount != static_cast<IGsize>(pointCount)) {
            reason = "point field element count mismatch: " + FieldName(attr);
            return false;
        }
        if (attr.attr.attachmentType == IG_CELL && valueCount != static_cast<IGsize>(cellCount)) {
            reason = "cell field element count mismatch: " + FieldName(attr);
            return false;
        }
        if (attr.attr.attachmentType != IG_POINT && attr.attr.attachmentType != IG_CELL) {
            reason = "unsupported CGNS field attachment: " + FieldName(attr);
            return false;
        }
        const int dimension = attr.attr.pointer ? attr.attr.pointer->GetDimension() : 0;
        const bool isScalar = attr.attr.type == IG_SCALAR && dimension == 1;
        const bool isVector = attr.attr.type == IG_VECTOR && dimension == 3;
        if (!isScalar && !isVector) {
            reason = "CGNS export supports scalar fields and three-component vector fields only: " + FieldName(attr);
            return false;
        }
    }

    const std::filesystem::path path(fileName);
    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(path.parent_path());
    }

    int fileIndex = 0;
    if (cg_open(fileName.c_str(), CG_MODE_WRITE, &fileIndex) != CG_OK) {
        reason = cg_get_error();
        return false;
    }

    auto closeFile = [&]() {
        if (fileIndex != 0) {
            cg_close(fileIndex);
            fileIndex = 0;
        }
    };

    int baseIndex = 0;
    if (cg_base_write(fileIndex, "Base", 3, 3, &baseIndex) != CG_OK) {
        reason = cg_get_error();
        closeFile();
        return false;
    }

    cgsize_t zoneSize[9] = {static_cast<cgsize_t>(dims[0]), static_cast<cgsize_t>(dims[1]),
                            static_cast<cgsize_t>(dims[2]), static_cast<cgsize_t>(std::max<igIndex>(dims[0] - 1, 0)),
                            static_cast<cgsize_t>(std::max<igIndex>(dims[1] - 1, 0)),
                            static_cast<cgsize_t>(std::max<igIndex>(dims[2] - 1, 0)), 0, 0, 0};
    int zoneIndex = 0;
    if (cg_zone_write(fileIndex, baseIndex, "zone", zoneSize, Structured, &zoneIndex) != CG_OK) {
        reason = cg_get_error();
        closeFile();
        return false;
    }

    // CGNS 坐标按 RealDouble 写出，避免直接导出路径引入 float 坐标截断
    std::vector<double> coordX(pointCount);
    std::vector<double> coordY(pointCount);
    std::vector<double> coordZ(pointCount);
    for (size_t i = 0; i < pointCount; ++i) {
        const auto p = pointSet->GetPoints()->GetPoint(static_cast<IGsize>(i));
        coordX[i] = static_cast<double>(p[0]);
        coordY[i] = static_cast<double>(p[1]);
        coordZ[i] = static_cast<double>(p[2]);
    }

    int coordIndex = 0;
    if (cg_coord_write(fileIndex, baseIndex, zoneIndex, RealDouble, "CoordinateX", coordX.data(), &coordIndex) !=
        CG_OK ||
        cg_coord_write(fileIndex, baseIndex, zoneIndex, RealDouble, "CoordinateY", coordY.data(), &coordIndex) !=
        CG_OK ||
        cg_coord_write(fileIndex, baseIndex, zoneIndex, RealDouble, "CoordinateZ", coordZ.data(), &coordIndex) !=
        CG_OK) {
        reason = cg_get_error();
        closeFile();
        return false;
    }

    int pointSolutionIndex = 0;
    int cellSolutionIndex = 0;
    for (const auto& attr: attrs) {
        // 点 field 和单元 field 分别放到 Vertex 与 CellCenter solution
        int* solutionIndex = nullptr;
        GridLocation_t location = Vertex;
        const char* solutionName = nullptr;
        if (attr.attr.attachmentType == IG_POINT) {
            solutionIndex = &pointSolutionIndex;
            location = Vertex;
            solutionName = "PointData";
        } else {
            solutionIndex = &cellSolutionIndex;
            location = CellCenter;
            solutionName = "CellData";
        }

        if (*solutionIndex == 0 &&
            cg_sol_write(fileIndex, baseIndex, zoneIndex, solutionName, location, solutionIndex) != CG_OK) {
            reason = cg_get_error();
            closeFile();
            return false;
        }

        if (!WriteCgnsField(fileIndex, baseIndex, zoneIndex, *solutionIndex, attr, reason)) {
            closeFile();
            return false;
        }
    }

    closeFile();
    return true;
}

// 把 iGame 普通单元类型转换为当前 CGNS reader 可回读的 CGNS 单元类型
bool CgnsElementTypeForCell(IGenum cellType, ElementType_t& elementType) {
    switch (cellType) {
        case iGame::IG_TRIANGLE:
            elementType = TRI_3;
            return true;
        case iGame::IG_QUAD:
            elementType = QUAD_4;
            return true;
        case iGame::IG_TETRA:
            elementType = TETRA_4;
            return true;
        case iGame::IG_HEXAHEDRON:
            elementType = HEXA_8;
            return true;
        case iGame::IG_PYRAMID:
            elementType = PYRA_5;
            return true;
        case iGame::IG_PRISM:
            elementType = PENTA_6;
            return true;
        default:
            elementType = ElementTypeNull;
            return false;
    }
}

// 写出 unstructured mesh 的 MIXED section，保留单元顺序和点连接
bool WriteUnstructuredMixedElements(int fileIndex, int baseIndex, int zoneIndex,
                                    const iGame::UnstructuredMesh::Pointer& mesh, size_t pointCount,
                                    std::string& reason) {
    auto cells = mesh ? mesh->GetCells() : nullptr;
    auto cellTypes = mesh ? mesh->GetCellTypes() : nullptr;
    if (!cells || !cellTypes) {
        reason = "unstructured mesh has no cells or cell types";
        return false;
    }

    const auto cellCount = static_cast<cgsize_t>(mesh->GetNumberOfCells());
    if (cellCount <= 0) {
        reason = "empty unstructured mesh cells";
        return false;
    }
    if (cellTypes->GetNumberOfValues() != mesh->GetNumberOfCells()) {
        reason = "cell type count mismatch";
        return false;
    }

    int sectionIndex = 0;
    if (cg_section_partial_write(fileIndex, baseIndex, zoneIndex, "Elements", MIXED, 1, cellCount, 0,
                                 &sectionIndex) != CG_OK) {
        reason = cg_get_error();
        return false;
    }

    constexpr cgsize_t chunkCellCount = 100000;
    cgsize_t globalOffset = 0;
    for (cgsize_t firstCell = 0; firstCell < cellCount; firstCell += chunkCellCount) {
        const cgsize_t lastCell = std::min(firstCell + chunkCellCount, cellCount);
        std::vector<cgsize_t> elements;
        std::vector<cgsize_t> offsets;
        elements.reserve(static_cast<size_t>((lastCell - firstCell) * 9));
        offsets.reserve(static_cast<size_t>(lastCell - firstCell + 1));
        offsets.push_back(globalOffset);

        for (cgsize_t cellId = firstCell; cellId < lastCell; ++cellId) {
            // MIXED section 中每个单元先写 CGNS element type，再写 1-based 点 id
            ElementType_t elementType = ElementTypeNull;
            const auto igCellType = static_cast<IGenum>(cellTypes->GetValue(static_cast<IGsize>(cellId)));
            if (!CgnsElementTypeForCell(igCellType, elementType)) {
                reason = "unsupported unstructured CGNS cell type: " + std::to_string(igCellType);
                return false;
            }

            const igIndex* ids = nullptr;
            const int cellPointCount = cells->GetCellIds(static_cast<IGsize>(cellId), ids);
            int expectedPointCount = 0;
            if (cg_npe(elementType, &expectedPointCount) != CG_OK || cellPointCount != expectedPointCount) {
                reason = "cell point count mismatch for CGNS element";
                return false;
            }

            elements.push_back(static_cast<cgsize_t>(elementType));
            for (int i = 0; i < cellPointCount; ++i) {
                if (ids[i] < 0 || static_cast<size_t>(ids[i]) >= pointCount) {
                    reason = "cell point id out of range";
                    return false;
                }
                elements.push_back(static_cast<cgsize_t>(ids[i]) + 1);
            }
            globalOffset += static_cast<cgsize_t>(cellPointCount + 1);
            offsets.push_back(globalOffset);
        }

        if (cg_poly_elements_partial_write(fileIndex, baseIndex, zoneIndex, sectionIndex, firstCell + 1, lastCell,
                                           elements.data(), offsets.data()) != CG_OK) {
            reason = cg_get_error();
            return false;
        }
    }

    return true;
}

// 写出当前 example 支持的 UnstructuredMesh 坐标、单元和 scalar float/double field
bool WriteSimpleUnstructuredCgns(const iGame::DataObject::Pointer& obj, const std::string& fileName,
                                 std::string& reason) {
    auto mesh = iGame::DynamicCast<iGame::UnstructuredMesh>(obj);
    auto pointSet = iGame::DynamicCast<iGame::PointSet>(obj);
    if (!mesh || !pointSet || !pointSet->GetPoints()) {
        reason = "CGNS export supports simple UnstructuredMesh objects only";
        return false;
    }

    const size_t pointCount = static_cast<size_t>(pointSet->GetPoints()->GetNumberOfPoints());
    const size_t cellCount = static_cast<size_t>(mesh->GetNumberOfCells());
    if (pointCount == 0 || cellCount == 0) {
        reason = "empty unstructured mesh";
        return false;
    }

    CompareStats collectStats;
    auto attrs = CollectFloatingFields(obj->GetAttributeSet(), collectStats, false);
    if (attrs.empty()) {
        reason = "no floating fields to export";
        return false;
    }

    for (const auto& attr: attrs) {
        const auto valueCount = attr.attr.pointer ? attr.attr.pointer->GetNumberOfElements() : 0;
        if (attr.attr.attachmentType == IG_POINT && valueCount != static_cast<IGsize>(pointCount)) {
            reason = "point field element count mismatch: " + FieldName(attr);
            return false;
        }
        if (attr.attr.attachmentType == IG_CELL && valueCount != static_cast<IGsize>(cellCount)) {
            reason = "cell field element count mismatch: " + FieldName(attr);
            return false;
        }
        if (attr.attr.attachmentType != IG_POINT && attr.attr.attachmentType != IG_CELL) {
            reason = "unsupported CGNS field attachment: " + FieldName(attr);
            return false;
        }
        const int dimension = attr.attr.pointer ? attr.attr.pointer->GetDimension() : 0;
        const bool isScalar = attr.attr.type == IG_SCALAR && dimension == 1;
        const bool isVector = attr.attr.type == IG_VECTOR && dimension == 3;
        if (!isScalar && !isVector) {
            reason = "CGNS export supports scalar fields and three-component vector fields only: " + FieldName(attr);
            return false;
        }
    }

    const std::filesystem::path path(fileName);
    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(path.parent_path());
    }

    int fileIndex = 0;
    if (cg_open(fileName.c_str(), CG_MODE_WRITE, &fileIndex) != CG_OK) {
        reason = cg_get_error();
        return false;
    }

    auto closeFile = [&]() {
        if (fileIndex != 0) {
            cg_close(fileIndex);
            fileIndex = 0;
        }
    };

    int baseIndex = 0;
    if (cg_base_write(fileIndex, "Base", 3, 3, &baseIndex) != CG_OK) {
        reason = cg_get_error();
        closeFile();
        return false;
    }

    cgsize_t zoneSize[3] = {static_cast<cgsize_t>(pointCount), static_cast<cgsize_t>(cellCount), 0};
    int zoneIndex = 0;
    if (cg_zone_write(fileIndex, baseIndex, "zone", zoneSize, Unstructured, &zoneIndex) != CG_OK) {
        reason = cg_get_error();
        closeFile();
        return false;
    }

    // 坐标统一写为 RealDouble，避免导出阶段降低几何精度
    std::vector<double> coordX(pointCount);
    std::vector<double> coordY(pointCount);
    std::vector<double> coordZ(pointCount);
    for (size_t i = 0; i < pointCount; ++i) {
        const auto p = pointSet->GetPoints()->GetPoint(static_cast<IGsize>(i));
        coordX[i] = static_cast<double>(p[0]);
        coordY[i] = static_cast<double>(p[1]);
        coordZ[i] = static_cast<double>(p[2]);
    }

    int coordIndex = 0;
    if (cg_coord_write(fileIndex, baseIndex, zoneIndex, RealDouble, "CoordinateX", coordX.data(), &coordIndex) !=
        CG_OK ||
        cg_coord_write(fileIndex, baseIndex, zoneIndex, RealDouble, "CoordinateY", coordY.data(), &coordIndex) !=
        CG_OK ||
        cg_coord_write(fileIndex, baseIndex, zoneIndex, RealDouble, "CoordinateZ", coordZ.data(), &coordIndex) !=
        CG_OK) {
        reason = cg_get_error();
        closeFile();
        return false;
    }

    if (!WriteUnstructuredMixedElements(fileIndex, baseIndex, zoneIndex, mesh, pointCount, reason)) {
        closeFile();
        return false;
    }

    int pointSolutionIndex = 0;
    int cellSolutionIndex = 0;
    for (const auto& attr: attrs) {
        // point field 和 cell field 分别写入 Vertex 与 CellCenter solution
        int* solutionIndex = nullptr;
        GridLocation_t location = Vertex;
        const char* solutionName = nullptr;
        if (attr.attr.attachmentType == IG_POINT) {
            solutionIndex = &pointSolutionIndex;
            location = Vertex;
            solutionName = "PointData";
        } else {
            solutionIndex = &cellSolutionIndex;
            location = CellCenter;
            solutionName = "CellData";
        }

        if (*solutionIndex == 0 &&
            cg_sol_write(fileIndex, baseIndex, zoneIndex, solutionName, location, solutionIndex) != CG_OK) {
            reason = cg_get_error();
            closeFile();
            return false;
        }

        if (!WriteCgnsField(fileIndex, baseIndex, zoneIndex, *solutionIndex, attr, reason)) {
            closeFile();
            return false;
        }
    }

    closeFile();
    return true;
}
#endif

// 按源文件类型导出当前 example 支持的对象
bool ExportSupportedObjectAsSourceFormat(const iGame::DataObject::Pointer& obj, const std::string& sourceFile,
                                         const std::string& exportedFile, std::string& reason) {
    const auto ext = LowerExtension(sourceFile);
    if (ext == ".vtk") {
        if (!iGame::FileIO::WriteFile(exportedFile, obj)) {
            reason = "VTK export failed";
            return false;
        }
        return true;
    }

    if (ext == ".cgns") {
#if defined(CGNS_ENABLE)
        if (iGame::DynamicCast<iGame::StructuredMesh>(obj)) {
            return WriteSimpleStructuredCgns(obj, exportedFile, reason);
        }
        if (iGame::DynamicCast<iGame::UnstructuredMesh>(obj)) {
            return WriteSimpleUnstructuredCgns(obj, exportedFile, reason);
        }
        reason = "CGNS export supports simple StructuredMesh or UnstructuredMesh objects only";
        return false;
#else
        reason = "CGNS export requires CGNS_ENABLE";
        return false;
#endif
    }

    reason = "unsupported source extension: " + ext;
    return false;
}

// endregion

// region 输出和 case 结果

// 打印命令行用法
void PrintUsage(const char* exeName) {
    std::cout << "Usage: " << exeName << " source.vtk|source.cgns [...]\n";
}

// 打印 field 检查统计和有限数量的差异详情
void PrintStats(const CompareStats& stats) {
    std::cout << "Checked arrays: " << stats.checkedArrays << "\n";
    std::cout << "Checked values: " << stats.checkedValues << "\n";
    std::cout << "Skipped non-floating arrays: " << stats.skippedArrays << "\n";
    std::cout << "Mismatch arrays: " << stats.mismatchArrays << "\n";
    std::cout << "Mismatch values: " << stats.mismatchValues << "\n";

    if (!stats.messages.empty()) {
        std::cout << "Details:\n";
        for (const auto& message: stats.messages) {
            std::cout << "  " << message << "\n";
        }
    }
}

// 构造失败结果并打印原因
CaseResult FailCase(const std::string& reason) {
    std::cerr << "Result: FAIL\n";
    std::cerr << reason << "\n";
    return {.failed = true};
}

// 构造跳过结果并打印原因
CaseResult SkipCase(const std::string& reason) {
    std::cout << "Result: SKIP\n";
    std::cout << "Reason: " << reason << "\n";
    return {.skipped = true};
}

// 根据比较统计输出 PASS、FAIL 或 SKIP
CaseResult FinishCompare(bool passed, const CompareStats& stats) {
    PrintStats(stats);
    if (stats.checkedArrays == 0) {
        return SkipCase("no floating field arrays");
    }
    if (passed && stats.Passed()) {
        std::cout << "Result: PASS\n";
        return {};
    }
    std::cout << "Result: FAIL\n";
    return {.failed = true};
}

// endregion

// region 无损 IGC 写入

// 使用无损配置写出 IGC，并捕获编码阶段产生的排序 trace
bool WriteLosslessIgc(const iGame::DataObject::Pointer& sourceObj, const std::string& encodedFile,
                      iGame::MeshEncodeTrace& encodeTrace, std::string& reason) {
    // main 版本默认参数上强制关闭几何和 field 的量化
    auto params = iGame::MeshEncoderFilter<iGame::EncodeOutputBinaryArray>::GenerateDefaultCodecParams(sourceObj);
    ForceLossless(params);

    // encodeTrace 后续用于恢复排序，检查阶段不会自行重新排序
    auto writer = iGame::IGCWriter::New();
    writer->SetCodecControlParams(params);
    writer->SetEncodeTrace(&encodeTrace);
    if (!writer->WriteToFile(sourceObj, encodedFile)) {
        reason = "Write IGC failed";
        return false;
    }
    if (!encodeTrace.hasOrderRemaps) {
        reason = "Encode trace was not produced";
        return false;
    }
    return true;
}

// endregion

// region 三条测试路径

// 路径1：原始数据编码解码后直接比对 field
// 读取源文件，写 IGC，读回 IGC，然后按编码 trace 比较 field
CaseResult CheckDecodeRoundTripFields(const std::string& sourceFile) {
    const std::string encodedFile =
        MakeSiblingPath(sourceFile, std::string("_lossless_decode_roundtrip_") + LosslessModeName, ".igc");

    std::cout << "Case: " << sourceFile << "\n";
    std::cout << "Path: decode_roundtrip\n";
    std::cout << "Mode: " << LosslessModeName << "\n";
    std::cout << "Encoded: " << encodedFile << "\n";

    auto sourceObj = iGame::FileIO::ReadFile(sourceFile);
    if (!sourceObj) {
        return FailCase("Read source failed");
    }

    std::string skipReason;
    if (!IsSupportedSingleBlockPointSet(sourceObj, skipReason)) {
        return SkipCase(skipReason);
    }

    // 唯一允许的排序恢复信息在这里捕获
    iGame::MeshEncodeTrace encodeTrace;
    std::string reason;
    if (!WriteLosslessIgc(sourceObj, encodedFile, encodeTrace, reason)) {
        return FailCase(reason);
    }
    auto decodedObj = iGame::FileIO::ReadFile(encodedFile);
    if (!decodedObj) {
        return FailCase("Read decoded IGC failed");
    }

    CompareStats stats;
    const bool passed =
        CompareFloatingFieldsByEncodedOrder("decode_roundtrip", sourceObj, decodedObj, encodeTrace.orderRemaps, stats);
    return FinishCompare(passed, stats);
}

// 路径2：编解码后导出为源格式再读回比对 field
// 读取源文件，写 IGC，读回 IGC，导出源格式，再读回导出文件比较 field
CaseResult CheckCodecExportRoundTripFields(const std::string& sourceFile) {
    const auto suffix = std::string("_lossless_codec_export_roundtrip_") + LosslessModeName;
    const std::string encodedFile = MakeSiblingPath(sourceFile, suffix, ".igc");
    const std::string exportedFile = MakeSiblingPath(sourceFile, suffix, LowerExtension(sourceFile));

    std::cout << "Case: " << sourceFile << "\n";
    std::cout << "Path: codec_export_roundtrip\n";
    std::cout << "Mode: " << LosslessModeName << "\n";
    std::cout << "Encoded: " << encodedFile << "\n";
    std::cout << "Exported: " << exportedFile << "\n";

    auto sourceObj = iGame::FileIO::ReadFile(sourceFile);
    if (!sourceObj) {
        return FailCase("Read source failed");
    }

    std::string skipReason;
    if (!IsSupportedSingleBlockPointSet(sourceObj, skipReason)) {
        return SkipCase(skipReason);
    }

    // 比较导出文件时仍然沿用这次编码产生的排序 trace
    iGame::MeshEncodeTrace encodeTrace;
    std::string reason;
    if (!WriteLosslessIgc(sourceObj, encodedFile, encodeTrace, reason)) {
        return FailCase(reason);
    }
    auto decodedObj = iGame::FileIO::ReadFile(encodedFile);
    if (!decodedObj) {
        return FailCase("Read decoded IGC failed");
    }

    std::string exportReason;
    if (!ExportSupportedObjectAsSourceFormat(decodedObj, sourceFile, exportedFile, exportReason)) {
        return SkipCase(exportReason);
    }

    auto exportedObj = iGame::FileIO::ReadFile(exportedFile);
    if (!exportedObj) {
        return FailCase("Read exported file failed");
    }

    CompareStats stats;
    const bool passed = CompareFloatingFieldsByEncodedOrder("codec_export_roundtrip", sourceObj, exportedObj,
                                                            encodeTrace.orderRemaps, stats);
    return FinishCompare(passed, stats);
}

// 路径3：原始数据直接导出为源格式再读回比对 field
// 读取源文件，直接导出源格式，再读回导出文件比较 field
CaseResult CheckDirectExportRoundTripFields(const std::string& sourceFile) {
    const std::string exportedFile =
        MakeSiblingPath(sourceFile, "_direct_export_roundtrip", LowerExtension(sourceFile));

    std::cout << "Case: " << sourceFile << "\n";
    std::cout << "Path: direct_export_roundtrip\n";
    std::cout << "Exported: " << exportedFile << "\n";

    auto sourceObj = iGame::FileIO::ReadFile(sourceFile);
    if (!sourceObj) {
        return FailCase("Read source failed");
    }

    std::string exportReason;
    if (!ExportSupportedObjectAsSourceFormat(sourceObj, sourceFile, exportedFile, exportReason)) {
        return SkipCase(exportReason);
    }

    auto exportedObj = iGame::FileIO::ReadFile(exportedFile);
    if (!exportedObj) {
        return FailCase("Read exported file failed");
    }

    // 直接导出没有编码重排，使用恒等排序
    const iGame::EncodeOrderRemaps identityOrderRemaps;
    CompareStats stats;
    const bool passed = CompareFloatingFieldsByEncodedOrder("direct_export_roundtrip", sourceObj, exportedObj,
                                                            identityOrderRemaps, stats);
    return FinishCompare(passed, stats);
}

// endregion

// region 汇总统计

struct SummaryStats {
    int failedCount = 0;
    int skippedCount = 0;
    int totalCount = 0;
};

// 累计一个 case 的结果
void RecordResult(const CaseResult& result, SummaryStats& summary) {
    summary.totalCount++;
    if (result.failed) {
        summary.failedCount++;
    }
    if (result.skipped) {
        summary.skippedCount++;
    }
}

// endregion

} // namespace

// region 命令行入口

// 运行启用的验证路径并返回失败状态
int main(int argc, char** argv) {
    if (argc < 2 || std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help") {
        PrintUsage(argv[0]);
        return argc < 2 ? 2 : 0;
    }

    SummaryStats summary;
    for (int i = 1; i < argc; ++i) {
        const std::string sourceFile = argv[i];
        bool hasPreviousCase = false;
        // 统一处理多个路径之间的空行和结果统计
        auto runCase = [&](auto check) {
            if (hasPreviousCase) {
                std::cout << "\n";
            }
            RecordResult(check(sourceFile), summary);
            hasPreviousCase = true;
        };

        if constexpr (RunDecodeRoundTrip) {
            runCase(CheckDecodeRoundTripFields);
        }
        if constexpr (RunCodecExportRoundTrip) {
            runCase(CheckCodecExportRoundTripFields);
        }
        if constexpr (RunDirectExportRoundTrip) {
            runCase(CheckDirectExportRoundTripFields);
        }

        if (i + 1 < argc) {
            std::cout << "\n";
        }
    }

    std::cout << "\nSummary: total=" << summary.totalCount << " failed=" << summary.failedCount
              << " skipped=" << summary.skippedCount << "\n";
    return summary.failedCount == 0 ? 0 : 1;
}

// endregion
