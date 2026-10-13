#include "iGameOutlineFilter.h"
#include "iGameUnstructuredMesh.h"

#include <cmath>

IGAME_NAMESPACE_BEGIN

OutlineFilter::OutlineFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

bool OutlineFilter::Execute() {
    SetOutput(nullptr);
    m_Message.clear();

    auto input = GetInput(0);
    if (!input) {
        m_Message = "No input data object.";
        return false;
    }

    const auto& bounds = input->GetBoundingBox();
    if (bounds.isNull()) {
        m_Message = "Input has no valid bounding box.";
        return false;
    }
    for (int axis = 0; axis < 3; ++axis) {
        if (!std::isfinite(bounds.min[axis]) || !std::isfinite(bounds.max[axis])) {
            m_Message = "Input bounding box contains NaN or infinity.";
            return false;
        }
    }

    auto output = UnstructuredMesh::New();
    output->SetName("Outline");

    // 由各轴的最小值和最大值生成八个角点
    for (int z = 0; z < 2; ++z) {
        for (int y = 0; y < 2; ++y) {
            for (int x = 0; x < 2; ++x) {
                output->AddPoint(Point(x ? bounds.max[0] : bounds.min[0],
                                       y ? bounds.max[1] : bounds.min[1],
                                       z ? bounds.max[2] : bounds.min[2]));
            }
        }
    }

    // 连接底面、顶面和四条竖边
    igIndex edges[12][2] = {
        {0, 1}, {1, 3}, {3, 2}, {2, 0},
        {4, 5}, {5, 7}, {7, 6}, {6, 4},
        {0, 4}, {1, 5}, {2, 6}, {3, 7}
    };
    for (auto& edge : edges) {
        output->AddCell(edge, 2, IG_LINE);
    }

    output->SetViewStyle(IG_WIREFRAME);
    SetOutput(output);
    m_Message = "Outline generated: 8 points, 12 line cells.";
    UpdateProgress(1.0);
    return true;
}

IGAME_NAMESPACE_END
