#ifndef iGameConnectivityFilter_h
#define iGameConnectivityFilter_h

#include "iGameArrayObject.h"
#include "iGameFilter.h"
#include "iGameSurfaceMesh.h"

#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

// 连通区域过滤器。
// 以「共享点」定义面之间的连通关系：只要两个面存在公共顶点即视为相邻。
// 支持 6 种抽取模式和两种输出语义：
//   1) 编号染色：保留全部面，给点/单元加上 RegionId 标量用于按区域着色；
//   2) 区域抽取：只输出被选中的连通区域（最大区域 / 指定编号 / 种子 / 最近点）。
// 目前仅支持 SurfaceMesh 输入，输出为一份独立的 SurfaceMesh。
class ConnectivityFilter : public Filter {
public:
    I_OBJECT(ConnectivityFilter);
    static Pointer New() { return new ConnectivityFilter; }

    // 与 VTK 对齐的 6 种抽取模式（数值与 VTK 一致）。
    enum ExtractionMode {
        POINT_SEEDED_REGIONS = 1,
        CELL_SEEDED_REGIONS = 2,
        SPECIFIED_REGIONS = 3,
        LARGEST_REGION = 4,
        ALL_REGIONS = 5,
        CLOSEST_POINT_REGION = 6,
    };

    // RegionId 重编号方式（与 VTK 对齐）。
    enum RegionIdAssignmentMode {
        UNSPECIFIED = 0,
        CELL_COUNT_ASCENDING = 1,
        CELL_COUNT_DESCENDING = 2,
    };

    // 输出点/单元标量数组名。
    static constexpr const char* RegionIdName = "RegionId";

    // 抽取模式。
    void SetExtractionMode(int mode) { m_ExtractionMode = mode; }
    int GetExtractionMode() const { return m_ExtractionMode; }

    // 是否给输出写入 RegionId 标量（默认开，对应 VTK ColorRegions）。
    void SetColorRegions(bool v) { m_ColorRegions = v; }
    bool GetColorRegions() const { return m_ColorRegions; }

    // 按单元数对区域重新编号（默认 UNSPECIFIED，保持遍历顺序）。
    void SetRegionIdAssignmentMode(int mode) { m_RegionIdAssignmentMode = mode; }
    int GetRegionIdAssignmentMode() const { return m_RegionIdAssignmentMode; }

    // 标量连通：单元除几何相邻外，还要求其顶点标量落在指定区间内才算连通。
    void SetScalarConnectivity(bool v) { m_ScalarConnectivity = v; }
    bool GetScalarConnectivity() const { return m_ScalarConnectivity; }
    // 全标量连通：开启时要求单元「所有」顶点都在区间内，否则任一顶点命中即可。
    void SetFullScalarConnectivity(bool v) { m_FullScalarConnectivity = v; }
    bool GetFullScalarConnectivity() const { return m_FullScalarConnectivity; }
    void SetScalarRange(double lo, double hi) { m_ScalarRange[0] = lo; m_ScalarRange[1] = hi; }
    double GetScalarRangeMin() const { return m_ScalarRange[0]; }
    double GetScalarRangeMax() const { return m_ScalarRange[1]; }
    // 用于标量连通的点属性数组名；为空时自动选取当前属性（或首个点标量数组）。
    void SetScalarArrayName(const std::string& name) { m_ScalarArrayName = name; }

    // 种子（POINT_SEEDED 为点 id，CELL_SEEDED 为单元 id）。
    void InitializeSeedList() { m_Seeds.clear(); }
    void AddSeed(igIndex id) { m_Seeds.push_back(id); }

    // 指定区域编号（SPECIFIED_REGIONS）：使用当前 RegionIdAssignmentMode 排序后的编号。
    void InitializeSpecifiedRegionList() { m_SpecifiedRegionIds.clear(); }
    void AddSpecifiedRegion(int id) { m_SpecifiedRegionIds.push_back(id); }

    // 最近点（CLOSEST_POINT_REGION）。
    void SetClosestPoint(double x, double y, double z) {
        m_ClosestPoint[0] = x; m_ClosestPoint[1] = y; m_ClosestPoint[2] = z;
    }

    // 抽取到的区域个数 / 各区域单元数。
    int GetNumberOfExtractedRegions() const { return static_cast<int>(m_RegionSizes.size()); }
    const std::vector<igIndex>& GetRegionSizes() const { return m_RegionSizes; }

    bool Execute() override;

    std::string GetMessage() const { return m_Message; }

protected:
    ConnectivityFilter();
    ~ConnectivityFilter() override = default;

private:
    // 构建点→面邻接（CSR 风格的 vector，不修改输入）。
    void BuildPointFaceAdjacency();
    // 按共享点扩散，入队时标记面，每个合格点的邻接只展开一次；返回标记的单元数。
    IGsize TraverseAndMark(const std::vector<igIndex>& seedCells, int regionNumber);
    // 几何相邻之外的可选标量连通判定。
    bool CellIsConnected(igIndex cellId) const;
    // 选择标量连通使用的点属性数组；返回是否成功。
    bool ResolveScalarArray();

    int m_ExtractionMode{LARGEST_REGION};
    bool m_ColorRegions{true};
    int m_RegionIdAssignmentMode{UNSPECIFIED};

    bool m_ScalarConnectivity{false};
    bool m_FullScalarConnectivity{false};
    double m_ScalarRange[2]{0.0, 0.0};
    std::string m_ScalarArrayName{""};

    std::vector<igIndex> m_Seeds;
    std::vector<int> m_SpecifiedRegionIds;
    double m_ClosestPoint[3]{0.0, 0.0, 0.0};

    std::string m_Message;

    // 执行期数据
    SurfaceMesh::Pointer m_Mesh{nullptr};
    std::vector<std::vector<igIndex>> m_PointFaces; // 点 -> 相邻面
    std::vector<int> m_Visited;                     // 面 -> 区域号（-1 未访问）
    std::vector<char> m_ExpandedPoints;             // 点邻接是否已展开
    std::vector<char> m_CellEligible;               // 每个面是否满足标量连通条件
    std::vector<igIndex> m_RegionSizes;             // 各区域单元数
    ArrayObject::Pointer m_ScalarArray{nullptr};    // 标量连通用的点属性
};

IGAME_NAMESPACE_END
#endif
