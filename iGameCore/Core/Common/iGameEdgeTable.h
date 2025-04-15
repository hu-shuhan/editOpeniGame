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

    void Initialize(int vertices_num);

    igIndex IsEdge(igIndex p1, igIndex p2);
    void InsertEdge(igIndex p1, igIndex p2);

    size_t GetNumberOfEdges();
    CellArray::Pointer GetOutput();

protected:
    EdgeTable();
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

    igIndex* GetBlockPointer(int blockId);

    BlockNode& GetBlockNode(int id);
    BlockHead& GetBlockHead(int id);

    MemoryPool<igIndex> Mp;

    std::vector<BlockNode> Nodes;
    std::vector<BlockHead> NodeHead;

    CellArray::Pointer Edges;
    igIndex NumberOfEdges;

    IdArray::Pointer e;
};

IGAME_NAMESPACE_END
#endif