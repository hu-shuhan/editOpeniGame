//
// Created by Sumzeek on 05/02/2024.
//

#ifndef IGAMEVIEW_LITE_CLUSTER_H
#define IGAMEVIEW_LITE_CLUSTER_H

#include "Partitioner.h"
#include "Bounds.h"
#include  <iostream>

struct Cluster{
    static const uint32 cluster_size = 128;

    std::vector<vec3> verts;
    std::vector<uint32> indexes;
    std::vector<uint32> external_edges;

    Bounds box_bounds;
    Sphere sphere_bounds;
    Sphere lod_bounds;
    float lod_error;
    uint32 mip_level;
    uint32 group_id;
};

struct ClusterGroup{
    // static const uint32 min_group_size=8;
    static const uint32 group_size = 32;

    Sphere bounds;
    Sphere lod_bounds;
    float min_lod_error;
    float max_parent_lod_error;
    uint32 mip_level;
    std::vector<uint32> clusters; //对cluster数组的下标
    std::vector<std::pair<uint32,uint32>> external_edges; //first: cluster id, second: edge id
};

void cluster_triangles(
        const std::vector<vec3>& verts,
        const std::vector<uint32>& indexes,
        std::vector<Cluster>& clusters
);

void group_clusters(
        std::vector<Cluster>& clusters,
        uint32 offset,
        uint32 num_cluster,
        std::vector<ClusterGroup>& cluster_groups,
        uint32 mip_level
);

void build_parent_clusters(
        ClusterGroup& cluster_group,
        std::vector<Cluster>& clusters
);

#endif //IGAMEVIEW_LITE_CLUSTER_H
