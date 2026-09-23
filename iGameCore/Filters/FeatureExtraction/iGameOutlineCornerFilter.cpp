#include "iGameOutlineCornerFilter.h"

#include <cmath>
#include <exception>

IGAME_NAMESPACE_BEGIN

void OutlineCornerFilter::SetCornerFactor(float factor) {
    if (!std::isfinite(factor)) {
        igDebug("[OutlineCornerFilter] Non-finite corner factor; using default 0.2.");
        factor = 0.2f;
    }
    if (factor < 0.001f) factor = 0.001f;
    if (factor > 0.5f) factor = 0.5f;
    m_CornerFactor = factor;
}

bool OutlineCornerFilter::Execute() {
    try {
        return ExecuteInternal();
    } catch (const std::exception& exception) {
        m_Result = nullptr;
        m_Message = std::string("Exception while building outline corners: ") + exception.what();
        igError("[OutlineCornerFilter] {}", m_Message);
    } catch (...) {
        m_Result = nullptr;
        m_Message = "Unknown exception while building outline corners.";
        igError("[OutlineCornerFilter] {}", m_Message);
    }
    return false;
}

bool OutlineCornerFilter::ExecuteInternal() {
    m_Result = nullptr;

    auto input = GetInput(0);
    if (input.IsNull()) {
        m_Message = "No input data object.";
        igError("[OutlineCornerFilter] No input data object; Execute aborted.");
        return false;
    }

    const auto& bounds = input->GetBoundingBox();
    if (bounds.isNull()) {
        m_Message = "Input data has no valid geometry bounding box.";
        igDebug("[OutlineCornerFilter] {}", m_Message);
        return false;
    }
    for (int axis = 0; axis < 3; ++axis) {
        if (!std::isfinite(bounds.min[axis]) || !std::isfinite(bounds.max[axis])) {
            m_Message = "Input bounding box contains NaN or infinity.";
            igError("[OutlineCornerFilter] {}", m_Message);
            return false;
        }
    }

    m_Result = BuildResult(bounds);
    if (m_Result.IsNull()) {
        m_Message = "Failed to build the outline-corner result.";
        igError("[OutlineCornerFilter] {}", m_Message);
        return false;
    }

    SetOutput(m_Result);
    m_Message = "Outline corner markers: 8, line segments: 24";
    UpdateProgress(1.0);
    return true;
}

UnstructuredMesh::Pointer OutlineCornerFilter::BuildResult(const BoundingBox& bounds) const {
    auto result = UnstructuredMesh::New();
    result->SetName("OutlineCorners");

    const float minX = static_cast<float>(bounds.min[0]);
    const float minY = static_cast<float>(bounds.min[1]);
    const float minZ = static_cast<float>(bounds.min[2]);
    const float maxX = static_cast<float>(bounds.max[0]);
    const float maxY = static_cast<float>(bounds.max[1]);
    const float maxZ = static_cast<float>(bounds.max[2]);
    const float coordinates[3][2] = {
        {minX, maxX},
        {minY, maxY},
        {minZ, maxZ},
    };
    const float armLengths[3] = {
        (maxX - minX) * m_CornerFactor,
        (maxY - minY) * m_CornerFactor,
        (maxZ - minZ) * m_CornerFactor,
    };

    for (int zSide = 0; zSide < 2; ++zSide) {
        for (int ySide = 0; ySide < 2; ++ySide) {
            for (int xSide = 0; xSide < 2; ++xSide) {
                const int sides[3] = {xSide, ySide, zSide};
                Point corner{
                    coordinates[0][xSide],
                    coordinates[1][ySide],
                    coordinates[2][zSide],
                };
                const igIndex cornerId = result->AddPoint(corner);

                for (int axis = 0; axis < 3; ++axis) {
                    Point armEnd = corner;
                    const float direction = sides[axis] == 0 ? 1.0f : -1.0f;
                    armEnd[axis] += direction * armLengths[axis];

                    const igIndex armEndId = result->AddPoint(armEnd);
                    igIndex line[2] = {cornerId, armEndId};
                    result->AddCell(line, 2, IG_LINE);
                }
            }
        }
    }
    result->SetViewStyle(IG_WIREFRAME);
    return result;
}

IGAME_NAMESPACE_END
