#ifndef iGameDataCodecScenarioSingleBlockMultiFrame_h
#define iGameDataCodecScenarioSingleBlockMultiFrame_h

#include <DataCodec/Filter/Test/Scenario/iGameDataCodecScenarioRunner.h>

#include <iostream>
namespace iGame::datacodec_test::scenario_single_block_multi_frame {
using namespace ::datacodec;
using namespace ::datacodec::test;

inline void PrintResult(const TestResult& result) {
    for (const auto& failure : result.failures) {
        std::cerr << failure.check << ": " << failure.message << '\n';
    }
    for (const auto& diagnostic : result.diagnostics) {
        std::cerr << diagnostic << '\n';
    }
}

} // namespace iGame::datacodec_test::scenario_single_block_multi_frame

namespace iGame::datacodec_test {
using namespace ::datacodec;
using namespace ::datacodec::test;

inline int RunDataCodecScenarioSingleBlockMultiFrame() {
    const auto result = RunDataCodecScenario(DataCodecScenarioKind::SingleBlockMultiFrame);
    scenario_single_block_multi_frame::PrintResult(result);
    if (!result.passed) {
        return 1;
    }
    std::cout << "DataCodec single-block multi-frame scenario passed\n";
    return 0;
}

} // namespace iGame::datacodec_test

#endif
