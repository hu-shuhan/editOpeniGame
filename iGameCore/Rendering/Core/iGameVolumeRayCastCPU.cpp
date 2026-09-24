#include "iGameVolumeRayCastCPU.h"

#include "iGameAttributeSet.h"
#include "iGameFlatArray.h"
#include "iGameThreadPool.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <vector>

IGAME_NAMESPACE_BEGIN

namespace {

// ---------------------------------------------------------------------------
// 标量场提取（与 iGameVolumeRayCastGPU::UploadVolumeTexture 口径一致）
// ---------------------------------------------------------------------------

// 将点标量属性转成 float 一维缓冲；向量属性取模长。
bool ExtractScalarBuffer(ArrayObject::Pointer scalarArray, IGsize count,
                         std::vector<float>& out) {
    if (!scalarArray) { return false; }
    if (scalarArray->GetNumberOfElements() < count) { return false; }

    out.resize(static_cast<size_t>(count));
    const int dim = scalarArray->GetDimension();

    if (auto fa = DynamicCast<FloatArray>(scalarArray)) {
        const float* src = fa->RawPointer();
        if (dim <= 1) {
            for (IGsize i = 0; i < count; ++i) { out[i] = src[i]; }
        } else {
            for (IGsize i = 0; i < count; ++i) {
                const float* v = src + i * dim;
                float m = 0.0f;
                for (int d = 0; d < dim; ++d) { m += v[d] * v[d]; }
                out[i] = std::sqrt(m);
            }
        }
        return true;
    }

    if (auto da = DynamicCast<DoubleArray>(scalarArray)) {
        const double* src = da->RawPointer();
        if (dim <= 1) {
            for (IGsize i = 0; i < count; ++i) {
                out[i] = static_cast<float>(src[i]);
            }
        } else {
            for (IGsize i = 0; i < count; ++i) {
                const double* v = src + i * dim;
                double m = 0.0;
                for (int d = 0; d < dim; ++d) { m += v[d] * v[d]; }
                out[i] = static_cast<float>(std::sqrt(m));
            }
        }
        return true;
    }

    // 通用兜底（较慢）
    std::vector<float> element(static_cast<size_t>(std::max(dim, 1)));
    for (IGsize i = 0; i < count; ++i) {
        if (dim <= 1) {
            out[i] = static_cast<float>(scalarArray->GetValue(i));
        } else {
            scalarArray->GetElement(i, element);
            float m = 0.0f;
            for (int d = 0; d < dim; ++d) { m += element[d] * element[d]; }
            out[i] = std::sqrt(m);
        }
    }
    return true;
}

// 结构网格单元数据 -> 点数据（取与顶点相邻单元的平均值，等价于 VTK CellDataToPointData）。
FloatArray::Pointer CellDataToPointData(ArrayObject::Pointer cellData, int ni,
                                        int nj, int nk) {
    const int comp = cellData->GetDimension();
    const int cx = ni - 1;
    const int cy = nj - 1;
    const int cz = nk - 1;

    FloatArray::Pointer out = FloatArray::New();
    out->SetDimension(comp);
    out->Resize(ni * nj * nk);

    std::vector<float> cellVal(static_cast<size_t>(comp));
    std::vector<float> sum(static_cast<size_t>(comp));

    for (int k = 0; k < nk; ++k) {
        const int kmin = std::max(0, k - 1);
        const int kmax = std::min(cz - 1, k);
        for (int j = 0; j < nj; ++j) {
            const int jmin = std::max(0, j - 1);
            const int jmax = std::min(cy - 1, j);
            for (int i = 0; i < ni; ++i) {
                const int imin = std::max(0, i - 1);
                const int imax = std::min(cx - 1, i);
                for (int c = 0; c < comp; ++c) { sum[c] = 0.0f; }
                int cnt = 0;
                for (int ck = kmin; ck <= kmax; ++ck) {
                    for (int cj = jmin; cj <= jmax; ++cj) {
                        for (int ci = imin; ci <= imax; ++ci) {
                            cellData->GetElement(ci + cj * cx + ck * cx * cy,
                                                 cellVal);
                            for (int c = 0; c < comp; ++c) { sum[c] += cellVal[c]; }
                            ++cnt;
                        }
                    }
                }
                for (int c = 0; c < comp; ++c) { sum[c] /= static_cast<float>(cnt); }
                out->SetElement(i + j * ni + k * ni * nj, sum.data());
            }
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// 三线性插值（对标 VolumeRayCast.frag 的 3D 纹理 GL_LINEAR + CLAMP_TO_EDGE 语义）
// ---------------------------------------------------------------------------

// Clamp 到 [0, dims-1]，边界处 frac 置 0（clamp-to-edge）。
static inline void ClampIndex(int& i, float& frac, int n) {
    if (i < 0) {
        i = 0;
        frac = 0.0f;
    } else if (i >= n - 1) {
        i = n - 1;
        frac = 0.0f;
    }
}

static inline float SampleTrilinear(const float* vol, int ni, int nj, int nk,
                                    float x, float y, float z) {
    int ix = static_cast<int>(std::floor(x));
    int iy = static_cast<int>(std::floor(y));
    int iz = static_cast<int>(std::floor(z));
    float fx = x - static_cast<float>(ix);
    float fy = y - static_cast<float>(iy);
    float fz = z - static_cast<float>(iz);
    ClampIndex(ix, fx, ni);
    ClampIndex(iy, fy, nj);
    ClampIndex(iz, fz, nk);

    const int ix1 = (ix + 1 < ni) ? ix + 1 : ix;
    const int iy1 = (iy + 1 < nj) ? iy + 1 : iy;
    const int iz1 = (iz + 1 < nk) ? iz + 1 : iz;

    const int sy = ni;
    const int sz = ni * nj;

    const float c000 = vol[iz * sz + iy * sy + ix];
    const float c100 = vol[iz * sz + iy * sy + ix1];
    const float c010 = vol[iz * sz + iy1 * sy + ix];
    const float c110 = vol[iz * sz + iy1 * sy + ix1];
    const float c001 = vol[iz1 * sz + iy * sy + ix];
    const float c101 = vol[iz1 * sz + iy * sy + ix1];
    const float c011 = vol[iz1 * sz + iy1 * sy + ix];
    const float c111 = vol[iz1 * sz + iy1 * sy + ix1];

    const float c00 = c000 + fx * (c100 - c000);
    const float c10 = c010 + fx * (c110 - c010);
    const float c01 = c001 + fx * (c101 - c001);
    const float c11 = c011 + fx * (c111 - c011);
    const float c0 = c00 + fy * (c10 - c00);
    const float c1 = c01 + fy * (c11 - c01);
    return c0 + fz * (c1 - c0);
}

static inline float SampleMaskTrilinear(const unsigned char* mask,
                                        size_t maskCount, int ni, int nj, int nk,
                                        float x, float y, float z) {
    int ix = static_cast<int>(std::floor(x));
    int iy = static_cast<int>(std::floor(y));
    int iz = static_cast<int>(std::floor(z));
    float fx = x - static_cast<float>(ix);
    float fy = y - static_cast<float>(iy);
    float fz = z - static_cast<float>(iz);
    ClampIndex(ix, fx, ni);
    ClampIndex(iy, fy, nj);
    ClampIndex(iz, fz, nk);

    const int ix1 = (ix + 1 < ni) ? ix + 1 : ix;
    const int iy1 = (iy + 1 < nj) ? iy + 1 : iy;
    const int iz1 = (iz + 1 < nk) ? iz + 1 : iz;

    const int sy = ni;
    const int sz = ni * nj;
    auto get = [&](int i, int j, int k) -> float {
        const size_t idx = static_cast<size_t>(k * sz + j * sy + i);
        if (idx >= maskCount) { return 1.0f; }
        return mask[idx] != 0 ? 1.0f : 0.0f;
    };

    const float c000 = get(ix, iy, iz);
    const float c100 = get(ix1, iy, iz);
    const float c010 = get(ix, iy1, iz);
    const float c110 = get(ix1, iy1, iz);
    const float c001 = get(ix, iy, iz1);
    const float c101 = get(ix1, iy, iz1);
    const float c011 = get(ix, iy1, iz1);
    const float c111 = get(ix1, iy1, iz1);

    const float c00 = c000 + fx * (c100 - c000);
    const float c10 = c010 + fx * (c110 - c010);
    const float c01 = c001 + fx * (c101 - c001);
    const float c11 = c011 + fx * (c111 - c011);
    const float c0 = c00 + fy * (c10 - c00);
    const float c1 = c01 + fy * (c11 - c01);
    return c0 + fz * (c1 - c0);
}

// ---------------------------------------------------------------------------
// 空体素跳过的首不透明阈值 / 屏幕 ROI
// ---------------------------------------------------------------------------

// 计算首个 alpha > 0 的归一化位置对应的标量值。opacity 恒>0 时返回 -FLT_MAX，
// 全透明时返回 +FLT_MAX。
float ComputeOpacityThreshold(const std::vector<unsigned char>& lut, int res,
                              double scalarMin, double scalarMax) {
    if (res < 1 || lut.empty()) { return -FLT_MAX; }
    for (int i = 0; i < res; ++i) {
        if (lut[static_cast<size_t>(i) * 4 + 3] > 0) {
            if (i == 0) { return -FLT_MAX; }
            const double t = static_cast<double>(i) /
                             static_cast<double>(res - 1);
            return static_cast<float>(scalarMin +
                                      t * (scalarMax - scalarMin));
        }
    }
    return FLT_MAX;
}

// 体包围盒 8 角点投影到屏幕，返回其屏幕包围盒（含 padding）。
bool ComputeScreenROI(const igm::mat4& projViewModel, const igm::vec3& boxMin,
                      const igm::vec3& boxMax, int w, int h, int& x0, int& y0,
                      int& x1, int& y1) {
    const float cs[8][3] = {
            {boxMin.x, boxMin.y, boxMin.z}, {boxMax.x, boxMin.y, boxMin.z},
            {boxMin.x, boxMax.y, boxMin.z}, {boxMax.x, boxMax.y, boxMin.z},
            {boxMin.x, boxMin.y, boxMax.z}, {boxMax.x, boxMin.y, boxMax.z},
            {boxMin.x, boxMax.y, boxMax.z}, {boxMax.x, boxMax.y, boxMax.z},
    };

    float minX = FLT_MAX, minY = FLT_MAX, maxX = -FLT_MAX, maxY = -FLT_MAX;
    for (int i = 0; i < 8; ++i) {
        igm::vec4 clip =
                projViewModel * igm::vec4{cs[i][0], cs[i][1], cs[i][2], 1.0f};
        float cw = clip.w;
        if (std::fabs(cw) < 1e-8f) { cw = (cw < 0.0f) ? -1e-8f : 1e-8f; }
        const float ndcX = clip.x / cw;
        const float ndcY = clip.y / cw;
        minX = std::min(minX, ndcX);
        maxX = std::max(maxX, ndcX);
        minY = std::min(minY, ndcY);
        maxY = std::max(maxY, ndcY);
    }

    minX = std::max(-1.0f, std::min(1.0f, minX));
    maxX = std::max(-1.0f, std::min(1.0f, maxX));
    minY = std::max(-1.0f, std::min(1.0f, minY));
    maxY = std::max(-1.0f, std::min(1.0f, maxY));
    if (minX > maxX || minY > maxY) { return false; }

    auto toPxMin = [&](float ndc, int S) {
        return static_cast<int>(std::floor((ndc * 0.5f + 0.5f) * S));
    };
    auto toPxMax = [&](float ndc, int S) {
        return static_cast<int>(std::ceil((ndc * 0.5f + 0.5f) * S)) - 1;
    };

    const int ix0 = std::clamp(toPxMin(minX, w), 0, w - 1);
    const int ix1 = std::clamp(toPxMax(maxX, w), 0, w - 1);
    const int iy0 = std::clamp(toPxMin(minY, h), 0, h - 1);
    const int iy1 = std::clamp(toPxMax(maxY, h), 0, h - 1);
    if (ix0 > ix1 || iy0 > iy1) { return false; }

    const int pad = 8;
    x0 = std::max(0, ix0 - pad);
    y0 = std::max(0, iy0 - pad);
    x1 = std::min(w - 1, ix1 + pad);
    y1 = std::min(h - 1, iy1 + pad);
    return true;
}

} // namespace

// ---------------------------------------------------------------------------
// iGameVolumeRayCastCPU
// ---------------------------------------------------------------------------

iGameVolumeRayCastCPU::iGameVolumeRayCastCPU() = default;

iGameVolumeRayCastCPU::~iGameVolumeRayCastCPU() = default;

bool iGameVolumeRayCastCPU::SetInput(StructuredMesh::Pointer mesh) {
    if (!mesh) { return false; }

    // 同一网格且激活属性未变化时才跳过重提；切换字段需重新提取标量场与数据范围。
    if (m_Input == mesh && m_InputAttributeIndex == mesh->GetAttributeIndex()) {
        return true;
    }
    m_Input = mesh;
    m_InputAttributeIndex = mesh->GetAttributeIndex();

    return ExtractScalarField(mesh);
}

bool iGameVolumeRayCastCPU::ExtractScalarField(StructuredMesh::Pointer mesh) {
    m_ScalarField.clear();
    m_Dims[0] = m_Dims[1] = m_Dims[2] = 0;

    igIndex* dims = mesh->GetDimensionSize();
    const int ni = static_cast<int>(dims[0]);
    const int nj = static_cast<int>(dims[1]);
    const int nk = static_cast<int>(dims[2]);

    if (ni < 2 || nj < 2 || nk < 2) {
        IGAME_RENDERING_WARN(
                "[iGameVolumeRayCastCPU] StructuredMesh is not a 3D volume "
                "(dims = {}, {}, {}).",
                ni, nj, nk);
        return false;
    }

    const IGsize numPoints = mesh->GetNumberOfPoints();
    const IGsize expected = static_cast<IGsize>(ni) * nj * nk;
    if (numPoints < expected) {
        IGAME_RENDERING_WARN(
                "[iGameVolumeRayCastCPU] point count ({}) < dims product ({}).",
                numPoints, expected);
        return false;
    }

    // 字段选择口径与 GPU 后端一致：激活属性 -> 点标量/向量 -> 单元标量/向量。
    ArrayObject::Pointer scalarArray = nullptr;
    bool isCellData = false;
    auto* attrs = mesh->GetAttributeSet();
    if (attrs) {
        const int attrIndex = mesh->GetAttributeIndex();
        if (attrIndex >= 0 &&
            attrIndex < static_cast<int>(attrs->GetNumberOfAttributes())) {
            auto& attr = attrs->GetAttribute(attrIndex);
            if (!attr.IsNone() && attr.pointer) {
                scalarArray = attr.pointer;
                isCellData = (attr.attachmentType == IG_CELL);
            }
        }
        if (!scalarArray) {
            for (int i = 0;
                 i < static_cast<int>(attrs->GetNumberOfAttributes()); ++i) {
                auto& a = attrs->GetAttribute(i);
                if (a.IsNone() || !a.pointer) { continue; }
                if (a.attachmentType == IG_POINT &&
                    (a.type == IG_SCALAR || a.type == IG_VECTOR)) {
                    scalarArray = a.pointer;
                    isCellData = false;
                    break;
                }
            }
        }
        if (!scalarArray) {
            for (int i = 0;
                 i < static_cast<int>(attrs->GetNumberOfAttributes()); ++i) {
                auto& a = attrs->GetAttribute(i);
                if (a.IsNone() || !a.pointer) { continue; }
                if (a.attachmentType == IG_CELL &&
                    (a.type == IG_SCALAR || a.type == IG_VECTOR)) {
                    scalarArray = a.pointer;
                    isCellData = true;
                    break;
                }
            }
        }
    }

    if (!scalarArray) {
        IGAME_RENDERING_WARN("[iGameVolumeRayCastCPU] no usable scalar field.");
        return false;
    }

    ArrayObject::Pointer pointScalars = scalarArray;
    if (isCellData) {
        pointScalars = CellDataToPointData(scalarArray, ni, nj, nk);
    }

    if (!ExtractScalarBuffer(pointScalars, numPoints, m_ScalarField)) {
        IGAME_RENDERING_WARN("[iGameVolumeRayCastCPU] no usable scalar field.");
        m_ScalarField.clear();
        return false;
    }

    // 实际数据范围（标量范围未初始化时的回退）。
    m_DataMin = m_ScalarField.front();
    m_DataMax = m_ScalarField.front();
    for (float v : m_ScalarField) {
        m_DataMin = std::min(m_DataMin, v);
        m_DataMax = std::max(m_DataMax, v);
    }
    if (m_DataMax <= m_DataMin) { m_DataMax = m_DataMin + 1.0f; }

    // 体包围盒（局部空间）。默认按均匀体素网格处理（与 GPU 后端口径一致）。
    const BoundingBox& bbox = mesh->GetBoundingBox();
    m_BoxMin = igm::vec3{static_cast<float>(bbox.min[0]),
                         static_cast<float>(bbox.min[1]),
                         static_cast<float>(bbox.min[2])};
    m_BoxMax = igm::vec3{static_cast<float>(bbox.max[0]),
                         static_cast<float>(bbox.max[1]),
                         static_cast<float>(bbox.max[2])};
    if (m_BoxMax.x - m_BoxMin.x < 1e-8f) { m_BoxMax.x = m_BoxMin.x + 1.0f; }
    if (m_BoxMax.y - m_BoxMin.y < 1e-8f) { m_BoxMax.y = m_BoxMin.y + 1.0f; }
    if (m_BoxMax.z - m_BoxMin.z < 1e-8f) { m_BoxMax.z = m_BoxMin.z + 1.0f; }

    m_Dims[0] = ni;
    m_Dims[1] = nj;
    m_Dims[2] = nk;
    m_BricksDirty = true;
    return true;
}

void iGameVolumeRayCastCPU::SetValidMask(UnsignedCharArray::Pointer mask) {
    m_ValidMask = mask;
    m_BricksDirty = true;
}

void iGameVolumeRayCastCPU::SetTransferFunction(
        iGameVolumeTransferFunction::Pointer tf) {
    if (!tf) { return; }

    m_TransferFunction = tf;
    m_ScalarMin = tf->GetScalarMin();
    m_ScalarMax = tf->GetScalarMax();

    m_LUT = tf->BakeLUT();
    m_LUTResolution = tf->GetLUTResolution();
    m_OpacityThreshold =
            ComputeOpacityThreshold(m_LUT, m_LUTResolution, m_ScalarMin,
                                    m_ScalarMax);
    m_BricksDirty = true;
}

void iGameVolumeRayCastCPU::SetScalarRange(double scalarMin, double scalarMax) {
    if (scalarMax <= scalarMin) { scalarMax = scalarMin + 1e-6; }
    m_ScalarMin = scalarMin;
    m_ScalarMax = scalarMax;
    m_OpacityThreshold =
            ComputeOpacityThreshold(m_LUT, m_LUTResolution, m_ScalarMin,
                                    m_ScalarMax);
    m_BricksDirty = true;
}

void iGameVolumeRayCastCPU::EnsureBricks() {
    if (!m_BricksDirty) { return; }
    m_BricksDirty = false;

    m_BrickMax.clear();
    m_BrickActive.clear();
    m_BrickDims[0] = m_BrickDims[1] = m_BrickDims[2] = 0;

    if (!m_EmptySpaceSkip || m_ScalarField.empty()) { return; }

    const int ni = m_Dims[0];
    const int nj = m_Dims[1];
    const int nk = m_Dims[2];
    const int B = m_BrickSize;
    const int bx = (ni + B - 1) / B;
    const int by = (nj + B - 1) / B;
    const int bz = (nk + B - 1) / B;
    m_BrickDims[0] = bx;
    m_BrickDims[1] = by;
    m_BrickDims[2] = bz;
    const int brickCount = bx * by * bz;

    m_BrickMax.assign(static_cast<size_t>(brickCount), -FLT_MAX);
    m_BrickActive.assign(static_cast<size_t>(brickCount), 0);

    const float* scalar = m_ScalarField.data();
    const unsigned char* mask =
            m_ValidMask ? m_ValidMask->RawPointer() : nullptr;
    const size_t maskCount =
            m_ValidMask ? static_cast<size_t>(m_ValidMask->GetNumberOfElements())
                        : 0;

    ThreadPool::parallelFor(0, brickCount, [&](int start, int end) {
        for (int bii = start; bii < end; ++bii) {
            const int bi = bii % bx;
            const int bj = (bii / bx) % by;
            const int bk = bii / (bx * by);
            const int i0 = bi * B;
            const int i1 = std::min((bi + 1) * B, ni);
            const int j0 = bj * B;
            const int j1 = std::min((bj + 1) * B, nj);
            const int k0 = bk * B;
            const int k1 = std::min((bk + 1) * B, nk);

            float mx = -FLT_MAX;
            bool any = false;
            for (int k = k0; k < k1; ++k) {
                for (int j = j0; j < j1; ++j) {
                    for (int i = i0; i < i1; ++i) {
                        const size_t idx =
                                (static_cast<size_t>(k) * nj + j) * ni + i;
                        const bool valid =
                                (!mask) || (idx < maskCount && mask[idx] != 0);
                        if (!valid) { continue; }
                        any = true;
                        const float v = scalar[idx];
                        if (v > mx) { mx = v; }
                    }
                }
            }
            m_BrickMax[static_cast<size_t>(bii)] = mx;
            m_BrickActive[static_cast<size_t>(bii)] = any ? 1 : 0;
        }
    });
}

void iGameVolumeRayCastCPU::SampleLUT(float scalar, float outColor[3],
                                      float& outAlpha) const {
    const double span = m_ScalarMax - m_ScalarMin;
    float normalized;
    if (span < 1e-12) {
        normalized = 0.5f;
    } else {
        normalized = static_cast<float>((scalar - m_ScalarMin) / span);
        normalized = std::clamp(normalized, 0.0f, 1.0f);
    }

    const int res = m_LUTResolution;
    if (res < 1) {
        outColor[0] = outColor[1] = outColor[2] = 0.5f;
        outAlpha = 0.0f;
        return;
    }

    const float f = normalized * static_cast<float>(res - 1);
    int i0 = static_cast<int>(std::floor(f));
    float frac = f - static_cast<float>(i0);
    i0 = std::clamp(i0, 0, res - 1);
    const int i1 = (i0 + 1 < res) ? i0 + 1 : i0;
    if (i0 >= res - 1) { frac = 0.0f; }

    const unsigned char* p0 = m_LUT.data() + static_cast<size_t>(i0) * 4;
    const unsigned char* p1 = m_LUT.data() + static_cast<size_t>(i1) * 4;
    for (int c = 0; c < 3; ++c) {
        outColor[c] = (static_cast<float>(p0[c]) +
                       frac * (static_cast<float>(p1[c]) -
                               static_cast<float>(p0[c]))) /
                      255.0f;
    }
    outAlpha = (static_cast<float>(p0[3]) +
                frac * (static_cast<float>(p1[3]) - static_cast<float>(p0[3]))) /
               255.0f;
}

bool iGameVolumeRayCastCPU::IntersectBox(const igm::vec3& origin,
                                         const igm::vec3& dir,
                                         const igm::vec3& boxMin,
                                         const igm::vec3& boxMax,
                                         float& tEnter, float& tExit) {
    tEnter = -FLT_MAX;
    tExit = FLT_MAX;

    const float o[3] = {origin.x, origin.y, origin.z};
    const float d[3] = {dir.x, dir.y, dir.z};
    const float mn[3] = {boxMin.x, boxMin.y, boxMin.z};
    const float mx[3] = {boxMax.x, boxMax.y, boxMax.z};

    for (int i = 0; i < 3; ++i) {
        if (std::fabs(d[i]) < 1e-8f) {
            if (o[i] < mn[i] || o[i] > mx[i]) { return false; }
        } else {
            const float invD = 1.0f / d[i];
            float t0 = (mn[i] - o[i]) * invD;
            float t1 = (mx[i] - o[i]) * invD;
            if (t0 > t1) { std::swap(t0, t1); }
            if (t0 > tEnter) { tEnter = t0; }
            if (t1 < tExit) { tExit = t1; }
            if (tExit < tEnter) { return false; }
        }
    }
    return true;
}

void iGameVolumeRayCastCPU::RayCastPixel(
        int px, int py, int w, int h, const igm::mat4& invViewProj,
        const igm::mat4& invModel, const igm::mat4& projViewModel,
        const float* scalarData, const unsigned char* maskData,
        size_t maskCount, float* rgba, float& depth) {
    rgba[0] = rgba[1] = rgba[2] = rgba[3] = 0.0f;
    depth = 0.0f; // 远平面

    const int ni = m_Dims[0];
    const int nj = m_Dims[1];
    const int nk = m_Dims[2];

    // 像素中心 -> NDC（reversed-z：near z=1.0，far z=0.0，对标 VolumeRayCast.frag）。
    const float ndcX = ((static_cast<float>(px) + 0.5f) / w) * 2.0f - 1.0f;
    const float ndcY = ((static_cast<float>(py) + 0.5f) / h) * 2.0f - 1.0f;

    igm::vec4 nearW = invViewProj * igm::vec4{ndcX, ndcY, 1.0f, 1.0f};
    nearW /= nearW.w;
    igm::vec4 farW = invViewProj * igm::vec4{ndcX, ndcY, 0.0f, 1.0f};
    farW /= farW.w;

    const igm::vec4 nearL4 =
            invModel * igm::vec4{nearW.x, nearW.y, nearW.z, 1.0f};
    const igm::vec4 farL4 =
            invModel * igm::vec4{farW.x, farW.y, farW.z, 1.0f};

    const igm::vec3 origin{nearL4.x, nearL4.y, nearL4.z};
    igm::vec3 dir{farL4.x - nearL4.x, farL4.y - nearL4.y, farL4.z - nearL4.z};
    const float dirLen = dir.length();
    if (dirLen < 1e-8f) { return; }
    dir /= dirLen;

    float tEnter = 0.0f;
    float tExit = 0.0f;
    if (!IntersectBox(origin, dir, m_BoxMin, m_BoxMax, tEnter, tExit)) {
        return;
    }
    if (tExit < 0.0f) { return; }

    float t = (tEnter < 0.0f) ? 0.0f : tEnter;
    const float tEnd = tExit;

    float step = m_StepSize;
    if (step <= 0.0f) {
        // 自适应步长（历史行为，与 GPU 后端 VolumeRayCast.frag 口径一致）：
        // 步长 = 本超块内光线弦长 / maxSamples。注意此时「不透明度密度」与步长相关，
        // 各 rank 弦长不同会导致块间明暗不均；并行体绘制请显式 SetStepSize(全局步长)。
        step = (tEnd - t) / static_cast<float>(m_MaxSamples);
        if (step <= 0.0f) { step = 1e-4f; }
    } else if (t > 0.0f) {
        // 全局统一步长：把首个采样点吸附到「以近裁剪面为原点的全局采样栅格」上。
        // 所有 rank 用同一相机/同一投影矩阵，同一像素的光线起点与方向逐位一致，因此
        // 参数 t 是全局量；吸附后相邻超块的采样点在 t 轴上严格接续（既不留缝隙也不
        // 重复），块边界不会出现接缝/明暗跳变（对标参考实现里全局统一的 sample
        // distance + 全局 voxel 尺寸的 ScalarOpacityUnitDistance）。
        const float snapped = std::ceil(t / step) * step;
        if (snapped <= tEnd) { t = snapped; }
    }

    // grid 空间增量步进（避免每步除法）：dirGrid = dir * invBoxSize * (dims-1)。
    const float invSX = 1.0f / (m_BoxMax.x - m_BoxMin.x);
    const float invSY = 1.0f / (m_BoxMax.y - m_BoxMin.y);
    const float invSZ = 1.0f / (m_BoxMax.z - m_BoxMin.z);
    const float dirGx = dir.x * invSX * static_cast<float>(ni - 1);
    const float dirGy = dir.y * invSY * static_cast<float>(nj - 1);
    const float dirGz = dir.z * invSZ * static_cast<float>(nk - 1);

    const float posX = origin.x + dir.x * t;
    const float posY = origin.y + dir.y * t;
    const float posZ = origin.z + dir.z * t;
    float gx = (posX - m_BoxMin.x) * invSX * static_cast<float>(ni - 1);
    float gy = (posY - m_BoxMin.y) * invSY * static_cast<float>(nj - 1);
    float gz = (posZ - m_BoxMin.z) * invSZ * static_cast<float>(nk - 1);

    const bool skipEnabled = m_EmptySpaceSkip && !m_BrickMax.empty();
    const int brickSize = m_BrickSize;
    const int bx = m_BrickDims[0];
    const int by = m_BrickDims[1];
    const int bz = m_BrickDims[2];
    const float* brickMax = m_BrickMax.data();
    const unsigned char* brickActive = m_BrickActive.data();
    const float opacityThreshold = m_OpacityThreshold;

    float aAccum = 0.0f;
    float rAccum = 0.0f;
    float gAccum = 0.0f;
    float bAccum = 0.0f;
    float firstHitT = -1.0f;

    // 采样预算与迭代上限分离：
    //   - samples 只统计「真正做了三线性采样」的步（空体素砖块跳跃是纯加速，不应吃掉
    //     预算，否则半空半实的超块会在大步长下提前截断，产生块状密度错误）；
    //   - iterations 只作死循环保护（砖块跳跃每次至少前进一个砖块 + 半步长）。
    int samples = 0;
    int iterations = 0;
    const int maxIterations = m_MaxSamples + 8192;

    while (t <= tEnd && samples < m_MaxSamples && iterations < maxIterations) {
        ++iterations;

        // ---- 空体素跳过：当前砖块透明/无效则整体跳到砖块出口 ----
        if (skipEnabled) {
            int bi = static_cast<int>(std::floor(gx / brickSize));
            int bj = static_cast<int>(std::floor(gy / brickSize));
            int bk = static_cast<int>(std::floor(gz / brickSize));
            bi = std::clamp(bi, 0, bx - 1);
            bj = std::clamp(bj, 0, by - 1);
            bk = std::clamp(bk, 0, bz - 1);
            const int brickIdx = (bk * by + bj) * bx + bi;
            if (brickActive[brickIdx] == 0 ||
                brickMax[brickIdx] < opacityThreshold) {
                const float loX = static_cast<float>(bi) * brickSize;
                const float hiX = (bi + 1 < bx)
                                          ? static_cast<float>(bi + 1) * brickSize
                                          : static_cast<float>(ni - 1);
                const float loY = static_cast<float>(bj) * brickSize;
                const float hiY = (bj + 1 < by)
                                          ? static_cast<float>(bj + 1) * brickSize
                                          : static_cast<float>(nj - 1);
                const float loZ = static_cast<float>(bk) * brickSize;
                const float hiZ = (bk + 1 < bz)
                                          ? static_cast<float>(bk + 1) * brickSize
                                          : static_cast<float>(nk - 1);

                float dExit = FLT_MAX;
                if (dirGx > 0.0f) {
                    dExit = (hiX - gx) / dirGx;
                } else if (dirGx < 0.0f) {
                    dExit = (loX - gx) / dirGx;
                }
                const float dyExit = (dirGy > 0.0f) ? (hiY - gy) / dirGy
                                    : (dirGy < 0.0f)
                                            ? (loY - gy) / dirGy
                                            : FLT_MAX;
                if (dyExit < dExit) { dExit = dyExit; }
                const float dzExit = (dirGz > 0.0f) ? (hiZ - gz) / dirGz
                                     : (dirGz < 0.0f)
                                             ? (loZ - gz) / dirGz
                                             : FLT_MAX;
                if (dzExit < dExit) { dExit = dzExit; }
                if (dExit < 0.0f) { dExit = 0.0f; }

                const float advance = dExit + step * 0.5f; // 略越过砖块边界
                t += advance;
                gx += dirGx * advance;
                gy += dirGy * advance;
                gz += dirGz * advance;
                continue;
            }
        }

        // 本步会真正采样：计入采样预算。
        ++samples;

        // ---- 三线性采样标量 + 有效点 mask ----
        const float scalarVal =
                SampleTrilinear(scalarData, ni, nj, nk, gx, gy, gz);

        float maskVal = 1.0f;
        if (maskData) {
            maskVal = SampleMaskTrilinear(maskData, maskCount, ni, nj, nk, gx,
                                          gy, gz);
            if (maskVal <= 0.001f) {
                t += step;
                gx += dirGx * step;
                gy += dirGy * step;
                gz += dirGz * step;
                continue;
            }
        }

        float col[3];
        float alpha;
        SampleLUT(scalarVal, col, alpha);
        if (alpha <= 0.0f) {
            t += step;
            gx += dirGx * step;
            gy += dirGy * step;
            gz += dirGz * step;
            continue;
        }

        // ---- 不透明度合成 ----
        // 默认（unitDistance<=0）：直接 alpha 合成，与 GPU 后端（VolumeRayCast.frag）
        // 完全一致且尺度无关；显式设置 unitDistance>0 时启用 Beer-Lambert 单位距离修正
        // （对标 MyCustomVolumePass，此时 unitDistance 需与体数据坐标单位同量级）。
        float alphaStep;
        if (m_UnitDistance > 0.0) {
            const float tau = alpha * (step / static_cast<float>(m_UnitDistance));
            alphaStep = (1.0f - std::exp(-tau)) * maskVal;
        } else {
            alphaStep = alpha * maskVal;
        }
        if (alphaStep <= 0.0f) {
            t += step;
            gx += dirGx * step;
            gy += dirGy * step;
            gz += dirGz * step;
            continue;
        }

        // ---- front-to-back 合成（预乘 alpha） ----
        const float inv = 1.0f - aAccum;
        rAccum += inv * (col[0] * alphaStep);
        gAccum += inv * (col[1] * alphaStep);
        bAccum += inv * (col[2] * alphaStep);
        aAccum += inv * alphaStep;

        if (firstHitT < 0.0f) { firstHitT = t; }

        if (aAccum > 0.995f) {
            aAccum = 1.0f;
            break;
        }

        t += step;
        gx += dirGx * step;
        gy += dirGy * step;
        gz += dirGz * step;
    }

    if (aAccum <= 0.0f) { return; } // 保持透明背景

    // ---- 首命中深度回写（reversed-z：near=1.0，far=0.0） ----
    const float hx = origin.x + dir.x * firstHitT;
    const float hy = origin.y + dir.y * firstHitT;
    const float hz = origin.z + dir.z * firstHitT;
    const igm::vec4 clip =
            projViewModel * igm::vec4{hx, hy, hz, 1.0f};
    float d = clip.z / clip.w;
    d = std::clamp(d, 0.0f, 1.0f);
    depth = d;

    rgba[0] = rAccum; // 预乘 alpha
    rgba[1] = gAccum;
    rgba[2] = bAccum;
    rgba[3] = aAccum;
}

void iGameVolumeRayCastCPU::Render(const igm::mat4& view,
                                   const igm::mat4& proj,
                                   const igm::mat4& modelMatrix,
                                   const igm::uvec2& viewport,
                                   std::vector<unsigned char>& outRGBA,
                                   std::vector<float>& outDepth) {
    const int w = static_cast<int>(viewport.x);
    const int h = static_cast<int>(viewport.y);
    outRGBA.assign(static_cast<size_t>(w) * h * 4, 0);
    outDepth.assign(static_cast<size_t>(w) * h, 0.0f);

    if (m_ScalarField.empty() || w <= 0 || h <= 0) { return; }

    EnsureBricks();

    // 矩阵与 GPU 后端（iGameVolumeRayCastGPU::Render / VolumeRayCast.frag）一致。
    const igm::mat4 projView = proj * view;
    const igm::mat4 invViewProj = projView.invert();
    const igm::mat4 invModel = modelMatrix.invert();
    const igm::mat4 projViewModel = projView * modelMatrix;

    const float* scalarData = m_ScalarField.data();
    const unsigned char* maskData =
            m_ValidMask ? m_ValidMask->RawPointer() : nullptr;
    const size_t maskCount =
            m_ValidMask ? static_cast<size_t>(m_ValidMask->GetNumberOfElements())
                        : 0;

    int roiX0 = 0;
    int roiY0 = 0;
    int roiX1 = w - 1;
    int roiY1 = h - 1;
    if (m_UseScreenROI &&
        !ComputeScreenROI(projViewModel, m_BoxMin, m_BoxMax, w, h, roiX0, roiY0,
                          roiX1, roiY1)) {
        return; // 体完全在屏幕外，保持已清空的背景
    }

    const int stride = m_PixelStride;
    const int roiW = roiX1 - roiX0 + 1;
    const int roiH = roiY1 - roiY0 + 1;
    const int gridW = (roiW + stride - 1) / stride;
    const int gridH = (roiH + stride - 1) / stride;
    const int total = gridW * gridH;
    if (total <= 0) { return; }

    auto toUChar = [](float v) {
        v = std::clamp(v, 0.0f, 1.0f);
        return static_cast<unsigned char>(v * 255.0f + 0.5f);
    };

    // 按"步进后像素块"并行：每个块 stride×stride 独立，线程间写区间互不重叠。
    ThreadPool::parallelFor(0, total, [&](int start, int end) {
        float pixel[4];
        float depth;
        for (int idx = start; idx < end; ++idx) {
            const int sx = idx % gridW;
            const int sy = idx / gridW;
            const int px = roiX0 + sx * stride;
            const int py = roiY0 + sy * stride;

            RayCastPixel(px, py, w, h, invViewProj, invModel, projViewModel,
                         scalarData, maskData, maskCount, pixel, depth);

            const int xEnd = std::min(px + stride, roiX1 + 1);
            const int yEnd = std::min(py + stride, roiY1 + 1);
            const unsigned char r = toUChar(pixel[0]);
            const unsigned char g = toUChar(pixel[1]);
            const unsigned char b = toUChar(pixel[2]);
            const unsigned char a = toUChar(pixel[3]);
            for (int dy = py; dy < yEnd; ++dy) {
                for (int dx = px; dx < xEnd; ++dx) {
                    const size_t p = static_cast<size_t>(dy) * w + dx;
                    outRGBA[p * 4 + 0] = r;
                    outRGBA[p * 4 + 1] = g;
                    outRGBA[p * 4 + 2] = b;
                    outRGBA[p * 4 + 3] = a;
                    outDepth[p] = depth;
                }
            }
        }
    });
}

IGAME_NAMESPACE_END
