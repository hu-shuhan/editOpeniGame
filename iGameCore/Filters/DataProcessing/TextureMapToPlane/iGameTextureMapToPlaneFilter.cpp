#include "iGameTextureMapToPlaneFilter.h"

#include <iGameAttributeSet.h>
#include <iGameCellArray.h>
#include <iGameStructuredMesh.h>
#include <iGameSurfaceMesh.h>
#include <iGameUnstructuredMesh.h>
#include <iGameVolumeMesh.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <utility>

IGAME_NAMESPACE_BEGIN

namespace {

DataObject::Pointer ClonePointSet(const DataObject::Pointer& input) {
    auto copyPoints = [](PointSet* source) {
        auto points = Points::New();
        points->DeepCopy(source->GetPoints());
        return points;
    };
    auto copyAttributes = [](DataObject* source) {
        auto attributes = AttributeSet::New();
        attributes->DeepCopy(source->GetAttributeSet());
        return attributes;
    };

    switch (input->GetDataObjectType()) {
        case IG_SURFACE_MESH: {
            auto source = DynamicCast<SurfaceMesh>(input);
            auto output = SurfaceMesh::New();
            output->SetPoints(copyPoints(source));
            if (source->GetFaces()) {
                auto faces = CellArray::New();
                faces->DeepCopy(source->GetFaces());
                output->SetFaces(faces);
            }
            output->SetAttributeSet(copyAttributes(source));
            output->SetName(source->GetName());
            return output;
        }
        case IG_UNSTRUCTURED_MESH: {
            auto source = DynamicCast<UnstructuredMesh>(input);
            auto output = UnstructuredMesh::New();
            output->SetPoints(copyPoints(source));
            auto cells = CellArray::New();
            cells->DeepCopy(source->GetCells());
            auto types = UnsignedIntArray::New();
            types->DeepCopy(source->GetCellTypes());
            output->SetCells(cells, types);
            output->SetAttributeSet(copyAttributes(source));
            output->SetName(source->GetName());
            return output;
        }
        case IG_VOLUME_MESH: {
            auto source = DynamicCast<VolumeMesh>(input);
            auto output = VolumeMesh::New();
            output->SetPoints(copyPoints(source));
            auto volumes = CellArray::New();
            volumes->DeepCopy(source->GetVolumes());
            output->SetVolumes(volumes);
            output->SetAttributeSet(copyAttributes(source));
            output->SetName(source->GetName());
            return output;
        }
        case IG_STRUCTURED_MESH: {
            auto source = DynamicCast<StructuredMesh>(input);
            auto output = StructuredMesh::New();
            output->SetPoints(copyPoints(source));
            output->SetDimensionSize(source->GetDimensionSize());
            output->SetAttributeSet(copyAttributes(source));
            output->SetName(source->GetName());
            return output;
        }
        case IG_POINT_SET: {
            auto source = DynamicCast<PointSet>(input);
            auto output = PointSet::New();
            output->SetPoints(copyPoints(source));
            output->SetAttributeSet(copyAttributes(source));
            output->SetName(source->GetName());
            return output;
        }
        default:
            return nullptr;
    }
}

double Dot(const std::array<double, 3>& left, const std::array<double, 3>& right) {
    return left[0] * right[0] + left[1] * right[1] + left[2] * right[2];
}

std::array<double, 3> Cross(const std::array<double, 3>& left,
                            const std::array<double, 3>& right) {
    return {left[1] * right[2] - left[2] * right[1],
            left[2] * right[0] - left[0] * right[2],
            left[0] * right[1] - left[1] * right[0]};
}

bool Normalize(std::array<double, 3>& vector) {
    const double length = std::sqrt(Dot(vector, vector));
    if (length == 0.0) return false;
    for (double& component : vector) component /= length;
    return true;
}

double Determinant(const std::array<double, 3>& row0,
                   const std::array<double, 3>& row1,
                   const std::array<double, 3>& row2) {
    return row0[0] * (row1[1] * row2[2] - row1[2] * row2[1])
           - row0[1] * (row1[0] * row2[2] - row1[2] * row2[0])
           + row0[2] * (row1[0] * row2[1] - row1[1] * row2[0]);
}

} // namespace

TextureMapToPlaneFilter::TextureMapToPlaneFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

bool TextureMapToPlaneFilter::Execute() {
    m_LastError.clear();
    m_TextureCoordinates = nullptr;

    const auto fail = [this](std::string message) {
        m_LastError = std::move(message);
        return false;
    };

    auto input = GetInput(0);
    auto inputPointSet = DynamicCast<PointSet>(input);
    if (inputPointSet.IsNull()) {
        return fail("Texture Map to Plane 仅支持 PointSet 及其派生网格类型；当前输入不包含可映射的点数据。");
    }
    const IGsize pointCount = inputPointSet->GetNumberOfPoints();
    if (pointCount < 2) return fail("输入至少需要两个点才能生成平面纹理坐标。");

    std::vector<std::array<double, 2>> coordinates(pointCount);
    constexpr double relativeTolerance = 1.0e-12;

    if (m_AutomaticPlaneGeneration) {
        if (pointCount < 3) return fail("自动平面生成至少需要三个点。");

        std::array<double, 6> bounds{
                std::numeric_limits<double>::max(), std::numeric_limits<double>::lowest(),
                std::numeric_limits<double>::max(), std::numeric_limits<double>::lowest(),
                std::numeric_limits<double>::max(), std::numeric_limits<double>::lowest()};
        for (IGsize pointId = 0; pointId < pointCount; ++pointId) {
            const auto& point = inputPointSet->GetPoint(pointId);
            for (int axis = 0; axis < 3; ++axis) {
                bounds[2 * axis] = std::min(bounds[2 * axis], static_cast<double>(point[axis]));
                bounds[2 * axis + 1] = std::max(bounds[2 * axis + 1], static_cast<double>(point[axis]));
            }
        }

        std::array<double, 3> extents{};
        double dataLengthSquared = 0.0;
        int minimumWidthAxis = 0;
        double minimumWidth = std::numeric_limits<double>::max();
        for (int axis = 0; axis < 3; ++axis) {
            extents[axis] = bounds[2 * axis + 1] - bounds[2 * axis];
            dataLengthSquared += extents[axis] * extents[axis];
            if (extents[axis] < minimumWidth) {
                minimumWidth = extents[axis];
                minimumWidthAxis = axis;
            }
        }
        const double dataLength = std::sqrt(dataLengthSquared);
        if (dataLength == 0.0) return fail("自动平面生成失败：输入点全部重合。");

        std::array<double, 3> normal{};
        normal[minimumWidthAxis] = 1.0;
        if (minimumWidth > dataLength * 1.0e-3) {
            std::array<std::array<double, 3>, 3> matrix{};
            std::array<double, 3> right{};
            for (IGsize pointId = 0; pointId < pointCount; ++pointId) {
                const auto& point = inputPointSet->GetPoint(pointId);
                const double x = point[0];
                const double y = point[1];
                const double z = point[2];
                right[0] += x * z;
                right[1] += y * z;
                right[2] += z;
                matrix[0][0] += x * x;
                matrix[0][1] += x * y;
                matrix[0][2] += x;
                matrix[1][0] += x * y;
                matrix[1][1] += y * y;
                matrix[1][2] += y;
                matrix[2][0] += x;
                matrix[2][1] += y;
            }
            matrix[2][2] = static_cast<double>(pointCount);
            const double determinant = Determinant(matrix[0], matrix[1], matrix[2]);
            if (determinant > 1.0e-3) {
                normal[0] = Determinant(right, matrix[1], matrix[2]) / determinant;
                normal[1] = Determinant(matrix[0], right, matrix[2]) / determinant;
                normal[2] = -1.0;
            }
        }
        if (!Normalize(normal)) return fail("自动平面生成失败：无法确定平面法向量。");

        int referenceAxis = 0;
        double minimumProjection = 1.0;
        for (int axis = 0; axis < 3; ++axis) {
            const double projection = std::abs(normal[axis]);
            if (projection < minimumProjection) {
                minimumProjection = projection;
                referenceAxis = axis;
            }
        }
        std::array<double, 3> reference{};
        reference[referenceAxis] = 1.0;
        auto tAxis = Cross(normal, reference);
        if (!Normalize(tAxis)) return fail("自动平面生成失败：无法确定 T 轴。");
        const auto sAxis = Cross(tAxis, normal);
        const double sExtent = Dot(sAxis, extents);
        const double tExtent = Dot(tAxis, extents);
        const double extentTolerance = dataLength * relativeTolerance;
        if (std::abs(sExtent) <= extentTolerance || std::abs(tExtent) <= extentTolerance) {
            return fail("自动平面生成失败：输入点在映射平面上的二维范围退化。");
        }

        for (IGsize pointId = 0; pointId < pointCount; ++pointId) {
            const auto& point = inputPointSet->GetPoint(pointId);
            const std::array<double, 3> offset{
                    point[0] - bounds[0], point[1] - bounds[2], point[2] - bounds[4]};
            coordinates[pointId] = {Dot(sAxis, offset) / sExtent,
                                    Dot(tAxis, offset) / tExtent};
        }
    } else {
        const std::array<double, 3> origin{m_Origin[0], m_Origin[1], m_Origin[2]};
        const std::array<double, 3> sAxis{m_Point1[0] - m_Origin[0], m_Point1[1] - m_Origin[1],
                                          m_Point1[2] - m_Origin[2]};
        const std::array<double, 3> tAxis{m_Point2[0] - m_Origin[0], m_Point2[1] - m_Origin[1],
                                          m_Point2[2] - m_Origin[2]};
        const double sLengthSquared = Dot(sAxis, sAxis);
        const double tLengthSquared = Dot(tAxis, tAxis);
        if (sLengthSquared <= relativeTolerance || tLengthSquared <= relativeTolerance) {
            return fail("手动映射失败：Origin 到 Point1、Point2 的两条平面轴长度都必须大于零。");
        }
        if (Dot(Cross(sAxis, tAxis), Cross(sAxis, tAxis)) <=
            relativeTolerance * sLengthSquared * tLengthSquared) {
            return fail("手动映射失败：Origin、Point1、Point2 不能共线。");
        }
        for (IGsize pointId = 0; pointId < pointCount; ++pointId) {
            const auto& point = inputPointSet->GetPoint(pointId);
            const std::array<double, 3> offset{
                    point[0] - origin[0], point[1] - origin[1], point[2] - origin[2]};
            coordinates[pointId] = {Dot(offset, sAxis) / sLengthSquared,
                                    Dot(offset, tAxis) / tLengthSquared};
        }
    }

    auto output = ClonePointSet(input);
    auto outputPointSet = DynamicCast<PointSet>(output);
    if (outputPointSet.IsNull()) return fail("无法为当前网格类型创建独立输出。");
    output->SetName(input->GetName() + "_TextureMapToPlane");

    m_TextureCoordinates = FloatArray::New();
    m_TextureCoordinates->SetName(TextureCoordinatesArrayName());
    m_TextureCoordinates->SetDimension(2);
    m_TextureCoordinates->Reserve(pointCount);
    for (const auto& coordinate : coordinates) {
        const float tuple[2]{static_cast<float>(coordinate[0]), static_cast<float>(coordinate[1])};
        m_TextureCoordinates->AddElement(tuple);
    }

    auto* attributes = outputPointSet->GetAttributeSet();
    for (IGsize attributeId = 0; attributeId < attributes->GetNumberOfAttributes(); ++attributeId) {
        auto& attribute = attributes->GetAttribute(attributeId);
        if (attribute.IsDeleted() || attribute.GetAttachmentType() != IG_POINT) continue;
        if (attribute.GetType() == IG_TCOORD ||
            (attribute.GetPointer() && attribute.GetPointer()->GetName() == TextureCoordinatesArrayName())) {
            attributes->DeleteAttribute(attributeId);
        }
    }
    attributes->AddAttribute(IG_TCOORD, IG_POINT, m_TextureCoordinates);
    SetOutput(output);
    return true;
}

IGAME_NAMESPACE_END
