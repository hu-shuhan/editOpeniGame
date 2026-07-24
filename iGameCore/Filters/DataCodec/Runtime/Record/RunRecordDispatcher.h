#ifndef DATACODEC_RUNTIME_RECORD_RUNRECORDDISPATCHER_H
#define DATACODEC_RUNTIME_RECORD_RUNRECORDDISPATCHER_H

#include "DataCodec/API/Adapter/IRunRecordSink.h"

#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace datacodec {

class RunRecordDispatcher final : public IRunRecordSink {
public:
    void AddSink(IRunRecordSink* sink) {
        if (sink == nullptr) {
            return;
        }
        std::lock_guard<std::mutex> lock(m_mutex);
        m_sinks.push_back(SinkEntry{.sink = sink});
    }

    void AddSink(std::shared_ptr<IRunRecordSink> sink) {
        if (sink == nullptr) {
            return;
        }
        std::lock_guard<std::mutex> lock(m_mutex);
        auto* raw = sink.get();
        m_sinks.push_back(SinkEntry{.owner = std::move(sink), .sink = raw});
    }

    [[nodiscard]] RunRecordMask Interests() const noexcept override {
        std::lock_guard<std::mutex> lock(m_mutex);
        RunRecordMask interests = 0u;
        for (const auto& entry : m_sinks) {
            interests |= entry.sink->Interests();
        }
        return interests;
    }

    [[nodiscard]] RunCollectionMask CollectionRequests() const noexcept override {
        std::lock_guard<std::mutex> lock(m_mutex);
        RunCollectionMask requests = 0u;
        for (const auto& entry : m_sinks) {
            requests |= entry.sink->CollectionRequests();
        }
        return requests;
    }

    void Submit(const RunRecord& record) override {
        const auto kind = GetRunRecordKind(record);
        std::vector<SinkEntry> sinks;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            sinks.reserve(m_sinks.size());
            for (const auto& entry : m_sinks) {
                if (entry.sink->Wants(kind)) {
                    sinks.push_back(entry);
                }
            }
        }
        for (const auto& entry : sinks) {
            entry.sink->Submit(record);
        }
    }

private:
    struct SinkEntry {
        std::shared_ptr<IRunRecordSink> owner;
        IRunRecordSink* sink{nullptr};
    };

    mutable std::mutex m_mutex;
    std::vector<SinkEntry> m_sinks;
};

} // 命名空间 datacodec

#endif
