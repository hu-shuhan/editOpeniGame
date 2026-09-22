#ifndef iGameVolumeDistributor_h
#define iGameVolumeDistributor_h

#include "iGameBoundingBox.h"
#include "iGameDataObject.h"
#include "iGameObject.h"
#include <vector>

IGAME_NAMESPACE_BEGIN

/**
 * @class iGameVolumeDistributor
 * @brief 多分块空间分配器（阶段 2）。
 *
 * @details
 *  输入已读入的多块 DataObject（.pvd/.vtm/.igcm 读出的 CompositeDataObject / 带
 *  SubDataObjects 的容器），递归收集叶子分块（含点集的网格），按包围盒左下角做确定性
 *  排序后，把所有分块按数量连续切成 Size 段，每个 rank 取自己那段。
 *
 *  每个分块恰好归属一个 rank，因此各 rank 分区的并集完整且互不重叠（无越界/空洞）。
 *  当前版本使用「数量连续切块」，TestPVolumeRender 的「k-d 二分负载均衡」留作后续优化；
 *  该切块结果不影响后续按包围盒深度做有序合成（阶段 3）。
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

    void CollectPieces(DataObject::Pointer obj);

    std::vector<Piece> m_Pieces;      // 全局有序分块
    std::vector<int> m_LocalIndices;  // 本 rank 在 m_Pieces 中的下标
    BoundingBox m_LocalBounds;
    DataObject::Pointer m_Input{nullptr};
};

IGAME_NAMESPACE_END

#endif // iGameVolumeDistributor_h
