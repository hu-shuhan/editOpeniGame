//
// Created by Sumzeek on 23/02/2024.
//

#ifndef IGAMEVIEW_LITE_PARTITIONER_H
#define IGAMEVIEW_LITE_PARTITIONER_H

#include <vector>
#include <map>
#include "metis.h"
#include "utils/types.h"
#include "utils/vec.h"

struct Graph {
    std::vector<std::map<uint32, int>> g;

    void init(uint32 n){
        g.resize(n);
    }
    void add_node(){
        g.push_back({});
    }
    void add_edge(uint32 from, uint32 to,int cost){
        g[from][to]=cost;
    }
    void increase_edge_cost(uint32 from, uint32 to,int i_cost){
        g[from][to]+=i_cost;
    }
};

struct MetisGraph {
    int nvtxs;
    std::vector<idx_t> xadj;
    std::vector<idx_t> adjncy; //压缩图表示
    std::vector<idx_t> adjwgt; //边权重
};

class GraphPartitioner{
private:
    uint32 BisectGraph(MetisGraph* graph_data,MetisGraph* child_graphs[2], uint32 start, uint32 end);
    void RecursiveBisectGraph(MetisGraph* graph_data,uint32 start,uint32 end);
public:
    void init(uint32 num_node); // 初始化Indexes和SortedTo结点
    void Partition(const Graph& graph, uint32 min_part_size, uint32 max_part_size);

    void partition(const Graph& graph,uint32 min_part_size,uint32 max_part_size);

    std::vector<uint32> Indexes; //每个簇的对应结点id(面id)，Indexes[0]-Indexes[127]属于簇0
    std::vector<std::pair<uint32, uint32>> Ranges; //簇的连续范围，Ranges[0]=<0, 128>，簇0有128个元素0-127
    std::vector<uint32> SortedTo; //结点id对应在Indexes中的索引，Indexes[100]=90，SortedTo[90]=100
    uint32 MinPartitionSize;
    uint32 MaxPartitionSize;
};

#endif //IGAMEVIEW_LITE_PARTITIONER_H
