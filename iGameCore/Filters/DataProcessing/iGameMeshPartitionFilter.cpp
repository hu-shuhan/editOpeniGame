#include "iGameMeshPartitionFilter.h"
#include "iGameSurfaceMesh.h"
#include "iGameFlatArray.h"
#include "iGameAttributeSet.h"
#include "metis.h"
#include "iGameThreadPool.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <numeric>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <cmath>
#include <map>
#include <unordered_map>
#include <cstring>
#include <span>
#include <cassert>

IGAME_NAMESPACE_BEGIN

namespace {
    using vec3 = Vector3f;
    using uint32 = uint32_t;
    using idx_t = ::idx_t;
    using real_t = ::real_t;

    // --- Utils ---
    inline uint32 cycle3(uint32 i) {
        uint32 imod3 = i % 3;
        return i - imod3 + ((1 << imod3) & 3);
    }

    inline uint32 cycle3(uint32 i, uint32 ofs) {
        return i - i % 3 + (i + ofs) % 3;
    }

    inline uint32 murmur_add(uint32 hash, uint32 elememt) {
        elememt *= 0xcc9e2d51;
        elememt = (elememt << 15) | (elememt >> (32 - 15));
        elememt *= 0x1b873593;

        hash ^= elememt;
        hash = (hash << 13) | (hash >> (32 - 13));
        hash = hash * 5 + 0xe6546b64;
        return hash;
    }

    inline uint32 murmur_mix(uint32 hash) {
        hash ^= hash >> 16;
        hash *= 0x85ebca6b;
        hash ^= hash >> 13;
        hash *= 0xc2b2ae35;
        hash ^= hash >> 16;
        return hash;
    }

    inline uint32 hash(vec3 v) {
        union { float f; uint32 u; } x, y, z;
        x.f = (v[0] == 0.f ? 0 : v[0]);
        y.f = (v[1] == 0.f ? 0 : v[1]);
        z.f = (v[2] == 0.f ? 0 : v[2]);
        return murmur_mix(murmur_add(murmur_add(x.u, y.u), z.u));
    }

    inline uint32 hash(std::pair<vec3, vec3> e) {
        uint32 h0 = hash(e.first);
        uint32 h1 = hash(e.second);
        return murmur_mix(murmur_add(h0, h1));
    }

    inline uint32 lower_nearest_2_power(uint32 x) {
        while (x & (x - 1)) x ^= (x & -x);
        return x;
    }

    inline uint32 upper_nearest_2_power(uint32 x) {
        if (x & (x - 1)) {
            while (x & (x - 1)) x ^= (x & -x);
            return x == 0 ? 1 : (x << 1);
        }
        else {
            return x == 0 ? 1 : (x << 1);
        }
    }

    class HashTable {
    private:
        uint32 hash_size;
        uint32 hash_mask;
        uint32 index_size;
        uint32* hash;
        uint32* next_index;
        void resize_index(uint32 _index_size) {
            uint32* indexs = new uint32[_index_size];
            memcpy(indexs, next_index, sizeof(uint32) * index_size);
            delete[] next_index;
            next_index = indexs;
            index_size = _index_size;
        }

    public:
        HashTable(uint32 _index_size = 0) {
            hash = nullptr, next_index = nullptr;
            resize(_index_size);
        }
        HashTable(uint32 _hash_size, uint32 _index_size) {
            hash = nullptr, next_index = nullptr;
            resize(_hash_size, _index_size);
        }
        ~HashTable() { free(); }

        void resize(uint32 _index_size) { resize(lower_nearest_2_power(_index_size), _index_size); }
        void resize(uint32 _hash_size, uint32 _index_size) {
            free();
            assert((_hash_size & (_hash_size - 1)) == 0);

            hash_size = _hash_size;
            hash_mask = hash_size - 1;
            index_size = _index_size;
            hash = new uint32[hash_size];
            next_index = new uint32[index_size];
            memset(hash, 0xff, hash_size * 4);
        }

        void free() {
            hash_size = 0;
            hash_mask = 0;
            index_size = 0;
            delete[] hash;
            hash = nullptr;
            delete[] next_index;
            next_index = nullptr;
        }
        void clear() { memset(hash, 0xff, hash_size * 4); }

        void add(uint32 key, uint32 idx) {
            if (idx >= index_size) { resize_index(upper_nearest_2_power(idx + 1)); }
            key &= hash_mask;
            next_index[idx] = hash[key];
            hash[key] = idx;
        }
        void remove(uint32 key, uint32 idx) {
            if (idx >= index_size) return;
            key &= hash_mask;
            if (hash[key] == idx) hash[key] = next_index[idx];
            else {
                for (uint32 i = hash[key]; i != ~0u; i = next_index[i]) {
                    if (next_index[i] == idx) {
                        next_index[i] = next_index[idx];
                        break;
                    }
                }
            }
        }

        struct Container {
            uint32 idx;
            uint32* next;
            struct iter {
                uint32 idx;
                uint32* next;
                void operator++() { idx = next[idx]; }
                bool operator!=(const iter& b) const { return idx != b.idx; }
                uint32 operator*() { return idx; }
            };
            iter begin() { return iter{idx, next}; }
            iter end() { return iter{~0u, nullptr}; }
        };

        Container operator[](uint32 key) {
            if (hash_size == 0 || index_size == 0) return Container{~0u, nullptr};
            key &= hash_mask;
            return Container{hash[key], next_index};
        }
    };

    struct FastEdgeHasher {
        template<typename T1, typename T2>
        size_t operator()(const std::pair<T1, T2>& p) const {
            return std::hash<T1>()(p.first) ^ std::hash<T2>()(p.second);
        }
    };

    class FastEdgeHash {
    public:
        typedef std::pair<int, int> key_t;
        typedef int val_t;
        typedef FastEdgeHasher hash_t;
        struct node { //每个哈希表的键值对
            key_t key;
            val_t val;
            struct node* next;
            node() : key(key_t{}), val(val_t{}), next(nullptr) {}
            node(key_t key, val_t val) : key(key), val(val), next(nullptr) {}
        };

        size_t count;
        size_t capacity;
        node** data;

    public:
        FastEdgeHash(int initCapacity = 16) : count(0), capacity(initCapacity) {
            data = new node*[capacity];
            for (int i = 0; i < capacity; i++) { data[i] = nullptr; }
        }
        ~FastEdgeHash() {
            for (int i = 0; i < capacity; i++) {
                if (data[i]) {
                    node* p = data[i];
                    while (p) {
                        node* nxt = p->next;
                        delete p;
                        p = nxt;
                    }
                    data[i] = nullptr;
                }
            }
            delete[] data;
        }

        size_t getIndex(key_t key) const {
            size_t code = hash_t()(key);
            code ^= (code >> 16);
            return code & (capacity - 1);
        }

        bool addOrRemove(key_t key, val_t value, val_t& oldValue) {
            if (count == capacity * 0.75) { resize(); }

            size_t index = getIndex(key);
            if (data[index] == nullptr) {
                data[index] = new node(key, value);
                count++;
                return false;
            }

            if (data[index]->key == key) {
                node* tmp = data[index];
                oldValue = tmp->val;
                data[index] = data[index]->next;
                count--;
                delete tmp;
                return true;
            }

            node* p = data[index];
            while (p->next) {
                if (p->next->key == key) {
                    node* tmp = p->next;
                    oldValue = tmp->val;
                    p->next = p->next->next;
                    count--;
                    delete tmp;
                    return true;
                }
                p = p->next;
            }

            p = new node(key, value);
            p->next = data[index];
            data[index] = p;
            count++;
            return false;
        }


    private:
        void resize() {
            capacity *= 2;
            node** old_data = data;
            data = new node*[capacity];
            for (int i = 0; i < capacity; i++) { data[i] = nullptr; }
            for (int i = 0; i < capacity / 2; i++) {
                if (old_data[i]) {
                    node* cur = old_data[i];
                    while (cur) {
                        node* nxt = cur->next;
                        insert(cur);
                        cur = nxt;
                    }
                }
            }
        }
        void insert(node* p) {
            size_t index = getIndex(p->key);
            p->next = data[index];
            data[index] = p;
        }
    };

    // --- Bounds ---
    struct Bounds {
        vec3 pmin, pmax;
        Bounds() { pmin = vec3(1e9, 1e9, 1e9); pmax = vec3(-1e9, -1e9, -1e9); }
        Bounds(vec3 p) { pmin = p; pmax = p; }
        // Simple update
        void add(vec3 p) {
            pmin[0] = std::min(pmin[0], p[0]); pmin[1] = std::min(pmin[1], p[1]); pmin[2] = std::min(pmin[2], p[2]);
            pmax[0] = std::max(pmax[0], p[0]); pmax[1] = std::max(pmax[1], p[1]); pmax[2] = std::max(pmax[2], p[2]);
        }
    };

    struct Sphere {
        vec3 center;
        float radius;
        static Sphere from_points(vec3* pos, uint32 size) {
            // Simplified bounding sphere
            if (size == 0) return { vec3(0,0,0), 0 };
            vec3 min = pos[0], max = pos[0];
            for (uint32 i = 1; i < size; ++i) {
                min[0] = std::min(min[0], pos[i][0]); min[1] = std::min(min[1], pos[i][1]); min[2] = std::min(min[2], pos[i][2]);
                max[0] = std::max(max[0], pos[i][0]); max[1] = std::max(max[1], pos[i][1]); max[2] = std::max(max[2], pos[i][2]);
            }
            vec3 center = (min + max) * 0.5f;
            float maxSq = 0;
            for (uint32 i = 0; i < size; ++i) {
                vec3 d = pos[i] - center;
                maxSq = std::max(maxSq, DotProduct(d, d));
            }
            return { center, std::sqrt(maxSq) };
        }
    };

    // --- Graph ---
    struct Graph {
        std::vector<std::map<uint32, int>> g;

        void init(uint32 n) {
            g.clear();
            g.resize(n);
        }
        void increase_edge_cost(uint32 from, uint32 to, int i_cost) {
            g[from][to] += i_cost;
        }
    };

    struct MetisGraph {
        idx_t nvtxs;
        std::vector<idx_t> xadj;
        std::vector<idx_t> adjncy;
        std::vector<idx_t> adjwgt;
    };

    // --- GraphPartitioner ---
    struct PartitionContext {
        std::atomic<int> activeTasks{ 0 };
        std::mutex mtx;
        std::condition_variable cv;
    };

    class GraphPartitioner {
    public:
        std::vector<uint32_t> Indexes;
        std::vector<std::pair<uint32_t, uint32_t>> Ranges;
        std::vector<uint32_t> SortedTo;
        uint32_t MinPartitionSize;
        uint32_t MaxPartitionSize;
        std::mutex ranges_mutex;

        void init(uint32_t NumElements) {
            Indexes.resize(NumElements);
            SortedTo.resize(NumElements);
            std::iota(Indexes.begin(), Indexes.end(), 0);
            std::iota(SortedTo.begin(), SortedTo.end(), 0);
            Ranges.clear();
        }

        MetisGraph* to_metis_data(const Graph& graph) {
            MetisGraph* g = new MetisGraph;
            g->nvtxs = (idx_t)graph.g.size();
            g->xadj.reserve(g->nvtxs + 1);
            g->adjncy.reserve(g->nvtxs * 3); // approximate
            for (auto& mp : graph.g) {
                g->xadj.push_back((idx_t)g->adjncy.size());
                for (auto [to, cost] : mp) {
                    g->adjncy.push_back(to);
                    g->adjwgt.push_back(cost);
                }
            }
            g->xadj.push_back((idx_t)g->adjncy.size());
            return g;
        }

        void Partition(const Graph& graph, uint32_t InMinPartitionSize, uint32_t InMaxPartitionSize) {
            uint32_t NumElements = static_cast<uint32_t>(graph.g.size());
            init(NumElements);

            MinPartitionSize = InMinPartitionSize;
            MaxPartitionSize = InMaxPartitionSize;

            const int32_t TargetPartitionSize = (MinPartitionSize + MaxPartitionSize) / 2;
            const int32_t TargetNumPartitions = (int32_t)std::ceil(static_cast<float>(NumElements) / static_cast<float>(TargetPartitionSize));

            if (TargetNumPartitions > 1) {
                MetisGraph* graph_data = to_metis_data(graph);
                idx_t nVertices = (idx_t)NumElements;
                idx_t nWeights = 1;
                idx_t nParts = TargetNumPartitions;
                idx_t objval;

                idx_t options[METIS_NOPTIONS];
                METIS_SetDefaultOptions(options);
                options[METIS_OPTION_UFACTOR] = 200;

                std::vector<idx_t> part(nVertices, 0);

                int ret = METIS_PartGraphKway(
                    &nVertices,
                    &nWeights,
                    graph_data->xadj.data(),
                    graph_data->adjncy.data(),
                    NULL, NULL,
                    graph_data->adjwgt.data(),
                    &nParts,
                    NULL, NULL,
                    options,
                    &objval,
                    part.data()
                );

                delete graph_data;

                if (ret == METIS_OK) {
                    std::vector<uint32> ElementCount(TargetNumPartitions, 0);
                    for (uint32 i = 0; i < NumElements; ++i) ElementCount[part[i]]++;

                    uint32 Begin = 0;
                    Ranges.resize(TargetNumPartitions);
                    for (int32_t PartitionIndex = 0; PartitionIndex < TargetNumPartitions; PartitionIndex++) {
                        Ranges[PartitionIndex] = { Begin, Begin + ElementCount[PartitionIndex] };
                        Begin += ElementCount[PartitionIndex];
                        ElementCount[PartitionIndex] = 0;
                    }

                    for (uint32 i = 0; i < NumElements; i++) {
                        uint32 PartitionIndex = part[i];
                        uint32 Offset = Ranges[PartitionIndex].first;
                        uint32 Num = ElementCount[PartitionIndex]++;
                        Indexes[Offset + Num] = i;
                    }
                }
            }
            else {
                Ranges.emplace_back(0, NumElements);
            }

            for (uint32 i = 0; i < NumElements; i++) {
                SortedTo[Indexes[i]] = i;
            }
        }

        void partition(const Graph& graph, uint32_t min_part_size, uint32_t max_part_size) {
            init((uint32)graph.g.size());
            this->MinPartitionSize = min_part_size;
            this->MaxPartitionSize = max_part_size;
            MetisGraph* graph_data = to_metis_data(graph);

            PartitionContext ctx;
            ctx.activeTasks = 1;

            RecursiveBisectGraph(graph_data, 0, (uint32)graph_data->nvtxs, ctx);

            if (ctx.activeTasks.fetch_sub(1) > 1) {
                std::unique_lock<std::mutex> lk(ctx.mtx);
                ctx.cv.wait(lk, [&] { return ctx.activeTasks == 0; });
            }

            std::sort(Ranges.begin(), Ranges.end());
            for (uint32 i = 0; i < Indexes.size(); i++) {
                SortedTo[Indexes[i]] = i;
            }
        }

    private:
        uint32_t BisectGraph(MetisGraph* graph_data, MetisGraph* child_graphs[2], uint32_t start, uint32_t end) {
            if (graph_data->nvtxs <= (idx_t)MaxPartitionSize) {
                std::lock_guard<std::mutex> lock(ranges_mutex);
                Ranges.push_back({ start, end });
                return end;
            }

            const uint32_t exp_part_size = (MinPartitionSize + MaxPartitionSize) / 2;
            const uint32_t exp_num_parts = std::max(2u, (uint32_t)((graph_data->nvtxs + exp_part_size - 1) / exp_part_size));

            std::vector<idx_t> swap_to(graph_data->nvtxs, 0);
            std::vector<idx_t> part(graph_data->nvtxs, 0);

            idx_t nw = 1;
            idx_t npart = 2;
            idx_t ncut = 0;

            real_t part_weight[] = {
                    float(exp_num_parts >> 1) / exp_num_parts,
                    1.0f - float(exp_num_parts >> 1) / exp_num_parts
            };

            idx_t options[METIS_NOPTIONS];
            METIS_SetDefaultOptions(options);
            options[METIS_OPTION_UFACTOR] = 200;

            int ret = METIS_PartGraphRecursive(
                &graph_data->nvtxs, &nw, graph_data->xadj.data(), graph_data->adjncy.data(),
                nullptr, nullptr, graph_data->adjwgt.data(), &npart, part_weight, nullptr,
                options, &ncut, part.data()
            );

            if (ret == METIS_OK) {
                int32_t l = 0, r = (int32_t)graph_data->nvtxs - 1;
                while (l <= r) {
                    while (l <= r && part[l] == 0) swap_to[l] = l, l++;
                    while (l <= r && part[r] == 1) swap_to[r] = r, r--;
                    if (l < r) {
                        std::swap(Indexes[start + l], Indexes[start + r]);
                        swap_to[l] = r, swap_to[r] = l;
                        l++, r--;
                    }
                }
                int32_t split = l;
                int32_t size[2] = { split, (int32_t)graph_data->nvtxs - split };

                if (size[0] <= (int32_t)MaxPartitionSize && size[1] <= (int32_t)MaxPartitionSize) {
                    std::lock_guard<std::mutex> lock(ranges_mutex);
                    Ranges.push_back({ start, start + (uint32_t)split });
                    Ranges.push_back({ start + (uint32_t)split, end });
                }
                else {
                    for (uint32_t i = 0; i < 2; i++) {
                        child_graphs[i] = new MetisGraph;
                        child_graphs[i]->adjncy.reserve(graph_data->adjncy.size() >> 1);
                        child_graphs[i]->adjwgt.reserve(graph_data->adjwgt.size() >> 1);
                        child_graphs[i]->xadj.reserve(size[i] + 1);
                        child_graphs[i]->nvtxs = size[i];
                    }
                    for (int32_t i = 0; i < graph_data->nvtxs; i++) {
                        uint32_t is_rs = (i >= child_graphs[0]->nvtxs);
                        idx_t u = swap_to[i];
                        MetisGraph* ch = child_graphs[is_rs];
                        ch->xadj.push_back((idx_t)ch->adjncy.size());

                        for (idx_t j = graph_data->xadj[u]; j < graph_data->xadj[u + 1]; j++) {
                            idx_t v = graph_data->adjncy[j];
                            idx_t w = graph_data->adjwgt[j];
                            v = swap_to[v] - (is_rs ? size[0] : 0);
                            if (0 <= v && v < size[is_rs]) {
                                ch->adjncy.push_back(v);
                                ch->adjwgt.push_back(w);
                            }
                        }
                    }
                    child_graphs[0]->xadj.push_back((idx_t)child_graphs[0]->adjncy.size());
                    child_graphs[1]->xadj.push_back((idx_t)child_graphs[1]->adjncy.size());
                }
                return start + split;
            }
            else {
                std::lock_guard<std::mutex> lock(ranges_mutex);
                Ranges.push_back({ start, end });
                return end;
            }
        }

        void RecursiveBisectGraph(MetisGraph* graph_data, uint32_t start, uint32_t end, PartitionContext& ctx) {
            MetisGraph* child_graphs[2] = { nullptr, nullptr };
            uint32_t split = BisectGraph(graph_data, child_graphs, start, end);
            delete graph_data;

            if (child_graphs[0] && child_graphs[1]) {
                if ((split - start) > 1000) {
                    ctx.activeTasks++;
                    ThreadPool::Instance()->Commit([=, &ctx]() {
                        this->RecursiveBisectGraph(child_graphs[0], start, split, ctx);
                        if (ctx.activeTasks.fetch_sub(1) == 1) {
                            std::unique_lock<std::mutex> lk(ctx.mtx);
                            ctx.cv.notify_all();
                        }
                        });
                    this->RecursiveBisectGraph(child_graphs[1], split, end, ctx);
                }
                else {
                    this->RecursiveBisectGraph(child_graphs[0], start, split, ctx);
                    this->RecursiveBisectGraph(child_graphs[1], split, end, ctx);
                }
            }
        }
    };

    // --- Core Functions ---

    void build_adjacency_edge_link(const std::vector<vec3>& verts, const std::vector<uint32>& indexes, Graph& edge_link) {
        HashTable edge_ht(indexes.size());
        edge_link.init(indexes.size());

        for (uint32 i = 0; i < indexes.size(); i++) {
            vec3 p0 = verts[indexes[i]];
            vec3 p1 = verts[indexes[cycle3(i)]];
            edge_ht.add(hash({p0, p1}), i); 

            for (uint32 j: edge_ht[hash({p1, p0})]) {
                if (p1 == verts[indexes[j]] && p0 == verts[indexes[cycle3(j)]]) {
                    edge_link.increase_edge_cost(i, j, 1);
                    edge_link.increase_edge_cost(j, i, 1);
                }
            }
        }
    }

    void build_adjacency_edge_link(const std::vector<uint32>& indexes, Graph& edge_link) {
        FastEdgeHash mp;
        edge_link.init(indexes.size());

        static const int next[3] = {1, 2, 0};

        for (size_t i = 0; i < indexes.size() / 3 ; i++) {
            for (int e = 0; e < 3; ++e) {
                uint32 i0 = std::min(indexes[i * 3 + e], indexes[i * 3 + next[e]]);
                uint32 i1 = std::max(indexes[i * 3 + e], indexes[i * 3 + next[e]]);
                uint32 current_edge_index = i * 3 + e; // 当前半边索引
                int neighbor_edge_index;

                if (mp.addOrRemove(std::make_pair<int, int>(i0, i1), current_edge_index, neighbor_edge_index)) {
                    edge_link.increase_edge_cost(current_edge_index, neighbor_edge_index, 1);
                    edge_link.increase_edge_cost(neighbor_edge_index, current_edge_index, 1);
                }
            }
        }
    }

    void build_adjacency_graph(const Graph& edge_link, Graph& graph) {
        graph.init((uint32)edge_link.g.size() / 3);
        uint32 u = 0;
        for (const auto& mp : edge_link.g) {
            for (auto [v, w] : mp) {
                graph.increase_edge_cost(u / 3, v / 3, 1);
            }
            u++;
        }
    }

    using Cluster = MeshPartitionFilter::Cluster;
    using ClusterGroup = MeshPartitionFilter::ClusterGroup;

    void cluster_triangles(const std::vector<vec3>& verts, const std::vector<uint32>& indexes, std::vector<Cluster>& clusters) {
        Graph edge_link, graph;
        auto start_build_adjacency = std::chrono::high_resolution_clock::now();
        build_adjacency_edge_link(indexes, edge_link);
    
        auto end_build_adjacency = std::chrono::high_resolution_clock::now();
        std::cout << "	Build Adjacency time: "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(end_build_adjacency - start_build_adjacency).count()
                  << "ms" << std::endl;
        build_adjacency_graph(edge_link, graph);
        auto start_partitioner = std::chrono::high_resolution_clock::now();
        GraphPartitioner partitioner;
        partitioner.partition(graph, Cluster::cluster_size - 4, Cluster::cluster_size);
        auto end_partitioner = std::chrono::high_resolution_clock::now();
        std::cout << "	Partitioner time: "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(end_partitioner - start_partitioner).count()
                  << "ms" << std::endl;



        for (auto [l, r] : partitioner.Ranges) {
            clusters.push_back({});
            Cluster& cluster = clusters.back();

            std::unordered_map<uint32, uint32> mp;
            for (uint32 i = l; i < r; i++) {
                uint32 t_idx = partitioner.Indexes[i];
                cluster.faceIndices.push_back(t_idx); // Store global face index

                for (uint32 k = 0; k < 3; k++) {
                    uint32 e_idx = t_idx * 3 + k;
                    uint32 v_idx = indexes[e_idx];
                    if (mp.find(v_idx) == mp.end()) {
                        mp[v_idx] = (uint32)cluster.verts.size();
                        cluster.verts.push_back(verts[v_idx]);
                    }
                    bool is_external = false;
                    for (auto [adj_edge, _] : edge_link.g[e_idx]) {
                        uint32 adj_tri = partitioner.SortedTo[adj_edge / 3];
                        if (adj_tri < l || adj_tri >= r) {
                            is_external = true;
                            break;
                        }
                    }
                    if (is_external) {
                        cluster.externalEdgeIds.push_back((int)cluster.indexes.size());
                    }
                    cluster.indexes.push_back(mp[v_idx]);
                }
            }

            cluster.mip_level = 0;
            cluster.lod_error = 0;
            // cluster.sphere_bounds = Sphere::from_points(cluster.verts.data(), (uint32)cluster.verts.size());
            // cluster.lod_bounds = cluster.sphere_bounds;
            cluster.box_min = cluster.verts[0];
            cluster.box_max = cluster.verts[0];
            for (const auto& p : cluster.verts) {
                cluster.box_min[0] = std::min(cluster.box_min[0], p[0]); cluster.box_min[1] = std::min(cluster.box_min[1], p[1]); cluster.box_min[2] = std::min(cluster.box_min[2], p[2]);
                cluster.box_max[0] = std::max(cluster.box_max[0], p[0]); cluster.box_max[1] = std::max(cluster.box_max[1], p[1]); cluster.box_max[2] = std::max(cluster.box_max[2], p[2]);
            }
        }
    }

    void build_clusters_edge_link(std::span<const Cluster> clusters, const std::vector<std::pair<uint32, uint32>>& ext_edges, Graph& edge_link) {
        std::unordered_map<uint32, std::vector<uint32>> edge_ht;
        edge_link.init((uint32)ext_edges.size());

        uint32 i = 0;
        for (auto [c_id, e_id] : ext_edges) {
            auto& pos = clusters[c_id].verts;
            auto& idx = clusters[c_id].indexes;
            vec3 p0 = pos[idx[e_id]];
            vec3 p1 = pos[idx[cycle3(e_id)]];
            edge_ht[hash({ p0,p1 })].push_back(i);
            
            auto it = edge_ht.find(hash({ p1,p0 }));
            if (it != edge_ht.end()) {
                for (uint32 j : it->second) {
                    auto [c_id1, e_id1] = ext_edges[j];
                    auto& pos1 = clusters[c_id1].verts;
                    auto& idx1 = clusters[c_id1].indexes;

                    if (pos1[idx1[e_id1]] == p1 && pos1[idx1[cycle3(e_id1)]] == p0) {
                        edge_link.increase_edge_cost(i, j, 1);
                        edge_link.increase_edge_cost(j, i, 1);
                    }
                }
            }
            i++;
        }
    }

    void build_clusters_graph(const Graph& edge_link, const std::vector<uint32>& mp, uint32 num_cluster, Graph& graph) {
        graph.init(num_cluster);
        uint32 u = 0;
        for (const auto& emp : edge_link.g) {
            for (auto [v, w] : emp) {
                graph.increase_edge_cost(mp[u], mp[v], 1);
            }
            u++;
        }
    }

    void group_clusters(std::vector<Cluster>& clusters, uint32 offset, uint32 num_cluster, std::vector<ClusterGroup>& cluster_groups, uint32 mip_level) {
        std::span<const Cluster> clusters_view(clusters.begin() + offset, num_cluster);

        std::vector<uint32> mp; // edge_id to cluster_id (local index in view)
        std::vector<uint32> mp1; // cluster_id to first_edge_id
        std::vector<std::pair<uint32, uint32>> ext_edges;
        uint32 i = 0;
        for (auto& cluster : clusters_view) {
            assert(cluster.mip_level == mip_level);
            mp1.push_back((uint32)mp.size());
            for (int e : cluster.externalEdgeIds) {
                ext_edges.push_back({ i, (uint32)e });
                mp.push_back(i);
            }
            i++;
        }
        Graph edge_link, graph;
        build_clusters_edge_link(clusters_view, ext_edges, edge_link);
        build_clusters_graph(edge_link, mp, num_cluster, graph);

        GraphPartitioner partitioner;
        partitioner.Partition(graph, ClusterGroup::group_size - 4, ClusterGroup::group_size);

        for (auto [l, r] : partitioner.Ranges) {
            cluster_groups.push_back({});
            auto& group = cluster_groups.back();
            group.mip_level = mip_level;
            for (uint32 i = l; i < r; i++) {
                uint32 c_id = partitioner.Indexes[i]; // local cluster id in view
                clusters[c_id + offset].group_id = (int)cluster_groups.size() - 1;
                group.clusters.push_back(c_id + offset);
                for (uint32 e_idx = mp1[c_id]; e_idx < mp.size() && mp[e_idx] == c_id; e_idx++) {
                    bool is_external = false;
                    for (auto [adj_e, _] : edge_link.g[e_idx]) {
                        uint32 adj_cl = partitioner.SortedTo[mp[adj_e]];
                        if (adj_cl < l || adj_cl >= r) {
                            is_external = true;
                            break;
                        }
                    }
                    if (is_external) {
                        uint32 e = ext_edges[e_idx].second;
                        group.external_edges.push_back({ c_id + offset, (int)e });
                    }
                }
            }
        }
    }

} // anonymous namespace

bool MeshPartitionFilter::Execute() {
    auto mesh = DynamicCast<SurfaceMesh>(GetInput(0));
    if (!mesh) return false;

    auto start_total = std::chrono::high_resolution_clock::now();

    // 1. Prepare Data
    auto start_prep = std::chrono::high_resolution_clock::now();
    IGsize nPoints = mesh->GetNumberOfPoints();
    IGsize nFaces = mesh->GetNumberOfFaces();
    
    std::vector<vec3> verts(nPoints);
    for (IGsize i = 0; i < nPoints; ++i) {
        verts[i] = mesh->GetPoint(i);
    }

    std::vector<uint32> indexes;
    indexes.reserve(nFaces * 3);
    for (IGsize i = 0; i < nFaces; ++i) {
        igIndex ptIds[3];
        mesh->GetFacePointIds(i, ptIds);
        indexes.push_back(ptIds[0]);
        indexes.push_back(ptIds[1]);
        indexes.push_back(ptIds[2]);
    }
    auto end_prep = std::chrono::high_resolution_clock::now();
    std::cout << "Prepare Data time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end_prep - start_prep).count() << "ms" << std::endl;

    // 2. Cluster Triangles
    auto start_cluster = std::chrono::high_resolution_clock::now();
    m_Clusters.clear();
    cluster_triangles(verts, indexes, m_Clusters);
    auto end_cluster = std::chrono::high_resolution_clock::now();
    std::cout << "Cluster Triangles time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end_cluster - start_cluster).count() << "ms" << std::endl;

    // 3. Group Clusters
    auto start_group = std::chrono::high_resolution_clock::now();
    m_ClusterGroups.clear();
    group_clusters(m_Clusters, 0, (uint32)m_Clusters.size(), m_ClusterGroups, 0);
    auto end_group = std::chrono::high_resolution_clock::now();
    std::cout << "Group Clusters time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end_group - start_group).count() << "ms" << std::endl;

    // 4. Output Attributes
    auto start_output = std::chrono::high_resolution_clock::now();
    auto partitionIdArray = IntArray::New();
    partitionIdArray->Resize(nFaces);
    partitionIdArray->SetName("PartitionId");
    
    auto clusterGroupIdArray = IntArray::New();
    clusterGroupIdArray->Resize(nFaces);
    clusterGroupIdArray->SetName("ClusterGroupId");

    for (size_t c = 0; c < m_Clusters.size(); ++c) {
        const auto& cluster = m_Clusters[c];
        for (int fIdx : cluster.faceIndices) {
            if (fIdx >= 0 && fIdx < nFaces) {
                partitionIdArray->SetValue(fIdx, (int)c % 10);
                clusterGroupIdArray->SetValue(fIdx, cluster.group_id % 10);
            }
        }
    }

    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_CELL, partitionIdArray);
    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_CELL, clusterGroupIdArray);
    SetOutput(mesh);

    auto end_output = std::chrono::high_resolution_clock::now();
    std::cout << "Output Attributes time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end_output - start_output).count() << "ms" << std::endl;

    auto end_total = std::chrono::high_resolution_clock::now();
    std::cout << "Total time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end_total - start_total).count() << "ms" << std::endl;

    return true;
}

MeshPartitionFilter::MeshPartitionFilter() { 
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

IGAME_NAMESPACE_END
