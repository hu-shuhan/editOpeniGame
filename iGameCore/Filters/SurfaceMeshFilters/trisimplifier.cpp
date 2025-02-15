#include "simplifier.h"

namespace tri{




size_t simplifyWithAttributes(unsigned int* destination, const unsigned int* indices, size_t index_count,
                              const float* vertex_positions_data, size_t vertex_count, size_t vertex_positions_stride,
                              const float* vertex_attributes_data, size_t vertex_attributes_stride,
                              const float* attribute_weights, size_t attribute_count, const unsigned char* vertex_lock,
                              size_t target_index_count, float target_error, unsigned int options,
                              float* out_result_error) 
{
    Allocator allocator;
    
    // 简化的结果索引数组
    unsigned int* result = destination;
    if (result != indices) 
        memcpy(result, indices, index_count * sizeof(unsigned int));

    // 建立边表和顶点的邻接边
    EdgeAdjacency adjacency = {};
    prepareEdgeAdjacency(adjacency, index_count, vertex_count, allocator);
    updateEdgeAdjacency(adjacency, result, index_count, vertex_count, NULL);

    // 顶点坐标映射
    unsigned int* remap = allocator.allocate<unsigned int>(vertex_count);
    unsigned int* wedge = allocator.allocate<unsigned int>(vertex_count);
    buildPositionRemap(remap, wedge, vertex_positions_data, vertex_count, vertex_positions_stride, nullptr, allocator);

    // 模型归一化
    Vector3* vertex_positions = allocator.allocate<Vector3>(vertex_count);
    float vertex_scale = rescalePositions(vertex_positions, vertex_positions_data, vertex_count,
                                          vertex_positions_stride, nullptr);

    // 根据权重重新计算属性值
    float* vertex_attributes = NULL;
    if (attribute_count) {
        unsigned int attribute_remap[32];

        // remap attributes to only include ones with weight > 0 to minimize memory/compute overhead for quadrics
        size_t attributes_used = 0;
        for (size_t i = 0; i < attribute_count; ++i)
            if (attribute_weights[i] > 0) attribute_remap[attributes_used++] = unsigned(i);

        attribute_count = attributes_used;
        vertex_attributes = allocator.allocate<float>(vertex_count * attribute_count);
        rescaleAttributes(vertex_attributes, vertex_attributes_data, vertex_count, vertex_attributes_stride,
                          attribute_weights, attribute_count, attribute_remap, nullptr);
    }

    Quadric* vertex_quadrics = allocator.allocate<Quadric>(vertex_count);
    memset(vertex_quadrics, 0, vertex_count * sizeof(Quadric));

    Quadric* attribute_quadrics = NULL;
    QuadricGrad* attribute_gradients = NULL;

    if (attribute_count) {
        attribute_quadrics = allocator.allocate<Quadric>(vertex_count);
        memset(attribute_quadrics, 0, vertex_count * sizeof(Quadric));

        attribute_gradients = allocator.allocate<QuadricGrad>(vertex_count * attribute_count);
        memset(attribute_gradients, 0, vertex_count * attribute_count * sizeof(QuadricGrad));
    }

    fillFaceQuadrics(vertex_quadrics, result, index_count, vertex_positions, remap);

    if (attribute_count)
        fillAttributeQuadrics(attribute_quadrics, attribute_gradients, result, index_count, vertex_positions,
                              vertex_attributes, attribute_count);

    size_t collapse_capacity = boundEdgeCollapses(adjacency, vertex_count, index_count, nullptr);

    Collapse* edge_collapses = allocator.allocate<Collapse>(collapse_capacity);
    unsigned int* collapse_order = allocator.allocate<unsigned int>(collapse_capacity);
    unsigned int* collapse_remap = allocator.allocate<unsigned int>(vertex_count);
    unsigned char* collapse_locked = allocator.allocate<unsigned char>(vertex_count);

    size_t result_count = index_count;
    float result_error = 0;
    float vertex_error = 0;

    // target_error input is linear; we need to adjust it to match quadricError units
    float error_scale = 1.f;
    float error_limit = (target_error * target_error) / (error_scale * error_scale);

    while (result_count > target_index_count)
    {
        updateEdgeAdjacency(adjacency, result, result_count, vertex_count, remap);

        size_t edge_collapse_count = pickEdgeCollapses(edge_collapses, collapse_capacity, result, result_count, remap,
                                                       nullptr, nullptr, nullptr);

        if (edge_collapse_count == 0) break;

        rankEdgeCollapses(edge_collapses, edge_collapse_count, vertex_positions, vertex_attributes, vertex_quadrics,
                          attribute_quadrics, attribute_gradients, attribute_count, remap);

        sortEdgeCollapses(collapse_order, edge_collapses, edge_collapse_count);

        size_t triangle_collapse_goal = (result_count - target_index_count) / 3;

        for (size_t i = 0; i < vertex_count; ++i) collapse_remap[i] = unsigned(i);

        memset(collapse_locked, 0, vertex_count);

        size_t collapses =
                performEdgeCollapses(collapse_remap, collapse_locked, edge_collapses, edge_collapse_count,
                                     collapse_order, remap, wedge, nullptr, nullptr, nullptr, vertex_positions,
                                     adjacency, triangle_collapse_goal, error_limit, result_error);

        // no edges can be collapsed any more due to hitting the error limit or triangle collapse limit
        if (collapses == 0) break;

        updateQuadrics(collapse_remap, vertex_count, vertex_quadrics, attribute_quadrics, attribute_gradients,
                       attribute_count, vertex_positions, remap, vertex_error);

        vertex_error = attribute_count == 0 ? result_error : vertex_error;

        size_t new_count = remapIndexBuffer(result, result_count, collapse_remap);

        result_count = new_count;

    }

	// result_error is quadratic; we need to remap it back to linear
    if (out_result_error) *out_result_error = sqrtf(result_error) * error_scale;

    return result_count;
}
}