#ifndef iGameDataCodecScenarioMultiBlockSingleFrame_h
#define iGameDataCodecScenarioMultiBlockSingleFrame_h

#include <DataCodec/Filter/Test/Scenario/iGameDataCodecScenarioRunner.h>

#include <iostream>
namespace iGame::datacodec_test::scenario_multi_block_single_frame {
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

} // namespace iGame::datacodec_test::scenario_multi_block_single_frame

namespace iGame::datacodec_test {
using namespace ::datacodec;
using namespace ::datacodec::test;

inline int RunDataCodecScenarioMultiBlockSingleFrame() {
    const auto result = RunDataCodecScenario(DataCodecScenarioKind::MultiBlockSingleFrame);
    scenario_multi_block_single_frame::PrintResult(result);
    if (!result.passed) {
        return 1;
    }
    std::cout << "DataCodec multi-block single-frame scenario passed\n";
    return 0;
}

} // namespace iGame::datacodec_test

#endif
