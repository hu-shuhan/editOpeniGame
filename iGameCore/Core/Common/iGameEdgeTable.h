#ifndef iGameEdgeTable_h
#define iGameEdgeTable_h

#include "iGameCellArray.h"
#include "iGameObject.h"

IGAME_NAMESPACE_BEGIN
class EdgeTable : public Object {
public:
    I_OBJECT(EdgeTable);
    static constexpr int BLOCK_SIZE = 1;
    static Pointer New() { return new EdgeTable; }

    void Initialize(int vertices_num) { this->NodeHead.resize(vertices_num); }

    igIndex IsEdge(igIndex p1, igIndex p2) {
        igIndex index, search, sum;
        sum = p1 + p2;
        if (p1 < p2) {
            index = p1;
            search = p2;
        } else {
            index = p2;
            search = p1;
        }

        if (index >= this->NodeHead.size() || GetBlockHead(index).NodeId == -1) { return (-1); }

        igIndex i, j, nodeId, size;

        BlockHead& Head = GetBlockHead(index);
        nodeId = Head.NodeId;
        for (i = 0; i < Head.BlockNum; i++) {
            BlockNode& node = GetBlockNode(nodeId);
            igIndex* Data = GetBlockPointer(node.BlockId);
            for (j = 0; j < (i == Head.BlockNum - 1 ? (Head.Size - 1) % BLOCK_SIZE + 1 : BLOCK_SIZE); j++) {
                igIndex idx = Data[j];
                this->Edges->GetCellIds(idx, e);
                if (sum == e->GetId(0) + e->GetId(1)) { return idx; }
            }
            nodeId = node.NextNodeId;
        }
        return (-1);
    }

    void InsertEdge(igIndex p1, igIndex p2) {
        igIndex index, search;

        if (p1 < p2) {
            index = p1;
            search = p2;
        } else {
            index = p2;
            search = p1;
        }

        if (index >= this->NodeHead.size()) {
            size_t newSize = std::max(index + 1, igIndex(this->NodeHead.size() * 1.5));
            this->NodeHead.resize(newSize);
        }

        BlockHead& Head = GetBlockHead(index);
        if (Head.NodeId == -1) {
            int blockId = Mp.Allocate();
            this->Nodes.emplace_back(BlockNode(blockId, -1));

            BlockHead& Head = GetBlockHead(index);
            Head.NodeId = this->Nodes.size() - 1;
            Head.RearId = Head.NodeId;
            Head.BlockNum = 1;
            Head.Size = 0;
        }

        if (Head.Size == Head.BlockNum * BLOCK_SIZE) {
            int blockId = Mp.Allocate();
            this->Nodes[Head.RearId].NextNodeId = this->Nodes.size();
            Head.RearId = this->Nodes.size();
            Head.BlockNum++;
            this->Nodes.emplace_back(BlockNode(blockId, -1));
        }

        igIndex* Data = GetBlockPointer(Nodes[Head.RearId].BlockId);
        Data[Head.Size % BLOCK_SIZE] = this->NumberOfEdges;
        Head.Size++;

        igIndex e[2]{p1, p2};
        this->Edges->AddCellIds(e, 2);
        this->NumberOfEdges++;
    }

    size_t GetNumberOfEdges() { return this->NumberOfEdges; }
    CellArray::Pointer GetOutput() { return this->Edges; }

protected:
    EdgeTable() {
        this->Edges = CellArray::New();
        this->NumberOfEdges = 0;
        this->Mp.SetBlockSize(BLOCK_SIZE);
        this->e = IdArray::New();
    }
    ~EdgeTable() override = default;

    template<class ValueType>
    class MemoryPool {
    public:
        MemoryPool() {}
        ~MemoryPool() {}

        void SetBlockSize(int blockSize) { this->BlockSize = blockSize; }

        int Allocate() {
            if (this->BlockId >= this->NumberOfBlocks) {
                this->Resize(std::max(this->BlockId + 1, this->NumberOfBlocks * 2));
            }
            return this->BlockId++;
        }

        ValueType* GetBlock(int index) { return &Memptr[index * this->BlockSize]; }

    private:
        void Resize(int newSize) {
            Memptr.resize(newSize * this->BlockSize, 0);
            this->NumberOfBlocks = newSize;
        }

        std::vector<ValueType> Memptr;
        int BlockId{0};

        int NumberOfBlocks{0};
        int BlockSize{0};
    };

    struct BlockNode {
        int BlockId{-1}, NextNodeId{-1};
        BlockNode(int BlockId, int NextNodeId) : BlockId(BlockId), NextNodeId(NextNodeId) {}
    };
    struct BlockHead {
        int NodeId{-1}, RearId{-1};
        uint8_t BlockNum{0}, Size{0};
    };

    igIndex* GetBlockPointer(int blockId) { return Mp.GetBlock(blockId); }

    BlockNode& GetBlockNode(int id) { return this->Nodes[id]; }
    BlockHead& GetBlockHead(int id) { return this->NodeHead[id]; }

    MemoryPool<igIndex> Mp;

    std::vector<BlockNode> Nodes;
    std::vector<BlockHead> NodeHead;

    CellArray::Pointer Edges;
    igIndex NumberOfEdges;

    IdArray::Pointer e;
};

IGAME_NAMESPACE_END
#endif