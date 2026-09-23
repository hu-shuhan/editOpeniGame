#ifndef iGameVolumeDistributor_h
#define iGameVolumeDistributor_h

#include "iGameBoundingBox.h"
#include "iGameDataObject.h"
#include "iGameObject.h"
#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

/**
 * @class iGameVolumeDistributor
 * @brief 多分块空间分配器（阶段 2）。
 *
 * @details
 *  两条路径：
 *   - 内存级（SetInput + ComputeDistribution）：输入已读入的多块 DataObject，递归收集
 *     叶子分块，按包围盒左下角确定性排序后数量连续切块；
 *   - 文件级（ComputeFileDistribution，阶段 7 推荐）：只解析分块文件列表 + 并行扫包围盒，
 *     推导规则网格后做 k-d 二分，每个 rank 只读自己超块内的分块（不整读全量数据）。
 *
 *  每个分块恰好归属一个 rank，因此各 rank 分区的并集完整且互不重叠（无越界/空洞）。
 */
class iGameVolumeDistributor : public Object {
public:
    I_OBJECT(iGameVolumeDistributor);
    static Pointer New() { return new iGameVolumeDistributor; }

    /** 输入：多块根对象（可单块，也可 CompositeDataObject/SubDataObjects 容器）。 */
    void SetInput(DataObject::Pointer root);

    /**
     * 收集分块、确定性排序、按 rank 连续切块。
     * 返回 false 表示失败（无分块，或进程数超过分块数）。
     */
    bool ComputeDistribution();

    /**
     * 文件级分发（阶段 7，严格对齐 UnifiedVersion 的 DataDistribution::ComputeDistribution）：
     * 解析 .pvd（选 timestep）/ 目录（figure_N.*）/ 单文件 得到分块文件列表，各 rank 只按
     * part % Size == rank 读元数据求包围盒，AllReduce(SUM) 汇总成全局表，推导规则网格后做
     * k-d 二分（对齐 BisectGrid），每个 rank 只读自己超块内的分块文件。**不整读全量数据**，
     * 内存不再随 rank 数线性增长。分块不构成完整规则网格时退回数量连续切块。
     *
     * 返回 false 表示失败（无分块，或进程数超过分块数）。
     */
    bool ComputeFileDistribution(const std::string& inputPath, int timestep);

    /** 本 rank 分到的分块文件数（ComputeFileDistribution 后有效）。 */
    int GetNumberOfLocalPieceFiles() const {
        return static_cast<int>(m_LocalFiles.size());
    }
    /** 本 rank 分到的第 i 个分块文件的绝对路径。 */
    const std::string& GetLocalPieceFile(int i) const { return m_LocalFiles[i].file; }
    /** 本 rank 分到的第 i 个分块的 part 号。 */
    int GetLocalPiecePart(int i) const { return m_LocalFiles[i].part; }

    /** 空间超块：网格坐标空间里连续的一段 [ix0..ix1]×[iy0..iy1]×[iz0..iz1]。 */
    struct Block {
        int ix0{0}, ix1{0}, iy0{0}, iy1{0}, iz0{0}, iz1{0};
    };
    /** 本 rank 分到的空间超块（k-d 切分结果，网格坐标空间；用于诊断日志）。 */
    Block GetLocalBlock() const { return m_LocalBlock; }

    /** 全局分块数 N。 */
    int GetNumberOfPieces() const { return static_cast<int>(m_Pieces.size()); }
    /** 本 rank 分到的分块数。 */
    int GetNumberOfLocalPieces() const {
        return static_cast<int>(m_LocalIndices.size());
    }

    /** 全局第 i 个分块（分发前也可用，用于列出可选字段）。 */
    DataObject::Pointer GetPiece(int i) const;
    /** 本 rank 分到的第 i 个分块（原始 DataObject）。 */
    DataObject::Pointer GetLocalPiece(int i) const;
    /** 本 rank 所有分块打包成一个 DataObject（SubDataObjects 容器）。 */
    DataObject::Pointer GetLocalComposite() const;
    /** 本 rank 分块的并集包围盒（供合成排序 / 相机裁剪使用）。 */
    const BoundingBox& GetLocalBlockBounds() const { return m_LocalBounds; }

protected:
    iGameVolumeDistributor() = default;
    ~iGameVolumeDistributor() override = default;

    struct Piece {
        DataObject::Pointer object;
        BoundingBox bounds;
        int originalIndex{0};
    };

    // 文件级分块（阶段 7）：只用文件路径 + 扫描得到的包围盒，不持有整块数据。
    struct FilePiece {
        int part{0};
        std::string file;
        BoundingBox bounds;
        double cost{1.0};         // 体素开销（物理体积近似），k-d 负载均衡用
        int ix{-1}, iy{-1}, iz{-1}; // 在 nx×ny×nz 网格里的坐标（推导后）
    };

    void CollectPieces(DataObject::Pointer obj);

    std::vector<Piece> m_Pieces;      // 全局有序分块（内存级，SetInput 路径）
    std::vector<int> m_LocalIndices;  // 本 rank 在 m_Pieces 中的下标
    std::vector<FilePiece> m_LocalFiles; // 本 rank 分到的分块（文件级路径）
    Block m_LocalBlock;               // 本 rank 的空间超块（文件级路径）
    BoundingBox m_LocalBounds;
    DataObject::Pointer m_Input{nullptr};
};

IGAME_NAMESPACE_END

#endif // iGameVolumeDistributor_h
