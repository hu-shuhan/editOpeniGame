#include "iGamePointSetToOctreeFilter.h"

#include "iGamePointSet.h"
#include "iGameStructuredMesh.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

IGAME_NAMESPACE_BEGIN

PointSetToOctreeFilter::PointSetToOctreeFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

//------------------------------------------------------------------------------
// 等价于 VTK vtkBoundingBox::ClampDivisions
void PointSetToOctreeFilter::ClampDivisions(igIndex64 targetBins, int divs[3]) {
    for (int i = 0; i < 3; ++i) {
        divs[i] = (divs[i] < 1 ? 1 : divs[i]);
    }
    igIndex64 numBins = static_cast<igIndex64>(divs[0]) * divs[1] * divs[2];
    while (numBins > targetBins) {
        for (int i = 0; i < 3; ++i) {
            divs[i] = (divs[i] > 1 ? (divs[i] - 1) : 1);
        }
        numBins = static_cast<igIndex64>(divs[0]) * divs[1] * divs[2];
    }
}

//------------------------------------------------------------------------------
// 等价于 VTK vtkBoundingBox::ComputeDivisions
void PointSetToOctreeFilter::ComputeDivisions(igIndex64 totalBins, const double minPnt[3],
                                              const double maxPnt[3], double bounds[6],
                                              int divs[3]) {
    // 始终至少产生一个分箱
    totalBins = (totalBins <= 0 ? 1 : totalBins);

    // 计算各边长度，并用有限容差判断退化（零长度）边，避免后续数值爆炸
    int numNonZero = 0, nonZero[3], maxIdx = (-1);
    double max = 0.0, lengths[3];
    for (int i = 0; i < 3; ++i) {
        lengths[i] = maxPnt[i] - minPnt[i];
    }
    double totLen = lengths[0] + lengths[1] + lengths[2];
    const double zeroDetectionTolerance = totLen * (0.001 / 3.);

    for (int i = 0; i < 3; ++i) {
        if (lengths[i] > max) {
            maxIdx = i;
            max = lengths[i];
        }
        if (lengths[i] > zeroDetectionTolerance) {
            nonZero[i] = 1;
            numNonZero++;
        } else {
            nonZero[i] = 0;
        }
    }

    // 完全退化的包围盒：一个任意大小的分箱
    if (numNonZero < 1) {
        divs[0] = divs[1] = divs[2] = 1;
        bounds[0] = minPnt[0] - 0.5;
        bounds[1] = maxPnt[0] + 0.5;
        bounds[2] = minPnt[1] - 0.5;
        bounds[3] = maxPnt[1] + 0.5;
        bounds[4] = minPnt[2] - 0.5;
        bounds[5] = maxPnt[2] + 0.5;
        return;
    }

    // 按包围盒边长比例划分，尽量让分箱接近立方体
    double f = static_cast<double>(totalBins);
    f /= (nonZero[0] ? (lengths[0] / totLen) : 1.0);
    f /= (nonZero[1] ? (lengths[1] / totLen) : 1.0);
    f /= (nonZero[2] ? (lengths[2] / totLen) : 1.0);
    f = std::pow(f, (1.0 / static_cast<double>(numNonZero)));

    for (int i = 0; i < 3; ++i) {
        divs[i] = (nonZero[i] ? static_cast<int>(std::floor(f * lengths[i] / totLen)) : 1);
        divs[i] = (divs[i] < 1 ? 1 : divs[i]);
    }

    // 确保不超过 totalBins
    ClampDivisions(totalBins, divs);

    // 计算最终 bounds，保证非零体积
    double delta = 0.5 * lengths[maxIdx] / static_cast<double>(divs[maxIdx]);
    for (int i = 0; i < 3; ++i) {
        if (nonZero[i]) {
            bounds[2 * i] = minPnt[i];
            bounds[2 * i + 1] = maxPnt[i];
        } else {
            bounds[2 * i] = minPnt[i] - delta;
            bounds[2 * i + 1] = maxPnt[i] + delta;
        }
    }
}

//------------------------------------------------------------------------------
// 等价于 VTK vtkPointSetToOctreeImageFilter::RequestData 的核心流程
bool PointSetToOctreeFilter::Execute() {
    DataObject::Pointer input = GetInput(0);
    if (input == nullptr) { return false; }
    PointSet::Pointer pointSet = DynamicCast<PointSet>(input);
    if (pointSet == nullptr) {
        igError("PointSetToOctreeFilter: input must be a PointSet.");
        return false;
    }

    const IGsize numberOfPoints = pointSet->GetNumberOfPoints();
    if (numberOfPoints == 0) {
        igError("PointSetToOctreeFilter: no input or empty input.");
        return false;
    }

    if (static_cast<IGsize>(m_NumberOfPointsPerCell) > numberOfPoints) {
        igError("PointSetToOctreeFilter: NumberOfPointsPerCell must be less than or equal to "
                "the number of points.");
        return false;
    }

    // 输入点集包围盒（等价 input->GetBounds()）
    double pointSetBounds[6];
    pointSetBounds[0] = pointSetBounds[2] = pointSetBounds[4] = std::numeric_limits<double>::max();
    pointSetBounds[1] = pointSetBounds[3] = pointSetBounds[5] = std::numeric_limits<double>::lowest();
    for (IGsize i = 0; i < numberOfPoints; ++i) {
        const Point& p = pointSet->GetPoint(i);
        const double x = static_cast<double>(p[0]);
        const double y = static_cast<double>(p[1]);
        const double z = static_cast<double>(p[2]);
        pointSetBounds[0] = std::min(pointSetBounds[0], x);
        pointSetBounds[1] = std::max(pointSetBounds[1], x);
        pointSetBounds[2] = std::min(pointSetBounds[2], y);
        pointSetBounds[3] = std::max(pointSetBounds[3], y);
        pointSetBounds[4] = std::min(pointSetBounds[4], z);
        pointSetBounds[5] = std::max(pointSetBounds[5], z);
    }

    // numBuckets = floor(numPoints / numPointsPerCell)（等价 VTK 的浮点除后截断）
    const igIndex64 numBuckets = static_cast<igIndex64>(static_cast<double>(numberOfPoints) /
        static_cast<double>(m_NumberOfPointsPerCell));

    // 计算输出图像的 origin / spacing / dimensions / numberOfCells
    const double minPnt[3] = {pointSetBounds[0], pointSetBounds[2], pointSetBounds[4]};
    const double maxPnt[3] = {pointSetBounds[1], pointSetBounds[3], pointSetBounds[5]};
    double imageBounds[6];
    int nDivs[3];
    ComputeDivisions(numBuckets, minPnt, maxPnt, imageBounds, nDivs);

    const double origin[3] = {imageBounds[0], imageBounds[2], imageBounds[4]};
    const double spacing[3] = {(imageBounds[1] - imageBounds[0]) / static_cast<double>(nDivs[0]),
                               (imageBounds[3] - imageBounds[2]) / static_cast<double>(nDivs[1]),
                               (imageBounds[5] - imageBounds[4]) / static_cast<double>(nDivs[2])};
    const int dimensions[3] = {1 + nDivs[0], 1 + nDivs[1], 1 + nDivs[2]};
    const int extent[6] = {0, nDivs[0], 0, nDivs[1], 0, nDivs[2]};
    const IGsize numberOfCells = static_cast<IGsize>(nDivs[0]) * nDivs[1] * nDivs[2];

    // 创建 octree 单元数组（等价 vtkUnsignedCharArray，值为 0）
    UnsignedCharArray::Pointer octree = UnsignedCharArray::New();
    octree->SetName("octree");
    octree->SetDimension(1);
    octree->Resize(numberOfCells);

    // 若需要处理输入点属性数组，则创建多分量 float 输出数组
    FloatArray::Pointer outField = nullptr;
    ArrayObject::Pointer inFieldArr = nullptr;
    std::vector<FieldFunctions> functions;
    if (m_ProcessInputPointArray) {
        AttributeSet* attrs = pointSet->GetAttributeSet();
        AttributeSet::Attribute inFieldAttr = AttributeSet::Attribute::None();
        if (!m_InputArrayName.empty()) {
            inFieldAttr = attrs->GetAttribute(m_InputArrayName);
        }
        if (inFieldAttr.IsNone()) {
            // 默认取第一个 IG_POINT + IG_SCALAR 属性（等价 VTK 的点数据 SCALARS）
            auto all = attrs->GetAllAttributes();
            for (IGsize i = 0; i < all->GetNumberOfElements(); ++i) {
                auto& a = all->GetElement(i);
                if (!a.isDeleted && a.pointer && a.attachmentType == IG_POINT && a.type == IG_SCALAR) {
                    inFieldAttr = a;
                    break;
                }
            }
        }
        if (!inFieldAttr.pointer) {
            igError("PointSetToOctreeFilter: array to process is null.");
            return false;
        }
        if (inFieldAttr.pointer->GetNumberOfElements() != numberOfPoints) {
            igError("PointSetToOctreeFilter: array to process must have as many tuples as the "
                    "number of points.");
            return false;
        }
        if (inFieldAttr.pointer->GetDimension() != 1) {
            igError("PointSetToOctreeFilter: array to process '{}' must have 1 component.",
                    inFieldAttr.pointer->GetName());
            return false;
        }

        const int numberOfFunctions =
            (m_ComputeLastValue ? 1 : 0) + (m_ComputeMin ? 1 : 0) + (m_ComputeMax ? 1 : 0) +
            ((m_ComputeCount || m_ComputeMean) ? 1 : 0) +
            ((m_ComputeSum || m_ComputeMean) ? 1 : 0) + (m_ComputeMean ? 1 : 0);
        if (numberOfFunctions == 0) {
            igError("PointSetToOctreeFilter: no function has been requested to be computed.");
            return false;
        }

        functions.resize(static_cast<size_t>(numberOfFunctions));
        outField = FloatArray::New();
        outField->SetName(inFieldAttr.pointer->GetName());
        outField->SetDimension(numberOfFunctions);
        int counter = 0;
        if (m_ComputeLastValue) { functions[counter++] = FieldFunctions::LAST_VALUE; }
        if (m_ComputeMin) { functions[counter++] = FieldFunctions::MIN; }
        if (m_ComputeMax) { functions[counter++] = FieldFunctions::MAX; }
        if (m_ComputeCount || m_ComputeMean) { functions[counter++] = FieldFunctions::COUNT; }
        if (m_ComputeSum || m_ComputeMean) { functions[counter++] = FieldFunctions::SUM; }
        if (m_ComputeMean) { functions[counter++] = FieldFunctions::MEAN; }

        // 分配并初始化默认值（等价 VTK 的 defaultValues + SMP 初始化）
        outField->Resize(numberOfCells);
        std::vector<float> defaultValues(functions.size());
        for (size_t i = 0; i < functions.size(); ++i) {
            switch (functions[i]) {
                case FieldFunctions::LAST_VALUE:
                    defaultValues[i] = 0.0f;
                    break;
                case FieldFunctions::MIN:
                    defaultValues[i] = std::numeric_limits<float>::max();
                    break;
                case FieldFunctions::MAX:
                    defaultValues[i] = std::numeric_limits<float>::lowest();
                    break;
                case FieldFunctions::COUNT:
                    defaultValues[i] = 0.0f;
                    break;
                case FieldFunctions::SUM:
                    defaultValues[i] = 0.0f;
                    break;
                case FieldFunctions::MEAN:
                    defaultValues[i] = 0.0f;
                    break;
            }
        }
        for (IGsize cell = 0; cell < numberOfCells; ++cell) {
            float* tuple = outField->RawPointer(cell);
            for (size_t j = 0; j < functions.size(); ++j) {
                tuple[j] = defaultValues[j];
            }
        }

        inFieldArr = inFieldAttr.pointer;
    }

    // 定义输出 StructuredMesh（等价 VTK 的 vtkImageData）
    StructuredMesh::Pointer output = StructuredMesh::New();
    output->SetName(pointSet->GetName());
    igIndex dims[3] = {dimensions[0], dimensions[1], dimensions[2]};
    output->SetDimensionSize(dims);

    Points::Pointer points = Points::New();
    points->Reserve(static_cast<IGsize>(dimensions[0]) * dimensions[1] * dimensions[2]);
    for (int k = 0; k < dimensions[2]; ++k) {
        for (int j = 0; j < dimensions[1]; ++j) {
            for (int i = 0; i < dimensions[0]; ++i) {
                points->AddPoint(static_cast<float>(origin[0] + i * spacing[0]),
                                 static_cast<float>(origin[1] + j * spacing[1]),
                                 static_cast<float>(origin[2] + k * spacing[2]));
            }
        }
    }
    output->SetPoints(points);

    output->GetAttributeSet()->AddScalar(IG_CELL, octree);
    if (outField) {
        output->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_CELL, outField);
    }
    output->GenStructuredCellConnectivities();

    SetOutput(output);

    // 填充 octree 与 field 数组（等价 PointSetToImageFunctor::operator()）
    const double spacing_2[3] = {0.5 * spacing[0], 0.5 * spacing[1], 0.5 * spacing[2]};
    int numFunctions = static_cast<int>(functions.size());
    // 均值函数在下面的归约步骤中计算，故此处不用遍历
    if (inFieldArr && !functions.empty() && functions.back() == FieldFunctions::MEAN) {
        --numFunctions;
    }

    for (IGsize i = 0; i < numberOfPoints; ++i) {
        const Point& inPt = pointSet->GetPoint(i);
        const double p[3] = {static_cast<double>(inPt[0]), static_cast<double>(inPt[1]),
                             static_cast<double>(inPt[2])};

        // 计算 ijk
        int ijk[3];
        ijk[0] = static_cast<int>((p[0] - origin[0]) / spacing[0]);
        ijk[0] = std::clamp(ijk[0], extent[0], extent[1] - 1);
        ijk[1] = static_cast<int>((p[1] - origin[1]) / spacing[1]);
        ijk[1] = std::clamp(ijk[1], extent[2], extent[3] - 1);
        ijk[2] = static_cast<int>((p[2] - origin[2]) / spacing[2]);
        ijk[2] = std::clamp(ijk[2], extent[4], extent[5] - 1);

        // cell id（等价 VTK 的 outCellId = ijk[0] + ijk[1]*extent[1] + ijk[2]*extent[1]*extent[3]）
        const IGsize outCellId = static_cast<IGsize>(ijk[0]) +
            static_cast<IGsize>(ijk[1]) * extent[1] +
            static_cast<IGsize>(ijk[2]) * extent[1] * extent[3];

        // 网格格点（等价 output->GetPoint(outPtId)），再加半间距得到体素中心
        double outPt[3] = {origin[0] + ijk[0] * spacing[0], origin[1] + ijk[1] * spacing[1],
                           origin[2] + ijk[2] * spacing[2]};
        outPt[0] += spacing_2[0];
        outPt[1] += spacing_2[1];
        outPt[2] += spacing_2[2];

        // 计算标量 octree 值（等价 VTK 的位乘积编码方式）
        unsigned int octreeValue = (p[0] > outPt[0] ? 2u : 1u);
        octreeValue *= (p[1] > outPt[1] ? 4u : 1u);
        octreeValue *= (p[2] > outPt[2] ? 16u : 1u);
        octree->ValueAt(outCellId) |= static_cast<unsigned char>(octreeValue);

        if (inFieldArr) {
            float inFieldValue = static_cast<float>(inFieldArr->GetValue(i));
            float* outTuple = outField->RawPointer(outCellId);
            for (int j = 0; j < numFunctions; ++j) {
                switch (functions[j]) {
                    case FieldFunctions::LAST_VALUE:
                        outTuple[j] = inFieldValue;
                        break;
                    case FieldFunctions::MIN:
                        outTuple[j] = std::min(outTuple[j], inFieldValue);
                        break;
                    case FieldFunctions::MAX:
                        outTuple[j] = std::max(outTuple[j], inFieldValue);
                        break;
                    case FieldFunctions::COUNT:
                        outTuple[j] += 1.0f;
                        break;
                    case FieldFunctions::SUM:
                        outTuple[j] += inFieldValue;
                        break;
                    default:
                        break;
                }
            }
        }
    }

    // 归约步骤：计算均值
    if (inFieldArr && !functions.empty() && functions.back() == FieldFunctions::MEAN) {
        const int fullSize = static_cast<int>(functions.size());
        const int meanIndex = fullSize - 1;
        const int sumIndex = fullSize - 2;
        const int countIndex = fullSize - 3;
        for (IGsize cell = 0; cell < numberOfCells; ++cell) {
            float* tuple = outField->RawPointer(cell);
            if (tuple[countIndex] != 0.0f) {
                tuple[meanIndex] = tuple[sumIndex] / tuple[countIndex];
            }
        }
    }

    // ---- 诊断信息（供界面提示）----
    {
        std::string info;
        info += "输出图像维度 " + std::to_string(dimensions[0]) + "×" + std::to_string(dimensions[1]) +
                "×" + std::to_string(dimensions[2]) + "（" +
                std::to_string(static_cast<long long>(numberOfCells)) + " 个体素）；";
        info += "输入 " + std::to_string(static_cast<long long>(numberOfPoints)) +
                " 个点，每体素平均点数参数 " +
                std::to_string(static_cast<long long>(m_NumberOfPointsPerCell)) + "。";
        if (inFieldArr && !functions.empty()) {
            info += " 已对点属性 \"" + inFieldArr->GetName() + "\" 统计 " +
                    std::to_string(static_cast<int>(functions.size())) + " 个分量。";
        } else {
            info += " 未处理点属性数组（仅输出 octree 占用位编码）。";
        }
        m_Message = info;
    }

    return true;
}

IGAME_NAMESPACE_END
