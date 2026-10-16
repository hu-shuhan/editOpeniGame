#ifndef iGameCleanToGridFilter_h
#define iGameCleanToGridFilter_h

#include "iGameFilter.h"
#include "iGamePointFinder.h"
#include "iGameUnstructuredMesh.h"

IGAME_NAMESPACE_BEGIN

class CleanToGridFilter : public Filter {
public:
    I_OBJECT(CleanToGridFilter);
    static Pointer New() { return new CleanToGridFilter; }

    CleanToGridFilter();
    ~CleanToGridFilter() override;

    bool Execute() override;

    // 参数设置函数
    void SetAbsoluteTolerance(double tol) { m_AbsoluteTolerance = tol; }
    double GetAbsoluteTolerance() const { return m_AbsoluteTolerance; }

    void SetToleranceIsAbsolute(bool isAbsolute) { m_ToleranceIsAbsolute = isAbsolute; }
    bool GetToleranceIsAbsolute() const { return m_ToleranceIsAbsolute; }

    void SetToleranceFraction(double frac) { m_ToleranceFraction = frac; }
    double GetToleranceFraction() const { return m_ToleranceFraction; }

    void SetMergePoints(bool merge) { m_MergePoints = merge; }
    bool GetMergePoints() const { return m_MergePoints; }

    void SetCompactPointFields(bool compact) { m_CompactPointFields = compact; }
    bool GetCompactPointFields() const { return m_CompactPointFields; }

    void SetRemoveDegenerateCells(bool remove) { m_RemoveDegenerateCells = remove; }
    bool GetRemoveDegenerateCells() const { return m_RemoveDegenerateCells; }

    // 公开给界面调用的"实际容差"查询
    double GetEffectiveTolerance(DataObject::Pointer input) { return ComputeEffectiveTolerance(input); }

    // 按单元类型判断拓扑退化
    static bool IsCellDegenerateWithIds(const igIndex* pointIds, int numPoints, IGenum cellType);

    // 代表点选择策略
    enum class RepresentativePolicy {
        FirstUsed, // 优先保留被单元引用的第一个点（默认，最安全）
        Average    // 平均（暂未实现，预留接口）
    };
    void SetRepresentativePolicy(RepresentativePolicy p) { m_RepPolicy = p; }
    RepresentativePolicy GetRepresentativePolicy() const { return m_RepPolicy; }

    // "移除未使用点"独立开关
    void SetRemoveUnusedPoints(bool v) {
        m_RemoveUnusedPoints = v;
        m_CompactPointFields = v;
    }
    bool GetRemoveUnusedPoints() const { return m_RemoveUnusedPoints; }

protected:
    double ComputeEffectiveTolerance(DataObject::Pointer input);

    // 合并点：返回时同时输出 newToRepOld（新索引 -> 最终代表点的旧索引）
    bool MergeCoincidentPointsBruteForce(Points::Pointer points, double tolerance, std::vector<igIndex>& oldToNewMap,
                                         std::vector<igIndex>& newToRepOld, igIndex& newPointCount,
                                         const std::vector<bool>& pointIsUsed);

    // 属性复制：从 newToRepOld 取代表点
    ArrayObject::Pointer CloneAttributeArray(ArrayObject::Pointer src, igIndex newSize,
                                             const std::vector<igIndex>& oldToNewMap,
                                             const std::vector<igIndex>& newToRepOld, igIndex numOldPoints);

    // 完整保留数组类型
    static ArrayObject::Pointer CreateArrayByTypeExact(IGenum dataType);

private:
    double m_AbsoluteTolerance = 0.001;
    double m_ToleranceFraction = 1e-6;
    bool m_ToleranceIsAbsolute = true;
    bool m_MergePoints = true;
    bool m_CompactPointFields = true;
    bool m_RemoveDegenerateCells = true;

    bool m_RemoveUnusedPoints = true;
    RepresentativePolicy m_RepPolicy = RepresentativePolicy::FirstUsed;
};

IGAME_NAMESPACE_END
#endif
