#include "iGameVolumeDistributor.h"

#include "iGameParallelContext.h"
#include <algorithm>

IGAME_NAMESPACE_BEGIN

void iGameVolumeDistributor::SetInput(DataObject::Pointer root) {
    m_Input = root;
    m_Pieces.clear();
    m_LocalIndices.clear();
    m_LocalBounds.reset();
    if (m_Input) { CollectPieces(m_Input); }
}

void iGameVolumeDistributor::CollectPieces(DataObject::Pointer obj) {
    if (!obj) { return; }

    if (obj->HasSubDataObject()) {
        for (auto it = obj->SubDataObjectIteratorBegin();
             it != obj->SubDataObjectIteratorEnd(); ++it) {
            CollectPieces(it->second);
        }
        return;
    }

    // 叶子分块：必须有几何（点集）才参与体绘制分配；纯容器/元数据对象跳过。
    if (obj->GetPoints() == nullptr) { return; }

    Piece piece;
    piece.object = obj;
    piece.originalIndex = static_cast<int>(m_Pieces.size());
    piece.bounds = obj->GetBoundingBox();
    m_Pieces.push_back(piece);
}

bool iGameVolumeDistributor::ComputeDistribution() {
    auto* ctx = ParallelContext::Instance().GetPointer();
    const int size = ctx->Size();
    const int rank = ctx->Rank();

    // 按左下角坐标确定性排序（x 优先，其次 y、z）。所有 rank 用同一输入树 + 同一
    // 比较器，因此得到完全一致的全局顺序，无需跨进程交换。
    std::sort(m_Pieces.begin(), m_Pieces.end(), [](const Piece& a, const Piece& b) {
        const Vector3d& amin = a.bounds.min;
        const Vector3d& bmin = b.bounds.min;
        if (amin[0] != bmin[0]) { return amin[0] < bmin[0]; }
        if (amin[1] != bmin[1]) { return amin[1] < bmin[1]; }
        return amin[2] < bmin[2];
    });

    const int n = static_cast<int>(m_Pieces.size());
    if (n < 1) { return false; }

    // 约束（对标 TestPVolumeRender）：进程数不能超过分块数，否则部分 rank 空跑，
    // 后续集合通信/合成会死锁。单进程 size==1 恒满足（-n 1 回退为整块）。
    if (size > n) { return false; }

    // 连续切块：把有序分块按数量切成 size 段，rank 取第 rank 段。
    const int base = n / size;
    const int rem = n % size;
    int start = 0;
    for (int r = 0; r < rank; ++r) {
        start += base + (r < rem ? 1 : 0);
    }
    const int count = base + (rank < rem ? 1 : 0);

    m_LocalIndices.clear();
    m_LocalBounds.reset();
    for (int i = start; i < start + count; ++i) {
        m_LocalIndices.push_back(i);
        m_LocalBounds.add(m_Pieces[i].bounds);
    }
    return true;
}

DataObject::Pointer iGameVolumeDistributor::GetPiece(int i) const {
    if (i < 0 || i >= static_cast<int>(m_Pieces.size())) { return nullptr; }
    return m_Pieces[i].object;
}

DataObject::Pointer iGameVolumeDistributor::GetLocalPiece(int i) const {
    if (i < 0 || i >= static_cast<int>(m_LocalIndices.size())) { return nullptr; }
    return m_Pieces[m_LocalIndices[i]].object;
}

DataObject::Pointer iGameVolumeDistributor::GetLocalComposite() const {
    DataObject::Pointer composite = DataObject::New();
    for (int idx : m_LocalIndices) {
        composite->AddSubDataObject(m_Pieces[idx].object);
    }
    return composite;
}

IGAME_NAMESPACE_END
