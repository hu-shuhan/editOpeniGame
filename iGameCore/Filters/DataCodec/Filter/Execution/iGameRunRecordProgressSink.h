#ifndef iGameDataCodeciGameRunRecordProgressSink_h
#define iGameDataCodeciGameRunRecordProgressSink_h

#include "DataCodec/API/Adapter/IRunRecordSink.h"
#include "DataCodec/Common/DataCodecCallback.h"

#include "iGameProgressObserver.h"

#include <algorithm>
#include <cmath>
#include <mutex>
#include <string>

IGAME_NAMESPACE_BEGIN

class iGameRunRecordProgressSink final : public ::datacodec::IRunRecordSink {
public:
    [[nodiscard]] ::datacodec::RunRecordMask Interests() const noexcept override {
        return ::datacodec::RunRecordBit(::datacodec::RunRecordKind::Progress);
    }

    void Submit(const ::datacodec::RunRecord& record) override {
        const auto* progress = std::get_if<::datacodec::RunProgressRecord>(&record);
        if (progress == nullptr) {
            return;
        }

        std::scoped_lock lock(GlobalSubmitMutex(), m_mutex);
        auto* observer = ProgressObserver::Instance();
        if (observer == nullptr) {
            return;
        }

        double normalized = ::datacodec::callback::NormalizeProgress(progress->normalized);
        if (progress->phase == ::datacodec::RunProgressPhase::Begin) {
            m_lastNormalized = 0.0;
            normalized = 0.0;
        } else if (progress->phase == ::datacodec::RunProgressPhase::Finish) {
            m_lastNormalized = progress->success ? 1.0 : 0.0;
            normalized = m_lastNormalized;
        } else if (normalized < m_lastNormalized) {
            return;
        } else {
            // 嵌套阶段完成仍属于处理中，100%只由最外层结束记录发出
            normalized = std::min(normalized, std::nextafter(1.0, 0.0));
            m_lastNormalized = std::max(m_lastNormalized, normalized);
            normalized = m_lastNormalized;
        }

        if (progress->frameCount > 1u) {
            const auto displayOrdinal = progress->frameOrdinal < progress->frameCount
                ? progress->frameOrdinal + 1u
                : progress->frameCount;
            observer->UpdateText(
                std::to_string(displayOrdinal) + "/" +
                std::to_string(progress->frameCount) + "帧");
        } else {
            observer->UpdateText(progress->text);
        }
        observer->UpdateProgress(normalized);
    }

private:
    [[nodiscard]] static std::mutex& GlobalSubmitMutex() {
        static std::mutex mutex;
        return mutex;
    }

    std::mutex m_mutex;
    double m_lastNormalized{0.0};
};

IGAME_NAMESPACE_END

#endif
