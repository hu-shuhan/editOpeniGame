//
// Created by Sumzeek on 05/02/2024.
//

#include "Builder.h"
#include <iostream>
#include "tracy/Tracy.hpp"

void Builder::build(const std::string& file_name) {
    // 读取文件
    {
      ZoneScopedNC("Load obj File", tracy::Color::Red);
      TracyMessage("Load obj File", strlen("Load obj File"));
      myMesh.ReadObjFile(file_name);
    }

    {
      ZoneScopedNC("cluster triangles", tracy::Color::Green);
      TracyMessage("cluster triangles", strlen("cluster triangles"));
      auto& [pos,idx] = myMesh;
      cluster_triangles(pos,idx,myVirtualMesh.clusters);
    }

    {
      ZoneScopedNC("group clusters", tracy::Color::Blue);
      TracyMessage("group clusters", strlen("group clusters"));
      // 将三角形分组为三角形簇
      uint32 level_offset = 0, mip_level = 0;
      uint32 num_level_clusters = myVirtualMesh.clusters.size()-level_offset;
      if(num_level_clusters <= 1) return;
      // 将三角形簇分组
      group_clusters(
          myVirtualMesh.clusters,
          level_offset,
          num_level_clusters,
          myVirtualMesh.cluster_groups,
          mip_level
      );
    }
}

void Builder::getGPUdata(int mode){
    #define MAX_COLORS 9
    std::vector<float> BrightColors = {
            0.9882, 0.1647, 0.2078,  // 红色
            0.9882, 0.6431, 0.0824,  // 橙色
            1.0000, 0.9216, 0.2314,  // 黄色
            0.3294, 0.9176, 0.5451,  // 绿色
            0.0902, 0.7451, 0.8118,  // 蓝绿色
            0.1882, 0.7490, 0.9333,  // 蓝色
            0.5373, 0.5412, 0.9882,  // 紫色
            0.8902, 0.4667, 0.7608,  // 粉红色
            0.9804, 0.7137, 0.8235   // 淡粉红色
            // ... 可以添加更多的亮色系颜色
    };

    uint32 cluster_idx = 0;
    uint32 clusterGroup_idx = 0;
    switch (mode) {
        case 0:
          std::cout << "origin" << std::endl;
            this->vertices->clear();
            this->vertices->reserve(myMesh.indices.size() * 18);

            // 面片
            for (uint32 i = 0; i < myMesh.indices.size(); i += 3) {
                std::vector<uint32> idx = {myMesh.indices[i], myMesh.indices[i + 1], myMesh.indices[i + 2]};
                for (int j = 0; j < 3; ++j) {
                    this->vertices->emplace_back(myMesh.positions[idx[j]].x);
                    this->vertices->emplace_back(myMesh.positions[idx[j]].y);
                    this->vertices->emplace_back(myMesh.positions[idx[j]].z);
                    this->vertices->emplace_back(0.5);
                    this->vertices->emplace_back(0.5);
                    this->vertices->emplace_back(0.5);
                }
            }

            // 线框
            for (uint32 i = 0; i < myMesh.indices.size(); i += 3) {
                std::vector<uint32> idx = {myMesh.indices[i], myMesh.indices[i + 1], myMesh.indices[i + 2]};
                for (int j = 0; j < 3; ++j) {
                    auto i0 = idx[j];
                    auto i1 = idx[(j + 1) % 3];
                    this->vertices->emplace_back(myMesh.positions[i0].x);
                    this->vertices->emplace_back(myMesh.positions[i0].y);
                    this->vertices->emplace_back(myMesh.positions[i0].z);
                    this->vertices->emplace_back(0);
                    this->vertices->emplace_back(0);
                    this->vertices->emplace_back(0);
                    this->vertices->emplace_back(myMesh.positions[i1].x);
                    this->vertices->emplace_back(myMesh.positions[i1].y);
                    this->vertices->emplace_back(myMesh.positions[i1].z);
                    this->vertices->emplace_back(0);
                    this->vertices->emplace_back(0);
                    this->vertices->emplace_back(0);
                }
            }
            break;
        case 1:
          std::cout << "cluster" << std::endl;
            this->vertices->clear();
            this->vertices->reserve(myMesh.indices.size() * 18);

            // 面片
            for (const auto& cluster : myVirtualMesh.clusters) {
                for (uint32 i = 0; i < cluster.indexes.size(); i += 3) {
                    std::vector<uint32> idx = {cluster.indexes[i], cluster.indexes[i + 1], cluster.indexes[i + 2]};
                    for (int j = 0; j < 3; ++j) {
                        this->vertices->emplace_back(cluster.verts[idx[j]].x);
                        this->vertices->emplace_back(cluster.verts[idx[j]].y);
                        this->vertices->emplace_back(cluster.verts[idx[j]].z);
                        this->vertices->emplace_back(BrightColors[(cluster_idx % MAX_COLORS) * 3]);
                        this->vertices->emplace_back(BrightColors[(cluster_idx % MAX_COLORS) * 3 + 1]);
                        this->vertices->emplace_back(BrightColors[(cluster_idx % MAX_COLORS) * 3 + 2]);
                    }
                }
                cluster_idx++;
            }

            // 线框
            for (const auto& cluster : myVirtualMesh.clusters) {
                for (uint32 i = 0; i < cluster.indexes.size(); i += 3) {
                    std::vector<uint32> idx = {cluster.indexes[i], cluster.indexes[i + 1], cluster.indexes[i + 2]};
                    for (int j = 0; j < 3; ++j) {
                        auto i0 = idx[j];
                        auto i1 = idx[(j + 1) % 3];
                        this->vertices->emplace_back(cluster.verts[i0].x);
                        this->vertices->emplace_back(cluster.verts[i0].y);
                        this->vertices->emplace_back(cluster.verts[i0].z);
                        this->vertices->emplace_back(0);
                        this->vertices->emplace_back(0);
                        this->vertices->emplace_back(0);
                        this->vertices->emplace_back(cluster.verts[i1].x);
                        this->vertices->emplace_back(cluster.verts[i1].y);
                        this->vertices->emplace_back(cluster.verts[i1].z);
                        this->vertices->emplace_back(0);
                        this->vertices->emplace_back(0);
                        this->vertices->emplace_back(0);
                    }
                }
            }
            std::cout << this->vertices->size() << std::endl;

            break;
        case 2:
          std::cout << "cluster group" << std::endl;
            this->vertices->clear();
            this->vertices->reserve(myMesh.indices.size() * 18);

            // 面片
            for (const auto& cluster_group : myVirtualMesh.cluster_groups) {
                for (const auto& cluster_idx : cluster_group.clusters) {
                    auto& cluster = myVirtualMesh.clusters[cluster_idx];
                    for (uint32 i = 0; i < cluster.indexes.size(); i += 3) {
                        std::vector<uint32> idx = {cluster.indexes[i], cluster.indexes[i + 1], cluster.indexes[i + 2]};
                        for (int j = 0; j < 3; ++j) {
                            this->vertices->emplace_back(cluster.verts[idx[j]].x);
                            this->vertices->emplace_back(cluster.verts[idx[j]].y);
                            this->vertices->emplace_back(cluster.verts[idx[j]].z);
                            this->vertices->emplace_back(BrightColors[(clusterGroup_idx % MAX_COLORS) * 3]);
                            this->vertices->emplace_back(BrightColors[(clusterGroup_idx % MAX_COLORS) * 3 + 1]);
                            this->vertices->emplace_back(BrightColors[(clusterGroup_idx % MAX_COLORS) * 3 + 2]);
                        }
                    }
                }
                clusterGroup_idx++;
            }

            // 线框
            for (const auto& cluster_group : myVirtualMesh.cluster_groups) {
                for (const auto& cluster_idx : cluster_group.clusters) {
                    auto& cluster = myVirtualMesh.clusters[cluster_idx];
                    for (uint32 i = 0; i < cluster.indexes.size(); i += 3) {
                        std::vector<uint32> idx = {cluster.indexes[i], cluster.indexes[i + 1], cluster.indexes[i + 2]};
                        for (int j = 0; j < 3; ++j) {
                            auto i0 = idx[j];
                            auto i1 = idx[(j + 1) % 3];
                            this->vertices->emplace_back(cluster.verts[i0].x);
                            this->vertices->emplace_back(cluster.verts[i0].y);
                            this->vertices->emplace_back(cluster.verts[i0].z);
                            this->vertices->emplace_back(0);
                            this->vertices->emplace_back(0);
                            this->vertices->emplace_back(0);
                            this->vertices->emplace_back(cluster.verts[i1].x);
                            this->vertices->emplace_back(cluster.verts[i1].y);
                            this->vertices->emplace_back(cluster.verts[i1].z);
                            this->vertices->emplace_back(0);
                            this->vertices->emplace_back(0);
                            this->vertices->emplace_back(0);
                        }
                    }
                }
            }
            std::cout << this->vertices->size() << std::endl;
            break;
        default:
            std::cout << "Mode Setting Error! " << std::endl;
            return;
    }

//    //  一个面30个数据
//    this->vertices->clear();
//    this->vertices->reserve(myMesh.indices.size() * 18);
//
//    // 面片
//    for (uint32 i = 0; i < myMesh.indices.size(); i += 3) {
//        std::vector<uint32> idx = {myMesh.indices[i], myMesh.indices[i + 1], myMesh.indices[i + 2]};
//        for (int j = 0; j < 3; ++j) {
//            this->vertices->emplace_back(myMesh.positions[idx[j]].x);
//            this->vertices->emplace_back(myMesh.positions[idx[j]].y);
//            this->vertices->emplace_back(myMesh.positions[idx[j]].z);
//            this->vertices->emplace_back(0.5);
//            this->vertices->emplace_back(0.5);
//            this->vertices->emplace_back(0.5);
//        }
//    }
//
//    // 线框
//    for (uint32 i = 0; i < myMesh.indices.size(); i += 3) {
//        std::vector<uint32> idx = {myMesh.indices[i], myMesh.indices[i + 1], myMesh.indices[i + 2]};
//        for (int j = 0; j < 3; ++j) {
//            auto i0 = idx[j];
//            auto i1 = idx[(j + 1) % 3];
//            this->vertices->emplace_back(myMesh.positions[i0].x);
//            this->vertices->emplace_back(myMesh.positions[i0].y);
//            this->vertices->emplace_back(myMesh.positions[i0].z);
//            this->vertices->emplace_back(0);
//            this->vertices->emplace_back(0);
//            this->vertices->emplace_back(0);
//            this->vertices->emplace_back(myMesh.positions[i1].x);
//            this->vertices->emplace_back(myMesh.positions[i1].y);
//            this->vertices->emplace_back(myMesh.positions[i1].z);
//            this->vertices->emplace_back(0);
//            this->vertices->emplace_back(0);
//            this->vertices->emplace_back(0);
//        }
//    }
}