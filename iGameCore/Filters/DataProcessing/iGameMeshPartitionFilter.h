#ifndef iGameMeshPartitionFilter_h
#define iGameMeshPartitionFilter_h

#include "iGameFilter.h"
#include <vector>

IGAME_NAMESPACE_BEGIN
class MeshPartitionFilter : public Filter {
public:
    I_OBJECT(MeshPartitionFilter);
    static Pointer New() { return new MeshPartitionFilter; }

    struct Cluster {
        static const int cluster_size = 128;

        std::vector<Vector3f> verts;
        std::vector<int> indexes; // Local indices
        std::vector<int> faceIndices; // Global face indices (keeping this for compatibility)
        std::vector<int> externalEdgeIds; // Indices into local 'indexes' array (tri_idx * 3 + k)
        int group_id = -1;

        // Simplified bounds for now
        Vector3f box_min;
        Vector3f box_max;
        float lod_error = 0.0f;
        int mip_level = 0;
    };

    struct ClusterGroup {
        static const int group_size = 32;

        std::vector<int> clusters; // Indices into m_Clusters
        std::vector<std::pair<int, int>> external_edges; // {cluster_index, edge_index_in_cluster}
            
        int mip_level = 0;
        float max_parent_lod_error = 0.0f;
    };

    bool Execute() override;

    std::vector<Cluster>& GetClusters() { return m_Clusters; }
    std::vector<ClusterGroup>& GetClusterGroups() { return m_ClusterGroups; }

protected:
    MeshPartitionFilter();
    ~MeshPartitionFilter() override = default;

    int m_NumberOfPartitions{2};
    std::vector<Cluster> m_Clusters;
    std::vector<ClusterGroup> m_ClusterGroups;
};
IGAME_NAMESPACE_END
#endif