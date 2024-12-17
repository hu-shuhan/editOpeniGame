#include "simplifier.h"

namespace tri{




size_t simplifyWithAttributes(unsigned int* destination, const unsigned int* indices, size_t index_count,
                              const float* vertex_positions_data, size_t vertex_count, size_t vertex_positions_stride,
                              const float* vertex_attributes_data, size_t vertex_attributes_stride,
                              const float* attribute_weights, size_t attribute_count, const unsigned char* vertex_lock,
                              size_t target_index_count, float target_error, unsigned int options,
                              float* result_error) 
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

    return 0;
}
}