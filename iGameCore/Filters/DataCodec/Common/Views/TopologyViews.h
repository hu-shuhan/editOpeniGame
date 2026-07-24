#ifndef DATACODEC_COMMON_VIEWS_TOPOLOGYVIEWS_H
#define DATACODEC_COMMON_VIEWS_TOPOLOGYVIEWS_H

#include "DataCodec/Common/Views/ArrayViews.h"
#include "DataCodec/Common/DataCodecTypes.h"

#include <cstddef>
namespace datacodec {

struct TopologyView {
    std::size_t cellCount{0};
    std::size_t pointCount{0};
    IndexArrayView connectivity;
    IndexArrayView offsets;
    bool fixedCellSizeEnabled{false};
    int fixedCellSize{0};
    NumericArrayView cellTypes;
    NumericArrayView cellPolynomialOrders;
};

struct PolyhedronFaceTableView {
    std::size_t cellCount{0};
    std::size_t faceCount{0};
    std::size_t pointCount{0};
    IndexArrayView faceVertexIds;
    IndexArrayView faceVertexOffsets;
    IndexArrayView cellFaceIds;
    IndexArrayView cellFaceOffsets;
};

// PolyhedronTopologyView
//
//   cellVertexOffsets   = [0, 4, 9]
//   cellUniqueVertexIds = [10, 21, 35, 40,       7, 8, 9, 15, 20]
//                         [------ cell0 ------) [--------- cell1 ---------)
//
//   cellFaceOffsets = [0, 4, 9]
//
//   faceVertexOffsets   = [0, 3, 6, 9, 12, 15, 18, 21, 24, 28]
//
//   localFaceVertexIds  = [
//                                       cellFaceOffsets
//       0, 1, 2,      // face0, cell0          0
//       0, 1, 3,      // face1, cell0
//       1, 2, 3,      // face2, cell0
//       0, 2, 3,      // face3, cell0          4
//
//       0, 1, 4,      // face4, cell1
//       1, 2, 4,      // face5, cell1
//       2, 3, 4,      // face6, cell1
//       3, 0, 4,      // face7, cell1
//       0, 1, 2, 3    // face8, cell1          9
//   ]
//
struct PolyhedronTopologyView {
    // 每个 cell 在 unique-vertex 偏移数组里的起止范围
    const IndexType* cellVertexOffsets{nullptr};
    // `cellVertexOffsets` 的元素个数，正常情况下等于 cell 数 + 1
    std::size_t cellVertexOffsetCount{0};

    // 按 cell 顺序拼接后的 unique-vertex 全局点 id 列表
    const IndexType* cellUniqueVertexIds{nullptr};
    // `cellUniqueVertexIds` 的元素个数
    std::size_t cellUniqueVertexIdCount{0};

    // 每个 cell 在面偏移数组里的起止范围
    const IndexType* cellFaceOffsets{nullptr};
    // `cellFaceOffsets` 的元素个数，正常情况下等于 cell 数 + 1
    std::size_t cellFaceOffsetCount{0};

    // 每个面在局部顶点偏移数组里的起止范围
    const IndexType* faceVertexOffsets{nullptr};
    // `faceVertexOffsets` 的元素个数，正常情况下等于面数 + 1
    std::size_t faceVertexOffsetCount{0};

    // 面局部顶点 id，索引到所属 cell 的 `cellUniqueVertexIds` 子区间
    const IndexType* localFaceVertexIds{nullptr};
    // `localFaceVertexIds` 的元素个数
    std::size_t localFaceVertexIdCount{0};
};

} // namespace datacodec

#endif
