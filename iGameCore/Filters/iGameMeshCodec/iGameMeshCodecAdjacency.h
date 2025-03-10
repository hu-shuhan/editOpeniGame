#ifndef MeshCodecAdjacency_h
#define MeshCodecAdjacency_h

#include "iGameMacro.h"
#include "meshoptimizer.h"

IGAME_NAMESPACE_BEGIN

// modified from meshoptimizer
// license of meshoptimizer in thirdparty\meshoptimizer-0.22
class MeshCodecAdjacency
{
public:
    struct CellAdjacency {
        unsigned int* counts;  // 顶点i -> 邻接live face数量
        unsigned int* offsets; // data的offset
        unsigned int* data;
        //       vertex0            vertex1             ...
        // |cell1, cell3, ...| cell2, cell9, ... |...
    };

    // 变长初始化
    MeshCodecAdjacency(const unsigned int* sourceCellBuffer, const unsigned int* sourceCellOffset, 
        size_t bufferSize, size_t offsetCount, size_t pointCount, size_t cellCount) :
        m_sourceCellBuffer(sourceCellBuffer),
        m_sourceCellOffset(sourceCellOffset),
        m_bufferSize(bufferSize),
        m_offsetCount(offsetCount),
        m_pointCount(pointCount),
        m_cellCount(cellCount)
    {
        BuildHybirdCellAdjacency();
    }

    // 定长初始化
    MeshCodecAdjacency(const unsigned int* sourceCellBuffer, 
        size_t bufferSize, size_t pointCount, size_t fixedCellSize):
        m_sourceCellBuffer(sourceCellBuffer),
        m_bufferSize(bufferSize),
        m_pointCount(pointCount),
        m_fixedCellSize(fixedCellSize)
    {
        BuildCellAdjacency();
    }

    CellAdjacency GetAdjacencyData()
    {
        // 不希望其他函数修改adj
        return m_adj;
    }

private:
    meshopt_Allocator m_optAllocator;
    CellAdjacency m_adj;

    // common field
    const unsigned int* m_sourceCellBuffer;
    size_t m_bufferSize;
    size_t m_pointCount;

    // 不定长field
    const unsigned int* m_sourceCellOffset;
    size_t m_cellCount;
    size_t m_offsetCount;

    // 定长field
    size_t m_fixedCellSize;
    
    // 不定长offset
    void BuildHybirdCellAdjacency() {
        m_adj.counts = m_optAllocator.allocate<unsigned int>(m_pointCount);
        m_adj.offsets = m_optAllocator.allocate<unsigned int>(m_pointCount);
        m_adj.data = m_optAllocator.allocate<unsigned int>(m_bufferSize);

        // fill cell counts
        memset(m_adj.counts, 0, m_pointCount * sizeof(unsigned int));

        // 计算顶点id的直方图 表达了mesh中每个顶点被cell引用的次数
        for (size_t i = 0; i < m_bufferSize; ++i) {
            assert(m_sourceCellBuffer[i] < m_pointCount);
            m_adj.counts[m_sourceCellBuffer[i]]++;
        }
        // 利用直方图计算data的offset
        unsigned int offset = 0;
        for (size_t i = 0; i < m_pointCount; ++i) {
            m_adj.offsets[i] = offset;
            offset += m_adj.counts[i];
        }
        assert(offset == m_bufferSize);

        // 填充data
        for (size_t i = 0; i < m_cellCount; i++) {
            for (size_t j = m_sourceCellOffset[i]; j < m_sourceCellOffset[i + 1]; j++) {
                m_adj.data[m_adj.offsets[m_sourceCellBuffer[j]]++] = unsigned(i);
            }
        }

        // fix offsets that have been disturbed by the previous pass
        for (size_t i = 0; i < m_pointCount; ++i) {
            assert(m_adj.offsets[i] >= m_adj.counts[i]);
            m_adj.offsets[i] -= m_adj.counts[i];
        }
    }

    // 固定offset 
    void BuildCellAdjacency()
    {
        size_t face_count = m_bufferSize / m_fixedCellSize;

        // allocate arrays
        m_adj.counts = m_optAllocator.allocate<unsigned int>(m_pointCount);
        m_adj.offsets = m_optAllocator.allocate<unsigned int>(m_pointCount);
        m_adj.data = m_optAllocator.allocate<unsigned int>(m_bufferSize);

        // fill triangle counts
        memset(m_adj.counts, 0, m_pointCount * sizeof(unsigned int));

        for (size_t i = 0; i < m_bufferSize; ++i) {
            assert(m_sourceCellBuffer[i] < m_pointCount);

            m_adj.counts[m_sourceCellBuffer[i]]++;
        }

        // fill offset table
        unsigned int offset = 0;

        for (size_t i = 0; i < m_pointCount; ++i) {
            m_adj.offsets[i] = offset;
            offset += m_adj.counts[i];
        }

        assert(offset == m_bufferSize);

        // fill cell data
        for (size_t i = 0; i < face_count; ++i) {
            for (size_t j = 0; j < m_fixedCellSize; j++) {
                m_adj.data[m_adj.offsets[m_sourceCellBuffer[i * m_fixedCellSize + j]]++] = unsigned(i);
            }
        }

        // fix offsets that have been disturbed by the previous pass
        for (size_t i = 0; i < m_pointCount; ++i) {
            assert(m_adj.offsets[i] >= m_adj.counts[i]);
            m_adj.offsets[i] -= m_adj.counts[i];
        }
    }
};

IGAME_NAMESPACE_END
#endif