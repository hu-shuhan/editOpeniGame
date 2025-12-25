//
// Created by Sumzeek on 23/02/2024.
//
#include "Partitioner.h"
#include <cmath>
#include "iostream"
#include <algorithm>

void GraphPartitioner::init(uint32 NumElements){
    Indexes.resize(NumElements);
    SortedTo.resize(NumElements);

    for (uint32 i = 0; i < NumElements; i++) {
        Indexes[i] = i;
        SortedTo[i] = i;
    }
}

MetisGraph* to_metis_data(const Graph& graph){
    MetisGraph* g = new MetisGraph;
    g->nvtxs = graph.g.size();
    for(auto& mp:graph.g){
        g->xadj.push_back(g->adjncy.size());
        for(auto[to,cost]:mp){
            g->adjncy.push_back(to);
            g->adjwgt.push_back(cost);
        }
    }
    g->xadj.push_back(g->adjncy.size());
    return g;
}

void GraphPartitioner::Partition(const Graph& graph, uint32 InMinPartitionSize, uint32 InMaxPartitionSize){
    auto NumElements = graph.g.size();
    init(NumElements);

    MinPartitionSize = InMinPartitionSize;
    MaxPartitionSize = InMaxPartitionSize;
    const int32 TargetPartitionSize = ( MinPartitionSize + MaxPartitionSize ) / 2;
    const int32 TargetNumPartitions = std::ceil(static_cast<float>(NumElements) / static_cast<float>(TargetPartitionSize));

    if( TargetNumPartitions > 1 ) {
        MetisGraph* graph_data = to_metis_data(graph);

        idx_t nVertices = NumElements; // 节点数
        idx_t nWeights = 1; // 节点权重维数
        idx_t nParts = TargetNumPartitions; // 子图个数≥2
        idx_t objval; // 目标函数值

        idx_t options[METIS_NOPTIONS];
        METIS_SetDefaultOptions(options);
        options[METIS_OPTION_UFACTOR] = 200;//( 1000 * MaxPartitionSize * TargetNumPartitions ) / NumElements - 1000;
        //Options[ METIS_OPTION_NCUTS ] = 8;
        //Options[ METIS_OPTION_IPTYPE ] = METIS_IPTYPE_RANDOM;
        //Options[ METIS_OPTION_SEED ] = 17;

        std::vector<idx_t> part(nVertices, 0);

        int ret = METIS_PartGraphKway(
                &nVertices,
                &nWeights,
                graph_data->xadj.data(),
                graph_data->adjncy.data(),
                NULL,
                NULL,
                graph_data->adjwgt.data(),
                &nParts,
                NULL,
                NULL,
                options,
                &objval,
                part.data()
        );

        if (ret == rstatus_et::METIS_OK) {
            std::vector<uint32> ElementCount(TargetNumPartitions);

            for (uint32 i = 0; i < NumElements; ++i) {
                ElementCount[part[i]] ++;
            }

            uint32 Begin = 0;

            // 建立Ranges关系
            Ranges.resize(TargetNumPartitions);
            for(int32 PartitionIndex = 0; PartitionIndex < TargetNumPartitions; PartitionIndex++) {
                Ranges[ PartitionIndex ] = { Begin, Begin + ElementCount[ PartitionIndex ] };
                Begin += ElementCount[ PartitionIndex ];
                ElementCount[ PartitionIndex ] = 0;
            }

            // 建立Indexes关系
            for( uint32 i = 0; i < NumElements; i++ ) {
                uint32 PartitionIndex = part[i];
                uint32 Offset = Ranges[ PartitionIndex ].first;
                uint32 Num = ElementCount[ PartitionIndex ]++;

                Indexes[ Offset + Num ] = i;
            }
        }
    }
    else {
        // Single
        Ranges.emplace_back(0, NumElements);
    }

    for(uint32 i = 0; i < NumElements; i++) {
        SortedTo[Indexes[i]] = i;
    }
}

uint32 GraphPartitioner::BisectGraph(MetisGraph* graph_data,MetisGraph* child_graphs[2], uint32 start, uint32 end)
{
    if(graph_data->nvtxs<=MaxPartitionSize){
        Ranges.push_back({start,end});
        return end;
    }
    const uint32 exp_part_size = (MinPartitionSize + MaxPartitionSize) / 2;
    const uint32 exp_num_parts=std::max(2u,(graph_data->nvtxs+exp_part_size-1)/exp_part_size);

    std::vector<idx_t> swap_to(graph_data->nvtxs, 0);
    std::vector<idx_t> part(graph_data->nvtxs, 0);

    idx_t nw=1,npart=2,ncut=0;
    real_t part_weight[]={
            float(exp_num_parts>>1)/exp_num_parts,
            1.0f-float(exp_num_parts>>1)/exp_num_parts
    };

    int ret=METIS_PartGraphRecursive(
            &graph_data->nvtxs,
            &nw,
            graph_data->xadj.data(),
            graph_data->adjncy.data(),
            nullptr, //vert weights
            nullptr, //vert size
            graph_data->adjwgt.data(),
            &npart,
            part_weight, //partition weight
            nullptr,
            nullptr, //options
            &ncut,
            part.data()
    );

    if (ret == rstatus_et::METIS_OK) {
        int32 l=0,r=graph_data->nvtxs-1;
        while(l<=r){
            while(l<=r&&part[l]==0) swap_to[l]=l,l++;
            while(l<=r&&part[r]==1) swap_to[r]=r,r--;
            if(l<r){
                std::swap(Indexes[start+l],Indexes[start+r]);
                swap_to[l]=r,swap_to[r]=l;
                l++,r--;
            }
        }
        int32 split=l;

        int32 size[2]={split,graph_data->nvtxs-split};

        if(size[0]<=MaxPartitionSize&&size[1]<=MaxPartitionSize){
            Ranges.push_back({start,start+split});
            Ranges.push_back({start+split,end});
        }
        else{
            for(uint32 i=0;i<2;i++){
                child_graphs[i]=new MetisGraph;
                child_graphs[i]->adjncy.reserve(graph_data->adjncy.size()>>1);
                child_graphs[i]->adjwgt.reserve(graph_data->adjwgt.size()>>1);
                child_graphs[i]->xadj.reserve(size[i]+1);
                child_graphs[i]->nvtxs=size[i];
            }
            for(uint32 i=0;i<graph_data->nvtxs;i++){
                uint32 is_rs=(i>=child_graphs[0]->nvtxs);
                uint32 u=swap_to[i];
                MetisGraph* ch=child_graphs[is_rs];
                ch->xadj.push_back(ch->adjncy.size());
                for(uint32 j=graph_data->xadj[u];j<graph_data->xadj[u+1];j++){
                    idx_t v=graph_data->adjncy[j];
                    idx_t w=graph_data->adjwgt[j];
                    v=swap_to[v]-(is_rs?size[0]:0);
                    if(0<=v&&v<size[is_rs]){
                        ch->adjncy.push_back(v);
                        ch->adjwgt.push_back(w);
                    }
                }
            }
            child_graphs[0]->xadj.push_back(child_graphs[0]->adjncy.size());
            child_graphs[1]->xadj.push_back(child_graphs[1]->adjncy.size());
        }
        return start+split;
    }
}

void GraphPartitioner::RecursiveBisectGraph(MetisGraph* graph_data,uint32 start,uint32 end){
    MetisGraph* child_graphs[2] = {nullptr, nullptr};
    uint32 split=BisectGraph(graph_data,child_graphs,start,end);
    delete graph_data;

    if(child_graphs[0]&&child_graphs[1]){
        RecursiveBisectGraph(child_graphs[0],start,split);
        RecursiveBisectGraph(child_graphs[1],split,end);
    }
}

void GraphPartitioner::partition(const Graph& graph,uint32 min_part_size,uint32 max_part_size)
{
    init(graph.g.size());
    this->MinPartitionSize=min_part_size;
    this->MaxPartitionSize=max_part_size;
    MetisGraph* graph_data=to_metis_data(graph);
    RecursiveBisectGraph(graph_data,0,graph_data->nvtxs);
    std::sort(Ranges.begin(),Ranges.end());
    for(uint32 i=0;i<Indexes.size();i++){
        SortedTo[Indexes[i]]=i;
    }
}