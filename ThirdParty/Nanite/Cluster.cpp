//
// Created by Sumzeek on 05/02/2024.
//

#include <unordered_map>
#include <span>
#include <assert.h>
#include "Cluster.h"
#include "utils/hash_table.h"
#include "Nanite/utils/cycle3.h"
#include "tracy/Tracy.hpp"

inline uint32 hash(vec3 v){
    union {float f;uint32 u;} x,y,z;
    x.f = (v.x == 0.f ? 0 : v.x);
    y.f = (v.y == 0.f ? 0 : v.y);
    z.f = (v.z == 0.f ? 0 : v.z);
    return murmur_mix(murmur_add(murmur_add(x.u, y.u), z.u));
}

inline uint32 hash(std::pair<vec3,vec3> e){
    uint32 h0 = ::hash(e.first);
    uint32 h1 = ::hash(e.second);
    return murmur_mix(murmur_add(h0, h1));
}

//边哈希，找到共享顶点且相反的边，代表两三角形相邻
void build_adjacency_edge_link(
        const std::vector<vec3>& verts,
        const std::vector<uint32>& indexes,
        Graph& edge_link
){
    HashTable edge_ht(indexes.size());
    edge_link.init(indexes.size());

    for(uint32 i = 0; i<indexes.size(); i++){
        vec3 p0 = verts[indexes[i]];
        vec3 p1 = verts[indexes[cycle3(i)]];
        edge_ht.add(::hash({p0, p1}), i);

        for(uint32 j : edge_ht[::hash({p1, p0})]){
            if(p1 == verts[indexes[j]] && p0 == verts[indexes[cycle3(j)]]){
                edge_link.increase_edge_cost(i, j, 1);
                edge_link.increase_edge_cost(j, i, 1);
            }
        }
    }
}
// 根据边的邻接构建三角形的邻接图，边权为1，当需要加入local时需要adjacency边权足够大
void build_adjacency_graph(
        const Graph& edge_link,
        Graph& graph
){
    graph.init(edge_link.g.size() / 3);
    uint32 u = 0;
    for(const auto& mp : edge_link.g){
        for(auto [v,w] : mp){
            graph.increase_edge_cost(u/3, v/3, 1);
        }
        u++;
    }
}

void cluster_triangles(
        const std::vector<vec3>& verts,
        const std::vector<uint32>& indexes,
        std::vector<Cluster>& clusters
){
    Graph edge_link,graph;
    {
      ZoneScopedNC("build_adjacency_edge_link", tracy::Color::Green);
      TracyMessage("build_adjacency_edge_link", strlen("build_adjacency_edge_link"));
      build_adjacency_edge_link(verts,indexes,edge_link);
    }
    {
      ZoneScopedNC("build_adjacency_graph", tracy::Color::Green);
      TracyMessage("build_adjacency_graph", strlen("build_adjacency_graph"));
      build_adjacency_graph(edge_link,graph);
    }

    GraphPartitioner partitioner;
    {
      ZoneScopedNC("partition", tracy::Color::Green);
      TracyMessage("partition", strlen("partition"));
      partitioner.partition(graph,Cluster::cluster_size - 4,Cluster::cluster_size);
    }

    // 根据划分结果构建clusters
    for(auto[l,r]:partitioner.Ranges){
        clusters.push_back({});
        Cluster& cluster=clusters.back();

        std::unordered_map<uint32,uint32> mp;
        for(uint32 i=l;i<r;i++){
            uint32 t_idx=partitioner.Indexes[i];
            for(uint32 k=0;k<3;k++){
                uint32 e_idx=t_idx*3+k;
                uint32 v_idx=indexes[e_idx];
                if(mp.find(v_idx)==mp.end()){ //重映射顶点下标
                    mp[v_idx]=cluster.verts.size();
                    cluster.verts.push_back(verts[v_idx]);
                }
                bool is_external=false;
                for(auto[adj_edge,_]:edge_link.g[e_idx]){
                    uint32 adj_tri=partitioner.SortedTo[adj_edge/3];
                    if(adj_tri<l||adj_tri>=r){ //出点在不同划分说明是边界
                        is_external=true;
                        break;
                    }
                }
                if(is_external){
                    cluster.external_edges.push_back(cluster.indexes.size());
                }
                cluster.indexes.push_back(mp[v_idx]);
            }
        }

        cluster.mip_level=0;
        cluster.lod_error=0;
        cluster.sphere_bounds=Sphere::from_points(cluster.verts.data(),cluster.verts.size());
        cluster.lod_bounds=cluster.sphere_bounds;
        cluster.box_bounds=cluster.verts[0];
        for(vec3 p:cluster.verts) cluster.box_bounds=cluster.box_bounds+p;
    }
}

void build_clusters_edge_link(
        std::span<const Cluster> clusters,
        const std::vector<std::pair<uint32,uint32>>& ext_edges,
        Graph& edge_link
){
    HashTable edge_ht(ext_edges.size());
    edge_link.init(ext_edges.size());

    uint32 i=0;
    for(auto[c_id,e_id]:ext_edges){
        auto& pos=clusters[c_id].verts;
        auto& idx=clusters[c_id].indexes;
        vec3 p0=pos[idx[e_id]];
        vec3 p1=pos[idx[cycle3(e_id)]];
        edge_ht.add(::hash({p0,p1}),i);
        for(uint32 j:edge_ht[::hash({p1,p0})]){
            auto [c_id1,e_id1]=ext_edges[j];
            auto& pos1=clusters[c_id1].verts;
            auto& idx1=clusters[c_id1].indexes;

            if(pos1[idx1[e_id1]]==p1&&pos1[idx1[cycle3(e_id1)]]==p0){
                edge_link.increase_edge_cost(i,j,1);
                edge_link.increase_edge_cost(j,i,1);
            }
        }
        i++;
    }
}

void build_clusters_graph(
        const Graph& edge_link,
        const std::vector<uint32>& mp,
        uint32 num_cluster,
        Graph& graph
){
    graph.init(num_cluster);
    uint32 u=0;
    for(const auto& emp:edge_link.g){
        for(auto [v,w]:emp){
            graph.increase_edge_cost(mp[u],mp[v],1);
        }
        u++;
    }
}

void group_clusters(
        std::vector<Cluster>& clusters,
        uint32 offset,
        uint32 num_cluster,
        std::vector<ClusterGroup>& cluster_groups,
        uint32 mip_level
){
    std::span<const Cluster> clusters_view(clusters.begin()+offset,num_cluster);

    //取出每个cluster的边界，并建立边id到簇id的映射
    std::vector<uint32> mp; //edge_id to cluster_id
    std::vector<uint32> mp1; //cluster_id to first_edge_id
    std::vector<std::pair<uint32,uint32>> ext_edges;
    uint32 i=0;
    for(auto& cluster:clusters_view){
        assert(cluster.mip_level==mip_level);
        mp1.push_back(mp.size());
        for(uint32 e:cluster.external_edges){
            ext_edges.push_back({i,e});
            mp.push_back(i);
        }
        i++;
    }
    Graph edge_link,graph;
    build_clusters_edge_link(clusters_view,ext_edges,edge_link);
    build_clusters_graph(edge_link,mp,num_cluster,graph);

    GraphPartitioner partitioner;
    partitioner.Partition(graph,ClusterGroup::group_size-4,ClusterGroup::group_size);

    //todo: 包围盒
    for(auto [l,r]:partitioner.Ranges){
        cluster_groups.push_back({});
        auto& group=cluster_groups.back();
        group.mip_level=mip_level;
        for(uint32 i=l;i<r;i++){
            uint32 c_id=partitioner.Indexes[i];
            clusters[c_id+offset].group_id=cluster_groups.size()-1;
            group.clusters.push_back(c_id+offset);
            for(uint32 e_idx=mp1[c_id];e_idx<mp.size()&&mp[e_idx]==c_id;e_idx++){
                bool is_external=false;
                for(auto [adj_e,_]:edge_link.g[e_idx]){
                    uint32 adj_cl=partitioner.SortedTo[mp[adj_e]];
                    if(adj_cl<l||adj_cl>=r){
                        is_external=true;
                        break;
                    }
                }
                if(is_external){
                    uint32 e=ext_edges[e_idx].second;
                    group.external_edges.push_back({c_id+offset,e});
                }
            }
        }
    }
}

//void build_parent_clusters(
//        ClusterGroup& cluster_group,
//        std::vector<Cluster>& clusters
//){
//    std::vector<vec3> pos;
//    std::vector<uint32> idx;
//    std::vector<Sphere> lod_bounds;
//    float max_parent_lod_error=0;
//    uint32 i_ofs=0;
//    for(uint32 c:cluster_group.clusters){
//        auto& cluster=clusters[c];
//        for(vec3 p:cluster.verts) pos.push_back(p);
//        for(uint32 i:cluster.indexes) idx.push_back(i+i_ofs);
//        i_ofs+=cluster.verts.size();
//        lod_bounds.push_back(cluster.lod_bounds);
//        max_parent_lod_error=std::max(max_parent_lod_error,cluster.lod_error); //强制父节点的error大于等于子节点
//    }
//    Sphere parent_lod_bound=Sphere::from_spheres(lod_bounds.data(),lod_bounds.size());
//
//    MeshSimplifier simplifier(pos.data(),pos.size(),idx.data(),idx.size());
//    HashTable edge_ht(cluster_group.external_edges.size());
//    uint32 i=0;
//
//    for(auto [c,e]:cluster_group.external_edges){
//        auto& pos=clusters[c].verts;
//        auto& idx=clusters[c].indexes;
//        vec3 p0=pos[idx[e]],p1=pos[idx[cycle3(e)]];
//        edge_ht.add(::hash({p0,p1}),i);
//        simplifier.lock_position(p0);
//        simplifier.lock_position(p1);
//        i++;
//    }
//
//    simplifier.simplify((Cluster::cluster_size-2)*(cluster_group.clusters.size()/2));
//    pos.resize(simplifier.remaining_num_vert());
//    idx.resize(simplifier.remaining_num_tri()*3);
//
//    max_parent_lod_error=max(max_parent_lod_error,sqrt(simplifier.max_error()));
//
//    Graph edge_link,graph;
//    build_adjacency_edge_link(pos,idx,edge_link);
//    build_adjacency_graph(edge_link,graph);
//
//    Partitioner partitioner;
//    partitioner.partition(graph,Cluster::cluster_size-4,Cluster::cluster_size);
//
//    for(auto[l,r]:partitioner.ranges){
//        clusters.push_back({});
//        Cluster& cluster=clusters.back();
//
//        unordered_map<uint32,uint32> mp;
//        for(uint32 i=l;i<r;i++){
//            uint32 t_idx=partitioner.node_id[i];
//            for(uint32 k=0;k<3;k++){
//                uint32 e_idx=t_idx*3+k;
//                uint32 v_idx=idx[e_idx];
//                if(mp.find(v_idx)==mp.end()){ //重映射顶点下标
//                    mp[v_idx]=cluster.verts.size();
//                    cluster.verts.push_back(pos[v_idx]);
//                }
//                bool is_external=false;
//                for(auto[adj_edge,_]:edge_link.g[e_idx]){
//                    uint32 adj_tri=partitioner.sort_to[adj_edge/3];
//                    if(adj_tri<l||adj_tri>=r){ //出点在不同划分说明是边界
//                        is_external=true;
//                        break;
//                    }
//                }
//                vec3 p0=pos[v_idx],p1=pos[idx[cycle3(e_idx)]]; //
//                if(!is_external){
//                    for(uint32 j:edge_ht[::hash({p0,p1})]){
//                        auto [c,e]=cluster_group.external_edges[j];
//                        auto& pos=clusters[c].verts;
//                        auto& idx=clusters[c].indexes;
//                        if(p0==pos[idx[e]]&&p1==pos[idx[cycle3(e)]]){
//                            is_external=true;
//                            break;
//                        }
//                    }
//                }
//
//                if(is_external){
//                    cluster.external_edges.push_back(cluster.indexes.size());
//                }
//                cluster.indexes.push_back(mp[v_idx]);
//            }
//        }
//
//        cluster.mip_level=cluster_group.mip_level+1;
//        cluster.sphere_bounds=Sphere::from_points(cluster.verts.data(),cluster.verts.size());
//        //强制父节点的lod包围盒覆盖所有子节点lod包围盒
//        cluster.lod_bounds=parent_lod_bound;
//        cluster.lod_error=max_parent_lod_error;
//        cluster.box_bounds=cluster.verts[0];
//        for(vec3 p:cluster.verts) cluster.box_bounds=cluster.box_bounds+p;
//    }
//    cluster_group.lod_bounds=parent_lod_bound;
//    cluster_group.max_parent_lod_error=max_parent_lod_error;
//}