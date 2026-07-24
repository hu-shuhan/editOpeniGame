#ifndef iGameDataCodecScenarioSingleBlockSingleFrame_h
#define iGameDataCodecScenarioSingleBlockSingleFrame_h

#include <DataCodec/Filter/Test/Scenario/iGameDataCodecScenarioRunner.h>

#include <array>
#include <iostream>
namespace iGame::datacodec_test::scenario_single_block_single_frame {
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

} // namespace iGame::datacodec_test::scenario_single_block_single_frame

namespace iGame::datacodec_test {
using namespace ::datacodec;
using namespace ::datacodec::test;

inline int RunDataCodecScenarioSingleBlockSingleFrame() {
    const std::array shapes{
        SyntheticDataShape::Surface,
        SyntheticDataShape::PointSet,
        SyntheticDataShape::Volume,
        SyntheticDataShape::Unstructured,
        SyntheticDataShape::Structured,
        SyntheticDataShape::Polyhedron,
    };

    for (const auto shape : shapes) {
        DataCodecScenarioOptions options;
        options.singleBlockShape = shape;
        const auto result = RunDataCodecScenario(
            DataCodecScenarioKind::SingleBlockSingleFrame,
            options);
        scenario_single_block_single_frame::PrintResult(result);
        if (!result.passed) {
            std::cerr << "DataCodec single-block single-frame scenario failed: "
                      << SyntheticDataShapeName(shape) << '\n';
            return 1;
        }
    }
    std::cout << "DataCodec single-block single-frame scenario passed\n";
    return 0;
}

} // namespace iGame::datacodec_test

#endif
