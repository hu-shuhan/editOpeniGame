#ifndef iGameCoderBase_h
#define iGameCoderBase_h

#include "iGameFilter.h"
#include "iGameSurfaceMesh.h"
#include "iGameBoundingBox.h"

namespace comp
{
using Vector3d = iGame::Vector3d;

class Octree {
protected:
    class Node;

public:
    typedef Node NodeType;
    typedef NodeType* NodePointer;
    typedef iGame::BoundingBox VoxelType;
    typedef VoxelType* VoxelPointer;
    typedef iGame::Vector3d Vector3d;
    typedef iGame::Vector3i Vector3i;

protected:
    class Node {
    public:
        Node() {
            parent = nullptr;
            level = -1;
        }
        Node(NodePointer parent, int level) {
            this->parent = parent;
            this->level = level;
            for (int i = 0; i < 8; ++i) { sons[i] = nullptr; }
        }
        ~Node() {}

        inline NodePointer& Son(int sonIndex) {
            assert(0 <= sonIndex && sonIndex <= 8);
            return sons[sonIndex];
        };

    private:
        int id;
        int level;
        NodePointer parent;
        VoxelType voxel;
        NodePointer sons[8];
        bool isLeaf;
        friend class Octree;
    };

public:
    void Initialize(VoxelType box, int maxDepth = 0) {
        this->boundingBox = box;
        this->maxDepth = maxDepth;
        size = 1 << maxDepth;

        Node* root = new Node(nullptr, 0);
        root->id = 0;
        nodes.clear();
        nodes.push_back(root);
        root->voxel = box;
        root->isLeaf = true;
    };

    inline VoxelType BoundingBox() { return boundingBox; }
    inline int Size() { return size; }
    inline int MaxDepth() { return maxDepth; }
    inline int NodeCount() const { return int(nodes.size()); }

    inline NodePointer Root() const { return nodes[0]; }
    inline NodePointer GetNode(int idx) const { return nodes[idx]; }
    inline int Idx(const NodePointer n) const { return n->id; }
    inline VoxelType Voxel(const NodePointer n) { return n->voxel; }
    inline int Level(const NodePointer n) const { return n->level; }
    inline NodePointer& Son(NodePointer n, int i) const { return n->Son(i); }
    inline NodePointer Parent(const NodePointer n) const { return n->parent; }
    inline bool IsLeaf(const NodePointer n) const { return n->isLeaf; }
    inline Vector3d Center(const NodePointer n) const {
        return n->voxel.center();
    }
    inline Vector3d Min(const NodePointer n) const { return n->voxel.min; }
    inline Vector3d Max(const NodePointer n) const { return n->voxel.max; }

    int WhatSon(NodePointer n) const {
        if (n == Root()) assert(false);

        NodePointer parent = Parent(n);
        for (int i = 0; i < 8; ++i)
            if (parent->Son(i) == n) return i;

        return -1;
    }
    NodePointer FindNode(const Vector3i& path) {
        assert(path[0] >= 0 && path[0] < size);
        assert(path[1] >= 0 && path[1] < size);
        assert(path[2] >= 0 && path[2] < size);

        NodePointer curNode = Root();
        int rootLevel = 0;
        int shiftLevel = maxDepth - 1;

        while (shiftLevel >= rootLevel) {
            int nextSon = 0;
            if ((path[2] >> shiftLevel) % 2) nextSon += 1;
            if ((path[1] >> shiftLevel) % 2) nextSon += 2;
            if ((path[0] >> shiftLevel) % 2) nextSon += 4;
            NodePointer nextNode = Son(curNode, nextSon);
            if (nextNode != nullptr) curNode = nextNode;
            else
                return curNode;
            --shiftLevel;
        }
        return curNode;
    }

    NodePointer FindNode2(const Vector3i& path) {
        assert(path[0] >= 0 && path[0] < size);
        assert(path[1] >= 0 && path[1] < size);
        assert(path[2] >= 0 && path[2] < size);

        NodePointer curNode = Root();
        int rootLevel = 0;
        int shiftLevel = maxDepth - 1;

        while (shiftLevel >= rootLevel) {
            int nextSon = 0;
            if ((path[2] >> shiftLevel) % 2) nextSon += 1;
            if ((path[1] >> shiftLevel) % 2) nextSon += 2;
            if ((path[0] >> shiftLevel) % 2) nextSon += 4;
            NodePointer nextNode = Son(curNode, nextSon);
            if (nextNode != nullptr) curNode = nextNode;
            else { curNode = NewNode(curNode, nextSon); }
            --shiftLevel;
        }
        return curNode;
    }

    NodePointer NewNode(NodePointer parent, int i) {
        int level = Level(parent) + 1;

        Node* node = new Node(parent, level);
        nodes.push_back(node);
        Son(parent, i) = node;
        parent->isLeaf = false;
        node->isLeaf = true;
        node->id = nodes.size() - 1;

        VoxelType& bbox = parent->voxel;
        Vector3d center = parent->voxel.center();
        switch (i) {
            case 0:
                node->voxel = VoxelType(bbox.min, center);
                break; // 左后下
            case 1:
                node->voxel = VoxelType(
                        Vector3d(bbox.min[0], bbox.min[1], center[2]), // 左后上
                        Vector3d(center[0], center[1], bbox.max[2]));
                break;
            case 2:
                node->voxel = VoxelType(
                        Vector3d(bbox.min[0], center[1], bbox.min[2]), // 右后下
                        Vector3d(center[0], bbox.max[1], center[2]));
                break;
            case 3:
                node->voxel = VoxelType(
                        Vector3d(bbox.min[0], center[1], center[2]), // 右后上
                        Vector3d(center[0], bbox.max[1], bbox.max[2]));
                break;
            case 4:
                node->voxel = VoxelType(
                        Vector3d(center[0], bbox.min[1], bbox.min[2]), // 左前下
                        Vector3d(bbox.max[0], center[1], center[2]));
                break;
            case 5:
                node->voxel = VoxelType(
                        Vector3d(center[0], bbox.min[1], center[2]), // 左前上
                        Vector3d(bbox.max[0], center[1], bbox.max[2]));
                break;
            case 6:
                node->voxel = VoxelType(
                        Vector3d(center[0], center[1], bbox.min[2]), // 右前下
                        Vector3d(bbox.max[0], bbox.max[1], center[2]));
                break;
            case 7:
                node->voxel = VoxelType(center, bbox.max);
                break; // 右前上
            default:
                break;
        }
        return node;
    }
    void NewNode(NodePointer parent) {
        assert(parent != nullptr);
        assert(parent->isLeaf == true);
        int level = Level(parent) + 1;
        if (level > maxDepth) {
            maxDepth = level;
            size = 1 << maxDepth;
        }

        for (int i = 0; i < 8; ++i) { Node* node = NewNode(parent, i); }
    }

    Vector3i Interize(const Vector3d& pf) const {
        Vector3i pi;

        assert(pf[0] >= boundingBox.min[0] && pf[0] <= boundingBox.max[0]);
        assert(pf[1] >= boundingBox.min[1] && pf[1] <= boundingBox.max[1]);
        assert(pf[2] >= boundingBox.min[2] && pf[2] <= boundingBox.max[2]);

        pi[0] = int((pf[0] - boundingBox.min[0]) * size /
                    (boundingBox.max[0] - boundingBox.min[0]));
        pi[1] = int((pf[1] - boundingBox.min[1]) * size /
                    (boundingBox.max[1] - boundingBox.min[1]));
        pi[2] = int((pf[2] - boundingBox.min[2]) * size /
                    (boundingBox.max[2] - boundingBox.min[2]));

        if (pi[0] >= size) pi[0] = size - 1;
        if (pi[1] >= size) pi[1] = size - 1;
        if (pi[2] >= size) pi[2] = size - 1;
        return pi;
    }

    Vector3d DeInterize(const Vector3i& pi) const {
        Vector3d pf;

        assert(pi[0] >= 0 && pi[0] < size);
        assert(pi[1] >= 0 && pi[1] < size);
        assert(pi[2] >= 0 && pi[2] < size);

        pf[0] = pi[0] * (boundingBox.max[0] - boundingBox.min[0]) / size +
                boundingBox.min[0];
        pf[1] = pi[1] * (boundingBox.max[1] - boundingBox.min[1]) / size +
                boundingBox.min[1];
        pf[2] = pi[2] * (boundingBox.max[2] - boundingBox.min[2]) / size +
                boundingBox.min[2];

        return pf;
    }


private:
    // 2^maxDepth
    int size;
    int maxDepth;

    std::vector<Node*> nodes;
    VoxelType boundingBox;
};

struct Block {
    int pn() const { return ids.size(); }
    std::vector<int> ids;
    Vector3d min;
    int chunk_id;
};
struct Chunk {
    int pn;
    int n_blocks() const { return blocks.size(); }
    std::vector<int> blocks;
};
class Partitioner {
public:
    Partitioner(iGame::PointSet::Pointer mesh) : mesh(mesh) {}

    void operator ()(int pointThr) {
        PointThr = pointThr;
        
        DivideMesh();

        //MergeBlock();
    }

    void DivideMesh() {
        int pn = mesh->GetNumberOfPoints();

        auto bbox = mesh->GetBoundingBox();

        if (pn <= PointThr) {
            blocks.resize(1);
            for (int i = 0; i < pn; i++) {
                blocks[0].ids.push_back(i);
                blocks[0].min = bbox.min;
            }
            label.resize(pn, 0);
            return;
        }

        t.Initialize(bbox);
        t.NewNode(t.Root());
        label.resize(pn);

        std::vector<Block> tempBlocks(t.NodeCount());
        for (int i = 0; i < pn; i++) {
            auto p = mesh->GetPoint(i);
            Octree::NodePointer node =
                    t.FindNode(t.Interize(Vector3d(p[0], p[1], p[2])));
            tempBlocks[t.Idx(node)].ids.push_back(i);
            label[i] = t.Idx(node);
        }


        for (int id = 0; id < tempBlocks.size(); id++) {
            if (tempBlocks[id].pn() > PointThr) {
                DivideBlock(tempBlocks);
                break;
            }
        }

        std::vector<int> block_newId(tempBlocks.size());
        std::vector<int> block_oldId;
        int count = 0;
        for (int id = 0; id < tempBlocks.size(); id++) {
            if (0 < tempBlocks[id].pn() && tempBlocks[id].pn() <= PointThr) {
                blocks.emplace_back(std::move(tempBlocks[id]));
                blocks[count].min = t.Min(t.GetNode(id));
                block_oldId.emplace_back(id);
                block_newId[id] = count;
                count++;
            }
        }
        assert(count == blocks.size());

        for (int i = 0; i < pn; i++) { label[i] = block_newId[label[i]]; }
    }

    void MergeBlock() {
        std::vector<Chunk> tempChunks(blocks.size());
        for (int id = 0; id < blocks.size(); ++id) {
            blocks[id].chunk_id = id;
            Chunk chunk;
            chunk.pn = blocks[id].pn();
            chunk.blocks.push_back(id);
            tempChunks[id] = chunk;
        }

        while (true) {
            bool flag = false;
            int last = -1;
            for (int chunk_id = 0; chunk_id < tempChunks.size(); ++chunk_id) {
                if (tempChunks[chunk_id].n_blocks() != 0 &&
                    tempChunks[chunk_id].pn <
                            PointThr / 2) {
                    if (last == -1) {
                        last = chunk_id;
                    } else {
                        auto& l = tempChunks[last];
                        auto& n = tempChunks[chunk_id];
                        int offset = n.n_blocks();

                        n.pn += l.pn;
                        n.blocks.resize(offset + l.n_blocks());
                        for (int i = 0; i < l.n_blocks(); ++i) {
                            n.blocks[offset + i] = l.blocks[i];
                            blocks[l.blocks[i]].chunk_id = chunk_id;
                        }
                        tempChunks[last].blocks.clear();

                        flag = true;
                        last = -1;
                    }
                }
            }

            if (flag == false) break;
        }

        int count = 0;
        for (int id = 0; id < tempChunks.size(); ++id) {
            if (tempChunks[id].n_blocks() != 0) {
                chunks.emplace_back(std::move(tempChunks[id]));
                for (int i = 0; i < chunks[count].n_blocks(); ++i) {
                    blocks[chunks[count].blocks[i]].chunk_id = count;
                }
                count++;
            }
        }
    }

    size_t BlockSize() const { return blocks.size(); }
    size_t ChunkSize() const { return chunks.size(); }
    const std::vector<Chunk>& GetChunks() const { return chunks; }
    const std::vector<Block>& GetBlocks() const { return blocks; }
    const std::vector<int>& GetLabel() const { return label; }

private:
    void DivideBlock(std::vector<Block>& tempBlocks) {
        for (int id = 0; id < tempBlocks.size(); id++) {
            if (tempBlocks[id].pn() > this->PointThr) {
                t.NewNode(t.GetNode(id));
                tempBlocks.resize(t.NodeCount());
                for (int i = 0; i < tempBlocks[id].pn(); ++i) {
                    auto p = mesh->GetPoint(tempBlocks[id].ids[i]);
                    Octree::NodePointer node =
                            t.FindNode(t.Interize(Vector3d(p[0], p[1], p[2])));
                    tempBlocks[t.Idx(node)].ids.push_back(
                            tempBlocks[id].ids[i]);
                    label[tempBlocks[id].ids[i]] = t.Idx(node);
                }
                tempBlocks[id].ids.clear();
            }
        }

        for (int id = 0; id < tempBlocks.size(); id++) {
            if (tempBlocks[id].pn() > PointThr) {
                DivideBlock(tempBlocks);
                break;
            }
        }
    }

    iGame::PointSet::Pointer mesh;
    std::vector<Block> blocks;
    std::vector<Chunk> chunks;
    std::vector<int> label;
    Octree t;

    int PointThr;
};

template<typename T>
inline uint8_t* Load(uint8_t* ip, T& v) {
    std::memcpy(&v, ip, sizeof(T));
    return ip + sizeof(T);
}
inline uint8_t* Load(uint8_t* ip, void* v, size_t n) {
    std::memcpy(v, ip, n);
    return ip + n;
}

template<typename T>
inline uint8_t* Store(uint8_t* op, T v) {
    std::memcpy(op, &v, sizeof(T));
    return op + sizeof(T);
}
inline uint8_t* Store(uint8_t* op, void* v, size_t n) {
    std::memcpy(op, v, n);
    return op + n;
}

inline uint16_t Load16(const void* p) {
    uint16_t v;
    std::memcpy(&v, p, sizeof(v));
    return v;
}
inline uint32_t Load32(const void* p) {
    uint32_t v;
    std::memcpy(&v, p, sizeof(v));
    return v;
}
inline uint64_t Load64(const void* p) {
    uint64_t v;
    std::memcpy(&v, p, sizeof(v));
    return v;
}

inline void Store16(void* p, uint16_t v) { std::memcpy(p, &v, sizeof(v)); }
inline void Store32(void* p, uint32_t v) { std::memcpy(p, &v, sizeof(v)); }
inline void Store64(void* p, uint64_t v) { std::memcpy(p, &v, sizeof(v)); }

inline void Copy128(const void* src, void* dst) {
    char tmp[16];
    std::memcpy(tmp, src, 16);
    std::memcpy(dst, tmp, 16);
}


// 浮点数量化
// 例如将32位float量化为16为int，Init(range, (1 << 16) - 1)，range = max(abs(valus))
// 返回32位整数，但是高16位都是0
class Quantizer {
public:
    Quantizer() : inverse_delta(1.f) {}
    void Init(float range, int32_t max_quantized_value) {
        inverse_delta = static_cast<float>(max_quantized_value) / range;
    }
    void Init(float delta) { inverse_delta = 1.f / delta; }
    inline int32_t QuantizeFloat(float val) const {
        val *= inverse_delta;
        return static_cast<int32_t>(floor(val + 0.5f));
    }
    inline int32_t operator()(float val) const { return QuantizeFloat(val); }

private:
    float inverse_delta;
};
// 反浮点数量化，参数必须和Quantizer一致
class Dequantizer {
public:
    Dequantizer() : delta(1.f) {}

    bool Init(float range, int32_t max_quantized_value) {
        if (max_quantized_value <= 0) { return false; }
        delta = range / static_cast<float>(max_quantized_value);
        return true;
    }
    bool Init(float delta) {
        delta = delta;
        return true;
    }

    inline float DequantizeFloat(int32_t val) const {
        return static_cast<float>(val) * delta;
    }
    inline float operator()(int32_t val) const { return DequantizeFloat(val); }

private:
    float delta;
};

}
#endif