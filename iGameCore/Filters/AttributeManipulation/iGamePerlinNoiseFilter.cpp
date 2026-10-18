/**
 * @class   iGamePerlinNoise
 * @brief   在每个输入点上采样 3D Perlin 噪声，输出深拷贝网格 + 点标量数组。
 */

#include "iGamePerlinNoiseFilter.h"

#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGameFlatArray.h"
#include "iGamePointSet.h"
#include "iGamePoints.h"
#include "iGameStructuredMesh.h"
#include "iGameSurfaceMesh.h"
#include "iGameType.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"

#include <cmath>

namespace {
// ---------------------------------------------------------------------------
// Perlin 噪声内核（Greg Ward, Graphics Gems II 的实现）
// 说明：伪随机序列依赖 32 位 int 溢出，这里保持同样的运算方式以获得可复现的序列。
// ---------------------------------------------------------------------------
constexpr int kIntMax = 0x7FFFFFFF; // 32 位 int 最大值

double hermite(double p0, double p1, double r0, double r1, double t) {
    double tt = t * t;
    return (p0 * ((2.0 * t - 3.0) * tt + 1.0) +
            p1 * (-2.0 * t + 3.0) * tt +
            r0 * ((t - 2.0) * t + 1.0) * t +
            r1 * (t - 1.0) * tt);
}

double frand(int s) {
    s = (s << 13) ^ s;
    s = (s * (s * s * 15731 + 789221) + 1376312589) & kIntMax;
    return 1.0 - static_cast<double>(s) / (kIntMax / 2 + 1);
}

void rand3abcd(int x, int y, int z, double outv[4]) {
    outv[0] = frand(67 * x + 59 * y + 71 * z);
    outv[1] = frand(73 * x + 79 * y + 83 * z);
    outv[2] = frand(89 * x + 97 * y + 101 * z);
    outv[3] = frand(103 * x + 107 * y + 109 * z);
}

void interpolate(double f[4], int i, int n, int xlim[3][2], double xarg[2]) {
    double f0[4], f1[4];

    if (n == 0) {
        rand3abcd(xlim[0][i & 1], xlim[1][(i >> 1) & 1], xlim[2][i >> 2], f);
        return;
    }
    n--;
    interpolate(f0, i, n, xlim, xarg);
    interpolate(f1, i | (1 << n), n, xlim, xarg);

    f[0] = (1.0 - xarg[n]) * f0[0] + xarg[n] * f1[0];
    f[1] = (1.0 - xarg[n]) * f0[1] + xarg[n] * f1[1];
    f[2] = (1.0 - xarg[n]) * f0[2] + xarg[n] * f1[2];
    f[3] = hermite(f0[3], f1[3], f0[n], f1[n], xarg[n]);
}

void perlinNoise(double x[3], double noise[4]) {
    double xarg[3];
    int xlim[3][2];

    xlim[0][0] = static_cast<int>(std::floor(x[0]));
    xlim[1][0] = static_cast<int>(std::floor(x[1]));
    xlim[2][0] = static_cast<int>(std::floor(x[2]));

    xlim[0][1] = xlim[0][0] + 1;
    xlim[1][1] = xlim[1][0] + 1;
    xlim[2][1] = xlim[2][0] + 1;

    xarg[0] = x[0] - xlim[0][0];
    xarg[1] = x[1] - xlim[1][0];
    xarg[2] = x[2] - xlim[2][0];

    interpolate(noise, 0, 3, xlim, xarg);
}

// 深拷贝输入网格，噪声只加到新网格上，原模型保持不变
iGame::DataObject::Pointer CloneMesh(iGame::DataObject::Pointer input) {
    using namespace iGame;
    auto copyPoints = [](PointSet* src) -> Points::Pointer {
        auto pts = Points::New();
        pts->DeepCopy(src->GetPoints());
        return pts;
    };
    auto copyAttrs = [](DataObject* src) -> AttributeSet::Pointer {
        auto attrs = AttributeSet::New();
        attrs->DeepCopy(src->GetAttributeSet());
        return attrs;
    };

    switch (input->GetDataObjectType()) {
        case IG_SURFACE_MESH: {
            auto src = DynamicCast<SurfaceMesh>(input);
            auto dst = SurfaceMesh::New();
            dst->SetPoints(copyPoints(src));
            if (src->GetFaces()) {
                auto faces = CellArray::New();
                faces->DeepCopy(src->GetFaces());
                dst->SetFaces(faces);
            }
            dst->SetAttributeSet(copyAttrs(src));
            dst->SetName(src->GetName());
            return dst;
        }
        case IG_UNSTRUCTURED_MESH: {
            auto src = DynamicCast<UnstructuredMesh>(input);
            auto dst = UnstructuredMesh::New();
            dst->SetPoints(copyPoints(src));
            auto cells = CellArray::New();
            cells->DeepCopy(src->GetCells());
            auto types = UnsignedIntArray::New();
            types->DeepCopy(src->GetCellTypes());
            dst->SetCells(cells, types);
            dst->SetAttributeSet(copyAttrs(src));
            dst->SetName(src->GetName());
            return dst;
        }
        case IG_VOLUME_MESH: {
            auto src = DynamicCast<VolumeMesh>(input);
            auto dst = VolumeMesh::New();
            dst->SetPoints(copyPoints(src));
            auto vols = CellArray::New();
            vols->DeepCopy(src->GetVolumes());
            dst->SetVolumes(vols);
            dst->SetAttributeSet(copyAttrs(src));
            dst->SetName(src->GetName());
            return dst;
        }
        case IG_STRUCTURED_MESH: {
            auto src = DynamicCast<StructuredMesh>(input);
            auto dst = StructuredMesh::New();
            dst->SetPoints(copyPoints(src));
            dst->SetDimensionSize(src->GetDimensionSize());
            dst->SetAttributeSet(copyAttrs(src));
            dst->SetName(src->GetName());
            return dst;
        }
        case IG_POINT_SET: {
            auto src = DynamicCast<PointSet>(input);
            auto dst = PointSet::New();
            dst->SetPoints(copyPoints(src));
            dst->SetAttributeSet(copyAttrs(src));
            dst->SetName(src->GetName());
            return dst;
        }
        default:
            return nullptr;
    }
}
} // namespace

IGAME_NAMESPACE_BEGIN

double EvaluatePerlinNoise(const double x[3], const double frequency[3],
                           const double phase[3], double amplitude) {
    double xd[3];
    double noise[4];

    xd[0] = x[0] * frequency[0] - phase[0] * 2.0;
    xd[1] = x[1] * frequency[1] - phase[1] * 2.0;
    xd[2] = x[2] * frequency[2] - phase[2] * 2.0;

    perlinNoise(xd, noise);
    return noise[3] * amplitude;
}

bool PerlinNoiseFilter::Execute() {
    auto input = GetInput(0);
    if (input == nullptr) return false;

    auto srcPointSet = DynamicCast<PointSet>(input);
    if (srcPointSet == nullptr) return false;
    const IGsize numPoints = srcPointSet->GetNumberOfPoints();
    if (numPoints <= 0) return false;

    // 深拷贝输入网格：噪声作为新网格的点属性，输入模型保持不变
    auto newMesh = CloneMesh(input);
    if (newMesh == nullptr) return false;

    auto pointSet = DynamicCast<PointSet>(newMesh);
    if (pointSet == nullptr) return false;

    auto scalars = DoubleArray::New();
    scalars->SetName(m_ScalarArrayName);
    scalars->SetDimension(1);

    for (IGsize i = 0; i < numPoints; ++i) {
        const Point& p = pointSet->GetPoint(i);
        const double xyz[3] = {static_cast<double>(p[0]),
                               static_cast<double>(p[1]),
                               static_cast<double>(p[2])};
        scalars->AddValue(EvaluatePerlinNoise(xyz, m_Frequency, m_Phase, m_Amplitude));
    }

    auto attrSet = pointSet->GetAttributeSet();
    attrSet->AddAttribute(IG_SCALAR, IG_POINT, scalars);
    attrSet->ForceReConvertToDrawableData();

    newMesh->SetName(input->GetName() + "_PerlinNoise");
    SetOutput(newMesh);
    return true;
}

void PerlinNoiseFilter::SetAmplitude(double amplitude) { m_Amplitude = amplitude; }

double PerlinNoiseFilter::GetAmplitude() const { return m_Amplitude; }

void PerlinNoiseFilter::SetFrequency(double x, double y, double z) {
    m_Frequency[0] = x;
    m_Frequency[1] = y;
    m_Frequency[2] = z;
}

void PerlinNoiseFilter::SetFrequency(const double frequency[3]) {
    if (frequency == nullptr) return;
    SetFrequency(frequency[0], frequency[1], frequency[2]);
}

void PerlinNoiseFilter::GetFrequency(double frequency[3]) const {
    if (frequency == nullptr) return;
    frequency[0] = m_Frequency[0];
    frequency[1] = m_Frequency[1];
    frequency[2] = m_Frequency[2];
}

void PerlinNoiseFilter::SetPhase(double x, double y, double z) {
    m_Phase[0] = x;
    m_Phase[1] = y;
    m_Phase[2] = z;
}

void PerlinNoiseFilter::SetPhase(const double phase[3]) {
    if (phase == nullptr) return;
    SetPhase(phase[0], phase[1], phase[2]);
}

void PerlinNoiseFilter::GetPhase(double phase[3]) const {
    if (phase == nullptr) return;
    phase[0] = m_Phase[0];
    phase[1] = m_Phase[1];
    phase[2] = m_Phase[2];
}

void PerlinNoiseFilter::SetScalarArrayName(const std::string& name) {
    if (!name.empty()) { m_ScalarArrayName = name; }
}

const std::string& PerlinNoiseFilter::GetScalarArrayName() const { return m_ScalarArrayName; }

PerlinNoiseFilter::PerlinNoiseFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

IGAME_NAMESPACE_END
