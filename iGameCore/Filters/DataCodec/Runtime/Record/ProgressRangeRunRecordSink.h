#ifndef DATACODEC_RUNTIME_RECORD_PROGRESSRANGERUNRECORDSINK_H
#define DATACODEC_RUNTIME_RECORD_PROGRESSRANGERUNRECORDSINK_H

#include "DataCodec/API/Adapter/IRunRecordSink.h"
#include "DataCodec/Common/DataCodecCallback.h"

#include <cstdint>
#include <string>
#include <utility>

namespace datacodec {

// 将嵌套任务进度映射到父任务区间
class ProgressRangeRunRecordSink final : public IRunRecordSink {
public:
    ProgressRangeRunRecordSink(
        IRunRecordSink* downstream,
        const double begin,
        const double end,
        const std::uint32_t frameOrdinal = 0u,
        const std::uint32_t frameCount = 0u,
        std::string textPrefix = {},
        const std::uint64_t parentRunId = 0u)
        : m_downstream(downstream),
          m_begin(callback::NormalizeProgress(begin)),
          m_end(callback::NormalizeProgress(end)),
          m_frameOrdinal(frameOrdinal),
          m_frameCount(frameCount),
          m_textPrefix(std::move(textPrefix)),
          m_parentRunId(parentRunId) {}

    [[nodiscard]] RunRecordMask Interests() const noexcept override {
        return m_downstream != nullptr ? m_downstream->Interests() : 0u;
    }

    [[nodiscard]] RunCollectionMask CollectionRequests() const noexcept override {
        return m_downstream != nullptr ? m_downstream->CollectionRequests() : 0u;
    }

    void Submit(const RunRecord& record) override {
        if (m_downstream == nullptr) {
            return;
        }
        const auto* progress = std::get_if<RunProgressRecord>(&record);
        if (progress == nullptr) {
            if (m_parentRunId != 0u) {
                if (const auto* begin = std::get_if<RunBeginRecord>(&record)) {
                    auto mapped = *begin;
                    if (mapped.run.parentRunId == 0u) {
                        mapped.run.parentRunId = m_parentRunId;
                    }
                    m_downstream->Submit(RunRecord{std::move(mapped)});
                    return;
                }
                if (const auto* end = std::get_if<RunEndRecord>(&record)) {
                    auto mapped = *end;
                    if (mapped.run.parentRunId == 0u) {
                        mapped.run.parentRunId = m_parentRunId;
                    }
                    m_downstream->Submit(RunRecord{std::move(mapped)});
                    return;
                }
            }
            m_downstream->Submit(record);
            return;
        }

        auto mapped = *progress;
        mapped.phase = RunProgressPhase::Update;
        mapped.normalized = m_begin +
            (m_end - m_begin) * callback::NormalizeProgress(progress->normalized);
        if (m_frameCount != 0u) {
            mapped.frameOrdinal = m_frameOrdinal;
            mapped.frameCount = m_frameCount;
        }
        if (!m_textPrefix.empty()) {
            mapped.text = mapped.text.empty()
                ? m_textPrefix
                : m_textPrefix +
                    (mapped.language == DataCodecLanguage::English ? ": " : "：") +
                    mapped.text;
        }
        m_downstream->Submit(RunRecord{std::move(mapped)});
    }

private:
    IRunRecordSink* m_downstream{nullptr};
    double m_begin{0.0};
    double m_end{1.0};
    std::uint32_t m_frameOrdinal{0u};
    std::uint32_t m_frameCount{0u};
    std::string m_textPrefix;
    std::uint64_t m_parentRunId{0u};
};

} // 命名空间 datacodec

#endif
