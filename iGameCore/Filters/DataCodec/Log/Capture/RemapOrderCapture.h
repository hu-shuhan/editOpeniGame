#ifndef DATACODEC_LOG_CAPTURE_REMAPORDERCAPTURE_H
#define DATACODEC_LOG_CAPTURE_REMAPORDERCAPTURE_H

#include "DataCodec/API/Adapter/IRunRecordSink.h"
#include "DataCodec/Codec/Remap/RemapProvider.h"

#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
namespace datacodec::log {

struct RemapOrderSnapshot {
    std::unordered_map<BlockPath, std::vector<IndexType>> pointOrders;
    std::unordered_map<BlockPath, std::vector<IndexType>> cellOrders;
};

class RemapOrderCapture final : public IRunRecordSink {
public:
    [[nodiscard]] RunRecordMask Interests() const noexcept override {
        return RunRecordBit(RunRecordKind::RemapOrder);
    }

    void Submit(const RunRecord& record) override {
        const auto* remap = std::get_if<RunRemapOrderRecord>(&record);
        if (remap == nullptr) {
            return;
        }
        auto order = ReadOrder(remap->provider);
        std::lock_guard<std::mutex> lock(m_mutex);
        auto& orders = remap->domain == RunRemapDomain::Point
            ? m_pointOrders
            : m_cellOrders;
        orders[remap->leafPath] = std::move(order);
    }

    [[nodiscard]] RemapOrderSnapshot TakeSnapshot() {
        std::lock_guard<std::mutex> lock(m_mutex);
        RemapOrderSnapshot snapshot;
        snapshot.pointOrders = std::move(m_pointOrders);
        snapshot.cellOrders = std::move(m_cellOrders);
        m_pointOrders.clear();
        m_cellOrders.clear();
        return snapshot;
    }

private:
    [[nodiscard]] static std::vector<IndexType> ReadOrder(const IRemapProvider* provider) {
        std::vector<IndexType> order;
        if (provider == nullptr || provider->IsIdentity()) {
            return order;
        }
        std::string error;
        if (!provider->ReadRange(0u, provider->Size(), order, &error)) {
            order.clear();
        }
        return order;
    }

    std::mutex m_mutex;
    std::unordered_map<BlockPath, std::vector<IndexType>> m_pointOrders;
    std::unordered_map<BlockPath, std::vector<IndexType>> m_cellOrders;
};

} // datacodec::log命名空间

#endif
