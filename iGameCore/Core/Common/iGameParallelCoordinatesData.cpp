#include "iGameParallelCoordinatesData.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iGameThreadPool.h>
#include <limits>
#include <random>
#include <sstream>
#include <unordered_map>
#include <vector>
using namespace std;
IGAME_NAMESPACE_BEGIN

static int VariableDiffValueMaxObjNum = 1000;

// 计算路径总成本
static double calculateTotalCost(const std::vector<int>& solution, const std::vector<std::vector<double>>& diffValue,
                                 const std::vector<int>& variables) {
    double totalCost = 0.0;

    // 构建变量到索引的映射
    std::unordered_map<int, int> var_to_index;
    for (size_t i = 0; i < variables.size(); ++i) { var_to_index[variables[i]] = i; }

    for (size_t i = 0; i < solution.size() - 1; ++i) {
        int idx1 = var_to_index[solution[i]];
        int idx2 = var_to_index[solution[i + 1]];
        totalCost += diffValue[idx1][idx2];
    }

    return totalCost;
}

static double ComputeVariance(const std::vector<double>& data) {
    if (data.empty()) {
        return 0.0; // 空向量返回 0
    }

    // 计算均值
    double sum = 0.0;
    for (double value: data) { sum += value; }
    double mean = sum / data.size();

    // 计算方差
    double variance = 0.0;
    for (double value: data) { variance += (value - mean) * (value - mean); }
    variance /= data.size();

    return variance;
}

static inline double GetPercentValue(double value, double minValue, double maxValue) {
    if (minValue == maxValue) return 0.5;
    return (value - minValue) / (maxValue - minValue);
}

// 动态规划算法求解旅行商问题
static std::vector<int> solveWithDP(const std::vector<std::vector<double>>& diffValue,
                                    const std::vector<int>& variables) {
    int n = variables.size();

    if (n == 0) return {};
    if (n == 1) return {variables[0]};

    // dp[mask][i]: 访问过mask集合中的节点，当前在节点i的最小代价
    int totalStates = 1 << n;
    std::vector<std::vector<double>> dp(totalStates, std::vector<double>(n, std::numeric_limits<double>::max()));
    std::vector<std::vector<int>> parent(totalStates, std::vector<int>(n, -1));

    // 初始化：从每个节点开始
    for (int i = 0; i < n; ++i) { dp[1 << i][i] = 0; }

    // 动态规划
    for (int mask = 1; mask < totalStates; ++mask) {
        for (int i = 0; i < n; ++i) {
            if ((mask & (1 << i)) == 0) continue;

            for (int j = 0; j < n; ++j) {
                if ((mask & (1 << j)) == 0) {
                    int newMask = mask | (1 << j);
                    double newCost = dp[mask][i] + diffValue[i][j];

                    if (newCost < dp[newMask][j]) {
                        dp[newMask][j] = newCost;
                        parent[newMask][j] = i;
                    }
                }
            }
        }
    }

    // 找到最优路径
    double minCost = std::numeric_limits<double>::max();
    int lastNode = -1;
    int fullMask = totalStates - 1;

    for (int i = 0; i < n; ++i) {
        if (dp[fullMask][i] < minCost) {
            minCost = dp[fullMask][i];
            lastNode = i;
        }
    }

    // 回溯构建路径
    std::vector<int> path;
    int currentMask = fullMask;
    int currentNode = lastNode;

    while (currentNode != -1) {
        path.push_back(variables[currentNode]);
        int prevNode = parent[currentMask][currentNode];
        if (prevNode != -1) {
            currentMask &= ~(1 << currentNode);
            currentNode = prevNode;
        } else {
            break;
        }
    }

    std::reverse(path.begin(), path.end());
    return path;
}

// 模拟退火算法
static std::vector<int> solveWithSimulatedAnnealing(const std::vector<std::vector<double>>& diffValue,
                                                    const std::vector<int>& variables) {
    int n = variables.size();

    // 随机数生成器
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> prob(0.0, 1.0);
    std::uniform_int_distribution<int> idxDist(0, n - 1);

    // 模拟退火参数
    double initialTemp = 1000.0;
    double finalTemp = 1e-8;
    double coolingRate = 0.999;
    int iterationsPerTemp = 50;

    // 初始解
    std::vector<int> currentSolution = variables;
    std::shuffle(currentSolution.begin(), currentSolution.end(), rng);
    double currentCost = calculateTotalCost(currentSolution, diffValue, variables);

    std::vector<int> bestSolution = currentSolution;
    double bestCost = currentCost;

    double temp = initialTemp;

    while (temp > finalTemp) {
        for (int i = 0; i < iterationsPerTemp; ++i) {
            // 生成邻域解
            std::vector<int> newSolution = currentSolution;

            // 随机选择两种邻域操作
            if (prob(rng) < 0.5) {
                // 交换两个随机位置
                int idx1 = idxDist(rng);
                int idx2 = idxDist(rng);
                std::swap(newSolution[idx1], newSolution[idx2]);
            } else {
                // 反转一段子序列
                int idx1 = idxDist(rng);
                int idx2 = idxDist(rng);
                if (idx1 > idx2) std::swap(idx1, idx2);
                std::reverse(newSolution.begin() + idx1, newSolution.begin() + idx2 + 1);
            }

            double newCost = calculateTotalCost(newSolution, diffValue, variables);
            double costDiff = newCost - currentCost;

            // 接受准则
            if (costDiff < 0 || std::exp(-costDiff / temp) > prob(rng)) {
                currentSolution = newSolution;
                currentCost = newCost;

                if (currentCost < bestCost) {
                    bestSolution = currentSolution;
                    bestCost = currentCost;
                }
            }
        }

        temp *= coolingRate;
    }

    return bestSolution;
}

void ParallelCoordinatesData::SetFilterMaxValue(const std::vector<double>& filterMaxValue) {
    m_FilterMaxValue = filterMaxValue;
}

const std::vector<double>& ParallelCoordinatesData::GetFilterMaxValue() { return m_FilterMaxValue; }

std::vector<double>& ParallelCoordinatesData::FilterMaxValue() { return m_FilterMaxValue; }

void ParallelCoordinatesData::SetFilterMinValue(const std::vector<double>& filterMinValue) {
    m_FilterMinValue = filterMinValue;
}

const std::vector<double>& ParallelCoordinatesData::GetFilterMinValue() { return m_FilterMinValue; }

std::vector<double>& ParallelCoordinatesData::FilterMinValue() { return m_FilterMinValue; }

void ParallelCoordinatesData::SetVariableDiffValue(const std::vector<std::vector<double>>& variableDiffValue) {
    m_VariableDiffValue = variableDiffValue;
}

const std::vector<std::vector<double>>& ParallelCoordinatesData::GetVariableDiffValue() { return m_VariableDiffValue; }

std::vector<int> ParallelCoordinatesData::GenerateDefaultVariableSort(int variableNum) {
    std::vector<int> re(variableNum);
    for (int i = 0; i < variableNum; i++) re[i] = i;
    return re;
}

std::vector<std::vector<double>> ParallelCoordinatesData::GenerateVariableDiffValue(
        int variableNum, ElementArray<AttributeSet::Attribute>::Pointer attrs, IGenum dataType, int objNum,
        int maxObjNum, const std::vector<double>& minValues, const std::vector<double>& maxValues) {
    auto keyObjIds = CtxPresObjData_Main::GenerateKeyObjectIds(objNum, maxObjNum);
    std::vector<std::vector<double>> variableObjValues(variableNum, std::vector<double>(keyObjIds.size()));
    auto variableIndexs_ = CtxPresObjData_Main::GenerateVariableIndex(attrs, dataType);
    for (int objIndex = 0; objIndex < keyObjIds.size(); objIndex++) {
        auto& objId = keyObjIds[objIndex];
        for (int variableIndex = 0; variableIndex < variableNum; variableIndex++) {
            auto& variableIndex_ = variableIndexs_[variableIndex];
            variableObjValues[variableIndex][objIndex] =
                    CtxPresObjData_Main::GenerateObjData(objId, attrs, variableIndex_);
        }
    }
    std::vector<std::vector<std::vector<double>>> betweenDiff(
            variableNum, std::vector<std::vector<double>>(variableNum, std::vector<double>(keyObjIds.size(), 0.0)));
    for (int variableIndexA = 0; variableIndexA < variableNum - 1; variableIndexA++) {
        for (int variableIndexB = variableIndexA + 1; variableIndexB < variableNum; variableIndexB++) {
            for (int objIndex = 0; objIndex < keyObjIds.size(); objIndex++) {
                betweenDiff[variableIndexA][variableIndexB][objIndex] =
                        GetPercentValue(variableObjValues[variableIndexA][objIndex], minValues[variableIndexA],
                                        maxValues[variableIndexA]) -
                        GetPercentValue(variableObjValues[variableIndexB][objIndex], minValues[variableIndexB],
                                        maxValues[variableIndexB]);
            }
        }
    }
    std::vector<std::vector<double>> re(variableNum, std::vector<double>(variableNum, 0.0));
    for (int variableIndexA = 0; variableIndexA < variableNum - 1; variableIndexA++) {
        for (int variableIndexB = variableIndexA + 1; variableIndexB < variableNum; variableIndexB++) {
            auto variance = ComputeVariance(betweenDiff[variableIndexA][variableIndexB]);
            auto standardDeviation = std::sqrt(variance);
            re[variableIndexA][variableIndexB] = standardDeviation;
            re[variableIndexB][variableIndexA] = standardDeviation;
        }
    }
    return re;
}

std::vector<int>
ParallelCoordinatesData::GenerateVariableSortByDiffValue(const std::vector<int>& variableSort,
                                                         const std::vector<std::vector<double>>& diffValue) {

    // 1. 处理特殊情况
    if (variableSort.empty()) { return {}; }

    if (variableSort.size() == 1) { return variableSort; }

    // 检查矩阵是否对称
    size_t n = diffValue.size();
    for (size_t i = 0; i < n; ++i) {
        if (diffValue[i].size() != n) {
            // 矩阵不是方阵，返回原排序
            return variableSort;
        }
        for (size_t j = i + 1; j < n; ++j) {
            if (std::abs(diffValue[i][j] - diffValue[j][i]) > 1e-10) {
                // 矩阵不对称，返回原排序
                return variableSort;
            }
        }
    }

    // 创建变量索引到值的映射
    std::vector<int> variables = variableSort;
    std::sort(variables.begin(), variables.end());
    variables.erase(std::unique(variables.begin(), variables.end()), variables.end());

    size_t m = variables.size();

    // 构建变量索引映射
    std::unordered_map<int, int> var_to_index;
    for (size_t i = 0; i < m; ++i) { var_to_index[variables[i]] = i; }

    // 构建子差异矩阵
    std::vector<std::vector<double>> subDiff(m, std::vector<double>(m, 0.0));
    for (size_t i = 0; i < m; ++i) {
        for (size_t j = 0; j < m; ++j) { subDiff[i][j] = diffValue[variables[i]][variables[j]]; }
    }

    // 2. 根据变量数量选择算法
    std::vector<int> result;

    if (m <= 9) {
        // 使用精确的动态规划算法（旅行商问题）
        result = solveWithDP(subDiff, variables);
    } else {
        // 使用模拟退火算法
        result = solveWithSimulatedAnnealing(subDiff, variables);
    }

    return result;
}

void ParallelCoordinatesData::SetVariableSort(const std::vector<int>& variableSort) { m_VariableSort = variableSort; }

const std::vector<int>& ParallelCoordinatesData::GetVariableSort() { return m_VariableSort; }

ParallelCoordinatesData::Pointer ParallelCoordinatesData::New(ElementArray<AttributeSet::Attribute>::Pointer attrs,
                                                              IGenum dataType, const std::set<igIndex>& selectedItems,
                                                              int objNum) {
    auto variableNames = ParallelCoordinatesData::GenerateVariableNames(attrs, dataType);
    int variableNum = variableNames.size();
    if (variableNum == 0) return ParallelCoordinatesData::Pointer();
    auto Data = ParallelCoordinatesData::New();
    Data->SetAttributes(attrs);
    Data->SetObjectNum(objNum);
    Data->SetVariableNum(variableNum);
    Data->SetVariableSort(ParallelCoordinatesData::GenerateDefaultVariableSort(variableNum));
    Data->SetVariableName(variableNames);
    auto variableIndex = ParallelCoordinatesData::GenerateVariableIndex(attrs, dataType);
    Data->SetVariableIndex(variableIndex);
    auto keyObjIds = ParallelCoordinatesData::GenerateKeyObjectIds(objNum, 10000);
    Data->SetKeyObjectIds(keyObjIds);
    Data->SetKeyObjectIdToIndexMap(ParallelCoordinatesData::GenerateKeyObjectIdToIndexs(keyObjIds));
    Data->SetObjectDrawSorts(ParallelCoordinatesData::GenerateObjectDrawSorts(variableNum, keyObjIds, Data));
    Data->SetDefaultColor(ParallelCoordinatesData::GenerateDefaultColor(Data->GetUnChoosedLight()));
    auto& choosedObjIds = selectedItems;
    Data->SetChoosedObjectIds(choosedObjIds);
    Data->SetChoosedObjectDrawSorts(ParallelCoordinatesData::GenerateObjectDrawSorts(variableNum, choosedObjIds, Data));
    Data->SetChoosedDefaultColor(ParallelCoordinatesData::GenerateDefaultColor(Data->GetChoosedLight()));
    auto [minValue, maxValue] = ParallelCoordinatesData::GenerateMinMaxData(attrs, dataType);
    Data->SetMinValueInVariables(minValue);
    Data->SetMaxValueInVariables(maxValue);
    Data->SetFilterMinValue(minValue);
    Data->SetFilterMaxValue(maxValue);
    Data->SetVariableDiffValue(ParallelCoordinatesData::GenerateVariableDiffValue(
            variableNum, attrs, dataType, objNum, VariableDiffValueMaxObjNum, minValue, maxValue));
    Data->SetDataType(dataType);
    Data->SetDataTypeName(ParallelCoordinatesData::GenerateDataTypeName(dataType));
    return Data;
}

ParallelCoordinatesData::Pointer ParallelCoordinatesData::New(ElementArray<AttributeSet::Attribute>::Pointer attrs,
                                                              IGenum dataType) {
    auto variableNames = ParallelCoordinatesData::GenerateVariableNames(attrs, dataType);
    int variableNum = variableNames.size();
    if (variableNum == 0) return ParallelCoordinatesData::Pointer();
    int objNum = ParallelCoordinatesData::GetLegalAttrsObjNum(attrs, dataType);
    auto Data = ParallelCoordinatesData::New();
    Data->SetAttributes(attrs);
    Data->SetObjectNum(objNum);
    Data->SetVariableNum(variableNum);
    Data->SetVariableSort(ParallelCoordinatesData::GenerateDefaultVariableSort(variableNum));
    Data->SetVariableName(variableNames);
    auto variableIndex = ParallelCoordinatesData::GenerateVariableIndex(attrs, dataType);
    Data->SetVariableIndex(variableIndex);
    auto keyObjIds = ParallelCoordinatesData::GenerateKeyObjectIds(objNum, 10000);
    Data->SetKeyObjectIds(keyObjIds);
    Data->SetKeyObjectIdToIndexMap(ParallelCoordinatesData::GenerateKeyObjectIdToIndexs(keyObjIds));
    Data->SetObjectDrawSorts(ParallelCoordinatesData::GenerateObjectDrawSorts(variableNum, keyObjIds, Data));
    Data->SetDefaultColor(ParallelCoordinatesData::GenerateDefaultColor(Data->GetUnChoosedLight()));
    //auto choosedObjDatas = ParallelCoordinatesData::GenerateChoosedObjectDatas(selectedItems, attrs, dataType);
    //Data->SetChoosedObjectDatas(choosedObjDatas);
    //Data->SetChoosedObjectDrawSorts(ParallelCoordinatesData::GenerateObjectDrawSorts(variableNum, choosedObjDatas));
    Data->SetChoosedObjectDrawSorts(ParallelCoordinatesData::GenerateDefaultObjectDrawSorts(variableNum));
    Data->SetChoosedDefaultColor(ParallelCoordinatesData::GenerateDefaultColor(Data->GetChoosedLight()));
    auto [minValue, maxValue] = ParallelCoordinatesData::GenerateMinMaxData(attrs, dataType);
    Data->SetMinValueInVariables(minValue);
    Data->SetMaxValueInVariables(maxValue);
    Data->SetFilterMinValue(minValue);
    Data->SetFilterMaxValue(maxValue);
    Data->SetVariableDiffValue(ParallelCoordinatesData::GenerateVariableDiffValue(
            variableNum, attrs, dataType, objNum, VariableDiffValueMaxObjNum, minValue, maxValue));
    Data->SetDataType(dataType);
    Data->SetDataTypeName(ParallelCoordinatesData::GenerateDataTypeName(dataType));
    return Data;
}

std::vector<igIndex>
ParallelCoordinatesData::FiltInRangeIds(const std::map<int, std::pair<double, double>>& variableMinMaxValues) {
    auto& attrs = m_Attrs;
    auto& objNum = m_ObjNum;
    std::vector<igIndex> ids;
    static mutex IDS_MUTEX;
    ThreadPool::parallelFor(0, objNum, [&](int st, int ed) {
        std::vector<igIndex> tempIds;
        for (int objId = st; objId < ed; objId++) {
            for (auto& variableMinMaxValue_: variableMinMaxValues) {
                auto& variableId = variableMinMaxValue_.first;
                auto& variableIndex = this->GetVariableIndex()[variableId];
                auto& minValue = variableMinMaxValue_.second.first;
                auto& maxValue = variableMinMaxValue_.second.second;
                auto value =
                        attrs->GetElement(variableIndex.first).pointer->GetElementValue(objId, variableIndex.second);
                if (value < minValue || maxValue < value) continue;
                if (value < this->GetFilterMinValue()[variableId] || this->GetFilterMaxValue()[variableId] < value)
                    continue;
                tempIds.push_back(objId);
                break;
            }
        }
        lock_guard lg(IDS_MUTEX);
        ids.insert(ids.end(), tempIds.begin(), tempIds.end());
    });
    return ids;
}

bool ParallelCoordinatesData::NotInFilterValueRange(int objId) {
    for (int variableIndex = 0; variableIndex < m_VariableNum; variableIndex++) {
        if (GetObjectData(objId, variableIndex) < GetFilterMinValue()[variableIndex] ||
            GetFilterMaxValue()[variableIndex] < GetObjectData(objId, variableIndex))
            return true;
    }
    return false;
}

void ParallelCoordinatesData::SetDefaultSelectionFunc(const std::string& funcName, Selection* selection) {
    selection->_SetSelectionCallBackEvent(funcName, &ParallelCoordinatesData::DefaultSelectionCallBackFunc, this,
                                          std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    selection->_SetClearSelectionCallBackEvent(funcName, &ParallelCoordinatesData::DefaultClearSelectionCallBackFunc,
                                               this);
}

void ParallelCoordinatesData::DefaultSelectionCallBackFunc(IGenum itemType, const std::vector<igIndex>& ids,
                                                           Selection::Operate ope) {
    auto Data = this;
    if (Data->GetDataType() != itemType) return;
    switch (ope) {
        case Selection::Add:
            for (auto& id: ids) { Data->AddChoosedObjectId(id); }
            break;
        case Selection::Remove:
            for (auto& id: ids) { Data->RemoveChoosedObjectId(id); }
            break;
        default:
            break;
    }
}

void ParallelCoordinatesData::DefaultClearSelectionCallBackFunc() {
    auto Data = this;
    Data->ClearChoosedObjectIds();
}

IGAME_NAMESPACE_END