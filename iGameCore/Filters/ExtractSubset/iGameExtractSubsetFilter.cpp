#include "ExtractSubset/iGameExtractSubsetFilter.h"
#include "iGamePoints.h"
#include "iGameCellArray.h"
#include "iGameAttributeSet.h"
#include "Log/iGameLogger.h"

#include <cstring>
#include <vector>

IGAME_NAMESPACE_BEGIN

namespace {

ArrayObject::Pointer CreateArrayLike(const ArrayObject::Pointer& inArray) {
    if (inArray == nullptr) return nullptr;
    if (DynamicCast<FloatArray>(inArray))         return FloatArray::New();
    if (DynamicCast<DoubleArray>(inArray))        return DoubleArray::New();
    if (DynamicCast<IntArray>(inArray))           return IntArray::New();
    if (DynamicCast<UnsignedIntArray>(inArray))   return UnsignedIntArray::New();
    if (DynamicCast<LongLongArray>(inArray))      return LongLongArray::New();
    if (DynamicCast<UnsignedLongLongArray>(inArray)) return UnsignedLongLongArray::New();
    if (DynamicCast<CharArray>(inArray))          return CharArray::New();
    if (DynamicCast<UnsignedCharArray>(inArray))  return UnsignedCharArray::New();
    if (DynamicCast<ShortArray>(inArray))         return ShortArray::New();
    if (DynamicCast<UnsignedShortArray>(inArray)) return UnsignedShortArray::New();
    return nullptr;
}

// 按 VOI 的点索引映射，把输入网格的点属性拷贝到输出网格。
//   - 保持原始数组类型、维度 (GetDimension) 以及 DataRange。
//   - 当输出点数为 0 时直接返回。
// 参考 iGameTensorFilter::UpdateGlyphDrawAttributeSet 的写法。
void CopyPointAttributesByVOI(igIndex outPointNum,
                              AttributeSet::Pointer inData,
                              AttributeSet::Pointer outData,
                              const std::vector<igIndex>& pointMap) {
    if (inData == nullptr || outData == nullptr || outPointNum == 0) return;

    auto inPointAttrs = inData->GetAllPointAttributes();
    if (inPointAttrs == nullptr) return;

    const size_t nAttrs = inPointAttrs->GetNumberOfElements();
    for (size_t i = 0; i < nAttrs; ++i) {
        // Attribute 是 struct，字段为 public；这里使用访问器方法
        // 避免 const 限定与 C2662 问题，并贴近 TensorFilter 的风格。
        auto& inAttr = inPointAttrs->GetElement(i);
        if (inAttr.IsNone()) continue;
        ArrayObject::Pointer inArray = inAttr.GetPointer();
        if (inArray == nullptr) continue;

        // 保持原始数组类型；不支持的具体类型则跳过
        ArrayObject::Pointer outArray = CreateArrayLike(inArray);
        if (outArray == nullptr) continue;
        outArray->SetName(inArray->GetName());
        outArray->SetDimension(inArray->GetDimension());
        outArray->Resize(outPointNum);

        // ArrayObject 公共 API 的 GetElement / SetElement 仅暴露
        // int* / float* / double* 三种重载；FlatArray<T> 内部通过
        // static_cast 在三种类型与底层 TValue 之间互转，所以用 double
        // 缓冲即可读 / 写任意具体类型。
        double values[IGAME_CELL_MAX_SIZE] = {0};
        const IGsize inNumElems = inArray->GetNumberOfElements();
        for (igIndex j = 0; j < outPointNum; ++j) {
            igIndex srcPt = pointMap[j];
            if (srcPt >= inNumElems) {
                // 输入无对应点数据：保持 values 已清零的原样写入
                std::memset(values, 0, sizeof(values));
            } else {
                inArray->GetElement(srcPt, values);
            }
            outArray->SetElement(j, values);
        }

        // 沿用 TensorFilter 的 4 参数版本，保留原始 DataRange
        outData->AddAttribute(inAttr.GetType(), IG_POINT,
                              outArray, inAttr.GetDataRange());
    }
}

// 按 VOI 的单元索引映射，把输入网格的单元属性拷贝到输出网格。
//   - 保持原始数组类型、维度 (GetDimension) 以及 DataRange。
//   - 当输出单元数为 0 (例如 1D 切线) 或 cellMap 为空时直接返回。
//   - 当某个输出单元对应的输入单元索引越界 (例如 2D 输入网格上做 IK/JK
//     方向切片时，输入没有 IK/JK 面的单元数据)，跳过该单元以避免越界读。
// 参考 iGameTensorFilter::UpdateGlyphDrawAttributeSet 的写法。
void CopyCellAttributesByVOI(igIndex outCellNum,
                             AttributeSet::Pointer inData,
                             AttributeSet::Pointer outData,
                             const std::vector<igIndex>& cellMap) {
    if (inData == nullptr || outData == nullptr ||
        outCellNum == 0 || cellMap.empty()) return;

    auto inCellAttrs = inData->GetAllCellAttributes();
    if (inCellAttrs == nullptr) return;

    const size_t nAttrs = inCellAttrs->GetNumberOfElements();
    for (size_t i = 0; i < nAttrs; ++i) {
        auto& inAttr = inCellAttrs->GetElement(i);
        if (inAttr.IsNone()) continue;
        ArrayObject::Pointer inArray = inAttr.GetPointer();
        if (inArray == nullptr) continue;

        ArrayObject::Pointer outArray = CreateArrayLike(inArray);
        if (outArray == nullptr) continue;
        outArray->SetName(inArray->GetName());
        outArray->SetDimension(inArray->GetDimension());
        outArray->Resize(outCellNum);

        double values[IGAME_CELL_MAX_SIZE] = {0};
        const IGsize inNumElems = inArray->GetNumberOfElements();
        for (igIndex j = 0; j < outCellNum; ++j) {
            igIndex srcCell = cellMap[j];
            if (srcCell >= inNumElems) {
                // 输入无对应单元数据：清零后写入
                std::memset(values, 0, sizeof(values));
            } else {
                inArray->GetElement(srcCell, values);
            }
            outArray->SetElement(j, values);
        }

        outData->AddAttribute(inAttr.GetType(), IG_CELL,
                              outArray, inAttr.GetDataRange());
    }
}

} // namespace

bool ExtractSubsetFilter::Execute()
{
    UpdateProgress(0);

    m_InputMesh = DynamicCast<StructuredMesh>(GetInput(0));
    if (m_InputMesh == nullptr) {
        return false;
    }

    igIndex* inputSize = m_InputMesh->GetDimensionSize();

    int minI = m_VOI[0];
    int maxI = m_VOI[1];
    int minJ = m_VOI[2];
    int maxJ = m_VOI[3];
    int minK = m_VOI[4];
    int maxK = m_VOI[5];

    if (maxI < 0) maxI = static_cast<int>(inputSize[0]) - 1;
    if (maxJ < 0) maxJ = static_cast<int>(inputSize[1]) - 1;
    if (maxK < 0) maxK = static_cast<int>(inputSize[2]) - 1;

    if (minI < 0 || minI >= inputSize[0] ||
        maxI < 0 || maxI >= inputSize[0] ||
        minJ < 0 || minJ >= inputSize[1] ||
        maxJ < 0 || maxJ >= inputSize[1] ||
        minK < 0 || minK >= inputSize[2] ||
        maxK < 0 || maxK >= inputSize[2]) {
        return false;
    }

    if (minI > maxI || minJ > maxJ || minK > maxK) {
        return false;
    }

    m_OutputMesh = StructuredMesh::New();

    igIndex newSize[3];
    newSize[0] = maxI - minI + 1;
    newSize[1] = maxJ - minJ + 1;
    newSize[2] = maxK - minK + 1;

    m_OutputMesh->SetDimensionSize(newSize);

    // ---------- 1) 构建输出点坐标 + 点索引映射 ----------
    const igIndex outPointNum = newSize[0] * newSize[1] * newSize[2];
    Points::Pointer outputPoints = Points::New();
    outputPoints->Resize(outPointNum);

    // pointIndexMap[outPtIdx] = 对应的输入点 id
    std::vector<igIndex> pointIndexMap(outPointNum);

    igIndex ptIdx = 0;
    for (igIndex k = minK; k <= maxK; ++k) {
        for (igIndex j = minJ; j <= maxJ; ++j) {
            for (igIndex i = minI; i <= maxI; ++i) {
                igIndex oldPtId = m_InputMesh->GetPointIndex(i, j, k);
                Point p = m_InputMesh->GetPoint(oldPtId);
                outputPoints->SetPoint(ptIdx, p);
                pointIndexMap[ptIdx] = oldPtId;
                ++ptIdx;
            }
        }
    }

    m_OutputMesh->SetPoints(outputPoints);
    UpdateProgress(0.3);

    // ---------- 2) 根据维度方向构建单元 ----------
    //  - 3D         (newSize[0,1,2] 均 > 1)            : 六面体体 (Volume)
    //  - 2D IJ 平面 (newSize[2] == 1 && n0,n1 > 1)    : K = minK 层四边形
    //  - 2D IK 平面 (newSize[1] == 1 && n0,n2 > 1)    : J = minJ 层四边形
    //  - 2D JK 平面 (newSize[0] == 1 && n1,n2 > 1)    : I = minI 层四边形
    std::vector<igIndex> cellIndexMap;
    igIndex outCellNum = 0;

    if (newSize[2] > 1 && newSize[1] > 1 && newSize[0] > 1) {
        // ---------- 3D: 六面体体 ----------
        CellArray::Pointer volumes = CellArray::New();

        igIndex vhs[8];
        igIndex tmpvhs[8] = {0,
                             1,
                             1 + newSize[0],
                             newSize[0],
                             newSize[0] * newSize[1],
                             1 + newSize[0] * newSize[1],
                             1 + newSize[0] + newSize[0] * newSize[1],
                             newSize[0] + newSize[0] * newSize[1]};

        outCellNum = (newSize[0] - 1) * (newSize[1] - 1) * (newSize[2] - 1);
        cellIndexMap.resize(outCellNum);

        igIndex cellIdx = 0;
        for (igIndex k = 0; k < newSize[2] - 1; ++k) {
            for (igIndex j = 0; j < newSize[1] - 1; ++j) {
                igIndex st = j * newSize[0] + k * newSize[0] * newSize[1];
                for (igIndex i = 0; i < newSize[0] - 1; ++i) {
                    for (int it = 0; it < 8; ++it) {
                        vhs[it] = st + tmpvhs[it];
                    }
                    volumes->AddCellIds(vhs, 8);
                    cellIndexMap[cellIdx] =
                        m_InputMesh->GetVolumeIndex(minI + i, minJ + j, minK + k);
                    ++cellIdx;
                    ++st;
                }
            }
        }

        m_OutputMesh->SetVolumes(volumes);
        m_OutputMesh->GenStructuredCellConnectivities();
        UpdateProgress(0.7);
    } else if (newSize[1] > 1 && newSize[0] > 1) {
        // ---------- 2D IJ 面 (K = minK 层) ----------
        CellArray::Pointer faces = CellArray::New();

        igIndex fhs[4];
        igIndex tmpfhs[4] = {0, 1, newSize[0] + 1, newSize[0]};

        outCellNum = (newSize[0] - 1) * (newSize[1] - 1);
        cellIndexMap.resize(outCellNum);

        igIndex cellIdx = 0;
        for (igIndex j = 0; j < newSize[1] - 1; ++j) {
            igIndex st = j * newSize[0];
            for (igIndex i = 0; i < newSize[0] - 1; ++i) {
                for (int it = 0; it < 4; ++it) {
                    fhs[it] = st + tmpfhs[it];
                }
                faces->AddCellIds(fhs, 4);
                cellIndexMap[cellIdx] =
                    m_InputMesh->GetVolumeIndex(minI + i, minJ + j, minK);
                ++cellIdx;
                ++st;
            }
        }

        m_OutputMesh->SetFaces(faces);
        // 不调用 BuildStructuredFaces():
        //   1. 我们已按 VOI 手动构造好本分支所需的面；
        //   2. CellArray::Resize 不清空旧数据；
        //   3. BuildStructuredFaces() 没有幂等保护，会 Resize 再 AddCellIds
        //      重新生成全部 3 方向面，导致 2N 个面被重复写入。
        UpdateProgress(0.7);
    } else if (newSize[0] > 1 && newSize[2] > 1) {
        // ---------- 2D IK 面 (J = minJ 层) ----------
        CellArray::Pointer faces = CellArray::New();

        igIndex fhs[4];
        // 在 J = minJ 平面，面 (i', k') 的 4 个顶点为
        //   pt(i', j'=0, k'), pt(i'+1, j'=0, k'),
        //   pt(i'+1, j'=0, k'+1), pt(i', j'=0, k'+1)
        // 相对 pt(i', 0, k') 的偏移量：
        igIndex tmpfhs[4] = {0, 1,
                             1 + newSize[0] * newSize[1],
                             newSize[0] * newSize[1]};

        outCellNum = (newSize[0] - 1) * (newSize[2] - 1);
        cellIndexMap.resize(outCellNum);

        igIndex cellIdx = 0;
        for (igIndex k = 0; k < newSize[2] - 1; ++k) {
            igIndex st = k * newSize[0] * newSize[1]; // pt(0, 0, k')
            for (igIndex i = 0; i < newSize[0] - 1; ++i) {
                for (int it = 0; it < 4; ++it) {
                    fhs[it] = st + tmpfhs[it];
                }
                faces->AddCellIds(fhs, 4);
                cellIndexMap[cellIdx] =
                    m_InputMesh->GetVolumeIndex(minI + i, minJ, minK + k);
                ++cellIdx;
                ++st;
            }
        }

        m_OutputMesh->SetFaces(faces);
        UpdateProgress(0.7);
    } else if (newSize[1] > 1 && newSize[2] > 1) {
        // ---------- 2D JK 面 (I = minI 层) ----------
        CellArray::Pointer faces = CellArray::New();

        igIndex fhs[4];
        // 在 I = minI 平面，面 (j', k') 的 4 个顶点为
        //   pt(i'=0, j', k'), pt(i'=0, j'+1, k'),
        //   pt(i'=0, j'+1, k'+1), pt(i'=0, j', k'+1)
        // 相对 pt(0, j', k') 的偏移量：
        igIndex tmpfhs[4] = {0, newSize[0],
                             newSize[0] + newSize[0] * newSize[1],
                             newSize[0] * newSize[1]};

        outCellNum = (newSize[1] - 1) * (newSize[2] - 1);
        cellIndexMap.resize(outCellNum);

        igIndex cellIdx = 0;
        for (igIndex k = 0; k < newSize[2] - 1; ++k) {
            for (igIndex j = 0; j < newSize[1] - 1; ++j) {
                igIndex st = j * newSize[0] + k * newSize[0] * newSize[1];
                for (int it = 0; it < 4; ++it) {
                    fhs[it] = st + tmpfhs[it];
                }
                faces->AddCellIds(fhs, 4);
                cellIndexMap[cellIdx] =
                    m_InputMesh->GetVolumeIndex(minI, minJ + j, minK + k);
                ++cellIdx;
            }
        }

        m_OutputMesh->SetFaces(faces);
        // 不调用 BuildStructuredFaces():
        //   1. 我们已按 VOI 手动构造好本分支所需的面；
        //   2. CellArray::Resize 不清空旧数据；
        //   3. BuildStructuredFaces() 没有幂等保护，会 Resize 再 AddCellIds
        //      重新生成全部 3 方向面，导致 2N 个面被重复写入。
        UpdateProgress(0.7);
    }
    // else: newSize 中有维度 <=1 且不构成 2D 平面 (例如 1D 线/单点) —— 不构建单元

    UpdateProgress(0.9);

    // ---------- 3) 按 VOI 索引映射拷贝点 / 单元属性 ----------
    //   参考 iGameTensorFilter::UpdateGlyphDrawAttributeSet 的写法，
    //   沿用其 GetAllPointAttributes / GetType / GetPointer / GetDataRange
    //   访问器模式，并对 Cell 属性同步处理。
    AttributeSet::Pointer inAttrSet = m_InputMesh->GetAttributeSet();
    AttributeSet::Pointer outAttrSet = AttributeSet::New();
    IGsize copiedPointAttrs = 0;
    IGsize copiedCellAttrs  = 0;
    if (inAttrSet != nullptr && inAttrSet->GetNumberOfAttributes() > 0) {
        CopyPointAttributesByVOI(outPointNum, inAttrSet, outAttrSet, pointIndexMap);
        CopyCellAttributesByVOI(outCellNum,  inAttrSet, outAttrSet, cellIndexMap);
        copiedPointAttrs = outAttrSet->GetAllPointAttributes()
                                ? outAttrSet->GetAllPointAttributes()->GetNumberOfElements()
                                : 0;
        copiedCellAttrs  = outAttrSet->GetAllCellAttributes()
                                ? outAttrSet->GetAllCellAttributes()->GetNumberOfElements()
                                : 0;
    }
    if (outAttrSet->GetNumberOfAttributes() > 0) {
        m_OutputMesh->SetAttributeSet(outAttrSet);
    }

    UpdateProgress(1.0);

    SetOutput(0, m_OutputMesh);
    return true;
}

IGAME_NAMESPACE_END