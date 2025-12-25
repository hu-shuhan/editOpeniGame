//
// Created by Sumzeek on 25/02/2024.
//

#ifndef IGAMEVIEW_LITE_MESH_H
#define IGAMEVIEW_LITE_MESH_H

#include <vector>
#include <string>
#include "utils/vec.h"
#include "utils/types.h"
#include "Cluster.h"

struct Mesh{
    std::vector<vec3> positions;
    std::vector<uint32> indices;

    bool ReadObjFile(const std::string& file_name);
};

struct VirtualMesh{
    std::vector<Cluster> clusters;
    std::vector<ClusterGroup> cluster_groups;
    uint32 num_mip_level;

    void build(Mesh& mesh);
};

#endif //IGAMEVIEW_LITE_MESH_H
