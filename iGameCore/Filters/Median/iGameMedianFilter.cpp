#include "iGameMedianFilter.h"

#include <algorithm>
#include <functional>
#include <vector>

IGAME_NAMESPACE_BEGIN

namespace {

/**
 * @brief 标量类型 T 到具体数组类（CharArray / FloatArray / ...）的映射。
 *
 * 【必须用具体数组类 New()，不能用 FlatArray<T>::New()】
 * FlatArray<T> 只是模板基类，GetArrayType() 仅在具体数组类（CharArray、FloatArray ...）
 * 中被覆写；基类沿用 ArrayObject::GetArrayType()，返回 IG_ARRAY_OBJECT(=0)。
 * 属性表、VTK 写出、着色器等一律靠 GetArrayType() 辨认数据类型，
 * 返回 0 时就会落到 unknown —— 界面即显示“未知类型”。
 * 因此这里按 T 映射到具体数组类，保证输出数组的 GetArrayType() 与输入完全一致。
 */
template <typename TValue>
struct ArrayClassOf;

template <> struct ArrayClassOf<char> { using Array = CharArray; };
template <> struct ArrayClassOf<unsigned char> { using Array = UnsignedCharArray; };
template <> struct ArrayClassOf<short> { using Array = ShortArray; };
template <> struct ArrayClassOf<unsigned short> { using Array = UnsignedShortArray; };
template <> struct ArrayClassOf<int> { using Array = IntArray; };
template <> struct ArrayClassOf<unsigned int> { using Array = UnsignedIntArray; };
template <> struct ArrayClassOf<long long> { using Array = LongLongArray; };
template <> struct ArrayClassOf<unsigned long long> { using Array = UnsignedLongLongArray; };
template <> struct ArrayClassOf<float> { using Array = FloatArray; };
template <> struct ArrayClassOf<double> { using Array = DoubleArray; };

/**
 * @brief 对结构化网格点标量做核窗口内中值滤波，输入输出同类型。
 *
 * @tparam T      原始标量类型（char / short / int / long long / float / double 等）。
 * @param mesh    输入结构化网格，取其维度与点序。
 * @param array   输入标量数组（dimension 必须为 1）。
 * @param kx/ky/kz 核大小，各维为奇数（2D 数据 kz 应为 1）。
 * @param onProgress 进度回调（0..1），可为空。
 */
template <typename T>
ArrayObject::Pointer ComputeMedianArray(StructuredMesh::Pointer mesh, ArrayObject::Pointer array, int kx, int ky,
                                        int kz, const std::function<void(double)>& onProgress) {
    auto input = DynamicCast<FlatArray<T>>(array);
    if (input == nullptr || input->GetDimension() != 1) { return nullptr; }

    igIndex dims[3]{1, 1, 1};
    std::copy(mesh->GetDimensionSize(), mesh->GetDimensionSize() + 3, dims);
    const igIndex nx = dims[0];
    const igIndex ny = dims[1];
    const igIndex nz = dims[2] > 1 ? dims[2] : 1;
    const IGsize numPoints = static_cast<IGsize>(nx) * static_cast<IGsize>(ny) * static_cast<IGsize>(nz);
    if (numPoints != mesh->GetNumberOfPoints()) { return nullptr; }

    // 长度校验（评审 P1）：标量数组的值个数必须覆盖网格全部点，否则下面的 in[idx] 会越界读取。
    // 例如 4 点网格只提供 2 个标量时，这里直接拒绝，而不是越界访问不存在的第 3、4 个元素。
    if (input->GetNumberOfValues() < numPoints || input->RawPointer() == nullptr) { return nullptr; }

    // 输出数组：与输入同名、同类型。
    // 注意这里 New() 的是具体数组类（见 ArrayClassOf），从而带出正确的 GetArrayType()；
    // 若用 FlatArray<T>::New()，产出的数组类型枚举为 IG_ARRAY_OBJECT，
    // 属性面板会把该字段显示成“未知类型”。
    auto output = ArrayClassOf<T>::Array::New();
    output->SetName(array->GetName());
    output->SetDimension(1);
    output->Resize(numPoints);

    const T* in = input->RawPointer();
    T* out = output->RawPointer();

    const int kxh = kx / 2; // 中心偏移：-(k/2) .. +(k/2)，整数除法
    const int kyh = ky / 2;
    const int kzh = kz / 2;

    std::vector<T> neighbors; // 中值候选，用原数据类型收集
    neighbors.reserve(static_cast<size_t>(kx) * static_cast<size_t>(ky) * static_cast<size_t>(kz));
    const IGsize progressStep = numPoints > 100 ? numPoints / 100 : 1;

    for (igIndex k = 0; k < nz; ++k) {
        for (igIndex j = 0; j < ny; ++j) {
            for (igIndex i = 0; i < nx; ++i) {
                const IGsize center = static_cast<IGsize>(i) + static_cast<IGsize>(j) * nx +
                                      static_cast<IGsize>(k) * nx * ny;
                if (onProgress && center % progressStep == 0) {
                    onProgress(static_cast<double>(center) / static_cast<double>(numPoints));
                }

                neighbors.clear();
                for (int dk = -kzh; dk <= kzh; ++dk) {
                    const igIndex kk = k + dk;
                    if (kk < 0 || kk >= nz) { continue; } // 越界跳过：不补零、不复制边界
                    for (int dj = -kyh; dj <= kyh; ++dj) {
                        const igIndex jj = j + dj;
                        if (jj < 0 || jj >= ny) { continue; }
                        for (int di = -kxh; di <= kxh; ++di) {
                            const igIndex ii = i + di;
                            if (ii < 0 || ii >= nx) { continue; }
                            const IGsize idx = static_cast<IGsize>(ii) + static_cast<IGsize>(jj) * nx +
                                               static_cast<IGsize>(kk) * nx * ny;
                            neighbors.push_back(in[idx]);
                        }
                    }
                }

                std::sort(neighbors.begin(), neighbors.end()); // 原类型升序排序
                out[center] = neighbors[neighbors.size() / 2]; // 上中位数，写回原类型
            }
        }
    }
    return output;
}

} // namespace

MedianFilter::MedianFilter() {
    this->SetNumberOfInputs(1);
    this->SetNumberOfOutputs(1); 
}

bool MedianFilter::Execute() {
    // 先清空上一次的提示：失败时下面每条分支都会写入具体原因，
    // 成功时保持为空，调用方用 GetMessage().empty() 即可判断“本次执行没有报错”。
    // 提示文案统一用英文：std::string 走的是裸字节，中文字面量在不同源文件编码 /
    // 控制台代码页（GBK / UTF-8）之间会变成乱码，英文可保证跨端一致。
    m_Message.clear();

    auto input = GetInput(0);
    if (input == nullptr) {
        m_Message = "Input data object is null.";
        return false;
    }
    if (input->GetDataObjectType() != IG_STRUCTURED_MESH) {
        m_Message = "Median filter only supports a structured mesh (StructuredMesh).";
        return false;
    }
    auto mesh = DynamicCast<StructuredMesh>(input);
    if (mesh == nullptr) {
        m_Message = "Failed to cast the input to StructuredMesh.";
        return false;
    }

    auto attrs = input->GetAttributeSet();
    if (attrs == nullptr || attrs->GetNumberOfAttributes() == 0) {
        m_Message = "The input model has no attribute.";
        return false;
    }

    int index = m_AttributeIndex;
    if (index < 0 && !m_AttributeName.empty()) { index = attrs->GetAttributeIndex(m_AttributeName); }
    if (index < 0 || index >= static_cast<int>(attrs->GetNumberOfAttributes())) {
        m_Message = "Please select a valid scalar attribute.";
        return false;
    }

    auto& attr = attrs->GetAttribute(index);
    if (attr.isDeleted || attr.pointer == nullptr) {
        m_Message = "The selected attribute is no longer valid, please select it again.";
        return false;
    }
    if (attr.type != IG_SCALAR) {
        m_Message = "Median filter only processes scalar attributes.";
        return false;
    }
    if (attr.attachmentType != IG_POINT) {
        m_Message = "Median filter only processes point scalar attributes.";
        return false;
    }
    if (attr.pointer->GetDimension() != 1) {
        m_Message = "Please extract or select a single-component scalar before running the median filter.";
        return false;
    }

    int kx = m_KernelSize[0], ky = m_KernelSize[1], kz = m_KernelSize[2];
    if (kx < 1 || ky < 1 || kz < 1 || (kx & 1) == 0 || (ky & 1) == 0 || (kz & 1) == 0) {
        m_Message = "Kernel size must be an odd positive number in every dimension.";
        return false;
    }
    igIndex dims[3]{1, 1, 1};
    std::copy(mesh->GetDimensionSize(), mesh->GetDimensionSize() + 3, dims);
    if (dims[2] <= 1) { kz = 1; } // 2D 数据第三维固定为 1

    // 长度校验（评审 P1）：在分派到 ComputeMedianArray 之前先给出明确的失败原因，
    // 避免把"数据不完整"误报成泛泛的 "Median computation failed."。
    const IGsize expectedValues = static_cast<IGsize>(dims[0]) * static_cast<IGsize>(dims[1]) *
                                  static_cast<IGsize>(dims[2] > 1 ? dims[2] : 1);
    if (attr.pointer->GetNumberOfValues() < expectedValues) {
        m_Message = "The selected scalar array has fewer values than the mesh points; incomplete data is rejected.";
        return false;
    }

    auto onProgress = [this](double p) { UpdateProgress(p); };
    ArrayObject::Pointer result;
    switch (attr.pointer->GetArrayType()) {
        case IG_CharArray:
            result = ComputeMedianArray<char>(mesh, attr.pointer, kx, ky, kz, onProgress);
            break;
        case IG_UnsignedCharArray:
            result = ComputeMedianArray<unsigned char>(mesh, attr.pointer, kx, ky, kz, onProgress);
            break;
        case IG_ShortArray:
            result = ComputeMedianArray<short>(mesh, attr.pointer, kx, ky, kz, onProgress);
            break;
        case IG_UnsignedShortArray:
            result = ComputeMedianArray<unsigned short>(mesh, attr.pointer, kx, ky, kz, onProgress);
            break;
        case IG_IntArray:
            result = ComputeMedianArray<int>(mesh, attr.pointer, kx, ky, kz, onProgress);
            break;
        case IG_UnsignedIntArray:
            result = ComputeMedianArray<unsigned int>(mesh, attr.pointer, kx, ky, kz, onProgress);
            break;
        case IG_LongLongArray:
            result = ComputeMedianArray<long long>(mesh, attr.pointer, kx, ky, kz, onProgress);
            break;
        case IG_UnsignedLongLongArray:
            result = ComputeMedianArray<unsigned long long>(mesh, attr.pointer, kx, ky, kz, onProgress);
            break;
        case IG_FloatArray:
            result = ComputeMedianArray<float>(mesh, attr.pointer, kx, ky, kz, onProgress);
            break;
        case IG_DoubleArray:
            result = ComputeMedianArray<double>(mesh, attr.pointer, kx, ky, kz, onProgress);
            break;
        default:
            m_Message = "Unsupported scalar array type.";
            return false;
    }
    if (result == nullptr) {
        m_Message = "Median computation failed.";
        return false;
    }
    // 兜底校验：输出数组的类型枚举必须与输入一致，否则属性面板会显示“未知类型”
    if (result->GetArrayType() != attr.pointer->GetArrayType()) {
        m_Message = "The output array type does not match the input array type.";
        return false;
    }

    // 输出：几何与输入一致（共享点坐标，重建单元连接），仅替换所选标量数组。
    StructuredMesh::Pointer out = StructuredMesh::New();
    out->SetName(input->GetName() + "_Median");

    igIndex outExtent[6]{0, 0, 0, 0, 0, 0};
    std::copy(mesh->GetExtent(), mesh->GetExtent() + 6, outExtent);
    out->SetExtent(outExtent);

    igIndex outDims[3]{dims[0], dims[1], dims[2] > 1 ? dims[2] : 1};
    out->SetDimensionSize(outDims);
    out->SetPoints(mesh->GetPoints());
    out->GenStructuredCellConnectivities();

    auto outAttrs = AttributeSet::New();
    int outIndex = -1;
    int added = 0;
    for (IGsize i = 0; i < attrs->GetNumberOfAttributes(); ++i) {
        auto& a = attrs->GetAttribute(i);
        if (a.isDeleted) { continue; }
        if (static_cast<int>(i) == index) {
            outIndex = added;
            outAttrs->AddAttribute(a.type, a.attachmentType, result);
        } else {
            outAttrs->AddAttribute(a.type, a.attachmentType, a.pointer, a.dataRange);
        }
        ++added;
    }
    out->SetAttributeSet(outAttrs);
    out->SetAttributeIndex(outIndex); // 活动标量保持为被滤波的那个数组

    SetOutput(out);
    UpdateProgress(1.0);
    return true;
}

IGAME_NAMESPACE_END
