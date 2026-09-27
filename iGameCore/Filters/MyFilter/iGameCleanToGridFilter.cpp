#include "iGameCleanToGridFilter.h"
#include "iGameCell.h"
#include "iGameCellArray.h"
#include "iGameFlatArray.h"

#include <cmath>
#include <iostream>
#include <set>
#include <vector>

IGAME_NAMESPACE_BEGIN

CleanToGridFilter::CleanToGridFilter() {
    this->SetNumberOfInputs(1);
    this->SetNumberOfOutputs(1);
}

CleanToGridFilter::~CleanToGridFilter() {}

// ComputeEffectiveTolerance：绝对模式允许 0；相对模式 bbox 退化时返回 0
double CleanToGridFilter::ComputeEffectiveTolerance(DataObject::Pointer input) {
    if (m_ToleranceIsAbsolute) { return m_AbsoluteTolerance; }

    auto pointSet = DynamicCast<PointSet>(input);
    if (!pointSet) { return 0.0; }

    auto bbox = pointSet->GetBoundingBox();
    double diag = (bbox.max - bbox.min).norm();

    if (diag < 1e-12) { return 0.0; }

    return diag * m_ToleranceFraction;
}

// IsCellDegenerateWithIds：按单元维度判断拓扑退化
bool CleanToGridFilter::IsCellDegenerateWithIds(const igIndex* pointIds, int numPoints, IGenum cellType) {
    if (!pointIds || numPoints < 1) return true;

    int dim = static_cast<int>(Cell::GetCellDimension(static_cast<igIndex>(cellType)));

    if (dim == 0 && cellType != IG_VERTEX && cellType != IG_EMPTY_CELL) {
        std::cerr << "[CleanToGrid] Unknown cell type " << cellType << ", rejecting\n";
        return true;
    }

    std::set<igIndex> uniq(pointIds, pointIds + numPoints);

    int need;
    switch (dim) {
        case 0:
            need = 1;
            break;
        case 1:
            need = 2;
            break;
        case 2:
            need = 3;
            break;
        case 3:
            need = 4;
            break;
        case 4:
            need = 2;
            break;
        default:
            return true;
    }
    return static_cast<int>(uniq.size()) < need;
}

// 完整保留数组类型
ArrayObject::Pointer CleanToGridFilter::CreateArrayByTypeExact(IGenum dataType) {
    switch (dataType) {
        case IG_FloatArray:
            return FloatArray::New();
        case IG_DoubleArray:
            return DoubleArray::New();
        case IG_IntArray:
            return IntArray::New();
        case IG_UnsignedIntArray:
            return UnsignedIntArray::New();
        case IG_CharArray:
            return CharArray::New();
        case IG_UnsignedCharArray:
            return UnsignedCharArray::New();
        case IG_ShortArray:
            return ShortArray::New();
        case IG_UnsignedShortArray:
            return UnsignedShortArray::New();
        case IG_LongLongArray:
            return LongLongArray::New();
        case IG_UnsignedLongLongArray:
            return UnsignedLongLongArray::New();
        default:
            std::cerr << "[CleanToGrid] Unsupported array type " << dataType << ", falling back to DoubleArray\n";
            return DoubleArray::New();
    }
}

// 属性复制：从 newToRepOld 取代表点
ArrayObject::Pointer CleanToGridFilter::CloneAttributeArray(ArrayObject::Pointer src, igIndex newSize,
                                                            const std::vector<igIndex>& oldToNewMap,
                                                            const std::vector<igIndex>& newToRepOld,
                                                            igIndex numOldPoints) {
    if (!src) return nullptr;

    const int dim = src->GetDimension();
    const IGenum dataType = src->GetArrayType();

    ArrayObject::Pointer dst = CreateArrayByTypeExact(dataType);
    if (!dst) {
        std::cerr << "CloneAttributeArray: Failed to create array of type " << dataType << std::endl;
        return nullptr;
    }

    dst->SetName(src->GetName());
    dst->SetDimension(dim);
    dst->Resize(newSize);

    for (igIndex newIdx = 0; newIdx < newSize; ++newIdx) {
        igIndex oldIdx = newToRepOld[newIdx];
        if (oldIdx < 0 || oldIdx >= numOldPoints) continue;

        if (dataType == IG_FloatArray) {
            auto s = DynamicCast<FloatArray>(src);
            auto d = DynamicCast<FloatArray>(dst);
            if (s && d)
                for (int k = 0; k < dim; ++k) d->SetValue(newIdx * dim + k, s->GetValue(oldIdx * dim + k));
        } else if (dataType == IG_DoubleArray) {
            auto s = DynamicCast<DoubleArray>(src);
            auto d = DynamicCast<DoubleArray>(dst);
            if (s && d)
                for (int k = 0; k < dim; ++k) d->SetValue(newIdx * dim + k, s->GetValue(oldIdx * dim + k));
        } else if (dataType == IG_IntArray) {
            auto s = DynamicCast<IntArray>(src);
            auto d = DynamicCast<IntArray>(dst);
            if (s && d)
                for (int k = 0; k < dim; ++k) d->SetValue(newIdx * dim + k, s->GetValue(oldIdx * dim + k));
        } else if (dataType == IG_UnsignedIntArray) {
            auto s = DynamicCast<UnsignedIntArray>(src);
            auto d = DynamicCast<UnsignedIntArray>(dst);
            if (s && d)
                for (int k = 0; k < dim; ++k) d->SetValue(newIdx * dim + k, s->GetValue(oldIdx * dim + k));
        } else if (dataType == IG_UnsignedCharArray) {
            auto s = DynamicCast<UnsignedCharArray>(src);
            auto d = DynamicCast<UnsignedCharArray>(dst);
            if (s && d)
                for (int k = 0; k < dim; ++k) d->SetValue(newIdx * dim + k, s->GetValue(oldIdx * dim + k));
        } else {
            std::vector<double> vals(dim, 0.0);
            src->GetElement(oldIdx, vals.data());
            dst->SetElement(newIdx, vals.data());
        }
    }
    return dst;
}


// 贪心合并 + FirstUsed 策略 + 最终代表点追踪
bool CleanToGridFilter::MergeCoincidentPointsBruteForce(Points::Pointer points, double tolerance,
                                                        std::vector<igIndex>& oldToNewMap,
                                                        std::vector<igIndex>& newToRepOld, igIndex& newPointCount,
                                                        const std::vector<bool>& pointIsUsed) {

    if (!points) return false;
    const igIndex numPoints = points->GetNumberOfPoints();
    if (numPoints == 0) {
        newPointCount = 0;
        newToRepOld.clear();
        oldToNewMap.clear();
        return true;
    }

    // 初始化：每个点默认指向自己（旧索引 -> 旧索引）
    oldToNewMap.assign(numPoints, -1);
    for (igIndex i = 0; i < numPoints; ++i) oldToNewMap[i] = i;

    std::vector<bool> isMerged(numPoints, false);
    const bool exactMode = (tolerance <= 0.0);
    const double tol2 = tolerance * tolerance;
    int mergeCount = 0;

    // ---------- 贪心合并 ----------
    for (igIndex i = 0; i < numPoints; ++i) {
        if (isMerged[i]) continue;

        const Point& pi = points->GetPoint(i);

        for (igIndex j = i + 1; j < numPoints; ++j) {
            if (isMerged[j]) continue;

            const Point& pj = points->GetPoint(j);
            const double dx = static_cast<double>(pi[0]) - static_cast<double>(pj[0]);
            const double dy = static_cast<double>(pi[1]) - static_cast<double>(pj[1]);
            const double dz = static_cast<double>(pi[2]) - static_cast<double>(pj[2]);
            const double d2 = dx * dx + dy * dy + dz * dz;

            const bool coincide = exactMode ? (d2 == 0.0) : (d2 < tol2);
            if (!coincide) continue;

            // FirstUsed：优先保留被引用的点
            if (pointIsUsed[j] && !pointIsUsed[i]) {
                // j 被引用、i 未被引用 → i 合并到 j
                oldToNewMap[i] = j;
                isMerged[i] = true;
                mergeCount++;
                // 把之前合并到 i 的点全部重定向到 j（扫所有 k）
                for (igIndex k = 0; k < numPoints; ++k) {
                    if (oldToNewMap[k] == i) oldToNewMap[k] = j;
                }
                break; // i 被合并，跳出内层循环
            } else {
                // 默认：合并 j 到 i
                oldToNewMap[j] = i;
                isMerged[j] = true;
                mergeCount++;
            }
        }

        if (i % 500 == 0) { this->UpdateProgress(static_cast<double>(i) / numPoints); }
    }
    this->UpdateProgress(1.0);

    std::cout << "[CleanToGrid] Merge: " << numPoints << " -> " << (numPoints - mergeCount) << " points"
              << " (merged " << mergeCount << ", tolerance=" << tolerance << (exactMode ? ", exact)" : ")")
              << std::endl;

    // ---------- 移除未使用点 ----------
    if (m_RemoveUnusedPoints) {
        std::vector<bool> rootUsed(numPoints, false);
        for (igIndex i = 0; i < numPoints; ++i) {
            igIndex rep = oldToNewMap[i];
            if (rep >= 0 && pointIsUsed[i]) rootUsed[rep] = true;
        }
        for (igIndex i = 0; i < numPoints; ++i) {
            igIndex rep = oldToNewMap[i];
            if (rep >= 0 && !rootUsed[rep]) oldToNewMap[i] = -1;
        }
    }

    // ---------- 找"最终代表点"（oldToNewMap[r] == r）----------
    std::vector<igIndex> compactMap(numPoints, -1);
    igIndex nextNew = 0;
    for (igIndex i = 0; i < numPoints; ++i) {
        igIndex rep = oldToNewMap[i];
        if (rep < 0) continue;
        if (compactMap[rep] < 0) compactMap[rep] = nextNew++;
    }

    // ---------- 构建 newToRepOld：新索引 -> 最终代表点的旧索引 ----------
    newToRepOld.assign(nextNew, -1);
    for (igIndex oldRep = 0; oldRep < numPoints; ++oldRep) {
        if (oldToNewMap[oldRep] != oldRep) continue; // 不是最终代表点
        igIndex newIdx = compactMap[oldRep];
        if (newIdx < 0) continue;
        newToRepOld[newIdx] = oldRep;
    }

    // ---------- 应用压缩：旧索引 -> 新索引 ----------
    for (igIndex i = 0; i < numPoints; ++i) {
        if (oldToNewMap[i] < 0) continue;
        oldToNewMap[i] = compactMap[oldToNewMap[i]];
    }
    newPointCount = nextNew;

    return true;
}

// CopyCellAttributes：复制单元属性
void CopyCellAttributes(ArrayObject::Pointer srcArray, ArrayObject::Pointer dstArray, igIndex validCellCount,
                        const std::vector<igIndex>& oldToNewMap, igIndex newPointCount, bool removeDegenerateCells,
                        UnstructuredMesh::Pointer inputMesh, CellArray::Pointer cellArray) {
    if (!srcArray || !dstArray) return;

    int dim = srcArray->GetDimension();
    IGenum dataType = srcArray->GetArrayType();

    igIndex cellIds[IGAME_CELL_MAX_SIZE];
    igIndex newIds[IGAME_CELL_MAX_SIZE];

    igIndex validCellIdx = 0;
    for (igIndex cellIdx = 0; cellIdx < inputMesh->GetNumberOfCells(); cellIdx++) {
        int cellSize = cellArray->GetCellSize(cellIdx);
        if (cellSize <= 0 || static_cast<IGsize>(cellSize) > IGAME_CELL_MAX_SIZE) { continue; }

        int actualSize = inputMesh->GetCellPointIds(cellIdx, cellIds);
        if (actualSize <= 0) { continue; }

        bool hasInvalidPoint = false;
        int validCount = 0;
        for (int j = 0; j < actualSize && j < IGAME_CELL_MAX_SIZE; j++) {
            igIndex newIdx = oldToNewMap[cellIds[j]];
            if (newIdx < 0 || newIdx >= newPointCount) {
                hasInvalidPoint = true;
                break;
            }
            newIds[validCount++] = newIdx;
        }

        if (hasInvalidPoint || validCount < 1) { continue; }

        if (removeDegenerateCells) {
            if (CleanToGridFilter::IsCellDegenerateWithIds(newIds, validCount, inputMesh->GetCellType(cellIdx))) {
                continue;
            }
        }

        // 复制单元属性
        if (dataType == IG_FloatArray) {
            auto src = DynamicCast<FloatArray>(srcArray);
            auto dst = DynamicCast<FloatArray>(dstArray);
            if (src && dst)
                for (int d = 0; d < dim; d++) dst->SetValue(validCellIdx * dim + d, src->GetValue(cellIdx * dim + d));
        } else if (dataType == IG_DoubleArray) {
            auto src = DynamicCast<DoubleArray>(srcArray);
            auto dst = DynamicCast<DoubleArray>(dstArray);
            if (src && dst)
                for (int d = 0; d < dim; d++) dst->SetValue(validCellIdx * dim + d, src->GetValue(cellIdx * dim + d));
        } else if (dataType == IG_IntArray) {
            auto src = DynamicCast<IntArray>(srcArray);
            auto dst = DynamicCast<IntArray>(dstArray);
            if (src && dst)
                for (int d = 0; d < dim; d++) dst->SetValue(validCellIdx * dim + d, src->GetValue(cellIdx * dim + d));
        } else if (dataType == IG_UnsignedIntArray) {
            auto src = DynamicCast<UnsignedIntArray>(srcArray);
            auto dst = DynamicCast<UnsignedIntArray>(dstArray);
            if (src && dst)
                for (int d = 0; d < dim; d++) dst->SetValue(validCellIdx * dim + d, src->GetValue(cellIdx * dim + d));
        } else if (dataType == IG_UnsignedCharArray) {
            auto src = DynamicCast<UnsignedCharArray>(srcArray);
            auto dst = DynamicCast<UnsignedCharArray>(dstArray);
            if (src && dst)
                for (int d = 0; d < dim; d++) dst->SetValue(validCellIdx * dim + d, src->GetValue(cellIdx * dim + d));
        } else {
            std::vector<double> values(dim);
            srcArray->GetElement(cellIdx, values.data());
            dstArray->SetElement(validCellIdx, values.data());
        }

        validCellIdx++;
    }
}

// Execute
bool CleanToGridFilter::Execute() {
    std::cout << "========== CleanToGridFilter::Execute() START ==========" << std::endl;

    auto input = this->GetInput(0);
    if (!input) {
        std::cerr << "CleanToGridFilter: No input data!" << std::endl;
        return false;
    }

    auto inputMesh = UnstructuredMesh::TransDataObjToUnstructuredMesh(input);
    if (!inputMesh) {
        std::cerr << "CleanToGridFilter: Failed to convert to UnstructuredMesh!" << std::endl;
        return false;
    }

    auto points = inputMesh->GetPoints();
    auto cellArray = inputMesh->GetCells();
    auto typeArray = inputMesh->GetCellTypes();

    igIndex numPoints = inputMesh->GetNumberOfPoints();
    igIndex numCells = inputMesh->GetNumberOfCells();

    if (numPoints == 0 || numCells == 0) {
        std::cerr << "CleanToGridFilter: Mesh is empty!" << std::endl;
        return false;
    }

    std::cout << "CleanToGridFilter: Input mesh - " << numPoints << " points, " << numCells << " cells" << std::endl;

    double tolerance = ComputeEffectiveTolerance(input);
    std::cout << "CleanToGridFilter: Tolerance = " << tolerance << std::endl;

    // 标记哪些点被单元引用
    std::vector<bool> pointIsUsed(numPoints, false);
    igIndex cellIds[IGAME_CELL_MAX_SIZE];

    for (igIndex i = 0; i < numCells; i++) {
        int cellSize = cellArray->GetCellSize(i);
        if (cellSize <= 0 || static_cast<IGsize>(cellSize) > IGAME_CELL_MAX_SIZE) { continue; }
        int actualSize = inputMesh->GetCellPointIds(i, cellIds);
        for (int j = 0; j < actualSize; j++) {
            if (cellIds[j] >= 0 && cellIds[j] < numPoints) pointIsUsed[cellIds[j]] = true;
        }
    }

    // 合并重合点
    std::vector<igIndex> oldToNewMap(numPoints, -1);
    std::vector<igIndex> newToRepOld;
    igIndex newPointCount = numPoints;

    if (m_MergePoints) {
        std::cout << "CleanToGridFilter: Starting merge (greedy)..." << std::endl;
        if (!MergeCoincidentPointsBruteForce(points, tolerance, oldToNewMap, newToRepOld, newPointCount, pointIsUsed)) {
            std::cerr << "CleanToGridFilter: Merge failed!" << std::endl;
            return false;
        }
    } else {
        oldToNewMap.assign(numPoints, -1);
        for (igIndex i = 0; i < numPoints; i++) oldToNewMap[i] = i;
        newPointCount = numPoints;
        newToRepOld.assign(numPoints, -1);
        for (igIndex i = 0; i < numPoints; ++i) newToRepOld[i] = i;
    }

    // 创建输出网格的点
    auto outputMesh = UnstructuredMesh::New();
    auto outPoints = Points::New();
    outPoints->Resize(newPointCount);

    for (igIndex ni = 0; ni < newPointCount; ++ni) {
        igIndex oi = newToRepOld[ni];
        if (oi < 0) continue;
        const Point& p = points->GetPoint(oi);
        outPoints->SetPoint(ni, p[0], p[1], p[2]);
    }
    outputMesh->SetPoints(outPoints);

    // 更新单元索引 + 移除退化单元
    auto outCells = CellArray::New();
    auto outTypes = UnsignedIntArray::New();
    igIndex validCellCount = 0;
    igIndex degenerateCount = 0;

    igIndex newIds[IGAME_CELL_MAX_SIZE];

    for (igIndex i = 0; i < numCells; i++) {
        int cellSize = cellArray->GetCellSize(i);
        if (cellSize <= 0 || static_cast<IGsize>(cellSize) > IGAME_CELL_MAX_SIZE) { continue; }

        int actualSize = inputMesh->GetCellPointIds(i, cellIds);
        if (actualSize <= 0) { continue; }

        bool hasInvalidPoint = false;
        int validCount = 0;

        for (int j = 0; j < actualSize && j < IGAME_CELL_MAX_SIZE; j++) {
            igIndex newIdx = oldToNewMap[cellIds[j]];
            if (newIdx < 0 || newIdx >= newPointCount) {
                hasInvalidPoint = true;
                break;
            }
            newIds[validCount++] = newIdx;
        }

        if (hasInvalidPoint || validCount < 1) { continue; }

        if (m_RemoveDegenerateCells) {
            if (IsCellDegenerateWithIds(newIds, validCount, inputMesh->GetCellType(i))) {
                degenerateCount++;
                continue;
            }
        }

        outCells->AddCellIds(newIds, validCount);
        outTypes->AddValue(typeArray->GetValue(i));
        validCellCount++;
    }

    outputMesh->SetCells(outCells, outTypes);

    std::cout << "CleanToGridFilter: After processing - " << validCellCount << " valid cells" << std::endl;
    if (degenerateCount > 0) {
        std::cout << "CleanToGridFilter: Removed " << degenerateCount << " degenerate cells" << std::endl;
    }

    // 复制点属性 + 单元属性
    auto inAttrSet = inputMesh->GetAttributeSet();
    if (inAttrSet) {
        auto outAttrSet = AttributeSet::New();

        // 点属性
        auto pointAttrs = inAttrSet->GetAllPointAttributes();
        if (pointAttrs) {
            for (int i = 0; i < pointAttrs->GetNumberOfElements(); i++) {
                auto& attr = pointAttrs->GetElement(i);
                if (attr.pointer && !attr.isDeleted) {
                    auto newArray =
                            CloneAttributeArray(attr.pointer, newPointCount, oldToNewMap, newToRepOld, numPoints);
                    if (newArray) outAttrSet->AddAttribute(attr.type, attr.attachmentType, newArray);
                }
            }
        }

        // 单元属性
        auto cellAttrs = inAttrSet->GetAllCellAttributes();
        if (cellAttrs) {
            for (int i = 0; i < cellAttrs->GetNumberOfElements(); i++) {
                auto& attr = cellAttrs->GetElement(i);
                if (attr.pointer && !attr.isDeleted) {
                    auto srcArray = attr.pointer;
                    int dim = srcArray->GetDimension();
                    IGenum dataType = srcArray->GetArrayType();

                    ArrayObject::Pointer dstArray = CreateArrayByTypeExact(dataType);
                    if (!dstArray) continue;

                    dstArray->SetName(srcArray->GetName());
                    dstArray->SetDimension(dim);
                    dstArray->Resize(validCellCount);

                    CopyCellAttributes(srcArray, dstArray, validCellCount, oldToNewMap, newPointCount,
                                       m_RemoveDegenerateCells, inputMesh, cellArray);

                    outAttrSet->AddAttribute(attr.type, attr.attachmentType, dstArray);
                }
            }
        }

        outputMesh->SetAttributeSet(outAttrSet);
    }

    this->SetOutput(0, outputMesh);

    std::cout << "CleanToGridFilter: Done!" << std::endl;
    std::cout << "========== CleanToGridFilter::Execute() END ==========" << std::endl;
    return true;
}

IGAME_NAMESPACE_END
