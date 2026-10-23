#ifndef iGameAppendDatasetsFilter_h
#define iGameAppendDatasetsFilter_h

#include "iGameFilter.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGamePoints.h"
#include "iGameCellArray.h"

#include <vector>
#include <unordered_map>
#include <string>

IGAME_NAMESPACE_BEGIN

/**
 * AppendDatasetsFilter — 对标 ParaView 的 Append Datasets 滤镜
 * (VTK 类: vtkAppendFilter)
 *
 * 将多个输入数据集合并为单个 UnstructuredMesh（保留原始 cell 类型）。
 *  - 支持多输入：通过 AddInput 动态追加
 *  - MergePoints=true 时合并重合点（哈希 + 距离校验）
 *  - 属性仅在所有输入都具备时才保留（交集规则）
 *
 * 类名/文件名与 ParaView 滤镜 "Append Datasets" 一一对应。
 */
class AppendDatasetsFilter : public Filter {
public:
    I_OBJECT(AppendDatasetsFilter);
    static Pointer New() { return new AppendDatasetsFilter; }

    ~AppendDatasetsFilter() override;

    bool Execute() override;

    void SetMergePoints(bool merge) { m_MergePoints = merge; }
    bool GetMergePoints() const { return m_MergePoints; }

    void SetTolerance(float tol) { m_Tolerance = tol; }
    float GetTolerance() const { return m_Tolerance; }

    void AddInput(DataObject::Pointer data);

    // 执行失败时的错误信息
    std::string GetMessage() const { return m_Message; }

protected:
    AppendDatasetsFilter();

private:
    // MeshPointMap 跟踪每个输入 mesh 的点/单元在输出 mesh 中的偏移映射。
    // 嵌套在类内，避免与上游 AppendReduceFilter 的同名全局 struct 重定义冲突。
    struct MeshPointMap {
        std::vector<igIndex> pointMap;
        igIndex cellOffset{0};
    };

    // 把任意 DataObject 归一为 UnstructuredMesh
    // 对 SurfaceMesh / VolumeMesh / UnstructuredMesh 直接转换；
    // 对 DrawObject / CompositeDataObject 递归收集子数据。
    void CollectMeshes(DataObject::Pointer obj,
                       std::vector<UnstructuredMesh::Pointer>& meshes);

    // 简单追加（不合并点），pointMap[i] = 输出点索引
    void AppendMeshSimple(UnstructuredMesh::Pointer src,
                          Points::Pointer outPoints,
                          CellArray::Pointer outCells,
                          UnsignedIntArray::Pointer outCellTypes,
                          igIndex& pointOffset,
                          std::vector<igIndex>& pointMap);

    // 合并重合点（哈希 + 距离校验），pointMap[i] = 输出点索引（可能复用已有点）
    void AppendMeshWithMerge(UnstructuredMesh::Pointer src,
                             Points::Pointer outPoints,
                             CellArray::Pointer outCells,
                             UnsignedIntArray::Pointer outCellTypes,
                             std::unordered_map<int64_t, std::vector<igIndex>>& pointHash,
                             std::vector<igIndex>& pointMap);

    // 合并属性：仅保留所有输入都具备的属性，按 pointMap / cellOffset 重映射
    void MergeAttributes(const std::vector<UnstructuredMesh::Pointer>& meshes,
                         const std::vector<MeshPointMap>& meshMaps,
                         UnstructuredMesh::Pointer outMesh);

    bool m_MergePoints;
    float m_Tolerance;
    std::string m_Message;
};

IGAME_NAMESPACE_END
#endif
