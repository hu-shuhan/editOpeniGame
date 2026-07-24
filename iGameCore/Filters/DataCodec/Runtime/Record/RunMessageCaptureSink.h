#ifndef DATACODEC_RUNTIME_RECORD_RUNMESSAGECAPTURESINK_H
#define DATACODEC_RUNTIME_RECORD_RUNMESSAGECAPTURESINK_H

#include "DataCodec/API/Adapter/IRunRecordSink.h"

#include <mutex>
#include <utility>
#include <vector>

namespace datacodec {

// 收集调用结果需要返回的诊断消息
class RunMessageCaptureSink final : public IRunRecordSink {
public:
    [[nodiscard]] RunRecordMask Interests() const noexcept override {
        return RunRecordBit(RunRecordKind::Message);
    }

    void Submit(const RunRecord& record) override {
        const auto* message = std::get_if<RunMessageRecord>(&record);
        if (message == nullptr) {
            return;
        }
        std::lock_guard<std::mutex> lock(m_mutex);
        m_messages.push_back(message->message);
    }

    [[nodiscard]] std::vector<TelemetryMessageRecord> TakeMessages() {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto messages = std::move(m_messages);
        m_messages.clear();
        return messages;
    }

private:
    std::mutex m_mutex;
    std::vector<TelemetryMessageRecord> m_messages;
};

} // 命名空间 datacodec

#endif
