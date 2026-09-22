#include "iGameVolumeRayCastGPU.h"

#include "iGameRenderingLogger.h"

#include <algorithm>
#include <cmath>
#include <vector>

IGAME_NAMESPACE_BEGIN

iGameVolumeRayCastGPU::iGameVolumeRayCastGPU() {
    m_VolumeTexture = GLTexture3d::New();
    m_LUTTexture = GLTexture2d::New();
    m_EmptyVAO = GLVertexArray::New();
}

iGameVolumeRayCastGPU::~iGameVolumeRayCastGPU() {}

void iGameVolumeRayCastGPU::EnsureGLResources() {
    if (m_GLReady) { return; }

    if (!m_VolumeTexture->Handle()) { m_VolumeTexture->Create(); }
    if (!m_LUTTexture->Handle()) { m_LUTTexture->Create(); }
    if (!m_EmptyVAO->Handle()) { m_EmptyVAO->Create(); }

    m_GLReady = true;
}

bool iGameVolumeRayCastGPU::SetInput(StructuredMesh::Pointer mesh) {
    if (!mesh) { return false; }

    EnsureGLResources();

    if (m_Input == mesh) { return true; }
    m_Input = mesh;

    return UploadVolumeTexture(mesh);
}

void iGameVolumeRayCastGPU::SetTransferFunction(
        iGameVolumeTransferFunction::Pointer tf) {
    if (!tf) { return; }

    EnsureGLResources();
    m_TransferFunction = tf;

    m_ScalarMin = static_cast<float>(tf->GetScalarMin());
    m_ScalarMax = static_cast<float>(tf->GetScalarMax());

    UploadLUTTexture();
}

namespace
{
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
// 仅在体渲染上传时调用，不修改读入的模型本身。
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
                            cellData->GetElement(ci + cj * cx + ck * cx * cy, cellVal);
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
} // namespace

bool iGameVolumeRayCastGPU::UploadVolumeTexture(
        StructuredMesh::Pointer mesh) {
    igIndex* dims = mesh->GetDimensionSize();
    const int ni = static_cast<int>(dims[0]);
    const int nj = static_cast<int>(dims[1]);
    const int nk = static_cast<int>(dims[2]);

    // 阶段 1 只处理真正的 3D 体数据。
    if (ni < 2 || nj < 2 || nk < 2) {
        IGAME_RENDERING_WARN(
                "[iGameVolumeRayCastGPU] StructuredMesh is not a 3D volume "
                "(dims = {}, {}, {}).",
                ni, nj, nk);
        return false;
    }

    const IGsize numPoints = mesh->GetNumberOfPoints();
    const IGsize expected = static_cast<IGsize>(ni) * nj * nk;
    if (numPoints < expected) {
        IGAME_RENDERING_WARN(
                "[iGameVolumeRayCastGPU] point count ({}) < dims product ({}).",
                numPoints, expected);
        return false;
    }

    // 选择标量场：优先当前激活属性；否则第一个点标量/点向量；
    // 若只有单元数据，则按结构网格做单元->点转换（仅在体渲染上传时进行）。
    ArrayObject::Pointer scalarArray = nullptr;
    bool isCellData = false;
    auto* attrs = mesh->GetAttributeSet();
    if (attrs) {
        const int attrIndex = mesh->GetAttributeIndex();
        if (attrIndex >= 0 &&
            attrIndex < static_cast<int>(attrs->GetNumberOfAttributes())) {
            auto& attr = attrs->GetAttribute(attrIndex);
            if (!attr.IsNone()) {
                scalarArray = attr.pointer;
                isCellData = (attr.attachmentType == IG_CELL);
            }
        }
        if (!scalarArray) {
            for (int i = 0; i < static_cast<int>(attrs->GetNumberOfAttributes()); ++i) {
                auto& a = attrs->GetAttribute(i);
                if (a.IsNone()) { continue; }
                if (a.attachmentType == IG_POINT &&
                    (a.type == IG_SCALAR || a.type == IG_VECTOR)) {
                    scalarArray = a.pointer;
                    isCellData = false;
                    break;
                }
            }
        }
        if (!scalarArray) {
            for (int i = 0; i < static_cast<int>(attrs->GetNumberOfAttributes()); ++i) {
                auto& a = attrs->GetAttribute(i);
                if (a.IsNone()) { continue; }
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
        IGAME_RENDERING_WARN("[iGameVolumeRayCastGPU] no usable scalar field.");
        return false;
    }

    ArrayObject::Pointer pointScalars = scalarArray;
    if (isCellData) {
        pointScalars = CellDataToPointData(scalarArray, ni, nj, nk);
    }

    std::vector<float> buffer;
    if (!ExtractScalarBuffer(pointScalars, numPoints, buffer)) {
        IGAME_RENDERING_WARN("[iGameVolumeRayCastGPU] no usable scalar field.");
        return false;
    }

    // 记录实际数据范围，作为颜色映射范围未初始化时的回退。
    m_DataMin = buffer.front();
    m_DataMax = buffer.front();
    for (float v : buffer) {
        m_DataMin = std::min(m_DataMin, v);
        m_DataMax = std::max(m_DataMax, v);
    }
    if (m_DataMax <= m_DataMin) { m_DataMax = m_DataMin + 1.0f; }

    // 体包围盒（局部空间）。默认按均匀体素网格处理。
    const BoundingBox& bbox = mesh->GetBoundingBox();
    m_BoxMin = igm::vec3{static_cast<float>(bbox.min[0]),
                         static_cast<float>(bbox.min[1]),
                         static_cast<float>(bbox.min[2])};
    m_BoxMax = igm::vec3{static_cast<float>(bbox.max[0]),
                         static_cast<float>(bbox.max[1]),
                         static_cast<float>(bbox.max[2])};
    // 防止退化包围盒导致除以零。
    for (int i = 0; i < 3; ++i) {
        if (m_BoxMax[i] - m_BoxMin[i] < 1e-8f) { m_BoxMax[i] = m_BoxMin[i] + 1.0f; }
    }

    m_VolumeTexture->Storage(1, GL_R32F, static_cast<unsigned>(ni),
                             static_cast<unsigned>(nj),
                             static_cast<unsigned>(nk));
    m_VolumeTexture->Parameteri(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    m_VolumeTexture->Parameteri(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    m_VolumeTexture->Parameteri(GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    m_VolumeTexture->Parameteri(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    m_VolumeTexture->Parameteri(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    m_VolumeTexture->SubImage(0, 0, 0, 0, static_cast<unsigned>(ni),
                              static_cast<unsigned>(nj),
                              static_cast<unsigned>(nk), GL_RED, GL_FLOAT,
                              buffer.data());

    return true;
}

void iGameVolumeRayCastGPU::UploadLUTTexture() {
    if (!m_TransferFunction) { return; }

    const int resolution = m_TransferFunction->GetLUTResolution();
    const auto lut = m_TransferFunction->BakeLUT();

    if (m_LUTResolution != resolution) {
        m_LUTTexture->Storage(1, GL_RGBA8, static_cast<unsigned>(resolution), 1);
        m_LUTTexture->Parameteri(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        m_LUTTexture->Parameteri(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        m_LUTTexture->Parameteri(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        m_LUTTexture->Parameteri(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        m_LUTResolution = resolution;
    }

    m_LUTTexture->SubImage(0, 0, 0, static_cast<unsigned>(resolution), 1,
                           GL_RGBA, GL_UNSIGNED_BYTE, lut.data());
}

void iGameVolumeRayCastGPU::Render(SmartPointer<GLShaderProgram> shader,
                                   SmartPointer<Camera> camera,
                                   const igm::mat4& modelMatrix,
                                   const igm::uvec2& viewport) {
    if (!shader || !camera || !m_Input) { return; }
    if (!m_VolumeTexture->Handle() || !m_LUTTexture->Handle()) { return; }

    shader->Use();

    const igm::mat4 proj = camera->GetProjectionMatrix();
    const igm::mat4 view = camera->GetViewMatrix();
    const igm::mat4 projView = proj * view;
    const igm::mat4 invViewProj = projView.invert();
    const igm::mat4 invModel = modelMatrix.invert();
    const igm::mat4 projViewModel = projView * modelMatrix;

    shader->SetUniformMatrix4x4("uInvViewProj", false, invViewProj);
    shader->SetUniformMatrix4x4("uInvModel", false, invModel);
    shader->SetUniformMatrix4x4("uProjViewModel", false, projViewModel);

    shader->SetUniform3f("uBoxMin", m_BoxMin);
    shader->SetUniform3f("uBoxMax", m_BoxMax);
    shader->SetUniform2f("uScalarRange", igm::vec2{m_ScalarMin, m_ScalarMax});
    shader->SetUniformf("uStepSize", m_StepSize);
    shader->SetUniformi("uMaxSamples", m_MaxSamples);
    shader->SetUniform2f("uViewport", igm::vec2{
            static_cast<float>(viewport.x), static_cast<float>(viewport.y)});

    m_VolumeTexture->Active(GL_TEXTURE1);
    shader->SetUniformi("volumeTexture", 1);
    m_LUTTexture->Active(GL_TEXTURE2);
    shader->SetUniformi("transferFunction", 2);

    m_EmptyVAO->DrawArrays(GL_TRIANGLES, 0, 3);
}

IGAME_NAMESPACE_END
