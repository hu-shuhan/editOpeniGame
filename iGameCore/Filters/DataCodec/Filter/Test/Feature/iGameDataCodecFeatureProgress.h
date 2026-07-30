#ifndef iGameDataCodecFeatureProgress_h
#define iGameDataCodecFeatureProgress_h

#include <DataCodec/API/Adapter/IRunRecordSink.h>
#include <DataCodec/Filter/Output/iGameDataCodecOutputSinks.h>
#include <DataCodec/Runtime/Record/ProgressRangeRunRecordSink.h>
#include <DataCodec/Test/Common/DataCodecTestResult.h>

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace iGame::datacodec_test::feature_progress {
using namespace ::datacodec;
using namespace ::datacodec::test;

class CapturingProgressSink final : public IRunRecordSink {
public:
    [[nodiscard]] RunRecordMask Interests() const noexcept override {
        return RunRecordBit(RunRecordKind::Progress);
    }

    void Submit(const RunRecord& record) override {
        const auto* progress = std::get_if<RunProgressRecord>(&record);
        if (progress != nullptr) {
            updates.push_back(*progress);
        }
    }

    std::vector<RunProgressRecord> updates;
};

inline bool NearlyEqual(const double left, const double right) {
    return std::abs(left - right) <= 1.0e-12;
}

inline void PrintResult(const TestResult& result) {
    for (const auto& failure : result.failures) {
        std::cerr << failure.check << ": " << failure.message << '\n';
    }
}

inline bool TestProgressRangeMapping() {
    TestResult result;
    CapturingProgressSink captured;
    ProgressRangeRunRecordSink reporter(&captured, 0.25, 0.50, 1u, 4u);

    reporter.Submit(RunRecord{RunProgressRecord{
        .phase = RunProgressPhase::Begin,
        .normalized = 0.0,
        .text = "prepare frame",
    }});
    reporter.Submit(RunRecord{RunProgressRecord{
        .phase = RunProgressPhase::Update,
        .normalized = 0.4,
        .text = "raw stage text",
    }});
    reporter.Submit(RunRecord{RunProgressRecord{
        .phase = RunProgressPhase::Finish,
        .normalized = 1.0,
        .text = "finish frame",
        .success = true,
    }});

    Require(result, captured.updates.size() == 3u,
            "progress.range.count", "range reporter should forward every nested update");
    if (captured.updates.size() == 3u) {
        const auto& begin = captured.updates[0];
        const auto& update = captured.updates[1];
        const auto& finish = captured.updates[2];
        Require(result, begin.phase == RunProgressPhase::Update,
                "progress.range.beginKind", "nested begin should become an update");
        Require(result, finish.phase == RunProgressPhase::Update,
                "progress.range.finishKind", "nested finish should become an update");
        Require(result, NearlyEqual(begin.normalized, 0.25),
                "progress.range.begin", "nested begin should map to the range start");
        Require(result, NearlyEqual(update.normalized, 0.35),
                "progress.range.update", "nested update should map within the parent range");
        Require(result, NearlyEqual(finish.normalized, 0.50),
                "progress.range.finish", "nested finish should map to the range end");
        Require(result, update.text == "raw stage text",
                "progress.range.text", "range mapping should preserve raw status text");
        Require(result, finish.success,
                "progress.range.success", "range mapping should preserve success state");
        for (const auto& forwarded : captured.updates) {
            Require(result, forwarded.frameOrdinal == 1u && forwarded.frameCount == 4u,
                    "progress.range.frame", "range mapping should tag the active frame");
        }
    }

    PrintResult(result);
    return result.passed;
}

inline bool TestProgressRangeWithoutFrameTag() {
    TestResult result;
    CapturingProgressSink captured;
    ProgressRangeRunRecordSink reporter(&captured, 0.0, 1.0);
    reporter.Submit(RunRecord{RunProgressRecord{
        .phase = RunProgressPhase::Update,
        .normalized = 0.5,
        .text = "preserve metadata",
        .frameOrdinal = 2u,
        .frameCount = 3u,
    }});

    Require(result, captured.updates.size() == 1u,
            "progress.range.untaggedCount", "untagged range should forward the update");
    if (!captured.updates.empty()) {
        Require(result,
                captured.updates.front().frameOrdinal == 2u && captured.updates.front().frameCount == 3u,
                "progress.range.untaggedFrame",
                "an untagged range should preserve upstream frame metadata");
    }

    PrintResult(result);
    return result.passed;
}

inline bool TestiGameProgressFrameText() {
    TestResult result;
    auto* observer = iGame::ProgressObserver::Instance();
    std::vector<std::string> texts;
    const auto textObserverTag = observer->AddObserver(
        iGame::Command::UpdateEvent,
        [&texts](iGame::Object*, unsigned long, void* data) {
            const auto* text = static_cast<const char*>(data);
            texts.emplace_back(text != nullptr ? text : "");
        });

    {
        iGame::iGameDataCodecProgressBarSink reporter;
        reporter.SubmitProgress(DataCodecProgressUpdate{
            .phase = DataCodecProgressPhase::Update,
            .normalized = 0.25,
            .frameOrdinal = 1u,
            .frameCount = 4u,
            .text = "raw stage text",
        });
    }
    Require(result, !texts.empty() && texts.back() == "第 2/4 帧：raw stage text",
            "progress.igame.multiFrameText", "multi-frame UI text should include frame and stage text");

    {
        iGame::iGameDataCodecProgressBarSink reporter;
        reporter.SubmitProgress(DataCodecProgressUpdate{
            .phase = DataCodecProgressPhase::Update,
            .normalized = 0.25,
            .frameOrdinal = 0u,
            .frameCount = 1u,
            .text = "single-frame stage",
        });
    }
    Require(result, !texts.empty() && texts.back() == "single-frame stage",
            "progress.igame.singleFrameText", "single-frame UI text should preserve the existing status text");

    observer->RemoveObserver(textObserverTag);
    PrintResult(result);
    return result.passed;
}

inline bool TestiGameProgressCallback() {
    TestResult result;
    std::vector<DataCodecProgressUpdate> updates;
    iGame::iGameDataCodecProgressBarSink reporter(
        iGame::iGameDataCodecProgressBarOutput{
            .updateProgressObserver = false,
            .callback = [&updates](const DataCodecProgressUpdate& progress) {
                updates.push_back(progress);
            },
        });
    reporter.SubmitProgress(DataCodecProgressUpdate{
        .phase = DataCodecProgressPhase::Update,
        .normalized = 0.5,
        .text = "callback status",
    });
    Require(result, updates.size() == 1u,
            "progress.igame.callbackCount", "progress callback count mismatch");
    if (!updates.empty()) {
        Require(result, updates.front().text == "callback status",
                "progress.igame.callbackText", "progress callback text mismatch");
    }

    PrintResult(result);
    return result.passed;
}

} // feature_progress命名空间

namespace iGame::datacodec_test {
using namespace ::datacodec;
using namespace ::datacodec::test;

inline int RunDataCodecFeatureProgress() {
    if (!feature_progress::TestProgressRangeMapping() ||
        !feature_progress::TestProgressRangeWithoutFrameTag() ||
        !feature_progress::TestiGameProgressFrameText() ||
        !feature_progress::TestiGameProgressCallback()) {
        return 1;
    }
    std::cout << "DataCodec progress feature tests passed\n";
    return 0;
}

} // datacodec::test命名空间

#endif
