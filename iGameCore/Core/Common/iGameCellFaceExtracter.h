#pragma once
#include <iGameMacro.h>
#include <vector>
#include <map>
#include <utility>
#include <set>
#include <iGameCell.h>
#include <mutex>
IGAME_NAMESPACE_BEGIN
class UnstructuredMesh;
class CellFaceExtracter {
public:
    void AddCell(const std::vector<int>& ids, UnstructuredMesh* mesh);
    void AddCell(int id, Cell* cell, bool useMutex = false);
    void RemoveCell(const std::vector<int>& ids, UnstructuredMesh* mesh);
    void RemoveCell(int id, Cell* cell, bool useMutex = false);
    void Clear();
    std::set<std::pair<int, int>> GetExtractPointIdPairs();

private:
    using PointId=int;
    using CellId=int;
    std::mutex m_CellsMutex;
    std::map<std::vector<PointId>, std::map<CellId, std::vector<std::pair<PointId, PointId>>>> m_Cells;

private:
    void _AddCell(int id, Cell* cell, bool useMutex);
    void _RemoveCell(int id, Cell* cell, bool useMutex);
};
IGAME_NAMESPACE_END