/**
 * @class   iGameRandomVectors
 * @brief   Mimic ParaView's "Random Vectors" filter (vtkBrownianPoints).
 */

#include "iGameRandomVectorsFilter.h"

#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGameDrawObject.h"
#include "iGameFlatArray.h"
#include "iGamePointSet.h"
#include "iGamePoints.h"
#include "iGameStructuredMesh.h"
#include "iGameSurfaceMesh.h"
#include "iGameType.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"

#include <cmath>
#include <random>

namespace {
// 全局随机数流（非确定性）
std::mt19937& GlobalRandomGen() {
    static std::mt19937 gen(std::random_device{}());
    return gen;
}

// 深拷贝输入网格，生成一个独立的新网格，避免修改原模型
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

bool RandomVectorsFilter::Execute() {
    auto input = GetInput(0);
    if (input == nullptr) return false;

    auto srcPointSet = DynamicCast<PointSet>(input);
    if (srcPointSet == nullptr) return false;
    if (srcPointSet->GetNumberOfPoints() <= 0) return false;

    // 深拷贝输入网格，随机向量只加到新网格上，原模型保持不变
    auto newMesh = CloneMesh(input);
    if (newMesh == nullptr) return false;

    auto pointSet = DynamicCast<PointSet>(newMesh);
    const IGsize numPoints = pointSet->GetNumberOfPoints();

    auto vectors = FloatArray::New();
    vectors->SetName("BrownianVectors");
    vectors->SetDimension(3);
    vectors->Reserve(numPoints);

    auto& gen = GlobalRandomGen();
    std::uniform_real_distribution<double> dirDist(-1.0, 1.0);
    std::uniform_real_distribution<double> speedDist(m_MinimumSpeed, m_MaximumSpeed);

    for (IGsize i = 0; i < numPoints; ++i) {
        double x = dirDist(gen);
        double y = dirDist(gen);
        double z = dirDist(gen);
        const double length = std::sqrt(x * x + y * y + z * z);
        if (length > 0.0) {
            x /= length;
            y /= length;
            z /= length;
        }
        const double speed = speedDist(gen);
        vectors->AddElement3(x * speed, y * speed, z * speed);
    }

    auto attrSet = pointSet->GetAttributeSet();
    attrSet->AddVector(IG_POINT, vectors);
    attrSet->ForceReConvertToDrawableData();

    newMesh->SetName(input->GetName() + "_RandomVectors");
    SetOutput(newMesh);
    return true;
}

void RandomVectorsFilter::SetMinimumSpeed(double speed) { m_MinimumSpeed = speed; }

void RandomVectorsFilter::SetMaximumSpeed(double speed) { m_MaximumSpeed = speed; }

double RandomVectorsFilter::GetMinimumSpeed() const { return m_MinimumSpeed; }

double RandomVectorsFilter::GetMaximumSpeed() const { return m_MaximumSpeed; }

RandomVectorsFilter::RandomVectorsFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

IGAME_NAMESPACE_END
